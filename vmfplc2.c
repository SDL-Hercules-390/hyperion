/* VMFPLC2.C  (C) Copyright Ivan S. Warren 2010-2012                 */
/*            Utility to create VMFPLC2 formatted tapes              */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#include "hercules.h"
#include "tapedev.h"
#include "hetlib.h"
#include "ftlib.h"
#include "sllib.h"
#include "herc_getopt.h"

#define UTILITY_NAME    "vmfplc2"

/*-------------------------------------------------------------------*/
/* Maximum sized tape I/O buffers...                                 */
/*-------------------------------------------------------------------*/
static BYTE bfr[ MAX_BLKLEN ];
static BYTE bfr2[ MAX_BLKLEN ];

/* The blocksize of 4000 is the block size expected by VMFPLC2 */
#define TAPE_BLOCKSIZE          4000
/* The last data record must have a modulo of 800 */
#define TAPE_BLKSIZE_MODULO     800
/* NOTE : The tape blocks will always be 5 more to account */
/*        for record headers                               */


/* The File Status Table (as expected by VMFPLC2) */
/* The "Basic" version is the FST for VM/370      */
/* while the EDF extention is for support of VM   */
/* version starting with BSEPP.                   */

struct FST
{
/* Basic (short) FST */
    unsigned char fn[8];    /* File name */
    unsigned char ft[8];    /* File Type */
    unsigned char dt[4];    /* date : MMDDHHmm (In DCB) */
    unsigned char wp[2];    /* Write ptr */
    unsigned char rp[2];    /* Read ptr */
    unsigned char fm[2];    /* File Mode */
    unsigned char reccount[2];      /* 16 bit record count */
    unsigned char fcl[2];   /* First chain link */
    unsigned char recfm;    /* Record fmt (F or V) */
    unsigned char flag1;    /* Flags */
    unsigned char lrecl[4]; /* 32 bit record length */
    unsigned char db[2];    /* Data Block Count */
    unsigned char year[2];  /* Year (EBCDIC) */
/* The following 2 fields are VMFPLC2 specific */
    unsigned char lastsz[4]; /* Number of 800 byte blocks in last record */
    unsigned char numblk[4]; /* Count of complete 4000 byte records */
/* Extended EDF FST */
    unsigned char afop[4];  /* Alt File Origin Ptr */
    unsigned char adbc[4];  /* Alt Data Block Count */
    unsigned char aic[4];   /* Alt Item Count */
    unsigned char levels;   /* Levels of indirection */
    unsigned char ptrsz;    /* Size of Pointers */
    unsigned char adt[6];   /* Alt Date (YYMMDDHHMMSS) (In DCB) */
    unsigned char extra[4]; /* Reserved */
/* Global length MUST be 72 */
};

/* The FST tape record (with VMFPLC2 header) */
struct FST_BLOCK
{
    unsigned char hdr[5];
    struct FST fst;
};

/* This structure contains processing options */
/* and some runtime status information        */

struct options
{
    char    *cmd;
    char    *procfile;
    char    *tapefile;
#define VMFPLC2DUMP     0
#define VMFPLC2LOAD     1
#define VMFPLC2SCAN     2
    char    verb;
#define OPT_FLAG_EDF    0x01
    unsigned int flags;
    HETB    *hetfd;
};

/* Utility function to perform ascii to ebcdic translation on a string */
void host_to_guest_str(unsigned char *d,char *s)
{
    int i;

    for(i=0;s[i];i++)
    {
        d[i]=host_to_guest(s[i]);
    }
}

static char* pgm;

/* Parse the command line */
int parse_parms(int ac, char **av, struct options *opts)
{
    opts->cmd=basename(av[0]);
    if(ac<2)
    {
        WRMSG( HHC02620, "I", pgm ); // "Usage: %s ...
        return 1;
    }
    do
    {
        if(strcasecmp(av[1],"DUMP")==0)
        {
            opts->verb=VMFPLC2DUMP;
            break;
        }
        if(strcasecmp(av[1],"SCAN")==0)
        {
            opts->verb=VMFPLC2SCAN;
            break;
        }
        if(strcasecmp(av[1],"LOAD")==0)
        {
            opts->verb=VMFPLC2LOAD;
            break;
        }
        // "Invalid function %s"
        FWRMSG( stderr, HHC02621, "E", av[1] );
        WRMSG( HHC02620, "I", pgm ); // "Usage: %s ...
        return 1;
    } while(0);
    switch(opts->verb)
    {
        case VMFPLC2SCAN:
        case VMFPLC2LOAD:
        if(ac<3)
        {
            // "%s not specified"
            FWRMSG( stderr, HHC02622, "E", "tape file" );
            WRMSG( HHC02620, "I", pgm ); // "Usage: %s ...
            return 1;
        }
        opts->tapefile=av[2];
        break;
        case VMFPLC2DUMP:
        if(ac<3)
        {
            // "%s not specified"
            FWRMSG( stderr, HHC02622, "E", "file list" );
            WRMSG( HHC02620, "I", pgm ); // "Usage: %s ...
            return 1;
        }
        if(ac<4)
        {
            // "%s not specified"
            FWRMSG( stderr, HHC02622, "E", "tape file" );
            WRMSG( HHC02620, "I", pgm ); // "Usage: %s ...
            return 1;
        }
        opts->procfile=av[2];
        opts->tapefile=av[3];
        break;
    }
    return 0;
}

/* Valid character for CMS file names and file types */
static  char    *validchars="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789$#@+-:_";
/* Valid CMS file mode numbers */
static  char    *validfmnum="0123456";


/* This function validates a CMS File Name or File Type */
int validate_fnft(char *s)
{
    int i,j,found;

    for(i=0;s[i];i++)
    {
        for(found=0,j=0;validchars[j];j++)
        {
            if(s[i]==validchars[j])
            {
                found=1;
                break;
            }
        }
        if(!found) return i+1;
    }
    return 0;
}

/* Validate a CMS FN FT FM construct */
char *validate_cmsfile(char *fn,char *ft,char *fm)
{
    int i,found;
    static char msg[1024];

    if(strlen(fn)>8)
    {
        return "CMS File name too long";
    }
    if(strlen(ft)>8)
    {
        return "CMS File type too long";
    }
    if(strlen(fm)>2)
    {
        return "CMS File mode too long";
    }
    if((i=validate_fnft(fn))!=0)
    {
        // "Invalid character in CMS file %s at position %d"
        MSGBUF( msg, MSG( HHC02628, "E", "name", i ));
        return msg;
    }
    if((i=validate_fnft(ft))!=0)
    {
        // "Invalid character in CMS file %s at position %d"
        MSGBUF( msg, MSG( HHC02628, "E", "type", i ));
        return msg;
    }
    if(!isalpha(fm[0]))
    {
        return "CMS File mode must start with a letter";
    }
    if(strlen(fm)>1)
    {
        found=0;
        for(i=0;validfmnum[i];i++)
        {
            if(fm[1]==validfmnum[i])
            {
                found=1;
                break;
            }
        }
        if(!found)
        {
            return "CMS File mode number must be 1-6";
        }
    }
    return NULL;
}

/* Utility routine to translate a C string to upper case */
void str_toupper(char *s)
{
    int i;

    for(i=0;s[i];i++)
    {
        if(islower(s[i])) s[i]=toupper(s[i]);
    }
}

/*****************************************************/
/* Tape management structures and functions          */
/*****************************************************/

/* the TAPE_BLOCK structure represents a single tape block */
struct TAPE_BLOCK
{
    unsigned char   *data;
    size_t          sz;
    struct TAPE_BLOCK *next;
};

/* The TAPE_BLOCKS structure represents a collection of tape blocks */
struct TAPE_BLOCKS
{
    unsigned char *hdr;     /* Header to append to every record */
    size_t  hdrsz;          /* Size of header */
    size_t  blksz;          /* Maximum block size */
    size_t  blk_modulo;     /* Must be a multiple of this */
    struct TAPE_BLOCK *first;       /* First block in chain */
    struct TAPE_BLOCK *current;     /* Working block */
};

/* Write a single tape mark */
int write_tape_mark(struct options *opts)
{
    het_tapemark(opts->hetfd);
    return 0;
}

/* Write a block of data to tape */
int write_tape_block_data(struct options *opts,unsigned char *bfr,int sz)
{
    het_write(opts->hetfd,bfr,sz);
    return 0;
}

/* Write a TAPE_BLOCK and adjust the block size to a modulo (without counting the header */
int write_tape_block(struct options *opts,struct TAPE_BLOCK *tb,size_t mod,size_t hdrsz)
{
    size_t      sz;

    sz=(tb->sz-hdrsz)/mod;
    if((tb->sz-hdrsz)%mod)
    {
        sz++;
    }
    sz*=mod;
    sz+=hdrsz;
    return write_tape_block_data(opts,tb->data,(int)sz);
}

/* Write a collection of blocks to tape */
int write_tape_blocks(struct options *opts,struct TAPE_BLOCKS *tbs)
{
    int     rc = 0;
    struct TAPE_BLOCK *tb;

    tb=tbs->first;
    while(tb)
    {
        rc=write_tape_block(opts,tb,tbs->blk_modulo,tbs->hdrsz);
        if(rc!=0) break;
        tb=tb->next;
    }
    return rc;
}

/* Initialize a collection of tape blocks */
struct TAPE_BLOCKS *init_blocks(size_t blksz,size_t modulo,unsigned char *hdr,int hdrsz)
{
    struct TAPE_BLOCKS *tbs;

    tbs=malloc(sizeof(struct TAPE_BLOCKS));
    tbs->first=NULL;
    tbs->current=NULL;
    tbs->hdr=hdr;
    tbs->hdrsz=hdrsz;
    tbs->blksz=blksz;
    tbs->blk_modulo=modulo;
    return tbs;
}

/* Add a chunk of data to a collection of tape blocks */
void append_data(struct TAPE_BLOCKS *tbs,unsigned char *bfr,size_t sz)
{
    int dsz;
    int of;
    struct TAPE_BLOCK *tb;

    of=0;
    while(sz)
    {
        if(tbs->current==NULL || tbs->current->sz==(tbs->blksz+tbs->hdrsz))
        {
            tb=malloc(sizeof(struct TAPE_BLOCK));
            tb->next=NULL;
            tb->sz=0;
            if(tbs->first==NULL)
            {
                tbs->first=tb;
                tbs->current=tb;
            }
            else
            {
                tbs->current->next=tb;
                tbs->current=tb;
            }
            tb->data=malloc(tbs->blksz+tbs->hdrsz);
            memset(tb->data, 0, tbs->blksz+tbs->hdrsz);
            memcpy(tb->data,tbs->hdr,tbs->hdrsz);
            tb->sz=tbs->hdrsz;
        }
        else
        {
            tb=tbs->current;
        }
        dsz=(int)MIN(sz,(tbs->blksz+tbs->hdrsz)-tb->sz);
        memcpy(&tb->data[tb->sz],&bfr[of],dsz);
        tb->sz+=dsz;
        sz-=dsz;
        of+=dsz;
    }
}

/* Release a collection of tape blocks */
void free_blocks(struct TAPE_BLOCKS *tbs)
{
    struct TAPE_BLOCK *tb,*ntb;

    tb=tbs->first;
    while(tb)
    {
        free(tb->data);
        ntb=tb->next;
        free(tb);
        tb=ntb;
    }
    free(tbs);
}

/*****************************************************/
/* File records management structures & functions    */
/*****************************************************/

/* The RECS strucure represents a file with its */
/* collection of tape blocks                    */
struct RECS
{
    size_t  bc;
    size_t  bts;
    struct  TAPE_BLOCKS *blocks;
    char    recfm;
    int     reclen;
    int     reccount;
    size_t  filesz;
};


/* Initialize a new RECS structure */
struct RECS *initrecs(char recfm,int recl,unsigned char *hdr,int hdrsz)
{
    struct RECS *recs;

    recs=malloc(sizeof(struct RECS));
    recs->blocks=init_blocks(TAPE_BLOCKSIZE,TAPE_BLKSIZE_MODULO,hdr,hdrsz);
    recs->reccount=0;
    recs->recfm=recfm;
    recs->reclen=recl;
    recs->filesz=0;
    return recs;
}

/* Add 1 (recfm V) or multiple (recfm F) records to a RECS structure */
/* and to the related tape block collection ù                        */
void addrecs(struct RECS *recs,unsigned char *bfr,int sz)
{
    unsigned char recd[2];

    recs->filesz+=sz;
    switch(recs->recfm)
    {
        case 'V':
            recd[1]=sz&0xff;
            recd[0]=(sz>>8)&0xff;
            append_data(recs->blocks,recd,2);
            append_data(recs->blocks,bfr,sz);
            recs->reccount++;
            recs->reclen=MAX(recs->reclen,sz);
            break;
        case 'F':
            append_data(recs->blocks,bfr,sz);
            break;
    }
}

/* Close a RECS structure. The RECS structue is released and the collection */
/* of tape block is returned for subsequent writing                         */
/* For empty files, a dummy record is created since CMS does not support    */
/* empty files.                                                             */
/* The last record of a RECFM F file is also padded if necessary            */

struct TAPE_BLOCKS *flushrecs( struct RECS *recs,
                               int *recl,
                               int *recc,
                               size_t *filesz,
                               unsigned char pad )
{
    struct  TAPE_BLOCKS *blks;
    int     residual;

    if(recs->filesz==0)
    {
        append_data(recs->blocks,&pad,1);
        recs->filesz=1;
        if(recs->recfm=='V')
        {
            recs->reccount=1;
            recs->reclen=1;
        }
    }
    if(recs->recfm=='F')
    {
        residual=recs->reclen-recs->filesz%recs->reclen;
        residual=residual%recs->reclen;
        if(residual)
        {
            unsigned char *padbfr;
            recs->filesz+=residual;
            padbfr=malloc(residual);
            memset(padbfr,pad,residual);
            append_data(recs->blocks,padbfr,residual);
            free(padbfr);

        }
        recs->reccount=(int)recs->filesz/recs->reclen;
    }
    *recc=recs->reccount;
    *recl=recs->reclen;
    *filesz=recs->filesz;
    blks=recs->blocks;
    free(recs);
    return blks;
}

/* PLCD (VMFPLC2 Data block) record header */
static unsigned char plcd_hdr[5]={0x02,0xd7,0xd3,0xc3,0xc4};
/* PLCH (VMFPLC2 Header block) record header */
static unsigned char plch_hdr[5]={0x02,0xd7,0xd3,0xc3,0xc8};

/* Load a binary file for DUMP function */
struct TAPE_BLOCKS *load_binary_file(char *infile,char recfm,int *recl,int *recc,size_t *filesz)
{
    int     rsz;
    FILE    *ifile;
    int     maxsize;
    struct  RECS *recs;

    if(recfm=='V')
        maxsize=MAX_BLKLEN-1;
    else
        maxsize=MAX_BLKLEN;

    ifile=fopen(infile,"r");
    if(ifile==NULL)
    {
        perror(infile);
        return NULL;
    }

    recs=initrecs(recfm,*recl,plcd_hdr,5);
    while((rsz=(int)fread(bfr,1,maxsize,ifile))!=0)
    {
        addrecs(recs,bfr,rsz);
    }
    fclose(ifile);
    return flushrecs(recs,recl,recc,filesz,0x40);
}

/* Load a textual file for DUMP function */
struct TAPE_BLOCKS *load_text_file(char *infile,char recfm,int *recl,int *recc,size_t *filesz)
{
    int     rsz;
    FILE    *ifile;
    struct  RECS *recs;
    char    *rec;

    ifile=fopen(infile,"r");
    if(ifile==NULL)
    {
        perror(infile);
        return NULL;
    }

    recs=initrecs(recfm,*recl,plcd_hdr,5);
    while((rec=fgets((char*)bfr,sizeof(bfr),ifile))!=NULL)
    {
        rsz=(int)strlen(rec)-1;
        if(recfm=='F')
        {
            if(rsz<(*recl))
            {
                memset(&rec[rsz],' ',(*recl)-rsz);
            }
            rsz=(*recl);
        }
        rec[rsz]=0;
        host_to_guest_str(bfr2,rec);
        addrecs(recs,bfr2,rsz);
    }
    fclose(ifile);
    return flushrecs(recs,recl,recc,filesz,0x40);
}

/* Load a structured file for DUMP function */
struct TAPE_BLOCKS *load_structured_file(char *infile,char recfm,int *recl,int *recc,size_t *filesz)
{
    int     rsz,rc;
    FILE    *ifile;
    uint16_t    rlbfr;
    struct  RECS *recs;

    ifile=fopen(infile,"r");
    if(ifile==NULL)
    {
        perror(infile);
        return NULL;
    }
    if(recfm!='V')
    {
        // "%s not supported (yet)"
        FWRMSG( stderr, HHC02623, "E", "Structured input files for output to RECFM F files" );
        return NULL; /* Structured files only for RECFM V for the time being */
    }
    recs=initrecs(recfm,*recl,plcd_hdr,5);
    while(((int)fread(&rlbfr,1,2,ifile))==2)
    {
        rsz=bswap_16(rlbfr);
        rc=(int)fread(bfr,1,rsz,ifile);
        if(rc!=rsz)
        {
            // "Expected %d bytes from file %s, but only %d file read"
            FWRMSG( stderr, HHC02629, "E", rsz, infile, rc );
            return NULL;
        }
        addrecs(recs,bfr,rsz);
    }
    fclose(ifile);
    return flushrecs(recs,recl,recc,filesz,0x40);
}

/* Load file for DUMP function */
struct TAPE_BLOCKS *load_file(char *infile,char *filefmt,char recfm,int *recl,int *recc,size_t *filesz)
{
    struct TAPE_BLOCKS *blks;
    switch(filefmt[0])
    {
        case 'B':
            blks=load_binary_file(infile,recfm,recl,recc,filesz);
            break;
        case 'T':
            blks=load_text_file(infile,recfm,recl,recc,filesz);
            break;
        case 'S':
            blks=load_structured_file(infile,recfm,recl,recc,filesz);
            break;
        default:
            // "INTERNAL ERROR %s"
            FWRMSG( stderr, HHC02634, "E", "0001" );
            return NULL;
    }
    return blks;
}


/* Convert a value to 16 bit Big Endian format */
void big_endian_16(unsigned char *d,int s)
{
    d[0]=(s>>8) & 0xff;
    d[1]=s&0xff;
}

/* Convert a value to 32 bit Big Endian format */
void big_endian_32(unsigned char *d,int s)
{
    d[0]=(s>>24) & 0xff;
    d[1]=(s>>16) & 0xff;
    d[2]=(s>>8) & 0xff;
    d[3]=s&0xff;
}

/* Convert a single byte decimal value to DCB (Decimal Coded Binary) format */
unsigned char to_dcb(int v)
{
    return((v/10)*16+v%10);
}

/* Create the VMFPLC2 Header record */
struct FST_BLOCK *format_fst(char *fn,char *ft,char *fm,char recfm,int lrecl,int reccount,int filesz,time_t dt)
{
    int     numfull;
    int     lastsz;
    char    bfr[3];
    struct  tm *ttm;
    struct  FST_BLOCK *fstb;

    fstb=malloc(sizeof(struct FST_BLOCK));
    memset(fstb, 0,sizeof(struct FST_BLOCK));
    memcpy(fstb->hdr,plch_hdr,5);

    memset(fstb->fst.fn,0x40,8);
    memset(fstb->fst.ft,0x40,8);
    memset(fstb->fst.fm,0x40,2);

    host_to_guest_str(fstb->fst.fn,fn);
    host_to_guest_str(fstb->fst.ft,ft);
    host_to_guest_str(fstb->fst.fm,fm);

    ttm=localtime(&dt);
    fstb->fst.dt[0]=to_dcb(ttm->tm_mon);
    fstb->fst.dt[1]=to_dcb(ttm->tm_mday);
    fstb->fst.dt[2]=to_dcb(ttm->tm_hour);
    fstb->fst.dt[3]=to_dcb(ttm->tm_min);
    MSGBUF( bfr, "%2.2u", ttm->tm_year % 100 );
    host_to_guest_str(fstb->fst.year,bfr);

    fstb->fst.adt[0]=to_dcb(ttm->tm_year%100);
    fstb->fst.adt[1]=to_dcb(ttm->tm_mon);
    fstb->fst.adt[2]=to_dcb(ttm->tm_mday);
    fstb->fst.adt[3]=to_dcb(ttm->tm_hour);
    fstb->fst.adt[4]=to_dcb(ttm->tm_min);
    fstb->fst.adt[5]=to_dcb(ttm->tm_sec);

    fstb->fst.recfm=host_to_guest(recfm);

    big_endian_32(fstb->fst.lrecl,lrecl);
    big_endian_16(fstb->fst.reccount,reccount);
    big_endian_16(fstb->fst.wp,reccount+1);
    big_endian_16(fstb->fst.rp,1);
    big_endian_32(fstb->fst.aic,reccount);

    if(recfm=='V')
    {
        filesz+=reccount*2;
    }
    numfull=filesz/TAPE_BLOCKSIZE;
    lastsz=filesz%TAPE_BLOCKSIZE;
    if(lastsz%TAPE_BLKSIZE_MODULO==0)
    {
        lastsz/=TAPE_BLKSIZE_MODULO;
    }
    else
    {
        lastsz=lastsz/TAPE_BLKSIZE_MODULO+1;
    }

    big_endian_32(fstb->fst.numblk,numfull);
    big_endian_32(fstb->fst.lastsz,lastsz);


    return  fstb;
}

/* Process a control file entry */
int process_entry(struct options *opts,char *orec,int recno)
{
    int     ignore;
    int     badentry;
    char    *fn,*ft,*fm;
    char    *recfm,*lrecl;
    char    *filefmt;
    char    *infile;
    int     reclen;
    char    *msg;
    char    *rec;
    char    *endptr;
    int     reccount;
    size_t  filesz;
    struct  TAPE_BLOCKS *blks;
    struct  FST_BLOCK *fstb;
    struct  stat stt;
    char   *strtok_str = NULL;

    ignore=0;
    rec=strdup(orec);
    do
    {
        badentry=1;
        fn=strtok_r(rec," ",&strtok_str);
        if(fn==NULL)
        {
            /* blank line : ignore */
            ignore=1;
            break;
        }
        if(strcmp(fn,"*")==0)
        {
            /* comment : ignore */
            ignore=1;
            break;
        }
        if(fn[0]=='@')
        {
            if(strcasecmp(fn,"@TM")==0)
            {
                write_tape_mark(opts);
                ignore=1;
                break;
            }
        }
        ft=strtok_r(NULL," ",&strtok_str);
        if(ft==NULL)
        {
            msg="File type missing";
            break;
        }
        fm=strtok_r(NULL," ",&strtok_str);
        if(fm==NULL)
        {
            msg="File mode missing";
            break;
        }
        str_toupper(fn);
        str_toupper(ft);
        str_toupper(fm);
        msg=validate_cmsfile(fn,ft,fm);
        if(msg!=NULL) break;
        recfm=strtok_r(NULL," ",&strtok_str);
        if(recfm==NULL)
        {
            msg="Record format missing";
            break;
        }
        str_toupper(recfm);
        if(strlen(recfm)>1 || ( recfm[0]!='F' && recfm[0]!='V'))
        {
            msg="Record format must be F or V";
            break;
        }
        if(recfm[0]=='F')
        {
            lrecl=strtok_r(NULL," ",&strtok_str);
            if(lrecl==NULL)
            {
                msg="Logical Record Length missing for F record format file";
                break;
            }
            errno=0;
            reclen=strtoul(lrecl,&endptr,0);
            if(errno || endptr[0]!=0)
            {
                msg="Logical Record Length must be a numeric value";
                break;
            }
            if(reclen<1 || reclen>=MAX_BLKLEN)
            {
                msg="Logical Record Length must be between 1 and 65535";
                break;
            }

        }
        else
        {
            reclen=0;
        }
        filefmt=strtok_r(NULL," ",&strtok_str);
        if(filefmt==NULL)
        {
            msg="File format missing";
            break;
        }
        str_toupper(filefmt);
        if(strlen(filefmt)>1 || ( filefmt[0]!='B' && filefmt[0]!='S' && filefmt[0]!='T'))
        {
            msg="File format must be B (Binary) S (Structured) or T (Text)";
            break;
        }
        infile=strtok_r(NULL,"",&strtok_str);   /* allow spaces here */
        if(infile==NULL)
        {
            msg="Input file name missing";
            break;
        }
        badentry=0;
    } while(0);
    if(ignore)
    {
        free(rec);
        return 0;
    }
    if(badentry)
    {
        // ">>> %s"
        FWRMSG( stderr, HHC02631, "E", orec );
        // "    Bad entry at line %d in file \"%s\""
        FWRMSG( stderr, HHC02632, "E", recno, opts->procfile );
        // "    \"%s\""
        FWRMSG( stderr, HHC02633, "E", msg );
        free(rec);
        return 1;
    }
    // "%-8s %-8s %-2s"
    WRMSG( HHC02626, "I", fn, ft, fm );
    blks=load_file(infile,filefmt,recfm[0],&reclen,&reccount,&filesz);
    if(blks==NULL)
    {
        free(rec);
        return 1;
    }
        stat(infile,&stt);
    fstb=format_fst(fn,ft,fm,recfm[0],reclen,reccount,(int)filesz,stt.st_mtime);
    if(fstb==NULL)
    {
        free(rec);
        free_blocks(blks);
        return 1;
    }
    (void)write_tape_block_data(opts,(unsigned char *)fstb,sizeof(struct FST_BLOCK));
    (void)write_tape_blocks(opts,blks);


    free(rec);
    free(fstb);
    free_blocks(blks);
    return 0;
}

/* Process the DUMP control file */
int process_procfile(struct options *opts)
{
    FILE    *pfile;
    char    *rec;
    int     recno=0;
    int     errcount=0;

    pfile=fopen(opts->procfile,"r");
    if(pfile==NULL)
    {
        perror(opts->procfile);
        return 1;
    }
    while((rec=fgets((char*)bfr,sizeof(bfr),pfile))!=NULL)
    {
        recno++;
        rec[strlen(rec)-1]=0;
        if ( process_entry(opts,rec,recno) != 0 )
        {
            // "%s"
            WRMSG( HHC02627, "I", "Entry ignored" );
            WRMSG( HHC02627, "I", "" );
            errcount++;
        }
    }
    fclose(pfile);
    if(errcount>0)
    {
        // "%d errors encountered"
        WRMSG( HHC02630, "I", errcount );
    }
    write_tape_mark(opts);
    write_tape_mark(opts);
    return errcount?1:0;
}

/* Open the tape file for output */
int open_tapefile(struct options *opts,int w)
{
    if(w)
    {
        het_open(&opts->hetfd,opts->tapefile,HETOPEN_CREATE);
        het_cntl(opts->hetfd,HETCNTL_SET|HETCNTL_COMPRESS,1);
    }
    else
    {
        het_open(&opts->hetfd,opts->tapefile,HETOPEN_READONLY);
        het_cntl(opts->hetfd,HETCNTL_SET|HETCNTL_DECOMPRESS,1);
    }
    return 0;
}

/* Perform vmfplc2 DUMP operation */
int dodump(struct options *opts)
{
    // "%s"
    WRMSG( HHC02625, "I", "DUMPING..." );
    open_tapefile(opts,1);
    return process_procfile(opts);
}

/* Perform vmfplc2 LOAD operation */
int doload(struct options *opts)
{
    UNREFERENCED(opts);
    // "%s function not implemented (yet)"
    FWRMSG( stderr, HHC02624, "E", "LOAD" );
    return 0;
}

/* Perform vmfplc2 SCAN operation */
int doscan(struct options *opts)
{
    UNREFERENCED(opts);
    // "%s function not implemented (yet)"
    FWRMSG( stderr, HHC02624, "E", "SCAN" );
    return 0;
}

/* Main routine */
int main(int argc,char **argv)
{
    int             rc;
    struct options  opts;

    INITIALIZE_UTILITY( UTILITY_NAME, "VM/CMS VMFPLC2 Utility", &pgm );

    if(parse_parms(argc,argv,&opts)!=0)
    {
        return 1;
    }
    switch(opts.verb)
    {
        case VMFPLC2DUMP:
            rc=dodump(&opts);
            break;
        case VMFPLC2LOAD:
            rc=doload(&opts);
            break;
        case VMFPLC2SCAN:
            rc=doscan(&opts);
            break;
        default:
            rc=0;
            break;
    }
    return rc;
}

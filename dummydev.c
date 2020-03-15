/* DUMMYDEV.C   (C) Copyright Roger Bowler & Others, 2002-2012       */
/*              (C) Copyright, MHP, 2007-2008 (see below)            */
/*              Hercules Communication Line Driver                   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* (c) 2020 Ivan Warren. Licensed as stated above 

   This device emulator does NOT emulate any existing or known device.
   It has essentially 2 purposes :
   - Provide a skeleton for future device handlers/emulators
   - Provide an exercising tool for the hercules I/O subsystems

   As such it is encouraged to use this "emulator" as a template 
   for other developpers who wish to implement devices for their own
   use or for the benefit of others should they see fit.
*/

/********************************************************************* */

/* This include gives acccess to standard OS and C standard types used in hercules */
#include "hstdinc.h"
/* This include gives access to hercules types, structures, macros, etc.. */
#include "hercules.h"
/* includes known IBM device types */
#include "devtype.h"
/* includes macros and definition for a generic parser used during initialisation */
/* This is optional but is used as a convenience to parse keyword=value init type */
/* arguments */
#include "parser.h"
/* Here add the header files for structures and macros that are private to this handler */
/* #include "dummydev.h" */

/*-------------------------------------------------------------------*/
/* Ivan Warren 20040227                                              */
/*                                                                   */
/* This table is used by channel.c to determine if a CCW code        */
/* is an immediate command or not.                                   */
/*                                                                   */
/* The table is addressed in the DEVHND structure as 'DEVIMM immed'  */
/*                                                                   */
/*     0:  ("false")  Command is *NOT* an immediate command          */
/*     1:  ("true")   Command *IS* an immediate command              */
/*                                                                   */
/* Note: An immediate command is defined as a command which returns  */
/* CE (channel end) during initialization (that is, no data is       */
/* actually transfered). In this case, IL is not indicated for a     */
/* Format 0 or Format 1 CCW when IL Suppression Mode is in effect.   */
/*                                                                   */
/* This table is currently necessary because of a design flaw in the */
/* I/O subsystem in hercules                                         */
/* On another side note, declaring a 'READ' type CCW (one that is    */
/* even does not make sense). It only applies to Write and Control   */
/* write commands.                                                   */
/*-------------------------------------------------------------------*/

static BYTE commadpt_immed_command[256]=
{ 0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/*---------------------------------------------------------------*/
/* PARSER TABLES                                                 */
/* The following assumes the device initialisation parameters    */
/* can have foo= and bar=                                        */
/* The array should terminate with a NULL                        */
/* Adapt to your own needs                                       */
/*---------------------------------------------------------------*/
static PARSER ptab[]={
    { "foo",      PARSER_STR_TYPE },
    { "bar",      PARSER_STR_TYPE },
    {NULL,NULL}  /* (end of table) */
};

/* The following must be in the same sequence as the above 'ptab' table */
/* Those can then be used in a switch/case logic during device          */
/* initialisation. If a keyword is not recognized, 0 is returned        */

enum {
    DUMMY_KW_FOO=1,
    DUMMY_KW_BAR
} dummy_kw;

/*-------------------------------------------------------------------*/
/* Halt currently executing I/O command                              */
/*-------------------------------------------------------------------*/
static void dummydev_halt_or_clear( DEVBLK* dev )
{
    /* The lock on the DEVBLK is held... don't touch it..
       do NOT reset the dev->busy flag if it is set 
       if dev->busy is not set then there is nothing else to do */
    if (dev->busy)
    {
        /* Perform any action necessary to terminate
           any ongoing I/O in progress. Only return
           when it is sure the I/O was terminated */
    }
}

/*-------------------------------------------------------------------*/
/* Device Initialization                                             */
/* In hercules devices are declared as follows :                     */
/* [MSS/CSS]:CCUU devtype <any free text>                            */
/* he MSS/CSS:CCUU are handled ahead, no need to worry about them    */
/*                 unless devices need to be grouped.                */
/* the devtype merely indicates what module to load                  */
/* The <free text> is what goes into argc and argv                   */
/*     blank separated                                               */
/*-------------------------------------------------------------------*/
static int dummydev_init_handler (DEVBLK *dev, int argc, char *argv[])
{
    int     i,j;
    union   { int num; char text[MAX_PARSER_STRLEN+1];  /* (+1 for null terminator) */ } res;

    /*
     Initialise stuff such as
     Allocate private structures, buffers, set up default values, etc..
    */

    for(i=0;i<argc;i++)
    {
        pc=parser(ptab,argv[i],&res);
        if(pc<0)
        {
            WRMSG(HHC01012, "E",LCSS_DEVNUM,argv[i]);
            errcnt++;
            continue;
        }
        if(pc==0)
        {
            WRMSG(HHC01012, "E",LCSS_DEVNUM,argv[i]);
            errcnt++;
            continue;
        }
        switch(pc)
        {
            case COMMADPT_KW_LPORT:
                rc=commadpt_getport(res.text);
                if(rc<0)
                {
                    errcnt++;
                    msg01007e(dev,"LPORT",res.text);
                    break;
                }
                dev->commadpt->lport=rc;
                break;
            case COMMADPT_KW_LHOST:
                if(strcmp(res.text,"*")==0)
                {
                    dev->commadpt->lhost=INADDR_ANY;
                    break;
                }
                rc=commadpt_getaddr(&dev->commadpt->lhost,res.text);
                if(rc!=0)
                {
                    msg01007e(dev,"LHOST",res.text);
                    errcnt++;
                }
                break;
            case COMMADPT_KW_RPORT:
                rc=commadpt_getport(res.text);
                if(rc<0)
                {
                    errcnt++;
                    msg01007e(dev,"RPORT",res.text);
                    break;
                }
                dev->commadpt->rport=rc;
                break;
            case COMMADPT_KW_RHOST:
                if(strcmp(res.text,"*")==0)
                {
                    dev->commadpt->rhost=INADDR_NONE;
                    break;
                }
                rc=commadpt_getaddr(&dev->commadpt->rhost,res.text);
                if(rc!=0)
                {
                    msg01007e(dev,"RHOST",res.text);
                    errcnt++;
                }
                break;
            case COMMADPT_KW_READTO:
                dev->commadpt->rto=atoi(res.text);
                break;
            case COMMADPT_KW_POLLTO:
                dev->commadpt->pto=atoi(res.text);
                break;
            case COMMADPT_KW_ENABLETO:
                dev->commadpt->eto=atoi(res.text);
                etospec=1;
                break;
            case COMMADPT_KW_LNCTL:
                if(strcasecmp(res.text,"tele2")==0
                || strcasecmp(res.text,"ibm1")==0 )
                {
                    dev->commadpt->lnctl = COMMADPT_LNCTL_ASYNC;
                    dev->commadpt->rto=28000;        /* Read Time-Out in milis */
                }
                else if(strcasecmp(res.text,"bsc")==0)
                {
                    dev->commadpt->lnctl = COMMADPT_LNCTL_BSC;
                }
                else
                {
                    msg01007e(dev,"LNCTL",res.text);
                }
                break;
            case COMMADPT_KW_TERM:
                if(strcasecmp(res.text,"tty")==0)
                {
                    dev->commadpt->term = COMMADPT_TERM_TTY;
                }
                else if(strcasecmp(res.text,"2741")==0)
                {
                    dev->commadpt->term = COMMADPT_TERM_2741;
                }
                else if(strcasecmp(res.text,"rxvt4apl")==0)
                {
                    dev->commadpt->term = COMMADPT_TERM_2741;
                    dev->commadpt->rxvt4apl = 1;
                }
                else
                {
                    msg01007e(dev,"TERM",res.text);
                }
                break;
            case COMMADPT_KW_CODE:
                if(strcasecmp(res.text,"corr")==0)
                {
                    dev->commadpt->code_table_toebcdic   = xlate_table_cc_toebcdic;
                    dev->commadpt->code_table_fromebcdic = xlate_table_cc_fromebcdic;
                }
                else if(strcasecmp(res.text,"ebcd")==0)
                {
                    dev->commadpt->code_table_toebcdic   = xlate_table_ebcd_toebcdic;
                    dev->commadpt->code_table_fromebcdic = xlate_table_ebcd_fromebcdic;
                }
                else if(strcasecmp(res.text,"none")==0)
                {
                    dev->commadpt->code_table_toebcdic   = NULL;
                    dev->commadpt->code_table_fromebcdic = NULL;
                }
                else
                {
                    msg01007e(dev,"CODE",res.text);
                }
                break;
            case COMMADPT_KW_CRLF:
                if(strcasecmp(res.text,"no")==0)
                {
                    dev->commadpt->crlf_opt = FALSE;
                }
                else if(strcasecmp(res.text,"yes")==0)
                {
                    dev->commadpt->crlf_opt = TRUE;
                }
                else
                {
                    msg01007e(dev,"CRLF",res.text);
                }
                break;
            case COMMADPT_KW_SENDCR:
                if(strcasecmp(res.text,"no")==0)
                {
                    dev->commadpt->sendcr_opt = FALSE;
                }
                else if(strcasecmp(res.text,"yes")==0)
                {
                    dev->commadpt->sendcr_opt = TRUE;
                }
                else
                {
                    msg01007e(dev,"SENDCR",res.text);
                }
                break;
            case COMMADPT_KW_BINARY:
                if(strcasecmp(res.text,"no")==0)
                {
                    dev->commadpt->binary_opt = FALSE;
                }
                else if(strcasecmp(res.text,"yes")==0)
                {
                    dev->commadpt->binary_opt = TRUE;
                }
                else
                {
                    msg01007e(dev,"BINARY",res.text);
                }
                break;
            case COMMADPT_KW_UCTRANS:
                if(strcasecmp(res.text,"no")==0)
                {
                    dev->commadpt->uctrans = FALSE;
                }
                else if(strcasecmp(res.text,"yes")==0)
                {
                    dev->commadpt->uctrans = TRUE;
                }
                else
                {
                    msg01007e(dev,"UCTRANS",res.text);
                }
                break;
            case COMMADPT_KW_EOL:
                if  (strlen(res.text) < 2)
                    break;
                bf[0] = res.text[0];
                bf[1] = res.text[1];
                bf[2] = 0;
                sscanf(bf, "%x", &ix);
                dev->commadpt->eol_char = ix;
                break;
            case COMMADPT_KW_SKIP:
                if  (strlen(res.text) < 2)
                    break;
                for (j=0; j < (int)strlen(res.text); j+= 2)
                {
                    bf[0] = res.text[j+0];
                    bf[1] = res.text[j+1];
                    bf[2] = 0;
                    sscanf(bf, "%x", &ix);
                    dev->commadpt->byte_skip_table[ix] = 1;
                }
                break;
            case COMMADPT_KW_PREPEND:
                if  (strlen(res.text) != 2 && strlen(res.text) != 4
                  && strlen(res.text) != 6 && strlen(res.text) != 8)
                    break;
                for (j=0; j < (int)strlen(res.text); j+= 2)
                {
                    bf[0] = res.text[j+0];
                    bf[1] = res.text[j+1];
                    bf[2] = 0;
                    sscanf(bf, "%x", &ix);
                    dev->commadpt->prepend_bytes[j>>1] = ix;
                }
                dev->commadpt->prepend_length = strlen(res.text) >> 1;
                break;
            case COMMADPT_KW_APPEND:
                if  (strlen(res.text) != 2 && strlen(res.text) != 4
                  && strlen(res.text) != 6 && strlen(res.text) != 8)
                    break;
                for (j=0; j < (int)strlen(res.text); j+= 2)
                {
                    bf[0] = res.text[j+0];
                    bf[1] = res.text[j+1];
                    bf[2] = 0;
                    sscanf(bf, "%x", &ix);
                    dev->commadpt->append_bytes[j>>1] = ix;
                }
                dev->commadpt->append_length = strlen(res.text) >> 1;
                break;
            case COMMADPT_KW_ISKIP:
                if  (strlen(res.text) < 2)
                    break;
                for (j=0; j < (int)strlen(res.text); j+= 2)
                {
                    bf[0] = res.text[j+0];
                    bf[1] = res.text[j+1];
                    bf[2] = 0;
                    sscanf(bf, "%x", &ix);
                    dev->commadpt->input_byte_skip_table[ix] = 1;
                }
                break;
            case COMMADPT_KW_BS:
                if(strcasecmp(res.text,"dumb")==0) {
                    dev->commadpt->dumb_bs = 1;
                }
                break;
            case COMMADPT_KW_BREAK:
                if(strcasecmp(res.text,"dumb")==0)
                    dev->commadpt->dumb_break = 1;
                break;
            case COMMADPT_KW_SWITCHED:
            case COMMADPT_KW_DIAL:
                if(strcasecmp(res.text,"yes")==0 || strcmp(res.text,"1")==0 || strcasecmp(res.text,"inout")==0)
                {
                    dev->commadpt->dialin=1;
                    dev->commadpt->dialout=1;
                    break;
                }
                if(strcasecmp(res.text,"no")==0 || strcmp(res.text,"0")==0)
                {
                    dev->commadpt->dialin=0;
                    dev->commadpt->dialout=0;
                    break;
                }
                if(strcasecmp(res.text,"in")==0)
                {
                    dev->commadpt->dialin=1;
                    dev->commadpt->dialout=0;
                    break;
                }
                if(strcasecmp(res.text,"out")==0)
                {
                    dev->commadpt->dialin=0;
                    dev->commadpt->dialout=1;
                    break;
                }
                WRMSG(HHC01013, "E",LCSS_DEVNUM,res.text);
                dev->commadpt->dialin=0;
                dev->commadpt->dialout=0;
                break;
            case COMMADPT_KW_KA:
                if(strcasecmp(res.text,"no")==0)
                {
                    dev->commadpt->kaidle=0;
                    dev->commadpt->kaintv=0;
                    dev->commadpt->kacnt=0;
                }
                else
                {
                    int idle=-1,intv=-1,cnt=-1;
                    if (parse_conkpalv(res.text,&idle,&intv,&cnt)==0
                        || (idle==0 && intv==0 && cnt==0))
                    {
                        dev->commadpt->kaidle = idle;
                        dev->commadpt->kaintv = intv;
                        dev->commadpt->kacnt  = cnt;
                    }
                    else
                    {
                        msg01007e(dev,"KA",res.text);
                        errcnt++;
                    }
                }
                break;
            case COMMADPT_KW_CRLF2CR:
                if(strcasecmp(res.text,"no")==0)
                {
                    dev->commadpt->crlf2cr_opt = FALSE;
                }
                else if(strcasecmp(res.text,"yes")==0)
                {
                    dev->commadpt->crlf2cr_opt = TRUE;
                }
                else
                {
                    msg01007e(dev,"CRLF2CR",res.text);
                }
                break;
            default:
                break;
        }
    }
    /*
     * Check parameters consistency
     * when DIAL=NO :
     *     lport must not be 0
     *     lhost may be anything
     *     rport must not be 0
     *     rhost must not be INADDR_NONE
     * when DIAL=IN or DIAL=INOUT
     *     lport must NOT be 0
     *     lhost may be anything
     *     rport MUST be 0
     *     rhost MUST be INADDR_NONE
     * when DIAL=OUT
     *     lport MUST be 0
     *     lhost MUST be INADDR_ANY
     *     rport MUST be 0
     *     rhost MUST be INADDR_NONE
    */
    switch(dev->commadpt->dialin+dev->commadpt->dialout*2)
    {
        case 0:
            dialt="NO";
            break;
        case 1:
            dialt="IN";
            break;
        case 2:
            dialt="OUT";
            break;
        case 3:
            dialt="INOUT";
            break;
        default:
            dialt="*ERR*";
            break;
    }
    switch(dev->commadpt->dialin+dev->commadpt->dialout*2)
    {
        case 0: /* DIAL = NO */
            dev->commadpt->eto=0;
            if(dev->commadpt->lport==0)
            {
                msg01008e(dev,dialt,"LPORT");
                errcnt++;
            }
            if(dev->commadpt->rport==0)
            {
                msg01008e(dev,dialt,"RPORT");
                errcnt++;
            }
            if(dev->commadpt->rhost==INADDR_NONE)
            {
                msg01008e(dev,dialt,"RHOST");
                errcnt++;
            }
            if(etospec)
            {
                MSGBUF(fmtbfr,"%d",dev->commadpt->eto);
                msg01009w(dev,dialt,"ETO",fmtbfr);
                errcnt++;
            }
            dev->commadpt->eto=0;
            break;
        case 1: /* DIAL = IN */
        case 3: /* DIAL = INOUT */
            if(dev->commadpt->lport==0)
            {
                msg01008e(dev,dialt,"LPORT");
                errcnt++;
            }
            if(dev->commadpt->rport!=0)
            {
                MSGBUF(fmtbfr,"%d",dev->commadpt->rport);
                msg01009w(dev,dialt,"RPORT",fmtbfr);
            }
            if(dev->commadpt->rhost!=INADDR_NONE)
            {
                in_temp.s_addr=dev->commadpt->rhost;
                msg01009w(dev,dialt,"RHOST",inet_ntoa(in_temp));
                dev->commadpt->rhost=INADDR_NONE;
            }
            break;
        case 2: /* DIAL = OUT */
            if(dev->commadpt->lport!=0)
            {
                MSGBUF(fmtbfr,"%d",dev->commadpt->lport);
                msg01009w(dev,dialt,"LPORT",fmtbfr);
                dev->commadpt->lport=0;
            }
            if(dev->commadpt->rport!=0)
            {
                MSGBUF(fmtbfr,"%d",dev->commadpt->rport);
                msg01009w(dev,dialt,"RPORT",fmtbfr);
                dev->commadpt->rport=0;
            }
            if(dev->commadpt->lhost!=INADDR_ANY)    /* Actually it's more like INADDR_NONE */
            {
                in_temp.s_addr=dev->commadpt->lhost;
                msg01009w(dev,dialt,"LHOST",inet_ntoa(in_temp));
                dev->commadpt->lhost=INADDR_ANY;
            }
            if(dev->commadpt->rhost!=INADDR_NONE)
            {
                in_temp.s_addr=dev->commadpt->rhost;
                msg01009w(dev,dialt,"RHOST",inet_ntoa(in_temp));
                dev->commadpt->rhost=INADDR_NONE;
            }
            break;
    }
    if(errcnt>0)
    {
        WRMSG(HHC01014, "I",LCSS_DEVNUM);
        return -1;
    }
    in_temp.s_addr=dev->commadpt->lhost;
    in_temp.s_addr=dev->commadpt->rhost;
    dev->bufsize=256;
    dev->numsense=2;
    memset(dev->sense, 0, sizeof(dev->sense));

    /* Initialise various flags & statuses */
    dev->commadpt->enabled=0;
    dev->commadpt->connect=0;
    dev->fd=100;    /* Ensures 'close' function called */
    dev->commadpt->devnum=dev->devnum;

    dev->commadpt->telnet_opt=0;
    dev->commadpt->telnet_iac=0;
    dev->commadpt->telnet_int=0;
    dev->commadpt->eol_flag=0;
    dev->commadpt->telnet_cmd=0;

    dev->commadpt->haltpending=0;
    dev->commadpt->haltprepare=0;

    /* Initialize the device identifier bytes */
    dev->numdevid = sysblk.legacysenseid ? 7 : 0;
    dev->devid[0] = 0xFF;
    dev->devid[1] = dev->devtype >> 8;
    dev->devid[2] = dev->devtype & 0xFF;
    dev->devid[3] = 0x00;
    dev->devid[4] = dev->devtype >> 8;
    dev->devid[5] = dev->devtype & 0xFF;
    dev->devid[6] = 0x00;

    /* Initialize the CA lock */
    initialize_lock(&dev->commadpt->lock);

    /* Initialise thread->I/O & halt initiation EVB */
    initialize_condition(&dev->commadpt->ipc);
    initialize_condition(&dev->commadpt->ipc_halt);

    /* Allocate I/O -> Thread signaling pipe */
    VERIFY(!create_pipe(dev->commadpt->pipe));

    /* Obtain the CA lock */
    obtain_lock(&dev->commadpt->lock);

    /* Indicate listen required if DIAL!=OUT */
    if(dev->commadpt->dialin ||
            (!dev->commadpt->dialin && !dev->commadpt->dialout))
    {
        dev->commadpt->dolisten=1;
    }
    else
    {
        dev->commadpt->dolisten=0;
    }

    /* Start the async worker thread */

    /* Set thread-name for debugging purposes */
    MSGBUF(thread_name, "commadpt %4.4X thread", dev->devnum);
    thread_name[sizeof(thread_name)-1]=0;

    dev->commadpt->curpending=COMMADPT_PEND_TINIT;
    rc = create_thread(&dev->commadpt->cthread,DETACHED,commadpt_thread,dev->commadpt,thread_name);
    if(rc)
    {
        WRMSG(HHC00102, "E", strerror(rc));
        release_lock(&dev->commadpt->lock);
        return -1;
    }
    commadpt_wait(dev);
    if(dev->commadpt->curpending!=COMMADPT_PEND_IDLE)
    {
        WRMSG(HHC01015, "E",LCSS_DEVNUM);
        /* Release the CA lock */
        release_lock(&dev->commadpt->lock);
        return -1;
    }
    dev->commadpt->have_cthread=1;

    /* Release the CA lock */
    release_lock(&dev->commadpt->lock);
    /* Indicate succesfull completion */
    return 0;
}

static char *commadpt_lnctl_names[]={
    "NONE",
    "BSC",
    "ASYNC"
};

/*-------------------------------------------------------------------*/
/* Query the device definition                                       */
/*-------------------------------------------------------------------*/
static void commadpt_query_device (DEVBLK *dev, char **devclass,
                int buflen, char *buffer)
{
    char filename[ PATH_MAX + 1 ];      /* full path or just name    */

    BEGIN_DEVICE_CLASS_QUERY( "LINE", dev, devclass, buflen, buffer );

    snprintf(buffer,buflen,"%s STA=%s CN=%s, EIB=%s OP=%s IO[%"PRIu64"]",
            commadpt_lnctl_names[dev->commadpt->lnctl],
            dev->commadpt->enabled?"ENA":"DISA",
            dev->commadpt->connect?"YES":"NO",
            dev->commadpt->eibmode?"YES":"NO",
            commadpt_pendccw_text[dev->commadpt->curpending],
            dev->excps );
}

/*-------------------------------------------------------------------*/
/* Close the device                                                  */
/* Invoked by HERCULES shutdown & DEVINIT processing                 */
/*-------------------------------------------------------------------*/
static int commadpt_close_device( DEVBLK* dev )
{
    if (dev->ccwtrace)
    {
        // "%1d:%04X COMM: closing down"
        WRMSG( HHC01060, "D", LCSS_DEVNUM );
    }

    /* Terminate current I/O thread if necessary */
    if (dev->busy)
        commadpt_halt_or_clear( dev );

    /* Obtain the CA lock */
    obtain_lock( &dev->commadpt->lock );

    /* Terminate worker thread if it is still up */
    if (dev->commadpt->have_cthread)
    {
        dev->commadpt->curpending   = COMMADPT_PEND_SHUTDOWN;

        commadpt_wakeup( dev->commadpt, 0 );
        commadpt_wait( dev );

        dev->commadpt->cthread      = (TID) -1;
        dev->commadpt->have_cthread = 0;
    }


    /* Free all work storage */
    /* The CA lock will be released by the cleanup routine */
    commadpt_clean_device( dev );  // also does "release_lock( &dev->commadpt->lock );"

    /* Indicate to hercules the device is no longer opened */
    dev->fd = -1;

    if (dev->ccwtrace)
    {
        // "%1d:%04X COMM: closed down"
        WRMSG( HHC01061, "D", LCSS_DEVNUM );
    }
    return 0;
}

/*-------------------------------------------------------------------*/
/* Execute a Channel Command Word                                    */
/*-------------------------------------------------------------------*/
static void dummy_execute_ccw (DEVBLK *dev, BYTE code, BYTE flags,
        BYTE chained, U32 count, BYTE prevcode, int ccwseq,
        BYTE *iobuf, BYTE *more, BYTE *unitstat, U32 *residual)
{
    switch(code)
    {
        case 0x03:
            *unistat=CSW_CE|CSW_DE;
            break;
        default:
            *unitstat=CSW_UC;
            dev->sense[0]=SENSE_CR;
    }
]


/*---------------------------------------------------------------*/
/* DEVICE FUNCTION POINTERS                                      */
/*---------------------------------------------------------------*/

static DEVHND dummydev_device_hndinfo =
{
        &dummydev_init_handler,        /* Device Initialization      */
        &dummydev_execute_ccw,         /* Device CCW execute         */
        &dummydev_close_device,        /* Device Close               */
        &dummydev_query_device,        /* Device Query               */
        NULL,                          /* Device Extended Query      */
        NULL,                          /* Device Start channel pgm   */
        NULL,                          /* Device End channel pgm     */
        NULL,                          /* Device Resume channel pgm  */
        NULL,                          /* Device Suspend channel pgm */
        &dummydev_halt_or_clear,       /* Device Halt channel pgm    */
        NULL,                          /* Device Read                */
        NULL,                          /* Device Write               */
        NULL,                          /* Device Query used          */
        NULL,                          /* Device Reserve             */
        NULL,                          /* Device Release             */
        NULL,                          /* Device Attention           */
        dummydev_immed_command,        /* Immediate CCW Codes        */
        NULL,                          /* Signal Adapter Input       */
        NULL,                          /* Signal Adapter Output      */
        NULL,                          /* Signal Adapter Sync        */
        NULL,                          /* Signal Adapter Output Mult */
        NULL,                          /* QDIO subsys desc           */
        NULL,                          /* QDIO set subchan ind       */
        NULL,                          /* Hercules suspend           */
        NULL                           /* Hercules resume            */
};


/* Libtool static name colision resolution */
/* note : lt_dlopen will look for symbol & modulename_LTX_symbol */

#if defined( HDL_USE_LIBTOOL )
#define hdl_ddev hdtdummydev_LTX_hdl_ddev
#define hdl_depc hdtdummydev_LTX_hdl_depc
#define hdl_reso hdtdummydev_LTX_hdl_reso
#define hdl_init hdtdummydev_LTX_hdl_init
#define hdl_fini hdtdummydev_LTX_hdl_fini
#endif


HDL_DEPENDENCY_SECTION;
{
     HDL_DEPENDENCY(HERCULES);
     HDL_DEPENDENCY(DEVBLK);
     HDL_DEPENDENCY(SYSBLK);
}
END_DEPENDENCY_SECTION

HDL_DEVICE_SECTION;
{
    HDL_DEVICE(dummy, dummydev_device_hndinfo );
}
END_DEVICE_SECTION

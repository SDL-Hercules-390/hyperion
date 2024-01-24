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

static BYTE dummydev_immed_command[256]=
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
    int     i,pc;
    int     errcnt=0;
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
            default:
                break;
        }
    }
    if(errcnt>0)
    {
        return(-1);
    }
    /*
     * Check parameters consistency
    */
    /*
     Set the sense ID */
    dev->numdevid=7;
    dev->devid[0]=0xff;
    dev->devid[1]=0x30;
    dev->devid[2]=0x88;
    dev->devid[3]=0x01;
    dev->devid[4]=0x30;
    dev->devid[5]=0x88;
    dev->devid[6]=0x01;
    /*
     Init device private information
     here base on parameters given
     return -1 if this is not consistent */
    return 0;
}

/*-------------------------------------------------------------------*/
/* Query the device definition                                       */
/* The text displayed for devlist and the REGS display               */
/*-------------------------------------------------------------------*/
static void dummydev_query_device (DEVBLK *dev, char **devclass,
                int buflen, char *buffer)
{
    /* This is required by the BEGIN_DEVICE_CLASS_QUERY macro.. UGLY !! */
    char filename[ PATH_MAX + 1 ];      /* full path or just name    */

    /* This is to make it look pretty */
    BEGIN_DEVICE_CLASS_QUERY( "DUMB", dev, devclass, buflen, buffer );

    /* and then some device specifiic stuff */

    snprintf(buffer,buflen,"%s","DUMMY DEVICE");
}

/*-------------------------------------------------------------------*/
/* Close the device                                                  */
/* Invoked by HERCULES shutdown & DEVINIT processing                 */
/*-------------------------------------------------------------------*/
static int dummydev_close_device( DEVBLK* dev )
{
    /* Terminate current I/O thread if necessary */
    if (dev->busy)
        dummydev_halt_or_clear( dev );

    /* Perform more termination if needed - signal thread(s) to close
       and whatnot
    */
    return 0;
}

/*-------------------------------------------------------------------*/
/* Execute a Channel Command Word                                    */
/* The actual CORE of a handler/driver - executing I/Os.             */
/* dev is the hercules device block - it contains all the information*/
/*     needed to interact between the handler and the I/O engine     */
/* code is the CCW command code. it is split out because the CCW     */
/*     be a CCW format 0 r format 1 - that's handled by the hercules */
/*     I/O code.                                                     */
/* flags same as above these are the CCW flags (SLI, etc..)          */
/* chained was parsed from the CCW by the hercules I/O code from the */
/*     previous CCW (if any).                                        */
/* count is the DATA count in the CCW. for immediate commands, this  */
/*     irrelevant                                                    */
/* prevcode is the code of the previous CCW if it was "chained" this */
/*     is usually relevant for DASD operations which require CCWs    */
/*     to follow a certain sequence of operations                    */
/* ccwseq : ????                                                     */
/* iobuf is the storage area to read from or write to. Caution !     */
/*     This is not compliant with the architecture. it is the Control*/
/*     unit that should pull data or indicate the amount written     */
/*     the current implementation may lead to channel protection     */
/*     checks, channel addressing check and/or overwriting data      */
/*     when it should not occur.                                     */
/* more simply indicates on a read tpe operation that there is more  */
/*     information to be read and the the count was too low.         */
/* unitstat is the device unit status such as CE, DE, UC, UX, etc..  */
/* Residual indicates that the read or write operation was short     */
/*     and there are "residual" bytes left                           */
/*-------------------------------------------------------------------*/
static void dummydev_execute_ccw (DEVBLK *dev, BYTE code, BYTE flags,
        BYTE chained, U32 count, BYTE prevcode, int ccwseq,
        BYTE *iobuf, BYTE *more, BYTE *unitstat, U32 *residual)
{
    int num;

    UNREFERENCED( flags );
    UNREFERENCED( chained );
    UNREFERENCED( prevcode );
    UNREFERENCED( ccwseq );

    switch(code)
    {
        case 0x01:
            /* That's a basic write */
            /* Here let's say we gobble everything */
            *residual=0;
            *unitstat=CSW_CE|CSW_DE;
            break;
        case 0x02:
            /* That's a basic read.. also works for IPLs */
            /* Let's just send back something stupid here */
            memset(iobuf,0,count);
            *residual=0;
            *unitstat=CSW_CE|CSW_DE;
            break;
        case 0x03:
            /* 0x03 is NO-OP and is by definition an immediate */
            /* command so iobuf, count are void */
            *residual=count;
            *unitstat=CSW_CE|CSW_DE;
            break;
            /* PS : If you wish that the device NOT be
               ready for any reason you see fit, only indicate
               CSW_UC (NO CE/DE here) and put SENSE_IR in the
               sense field
                something like :
                *unitstat=CSW_UC;
                dev->sense[0]=SENSE_IR;
            */
        case 0x04:
            /* Basic sense... we can do this */
            /* this is filled by any previous */
            /* command */
            if(dev->numsense>count)
            {
                *more=1;
                memcpy(iobuf,dev->sense,count);
                *residual=0;
                /* Not sure how a sense works on a short read */
            }
            else
            {
                /* sense fits in the IO buffer */
                *more=0;
                memcpy(iobuf,dev->sense,dev->numsense);
                *residual=count-dev->numsense;
            }
            *unitstat=CSW_CE|CSW_DE;
            break;
        case 0xe4:
            /* Sense ID (device type) */
            num=(int)dev->numdevid;
            if((int)count<num)
            {
                num=(int)count;
            }
            memcpy(iobuf,dev->devid,num);
            *residual=count-num;
            *unitstat=CSW_CE|CSW_DE;
            break;
        default:
            /* We didn't understand.. so Command Reject */
            *unitstat=CSW_UC;
            dev->sense[0]=SENSE_CR;
            dev->numsense=1;
            break;
    }
}


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

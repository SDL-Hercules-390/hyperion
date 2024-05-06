/* TFPRINT.C    (C) Copyright "Fish" (David B. Trout), 2023          */
/*              (C) and others 2024                                  */
/*              Print Trace File Utility                             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This program reads a Hercules intruction trace file and produces  */
/* the corresponding textual printout.                               */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"

#define UTILITY_NAME    "tfprint"
#define UTILITY_DESC    "Print Trace File Utility"

PUSH_GCC_WARNINGS()
DISABLE_GCC_WARNING( "-Waddress-of-packed-member" )

/*-------------------------------------------------------------------*/
/*                        TFPRINT                                    */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  Usage:  tfprint  [options...] tracefile                          */
/*                                                                   */
/*         [-i|--info]                                               */
/*         [-c|--cpu hh[[-hh][,hh]]                                  */
/*         [-r|--traceopt TRADITIONAL|REGSFIRST|NOREGS]              */
/*         [-n|--count nnnnnn[[-nnnnnn]|[.nnn]]                      */
/*         [-e|--msg nnnnn[,nnnnn]                                   */
/*         [-s|--storage V|R:hhhhhh[[-hhhhhh]|[.hhh]]                */
/*         [-d|--date YYYY/MM/DD-YYYY/MM/DD                          */
/*         [-t|--time HH:MM:SS.nnnnnn-HH:MM:SS.nnnnnn                */
/*         [-o|--opcode hhhhhhhh[,hhxxhhxxhhhh]                      */
/*         [-m|--msglvl xxxxx                                        */
/*         [-u|--unit uuuu[[-uuuu]|[.nnn]]                           */
/*         [-p]--codepage zzzzzzzz                                   */
/*                                                                   */
/*    -i   Print only TFSYS header information then exit             */
/*    -c   Print only specified CPU(s)                               */
/*    -r   Print registers trace option                              */
/*    -n   Print only records nnnnnn to nnnnnn (by count)            */
/*    -e   Print only messages with specified message number         */
/*    -s   Print only instructions that reference or modify          */
/*         the specified 'V'irtual or 'R'eal storage range           */
/*    -d   Print only records within specified date range            */
/*    -t   Print only records within specified time range            */
/*    -o   Print only specified instruction(s) (by opcode)           */
/*         Only as many bytes are checked as are specified.          */
/*         Use 'x' as wildcard for nibbles not to be checked         */
/*    -m   Modify Hercules 'msglvl' setting                          */
/*    -u   Print only trace records for specified I/O unit(s)        */
/*    -p   Use EBCDIC/ASCII codepage conversion table zzzzzzzz       */
/*                                                                   */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*  Mulitple Options array entry -- used by parse_opt_str function   */
/*-------------------------------------------------------------------*/
struct mopt
{
    U64   opt1;         // Start of range (if isrange is true),
                        // or only value  (if isrange is false).
    U64   opt2;         // End of range   (only if isrange is true)
    BYTE  icode[6];     // Instruction bytes
    BYTE  imask[6];     // Instruction mask
    BYTE  isrange;      // true: "opt1-opt2", false: "opt1" only
    BYTE  isreal;       // true: opts are real addresses; else virtual
};
typedef struct mopt MOPT;
#define TM struct tm

/*-------------------------------------------------------------------*/
/*           Table of thread-numbers and thread-names                */
/*-------------------------------------------------------------------*/
struct tidtab
{
    U64   tidnum;       // Thread-id number (not 'TID'!)
    char  thrdname[16]; // Thread name
};
typedef struct tidtab TIDTAB;

static TIDTAB* tidtab = NULL;   // (ptr to table)
static int numtids = 0;         // (number of entries)

/*-------------------------------------------------------------------*/
/*               static global work variables                        */
/*-------------------------------------------------------------------*/
static char* pgm       = NULL;          /* less any extension (.ext) */
static FILE* inf       = NULL;          /* Input file stream         */
static int   arg_errs  = 0;             /* Parse options error count */
static size_t hdr_size = sizeof(TFHDR); /* Size of file's TFHDR's    */
static bool  info_only = false;         /* --info option             */
static bool  regsfirst = false;         /* --traceopt REGSFIRST      */
static bool  noregs    = false;         /* --traceopt NOREGS         */
static bool  doendswap = false;         /* endian swaps needed       */
static bool  out_istty = false;         /* stdout is a TTY device    */
static bool  err_istty = false;         /* stderr is a TTY device    */
static double filesize = 0;             /* File size as double       */
static CPU_BITMAP cpu_map = 0;          /* --cpu option              */
static U64   recnum    = 0;             /* Current record number     */
static U64   fromrec   = 0;             /* --count option            */
static U64   torec     = ~0;            /* --count option            */
static U64   totins    = 0;             /* Total instruct printed    */
static U64   totios    = 0;             /* Total dev I/Os printed    */
static MOPT* pMsgMOPT  = NULL;          /* --msg option              */
static int   nMsgMOPT  = 0;             /* --msg option              */
static MOPT* pStorMOPT = NULL;          /* --storage option          */
static int   nStorMOPT = 0;             /* --storage option          */
static MOPT* pInstMOPT = NULL;          /* --opcode option           */
static int   nInstMOPT = 0;             /* --opcode option           */
static MOPT* pUnitMOPT = NULL;          /* --unit option             */
static int   nUnitMOPT = 0;             /* --unit option             */
static TIMEVAL beg_tim = {0};           /* --time option             */
static TIMEVAL end_tim = {0};           /* --time option             */
static time_t  beg_dat = {0};           /* --date option             */
static time_t  end_dat = {0};           /* --date option             */
static U16   prvcpuad  = 0;             /* cpuad  of prv printed rec */
static U16   prvdevnum = 0;             /* devnum of prv printed rec */
static bool  previnst  = true;          /* prv was instruction print */
static BYTE  sys_ffmt  = TF_FMT;        /* Saved TFSYS file format   */
static char  pathname[ MAX_PATH ] = {0};/* Trace file name           */
static BYTE  iobuff[ _64_KILOBYTE ];    /* Trace file I/O buffer     */

/*-------------------------------------------------------------------*/
/*                   Show Usage and exit                             */
/*-------------------------------------------------------------------*/
static void show_usage()
{
    fprintf( stderr, "\n" );
    FWRMSG( stderr, HHC03200, "I", pgm );
    fprintf( stderr, "\n" );
    exit( -1 );
}

/*-------------------------------------------------------------------*/
/*             Issue Periodic Progress Messages                      */
/*-------------------------------------------------------------------*/
static void show_file_progress()
{
    off_t   currpos;
    double  percent;

    if ((currpos = ftell( inf )) < 0)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC00075, "E", "ftell", strerror( errno ));
        exit( -1 );
    }

    percent = ((double) currpos) / filesize;
    percent *= 100.0;

    if (extgui)
        fprintf( stderr, "PCT=%.0f\n", percent );
    else if (err_istty)
    {
        char scale[50+1];
        int i;

        /* Draw a nice scale too */
        memset( scale, '.', 50 );
        for (i=1; i <= 50; ++i)         // (50 = 2 percent per position)
        {
            if (percent >= (i << 1))    // (percent greater than here?)
                scale[i-1] = '*';
            else
                break;
        }
        scale[50] = 0;

        fprintf( stderr, "%.0f%% of file processed...   [%s]\r",
            percent, scale );
    }
}

/******************************************************************************/
/*                                                                            */
/*                     STATIC RECORDS SAVE AREA                               */
/*                                                                            */
/*                All TraceFile Records for Each CPU                          */
/*                                                                            */
/******************************************************************************/

struct ALLRECS
{
    // INSTRUCTION TRACING: since many (most?) of the instruction tracing
    // records are not printed until record 2324 is processed (the primary
    // instruction trace record), we need to save them in our table until
    // record 2324 is eventually encountered. Thus they each need a unique
    // table slot and GOT mask value.

#define  GOT_TF00800   0x80000000
#define  GOT_TF00801   0x40000000
#define  GOT_TF00802   0x20000000
#define  GOT_TF00803   0x10000000
#define  GOT_TF00804   0x08000000
#define  GOT_TF00806   0x04000000
#define  GOT_TF00807   0x02000000
#define  GOT_TF00808   0x01000000
#define  GOT_TF00809   0x00800000
#define  GOT_TF00811   0x00400000
#define  GOT_TF00812   0x00200000
#define  GOT_TF00814   0x00100000
#define  GOT_TF00840   0x00080000
#define  GOT_TF00844   0x00040000
#define  GOT_TF00845   0x00020000
#define  GOT_TF00846   0x00010000

#define  GOT_TF02266   0x00008000
#define  GOT_TF02269   0x00004000
#define  GOT_TF02270   0x00002000
#define  GOT_TF02271   0x00001000
#define  GOT_TF02272   0x00000800
#define  GOT_TF02276   0x00000400
#define  GOT_TF02324   0x00000200
#define  GOT_TF02326   0x00000100

    TF00800  tf00800;           // Wait State PSW
    TF00801  tf00801;           // Program Interrupt
    TF00801  tf00802;           // PER event
    TF00801  tf00803;           // Program interrupt loop
    TF00801  tf00804;           // I/O interrupt (S/370)
    TF00806  tf00806;           // I/O Interrupt
    TF00807  tf00807;           // Machine Check Interrupt
    TF00801  tf00808;           // Store Status
    TF00801  tf00809;           // Disabled Wait State
    TF00801  tf00811;           // Architecture mode
    TF00801  tf00812;           // Vector facility online (370/390)
    TF00814  tf00814;           // Signal Processor
    TF00840  tf00840;           // External Interrupt
    TF00844  tf00844;           // Block I/O Interrupt
    TF00845  tf00845;           // Block I/O External interrupt
    TF00846  tf00846;           // Service Signal External Interrupt

    TF02266  tf02266;           // Vector Register
    TF02269  tf02269;           // General Registers
    TF02270  tf02270;           // Floating Point Registers
    TF02271  tf02271;           // Control Registers
    TF02272  tf02272;           // Access Registers
    TF02276  tf02276;           // Floating Point Control Register
    TF02324  tf02324;           // Instruction Trace
    TF02326  tf02326;           // Instruction Operands

    // DEVICE TRACING: since each device trace record is always immediately
    // printed (and the GOT flag cleared afterwards) we can use the same
    // GOT mask for all of them.
    //
    // Also note that even though we don't need a separate table slot for each
    // one (because we have no need to hang onto them for later), we provide a
    // separate slot for each one anyway since it makes our logic simpler.

#define  GOT_TF00423   0x00000001
#define  GOT_TF00424   0x00000001
#define  GOT_TF00425   0x00000001
#define  GOT_TF00426   0x00000001
#define  GOT_TF00427   0x00000001
#define  GOT_TF00428   0x00000001
#define  GOT_TF00429   0x00000001
#define  GOT_TF00430   0x00000001
#define  GOT_TF00431   0x00000001
#define  GOT_TF00432   0x00000001
#define  GOT_TF00433   0x00000001
#define  GOT_TF00434   0x00000001
#define  GOT_TF00435   0x00000001
#define  GOT_TF00436   0x00000001
#define  GOT_TF00437   0x00000001
#define  GOT_TF00438   0x00000001
#define  GOT_TF00439   0x00000001
#define  GOT_TF00440   0x00000001
#define  GOT_TF00441   0x00000001
#define  GOT_TF00442   0x00000001

#define  GOT_TF00516   0x00000001
#define  GOT_TF00517   0x00000001
#define  GOT_TF00518   0x00000001
#define  GOT_TF00519   0x00000001
#define  GOT_TF00520   0x00000001

#define  GOT_TF01300   0x00000001
#define  GOT_TF01301   0x00000001
#define  GOT_TF01304   0x00000001
#define  GOT_TF01305   0x00000001
#define  GOT_TF01306   0x00000001
#define  GOT_TF01307   0x00000001
#define  GOT_TF01308   0x00000001
#define  GOT_TF01309   0x00000001
#define  GOT_TF01310   0x00000001
#define  GOT_TF01311   0x00000001
#define  GOT_TF01312   0x00000001
#define  GOT_TF01313   0x00000001
#define  GOT_TF01315   0x00000001
#define  GOT_TF01316   0x00000001
#define  GOT_TF01317   0x00000001
#define  GOT_TF01318   0x00000001
#define  GOT_TF01320   0x00000001
#define  GOT_TF01321   0x00000001
#define  GOT_TF01329   0x00000001
#define  GOT_TF01330   0x00000001
#define  GOT_TF01331   0x00000001
#define  GOT_TF01332   0x00000001
#define  GOT_TF01333   0x00000001
#define  GOT_TF01334   0x00000001
#define  GOT_TF01336   0x00000001

    TF00423  tf00423;           // Search key
    TF00424  tf00424;           // Cur trk
    TF00425  tf00425;           // Updating track
    TF00426  tf00426;           // Cache hit
    TF00427  tf00427;           // Unavailable cache
    TF00428  tf00428;           // Cache miss
    TF00429  tf00429;           // Offset
    TF00430  tf00430;           // Trkhdr
    TF00431  tf00431;           // Seeking
    TF00432  tf00432;           // MT advance error
    TF00433  tf00433;           // MT advance
    TF00434  tf00434;           // Read count orientation
    TF00435  tf00435;           // Cyl head record kl dl
    TF00436  tf00436;           // Read key
    TF00437  tf00437;           // Read data
    TF00438  tf00438;           // Write cyl head record kl dl
    TF00439  tf00439;           // Set track overflow flag
    TF00440  tf00440;           // Update cyl head record kl dl
    TF00441  tf00441;           // Update cyl head record dl
    TF00442  tf00442;           // Set file mask

    TF00516  tf00516;           // Cache hit
    TF00517  tf00517;           // Unavailable cache
    TF00518  tf00518;           // Cache miss
    TF00519  tf00519;           // Offset len
    TF00520  tf00520;           // Positioning

    TF01300  tf01300;           // Halt subchannel
    TF01301  tf01301;           // IDAW/MIDAW
    TF01304  tf01304;           // Attention signaled
    TF01305  tf01305;           // Attention
    TF01306  tf01306;           // Initial status interrupt
    TF01307  tf01307;           // Attention completed
    TF01308  tf01308;           // Clear completed
    TF01309  tf01309;           // Halt completed
    TF01310  tf01310;           // Suspended
    TF01311  tf01311;           // Resumed
    TF01312  tf01312;           // I/O stat
    TF01313  tf01313;           // Sense
    TF01315  tf01315;           // CCW
    TF01316  tf01316;           // CSW (370)
    TF01317  tf01317;           // SCSW
    TF01318  tf01318;           // TEST I/O
    TF01320  tf01320;           // S/370 start I/O conversion started
    TF01321  tf01321;           // S/370 start I/O conversion success
    TF01329  tf01329;           // Halt I/O
    TF01330  tf01330;           // HIO modification
    TF01331  tf01331;           // Clear subchannel
    TF01332  tf01332;           // Halt subchannel
    TF01333  tf01333;           // Resume subchannel
    TF01334  tf01334;           // ORB
    TF01336  tf01336;           // Startio cc=2

    U32      gotmask;           // Mask identifying which records we got
};
typedef struct ALLRECS ALLRECS;

static ALLRECS all_recs[ MAX_CPU_ENGS + 1 ] = {0}; // (recs for ALL cpus + dev)


/******************************************************************************/
/*                                                                            */
/*                        HELPER FUNCTIONS                                    */
/*                                                                            */
/******************************************************************************/

/*-------------------------------------------------------------------*/
/*           Return GOT mask for given record number                 */
/*-------------------------------------------------------------------*/
static U32 gotmask( U16 msgnum )
{
    switch (msgnum)
    {
    // Instruction tracing...

    case  800: return GOT_TF00800;
    case  801: return GOT_TF00801;
    case  802: return GOT_TF00802;
    case  803: return GOT_TF00803;
    case  804: return GOT_TF00804;
    case  806: return GOT_TF00806;
    case  807: return GOT_TF00807;
    case  808: return GOT_TF00808;
    case  809: return GOT_TF00809;
    case  811: return GOT_TF00811;
    case  812: return GOT_TF00812;
    case  814: return GOT_TF00814;
    case  840: return GOT_TF00840;
    case  844: return GOT_TF00844;
    case  845: return GOT_TF00845;
    case  846: return GOT_TF00846;

    case 2266: return GOT_TF02266;
    case 2269: return GOT_TF02269;
    case 2270: return GOT_TF02270;
    case 2271: return GOT_TF02271;
    case 2272: return GOT_TF02272;
    case 2276: return GOT_TF02276;
    case 2324: return GOT_TF02324;
    case 2326: return GOT_TF02326;

    // Device tracing...

    case  423: return GOT_TF00423;
    case  424: return GOT_TF00424;
    case  425: return GOT_TF00425;
    case  426: return GOT_TF00426;
    case  427: return GOT_TF00427;
    case  428: return GOT_TF00428;
    case  429: return GOT_TF00429;
    case  430: return GOT_TF00430;
    case  431: return GOT_TF00431;
    case  432: return GOT_TF00432;
    case  433: return GOT_TF00433;
    case  434: return GOT_TF00434;
    case  435: return GOT_TF00435;
    case  436: return GOT_TF00436;
    case  437: return GOT_TF00437;
    case  438: return GOT_TF00438;
    case  439: return GOT_TF00439;
    case  440: return GOT_TF00440;
    case  441: return GOT_TF00441;
    case  442: return GOT_TF00442;

    case  516: return GOT_TF00516;
    case  517: return GOT_TF00517;
    case  518: return GOT_TF00518;
    case  519: return GOT_TF00519;
    case  520: return GOT_TF00520;

    case 1300: return GOT_TF01300;
    case 1301: return GOT_TF01301;
    case 1304: return GOT_TF01304;
    case 1305: return GOT_TF01305;
    case 1306: return GOT_TF01306;
    case 1307: return GOT_TF01307;
    case 1308: return GOT_TF01308;
    case 1309: return GOT_TF01309;
    case 1310: return GOT_TF01310;
    case 1311: return GOT_TF01311;
    case 1312: return GOT_TF01312;
    case 1313: return GOT_TF01313;
    case 1315: return GOT_TF01315;
    case 1316: return GOT_TF01316;
    case 1317: return GOT_TF01317;
    case 1318: return GOT_TF01318;
    case 1320: return GOT_TF01320;
    case 1321: return GOT_TF01321;
    case 1329: return GOT_TF01329;
    case 1330: return GOT_TF01330;
    case 1331: return GOT_TF01331;
    case 1332: return GOT_TF01332;
    case 1333: return GOT_TF01333;
    case 1334: return GOT_TF01334;
    case 1336: return GOT_TF01336;

    default: CRASH(); UNREACHABLE_CODE( return 0 );
    }
}

/*-------------------------------------------------------------------*/
/*          Return record size for given record number              */
/*-------------------------------------------------------------------*/
static size_t recsize( U16 msgnum )
{
    switch (msgnum)
    {
    // Instruction tracing...

    case  800: return sizeof( TF00800 );
    case  801: return sizeof( TF00801 );
    case  802: return sizeof( TF00802 );
    case  803: return sizeof( TF00803 );
    case  804: return sizeof( TF00804 );
    case  806: return sizeof( TF00806 );
    case  807: return sizeof( TF00807 );
    case  808: return sizeof( TF00808 );
    case  809: return sizeof( TF00809 );
    case  811: return sizeof( TF00811 );
    case  812: return sizeof( TF00812 );
    case  814: return sizeof( TF00814 );
    case  840: return sizeof( TF00840 );
    case  844: return sizeof( TF00844 );
    case  845: return sizeof( TF00845 );
    case  846: return sizeof( TF00846 );

    case 2266: return sizeof( TF02266 );
    case 2269: return sizeof( TF02269 );
    case 2270: return sizeof( TF02270 );
    case 2271: return sizeof( TF02271 );
    case 2272: return sizeof( TF02272 );
    case 2276: return sizeof( TF02276 );
    case 2324: return sizeof( TF02324 );
    case 2326: return sizeof( TF02326 );

    // Device tracing...

    case  423: return sizeof( TF00423 );
    case  424: return sizeof( TF00424 );
    case  425: return sizeof( TF00425 );
    case  426: return sizeof( TF00426 );
    case  427: return sizeof( TF00427 );
    case  428: return sizeof( TF00428 );
    case  429: return sizeof( TF00429 );
    case  430: return sizeof( TF00430 );
    case  431: return sizeof( TF00431 );
    case  432: return sizeof( TF00432 );
    case  433: return sizeof( TF00433 );
    case  434: return sizeof( TF00434 );
    case  435: return sizeof( TF00435 );
    case  436: return sizeof( TF00436 );
    case  437: return sizeof( TF00437 );
    case  438: return sizeof( TF00438 );
    case  439: return sizeof( TF00439 );
    case  440: return sizeof( TF00440 );
    case  441: return sizeof( TF00441 );
    case  442: return sizeof( TF00442 );

    case  516: return sizeof( TF00516 );
    case  517: return sizeof( TF00517 );
    case  518: return sizeof( TF00518 );
    case  519: return sizeof( TF00519 );
    case  520: return sizeof( TF00520 );

    case 1300: return sizeof( TF01300 );
    case 1301: return sizeof( TF01301 );
    case 1304: return sizeof( TF01304 );
    case 1305: return sizeof( TF01305 );
    case 1306: return sizeof( TF01306 );
    case 1307: return sizeof( TF01307 );
    case 1308: return sizeof( TF01308 );
    case 1309: return sizeof( TF01309 );
    case 1310: return sizeof( TF01310 );
    case 1311: return sizeof( TF01311 );
    case 1312: return sizeof( TF01312 );
    case 1313: return sizeof( TF01313 );
    case 1315: return sizeof( TF01315 );
    case 1316: return sizeof( TF01316 );
    case 1317: return sizeof( TF01317 );
    case 1318: return sizeof( TF01318 );
    case 1320: return sizeof( TF01320 );
    case 1321: return sizeof( TF01321 );
    case 1329: return sizeof( TF01329 );
    case 1330: return sizeof( TF01330 );
    case 1331: return sizeof( TF01331 );
    case 1332: return sizeof( TF01332 );
    case 1333: return sizeof( TF01333 );
    case 1334: return sizeof( TF01334 );
    case 1336: return sizeof( TF01336 );

    default: CRASH(); UNREACHABLE_CODE( return 0 );
    }
}

/*-------------------------------------------------------------------*/
/*      Return ALLRECS record pointer for given record number        */
/*-------------------------------------------------------------------*/
static void* all_recs_ptr( U16 cpuad, U16 msgnum )
{
    switch (msgnum)
    {
    // Instruction tracing...

    case  800: return &all_recs[ cpuad ].tf00800;
    case  801: return &all_recs[ cpuad ].tf00801;
    case  802: return &all_recs[ cpuad ].tf00802;
    case  803: return &all_recs[ cpuad ].tf00803;
    case  804: return &all_recs[ cpuad ].tf00804;
    case  806: return &all_recs[ cpuad ].tf00806;
    case  807: return &all_recs[ cpuad ].tf00807;
    case  808: return &all_recs[ cpuad ].tf00808;
    case  809: return &all_recs[ cpuad ].tf00809;
    case  811: return &all_recs[ cpuad ].tf00811;
    case  812: return &all_recs[ cpuad ].tf00812;
    case  814: return &all_recs[ cpuad ].tf00814;
    case  840: return &all_recs[ cpuad ].tf00840;
    case  844: return &all_recs[ cpuad ].tf00844;
    case  845: return &all_recs[ cpuad ].tf00845;
    case  846: return &all_recs[ cpuad ].tf00846;

    case 2266: return &all_recs[ cpuad ].tf02266;
    case 2269: return &all_recs[ cpuad ].tf02269;
    case 2270: return &all_recs[ cpuad ].tf02270;
    case 2271: return &all_recs[ cpuad ].tf02271;
    case 2272: return &all_recs[ cpuad ].tf02272;
    case 2276: return &all_recs[ cpuad ].tf02276;
    case 2324: return &all_recs[ cpuad ].tf02324;
    case 2326: return &all_recs[ cpuad ].tf02326;

    // Device tracing...

    case  423: return &all_recs[ cpuad ].tf00423;
    case  424: return &all_recs[ cpuad ].tf00424;
    case  425: return &all_recs[ cpuad ].tf00425;
    case  426: return &all_recs[ cpuad ].tf00426;
    case  427: return &all_recs[ cpuad ].tf00427;
    case  428: return &all_recs[ cpuad ].tf00428;
    case  429: return &all_recs[ cpuad ].tf00429;
    case  430: return &all_recs[ cpuad ].tf00430;
    case  431: return &all_recs[ cpuad ].tf00431;
    case  432: return &all_recs[ cpuad ].tf00432;
    case  433: return &all_recs[ cpuad ].tf00433;
    case  434: return &all_recs[ cpuad ].tf00434;
    case  435: return &all_recs[ cpuad ].tf00435;
    case  436: return &all_recs[ cpuad ].tf00436;
    case  437: return &all_recs[ cpuad ].tf00437;
    case  438: return &all_recs[ cpuad ].tf00438;
    case  439: return &all_recs[ cpuad ].tf00439;
    case  440: return &all_recs[ cpuad ].tf00440;
    case  441: return &all_recs[ cpuad ].tf00441;
    case  442: return &all_recs[ cpuad ].tf00442;

    case  516: return &all_recs[ cpuad ].tf00516;
    case  517: return &all_recs[ cpuad ].tf00517;
    case  518: return &all_recs[ cpuad ].tf00518;
    case  519: return &all_recs[ cpuad ].tf00519;
    case  520: return &all_recs[ cpuad ].tf00520;

    case 1300: return &all_recs[ cpuad ].tf01300;
    case 1301: return &all_recs[ cpuad ].tf01301;
    case 1304: return &all_recs[ cpuad ].tf01304;
    case 1305: return &all_recs[ cpuad ].tf01305;
    case 1306: return &all_recs[ cpuad ].tf01306;
    case 1307: return &all_recs[ cpuad ].tf01307;
    case 1308: return &all_recs[ cpuad ].tf01308;
    case 1309: return &all_recs[ cpuad ].tf01309;
    case 1310: return &all_recs[ cpuad ].tf01310;
    case 1311: return &all_recs[ cpuad ].tf01311;
    case 1312: return &all_recs[ cpuad ].tf01312;
    case 1313: return &all_recs[ cpuad ].tf01313;
    case 1315: return &all_recs[ cpuad ].tf01315;
    case 1316: return &all_recs[ cpuad ].tf01316;
    case 1317: return &all_recs[ cpuad ].tf01317;
    case 1318: return &all_recs[ cpuad ].tf01318;
    case 1320: return &all_recs[ cpuad ].tf01320;
    case 1321: return &all_recs[ cpuad ].tf01321;
    case 1329: return &all_recs[ cpuad ].tf01329;
    case 1330: return &all_recs[ cpuad ].tf01330;
    case 1331: return &all_recs[ cpuad ].tf01331;
    case 1332: return &all_recs[ cpuad ].tf01332;
    case 1333: return &all_recs[ cpuad ].tf01333;
    case 1334: return &all_recs[ cpuad ].tf01334;
    case 1336: return &all_recs[ cpuad ].tf01336;

    default: CRASH(); UNREACHABLE_CODE( return NULL );
    }
}

/*-------------------------------------------------------------------*/
/*         Return CPU type number string for printing                */
/*-------------------------------------------------------------------*/

// (Note: '5' because e.g. "CPnn" = 4 chars + NULL)
static char ptyp_strs[ MAX_CPU_ENGS ][ 5 ];

static const char* ptyp_str( BYTE cpuad )
{
    const char* cpnn = ptyp_strs[ cpuad ];
    return cpnn;
}

/*-------------------------------------------------------------------*/
/*        Store the PSW according to runtime architecture            */
/*-------------------------------------------------------------------*/
static void tf_store_psw( PSW* psw, QWORD* qw, BYTE arch_mode )
{
    BYTE*  addr;
    int    arch;

    addr = (BYTE*) qw;

    switch (arch_mode)
    {
#if defined(     _370 )
        case ARCH_370_IDX: arch = 370; break;
#endif
#if defined(     _390 )
        case ARCH_390_IDX: arch = 390; break;
#endif
#if defined(     _900 )
        case ARCH_900_IDX: arch = 900; break;
#endif
        default: CRASH(); UNREACHABLE_CODE( return );
    }

    if (arch == 370)
        STORE_DW( addr + 0, do_make_psw64( psw, psw->ilc, arch, !ECMODE( psw )));
    else
        STORE_DW( addr + 0, do_make_psw64( psw, psw->ilc, arch, false ));

    if (arch == 900)
        STORE_DW( addr + 8, psw->IA_G );
}

/*-------------------------------------------------------------------*/
/*       Sort tidtab into ascending thread name sequence             */
/*-------------------------------------------------------------------*/
static int sort_tidtab_by_thrdname( const void* a, const void* b )
{
    const TIDTAB* tab_a = (const TIDTAB*)a;
    const TIDTAB* tab_b = (const TIDTAB*)b;
    int rc = strncmp( tab_a->thrdname, tab_b->thrdname, sizeof( tab_a->thrdname ));
    if (!rc)
        rc = (tab_a->tidnum - tab_b->tidnum);
    return rc;
}

/*-------------------------------------------------------------------*/
/*       Sort tidtab into ascending thread-id number sequence        */
/*-------------------------------------------------------------------*/
static int sort_tidtab_by_tidnum( const void* a, const void* b )
{
    const TIDTAB* tab_a = (const TIDTAB*)a;
    const TIDTAB* tab_b = (const TIDTAB*)b;
    return (tab_a->tidnum - tab_b->tidnum);
}

/*-------------------------------------------------------------------*/
/*               Save threading information                          */
/*-------------------------------------------------------------------*/
static void tf_save_tidinfo( const TFHDR* hdr )
{
    TIDTAB* my_tidptr = tidtab;
    int i;

    /* Check if we already have this thread in our table */
    for (i=0; i < numtids; ++i)
    {
        if (my_tidptr[i].tidnum > hdr->tidnum)
            break; // (past it; not in table)

        if (my_tidptr[i].tidnum == hdr->tidnum)
            return; // (already in our table)
    }

    /* Add this new entry to our table */
    tidtab = realloc( tidtab, sizeof( TIDTAB ) * (numtids + 1 ));
    tidtab[ numtids ].tidnum = hdr->tidnum;
    STRLCPY( tidtab[ numtids ].thrdname, hdr->thrdname );
    numtids += 1;

    /* Keep the table in ascending thread-id number order */
    qsort( tidtab, numtids, sizeof( TIDTAB ), sort_tidtab_by_tidnum );
}

/******************************************************************************/
/*                                                                            */
/*                     RECORD FILTERING FUNCTIONS                             */
/*                                                                            */
/******************************************************************************/

/*-------------------------------------------------------------------*/
/*                  Filter by record count                           */
/*-------------------------------------------------------------------*/
static bool is_recnum_wanted()
{
    return (recnum >= fromrec && recnum <= torec);
}

/*-------------------------------------------------------------------*/
/*                  Filter by message number                         */
/*-------------------------------------------------------------------*/
static bool is_msgnum_wanted( int msgnum )
{
    int i;

    if (!nMsgMOPT)          // If not filtering by message number
        return true;        // Then all messages are wanted

    for (i=0; i < nMsgMOPT; ++i)
        if (msgnum == (int)pMsgMOPT[i].opt1)
            return true;

    return false;           // It's not one they're interested in
}

/*-------------------------------------------------------------------*/
/*                        Filter by CPU                              */
/*-------------------------------------------------------------------*/
static bool is_cpu_wanted( BYTE cpu )
{
    return (0
        || cpu_map == 0                 // (all CPUs)
        || cpu_map & CPU_BIT( cpu )     // (this CPU)
    );
}

/*-------------------------------------------------------------------*/
/*                  Filter by date/time                              */
/*-------------------------------------------------------------------*/
static bool is_time_wanted( TIMEVAL* pTV )
{
    if (0
        || beg_tim.tv_sec       // if a
        || beg_tim.tv_usec      // ... time
        || end_tim.tv_sec       // ...... range
        || end_tim.tv_usec      // .........was specified
    )
    {
        /* Then we need to check further */

        static U64 max_past = 0;    // (usecs)
               U64 past_beg;        // (usecs; work)
               TIMEVAL dif_tim;     // (work)

        if (!max_past)              // (first time here?)
        {
            /* Calculate requested time range in usecs */
            timeval_subtract( &beg_tim, &end_tim, &dif_tim );
            max_past = ((U64)dif_tim.tv_sec * 1000000) + dif_tim.tv_usec;
        }

        /* Calculate how far past begin time this record's time is */
        if (timeval_subtract( &beg_tim, pTV, &dif_tim ) < 0)
            return false; // (we haven't reached begin time yet)

        past_beg = ((U64)dif_tim.tv_sec * 1000000) + dif_tim.tv_usec;

        /* Not interested if outside of specified range */
        if (past_beg > max_past)
            return false;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*              Perform storage address filtering                    */
/*-------------------------------------------------------------------*/
static bool is_wanted_storage( U64 want_addr, U64 want_amt,
                               U64 inst_addr, U64 inst_amt )
{
    bool within_want_range =
    (0
     || (1
         && inst_addr >=  want_addr
         && inst_addr <= (want_addr + want_amt - 1)
        )
     || (1
         && (inst_addr + inst_amt - 1) >=  want_addr
         && (inst_addr + inst_amt - 1) <= (want_addr + want_amt - 1)
        )
    );
    return within_want_range;
}

//---------------------------------------------------------------------

static bool is_wanted_operand( MOPT* pMOPT, TFOP* pTFOP )
{
    U64*  pinst_addr;
    U64    inst_amt;
    U64    want_addr;
    U64    want_amt;

    /* Make sure we even HAVE a storage address to examine! */
    if (0
        || (1
            && pMOPT->isreal    // Real address filtering specified?
            && pTFOP->xcode     // But no real address available?
           )
        || !(inst_amt = pTFOP->amt)
    )
        return false;

    pinst_addr = pMOPT->isreal ? &pTFOP->raddr : &pTFOP->vaddr;

    want_addr = pMOPT->opt1;
    want_amt  = pMOPT->isrange ? (pMOPT->opt2 - pMOPT->opt1) + 1 : 1;

    return is_wanted_storage( want_addr, want_amt, *pinst_addr, inst_amt );
}

//---------------------------------------------------------------------

static bool is_wanted_tf02326( TF02326* pTF02326 )
{
    int i;

    if (pTF02326->b1 >= 0)
    {
        for (i=0; i < nStorMOPT; ++i)
            if (is_wanted_operand( &pStorMOPT[i], &pTF02326->op1 ))
                return true;
    }

    if (pTF02326->b2 >= 0)
    {
        for (i=0; i < nStorMOPT; ++i)
            if (is_wanted_operand( &pStorMOPT[i], &pTF02326->op2 ))
                return true;
    }
    return false;
}

/*-------------------------------------------------------------------*/
/*             Perform instruction opcode filtering                  */
/*-------------------------------------------------------------------*/
static bool is_wanted_opcode( BYTE* p )
{
    int   i, n;
    BYTE  masked_inst[6];

    for (i=0; i < nInstMOPT; ++i)
    {
        /* Build masked copy of instruction */
        for (n=0; n < 6; ++n)
            masked_inst[n] = p[n] & pInstMOPT[i].imask[n];

        // Compare masked copy with what they're intrested in */
        if (memcmp( &masked_inst[0], &pInstMOPT[i].icode[0], 6 ) == 0)
            return true;
    }
    return false;
}

/*-------------------------------------------------------------------*/
/*                     Record filtering                              */
/*-------------------------------------------------------------------*/
static bool is_wanted( TFHDR* hdr )
{
    if (!is_recnum_wanted())
        return false;

    if (!is_msgnum_wanted( hdr->msgnum ))
        return false;

    if (!is_cpu_wanted( hdr->cpuad ))
        return false;

    if (!is_time_wanted( &hdr->tod ))
        return false;

    return true;
}

/*-------------------------------------------------------------------*/
/*                  Device (Unit) filtering                          */
/*-------------------------------------------------------------------*/
static bool is_devnum_wanted( U16 devnum )
{
    int i;

    for (i=0; i < nUnitMOPT; ++i)
    {
        if (pUnitMOPT[i].isrange)
        {
            if (1
                && devnum >= pUnitMOPT[i].opt1
                && devnum <= pUnitMOPT[i].opt2
            )
                return true;
        }
        else
        {
            if (devnum == pUnitMOPT[i].opt1)
                return true;
        }
    }
    return nUnitMOPT ? false: true;
}

/******************************************************************************/
/*                                                                            */
/*                        PRINTING FUNCTIONS                                  */
/*                                                                            */
/******************************************************************************/

/*-------------------------------------------------------------------*/
/*                      (helper functions)                           */
/*-------------------------------------------------------------------*/

/* For instruction (non-device) trace printing... */
static void tf_do_blank_sep( TFHDR* hdr )
{
    if (0
        || prvcpuad >= MAX_CPU_ENGS     // (prev was device record?)
        || hdr->cpuad != prvcpuad       // (or not same CPU?)
        || previnst                     // (or was "instruction"?)
    )
        printf("\n");                   // (separate from previous)

    // (else prv NOT dev, *AND* same cpu, *AND* not "instruction")

    prvcpuad  = hdr->cpuad;
    prvdevnum = hdr->devnum;
    previnst  = false;
}

//---------------------------------------------------------------------

/* For device trace printing... */
static void tf_dev_do_blank_sep( TFHDR* hdr )
{
    if (0
        || prvcpuad < MAX_CPU_ENGS      // (prev not device record?)
        || prvdevnum != hdr->devnum     // (or not for same device?)
        || hdr->msgnum == 1334          // (or ORB = start new I/O?)
    )
        printf("\n");                   // (separate from previous)

    prvcpuad  = hdr->cpuad;
    prvdevnum = hdr->devnum;
    previnst  = false;
}

/*-------------------------------------------------------------------*/
/*                      (helper macros)                              */
/*-------------------------------------------------------------------*/

/* For instruction trace printing... */
#define TF_FLOGMSG( _nnnn )                                           \
                                                                      \
    TF_FLOGMSG_HHC( HHC0 ## _nnnn )

#define TF_FLOGMSG_HHC( _hhc )                                        \
                                                                      \
    tf_do_blank_sep( &rec->rhdr );                                    \
    FLOGMSG( stdout, "%s " # _hhc "I " _hhc "\n", &timstr[ 11 ]


//---------------------------------------------------------------------

/* For device trace printing... */
#define TF_DEV_FLOGMSG( _nnnn )                                       \
                                                                      \
    TF_DEV_FLOGMSG_HHC( HHC0 ## _nnnn )

#define TF_DEV_FLOGMSG_HHC( _hhc )                                    \
                                                                      \
    tf_dev_do_blank_sep( &rec->rhdr );                                \
    FLOGMSG( stdout, "%s " # _hhc "I " _hhc "\n", &timstr[ 11 ]

// (same as above, but for "E" error message)
#define TF_DEV_FLOGMSG_E( _nnnn )                                     \
                                                                      \
    TF_DEV_FLOGMSG_HHC_E( HHC0 ## _nnnn )

// (same as TF_FLOGMSG_HHC, but for "E" error message)
#define TF_DEV_FLOGMSG_HHC_E( _hhc )                                  \
                                                                      \
    tf_dev_do_blank_sep( &rec->rhdr );                                \
    FLOGMSG( stdout, "%s " # _hhc "E " _hhc "\n", &timstr[ 11 ]

//---------------------------------------------------------------------

/* Used by device trace messages 1300-1336... */
#define PRINT_DEV_FUNC( _nnnn )                                       \
    PRINT_DEV_TF1( _nnnn )

/* Almost the entire device trace printing function,
   except for last few printf arguments...
*/
#define PRINT_DEV_TF1( _nnnn )                                        \
                                                                      \
    static inline void print_TF0 ## _nnnn( TF0 ## _nnnn* rec )        \
    {                                                                 \
        char timstr[ 64] = {0};                                       \
        FormatTIMEVAL( &rec->rhdr.tod, timstr, sizeof( timstr ));     \
                                                                      \
        TF_DEV_FLOGMSG( _nnnn ),                                      \
            rec->rhdr.lcss, rec->rhdr.devnum

//---------------------------------------------------------------------

/* Same as above but with thread-id & filename before remaining printf
   arguments. Used by device trace messages 0423-0442 and 0516-0520...
*/
#define PRINT_DEV_FUNC0( _nnnn )                                      \
    PRINT_DEV_TF0( _nnnn ), rec->filename

// (same as above, but for "E" error message)
#define PRINT_DEV_FUNC0_E( _nnnn )                                    \
    PRINT_DEV_TF0_E( _nnnn ), rec->filename

/* Almost the entire device trace printing function,
   except for last few printf arguments...
*/
#define PRINT_DEV_TF0( _nnnn )                                        \
                                                                      \
    static inline void print_TF0 ## _nnnn( TF0 ## _nnnn* rec )        \
    {                                                                 \
        char timstr[ 64] = {0};                                       \
        FormatTIMEVAL( &rec->rhdr.tod, timstr, sizeof( timstr ));     \
                                                                      \
        TF_DEV_FLOGMSG( _nnnn ),                                      \
        sys_ffmt >= TF_FMT2 ? rec->rhdr.tidnum : 0, rec->rhdr.lcss, rec->rhdr.devnum

// (same as above, but for "E" error message)
#define PRINT_DEV_TF0_E( _nnnn )                                      \
                                                                      \
    static inline void print_TF0 ## _nnnn( TF0 ## _nnnn* rec )        \
    {                                                                 \
        char timstr[ 64] = {0};                                       \
        FormatTIMEVAL( &rec->rhdr.tod, timstr, sizeof( timstr ));     \
                                                                      \
        TF_DEV_FLOGMSG_E( _nnnn ),                                    \
        sys_ffmt >= TF_FMT2 ? rec->rhdr.tidnum : 0, rec->rhdr.lcss, rec->rhdr.devnum

/*-------------------------------------------------------------------*/
/*                    print TFSYS record                             */
/*-------------------------------------------------------------------*/
static void print_TFSYS( TFSYS* sys, bool was_bigend )
{
    // PROGRAMMING NOTE: the TFSYS record passed to us was already
    // swapped into our native endian format before we were called.

    int i;
    BYTE ptyp;
    const char* str;
    char buffer[128] = {0};
    char inscnt[ 32] = {0};
    char devcnt[ 32] = {0};

    // "Format-%c trace file created by: %s"
    WRMSG( HHC03208, "I", sys->ffmt[3], sys->version );

    printf( "\n" );

    // "Trace %s: %s"
    FormatTIMEVAL( &sys->beg_tod, buffer, sizeof( buffer ));
    WRMSG( HHC03209, "I", "began", buffer );

    // "Trace %s: %s"
    FormatTIMEVAL( &sys->end_tod, buffer, sizeof( buffer ));
    WRMSG( HHC03209, "I", "ended", buffer );

    // "Trace count: instruction=%s records, device=%s records"
    fmt_S64( inscnt, (S64) sys->tot_ins );
    fmt_S64( devcnt, (S64) sys->tot_dev );
    WRMSG( HHC03211, "I", inscnt, devcnt );

    /* Build pre-formatted processor type strings for all CPUs */
    for (i=0; i < MAX_CPU_ENGS; ++i)
    {
        ptyp = sys->ptyp[ i ];
        str  = ptyp2short( ptyp );
        // (Note: '5' because e.g. "CPnn" = 4 chars + NULL)
        snprintf( ptyp_strs[ i ], 5, "%s%02X", str, i );
    }

    if (doendswap)
    {
        printf( "\n" );

        // "Endianness of %s = %s"
        WRMSG( HHC03221, "I", "file", was_bigend       ? "BIG" : "little" );
        WRMSG( HHC03221, "I", "host", are_big_endian() ? "BIG" : "little" );

        // "WARNING: possible performance impact due to endianness!"
        printf( "\n" );
        WRMSG( HHC03222, "W" );

        /* If stdout was redirected to a file (very likely true), but
           stderr was not (i.e. is still a terminal, also very likely
           true) then print the above messages again, but this time
           to stderr so that they can see them on their terminal.
        */
        if (!out_istty && err_istty)
        {
            // "Endianness of %s = %s"
            FWRMSG( stderr, HHC03221, "I", "file", was_bigend       ? "BIG" : "little" );
            FWRMSG( stderr, HHC03221, "I", "host", are_big_endian() ? "BIG" : "little" );

            // "WARNING: possible performance impact due to endianness!"
            FWRMSG( stderr, HHC03222, "W" );
        }
    }
}

/*-------------------------------------------------------------------*/
/*                Print General Purpose Registers                    */
/*-------------------------------------------------------------------*/
static inline void print_gr_regs( TF02269* rec )
{
    char tim [ 64 ]  = {0};     // "YYYY-MM-DD HH:MM:SS.uuuuuu"
    char pfx [ 64 ]  = {0};     // "16:22:47.745999 HHC02269I CP00:"
    char buf [ 128 ] = {0};     // " R0=0000000000000000 ..."

    int  i, r;                  // (work for iterating)
    DW*  gr;                    // (so GR_G/GR_L macros work right)

    gr = rec->gr;               // (so GR_G/GR_L macros work right)

    if (regsfirst && !noregs)
        printf("\n");           // (blank line before inst)

    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));
    MSGBUF( pfx, "%s HHC02269I %s:", &tim[ 11 ], ptyp_str( rec->rhdr.cpuad ));

    if (ARCH_900_IDX == rec->rhdr.arch_mode)
    {
        for (i=0, r=0; i < 4; r += 4, i++)
        {
            MSGBUF
            (
                buf,

                "%s"

                " R%1.1X=%16.16"PRIX64
                " R%1.1X=%16.16"PRIX64
                " R%1.1X=%16.16"PRIX64
                " R%1.1X=%16.16"PRIX64,

                pfx,

                r+0, GR_G( r+0 ),
                r+1, GR_G( r+1 ),
                r+2, GR_G( r+2 ),
                r+3, GR_G( r+3 )

            );

            FLOGMSG( stdout, "%s\n", buf );
        }
    }
    else
    {
        for (i=0, r=0; i < 4; r += 4, i++)
        {
            MSGBUF
            (
                buf,

                "%s"

                " GR%2.2d=%8.8"PRIX32
                " GR%2.2d=%8.8"PRIX32
                " GR%2.2d=%8.8"PRIX32
                " GR%2.2d=%8.8"PRIX32,

                pfx,

                r+0, GR_L( r+0 ),
                r+1, GR_L( r+1 ),
                r+2, GR_L( r+2 ),
                r+3, GR_L( r+3 )
            );

            FLOGMSG( stdout, "%s\n", buf );
        }
    }
}

/*-------------------------------------------------------------------*/
/*                  Print Control Registers                          */
/*-------------------------------------------------------------------*/
static inline void print_cr_regs( TF02271* rec )
{
    char tim [ 64 ]  = {0};     // "YYYY-MM-DD HH:MM:SS.uuuuuu"
    char pfx [ 64 ]  = {0};     // "16:22:47.745999 HHC02269I CP00:"
    char buf [ 128 ] = {0};     // " R0=0000000000000000 ..."

    int  i, r;                  // (work for iterating)
    DW*  gr;                    // (so GR_G/GR_L macros work right)

    gr = rec->cr;               // (so GR_G/GR_L macros work right)

    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));
    MSGBUF( pfx, "%s HHC02271I %s:", &tim[ 11 ], ptyp_str( rec->rhdr.cpuad ));

    if (ARCH_900_IDX == rec->rhdr.arch_mode)
    {
        for (i=0, r=0; i < 4; r += 4, i++)
        {
            MSGBUF
            (
                buf,

                "%s"

                " C%1.1X=%16.16"PRIX64
                " C%1.1X=%16.16"PRIX64
                " C%1.1X=%16.16"PRIX64
                " C%1.1X=%16.16"PRIX64,

                pfx,

                r+0, GR_G( r+0 ),
                r+1, GR_G( r+1 ),
                r+2, GR_G( r+2 ),
                r+3, GR_G( r+3 )

            );

            FLOGMSG( stdout, "%s\n", buf );
        }
    }
    else
    {
        for (i=0, r=0; i < 4; r += 4, i++)
        {
            MSGBUF
            (
                buf,

                "%s"

                " CR%2.2d=%8.8"PRIX32
                " CR%2.2d=%8.8"PRIX32
                " CR%2.2d=%8.8"PRIX32
                " CR%2.2d=%8.8"PRIX32,

                pfx,

                r+0, GR_L( r+0 ),
                r+1, GR_L( r+1 ),
                r+2, GR_L( r+2 ),
                r+3, GR_L( r+3 )
            );

            FLOGMSG( stdout, "%s\n", buf );
        }
    }
}

/*-------------------------------------------------------------------*/
/*                  Print Access Registers                           */
/*-------------------------------------------------------------------*/
static inline void print_ar_regs( TF02272* rec )
{
    char tim [ 64 ]  = {0};     // "YYYY-MM-DD HH:MM:SS.uuuuuu"
    char pfx [ 64 ]  = {0};     // "16:22:47.745999 HHC02269I CP00:"
    char buf [ 128 ] = {0};     // " R0=0000000000000000 ..."

    int  i, r;                  // (work for iterating)

    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));
    MSGBUF( pfx, "%s HHC02272I %s:", &tim[ 11 ], ptyp_str( rec->rhdr.cpuad ));

    for (i=0, r=0; i < 4; r += 4, i++)
    {
        MSGBUF
        (
            buf,

            "%s"

            " AR%2.2d=%8.8"PRIX32
            " AR%2.2d=%8.8"PRIX32
            " AR%2.2d=%8.8"PRIX32
            " AR%2.2d=%8.8"PRIX32,

            pfx,

            r+0, rec->ar[ r+0 ],
            r+1, rec->ar[ r+1 ],
            r+2, rec->ar[ r+2 ],
            r+3, rec->ar[ r+3 ]
        );

        FLOGMSG( stdout, "%s\n", buf );
    }
}

/*-------------------------------------------------------------------*/
/*                Print Floating Point Registers                     */
/*-------------------------------------------------------------------*/
static inline void print_fpr_regs( TF02270* rec )
{
    char tim [ 64 ]  = {0};     // "YYYY-MM-DD HH:MM:SS.uuuuuu"
    char pfx [ 64 ]  = {0};     // "16:22:47.745999 HHC02269I CP00:"


    char buf [ 128 ] = {0};     // " R0=0000000000000000 ..."

    int  i, r;                  // (work for iterating)
    DW*  gr;                    // (so GR_G/GR_L macros work right)

    gr = rec->fpr;              // (so GR_G/GR_L macros work right)

    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));
    MSGBUF( pfx, "%s HHC02270I %s:", &tim[ 11 ], ptyp_str( rec->rhdr.cpuad ));

    if (rec->afp)
    {
        for (i=0, r=0; i < 4; r += 4, i++)
        {
            MSGBUF
            (
                buf,

                "%s"

                " F%1.1X=%16.16"PRIX64
                " F%1.1X=%16.16"PRIX64
                " F%1.1X=%16.16"PRIX64
                " F%1.1X=%16.16"PRIX64,

                pfx,

                r+0, GR_G( r+0 ),
                r+1, GR_G( r+1 ),
                r+2, GR_G( r+2 ),
                r+3, GR_G( r+3 )
            );

            FLOGMSG( stdout, "%s\n", buf );
        }
    }
    else
    {
        MSGBUF
        (
            buf,

            "%s"

            " F%1.1X=%16.16"PRIX64
            " F%1.1X=%16.16"PRIX64
            " F%1.1X=%16.16"PRIX64
            " F%1.1X=%16.16"PRIX64,

            pfx,

            0, GR_G( 0 ),
            2, GR_G( 2 ),
            4, GR_G( 4 ),
            6, GR_G( 6 )
        );

        FLOGMSG( stdout, "%s\n", buf );
    }
}

/*-------------------------------------------------------------------*/
/*            Print Floating Point Control Register                  */
/*-------------------------------------------------------------------*/
static inline void print_fpc_reg( TF02276* rec )
{
    if (rec->afp)
    {
        char timstr[ 64 ] = {0};   // "YYYY-MM-DD HH:MM:SS.uuuuuu"
        FormatTIMEVAL( &rec->rhdr.tod, timstr, sizeof( timstr ));
        // "Floating point control register: %08"PRIX32
        TF_FLOGMSG( 2276 ),
            rec->fpc );
    }
}

/*-------------------------------------------------------------------*/
/*                Print Vector Registers                             */
/*-------------------------------------------------------------------*/
static inline void print_vr_regs( TF02266* rec )
{
    char tim[64] = {0};         // "YYYY-MM-DD HH:MM:SS.uuuuuu"
    char pfx[64] = {0};         // "16:22:47.745999 HHC02269I CP00:"

    int  i;                     // (work for iterating)

    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));
    MSGBUF( pfx, "%s HHC02266I %s:", &tim[11], ptyp_str( rec->rhdr.cpuad ));

    for (i = 0; i < 32; i += 2)
    {
        FLOGMSG( stdout, "%s VR%02d=%016" PRIx64 ".%016" PRIx64 " VR%02d=%016" PRIx64 ".%016" PRIx64 "\n",
            pfx,
            i,     rec->VR_D( i,   0), rec->VR_D( i,   1),
            i + 1, rec->VR_D( i+1, 0), rec->VR_D( i+1, 1)
        );
    }

}

/*-------------------------------------------------------------------*/
/*                Print ALL Available Registers                      */
/*-------------------------------------------------------------------*/
static inline void print_all_available_regs( BYTE cpuad )
{
    // General Purpose Registers
    if (all_recs[ cpuad ].gotmask  &   GOT_TF02269)
        print_gr_regs(  &all_recs[ cpuad ].tf02269 );

    // Control Registers
    if (all_recs[ cpuad ].gotmask  &   GOT_TF02271)
        print_cr_regs(  &all_recs[ cpuad ].tf02271 );

    // Access Registers
    if (all_recs[ cpuad ].gotmask  &   GOT_TF02272)
        print_ar_regs(  &all_recs[ cpuad ].tf02272 );

    // Floating Point Control Register
    if (all_recs[ cpuad ].gotmask  &   GOT_TF02276)
        print_fpc_reg(  &all_recs[ cpuad ].tf02276 );

    // Floating Point Registers
    if (all_recs[ cpuad ].gotmask  &   GOT_TF02270)
        print_fpr_regs( &all_recs[ cpuad ].tf02270 );

    // Vector Registers
    if (all_recs[ cpuad ].gotmask  &   GOT_TF02266)
        print_vr_regs(  &all_recs[ cpuad ].tf02266 );
}

/*-------------------------------------------------------------------*/
/*                   Format PSW string                               */
/*-------------------------------------------------------------------*/
static const char* fmt_psw_str( PSW* psw, BYTE arch_mode )
{
    QWORD  qword;
    BYTE*  b = (BYTE*) &qword;

    static char   psw_str [ 64 ];
           char   psw_str2[ 32 ];

    tf_store_psw( psw, &qword, arch_mode );

    if (ARCH_900_IDX == arch_mode)
        MSGBUF( psw_str2, " %2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X",
                b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15] );
    else
        psw_str2[0] = 0;

    MSGBUF( psw_str, "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%s",
            b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], psw_str2 );

    return psw_str;
}

/*-------------------------------------------------------------------*/
/*              Format PSW= and INST= of trace                       */
/*-------------------------------------------------------------------*/
static const char* fmt_psw_inst_str( PSW* psw, BYTE arch_mode, BYTE* inst, BYTE real_ilc )
{
    static char psw_inst[ 64 ] = {0};
           char inst_str[ 32 ] = {0};

    if (2 == real_ilc)
        MSGBUF( inst_str, "%2.2X%2.2X        ",
            inst[0], inst[1] );
    else if (4 == real_ilc)
        MSGBUF( inst_str, "%2.2X%2.2X%2.2X%2.2X    ",
            inst[0], inst[1], inst[2], inst[3] );
    else if (6 == real_ilc)
        MSGBUF( inst_str, "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X",
            inst[0], inst[1], inst[2], inst[3], inst[4], inst[5] );
    else
        CRASH();

    MSGBUF( psw_inst, "PSW=%s INST=%s",
        fmt_psw_str( psw, arch_mode ), inst_str );

    return psw_inst;
}

/*-------------------------------------------------------------------*/
/*                Print Wait State PSW Record                        */
/*-------------------------------------------------------------------*/
static inline void print_800_wait_state_psw( TF00800* rec )
{
    char timstr[ 64 ] = {0};    // "YYYY-MM-DD HH:MM:SS.uuuuuu"

    FormatTIMEVAL( &rec->rhdr.tod, timstr, sizeof( timstr ));

    // "Processor %s%02X: loaded wait state PSW %s"
    tf_do_blank_sep( &rec->rhdr );
    FLOGMSG( stdout,

        "%s HHC00800I Processor %s: loaded wait state PSW %s\n",

        &timstr[ 11 ],
        ptyp_str( rec->rhdr.cpuad ),
        fmt_psw_str( &rec->psw, rec->rhdr.arch_mode )
    );
}

/*-------------------------------------------------------------------*/
/*               Print Program Interrupt Record                      */
/*-------------------------------------------------------------------*/
static inline void print_801_program_interrupt( TF00801* rec )
{
    char timstr [  64 ]  = {0};   // "YYYY-MM-DD HH:MM:SS.uuuuuu"
    char dxcstr [   8 ]  = {0};   // data exception code if PGM_DATA_EXCEPTION
    char whystr [ 256 ]  = {0};   // TXF "why" string if txf pgmint

    FormatTIMEVAL( &rec->rhdr.tod, timstr, sizeof( timstr ));

    if ((rec->pcode & 0xFF) == PGM_DATA_EXCEPTION)
       MSGBUF( dxcstr, " DXC=%2.2X", rec->dxc );

    if ((rec->pcode & PGM_TXF_EVENT) && rec->why)
        txf_why_str( whystr, sizeof( whystr ), rec->why );

    // "Processor %s%02X: %s%s%s interruption code %4.4X ilc %d%s%s"
    tf_do_blank_sep( &rec->rhdr );
    FLOGMSG( stdout,

        "%s HHC00801I Processor %s: %s%s interruption code %4.4X ilc %d%s%s\n",

        &timstr[ 11 ],
        ptyp_str( rec->rhdr.cpuad ),
        rec->sie ? "SIE: " : "",
        PIC2Name( rec->pcode & 0xFF ),
        rec->pcode,
        rec->ilc,
        dxcstr,
        whystr
    );
}

/*-------------------------------------------------------------------*/
/*                  Print PER Event Record                           */
/*-------------------------------------------------------------------*/
static inline void print_802_per_event( TF00802* rec )
{
    char tim [ 64 ]  = {0};  // "YYYY-MM-DD HH:MM:SS.uuuuuu"
    char percname[ 32 ] = {0};

    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));

    perc2name( rec->perc, percname, sizeof( percname ));

    tf_do_blank_sep( &rec->rhdr );

    if (ARCH_900_IDX == rec->rhdr.arch_mode)
    {
        // "Processor %s%02X: PER event: code %4.4X perc %2.2X=%s addr "F_VADR
        FLOGMSG( stdout,
            "%s HHC00802I Processor %s: PER event: code %4.4X perc %2.2X=%s addr %16.16"PRIX64"\n",
            &tim[ 11 ], ptyp_str( rec->rhdr.cpuad ),
            rec->pcode, rec->perc, percname, rec->ia );
    }
    else // (390/370)
    {
        // "Processor %s%02X: PER event: code %4.4X perc %2.2X=%s addr "F_VADR
        FLOGMSG( stdout,
            "%s HHC00802I Processor %s: PER event: code %4.4X perc %2.2X=%s addr %8.8"PRIX32"\n",
            &tim[ 11 ], ptyp_str( rec->rhdr.cpuad ),
            rec->pcode, rec->perc, percname, (U32) rec->ia );
    }
}

/*-------------------------------------------------------------------*/
/*             Print Program Interrupt Loop Record                   */
/*-------------------------------------------------------------------*/
static inline void print_803_pgm_int_loop( TF00803* rec )
{
    char tim [ 64 ]  = {0};  // "YYYY-MM-DD HH:MM:SS.uuuuuu"

    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));

    // "Processor %s%02X: program interrupt loop PSW %s"
    tf_do_blank_sep( &rec->rhdr );
    FLOGMSG( stdout,
        "%s HHC00803I Processor %s: program interrupt loop PSW %s\n",
        &tim[ 11 ], ptyp_str( rec->rhdr.cpuad ), rec->str );
}

/*-------------------------------------------------------------------*/
/*             Print I/O interrupt (S/370) Record                    */
/*-------------------------------------------------------------------*/
static inline void print_804_io_rupt_370( TF00804* rec )
{
    char tim [ 64 ]  = {0};  // "YYYY-MM-DD HH:MM:SS.uuuuuu"

    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));

    // "Processor %s%02X: I/O interrupt code %1.1X:%4.4X CSW %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X"
    tf_do_blank_sep( &rec->rhdr );
    FLOGMSG( stdout,
        "%s HHC00804I Processor %s: I/O interrupt code %1.1X:%4.4X "
        "CSW %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X\n",
        &tim[ 11 ], ptyp_str( rec->rhdr.cpuad ),
        rec->rhdr.lcss, rec->ioid, rec->csw[0], rec->csw[1], rec->csw[2], rec->csw[3],
                              rec->csw[4], rec->csw[5], rec->csw[6], rec->csw[7] );
}

/*-------------------------------------------------------------------*/
/*             Print Machine Check Interrupt Record                  */
/*-------------------------------------------------------------------*/
static inline void print_807_machine_check_interrupt( TF00807* rec )
{
    char tim [ 64 ]  = {0};  // "YYYY-MM-DD HH:MM:SS.uuuuuu"

    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));

    // "Processor %s%02X: machine check code %16.16"PRIu64
    tf_do_blank_sep( &rec->rhdr );
    FLOGMSG( stdout,
        "%s HHC00807I Processor %s: machine check code %16.16"PRIu64"\n",
        &tim[ 11 ], ptyp_str( rec->rhdr.cpuad ), rec->mcic );
}

/*-------------------------------------------------------------------*/
/*                Print Store Status Record                          */
/*-------------------------------------------------------------------*/
static inline void print_808_store_status( TF00808* rec )
{
    char tim [ 64 ]  = {0};  // "YYYY-MM-DD HH:MM:SS.uuuuuu"

    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));

    // "Processor %s%02X: store status completed"
    tf_do_blank_sep( &rec->rhdr );
    FLOGMSG( stdout,
        "%s HHC00808I Processor %s: store status completed\n",
        &tim[ 11 ], ptyp_str( rec->rhdr.cpuad ) );
}

/*-------------------------------------------------------------------*/
/*             Print Disabled Wait State Record                      */
/*-------------------------------------------------------------------*/
static inline void print_809_disabled_wait( TF00809* rec )
{
    char tim [ 64 ]  = {0};  // "YYYY-MM-DD HH:MM:SS.uuuuuu"

    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));

    // "Processor %s%02X: disabled wait state %s"
    tf_do_blank_sep( &rec->rhdr );
    FLOGMSG( stdout,
        "%s HHC00809I Processor %s: disabled wait state %s\n",
        &tim[ 11 ], ptyp_str( rec->rhdr.cpuad ), rec->str );
}

/*-------------------------------------------------------------------*/
/*             Print Architecture Mode Record                        */
/*-------------------------------------------------------------------*/
static inline void print_811_arch_mode( TF00811* rec )
{
    char tim [ 64 ]  = {0};  // "YYYY-MM-DD HH:MM:SS.uuuuuu"

    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));

    // "Processor %s%02X: architecture mode %s"
    tf_do_blank_sep( &rec->rhdr );
    FLOGMSG( stdout,
        "%s HHC00811I Processor %s: architecture mode %s\n",
        &tim[ 11 ], ptyp_str( rec->rhdr.cpuad ), rec->archname );
}

/*-------------------------------------------------------------------*/
/*        Print Vector Facility Online (370/390) Record              */
/*-------------------------------------------------------------------*/
static inline void print_812_vect_online_370( TF00812* rec )
{
    char tim [ 64 ]  = {0};  // "YYYY-MM-DD HH:MM:SS.uuuuuu"

    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));

    // "Processor %s%02X: vector facility online"
    tf_do_blank_sep( &rec->rhdr );
    FLOGMSG( stdout,
        "%s HHC00812I Processor %s: vector facility online\n",
        &tim[ 11 ], ptyp_str( rec->rhdr.cpuad ) );
}

/*-------------------------------------------------------------------*/
/*              Print External Interrupt Record                      */
/*-------------------------------------------------------------------*/
static inline void print_840_ext_rupt( TF00840* rec )
{
    char tim [ 64 ]  = {0};  // "YYYY-MM-DD HH:MM:SS.uuuuuu"
    char pfx [ 64 ]  = {0};

    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));

    MSGBUF( pfx, "%s HHC0084?I Processor %s: External interrupt",
        &tim[ 11 ], ptyp_str( rec->rhdr.cpuad ));

    tf_do_blank_sep( &rec->rhdr );

    switch (rec->icode)
    {
        case EXT_INTERRUPT_KEY_INTERRUPT:

            // "Processor %s%02X: External interrupt: interrupt key"
            pfx[23] = '0'; // HHC00840I
            FLOGMSG( stdout, "%s: interrupt key\n", pfx );
            break;

        case EXT_CLOCK_COMPARATOR_INTERRUPT:

            // "Processor %s%02X: External interrupt: clock comparator"
            pfx[23] = '1'; // HHC00841I
            FLOGMSG( stdout, "%s: clock comparator\n", pfx );
            break;

        case EXT_CPU_TIMER_INTERRUPT:

            // "Processor %s%02X: External interrupt: CPU timer=%16.16"PRIX64
            pfx[23] = '2'; // HHC00842I
            FLOGMSG( stdout, "%s: CPU timer=%16.16"PRIX64"\n", pfx, rec->cpu_timer );
            break;

        case EXT_INTERVAL_TIMER_INTERRUPT:

            // "Processor %s%02X: External interrupt: interval timer"
            pfx[23] = '3'; // HHC00843I
            FLOGMSG( stdout, "%s: interval timer\n", pfx );
            break;

        default:

            // "Unsupported external interrupt: processor %s, code %4.4"PRIX16
            FWRMSG( stderr, HHC03218, "W", ptyp_str( rec->rhdr.cpuad ), rec->icode );
            break;
    }
}

/*-------------------------------------------------------------------*/
/*              Print Block I/O Interrupt Record                     */
/*-------------------------------------------------------------------*/
static inline void print_844_blkio_rupt( TF00844* rec )
{
    char tim [ 64 ]  = {0};  // "YYYY-MM-DD HH:MM:SS.uuuuuu"

    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));

    // "Processor %s%02X: %1d:%04X: processing block I/O interrupt: code %4.4X parm %16.16"PRIX64" status %2.2X subcode %2.2X"
    tf_do_blank_sep( &rec->rhdr );
    FLOGMSG( stdout,
        "%s HHC00844I Processor %s: %1d:%04X: processing block I/O interrupt: code %4.4X parm %16.16"PRIX64" status %2.2X subcode %2.2X\n",
        &tim[ 11 ], ptyp_str( rec->rhdr.cpuad ), rec->rhdr.lcss, rec->rhdr.devnum,
        rec->servcode, rec->bioparm, rec->biostat, rec->biosubcd );
}

/*-------------------------------------------------------------------*/
/*           Print Block I/O External interrupt Record               */
/*-------------------------------------------------------------------*/
static inline void print_845_blkio_ext_rupt( TF00845* rec )
{
    char tim [ 64 ]  = {0};  // "YYYY-MM-DD HH:MM:SS.uuuuuu"

    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));

    tf_do_blank_sep( &rec->rhdr );

    if (rec->biosubcd == 0x07)
        // "Processor %s%02X: External interrupt: block I/O %s"
        FLOGMSG( stdout,
            "%s HHC00845I Processor %s: External interrupt: block I/O %16.16"PRIX64"\n",
            &tim[ 11 ], ptyp_str( rec->rhdr.cpuad ), (U64)rec->bioparm );
    else
        // "Processor %s%02X: External interrupt: block I/O %s"
        FLOGMSG( stdout,
            "%s HHC00845I Processor %s: External interrupt: block I/O %8.8"PRIX32"\n",
            &tim[ 11 ], ptyp_str( rec->rhdr.cpuad ), (U32)rec->bioparm );
}

/*-------------------------------------------------------------------*/
/*         Print Service Signal External Interrupt Record            */
/*-------------------------------------------------------------------*/
static inline void print_846_srvsig_ext_rupt( TF00846* rec )
{
    char tim [ 64 ]  = {0};  // "YYYY-MM-DD HH:MM:SS.uuuuuu"

    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));

    // "Processor %s%02X: External interrupt: service signal %8.8X"
    tf_do_blank_sep( &rec->rhdr );
    FLOGMSG( stdout,
        "%s HHC00846I Processor %s: External interrupt: service signal %8.8X\n",
        &tim[ 11 ], ptyp_str( rec->rhdr.cpuad ), rec->servparm );
}

/*-------------------------------------------------------------------*/
/*          Format instruction assembler mnemonic, etc.              */
/*-------------------------------------------------------------------*/
static const char* fmt_inst_name( BYTE arch_mode, BYTE* inst )
{
    static char mnemonic_etc[ 128 ] = {0};
    PRINT_INST( arch_mode, inst, mnemonic_etc );
    return mnemonic_etc;
}

/*-------------------------------------------------------------------*/
/*                  Print Operand Storage                            */
/*-------------------------------------------------------------------*/
static inline void print_op_stor( const char* pfx, BYTE arch_mode, BYTE real_mode, TFOP* op )
{
    char vadr[ 32 ];
    char stor[ 64 ];

    /* First, format just the virtual address part... */
    if (real_mode)
        vadr[0] = 0;
    else
    {
        if (ARCH_900_IDX == arch_mode)
            MSGBUF( vadr, "V:%16.16"PRIX64":", op->vaddr );
        else
            MSGBUF( vadr, "V:%8.8"PRIX32":", (U32)op->vaddr );
    }

    /* Next, print the real address and storage part... */
    if (op->xcode)
    {
        char pfx_and_vadr[128];

        MSGBUF(  pfx_and_vadr, "%s %s ", pfx, vadr );

        RTRIM(   pfx_and_vadr );        // (vadr likely empty)
        STRLCAT( pfx_and_vadr, " " );   // (blank is expected)

        FLOGMSG( stdout, "%sTranslation exception %04"PRIX16" (%s)\n",
            pfx_and_vadr, op->xcode, PIC2Name( op->xcode ));
    }
    else
    {
        char hbuf[64] = {0};   // hex buffer
        char cbuf[17] = {0};   // char buffer
        BYTE i, c;

        // (table where to place our null in hbuf)
        static const BYTE hbuf_end[ 17 ] = {  0,
                                              2,  4,  6,  8,
                                             11, 13, 15, 17,
                                             20, 22, 24, 26,
                                             29, 31, 33, 35 };

        if (op->amt < 1 || op->amt > 16) CRASH(); // (sanity check)

        // (more efficient to print everything than to loop)
        MSGBUF( hbuf,

            "%02X%02X%02X%02X "
            "%02X%02X%02X%02X "
            "%02X%02X%02X%02X "
            "%02X%02X%02X%02X",

            op->stor[ 0], op->stor[ 1], op->stor[ 2], op->stor[ 3],
            op->stor[ 4], op->stor[ 5], op->stor[ 6], op->stor[ 7],
            op->stor[ 8], op->stor[ 9], op->stor[10], op->stor[11],
            op->stor[12], op->stor[13], op->stor[14], op->stor[15]
        );

        hbuf[ hbuf_end[ op->amt ]] = 0;

        // (more efficient to loop on this one!)
        for (i=0; i < op->amt; i++)
        {
            c = guest_to_host( op->stor[ i ]);
            if (!isprint( (unsigned char)c )) c = '.';
            cbuf[ i ] = c;
        }

        cbuf[ op->amt ] = 0;

        /* Now format both fixed-width pieces together */
        MSGBUF( stor, "%-36.36s %-16.16s", hbuf, cbuf);

        /* And finally print all pieces together as one line */
        if (ARCH_900_IDX == arch_mode)
        {
            FLOGMSG( stdout, "%s%sR:%016"PRIX64":K:%02X=%s\n",
                pfx, vadr, op->raddr, op->skey, stor );
        }
        else
        {
            FLOGMSG( stdout, "%s%sR:%08"PRIX32":K:%02X=%s\n",
                pfx, vadr, (U32)op->raddr, op->skey, stor );
        }
    }
}

/*-------------------------------------------------------------------*/
/*                Print Operand Storage lines                        */
/*-------------------------------------------------------------------*/
static inline void print_storage_lines( BYTE cpuad )
{
    TF02326* rec;
    char tim[ 64 ]  = {0};  // "YYYY-MM-DD HH:MM:SS.uuuuuu"
    char pfx[ 64 ]  = {0};  // "16:22:48.753999 HHC02326I IP05: "

    ASSERT(( all_recs[ cpuad ].gotmask & GOT_TF02326 ));
    rec = &all_recs[ cpuad ].tf02326;

    if (!rec->valid)
        return;

    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));
    MSGBUF( pfx, "%s HHC02326I %s%s: ", &tim[ 11 ],
        rec->sie ? "SIE: " : "", ptyp_str( cpuad ));

    if (rec->b1 >= 0)
        print_op_stor( pfx, rec->rhdr.arch_mode, rec->real_mode, &rec->op1 );

    if (rec->b2 >= 0)
        print_op_stor( pfx, rec->rhdr.arch_mode, rec->real_mode, &rec->op2 );
}

/*-------------------------------------------------------------------*/
/*                    Print I/O Interrupt                            */
/*-------------------------------------------------------------------*/
static inline void print_806_io_rupt( TF00806* rec )
{
    char tim [ 64 ]  = {0};  // "YYYY-MM-DD HH:MM:SS.uuuuuu"
    char pfx [ 96 ]  = {0};

    bool is370 = (ARCH_370_IDX == rec->rhdr.arch_mode);

    const char* msgid = is370 ? "HHC00805I"
                              : "HHC00806I";

    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));

    // HHC00805 "Processor %s%02X: I/O interrupt code %8.8X parm %8.8X"
    // HHC00806 "Processor %s%02X: I/O interrupt code %8.8X parm %8.8X id %8.8X"

    tf_do_blank_sep( &rec->rhdr );

    MSGBUF( pfx, "%s %s Processor %s: I/O interrupt code %8.8X parm %8.8X",
        &tim[ 11 ], msgid, ptyp_str( rec->rhdr.cpuad ), rec->ioid, rec->ioparm );

    if (is370)
        // "Processor %s%02X: I/O interrupt code %8.8X parm %8.8X"
        FLOGMSG( stdout, "%s\n", pfx );
    else
        // "Processor %s%02X: I/O interrupt code %8.8X parm %8.8X id %8.8X"
        FLOGMSG( stdout, "%s id %8.8X\n", pfx, rec->iointid );
}


/*-------------------------------------------------------------------*/
/*                  Print Signal Processor                           */
/*-------------------------------------------------------------------*/
static inline void print_814_sigp( TF00814* rec )
{
    char tim [  64 ]  = {0};    // "YYYY-MM-DD HH:MM:SS.uuuuuu"
    char prm [  32 ]  = {0};
    char pfx [ 128 ]  = {0};

    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));

    if (ARCH_900_IDX == rec->rhdr.arch_mode)
        MSGBUF( prm, "%16.16"PRIX64, (U64) rec->parm );
    else
        MSGBUF( prm, "%8.8"PRIX32, (U32) rec->parm );

    // "Processor %s: CC %d%s"
    MSGBUF( pfx, "%s HHC00814I Processor %s: SIGP %-32s (%02X) %s, PARM %s: CC %d",
        &tim[ 11 ], ptyp_str( rec->rhdr.cpuad ),
        order2name( rec->order ), rec->order,
        ptyp_str( rec->cpad ), prm, rec->cc );

    tf_do_blank_sep( &rec->rhdr );

    if (rec->got_status)
        FLOGMSG( stdout, "%s\n", pfx );
    else
        FLOGMSG( stdout, "%s status %08X\n", pfx, rec->status );
}

/******************************************************************************/
/*                                                                            */
/*                          DEVICE TRACING                                    */
/*                     Record PRINTING Functions                              */
/*                                                                            */
/******************************************************************************/

/*-------------------------------------------------------------------*/
/*  e7_fmtdata -- format 64 bytes of hex and character buffer data   */
/*-------------------------------------------------------------------*/
static const char* e7_fmtdata( BYTE code, BYTE* data, BYTE amt )
{
    static char both_buf[(64/16)*96] = {0};
           char byte_buf[(64/16)*64] = {0};
           char char_buf[(64/16)*32] = {0};

    UNREFERENCED( code );

    if (amt > 64) CRASH();  // (sanity check)

    if (!amt)   // (might be e.g. TIC, which doesn't xfer any data)
    {
        both_buf[0] = 0;
        return both_buf;
    }

    MSGBUF( byte_buf,
        " => "
        "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
        "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
        "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
        "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X"

        , data[ 0], data[ 1], data[ 2], data[ 3], data[ 4], data[ 5], data[ 6], data[ 7]
        , data[ 8], data[ 9], data[10], data[11], data[12], data[13], data[14], data[15]

        , data[16], data[17], data[18], data[19], data[20], data[21], data[22], data[23]
        , data[24], data[25], data[26], data[27], data[28], data[29], data[30], data[31]

        , data[32], data[33], data[34], data[35], data[36], data[37], data[38], data[39]
        , data[40], data[41], data[42], data[43], data[44], data[45], data[46], data[47]

        , data[48], data[49], data[50], data[51], data[52], data[53], data[54], data[55]
        , data[56], data[57], data[58], data[59], data[60], data[61], data[62], data[63]
    );

    // Truncate according to passed len (the below accounts for the
    // 4 char " => " prefix, plus the 2 printed hex characters for
    // each byte, plus the blank/space after every 4 printed bytes.

    byte_buf[ 4 + (amt << 1) + (amt >> 2) ] = 0;

    // Now format the character representation of those bytes

    prt_guest_to_host( data, char_buf, amt );

    MSGBUF( both_buf, "%-*.*s%s", 4+((64/4)*9),
                                  4+((64/4)*9),
                                  byte_buf, char_buf );

    return both_buf;
}

/*-------------------------------------------------------------------*/
/*   fmtdata -- format 16 bytes of hex and character buffer data     */
/*-------------------------------------------------------------------*/
static const char* fmtdata( BYTE code, BYTE* data, BYTE amt )
{
    if (sys_ffmt >= TF_FMT1 && code == 0xE7)
        return e7_fmtdata( code, data, amt );
    else
    {
        static char both_buf[(16/16)*96] = {0};
               char byte_buf[(16/16)*64] = {0};
               char char_buf[(16/16)*32] = {0};

        if (amt > 16) CRASH();  // (sanity check)

        if (!amt)   // (might be e.g. TIC, which doesn't xfer any data)
        {
            both_buf[0] = 0;
            return both_buf;
        }

        MSGBUF( byte_buf,
            " => "
            "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X"

            , data[0], data[1], data[ 2], data[ 3], data[ 4], data[ 5], data[ 6], data[ 7]
            , data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]
        );

        // Truncate according to passed len (the below accounts for the
        // 4 char " => " prefix, plus the 2 printed hex characters for
        // each byte, plus the blank/space after every 4 printed bytes.

        byte_buf[ 4 + (amt << 1) + (amt >> 2) ] = 0;

        // Now format the character representation of those bytes

        prt_guest_to_host( data, char_buf, amt );

        MSGBUF( both_buf, "%-*.*s%s", 4+((16/4)*9),
                                      4+((16/4)*9),
                                      byte_buf, char_buf );

        return both_buf;
    }
}

/*-------------------------------------------------------------------*/
/*         The Device Trace printing functions themselves            */
/*                  msgnum 423... and 516...                         */
/*-------------------------------------------------------------------*/

//HHC00423 "Thread "TIDPAT" %1d:%04X CKD file %s: search key %s"
PRINT_DEV_FUNC0( 0423 ), RTRIM( str_guest_to_host( rec->key, rec->key, rec->kl ))); }

//HHC00424 "Thread "TIDPAT" %1d:%04X CKD file %s: read trk %d cur trk %d"
PRINT_DEV_FUNC0( 0424 ), rec->trk, rec->bufcur ); }

//HHC00425 "Thread "TIDPAT" %1d:%04X CKD file %s: read track updating track %d"
PRINT_DEV_FUNC0( 0425 ), rec->bufcur ); }

//HHC00426 "Thread "TIDPAT" %1d:%04X CKD file %s: read trk %d cache hit, using cache[%d]"
PRINT_DEV_FUNC0( 0426 ), rec->trk, rec->idx ); }

//HHC00427 "Thread "TIDPAT" %1d:%04X CKD file %s: read trk %d no available cache entry, waiting"
PRINT_DEV_FUNC0( 0427 ), rec->trk ); }

//HHC00428 "Thread "TIDPAT" %1d:%04X CKD file %s: read trk %d cache miss, using cache[%d]"
PRINT_DEV_FUNC0( 0428 ), rec->trk, rec->idx ); }

//HHC00429 "Thread "TIDPAT" %1d:%04X CKD file %s: read trk %d reading file %d offset %"PRId64" len %d"
PRINT_DEV_FUNC0( 0429 ), rec->trk, rec->fnum, rec->offset, rec->len ); }

//HHC00430 "Thread "TIDPAT" %1d:%04X CKD file %s: read trk %d trkhdr %02X %02X%02X %02X%02X"
PRINT_DEV_FUNC0( 0430 ), rec->trk, rec->buf[0], rec->buf[1], rec->buf[2], rec->buf[3], rec->buf[4] ); }

//HHC00431 "Thread "TIDPAT" %1d:%04X CKD file %s: seeking to cyl %d head %d"
PRINT_DEV_FUNC0( 0431 ), rec->cyl, rec->head ); }

//HHC00432 "Thread "TIDPAT" %1d:%04X CKD file %s: error: MT advance: locate record %d file mask %02X"
// (Note: PRINT_DEV_FUNC0_E = "E" error message, not "I" informational message!)
PRINT_DEV_FUNC0_E( 0432 ), rec->count, rec->mask ); }

//HHC00433 "Thread "TIDPAT" %1d:%04X CKD file %s: MT advance to cyl(%d) head(%d)"
PRINT_DEV_FUNC0( 0433 ), rec->cyl, rec->head ); }

//HHC00434 "Thread "TIDPAT" %1d:%04X CKD file %s: read count orientation %s"
static const char* orient[] = { "none", "index", "count", "key", "data", "eot" };
PRINT_DEV_FUNC0( 0434 ), orient[ rec->orient ] ); }

//HHC00435 "Thread "TIDPAT" %1d:%04X CKD file %s: cyl %d head %d record %d kl %d dl %d of %d"
PRINT_DEV_FUNC0( 0435 ), rec->cyl, rec->head, rec->record, rec->kl, rec->dl, rec->offset ); }

//HHC00436 "Thread "TIDPAT" %1d:%04X CKD file %s: read key %d bytes"
PRINT_DEV_FUNC0( 0436 ), rec->kl ); }

//HHC00437 "Thread "TIDPAT" %1d:%04X CKD file %s: read data %d bytes"
PRINT_DEV_FUNC0( 0437 ), rec->dl ); }

//HHC00438 "Thread "TIDPAT" %1d:%04X CKD file %s: writing cyl %d head %d record %d kl %d dl %d"
PRINT_DEV_FUNC0( 0438 ), rec->cyl, rec->head, rec->recnum, rec->keylen, rec->datalen ); }

//HHC00439 "Thread "TIDPAT" %1d:%04X CKD file %s: setting track overflow flag for cyl %d head %d record %d"
PRINT_DEV_FUNC0( 0439 ), rec->cyl, rec->head, rec->recnum ); }

//HHC00440 "Thread "TIDPAT" %1d:%04X CKD file %s: updating cyl %d head %d record %d kl %d dl %d"
PRINT_DEV_FUNC0( 0440 ), rec->cyl, rec->head, rec->recnum, rec->keylen, rec->datalen ); }

//HHC00441 "Thread "TIDPAT" %1d:%04X CKD file %s: updating cyl %d head %d record %d dl %d"
PRINT_DEV_FUNC0( 0441 ), rec->cyl, rec->head, rec->recnum, rec->datalen ); }

//HHC00442 "Thread "TIDPAT" %1d:%04X CKD file %s: set file mask %02X"
PRINT_DEV_FUNC0( 0442 ), rec->mask ); }

//HHC00516 "Thread "TIDPAT" %1d:%04X FBA file %s: read blkgrp %d cache hit, using cache[%d]"
PRINT_DEV_FUNC0( 0516 ), rec->blkgrp, rec->idx ); }

//HHC00517 "Thread "TIDPAT" %1d:%04X FBA file %s: read blkgrp %d no available cache entry, waiting"
PRINT_DEV_FUNC0( 0517 ), rec->blkgrp ); }

//HHC00518 "Thread "TIDPAT" %1d:%04X FBA file %s: read blkgrp %d cache miss, using cache[%d]"
PRINT_DEV_FUNC0( 0518 ), rec->blkgrp, rec->idx ); }

//HHC00519 "Thread "TIDPAT" %1d:%04X FBA file %s: read blkgrp %d offset %"PRId64" len %d"
PRINT_DEV_FUNC0( 0519 ), rec->blkgrp, rec->offset, rec->len ); }

//HHC00520 "Thread "TIDPAT" %1d:%04X FBA file %s: positioning to 0x%"PRIX64" %"PRId64
PRINT_DEV_FUNC0( 0520 ), rec->rba, rec->rba ); }

/*-------------------------------------------------------------------*/
/*         The Device Trace printing functions themselves            */
/*                       msgnum 1300...                              */
/*-------------------------------------------------------------------*/

// "%1d:%04X CHAN: halt subchannel: cc=%d"
PRINT_DEV_FUNC( 1300 ), rec->cc ); }

/*-------------------------------------------------------------------*/
/*                  Print IDAW/MIDAW Record                          */
/*-------------------------------------------------------------------*/
static inline void print_TF01301( TF01301* rec )
{
    char timstr[ 64] = {0};    // "YYYY-MM-DD HH:MM:SS.uuuuuu"
    FormatTIMEVAL( &rec->rhdr.tod, timstr, sizeof( timstr ));

    switch (rec->type)
    {
    case PF_IDAW1:
        // "%1d:%04X CHAN: idaw %8.8"PRIX32", len %3.3"PRIX16"%s"
        TF_DEV_FLOGMSG( 1302 ),
            rec->rhdr.lcss, rec->rhdr.devnum, (U32)rec->addr, rec->count, fmtdata( rec->code, rec->data, rec->amt ));
        break;

    case PF_IDAW2:

        // "%1d:%04X CHAN: idaw %16.16"PRIX64", len %4.4"PRIX16"%s"
        TF_DEV_FLOGMSG( 1303 ),
            rec->rhdr.lcss, rec->rhdr.devnum, (U64)rec->addr, rec->count, fmtdata( rec->code, rec->data, rec->amt ));
        break;

    case PF_MIDAW:

        // "%1d:%04X CHAN: midaw %2.2X %4.4"PRIX16" %16.16"PRIX64"%s"
        TF_DEV_FLOGMSG( 1301 ),
            rec->rhdr.lcss, rec->rhdr.devnum, rec->mflag, rec->count, (U64)rec->addr, fmtdata( rec->code, rec->data, rec->amt ));
        break;

    default: CRASH(); UNREACHABLE_CODE( return );
    }
}

// "%1d:%04X CHAN: attention signaled"
PRINT_DEV_FUNC( 1304 )); }

// "%1d:%04X CHAN: attention"
PRINT_DEV_FUNC( 1305 )); }

// "%1d:%04X CHAN: initial status interrupt"
PRINT_DEV_FUNC( 1306 )); }

// "%1d:%04X CHAN: attention completed"
PRINT_DEV_FUNC( 1307 )); }

// "%1d:%04X CHAN: clear completed"
PRINT_DEV_FUNC( 1308 )); }

// "%1d:%04X CHAN: halt completed"
PRINT_DEV_FUNC( 1309 )); }

// "%1d:%04X CHAN: suspended"
PRINT_DEV_FUNC( 1310 )); }

// "%1d:%04X CHAN: resumed"
PRINT_DEV_FUNC( 1311 )); }

// "%1d:%04X CHAN: stat %2.2X%2.2X, count %4.4X"
PRINT_DEV_FUNC( 1312 ), rec->unitstat, rec->chanstat, rec->residual ); }

/*-------------------------------------------------------------------*/
/*                   Print Sense Record                              */
/*-------------------------------------------------------------------*/
static inline void print_TF01313( TF01313* rec )
{
    char timstr[ 64] = {0};    // "YYYY-MM-DD HH:MM:SS.uuuuuu"
    FormatTIMEVAL( &rec->rhdr.tod, timstr, sizeof( timstr ));

    // "%1d:%04X CHAN: sense %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X"
    TF_DEV_FLOGMSG( 1313 ),
        rec->rhdr.lcss, rec->rhdr.devnum,
        rec->sense[ 0], rec->sense[ 1], rec->sense[ 2], rec->sense[ 3],
        rec->sense[ 4], rec->sense[ 5], rec->sense[ 6], rec->sense[ 7],
        rec->sense[ 8], rec->sense[ 9], rec->sense[10], rec->sense[11],
        rec->sense[12], rec->sense[13], rec->sense[14], rec->sense[15],
        rec->sense[16], rec->sense[17], rec->sense[18], rec->sense[19],
        rec->sense[20], rec->sense[21], rec->sense[22], rec->sense[23],
        rec->sense[24], rec->sense[25], rec->sense[26], rec->sense[27],
        rec->sense[28], rec->sense[29], rec->sense[30], rec->sense[31] );

    /* Use the device's interpretation of its own sense bytes if available.
       Otherwise format the default interpretation
    */
    if (!rec->sns[0])
        default_sns( rec->sns, sizeof( rec->sns ), rec->sense[0], rec->sense[1] );

    // "%1d:%04X CHAN: sense %s"
    TF_DEV_FLOGMSG( 1314 ),
        rec->rhdr.lcss, rec->rhdr.devnum, rec->sns );
}

/*-------------------------------------------------------------------*/
/*                   Print CCW Record                                */
/*-------------------------------------------------------------------*/
static inline void print_TF01315( TF01315* rec )
{
    char timstr [ 64 ] = {0};      // "YYYY-MM-DD HH:MM:SS.uuuuuu"

    FormatTIMEVAL( &rec->rhdr.tod, timstr, sizeof( timstr ));

    // "%1d:%04X CHAN: ccw %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X%s"
    TF_DEV_FLOGMSG( 1315 ),
        rec->rhdr.lcss, rec->rhdr.devnum,
        rec->ccw[0], rec->ccw[1], rec->ccw[2], rec->ccw[3],
        rec->ccw[4], rec->ccw[5], rec->ccw[6], rec->ccw[7],
        fmtdata( rec->ccw[0], rec->data, rec->amt ));
}

// "%1d:%04X CHAN: csw %2.2X, stat %2.2X%2.2X, count %2.2X%2.2X, ccw %2.2X%2.2X%2.2X"
PRINT_DEV_FUNC( 1316 ), rec->csw[0]
        , rec->csw[4], rec->csw[5]
        , rec->csw[6], rec->csw[7]
        , rec->csw[1], rec->csw[2], rec->csw[3] ); }

// "%1d:%04X CHAN: scsw %2.2X%2.2X%2.2X%2.2X, stat %2.2X%2.2X, count %2.2X%2.2X, ccw %2.2X%2.2X%2.2X%2.2X"
PRINT_DEV_FUNC( 1317 ), rec->scsw.flag0, rec->scsw.flag1, rec->scsw.flag2, rec->scsw.flag3
        , rec->scsw.unitstat, rec->scsw.chanstat
        , rec->scsw.count[0], rec->scsw.count[1]
        , rec->scsw.ccwaddr[0], rec->scsw.ccwaddr[1], rec->scsw.ccwaddr[2], rec->scsw.ccwaddr[3] ); }

// "%1d:%04X CHAN: test I/O: cc=%d"
PRINT_DEV_FUNC( 1318 ), rec->cc ); }

// "%1d:%04X CHAN: start I/O S/370 conversion to asynchronous operation started"
PRINT_DEV_FUNC( 1320 )); }

// "%1d:%04X CHAN: start I/O S/370 conversion to asynchronous operation successful"
PRINT_DEV_FUNC( 1321 )); }

// "%1d:%04X CHAN: halt I/O"
PRINT_DEV_FUNC( 1329 )); }

// "%1d:%04X CHAN: HIO modification executed: cc=1"
PRINT_DEV_FUNC( 1330 )); }

// "%1d:%04X CHAN: clear subchannel"
PRINT_DEV_FUNC( 1331 )); }

// "%1d:%04X CHAN: halt subchannel"
PRINT_DEV_FUNC( 1332 )); }

// "%1d:%04X CHAN: resume subchannel: cc=%d"
PRINT_DEV_FUNC( 1333 ), rec->cc ); }

/*-------------------------------------------------------------------*/
/*                    Print ORB Record                               */
/*-------------------------------------------------------------------*/
static inline void print_TF01334( TF01334* rec )
{
    char timstr[ 64] = {0};    // "YYYY-MM-DD HH:MM:SS.uuuuuu"
    char msgbuf[128] = {0};

    FormatTIMEVAL( &rec->rhdr.tod, timstr, sizeof( timstr ));
    FormatORB( &rec->orb, msgbuf, sizeof( msgbuf ));

    // "%1d:%04X CHAN: ORB: %s"
    TF_DEV_FLOGMSG( 1334 ),
        rec->rhdr.lcss, rec->rhdr.devnum, msgbuf );

    ++totios;  // (count total device I/Os printed)
}

// "%1d:%04X CHAN: startio cc=2 (busy=%d startpending=%d)"
PRINT_DEV_FUNC( 1336 ), rec->busy, rec->startpending ); }

/******************************************************************************/
/*                                                                            */
/*                         INSTRUCTION TRACING                                */
/*                     Record PROCESSING Functions                            */
/*                                                                            */
/******************************************************************************/

/*-------------------------------------------------------------------*/
/*               Process Wait State PSW Record                       */
/*-------------------------------------------------------------------*/
static void process_TF00800( TF00800* rec )
{
    print_800_wait_state_psw( rec );

    /* Reset ONLY *our* GOT flag and return */
    all_recs[ rec->rhdr.cpuad ].gotmask &= ~GOT_TF00800;
}

/*-------------------------------------------------------------------*/
/*              Process Program Interrupt Record                     */
/*-------------------------------------------------------------------*/
static void process_TF00801( TF00801* rec )
{
    print_801_program_interrupt( rec );

    /* Reset ONLY *our* GOT flag and return */
    all_recs[ rec->rhdr.cpuad ].gotmask &= ~GOT_TF00801;
}

/*-------------------------------------------------------------------*/
/*                 Process PER Event Record                          */
/*-------------------------------------------------------------------*/
static void process_TF00802( TF00802* rec )
{
    print_802_per_event( rec );

    /* Reset ONLY *our* GOT flag and return */
    all_recs[ rec->rhdr.cpuad ].gotmask &= ~GOT_TF00802;
}

/*-------------------------------------------------------------------*/
/*           Process Program Interrupt Loop Record                   */
/*-------------------------------------------------------------------*/
static void process_TF00803( TF00803* rec )
{
    print_803_pgm_int_loop( rec );

    /* Reset ONLY *our* GOT flag and return */
    all_recs[ rec->rhdr.cpuad ].gotmask &= ~GOT_TF00803;
}

/*-------------------------------------------------------------------*/
/*            Process I/O interrupt (S/370) Record                   */
/*-------------------------------------------------------------------*/
static void process_TF00804( TF00804* rec )
{
    print_804_io_rupt_370( rec );

    /* Reset ONLY *our* GOT flag and return */
    all_recs[ rec->rhdr.cpuad ].gotmask &= ~GOT_TF00804;
}

/*-------------------------------------------------------------------*/
/*               Process I/O Interrupt Record                        */
/*-------------------------------------------------------------------*/
static void process_TF00806( TF00806* rec )
{
    print_806_io_rupt( rec );

    /* Reset ONLY *our* GOT flag and return */
    all_recs[ rec->rhdr.cpuad ].gotmask &= ~GOT_TF00806;
}

/*-------------------------------------------------------------------*/
/*            Process Machine Check Interrupt Record                 */
/*-------------------------------------------------------------------*/
static void process_TF00807( TF00807* rec )
{
    print_807_machine_check_interrupt( rec );

    /* Reset ONLY *our* GOT flag and return */
    all_recs[ rec->rhdr.cpuad ].gotmask &= ~GOT_TF00807;
}

/*-------------------------------------------------------------------*/
/*               Process Store Status Record                         */
/*-------------------------------------------------------------------*/
static void process_TF00808( TF00808* rec )
{
    print_808_store_status( rec );

    /* Reset ONLY *our* GOT flag and return */
    all_recs[ rec->rhdr.cpuad ].gotmask &= ~GOT_TF00808;
}

/*-------------------------------------------------------------------*/
/*           Process Disabled Wait State Record                      */
/*-------------------------------------------------------------------*/
static void process_TF00809( TF00809* rec )
{
    print_809_disabled_wait( rec );

    /* Reset ONLY *our* GOT flag and return */
    all_recs[ rec->rhdr.cpuad ].gotmask &= ~GOT_TF00809;
}

/*-------------------------------------------------------------------*/
/*             Process Architecture Mode Record                      */
/*-------------------------------------------------------------------*/
static void process_TF00811( TF00811* rec )
{
    print_811_arch_mode( rec );

    /* Reset ONLY *our* GOT flag and return */
    all_recs[ rec->rhdr.cpuad ].gotmask &= ~GOT_TF00811;
}

/*-------------------------------------------------------------------*/
/*      Process Vector Facility Online (370/390) Record              */
/*-------------------------------------------------------------------*/
static void process_TF00812( TF00812* rec )
{
    print_812_vect_online_370( rec );

    /* Reset ONLY *our* GOT flag and return */
    all_recs[ rec->rhdr.cpuad ].gotmask &= ~GOT_TF00812;
}

/*-------------------------------------------------------------------*/
/*               Process Signal Processor Record                     */
/*-------------------------------------------------------------------*/
static void process_TF00814( TF00814* rec )
{
    print_814_sigp( rec );

    /* Reset ONLY *our* GOT flag and return */
    all_recs[ rec->rhdr.cpuad ].gotmask &= ~GOT_TF00814;
}

/*-------------------------------------------------------------------*/
/*              Process External Interrupt Record                    */
/*-------------------------------------------------------------------*/
static void process_TF00840( TF00840* rec )
{
    print_840_ext_rupt( rec );

    /* Reset ONLY *our* GOT flag and return */
    all_recs[ rec->rhdr.cpuad ].gotmask &= ~GOT_TF00840;
}

/*-------------------------------------------------------------------*/
/*              Process Block I/O Interrupt Record                   */
/*-------------------------------------------------------------------*/
static void process_TF00844( TF00844* rec )
{
    print_844_blkio_rupt( rec );

    /* Reset ONLY *our* GOT flag and return */
    all_recs[ rec->rhdr.cpuad ].gotmask &= ~GOT_TF00844;
}

/*-------------------------------------------------------------------*/
/*           Process Block I/O External interrupt Record             */
/*-------------------------------------------------------------------*/
static void process_TF00845( TF00845* rec )
{
    print_845_blkio_ext_rupt( rec );

    /* Reset ONLY *our* GOT flag and return */
    all_recs[ rec->rhdr.cpuad ].gotmask &= ~GOT_TF00845;
}

/*-------------------------------------------------------------------*/
/*           Process Service Signal External Interrupt Record        */
/*-------------------------------------------------------------------*/
static void process_TF00846( TF00846* rec )
{
    print_846_srvsig_ext_rupt( rec );

    /* Reset ONLY *our* GOT flag and return */
    all_recs[ rec->rhdr.cpuad ].gotmask &= ~GOT_TF00846;
}

/*-------------------------------------------------------------------*/
/*               Process Control Registers Record                    */
/*-------------------------------------------------------------------*/
static void process_TF02271( TF02271* rec )
{
    UNREFERENCED( rec );
    // Do nothing. Regs are printed by process_TF02324.
}

/*-------------------------------------------------------------------*/
/*               Process Access Registers Record                     */
/*-------------------------------------------------------------------*/
static void process_TF02272( TF02272* rec )
{
    UNREFERENCED( rec );
    // Do nothing. Regs are printed by process_TF02324.
}

/*-------------------------------------------------------------------*/
/*         Process Floating Point Control Register Record            */
/*-------------------------------------------------------------------*/
static void process_TF02276( TF02276* rec )
{
    UNREFERENCED( rec );
    // Do nothing. Regs are printed by process_TF02324.
}

/*-------------------------------------------------------------------*/
/*             Process Floating Point Registers Record               */
/*-------------------------------------------------------------------*/
static void process_TF02270( TF02270* rec )
{
    UNREFERENCED( rec );
    // Do nothing. Regs are printed by process_TF02324.
}

/*-------------------------------------------------------------------*/
/*             Process Vector Registers Record                       */
/*-------------------------------------------------------------------*/
static void process_TF02266( TF02266* rec )
{
    UNREFERENCED( rec );
    // Do nothing. Regs are printed by process_TF02324.
}

/*-------------------------------------------------------------------*/
/*               Process General Registers Record                    */
/*-------------------------------------------------------------------*/
static void process_TF02269( TF02269* rec )
{
    if (rec->ifetch_error)
    {
        char tim [ 64 ] = {0};  // "YYYY-MM-DD HH:MM:SS.uuuuuu"
        char buf [ 64 ] = {0};  // "16:22:47.745999 HHC02269I SIE:"

        print_gr_regs( rec );     // print general purpose registers

        // Display "Instruction Fetch Error" message
        FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));
        MSGBUF( buf, "%s HHC02269E %s", &tim[ 11 ], rec->sie ? "SIE: " : "" );
        tf_do_blank_sep( &rec->rhdr );
        FLOGMSG( stderr, "%s%s\n", buf, "Instruction fetch error" );

        /* Reset ONLY *our* GOT flag and return */
        all_recs[ rec->rhdr.cpuad ].gotmask &= ~GOT_TF02269;
    }
    // Otherwise do nothing. Regs normally printed by process_TF02324.
}

/*-------------------------------------------------------------------*/
/*              Process Instruction Trace Record                     */
/*-------------------------------------------------------------------*/
static void process_TF02324( TF02324* rec )
{
    char tim [ 64 ] = {0};  // "YYYY-MM-DD HH:MM:SS.uuuuuu"
    BYTE cpuad;

    /* (just a more covenient shorter variable name) */
    cpuad = (BYTE) rec->rhdr.cpuad;

    /* Perform storage address filtering if requested */
    if (nStorMOPT)
    {
        if (0
            || !(all_recs[ cpuad ].gotmask & GOT_TF02326)
            || !is_wanted_tf02326( &all_recs[ cpuad ].tf02326 )
        )
        {
            /* Reset *all* GOT flags and return */
            all_recs[ cpuad ].gotmask = 0;
            return; // (not interested in this trace record)
        }
    }

    /* Perform opcode filtering if requested */
    if (nInstMOPT)
    {
        if (!is_wanted_opcode( &rec->inst[0] ))
        {
            /* Reset *all* GOT flags and return */
            all_recs[ cpuad ].gotmask = 0;
            return; // (not interested in this trace record)
        }
    }

    /* Count instruction records printed */
    ++totins;

    /* Update previous values */
    prvcpuad  = rec->rhdr.cpuad;
    prvdevnum = rec->rhdr.devnum;

    /* Print a blank line before printing each instruction so that
       there is a blank line between each printed instruction.

       PLEASE NOTE that we wish to treat each instruction that is
       printed as if it were a single line (even though multiple
       lines are always ptinted for each).

       Thus the "print_all_available_regs" function prints the blank
       line for us before it prints the registers, but only does so
       if the REGSFIRST option was specified. Otherwise it doesn't,
       so we need to do it ourselves here before printing the actual
       instruction line.
    */
    /* Print the registers line(s) if appropriate... */
    if (regsfirst && !noregs)              // (REGSFIRST?)
        print_all_available_regs( cpuad ); // (also prints blank line)
    else
        printf("\n");                      // (blank line before inst)

    /* Print the HHC02324 PSW and INST line... */
    FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));
    FLOGMSG( stdout, "%s HHC02324I %s: %s %s\n",
        &tim[ 11 ],
        ptyp_str( cpuad ),
        fmt_psw_inst_str( &rec->psw, rec->rhdr.arch_mode, rec->inst, rec->ilc ),
        fmt_inst_name( rec->rhdr.arch_mode, rec->inst )
    );

    /* Print the HHC02326 operand storage line(s)... */
    if (all_recs[ cpuad ].gotmask & GOT_TF02326)
        print_storage_lines( cpuad );

    /* Print the registers line(s) if appropriate... */
    if (!regsfirst && !noregs)
        print_all_available_regs( cpuad );

    /* Remember we printed an instruction */
    previnst = true;

    /* Reset *all* GOT flags and return */
    all_recs[ cpuad ].gotmask = 0;
}

/*-------------------------------------------------------------------*/
/*             Process Instruction Operands Record                   */
/*-------------------------------------------------------------------*/
static void process_TF02326( TF02326* rec )
{
    if (!rec->valid)
    {
        char tim [ 64 ] = {0};  // "YYYY-MM-DD HH:MM:SS.uuuuuu"

        FormatTIMEVAL( &rec->rhdr.tod, tim, sizeof( tim ));
        tf_do_blank_sep( &rec->rhdr );
        FLOGMSG( stdout, "%s HHC02267I Real address is not valid\n", &tim[ 11 ] );

        /* Reset ONLY *our* GOT flag and return */
        all_recs[ rec->rhdr.cpuad ].gotmask &= ~GOT_TF02326;
    }
    // Otherwise do nothing. operands normally printed by process_TF02324.
}

/******************************************************************************/
/*                                                                            */
/*                           DEVICE TRACING                                   */
/*                     Record PROCESSING Functions                            */
/*                                                                            */
/******************************************************************************/

#define PROCESS_DEV_RECORD_FUNC( _nnnn )                            \
                                                                    \
    static void process_TF0 ## _nnnn( TF0 ## _nnnn* rec )           \
    {                                                               \
        /* Print the record immediately if wanted */                \
        if (is_devnum_wanted( rec->rhdr.devnum ))                   \
            print_TF0 ## _nnnn( rec );                              \
                                                                    \
        /* Reset ONLY *our* GOT flag and return */                  \
        all_recs[ rec->rhdr.cpuad ].gotmask &= ~GOT_TF0 ## _nnnn;   \
    }

PROCESS_DEV_RECORD_FUNC( 0423 ); // Search key
PROCESS_DEV_RECORD_FUNC( 0424 ); // Cur trk
PROCESS_DEV_RECORD_FUNC( 0425 ); // Updating track
PROCESS_DEV_RECORD_FUNC( 0426 ); // Cache hit
PROCESS_DEV_RECORD_FUNC( 0427 ); // Unavailable cache
PROCESS_DEV_RECORD_FUNC( 0428 ); // Cache miss
PROCESS_DEV_RECORD_FUNC( 0429 ); // Offset
PROCESS_DEV_RECORD_FUNC( 0430 ); // Trkhdr
PROCESS_DEV_RECORD_FUNC( 0431 ); // Seeking
PROCESS_DEV_RECORD_FUNC( 0432 ); // MT advance error
PROCESS_DEV_RECORD_FUNC( 0433 ); // MT advance
PROCESS_DEV_RECORD_FUNC( 0434 ); // Read count orientation
PROCESS_DEV_RECORD_FUNC( 0435 ); // Cyl head record kl dl
PROCESS_DEV_RECORD_FUNC( 0436 ); // Read key
PROCESS_DEV_RECORD_FUNC( 0437 ); // Read data
PROCESS_DEV_RECORD_FUNC( 0438 ); // Write cyl head record kl dl
PROCESS_DEV_RECORD_FUNC( 0439 ); // Set track overflow flag
PROCESS_DEV_RECORD_FUNC( 0440 ); // Update cyl head record kl dl
PROCESS_DEV_RECORD_FUNC( 0441 ); // Update cyl head record dl
PROCESS_DEV_RECORD_FUNC( 0442 ); // Set file mask

PROCESS_DEV_RECORD_FUNC( 0516 ); // Cache hit
PROCESS_DEV_RECORD_FUNC( 0517 ); // Unavailable cache
PROCESS_DEV_RECORD_FUNC( 0518 ); // Cache miss
PROCESS_DEV_RECORD_FUNC( 0519 ); // Offset len
PROCESS_DEV_RECORD_FUNC( 0520 ); // Positioning

PROCESS_DEV_RECORD_FUNC( 1300 ); // Halt subchannel
PROCESS_DEV_RECORD_FUNC( 1301 ); // IDAW/MIDAW
PROCESS_DEV_RECORD_FUNC( 1304 ); // Attention signaled
PROCESS_DEV_RECORD_FUNC( 1305 ); // Attention
PROCESS_DEV_RECORD_FUNC( 1306 ); // Initial status interrupt
PROCESS_DEV_RECORD_FUNC( 1307 ); // Attention completed
PROCESS_DEV_RECORD_FUNC( 1308 ); // Clear completed
PROCESS_DEV_RECORD_FUNC( 1309 ); // Halt completed
PROCESS_DEV_RECORD_FUNC( 1310 ); // Suspended
PROCESS_DEV_RECORD_FUNC( 1311 ); // Resumed
PROCESS_DEV_RECORD_FUNC( 1312 ); // I/O stat
PROCESS_DEV_RECORD_FUNC( 1313 ); // Sense
PROCESS_DEV_RECORD_FUNC( 1315 ); // CCW
PROCESS_DEV_RECORD_FUNC( 1316 ); // CSW (370)
PROCESS_DEV_RECORD_FUNC( 1317 ); // SCSW
PROCESS_DEV_RECORD_FUNC( 1318 ); // TEST I/O
PROCESS_DEV_RECORD_FUNC( 1320 ); // S/370 start I/O conversion started
PROCESS_DEV_RECORD_FUNC( 1321 ); // S/370 start I/O conversion success
PROCESS_DEV_RECORD_FUNC( 1329 ); // Halt I/O
PROCESS_DEV_RECORD_FUNC( 1330 ); // HIO modification
PROCESS_DEV_RECORD_FUNC( 1331 ); // Clear subchannel
PROCESS_DEV_RECORD_FUNC( 1332 ); // Halt subchannel
PROCESS_DEV_RECORD_FUNC( 1333 ); // Resume subchannel
PROCESS_DEV_RECORD_FUNC( 1334 ); // ORB
PROCESS_DEV_RECORD_FUNC( 1336 ); // Startio cc=2

/******************************************************************************/
/*                                                                            */
/*                         MAIN FUNCTIONS                                     */
/*                                                                            */
/******************************************************************************/

/*-------------------------------------------------------------------*/
/*             Common Finish Reading Record routine                  */
/*-------------------------------------------------------------------*/
static bool finish_reading_rec( U16 msgnum )
{
    TFHDR* hdr        = NULL;         /* Ptr to record header        */
    BYTE*  rec        = NULL;         /* Ptr to remainder of record  */
    int    amt        = 0;            /* Size of remainder of record */
    int    bytes_read = 0;            /* Number of BYTES read so far */
    U16    cpuad      = 0;            /* cpuad from TFHDR header     */

    static bool first_time = true;    /* First time switch           */

    hdr = (TFHDR*) iobuff;            /* point to begin of record    */
    rec = (BYTE*) (hdr + 1);          /* point past TFHDR to the rec */
    cpuad = (BYTE) hdr->cpuad;        /* which CPU this rec is for   */

    /* Read in the remainder of this record. PROGRAMMING NOTE: we
       read the rest of the record into our I/O buffer at an offset
       that matches the CURRENT size of the THDR portion, such that
       the entire record that ends up in our buffer (TFHDR + record
       fields) ends up looking like a NEW (current) format record,
       and not the old format of the file being read and processed.

       This makes the rest of our trace file printing logic simpler
       as they can then simply access the record fields directly
       without being concerned about the difference in the THDR sizes
       between the old and new formats, since will never access any
       of the new TFHDR fields anyway without first checking if they
       actually exist beforehand.
    */
    amt = hdr->curr - hdr_size;
    if ((bytes_read = fread( rec, 1, amt, inf )) < 0)
    {
        /* "Error reading trace file: %s" */
        FWRMSG( stderr, HHC03206, "E", strerror( errno ));
        exit( -1 );
    }

    if (bytes_read < amt)
    {
        /* "Truncated %s record; aborting" */
        char buf[8];
        MSGBUF( buf, "%"PRIu16, msgnum );
        FWRMSG( stderr, HHC03207, "E", buf );
        exit( -1 );
    }

    /* AT THIS POINT, the trace record in our buffer now looks like
       what a NEW (current) format trace record looks like, with all
       of its fields aligned correctly. This makes the remainder of
       our processing logic much simpler and more straightforward.
    */

    /* Don't save this record if they're not interested in it */
    if (!is_wanted( hdr ))
        return false;

    /* Swap endianness of record if needed before saving it.
       Note that the TFHDR itself has already been swapped. */
    if (doendswap)
        tf_swap_rec( hdr, msgnum );

    /* Initialize some printing variables so spacing works right */
    if (first_time)
    {
        first_time = false;
        prvdevnum  = ~hdr->devnum;
        prvcpuad   = ~hdr->cpuad;
    }

    /* Save this record for possible later processing */
    memcpy( all_recs_ptr( cpuad, msgnum ), hdr, recsize( msgnum ));
    all_recs[ cpuad ].gotmask |= gotmask( msgnum );
    return true;
}

// (forward reference)
static void process_args( int argc, char* argv[] );

/*-------------------------------------------------------------------*/
/*                            MAIN                                   */
/*-------------------------------------------------------------------*/
int main( int argc, char* argv[] )
{
    TFSYS*  sys         = NULL;     /* Ptr to SYSTEM record          */
    TFHDR*  hdr         = NULL;     /* Ptr to record header          */
    size_t  bytes_read  = 0;        /* Number of BYTES read          */
    bool    was_bigend  = false;    /* Endianness of TFSYS record    */
    int     i;                      /* Work for iterating            */

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    /* Save for later */
    out_istty = isatty( fileno( stdout )) ? true : false;
    err_istty = isatty( fileno( stderr )) ? true : false;

    /* Process command-line arguments. Show help and exit if errors. */
    process_args( argc, argv );

    /* Read Trace File TFSYS record */
    if ((bytes_read = fread( iobuff, 1, sizeof( TFSYS ), inf )) != sizeof( TFSYS ))
    {
        // "Error reading trace file: %s"
        FWRMSG( stderr, HHC03206, "E", strerror( errno ));
        exit( -1 );
    }

    if (bytes_read < sizeof( TFSYS ))
    {
        // "Truncated %s record; aborting"
        FWRMSG( stderr, HHC03207, "E", "TFSYS" );
        exit( -1 );
    }

    /* Point to the TFSYS record we just read */
    sys = (TFSYS*) iobuff;

    /* Check the TFSYS record for compatibility */
    if (0
        || sys->ffmt[0] != '%'
        || sys->ffmt[1] != 'T'
        || sys->ffmt[2] != 'F'
    )
    {
        // "File does not start with TFSYS record; aborting"
        FWRMSG( stderr, HHC03212, "E" );
        exit( -1 );
    }

    if (0
        || sys->ffmt[3] < TF_FMT0
        || sys->ffmt[3] > TF_FMT
    )
    {
        // "Unsupported Trace File format: %%TF%c"
        FWRMSG( stderr, HHC03213, "E", sys->ffmt[3] );
        exit( -1 );
    }

    /* Remember the format of the file we're processing */
    sys_ffmt = sys->ffmt[3];

    /* Determine TFHDR size */
    if (sys_ffmt < TF_FMT2)
        hdr_size = offsetof( TFHDR, tidnum );

    if (sys->engs > MAX_CPU_ENGS)
    {
        // "Incompatible MAX_CPU_ENGS"
        FWRMSG( stderr, HHC03210, "E" );
        exit( -1 );
    }

    if (0
        || sys->archnum0 != _ARCH_NUM_0
        || sys->archnum1 != _ARCH_NUM_1
        || sys->archnum2 != _ARCH_NUM_2
    )
    {
        // "Incompatible Hercules build architectures"
        FWRMSG( stderr, HHC03216, "E" );
        exit( -1 );
    }

    /* Save actual endianness of the file */
    was_bigend = sys->bigend;

    /* Swap endianness of TFSYS record, if needed */
    if ((doendswap = tf_are_swaps_needed( sys )))
        tf_swap_sys( sys );

    /* Print TFSYS record information */
    print_TFSYS( sys, was_bigend );

    if (info_only)
        goto done;

    /* Now read and process trace file records until EOF is reached */
    hdr = (TFHDR*) iobuff;
    while (recnum < torec)
    {
        /* Read just TFHDR for now, so we can identify record */
        if ((bytes_read = fread( hdr, 1, hdr_size, inf )) != hdr_size)
        {
            /* Stop when EOF reached */
            if (!bytes_read)
                break;

            // "Error reading trace file: %s"
            FWRMSG( stderr, HHC03206, "E", strerror( errno ));
            exit( -1 );
        }

        /* Count records read and show progress */
        if (!(++recnum % 3000))
            show_file_progress();

        /* Make sure we have a complete header to work with */
        if (bytes_read < hdr_size)
        {
            // "Truncated %s record; aborting"
            FWRMSG( stderr, HHC03207, "E", "TFHDR" );
            exit( -1 );
        }

        /* Swap endianness of header, if needed */
        if (doendswap)
            tf_swap_hdr( sys_ffmt, hdr );

        /* Save the thread information */
        if (sys_ffmt >= TF_FMT2)
            tf_save_tidinfo( hdr );

        /* Fix the 'cpuad' if this is a device trace record */
        if (hdr->cpuad == 0xFFFF)
            hdr->cpuad = MAX_CPU_ENGS;

        /* Now finish reading the rest of the record (via the
           'finish_reading_rec()' function) and then process it
           afterwards, all based on the hdr's message number.
        */
        switch (hdr->msgnum)
        {

#define CASE_FOR_MSGNUM( _num )                                       \
                                                                      \
        case _num:                                                    \
        {                                                             \
            if (finish_reading_rec( _num ))                           \
                process_TF0  ## _num( (TF0 ## _num*) hdr );           \
            continue;                                                 \
        }

#define CASE_FOR_MSGNUM0( _num )                                      \
                                                                      \
        /* (same as above except "TF00" instead of "TF0") */          \
        case _num:                                                    \
        {                                                             \
            if (finish_reading_rec( _num ))                           \
                process_TF00 ## _num( (TF00 ## _num*) hdr );          \
            continue;                                                 \
        }

        // Instruction tracing...

        CASE_FOR_MSGNUM0( 800 ); // Wait State PSW
        CASE_FOR_MSGNUM0( 801 ); // Program Interrupt
        CASE_FOR_MSGNUM0( 802 ); // PER event
        CASE_FOR_MSGNUM0( 803 ); // Program interrupt loop
        CASE_FOR_MSGNUM0( 804 ); // I/O interrupt (S/370)

        CASE_FOR_MSGNUM0( 806 ); // I/O Interrupt
        CASE_FOR_MSGNUM0( 807 ); // Machine Check Interrupt
        CASE_FOR_MSGNUM0( 808 ); // Store Status
        CASE_FOR_MSGNUM0( 809 ); // Disabled Wait State

        CASE_FOR_MSGNUM0( 811 ); // Architecture mode
        CASE_FOR_MSGNUM0( 812 ); // Vector facility online (370/390)

        CASE_FOR_MSGNUM0( 814 ); // Signal Processor

        CASE_FOR_MSGNUM0( 840 ); // External Interrupt

        CASE_FOR_MSGNUM0( 844 ); // Block I/O Interrupt
        CASE_FOR_MSGNUM0( 845 ); // Block I/O External interrupt
        CASE_FOR_MSGNUM0( 846 ); // Service Signal External Interrupt

        CASE_FOR_MSGNUM( 2266 ); // Vector Register
        CASE_FOR_MSGNUM( 2269 ); // General Registers
        CASE_FOR_MSGNUM( 2270 ); // Floating Point Registers
        CASE_FOR_MSGNUM( 2271 ); // Control Registers
        CASE_FOR_MSGNUM( 2272 ); // Access Registers

        CASE_FOR_MSGNUM( 2276 ); // Floating Point Control Register

        CASE_FOR_MSGNUM( 2324 ); // Instruction Trace

        CASE_FOR_MSGNUM( 2326 ); // Instruction Operands

        // Device tracing ...

        CASE_FOR_MSGNUM0( 423 ); // Search key
        CASE_FOR_MSGNUM0( 424 ); // Cur trk
        CASE_FOR_MSGNUM0( 425 ); // Updating track
        CASE_FOR_MSGNUM0( 426 ); // Cache hit
        CASE_FOR_MSGNUM0( 427 ); // Unavailable cache
        CASE_FOR_MSGNUM0( 428 ); // Cache miss
        CASE_FOR_MSGNUM0( 429 ); // Offset
        CASE_FOR_MSGNUM0( 430 ); // Trkhdr
        CASE_FOR_MSGNUM0( 431 ); // Seeking
        CASE_FOR_MSGNUM0( 432 ); // MT advance error
        CASE_FOR_MSGNUM0( 433 ); // MT advance
        CASE_FOR_MSGNUM0( 434 ); // Read count orientation
        CASE_FOR_MSGNUM0( 435 ); // Cyl head record kl dl
        CASE_FOR_MSGNUM0( 436 ); // Read key
        CASE_FOR_MSGNUM0( 437 ); // Read data
        CASE_FOR_MSGNUM0( 438 ); // Write cyl head record kl dl
        CASE_FOR_MSGNUM0( 439 ); // Set track overflow flag
        CASE_FOR_MSGNUM0( 440 ); // Update cyl head record kl dl
        CASE_FOR_MSGNUM0( 441 ); // Update cyl head record dl
        CASE_FOR_MSGNUM0( 442 ); // Set file mask

        CASE_FOR_MSGNUM0( 516 ); // Cache hit
        CASE_FOR_MSGNUM0( 517 ); // Unavailable cache
        CASE_FOR_MSGNUM0( 518 ); // Cache miss
        CASE_FOR_MSGNUM0( 519 ); // Offset len
        CASE_FOR_MSGNUM0( 520 ); // Positioning

        CASE_FOR_MSGNUM( 1300 ); // Halt subchannel
        CASE_FOR_MSGNUM( 1301 ); // IDAW/MIDAW

        CASE_FOR_MSGNUM( 1304 ); // Attention signaled
        CASE_FOR_MSGNUM( 1305 ); // Attention
        CASE_FOR_MSGNUM( 1306 ); // Initial status interrupt
        CASE_FOR_MSGNUM( 1307 ); // Attention completed
        CASE_FOR_MSGNUM( 1308 ); // Clear completed
        CASE_FOR_MSGNUM( 1309 ); // Halt completed
        CASE_FOR_MSGNUM( 1310 ); // Suspended
        CASE_FOR_MSGNUM( 1311 ); // Resumed
        CASE_FOR_MSGNUM( 1312 ); // I/O stat
        CASE_FOR_MSGNUM( 1313 ); // Sense

        CASE_FOR_MSGNUM( 1315 ); // CCW
        CASE_FOR_MSGNUM( 1316 ); // CSW (370)
        CASE_FOR_MSGNUM( 1317 ); // SCSW
        CASE_FOR_MSGNUM( 1318 ); // TEST I/O

        CASE_FOR_MSGNUM( 1320 ); // S/370 SIO conversion started
        CASE_FOR_MSGNUM( 1321 ); // S/370 SIO conversion success

        CASE_FOR_MSGNUM( 1329 ); // Halt I/O
        CASE_FOR_MSGNUM( 1330 ); // HIO modification
        CASE_FOR_MSGNUM( 1331 ); // Clear subchannel
        CASE_FOR_MSGNUM( 1332 ); // Halt subchannel
        CASE_FOR_MSGNUM( 1333 ); // Resume subchannel
        CASE_FOR_MSGNUM( 1334 ); // ORB

        CASE_FOR_MSGNUM( 1336 ); // Startio cc=2

        default:

            // "Unsupported Trace File record: msgnum %"PRIu16
            FWRMSG( stderr, HHC03214, "W", hdr->msgnum );

        } /* end switch */
    } /* end while */

    /* Show 100% of file processed */
    show_file_progress();

    /* Print totals */
    {
        char inscnt[ 32] = {0};
        char tiocnt[ 32] = {0};

        fmt_S64( inscnt, (S64) totins );
        fmt_S64( tiocnt, (S64) totios );

        printf( "\n" );

        // "%s %s printed"
        FWRMSG( stdout, HHC03215, "I", inscnt, "instructions" );
        FWRMSG( stdout, HHC03215, "I", tiocnt, "device I/O's" );
    }

    /* List thread-id numbers and their corresponding names... */
    if (sys_ffmt >= TF_FMT2)
    {
        printf( "\n" );

        /* Sort the table into a more user-friendly name sequence */
        qsort( tidtab, numtids, sizeof( TIDTAB ), sort_tidtab_by_thrdname );

        /* Now print the table */
        for (i=0; i < numtids; ++i)
        {
            // Thread Id "TIDPAT" is %s"
            FWRMSG( stdout, HHC03223, "I", tidtab[i].tidnum, tidtab[i].thrdname );
        }
    }

done:

    /* Free resources, close input file and exit */
    free( tidtab );
    fclose( inf );
    return info_only ? 0 : ((totins > 0 || totios > 0) ? 0 : 1);

} /* end function main */

/*-------------------------------------------------------------------*/
/*             Show them their command line arguments                */
/*-------------------------------------------------------------------*/
static void print_args( int argc, char* argv[] )
{
    // PROGRAMMING NOTE: I know of no other way to accomplish this!

    int i; bool hasblanks;

    printf( "\n" );

    /* If debug mode, show filename and line number prefix
       the same way the logmsg.c "vfwritemsg" function does. */
    if (MLVL( DEBUG ))
    {
        char wrk    [ MLVL_DEBUG_PFXLEN + 2 ];
        char prefix [ MLVL_DEBUG_PFXLEN + 2 ];

        MSGBUF( wrk, "%s(%d)", TRIMLOC( __FILE__ ), __LINE__ );
        MSGBUF( prefix, MLVL_DEBUG_PFXFMT, wrk );
        printf( "%s", prefix );
    }

    // PROGRAMMING NOTE: we purposely do *NOT* try to print everything
    // into a buffer and then do one printf of that buffer since we do
    // not know how big that buffer should be. Some arguments could be
    // quite long! (Yeah, yeah, we COULD use malloc/realloc much like
    // BFR_VSNPRINTF does, but that would be overkill IMHO).

    // HHC03217 "Args: %s"
    printf( "HHC03217I Args:" );

    for (i=1; i < argc; ++i)
    {
        printf( " " );  // (separate arguments with a blank)

        if ((hasblanks = strpbrk( argv[i], WHITESPACE ) ? true : false))
            printf( "\"" ); // (enclose within quotes if needed)

        printf( "%s", argv[i] );  // (the argument, as-is)

        if (hasblanks)
            printf( "\"" );
    }

    printf( "\n\n" );
}

/*-------------------------------------------------------------------*/
/*                        Options                                    */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  Usage:  tfprint  [options...] tracefile                          */
/*                                                                   */
/*         [-i|--info]                                               */
/*         [-c|--cpu hh[[-hh][,hh]]                                  */
/*         [-r|--traceopt TRADITIONAL|REGSFIRST|NOREGS]              */
/*         [-n|--count nnnnnn[[-nnnnnn]|[.nnn]]                      */
/*         [-e|--msg nnnnn[,nnnnn]                                   */
/*         [-s|--storage V|R:hhhhhh[[-hhhhhh]|[.hhh]]                */
/*         [-d|--date YYYY/MM/DD-YYYY/MM/DD                          */
/*         [-t|--time HH:MM:SS.nnnnnn-HH:MM:SS.nnnnnn                */
/*         [-o|--opcode hhhhhhhh[,hhxxhhxxhhhh]                      */
/*         [-m|--msglvl xxxxx                                        */
/*         [-u|--unit uuuu[[-uuuu]|[.nnn]]                           */
/*         [-p]--codepage zzzzzzzz                                   */
/*                                                                   */
/*    -i   Print only TFSYS header information then exit             */
/*    -c   Print only specified CPU(s)                               */
/*    -r   Print registers trace option                              */
/*    -n   Print only records nnnnnn to nnnnnn (by count)            */
/*    -e   Print only messages with specified message number         */
/*    -s   Print only instructions that reference or modify          */
/*         the specified 'V'irtual or 'R'eal storage range           */
/*    -d   Print only records within specified date range            */
/*    -t   Print only records within specified time range            */
/*    -o   Print only specified instruction(s) (by opcode)           */
/*         Only as many bytes are checked as are specified.          */
/*         Use 'x' as wildcard for nibbles not to be checked         */
/*    -m   Modify Hercules 'msglvl' setting                          */
/*    -u   Print only trace records for specified I/O unit(s)        */
/*    -p   Use EBCDIC/ASCII codepage conversion table zzzzzzzz       */
/*                                                                   */
/*-------------------------------------------------------------------*/

static char shortopts[] = ":ic:r:n:e:s:d:t:o:m:u:p:";

static struct option longopts[] =
{
    { "info",     no_argument,       NULL, 'i' },
    { "cpu",      required_argument, NULL, 'c' },
    { "traceopt", required_argument, NULL, 'r' },
    { "count",    required_argument, NULL, 'n' },
    { "msg",      required_argument, NULL, 'e' },
    { "storage",  required_argument, NULL, 's' },
    { "date",     required_argument, NULL, 'd' },
    { "time",     required_argument, NULL, 't' },
    { "opcode",   required_argument, NULL, 'o' },
    { "msglvl",   required_argument, NULL, 'm' },
    { "unit",     required_argument, NULL, 'u' },
    { "codepage", required_argument, NULL, 'p' },
    { NULL,       0,                 NULL,  0  }
};

#if defined( HAVE_GETOPT_LONG )
  #define GET_OPTION() \
    getopt_long( argc, argv, shortopts, longopts, NULL )
#else
  #define GET_OPTION() \
    getopt( argc, argv, shortopts )
#endif

/*-------------------------------------------------------------------*/
/*             (option parsing helper macros)                        */
/*-------------------------------------------------------------------*/

#define MISSING_OPTNAME             argv[ optind-1 ]
#define OUR_OPTNAME                 argv[ optind-2 ]

#define MISSING_REQUIRED_ARG()                              \
                                                            \
    if (!optarg)                                            \
    {                                                       \
        /* "Missing argument for option %s" */              \
        FWRMSG( stderr, HHC03204, "E", MISSING_OPTNAME );   \
        arg_errs++;                                         \
        break;                                              \
    }

#define PARSE_OPTION_NOARG_CASE( _name )                    \
                                                            \
    parse_option_ ## _name( OUR_OPTNAME );                  \
    break

#define PARSE_OPTION_CASE( _name )                          \
                                                            \
    MISSING_REQUIRED_ARG();                                 \
    PARSE_OPTION_NOARG_CASE( _name )

/*-------------------------------------------------------------------*/
/*          Option parsing functions forward references              */
/*-------------------------------------------------------------------*/

static void  parse_option_msglvl   ( const char* optname );
static void  parse_option_info     ( const char* optname );
static void  parse_option_cpu      ( const char* optname );
static void  parse_option_traceopt ( const char* optname );
static void  parse_option_count    ( const char* optname );
static void  parse_option_msg      ( const char* optname );
static void  parse_option_storage  ( const char* optname );
static void  parse_option_date     ( const char* optname );
static void  parse_option_time     ( const char* optname );
static void  parse_option_opcode   ( const char* optname );
static void  parse_option_unit     ( const char* optname );
static void  parse_option_codepage ( const char* optname );
static void  parse_tracefile       ( const char* filename );

/*-------------------------------------------------------------------*/
/*      function to calculate time zone offset from UTC              */
/*-------------------------------------------------------------------*/
static time_t  time_zone_offset()
{
    time_t  gmttime;
    time_t  rawtime;
    TM*     pTM;

    rawtime = time( NULL );
    pTM = gmtime( &rawtime );
    pTM->tm_isdst = -1;
    gmttime = mktime( pTM );

    return (time_t) difftime( rawtime, gmttime );
}

/*-------------------------------------------------------------------*/
/*         process_args - Parse command-line arguments               */
/*-------------------------------------------------------------------*/
static void process_args( int argc, char* argv[] )
{
    int  c = 0;

    /* First, show them exactly WHAT we'll be parsing... */
    print_args( argc, argv );

    // We'll print our own error messages thank you very much! */
    opterr = 0;

    /* Parse the command-line... */
    for (; EOF != c;)
    {
        switch (c = GET_OPTION())
        {
            case 0:     // getopt_long() set a variable for us;
                break;  // do nothing and continue...

            case EOF:   // No more options! (i.e. we're done)
                break;

            case '?':   // Invalid/unsupported option
            {
                // "Invalid/unsupported option: %s"
                FWRMSG( stderr, HHC03219, "E", MISSING_OPTNAME );
                arg_errs++;
                break;
            }

            default:    // Missing option argument

                // "Missing argument for option %s"
                FWRMSG( stderr, HHC03204, "E", MISSING_OPTNAME );
                arg_errs++;
                break;

            case 'i': PARSE_OPTION_NOARG_CASE( info );

            case 'c': PARSE_OPTION_CASE( cpu      );
            case 'r': PARSE_OPTION_CASE( traceopt );
            case 'n': PARSE_OPTION_CASE( count    );
            case 'e': PARSE_OPTION_CASE( msg      );
            case 's': PARSE_OPTION_CASE( storage  );
            case 'd': PARSE_OPTION_CASE( date     );
            case 't': PARSE_OPTION_CASE( time     );
            case 'o': PARSE_OPTION_CASE( opcode   );
            case 'm': PARSE_OPTION_CASE( msglvl   );
            case 'u': PARSE_OPTION_CASE( unit     );
            case 'p': PARSE_OPTION_CASE( codepage );

        } // end switch
    } // end for

    /* Any non-option argument is presumed to be the tracefile name */
    if (argv[ optind ])
        parse_tracefile( argv[ optind++ ]);

    /* Any arguments remaining on the command-line are errors */
    if (!arg_errs)
    {
        while (optind < argc)
        {
            // "Invalid/unsupported option: %s"
            FWRMSG( stderr, HHC03219, "E", argv[ optind++ ] );
            arg_errs++;
        }
    }

    /* The name of the input tracefile is required */
    if (!pathname[0])
    {
        // "Missing input-file specification"
        FWRMSG( stderr, HHC03201, "E" );
        arg_errs++;
    }

    /* Fix 'beg_tim' and 'end_tim' as needed */
    if (beg_tim.tv_sec || beg_tim.tv_usec ||
        end_tim.tv_sec || end_tim.tv_usec)
    {
        if (!beg_dat || !end_dat)
        {
            // "--date range is required when --time range specified"
            FWRMSG( stderr, HHC03220, "E" );
            arg_errs++;
        }
        else // (adjust beg/end date/time range)
        {
            time_t  tz_offset  = time_zone_offset();

            /* Add date to time tange */
            beg_tim.tv_sec += beg_dat;
            end_tim.tv_sec += end_dat;

            /* Compensate for timezone */
            beg_tim.tv_sec += tz_offset;
            end_tim.tv_sec += tz_offset;
        }
    }

    /* Terminate if any errors were detected */
    if (arg_errs)
        show_usage();   // (does not return)

} // end process_args

/*-------------------------------------------------------------------*/
/*       Helper macro    (temporary during development)              */
/*-------------------------------------------------------------------*/

#define UNIMPLEMENTED_OPTION()                              \
                                                            \
    /* "Option %s has not been implemented yet!" */         \
    FWRMSG( stderr, HHC03203, "W", optname )

/*-------------------------------------------------------------------*/
/*  Parse option info:  (no argument)                                */
/*-------------------------------------------------------------------*/
static void parse_option_info( const char* optname )
{
    UNREFERENCED( optname );
    info_only = true;
}

/*-------------------------------------------------------------------*/
/*  Parse option msglvl:  xxxx...  (see Hercules documentation)      */
/*-------------------------------------------------------------------*/
static void parse_option_msglvl( const char* optname )
{
    char msglvl_cmd[64];
    UNREFERENCED( optname );
    MSGBUF( msglvl_cmd, "msglvl %s", optarg );
    if ((S64)the_real_panel_command( msglvl_cmd ) < 0)
        arg_errs++;
}

/*-------------------------------------------------------------------*/
/*  typedef for below parse_opt_str convert_opt_str helper function  */
/*-------------------------------------------------------------------*/
typedef bool CNVOPT( bool, const char*, U64*, MOPT* );

/*-------------------------------------------------------------------*/
/*  Convert numeric or hexadecimal charater string to binary         */
/*-------------------------------------------------------------------*/
static bool convert_opt_str( bool ishex, const char* str, U64* pU64, MOPT* pMOPT )
{
    UNREFERENCED( pMOPT );

    if (ishex)
    {
        if (!is_hex( str ))
            return false;
        VERIFY( sscanf( str, "%"SCNx64, pU64 ) == 1 );
    }
    else
    {
        if (!is_numeric( str ))
            return false;
        VERIFY( sscanf( str, "%"SCNu64, pU64 ) == 1 );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*  Parse delimited option string e.g. xxx-xxx,xxx.xxx,xxx ...       */
/*-------------------------------------------------------------------*/
static bool parse_opt_str
(
    bool         ishex,     // [in] True = hex option, false = numeric
    bool         rangeok,   // [in] True = "xxx-xxx" or "nnn.nn" okay
    const char*  optstr,    // [in] Pointer to option string
    CNVOPT*      pCNVOPT,   // [in] Pointer to opt str convert function
    MOPT**       ppMOPT,    // [out] Address of MOPT array (caller
                            //       responsible for freeing array!)
    int*         nMOPT      // [out] Address of MOPT array count
)
{
    int   i, n, k;
    char  work[32] = {0};
    bool  range = false;    // "xxx-xxx"
    bool  dotlen = false;   // "xxx.xxx"

    *nMOPT = 0;
    *ppMOPT = malloc(   sizeof( MOPT ));
    memset( *ppMOPT, 0, sizeof( MOPT ));

    if (!pCNVOPT)                   // (if not specified)
        pCNVOPT = &convert_opt_str; // (then use default)

    for (i=0, n=0, k = strlen( optstr ); i < k; ++i)
    {
        if (0
            || optstr[i] == '-'
            || optstr[i] == '.'
        )
        {
            if (!rangeok)           // Is range syntax allowed?
                return false;       // No, error

            work[n] = 0;            // Mark end of value string
            n = 0;                  // Next char begins new value

            if (range)              // Not first time here? ("xx-xx-xx")
                return false;       // Yes, error

            range = true;           // Remember range delimiter seen
            dotlen = (optstr[i] == '.');

            // Save opt1 start of range

            (*ppMOPT)[ *nMOPT ].isrange = true;

            if (!pCNVOPT( ishex, work, &(*ppMOPT)[ *nMOPT ].opt1, *ppMOPT ))
                return false;
        }
        else if (optstr[i] == ',')
        {
            work[n] = 0;            // Mark end of value string
            n = 0;                  // Next char begins new value

            if (range)              // Was previous delimiter range?
            {
                // Save opt2 end of range

                (*ppMOPT)[ *nMOPT ].isrange = true;

                if (!pCNVOPT( ishex, work, &(*ppMOPT)[ *nMOPT ].opt2, *ppMOPT ))
                    return false;

                if (dotlen)
                {
                    if (!(*ppMOPT)[ *nMOPT ].opt2)
                        return false;

                    (*ppMOPT)[ *nMOPT ].opt2 += (*ppMOPT)[ *nMOPT ].opt1;
                    (*ppMOPT)[ *nMOPT ].opt2--;
                }

                range = false;      // Range complete; reset flag
                dotlen = false;     // Range complete; reset flag
            }
            else
            {
                // Save lone opt1 value

                (*ppMOPT)[ *nMOPT ].isrange = false;

                if (!pCNVOPT( ishex, work, &(*ppMOPT)[ *nMOPT ].opt1, *ppMOPT ))
                    return false;
            }

            // Need another entry for next range or next lone value

            *nMOPT += 1;
            *ppMOPT = realloc( *ppMOPT, (*nMOPT + 1) * sizeof( MOPT ));
            memset( &(*ppMOPT)[ *nMOPT ], 0,  sizeof( MOPT ));
        }
        else // (anything else is a value character)
        {
            if (n >= (int)sizeof( work)) // Room for another char in work?
                return false;            // No, error
            work[n++] = optstr[i];       // Save value character
        }
    }

    // End of string reached. Convert final value.

    if (!n)                 // Can only occur if e.g. "xx," or "xx-"
        return false;       // Syntax error

    work[n] = 0;            // Mark end of value string

    if (range)              // Was previous delimiter range?
    {
        // Save opt2 end of range

        (*ppMOPT)[ *nMOPT ].isrange = true;

        if (!pCNVOPT( ishex, work, &(*ppMOPT)[ *nMOPT ].opt2, *ppMOPT ))
            return false;

        if (dotlen)
        {
            if (!(*ppMOPT)[ *nMOPT ].opt2)
                return false;

            (*ppMOPT)[ *nMOPT ].opt2 += (*ppMOPT)[ *nMOPT ].opt1;
            (*ppMOPT)[ *nMOPT ].opt2--;
        }
    }
    else
    {
        // Save opt1 only value

        (*ppMOPT)[ *nMOPT ].isrange = false;

        if (!pCNVOPT( ishex, work, &(*ppMOPT)[ *nMOPT ].opt1, *ppMOPT ))
            return false;
    }

    *nMOPT += 1;            // Account for last array entry
    return true;            // Successful parse
}

/*-------------------------------------------------------------------*/
/*  Issue HHC03205E invalid option error message                     */
/*-------------------------------------------------------------------*/
static void parse_option_error( const char* optname )
{
    // "Option \"%s\" value \"%s\" is invalid"
    FWRMSG( stderr, HHC03205, "E", optname, optarg );
    arg_errs++;
}

/*-------------------------------------------------------------------*/
/*  Parse option cpu:  hh[[-hh][,hh]                                 */
/*-------------------------------------------------------------------*/
static void parse_option_cpu( const char* optname )
{
    MOPT* pMOPT = NULL;
    int   nMOPT = 0;
    int   i, cpu1, cpu2;
    bool  ishex = true;
    bool  rangeok = true;

    if (!parse_opt_str( ishex, rangeok, optarg, NULL, &pMOPT, &nMOPT ))
        goto opt_error;

    for (i=0; i < nMOPT; ++i)
    {
        if (0
            || pMOPT[i].opt1 >= MAX_CPU_ENGS
            || pMOPT[i].opt2 >= MAX_CPU_ENGS
        )
            goto opt_error;

        if (!pMOPT[i].isrange)
        {
            cpu1 = (int) pMOPT[i].opt1;
            cpu_map |= CPU_BIT( cpu1 );
            continue;
        }

        cpu1 = (int) pMOPT[i].opt1;
        cpu2 = (int) pMOPT[i].opt2;

        if (cpu1 > cpu2)
            goto opt_error;

        for (; cpu1 <= cpu2; ++cpu1)
            cpu_map |= CPU_BIT( cpu1 );
    }

    goto opt_return;

opt_error:

    parse_option_error( optname );

opt_return:

    free( pMOPT );
}

/*-------------------------------------------------------------------*/
/*  Parse option traceopt:  TRADITIONAL|REGSFIRST|NOREGS             */
/*-------------------------------------------------------------------*/
static void parse_option_traceopt( const char* optname )
{
    if (str_caseless_eq( "NOREGS", optarg ))
    {
        regsfirst = false;
        noregs    = true;
    }
    else
    if (str_caseless_eq( "REGSFIRST", optarg ))
    {
        regsfirst = true;
        noregs    = false;
    }
    else
    if (str_caseless_eq( "TRADITIONAL", optarg ))
    {
        regsfirst = false;
        noregs    = false;
    }
    else
        parse_option_error( optname );
}

/*-------------------------------------------------------------------*/
/*  Parse option count:  nnnnnn[[-nnnnnn]|[.nnn]]                    */
/*-------------------------------------------------------------------*/
static void parse_option_count( const char* optname )
{
    MOPT* pMOPT = NULL;
    int   nMOPT = 0;
    bool  ishex = false;
    bool  rangeok = true;

    if (0
        || !parse_opt_str( ishex, rangeok, optarg, NULL, &pMOPT, &nMOPT )
        || nMOPT != 1
    )
        goto opt_error;

    if (pMOPT->isrange)
    {
        fromrec = pMOPT->opt1;
        torec   = pMOPT->opt2;
    }
    else
    {
        if (!pMOPT->opt1)
            goto opt_error;

        fromrec = 0;
        torec   = pMOPT->opt1;
    }

    goto opt_return;

opt_error:

    parse_option_error( optname );

opt_return:

    free( pMOPT );
}

/*-------------------------------------------------------------------*/
/*  Parse option msg:  nnnnn[,nnnnn]]                                */
/*-------------------------------------------------------------------*/
static void parse_option_msg( const char* optname )
{
    bool ishex = false;
    bool rangeok = false;

    if (parse_opt_str( ishex, rangeok, optarg, NULL, &pMsgMOPT, &nMsgMOPT ))
        return;

    parse_option_error( optname );
    free( pMsgMOPT );
    pMsgMOPT = NULL;
    nMsgMOPT = 0;
}

/*-------------------------------------------------------------------*/
/*  Convert storage option string to binary                          */
/*-------------------------------------------------------------------*/
static bool convert_storage_opt_str( bool ishex, const char* str, U64* pU64, MOPT* pMOPT )
{
    if (!ishex)                 // storage addrs must always be in hex
        return false;

    if (str[1] == ':')
    {
        char str0 = toupper( (unsigned char)str[0] );

        if (1
            && str0 != 'V'      // Virtual address?
            && str0 != 'R'      // Real address?
        )
            return false;

        pMOPT->isreal = (str0 == 'R');
        str += 2; // (get past "V:" or "R:")
    }

    if (!is_hex( str ))
        return false;

    VERIFY( sscanf( str, "%"SCNx64, pU64 ) == 1 );
    return true;
}

/*-------------------------------------------------------------------*/
/*  Parse option storage:  V/R:hhhhhh[[-hhhhhh]|[.hhh]]              */
/*-------------------------------------------------------------------*/
static void parse_option_storage( const char* optname )
{
    bool ishex = true;
    bool rangeok = true;

    if (parse_opt_str( ishex, rangeok, optarg, &convert_storage_opt_str,
                       &pStorMOPT, &nStorMOPT ))
        return;

    parse_option_error( optname );
    free( pStorMOPT );
    pStorMOPT = NULL;
    nStorMOPT = 0;
}

/*-------------------------------------------------------------------*/
/*  Parse "YYYY/MM/DD"  ===>  time_t                                 */
/*-------------------------------------------------------------------*/
static bool parse_date_str( char* optstr, time_t* tt )
{
    char *pYY, *pSlash1, *pMM, *pSlash2, *pDD;
    U64    YY,             MM,             DD;
    TM     tm;

    if (!(pSlash1 = strchr( pYY = optstr, '/' )))
        return false;

    if (!(pSlash2 = strchr( pMM = pSlash1+1, '/' )))
        return false;

    pDD = pSlash2+1;

    *pSlash1 = *pSlash2 = 0;

    if (0
        || !is_numeric( pYY )
        || !is_numeric( pMM )
        || !is_numeric( pDD )
    )
        return false;

    VERIFY( sscanf( pYY, "%"SCNu64, &YY ) == 1 );
    VERIFY( sscanf( pMM, "%"SCNu64, &MM ) == 1 );
    VERIFY( sscanf( pDD, "%"SCNu64, &DD ) == 1 );

    if (0
        || YY <     1
        || YY >= 2038           // (2038 epoch bug)
        || MM <     1
        || MM >    12
        || DD <     1
        || DD >    31
    )
        return false;

    tm.tm_year  =  YY-1900;     // Years since 1900
    tm.tm_mon   =  MM-1;        // Months since January     - [0,11]
    tm.tm_mday  =  DD;          // Day of the month         - [1,31]

    tm.tm_hour  =  0;           // Hours since midnight     - [0,23]
    tm.tm_min   =  0;           // Minutes after the hour   - [0,59]
    tm.tm_sec   =  0;           // Seconds after the minute - [0,59]

    tm.tm_wday  =  0;           // Days since Sunday        - [0,6]
    tm.tm_yday  =  0;           // Days since January 1     - [0,365]
    tm.tm_isdst = -1;           // Daylight savings time flag

    if ((*tt = mktime( &tm )) < 0)
        return false;

    return true;
}

/*-------------------------------------------------------------------*/
/*  Parse "HH:MM:SS.nnnnnn"  ===>  timeval                           */
/*-------------------------------------------------------------------*/
static bool parse_time_str( char* optstr, TIMEVAL* tv )
{
    char *pHH, *pCol1, *pMM, *pCol2, *pSS, *pDot, *pNN;
    U64    HH,           MM,           SS,          NN;
    TM     tm;
    time_t secs;
    size_t nn_len;

    if (!(pCol1 = strchr( pHH = optstr, ':' )))
        return false;

    if (!(pCol2 = strchr( pMM = pCol1+1, ':' )))
        return false;

    if (!(pDot = strchr( pSS = pCol2+1, '.' )))
        return false;

    pNN = pDot+1;

    *pCol1 = *pCol2 = *pDot = 0;

    if (0
        || !is_numeric( pHH )
        || !is_numeric( pMM )
        || !is_numeric( pSS )
        || !is_numeric( pNN )
    )
        return false;

    VERIFY( sscanf( pHH, "%"SCNu64, &HH ) == 1 );
    VERIFY( sscanf( pMM, "%"SCNu64, &MM ) == 1 );
    VERIFY( sscanf( pSS, "%"SCNu64, &SS ) == 1 );
    VERIFY( sscanf( pNN, "%"SCNu64, &NN ) == 1 );

    if (0
        || HH > 23
        || MM > 59
        || SS > 59
        || NN > 999999
    )
        return false;

    tm.tm_hour  =  HH;          // Hours since midnight     - [0,23]
    tm.tm_min   =  MM;          // Minutes after the hour   - [0,59]
    tm.tm_sec   =  SS;          // Seconds after the minute - [0,59]

    tm.tm_year  =  1970-1900;   // Years since 1900
    tm.tm_mon   =  0;           // Months since January     - [0,11]
    tm.tm_mday  =  1;           // Day of the month         - [1,31]

    tm.tm_wday  =  0;           // Days since Sunday        - [0,6]
    tm.tm_yday  =  0;           // Days since January 1     - [0,365]
    tm.tm_isdst = -1;           // Daylight savings time flag

    if ((secs = mktime( &tm )) < 0)
        return false;

    for (nn_len = strlen( pNN ); nn_len < 6; ++nn_len)
        NN *= 10;

    tv->tv_sec  = secs;
    tv->tv_usec = NN;

    return true;
}

/*-------------------------------------------------------------------*/
/*  Parse option date:  YYYY/MM/DD-YYYY/MM/DD                        */
/*-------------------------------------------------------------------*/
static void parse_option_date( const char* optname )
{
    char  begstr[11] = {0};   // "YYYY/MM/DD"
    char  endstr[11] = {0};   // "YYYY/MM/DD"
    char* pDash;

    if (!(pDash = strchr( optarg, '-' )))
        goto opt_error;

    *pDash = 0;
    STRLCPY( begstr, optarg );
    STRLCPY( endstr, pDash+1);
    *pDash = '-';

    if (!parse_date_str( begstr, &beg_dat ))
        goto opt_error;

    if (!parse_date_str( endstr, &end_dat ))
        goto opt_error;

    // Validate parsed value

    if (beg_dat <= end_dat)
        return;

opt_error:

    parse_option_error( optname );
}

/*-------------------------------------------------------------------*/
/*  Parse option time:  HH:MM:SS.nnnnnn-HH:MM:SS.nnnnnn              */
/*-------------------------------------------------------------------*/
static void parse_option_time( const char* optname )
{
    char  begstr[16] = {0};   // "HH:MM:SS.nnnnnn"
    char  endstr[16] = {0};   // "HH:MM:SS.nnnnnn"
    char* pDash;

    if (!(pDash = strchr( optarg, '-' )))
        goto opt_error;

    *pDash = 0;
    STRLCPY( begstr, optarg );
    STRLCPY( endstr, pDash+1);
    *pDash = '-';

    if (!parse_time_str( begstr, &beg_tim ))
        goto opt_error;

    if (!parse_time_str( endstr, &end_tim ))
        goto opt_error;

    // Validate parsed value

    if (beg_tim.tv_sec > end_tim.tv_sec)
        goto opt_error;

    if (beg_tim.tv_sec < end_tim.tv_sec)
        return;

    if (beg_tim.tv_usec <= end_tim.tv_usec)
        return;

opt_error:

    parse_option_error( optname );
}

/*-------------------------------------------------------------------*/
/*  Convert opcode option string hhxxhhxxhhhh to binary              */
/*-------------------------------------------------------------------*/
static BYTE c2x( char c /* presumed valid uppercase hex char */)
{
    if (c <='9') return (c - '0') +  0;
    else         return (c - 'A') + 10;
}
static bool convert_opcode_opt_str( bool ishex, const char* str, U64* pU64, MOPT* pMOPT )
{
    size_t  i, n;
    BYTE    left_inst, right_inst;
    BYTE    left_mask, right_mask;
    char    c;

    *pU64 = 0;                  // (not used)

    if (!ishex)                 // Instructions always specified in hex
        return false;

    if (1
        && (n = strlen( str )) != 4
        &&  n                  != 8
        &&  n                  != 12
    )
        return false;               // Not proper length

    for (i=0; i < n; i += 2)
    {
        c = toupper( (unsigned char)str[i] );      // Left nibble

        if (is_hex_l( &c, 1 ))
        {
            left_inst = c2x( c );
            left_mask = 0xF;
        }
        else
        {
            if (c != 'X')
                return false;
            left_inst = 0;
            left_mask = 0;
        }

        c = toupper( (unsigned char)str[i+1] );    // Right nibble

        if (is_hex_l( &c, 1 ))
        {
            right_inst = c2x( c );
            right_mask = 0xF;
        }
        else
        {
            if (c != 'X')
                return false;
            right_inst = 0;
            right_mask = 0;
        }

        pMOPT->icode[ i >> 1 ] = (left_inst << 4) | right_inst;
        pMOPT->imask[ i >> 1 ] = (left_mask << 4) | right_mask;
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*  Parse option opcode:  hhhhhhhh[,hhxxhhxxhhhh]                    */
/*-------------------------------------------------------------------*/
static void parse_option_opcode( const char* optname )
{
    bool ishex = true;
    bool rangeok = false;

    if (parse_opt_str( ishex, rangeok, optarg, &convert_opcode_opt_str,
                       &pInstMOPT, &nInstMOPT ))
        return;

    parse_option_error( optname );
    free( pInstMOPT );
    pInstMOPT = NULL;
    nInstMOPT = 0;
}

/*-------------------------------------------------------------------*/
/*  Parse option unit:  uuuu[[-uuuu]|[.nnn]]                         */
/*-------------------------------------------------------------------*/
static void parse_option_unit( const char* optname )
{
    bool ishex = true;
    bool rangeok = true;

    if (parse_opt_str( ishex, rangeok, optarg, NULL, &pUnitMOPT, &nUnitMOPT ))
        return;

    parse_option_error( optname );
    free( pMsgMOPT );
    pMsgMOPT = NULL;
    nMsgMOPT = 0;
}

/*-------------------------------------------------------------------*/
/*  Parse option codepage:  zzzzzzzz                                 */
/*-------------------------------------------------------------------*/
static void parse_option_codepage( const char* optname )
{
    if (!valid_codepage_name( optarg ))
        parse_option_error( optname );
    else
        set_codepage( optarg );
}

/*-------------------------------------------------------------------*/
/*  Parse and open tracefile                                         */
/*-------------------------------------------------------------------*/
static void parse_tracefile( const char* filename )
{
    off_t  off_size;    // (file size)

    /* ALWAYS save the filename if it was provided to us */
    hostpath( pathname, filename, sizeof( pathname ));

    /* Don't bother opening it if we're going to be aborting anyway */
    if (arg_errs)
        return;         // (quick exit)

#if defined( _MSVC_ )
#define FOPEN_MODE    "rbS"     // ('S' = cached sequential access)
#else
#define FOPEN_MODE    "rb"
#endif

    /* Otherwise open it and determine its size */
    if (!(inf = fopen( pathname, FOPEN_MODE )))
    {
        // "Error opening \"%s\": %s"
        FWRMSG( stderr, HHC03202, "E", pathname, strerror( errno ));
        arg_errs++;
    }
    else
    {
        /* Save file size */
        if (fseek( inf, 0, SEEK_END ) < 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC00075, "E", "fseek", strerror( errno ));
            arg_errs++;
        }
        else
        {
            if ((off_size = ftell( inf )) < 0)
            {
                // "Error in function %s: %s"
                FWRMSG( stderr, HHC00075, "E", "ftell", strerror( errno ));
                arg_errs++;
            }
            else
            {
                if (fseek( inf, 0, SEEK_SET ) < 0)
                {
                    // "Error in function %s: %s"
                    FWRMSG( stderr, HHC00075, "E", "fseek", strerror( errno ));
                    arg_errs++;
                }
                else
                    filesize = (double) off_size;
            }
        }
    }
}

POP_GCC_WARNINGS()

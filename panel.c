/* PANEL.C      (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright TurboHercules, SAS 2010-2011           */
/*              Hercules Control Panel Commands                      */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*              Modified for New Panel Display =NP=                  */
/*-------------------------------------------------------------------*/
/* This module is the control panel for the ESA/390 emulator.        */
/* It provides a command interface into hercules, and it displays    */
/* messages that are issued by various hercules components.          */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*      breakpoint command contributed by Dan Horak                  */
/*      devinit command contributed by Jay Maynard                   */
/*      New Panel Display contributed by Dutch Owen                  */
/*      HMC system console commands contributed by Jan Jaeger        */
/*      Set/reset bad frame indicator command by Jan Jaeger          */
/*      attach/detach/define commands by Jan Jaeger                  */
/*      Panel refresh rate triva by Reed H. Petty                    */
/*      64-bit address support by Roger Bowler                       */
/*      Display subchannel command by Nobumichi Kozawa               */
/*      External GUI logic contributed by "Fish" (David B. Trout)    */
/*      Socket Devices originally designed by Malcolm Beattie;       */
/*      actual implementation by "Fish" (David B. Trout).            */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _PANEL_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "devtype.h"
#include "opcode.h"
#include "history.h"
#include "fillfnam.h"
#include "hconsole.h"

#define PANEL_MAX_ROWS  (256)
#define PANEL_MAX_COLS  (256)

int     redraw_msgs;                    /* 1=Redraw message area     */
int     redraw_cmd;                     /* 1=Redraw command line     */
int     redraw_status;                  /* 1=Redraw status line      */

/*=NP================================================================*/
/* Global data for new panel display                                 */
/*   (Note: all NPD mods are identified by the string =NP=           */
/*===================================================================*/

static int    NPDup = 0;               /* 1 = new panel is up       */
static int    NPDinit = 0;             /* 1 = new panel initialized */
static int    NPhelpup = 0;            /* 1 = help panel showing    */
static int    NPhelppaint = 1;         /* 1 = help pnl s/b painted  */
static int    NPhelpdown = 0;          /* 1 = help pnl coming down  */
static int    NPregdisp = 0;           /* which regs are displayed: */
                                       /* 0=gpr, 1=cr, 2=ar, 3=fpr  */
static int    NPcmd = 0;               /* 1 = NP in command mode    */
static int    NPdataentry = 0;         /* 1 = NP in data-entry mode */
static int    NPdevsel = 0;            /* 1 = device being selected */
static char   NPpending;               /* pending data entry cmd    */
static char   NPentered[256];          /* Data which was entered    */
static char   NPprompt1[40];           /* Left bottom screen prompt */
static char   NPoldprompt1[40];        /* Left bottom screen prompt */
static char   NPprompt2[40];           /* Right bottom screen prompt*/
static char   NPoldprompt2[40];        /* Right bottom screen prompt*/
static char   NPsel2;                  /* dev sel part 2 cmd letter */
static char   NPdevice;                /* Which device is selected  */
static int    NPasgn;                  /* Index to dev being init'ed*/
static int    NPlastdev;               /* Number of devices         */
static int    NPcpugraph_ncpu;         /* Number of CPUs to display */

static char  *NPregnum[]   = {" 0"," 1"," 2"," 3"," 4"," 5"," 6"," 7",
                              " 8"," 9","10","11","12","13","14","15"
                             };
static char  *NPregnum64[] = {"0", "1", "2", "3", "4", "5", "6", "7",
                              "8", "9", "A", "B", "C", "D", "E", "F"
                             };

/* Boolean fields; redraw the corresponding data field if false */
static int    NPcpunum_valid,
              NPcpupct_valid,
              NPpsw_valid,
              NPpswstate_valid,
              NPregs_valid,
              NPaddr_valid,
              NPdata_valid,
              NPmips_valid,
              NPsios_valid,
              NPdevices_valid,
              NPcpugraph_valid;

/* Current CPU states */
//static U16    NPcpunum;
//static int    NPcpupct;
static int    NPpswmode;
static int    NPpswzhost;
static QWORD  NPpsw;
static char   NPpswstate[16];
static int    NPregmode;
static int    NPregzhost;
static U64    NPregs64[16];
static U32    NPregs[16];
static U32    NPaddress;
static U32    NPdata;
static U32    NPmips;
static U32    NPsios;
static int    NPcpugraph;
static int    NPcpugraphpct[ MAX_CPU_ENGS ];

/* Current device states */
#define       NP_MAX_DEVICES (PANEL_MAX_ROWS - 3)
static int    NPonline[NP_MAX_DEVICES];
static U16    NPdevnum[NP_MAX_DEVICES];
static int    NPbusy[NP_MAX_DEVICES];
static U16    NPdevtype[NP_MAX_DEVICES];
static int    NPopen[NP_MAX_DEVICES];
static char   NPdevnam[NP_MAX_DEVICES][128];

static short  NPcurrow, NPcurcol;
static int    NPcolorSwitch;
static short  NPcolorFore;
static short  NPcolorBack;
static int    NPdatalen;

static char   *NPhelp[] = {
/*                   1         2         3         4         5         6         7         8 */
/* Line     ....+....0....+....0....+....0....+....0....+....0....+....0....+....0....+....0 */
/*    1 */ "All commands consist of one character keypresses. The various commands are",
/*    2 */ "highlighted onscreen by bright white versus the gray of other lettering.",
/*    3 */ "Disabled buttons, commands and areas are not shown when operating without",
/*    4 */ "defined CPUs (device server only mode).",
/*    5 */ " ",
/*    6 */ "Press the escape key to terminate the control panel and go to command mode.",
/*    7 */ " ",
/*    8 */ "Display Controls:   G - General purpose regs    C - Control regs",
/*    9 */ "                    A - Access registers        F - Floating Point regs",
/*   10 */ "                    I - Display main memory at ADDRESS",
/*   11 */ "CPU controls:       L - IPL                     S - Start CPU",
/*   12 */ "                    E - External interrupt      P - Stop CPU",
/*   13 */ "                    W - Exit Hercules           T - Restart interrupt",
/*   14 */ "Storage update:     R - Enter ADDRESS to be updated",
/*   15 */ "                    D - Enter DATA to be updated at ADDRESS",
/*   16 */ "                    O - place DATA value at ADDRESS",
/*   17 */ " ",
/*   18 */ "Peripherals:        N - enter a new name for the device file assignment",
/*   19 */ "                    U - send an I/O attention interrupt",
/*   20 */ " ",
/*   21 */ "In the display of the first 26 devices, a green device letter means the device",
/*   22 */ "is online, a highlighted device address means the device is busy, and a green",
/*   23 */ "model number means the attached file is open to the device.",
/*   24 */ " ",
/*   25 */ "               Press Escape to return to control panel operations",
"" };

///////////////////////////////////////////////////////////////////////

#define MSG_SIZE     PANEL_MAX_COLS     /* Size of one message       */
#define MAX_MSGS     2048                /* Number of slots in buffer */
//#define MAX_MSGS     300                /* (for testing scrolling)   */
#define MSG_LINES    (cons_rows - 2)    /* #lines in message area    */
#define SCROLL_LINES (MSG_LINES - numkept) /* #of scrollable lines   */
#define CMD_SIZE     256                /* cmdline buffer size       */

#define DEV_LINE        3               /* Line to start devices   */
#define PSW_LINE        2               /* Line to place PSW       */
#define REGS_LINE       5               /* Line to place REGS      */
#define ADDR_LINE       15              /* Line to place Addr/data */
#define BUTTONS_LINE    17              /* Line to place Buttons   */
#define CPU_GRAPH_LINE 20               /* Line to start CPU Graph */

///////////////////////////////////////////////////////////////////////

static int   cons_rows = 0;             /* console height in lines   */
static int   cons_cols = 0;             /* console width in chars    */
static short cur_cons_row = 0;          /* current console row       */
static short cur_cons_col = 0;          /* current console column    */
static const char *cons_term = NULL;    /* TERM env value            */
static char  cmdins  = 1;               /* 1==insert mode, 0==overlay*/

static char  cmdline[CMD_SIZE+1];       /* Command line buffer       */
static int   cmdlen  = 0;               /* cmdline data len in bytes */
static int   cmdoff  = 0;               /* cmdline buffer cursor pos */

static int   cursor_on_cmdline = 1;     /* bool: cursor on cmdline   */
static char  saved_cmdline[CMD_SIZE+1]; /* Saved command             */
static int   saved_cmdlen   = 0;        /* Saved cmdline data len    */
static int   saved_cmdoff   = 0;        /* Saved cmdline buffer pos  */
static short saved_cons_row = 0;        /* Saved console row         */
static short saved_cons_col = 0;        /* Saved console column      */

static int   cmdcols = 0;               /* visible cmdline width cols*/
static int   cmdcol  = 0;               /* cols cmdline scrolled righ*/
static FILE *confp   = NULL;            /* Console file pointer      */

///////////////////////////////////////////////////////////////////////

#define CMD_PREFIX_HERC     "herc =====> "
#define CMD_PREFIX_LEN      (strlen(CMD_PREFIX_HERC))
#define CMDLINE_ROW         ((short)(cons_rows-1))
#define CMDLINE_COL         ((short)(CMD_PREFIX_LEN+1))

///////////////////////////////////////////////////////////////////////

#define ADJ_SCREEN_SIZE() \
    do { \
        int rows, cols; \
        get_dim (&rows, &cols); \
        if (rows != cons_rows || cols != cons_cols) { \
            cons_rows = rows; \
            cons_cols = cols; \
            cmdcols = cons_cols - CMDLINE_COL; \
            redraw_msgs = redraw_cmd = redraw_status = 1; \
            NPDinit = 0; \
            clr_screen(); \
        } \
    } while (0)

#define ADJ_CMDCOL() /* (called after modifying cmdoff) */ \
    do { \
        if (cmdoff-cmdcol > cmdcols) { /* past right edge of screen */ \
            cmdcol = cmdoff-cmdcols; \
        } else if (cmdoff < cmdcol) { /* past left edge of screen */ \
            cmdcol = cmdoff; \
        } \
    } while (0)

#define PUTC_CMDLINE() \
    do { \
        ASSERT(cmdcol <= cmdlen); \
        for (i=0; cmdcol+i < cmdlen && i < cmdcols; i++) \
            draw_char (cmdline[cmdcol+i]); \
    } while (0)

///////////////////////////////////////////////////////////////////////

typedef struct _PANMSG      /* Panel message control block structure */
{
    struct _PANMSG*     next;           /* --> next entry in chain   */
    struct _PANMSG*     prev;           /* --> prev entry in chain   */
    int                 msgnum;         /* msgbuf 0-relative entry#  */
    char                msg[MSG_SIZE];  /* text of panel message     */
}
PANMSG;                     /* Panel message control block structure */

static PANMSG*  msgbuf = NULL;          /* Circular message buffer   */
static PANMSG*  topmsg = NULL;          /* message at top of screen  */
static PANMSG*  curmsg = NULL;          /* newest message            */
static int      wrapped = 0;            /* wrapped-around flag       */
static int      numkept = 0;            /* count of kept messages    */
static int      npquiet = 0;            /* screen updating flag      */

///////////////////////////////////////////////////////////////////////

static char *lmsbuf = NULL;             /* xxx                       */
static int   lmsndx = 0;                /* xxx                       */
static int   lmsnum = -1;               /* xxx                       */
static int   lmscnt = -1;               /* xxx                       */
static int   lmsmax = LOG_DEFSIZE/2;    /* xxx                       */
static int   keybfd = -1;               /* Keyboard file descriptor  */

static REGS  copyregs, copysieregs;     /* Copied regs               */

/*-------------------------------------------------------------------*/
/* Screen manipulation primitives                                    */
/*-------------------------------------------------------------------*/

static void beep()
{
    console_beep( confp );
}

static PANMSG* oldest_msg()
{
    return (wrapped) ? curmsg->next : msgbuf;
}

static PANMSG* newest_msg()
{
    return curmsg;
}

static int lines_scrolled()
{
    /* return # of lines 'up' from current line that we're scrolled. */
    if (topmsg->msgnum <= curmsg->msgnum)
        return curmsg->msgnum - topmsg->msgnum;
    return MAX_MSGS - (topmsg->msgnum - curmsg->msgnum);
}

static int visible_lines()
{
    return (lines_scrolled() + 1);
}

static int is_currline_visible()
{
    return (visible_lines() <= SCROLL_LINES);
}

static int lines_remaining()
{
    return (SCROLL_LINES - visible_lines());
}

static void scroll_up_lines( int numlines )
{
    int i;

    for (i=0; i < numlines && topmsg != oldest_msg(); i++)
        topmsg = topmsg->prev;
}

static void scroll_down_lines( int numlines )
{
    int i;

    for (i=0; i < numlines && topmsg != newest_msg(); i++)
    {
        if (topmsg != newest_msg())
            topmsg = topmsg->next;
    }
}

static void page_up()
{
    scroll_up_lines( SCROLL_LINES - 1 );
}
static void page_down()
{
    scroll_down_lines( SCROLL_LINES - 1 );
}

static void scroll_to_top_line()
{
    topmsg = oldest_msg();
}

static void scroll_to_bottom_line()
{
    while (topmsg != newest_msg())
        scroll_down_lines( 1 );
}

static void scroll_to_bottom_screen()
{
    scroll_to_bottom_line();
    page_up();
}

static void do_panel_command( void* cmd )
{
    if (!is_currline_visible())
        scroll_to_bottom_screen();

    if (cmd != (void*) cmdline)
        STRLCPY( cmdline, cmd );

    panel_command( cmdline );

    // Reset global variables

    cmdline[0] = 0;
    cmdlen     = 0;
    cmdoff     = 0;

    ADJ_CMDCOL();
}

static void do_prev_history()
{
    if (history_prev() != -1)
    {
        STRLCPY( cmdline, historyCmdLine );
        cmdlen = (int)strlen(cmdline);
        cmdoff = cmdlen < cmdcols ? cmdlen : 0;
        ADJ_CMDCOL();
    }
}

static void do_next_history()
{
    if (history_next() != -1)
    {
        STRLCPY( cmdline, historyCmdLine );
        cmdlen = (int)strlen(cmdline);
        cmdoff = cmdlen < cmdcols ? cmdlen : 0;
        ADJ_CMDCOL();
    }
}

static void clr_screen ()
{
    clear_screen (confp);
}

static void get_dim (int *y, int *x)
{
    get_console_dim( confp, y, x);
    if (*y > PANEL_MAX_ROWS)
        *y = PANEL_MAX_ROWS;
    if (*x > PANEL_MAX_COLS)
        *x = PANEL_MAX_COLS;
#if defined(WIN32) && !defined( _MSVC_ )
   /* If running from a cygwin command prompt we do
     * better with one less row.
     */
    if (!cons_term || !*cons_term || strcmp(cons_term, "xterm"))
        (*y)--;
#endif // defined(WIN32) && !defined( _MSVC_ )
}

static void set_color (short fg, short bg)
{
    set_screen_color (confp, fg, bg);
}

static void set_pos (short y, short x)
{
    cur_cons_row = y;
    cur_cons_col = x;
    y = y < 1 ? 1 : y > cons_rows ? cons_rows : y;
    x = x < 1 ? 1 : x > cons_cols ? cons_cols : x;
    set_screen_pos (confp, y, x);
}

static int is_cursor_on_cmdline()
{
#if defined(OPTION_EXTCURS)
    get_cursor_pos( keybfd, confp, &cur_cons_row, &cur_cons_col );
    cursor_on_cmdline =
    (1
        && cur_cons_row == CMDLINE_ROW
        && cur_cons_col >= CMDLINE_COL
        && cur_cons_col <= CMDLINE_COL + cmdcols
    );
#else // !defined(OPTION_EXTCURS)
    cursor_on_cmdline = 1;
#endif // defined(OPTION_EXTCURS)
    return cursor_on_cmdline;
}

static void cursor_cmdline_home()
{
    cmdoff = 0;
    ADJ_CMDCOL();
    set_pos( CMDLINE_ROW, CMDLINE_COL );
}

static void cursor_cmdline_end()
{
    cmdoff = cmdlen;
    ADJ_CMDCOL();
    set_pos( CMDLINE_ROW, CMDLINE_COL + cmdoff - cmdcol );
}

static void save_command_line()
{
    memcpy( saved_cmdline, cmdline, sizeof(saved_cmdline) );
    saved_cmdlen = cmdlen;
    saved_cmdoff = cmdoff;
    saved_cons_row = cur_cons_row;
    saved_cons_col = cur_cons_col;
}

static void restore_command_line()
{
    memcpy( cmdline, saved_cmdline, sizeof(cmdline) );
    cmdlen = saved_cmdlen;
    cmdoff = saved_cmdoff;
    cur_cons_row = saved_cons_row;
    cur_cons_col = saved_cons_col;
}

static void draw_text( const char* text )
{
    int   len;
    char *short_text;

    if (cur_cons_row < 1 || cur_cons_row > cons_rows
     || cur_cons_col < 1 || cur_cons_col > cons_cols)
        return;
    len = (int)strlen(text);
    if ((cur_cons_col + len - 1) <= cons_cols)
        fprintf (confp, "%s", text);
    else
    {
        len = cons_cols - cur_cons_col + 1;
        if ((short_text = strdup(text)) == NULL)
            return;
        short_text[len] = '\0';
        fprintf (confp, "%s", short_text);
        free (short_text);
    }
    cur_cons_col += len;
}

static void write_text (char *text, int size)
{
    if (cur_cons_row < 1 || cur_cons_row > cons_rows
     || cur_cons_col < 1 || cur_cons_col > cons_cols)
        return;
    if (cons_cols - cur_cons_col + 1 < size)
        size = cons_cols - cur_cons_col + 1;
    fwrite (text, size, 1, confp);
    cur_cons_col += size;
}

static void draw_char (int c)
{
    if (cur_cons_row < 1 || cur_cons_row > cons_rows
     || cur_cons_col < 1 || cur_cons_col > cons_cols)
        return;
    fputc (c, confp);
    cur_cons_col++;
}

static void draw_fw (U32 fw)
{
    char buf[9];
    MSGBUF (buf, "%8.8X", fw);
    draw_text (buf);
}

static void draw_dw (U64 dw)
{
    char buf[17];
    MSGBUF (buf, "%16.16"PRIX64, dw);
    draw_text (buf);
}

static void fill_text (char c, short x)
{
    char buf[PANEL_MAX_COLS+1];
    int  len;

    if (x > PANEL_MAX_COLS) x = PANEL_MAX_COLS;
    len = x + 1 - cur_cons_col;
    if (len <= 0) return;
    memset( buf, c, len );
    buf[len] = '\0';
    draw_text (buf);
}

static void draw_button (short bg, short fg, short hfg, char *left, char *mid, char *right)
{
    set_color (fg, bg);
    draw_text (left);
    set_color (hfg, bg);
    draw_text (mid);
    set_color (fg, bg);
    draw_text (right);
}

void set_console_title ( char *status )
{
    char    title[256];

    if ( sysblk.daemon_mode ) return;

    redraw_status = 1;

    if ( !sysblk.pantitle && ( !status || strlen(status) == 0 ) ) return;

    if ( !sysblk.pantitle )
    {
        char msgbuf[256];
        char sysname[16] = { 0 };
        char sysplex[16] = { 0 };
        char systype[16] = { 0 };
        char lparnam[16] = { 0 };

        memset( msgbuf, 0, sizeof(msgbuf) );

        STRLCAT( systype, str_systype()  );
        STRLCAT( sysname, str_sysname()  );
        STRLCAT( sysplex, str_sysplex()  );
        STRLCAT( lparnam, str_lparname() );

        if ( strlen(lparnam)+strlen(systype)+strlen(sysname)+strlen(sysplex) > 0 )
        {
            if ( strlen(lparnam) > 0 )
            {
                STRLCAT( msgbuf, lparnam );
                if ( strlen(systype)+strlen(sysname)+strlen(sysplex) > 0 )
                    STRLCAT( msgbuf, " - " );
            }
            if ( strlen(systype) > 0 )
            {
                STRLCAT( msgbuf, systype );
                if ( strlen(sysname)+strlen(sysplex) > 0 )
                    STRLCAT( msgbuf, " * " );
            }
            if ( strlen(sysname) > 0 )
            {
                STRLCAT( msgbuf, sysname );
                if ( strlen(sysplex) > 0 )
                    STRLCAT( msgbuf, " * " );
            }
            if ( strlen(sysplex) > 0 )
            {
                STRLCAT( msgbuf, sysplex );
            }

            MSGBUF( title, "%s - System Status: %s", msgbuf, status );
        }
        else
        {
            MSGBUF( title, "System Status: %s", status );
        }
    }
    else
    {
        if ( !status || strlen(status) == 0 )
            MSGBUF( title, "%s", sysblk.pantitle );
        else
            MSGBUF( title, "%s - System Status: %s", sysblk.pantitle, status );
    }

  #if defined( _MSVC_ )
    w32_set_console_title( title );
  #else /*!defined(_MSVC_) */
    /* For Unix systems we set the window title by sending a special
       escape sequence (depends on terminal type) to the console.
       See http://www.faqs.org/docs/Linux-mini/Xterm-Title.html */
    if (!cons_term || !*cons_term) return;
    if (strcmp(cons_term,"xterm")==0
     || strcmp(cons_term,"rxvt")==0
     || strcmp(cons_term,"dtterm")==0
     || strcmp(cons_term,"screen")==0)
    {
        fprintf( confp, "%c]0;%s%c", '\033', title, '\007' );
    }
  #endif /*!defined(_MSVC_) */
}

/*=NP================================================================*/
/*  Initialize the NP data                                           */
/*===================================================================*/

static void NP_init()
{
    NPdataentry = 0;
    STRLCPY( NPprompt1, "" );
    STRLCPY( NPprompt2, "" );
}

/*=NP================================================================*/
/*  This draws the initial screen template                           */
/*===================================================================*/

static void NP_screen_redraw (REGS *regs)
{
    int  i, line;
    char buf[1024];

    /* Force all data to be redrawn */
    NPcpunum_valid   = NPcpupct_valid   = NPpsw_valid  =
    NPpswstate_valid = NPregs_valid     = NPaddr_valid =
    NPdata_valid     = NPdevices_valid  = NPcpugraph_valid =
    NPmips_valid     = NPsios_valid     = 0;

#if defined(_FEATURE_SIE)
    if(regs->sie_active)
        regs = GUESTREGS;
#endif /*defined(_FEATURE_SIE)*/

    /*
     * Draw the static parts of the NP screen
     */
    set_color (COLOR_LIGHT_GREY, COLOR_BLACK );
    clr_screen ();

    /* Line 1 - title line */
    set_color (COLOR_WHITE, COLOR_BLUE );
    set_pos   (1, 1);
    draw_text ("  Hercules");
    if (sysblk.hicpu)
    {
        fill_text (' ', 16);
        draw_text ("CPU:    %");
        fill_text (' ', 30);
        draw_text (get_arch_name( NULL ));
    }

    set_color (COLOR_LIGHT_GREY, COLOR_BLUE);
    fill_text (' ', 38);
    draw_text ("| ");
    set_color (COLOR_WHITE, COLOR_BLUE);

#if defined( OPTION_SHARED_DEVICES )

    /* Center "Peripherals" on the right-hand-side */
    i = 40 + MSGBUF(buf,
                      "Peripherals [Shared Port %u]",
                      sysblk.shrdport);
    if ((cons_cols < i) || !sysblk.shrdport)
        i = 52, buf[11] = 0;            /* Truncate string */
    if (cons_cols > i)                  /* Center string   */
        fill_text (' ', 40 + ((cons_cols - i) / 2));
    draw_text (buf);
    fill_text (' ', (short)cons_cols);

#endif

    /* Line 2 - peripheral headings */
    set_pos (2, 41);
    set_color (COLOR_WHITE, COLOR_BLACK);
    draw_char ('U');
    set_color (COLOR_LIGHT_GREY, COLOR_BLACK);
    draw_text(" Addr Modl Type Assig");
    set_color (COLOR_WHITE, COLOR_BLACK);
    draw_char ('n');
    set_color (COLOR_LIGHT_GREY, COLOR_BLACK);
    draw_text("ment");

    if (sysblk.hicpu)
    {
        /* PSW_LINE = PSW */
        NPpswmode = (regs->arch_mode == ARCH_900_IDX);
        NPpswzhost =
#if defined(_FEATURE_SIE)
                     !NPpswmode && SIE_MODE(regs) && HOSTREGS->arch_mode == ARCH_900_IDX;
#else
                     0;
#endif /*defined(_FEATURE_SIE)*/
        set_pos (PSW_LINE+1, NPpswmode || NPpswzhost ? 19 : 10);
        draw_text ("PSW");

        /* Register area */
        set_color (COLOR_LIGHT_GREY, COLOR_BLACK);
        NPregmode = (regs->arch_mode == ARCH_900_IDX && (NPregdisp == 0 || NPregdisp == 1));
        NPregzhost =
#if defined(_FEATURE_SIE)
                     (regs->arch_mode != ARCH_900_IDX
                   && SIE_MODE(regs) && HOSTREGS->arch_mode == ARCH_900_IDX
                   && (NPregdisp == 0 || NPregdisp == 1));
#else
                     0;
#endif /*defined(_FEATURE_SIE)*/
        if (NPregmode == 1 || NPregzhost)
        {
            for (i = 0; i < 8; i++)
            {
                set_pos (REGS_LINE+i, 1);
                draw_text (NPregnum64[i*2]);
                set_pos (REGS_LINE+i, 20);
                draw_text (NPregnum64[i*2+1]);
            }
        }
        else
        {
            for (i = 0; i < 4; i++)
            {
                set_pos (i*2+(REGS_LINE+1),9);
                draw_text (NPregnum[i*4]);
                set_pos (i*2+(REGS_LINE+1),18);
                draw_text (NPregnum[i*4+1]);
                set_pos (i*2+(REGS_LINE+1),27);
                draw_text (NPregnum[i*4+2]);
                set_pos (i*2+(REGS_LINE+1),36);
                draw_text (NPregnum[i*4+3]);
            }
        }

        /* Register selection */
        set_color (COLOR_LIGHT_GREY, COLOR_BLACK);
        set_pos ((REGS_LINE+8), 6);
        draw_text ("GPR");
        set_pos ((REGS_LINE+8), 14);
        draw_text ("CR");
        set_pos ((REGS_LINE+8), 22);
        draw_text ("AR");
        set_pos ((REGS_LINE+8), 30);
        draw_text ("FPR");

        /* Address and data */
        set_pos (ADDR_LINE, 2);
        draw_text ("ADD");
        set_color (COLOR_WHITE, COLOR_BLACK);
        draw_char ('R');
        set_color (COLOR_LIGHT_GREY, COLOR_BLACK);
        draw_text ("ESS:");
        set_pos (ADDR_LINE, 22);
        set_color (COLOR_WHITE, COLOR_BLACK);
        draw_char ('D');
        set_color (COLOR_LIGHT_GREY, COLOR_BLACK);
        draw_text ("ATA:");
    }
    else
    {
        set_pos (8, 12);
        set_color (COLOR_LIGHT_RED, COLOR_BLACK);
        draw_text ("No CPUs defined");
        set_color (COLOR_LIGHT_GREY, COLOR_BLACK);
    }

    /* separator */
    set_pos (ADDR_LINE+1, 1);
    fill_text ('-', 38);

    /* Buttons */

    if (sysblk.hicpu)
    {
        set_pos (BUTTONS_LINE, 16);
        draw_button(COLOR_BLUE,  COLOR_LIGHT_GREY, COLOR_WHITE,  " ST", "O", " "  );
        set_pos (BUTTONS_LINE, 24);
        draw_button(COLOR_BLUE,  COLOR_LIGHT_GREY, COLOR_WHITE,  " D",  "I", "S " );
        set_pos (BUTTONS_LINE, 32);
        draw_button(COLOR_BLUE,  COLOR_LIGHT_GREY, COLOR_WHITE,  " RS", "T", " "  );
    }

    if (sysblk.hicpu)
    {
        set_pos ((BUTTONS_LINE+1), 3);
        set_color (COLOR_LIGHT_GREY, COLOR_BLACK);
        draw_text ("MIPS");
    }

    if (0
        || sysblk.hicpu

#if defined( OPTION_SHARED_DEVICES )
        || sysblk.shrdport
#endif
    )
    {
        set_pos ((BUTTONS_LINE+1), 10);
        draw_text ("IO/s");
    }

    if (sysblk.hicpu)
    {
        set_pos ((BUTTONS_LINE+2), 2);
        draw_button(COLOR_GREEN, COLOR_LIGHT_GREY, COLOR_WHITE,  " ",   "S", "TR ");
        set_pos ((BUTTONS_LINE+2), 9);
        draw_button(COLOR_RED,   COLOR_LIGHT_GREY, COLOR_WHITE,  " ST", "P", " "  );
        set_pos ((BUTTONS_LINE+2), 16);
        draw_button(COLOR_BLUE,  COLOR_LIGHT_GREY, COLOR_WHITE,  " ",   "E", "XT ");
        set_pos ((BUTTONS_LINE+2), 24);
        draw_button(COLOR_BLUE,  COLOR_LIGHT_GREY, COLOR_WHITE,  " IP", "L", " "  );
    }

    set_pos ((BUTTONS_LINE+2), 32);
    draw_button(COLOR_RED,   COLOR_LIGHT_GREY, COLOR_WHITE,  " P",  "W", "R " );

    set_color (COLOR_LIGHT_GREY, COLOR_BLACK);

    /* CPU busy graph */
    line = CPU_GRAPH_LINE;                          // this is where the dashes start
    NPcpugraph_ncpu = MIN(cons_rows - line - 1, sysblk.hicpu);
    set_pos (line++, 1);
    fill_text ('-', 38);

    if (sysblk.hicpu)
    {
        NPcpugraph = 1;
        NPcpugraph_valid = 0;
        for (i = 0; i < NPcpugraph_ncpu; i++)
        {
            MSGBUF (buf, "%s%02X ", PTYPSTR(i), i);
            set_pos (line++, 1);
            draw_text (buf);
        }
    }
    else
        NPcpugraph = 0;

    /* Vertical separators */
    for (i = 2; i <= cons_rows; i++)
    {
        set_pos (i , 39);
        draw_char ('|');
    }

    /* Last line : horizontal separator */
    if (cons_rows >= 24)
    {
        set_pos (cons_rows, 1);
        fill_text ('-', 38);
        draw_char ('|');
        fill_text ('-', cons_cols);
    }

    /* positions the cursor */
    set_pos (cons_rows, cons_cols);
}

static char *format_int(uint64_t ic)
{
    static char     obfr[32];   /* Enough for displaying 2^64-1 */
    char            grps[7][4]; /* 7 groups of 3 digits */
    int             maxg=0;
    int             i;

    STRLCPY( grps[0], "0" );
    while(ic>0)
    {
        int grp;
        grp=ic%1000;
        ic/=1000;
        if(ic==0)
        {
            MSGBUF(grps[maxg],"%u",grp);
        }
        else
        {
            MSGBUF(grps[maxg],"%3.3u",grp);
        }
        maxg++;
    }
    if(maxg) maxg--;
    obfr[0]=0;
    for(i=maxg;i>=0;i--)
    {
        STRLCAT( obfr, grps[i] );
        if(i)
        {
            STRLCAT( obfr, "," );
        }
    }
    return obfr;
}

/*=NP================================================================*/
/*  This refreshes the screen with new data every cycle              */
/*===================================================================*/

static void NP_update(REGS *regs)
{
    int     i, n;
    int     mode, zhost;
    int     cpupct_total;
    QWORD   curr_psw;
    U32     addr, aaddr;
    DEVBLK *dev;
    int     online, busy, open;
    char   *devclass;
    char    devnam[MAX_PATH];
    char    buf[1024];

    if (NPhelpup == 1)
    {
        if (NPhelpdown == 1)
        {
             NP_init();
             NP_screen_redraw(regs);
             NPhelpup = 0;
             NPhelpdown = 0;
             NPhelppaint = 1;
        }
        else
        {
            if (NPhelppaint)
            {
                set_color (COLOR_LIGHT_GREY, COLOR_BLACK);
                clr_screen ();
                for (i = 0; *NPhelp[i]; i++)
                {
                    set_pos (i+1, 1);
                    draw_text (NPhelp[i]);
                }
                NPhelppaint = 0;
            }
            return;
        }
    }

#if defined(_FEATURE_SIE)
    if(SIE_MODE(regs))
        regs = HOSTREGS;
#endif /*defined(_FEATURE_SIE)*/

    /* percent CPU busy */
    if (sysblk.hicpu)
    {
        cpupct_total = 0;
        n = 0;
        for ( i = 0; i < sysblk.maxcpu; i++ )
            if ( IS_CPU_ONLINE(i) )
                if ( sysblk.regs[i]->cpustate == CPUSTATE_STARTED )
                {
                    n++;
                    cpupct_total += sysblk.regs[i]->cpupct;
                }
        set_color (COLOR_WHITE, COLOR_BLUE);
        set_pos (1, 22);
        MSGBUF(buf, "%3d", (n > 0 ? cpupct_total/n : 0));
        draw_text (buf);
    }

    if (sysblk.hicpu)
    {
#if defined(_FEATURE_SIE)
        if(regs->sie_active)
            regs = GUESTREGS;
#endif /*defined(_FEATURE_SIE)*/

        mode = (regs->arch_mode == ARCH_900_IDX);
        zhost =
#if defined(_FEATURE_SIE)
                !mode && SIE_MODE(regs) && HOSTREGS->arch_mode == ARCH_900_IDX;
#else // !defined(_FEATURE_SIE)
                0;
#endif // defined(_FEATURE_SIE)

        /* Redraw the psw template if the mode changed */
        if (NPpswmode != mode || NPpswzhost != zhost)
        {
            NPpswmode = mode;
            NPpswzhost = zhost;
            NPpsw_valid = NPpswstate_valid = 0;
            set_color (COLOR_LIGHT_GREY, COLOR_BLACK);
            set_pos (PSW_LINE, 1);
            fill_text (' ',38);
            set_pos (PSW_LINE+1, 1);
            fill_text (' ', 38);
            set_pos (PSW_LINE+1, NPpswmode || NPpswzhost ? 19 : 10);
            draw_text ("PSW");
        }

        /* Display the psw */
        memset( curr_psw, 0, sizeof( curr_psw ));
        copy_psw( regs, curr_psw );
        if (!NPpsw_valid || memcmp( NPpsw, curr_psw, sizeof( NPpsw )))
        {
            set_color (COLOR_LIGHT_YELLOW, COLOR_BLACK);
            set_pos (PSW_LINE, 3);
            if (mode)
            {
                draw_dw( fetch_dw( curr_psw ));
                set_pos( PSW_LINE, 22 );
                draw_dw( fetch_dw( curr_psw + 8 ));
            }
            else if (zhost)
            {
                draw_fw( fetch_fw( curr_psw ));
                draw_fw( fetch_fw( curr_psw + 4 )); /* *JJ */
                set_pos( PSW_LINE, 22 );
                draw_text( "----------------" ); /* *JJ */
            }
            else
            {
                draw_fw( fetch_fw( curr_psw ));
                set_pos( PSW_LINE, 12 );
                draw_fw( fetch_fw( curr_psw + 4 ));
            }
            NPpsw_valid = 1;
            memcpy( NPpsw, curr_psw, sizeof( NPpsw ));
        }

        /* Display psw state */
        MSGBUF (buf, "%2d%c%c%c%c%c%c%c%c",
                      regs->psw.amode64                  ? 64  :
                      regs->psw.amode                    ? 31  : 24,
                      regs->cpustate == CPUSTATE_STOPPED ? 'M' : '.',
                      sysblk.instbreak                   ? 'T' : '.',
                      WAITSTATE (&regs->psw)             ? 'W' : '.',
                      regs->loadstate                    ? 'L' : '.',
                      regs->checkstop                    ? 'C' : '.',
                      PROBSTATE(&regs->psw)              ? 'P' : '.',
                      SIE_MODE(regs)                     ? 'S' : '.',
                      mode                               ? 'Z' : '.');
        if (!NPpswstate_valid || strcmp(NPpswstate, buf))
        {
            set_color( COLOR_LIGHT_YELLOW, COLOR_BLACK );
            set_pos( mode || zhost ? (PSW_LINE+1) : PSW_LINE, 28 );
            draw_text( buf );
            NPpswstate_valid = 1;
            STRLCPY( NPpswstate, buf );
        }

        /* Redraw the register template if the regmode switched */
        mode = (regs->arch_mode == ARCH_900_IDX && (NPregdisp == 0 || NPregdisp == 1));
        zhost =
#if defined(_FEATURE_SIE)
                (regs->arch_mode != ARCH_900_IDX
              && SIE_MODE(regs) && HOSTREGS->arch_mode == ARCH_900_IDX
              && (NPregdisp == 0 || NPregdisp == 1));
#else // !defined(_FEATURE_SIE)
                     0;
#endif /*defined(_FEATURE_SIE)*/
        if (NPregmode != mode || NPregzhost != zhost)
        {
            NPregmode = mode;
            NPregzhost = zhost;
            NPregs_valid = 0;
            set_color (COLOR_LIGHT_GREY, COLOR_BLACK);
            if (NPregmode || NPregzhost)
            {
                /* 64 bit registers */
                for (i = 0; i < 8; i++)
                {
                    set_pos (REGS_LINE+i, 1);
                    fill_text (' ', 38);
                    set_pos (REGS_LINE+i, 1);
                    draw_text (NPregnum64[i*2]);
                    set_pos (REGS_LINE+i, 20);
                    draw_text (NPregnum64[i*2+1]);
                }
            }
            else
            {
                /* 32 bit registers */
                for (i = 0; i < 4; i++)
                {
                    set_pos (i*2+REGS_LINE,1);
                    fill_text (' ', 38);
                    set_pos (i*2+(REGS_LINE+1),1);
                    fill_text (' ', 38);
                    set_pos (i*2+(REGS_LINE+1),9);
                    draw_text (NPregnum[i*4]);
                    set_pos (i*2+(REGS_LINE+1),18);
                    draw_text (NPregnum[i*4+1]);
                    set_pos (i*2+(REGS_LINE+1),27);
                    draw_text (NPregnum[i*4+2]);
                    set_pos (i*2+(REGS_LINE+1),36);
                    draw_text (NPregnum[i*4+3]);
                }
            }
        }

        /* Display register values */
        set_color (COLOR_LIGHT_YELLOW, COLOR_BLACK );
        if (NPregmode)
        {
            /* 64 bit registers */
            for (i = 0; i < 16; i++)
            {
                switch (NPregdisp) {
                case 0:
                    if (!NPregs_valid || NPregs64[i] != regs->GR_G(i))
                    {
                        set_pos (REGS_LINE + i/2, 3 + (i%2)*19);
                        draw_dw (regs->GR_G(i));
                        NPregs64[i] = regs->GR_G(i);
                    }
                    break;
                case 1:
                    if (!NPregs_valid || NPregs64[i] != regs->CR_G(i))
                    {
                        set_pos (REGS_LINE + i/2, 3 + (i%2)*19);
                        draw_dw (regs->CR_G(i));
                        NPregs64[i] = regs->CR_G(i);
                    }
                    break;
                }
            }
        }
        else if (NPregzhost)
        {
            /* 32 bit registers on 64 bit template */
            for (i = 0; i < 16; i++)
            {
                switch (NPregdisp) {
                case 0:
                    if (!NPregs_valid || NPregs[i] != regs->GR_L(i))
                    {
                        set_pos (REGS_LINE + i/2, 3 + (i%2)*19);
//                      draw_fw (0);
                        draw_text("--------");
                        draw_fw (regs->GR_L(i));
                        NPregs[i] = regs->GR_L(i);
                    }
                    break;
                case 1:
                    if (!NPregs_valid || NPregs[i] != regs->CR_L(i))
                    {
                        set_pos (REGS_LINE + i/2, 3 + (i%2)*19);
//                      draw_fw (0);
                        draw_text("--------");
                        draw_fw (regs->CR_L(i));
                        NPregs[i] = regs->CR_L(i);
                    }
                    break;
                }
            }
        }
        else
        {
            /* 32 bit registers */
            addr = NPaddress;
            for (i = 0; i < 16; i++)
            {
                switch (NPregdisp) {
                default:
                case 0:
                    if (!NPregs_valid || NPregs[i] != regs->GR_L(i))
                    {
                        set_pos (REGS_LINE + (i/4)*2, 3 + (i%4)*9);
                        draw_fw (regs->GR_L(i));
                        NPregs[i] = regs->GR_L(i);
                    }
                    break;
                case 1:
                    if (!NPregs_valid || NPregs[i] != regs->CR_L(i))
                    {
                        set_pos (REGS_LINE + (i/4)*2, 3 + (i%4)*9);
                        draw_fw (regs->CR_L(i));
                        NPregs[i] = regs->CR_L(i);
                    }
                    break;
                case 2:
                    if (!NPregs_valid || NPregs[i] != regs->AR(i))
                    {
                        set_pos (REGS_LINE + (i/4)*2, 3 + (i%4)*9);
                        draw_fw (regs->AR(i));
                        NPregs[i] = regs->AR(i);
                    }
                    break;
                case 3:
                    if (!NPregs_valid || NPregs[i] != regs->fpr[i])
                    {
                        set_pos (REGS_LINE + (i/4)*2, 3 + (i%4)*9);
                        draw_fw (regs->fpr[i]);
                        NPregs[i] = regs->fpr[i];
                    }
                    break;
                case 4:
                    aaddr = APPLY_PREFIXING (addr, regs->PX);
                    addr += 4;
                    if (aaddr + 3 > regs->mainlim)
                        break;
                    if (!NPregs_valid || NPregs[i] != fetch_fw(regs->mainstor + aaddr))
                    {
                        set_pos (REGS_LINE + (i/4)*2, 3 + (i%4)*9);
                        draw_fw (fetch_fw(regs->mainstor + aaddr));
                        NPregs[i] = fetch_fw(regs->mainstor + aaddr);
                    }
                    break;
                }
            }
        }

        /* Update register selection indicator */
        if (!NPregs_valid)
        {
            set_pos ((REGS_LINE+8), 6);
            set_color (NPregdisp == 0 ? COLOR_LIGHT_YELLOW : COLOR_WHITE, COLOR_BLACK);
            draw_char ('G');

            set_pos ((REGS_LINE+8), 14);
            set_color (NPregdisp == 1 ? COLOR_LIGHT_YELLOW : COLOR_WHITE, COLOR_BLACK);
            draw_char ('C');

            set_pos ((REGS_LINE+8), 22);
            set_color (NPregdisp == 2 ? COLOR_LIGHT_YELLOW : COLOR_WHITE, COLOR_BLACK);
            draw_char ('A');

            set_pos ((REGS_LINE+8), 30);
            set_color (NPregdisp == 3 ? COLOR_LIGHT_YELLOW : COLOR_WHITE, COLOR_BLACK);
            draw_char ('F');
        }

        NPregs_valid = 1;

        /* Address & Data */
        if (!NPaddr_valid)
        {
            set_color (COLOR_LIGHT_YELLOW, COLOR_BLACK);
            set_pos (ADDR_LINE, 12);
            draw_fw (NPaddress);
            NPaddr_valid = 1;
        }
        if (!NPdata_valid)
        {
            set_color (COLOR_LIGHT_YELLOW, COLOR_BLACK);
            set_pos (ADDR_LINE, 30);
            draw_fw (NPdata);
            NPdata_valid = 1;
        }
    }

    /* Rates */
    if ((!NPmips_valid || sysblk.mipsrate != NPmips) && sysblk.hicpu)
    {
        set_color (COLOR_LIGHT_YELLOW, COLOR_BLACK);
        set_pos (BUTTONS_LINE, 1);
        if((sysblk.mipsrate / 1000000) > 999)
          MSGBUF(buf, "%2d,%03d", sysblk.mipsrate / 1000000000, sysblk.mipsrate % 1000000000 / 1000000);
        else if((sysblk.mipsrate / 1000000) > 99)
          MSGBUF(buf, "%4d.%01d", sysblk.mipsrate / 1000000, sysblk.mipsrate % 1000000 / 100000);
        else if((sysblk.mipsrate / 1000000) > 9)
          MSGBUF(buf, "%3d.%02d", sysblk.mipsrate / 1000000, sysblk.mipsrate % 1000000 / 10000);
        else
          MSGBUF(buf, "%2d.%03d", sysblk.mipsrate / 1000000, sysblk.mipsrate % 1000000 / 1000);
        draw_text (buf);
        NPmips = sysblk.mipsrate;
        NPmips_valid = 1;
    }

    if (0
        || (sysblk.hicpu && (!NPsios_valid || NPsios != sysblk.siosrate))

#if defined( OPTION_SHARED_DEVICES )
        || sysblk.shrdport
#endif
    )
    {
        set_color (COLOR_LIGHT_YELLOW, COLOR_BLACK);
        set_pos (BUTTONS_LINE, 8);
        MSGBUF(buf, "%6.6s", format_int(sysblk.siosrate));
        draw_text (buf);
        NPsios = sysblk.siosrate;
        NPsios_valid = 1;
    }

    /* Optional cpu graph */
    if (NPcpugraph)
    {
        for (i = 0; i < NPcpugraph_ncpu; i++)
        {
            if (!IS_CPU_ONLINE(i))
            {
                if (!NPcpugraph_valid || NPcpugraphpct[i] != -2.0)
                {
                    set_color (COLOR_RED, COLOR_BLACK);
                    set_pos (CPU_GRAPH_LINE+1+i, 6);
                    draw_text ("OFFLINE");
                    fill_text (' ', 38);
                    NPcpugraphpct[i] = -2.0;
                }
            }
            else if (sysblk.regs[i]->cpustate != CPUSTATE_STARTED)
            {
                if (!NPcpugraph_valid || NPcpugraphpct[i] != -1.0)
                {
                    set_color (COLOR_LIGHT_YELLOW, COLOR_BLACK);
                    set_pos (CPU_GRAPH_LINE+1+i, 6);
                    draw_text ("STOPPED");
                    fill_text (' ', 38);
                    NPcpugraphpct[i] = -1.0;
                }
            }
            else if (!NPcpugraph_valid || NPcpugraphpct[i] != sysblk.regs[i]->cpupct)
            {
                n = (34 * sysblk.regs[i]->cpupct) / 100;
                if (n == 0 && sysblk.regs[i]->cpupct > 0)
                    n = 1;
                else if (n > 34)
                    n = 34;
                set_color (n > 17 ? COLOR_WHITE : COLOR_LIGHT_GREY, COLOR_BLACK);
                set_pos (CPU_GRAPH_LINE+1+i, 6);
                fill_text ('*', n+3);
                fill_text (' ', 38);
                NPcpugraphpct[i] = sysblk.regs[i]->cpupct;
            }

            set_color (COLOR_LIGHT_GREY, COLOR_BLACK);
        }
        NPcpugraph_valid = 1;
    }

    /* Process devices */
    for (i = 0, dev = sysblk.firstdev; dev != NULL; i++, dev = dev->nextdev)
    {
        if (i >= cons_rows - 3) break;
        if (!dev->allocated) continue;

        online = (dev->console && dev->connected) || strlen(dev->filename) > 0;
        busy   = dev->busy != 0 || IOPENDING(dev) != 0;
        open   = dev->fd >= 0;

        /* device identifier */
        if (!NPdevices_valid || online != NPonline[i])
        {
            set_pos (DEV_LINE+i, 41);
            set_color (online ? COLOR_LIGHT_GREEN : COLOR_LIGHT_GREY, COLOR_BLACK);
            draw_char (i < 26 ? 'A' + i : '.');
            NPonline[i] = online;
        }

        /* device number */
        if (!NPdevices_valid || dev->devnum != NPdevnum[i] || NPbusy[i] != busy)
        {
            set_pos (DEV_LINE+i, 43);
            set_color (busy ? COLOR_LIGHT_YELLOW : COLOR_LIGHT_GREY, COLOR_BLACK);
            if (dev == sysblk.sysgdev)
                STRLCPY( buf, "SYSG" );
            else
                MSGBUF (buf, "%4.4X", dev->devnum);
            draw_text (buf);
            NPdevnum[i] = dev->devnum;
            NPbusy[i] = busy;
        }

        /* device type */
        if (!NPdevices_valid || dev->devtype != NPdevtype[i] || open != NPopen[i])
        {
            set_pos (DEV_LINE+i, 48);
            set_color (open ? COLOR_LIGHT_GREEN : COLOR_LIGHT_GREY, COLOR_BLACK);
            MSGBUF (buf, "%4.4X", dev->devtype);
            draw_text (buf);
            NPdevtype[i] = dev->devtype;
            NPopen[i] = open;
        }

        /* device class and name */
        (dev->hnd->query)(dev, &devclass, sizeof(devnam), devnam);
        if (!NPdevices_valid || strcmp(NPdevnam[i], devnam))
        {
            set_color (COLOR_LIGHT_GREY, COLOR_BLACK);
            set_pos (DEV_LINE+i, 53);
            MSGBUF (buf, "%-4.4s", devclass);
            draw_text (buf);
            /* Draw device name only if they're NOT assigning a new one */
            if (0
                || NPdataentry != 1
                || NPpending != 'n'
                || NPasgn != i
            )
            {
                /* locate first nonprintable and truncate */
                int l, p;

                l = (int)strlen(devnam);
                for ( p = 0; p < l; p++ )
                {
                    if ( !isprint((unsigned char)devnam[p]) )
                    {
                        devnam[p] = '\0';
                        break;
                    }
                }
                set_pos (DEV_LINE+i, 58);
                draw_text (devnam);
                fill_text (' ', PANEL_MAX_COLS);
            }
        }
    }

    /* Complete the device state table */
    if (!NPdevices_valid)
    {
        NPlastdev = i > 26 ? 26 : i - 1;
        for ( ; i < NP_MAX_DEVICES; i++)
        {
            NPonline[i] = NPdevnum[i] = NPbusy[i] = NPdevtype[i] = NPopen[i] = 0;
            STRLCPY( NPdevnam[i], "" );
        }
        NPdevices_valid = 1;
    }

    /* Prompt 1 */
    if (strcmp(NPprompt1, NPoldprompt1))
    {
        STRLCPY( NPoldprompt1, NPprompt1 );
        if (strlen(NPprompt1) > 0)
        {
            set_color (COLOR_WHITE, COLOR_BLUE);
            set_pos (cons_rows, (40 - (short)strlen(NPprompt1)) / 2);
            draw_text (NPprompt1);
        }
        else if (cons_rows >= 24)
        {
            set_color (COLOR_LIGHT_GREY, COLOR_BLACK);
            set_pos (cons_rows, 1);
            fill_text ('-', 38);
        }
    }

    /* Prompt 2 */
    if (strcmp(NPprompt2, NPoldprompt2))
    {
        STRLCPY( NPoldprompt2, NPprompt2 );
        if (strlen(NPprompt2) > 0)
        {
            set_color(COLOR_WHITE, COLOR_BLUE);
            set_pos(cons_rows, 41);
            draw_text(NPprompt2);
        }
        else if (cons_rows >= 24)
        {
            set_color( COLOR_LIGHT_GREY, COLOR_BLACK );
            set_pos( cons_rows, 41) ;
            fill_text( '-', cons_cols );
        }
    }

    /* Data entry field */
    if (NPdataentry && redraw_cmd)
    {
        set_pos (NPcurrow, NPcurcol);
        if (NPcolorSwitch)
            set_color (NPcolorFore, NPcolorBack);
        fill_text (' ', NPcurcol + NPdatalen - 1);
        set_pos (NPcurrow, NPcurcol);
        PUTC_CMDLINE();
        redraw_cmd = 0;
    }
    else
        /* Position the cursor to the bottom right */
        set_pos(cons_rows, cons_cols);
}

/* ==============   End of the main NP block of code    =============*/

static void panel_cleanup(void *unused);    // (forward reference)

///////////////////////////////////////////////////////////////////////
// "maxrates" command support...

#define  DEF_MAXRATES_RPT_INTVL   (  1440  )

DLL_EXPORT U32    curr_high_mips_rate = 0;  // (high water mark for current interval)
DLL_EXPORT U32    curr_high_sios_rate = 0;  // (high water mark for current interval)

DLL_EXPORT U32    prev_high_mips_rate = 0;  // (saved high water mark for previous interval)
DLL_EXPORT U32    prev_high_sios_rate = 0;  // (saved high water mark for previous interval)

DLL_EXPORT time_t curr_int_start_time = 0;  // (start time of current interval)
DLL_EXPORT time_t prev_int_start_time = 0;  // (start time of previous interval)

DLL_EXPORT U32    maxrates_rpt_intvl  = DEF_MAXRATES_RPT_INTVL;

DLL_EXPORT void update_maxrates_hwm()       // (update high-water-mark values)
{
    time_t  current_time = 0;
    U32     elapsed_secs = 0;

    if (curr_high_mips_rate < sysblk.mipsrate)
        curr_high_mips_rate = sysblk.mipsrate;

    if (curr_high_sios_rate < sysblk.siosrate)
        curr_high_sios_rate = sysblk.siosrate;

    // Save high water marks for current interval...

    time( &current_time );

    elapsed_secs = current_time - curr_int_start_time;

    if ( elapsed_secs >= ( maxrates_rpt_intvl * 60 ) )
    {
        prev_high_mips_rate = curr_high_mips_rate;
        prev_high_sios_rate = curr_high_sios_rate;

        curr_high_mips_rate = 0;
        curr_high_sios_rate = 0;

        prev_int_start_time = curr_int_start_time;
        curr_int_start_time = current_time;
    }
}

///////////////////////////////////////////////////////////////////////
// Get a working copy of the active REGS struct(s) for specified CPU

static REGS* panel_copy_regs( int cpu )
{
    REGS* regs; /* (pointer to REGS struct that we'll be returning) */

    /* Default to CPU #0 if invalid CPU number is passed */
    if (cpu < 0 || cpu >= sysblk.maxcpu)
        cpu = 0;

    /* Use the standardized REGS copy function to make the copy */
    obtain_lock( &sysblk.cpulock[ cpu ]);
    {
        if (!sysblk.regs[ cpu ])
        {
            /* Specified CPU does not exist. Use dummyregs instead */
            release_lock( &sysblk.cpulock[ cpu ]);
            return &sysblk.dummyregs;
        }

        /* Make a working copy of this CPU's REGS structs */
        regs = copy_regs( sysblk.regs[ cpu ] );
    }
    release_lock( &sysblk.cpulock[ cpu ]);

    /* Copy the working copy to our static work variables */
    memcpy( &copyregs, regs, sysblk.regs_copy_len );

    /* Free the original working copy */
    free_aligned( regs );

    /* Point to our static copy */
    regs = &copyregs;

    /* Make a separate copy of the guest REGS, if they exist */
    if (GUESTREGS)
        memcpy( &copysieregs, GUESTREGS, sysblk.regs_copy_len );

    /* Switch to using the guest REGS instead, if SIE is active */
#if defined( _FEATURE_SIE )
    if (regs->sie_active)
        regs = &copysieregs;
#endif

    /* Ensure PSW accurately reflects the current instruction */
    MAYBE_SET_PSW_IA_FROM_IP( regs );

    /* Return pointer to static copy of active CPU's REGS struct */
    return regs;
}

///////////////////////////////////////////////////////////////////////
// Set panel colors

DLL_EXPORT void set_panel_colors()
{
    switch (sysblk.pan_colors)
    {
    default:
    case PANC_NONE:   // No colors: use defaults

        sysblk.pan_color[ PANC_X_IDX ][ PANC_FG_IDX ] = COLOR_DEFAULT_FG;
        sysblk.pan_color[ PANC_I_IDX ][ PANC_FG_IDX ] = COLOR_DEFAULT_FG;
        sysblk.pan_color[ PANC_E_IDX ][ PANC_FG_IDX ] = COLOR_DEFAULT_FG;
        sysblk.pan_color[ PANC_W_IDX ][ PANC_FG_IDX ] = COLOR_DEFAULT_FG;
        sysblk.pan_color[ PANC_D_IDX ][ PANC_FG_IDX ] = COLOR_DEFAULT_FG;
        sysblk.pan_color[ PANC_S_IDX ][ PANC_FG_IDX ] = COLOR_DEFAULT_FG;
        sysblk.pan_color[ PANC_A_IDX ][ PANC_FG_IDX ] = COLOR_DEFAULT_FG;

        sysblk.pan_color[ PANC_X_IDX ][ PANC_BG_IDX ] = COLOR_DEFAULT_BG;
        sysblk.pan_color[ PANC_I_IDX ][ PANC_BG_IDX ] = COLOR_DEFAULT_BG;
        sysblk.pan_color[ PANC_E_IDX ][ PANC_BG_IDX ] = COLOR_DEFAULT_BG;
        sysblk.pan_color[ PANC_W_IDX ][ PANC_BG_IDX ] = COLOR_DEFAULT_BG;
        sysblk.pan_color[ PANC_D_IDX ][ PANC_BG_IDX ] = COLOR_DEFAULT_BG;
        sysblk.pan_color[ PANC_S_IDX ][ PANC_BG_IDX ] = COLOR_DEFAULT_BG;
        sysblk.pan_color[ PANC_A_IDX ][ PANC_BG_IDX ] = COLOR_DEFAULT_BG;

        break;

    case PANC_DARK:   // Dark scheme: light text on dark background

        sysblk.pan_color[ PANC_X_IDX ][ PANC_FG_IDX ] = COLOR_WHITE;
        sysblk.pan_color[ PANC_X_IDX ][ PANC_BG_IDX ] = COLOR_DEFAULT_BG;

        sysblk.pan_color[ PANC_I_IDX ][ PANC_FG_IDX ] = COLOR_LIGHT_GREY;
        sysblk.pan_color[ PANC_I_IDX ][ PANC_BG_IDX ] = COLOR_DEFAULT_BG;

        sysblk.pan_color[ PANC_E_IDX ][ PANC_FG_IDX ] = COLOR_WHITE;
        sysblk.pan_color[ PANC_E_IDX ][ PANC_BG_IDX ] = COLOR_LIGHT_RED;

        sysblk.pan_color[ PANC_W_IDX ][ PANC_FG_IDX ] = COLOR_LIGHT_GREY;
        sysblk.pan_color[ PANC_W_IDX ][ PANC_BG_IDX ] = COLOR_RED;

        sysblk.pan_color[ PANC_D_IDX ][ PANC_FG_IDX ] = COLOR_LIGHT_GREY;
        sysblk.pan_color[ PANC_D_IDX ][ PANC_BG_IDX ] = COLOR_BLUE;

        sysblk.pan_color[ PANC_S_IDX ][ PANC_FG_IDX ] = COLOR_WHITE;
        sysblk.pan_color[ PANC_S_IDX ][ PANC_BG_IDX ] = COLOR_LIGHT_RED;

        sysblk.pan_color[ PANC_A_IDX ][ PANC_FG_IDX ] = COLOR_WHITE;
        sysblk.pan_color[ PANC_A_IDX ][ PANC_BG_IDX ] = COLOR_DEFAULT_BG;

        break;

    case PANC_LIGHT:  // Light scheme: dark text on light background

        sysblk.pan_color[ PANC_X_IDX ][ PANC_FG_IDX ] = COLOR_DARK_GREY;
        sysblk.pan_color[ PANC_X_IDX ][ PANC_BG_IDX ] = COLOR_DEFAULT_BG;

        sysblk.pan_color[ PANC_I_IDX ][ PANC_FG_IDX ] = COLOR_BLACK;
        sysblk.pan_color[ PANC_I_IDX ][ PANC_BG_IDX ] = COLOR_DEFAULT_BG;

        sysblk.pan_color[ PANC_E_IDX ][ PANC_FG_IDX ] = COLOR_WHITE;
        sysblk.pan_color[ PANC_E_IDX ][ PANC_BG_IDX ] = COLOR_LIGHT_RED;

        sysblk.pan_color[ PANC_W_IDX ][ PANC_FG_IDX ] = COLOR_LIGHT_GREY;
        sysblk.pan_color[ PANC_W_IDX ][ PANC_BG_IDX ] = COLOR_RED;

        sysblk.pan_color[ PANC_D_IDX ][ PANC_FG_IDX ] = COLOR_LIGHT_GREY;
        sysblk.pan_color[ PANC_D_IDX ][ PANC_BG_IDX ] = COLOR_BLUE;

        sysblk.pan_color[ PANC_S_IDX ][ PANC_FG_IDX ] = COLOR_WHITE;
        sysblk.pan_color[ PANC_S_IDX ][ PANC_BG_IDX ] = COLOR_LIGHT_RED;

        sysblk.pan_color[ PANC_A_IDX ][ PANC_FG_IDX ] = COLOR_DARK_GREY;
        sysblk.pan_color[ PANC_A_IDX ][ PANC_BG_IDX ] = COLOR_DEFAULT_BG;

        break;
    }
}

///////////////////////////////////////////////////////////////////////
// Return panel message FG/BG color for given message severity code

static int msgcolor( int sev, int fgbg )
{
    switch (sev) {
    case 'I': return sysblk.pan_color[ PANC_I_IDX ][ fgbg ];
    case 'E': return sysblk.pan_color[ PANC_E_IDX ][ fgbg ];
    case 'W': return sysblk.pan_color[ PANC_W_IDX ][ fgbg ];
    case 'D': return sysblk.pan_color[ PANC_D_IDX ][ fgbg ];
    case 'S': return sysblk.pan_color[ PANC_S_IDX ][ fgbg ];
    case 'A': return sysblk.pan_color[ PANC_A_IDX ][ fgbg ]; default: break; }
              return sysblk.pan_color[ PANC_X_IDX ][ fgbg ];
}
static int fg_msgcolor( int sev ) { return msgcolor( sev, PANC_FG_IDX ); }
static int bg_msgcolor( int sev ) { return msgcolor( sev, PANC_BG_IDX ); }

static bool have_regexp = false;

#if defined(HAVE_REGEX_H) || defined(HAVE_PCRE)

// PROGRAMMING NOTE: using a variable named "regex" conflicts with
// <libgen.h> on Solaris, so we use variable name "regexp" instead.

static regex_t     regexp;      // (see above PROGRAMMING NOTE)
static regmatch_t  regmatch;

static void init_HHC_regexp()
{
    // "HHC99999S"
    have_regexp = (0 == regcomp( &regexp, "(HHC[0-9][0-9][0-9][0-9][0-9]\\S)", REG_EXTENDED ))
        ? true : false;
}
#endif // defined(HAVE_REGEX_H) || defined(HAVE_PCRE)

///////////////////////////////////////////////////////////////////////
// Return panel message severity code

static int msg_sev( const char* msg )
{
#if defined(HAVE_REGEX_H) || defined(HAVE_PCRE)
    if (have_regexp)
    {
        if (regexec( &regexp, msg, 1, &regmatch, 0 ) == 0)
            return (int)(msg[ regmatch.rm_so + 8 ]);
    }
    else
#endif // defined(HAVE_REGEX_H) || defined(HAVE_PCRE)
    {
        int sevidx = MLVL( DEBUG ) ? MLVL_DEBUG_PFXIDX + 8 : 8;
        if ((int)strlen( msg ) > sevidx)
            return (int)(msg[ sevidx ]);
    }
    return (int)' ';
}

/*-------------------------------------------------------------------*/
/*                    Panel display thread                           */
/*-------------------------------------------------------------------*/
/* This function runs on the main thread.  It receives messages      */
/* from the log task and displays them on the screen.  It accepts    */
/* panel commands from the keyboard and executes them.  It samples   */
/* the PSW periodically and displays it on the screen status line.   */
/*-------------------------------------------------------------------*/
DLL_EXPORT void the_real_panel_display()
{
#ifndef _MSVC_
  int     rc;                           /* Return code               */
  fd_set  readset;                      /* Select file descriptors   */
  struct  timeval tv;                   /* Select timeout structure  */
#endif // _MSVC_
int     i;                              /* Array subscripts          */
int     len;                            /* Length                    */
REGS   *regs;                           /* -> CPU register context   */
QWORD   curr_psw;                       /* Current PSW               */
QWORD   prev_psw;                       /* Previous PSW              */
int     prev_pcpu      = 0;             /* Previous pcpu             */
int     prev_arch_mode = 0;             /* Previous primary arch.    */
int     prev_cpupct    = 0;             /* Previous cpu percentage   */
BYTE    prev_cpustate  = 0xFF;          /* Previous stopped state    */
U64     prev_instcount = 0;             /* Previous instruction count*/
U32     prev_mipsrate  = 0;             /* Previous MIPS rate        */
U32     prev_siosrate  = 0;             /* Previous SIOS rate        */
#if defined( OPTION_SHARED_DEVICES )
U32     prev_shrdcount = 0;             /* Previous shrdcount        */
#endif
U32     numcpu = 0;                     /* Online CPU count          */
char    readbuf[MSG_SIZE];              /* Message read buffer       */
int     readoff = 0;                    /* Number of bytes in readbuf*/
BYTE    c;                              /* Character work area       */
size_t  kbbufsize = CMD_SIZE;           /* Size of keyboard buffer   */
char   *kbbuf = NULL;                   /* Keyboard input buffer     */
int     kblen;                          /* Number of chars in kbbuf  */
U32     aaddr;                          /* Absolute address for STO  */
char    buf[1024];                      /* Buffer workarea           */
size_t  loopcount;                      /* Number of iterations done */

    SET_THREAD_NAME( PANEL_THREAD_NAME );
    hdl_addshut( "panel_cleanup", panel_cleanup, NULL );
    history_init();

#if defined(HAVE_REGEX_H) || defined(HAVE_PCRE)
    init_HHC_regexp();
#endif

    /* Set Some Function Key Defaults */
    {
        set_symbol("PF01", "SUBST IMMED help &0");
        set_symbol("PF11", "IMMED devlist TAPE");
        set_symbol("PF10", "SUBST DELAY devinit &*");
    }

    /* Set up the input file descriptors */
    confp = stderr;
    keybfd = STDIN_FILENO;

    /* Initialize screen dimensions */
    cons_term = get_symbol( "TERM" ); // Note! result could be "" empty string!
    get_dim (&cons_rows, &cons_cols);

    /* Clear the command-line buffer */
    memset(cmdline, 0, sizeof(cmdline));
    cmdcols = cons_cols - CMDLINE_COL;

    /* Obtain storage for the keyboard buffer */
    if (!(kbbuf = malloc (kbbufsize)))
    {
        char buf[40];
        MSGBUF(buf, "malloc(%d)", (int)kbbufsize);
        WRMSG(HHC00075, "E", buf, strerror(errno));
        return;
    }

    /* Obtain storage for the circular message buffer */
    msgbuf = malloc (MAX_MSGS * sizeof(PANMSG));
    if (msgbuf == NULL)
    {
        char buf[40];
        MSGBUF(buf, "malloc(%d)", (int)(MAX_MSGS * (int)sizeof(PANMSG)));
        fprintf (stderr,
                MSG(HHC00075, "E", buf, strerror(errno)));
        return;
    }

    /* Initialize circular message buffer */
    for (curmsg = msgbuf, i=0; i < MAX_MSGS; curmsg++, i++)
    {
        curmsg->next = curmsg + 1;
        curmsg->prev = curmsg - 1;
        curmsg->msgnum = i;
        memset(curmsg->msg,SPACE,MSG_SIZE);
    }

    /* Complete the circle */
    msgbuf->prev = msgbuf + MAX_MSGS - 1;
    msgbuf->prev->next = msgbuf;

    /* Indicate "first-time" state */
    curmsg = topmsg = NULL;
    wrapped = 0;
    numkept = 0;

    /* Set screen output stream to NON-buffered */
    setvbuf (confp, NULL, _IONBF, 0);

    /* Put the terminal into cbreak mode */
    set_or_reset_console_mode( keybfd, 1 );

    /* Set console title */
    set_console_title(NULL);

    /* Clear the screen */
    set_color (COLOR_DEFAULT_FG, COLOR_DEFAULT_BG);
    clr_screen ();
    redraw_msgs = redraw_cmd = redraw_status = 1;

    /* Notify logger_thread we're in control */
    sysblk.panel_init = 1;

    /* Initialize "maxrates" command reporting intervals */
    if ( maxrates_rpt_intvl == 1440 )
    {
        time_t      current_time;
        struct tm  *current_tm;
        time_t      since_midnight = 0;
        current_time = time( NULL );
        current_tm   = localtime( &current_time );
        since_midnight = (time_t)( ( ( current_tm->tm_hour  * 60 ) +
                                       current_tm->tm_min ) * 60   +
                                       current_tm->tm_sec );
        curr_int_start_time = current_time - since_midnight;
    }
    else
        curr_int_start_time = time( NULL );

    prev_int_start_time = curr_int_start_time;

    /* Process messages and commands */
    for (loopcount = 0; ; loopcount++)
    {
#if defined( _MSVC_ )
        /* Wait for keyboard input */
        for (i = sysblk.panrate / WAIT_FOR_KEYBOARD_INPUT_SLEEP_MILLISECS; i && !kbhit(); i--)
            Sleep(                WAIT_FOR_KEYBOARD_INPUT_SLEEP_MILLISECS );

        ADJ_SCREEN_SIZE();

        /* If keyboard input has [finally] arrived, then process it */
        if ( kbhit() )
        {
            /* Read character(s) from the keyboard */
            kbbuf[0] = getch();
            kbbuf[kblen=1] = '\0';
            translate_keystroke( kbbuf, &kblen );

#else // !defined( _MSVC_ )

        /* Set the file descriptors for select */
        FD_ZERO (&readset);
        FD_SET (keybfd, &readset);

        /* Wait for a key to be pressed,
           or the inactivity interval to expire */
        tv.tv_sec  =  sysblk.panrate / 1000;
        tv.tv_usec = (sysblk.panrate * 1000) % 1000000;
        rc = select (keybfd + 1, &readset, NULL, NULL, &tv);
        if (rc < 0 )
        {
            if (errno == EINTR) continue;
            // "select: %s"
            fprintf (stderr, MSG(HHC00014, "E", strerror(errno) ) );
            break;
        }

        ADJ_SCREEN_SIZE();

        /* If keyboard input has arrived then process it */
        if (loopcount && FD_ISSET(keybfd, &readset))
        {
            /* Read character(s) from the keyboard */
            kblen = read (keybfd, kbbuf, kbbufsize-1);

            if (kblen < 0)
            {
                // "keyboard read: %s"
                fprintf (stderr, MSG(HHC00015, "E", strerror(errno) ) );
                break;
            }
            if (!kblen)
            {
                panel_command("quit");             /* Force shutdown */
                break;                /* EOF on input.  Don't loop   */
            }

            kbbuf[kblen] = '\0';

#endif // defined( _MSVC_ )

            /* =NP= : Intercept NP commands & process */
            if (NPDup == 1)
            {
                if (NPdevsel == 1)
                {
                    NPdevsel = 0;
                    NPdevice = kbbuf[0];  /* save the device selected */
                    kbbuf[0] = NPsel2;    /* setup for 2nd part of rtn */
                }
                if (NPdataentry == 0 && kblen == 1)
                {   /* We are in command mode */
                    if (NPhelpup == 1)
                    {
                        if (kbbuf[0] == 0x1b)
                            NPhelpdown = 1;
                        kbbuf[0] = '\0';
                        redraw_status = 1;
                    }
                    cmdline[0] = '\0';
                    cmdlen = 0;
                    cmdoff = 0;
                    ADJ_CMDCOL();
                    switch(kbbuf[0]) {
                        case 0x1B:                  /* ESC */
                            NPDup = 0;
                            restore_command_line();
                            ADJ_CMDCOL();
                            redraw_msgs = redraw_cmd = redraw_status = 1;
                            npquiet = 0; // (forced for one paint cycle)
                            break;
                        case '?':
                            NPhelpup = 1;
                            redraw_status = 1;
                            break;
                        case 'S':                   /* START */
                        case 's':
                            if (!sysblk.hicpu)
                              break;
                            do_panel_command( "startall" );
                            break;
                        case 'P':                   /* STOP */
                        case 'p':
                            if (!sysblk.hicpu)
                              break;
                            do_panel_command( "stopall" );
                            break;
                        case 'O':                   /* Store */
                        case 'o':
                            if (!sysblk.hicpu)
                              break;
                            regs = panel_copy_regs(sysblk.pcpu);
                            aaddr = APPLY_PREFIXING (NPaddress, regs->PX);
                            if (aaddr > regs->mainlim)
                                break;
                            store_fw (regs->mainstor + aaddr, NPdata);
                            redraw_status = 1;
                            break;
                        case 'I':                   /* Display */
                        case 'i':
                            if (!sysblk.hicpu)
                              break;
                            NPregdisp = 4;
                            NPregs_valid = 0;
                            redraw_status = 1;
                            break;
                        case 'g':                   /* display GPR */
                        case 'G':
                            if (!sysblk.hicpu)
                              break;
                            NPregdisp = 0;
                            NPregs_valid = 0;
                            redraw_status = 1;
                            break;
                        case 'a':                   /* Display AR */
                        case 'A':
                            if (!sysblk.hicpu)
                              break;
                            NPregdisp = 2;
                            NPregs_valid = 0;
                            redraw_status = 1;
                            break;
                        case 'c':
                        case 'C':                   /* Case CR */
                            if (!sysblk.hicpu)
                              break;
                            NPregdisp = 1;
                            NPregs_valid = 0;
                            redraw_status = 1;
                            break;
                        case 'f':                   /* Case FPR */
                        case 'F':
                            if (!sysblk.hicpu)
                              break;
                            NPregdisp = 3;
                            NPregs_valid = 0;
                            redraw_status = 1;
                            break;
                        case 'r':                   /* Enter address */
                        case 'R':
                            if (!sysblk.hicpu)
                              break;
                            NPdataentry = 1;
                            redraw_cmd = 1;
                            NPpending = 'r';
                            NPcurrow = 16;
                            NPcurcol = 12;
                            NPdatalen = 8;
                            NPcolorSwitch = 1;
                            NPcolorFore = COLOR_WHITE;
                            NPcolorBack = COLOR_BLUE;
                            STRLCPY( NPentered, "" );
                            STRLCPY( NPprompt1, "Enter Address" );
                            redraw_status = 1;
                            break;
                        case 'd':                   /* Enter data */
                        case 'D':
                            if (!sysblk.hicpu)
                              break;
                            NPdataentry = 1;
                            redraw_cmd = 1;
                            NPpending = 'd';
                            NPcurrow = 16;
                            NPcurcol = 30;
                            NPdatalen = 8;
                            NPcolorSwitch = 1;
                            NPcolorFore = COLOR_WHITE;
                            NPcolorBack = COLOR_BLUE;
                            STRLCPY( NPentered, "" );
                            STRLCPY( NPprompt1, "Enter Data Value" );
                            redraw_status = 1;
                            break;
                        case 'l':                   /* IPL */
                        case 'L':
                            if (!sysblk.hicpu)
                              break;
                            NPdevsel = 1;
                            NPsel2 = 1;
                            STRLCPY( NPprompt2, "Select Device for IPL" );
                            redraw_status = 1;
                            break;
                        case 1:                     /* IPL - 2nd part */
                            if (!sysblk.hicpu)
                              break;
                            i = toupper((unsigned char)NPdevice) - 'A';
                            if (i < 0 || i > NPlastdev) {
                                memset(NPprompt2,0,sizeof(NPprompt2));
                                redraw_status = 1;
                                break;
                            }
                            MSGBUF (cmdline, "ipl %4.4x", NPdevnum[i]);
                            do_panel_command(cmdline);
                            memset(NPprompt2,0,sizeof(NPprompt2));
                            redraw_status = 1;
                            break;
                        case 'u':                   /* Device interrupt */
                        case 'U':
                            NPdevsel = 1;
                            NPsel2 = 2;
                            STRLCPY( NPprompt2, "Select Device for Interrupt" );
                            redraw_status = 1;
                            break;
                        case 2:                     /* Device int: part 2 */
                            if (!sysblk.hicpu)
                              break;
                            i = toupper((unsigned char)NPdevice) - 'A';
                            if (i < 0 || i > NPlastdev) {
                                memset(NPprompt2,0,sizeof(NPprompt2));
                                redraw_status = 1;
                                break;
                            }
                            MSGBUF( cmdline, "i %4.4x", NPdevnum[i]);
                            do_panel_command(cmdline);
                            memset(NPprompt2,0,sizeof(NPprompt2));
                            redraw_status = 1;
                            break;
                        case 'n':                   /* Device Assignment */
                        case 'N':
                            NPdevsel = 1;
                            NPsel2 = 3;
                            STRLCPY( NPprompt2, "Select Device to Reassign" );
                            redraw_status = 1;
                            break;
                        case 3:                     /* Device asgn: part 2 */
                            i = toupper((unsigned char)NPdevice) - 'A';
                            if (i < 0 || i > NPlastdev) {
                                memset(NPprompt2,0,sizeof(NPprompt2));
                                redraw_status = 1;
                                break;
                            }
                            NPdataentry = 1;
                            redraw_cmd = 1;
                            NPpending = 'n';
                            NPasgn = i;
                            NPcurrow = 3 + i;
                            NPcurcol = 58;
                            NPdatalen = cons_cols - 57;
                            NPcolorSwitch = 1;
                            NPcolorFore = COLOR_DEFAULT_LIGHT;
                            NPcolorBack = COLOR_BLUE;
                            memset(NPentered,0,sizeof(NPentered));
                            STRLCPY( NPprompt2, "New Name, or [enter] to Reload" );
                            redraw_status = 1;
                            break;
                        case 'W':                   /* POWER */
                        case 'w':
                            NPdevsel = 1;
                            NPsel2 = 4;
                            STRLCPY( NPprompt1, "Confirm Powerdown Y or N" );
                            redraw_status = 1;
                            break;
                        case 4:                     /* POWER - 2nd part */
                            if (NPdevice == 'y' || NPdevice == 'Y')
                                do_panel_command( "quit" );
                            memset(NPprompt1, 0, sizeof(NPprompt1));
                            redraw_status = 1;
                            break;
                        case 'T':                   /* Restart */
                        case 't':
                            if (!sysblk.hicpu)
                              break;
                            NPdevsel = 1;
                            NPsel2 = 5;
                            STRLCPY( NPprompt1, "Confirm Restart Y or N" );
                            redraw_status = 1;
                            break;
                        case 5:                    /* Restart - part 2 */
                            if (!sysblk.hicpu)
                              break;
                            if (NPdevice == 'y' || NPdevice == 'Y')
                                do_panel_command( "restart" );
                            memset(NPprompt1, 0, sizeof(NPprompt1));
                            redraw_status = 1;
                            break;
                        case 'E':                   /* Ext int */
                        case 'e':
                            if (!sysblk.hicpu)
                              break;
                            NPdevsel = 1;
                            NPsel2 = 6;
                            STRLCPY( NPprompt1, "Confirm External Interrupt Y or N" );
                            redraw_status = 1;
                            break;
                        case 6:                    /* External - part 2 */
                            if (!sysblk.hicpu)
                              break;
                            if (NPdevice == 'y' || NPdevice == 'Y')
                                do_panel_command( "ext" );
                            memset(NPprompt1, 0, sizeof(NPprompt1));
                            redraw_status = 1;
                            break;
                        default:
                            break;
                    }
                    NPcmd = 1;
                } else {  /* We are in data entry mode */
                    if (kbbuf[0] == 0x1B) {
                        /* Switch back to command mode */
                        NPdataentry = 0;
                        NPaddr_valid = 0;
                        NPdata_valid = 0;
                        memset(NPprompt1, 0,sizeof(NPprompt1));
                        memset(NPprompt2, 0, sizeof(NPprompt2));
                        NPcmd = 1;
                    }
                    else
                        NPcmd = 0;
                }
                if (NPcmd == 1)
                    kblen = 0;                  /* don't process as command */
            }
            /* =NP END= */

            /* Process characters in the keyboard buffer */
            for (i = 0; i < kblen; )
            {
#if defined ( _MSVC_ )
                /* Test for PF key  Windows */
                if ( strlen(kbbuf+i) == 4 && kbbuf[i] == '\x1b' && kbbuf[i+1] == ')' ) /* this is a PF Key */
                {
                    char szPF[6];
                    char msgbuf[32];
                    char *pf;
                    char *psz_PF;
                    int j;

                    MSGBUF( szPF, "PF%s", kbbuf+2 );
                    szPF[4] = '\0';

#else   // ! _MSVC_
                if ( !strcmp(kbbuf+i, KBD_PF1) || !strcmp(kbbuf+i, KBD_PF1_a) ||
                     !strcmp(kbbuf+i, KBD_PF2) || !strcmp(kbbuf+i, KBD_PF2_a) ||
                     !strcmp(kbbuf+i, KBD_PF3) || !strcmp(kbbuf+i, KBD_PF3_a) ||
                     !strcmp(kbbuf+i, KBD_PF4) || !strcmp(kbbuf+i, KBD_PF4_a) ||
                     !strcmp(kbbuf+i, KBD_PF5) || !strcmp(kbbuf+i, KBD_PF6  ) ||
                     !strcmp(kbbuf+i, KBD_PF7) || !strcmp(kbbuf+i, KBD_PF8  ) ||
                     !strcmp(kbbuf+i, KBD_PF9) || !strcmp(kbbuf+i, KBD_PF10 ) ||
                     !strcmp(kbbuf+i, KBD_PF11)|| !strcmp(kbbuf+i, KBD_PF12 ) ||
                     !strcmp(kbbuf+i, KBD_PF13)|| !strcmp(kbbuf+i, KBD_PF14 ) ||
                     !strcmp(kbbuf+i, KBD_PF15)|| !strcmp(kbbuf+i, KBD_PF16 ) ||
                     !strcmp(kbbuf+i, KBD_PF17)|| !strcmp(kbbuf+i, KBD_PF18 ) ||
                     !strcmp(kbbuf+i, KBD_PF19)|| !strcmp(kbbuf+i, KBD_PF20 ) )
                {
                    char *szPF;
                    char msgbuf[32];
                    char *pf;
                    char *psz_PF;
                    int j;

                    if      ( !strcmp(kbbuf+i, KBD_PF1) || !strcmp(kbbuf+i, KBD_PF1_a) ) szPF = "PF01";
                    else if ( !strcmp(kbbuf+i, KBD_PF2) || !strcmp(kbbuf+i, KBD_PF2_a) ) szPF = "PF02";
                    else if ( !strcmp(kbbuf+i, KBD_PF3) || !strcmp(kbbuf+i, KBD_PF3_a) ) szPF = "PF03";
                    else if ( !strcmp(kbbuf+i, KBD_PF4) || !strcmp(kbbuf+i, KBD_PF4_a) ) szPF = "PF04";
                    else if ( !strcmp(kbbuf+i, KBD_PF5)                                ) szPF = "PF05";
                    else if ( !strcmp(kbbuf+i, KBD_PF6)                                ) szPF = "PF06";
                    else if ( !strcmp(kbbuf+i, KBD_PF7)                                ) szPF = "PF07";
                    else if ( !strcmp(kbbuf+i, KBD_PF8)                                ) szPF = "PF08";
                    else if ( !strcmp(kbbuf+i, KBD_PF9)                                ) szPF = "PF09";
                    else if ( !strcmp(kbbuf+i, KBD_PF10)                               ) szPF = "PF10";
                    else if ( !strcmp(kbbuf+i, KBD_PF11)                               ) szPF = "PF11";
                    else if ( !strcmp(kbbuf+i, KBD_PF12)                               ) szPF = "PF12";
                    else if ( !strcmp(kbbuf+i, KBD_PF13)                               ) szPF = "PF13";
                    else if ( !strcmp(kbbuf+i, KBD_PF14)                               ) szPF = "PF14";
                    else if ( !strcmp(kbbuf+i, KBD_PF15)                               ) szPF = "PF15";
                    else if ( !strcmp(kbbuf+i, KBD_PF16)                               ) szPF = "PF16";
                    else if ( !strcmp(kbbuf+i, KBD_PF17)                               ) szPF = "PF17";
                    else if ( !strcmp(kbbuf+i, KBD_PF18)                               ) szPF = "PF18";
                    else if ( !strcmp(kbbuf+i, KBD_PF19)                               ) szPF = "PF19";
                    else if ( !strcmp(kbbuf+i, KBD_PF20)                               ) szPF = "PF20";
                    else szPF = NULL;
#endif

                    if (!(pf = (char*) get_symbol( szPF )) || !*pf)
                    {
                        MSGBUF( msgbuf, "DELAY * %s UNDEFINED", szPF );
                        pf = msgbuf;
                    }

                    psz_PF = strdup(pf);

                    /* test for 1st of IMMED, DELAY or SUBST */
                    for ( j = 0; j < (int)strlen(psz_PF); j++ )
                        if ( psz_PF[j] != ' ' ) break;

                    if ( !strncasecmp( psz_PF+j, "IMMED ", 6 ) )
                    {
                        for ( j += 5; j < (int)strlen(psz_PF); j++ )
                            if ( psz_PF[j] != ' ' ) break;
                        do_panel_command( psz_PF+j );
                    }
                    else if ( !strncasecmp( psz_PF+j, "DELAY ", 6 ) )
                    {
                        for ( j += 5; j < (int)strlen(psz_PF); j++ )
                            if ( psz_PF[j] != ' ' ) break;
                        STRLCPY( cmdline, psz_PF+j );
                        cmdlen = (int)strlen(cmdline);
                        cmdoff = cmdlen < cmdcols ? cmdlen : 0;
                        ADJ_CMDCOL();
                    }
                    else if ( !strncasecmp( psz_PF+j, "SUBST ", 6 ) )
                    {
                        int  isdelay = TRUE;
                        char *cmd_tok[11] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
                        int   ncmd_tok = 0;
                        char *pt1;

                        for ( j += 5; j < (int)strlen(psz_PF); j++ )
                            if ( psz_PF[j] != ' ' ) break;

                        if ( !strncasecmp( psz_PF+j, "IMMED ", 6 ) )
                        {
                            isdelay = FALSE;
                            for ( j += 5; j < (int)strlen(psz_PF); j++ )
                                if ( psz_PF[j] != ' ' ) break;

                        }
                        else if ( !strncasecmp( psz_PF+j, "DELAY ", 6 ) )
                        {
                            for ( j += 5; j < (int)strlen(psz_PF); j++ )
                                if ( psz_PF[j] != ' ' ) break;
                        }

                        if ( ( cmdlen = (int)strlen(cmdline) ) > 0 )
                        {
                            char    psz_cmdline[sizeof(cmdline)];
                            char   *p = cmdline;

                            while (*p && ncmd_tok < 10 )
                            {
                                while (*p && isspace((unsigned char)*p))
                                {
                                    p++;
                                }
                                if (!*p)
                                {
                                    break; /* find start of arg */
                                }

                                if (*p == '#') break; // stop on comments

                                cmd_tok[ncmd_tok] = p; ++ncmd_tok; // count new arg

                                while ( *p
                                        && !isspace((unsigned char)*p)
                                        && *p != '\"'
                                        && *p != '\'' )
                                {
                                    p++;
                                }
                                if (!*p)
                                {
                                    break; /* find end of arg */
                                }

                                if (*p == '\"' || *p == '\'')
                                {
                                    char delim = *p;

                                    do {} while (*++p && *p != delim);
                                    if (!*p) break;                    // find end of quoted string
                                    p++;
                                    if (!*p) break;                    // found end of args

                                    if ( *p != ' ')
                                    {
                                        strlcpy(psz_cmdline, p, sizeof(cmdline));
                                        *p = ' ';                       // insert a space
                                        strcpy(p+1, psz_cmdline);
                                        psz_cmdline[0] = 0;
                                    }
                                }

                                *p++ = 0; // mark end of arg
                            }

                            ncmd_tok--;

                            /* token 10 represents the rest of the line */
                            if ( ncmd_tok == 9 && strlen(p) > 0 )
                                cmd_tok[++ncmd_tok] = p;
                        }

                        {
                            int     ctok = -1;
                            int     odx  = 0;
                            int     idx  = 0;
                            char    psz_cmdline[(sizeof(cmdline) * 11)];

                            memset(psz_cmdline, 0, sizeof(psz_cmdline));

                            pt1 = psz_PF+j;

                            for ( idx = 0; idx < (int)strlen(pt1); idx++ )
                            {
                                if ( pt1[idx] != '&' )
                                {
                                    psz_cmdline[odx++] = pt1[idx];
                                }
                                else
                                {
                                    if ( pt1[idx+1] == '&' )
                                    {
                                        idx++;
                                        psz_cmdline[odx++] = pt1[idx];
                                    }
                                    else if ( pt1[idx+1] == '*' )
                                    {
                                        idx++;

                                        if ( ctok < 9 )
                                        {
                                            int first = TRUE;

                                            for( ctok++; ctok <= 10; ctok++ )
                                            {
                                                if ( cmd_tok[ctok] != NULL )
                                                {
                                                    if ( first )
                                                        first = FALSE;
                                                    else
                                                        psz_cmdline[odx++] = ' ';         // add one space
                                                    strcpy( &psz_cmdline[odx], cmd_tok[ctok] );
                                                    odx += (int)strlen(cmd_tok[ctok]);

                                                }
                                            }
                                        }
                                    }
                                    else if ( !isdigit( (unsigned char)pt1[idx+1] ) && ( pt1[idx+1] != '$' ) )
                                    {
                                        psz_cmdline[odx++] = pt1[idx];
                                    }
                                    else
                                    {
                                        idx++;
                                        if ( pt1[idx] == '$' )
                                            ctok = 10;
                                        else
                                        {
                                            ctok = (int)pt1[idx] - '0';
                                            if ( ctok < 0 || ctok > 9 ) ctok = 10;
                                        }
                                        if ( cmd_tok[ctok] != NULL && strlen( cmd_tok[ctok] ) > 0 )
                                        {
                                            memcpy(&psz_cmdline[odx], cmd_tok[ctok], strlen( cmd_tok[ctok] ) );
                                            odx += (int)strlen( cmd_tok[ctok] );
                                        }
                                        else
                                            psz_cmdline[odx++] = ' ';
                                    }
                                }

                                if ( odx > (int)sizeof(cmdline) )
                                {
                                    WRMSG(HHC01608, "W", ((int)sizeof(cmdline) - 1) );
                                    break;
                                }
                            }

                            if ( isdelay )
                            {
                                strncpy( cmdline, psz_cmdline, sizeof(cmdline) );
                                cmdline[sizeof(cmdline) - 1] = 0;
                                cmdlen = (int)strlen(cmdline);
                                cmdoff = cmdlen < cmdcols ? cmdlen : 0;
                                ADJ_CMDCOL();
                            }
                            else
                                do_panel_command( psz_cmdline );
                        }
                    }
                    else /* this is the same as IMMED */
                    {
                        do_panel_command( psz_PF );
                    }
                    free(psz_PF);
                    redraw_cmd = 1;
                    redraw_msgs = 1;
                    break;
                }

                /* Test for HOME */
                if (strcmp(kbbuf+i, KBD_HOME) == 0) {
                    if (NPDup == 1 || !is_cursor_on_cmdline() || cmdlen) {
                        cursor_cmdline_home();
                        redraw_cmd = 1;
                    } else {
                        scroll_to_top_line();
                        redraw_msgs = 1;
                    }
                    break;
                }

                /* Test for END */
                if (strcmp(kbbuf+i, KBD_END) == 0) {
                    if (NPDup == 1 || !is_cursor_on_cmdline() || cmdlen) {
                        cursor_cmdline_end();
                        redraw_cmd = 1;
                    } else {
                        scroll_to_bottom_screen();
                        redraw_msgs = 1;
                    }
                    break;
                }

                /* Test for CTRL+HOME */
                if (NPDup == 0 && strcmp(kbbuf+i, KBD_CTRL_HOME) == 0) {
                    scroll_to_top_line();
                    redraw_msgs = 1;
                    break;
                }

                /* Test for CTRL+END */
                if (NPDup == 0 && strcmp(kbbuf+i, KBD_CTRL_END) == 0) {
                    scroll_to_bottom_line();
                    redraw_msgs = 1;
                    break;
                }

                /* Process UPARROW */
                if (NPDup == 0 && strcmp(kbbuf+i, KBD_UP_ARROW) == 0)
                {
                    do_prev_history();
                    redraw_cmd = 1;
                    break;
                }

                /* Process DOWNARROW */
                if (NPDup == 0 && strcmp(kbbuf+i, KBD_DOWN_ARROW) == 0)
                {
                    do_next_history();
                    redraw_cmd = 1;
                    break;
                }

#if defined(OPTION_EXTCURS)
                /* Process ALT+UPARROW */
                if (NPDup == 0 && strcmp(kbbuf+i, KBD_ALT_UP_ARROW) == 0) {
                    get_cursor_pos( keybfd, confp, &cur_cons_row, &cur_cons_col );
                    if (cur_cons_row <= 1)
                        cur_cons_row = cons_rows + 1;
                    set_pos( --cur_cons_row, cur_cons_col );
                    break;
                }

                /* Process ALT+DOWNARROW */
                if (NPDup == 0 && strcmp(kbbuf+i, KBD_ALT_DOWN_ARROW) == 0) {
                    get_cursor_pos( keybfd, confp, &cur_cons_row, &cur_cons_col );
                    if (cur_cons_row >= cons_rows)
                        cur_cons_row = 1 - 1;
                    set_pos( ++cur_cons_row, cur_cons_col );
                    break;
                }
#endif // defined(OPTION_EXTCURS)

                /* Test for PAGEUP */
                if (NPDup == 0 && strcmp(kbbuf+i, KBD_PAGE_UP) == 0) {
                    page_up();
                    redraw_msgs = 1;
                    break;
                }

                /* Test for PAGEDOWN */
                if (NPDup == 0 && strcmp(kbbuf+i, KBD_PAGE_DOWN) == 0) {
                    page_down();
                    redraw_msgs = 1;
                    break;
                }

                /* Test for CTRL+UPARROW */
                if (NPDup == 0 && strcmp(kbbuf+i, KBD_CTRL_UP_ARROW) == 0) {
                    scroll_up_lines(1);
                    redraw_msgs = 1;
                    break;
                }

                /* Test for CTRL+DOWNARROW */
                if (NPDup == 0 && strcmp(kbbuf+i, KBD_CTRL_DOWN_ARROW) == 0) {
                    scroll_down_lines(1);
                    redraw_msgs = 1;
                    break;
                }

                /* Process BACKSPACE */
                if (kbbuf[i] == '\b' || kbbuf[i] == '\x7F') {
                    if (NPDup == 0 && !is_cursor_on_cmdline())
                        beep();
                    else {
                        if (cmdoff > 0) {
                            int j;
                            for (j = cmdoff-1; j<cmdlen; j++)
                                cmdline[j] = cmdline[j+1];
                            cmdoff--;
                            cmdlen--;
                            ADJ_CMDCOL();
                        }
                        i++;
                        redraw_cmd = 1;
                    }
                    break;
                }

                /* Process DELETE */
                if (strcmp(kbbuf+i, KBD_DELETE) == 0) {
                    if (NPDup == 0 && !is_cursor_on_cmdline())
                        beep();
                    else {
                        if (cmdoff < cmdlen) {
                            int j;
                            for (j = cmdoff; j<cmdlen; j++)
                                cmdline[j] = cmdline[j+1];
                            cmdlen--;
                        }
                        i++;
                        redraw_cmd = 1;
                    }
                    break;
                }

                /* Process LEFTARROW */
                if (strcmp(kbbuf+i, KBD_LEFT_ARROW) == 0) {
                    if (cmdoff > 0) cmdoff--;
                    ADJ_CMDCOL();
                    i++;
                    redraw_cmd = 1;
                    break;
                }

                /* Process RIGHTARROW */
                if (strcmp(kbbuf+i, KBD_RIGHT_ARROW) == 0) {
                    if (cmdoff < cmdlen) cmdoff++;
                    ADJ_CMDCOL();
                    i++;
                    redraw_cmd = 1;
                    break;
                }

#if defined(OPTION_EXTCURS)
                /* Process ALT+LEFTARROW */
                if (NPDup == 0 && strcmp(kbbuf+i, KBD_ALT_LEFT_ARROW) == 0) {
                    get_cursor_pos( keybfd, confp, &cur_cons_row, &cur_cons_col );
                    if (cur_cons_col <= 1)
                    {
                        cur_cons_row--;
                        cur_cons_col = cons_cols + 1;
                    }
                    if (cur_cons_row < 1)
                        cur_cons_row = cons_rows;
                    set_pos( cur_cons_row, --cur_cons_col );
                    break;
                }

                /* Process ALT+RIGHTARROW */
                if (NPDup == 0 && strcmp(kbbuf+i, KBD_ALT_RIGHT_ARROW) == 0) {
                    get_cursor_pos( keybfd, confp, &cur_cons_row, &cur_cons_col );
                    if (cur_cons_col >= cons_cols)
                    {
                        cur_cons_row++;
                        cur_cons_col = 0;
                    }
                    if (cur_cons_row > cons_rows)
                        cur_cons_row = 1;
                    set_pos( cur_cons_row, ++cur_cons_col );
                    break;
                }
#endif // defined(OPTION_EXTCURS)

                /* Process INSERT */
                if (strcmp(kbbuf+i, KBD_INSERT) == 0 ) {
                    cmdins = !cmdins;
                    set_console_cursor_shape( confp, cmdins );
                    i++;
                    break;
                }

                /* Process ESCAPE */
                if (kbbuf[i] == '\x1B') {
                    /* If data on cmdline, erase it */
                    if ((NPDup == 1 || is_cursor_on_cmdline()) && cmdlen) {
                        cmdline[0] = '\0';
                        cmdlen = 0;
                        cmdoff = 0;
                        ADJ_CMDCOL();
                        redraw_cmd = 1;
                    } else {
                        /* =NP= : Switch to new panel display */
                        save_command_line();
                        NP_init();
                        NPDup = 1;
                        /* =END= */
                    }
                    break;
                }

                /* Process TAB */
                if (kbbuf[i] == '\t' || kbbuf[i] == '\x7F')
                {
                    if (NPDup == 1 || !is_cursor_on_cmdline())
                    {
                        cursor_cmdline_home();
                        redraw_cmd = 1;
                    }
                    else
                    {
                        tab_pressed(cmdline, sizeof(cmdline), &cmdoff);
                        cmdlen = (int)strlen(cmdline);
                        ADJ_CMDCOL();
                        i++;
                        redraw_cmd = 1;
                    }
                    break;
                }

#if defined(OPTION_EXTCURS)
                /* ENTER key special handling */
                if (NPDup == 0 && kbbuf[i] == '\n')
                {
                    /* Get cursor pos and check if on cmdline */
                    if (!is_cursor_on_cmdline())
                    {
                        beep();
                        break;
                    }
                }
#endif // defined(OPTION_EXTCURS)

                /* Process the command when the ENTER key is pressed */
                if (kbbuf[i] == '\n') {
                    if (1
                        && cmdlen == 0
                        && NPDup == 0
                        && !sysblk.instbreak
                    ) {
                        history_show();
                    } else {
                        cmdline[cmdlen] = '\0';
                        /* =NP= create_thread replaced with: */
                        if (NPDup == 0) {
                            if ('#' == cmdline[0] || '*' == cmdline[0]) {
                                if (!is_currline_visible())
                                    scroll_to_bottom_screen();
                                history_requested = 0;
                                do_panel_command(cmdline);
                                redraw_cmd = 1;
                                cmdlen = 0;
                                cmdoff = 0;
                                ADJ_CMDCOL();
                                redraw_cmd = 1;
                            } else {
                                history_requested = 0;
                                do_panel_command(cmdline);
                                redraw_cmd = 1;
                                if (history_requested == 1) {
                                    STRLCPY(cmdline, historyCmdLine);
                                    cmdlen = (int)strlen(cmdline);
                                    cmdoff = cmdlen < cmdcols ? cmdlen : 0;
                                    ADJ_CMDCOL();
                                    redraw_cmd = 1;
                                    cursor_cmdline_end();
                                    break;
                                }
                            }
                        } else {
                            NPdataentry = 0;
                            NPcurrow = cons_rows;
                            NPcurcol = cons_cols;
                            NPcolorSwitch = 0;
                            switch (NPpending) {
                                case 'r':
                                    sscanf(cmdline, "%x", &NPaddress);
                                    NPaddr_valid = 0;
                                    STRLCPY(NPprompt1, "");
                                    break;
                                case 'd':
                                    sscanf(cmdline, "%x", &NPdata);
                                    NPdata_valid = 0;
                                    STRLCPY(NPprompt1, "");
                                    break;
                                case 'n':
                                    if (strlen(cmdline) < 1) {
                                        STRLCPY(cmdline, NPdevnam[NPasgn]);
                                    }
                                    STRLCPY(NPdevnam[NPasgn], "");
                                    MSGBUF (NPentered, "devinit %4.4x %s",
                                             NPdevnum[NPasgn], cmdline);
                                    do_panel_command(NPentered);
                                    STRLCPY(NPprompt2, "");
                                    break;
                                default:
                                    break;
                            }
                            redraw_status = 1;
                            cmdline[0] = '\0';
                            cmdlen = 0;
                            cmdoff = 0;
                            ADJ_CMDCOL();
                        }
                        /* =END= */
                        redraw_cmd = 1;
                    }
                    break;
                } /* end if (kbbuf[i] == '\n') */

                /* Ignore non-printable characters */
                if (!isprint((unsigned char)kbbuf[i])) {
                    beep();
                    i++;
                    continue;
                }

                /* Ignore all other keystrokes not on cmdline */
                if (NPDup == 0 && !is_cursor_on_cmdline()) {
                    beep();
                    break;
                }

                /* Append the character to the command buffer */
                ASSERT(cmdlen <= CMD_SIZE-1 && cmdoff <= cmdlen);
                if (0
                    || (cmdoff >= CMD_SIZE-1)
                    || (cmdins && cmdlen >= CMD_SIZE-1)
                )
                {
                    /* (no more room!) */
                    beep();
                }
                else /* (there's still room) */
                {
                    ASSERT(cmdlen < CMD_SIZE-1 || (!cmdins && cmdoff < cmdlen));
                    if (cmdoff >= cmdlen)
                    {
                        /* Append to end of buffer */
                        ASSERT(!(cmdoff > cmdlen)); // (sanity check)
                        cmdline[cmdoff++] = kbbuf[i];
                        cmdline[cmdoff] = '\0';
                        cmdlen++;
                    }
                    else
                    {
                        ASSERT(cmdoff < cmdlen);
                        if (cmdins)
                        {
                            /* Insert: make room by sliding all
                               following chars right one position */
                            int j;
                            for (j=cmdlen-1; j>=cmdoff; j--)
                                cmdline[j+1] = cmdline[j];
                            cmdline[cmdoff++] = kbbuf[i];
                            cmdlen++;
                        }
                        else
                        {
                            /* Overlay: replace current position */
                            cmdline[cmdoff++] = kbbuf[i];
                        }
                    }
                    ADJ_CMDCOL();
                    redraw_cmd = 1;
                }
                i++;
            } /* end for(i) */
        } /* end if keystroke */

FinishShutdown:

        // If we finished processing all of the message data
        // the last time we were here, then get some more...

        if ( lmsndx >= lmscnt )  // (all previous data processed?)
        {
            lmscnt = log_read( &lmsbuf, &lmsnum, LOG_NOBLOCK );
            lmsndx = 0;
        }
        else if ( lmsndx >= lmsmax )
        {
            lmsbuf += lmsndx;   // pick up where we left off at...
            lmscnt -= lmsndx;   // pick up where we left off at...
            lmsndx = 0;
        }

        // Process all message data or until limit reached...

        // (limiting the amount of data we process a console message flood
        // from preventing keyboard from being read since we need to break
        // out of the below message data processing loop to loop back up
        // to read the keyboard again...)

        /* Read message bytes until newline... */
        while ( lmsndx < lmscnt && lmsndx < lmsmax )
        {
            /* Initialize the read buffer */
            if (!readoff || readoff >= MSG_SIZE) {
                memset (readbuf, SPACE, MSG_SIZE);
                readoff = 0;
            }

            /* Read message bytes and copy into read buffer
               until we either encounter a newline character
               or our buffer is completely filled with data. */
            while ( lmsndx < lmscnt && lmsndx < lmsmax )
            {
                /* Read a byte from the message pipe */
                c = *(lmsbuf + lmsndx); lmsndx++;

                /* Break to process received message
                   whenever a newline is encountered */
                if (c == '\n' || c == '\r') {
                    readoff = 0;    /* (for next time) */
                    break;
                }

                /* Handle tab character */
                if (c == '\t') {
                    readoff += 8;
                    readoff &= 0xFFFFFFF8;
                    /* Messages longer than one screen line will
                       be continued on the very next screen line */
                    if (readoff >= MSG_SIZE)
                        break;
                    else continue;
                }

                /* Eliminate non-displayable characters */
                if (!isgraph(c)) c = SPACE;

                /* Stuff byte into message processing buffer */
                readbuf[readoff++] = c;

                /* Messages longer than one screen line will
                   be continued on the very next screen line */
                if (readoff >= MSG_SIZE)
                    break;
            } /* end while ( lmsndx < lmscnt && lmsndx < lmsmax ) */

            /* If we have a message to be displayed (or a complete
               part of one), then copy it to the circular buffer. */
            if (!readoff || readoff >= MSG_SIZE) {

                /* First-time here? */
                if (curmsg == NULL) {
                    curmsg = topmsg = msgbuf;
                } else {
                    /* Perform autoscroll if needed */
                    if (is_currline_visible()) {
                        while (lines_remaining() < 1)
                            scroll_down_lines(1);
                        /* Set the display update indicator */
                        redraw_msgs = 1;
                    }

                    /* Go on to next available msg buffer */
                    curmsg = curmsg->next;

                    /* Updated wrapped indicator */
                    if (curmsg == msgbuf)
                        wrapped = 1;
                }

                /* Copy message into next available PANMSG slot */
                memcpy( curmsg->msg, readbuf, MSG_SIZE );

            } /* end if (!readoff || readoff >= MSG_SIZE) */
        } /* end Read message bytes until newline... */

        /* Don't read or otherwise process any input
           once system shutdown has been initiated
        */
        if ( sysblk.shutdown )
        {
            if ( sysblk.shutfini ) break;
            /* wait for system to finish shutting down */
            USLEEP(10000);
            lmsmax = INT_MAX;
            goto FinishShutdown;
        }

        /* =NP= : Reinit traditional panel if NP is down */
        if (NPDup == 0 && NPDinit == 1) {
            redraw_msgs = redraw_status = redraw_cmd = 1;
            set_color (COLOR_DEFAULT_FG, COLOR_DEFAULT_BG);
            clr_screen ();
        }
        /* =END= */

        /* Obtain the PSW for target CPU */
        regs = panel_copy_regs( sysblk.pcpu );
        memset( curr_psw, 0, sizeof( curr_psw ));
        copy_psw( regs, curr_psw );

        numcpu = 0;
        for (i=0; i < sysblk.maxcpu; ++i )
            if (IS_CPU_ONLINE( i ))
                ++numcpu;

        /* Set the display update indicator
           if anything interesting happened.
        */
        if (0
            || memcmp( curr_psw, prev_psw, sizeof( curr_psw )) != 0
            || prev_cpupct    != regs->cpupct
            || prev_cpustate  != regs->cpustate
            || prev_instcount != sysblk.instcount
            || prev_mipsrate  != sysblk.mipsrate
            || prev_siosrate  != sysblk.siosrate
#if defined( OPTION_SHARED_DEVICES )
            || prev_shrdcount != sysblk.shrdcount
#endif
        )
        {
            redraw_status = 1;
            memcpy( prev_psw, curr_psw, sizeof( prev_psw ));
            prev_cpupct    = regs->cpupct;
            prev_cpustate  = regs->cpustate;
            prev_instcount = sysblk.instcount;
            prev_mipsrate  = sysblk.mipsrate;
            prev_siosrate  = sysblk.siosrate;
#if defined( OPTION_SHARED_DEVICES )
            prev_shrdcount = sysblk.shrdcount;
#endif
        }

        /* =NP= : Display the screen - traditional or NP */
        /*        Note: this is the only code block modified rather */
        /*        than inserted.  It makes the block of 3 ifs in the */
        /*        original code dependent on NPDup == 0, and inserts */
        /*        the NP display as an else after those ifs */

        if (NPDup == 0) {
            int sev, fg_color, bg_color;
            /* Rewrite the screen if display update indicators are set */
            if (redraw_msgs && !npquiet)
            {
                /* Display messages in scrolling area */
                PANMSG* p;

                /* Save cursor location */
                saved_cons_row = cur_cons_row;
                saved_cons_col = cur_cons_col;

                i = 0;
                /* Then draw current screen */
                for (p=topmsg; i < (SCROLL_LINES + numkept) && (p != curmsg->next || p == topmsg); i++, p = p->next)
                {
                    sev = msg_sev( p->msg );

                    fg_color = fg_msgcolor( sev );
                    bg_color = bg_msgcolor( sev );

                    set_pos( i+1, 1 );
                    set_color( fg_color, bg_color );
                    write_text( p->msg, MSG_SIZE );
                }

                /* Pad remainder of screen with blank lines */
                for (; i < (SCROLL_LINES + numkept); i++)
                {
                    set_pos (i+1, 1);
                    set_color (COLOR_DEFAULT_FG, COLOR_DEFAULT_BG);
                    erase_to_eol( confp );
                }

                /* Display the scroll indicators */
                if (topmsg != oldest_msg())
                {
                    /* More messages precede top line */
                    set_pos (1, cons_cols);
                    set_color (COLOR_DEFAULT_LIGHT, COLOR_DEFAULT_BG);
                    draw_text ("+");
                }
                if (!is_currline_visible())
                {
                    /* More messages follow bottom line */
                    set_pos (cons_rows-2, cons_cols);
                    set_color (COLOR_DEFAULT_LIGHT, COLOR_DEFAULT_BG);
                    draw_text ("V");
                }

                /* restore cursor location */
                cur_cons_row = saved_cons_row;
                cur_cons_col = saved_cons_col;
            } /* end if(redraw_msgs) */

            if (redraw_cmd)
            {
                /* Save cursor location */
                saved_cons_row = cur_cons_row;
                saved_cons_col = cur_cons_col;

                /* Display the command line */
                set_pos (CMDLINE_ROW, 1);
                set_color (COLOR_DEFAULT_LIGHT, COLOR_DEFAULT_BG);
                draw_text( CMD_PREFIX_HERC );
                set_color (COLOR_DEFAULT_FG, COLOR_DEFAULT_BG);
                PUTC_CMDLINE ();
                fill_text (' ',cons_cols);

                /* restore cursor location */
                cur_cons_row = saved_cons_row;
                cur_cons_col = saved_cons_col;
            } /* end if(redraw_cmd) */

            /* Determine if redraw required for CPU or architecture
             * change.
             */
            if (0
                || (sysblk.pcpu != prev_pcpu && (regs = sysblk.regs[ sysblk.pcpu ]) != NULL)
                || ((regs = sysblk.regs[ prev_pcpu ]) != NULL && regs->arch_mode != prev_arch_mode)
            )
            {
                redraw_status = 1;
                prev_pcpu = sysblk.pcpu;
                prev_arch_mode = regs->arch_mode;
            }

            if (redraw_status && !npquiet)
            {
                char ibuf[64];      /* Rate buffer */
                {
                    int cnt_disabled = 0;
                    int cnt_stopped  = 0;
                    int cnt_online = 0;
                    char   *state;
                    for ( i = 0; i < sysblk.maxcpu; i++ )
                    {
                        if ( IS_CPU_ONLINE(i) )
                        {
                            cnt_online++;
                            if ( sysblk.regs[i]->cpustate != CPUSTATE_STARTED )
                                cnt_stopped++;
                            if ( WAITSTATE(&sysblk.regs[i]->psw) &&
                                 IS_IC_DISABLED_WAIT_PSW( sysblk.regs[i] ) )
                                cnt_disabled++;
                        }
                    }
                    state = "RED";

                    if ( cnt_online > cnt_stopped && cnt_disabled == 0 )
                        state = "AMBER";

                    if (0
                        || ( sysblk.hicpu && (cnt_stopped == 0 && cnt_disabled == 0))
#if defined( OPTION_SHARED_DEVICES )
                        || (!sysblk.hicpu && (sysblk.shrdport))
#endif
                    )
                        state = "GREEN";
                    set_console_title(state);
                }

                /* Save cursor location */
                saved_cons_row = cur_cons_row;
                saved_cons_col = cur_cons_col;

                memset (buf, ' ', cons_cols);
                len = MSGBUF ( buf, "%s%02X ",
                    PTYPSTR(sysblk.pcpu), sysblk.pcpu ) ;
                if (IS_CPU_ONLINE(sysblk.pcpu))
                {
                    len += idx_snprintf( len, buf, sizeof(buf), "PSW=%8.8X%8.8X ",
                                   fetch_fw( curr_psw ), fetch_fw( curr_psw + 4 ));
                    if (regs->arch_mode == ARCH_900_IDX)
                        len += idx_snprintf( len, buf, sizeof(buf), "%16.16"PRIX64" ",
                                        fetch_dw( curr_psw + 8 ));
#if defined(_FEATURE_SIE)
                    else
                        if( SIE_MODE(regs) )
                        {
                            for(i = 0;i < 16;i++)
                                buf[len++] = '-';
                            buf[len++] = ' ';
                        }
#endif /*defined(_FEATURE_SIE)*/
                    len += idx_snprintf( len, buf, sizeof(buf), "%2d%c%c%c%c%c%c%c%c",
                           regs->psw.amode64                  ? 64 :
                           regs->psw.amode                    ? 31 : 24,
                           regs->cpustate == CPUSTATE_STOPPED ? 'M' : '.',
                           sysblk.instbreak                   ? 'T' : '.',
                           WAITSTATE(&regs->psw)              ? 'W' : '.',
                           regs->loadstate                    ? 'L' : '.',
                           regs->checkstop                    ? 'C' : '.',
                           PROBSTATE(&regs->psw)              ? 'P' : '.',
                           SIE_MODE(regs)                     ? 'S' : '.',
                           regs->arch_mode == ARCH_900_IDX    ? 'Z' : '.');
                }
                else
                    len += idx_snprintf( len, buf, sizeof(buf), "%s", "Offline");

                memset( buf+len, ' ', sizeof( buf ) - len - 1 );
                buf[ sizeof( buf ) - 1 ] = 0;
                len++;

                /* Bottom line right corner can be when there is space:
                 * ""
                 * "instcnt <string>"
                 * "instcnt <string>; mips nnnnn"
                 * nnnnn can be nnnnn, nnn.n, nn.nn or n.nnn
                 * "instcnt <string>; mips nnnnn; IO/s nnnnnn"
                 * "IO/s nnnnnn"
                 */
                i = 0;
                if (numcpu)
                {
                    U32 mipsrate = prev_mipsrate / 1000000;

                    /* Format instruction count */
                    i = MSGBUF(ibuf,
                                 "instcnt %s",
                                 format_int( prev_instcount ));

                    if ((len + i + 12) < cons_cols)
                    {
                        if (mipsrate > 999)
                            i += idx_snprintf( i, ibuf, sizeof(ibuf),
                                          "; mips %1d,%03d",
                                          prev_mipsrate / 1000000000,
                                          ((prev_mipsrate % 1000000000) +
                                            500000) / 1000000);
                        else
                        {
                            U32 mipsfrac = prev_mipsrate % 1000000;

                            if (mipsrate > 99)
                                i += idx_snprintf( i, ibuf, sizeof(ibuf),
                                              "; mips %3d.%01d",
                                              mipsrate,
                                              (mipsfrac + 50000) / 100000);
                            else if (mipsrate > 9)
                                i += idx_snprintf( i, ibuf, sizeof(ibuf),
                                              "; mips %2d.%02d",
                                              mipsrate,
                                              (mipsfrac + 5000) / 10000);
                            else
                                i += idx_snprintf( i, ibuf, sizeof(ibuf),
                                              "; mips %1d.%03d",
                                              mipsrate,
                                              (mipsfrac + 500) / 1000);
                        }
                    }
                }

                /* Prepare I/O statistics */
                if (1
                    && (len + i + (numcpu ? 13 : 11)) < cons_cols
                    && (0
                        ||   numcpu
#if defined( OPTION_SHARED_DEVICES )
                        || (!numcpu && sysblk.shrdport)
#endif
                       )
                )
                {
                    if (numcpu)
                        ibuf[(int)i++] = ';',
                        ibuf[(int)i++] = ' ';
                    i += idx_snprintf( i, ibuf, sizeof(ibuf),
                                  "I/O %6.6s",
                                  format_int( prev_siosrate ));
                }

                /* Copy prepared statistics to buffer */
                if (i)
                {
                    if ((len + i) < cons_cols)
                        len = cons_cols - i;
                    strcpy (buf + len, ibuf);
                    len = cons_cols - 1;
                }

                buf[cons_cols] = '\0';
                set_pos (cons_rows, 1);
                set_color (COLOR_LIGHT_YELLOW, COLOR_RED);
                draw_text (buf);

                /* restore cursor location */
                cur_cons_row = saved_cons_row;
                cur_cons_col = saved_cons_col;
            } /* end if(redraw_status) */

            /* Flush screen buffer and reset display update indicators */
            if (redraw_msgs || redraw_cmd || redraw_status)
            {
                set_color (COLOR_DEFAULT_FG, COLOR_DEFAULT_BG);
                if (NPDup == 0 && NPDinit == 1)
                {
                    NPDinit = 0;
                    restore_command_line();
                    set_pos (cur_cons_row, cur_cons_col);
                }
                else if (redraw_cmd)
                    set_pos (CMDLINE_ROW, CMDLINE_COL + cmdoff - cmdcol);
                else
                    set_pos (cur_cons_row, cur_cons_col);
                fflush (confp);
                redraw_msgs = redraw_cmd = redraw_status = 0;
            }

        } else { /* (NPDup == 1) */

            if (redraw_status || (NPDinit == 0 && NPDup == 1)
                   || (redraw_cmd && NPdataentry == 1)) {
                if (NPDinit == 0) {
                    NPDinit = 1;
                    NP_screen_redraw(regs);
                    NP_update(regs);
                    fflush (confp);
                }
            }
            /* Update New Panel every panrate interval */
            if (!npquiet) {
                NP_update(regs);
                fflush (confp);
            }
            redraw_msgs = redraw_cmd = redraw_status = 0;
        }

    /* =END= */

        /* Force full screen repaint if needed */
        if (!sysblk.npquiet && npquiet)
            redraw_msgs = redraw_cmd = redraw_status = 1;
        npquiet = sysblk.npquiet;

    } /* end while */

    free( kbbuf );

    sysblk.panel_init = 0;

    LOG_THREAD_END( PANEL_THREAD_NAME  );

    ASSERT( sysblk.shutdown );  // (why else would we be here?!)

} /* end function panel_display */

static void panel_cleanup(void *unused)
{
int i;
PANMSG* p;

    UNREFERENCED(unused);

    log_wakeup(NULL);

    if(topmsg)
    {
        set_screen_color( stderr, COLOR_DEFAULT_FG, COLOR_DEFAULT_BG );
        clear_screen( stderr );

        /* Scroll to last full screen's worth of messages */
        scroll_to_bottom_screen();

        /* Display messages in scrolling area */
        for (i=0, p = topmsg; i < SCROLL_LINES && p != curmsg->next; i++, p = p->next)
        {
            set_pos (i+1, 1);
            set_color (COLOR_DEFAULT_FG, COLOR_DEFAULT_BG);
            write_text (p->msg, MSG_SIZE);
        }
    }
    sysblk.panel_init = 0;      /* Panel is no longer running */

    /* Restore the terminal mode */
    set_or_reset_console_mode( keybfd, 0 );

    /* Position to next line */
    fwrite("\n",1,1,stderr);

    set_screen_color(stderr, COLOR_DEFAULT_FG, COLOR_DEFAULT_BG);

    fflush(stderr);
}

/* HCONSOLE.H   (C) Copyright "Fish" (David B. Trout), 2009-2012     */
/*              (C) Copyright TurboHercules, SAS 2010-2011           */
/*          Hercules hardware console (panel) support functions      */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _HCONSOLE_H
#define _HCONSOLE_H

#define DEFAULT_CONSOLE_ROWS    24
#define DEFAULT_CONSOLE_COLS    80

//-----------------------------------------------------------------------------
//
//                            VT100 User Guide
//
//                   Table 3-6 Cursor Control Key Codes
//
// Cursor Key   VT52       ANSI and Cursor Key Mode   ANSI and Cursor Key Mode
//  (Arrow)     Mode       Reset    (Normal mode)     Set  (Application mode)
//
//   Up         ESC A      ESC [ A                    ESC O A
//   Down       ESC B      ESC [ B                    ESC O B
//   Right      ESC C      ESC [ C                    ESC O C
//   Left       ESC D      ESC [ D                    ESC O D
//
//-----------------------------------------------------------------------------

#define ANSI_RESET_ALL_ATTRIBUTES  "\x1B[0m"

#define KBD_HOME                "\x1B[1~"
#define KBD_INSERT              "\x1B[2~"
#define KBD_DELETE              "\x1B[3~"
#define KBD_END                 "\x1B[4~"
#define KBD_PAGE_UP             "\x1B[5~"
#define KBD_PAGE_DOWN           "\x1B[6~"

#define KBD_UP_ARROW            "\x1B[A"
#define KBD_DOWN_ARROW          "\x1B[B"
#define KBD_RIGHT_ARROW         "\x1B[C"
#define KBD_LEFT_ARROW          "\x1B[D"

#define KBD_UP_ARROW2           "\x1BOA"
#define KBD_DOWN_ARROW2         "\x1BOB"
#define KBD_RIGHT_ARROW2        "\x1BOC"
#define KBD_LEFT_ARROW2         "\x1BOD"

#if defined( _MSVC_ )
#define KBD_PF1                 "\x1B"")01"
#define KBD_PF2                 "\x1B"")02"
#define KBD_PF3                 "\x1B"")03"
#define KBD_PF4                 "\x1B"")04"

#define KBD_PF5                 "\x1B"")05"
#define KBD_PF6                 "\x1B"")06"
#define KBD_PF7                 "\x1B"")07"
#define KBD_PF8                 "\x1B"")08"

#define KBD_PF9                 "\x1B"")09"
#define KBD_PF10                "\x1B"")10"
#define KBD_PF11                "\x1B"")11"
#define KBD_PF12                "\x1B"")12"

#define KBD_PF13                "\x1B"")13"
#define KBD_PF14                "\x1B"")14"
#define KBD_PF15                "\x1B"")15"
#define KBD_PF16                "\x1B"")16"

#define KBD_PF17                "\x1B"")17"
#define KBD_PF18                "\x1B"")18"
#define KBD_PF19                "\x1B"")19"
#define KBD_PF20                "\x1B"")20"

#define KBD_PF21                "\x1B"")21"
#define KBD_PF22                "\x1B"")22"
#define KBD_PF23                "\x1B"")23"
#define KBD_PF24                "\x1B"")24"

#define KBD_PF25                "\x1B"")25"
#define KBD_PF26                "\x1B"")26"
#define KBD_PF27                "\x1B"")27"
#define KBD_PF28                "\x1B"")28"

#define KBD_PF29                "\x1B"")29"
#define KBD_PF30                "\x1B"")30"
#define KBD_PF31                "\x1B"")31"
#define KBD_PF32                "\x1B"")32"

#define KBD_PF33                "\x1B"")33"
#define KBD_PF34                "\x1B"")34"
#define KBD_PF35                "\x1B"")35"
#define KBD_PF36                "\x1B"")36"

#define KBD_PF37                "\x1B"")37"
#define KBD_PF38                "\x1B"")38"
#define KBD_PF39                "\x1B"")39"
#define KBD_PF40                "\x1B"")40"

#define KBD_PF41                "\x1B"")41"
#define KBD_PF42                "\x1B"")42"
#define KBD_PF43                "\x1B"")43"
#define KBD_PF44                "\x1B"")44"

#define KBD_PF45                "\x1B"")45"
#define KBD_PF46                "\x1B"")46"
#define KBD_PF47                "\x1B"")47"
#define KBD_PF48                "\x1B"")48"
#else
#define KBD_PF1                 "\x1B""OP"
#define KBD_PF1_a               "\x1B""[11~"
#define KBD_PF2                 "\x1B""OQ"
#define KBD_PF2_a               "\x1B""[12~"
#define KBD_PF3                 "\x1B""OR"
#define KBD_PF3_a               "\x1B""[13~"
#define KBD_PF4                 "\x1B""OS"
#define KBD_PF4_a               "\x1B""[14~"

#define KBD_PF5                 "\x1B""[15~"
#define KBD_PF6                 "\x1B""[17~"
#define KBD_PF7                 "\x1B""[18~"
#define KBD_PF8                 "\x1B""[19~"

#define KBD_PF9                 "\x1B""[20~"
#define KBD_PF10                "\x1B""[21~"
#define KBD_PF11                "\x1B""[23~"
#define KBD_PF12                "\x1B""[24~"

#define KBD_PF13                "\x1B""[25~"
#define KBD_PF14                "\x1B""[26~"
#define KBD_PF15                "\x1B""[28~"
#define KBD_PF16                "\x1B""[29~"

#define KBD_PF17                "\x1B""[31~"
#define KBD_PF18                "\x1B""[32~"
#define KBD_PF19                "\x1B""[33~"
#define KBD_PF20                "\x1B""[34~"

#endif

// Does anyone know what the actual escape sequence that
// gets generated actually is on Linux for "Alt+UpArrow"
// and "Alt+DownArrow", etc?? Thanks! -- Fish

#define KBD_ALT_UP_ARROW        KBD_UP_ARROW2
#define KBD_ALT_DOWN_ARROW      KBD_DOWN_ARROW2
#define KBD_ALT_RIGHT_ARROW     KBD_RIGHT_ARROW2
#define KBD_ALT_LEFT_ARROW      KBD_LEFT_ARROW2

#define KBD_CTRL_HOME           "\x1B""w"           // (is this right??)
#define KBD_CTRL_END            "\x1B""u"           // (is this right??)

#define KBD_CTRL_UP_ARROW       "\x1B""D"           // (is this right??)
#define KBD_CTRL_DOWN_ARROW     "\x1B""M"           // (is this right??)
//efine KBD_CTRL_RIGHT_ARROW    "???????"           // (luckily we don't need it right now)
//efine KBD_CTRL_LEFT_ARROW     "???????"           // (luckily we don't need it right now)

#define KBD_ASK_CURSOR_POS      "\x1B[6n"           // Return value is the string "\x1B[n;mR"
                                                    // returned in the keyboard buffer where
                                                    // n = decimal row, m = decimal column.

// Hercules console color codes...

#define  COLOR_BLACK           0
#define  COLOR_RED             1
#define  COLOR_GREEN           2
#define  COLOR_BLUE            3
#define  COLOR_CYAN            4
#define  COLOR_MAGENTA         5
#define  COLOR_YELLOW          6
#define  COLOR_DARK_GREY       7
#define  COLOR_LIGHT_GREY      8
#define  COLOR_LIGHT_RED       9
#define  COLOR_LIGHT_GREEN     10
#define  COLOR_LIGHT_BLUE      11
#define  COLOR_LIGHT_CYAN      12
#define  COLOR_LIGHT_MAGENTA   13
#define  COLOR_LIGHT_YELLOW    14
#define  COLOR_WHITE           15
#define  COLOR_DEFAULT_FG      16
#define  COLOR_DEFAULT_BG      17
#define  COLOR_DEFAULT_LIGHT   18

extern  int   set_screen_color ( FILE* confp, short herc_fore,  short herc_back  );

// screen positions are 1-based; row 1 == top line; col 1 == leftmost column

extern  int   set_screen_pos   ( FILE* confp, short rowY1, short colX1 );

extern  int   clear_screen     ( FILE* confp );
extern  int   erase_to_eol     ( FILE* confp );

// 'save_and_set' = 1 --> just what it says; 0 --> restore from saved value.
extern  int   set_or_reset_console_mode ( int keybrd_fd, short save_and_set );

extern  void  translate_keystroke( char kbbuf[], int* pkblen );

extern  int   console_beep( FILE* confp );
extern  int   get_console_dim( FILE* confp, int* rows, int* cols );
#ifdef OPTION_EXTCURS
extern  int   get_cursor_pos( int keybrd_fd, FILE* confp, short* row, short* col );
#endif // OPTION_EXTCURS
extern  int   set_console_cursor_shape( FILE* confp, int ins );

#if defined( _MSVC_ )
extern  int   w32_set_console_title( char* pszTitle );
#endif

#endif // _HCONSOLE_H

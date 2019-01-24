/* HCONSOLE.C   (C) Copyright "Fish" (David B. Trout), 2005-2012     */
/*              (C) Copyright TurboHercules, SAS 2010-2011           */
/*          Hercules hardware console (panel) support functions      */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#include "hercules.h"
#include "hconsole.h"

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

#if defined( _MSVC_ )

//////////////////////////////////////////////////////////////////////////////////////////
// 'save_and_set' = 1 --> just what it says; 0 --> restore from saved value.

static DWORD g_dwConsoleInputMode  = 0;     // (saved value so we can later restore it)
static DWORD g_dwConsoleOutputMode = 0;     // (saved value so we can later restore it)
static WORD  g_wDefaultAttrib      = 0;     // (saved value so we can later restore it)

static WORD default_FG_color() { return  g_wDefaultAttrib       & 0x0F; }
static WORD default_BG_color() { return (g_wDefaultAttrib >> 4) & 0x0F; }

int set_or_reset_console_mode( int keybrd_fd, short save_and_set )
{
    CONSOLE_SCREEN_BUFFER_INFO  csbi;
    HANDLE  hStdIn, hStdErr;
    DWORD   dwNewInputMode;
    DWORD   dwNewOutputMode;

    if ( ! _isatty( keybrd_fd ) )
    {
        errno = EBADF;
        return -1;
    }

    hStdIn  = (HANDLE) _get_osfhandle( keybrd_fd );
    ASSERT( hStdIn && INVALID_HANDLE_VALUE != hStdIn );

    hStdErr  = GetStdHandle( STD_ERROR_HANDLE );
    ASSERT( hStdErr && INVALID_HANDLE_VALUE != hStdErr );

    if ( save_and_set )
    {
        VERIFY( GetConsoleMode( hStdIn,  &g_dwConsoleInputMode  ) );
        VERIFY( GetConsoleMode( hStdErr, &g_dwConsoleOutputMode ) );
        VERIFY( GetConsoleScreenBufferInfo( hStdErr, &csbi ) );
        g_wDefaultAttrib = csbi.wAttributes;
        dwNewInputMode  = 0;
        dwNewOutputMode = 0;
    }
    else // (restore/reset)
    {
        VERIFY( SetConsoleTextAttribute( hStdErr, g_wDefaultAttrib ) );
        dwNewInputMode  = g_dwConsoleInputMode;
        dwNewOutputMode = g_dwConsoleOutputMode;
    }

    VERIFY( SetConsoleMode( hStdIn,  dwNewInputMode  ) );
    VERIFY( SetConsoleMode( hStdErr, dwNewOutputMode ) );

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Translate Herc color to Win32 color...

#define  W32_FOREGROUND_COLOR( w32_color )    ( ( w32_color )      )
#define  W32_BACKGROUND_COLOR( w32_color )    ( ( w32_color ) << 4 )

#define  W32_COLOR_BLACK           ( 0 )
#define  W32_COLOR_RED             ( FOREGROUND_RED   )
#define  W32_COLOR_GREEN           ( FOREGROUND_GREEN )
#define  W32_COLOR_BLUE            ( FOREGROUND_BLUE  )
#define  W32_COLOR_CYAN            ( FOREGROUND_GREEN | FOREGROUND_BLUE  )
#define  W32_COLOR_MAGENTA         ( FOREGROUND_RED   | FOREGROUND_BLUE  )
#define  W32_COLOR_YELLOW          ( FOREGROUND_RED   | FOREGROUND_GREEN )
#define  W32_COLOR_LIGHT_GREY      ( FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE )

#define  W32_COLOR_DARK_GREY       ( FOREGROUND_INTENSITY | W32_COLOR_BLACK     )
#define  W32_COLOR_LIGHT_RED       ( FOREGROUND_INTENSITY | W32_COLOR_RED        )
#define  W32_COLOR_LIGHT_GREEN     ( FOREGROUND_INTENSITY | W32_COLOR_GREEN      )
#define  W32_COLOR_LIGHT_BLUE      ( FOREGROUND_INTENSITY | W32_COLOR_BLUE       )
#define  W32_COLOR_LIGHT_CYAN      ( FOREGROUND_INTENSITY | W32_COLOR_CYAN       )
#define  W32_COLOR_LIGHT_MAGENTA   ( FOREGROUND_INTENSITY | W32_COLOR_MAGENTA    )
#define  W32_COLOR_LIGHT_YELLOW    ( FOREGROUND_INTENSITY | W32_COLOR_YELLOW     )
#define  W32_COLOR_WHITE           ( FOREGROUND_INTENSITY | W32_COLOR_LIGHT_GREY )

static WORD W32_COLOR( short herc_color )
{
    switch ( herc_color )
    {
        case COLOR_BLACK:         return W32_COLOR_BLACK;
        case COLOR_RED:           return W32_COLOR_RED;
        case COLOR_GREEN:         return W32_COLOR_GREEN;
        case COLOR_BLUE:          return W32_COLOR_BLUE;
        case COLOR_CYAN:          return W32_COLOR_CYAN;
        case COLOR_MAGENTA:       return W32_COLOR_MAGENTA;
        case COLOR_YELLOW:        return W32_COLOR_YELLOW;
        case COLOR_DARK_GREY:     return W32_COLOR_DARK_GREY;

        case COLOR_LIGHT_GREY:    return W32_COLOR_LIGHT_GREY;
        case COLOR_LIGHT_RED:     return W32_COLOR_LIGHT_RED;
        case COLOR_LIGHT_GREEN:   return W32_COLOR_LIGHT_GREEN;
        case COLOR_LIGHT_BLUE:    return W32_COLOR_LIGHT_BLUE;
        case COLOR_LIGHT_CYAN:    return W32_COLOR_LIGHT_CYAN;
        case COLOR_LIGHT_MAGENTA: return W32_COLOR_LIGHT_MAGENTA;
        case COLOR_LIGHT_YELLOW:  return W32_COLOR_LIGHT_YELLOW;
        case COLOR_WHITE:         return W32_COLOR_WHITE;

        case COLOR_DEFAULT_BG:    return default_BG_color();
        case COLOR_DEFAULT_FG:    return default_FG_color();
        case COLOR_DEFAULT_LIGHT: return default_FG_color() | FOREGROUND_INTENSITY;

        default:                  return default_FG_color();
    }
}

//////////////////////////////////////////////////////////////////////////////////////////

int set_screen_color ( FILE* confp, short herc_fore, short herc_back )
{
    HANDLE  hStdErr;
    WORD    wColor;
    int     cons_fd;

    if ( !confp )
    {
        errno = EINVAL;
        return -1;
    }

    if ( ! _isatty( cons_fd = fileno( confp ) ) )
    {
        errno = EBADF;
        return -1;
    }

    hStdErr = (HANDLE) _get_osfhandle( cons_fd );
    ASSERT( hStdErr && INVALID_HANDLE_VALUE != hStdErr );

    wColor =
        0
        | W32_FOREGROUND_COLOR( W32_COLOR( herc_fore ) )
        | W32_BACKGROUND_COLOR( W32_COLOR( herc_back ) )
        ;

    VERIFY( SetConsoleTextAttribute( hStdErr, wColor ) );

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// screen positions are 1-based; row 1 == top line; col 1 == leftmost column

int set_screen_pos( FILE* confp, short rowY1, short colX1 )
{
    CONSOLE_SCREEN_BUFFER_INFO  csbi;
    HANDLE  hStdErr;
    COORD   ptConsole;
    int     cons_fd;

    if ( !confp )
    {
        errno = EINVAL;
        return -1;
    }

    if ( ! _isatty( cons_fd = fileno( confp ) ) )
    {
        errno = EBADF;
        return -1;
    }

    hStdErr = (HANDLE) _get_osfhandle( cons_fd );
    ASSERT( hStdErr && INVALID_HANDLE_VALUE != hStdErr );

    VERIFY( GetConsoleScreenBufferInfo( hStdErr, &csbi ) );

    // Note: ANSI escape codes are 1-based, whereas
    // SetConsoleCursorPosition values are 0-based...

    if (0
        || colX1 < 1 || colX1 > csbi.dwSize.X
        || rowY1 < 1 || rowY1 > csbi.dwSize.Y
    )
    {
        errno = EINVAL;
        return -1;
    }

    ptConsole.X = colX1 - 1;
    ptConsole.Y = rowY1 - 1;

    VERIFY( SetConsoleCursorPosition( hStdErr, ptConsole ) );

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// (From KB article 99261)

int clear_screen( FILE* confp )
{
    CONSOLE_SCREEN_BUFFER_INFO  csbi;
    HANDLE  hStdErr;
    DWORD   dwNumCells, dwCellsWritten;
    COORD   ptConsole = { 0, 0 };
    int     cons_fd;

    if ( !confp )
    {
        errno = EINVAL;
        return -1;
    }

    if ( ! _isatty( cons_fd = fileno( confp ) ) )
    {
        errno = EBADF;
        return -1;
    }

    hStdErr = (HANDLE) _get_osfhandle( cons_fd );
    ASSERT( hStdErr && INVALID_HANDLE_VALUE != hStdErr );

    VERIFY( GetConsoleScreenBufferInfo( hStdErr, &csbi ) ); dwNumCells = csbi.dwSize.X * csbi.dwSize.Y;
    VERIFY( FillConsoleOutputCharacter( hStdErr, ' ', dwNumCells, ptConsole, &dwCellsWritten ) );
    VERIFY( GetConsoleScreenBufferInfo( hStdErr, &csbi ) );
    VERIFY( FillConsoleOutputAttribute( hStdErr, csbi.wAttributes, dwNumCells, ptConsole, &dwCellsWritten ) );
    VERIFY( SetConsoleCursorPosition  ( hStdErr, ptConsole ) );

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////

int erase_to_eol( FILE* confp )
{
    CONSOLE_SCREEN_BUFFER_INFO  csbi;
    HANDLE  hStdErr;
    DWORD   dwCellsWritten;
    COORD   ptConsole;
    int     cons_fd;

    if ( !confp )
    {
        errno = EINVAL;
        return -1;
    }

    if ( ! _isatty( cons_fd = fileno( confp ) ) )
    {
        errno = EBADF;
        return -1;
    }

    hStdErr = (HANDLE) _get_osfhandle( cons_fd );
    ASSERT( hStdErr && INVALID_HANDLE_VALUE != hStdErr );

    VERIFY( GetConsoleScreenBufferInfo( hStdErr, &csbi ) ); ptConsole = csbi.dwCursorPosition;
    VERIFY( FillConsoleOutputAttribute( hStdErr, csbi.wAttributes, csbi.dwSize.X - ptConsole.X, ptConsole, &dwCellsWritten ) );
    VERIFY( FillConsoleOutputCharacter( hStdErr,     ' ',          csbi.dwSize.X - ptConsole.X, ptConsole, &dwCellsWritten ) );

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Handles key presses that result in TWO characters being generated...

void translate_keystroke( char kbbuf[], int* pkblen )
{
    BYTE ch = kbbuf[0];         // (move char to work var)

    switch ( ch )               // Check if special key pressed...
    {
        case 0x0D:              // enter key
            kbbuf[0] = '\n';    // change to newline character
                                // fall thru to default case
        default:                // no further translation needed
            break;              // accept the keystroke as-is

        // translate special key (escape sequence)...

        case 0x00:              // 1st char of special key press
        case 0xE0:              // 1st char of special key press
        {
            BYTE orig_ch, ch2;

            if ( !kbhit() )     // if not two chars generated,
                break;          // then not special key press

            orig_ch = ch;       // save original keystroke
            ch = '\x1B';        // change it to an escape char
            ch2 = getch();      // get second keystroke of pair

            switch ( ch2 )      // generate ANSI escape sequence
            {
                case 0x47: strcpy( kbbuf, KBD_HOME            ); break;
                case 0x52: strcpy( kbbuf, KBD_INSERT          ); break;
                case 0x53: strcpy( kbbuf, KBD_DELETE          ); break;
                case 0x4F: strcpy( kbbuf, KBD_END             ); break;
                case 0x49: strcpy( kbbuf, KBD_PAGE_UP         ); break;
                case 0x51: strcpy( kbbuf, KBD_PAGE_DOWN       ); break;

                case 0x48: strcpy( kbbuf, KBD_UP_ARROW        ); break;
                case 0x50: strcpy( kbbuf, KBD_DOWN_ARROW      ); break;
                case 0x4D: strcpy( kbbuf, KBD_RIGHT_ARROW     ); break;
                case 0x4B: strcpy( kbbuf, KBD_LEFT_ARROW      ); break;

                case 0x77: strcpy( kbbuf, KBD_CTRL_HOME       ); break;
                case 0x75: strcpy( kbbuf, KBD_CTRL_END        ); break;

                case 0x8D: strcpy( kbbuf, KBD_CTRL_UP_ARROW   ); break;
                case 0x91: strcpy( kbbuf, KBD_CTRL_DOWN_ARROW ); break;

                case 0x98: strcpy( kbbuf, KBD_ALT_UP_ARROW    ); break;
                case 0xA0: strcpy( kbbuf, KBD_ALT_DOWN_ARROW  ); break;
                case 0x9D: strcpy( kbbuf, KBD_ALT_RIGHT_ARROW ); break;
                case 0x9B: strcpy( kbbuf, KBD_ALT_LEFT_ARROW  ); break;

                case 0x3b: strcpy( kbbuf, KBD_PF1             ); break;
                case 0x3c: strcpy( kbbuf, KBD_PF2             ); break;
                case 0x3d: strcpy( kbbuf, KBD_PF3             ); break;
                case 0x3e: strcpy( kbbuf, KBD_PF4             ); break;

                case 0x3f: strcpy( kbbuf, KBD_PF5             ); break;
                case 0x40: strcpy( kbbuf, KBD_PF6             ); break;
                case 0x41: strcpy( kbbuf, KBD_PF7             ); break;
                case 0x42: strcpy( kbbuf, KBD_PF8             ); break;

                case 0x43: strcpy( kbbuf, KBD_PF9             ); break;
                case 0x44: strcpy( kbbuf, KBD_PF10            ); break;
                case 0x85: strcpy( kbbuf, KBD_PF11            ); break;
                case 0x86: strcpy( kbbuf, KBD_PF12            ); break;

                case 0x54: strcpy( kbbuf, KBD_PF13            ); break;
                case 0x55: strcpy( kbbuf, KBD_PF14            ); break;
                case 0x56: strcpy( kbbuf, KBD_PF15            ); break;
                case 0x57: strcpy( kbbuf, KBD_PF16            ); break;

                case 0x58: strcpy( kbbuf, KBD_PF17            ); break;
                case 0x59: strcpy( kbbuf, KBD_PF18            ); break;
                case 0x5a: strcpy( kbbuf, KBD_PF19            ); break;
                case 0x5b: strcpy( kbbuf, KBD_PF20            ); break;

                case 0x5c: strcpy( kbbuf, KBD_PF21            ); break;
                case 0x5d: strcpy( kbbuf, KBD_PF22            ); break;
                case 0x87: strcpy( kbbuf, KBD_PF23            ); break;
                case 0x88: strcpy( kbbuf, KBD_PF24            ); break;

                case 0x5e: strcpy( kbbuf, KBD_PF25            ); break;
                case 0x5f: strcpy( kbbuf, KBD_PF26            ); break;
                case 0x60: strcpy( kbbuf, KBD_PF27            ); break;
                case 0x61: strcpy( kbbuf, KBD_PF28            ); break;

                case 0x62: strcpy( kbbuf, KBD_PF29            ); break;
                case 0x63: strcpy( kbbuf, KBD_PF30            ); break;
                case 0x64: strcpy( kbbuf, KBD_PF31            ); break;
                case 0x65: strcpy( kbbuf, KBD_PF32            ); break;

                case 0x66: strcpy( kbbuf, KBD_PF33            ); break;
                case 0x67: strcpy( kbbuf, KBD_PF34            ); break;
                case 0x89: strcpy( kbbuf, KBD_PF35            ); break;
                case 0x8a: strcpy( kbbuf, KBD_PF36            ); break;

                case 0x68: strcpy( kbbuf, KBD_PF37            ); break;
                case 0x69: strcpy( kbbuf, KBD_PF38            ); break;
                case 0x6a: strcpy( kbbuf, KBD_PF39            ); break;
                case 0x6b: strcpy( kbbuf, KBD_PF40            ); break;

                case 0x6c: strcpy( kbbuf, KBD_PF41            ); break;
                case 0x6d: strcpy( kbbuf, KBD_PF42            ); break;
                case 0x6e: strcpy( kbbuf, KBD_PF43            ); break;
                case 0x6f: strcpy( kbbuf, KBD_PF44            ); break;

                case 0x70: strcpy( kbbuf, KBD_PF45            ); break;
                case 0x71: strcpy( kbbuf, KBD_PF46            ); break;
                case 0x8b: strcpy( kbbuf, KBD_PF47            ); break;
                case 0x8c: strcpy( kbbuf, KBD_PF48            ); break;

                default:
                {
                    /* EAT IT */
                    kbbuf[0] = 0;
                    kbbuf[1] = 0;
                    kbbuf[2] = 0;
                    break;

                } // end default

            } // end switch( ch2 )

            *pkblen = (int)strlen( kbbuf );      // inform caller #of chars
            break;

        } // end case: 0x00, 0xE0

    } // end switch( ch )
}

//////////////////////////////////////////////////////////////////////////////////////////

int console_beep( FILE* confp )
{
    int cons_fd;

    if ( !confp )
    {
        errno = EINVAL;
        return -1;
    }

    if ( ! _isatty( cons_fd = fileno( confp ) ) )
    {
        errno = EBADF;
        return -1;
    }

    MessageBeep(-1);

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////

#ifdef OPTION_EXTCURS
int get_cursor_pos( int keybrd_fd, FILE* confp, short* row, short* col )
{
    CONSOLE_SCREEN_BUFFER_INFO  csbi;
    HANDLE  hStdErr;
    int     cons_fd;

    UNREFERENCED( keybrd_fd );

    if ( !confp || !row || !col )
    {
        errno = EINVAL;
        return -1;
    }

    if ( ! _isatty( cons_fd = fileno( confp ) ) )
    {
        errno = EBADF;
        return -1;
    }

    hStdErr = (HANDLE) _get_osfhandle( cons_fd );
    ASSERT( hStdErr && INVALID_HANDLE_VALUE != hStdErr );

    if ( !GetConsoleScreenBufferInfo( hStdErr, &csbi ) )
    {
        errno = EIO;
        return -1;
    }

    *row = 1 + csbi.dwCursorPosition.Y;
    *col = 1 + csbi.dwCursorPosition.X;

    return 0;
}
#endif // OPTION_EXTCURS

//////////////////////////////////////////////////////////////////////////////////////////

int  get_console_dim( FILE* confp, int* rows, int* cols )
{
    CONSOLE_SCREEN_BUFFER_INFO  csbi;
    HANDLE  hStdErr;
    int     cons_fd;

    if ( !confp || !rows || !cols )
    {
        errno = EINVAL;
        return -1;
    }

    *rows = *cols = 0;

    if ( ! _isatty( cons_fd = fileno( confp ) ) )
    {
        errno = EBADF;
        return -1;
    }

    hStdErr = (HANDLE) _get_osfhandle( cons_fd );
    ASSERT( hStdErr && INVALID_HANDLE_VALUE != hStdErr );

    if ( !GetConsoleScreenBufferInfo( hStdErr, &csbi ) )
    {
        errno = EIO;
        return -1;
    }

    *rows = 1 + csbi.srWindow.Bottom - csbi.srWindow.Top;
    *cols = 1 + csbi.srWindow.Right  - csbi.srWindow.Left;
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////

int  set_console_cursor_shape( FILE* confp, int ins )
{
    CONSOLE_CURSOR_INFO  ci;
    HANDLE  hStdErr;
    int     cons_fd;

    if ( !confp )
    {
        errno = EINVAL;
        return -1;
    }

    if ( ! _isatty( cons_fd = fileno( confp ) ) )
    {
        errno = EBADF;
        return -1;
    }

    hStdErr = (HANDLE) _get_osfhandle( cons_fd );
    ASSERT( hStdErr && INVALID_HANDLE_VALUE != hStdErr );

    ci.bVisible = TRUE;
    ci.dwSize = ins ? 20 : 100; // (note: values are percent of cell height)

    if ( !SetConsoleCursorInfo( hStdErr, &ci ) )
    {
        errno = EIO;
        return -1;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// From KB article 124103: "How To Obtain a Console Window Handle (HWND)"
// http://support.microsoft.com/?kbid=124103

#define MAX_WINDOW_TITLE_LEN  (256)   // (purely arbitrary)

static TCHAR g_szOriginalTitle[ MAX_WINDOW_TITLE_LEN ] = {0};

int w32_set_console_title( char* pszTitle )
{
    TCHAR    szNewTitleBuff [ MAX_WINDOW_TITLE_LEN ];
    LPCTSTR  pszNewTitle = NULL;

    if (!g_szOriginalTitle[0])
        VERIFY(GetConsoleTitle( g_szOriginalTitle, MAX_WINDOW_TITLE_LEN ));

    if (pszTitle)
    {
        _sntprintf( szNewTitleBuff, MAX_WINDOW_TITLE_LEN-1, _T("%hs"), pszTitle );
        szNewTitleBuff[MAX_WINDOW_TITLE_LEN-1]=0;
        pszNewTitle = szNewTitleBuff;
    }
    else
        pszNewTitle = g_szOriginalTitle;

    if (!SetConsoleTitle( pszNewTitle ))
    {
        errno = GetLastError();
        return -1;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

#else // !defined( WIN32 )

#ifdef HAVE_TERMIOS_H
static struct termios saved_kbattr;  // (saved value so we can later restore it)
#endif

// 'save_and_set' = 1 --> just what it says; 0 --> restore from saved value.

int set_or_reset_console_mode( int keybrd_fd, short save_and_set )
{
#ifdef HAVE_TERMIOS_H
struct termios kbattr;

    if ( save_and_set )
    {
        // Put the terminal into cbreak mode

        tcgetattr( keybrd_fd, &saved_kbattr );      // (save for later restore)

        kbattr = saved_kbattr;                      // (make copy)

        kbattr.c_lflag     &=  ~(ECHO | ICANON);    // (set desired values...)
        kbattr.c_cc[VMIN]   =         0;
        kbattr.c_cc[VTIME]  =         0;

        tcsetattr( keybrd_fd, TCSANOW, &kbattr );
    }
    else
    {
        // Restore the terminal mode

        tcsetattr( STDIN_FILENO, TCSANOW, &saved_kbattr );  // (restore prev value)
    }

#else

    UNREFERENCED( keybrd_fd );
    UNREFERENCED( save_and_set );

#endif

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// screen positions are 1-based; row 1 == top line; col 1 == leftmost column

int set_screen_pos( FILE* confp, short rowY1, short colX1 )
{
#define                      ANSI_POSITION_CURSOR    "\x1B[%d;%dH"

    return ( fprintf( confp, ANSI_POSITION_CURSOR, rowY1, colX1 ) ? 0 : -1 );
}

//////////////////////////////////////////////////////////////////////////////////////////

int erase_to_eol( FILE* confp )
{
#define                      ANSI_ERASE_EOL    "\x1B[K"

    return ( fprintf( confp, ANSI_ERASE_EOL ) ? 0 : -1 );
}

//////////////////////////////////////////////////////////////////////////////////////////

int clear_screen( FILE* confp )
{
#define                      ANSI_ERASE_SCREEN    "\x1B[2J"

    return ( fprintf( confp, ANSI_ERASE_SCREEN ) ? 0 : -1 );
}

//////////////////////////////////////////////////////////////////////////////////////////
/*
From:  (http://www.vtt.fi/tte/EuroBridge/Xew/iso6429.html)

CSI (Control Sequence Introducer) = "\033[" or "\233"

SGR (Select Graphic Rendition) = CSI Ps1;Ps2;... m

SGR (Select Graphic Rendition):

        CSI Ps ... 0x6d

        (Note: 0x6d = ASCII 'm')

    SGR is used to establish one or more graphic rendition aspects for
    subsequent text. The established aspects remain in effect until the
    next occurrence of SGR in the data stream, depending on the setting
    of the GRAPHIC RENDITION COMBINATION MODE (GRCM).

    The implementation assumes the GRCM = CUMULATIVE (that is, each aspect
    will be in effect until explicitly cancelled by another SGR).

    --------------------------------------------------------------------------
    Ps     Description
    --------------------------------------------------------------------------
    0      default rendition; cancel all preceding occurrences of SGR;
           invoke primary font.
    1      bold or increased intensity

    2      faint   <------<<<    Fish: non-standard, but apparently anyway
                                       (based on other documents I've seen)

    3      italicized
    4      underlined
    7      negative image
    9      crossed out
    10     primary (default) font
    11-19  first to ninth alternative fonts (see also fonts)
    21     doubly underlined
    22     normal intensity (neither bold nor faint)
    23     not italicized
    24     not underlined (neither singly or doubly)
    27     positive image
    29     not crossed out
    30-37  foreground color of the text; 30=black, 31=red, 32=green,
           33=yellow, 34=blue, 35=magenta, 36=cyan, 37=white.
    39     default foreground text color (foreground color of the widget)
    40-47  background color of the text; 40=black, 41=red, 42=green,
           43=yellow, 44=blue, 45=magenta, 46=cyan, 47=white.
    49     default background text color (background color of the widget)
    51     framed (see FrameRendition)
    53     overlined [not implemented yet]
    54     not framed, not encircled
    55     not overlined [not implemented yet]
    --------------------------------------------------------------------------


    "\033[m"    Reset
    "\033[0m"   Reset
    "\033[1m"   Bold

    "\033[2m"   Faint  <------<<<   Fish: non-standard, but apparently anyway
                                          (based on other documents I've seen)

    "\033[3m"   Italic
    "\033[4m"   Underline
    "\033[7m"   Inverse
    "\033[9m"   Crossed out
    "\033[10m"  primary font
    "\033[11m"  1st alternate font
    "\033[12m"  2nd alternate font
    "\033[13m"  3rd alternate font
    "\033[14m"  4th alternate font
    "\033[15m"  5th alternate font
    "\033[16m"  6th alternate font
    "\033[17m"  7th alternate font
    "\033[18m"  8th alternate font
    "\033[19m"  9th alternate font
    "\033[21m"  Double underline
    "\033[22m"  Bold off
    "\033[23m"  Italic off
    "\033[24m"  Underline off (double or single)
    "\033[27m"  Inverse off
    "\033[29m"  Crossed out off

    "\033[30m"  Black foreground
    "\033[31m"  Red foreground
    "\033[32m"  Green foreground
    "\033[33m"  Yellow foreground
    "\033[34m"  Blue foreground
    "\033[35m"  Magenta foreground
    "\033[36m"  Cyan foreground
    "\033[37m"  White foreground
    "\033[39m"  Default foreground

    "\033[40m"  Black background
    "\033[41m"  Red background
    "\033[42m"  Green background
    "\033[43m"  Yellow background
    "\033[44m"  Blue background
    "\033[45m"  Magenta background
    "\033[46m"  Cyan background
    "\033[47m"  White background
    "\033[49m"  Default background

    "\033[51m"  Framed on
    "\033[54m"  Framed off
*/
//////////////////////////////////////////////////////////////////////////////////////////
// Translate Herc color to ANSI/ISO-6429 (ECMA-48) color...
// High-order byte is attribute (0=normal, 1=bold), low-order byte is color.

#define  ISO_COLOR_BLACK     ( 30 )
#define  ISO_COLOR_RED       ( 31 )
#define  ISO_COLOR_GREEN     ( 32 )
#define  ISO_COLOR_YELLOW    ( 33 )
#define  ISO_COLOR_BLUE      ( 34 )
#define  ISO_COLOR_MAGENTA   ( 35 )
#define  ISO_COLOR_CYAN      ( 36 )
#define  ISO_COLOR_WHITE     ( 37 )
#define  ISO_COLOR_DEFAULT   ( 39 )

#define  ISO_NORMAL( iso_color )    ( ( 0x0000 ) | ( (uint16_t)( iso_color ) ) )
#define  ISO_BRIGHT( iso_color )    ( ( 0x0100 ) | ( (uint16_t)( iso_color ) ) )

#define  ISO_IS_ISO_BRIGHT( iso_color )       ( ( ( iso_color ) >> 8 ) & 0x01 )

// PROGRAMMING NOTE: the '2' (faint/dim) and '22' (bold-off)
// attribute codes are UNRELIABLE. They are apparently rarely
// implemented properly on many platforms (Cygwin included).
// Thus we prefer to use the more reliable (but programmatically
// bothersome) '0' (reset) attribute instead [whenever we wish
// to paint a 'dim/faint' (non-bold) color]. As a result however,
// we need to be careful to paint the foregoround/background
// colors in the proper sequence/order and in the proper manner.
// See the below 'set_screen_color' function for details. (Fish)

//#define  ISO_NORMAL_OR_BRIGHT( iso_color )    ISO_IS_ISO_BRIGHT( iso_color ) ? 1 : 22
//#define  ISO_NORMAL_OR_BRIGHT( iso_color )    ISO_IS_ISO_BRIGHT( iso_color ) ? 1 : 2
#define  ISO_NORMAL_OR_BRIGHT( iso_color )    ISO_IS_ISO_BRIGHT( iso_color ) ? 1 : 0

#define  ISO_FOREGROUND_COLOR( iso_color )    ( ( ( iso_color ) & 0x00FF )      )
#define  ISO_BACKGROUND_COLOR( iso_color )    ( ( ( iso_color ) & 0x00FF ) + 10 )

static uint16_t ISO_COLOR( short herc_color )
{
    switch ( herc_color )
    {
        case COLOR_BLACK:         return ISO_NORMAL( ISO_COLOR_BLACK   );
        case COLOR_RED:           return ISO_NORMAL( ISO_COLOR_RED     );
        case COLOR_GREEN:         return ISO_NORMAL( ISO_COLOR_GREEN   );
        case COLOR_BLUE:          return ISO_NORMAL( ISO_COLOR_BLUE    );
        case COLOR_CYAN:          return ISO_NORMAL( ISO_COLOR_CYAN    );
        case COLOR_MAGENTA:       return ISO_NORMAL( ISO_COLOR_MAGENTA );
        case COLOR_YELLOW:        return ISO_NORMAL( ISO_COLOR_YELLOW  );
        case COLOR_DARK_GREY:     return ISO_BRIGHT( ISO_COLOR_BLACK   );

        case COLOR_LIGHT_GREY:    return ISO_NORMAL( ISO_COLOR_WHITE   );
        case COLOR_LIGHT_RED:     return ISO_BRIGHT( ISO_COLOR_RED     );
        case COLOR_LIGHT_GREEN:   return ISO_BRIGHT( ISO_COLOR_GREEN   );
        case COLOR_LIGHT_BLUE:    return ISO_BRIGHT( ISO_COLOR_BLUE    );
        case COLOR_LIGHT_CYAN:    return ISO_BRIGHT( ISO_COLOR_CYAN    );
        case COLOR_LIGHT_MAGENTA: return ISO_BRIGHT( ISO_COLOR_MAGENTA );
        case COLOR_LIGHT_YELLOW:  return ISO_BRIGHT( ISO_COLOR_YELLOW  );
        case COLOR_WHITE:         return ISO_BRIGHT( ISO_COLOR_WHITE   );

        case COLOR_DEFAULT_FG:    return ISO_NORMAL( ISO_COLOR_DEFAULT );
        case COLOR_DEFAULT_BG:    return ISO_NORMAL( ISO_COLOR_DEFAULT );
        case COLOR_DEFAULT_LIGHT: return ISO_BRIGHT( ISO_COLOR_DEFAULT );

        default:                  return ISO_NORMAL( ISO_COLOR_DEFAULT );
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// Translate Herc color to ANSI/ISO-6429 (ECMA-48) SGR terminal escape sequence...

int set_screen_color( FILE* confp, short herc_fore, short herc_back )
{
    uint16_t iso_fore, iso_back;
    uint16_t  iso_bold_color, iso_dim_color;
    int rc;

    // Translate Herc color to ANSI (ISO) color...

    iso_fore = ISO_COLOR( herc_fore );
    iso_back = ISO_COLOR( herc_back );

    // PROGRAMMING NOTE: Because the only means we have to RELIABLY
    // set non-bold (faint/dim) color attributes across ALL platforms
    // is to use the '0' (reset) escape code (which of course has the
    // unfortunate(?) side-effect of resetting BOTH the background
    // AND foreground colors to dim/faint instead of just one or the
    // other), we need to be careful to always set the dim (NON-bold)
    // color attribute FIRST (which will of course reset both the fore-
    // ground AND the backgound colors to non-bold/faint/dim as well),
    // and then to, AFTERWARDS, set the BOLD color attribute. This is
    // the ONLY way I've been able to discover (empirically via trial
    // and error) how to RELIABLY set bold/faint foreground/background
    // color attributes across all(?) supported platforms. (Fish)

    if ( ISO_IS_ISO_BRIGHT(iso_fore) == ISO_IS_ISO_BRIGHT(iso_back) )
    {
        // BOTH the foreground color AND the background colors
        // are either BOTH bold or BOTH dim/faint (normal)...

        rc = fprintf
        (
            confp,

            // Set the bold/dim attribute FIRST and then
            // BOTH foreground/background colors afterwards...

            "\x1B[%d;%d;%dm"

            ,ISO_NORMAL_OR_BRIGHT( iso_back )
            ,ISO_BACKGROUND_COLOR( iso_back )
            ,ISO_FOREGROUND_COLOR( iso_fore )
        );
    }
    else // ( ISO_IS_ISO_BRIGHT(iso_fore) != ISO_IS_ISO_BRIGHT(iso_back) )
    {
        // ONE of either the foreground OR background colors
        // is bold, but the OTHER one is dim/faint (normal)...

        if ( ISO_IS_ISO_BRIGHT(iso_fore) )
        {
            // The foregound color is the bright/bold one...

            iso_bold_color = ISO_FOREGROUND_COLOR( iso_fore );
            iso_dim_color  = ISO_BACKGROUND_COLOR( iso_back );
        }
        else // ( !ISO_IS_ISO_BRIGHT(iso_fore) )
        {
            // The background color is the bright/bold one...

            iso_bold_color = ISO_BACKGROUND_COLOR( iso_back );
            iso_dim_color  = ISO_FOREGROUND_COLOR( iso_fore );
        }

        // Set whichever is the DIM color attribute FIRST
        // and then AFTERWARDS whichever one is the BOLD...

        rc = fprintf
        (
            confp,

            "\x1B[0;%d;1;%dm" // (reset, dim-color, bold, bold-color)

            ,iso_dim_color
            ,iso_bold_color
        );
    }

    return rc < 0 ? -1 : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////

void translate_keystroke( char kbbuf[], int* pkblen )
{
    UNREFERENCED( kbbuf );
    UNREFERENCED( pkblen );
    return;
}

//////////////////////////////////////////////////////////////////////////////////////////

int console_beep( FILE* confp )
{
    return fprintf( confp, "\a" ) < 0 ? -1 : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////

int  get_console_dim( FILE* confp, int* rows, int* cols )
{
    const char* sym;

#if defined(TIOCGWINSZ)
    struct winsize winsize;
#else
    UNREFERENCED( confp );
#endif

    if ( !rows || !cols )
    {
        errno = EINVAL;
        return -1;
    }

#if defined(TIOCGWINSZ)
    if (ioctl(fileno(confp), TIOCGWINSZ, &winsize) >= 0)
    {
        *rows = winsize.ws_row;
        *cols = winsize.ws_col;
    }
    else
#endif
    {
        if (!(sym = get_symbol( "LINES"   )) || !*sym) *rows = DEFAULT_CONSOLE_ROWS;
        else                                           *rows = atoi( sym );
        if (!(sym = get_symbol( "COLUMNS" )) || !*sym) *cols = DEFAULT_CONSOLE_COLS;
        else                                           *cols = atoi( sym );
    }

    if (!*rows || !*cols)
    {
        errno = EIO;
        return -1;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////

#ifdef OPTION_EXTCURS
int get_cursor_pos( int keybrd_fd, FILE* confp, short* row, short* col )
{
    struct  timeval tv;                 /* Select timeout structure  */
    fd_set  readset;                    /* Select file descriptors   */
    char    kbbuf[16];                  /* Keyboard i/p buffer       */
    char*   semi;                       /* Index of semicolon        */
    int     kblen;                      /* Number of chars in kbbuf  */
    int     maxfd;                      /* Highest file descriptor   */
    int     rc;                         /* Return code               */
    char    c;                          /* Work for scanf            */

    /* Request the CPR (Cursor Position Report) */
    if ( fprintf( confp, KBD_ASK_CURSOR_POS ) < 0 )
        return -1;

    /* Read the CPR from the keyboard i/p buffer */
    while (1)
    {
        FD_ZERO (&readset);
        FD_SET (keybrd_fd, &readset);
        maxfd = keybrd_fd;
        tv.tv_sec  = 0;
        tv.tv_usec = 50 * 1000; // (PLENTY long enough!)

        /* Wait for CPR to arrive in our i/p buffer */
        rc = select (maxfd + 1, &readset, NULL, NULL, &tv);

        if (rc < 0 )
        {
            if (errno == EINTR) continue;
            errno = EIO;
            break;
        }

        /* If keyboard input has arrived then process it */
        if (!FD_ISSET(keybrd_fd, &readset))
            continue;

        /* Read character(s) from the keyboard */
        kblen = read (keybrd_fd, kbbuf, sizeof(kbbuf)-1);

        if (kblen < 0)
        {
            errno = EIO;
            break;
        }

        kbbuf[kblen] = 0;

        // The returned CPR is the string "\x1B[n;mR"
        // where n = decimal row, m = decimal column.

        // Note: we expect the entire the CPR to have
        // been read on our first i/o (i.e. for it to
        // have arrived all at once in one piece) and
        // not piecemeal requiring several i/o's...
        if (0
            || kblen < 6
            || kbbuf[   0   ] != '\x1B'
            || kbbuf[   1   ] != '['
            || kbbuf[kblen-1] != 'R'
            || (semi = memchr( kbbuf, ';', kblen )) == NULL
            || (semi - kbbuf) < 3
            || sscanf( &kbbuf[2], "%hu%c", row, &c ) != 2 || c != ';'
            || sscanf( semi+1,    "%hu%c", col, &c ) != 2 || c != 'R'
        )
        {
            errno = EIO;
            rc = -1;
            break;
        }

        /* Success! */
        rc = 0;
        break
    }

    return rc;
}
#endif // OPTION_EXTCURS

//////////////////////////////////////////////////////////////////////////////////////////
/*
From: (http://groups-beta.google.com/group/comp.protocols.kermit.misc/msg/1cc3ec6f0bfc0084)

VGA-softcursor.txt, from the 2.2 kernel

Software cursor for VGA    by Pavel Machek <p...@atrey.karlin.mff.cuni.cz>
=======================    and Martin Mares <m...@atrey.karlin.mff.cuni.cz>

   Linux now has some ability to manipulate cursor appearance. Normally, you
can set the size of hardware cursor (and also work around some ugly bugs in
those miserable Trident cards--see #define TRIDENT_GLITCH in drivers/video/
vgacon.c). You can now play a few new tricks:  you can make your cursor look
like a non-blinking red block, make it inverse background of the character it's
over or to highlight that character and still choose whether the original
hardware cursor should remain visible or not.  There may be other things I have
never thought of.

   The cursor appearance is controlled by a "<ESC>[?1;2;3c" escape sequence
where 1, 2 and 3 are parameters described below. If you omit any of them,
they will default to zeroes.

   Parameter 1 specifies cursor size (0=default, 1=invisible, 2=underline, ...,
8=full block) + 16 if you want the software cursor to be applied + 32 if you
want to always change the background color + 64 if you dislike having the
background the same as the foreground.  Highlights are ignored for the last two
flags.

   The second parameter selects character attribute bits you want to change
(by simply XORing them with the value of this parameter). On standard VGA,
the high four bits specify background and the low four the foreground. In both
groups, low three bits set color (as in normal color codes used by the console)
and the most significant one turns on highlight (or sometimes blinking--it
depends on the configuration of your VGA).

   The third parameter consists of character attribute bits you want to set.
Bit setting takes place before bit toggling, so you can simply clear a bit by
including it in both the set mask and the toggle mask.

Examples:
=========

To get normal blinking underline, use: echo -e '\033[?2c'
To get blinking block, use:            echo -e '\033[?6c'
To get red non-blinking block, use:    echo -e '\033[?17;0;64c'
*/

int  set_console_cursor_shape( FILE* confp, int ins )
{
#if SET_CONSOLE_CURSOR_SHAPE_METHOD == CURSOR_SHAPE_NOT_SUPPORTED

    UNREFERENCED( confp );
    UNREFERENCED( ins );
    return 0;

#elif SET_CONSOLE_CURSOR_SHAPE_METHOD == CURSOR_SHAPE_VIA_SPECIAL_LINUX_ESCAPE

#define  LINUX_UNDER_BLINK_CURSOR    "\x1B[?2c"
#define  LINUX_BLINK_BLOCK_CURSOR    "\x1B[?6c"

    return fprintf( confp, ins ? LINUX_UNDER_BLINK_CURSOR : LINUX_BLINK_BLOCK_CURSOR );

#else
    #error Invalid #defined SET_CONSOLE_CURSOR_SHAPE_METHOD value
    return -1;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////

#endif // defined( WIN32 )

//////////////////////////////////////////////////////////////////////////////////////////

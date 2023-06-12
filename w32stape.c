/* W32STAPE.C   (C) Copyright "Fish" (David B. Trout), 2005-2012     */
/*               Hercules Win32 SCSI Tape handling module            */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

////////////////////////////////////////////////////////////////////////////////////
//
//  This module contains only WIN32 support for SCSI tapes.
//  Primary SCSI Tape support is in module 'scsitape.c'...
//
//  PROGRAMMING NOTE: we rely on the known fact that
//  'NO_ERROR' == 0 and 'INVALID_HANDLE_VALUE' == -1.
//
////////////////////////////////////////////////////////////////////////////////////

#include "hstdinc.h"

#define _W32STAPE_C_
#define _HTAPE_DLL_

#include "hercules.h"
#include "w32stape.h"
#include "tapedev.h"        // (need IS_TAPE_BLKID_BOT)

#ifdef _MSVC_

////////////////////////////////////////////////////////////////////////////////////
// Global data...

#define  W32STAPE_MAX_FDNUMS  (32)      // (admitedly low, but easily increased)

typedef int  ifd_t;     // (internal fd)
typedef int  ufd_t;     // (user fd)

#define  W32STAPE_IFD2UFD( ifd )         ((ufd_t)( (ifd) |  0x7F000000 ))
#define  W32STAPE_UFD2IFD( ufd )         ((ifd_t)( (ufd) & ~0x7F000000 ))

static  BYTE    g_ifds    [ W32STAPE_MAX_FDNUMS ]  = {0};   // (0 == avail, 0xFF == used)
static  HANDLE  g_handles [ W32STAPE_MAX_FDNUMS ]  = {0};   // (WIN32 handles)
static  char*   g_fnames  [ W32STAPE_MAX_FDNUMS ]  = {0};   // (for posterity)
static  U32     g_fstats  [ W32STAPE_MAX_FDNUMS ]  = {0};   // (running status)
static  U32     g_BOTmsk  [ W32STAPE_MAX_FDNUMS ]  = {0};   // (BOT block-id mask)
static  U32     g_BOTbot  [ W32STAPE_MAX_FDNUMS ]  = {0};   // (BOT block-id value)

static  TAPE_GET_DRIVE_PARAMETERS g_drive_parms [ W32STAPE_MAX_FDNUMS ] = {0};  // (drive parameters)

static  LOCK    g_lock; // (master global access lock)

#define  lock()     obtain_w32stape_lock()
#define  unlock()   release_lock( &g_lock )

static void obtain_w32stape_lock()
{
    static int bDidInit  = 0;
    static int bInitBusy = 1;
    if (!bDidInit)
    {
        bDidInit = 1;
        initialize_lock ( &g_lock );
        memset( g_ifds,    0,    sizeof ( g_ifds    ) );
        memset( g_handles, 0,    sizeof ( g_handles ) );
        memset( g_fnames,  0,    sizeof ( g_fnames  ) );
        memset( g_fstats,  0,    sizeof ( g_fstats  ) );
        memset( g_BOTmsk,  0xFF, sizeof ( g_BOTmsk  ) );
        memset( g_BOTbot,  0,    sizeof ( g_BOTbot  ) );
        bInitBusy = 0;
    }
    while (bInitBusy) Sleep(10);
    obtain_lock ( &g_lock );
}

////////////////////////////////////////////////////////////////////////////////////
// Allocate an internal fd number...

static
ifd_t  w32_alloc_ifd()
{
    ifd_t ifd = -1;
    errno = EMFILE;

    lock();
    {
        BYTE* pifd_slot = memchr( g_ifds, 0, W32STAPE_MAX_FDNUMS );

        if (pifd_slot)
        {
            int n = (int) (pifd_slot - g_ifds);

            if ( n >= 0  &&  n < W32STAPE_MAX_FDNUMS )
            {
                *pifd_slot = 1;
                errno = 0;
                ifd = n;
            }
        }
    }
    unlock();

    return ifd;
}

////////////////////////////////////////////////////////////////////////////////////
// Release an internal fd number...

static
int w32_free_ifd_nolock( ifd_t  ifd )
{
    int rc = 0;
    errno = 0;

    if ( ifd >= 0  &&  ifd < W32STAPE_MAX_FDNUMS )
        g_ifds [ ifd ] = 0;
    else
    {
        rc = -1;
        errno = EBADF;
    }

    return rc;
}

////////////////////////////////////////////////////////////////////////////////////
// Release an internal fd number...

static
int w32_free_ifd( ifd_t  ifd )
{
    int rc = 0;
    errno = 0;

    lock();
    {
        rc = w32_free_ifd_nolock( ifd );
    }
    unlock();

    return rc;
}

////////////////////////////////////////////////////////////////////////////////////
// Retrieve the status of the tape drive...

static
DWORD  w32_get_tape_status ( HANDLE hFile )
{
    // ***************************************************************
    //  PROGRAMMING NOTE: it is THIS LOOP (retrieving the status
    //  of the tape drive) that takes UP TO *10* SECONDS TO COMPLETE
    //  if there is no tape mounted on the drive whereas it completes
    //  immediately when there IS a tape mounted!  I have no idea why
    //  Windows behave so unusually/inefficiently in this way! - Fish
    // ***************************************************************

    DWORD dwTapeStatus;

    // (NOTE: see also: KB 111837: "ERROR_BUS_RESET May Be Benign")

    do dwTapeStatus = GetTapeStatus( hFile );
    while (ERROR_BUS_RESET == dwTapeStatus);

    return dwTapeStatus;
}

////////////////////////////////////////////////////////////////////////////////////
// Open tape device...

DLL_EXPORT
ufd_t w32_open_tape ( const char* path, int oflag, ... )
{
    ifd_t       ifd;
    HANDLE      hFile;
    char        szTapeDeviceName[10];
    const char* pszTapeDevNum;
    DWORD       dwDesiredAccess, dwSizeofDriveParms, dwRetCode;

    // Reserve an fd number right away and bail if none available...
    if ( (ifd = w32_alloc_ifd()) < 0 )
        return -1;

    // If they specified a Windows device name,
    // use it as-is.

    if (1
        && strnfilenamecmp( path, "\\\\.\\", 4 ) == 0
        &&                  path            [4]  != 0
    )
    {
        STRLCPY( szTapeDeviceName, path );
    }
    else // (not a Windows device name)
    {
        // The device name is a Cygwin/*nix device name.
        // Name must be either "/dev/nst0" or "/dev/st0"

        if (1
            &&  strnfilenamecmp( path, "/dev/", 5 ) == 0
            &&  (
                    strnfilenamecmp( (pszTapeDevNum=path+8)-3, "nst", 3 ) == 0
                    ||
                    strnfilenamecmp( (pszTapeDevNum=path+7)-2, "st",  2 ) == 0
                )
            &&  strlen(pszTapeDevNum) == 1
            &&  isdigit((unsigned char)*pszTapeDevNum)
        )
        {
            // Change it to a Windows device name (e.g. \\.\Tape0)

            STRLCPY( szTapeDeviceName, WIN32_TAPE_DEVICE_NAME );
            szTapeDeviceName[8] = *pszTapeDevNum;
            szTapeDeviceName[9] = 0;

            // PROGRAMMING NOTE: the "rewind at close" option (implied by
            // virtue of the filename being "/dev/st0" and not "/dev/nst0")
            // was handled (detected/remembered) by the higher-level caller.
        }
        else
        {
            VERIFY( w32_free_ifd( ifd ) == 0 );
            errno = EINVAL;     // (bad device name)
            return -1;          // (open failure)
        }
    }

    // We only support O_BINARY with either O_RDWR or O_RDONLY

    if (1
        && (( O_BINARY | O_RDWR  ) != oflag)
        && (( O_BINARY | O_RDONLY) != oflag)
    )
    {
        VERIFY( w32_free_ifd( ifd ) == 0 );
        errno = EINVAL;     // (invalid open flags)
        return -1;          // (open failure)
    }

    // Set desired access

    dwDesiredAccess = GENERIC_READ;

    if ( oflag & O_RDWR )
        dwDesiredAccess |= GENERIC_WRITE;

    // Open the tape drive...

    hFile = CreateFile
    (
        szTapeDeviceName,   // filename
        dwDesiredAccess,    // desired access
        0,                  // share mode (0 == exclusive)
        NULL,               // security == default
        OPEN_EXISTING,      // "file" (device actually) must already exist
        0,                  // no special access flags needed
        NULL                // not using template
    );

    if ( INVALID_HANDLE_VALUE == hFile )
    {
        int save_errno = w32_trans_w32error( GetLastError() );
        VERIFY( w32_free_ifd( ifd ) == 0 );
        errno = save_errno;
        return -1;
    }

    // Save drive parameters for later...

    memset( &g_drive_parms[ifd], 0, sizeof(TAPE_GET_DRIVE_PARAMETERS) );
    dwSizeofDriveParms = sizeof(TAPE_GET_DRIVE_PARAMETERS);

    do
    {
        dwRetCode = GetTapeParameters
        (
            hFile,
            GET_TAPE_DRIVE_INFORMATION,
            &dwSizeofDriveParms,
            &g_drive_parms[ifd]
        );
    }
    while ((NO_ERROR != dwRetCode)              // (if not normal completion,
    &&                                          // check for retry conditions)
    (0
        || ERROR_MEDIA_CHANGED == dwRetCode     // (likely but unimportant; retry)
        || ERROR_BUS_RESET     == dwRetCode     // (unlikely but possible;  retry)
    ));

    // Did that work?

    if (NO_ERROR != dwRetCode)
    {
        int save_errno = w32_trans_w32error( GetLastError() );
        CloseHandle( hFile );
        VERIFY( w32_free_ifd( ifd ) == 0 );
        errno = save_errno;
        return -1;
    }

    ASSERT( NO_ERROR == dwRetCode );
    ASSERT( sizeof(TAPE_GET_DRIVE_PARAMETERS) == dwSizeofDriveParms );

    // Save control info & return their file descriptor...

    g_handles [ ifd ]  = hFile;                     // (WIN32 handle)
    g_fnames  [ ifd ]  = strdup( path );            // (for posterity)
    g_fstats  [ ifd ]  = GMT_ONLINE (0xFFFFFFFF);   // (initial status)
    g_BOTmsk  [ ifd ]  = 0xFFFFFFFF;                // (BOT block-id mask)
    g_BOTbot  [ ifd ]  = 0x00000000;                // (BOT block-id value)

    return W32STAPE_IFD2UFD( ifd );                 // (user fd result)
}

////////////////////////////////////////////////////////////////////////////////////
// Define physical BOT block-id mask / value...

// PROGRAMMING NOTE: For the time being, we require 'tapedev.c' to provide to us
// the information we need in order to detect physical BOT (load-point). This is
// only until such time as I can add SCSI PassThru support to Hercules so that we
// can talk SCSI directly to the device ourselves (to determine such things as
// what type of device (manufacturer/model) we're dealing with, etc).

DLL_EXPORT
int w32_define_BOT ( ufd_t ufd, U32 msk, U32 bot )
{
    ifd_t  ifd  = W32STAPE_UFD2IFD( ufd );

    lock();

    if (0
        ||         ifd    <   0
        ||         ifd    >=  W32STAPE_MAX_FDNUMS
        || g_ifds[ ifd ]  ==  0
    )
    {
        unlock();
        errno = EBADF;
        return -1;
    }

    g_BOTmsk [ ifd ]  = msk;   // (BOT block-id mask)
    g_BOTbot [ ifd ]  = bot;   // (BOT block-id value)

    unlock();

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////
// Post-process a tape i/o return code...
//
// Examine 'errno' (which should have been manually set to the return
// code from the current i/o) and update the internal status appropriately,
// depending on what type of error it was (tapemark, etc)...
//
//     -------------------------------------------------------------
//     ***  THIS FUNCTION SHOULD BE CALLED AFTER EVERY TAPE I/O  ***
//     -------------------------------------------------------------
//
// An errno of 'EINTR' means the error was spurious (media changed, etc)
// and that the caller should try the same i/o again (retry their i/o).
//
// EXAMPLE:
//
//     do
//     {
//         errno = SetTapePosition( ... );
//         errno = w32_internal_rc ( pStat );
//     }
//     while ( EINTR == errno );
//     return errno ? -1 : 0;
//
////////////////////////////////////////////////////////////////////////////////////

//     ***  THIS FUNCTION SHOULD BE CALLED AFTER EVERY TAPE I/O  ***

static
int w32_internal_rc ( U32* pStat )
{
    ASSERT( pStat );    // (sanity check)

    // PROGRAMMING NOTE: the 'door open' (no tape in drive) and the
    // 'write protected' statuses are "sticky" in that they never change
    // until a new/different tape is mounted. All the other statuses
    // however, change dynamically as one does i/o to the tape...

    if (0
        || ERROR_BUS_RESET            == errno // (See KB 111837: "ERROR_BUS_RESET May Be Benign")
        || ERROR_MEDIA_CHANGED        == errno
        || ERROR_DEVICE_NOT_CONNECTED == errno // (shouldn't occur but we'll check anyway)
        || ERROR_DEV_NOT_EXIST        == errno // (shouldn't occur but we'll check anyway)
        || ERROR_FILE_NOT_FOUND       == errno // (shouldn't occur but we'll check anyway)
    )
    {
        *pStat  &=  ~GMT_DR_OPEN (0xFFFFFFFF);
        *pStat  &=  ~GMT_WR_PROT (0xFFFFFFFF);
    }

    // (see PROGRAMMING NOTE above)

    *pStat  &=  ~GMT_BOT (0xFFFFFFFF);
    *pStat  &=  ~GMT_SM  (0xFFFFFFFF);
    *pStat  &=  ~GMT_EOF (0xFFFFFFFF);
    *pStat  &=  ~GMT_EOT (0xFFFFFFFF);
    *pStat  &=  ~GMT_EOD (0xFFFFFFFF);

    if (0
        || ERROR_BUS_RESET            == errno // (spurious error; retry)
        || ERROR_MEDIA_CHANGED        == errno // (spurious error; retry)
//      || ERROR_DEVICE_NOT_CONNECTED == errno // (PERM ERROR! NO RETRY!)
//      || ERROR_DEV_NOT_EXIST        == errno // (PERM ERROR! NO RETRY!)
//      || ERROR_FILE_NOT_FOUND       == errno // (PERM ERROR! NO RETRY!)
    )
    {
        return EINTR;   // (Interrupted system call; Retry)
    }

    // (see PROGRAMMING NOTE further above)

    switch (errno)
    {
        default:                          break;  // (leave errno set to whatever it already is)
        case NO_ERROR:    errno = 0;      break;  // (normal expected i/o result)
        case ERROR_CRC:   errno = EIO;    break;  // (dirty drive or bad media)

        case ERROR_BEGINNING_OF_MEDIA: *pStat |= GMT_BOT     (0xFFFFFFFF); errno = EIO;       break;
        case ERROR_END_OF_MEDIA:       *pStat |= GMT_EOT     (0xFFFFFFFF); errno = ENOSPC;    break;

        //  "ERROR_END_OF_MEDIA"
        //
        //      Msg:   "The physical end of the tape has been reached."
        //
        //      The EOT warning reflector has been reached or passed (i.e. you're
        //      now/still in the "EOT Warning Zone" area). Writing additional data
        //      and/or tapemarks may still be possible depending on the size of the
        //      EOT Warning Zone (as set by a SetTapeParameters call with a non-zero
        //      EOTWarningZoneSize value (if supported; see further below)) and
        //      how much data you've already written to the EOT Warning Zone area
        //      (i.e. once you're in the warning area, this "error" occurs after
        //      EACH and EVERY I/O [in the warning zone area] until the ABSOLUTE
        //      physical end-of-tape (ERROR_EOM_OVERFLOW) is reached; see below).
        //
        //
        //                       ***********************
        //                       **  IMPORTANT NOTE!  **
        //                       ***********************
        //
        //                    This is NOT actually an "error"!!!
        //
        //
        //      When this "error" occurs, your "ReadFile" and/or "WriteFile" call
        //      returns 'FALSE' even though ALL of your requested data was actually
        //      written successfully!! This can be verified by checking to ensure
        //      the returned "number of bytes written" actually matches the amount
        //      you asked to be written. If they're the same (and they ALWAYS will
        //      be for this specific "error" code), then it means this "error" is
        //      NOT actually an error at all, but rather just a WARNING instead!!
        //      (Had it been an actual i/o error, the error code would have been
        //      some other DIFFERENT error code value instead!!)
        //
        //
        //                       ***********************
        //                       **  ALSO IMPORTANT!  **
        //                       ***********************
        //      See also:
        //
        //    http://fixunix.com/storage/205622-bug-dlttape-sys-no-eot-warning.html
        //
        //      for ADDITIONAL IMPORTANT INFORMATION regarding always having to
        //      specifically request that this "error" code be returned to you:
        //
        //      Even when a drive reports it does not support the setting of the
        //      the 'EOTWarningZoneSize' value (i.e. the FeaturesLow field of the
        //      GetTapeParameters call returns '0' for TAPE_DRIVE_SET_EOT_WZ_SIZE
        //      field), it may still be possible for "ERROR_END_OF_MEDIA" warnings
        //      to be generated anyway by simply calling SetTapeParameters with a
        //      non-zero 'EOTWarningZoneSize' value anyway.
        //
        //      The reason for this is because some drives may not allow CHANGING
        //      the value (thus the reason for it reporting that setting the value
        //      is not supported), but may nevertheless still support the ENABLING
        //      of their own hard-coded internal value. That is to say, while the
        //      size of the warning zone may not be modifiable (as it may be hard-
        //      coded and thus unchangeable), the drive may still have the ability
        //      to REPORT reaching the EOT Warning zone IF SPECIFICALLY REQUESTED
        //      TO DO SO! (which is presumably what requesting a non-zero Warning
        //      Zone size would end up doing: i.e. even though such calls APPEAR
        //      to fail, they actually DO succeed in accomplishing SOMETHING, just
        //      not what you originally/specifically requested).
        //
        //      Thus calling SetTapeParameters with a non-zero 'EOTWarningZoneSize'
        //      value might very well succeed anyway even though GetTapeParameters
        //      reports that doing so is not supported, and by so doing, may cause
        //      the drive to begin reporting of "ERROR_END_OF_MEDIA" (whereas not
        //      attempting to do so would end up leaving the drive in its default
        //      non-reporting mode. That is to say, you should ALWAYS try setting
        //      a non-zero 'EOTWarningZoneSize' value, ignoring any "unsupported"
        //      error code that may be returned from such a call.)

        case ERROR_EOM_OVERFLOW:       *pStat |= GMT_EOT     (0xFFFFFFFF); errno = EIO;       break;

        //  "ERROR_EOM_OVERFLOW"
        //
        //      Msg:   "Physical end of tape encountered."
        //
        //      This error code means that the actual physical end-of-media has been
        //      reached, and no more data can be written to the tape. This includes
        //      tapemarks as well.
        //
        //                       ***********************
        //                       **  IMPORTANT NOTE!  **
        //                       ***********************
        //
        //                 This is a HARD (UNRECOVERABLE) error!!
        //
        //      To be programmatically informed of when you are coming close to the
        //      physical end-of-the-tape (such that you could be assured room still
        //      remained to write logical end-of-volume labels for example), simply
        //      call SetTapeParameters with a non-zero 'EOTWarningZoneSize' value
        //      and treat any "ERROR_END_OF_MEDIA" "errors" received when writing
        //      as warnings instead. (See prior discussion of "ERROR_END_OF_MEDIA"
        //      return code further above)

        case ERROR_NO_DATA_DETECTED:   *pStat |= GMT_EOD     (0xFFFFFFFF); errno = EIO;       break;
        case ERROR_FILEMARK_DETECTED:  *pStat |= GMT_EOF     (0xFFFFFFFF); errno = EIO;       break;
        case ERROR_SETMARK_DETECTED:   *pStat |= GMT_SM      (0xFFFFFFFF); errno = EIO;       break;
        case ERROR_NOT_READY:          *pStat |= GMT_DR_OPEN (0xFFFFFFFF); errno = ENOMEDIUM; break;
        case ERROR_NO_MEDIA_IN_DRIVE:  *pStat |= GMT_DR_OPEN (0xFFFFFFFF); errno = ENOMEDIUM; break;
        case ERROR_WRITE_PROTECT:      *pStat |= GMT_WR_PROT (0xFFFFFFFF); errno = EROFS;     break;
    }
    return errno;
}

////////////////////////////////////////////////////////////////////////////////////
// (forward references for private helper functions)

int  w32_internal_mtop  ( HANDLE hFile, U32* pStat, struct mtop*  mtop,  ifd_t ifd );
int  w32_internal_mtget ( HANDLE hFile, U32* pStat, struct mtget* mtget, ifd_t ifd );
int  w32_internal_mtpos ( HANDLE hFile, U32* pStat, DWORD* pdwLogPos,
                                                    DWORD* pdwAbsPos,    ifd_t ifd );

////////////////////////////////////////////////////////////////////////////////////
// Close tape device...

DLL_EXPORT
int w32_close_tape ( ufd_t  ufd )
{
    ifd_t  ifd  = W32STAPE_UFD2IFD( ufd );
    int    rc   = -1;
    errno       = EBADF;

    lock();

    if (1
        &&         ifd    >=  0
        &&         ifd    <   W32STAPE_MAX_FDNUMS
        && g_ifds[ ifd ]  !=  0
    )
    {
        // Deallocate resources

        HANDLE  hFile  = g_handles[ ifd ];
        char*   pName  = g_fnames [ ifd ];

        g_handles[ ifd ] = NULL;
        g_fnames [ ifd ] = NULL;
        g_fstats [ ifd ] = GMT_DR_OPEN (0xFFFFFFFF);
        g_BOTmsk [ ifd ] = 0xFFFFFFFF;
        g_BOTbot [ ifd ] = 0x00000000;

        VERIFY( w32_free_ifd_nolock( ifd ) == 0 );

        // Close the file...

        free( pName );

        errno  =  CloseHandle( hFile ) ? 0 : w32_trans_w32error( GetLastError() );
        rc     =  errno ? -1 : 0;
    }

    unlock();

    return rc;
}

////////////////////////////////////////////////////////////////////////////////////
// Read tape...

DLL_EXPORT
ssize_t  w32_read_tape ( ufd_t ufd, void* buf, size_t nbyte )
{
    BOOL    bSuccess;
    DWORD   dwBytesRead;
    DWORD   dwLastError;

    ifd_t   ifd    = W32STAPE_UFD2IFD( ufd );
    U32*    pStat  = NULL;
    HANDLE  hFile;

    if (!buf)
    {
        errno = EINVAL;
        return -1;
    }

    lock();

    if (0
        ||         ifd    <   0
        ||         ifd    >=  W32STAPE_MAX_FDNUMS
        || g_ifds[ ifd ]  ==  0
    )
    {
        unlock();
        errno = EBADF;
        return -1;
    }

    unlock();

    hFile = g_handles[ ifd ];
    pStat = &g_fstats[ ifd ];

    // Do the i/o, save results, update device status
    // (based on the results), then check results...

    do
    {
        dwBytesRead = 0;
        bSuccess    = ReadFile( hFile, buf, (DWORD)nbyte, &dwBytesRead, NULL );
        errno       = (dwLastError = GetLastError());
        errno       = w32_internal_rc ( pStat );
    }
    while ( !bSuccess && EINTR == errno );

    // Success?  (see: "ERROR_END_OF_MEDIA" in function 'w32_internal_rc')

    if (bSuccess || ERROR_END_OF_MEDIA == dwLastError)
    {
        ASSERT( bSuccess || ENOSPC == errno );
        return ( (ssize_t) dwBytesRead );
    }

    ASSERT( !bSuccess && ERROR_END_OF_MEDIA != dwLastError && ENOSPC != errno );

    // The i/o "failed".  Check to see if it was just a tapemark...

    if ( EIO == errno && GMT_EOF( *pStat ) )
    {
        ASSERT( ERROR_FILEMARK_DETECTED == dwLastError );
        return 0;   // (tapemark)
    }

    // EIO != errno || !GMT_EOF( *pStat )  -->  bonafide i/o error...

    ASSERT( ERROR_FILEMARK_DETECTED != dwLastError );
    return -1;
}

////////////////////////////////////////////////////////////////////////////////////
// Write tape...

DLL_EXPORT
ssize_t  w32_write_tape ( ufd_t ufd, const void* buf, size_t nbyte )
{
    BOOL     bSuccess;
    DWORD    dwBytesWritten;
    DWORD    dwLastError;

    ifd_t    ifd    = W32STAPE_UFD2IFD( ufd );
    U32*     pStat  = NULL;
    HANDLE   hFile;

    if (!buf)
    {
        errno = EINVAL;
        return -1;
    }

    lock();

    if (0
        ||         ifd    <   0
        ||         ifd    >=  W32STAPE_MAX_FDNUMS
        || g_ifds[ ifd ]  ==  0
    )
    {
        unlock();
        errno = EBADF;
        return -1;
    }

    unlock();

    hFile = g_handles[ ifd ];
    pStat = &g_fstats[ ifd ];

    // Do the i/o, save results, update device status
    // (based on the results), then check results...

    do
    {
        dwBytesWritten = 0;
        bSuccess       = WriteFile( hFile, buf, (DWORD)nbyte, &dwBytesWritten, NULL );
        errno          = (dwLastError = GetLastError());
        errno          = w32_internal_rc ( pStat );
    }
    while ( !bSuccess && EINTR == errno );

    // Success?  (see: "ERROR_END_OF_MEDIA" in function 'w32_internal_rc')

    if (bSuccess || ERROR_END_OF_MEDIA == dwLastError)
    {
        ASSERT( bSuccess || ENOSPC == errno );
        ASSERT( ((size_t)dwBytesWritten) == nbyte );  // (MUST be true!!)
        return ( (ssize_t) dwBytesWritten );
    }

    // I/O error...

    ASSERT( !bSuccess && ERROR_END_OF_MEDIA != dwLastError && ENOSPC != errno );
    return -1;
}

////////////////////////////////////////////////////////////////////////////////////
// ioctl...    (perform some type of control function, e.g. fsf, rewind, etc)

DLL_EXPORT
int w32_ioctl_tape ( ufd_t ufd, int request, ... )
{
    va_list  vl;
    void*    ptr    = NULL;
    int      rc     = 0;
    ifd_t    ifd    = W32STAPE_UFD2IFD( ufd );
    U32*     pStat  = NULL;
    HANDLE   hFile;

    lock();

    if (0
        ||         ifd    <   0
        ||         ifd    >=  W32STAPE_MAX_FDNUMS
        || g_ifds[ ifd ]  ==  0
    )
    {
        unlock();
        errno = EBADF;
        return -1;
    }

    unlock();

    hFile = g_handles[ ifd ];
    pStat = &g_fstats[ ifd ];

    va_start ( vl, request );
    ptr = va_arg( vl, void* );

    if ( !ptr )
    {
        errno = EINVAL;
        return -1;
    }

    switch (request)
    {
        case MTIOCTOP:  // (perform tape operation)
        {
            struct mtop* mtop = ptr;
            rc = w32_internal_mtop ( hFile, pStat, mtop, ifd );
        }
        break;

        case MTIOCGET:  // (retrieve tape status)
        {
            struct mtget* mtget = ptr;
            memset( mtget, 0, sizeof(*mtget) );
            rc = w32_internal_mtget ( hFile, pStat, mtget, ifd );
        }
        break;

        case MTIOCPOS:  // (retrieve tape position)
        {
            struct mtpos* mtpos = ptr;
            memset( mtpos, 0, sizeof(*mtpos) );
            rc = w32_internal_mtpos( hFile, pStat, &mtpos->mt_blkno, NULL, ifd );
        }
        break;

        default:    // (invalid/unsupported ioctl code)
        {
            errno = EINVAL;
            rc = -1;
        }
        break;
    }

    return rc;
}

////////////////////////////////////////////////////////////////////////////////////
// Private internal helper function...   return 0 == success, -1 == failure

static
int w32_internal_mtop ( HANDLE hFile, U32* pStat, struct mtop* mtop, ifd_t ifd )
{
    int rc = 0;

    ASSERT( pStat && mtop );    // (sanity check)

    // General technique: do the i/o, save results, update the
    // device status (based on the results), then check results...

    switch ( mtop->mt_op )
    {
        case MTLOAD:    // (load media)
        {
            if ( 1 != mtop->mt_count )
            {
                errno = EINVAL;
                rc = -1;
            }
            else
            {
                do
                {
                    errno = PrepareTape( hFile, TAPE_LOAD, FALSE );
                    errno = w32_internal_rc ( pStat );
                }
                while ( EINTR == errno );
            }
        }
        break;

        case MTUNLOAD:  // (unload media)
        case MTOFFL:    // (make media offline (same as unload))
        {
            if ( 1 != mtop->mt_count )
            {
                errno = EINVAL;
                rc = -1;
            }
            else
            {
                do
                {
                    errno = PrepareTape( hFile, TAPE_UNLOAD, FALSE );
                    errno = w32_internal_rc ( pStat );
                }
                while ( EINTR == errno );
            }
        }
        break;

        case MTSEEK:    // (position media)
        {
            do
            {
                errno = SetTapePosition( hFile, TAPE_LOGICAL_BLOCK, 0, mtop->mt_count, 0, FALSE );
                errno = w32_internal_rc ( pStat );
            }
            while ( EINTR == errno );
        }
        break;

        case MTREW:     // (rewind)
        {
            if ( 1 != mtop->mt_count )
            {
                errno = EINVAL;
                rc = -1;
            }
            else
            {
                do
                {
                    errno = SetTapePosition( hFile, TAPE_REWIND, 0, 0, 0, FALSE );
                    errno = w32_internal_rc ( pStat );
                }
                while ( EINTR == errno );
            }
        }
        break;

        case MTFSF:     // (FORWARD  space FILE)
        case MTBSF:     // (BACKWARD space FILE)
        {
            if ( !mtop->mt_count )
            {
                errno = EINVAL;
                rc = -1;
            }
            else
            {
                LARGE_INTEGER liCount;

                liCount.QuadPart = mtop->mt_count;

                if ( MTBSF == mtop->mt_op )
                    liCount.QuadPart = -liCount.QuadPart; // (negative == backwards)

                do
                {
                    errno = SetTapePosition( hFile, TAPE_SPACE_FILEMARKS, 0, liCount.LowPart, liCount.HighPart, FALSE );
                    errno = w32_internal_rc ( pStat );
                }
                while ( EINTR == errno );
            }
        }
        break;

        case MTFSR:     // (FORWARD  space BLOCK)
        case MTBSR:     // (BACKWARD space BLOCK)
        {
            if ( !mtop->mt_count )
            {
                errno = EINVAL;
                rc = -1;
            }
            else
            {
                LARGE_INTEGER liCount;

                liCount.QuadPart = mtop->mt_count;

                if ( MTBSR == mtop->mt_op )
                    liCount.QuadPart = -liCount.QuadPart; // (negative == backwards)

                do
                {
                    errno = SetTapePosition( hFile, TAPE_SPACE_RELATIVE_BLOCKS, 0, liCount.LowPart, liCount.HighPart, FALSE );
                    errno = w32_internal_rc ( pStat );
                }
                while ( EINTR == errno );
            }
        }
        break;

        case MTSETBLK:  // (set blocksize)
        {
            TAPE_SET_MEDIA_PARAMETERS  media_parms;

            media_parms.BlockSize = mtop->mt_count;

            do
            {
                errno = SetTapeParameters( hFile, SET_TAPE_MEDIA_INFORMATION, &media_parms );
                errno = w32_internal_rc ( pStat );
            }
            while ( EINTR == errno );
        }
        break;

        case MTEOTWARN:   // (set EOT Warning Zone size in bytes)
        {
            TAPE_SET_DRIVE_PARAMETERS   set_drive_parms;

            set_drive_parms.ECC                = g_drive_parms[ifd].ECC;
            set_drive_parms.Compression        = g_drive_parms[ifd].Compression;
            set_drive_parms.DataPadding        = g_drive_parms[ifd].DataPadding;
            set_drive_parms.ReportSetmarks     = g_drive_parms[ifd].ReportSetmarks;
            set_drive_parms.EOTWarningZoneSize = mtop->mt_count;

            do
            {
                errno = SetTapeParameters( hFile, SET_TAPE_DRIVE_INFORMATION, &set_drive_parms );
                errno = w32_internal_rc ( pStat );
            }
            while ( EINTR == errno );
        }
        break;

        case MTWEOF:    // (write TAPEMARK)
        {
            if ( mtop->mt_count < 0 )
            {
                errno = EINVAL;
                rc = -1;
            }
            else
            {
                // PROGRAMMING NOTE: We prefer "long" filemarks over any other type
                // because, according to the SDK documentaion:
                //
                //    "A short filemark contains a short erase gap that cannot be
                //     overwritten unless the write operation is performed from the
                //     beginning of the partition or from an earlier long filemark."
                //
                //    "A long filemark contains a long erase gap that allows an
                //     application to position the tape at the beginning of the filemark
                //     and to overwrite the filemark and the erase gap."
                //
                // Thus if TAPE_LONG_FILEMARKS is not supported we try ONLY the generic
                // TAPE_FILEMARKS variety and return an error if that fails; we do NOT
                // ever attempt the TAPE_SHORT_FILEMARKS or TAPE_SETMARKS variety.

                DWORD  dwTapemarkType = TAPE_LONG_FILEMARKS;

                if ( !( g_drive_parms[ifd].FeaturesHigh & TAPE_DRIVE_WRITE_LONG_FMKS ) )
                    dwTapemarkType = TAPE_FILEMARKS;

                do
                {
                    errno = WriteTapemark( hFile, dwTapemarkType, mtop->mt_count, FALSE );
                    errno = w32_internal_rc ( pStat );
                }
                while ( EINTR == errno );
            }
        }
        break;

        case MTERASE: // (write erase gap or erase entire tape (data security erase))
        {
            if (1
                && 0 != mtop->mt_count  // (0 == write erase gap at current position)
                && 1 != mtop->mt_count  // (1 == erases the remainder of entire tape)
            )
            {
                errno = EINVAL;
                rc = -1;
            }
            else
            {
                DWORD  dwEraseType  =
                    mtop->mt_count ? TAPE_ERASE_LONG : TAPE_ERASE_SHORT;

                do
                {
                    errno = EraseTape( hFile, dwEraseType, FALSE );
                    errno = w32_internal_rc ( pStat );
                }
                while ( EINTR == errno );
            }
        }
        break;

        case MTNOP:         // (no operation)
        {
            errno = 0;
            rc = 0;
        }
        break;

        default:        // (invalid/unsupported tape operation)
        {
            errno = EINVAL;
            rc = -1;
        }
        break;
    }

    return (rc = (0 == errno || ENOSPC == errno) ? 0 : /* errno != 0 && errno != ENOSPC */ -1);
}

////////////////////////////////////////////////////////////////////////////////////
// Private internal helper function...   return 0 == success, -1 == failure

static
int w32_internal_mtget ( HANDLE hFile, U32* pStat, struct mtget* mtget, ifd_t ifd )
{
    TAPE_GET_MEDIA_PARAMETERS   media_parms;
    DWORD                       dwRetCode, dwSize, dwLogicalPosition;

    ASSERT( pStat && mtget );

    mtget->mt_resid   =   0;            // (unknown/unsupported)
    mtget->mt_erreg   =   0;            // (unknown/unsupported)
    mtget->mt_fileno  =  -1;            // (unknown/unsupported)
    mtget->mt_blkno   =  -1;            // (unknown as of yet; set further below)
    mtget->mt_type    =  MT_ISSCSI2;    // "Generic ANSI SCSI-2 tape unit"
    mtget->mt_gstat   =  -1;            // (purposely invalid; set correctly below)

    // Reset the mounted status; it will get set further below...

    *pStat &= ~GMT_DR_OPEN (0xFFFFFFFF);

    // Attempt to retrieve the status of the tape-drive...

    dwRetCode = w32_get_tape_status( hFile );

    // Windows returns 'ERROR_NOT_READY' if no tape is mounted
    // instead of the usual expected 'ERROR_NO_MEDIA_IN_DRIVE'

    if ( ERROR_NOT_READY == dwRetCode )
        dwRetCode = ERROR_NO_MEDIA_IN_DRIVE;

    // If there is not tape mounted OR a new tape was mounted,
    // then the following status bits are now unknown/obsolete

    if (0
        || ERROR_NO_MEDIA_IN_DRIVE == dwRetCode
        || ERROR_MEDIA_CHANGED     == dwRetCode
    )
    {
        // (these statuse are now obsolete)
        *pStat  &=  ~GMT_WR_PROT (0xFFFFFFFF);
        *pStat  &=  ~GMT_BOT     (0xFFFFFFFF);
        *pStat  &=  ~GMT_EOT     (0xFFFFFFFF);
        *pStat  &=  ~GMT_EOD     (0xFFFFFFFF);
        *pStat  &=  ~GMT_EOF     (0xFFFFFFFF);
        *pStat  &=  ~GMT_SM      (0xFFFFFFFF);
    }

    // There's no sense trying to get media parameters
    // unless there's some media loaded on the drive!

    if ( ERROR_NO_MEDIA_IN_DRIVE == dwRetCode )
    {
        *pStat |= GMT_DR_OPEN (0xFFFFFFFF);     // (no tape mounted in drive)
        mtget->mt_gstat = *pStat;               // (return current status)
        return 0;                               // (nothing more we can do)
    }

    // A tape appears to be mounted on the drive...
    // Retrieve the media parameters information...

    dwSize = sizeof(media_parms);
    memset( &media_parms, 0, dwSize );
    dwRetCode = GetTapeParameters( hFile, GET_TAPE_MEDIA_INFORMATION, &dwSize, &media_parms );
    ASSERT( sizeof(media_parms) == dwSize );

    if ( NO_ERROR == dwRetCode )
    {
        mtget->mt_dsreg = media_parms.BlockSize;

        if (media_parms.WriteProtected)
            *pStat |=  GMT_WR_PROT (0xFFFFFFFF);
        else
            *pStat &= ~GMT_WR_PROT (0xFFFFFFFF);
    }
    else
        mtget->mt_dsreg = 0;    // (unknown; variable blocks presumed)

    // Lastly, attempt to determine if we are at BOT (i.e. load-point)...

    if ( 0 != ( errno = w32_internal_mtpos( hFile, pStat, &dwLogicalPosition, NULL, ifd ) ) )
    {
        mtget->mt_gstat = *pStat;
        return -1;
    }

    mtget->mt_blkno = dwLogicalPosition;

    if ( ( dwLogicalPosition & g_BOTmsk[ ifd ] ) == g_BOTbot[ ifd ] )
        *pStat |=  GMT_BOT (0xFFFFFFFF);
    else
        *pStat &= ~GMT_BOT (0xFFFFFFFF);

    mtget->mt_gstat = *pStat;
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////
// Private internal helper function...   return 0 == success, -1 == failure

static
int  w32_internal_mtpos ( HANDLE hFile, U32* pStat, DWORD* pdwLogPos,
                                                    DWORD* pdwAbsPos, ifd_t ifd )
{
    DWORD dwDummyPartition, dwDummyPositionHigh;

    ASSERT( pStat && pdwLogPos );    // (sanity check)

    // PROGRAMMING NOTE: the SDK docs state that for the 'lpdwOffsetHigh'
    // parameter (i.e. dwDummyPositionHigh, the 5th paramater):
    //
    //    "This parameter can be NULL if the
    //     high-order bits are not required."
    //
    // But it LIES! Simple expirical observation reveals that ALL parameters
    // are in fact required. If any are NULL then 'GetTapePosition' crashes
    // and burns (which is unusual since usually when you pass invalid args
    // to an API it usually just returns an error code, but in this case it
    // doesn't. It actually crashes)

    do
    {
        U32 dummy_stat = 0;
        errno = GetTapePosition
        (
            hFile,
            TAPE_LOGICAL_POSITION,
            &dwDummyPartition,
            pdwLogPos,
            &dwDummyPositionHigh
        );
        errno = w32_internal_rc ( &dummy_stat );
    }
    while ( EINTR == errno );

    if (errno)
        return -1;

    if (pdwAbsPos)  // (may be NULL if they're not interested in it)
    {
        do
        {
            U32 dummy_stat = 0;
            errno = GetTapePosition
            (
                hFile,
                TAPE_ABSOLUTE_POSITION,
                &dwDummyPartition,
                pdwAbsPos,
                &dwDummyPositionHigh
            );
            errno = w32_internal_rc ( &dummy_stat );
        }
        while ( EINTR == errno );

        if (errno)
            return -1;
    }

    // PROGRAMMING NOTE: the Windows 'GetTapePosition' API returns either
    // a LOGICAL position value or an ABSOLUTE position value. Based on
    // trial and error it was determined the LOGICAL position corresponds
    // to the SCSI "READ POSITION" command's "first block location" value,
    // and the ABSOLUTE tape position appears to correspond to the SCSI
    // "last block location".

    // Since what we want is what IBM calls the "Channel block ID" (which
    // itself appears to correspond to what the SCSI documentation refers
    // to as the "First block location"), then what we want here is what
    // Windows refers to as the LOGICAL position, not the ABSOLUTE (i.e.
    // device-relative) position I originally thought we needed/wanted.

    if ( ( *pdwLogPos & g_BOTmsk[ ifd ] ) == g_BOTbot[ ifd ] )
        *pStat |=  GMT_BOT (0xFFFFFFFF);
    else
        *pStat &= ~GMT_BOT (0xFFFFFFFF);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////

#endif /* _MSVC_ */

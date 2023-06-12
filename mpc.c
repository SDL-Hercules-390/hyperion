/* MPC.C        (C) Copyright Jan Jaeger,  2010-2012                 */
/*              (C) Copyright Ian Shorter, 2011-2012                 */
/*              (C) Copyright Harold Grovesteen, 2011-2012           */
/*              MPC (Multi-Path Channel) functions                   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* This implementation is based on the S/390 Linux implementation    */

#include "hstdinc.h"

#define _MPC_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "mpc.h"


/*--------------------------------------------------------------------*/
/* mpc_point_ipa():                                                   */
/*--------------------------------------------------------------------*/
DLL_EXPORT MPC_IPA*  mpc_point_ipa( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, MPC_RRH* pMPC_RRH )
{
    MPC_PH*    pMPC_PH;
    MPC_IPA*   pMPC_IPA;
    U32        uOffData;
    U16        uOffPH;

    UNREFERENCED( pDEVBLK );

    // Point to the MPC_PH.
    FETCH_HW( uOffPH, pMPC_RRH->offph );
    pMPC_PH = (MPC_PH*)((BYTE*)pMPC_RRH + uOffPH);

    // Get the length of and point to the data referenced by the
    // MPC_PH. The data contain a MPC_IPA.
    FETCH_FW( uOffData, pMPC_PH->offdata );
    pMPC_IPA = (MPC_IPA*)((BYTE*)pMPC_TH + uOffData);

    return pMPC_IPA;
}   /* End function  mpc_point_ipa() */

/*--------------------------------------------------------------------*/
/* mpc_point_puk():                                                   */
/*--------------------------------------------------------------------*/
DLL_EXPORT MPC_PUK*  mpc_point_puk( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, MPC_RRH* pMPC_RRH )
{
    MPC_PH*    pMPC_PH;
    MPC_PUK*   pMPC_PUK;
    U32        uOffData;
    U16        uOffPH;

    UNREFERENCED( pDEVBLK );

    // Point to the MPC_PH.
    FETCH_HW( uOffPH, pMPC_RRH->offph );
    pMPC_PH = (MPC_PH*)((BYTE*)pMPC_RRH + uOffPH);

    // Get the length of and point to the data referenced by the
    // MPC_PH. The data contain a MPC_PUK and one or more MPC_PUSs.
    FETCH_FW( uOffData, pMPC_PH->offdata );
    pMPC_PUK = (MPC_PUK*)((BYTE*)pMPC_TH + uOffData);

    return pMPC_PUK;
}   /* End function  mpc_point_puk() */

/*--------------------------------------------------------------------*/
/* mpc_point_pus():                                                   */
/*--------------------------------------------------------------------*/
/* Return a pointer to the MPC_PUS of the required type. If the       */
/* required type is not found a null pointer is returned. Although    */
/* never seen, if multiple MPC_PUS of the required type exist, only   */
/* the first MPC_PUS of the required type will ever be returned.      */
DLL_EXPORT MPC_PUS*  mpc_point_pus( DEVBLK* pDEVBLK, MPC_PUK* pMPC_PUK, BYTE bType )
{
    MPC_PUS*   pMPC_PUS;
    int        iTotLenPUS;
    U16        uTotLenPUS;
    U16        uLenPUS;
    U16        uLenPUK;

    UNREFERENCED( pDEVBLK );

    /* Get the length of the MPC_PUK, the total length of the */
    /* following MPC_PUSs, then point to the first MPC_PUS.   */
    FETCH_HW( uLenPUK, pMPC_PUK->length );
    FETCH_HW( uTotLenPUS, pMPC_PUK->lenpus );
    iTotLenPUS = uTotLenPUS;
    pMPC_PUS = (MPC_PUS*)((BYTE*)pMPC_PUK + uLenPUK);

    /* Find the required MPC_PUS. */
    while( iTotLenPUS > 0 )
    {
        /* Ensure there are at least the first 4-bytes of an MPC_PUS. */
        if( iTotLenPUS < 4 )
            return NULL;

        /* Get the length of the MPC_PUS. */
        FETCH_HW( uLenPUS, pMPC_PUS->length );
        if( uLenPUS == 0 )                  /* Better safe than sorry */
            return NULL;

        /* Ensure there is the whole of the MPC_PUS. */
        if( iTotLenPUS < uLenPUS )
            return NULL;

        /* Check for the required MPC_PUS. */
        if( pMPC_PUS->type == bType )
            return pMPC_PUS;

        /* Point to the next MPC_PUS. */
        pMPC_PUS = (MPC_PUS*)((BYTE*)pMPC_PUS + uLenPUS);
        iTotLenPUS -= uLenPUS;
    }

    return NULL;
}   /* End function  mpc_point_pus() */

/*--------------------------------------------------------------------*/
/* mpc_display_description():                                         */
/*--------------------------------------------------------------------*/
DLL_EXPORT void  mpc_display_description( DEVBLK* pDEVBLK, char* pDesc )
{

    /* Display description, if one has been provided. */
    if( pDesc )
    {
        if( pDEVBLK )
        {
            // HHC03983 "%1d:%04X %s: %s
            WRMSG(HHC03983, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname, pDesc );
        }
        else
        {
            // HHC03984 "%s"
            WRMSG(HHC03984, "D", pDesc );
        }
    }

    return;
}   /* End function  mpc_display_description() */

/*--------------------------------------------------------------------*/
/* mpc_display_stuff()                                                */
/*--------------------------------------------------------------------*/
/* Function to display storage.                                       */
DLL_EXPORT void  mpc_display_stuff( DEVBLK* pDEVBLK, char* cWhat, BYTE* pAddr, int iLen, BYTE bDir )
{
    int           offset;
    unsigned int  i;
    u_char        c = '\0';
    u_char        e = '\0';
    char          print_ascii[17];
    char          print_ebcdic[17];
    char          print_line[64];
    char          tmp[32];

    for( offset = 0; offset < iLen; )
    {
        memset( print_ascii, ' ', sizeof(print_ascii)-1 );    /* set to spaces */
        print_ascii[sizeof(print_ascii)-1] = '\0';            /* with null termination */
        memset( print_ebcdic, ' ', sizeof(print_ebcdic)-1 );  /* set to spaces */
        print_ebcdic[sizeof(print_ebcdic)-1] = '\0';          /* with null termination */
        memset( print_line, 0, sizeof( print_line ) );

        MSGBUF( print_line, "+%4.4X%c ", offset, bDir );

        for( i = 0; i < 16; i++ )
        {
            c = *pAddr++;

            if( offset < iLen )
            {
                MSGBUF(tmp, "%2.2X", c );
                tmp[sizeof(tmp)-1] = '\0';
                STRLCAT( print_line, tmp );

                print_ebcdic[i] = print_ascii[i] = '.';
                e = guest_to_host( c );

                if( isprint( (unsigned char)e ) )
                    print_ebcdic[i] = e;
                if( isprint( (unsigned char)c ) )
                    print_ascii[i] = c;
            }
            else
            {
                STRLCAT( print_line, "  " );
            }

            offset++;
            if( ( offset & 3 ) == 0 )
            {
                STRLCAT( print_line, " " );
            }
        }

        if( pDEVBLK )
        {
            // HHC03981 "%1d:%04X %s: %s: %s %s  %s"
            WRMSG(HHC03981, "D", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->typname,
                                 cWhat, print_line, print_ascii, print_ebcdic );
        }
        else
        {
            // HHC03982 "%s: %s %s  %s"
            WRMSG(HHC03982, "D", cWhat, print_line, print_ascii, print_ebcdic );
        }
    }

    return;
}   /* End function  mpc_display_stuff() */

/*--------------------------------------------------------------------*/
/* mpc_display_th(): Display Transport Header (MPC_TH)                */
/*--------------------------------------------------------------------*/
DLL_EXPORT void  mpc_display_th( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, BYTE bDir )
{
    U32    uOffRRH;

    // Display the MPC_TH.
    FETCH_FW( uOffRRH, pMPC_TH->offrrh );
    mpc_display_stuff( pDEVBLK, "TH ", (BYTE*)pMPC_TH, uOffRRH, bDir );

    return;
}   /* End function  mpc_display_th() */

/*--------------------------------------------------------------------*/
/* mpc_display_rrh(): Display Request/Response Header (MPC_RRH)       */
/*--------------------------------------------------------------------*/
DLL_EXPORT void  mpc_display_rrh( DEVBLK* pDEVBLK, MPC_RRH* pMPC_RRH, BYTE bDir )
{
    U16    uOffPH;

    // Display the MPC_RRH.
    FETCH_HW( uOffPH, pMPC_RRH->offph );
    mpc_display_stuff( pDEVBLK, "RRH", (BYTE*)pMPC_RRH, uOffPH, bDir );

    return;
}   /* End function  mpc_display_rrh() */

/*--------------------------------------------------------------------*/
/* mpc_display_ph(): Display Protocol Data Unit Header (MPC_PH)       */
/*--------------------------------------------------------------------*/
DLL_EXPORT void  mpc_display_ph( DEVBLK* pDEVBLK, MPC_PH* pMPC_PH, BYTE bDir )
{

    // Display the MPC_PH.
    mpc_display_stuff( pDEVBLK, "PH ", (BYTE*)pMPC_PH, SIZE_PH, bDir );

    return;
}   /* End function  mpc_display_ph() */

/*--------------------------------------------------------------------*/
/* mpc_display_rrh_and_puk():                                         */
/*--------------------------------------------------------------------*/
/* In all cases that have been seen, on both OSA and PTP, when        */
/* MPC_RRH->proto == PROTOCOL_UNKNOWN (0x7E), the MPC_RRH is followed */
/* by a single MPC_PH, which is followed by a single MPC_PUK, which   */
/* is followed by up to four MPC_PUSs.                                */
DLL_EXPORT void  mpc_display_rrh_and_puk( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, MPC_RRH* pMPC_RRH, BYTE bDir )
{
    MPC_PH*    pMPC_PH;
    MPC_PUK*   pMPC_PUK;
    MPC_PUS*   pMPC_PUS;
    int        iTotLenPUS;
    U32        uOffData;
    U16        uTotLenPUS;
    U16        uLenPUS;
    U16        uLenPUK;
    U16        uOffPH;

    // Display the MPC_RRH.
    FETCH_HW( uOffPH, pMPC_RRH->offph );
    mpc_display_stuff( pDEVBLK, "RRH", (BYTE*)pMPC_RRH, uOffPH, bDir );

    // Point to and display the MPC_PH.
    pMPC_PH = (MPC_PH*)((BYTE*)pMPC_RRH + uOffPH);
    mpc_display_stuff( pDEVBLK, "PH ", (BYTE*)pMPC_PH, SIZE_PH, bDir );

    // Get the length of and point to the data referenced by the
    // MPC_PH. The data contain a MPC_PUK and one or more MPC_PUSs.
    FETCH_FW( uOffData, pMPC_PH->offdata );
    pMPC_PUK = (MPC_PUK*)((BYTE*)pMPC_TH + uOffData);

    // Display the MPC_PUK.
    FETCH_HW( uLenPUK, pMPC_PUK->length );
    mpc_display_stuff( pDEVBLK, "PUK", (BYTE*)pMPC_PUK, uLenPUK, bDir );

    // Get the total length of the following MPC_PUSs, then point to
    // the first MPC_PUS.
    FETCH_HW( uTotLenPUS, pMPC_PUK->lenpus );
    iTotLenPUS = uTotLenPUS;
    pMPC_PUS = (MPC_PUS*)((BYTE*)pMPC_PUK + uLenPUK);

    // Display all of the MPC_PUSs.
    while( iTotLenPUS > 0 )
    {
        // Ensure there are at least the first 4-bytes of the MPC_PUS.
        if( iTotLenPUS < 4 )
        {
            mpc_display_stuff( pDEVBLK, "???", (BYTE*)pMPC_PUS, iTotLenPUS, bDir );
            break;
        }

        // Get the length of the MPC_PUS.
        FETCH_HW( uLenPUS, pMPC_PUS->length );
        if( uLenPUS == 0 )                     /* Better safe than sorry */
        {
            mpc_display_stuff( pDEVBLK, "???", (BYTE*)pMPC_PUS, iTotLenPUS, bDir );
            break;
        }

        // Ensure there is the whole of the MPC_PUS.
        if( iTotLenPUS < uLenPUS )
        {
            mpc_display_stuff( pDEVBLK, "???", (BYTE*)pMPC_PUS, iTotLenPUS, bDir );
            break;
        }

        // Display the MPC_PUS.
        mpc_display_stuff( pDEVBLK, "PUS", (BYTE*)pMPC_PUS, uLenPUS, bDir );

        // Point to the next MPC_PUS
        pMPC_PUS = (MPC_PUS*)((BYTE*)pMPC_PUS + uLenPUS);
        iTotLenPUS -= uLenPUS;
    }

    return;
}   /* End function  mpc_display_rrh_and_puk() */

/*--------------------------------------------------------------------*/
/* mpc_display_rrh_and_pix():                                         */
/*--------------------------------------------------------------------*/
/* In all cases that have been seen, only on PTP, when MPC_RRH->type  */
/* == RRH_TYPE_IPA (0xC1) and MPC_RRH->proto == PROTOCOL_LAYER2       */
/* (0x08), the MPC_RRH is followed by a single MPC_PH, which is       */
/* followed by a single MPC_PIX.                                      */
DLL_EXPORT void  mpc_display_rrh_and_pix( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, MPC_RRH* pMPC_RRH, BYTE bDir )
{
    MPC_PH*    pMPC_PH;
    MPC_PIX*   pMPC_PIX;
    U32        uOffData;
    U32        uLenData;
    U16        uOffPH;

    // Display the MPC_RRH.
    FETCH_HW( uOffPH, pMPC_RRH->offph );
    mpc_display_stuff( pDEVBLK, "RRH", (BYTE*)pMPC_RRH, uOffPH, bDir );

    // Point to and display the MPC_PH.
    pMPC_PH = (MPC_PH*)((BYTE*)pMPC_RRH + uOffPH);
    mpc_display_stuff( pDEVBLK, "PH ", (BYTE*)pMPC_PH, SIZE_PH, bDir );

    // Point to and display the MPC_PIX.
    FETCH_F3( uLenData, pMPC_PH->lendata );
    FETCH_FW( uOffData, pMPC_PH->offdata );
    pMPC_PIX = (MPC_PIX*)((BYTE*)pMPC_TH + uOffData);
    mpc_display_stuff( pDEVBLK, "PIX", (BYTE*)pMPC_PIX, uLenData, bDir );

    return;
}   /* End function  mpc_display_rrh_and_pix() */

/*--------------------------------------------------------------------*/
/* mpc_display_rrh_and_ipa():                                         */
/*--------------------------------------------------------------------*/
DLL_EXPORT void  mpc_display_rrh_and_ipa( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, MPC_RRH* pMPC_RRH, BYTE bDir )
{
    MPC_PH*    pMPC_PH;
    MPC_IPA*   pMPC_IPA;
    BYTE*      pMPC_IPA_CMD;
    U32        uOffData;
    U32        uLenData;
    U16        uOffPH;
    int        iLenIPA;
    int        iLenCmd;

    // Display the MPC_RRH.
    FETCH_HW( uOffPH, pMPC_RRH->offph );
    mpc_display_stuff( pDEVBLK, "RRH", (BYTE*)pMPC_RRH, uOffPH, bDir );

    // Point to and display the MPC_PH.
    pMPC_PH = (MPC_PH*)((BYTE*)pMPC_RRH + uOffPH);
    mpc_display_stuff( pDEVBLK, "PH ", (BYTE*)pMPC_PH, SIZE_PH, bDir );

    /* Point to and display the MPC_IPA (and commands, if any). */
    FETCH_F3( uLenData, pMPC_PH->lendata );
    FETCH_FW( uOffData, pMPC_PH->offdata );
    if( uLenData > sizeof(MPC_IPA) )
    {
        iLenIPA = sizeof(MPC_IPA);
        iLenCmd = uLenData - sizeof(MPC_IPA);
    }
    else
    {
        iLenIPA = uLenData;
        iLenCmd = 0;
    }
    pMPC_IPA = (MPC_IPA*)((BYTE*)pMPC_TH + uOffData);
    mpc_display_stuff( pDEVBLK, "IPA", (BYTE*)pMPC_IPA, iLenIPA, bDir );
    if( iLenCmd )
    {
        pMPC_IPA_CMD = (BYTE*)pMPC_IPA + iLenIPA;
        mpc_display_stuff( pDEVBLK, "Cmd", (BYTE*)pMPC_IPA_CMD, iLenCmd, bDir );
    }

    return;
}   /* End function  mpc_display_rrh_and_ipa() */

/*--------------------------------------------------------------------*/
/* mpc_display_rrh_and_pkt():                                         */
/*--------------------------------------------------------------------*/
DLL_EXPORT void  mpc_display_rrh_and_pkt( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, MPC_RRH* pMPC_RRH, BYTE bDir, int iLimit )
{
    MPC_PH*    pMPC_PH;
    U16        uNumPH;
    U16        uOffPH;
    int        iForPH;
    int        iDone;
    U32        uLenData;
    U32        uOffData;
    BYTE*      pData;

    /* Display the MPC_RRH.*/
    FETCH_HW( uOffPH, pMPC_RRH->offph );
    mpc_display_stuff( pDEVBLK, "RRH", (BYTE*)pMPC_RRH, uOffPH, bDir );

    /* Display the MPC_PH(s). */
    FETCH_HW( uNumPH, pMPC_RRH->numph );
    pMPC_PH = (MPC_PH*)((BYTE*)pMPC_RRH + uOffPH);
    for( iForPH = 1; iForPH <= uNumPH; iForPH++ )
    {
        mpc_display_stuff( pDEVBLK, "PH ", (BYTE*)pMPC_PH, SIZE_PH, bDir );
        pMPC_PH = (MPC_PH*)((BYTE*)pMPC_PH + SIZE_PH);
    }

    /* Display the data referenced by the MPC_PH(s).              */
    /* if limit is negative or a silly number, don't display the  */
    /* data. If limit is zero, display all of the data, otherwise */
    /* limit the length of the data displayed.                    */
    iDone = 0;
    if( iLimit >= 0 && iLimit <= 65535 )
    {
        pMPC_PH = (MPC_PH*)((BYTE*)pMPC_RRH + uOffPH);
        for( iForPH = 1; iForPH <= uNumPH; iForPH++ )
        {
            FETCH_F3( uLenData, pMPC_PH->lendata );
            FETCH_FW( uOffData, pMPC_PH->offdata );
            pData = (BYTE*)pMPC_TH + uOffData;
            if( iLimit > 0 )
            {
                if( iDone >= iLimit )
                    break;
                if( (int)uLenData > ( iLimit - iDone ) )
                    uLenData = ( iLimit - iDone );
                iDone += uLenData;
            }
            mpc_display_stuff( pDEVBLK, "Pkt", pData, uLenData, bDir );
            pMPC_PH = (MPC_PH*)((BYTE*)pMPC_PH + SIZE_PH);
        }
    }

    return;
}   /* End function  mpc_display_rrh_and_pkt() */

/*--------------------------------------------------------------------*/
/* mpc_display_rrh_and_pdu():                                         */
/*--------------------------------------------------------------------*/
DLL_EXPORT void  mpc_display_rrh_and_pdu( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, MPC_RRH* pMPC_RRH, BYTE bDir, int iLimit )
{
    MPC_PH*    pMPC_PH;
    U16        uNumPH;
    U16        uOffPH;
    int        iForPH;
    int        iDone;
    U32        uLenData;
    U32        uOffData;
    BYTE*      pData;

    /* Display the MPC_RRH.*/
    FETCH_HW( uOffPH, pMPC_RRH->offph );
    mpc_display_stuff( pDEVBLK, "RRH", (BYTE*)pMPC_RRH, uOffPH, bDir );

    /* Display the MPC_PH(s). */
    FETCH_HW( uNumPH, pMPC_RRH->numph );
    pMPC_PH = (MPC_PH*)((BYTE*)pMPC_RRH + uOffPH);
    for( iForPH = 1; iForPH <= uNumPH; iForPH++ )
    {
        mpc_display_stuff( pDEVBLK, "PH ", (BYTE*)pMPC_PH, SIZE_PH, bDir );
        pMPC_PH = (MPC_PH*)((BYTE*)pMPC_PH + SIZE_PH);
    }

    /* Display the data referenced by the MPC_PH(s).              */
    /* if limit is negative or a silly number, don't display the  */
    /* data. If limit is zero, display all of the data, otherwise */
    /* limit the length of the data displayed.                    */
    iDone = 0;
    if( iLimit >= 0 && iLimit <= 65535 )
    {
        pMPC_PH = (MPC_PH*)((BYTE*)pMPC_RRH + uOffPH);
        for( iForPH = 1; iForPH <= uNumPH; iForPH++ )
        {
            FETCH_F3( uLenData, pMPC_PH->lendata );
            FETCH_FW( uOffData, pMPC_PH->offdata );
            pData = (BYTE*)pMPC_TH + uOffData;
            if( iLimit > 0 )
            {
                if( iDone >= iLimit )
                    break;
                if( (int)uLenData > ( iLimit - iDone ) )
                    uLenData = ( iLimit - iDone );
                iDone += uLenData;
            }
            mpc_display_stuff( pDEVBLK, "PDU", pData, uLenData, bDir );
            pMPC_PH = (MPC_PH*)((BYTE*)pMPC_PH + SIZE_PH);
        }
    }

    return;
}   /* End function  mpc_display_rrh_and_pdu() */

/*--------------------------------------------------------------------*/
/* mpc_display_osa_iea():                                             */
/*--------------------------------------------------------------------*/
DLL_EXPORT void  mpc_display_osa_iea( DEVBLK* pDEVBLK, MPC_IEA* pMPC_IEA, BYTE bDir, int iSize )
{

    /* Display MPC_IEA. */
    mpc_display_stuff( pDEVBLK, "IEA", (BYTE*)pMPC_IEA, iSize, bDir );

    return;
}   /* End function  mpc_display_osa_iea() */

/*--------------------------------------------------------------------*/
/* mpc_display_osa_iear():                                            */
/*--------------------------------------------------------------------*/
DLL_EXPORT void  mpc_display_osa_iear( DEVBLK* pDEVBLK, MPC_IEAR* pMPC_IEAR, BYTE bDir, int iSize )
{

    /* Display MPC_IEAR. */
    mpc_display_stuff( pDEVBLK, "IEAR", (BYTE*)pMPC_IEAR, iSize, bDir );

    return;
}   /* End function  mpc_display_osa_iear() */

/*--------------------------------------------------------------------*/
/* mpc_display_osa_th_etc():                                          */
/*--------------------------------------------------------------------*/
DLL_EXPORT void  mpc_display_osa_th_etc( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, BYTE bDir, int iLimit )
{
    MPC_RRH*   pMPC_RRH;
    int        iForRRH;
    U32        uOffRRH;
    U16        uNumRRH;

    /* Display MPC_TH. */
    mpc_display_th( pDEVBLK, pMPC_TH, bDir );

    /* Get the number of MPC_RRHs and the displacement from    */
    /* the start of the MPC_TH to the first (or only) MPC_RRH. */
    FETCH_HW( uNumRRH, pMPC_TH->numrrh );
    FETCH_FW( uOffRRH, pMPC_TH->offrrh );

    /* Process each of the MPC_RRHs. */
    for( iForRRH = 1; iForRRH <= uNumRRH; iForRRH++ )
    {

        /* Point to the first or subsequent MPC_RRH. */
        pMPC_RRH = (MPC_RRH*)((BYTE*)pMPC_TH + uOffRRH);

        /* Display the MPC_RRH etc. */
        if( pMPC_RRH->type == RRH_TYPE_CM ||
            pMPC_RRH->type == RRH_TYPE_ULP )
        {

            /* Display MPC_RRH and following MPC_PUK etc. */
            mpc_display_rrh_and_puk( pDEVBLK, pMPC_TH, pMPC_RRH, bDir );

        }
        else if( pMPC_RRH->type == RRH_TYPE_IPA )
        {

            /* Display MPC_RRH and following MPC_IPA etc. */
            mpc_display_rrh_and_ipa( pDEVBLK, pMPC_TH, pMPC_RRH, bDir );

        }
        else
        {

            /* Display MPC_RRH and following PDU. */
            mpc_display_rrh_and_pdu( pDEVBLK, pMPC_TH, pMPC_RRH, bDir, iLimit );

        }

        /* Get the displacement from the start of the MPC_TH to the */
        /* next MPC_RRH. pMPC_RRH->offrrh will contain zero if this */
        /* is the last MPC_RRH.                                     */
        FETCH_FW( uOffRRH, pMPC_RRH->offrrh );

    }

    return;
}   /* End function  mpc_display_osa_th_etc() */

/*--------------------------------------------------------------------*/
/* mpc_display_ptp_th_etc():                                          */
/*--------------------------------------------------------------------*/
DLL_EXPORT void  mpc_display_ptp_th_etc( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, BYTE bDir, int iLimit )
{
    MPC_RRH*   pMPC_RRH;
    int        iForRRH;
    U32        uOffRRH;
    U16        uNumRRH;

    /* Display MPC_TH. */
    mpc_display_th( pDEVBLK, pMPC_TH, bDir );

    /* Get the number of MPC_RRHs and the displacement from    */
    /* the start of the MPC_TH to the first (or only) MPC_RRH. */
    FETCH_HW( uNumRRH, pMPC_TH->numrrh );
    FETCH_FW( uOffRRH, pMPC_TH->offrrh );

    /* Process each of the MPC_RRHs. */
    for( iForRRH = 1; iForRRH <= uNumRRH; iForRRH++ )
    {

        /* Point to the first or subsequent MPC_RRH. */
        pMPC_RRH = (MPC_RRH*)((BYTE*)pMPC_TH + uOffRRH);

        /* Display the MPC_RRH etc. */
        if( pMPC_RRH->proto == PROTOCOL_LAYER2 &&
            pMPC_RRH->type == RRH_TYPE_CM )
        {
            /* Display MPC_RRH and following packet data. */
            mpc_display_rrh_and_pkt( pDEVBLK, pMPC_TH, pMPC_RRH, bDir, iLimit );
        }
        else if( pMPC_RRH->proto == PROTOCOL_LAYER2 &&
                 pMPC_RRH->type == RRH_TYPE_IPA )
        {
            /* Display MPC_RRH and following MPC_PIX etc. */
            mpc_display_rrh_and_pix( pDEVBLK, pMPC_TH, pMPC_RRH, bDir );
        }
        else if( pMPC_RRH->proto == PROTOCOL_UNKNOWN )
        {
            /* Display MPC_RRH and following MPC_PUK etc. */
            mpc_display_rrh_and_puk( pDEVBLK, pMPC_TH, pMPC_RRH, bDir );
        }
        else
        {
            /* Display MPC_RRH */
            mpc_display_rrh( pDEVBLK, pMPC_RRH, bDir );
        }

        /* Get the displacement from the start of the MPC_TH to the */
        /* next MPC_RRH. pMPC_RRH->offrrh will contain zero if this */
        /* is the last MPC_RRH.                                     */
        FETCH_FW( uOffRRH, pMPC_RRH->offrrh );

    }

    return;
}   /* End function  mpc_display_ptp_th_etc() */


/* VSTORE.H     (C) Copyright Roger Bowler, 2000-2012                */
/*              (C) and others 2013-2023                             */
/*                  Virtual Storage Functions                        */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/*                                                                   */
/*                  Virtual Storage Functions                        */
/*                                                                   */
/*  This module contains various functions which store, fetch, and   */
/*  copy values to, from, or between virtual storage locations.      */
/*                                                                   */
/*  Functions provided in this module are:                           */
/*                                                                   */
/*  vstoreb      Store a single byte into virtual storage            */
/*  vstore2      Store a two-byte integer into virtual storage       */
/*  vstore4      Store a four-byte integer into virtual storage      */
/*  vstore8      Store an eight-byte integer into virtual storage    */
/*  vstorec      Store 1 to 256 characters into virtual storage      */
/*                                                                   */
/*  wstoreX      Address-wrapping version of the above               */
/*                                                                   */
/*  vfetchb      Fetch a single byte from virtual storage            */
/*  vfetch2      Fetch a two-byte integer from virtual storage       */
/*  vfetch4      Fetch a four-byte integer from virtual storage      */
/*  vfetch8      Fetch an eight-byte integer from virtual storage    */
/*  vfetchc      Fetch 1 to 256 characters from virtual storage      */
/*                                                                   */
/*  wfetchX      Address-wrapping version of the above               */
/*                                                                   */
/*  instfetch    Fetch instruction from virtual storage              */
/*                                                                   */
/*  validate_operand   Validate addressing, protection, translation  */
/*  wvalidate_opend    Address wrapping version of the above         */
/*                                                                   */
/*  move_chars   Move characters using specified keys and addrspaces */
/*  wmove_chars  Address-wrapping version of the above               */
/*                                                                   */
/*  move_charx   Move characters with optional specifications        */
/*                                                                   */
/*-------------------------------------------------------------------*/
/* 2019-12-05  Bob Wood                                              */
/* Code modified to pass the correct length when needed for MADDR.   */
/* This is needed to support transaction execution mode.  Note that  */
/* for most of these instructions, no change is required.  Since     */
/* transaction mode operates at the cache line level (per the POP)   */
/* it is only an issue if the actual length spans a cache line       */
/* boundary but not a page boundary.                                 */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*   ARCH_DEP section: compiled multiple times, once for each arch.  */
/*-------------------------------------------------------------------*/

#if (ARCH_370_IDX == ARCH_IDX && !defined( DID_370_VSTORE_H )) \
 || (ARCH_390_IDX == ARCH_IDX && !defined( DID_390_VSTORE_H )) \
 || (ARCH_900_IDX == ARCH_IDX && !defined( DID_900_VSTORE_H ))

#define s370_wstorec(_src, _len, _addr, _arn, _regs) \
        s370_vstorec((_src), (_len), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s370_wstoreb(_value, _addr, _arn, _regs) \
        s370_vstoreb((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s370_wstore2(_value, _addr, _arn, _regs) \
        s370_vstore2((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s370_wstore4(_value, _addr, _arn, _regs) \
        s370_vstore4((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s370_wstore8(_value, _addr, _arn, _regs) \
        s370_vstore8((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s370_wfetchc(_dest, _len, _addr, _arn, _regs) \
        s370_vfetchc((_dest), (_len), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s370_wfetchb(_addr, _arn, _regs) \
        s370_vfetchb(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s370_wfetch2(_addr, _arn, _regs) \
        s370_vfetch2(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s370_wfetch4(_addr, _arn, _regs) \
        s370_vfetch4(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s370_wfetch8(_addr, _arn, _regs) \
        s370_vfetch8(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s370_wmove_chars(_addr1, _arn1, _key1, _addr2, _arn2, _key2, _len, _regs) \
        s370_move_chars(((_addr1) & ADDRESS_MAXWRAP((_regs))), (_arn1), (_key1), \
                        ((_addr2) & ADDRESS_MAXWRAP((_regs))), (_arn2), (_key2), (_len), (_regs))
#define s370_wvalidate_operand(_addr, _arn, _len, _acctype, _regs) \
        s370_validate_operand(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_len), (_acctype), (_regs))

/*-------------------------------------------------------------------*/

#define s390_wstorec(_src, _len, _addr, _arn, _regs) \
        s390_vstorec((_src), (_len), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s390_wstoreb(_value, _addr, _arn, _regs) \
        s390_vstoreb((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s390_wstore2(_value, _addr, _arn, _regs) \
        s390_vstore2((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s390_wstore4(_value, _addr, _arn, _regs) \
        s390_vstore4((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s390_wstore8(_value, _addr, _arn, _regs) \
        s390_vstore8((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s390_wfetchc(_dest, _len, _addr, _arn, _regs) \
        s390_vfetchc((_dest), (_len), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s390_wfetchb(_addr, _arn, _regs) \
        s390_vfetchb(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s390_wfetch2(_addr, _arn, _regs) \
        s390_vfetch2(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s390_wfetch4(_addr, _arn, _regs) \
        s390_vfetch4(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s390_wfetch8(_addr, _arn, _regs) \
        s390_vfetch8(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define s390_wmove_chars(_addr1, _arn1, _key1, _addr2, _arn2, _key2, _len, _regs) \
        s390_move_chars(((_addr1) & ADDRESS_MAXWRAP((_regs))), (_arn1), (_key1), \
                        ((_addr2) & ADDRESS_MAXWRAP((_regs))), (_arn2), (_key2), (_len), (_regs))
#define s390_wvalidate_operand(_addr, _arn, _len, _acctype, _regs) \
        s390_validate_operand(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_len), (_acctype), (_regs))

/*-------------------------------------------------------------------*/

#define z900_wstorec(_src, _len, _addr, _arn, _regs) \
        z900_vstorec((_src), (_len), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define z900_wstoreb(_value, _addr, _arn, _regs) \
        z900_vstoreb((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define z900_wstore2(_value, _addr, _arn, _regs) \
        z900_vstore2((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define z900_wstore4(_value, _addr, _arn, _regs) \
        z900_vstore4((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define z900_wstore8(_value, _addr, _arn, _regs) \
        z900_vstore8((_value), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define z900_wfetchc(_dest, _len, _addr, _arn, _regs) \
        z900_vfetchc((_dest), (_len), ((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define z900_wfetchb(_addr, _arn, _regs) \
        z900_vfetchb(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define z900_wfetch2(_addr, _arn, _regs) \
        z900_vfetch2(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define z900_wfetch4(_addr, _arn, _regs) \
        z900_vfetch4(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define z900_wfetch8(_addr, _arn, _regs) \
        z900_vfetch8(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_regs))
#define z900_wmove_chars(_addr1, _arn1, _key1, _addr2, _arn2, _key2, _len, _regs) \
        z900_move_chars(((_addr1) & ADDRESS_MAXWRAP((_regs))), (_arn1), (_key1), \
                        ((_addr2) & ADDRESS_MAXWRAP((_regs))), (_arn2), (_key2), (_len), (_regs))
#define z900_wvalidate_operand(_addr, _arn, _len, _acctype, _regs) \
        z900_validate_operand(((_addr) & ADDRESS_MAXWRAP((_regs))), (_arn), (_len), (_acctype), (_regs))

/*-------------------------------------------------------------------*/
/* The following two functions (concpy, concpy_rl) must be defined   */
/* first, before all the others, since the inline functions (which   */
/* follow afterwards) use them.                                      */
/*-------------------------------------------------------------------*/

#ifndef _VSTORE_CONCPY
#define _VSTORE_CONCPY

/*-------------------------------------------------------------------*/
/* Copy 8 bytes at a time concurrently   (from left to right)        */
/*-------------------------------------------------------------------*/
inline void concpy( REGS* regs, void* d, void* s, int n )
{
    BYTE* u8d = d;
    BYTE* u8s = s;

    /* Copy until ready or 8 byte integral boundary */
    while (n && ((uintptr_t) u8d & 7))
    {
        *u8d++ = *u8s++;
        n--;
    }

#if !((defined( SIZEOF_LONG )  && SIZEOF_LONG  > 7) || \
      (defined( SIZEOF_INT_P ) && SIZEOF_INT_P > 7) || \
       defined( OPTION_STRICT_ALLIGNMENT ))

    /* Code for 32-bit builds... */

    /* Copy full words in right condition, on enough length and src - dst distance */
    if (1
        && n
        && regs->cpubit == regs->sysblk->started_mask
        && abs((int)(u8d - u8s)) > 3
    )
    {
        while (n > 3)
        {
            store_fw_noswap( u8d, fetch_fw_noswap( u8s ));
            u8d += 4;
            u8s += 4;
            n   -= 4;
        }
    }
    else // copy double words

#else // x64 builds

    UNREFERENCED( regs );

#endif // end code for 32-bit builds

    /* Copy double words on enough length and src - dst distance */
    if (n && labs( u8d - u8s ) > 7)
    {
        while(n > 7)
        {
            store_dw_noswap( u8d, fetch_dw_noswap( u8s ));
            u8d += 8;
            u8s += 8;
            n -= 8;
        }
    }

    /* Copy leftovers */
    while (n)
    {
        *u8d++ = *u8s++;
        n--;
    }
} /* end function concpy */
#endif // _VSTORE_CONCPY

#if defined( FEATURE_061_MISC_INSTR_EXT_FACILITY_3 )

#ifndef _VSTORE_CONCPY_RL
#define _VSTORE_CONCPY_RL

/*-------------------------------------------------------------------*/
/* Copy 8 bytes at a time concurrently   (from right to left)        */
/*-------------------------------------------------------------------*/
inline void concpy_rl( REGS* regs, void* d, void* s, int n )
{
    BYTE* u8d = (BYTE*)d + n;
    BYTE* u8s = (BYTE*)s + n;

    /* Copy until ready or 8 byte integral boundary */
    while (n && ((uintptr_t) u8d & 7))
    {
        *--u8d = *--u8s;
        n--;
    }

#if !((defined( SIZEOF_LONG )  && SIZEOF_LONG  > 7) || \
      (defined( SIZEOF_INT_P ) && SIZEOF_INT_P > 7) || \
       defined( OPTION_STRICT_ALLIGNMENT ))

    /* Code for 32-bit builds... */

    /* Copy full words in right condition, on enough length and (src - dst) distance */
    if (1
        && n
        && regs->cpubit == regs->sysblk->started_mask
        && abs( u8d - u8s ) > 3
    )
    {
        while (n > 3)
        {
            store_fw_noswap( u8d-4, fetch_fw_noswap( u8s-4 ));
            u8d -= 4;
            u8s -= 4;
            n   -= 4;
        }
    }
    else // copy double words...

#else // x64 builds

    UNREFERENCED( regs );

#endif // end code for 32-bit builds

    /* Copy double words on enough length and (src - dst) distance */
    if (n && labs( u8d - u8s ) > 7)
    {
        while (n > 7)
        {
            store_dw_noswap( u8d-8, fetch_dw_noswap( u8s - 8 ));
            u8d -= 8;
            u8s -= 8;
            n   -= 8;
        }
    }

    /* Copy leftovers */
    while (n)
    {
        *--u8d = *--u8s;
        n--;
    }
} /* end function concpy_rl */

#endif // _VSTORE_CONCPY_RL

#endif // defined( FEATURE_061_MISC_INSTR_EXT_FACILITY_3 )

/*-------------------------------------------------------------------*/
/* Store a two-byte integer into virtual storage operand             */
/*                                                                   */
/* Input:                                                            */
/*      value   16-bit integer value to be stored                    */
/*      addr    Logical address of leftmost operand byte             */
/*      arn     Access register number                               */
/*      regs    CPU register context                                 */
/*                                                                   */
/*      A program check may be generated if the logical address      */
/*      causes an addressing, translation, or protection             */
/*      exception, and in this case the function does not return.    */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( vstore2_full )( U16 value, VADR addr, int arn, REGS* regs )
{
BYTE   *main1, *main2;                  /* Mainstor addresses        */
BYTE   *sk;                             /* Storage key addresses     */

    main1 = MADDR( addr, arn, regs, ACCTYPE_WRITE_SKP, regs->psw.pkey );
    sk = regs->dat.storkey;
    main2 = MADDR( (addr + 1) & ADDRESS_MAXWRAP( regs ), arn, regs,
                    ACCTYPE_WRITE, regs->psw.pkey );
    *sk |= (STORKEY_REF | STORKEY_CHANGE);
    *main1 = value >> 8;
    *main2 = value & 0xFF;
}

/*-------------------------------------------------------------------*/
/* Store a four-byte integer into virtual storage operand            */
/*                                                                   */
/* Input:                                                            */
/*      value   32-bit integer value to be stored                    */
/*      addr    Logical address of leftmost operand byte             */
/*      arn     Access register number                               */
/*      regs    CPU register context                                 */
/*                                                                   */
/*      A program check may be generated if the logical address      */
/*      causes an addressing, translation, or protection             */
/*      exception, and in this case the function does not return.    */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( vstore4_full )( U32 value, VADR addr, int arn, REGS* regs )
{
BYTE   *main1, *main2;                  /* Mainstor addresses        */
BYTE   *sk;                             /* Storage key addresses     */
int     len;                            /* Length to end of page     */
BYTE    temp[4];                        /* Copied value              */

    len = PAGEFRAME_PAGESIZE - (addr & PAGEFRAME_BYTEMASK);
    main1 = MADDRL( addr, len, arn, regs, ACCTYPE_WRITE_SKP, regs->psw.pkey );
    sk = regs->dat.storkey;
    main2 = MADDRL( (addr + len) & ADDRESS_MAXWRAP( regs ), 4-len, arn, regs,
                    ACCTYPE_WRITE, regs->psw.pkey );
    *sk |= (STORKEY_REF | STORKEY_CHANGE);
    STORE_FW( temp, value );
    memcpy( main1, temp,       len );
    memcpy( main2, temp+len, 4-len );
}

/*-------------------------------------------------------------------*/
/* Store an eight-byte integer into virtual storage operand          */
/*                                                                   */
/* Input:                                                            */
/*      value   64-bit integer value to be stored                    */
/*      addr    Logical address of leftmost operand byte             */
/*      arn     Access register number                               */
/*      regs    CPU register context                                 */
/*                                                                   */
/*      A program check may be generated if the logical address      */
/*      causes an addressing, translation, or protection             */
/*      exception, and in this case the function does not return.    */
/*                                                                   */
/*      NOTE that vstore8_full should only be invoked when a page    */
/*           boundary IS going to be crossed.                        */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( vstore8_full )( U64 value, VADR addr, int arn, REGS* regs )
{
BYTE   *main1, *main2;                  /* Mainstor addresses        */
BYTE   *sk;                             /* Storage key addresses     */
int     len;                            /* Length to end of page     */
BYTE    temp[8];                        /* Copied value              */

    len = PAGEFRAME_PAGESIZE - (addr & PAGEFRAME_BYTEMASK);
    main1 = MADDRL( addr, len, arn, regs, ACCTYPE_WRITE_SKP, regs->psw.pkey );
    sk = regs->dat.storkey;
    main2 = MADDRL( (addr + len) & ADDRESS_MAXWRAP( regs ), 8-len, arn, regs,
                    ACCTYPE_WRITE, regs->psw.pkey );
    *sk |= (STORKEY_REF | STORKEY_CHANGE);
    STORE_DW( temp, value );
    memcpy( main1, temp,       len );
    memcpy( main2, temp+len, 8-len );
}

/*-------------------------------------------------------------------*/
/* Fetch a two-byte integer operand from virtual storage             */
/*                                                                   */
/* Input:                                                            */
/*      addr    Logical address of leftmost byte of operand          */
/*      arn     Access register number                               */
/*      regs    CPU register context                                 */
/* Returns:                                                          */
/*      Operand in 16-bit integer format                             */
/*                                                                   */
/*      A program check may be generated if the logical address      */
/*      causes an addressing, translation, or fetch protection       */
/*      exception, and in this case the function does not return.    */
/*-------------------------------------------------------------------*/
inline U16 ARCH_DEP( vfetch2_full )( VADR addr, int arn, REGS* regs )
{
BYTE   *mn;                             /* Main storage addresses    */
U16     value;

    mn = MADDR( addr, arn, regs, ACCTYPE_READ, regs->psw.pkey );
    value = *mn << 8;
    mn = MADDR( (addr + 1) & ADDRESS_MAXWRAP( regs ), arn, regs,
                 ACCTYPE_READ, regs->psw.pkey );
    value |= *mn;
    return value;
}

/*-------------------------------------------------------------------*/
/* Fetch a four-byte integer operand from virtual storage            */
/*                                                                   */
/* Input:                                                            */
/*      addr    Logical address of leftmost byte of operand          */
/*      arn     Access register number                               */
/*      regs    CPU register context                                 */
/* Returns:                                                          */
/*      Operand in 32-bit integer format                             */
/*                                                                   */
/*      A program check may be generated if the logical address      */
/*      causes an addressing, translation, or fetch protection       */
/*      exception, and in this case the function does not return.    */
/*-------------------------------------------------------------------*/
inline U32 ARCH_DEP( vfetch4_full )( VADR addr, int arn, REGS* regs )
{
BYTE   *mn;                             /* Main storage addresses    */
int     len;                            /* Length to end of page     */
BYTE    temp[8];                        /* Copy destination          */

    len = PAGEFRAME_PAGESIZE - (addr & PAGEFRAME_BYTEMASK);
    mn = MADDRL( addr, len, arn, regs, ACCTYPE_READ, regs->psw.pkey );
    memcpy( temp, mn, len);
    mn = MADDRL( (addr + len) & ADDRESS_MAXWRAP( regs ), 4 - len, arn, regs,
                  ACCTYPE_READ, regs->psw.pkey );
    memcpy( temp+len, mn, 4 - len);
    return fetch_fw( temp );
}

/*-------------------------------------------------------------------*/
/* Fetch an eight-byte integer operand from virtual storage          */
/*                                                                   */
/* Input:                                                            */
/*      addr    Logical address of leftmost byte of operand          */
/*      arn     Access register number                               */
/*      regs    CPU register context                                 */
/* Returns:                                                          */
/*      Operand in 64-bit integer format                             */
/*                                                                   */
/*      A program check may be generated if the logical address      */
/*      causes an addressing, translation, or fetch protection       */
/*      exception, and in this case the function does not return.    */
/*-------------------------------------------------------------------*/
inline U64 ARCH_DEP( vfetch8_full )( VADR addr, int arn, REGS* regs )
{
BYTE   *mn;                             /* Main storage addresses    */
int     len;                            /* Length to end of page     */
BYTE    temp[16];                       /* Copy destination          */

    /* Get absolute address of first byte of operand */
    len = PAGEFRAME_PAGESIZE - (addr & PAGEFRAME_BYTEMASK);
    mn = MADDRL( addr, len, arn, regs, ACCTYPE_READ, regs->psw.pkey );
    memcpy( temp, mn, len);
    mn = MADDRL( (addr + len) & ADDRESS_MAXWRAP( regs ), 8 - len, arn, regs,
                 ACCTYPE_READ, regs->psw.pkey );
    memcpy( temp+len, mn, 8 );
    return fetch_dw( temp );
}

/*-------------------------------------------------------------------*/
/* Store 1 to 256 characters into virtual storage operand            */
/*                                                                   */
/* Input:                                                            */
/*      src     1 to 256 byte input buffer                           */
/*      len     Size of operand minus 1                              */
/*      addr    Logical address of leftmost character of operand     */
/*      arn     Access register number                               */
/*      regs    CPU register context                                 */
/*                                                                   */
/*      A program check may be generated if the logical address      */
/*      range causes an addressing, translation, or protection       */
/*      exception, and in this case no real storage locations are    */
/*      updated, and the function does not return.                   */
/*-------------------------------------------------------------------*/
// (needed by dyncrypt.c)
VSTORE_INL_DLL_IMPORT inline void ARCH_DEP( vstorec )( const void* src, BYTE len, VADR addr, int arn, REGS* regs )
{
BYTE   *main1, *main2;                  /* Mainstor addresses        */
BYTE   *sk;                             /* Storage key addresses     */
int     len2;                           /* Length to end of page     */

    if (NOCROSSPAGE( addr,len ))
    {
        memcpy( MADDRL( addr, len+1, arn, regs, ACCTYPE_WRITE, regs->psw.pkey ),
               src, len + 1);
        ITIMER_UPDATE( addr, len, regs );
    }
    else
    {
        len2 = PAGEFRAME_PAGESIZE - (addr & PAGEFRAME_BYTEMASK);
        main1 = MADDRL( addr, len2, arn, regs, ACCTYPE_WRITE_SKP,
                        regs->psw.pkey );
        sk = regs->dat.storkey;
        main2 = MADDRL( (addr + len2) & ADDRESS_MAXWRAP( regs ),
                        len+1-len2, arn,
                        regs, ACCTYPE_WRITE, regs->psw.pkey );
        *sk |= (STORKEY_REF | STORKEY_CHANGE);
        memcpy( main1, src, len2 );
        memcpy( main2, (BYTE*)src + len2, len + 1 - len2 );
    }
}

/*-------------------------------------------------------------------*/
/* Store a single byte into virtual storage operand                  */
/*                                                                   */
/* Input:                                                            */
/*      value   Byte value to be stored                              */
/*      addr    Logical address of operand byte                      */
/*      arn     Access register number                               */
/*      regs    CPU register context                                 */
/*                                                                   */
/*      A program check may be generated if the logical address      */
/*      causes an addressing, translation, or protection             */
/*      exception, and in this case the function does not return.    */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( vstoreb )( BYTE value, VADR addr, int arn, REGS* regs )
{
BYTE   *main1;                          /* Mainstor address          */

    main1 = MADDR( addr, arn, regs, ACCTYPE_WRITE, regs->psw.pkey );
    *main1 = value;
    ITIMER_UPDATE( addr, 1-1, regs );
}

/*-------------------------------------------------------------------*/
/* vstore2 accelerator - Simple case only (better inline candidate)  */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( vstore2 )( U16 value, VADR addr, int arn, REGS* regs )
{
    /* Most common case : Aligned & not crossing page boundary */
    if (likely(!((VADR_L)addr & 1)
        || ((VADR_L)addr & PAGEFRAME_BYTEMASK) != PAGEFRAME_BYTEMASK))
    {
        BYTE* mn;
        mn = MADDRL( addr, 2, arn, regs, ACCTYPE_WRITE, regs->psw.pkey );
        STORE_HW( mn, value );
        ITIMER_UPDATE( addr, 2-1, regs );
    }
    else
        ARCH_DEP( vstore2_full )( value, addr, arn, regs );
}

/*-------------------------------------------------------------------*/
/* vstore4 accelerator - Simple case only (better inline candidate)  */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( vstore4 )( U32 value, VADR addr, int arn, REGS* regs )
{
    /* Most common case : Aligned & not crossing page boundary */
    if (likely(!((VADR_L)addr & 0x03))
        || (((VADR_L)addr & PAGEFRAME_BYTEMASK) <= (PAGEFRAME_BYTEMASK-3)))
    {
        BYTE *mn;
        mn = MADDRL( addr, 4, arn, regs, ACCTYPE_WRITE, regs->psw.pkey );
        STORE_FW( mn, value );
        ITIMER_UPDATE( addr, 4-1, regs );
    }
    else
        ARCH_DEP( vstore4_full )( value, addr, arn, regs );
}

/*-------------------------------------------------------------------*/
/* vstore8 accelerator - Simple case only (better inline candidate)  */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( vstore8 )( U64 value, VADR addr, int arn, REGS* regs )
{
#if defined( OPTION_SINGLE_CPU_DW ) && defined( ASSIST_STORE_DW )
    /* Check alignement. If aligned then we are guaranteed
       not to cross a page boundary */
    if (likely(!((VADR_L)addr & 0x07)))
    {
        /* Most common case : Aligned */
        U64 *mn;
        mn = (U64*)MADDRL( addr, 8, arn, regs, ACCTYPE_WRITE, regs->psw.pkey );
        if (regs->cpubit == regs->sysblk->started_mask)
            *mn = CSWAP64( value );
        else
            STORE_DW( mn, value );
    }
    else
#endif
    {
        /* We're not aligned. So we have to check whether we are
           crossing a page boundary. This cannot be the same
           code as above because casting U64* to a non aligned
           pointer may break on those architectures mandating
           strict alignement */
        if (likely(((VADR_L)addr & PAGEFRAME_BYTEMASK) <= (PAGEFRAME_BYTEMASK-7)))
        {
            /* Non aligned but not crossing page boundary */
            BYTE *mn;
            mn = MADDRL( addr, 8, arn, regs, ACCTYPE_WRITE, regs->psw.pkey );
            /* invoking STORE_DW ensures endianness correctness */
            STORE_DW( mn, value );
        }
        else
            /* Crossing page boundary */
            ARCH_DEP( vstore8_full )( value, addr, arn, regs );
    }
    ITIMER_UPDATE( addr, 8-1, regs );
}

/*-------------------------------------------------------------------*/
/* Fetch a 1 to 256 character operand from virtual storage           */
/*                                                                   */
/* Input:                                                            */
/*      len     Size of operand minus 1                              */
/*      addr    Logical address of leftmost character of operand     */
/*      arn     Access register number                               */
/*      regs    CPU register context                                 */
/* Output:                                                           */
/*      dest    1 to 256 byte output buffer                          */
/*                                                                   */
/*      A program check may be generated if the logical address      */
/*      causes an addressing, translation, or fetch protection       */
/*      exception, and in this case the function does not return.    */
/*-------------------------------------------------------------------*/
// (needed by dyncrypt.c)
VSTORE_INL_DLL_IMPORT inline void ARCH_DEP( vfetchc )( void* dest, BYTE len, VADR addr, int arn, REGS* regs )
{
BYTE   *main1, *main2;                  /* Main storage addresses    */
int     len2;                           /* Length to copy on page    */

    if (NOCROSSPAGE( addr, len ))
    {
        ITIMER_SYNC( addr, len, regs );
        main1 = MADDRL( addr, len + 1, arn, regs, ACCTYPE_READ, regs->psw.pkey );
        memcpy( dest, main1, len + 1 );
    }
    else
    {
        len2 = PAGEFRAME_PAGESIZE - (addr & PAGEFRAME_BYTEMASK);
        main1 = MADDRL( addr, len2, arn, regs, ACCTYPE_READ, regs->psw.pkey );
        main2 = MADDRL( (addr + len2) & ADDRESS_MAXWRAP( regs ), len + 1 - len2,
                        arn, regs, ACCTYPE_READ, regs->psw.pkey );
        memcpy(        dest,        main1,           len2 );
        memcpy( (BYTE*)dest + len2, main2, len + 1 - len2 );
    }
}

/*-------------------------------------------------------------------*/
/* Fetch a single byte operand from virtual storage                  */
/*                                                                   */
/* Input:                                                            */
/*      addr    Logical address of operand character                 */
/*      arn     Access register number                               */
/*      regs    CPU register context                                 */
/* Returns:                                                          */
/*      Operand byte                                                 */
/*                                                                   */
/*      A program check may be generated if the logical address      */
/*      causes an addressing, translation, or fetch protection       */
/*      exception, and in this case the function does not return.    */
/*-------------------------------------------------------------------*/
inline BYTE ARCH_DEP( vfetchb )( VADR addr, int arn, REGS* regs )
{
BYTE   *mn;                             /* Main storage address      */

    ITIMER_SYNC( addr, 1-1, regs );
    mn = MADDR( addr, arn, regs, ACCTYPE_READ, regs->psw.pkey );
    return *mn;
}

/*-------------------------------------------------------------------*/
/* vfetch2 accelerator - Simple case only (better inline candidate)  */
/*-------------------------------------------------------------------*/
inline U16 ARCH_DEP( vfetch2 )( VADR addr, int arn, REGS* regs )
{
    if (likely(!((VADR_L)addr & 0x01))
        || (((VADR_L)addr & PAGEFRAME_BYTEMASK) != PAGEFRAME_BYTEMASK ))
    {
        BYTE *mn;
        ITIMER_SYNC( addr, 2-1, regs );
        mn = MADDRL( addr, 2,arn, regs, ACCTYPE_READ, regs->psw.pkey );
        return fetch_hw( mn );
    }
    return ARCH_DEP( vfetch2_full )( addr, arn, regs );
}

/*-------------------------------------------------------------------*/
/* vfetch4 accelerator - Simple case only (better inline candidate)  */
/*-------------------------------------------------------------------*/
inline U32 ARCH_DEP( vfetch4 )( VADR addr, int arn, REGS* regs )
{
    if ((likely(!((VADR_L)addr & 0x03))
        || (((VADR_L)addr & PAGEFRAME_BYTEMASK) <= (PAGEFRAME_BYTEMASK-3) )))
    {
        BYTE *mn;
        ITIMER_SYNC( addr, 4-1, regs );
        mn = MADDRL( addr, 4,arn, regs, ACCTYPE_READ, regs->psw.pkey );
        return fetch_fw( mn );
    }
    return ARCH_DEP( vfetch4_full )( addr, arn, regs );
}

/*-------------------------------------------------------------------*/
/* vfetch8 accelerator - Simple case only (better inline candidate)  */
/*-------------------------------------------------------------------*/
inline U64 ARCH_DEP( vfetch8 )( VADR addr, int arn, REGS* regs )
{
#if defined( OPTION_SINGLE_CPU_DW ) && defined( ASSIST_STORE_DW )
    if(likely(!((VADR_L)addr & 0x07)))
    {
        /* doubleword aligned fetch */
        U64 *mn;
        ITIMER_SYNC( addr, 8-1, regs );
        mn = (U64*)MADDRL( addr, 8, arn, regs, ACCTYPE_READ, regs->psw.pkey );
        if (regs->cpubit == regs->sysblk->started_mask)
            return CSWAP64( *mn );
        return fetch_dw( mn );
    }
    else
#endif
    {
        if (likely(((VADR_L)addr & PAGEFRAME_BYTEMASK) <= (PAGEFRAME_BYTEMASK-7)))
        {
            /* unaligned, non-crossing doubleword fetch */
            BYTE *mn;
            ITIMER_SYNC( addr, 8-1, regs );
            mn = MADDRL( addr, 8, arn, regs, ACCTYPE_READ, regs->psw.pkey );
            return fetch_dw( mn );
        }
    }
    /* page crossing doubleword fetch */
    return ARCH_DEP( vfetch8_full )( addr, arn, regs );
}

/*-------------------------------------------------------------------*/
/* Fetch instruction from halfword-aligned virtual storage location  */
/*                                                                   */
/* Input:                                                            */
/*      regs    Pointer to the CPU register context                  */
/*      exec    If 1 then called by EXecute otherwise called by      */
/*              INSTRUCTION_FETCH                                    */
/*                                                                   */
/* If called by INSTRUCTION_FETCH then                               */
/*      addr    regs->psw.IA                                         */
/*      dest    regs->inst                                           */
/*                                                                   */
/* If called by EXecute then                                         */
/*      addr    regs->ET                                             */
/*      dest    regs->exinst                                         */
/*                                                                   */
/* Output:                                                           */
/*      If successful, a pointer is returned to the instruction. If  */
/*      the instruction crossed a page boundary then the instruction */
/*      is copied either to regs->inst or regs->exinst (depending on */
/*      the exec flag).  Otherwise the pointer points into mainstor. */
/*                                                                   */
/*      If the exec flag is 0 and tracing or PER is not active then  */
/*      the AIA is updated.  This forces interrupts to be checked    */
/*      and instfetch to be call for each instruction.               */
/*                                                                   */
/*      A program check may be generated if the instruction address  */
/*      is odd, or causes an addressing or translation exception,    */
/*      and in this case the function does not return.  In the       */
/*      latter case, regs->instinvalid is 1 which indicates to       */
/*      program_interrupt that the exception occurred during         */
/*      instruction fetch.                                           */
/*                                                                   */
/*      Because this function is inlined and 'exec' is a constant    */
/*      (either 0 or 1) the references to exec are optimized out by  */
/*      the compiler.                                                */
/*-------------------------------------------------------------------*/
inline BYTE* ARCH_DEP( instfetch )( REGS* regs, int exec )
{
VADR    addr;                           /* Instruction address       */
BYTE*   ip;                             /* Instruction pointer       */
BYTE*   dest;                           /* Fetched Instruction       */
int     pagesz;                         /* Effective page size       */
int     offset;                         /* Address offset into page  */
int     len;                            /* Length for page crossing  */

    addr = exec ? regs->ET : VALID_AIE( regs ) ?
        PSW_IA_FROM_IP( regs, 0 ) : regs->psw.IA;

    /* Ensure PSW IA matches the instruction we're fetching */
    if (!exec)
        MAYBE_SET_PSW_IA_FROM_IP( regs );

    offset = (int)(addr & PAGEFRAME_BYTEMASK);

    pagesz = unlikely( addr < 0x800 ) ? 0x800 : PAGEFRAME_PAGESIZE;

    /* Program check if instruction address is odd */
    if (unlikely( offset & 0x01 ))
    {
        if (!exec)
            regs->instinvalid = 1;

        regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );
    }

#if defined( FEATURE_PER )

    /* Save the address used to fetch the instruction */
    if (EN_IC_PER( regs ))
    {
#if defined( FEATURE_PER2 )
        regs->perc = 0x40    /* ATMID-validity */
                   | (regs->psw.amode64        << 7 )
                   | (regs->psw.amode          << 5 )
                   | (!REAL_MODE( &regs->psw ) ? 0x10 : 0 )
                   | (SPACE_BIT(  &regs->psw ) << 3 )
                   | (AR_BIT   (  &regs->psw ) << 2 );
#else
        regs->perc = 0;
#endif /* defined( FEATURE_PER2 ) */

        if (!exec)
            regs->peradr = addr;

        /* Test for PER instruction-fetching event */
        if (1
            && EN_IC_PER_IF( regs )
            && PER_RANGE_CHECK( addr, regs->CR(10), regs->CR(11) )
        )
        {
            ON_IC_PER_IF( regs );

#if defined( FEATURE_PER3 )
            /* If CR9_IFNUL (PER instruction-fetching nullification) is
               set, take a program check immediately, without executing
               the instruction or updating the PSW instruction address */
            if (EN_IC_PER_IFNUL( regs ))
            {
                ON_IC_PER_IFNUL( regs );
                regs->psw.IA = addr;
                regs->psw.zeroilc = 1;
                regs->program_interrupt( regs, PGM_PER_EVENT );
            }
#endif /* defined( FEATURE_PER3 ) */
        }

        /* Quick exit if AIA is still valid */
        if (1
            && !exec
            && !regs->breakortrace
            &&  VALID_AIE( regs )
            &&  regs->ip < (regs->aip + pagesz - 5)
        )
        {
#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )

            /* Update CONSTRAINED trans instruction fetch constraint */
            if (regs->txf_contran)
            {
                if (regs->AIV == regs->txf_aie_aiv2)
                    regs->txf_aie = regs->aip + regs->txf_aie_off2;
            }

            TXF_INSTRADDR_CONSTRAINT( regs );

#endif /* defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) */

            /* Save the address of the instruction ABOUT to be executed */
            regs->periaddr = PSW_IA_FROM_IP( regs, 0 );

            /* Suppress PER instruction fetch event if appropriate */
            if (IS_PER_SUPRESS( regs, CR9_IF ))
                OFF_IC_PER_IF( regs );

            /* Return to caller to execute this instruction */
            return regs->ip;
        }
    }
#endif /* defined( FEATURE_PER ) */

    /* Set instinvalid in case of addressing or translation exception */
    if (!exec)
        regs->instinvalid = 1;

    /* Get instruction address */
    ip = MADDRL( addr, 6, USE_INST_SPACE, regs, ACCTYPE_INSTFETCH, regs->psw.pkey );

    /* If boundary is crossed then copy instruction to destination */
    if (offset + ILC( ip[0] ) > pagesz)
    {
        /* Copy first part of instruction (note: dest is 8 bytes) */
        dest = exec ? regs->exinst : regs->inst;
        memcpy( dest, ip, 4 );

        /* Copy second part of instruction */
        len = pagesz - offset;
        addr = (addr + len) & ADDRESS_MAXWRAP( regs );
        ip = MADDR( addr, USE_INST_SPACE, regs, ACCTYPE_INSTFETCH, regs->psw.pkey );
        if (!exec)
            regs->ip = ip - len;
        memcpy( dest + len, ip, 4 );
    }
    else /* boundary NOT crossed */
    {
        dest = ip;

        if (!exec)
            regs->ip = ip;
    }

    if (!exec)
    {
        /* Instr addr now known to be valid so reset instinvalid flag */
        regs->instinvalid = 0;

        /* Update the AIA values */
        regs->AIV = addr & PAGEFRAME_PAGEMASK;
        regs->aip = (BYTE*)((uintptr_t)ip & ~PAGEFRAME_BYTEMASK);

        /* If tracing, stepping or PER is still active,
           force another instfetch after this instruction.
        */
        if (regs->breakortrace || regs->permode)
        {
            /* Force instfetch to be called again on next inst. */
            regs->aie = PSEUDO_INVALID_AIE;
        }
        else /* (stepping/tracing/PER is NOT active) */
        {
            /* Call instfetch again ONLY when truly needed */
            regs->aie = regs->aip + pagesz - 5;
        }
    }

#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )

    /* Update CONSTRAINED trans instruction fetch constraint */
    if (regs->txf_contran)
    {
        if (regs->AIV == regs->txf_aie_aiv2)
            regs->txf_aie = regs->aip + regs->txf_aie_off2;
    }

    TXF_INSTRADDR_CONSTRAINT( regs );

#endif /* defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) */

    /* Save the address of the instruction ABOUT to be executed */
    regs->periaddr = PSW_IA_FROM_IP( regs, 0 );

    /* Suppress PER instruction fetch event if appropriate */
    if (IS_PER_SUPRESS( regs, CR9_IF ))
        OFF_IC_PER_IF( regs );

    /* Return to caller to execute this instruction */
    return dest;

} /* end function ARCH_DEP( instfetch ) */

/*-------------------------------------------------------------------*/
/*                  Move characters left to right                    */
/*              using specified keys and address spaces              */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* Input:                                                            */
/*      addr1   Effective address of destination operand             */
/*      arn1    Access register number for destination operand,      */
/*              or USE_PRIMARY_SPACE or USE_SECONDARY_SPACE          */
/*      key1    Bits 0-3=first operand access key, 4-7=zeroes        */
/*                                                                   */
/*                                                                   */
/*      addr2   Effective address of source operand                  */
/*      arn2    Access register number for source operand,           */
/*              or USE_PRIMARY_SPACE or USE_SECONDARY_SPACE          */
/*      key2    Bits 0-3=second operand access key, 4-7=zeroes       */
/*                                                                   */
/*                                                                   */
/*      len     Operand length minus 1 (range 0-255)                 */
/*      regs    Pointer to the CPU register context                  */
/*                                                                   */
/*                                                                   */
/*      This function implements the MVC, MVCP, MVCS, MVCK, MVCSK,   */
/*      and MVCDK instructions.  These instructions move up to 256   */
/*      characters using the address space and key specified by      */
/*      the caller for each operand.  Operands are moved byte by     */
/*      byte to ensure correct processing of overlapping operands.   */
/*                                                                   */
/*      The arn parameter for each operand may be an access          */
/*      register number, in which case the operand is in the         */
/*      primary, secondary, or home space, or in the space           */
/*      designated by the specified access register, according to    */
/*      the current PSW addressing mode.                             */
/*                                                                   */
/*      Alternatively the arn parameter may be one of the special    */
/*      values USE_PRIMARY_SPACE or USE_SECONDARY_SPACE in which     */
/*      case the operand is in the specified space regardless of     */
/*      the current PSW addressing mode.                             */
/*                                                                   */
/*      A program check may be generated if either logical address   */
/*      causes an addressing, protection, or translation exception,  */
/*      and in this case the function does not return.               */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( move_chars )( VADR addr1, int arn1, BYTE key1,
                                    VADR addr2, int arn2, BYTE key2,
                                    int len, REGS* regs )
{
BYTE   *dest1,   *dest2;                /* Destination addresses     */
BYTE   *source1, *source2;              /* Source addresses          */
BYTE   *sk1,     *sk2;                  /* Storage key addresses     */
int     len1,     len2;                 /* Lengths to copy           */

    ITIMER_SYNC( addr2, len, regs );

    /* Quick out if copying just 1 byte */
    if (unlikely( !len ))
    {
        source1 = MADDR( addr2, arn2, regs, ACCTYPE_READ,  key2 );
        dest1   = MADDR( addr1, arn1, regs, ACCTYPE_WRITE, key1 );
        *dest1 = *source1;
        ITIMER_UPDATE( addr1, len, regs );
        return;
    }

    /* Translate addresses of leftmost operand bytes */
    source1 = MADDRL( addr2, len+1, arn2, regs, ACCTYPE_READ, key2 );
    dest1   = MADDRL( addr1, len+1, arn1, regs, ACCTYPE_WRITE_SKP, key1 );
    sk1 = regs->dat.storkey;

    /* There are several scenarios (in optimal order):
     * (1) dest boundary and source boundary not crossed
     * (2) dest boundary not crossed and source boundary crossed
     * (3) dest boundary crossed and source boundary not crossed
     * (4) dest boundary and source boundary are crossed
     *     (a) dest and source boundary cross at the same time
     *     (b) dest boundary crossed first
     *     (c) source boundary crossed first
     * Note: since the operand length is limited to 256 bytes,
     *       neither operand can cross more than one 2K boundary.
     */
    if (NOCROSSPAGE( addr1, len ))
    {
        if (NOCROSSPAGE( addr2, len ))
        {
            /* (1) - No boundaries are crossed */
            concpy( regs, dest1, source1, len + 1 );
        }
        else
        {
            /* (2) - Source operand crosses a boundary */
            len1 = PAGEFRAME_PAGESIZE - (addr2 & PAGEFRAME_BYTEMASK);
            source2 = MADDRL( (addr2 + len1) & ADDRESS_MAXWRAP( regs ),
                   len + 1 - len1, arn2, regs, ACCTYPE_READ, key2 );

            concpy( regs, dest1,        source1,       len1     );
            concpy( regs, dest1 + len1, source2, len - len1 + 1 );
        }
        *sk1 |= (STORKEY_REF | STORKEY_CHANGE);
    }
    else
    {
        /* Destination operand crosses a boundary */
        len1 = PAGEFRAME_PAGESIZE - (addr1 & PAGEFRAME_BYTEMASK);
        dest2 = MADDRL( (addr1 + len1) & ADDRESS_MAXWRAP( regs ),
            len + 1 - len1, arn1, regs, ACCTYPE_WRITE_SKP, key1 );
        sk2 = regs->dat.storkey;

        if (NOCROSSPAGE( addr2, len ))
        {
             /* (3) - Source operand crosses a boundary */
             concpy( regs, dest1, source1,              len1     );
             concpy( regs, dest2, source1 + len1, len - len1 + 1 );
        }
        else
        {
            /* (4) - Both operands cross a boundary */
            len2 = PAGEFRAME_PAGESIZE - (addr2 & PAGEFRAME_BYTEMASK);
            source2 = MADDRL( (addr2 + len2) & ADDRESS_MAXWRAP( regs ),
                len + 1 - len2, arn2, regs, ACCTYPE_READ, key2 );

            if (len1 == len2)
            {
                /* (4a) - Both operands cross at the same time */
                concpy( regs, dest1, source1,       len1     );
                concpy( regs, dest2, source2, len - len1 + 1 );
            }
            else if (len1 < len2)
            {
                /* (4b) - Destination operand crosses first */
                concpy( regs, dest1,               source1,               len1     );
                concpy( regs, dest2,               source1 + len1, len2 - len1     );
                concpy( regs, dest2 + len2 - len1, source2,        len  - len2 + 1 );
            }
            else
            {
                /* (4c) - Source operand crosses first */
                concpy( regs, dest1,        source1,                      len2     );
                concpy( regs, dest1 + len2, source2,               len1 - len2     );
                concpy( regs, dest2,        source2 + len1 - len2, len -  len1 + 1 );
            }
        }
        *sk1 |= (STORKEY_REF | STORKEY_CHANGE);
        *sk2 |= (STORKEY_REF | STORKEY_CHANGE);
    }
    ITIMER_UPDATE( addr1, len, regs );

} /* end function ARCH_DEP(move_chars) */

#if defined( FEATURE_061_MISC_INSTR_EXT_FACILITY_3 )
/*-------------------------------------------------------------------*/
/*                  Move characters right to left                    */
/*              using specified keys and address spaces              */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* This function is essentially identical to the above 'move_chars'  */
/* function except that it moves bytes in a right-to-left sequence   */
/* beginning with the rightmost byte of each operand. Both operands  */
/* designate the leftmost byte of their respective operands.         */
/*                                                                   */
/* Input:                                                            */
/*      addr1   Effective address of first operand                   */
/*      arn1    Access register number for first operand,            */
/*              or USE_PRIMARY_SPACE or USE_SECONDARY_SPACE          */
/*      key1    Bits 0-3=first operand access key, 4-7=zeroes        */
/*                                                                   */
/*                                                                   */
/*      addr2   Effective address of second operand                  */
/*      arn2    Access register number for second operand,           */
/*              or USE_PRIMARY_SPACE or USE_SECONDARY_SPACE          */
/*      key2    Bits 0-3=second operand access key, 4-7=zeroes       */
/*                                                                   */
/*                                                                   */
/*      len     Operand length minus 1 (range 0-255)                 */
/*      regs    Pointer to the CPU register context                  */
/*                                                                   */
/*                                                                   */
/*      This function implements the MVCRL instruction, and moves    */
/*      up to 256 bytes using the address space and key specified    */
/*      by the caller for each operand.  Data is moved byte by       */
/*      byte to ensure correct processing of overlapping operands.   */
/*                                                                   */
/*      The arn parameter for each operand may be an access          */
/*      register number, in which case the operand is in the         */
/*      primary, secondary, or home space, or in the space           */
/*      designated by the specified access register, according       */
/*      to the current PSW addressing mode.                          */
/*                                                                   */
/*      Alternatively the arn parameter may be one of the special    */
/*      values USE_PRIMARY_SPACE or USE_SECONDARY_SPACE in which     */
/*      case the operand is in the specified space regardless of     */
/*      the current PSW addressing mode.                             */
/*                                                                   */
/*      A program check may be generated if either logical address   */
/*      causes an addressing, protection, or translation exception,  */
/*      and in this case the function does not return.               */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( move_chars_rl )( VADR addr1, int arn1, BYTE key1,
                                       VADR addr2, int arn2, BYTE key2,
                                       int len, REGS* regs )
{
BYTE   *dest1,   *dest2;                /* Destination addresses     */
BYTE   *source1, *source2;              /* Source addresses          */
BYTE   *sk1,     *sk2;                  /* Storage key addresses     */
int     len1,     len2;                 /* Lengths to copy           */

    ITIMER_SYNC( addr2, len, regs );

    /* Quick out if copying just 1 byte */
    if (unlikely( !len ))
    {
        source1 = MADDR( addr2, arn2, regs, ACCTYPE_READ,  key2 );
        dest1   = MADDR( addr1, arn1, regs, ACCTYPE_WRITE, key1 );
        *dest1  = *source1;
        ITIMER_UPDATE( addr1, len, regs );
        return;
    }

    /* Translate addresses of leftmost operand bytes */
    source1 = MADDRL( addr2, len+1, arn2, regs, ACCTYPE_READ, key2 );
    dest1   = MADDRL( addr1, len+1, arn1, regs, ACCTYPE_WRITE_SKP, key1 );
    sk1 = regs->dat.storkey;

    /* There are several scenarios (in optimal order):
     * (1) dest boundary and source boundary not crossed
     * (2) dest boundary not crossed and source boundary crossed
     * (3) dest boundary crossed and source boundary not crossed
     * (4) dest boundary and source boundary are crossed
     *     (a) dest and source boundary cross at the same time
     *     (b) dest boundary crossed first
     *     (c) source boundary crossed first
     * Note: since the operand length is limited to 256 bytes,
     *       neither operand can cross more than one 2K boundary.
     */
    if (NOCROSSPAGE( addr1, len ))
    {
        if (NOCROSSPAGE( addr2, len ))
        {
            /* (1) - No boundaries are crossed */
            concpy_rl( regs, dest1, source1, len + 1 );
        }
        else
        {
            /* (2) - Source operand crosses a boundary */
            len1 = PAGEFRAME_PAGESIZE - (addr2 & PAGEFRAME_BYTEMASK);
            source2 = MADDRL( (addr2 + len1) & ADDRESS_MAXWRAP( regs ),
                   len + 1 - len1, arn2, regs, ACCTYPE_READ, key2 );

            concpy_rl( regs, dest1 + len1, source2, len - len1 + 1 );
            concpy_rl( regs, dest1,        source1,       len1     );
        }
        *sk1 |= (STORKEY_REF | STORKEY_CHANGE);
    }
    else
    {
        /* Destination operand crosses a boundary */
        len1 = PAGEFRAME_PAGESIZE - (addr1 & PAGEFRAME_BYTEMASK);
        dest2 = MADDRL( (addr1 + len1) & ADDRESS_MAXWRAP( regs ),
            len + 1 - len1, arn1, regs, ACCTYPE_WRITE_SKP, key1 );
        sk2 = regs->dat.storkey;

        if (NOCROSSPAGE( addr2, len ))
        {
             /* (3) - Source operand crosses a boundary */
             concpy_rl( regs, dest2, source1 + len1, len - len1 + 1 );
             concpy_rl( regs, dest1, source1,              len1     );
        }
        else
        {
            /* (4) - Both operands cross a boundary */
            len2 = PAGEFRAME_PAGESIZE - (addr2 & PAGEFRAME_BYTEMASK);
            source2 = MADDRL( (addr2 + len2) & ADDRESS_MAXWRAP( regs ),
                len + 1 - len2, arn2, regs, ACCTYPE_READ, key2 );

            if (len1 == len2)
            {
                /* (4a) - Both operands cross at the same time */
                concpy_rl( regs, dest2, source2, len - len1 + 1 );
                concpy_rl( regs, dest1, source1,       len1     );
            }
            else if (len1 < len2)
            {
                /* (4b) - Destination operand crosses first */
                concpy_rl( regs, dest2 + len2 - len1, source2,        len  - len2 + 1 );
                concpy_rl( regs, dest2,               source1 + len1, len2 - len1     );
                concpy_rl( regs, dest1,               source1,               len1     );
            }
            else
            {
                /* (4c) - Source operand crosses first */
                concpy_rl( regs, dest2,        source2 + len1 - len2, len -  len1 + 1 );
                concpy_rl( regs, dest1 + len2, source2,               len1 - len2     );
                concpy_rl( regs, dest1,        source1,                      len2     );
            }
        }
        *sk1 |= (STORKEY_REF | STORKEY_CHANGE);
        *sk2 |= (STORKEY_REF | STORKEY_CHANGE);
    }
    ITIMER_UPDATE( addr1, len, regs );

} /* end function ARCH_DEP( move_chars_rl ) */
#endif /* defined( FEATURE_061_MISC_INSTR_EXT_FACILITY_3 ) */

#if defined( FEATURE_027_MVCOS_FACILITY )
/*-------------------------------------------------------------------*/
/* Move characters with optional specifications                      */
/*                                                                   */
/* Input:                                                            */
/*      addr1   Effective address of first operand                   */
/*      space1  Address space for first operand:                     */
/*                 USE_PRIMARY_SPACE                                 */
/*                 USE_SECONDARY_SPACE                               */
/*                 USE_ARMODE + access register number               */
/*                 USE_HOME_SPACE                                    */
/*      key1    Bits 0-3=first operand access key, 4-7=zeroes        */
/*      addr2   Effective address of second operand                  */
/*      space1  Address space for second operand (values as space1)  */
/*      key2    Bits 0-3=second operand access key, 4-7=zeroes       */
/*      len     Operand length (range 0-4096)                        */
/*      regs    Pointer to the CPU register context                  */
/*                                                                   */
/*      This function implements the MVCOS instruction which moves   */
/*      up to 4096 characters using the address space and key        */
/*      specified by the caller for each operand.  Results are       */
/*      unpredictable if destructive overlap exists.                 */
/*                                                                   */
/*      The space1 and space2 parameters force the use of the        */
/*      specified address space, or the use of the specified         */
/*      access register, regardless of the current PSW addressing    */
/*      mode.                                                        */
/*                                                                   */
/*      A program check may be generated if either logical address   */
/*      causes an addressing, protection, or translation exception,  */
/*      and in this case the function does not return.               */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( move_charx )( VADR addr1, int space1, BYTE key1,
                                    VADR addr2, int space2, BYTE key2,
                                    int len, REGS* regs )
{
BYTE   *main1, *main2;                  /* Main storage pointers     */
int     len1, len2, len3;               /* Work areas for lengths    */

    /* Ultra quick out if copying zero bytes */
    if (unlikely( !len ))
        return;

    ITIMER_SYNC( addr2, len-1, regs );

    /* Quick out if copying just 1 byte */
    if (unlikely( len == 1 ))
    {
        main2 = MADDR( addr2, space2, regs, ACCTYPE_READ,  key2 );
        main1 = MADDR( addr1, space1, regs, ACCTYPE_WRITE, key1 );
        *main1 = *main2;
        ITIMER_UPDATE( addr1, len-1, regs );
        return;
    }

    /* Translate addresses of leftmost operand bytes */
    main2 = MADDRL( addr2, len, space2, regs, ACCTYPE_READ, key2 );
    main1 = MADDRL( addr1, len, space1, regs, ACCTYPE_WRITE, key1 );

    /* Copy the largest chunks which do not cross a page
       boundary of either source or destination operand */
    while (len > 0)
    {
        /* Calculate distance to next 2K boundary */
        len1 = NOCROSSPAGEL( addr1, len ) ? len : (int)(PAGEFRAME_PAGESIZE - (addr1 & PAGEFRAME_BYTEMASK));
        len2 = NOCROSSPAGEL( addr2, len ) ? len : (int)(PAGEFRAME_PAGESIZE - (addr2 & PAGEFRAME_BYTEMASK));
        len3 = len1 < len2 ? len1 : len2;

        /* Copy bytes from source to destination */
        concpy( regs, main1, main2, len3 );

        /* Calculate virtual addresses for next chunk */
        addr1 = (addr1 + len3) & ADDRESS_MAXWRAP( regs );
        addr2 = (addr2 + len3) & ADDRESS_MAXWRAP( regs );

        /* Adjust remaining length */
        len -= len3;

        /* Exit if no more bytes to move */
        if (len == 0) break;

        /* Adjust addresses for start of next chunk, or
           translate again if a 2K boundary was crossed */
        main2 = (addr2 & PAGEFRAME_BYTEMASK) ? main2 + len3 : MADDRL( addr2, len, space2, regs, ACCTYPE_READ,  key2 );
        main1 = (addr1 & PAGEFRAME_BYTEMASK) ? main1 + len3 : MADDRL( addr1, len, space1, regs, ACCTYPE_WRITE, key1 );

    } /* end while(len) */

    ITIMER_UPDATE( addr1, len-1, regs );

} /* end function ARCH_DEP( move_charx ) */
#endif /* defined( FEATURE_027_MVCOS_FACILITY ) */

/*-------------------------------------------------------------------*/
/* Validate operand for addressing, protection, translation          */
/*                                                                   */
/* Input:                                                            */
/*      addr    Effective address of operand                         */
/*      arn     Access register number                               */
/*      len     Operand length minus 1 (range 0-255)                 */
/*      acctype Type of access requested: READ or WRITE              */
/*      regs    Pointer to the CPU register context                  */
/*                                                                   */
/*      The purpose of this function is to allow an instruction      */
/*      operand to be validated for addressing, protection, and      */
/*      translation exceptions, thus allowing the instruction to     */
/*      be nullified or suppressed before any updates occur.         */
/*                                                                   */
/*      A program check is generated if the operand causes an        */
/*      addressing, protection, or translation exception, and        */
/*      in this case the function does not return.                   */
/*-------------------------------------------------------------------*/
// (needed by dyncrypt.c)
VSTORE_INL_DLL_IMPORT inline void ARCH_DEP( validate_operand )( VADR addr, int arn, int len,
                                          int acctype, REGS* regs )
{
    /* Translate address of leftmost operand byte */
    MADDR( addr, arn, regs, acctype, regs->psw.pkey );

    /* Translate next page if boundary crossed */
    if (CROSSPAGE( addr, len ))
    {
        MADDR( (addr + len) & ADDRESS_MAXWRAP( regs ),
               arn, regs, acctype, regs->psw.pkey );
    }
#if defined( FEATURE_INTERVAL_TIMER )
    else
        ITIMER_SYNC( addr, len, regs );
#endif
}

/*-------------------------------------------------------------------*/
/*  We only need to compile this header ONCE for each architecture!  */
/*-------------------------------------------------------------------*/

#if      ARCH_370_IDX == ARCH_IDX
  #define DID_370_VSTORE_H
#endif

#if      ARCH_390_IDX == ARCH_IDX
  #define DID_390_VSTORE_H
#endif

#if      ARCH_900_IDX == ARCH_IDX
  #define DID_900_VSTORE_H
#endif

#endif // #if (ARCH_xxx_IDX == ARCH_IDX && !defined( DID_xxx_DAT_H )) ...

/* end of VSTORE.H */

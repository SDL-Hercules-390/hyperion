/* SIE.H        (C) Copyright Roger Bowler, 1994-2012                */
/*              (C) and others 2013-2021                             */
/*              SIE structures, constants and functions              */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/* Format-1 State Descriptor Block (SIE1BK) documented in:           */
/*  SA22-7095-1 System/370 XA Interpretive Execution                 */

/* Format-1 State Descriptor Block (SIE1BK) documented at:           */
/*  https://www.vm.ibm.com/pubs/cp240/SIEBK.HTML                     */

/* Format-2 State Descriptor Block (SIE2BK) documented at:           */
/*  http://www.vm.ibm.com/pubs/cp710/SIEBK.HTML                      */

/* The RCP (Reference and Change Preservation) table and how it's    */
/* used is documented on page 20 of SA22-7095-1 "System/370 XA       */
/* Interpretive Execution", on page 541 of "IBM Journal of Research  */
/* and Development" vol. 27, no. 6 (November 1983) in article        */
/* "System/370 XA: Facilities for Virtual Machines", by P.H.Gum,     */
/* as well as in US Patent 4,792,895 "INSTRUCTION PROCESSING IN      */
/* HIGHER LEVEL VIRTUAL MACHINES BY A REAL MACHINE" (1988)           */

#ifndef _SIE_H
#define _SIE_H

/*-------------------------------------------------------------------*/
/*  non-ARCH_DEP section of header: due to above _SIE_H guard,       */
/*  the below header section is compiled only ONCE, *before*         */
/*  the very first architecture is ever built.                       */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*                System Control Area (SCABK )                       */
/*-------------------------------------------------------------------*/
/*       https://www.vm.ibm.com/pubs/cp710/SCABK.HTML                */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  The SCABK maps the architected System Control Area for SIE,      */
/*  used in support of virtual multiprocessing. NOTE: SIEISCAA       */
/*  (i.e. field "scao" in Hercules's SIE1BK/SIE2BK structs) does     */
/*  NOT point to the front of the block. Rather, it points to        */
/*  the TRUE start of the SCA, field SCASTART (i.e. scaiplk0).       */
/*  SCAIPLOK is a formal lock.                                       */
/*                                                                   */
/*  NOTE: Not all fields of the SCABK are interesting or needed      */
/*  by Hercules.  Only those fields actually needed by Hercules      */
/*  are defined.                                                     */
/*                                                                   */
/*-------------------------------------------------------------------*/
struct SCABK                /* System Control Area block             */
{
    BYTE  scaiplk0;         /* IPTE interlock flag     (lock byte)   */
#define   SCAIPLKH   0x80   /* IPTE interlock is held  (lock held)   */
};
typedef struct SCABK SCABK;

/*-------------------------------------------------------------------*/
/*       Reference and Change Preservation (RCP) Table Entry         */
/*-------------------------------------------------------------------*/
/*         https://www.vm.ibm.com/pubs/cp43064/RCPTE.HTML            */
/*         https://www.vm.ibm.com/pubs/cp720/RCPTE.HTML              */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  The RCP (Reference and Change Preservation) area is used to      */
/*  allow the hardware reference and change bits for a single        */
/*  real page to record reference and change information for use     */
/*  by both the guest and by the host.                               */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  In order that only one CPU is accessing a particular RCP area    */
/*  byte at a time (either by SIE or by z/VM simulation), a lock     */
/*  bit is defined in the RCP area byte.  Prior to starting any      */
/*  operation involving the RCP area, a lock is obtained on the      */
/*  RCP area byte by attempting to compare-and-swap the lock bit.    */
/*  If SIE is unable to obtain the lock, an instruction intercept    */
/*  occurs so that z/VM can then deal with the situation itself.     */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  When SIE executes a guest Set Storage Key (SSK) instruction,     */
/*  the original value of the R/C bits in the real page are first    */
/*  saved by OR'ing them into the host R/C bits in the RCP area      */
/*  and the real R/C bits in the real page are then set to zero.     */
/*  The requested new guest R/C bits are placed into the RCP byte.   */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  When SIE executes a guest Reset Reference Bit (RRB) instruction, */
/*  the condition code is determined from the logical 'OR' of the    */
/*  the real page's R/C bits and the guest's R/C bits from the RCP   */
/*  area byte for that page.  The original value of the reference    */
/*  bit in the real page is then saved by OR'ing it into the host    */
/*  RCP area byte, and then both the reference bit the real page     */
/*  and the guest's bit in the RCP area byte are both set to zero.   */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  When SIE executes a guest Insert Storage Key (ISK) instruction,  */
/*  the guest is given the key from real storage, plus the R/C bits  */
/*  from that real page OR'ed with the guest backup R/C bits from    */
/*  the RCP area byte for that page.                                 */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  If z/VM modifies the R/C bits for a page in real storage that    */
/*  corresponds to a guest page, z/VM will logically OR the R/C      */
/*  bits from real storage into the guest backup R/C bits for that   */
/*  page prior to modifying the real storage R/C bits.               */
/*                                                                   */
/*-------------------------------------------------------------------*/
struct RCPTE
{
    BYTE  rcpbyte;          /* Hardware RCP byte for one 4K page     */

#define RCPLOCK     0x80    /* RCP lock held                         */

#define RCPHREF     0x40    /* Host backup reference bit             */
#define RCPHCH      0x20    /* Host backup change bit                */
#define RCPHOST     0x60    /* RCPHREF+RCPHCH mask for host bits     */

#define RCPGREF     0x04    /* Guest backup reference bit            */
#define RCPGCH      0x02    /* Guest backup change bit               */
#define RCPGUEST    0x06    /* RCPGREF+RCPGCH mask for guest bits    */

};
typedef struct RCPTE RCPTE;

/*-------------------------------------------------------------------*/
/*                Page Management Block (PGMBK)                      */
/*-------------------------------------------------------------------*/
/*       https://www.vm.ibm.com/pubs/cp42032/PGMBK.HTML              */
/*       https://www.vm.ibm.com/pubs/cp720/pgm64.html                */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  The Page Management Block describes 1 MB of guest virtual        */
/*  storage.  It contains the page table, the page status table,     */
/*  and the auxiliary storage address table for the megabyte.        */
/*                                                                   */
/*  The PGMBK is a 4K aligned 4K block for ESA/390 mode guests.      */
/*  For z/Arch mode guests, the PGMBK is a 4K aligned 8K block.      */
/*                                                                   */
/*  NOTE: Not all fields of the PGMBK are interesting or needed      */
/*  by Hercules.  Only those fields actually needed by Hercules      */
/*  are defined.                                                     */
/*                                                                   */
/*-------------------------------------------------------------------*/
struct PGMBK
{
    FWORD  pgmpagtb[ 256 ]; /* ESA/390 guest Page Table: contains    */
                            /* 256 4-byte entries used to describe   */
                            /* each 4K of ESA/390 guest virtual      */
                            /* storage. The complete 256-entry       */
                            /* table describes 1 megabyte of guest   */
                            /* absolute storage.                     */

    FWORD  pgmpgstb[ 256 ]; /* Page Status Table. 256 entries of 4   */
                            /* bytes in each entry, describing the   */
                            /* status of each of the above 4K pages. */
                            /* Each 4-byte Page Status Table Entry   */
                            /* is described by the PGSTE struct.     */
};
typedef struct PGMBK PGMBK;

struct PGMBK64
{
    DBLWRD pgmpagtb[ 256 ]; /* z/Arch guest Page Table: contains     */
                            /* 256 8-byte entries used to describe   */
                            /* each 4K of z/Arch guest virtual       */
                            /* storage. The complete 256-entry       */
                            /* table describes 1 megabyte of guest   */
                            /* absolute storage.                     */

    DBLWRD pgmpgstb[ 256 ]; /* Page Status Table. 256 entries of 8   */
                            /* bytes in each entry, describing the   */
                            /* status of each of the above 4K pages. */
                            /* Each 8-byte Page Status Table Entry   */
                            /* is described by the PGSTE struct.     */
};
typedef struct PGMBK64 PGMBK64;

/*-------------------------------------------------------------------*/
/*                Page Status Table Entry (PGSTE)                    */
/*-------------------------------------------------------------------*/
/*         https://www.vm.ibm.com/pubs/cp240/PGSTE.HTML              */
//         https://www.vm.ibm.com/pubs/cp720/pgs64.html              */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  The PGSTE describes the status of one 4K page of guest virtual   */
/*  storage.  There are 256 contiguous Page Status Table Entries.    */
/*  Any specific table entry can be obtained by extracting the page  */
/*  number (bits 12-19) of a guest's virtual address, multiplying    */
/*  that page number by 4 or 8, and adding the result to PGMBK       */
/*  struct field pgmpgstb.                                           */
/*                                                                   */
/*  NOTE: Not all fields of the PGSTE are interesting or needed      */
/*  by Hercules.  Only those fields actually needed by Hercules      */
/*  are defined.  For example, the size of each PGSTE entry is       */
/*  4 bytes for ESA/390 guests whereas for z/Arch guests the         */
/*  size is 8 bytes.  Luckily the first 2 bytes are identical        */
/*  for both, which are the only 2 bytes Hercules really needs.      */
/*                                                                   */
/*-------------------------------------------------------------------*/
struct PGSTE
{
    BYTE   pgsvkey;         /* Guest storage key bits 0-4            */

#define PGSVKACC    0xF0    /* Access-control key value              */
#define PGSVKFET    0x08    /* Fetch-protection bit                  */
#define PGSVKACF    0xF8    /* PGSVKACC + PGSVKFET                   */

    RCPTE  pgsrcp;          /* RCP byte if SKA is being utilized     */
                            /* (Storage Key Assist) or the page      */
                            /* belongs to the system.  Refer to      */
                            /* struct RCPTE for the layout.          */
};
typedef struct PGSTE PGSTE;

#endif // _SIE_H        // (end of NON-archdep code)

//-------------------------------------------------------------------
//                      ARCH_DEP() code
//-------------------------------------------------------------------
// ARCH_DEP (build-architecture / FEATURE-dependent) functions here.
// All BUILD architecture dependent (ARCH_DEP) function are compiled
// multiple times (once for each defined build architecture) and each
// time they are compiled with a different set of FEATURE_XXX defines
// appropriate for that architecture. Use #ifdef FEATURE_XXX guards
// to check whether the current BUILD architecture has that given
// feature #defined for it or not. WARNING: Do NOT use _FEATURE_XXX.
// The underscore feature #defines mean something else entirely. Only
// test for FEATURE_XXX. (WITHOUT the underscore)
//-------------------------------------------------------------------

/*-------------------------------------------------------------------*/
/*                    LockUnlockSCALock                              */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*                     *** IMPORTANT! ***                            */
/*                                                                   */
/*  If the instruction calling this function requires obtaining      */
/*  INTLOCK (e.g. SYNCHRONIZE_CPUS), then it must do so BEFORE       */
/*  calling this function! If the instruction is incorrectly         */
/*  coded to try obtaining INTLOCK *after* calling this function,    */
/*  then a DEADLOCK can very easily occur! YOU HAVE BEEN WARNED!     */
/*                                                                   */
/*-------------------------------------------------------------------*/
inline bool ARCH_DEP( LockUnlockSCALock )( REGS* regs, bool lock, bool trylock )
{
    BYTE new;
    bool obtained = false;
    SCABK* scabk = (SCABK*) &regs->mainstor[ regs->sie_scao ];

    if (lock)
    {
        BYTE old = scabk->scaiplk0;

        // MAINLOCK may be required if cmpxchg assists unavailable
        OBTAIN_MAINLOCK( regs );
        {
            // If not TRY call, keep looping until we obtain it.
            // Otherwise TRY just once, and return success or not.
            do
            {
                old &= ~SCAIPLKH;       // Want bit to be initially off
                new = (old | SCAIPLKH); // And *WE* want to turn it on!
            }
            while (!(obtained = (0 == cmpxchg1( &old, new, &scabk->scaiplk0 ))) && !trylock);
        }
        RELEASE_MAINLOCK( regs );
    }
    else // (unlock)
    {
        // Atomically 'and' the lock bit off
        new = (BYTE)(~SCAIPLKH);
        (void) H_ATOMIC_OP( &scabk->scaiplk0, new, and, And, & );
    }

    // Set Reference and Change bit for the byte we just modified
    {
        U64 abspage = (U64) (&scabk->scaiplk0 - regs->mainstor);
        ARCH_DEP( or_4K_storage_key )( abspage, (STORKEY_REF | STORKEY_CHANGE) );
    }

    return obtained;
}

#undef  OBTAIN_SCALOCK
#undef  TRY_OBTAIN_SCALOCK
#undef  RELEASE_SCALOCK

#define OBTAIN_SCALOCK(     regs )  ARCH_DEP( LockUnlockSCALock )( (regs), true,  false )
#define TRY_OBTAIN_SCALOCK( regs )  ARCH_DEP( LockUnlockSCALock )( (regs), true,  true  )
#define RELEASE_SCALOCK(    regs )  ARCH_DEP( LockUnlockSCALock )( (regs), false, false )

/*-------------------------------------------------------------------*/
/*                     LockUnlockRCPLock                             */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*                     *** IMPORTANT! ***                            */
/*                                                                   */
/*  If the instruction calling this function requires obtaining      */
/*  INTLOCK (e.g. SYNCHRONIZE_CPUS), then it must do so BEFORE       */
/*  calling this function! If the instruction is incorrectly         */
/*  coded to try obtaining INTLOCK *after* calling this function,    */
/*  then a DEADLOCK can very easily occur! YOU HAVE BEEN WARNED!     */
/*                                                                   */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( LockUnlockRCPLock )( REGS* regs, RCPTE* rcpte, bool lock )
{
    BYTE new;

    if (lock)
    {
        BYTE old = rcpte->rcpbyte;

        // MAINLOCK may be required if cmpxchg assists unavailable
        OBTAIN_MAINLOCK( regs );
        {
            // Keep looping until we eventually obtain it...
            do
            {
                old &= ~RCPLOCK;        // Want bit to be initially off
                new = (old | RCPLOCK);  // And *WE* want to turn it on!
            }
            while (cmpxchg1( &old, new, &rcpte->rcpbyte ) != 0);
        }
        RELEASE_MAINLOCK( regs );
    }
    else // (unlock)
    {
        // Atomically 'and' the lock bit off
        new = (BYTE)(~RCPLOCK);
        (void) H_ATOMIC_OP( &rcpte->rcpbyte, new, and, And, & );
    }

    // Set Reference and Change bit for the byte we just modified
    {
        U64 abspage = (U64) (((BYTE*)rcpte) - regs->mainstor);
        ARCH_DEP( or_4K_storage_key )( abspage, (STORKEY_REF | STORKEY_CHANGE) );
    }
}


#if defined( OPTION_USE_SKAIP_AS_LOCK )
/*-------------------------------------------------------------------*/
/*                    LockUnlockSKALock                              */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*                     *** IMPORTANT! ***                            */
/*                                                                   */
/*  If the instruction calling this function requires obtaining      */
/*  INTLOCK (e.g. SYNCHRONIZE_CPUS), then it must do so BEFORE       */
/*  calling this function! If the instruction is incorrectly         */
/*  coded to try obtaining INTLOCK *after* calling this function,    */
/*  then a DEADLOCK can very easily occur! YOU HAVE BEEN WARNED!     */
/*                                                                   */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( LockUnlockSKALock )( REGS* regs, bool lock )
{
    BYTE new;

    if (lock)
    {
        BYTE old = regs->siebk->SIE_RCPO0;

        // MAINLOCK may be required if cmpxchg assists unavailable
        OBTAIN_MAINLOCK( regs );
        {
            // Keep looping until we eventually obtain it...
            do
            {
                old &= ~SIE_RCPO0_SKAIP;        // Want bit to be initially off
                new = (old | SIE_RCPO0_SKAIP);  // And *WE* want to turn it on!
            }
            while (cmpxchg1( &old, new, &regs->siebk->SIE_RCPO0 ) != 0);
        }
        RELEASE_MAINLOCK( regs );
    }
    else // (unlock)
    {
        // Atomically 'and' the lock bit off
        new = ~SIE_RCPO0_SKAIP;
        (void) H_ATOMIC_OP( &regs->siebk->SIE_RCPO0, new, and, And, & );
    }

    // Set Reference and Change bit for the byte we just modified
    {
        U64 abspage = (U64) (((BYTE*)regs->siebk) - regs->mainstor);
        ARCH_DEP( or_4K_storage_key )( abspage, (STORKEY_REF | STORKEY_CHANGE) );
    }
}
#endif // defined( OPTION_USE_SKAIP_AS_LOCK )


/*-------------------------------------------------------------------*/
/*                      LockUnlockKeyLock                            */
/*-------------------------------------------------------------------*/
/*                                                                   */
#if defined( OPTION_USE_SKAIP_AS_LOCK )
/*                                                                   */
/*  Obtain or Release the VM Storage Key Lock. The value of the      */
/*  'pgste' argument that is passed to this function determines      */
/*  whether Storage Key Assist is active or not, and thus whether    */
/*  the new SKA method should be used or the old RCP table byte      */
/*  method. The new SKA method locks the entire PGSTE table via      */
/*  the SIE state block SKAIP (Storage Key Assist in Progress)       */
/*  flag. Otherwise (original RCP table byte method) the X'80'       */
/*  bit of the original RCP table byte is the lock bit.              */
/*                                                                   */
#else // !defined( OPTION_USE_SKAIP_AS_LOCK )
/*                                                                   */
/*  Obtain or Release the VM Storage Key Lock. The VM Storage Key    */
/*  Lock is the high-order X'80' bit of the RCP table byte holding   */
/*  the host and guest Reference and Change bits corresponding to    */
/*  the guest page you are accessing.                                */
/*                                                                   */
#endif // defined( OPTION_USE_SKAIP_AS_LOCK )
/*                                                                   */
/*                     *** IMPORTANT! ***                            */
/*                                                                   */
/*  If the instruction calling this function requires obtaining      */
/*  INTLOCK (e.g. SYNCHRONIZE_CPUS), then it must do so BEFORE       */
/*  calling this function! If the instruction is incorrectly         */
/*  coded to try obtaining INTLOCK *after* calling this function,    */
/*  then a DEADLOCK can very easily occur! YOU HAVE BEEN WARNED!     */
/*                                                                   */
/*-------------------------------------------------------------------*/

inline void ARCH_DEP( LockUnlockKeyLock )( REGS* regs, PGSTE* pgste, RCPTE* rcpte, bool lock )
{
#if defined( OPTION_USE_SKAIP_AS_LOCK )
    if (pgste)
        ARCH_DEP( LockUnlockSKALock )( regs, lock );
    else
#endif
        ARCH_DEP( LockUnlockRCPLock )( regs, rcpte, lock );
}

#undef  OBTAIN_KEYLOCK
#undef  RELEASE_KEYLOCK

#define OBTAIN_KEYLOCK(  pgste, rcpte, regs )   ARCH_DEP( LockUnlockKeyLock )( (regs), (pgste), (rcpte), true )
#define RELEASE_KEYLOCK( pgste, rcpte, regs )   ARCH_DEP( LockUnlockKeyLock )( (regs), (pgste), (rcpte), false )

/*-------------------------------------------------------------------*/
/*        Return pointer to PGSTE based on host abs PTE              */
/*-------------------------------------------------------------------*/
inline PGSTE* ARCH_DEP( GetPGSTEFromPTE )( REGS* regs, U64 pte )
{
    PGSTE*  pgste;
    size_t  pte_offset_to_pgste;

    // Calculate offset to PGSTE (depends if host is z/VM or VM/ESA)
    pte_offset_to_pgste = (ARCH_900_IDX == HOSTREGS->arch_mode)
        ? offsetof( PGMBK64, pgmpgstb )   // (z/Arch z/VM)
        : offsetof( PGMBK,   pgmpgstb );  // (S/390 VM/ESA)

    // Return mainstor address of PGSTE
    pgste = (PGSTE*)&regs->mainstor[ pte + pte_offset_to_pgste ];

    return pgste;
}

/*-------------------------------------------------------------------*/
/*     Return pointer to PGSTE based on guest page address           */
/*-------------------------------------------------------------------*/
inline PGSTE* ARCH_DEP( GetPGSTE )( REGS* regs, U64 gabspage )
{
    PGSTE*  pgste;
    U64     pte;

    // Guest absolute to host PTE addr
    if (SIE_TRANSLATE_ADDR( regs->sie_mso + gabspage,
                            USE_PRIMARY_SPACE,
                            HOSTREGS, ACCTYPE_PTE ))
        SIE_INTERCEPT( regs );

    // Convert host real address to host absolute address
    pte = apply_host_prefixing( HOSTREGS, HOSTREGS->dat.raddr );

    // Convert host abs PTE to PGSTE
    pgste = ARCH_DEP( GetPGSTEFromPTE )( regs, pte );
    return pgste;
}

/*-------------------------------------------------------------------*/
/* Return pointer to old RCP using old SIE state block rcpo field    */
/*-------------------------------------------------------------------*/
inline RCPTE* ARCH_DEP( GetOldRCP )( REGS* regs, U64 gabspage )
{
    /* Obtain address of the RCP area from the state desc */
    RADR rcpa = regs->sie_rcpo;

    /* Use page index as byte offset into RCP table */
    rcpa += (gabspage >> STORKEY_KEY_4K_SHIFTAMT);

    /* Translate host primary address to host absolute */
    rcpa = SIE_LOGICAL_TO_ABS( rcpa, USE_PRIMARY_SPACE,
                               HOSTREGS, ACCTYPE_SIE, 0 );

    /* Return mainstor address of RCP table byte */
    return (RCPTE*)(&regs->mainstor[ rcpa ]);
}

/*-------------------------------------------------------------------*/
/* Return pointer to PGSTE and RCPTE based on guest abs page address */
/*-------------------------------------------------------------------*/
inline void ARCH_DEP( GetPGSTE_and_RCPTE )( REGS* regs, U64 gabspage, PGSTE** ppPGSTE, RCPTE** ppRCPTE )
{
#if defined( _FEATURE_STORAGE_KEY_ASSIST )
    if (0
        || SIE_STATE_BIT_ON( regs, RCPO0, SKA )
#if defined( _FEATURE_ZSIE )
        // SKA is always active for z/VM
        || ARCH_900_IDX == HOSTREGS->arch_mode
#endif
    )
    {
        /* SKA: Get pointer to both PGSTE and RCPTE */
        *ppPGSTE = ARCH_DEP( GetPGSTE )( regs, gabspage );
        *ppRCPTE = &(*ppPGSTE)->pgsrcp;
    }
    else // (NOT SKA...)
#endif /* defined( _FEATURE_STORAGE_KEY_ASSIST ) */
    {
#if defined( FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
        if (SIE_STATE_BIT_ON( regs, MX, XC ))
            SIE_INTERCEPT( regs );
#endif
        /* Get just the pointer to the old RCP table byte */
        *ppPGSTE = NULL;
        *ppRCPTE = ARCH_DEP( GetOldRCP )( regs, gabspage );
    }
}

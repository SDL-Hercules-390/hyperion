/* DAT.H        (C) Copyright Roger Bowler, 1999-2012                */
/*              ESA/390 Dynamic Address Translation                  */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/*   ARCH_DEP section: compiled multiple times, once for each arch.  */
/*   that use this header file.                                      */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*                           maddr_l                                 */
/* For compatibility, it is usually invoked using the MADDRL macro   */
/* in feature.h                                                      */
/*-------------------------------------------------------------------*/
/* Convert logical address to absolute address.  This is the DAT     */
/* logic that does an accelerated TLB lookup to return the prev-     */
/* iously determined value from an earlier translation for this      */
/* logical address.  It performs a series of checks to ensure the    */
/* values that were used in the previous translation (the results    */
/* of which are in the corresponding TLB entry) haven't changed      */
/* for the current address being translated.  If any of the cond-    */
/* itions have changed (i.e. if any of the comparisons fail) then    */
/* the TLB cannot be used (TLB miss) and "logical_to_main_l" is      */
/* called to perform a full address translation.  Otherwise if all   */
/* of the conditions are still true (nothing has changed from the    */
/* the last time we translated this address), then the previously    */
/* translated address from the TLB is returned instead (TLB hit).    */
/*                                                                   */
/* PLEASE NOTE that the address that is retrieved from the TLB is    */
/* an absolute address from the Hercules guest's point of view but   */
/* the address RETURNED TO THE CALLER is a Hercules host address     */
/* pointing to MAINSTOR that Hercules can then directly use.         */
/*                                                                   */
/* Input:                                                            */
/*      addr    Logical address to be translated                     */
/*      len     Length of data access for PER SA purpose             */
/*      arn     Access register number (or USE_REAL_ADDR,            */
/*              USE_PRIMARY_SPACE, USE_SECONDARY_SPACE)              */
/*      regs    Pointer to the CPU register context                  */
/*      acctype Type of access requested: READ, WRITE, INSTFETCH,    */
/*              LRA, IVSK, TPROT, STACK, PTE, LPTEA                  */
/*      akey    Bits 0-3=access key, 4-7=zeroes                      */
/*                                                                   */
/* Returns:                                                          */
/*      Directly usable guest absolute storage MAINADDR address.     */
/*                                                                   */
/*-------------------------------------------------------------------*/
static inline  BYTE* ARCH_DEP( maddr_l )
    ( VADR addr, size_t len, const int arn, REGS* regs, const int acctype, const BYTE akey )
{
    /* Note: ALL of the below conditions must be true for a TLB hit
       to occur.  If ANY of them are false, then it's a TLB miss,
       requiring us to then perform a full DAT address translation.

       Note too that on the grand scheme of things the order/sequence
       of the below tests (if statements) is completely unimportant
       since ALL conditions must be checked anyway in order for a hit
       to occur, and it doesn't matter that a miss tests a few extra
       conditions since it's going to do a full translation anyway!
       (which is many, many instructions)
    */

    int  aea_arn  = regs->AEA_AR( arn );
    U16  tlbix    = TLBIX( addr );
    BYTE *maddr = NULL;

    /* Non-zero AEA Access Register number? */
    if (aea_arn)
    {
        /* Same Addess Space Designator as before? */
        /* Or if not, is address in a common segment? */
        if (0
            || (regs->CR( aea_arn ) == regs->tlb.TLB_ASD( tlbix ))
            || (regs->AEA_COMMON( aea_arn ) & regs->tlb.common[ tlbix ])
        )
        {
            /* Storage Key zero? */
            /* Or if not, same Storage Key as before? */
            if (0
                || akey == 0
                || akey == regs->tlb.skey[ tlbix ]
            )
            {
                /* Does the page address match the one in the TLB? */
                /* (does a TLB entry exist for this page address?) */
                if (
                    ((addr & TLBID_PAGEMASK) | regs->tlbID)
                    ==
                    regs->tlb.TLB_VADDR( tlbix )
                )
                {
                    /* Is storage being accessed same way as before? */
                    if (acctype & regs->tlb.acc[ tlbix ])
                    {
                        /*------------------------------------------*/
                        /* TLB hit: use previously translated value */
                        /*------------------------------------------*/

                        if (acctype & ACC_CHECK)
                            regs->dat.storkey = regs->tlb.storkey[ tlbix ];

                        maddr = MAINADDR(regs->tlb.main[tlbix], addr);
                    }
                }
            }
        }
    }

    /*---------------------------------------*/
    /* TLB miss: do full address translation */
    /*---------------------------------------*/
    if (!maddr)
        maddr = ARCH_DEP( logical_to_main_l )( addr, arn, regs, acctype, akey, len );

#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
    if (FACILITY_ENABLED( 073_TRANSACT_EXEC, regs ))
    {
        /* SA22-7832-12 Principles of Operation, page 5-99:

             "Storage accesses for instruction and DAT- and ART-
              table fetches follow the non-transactional rules."
        */
        if (0
            || arn == USE_INST_SPACE    /* Instruction fetching */
            || arn == USE_REAL_ADDR     /* Address translation  */
        )
            return maddr;               /* return ACTUAL address */

        /* Quick exit if no CPUs executing any transactions */
        if (!sysblk.txf_transcpus)
            return maddr;

        /* Quick exit if NTSTG call */
        if (regs && regs->txf_NTSTG)
        {
            regs->txf_NTSTG = false;
            return maddr;
        }

        /* Translate to alternate TXF address if appropriate */
        maddr = TXF_MADDRL( addr, len, arn, regs, acctype, maddr );
    }
#endif

    return maddr;
}

#if defined( FEATURE_DUAL_ADDRESS_SPACE )

U16  ARCH_DEP( translate_asn )( U16 asn, REGS* regs, U32* asteo, U32 aste[] );
int  ARCH_DEP( authorize_asn )( U16 ax, U32 aste[], int atemask, REGS* regs );

#endif

/* end of DAT.H */

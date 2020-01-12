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
    ( VADR addr, size_t len, int arn, REGS* regs, int acctype, BYTE akey )
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
    BYTE *rtnaddr = NULL;

#if defined (FEATURE_073_TRANSACT_EXEC_FACILITY)
#if _GEN_ARCH == 900
    BYTE *altaddr;
    BYTE *savepage;
    BYTE *pageaddr;
    BYTE *altpage;
    BYTE *savepagec;
    BYTE *altpagec;
    BYTE *pageaddrc;
    U64  cacheidx;
    U64  cacheidxe;
    U64  pageoffs;
    U64  pageoffe;
    U64  addrwork;
    U64  addrpage;
    U64  plen;
    U64  elen;
    int pagecap;
    TPAGEMAP *pmap;
    BYTE newacc;
    int abortcode;
    int i;
    int j;
    int k;
    REGS *rchk;
    REGS *hregs;
#endif
#endif

    /* Non-zero AEA Access Register number? */
    regs->hostregs->tranlastaccess = acctype;
    regs->hostregs->tranlastarn = arn;
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
                        rtnaddr = MAINADDR(regs->tlb.main[tlbix], addr);
                    }
                }
            }
        }
    }
    /*---------------------------------------*/
    /* TLB miss: do full address translation */
    /*---------------------------------------*/
    if (!rtnaddr)
      rtnaddr = ARCH_DEP( logical_to_main_l )( addr, arn, regs, acctype, akey, len );

#if defined (FEATURE_073_TRANSACT_EXEC_FACILITY)
/*------------------------------------------------------------------*/
/*  The following code supports the transaction execution facility. */
/*  If the CPU is in transaction execution mode (constrained or     */
/*  unconstrained), the actual real storage address is saved in the */
/*  transaction data area, and then it is mapped to an alternate    */
/*  address.  The contents of the real page are also saved.  The    */
/*  cache line within the page is marked as having been fetched or  */
/*  stored.   The first time that a cache line is accessed, it is   */
/*  captured from the real page.  When a cache line or page is      */
/*  captured, two copies are made.  One copy is presented to the    */
/*  caller, and one is a save copy which will be used to see if the */
/*  cache line has changed.  In order to make sure that the capture */
/*  is clean, the two copies must match.  If they do not match,     */
/*  the copy is retried up to 128 times.  If a copy cannot be made  */
/*  in that many tries, the transaction is aborted with a fetch     */
/*  conflict.  Note that it is OK to use real addresses here because*/
/*  the transaction will be aborted if the real page is invalidated.*/
/*------------------------------------------------------------------*/

    if (sysblk.txf_transcpus == 0)      /* no cpus are in tran mode */
      return rtnaddr;                   /* return now               */

    /*  we are only interested in fetch and store access  */
    if (acctype != ACCTYPE_READ && acctype != ACCTYPE_WRITE && acctype != ACCTYPE_WRITE_SKP)
      return rtnaddr;

    if (arn == USE_INST_SPACE || arn == USE_REAL_ADDR)  /* should only be set for instruction fetch */
      return rtnaddr;     

    hregs = regs->hostregs;
    addrwork = (U64)rtnaddr;                    
    pageoffs = addrwork & PAGEFRAME_BYTEMASK;       
    cacheidx = pageoffs >> ZCACHE_LINE_SHIFT;  
    addrpage = addrwork & PAGEFRAME_PAGEMASK;  

    /* find the length to the end of the page */
    plen = ZCACHE_PAGE_SIZE - (addr & PAGEFRAME_BYTEMASK);

    /* if the length to the end of the page is less than the length passed */
    /* reset to the end of page length */

    if (plen < len)
      elen = plen;
    else
      elen = len;

    pageoffe = pageoffs + elen; 
    cacheidxe = pageoffe >> ZCACHE_LINE_SHIFT;

/*------------------------------------------------------------------*/
/*   if any cpus are in transaction mode, we need to see if this    */
/*   cpu is storing or fetching into a cache line that has been     */
/*   touched by a cpu in transaction mode.  If that is the case,    */
/*   we will flag the other cpu for abort.  The actual abort will   */
/*   happen the next time the transaction cpu references storage or */
/*   gets to transaction end.                                       */
/*                                                                  */
/*   The process of moving CPUS in and out of transaction mode is   */
/*   serialized, so that state will not change during the check.    */
/*   The scan of the cache lines for the cpu is serialized by       */
/*   obtaining the cpu lock for that cpu.  The cpu lock is also     */
/*   obtained to update the pagemap and/or cachemap.                */
/*------------------------------------------------------------------*/

    for (i=0; i < sysblk.hicpu; i++)
    {
      if (sysblk.regs[i]->hostregs == hregs)        /* skip my entry       */
        continue;

      rchk = sysblk.regs[i];             /* point to other cpu entry */

      if (rchk->tranlvl == 0)            /* this cpu not in transaction mode */
        continue;                        /* skip it */

      if (rchk->abortcode)               /* abort already detected */
        continue;                        /* no need to check it */

      if (rchk->cpustate != CPUSTATE_STARTED)  /* skip cpus not started*/
        continue;

      abortcode = 0;                     /* clear abort code */
      pmap = rchk->tpagemap;

      for (j=0; j < rchk->tranpagenum; j++, pmap++)  /* scan the page map  */
      {
        if ((U64)pmap->mainpageaddr == addrpage)  /* same page ?*/
        {
          for (k = cacheidx; k <= cacheidxe; k++)
          {
            if (pmap->cachemap[k] == CM_CLEAN)  /* no conflict */
              continue;

            if (pmap->cachemap[k] == CM_FETCHED)  /* transactional reference was fetch */
            {
              if (acctype == ACCTYPE_READ)  /* current access also read */
                continue;                /* if both are fetch, not a conflict */
              abortcode = ABORT_CODE_FETCH_CNF;       /* set fetch conflict */
            }
            else
              abortcode = ABORT_CODE_STORE_CNF;      /* store conflict */

            /* the real routine is used here instead of obtain_lock to get around a problem */
            /* compiling dyn76.c, which redefines obtain_lock as EnterCriticalSection */

            hthread_obtain_lock(&rchk->sysblk->txf_lock[i], PTT_LOC );  /*get the cpu lock */
            {
              if (rchk->tranlvl > 0)
              {
                rchk->conflictaddr = hregs->psw.ia.D - hregs->psw.ilc;
                rchk->abortcode = abortcode;
              }
            }
            hthread_release_lock(&rchk->sysblk->txf_lock[i], PTT_LOC );  /* release the cpu lock */
            break;
          }

          if (abortcode)                   /* conflict found, stop scanning */
            break;
        }
      }
    }

    if (hregs->tranlvl == 0)              /* tranlvl will always be zero if the transaction */
      return rtnaddr;                    /* facility is not enabled. */

    /* if an abort has already been requested, call the abort code now */
    if (hregs->abortcode)
      ARCH_DEP(abort_transaction)(hregs, ABORT_RETRY_CC, hregs->abortcode);

    /*------------------------------------------------------------*/
    /*   We will return an alternate real address to the caller,  */
    /*   which will be visible only to this cpu.  When/if the     */
    /*   transaction is commited, the alternate page will be      */
    /*   copied to the real page, as long as there are no changes */
    /*   to the cache lines we touched. The size of the cache     */
    /*   line is 256 bytes.  We will mark all cache lines that    */
    /*   were touched (fetched or modified).  All cache lines     */
    /*   that were touched must not have changed in the original  */
    /*   page, or the transaction will abort.   We can safely use */
    /*   real addresses, because if a page fault occurs, the      */
    /*   transaction will abort and we will start over.           */
    /*   In most cases, the length passed to MADDRL is correct,   */
    /*   but in some cases the source length is one byte instead  */
    /*   of the true length.  In those cases,  we must check to   */
    /*   see if more than one cache line is crossed.              */
    /*------------------------------------------------------------*/

    /*------------------------------------------------------------*/
    /*  See if we have already captured this page, if not, we     */
    /*  will capture the real page and also save a copy.  The     */
    /*  copy will be used to determine if unexpected changes have */
    /*  been made at commit time.                                 */
    /*------------------------------------------------------------*/

    altpage = 0;
    pmap = hregs->tpagemap;

    for (i = 0; i < hregs->tranpagenum; i++, pmap++)
    {
      /*  if this page has already been mapped, us it*/
      if (addrpage == (U64)pmap->mainpageaddr)
      {
        altpage = pmap->altpageaddr;
        pagecap = 0;        /* page not captured */
        break;
      }
    }

    if (!altpage)
    {
      if (hregs->tranpagenum >= MAX_TXF_PAGES)
      {
        if (acctype == ACCTYPE_READ)
          ARCH_DEP(abort_transaction)(hregs, ABORT_RETRY_PGMCHK, ABORT_CODE_FETCH_OVF);
        else
          ARCH_DEP(abort_transaction)(hregs, ABORT_RETRY_PGMCHK, ABORT_CODE_STORE_OVF);
      }

      pmap = &hregs->tpagemap[hregs->tranpagenum];
      altpage = pmap->altpageaddr;
      savepage = altpage + ZCACHE_PAGE_SIZE;
      pageaddr = (BYTE *)addrpage;

      for (i=0; i < MAX_CAPTURE_TRIES; i++)
      {
        memcpy(altpage, pageaddr, ZCACHE_PAGE_SIZE);
        memcpy(savepage, pageaddr, ZCACHE_PAGE_SIZE);

        if (memcmp(altpage, savepage, ZCACHE_PAGE_SIZE) == 0)
          break;
      }

      if (i >= MAX_CAPTURE_TRIES)
        ARCH_DEP(abort_transaction)(hregs, ABORT_RETRY_CC, ABORT_CODE_FETCH_CNF);

      pagecap = 1;
      pmap->mainpageaddr = (BYTE *)addrpage;
      hregs->tranpagenum++;
    }

    altaddr = altpage + pageoffs;

    if (acctype == ACCTYPE_READ)
      newacc = CM_FETCHED;
    else
      newacc = CM_STORED;

    for (; cacheidx <= cacheidxe; cacheidx++)
    {
      switch (pmap->cachemap[cacheidx])
      {
      case CM_CLEAN:

        /* if the cache line has not been touched, refresh it */
        pageaddrc = pmap->mainpageaddr + (cacheidx << ZCACHE_LINE_SHIFT);
        altpagec = pmap->altpageaddr + (cacheidx << ZCACHE_LINE_SHIFT);
        savepagec = altpagec + ZCACHE_PAGE_SIZE;

        for (i=0; i < MAX_CAPTURE_TRIES; i++)
        {
          memcpy(altpagec, pageaddrc, ZCACHE_LINE_SIZE);
          memcpy(savepagec, pageaddrc, ZCACHE_LINE_SIZE);

          if (memcmp(altpagec, savepagec, ZCACHE_LINE_SIZE) == 0)
            break;
        }

        if (i >= MAX_CAPTURE_TRIES)
          ARCH_DEP(abort_transaction)(regs, ABORT_RETRY_CC, ABORT_CODE_FETCH_CNF);

        pmap->cachemap[cacheidx] = newacc;
        break;

      case CM_FETCHED:
        if (newacc == CM_CLEAN)
          break;

        pmap->cachemap[cacheidx] = newacc;
        break;

      case CM_STORED:
        break;
      }
    }
    return altaddr;
#else
    return rtnaddr;
#endif
}

#if defined( FEATURE_DUAL_ADDRESS_SPACE )
U16 ARCH_DEP(translate_asn) (U16 asn, REGS *regs,
        U32 *asteo, U32 aste[]);
int ARCH_DEP(authorize_asn) (U16 ax, U32 aste[],
        int atemask, REGS *regs);
#endif

/* end of DAT.H */

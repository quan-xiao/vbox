/* $Id: NEMAllNativeTemplate-win.cpp.h 86117 2020-09-14 08:03:03Z vboxsync $ */
/** @file
 * NEM - Native execution manager, Windows code template ring-0/3.
 */

/*
 * Copyright (C) 2018-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** Copy back a segment from hyper-V. */
#define NEM_WIN_COPY_BACK_SEG(a_Dst, a_Src) \
            do { \
                (a_Dst).u64Base  = (a_Src).Base; \
                (a_Dst).u32Limit = (a_Src).Limit; \
                (a_Dst).ValidSel = (a_Dst).Sel = (a_Src).Selector; \
                (a_Dst).Attr.u   = (a_Src).Attributes; \
                (a_Dst).fFlags   = CPUMSELREG_FLAGS_VALID; \
            } while (0)

/** @def NEMWIN_ASSERT_MSG_REG_VAL
 * Asserts the correctness of a register value in a message/context.
 */
#if 0
# define NEMWIN_NEED_GET_REGISTER
# if defined(IN_RING0) || defined(NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS)
#  define NEMWIN_ASSERT_MSG_REG_VAL(a_pVCpu, a_enmReg, a_Expr, a_Msg) \
    do { \
        HV_REGISTER_VALUE TmpVal; \
        nemHCWinGetRegister(a_pVCpu, a_enmReg, &TmpVal); \
        AssertMsg(a_Expr, a_Msg); \
    } while (0)
# else
#  define NEMWIN_ASSERT_MSG_REG_VAL(a_pVCpu, a_enmReg, a_Expr, a_Msg) \
    do { \
        WHV_REGISTER_VALUE TmpVal; \
        nemR3WinGetRegister(a_pVCpu, a_enmReg, &TmpVal); \
        AssertMsg(a_Expr, a_Msg); \
    } while (0)
# endif
#else
# define NEMWIN_ASSERT_MSG_REG_VAL(a_pVCpu, a_enmReg, a_Expr, a_Msg) do { } while (0)
#endif

/** @def NEMWIN_ASSERT_MSG_REG_VAL
 * Asserts the correctness of a 64-bit register value in a message/context.
 */
#define NEMWIN_ASSERT_MSG_REG_VAL64(a_pVCpu, a_enmReg, a_u64Val) \
    NEMWIN_ASSERT_MSG_REG_VAL(a_pVCpu, a_enmReg, (a_u64Val) == TmpVal.Reg64, \
                              (#a_u64Val "=%#RX64, expected %#RX64\n", (a_u64Val), TmpVal.Reg64))
/** @def NEMWIN_ASSERT_MSG_REG_VAL
 * Asserts the correctness of a segment register value in a message/context.
 */
#define NEMWIN_ASSERT_MSG_REG_SEG(a_pVCpu, a_enmReg, a_SReg) \
    NEMWIN_ASSERT_MSG_REG_VAL(a_pVCpu, a_enmReg, \
                                 (a_SReg).Base       == TmpVal.Segment.Base \
                              && (a_SReg).Limit      == TmpVal.Segment.Limit \
                              && (a_SReg).Selector   == TmpVal.Segment.Selector \
                              && (a_SReg).Attributes == TmpVal.Segment.Attributes, \
                              ( #a_SReg "=%#RX16 {%#RX64 LB %#RX32,%#RX16} expected %#RX16 {%#RX64 LB %#RX32,%#RX16}\n", \
                               (a_SReg).Selector, (a_SReg).Base, (a_SReg).Limit, (a_SReg).Attributes, \
                               TmpVal.Segment.Selector, TmpVal.Segment.Base, TmpVal.Segment.Limit, TmpVal.Segment.Attributes))


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** NEM_WIN_PAGE_STATE_XXX names. */
NEM_TMPL_STATIC const char * const g_apszPageStates[4] = { "not-set", "unmapped", "readable", "writable" };

/** HV_INTERCEPT_ACCESS_TYPE names. */
static const char * const g_apszHvInterceptAccessTypes[4] = { "read", "write", "exec", "!undefined!" };


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
NEM_TMPL_STATIC int nemHCNativeSetPhysPage(PVMCC pVM, PVMCPUCC pVCpu, RTGCPHYS GCPhysSrc, RTGCPHYS GCPhysDst,
                                           uint32_t fPageProt, uint8_t *pu2State, bool fBackingChanged);



#ifdef NEM_WIN_USE_HYPERCALLS_FOR_PAGES

/**
 * Wrapper around VMMR0_DO_NEM_MAP_PAGES for a single page.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the caller.
 * @param   GCPhysSrc   The source page.  Does not need to be page aligned.
 * @param   GCPhysDst   The destination page.  Same as @a GCPhysSrc except for
 *                      when A20 is disabled.
 * @param   fFlags      HV_MAP_GPA_XXX.
 */
DECLINLINE(int) nemHCWinHypercallMapPage(PVMCC pVM, PVMCPUCC pVCpu, RTGCPHYS GCPhysSrc, RTGCPHYS GCPhysDst, uint32_t fFlags)
{
#ifdef IN_RING0
    /** @todo optimize further, caller generally has the physical address. */
    return nemR0WinMapPages(pVM, pVCpu,
                            GCPhysSrc & ~(RTGCPHYS)X86_PAGE_OFFSET_MASK,
                            GCPhysDst & ~(RTGCPHYS)X86_PAGE_OFFSET_MASK,
                            1, fFlags);
#else
    pVCpu->nem.s.Hypercall.MapPages.GCPhysSrc   = GCPhysSrc & ~(RTGCPHYS)X86_PAGE_OFFSET_MASK;
    pVCpu->nem.s.Hypercall.MapPages.GCPhysDst   = GCPhysDst & ~(RTGCPHYS)X86_PAGE_OFFSET_MASK;
    pVCpu->nem.s.Hypercall.MapPages.cPages      = 1;
    pVCpu->nem.s.Hypercall.MapPages.fFlags      = fFlags;
    return VMMR3CallR0Emt(pVM, pVCpu, VMMR0_DO_NEM_MAP_PAGES, 0, NULL);
#endif
}


/**
 * Wrapper around VMMR0_DO_NEM_UNMAP_PAGES for a single page.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the caller.
 * @param   GCPhys      The page to unmap.  Does not need to be page aligned.
 */
DECLINLINE(int) nemHCWinHypercallUnmapPage(PVMCC pVM, PVMCPUCC pVCpu, RTGCPHYS GCPhys)
{
# ifdef IN_RING0
    return nemR0WinUnmapPages(pVM, pVCpu, GCPhys & ~(RTGCPHYS)X86_PAGE_OFFSET_MASK, 1);
# else
    pVCpu->nem.s.Hypercall.UnmapPages.GCPhys    = GCPhys & ~(RTGCPHYS)X86_PAGE_OFFSET_MASK;
    pVCpu->nem.s.Hypercall.UnmapPages.cPages    = 1;
    return VMMR3CallR0Emt(pVM, pVCpu, VMMR0_DO_NEM_UNMAP_PAGES, 0, NULL);
# endif
}

#endif /* NEM_WIN_USE_HYPERCALLS_FOR_PAGES */
#ifndef IN_RING0

NEM_TMPL_STATIC int nemHCWinCopyStateToHyperV(PVMCC pVM, PVMCPUCC pVCpu)
{
# if defined(NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS) || defined(NEM_WIN_WITH_RING0_RUNLOOP)
#  if !defined(NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS) && defined(NEM_WIN_WITH_RING0_RUNLOOP)
    if (pVM->nem.s.fUseRing0Runloop)
#  endif
    {
        int rc = VMMR3CallR0Emt(pVM, pVCpu, VMMR0_DO_NEM_EXPORT_STATE, 0, NULL);
        AssertLogRelRCReturn(rc, rc);
        return rc;
    }
# endif
# ifndef NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS

    /*
     * The following is very similar to what nemR0WinExportState() does.
     */
    WHV_REGISTER_NAME  aenmNames[128];
    WHV_REGISTER_VALUE aValues[128];

    uint64_t const fWhat = ~pVCpu->cpum.GstCtx.fExtrn & (CPUMCTX_EXTRN_ALL | CPUMCTX_EXTRN_NEM_WIN_MASK);
    if (   !fWhat
        && pVCpu->nem.s.fCurrentInterruptWindows == pVCpu->nem.s.fDesiredInterruptWindows)
        return VINF_SUCCESS;
    uintptr_t iReg = 0;

#  define ADD_REG64(a_enmName, a_uValue) do { \
            aenmNames[iReg]      = (a_enmName); \
            aValues[iReg].Reg128.High64 = 0; \
            aValues[iReg].Reg64  = (a_uValue); \
            iReg++; \
        } while (0)
#  define ADD_REG128(a_enmName, a_uValueLo, a_uValueHi) do { \
            aenmNames[iReg] = (a_enmName); \
            aValues[iReg].Reg128.Low64  = (a_uValueLo); \
            aValues[iReg].Reg128.High64 = (a_uValueHi); \
            iReg++; \
        } while (0)

    /* GPRs */
    if (fWhat & CPUMCTX_EXTRN_GPRS_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_RAX)
            ADD_REG64(WHvX64RegisterRax, pVCpu->cpum.GstCtx.rax);
        if (fWhat & CPUMCTX_EXTRN_RCX)
            ADD_REG64(WHvX64RegisterRcx, pVCpu->cpum.GstCtx.rcx);
        if (fWhat & CPUMCTX_EXTRN_RDX)
            ADD_REG64(WHvX64RegisterRdx, pVCpu->cpum.GstCtx.rdx);
        if (fWhat & CPUMCTX_EXTRN_RBX)
            ADD_REG64(WHvX64RegisterRbx, pVCpu->cpum.GstCtx.rbx);
        if (fWhat & CPUMCTX_EXTRN_RSP)
            ADD_REG64(WHvX64RegisterRsp, pVCpu->cpum.GstCtx.rsp);
        if (fWhat & CPUMCTX_EXTRN_RBP)
            ADD_REG64(WHvX64RegisterRbp, pVCpu->cpum.GstCtx.rbp);
        if (fWhat & CPUMCTX_EXTRN_RSI)
            ADD_REG64(WHvX64RegisterRsi, pVCpu->cpum.GstCtx.rsi);
        if (fWhat & CPUMCTX_EXTRN_RDI)
            ADD_REG64(WHvX64RegisterRdi, pVCpu->cpum.GstCtx.rdi);
        if (fWhat & CPUMCTX_EXTRN_R8_R15)
        {
            ADD_REG64(WHvX64RegisterR8, pVCpu->cpum.GstCtx.r8);
            ADD_REG64(WHvX64RegisterR9, pVCpu->cpum.GstCtx.r9);
            ADD_REG64(WHvX64RegisterR10, pVCpu->cpum.GstCtx.r10);
            ADD_REG64(WHvX64RegisterR11, pVCpu->cpum.GstCtx.r11);
            ADD_REG64(WHvX64RegisterR12, pVCpu->cpum.GstCtx.r12);
            ADD_REG64(WHvX64RegisterR13, pVCpu->cpum.GstCtx.r13);
            ADD_REG64(WHvX64RegisterR14, pVCpu->cpum.GstCtx.r14);
            ADD_REG64(WHvX64RegisterR15, pVCpu->cpum.GstCtx.r15);
        }
    }

    /* RIP & Flags */
    if (fWhat & CPUMCTX_EXTRN_RIP)
        ADD_REG64(WHvX64RegisterRip, pVCpu->cpum.GstCtx.rip);
    if (fWhat & CPUMCTX_EXTRN_RFLAGS)
        ADD_REG64(WHvX64RegisterRflags, pVCpu->cpum.GstCtx.rflags.u);

    /* Segments */
#  define ADD_SEG(a_enmName, a_SReg) \
        do { \
            aenmNames[iReg]                  = a_enmName; \
            aValues[iReg].Segment.Base       = (a_SReg).u64Base; \
            aValues[iReg].Segment.Limit      = (a_SReg).u32Limit; \
            aValues[iReg].Segment.Selector   = (a_SReg).Sel; \
            aValues[iReg].Segment.Attributes = (a_SReg).Attr.u; \
            iReg++; \
        } while (0)
    if (fWhat & CPUMCTX_EXTRN_SREG_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_ES)
            ADD_SEG(WHvX64RegisterEs,   pVCpu->cpum.GstCtx.es);
        if (fWhat & CPUMCTX_EXTRN_CS)
            ADD_SEG(WHvX64RegisterCs,   pVCpu->cpum.GstCtx.cs);
        if (fWhat & CPUMCTX_EXTRN_SS)
            ADD_SEG(WHvX64RegisterSs,   pVCpu->cpum.GstCtx.ss);
        if (fWhat & CPUMCTX_EXTRN_DS)
            ADD_SEG(WHvX64RegisterDs,   pVCpu->cpum.GstCtx.ds);
        if (fWhat & CPUMCTX_EXTRN_FS)
            ADD_SEG(WHvX64RegisterFs,   pVCpu->cpum.GstCtx.fs);
        if (fWhat & CPUMCTX_EXTRN_GS)
            ADD_SEG(WHvX64RegisterGs,   pVCpu->cpum.GstCtx.gs);
    }

    /* Descriptor tables & task segment. */
    if (fWhat & CPUMCTX_EXTRN_TABLE_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_LDTR)
            ADD_SEG(WHvX64RegisterLdtr, pVCpu->cpum.GstCtx.ldtr);
        if (fWhat & CPUMCTX_EXTRN_TR)
            ADD_SEG(WHvX64RegisterTr,   pVCpu->cpum.GstCtx.tr);
        if (fWhat & CPUMCTX_EXTRN_IDTR)
        {
            aenmNames[iReg] = WHvX64RegisterIdtr;
            aValues[iReg].Table.Limit = pVCpu->cpum.GstCtx.idtr.cbIdt;
            aValues[iReg].Table.Base  = pVCpu->cpum.GstCtx.idtr.pIdt;
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_GDTR)
        {
            aenmNames[iReg] = WHvX64RegisterGdtr;
            aValues[iReg].Table.Limit = pVCpu->cpum.GstCtx.gdtr.cbGdt;
            aValues[iReg].Table.Base  = pVCpu->cpum.GstCtx.gdtr.pGdt;
            iReg++;
        }
    }

    /* Control registers. */
    if (fWhat & CPUMCTX_EXTRN_CR_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_CR0)
            ADD_REG64(WHvX64RegisterCr0, pVCpu->cpum.GstCtx.cr0);
        if (fWhat & CPUMCTX_EXTRN_CR2)
            ADD_REG64(WHvX64RegisterCr2, pVCpu->cpum.GstCtx.cr2);
        if (fWhat & CPUMCTX_EXTRN_CR3)
            ADD_REG64(WHvX64RegisterCr3, pVCpu->cpum.GstCtx.cr3);
        if (fWhat & CPUMCTX_EXTRN_CR4)
            ADD_REG64(WHvX64RegisterCr4, pVCpu->cpum.GstCtx.cr4);
    }
    if (fWhat & CPUMCTX_EXTRN_APIC_TPR)
        ADD_REG64(WHvX64RegisterCr8, CPUMGetGuestCR8(pVCpu));

    /* Debug registers. */
/** @todo fixme. Figure out what the hyper-v version of KVM_SET_GUEST_DEBUG would be. */
    if (fWhat & CPUMCTX_EXTRN_DR0_DR3)
    {
        ADD_REG64(WHvX64RegisterDr0, pVCpu->cpum.GstCtx.dr[0]); // CPUMGetHyperDR0(pVCpu));
        ADD_REG64(WHvX64RegisterDr1, pVCpu->cpum.GstCtx.dr[1]); // CPUMGetHyperDR1(pVCpu));
        ADD_REG64(WHvX64RegisterDr2, pVCpu->cpum.GstCtx.dr[2]); // CPUMGetHyperDR2(pVCpu));
        ADD_REG64(WHvX64RegisterDr3, pVCpu->cpum.GstCtx.dr[3]); // CPUMGetHyperDR3(pVCpu));
    }
    if (fWhat & CPUMCTX_EXTRN_DR6)
        ADD_REG64(WHvX64RegisterDr6, pVCpu->cpum.GstCtx.dr[6]); // CPUMGetHyperDR6(pVCpu));
    if (fWhat & CPUMCTX_EXTRN_DR7)
        ADD_REG64(WHvX64RegisterDr7, pVCpu->cpum.GstCtx.dr[7]); // CPUMGetHyperDR7(pVCpu));

    /* Floating point state. */
    if (fWhat & CPUMCTX_EXTRN_X87)
    {
        ADD_REG128(WHvX64RegisterFpMmx0, pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[0].au64[0], pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[0].au64[1]);
        ADD_REG128(WHvX64RegisterFpMmx1, pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[1].au64[0], pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[1].au64[1]);
        ADD_REG128(WHvX64RegisterFpMmx2, pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[2].au64[0], pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[2].au64[1]);
        ADD_REG128(WHvX64RegisterFpMmx3, pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[3].au64[0], pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[3].au64[1]);
        ADD_REG128(WHvX64RegisterFpMmx4, pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[4].au64[0], pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[4].au64[1]);
        ADD_REG128(WHvX64RegisterFpMmx5, pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[5].au64[0], pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[5].au64[1]);
        ADD_REG128(WHvX64RegisterFpMmx6, pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[6].au64[0], pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[6].au64[1]);
        ADD_REG128(WHvX64RegisterFpMmx7, pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[7].au64[0], pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[7].au64[1]);

        aenmNames[iReg] = WHvX64RegisterFpControlStatus;
        aValues[iReg].FpControlStatus.FpControl = pVCpu->cpum.GstCtx.pXStateR3->x87.FCW;
        aValues[iReg].FpControlStatus.FpStatus  = pVCpu->cpum.GstCtx.pXStateR3->x87.FSW;
        aValues[iReg].FpControlStatus.FpTag     = pVCpu->cpum.GstCtx.pXStateR3->x87.FTW;
        aValues[iReg].FpControlStatus.Reserved  = pVCpu->cpum.GstCtx.pXStateR3->x87.FTW >> 8;
        aValues[iReg].FpControlStatus.LastFpOp  = pVCpu->cpum.GstCtx.pXStateR3->x87.FOP;
        aValues[iReg].FpControlStatus.LastFpRip = (pVCpu->cpum.GstCtx.pXStateR3->x87.FPUIP)
                                                | ((uint64_t)pVCpu->cpum.GstCtx.pXStateR3->x87.CS << 32)
                                                | ((uint64_t)pVCpu->cpum.GstCtx.pXStateR3->x87.Rsrvd1 << 48);
        iReg++;

        aenmNames[iReg] = WHvX64RegisterXmmControlStatus;
        aValues[iReg].XmmControlStatus.LastFpRdp            = (pVCpu->cpum.GstCtx.pXStateR3->x87.FPUDP)
                                                            | ((uint64_t)pVCpu->cpum.GstCtx.pXStateR3->x87.DS << 32)
                                                            | ((uint64_t)pVCpu->cpum.GstCtx.pXStateR3->x87.Rsrvd2 << 48);
        aValues[iReg].XmmControlStatus.XmmStatusControl     = pVCpu->cpum.GstCtx.pXStateR3->x87.MXCSR;
        aValues[iReg].XmmControlStatus.XmmStatusControlMask = pVCpu->cpum.GstCtx.pXStateR3->x87.MXCSR_MASK; /** @todo ??? (Isn't this an output field?) */
        iReg++;
    }

    /* Vector state. */
    if (fWhat & CPUMCTX_EXTRN_SSE_AVX)
    {
        ADD_REG128(WHvX64RegisterXmm0,  pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 0].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 0].uXmm.s.Hi);
        ADD_REG128(WHvX64RegisterXmm1,  pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 1].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 1].uXmm.s.Hi);
        ADD_REG128(WHvX64RegisterXmm2,  pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 2].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 2].uXmm.s.Hi);
        ADD_REG128(WHvX64RegisterXmm3,  pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 3].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 3].uXmm.s.Hi);
        ADD_REG128(WHvX64RegisterXmm4,  pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 4].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 4].uXmm.s.Hi);
        ADD_REG128(WHvX64RegisterXmm5,  pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 5].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 5].uXmm.s.Hi);
        ADD_REG128(WHvX64RegisterXmm6,  pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 6].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 6].uXmm.s.Hi);
        ADD_REG128(WHvX64RegisterXmm7,  pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 7].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 7].uXmm.s.Hi);
        ADD_REG128(WHvX64RegisterXmm8,  pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 8].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 8].uXmm.s.Hi);
        ADD_REG128(WHvX64RegisterXmm9,  pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 9].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 9].uXmm.s.Hi);
        ADD_REG128(WHvX64RegisterXmm10, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[10].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[10].uXmm.s.Hi);
        ADD_REG128(WHvX64RegisterXmm11, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[11].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[11].uXmm.s.Hi);
        ADD_REG128(WHvX64RegisterXmm12, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[12].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[12].uXmm.s.Hi);
        ADD_REG128(WHvX64RegisterXmm13, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[13].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[13].uXmm.s.Hi);
        ADD_REG128(WHvX64RegisterXmm14, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[14].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[14].uXmm.s.Hi);
        ADD_REG128(WHvX64RegisterXmm15, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[15].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[15].uXmm.s.Hi);
    }

    /* MSRs */
    // WHvX64RegisterTsc - don't touch
    if (fWhat & CPUMCTX_EXTRN_EFER)
        ADD_REG64(WHvX64RegisterEfer, pVCpu->cpum.GstCtx.msrEFER);
    if (fWhat & CPUMCTX_EXTRN_KERNEL_GS_BASE)
        ADD_REG64(WHvX64RegisterKernelGsBase, pVCpu->cpum.GstCtx.msrKERNELGSBASE);
    if (fWhat & CPUMCTX_EXTRN_SYSENTER_MSRS)
    {
        ADD_REG64(WHvX64RegisterSysenterCs, pVCpu->cpum.GstCtx.SysEnter.cs);
        ADD_REG64(WHvX64RegisterSysenterEip, pVCpu->cpum.GstCtx.SysEnter.eip);
        ADD_REG64(WHvX64RegisterSysenterEsp, pVCpu->cpum.GstCtx.SysEnter.esp);
    }
    if (fWhat & CPUMCTX_EXTRN_SYSCALL_MSRS)
    {
        ADD_REG64(WHvX64RegisterStar, pVCpu->cpum.GstCtx.msrSTAR);
        ADD_REG64(WHvX64RegisterLstar, pVCpu->cpum.GstCtx.msrLSTAR);
        ADD_REG64(WHvX64RegisterCstar, pVCpu->cpum.GstCtx.msrCSTAR);
        ADD_REG64(WHvX64RegisterSfmask, pVCpu->cpum.GstCtx.msrSFMASK);
    }
    if (fWhat & CPUMCTX_EXTRN_OTHER_MSRS)
    {
        ADD_REG64(WHvX64RegisterApicBase, APICGetBaseMsrNoCheck(pVCpu));
        ADD_REG64(WHvX64RegisterPat, pVCpu->cpum.GstCtx.msrPAT);
#if 0 /** @todo check if WHvX64RegisterMsrMtrrCap works here... */
        ADD_REG64(WHvX64RegisterMsrMtrrCap, CPUMGetGuestIa32MtrrCap(pVCpu));
#endif
        PCPUMCTXMSRS pCtxMsrs = CPUMQueryGuestCtxMsrsPtr(pVCpu);
        ADD_REG64(WHvX64RegisterMsrMtrrDefType, pCtxMsrs->msr.MtrrDefType);
        ADD_REG64(WHvX64RegisterMsrMtrrFix64k00000, pCtxMsrs->msr.MtrrFix64K_00000);
        ADD_REG64(WHvX64RegisterMsrMtrrFix16k80000, pCtxMsrs->msr.MtrrFix16K_80000);
        ADD_REG64(WHvX64RegisterMsrMtrrFix16kA0000, pCtxMsrs->msr.MtrrFix16K_A0000);
        ADD_REG64(WHvX64RegisterMsrMtrrFix4kC0000,  pCtxMsrs->msr.MtrrFix4K_C0000);
        ADD_REG64(WHvX64RegisterMsrMtrrFix4kC8000,  pCtxMsrs->msr.MtrrFix4K_C8000);
        ADD_REG64(WHvX64RegisterMsrMtrrFix4kD0000,  pCtxMsrs->msr.MtrrFix4K_D0000);
        ADD_REG64(WHvX64RegisterMsrMtrrFix4kD8000,  pCtxMsrs->msr.MtrrFix4K_D8000);
        ADD_REG64(WHvX64RegisterMsrMtrrFix4kE0000,  pCtxMsrs->msr.MtrrFix4K_E0000);
        ADD_REG64(WHvX64RegisterMsrMtrrFix4kE8000,  pCtxMsrs->msr.MtrrFix4K_E8000);
        ADD_REG64(WHvX64RegisterMsrMtrrFix4kF0000,  pCtxMsrs->msr.MtrrFix4K_F0000);
        ADD_REG64(WHvX64RegisterMsrMtrrFix4kF8000,  pCtxMsrs->msr.MtrrFix4K_F8000);
        ADD_REG64(WHvX64RegisterTscAux, pCtxMsrs->msr.TscAux);
#if 0 /** @todo these registers aren't available? Might explain something.. .*/
        const CPUMCPUVENDOR enmCpuVendor = CPUMGetHostCpuVendor(pVM);
        if (enmCpuVendor != CPUMCPUVENDOR_AMD)
        {
            ADD_REG64(HvX64RegisterIa32MiscEnable, pCtxMsrs->msr.MiscEnable);
            ADD_REG64(HvX64RegisterIa32FeatureControl, CPUMGetGuestIa32FeatureControl(pVCpu));
        }
#endif
    }

    /* event injection (clear it). */
    if (fWhat & CPUMCTX_EXTRN_NEM_WIN_EVENT_INJECT)
        ADD_REG64(WHvRegisterPendingInterruption, 0);

    /* Interruptibility state.  This can get a little complicated since we get
       half of the state via HV_X64_VP_EXECUTION_STATE. */
    if (   (fWhat & (CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT | CPUMCTX_EXTRN_NEM_WIN_INHIBIT_NMI))
        ==          (CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT | CPUMCTX_EXTRN_NEM_WIN_INHIBIT_NMI) )
    {
        ADD_REG64(WHvRegisterInterruptState, 0);
        if (   VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS)
            && EMGetInhibitInterruptsPC(pVCpu) == pVCpu->cpum.GstCtx.rip)
            aValues[iReg - 1].InterruptState.InterruptShadow = 1;
        if (VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_BLOCK_NMIS))
            aValues[iReg - 1].InterruptState.NmiMasked = 1;
    }
    else if (fWhat & CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT)
    {
        if (   pVCpu->nem.s.fLastInterruptShadow
            || (   VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS)
                && EMGetInhibitInterruptsPC(pVCpu) == pVCpu->cpum.GstCtx.rip))
        {
            ADD_REG64(WHvRegisterInterruptState, 0);
            if (   VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS)
                && EMGetInhibitInterruptsPC(pVCpu) == pVCpu->cpum.GstCtx.rip)
                aValues[iReg - 1].InterruptState.InterruptShadow = 1;
            /** @todo Retrieve NMI state, currently assuming it's zero. (yes this may happen on I/O) */
            //if (VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_BLOCK_NMIS))
            //    aValues[iReg - 1].InterruptState.NmiMasked = 1;
        }
    }
    else
        Assert(!(fWhat & CPUMCTX_EXTRN_NEM_WIN_INHIBIT_NMI));

    /* Interrupt windows. Always set if active as Hyper-V seems to be forgetful. */
    uint8_t const fDesiredIntWin = pVCpu->nem.s.fDesiredInterruptWindows;
    if (   fDesiredIntWin
        || pVCpu->nem.s.fCurrentInterruptWindows != fDesiredIntWin)
    {
        pVCpu->nem.s.fCurrentInterruptWindows = pVCpu->nem.s.fDesiredInterruptWindows;
        ADD_REG64(WHvX64RegisterDeliverabilityNotifications, fDesiredIntWin);
        Assert(aValues[iReg - 1].DeliverabilityNotifications.NmiNotification == RT_BOOL(fDesiredIntWin & NEM_WIN_INTW_F_NMI));
        Assert(aValues[iReg - 1].DeliverabilityNotifications.InterruptNotification == RT_BOOL(fDesiredIntWin & NEM_WIN_INTW_F_REGULAR));
        Assert(aValues[iReg - 1].DeliverabilityNotifications.InterruptPriority == (unsigned)((fDesiredIntWin & NEM_WIN_INTW_F_PRIO_MASK) >> NEM_WIN_INTW_F_PRIO_SHIFT));
    }

    /// @todo WHvRegisterPendingEvent

    /*
     * Set the registers.
     */
    Assert(iReg < RT_ELEMENTS(aValues));
    Assert(iReg < RT_ELEMENTS(aenmNames));
#  ifdef NEM_WIN_INTERCEPT_NT_IO_CTLS
    Log12(("Calling WHvSetVirtualProcessorRegisters(%p, %u, %p, %u, %p)\n",
           pVM->nem.s.hPartition, pVCpu->idCpu, aenmNames, iReg, aValues));
#  endif
    HRESULT hrc = WHvSetVirtualProcessorRegisters(pVM->nem.s.hPartition, pVCpu->idCpu, aenmNames, iReg, aValues);
    if (SUCCEEDED(hrc))
    {
        pVCpu->cpum.GstCtx.fExtrn |= CPUMCTX_EXTRN_ALL | CPUMCTX_EXTRN_NEM_WIN_MASK | CPUMCTX_EXTRN_KEEPER_NEM;
        return VINF_SUCCESS;
    }
    AssertLogRelMsgFailed(("WHvSetVirtualProcessorRegisters(%p, %u,,%u,) -> %Rhrc (Last=%#x/%u)\n",
                           pVM->nem.s.hPartition, pVCpu->idCpu, iReg,
                           hrc, RTNtLastStatusValue(), RTNtLastErrorValue()));
    return VERR_INTERNAL_ERROR;

#  undef ADD_REG64
#  undef ADD_REG128
#  undef ADD_SEG

# endif /* !NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS */
}


NEM_TMPL_STATIC int nemHCWinCopyStateFromHyperV(PVMCC pVM, PVMCPUCC pVCpu, uint64_t fWhat)
{
# if defined(NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS) || defined(NEM_WIN_WITH_RING0_RUNLOOP)
#  if !defined(NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS) && defined(NEM_WIN_WITH_RING0_RUNLOOP)
    if (pVM->nem.s.fUseRing0Runloop)
#  endif
    {
        /* See NEMR0ImportState */
        int rc = VMMR3CallR0Emt(pVM, pVCpu, VMMR0_DO_NEM_IMPORT_STATE, fWhat, NULL);
        if (RT_SUCCESS(rc))
            return rc;
        if (rc == VERR_NEM_FLUSH_TLB)
            return PGMFlushTLB(pVCpu, pVCpu->cpum.GstCtx.cr3, true /*fGlobal*/);
        AssertLogRelRCReturn(rc, rc);
        return rc;
    }
# endif
# ifndef NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS
    WHV_REGISTER_NAME  aenmNames[128];

    fWhat &= pVCpu->cpum.GstCtx.fExtrn;
    uintptr_t iReg = 0;

    /* GPRs */
    if (fWhat & CPUMCTX_EXTRN_GPRS_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_RAX)
            aenmNames[iReg++] = WHvX64RegisterRax;
        if (fWhat & CPUMCTX_EXTRN_RCX)
            aenmNames[iReg++] = WHvX64RegisterRcx;
        if (fWhat & CPUMCTX_EXTRN_RDX)
            aenmNames[iReg++] = WHvX64RegisterRdx;
        if (fWhat & CPUMCTX_EXTRN_RBX)
            aenmNames[iReg++] = WHvX64RegisterRbx;
        if (fWhat & CPUMCTX_EXTRN_RSP)
            aenmNames[iReg++] = WHvX64RegisterRsp;
        if (fWhat & CPUMCTX_EXTRN_RBP)
            aenmNames[iReg++] = WHvX64RegisterRbp;
        if (fWhat & CPUMCTX_EXTRN_RSI)
            aenmNames[iReg++] = WHvX64RegisterRsi;
        if (fWhat & CPUMCTX_EXTRN_RDI)
            aenmNames[iReg++] = WHvX64RegisterRdi;
        if (fWhat & CPUMCTX_EXTRN_R8_R15)
        {
            aenmNames[iReg++] = WHvX64RegisterR8;
            aenmNames[iReg++] = WHvX64RegisterR9;
            aenmNames[iReg++] = WHvX64RegisterR10;
            aenmNames[iReg++] = WHvX64RegisterR11;
            aenmNames[iReg++] = WHvX64RegisterR12;
            aenmNames[iReg++] = WHvX64RegisterR13;
            aenmNames[iReg++] = WHvX64RegisterR14;
            aenmNames[iReg++] = WHvX64RegisterR15;
        }
    }

    /* RIP & Flags */
    if (fWhat & CPUMCTX_EXTRN_RIP)
        aenmNames[iReg++] = WHvX64RegisterRip;
    if (fWhat & CPUMCTX_EXTRN_RFLAGS)
        aenmNames[iReg++] = WHvX64RegisterRflags;

    /* Segments */
    if (fWhat & CPUMCTX_EXTRN_SREG_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_ES)
            aenmNames[iReg++] = WHvX64RegisterEs;
        if (fWhat & CPUMCTX_EXTRN_CS)
            aenmNames[iReg++] = WHvX64RegisterCs;
        if (fWhat & CPUMCTX_EXTRN_SS)
            aenmNames[iReg++] = WHvX64RegisterSs;
        if (fWhat & CPUMCTX_EXTRN_DS)
            aenmNames[iReg++] = WHvX64RegisterDs;
        if (fWhat & CPUMCTX_EXTRN_FS)
            aenmNames[iReg++] = WHvX64RegisterFs;
        if (fWhat & CPUMCTX_EXTRN_GS)
            aenmNames[iReg++] = WHvX64RegisterGs;
    }

    /* Descriptor tables. */
    if (fWhat & CPUMCTX_EXTRN_TABLE_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_LDTR)
            aenmNames[iReg++] = WHvX64RegisterLdtr;
        if (fWhat & CPUMCTX_EXTRN_TR)
            aenmNames[iReg++] = WHvX64RegisterTr;
        if (fWhat & CPUMCTX_EXTRN_IDTR)
            aenmNames[iReg++] = WHvX64RegisterIdtr;
        if (fWhat & CPUMCTX_EXTRN_GDTR)
            aenmNames[iReg++] = WHvX64RegisterGdtr;
    }

    /* Control registers. */
    if (fWhat & CPUMCTX_EXTRN_CR_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_CR0)
            aenmNames[iReg++] = WHvX64RegisterCr0;
        if (fWhat & CPUMCTX_EXTRN_CR2)
            aenmNames[iReg++] = WHvX64RegisterCr2;
        if (fWhat & CPUMCTX_EXTRN_CR3)
            aenmNames[iReg++] = WHvX64RegisterCr3;
        if (fWhat & CPUMCTX_EXTRN_CR4)
            aenmNames[iReg++] = WHvX64RegisterCr4;
    }
    if (fWhat & CPUMCTX_EXTRN_APIC_TPR)
        aenmNames[iReg++] = WHvX64RegisterCr8;

    /* Debug registers. */
    if (fWhat & CPUMCTX_EXTRN_DR7)
        aenmNames[iReg++] = WHvX64RegisterDr7;
    if (fWhat & CPUMCTX_EXTRN_DR0_DR3)
    {
        if (!(fWhat & CPUMCTX_EXTRN_DR7) && (pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_DR7))
        {
            fWhat |= CPUMCTX_EXTRN_DR7;
            aenmNames[iReg++] = WHvX64RegisterDr7;
        }
        aenmNames[iReg++] = WHvX64RegisterDr0;
        aenmNames[iReg++] = WHvX64RegisterDr1;
        aenmNames[iReg++] = WHvX64RegisterDr2;
        aenmNames[iReg++] = WHvX64RegisterDr3;
    }
    if (fWhat & CPUMCTX_EXTRN_DR6)
        aenmNames[iReg++] = WHvX64RegisterDr6;

    /* Floating point state. */
    if (fWhat & CPUMCTX_EXTRN_X87)
    {
        aenmNames[iReg++] = WHvX64RegisterFpMmx0;
        aenmNames[iReg++] = WHvX64RegisterFpMmx1;
        aenmNames[iReg++] = WHvX64RegisterFpMmx2;
        aenmNames[iReg++] = WHvX64RegisterFpMmx3;
        aenmNames[iReg++] = WHvX64RegisterFpMmx4;
        aenmNames[iReg++] = WHvX64RegisterFpMmx5;
        aenmNames[iReg++] = WHvX64RegisterFpMmx6;
        aenmNames[iReg++] = WHvX64RegisterFpMmx7;
        aenmNames[iReg++] = WHvX64RegisterFpControlStatus;
    }
    if (fWhat & (CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX))
        aenmNames[iReg++] = WHvX64RegisterXmmControlStatus;

    /* Vector state. */
    if (fWhat & CPUMCTX_EXTRN_SSE_AVX)
    {
        aenmNames[iReg++] = WHvX64RegisterXmm0;
        aenmNames[iReg++] = WHvX64RegisterXmm1;
        aenmNames[iReg++] = WHvX64RegisterXmm2;
        aenmNames[iReg++] = WHvX64RegisterXmm3;
        aenmNames[iReg++] = WHvX64RegisterXmm4;
        aenmNames[iReg++] = WHvX64RegisterXmm5;
        aenmNames[iReg++] = WHvX64RegisterXmm6;
        aenmNames[iReg++] = WHvX64RegisterXmm7;
        aenmNames[iReg++] = WHvX64RegisterXmm8;
        aenmNames[iReg++] = WHvX64RegisterXmm9;
        aenmNames[iReg++] = WHvX64RegisterXmm10;
        aenmNames[iReg++] = WHvX64RegisterXmm11;
        aenmNames[iReg++] = WHvX64RegisterXmm12;
        aenmNames[iReg++] = WHvX64RegisterXmm13;
        aenmNames[iReg++] = WHvX64RegisterXmm14;
        aenmNames[iReg++] = WHvX64RegisterXmm15;
    }

    /* MSRs */
    // WHvX64RegisterTsc - don't touch
    if (fWhat & CPUMCTX_EXTRN_EFER)
        aenmNames[iReg++] = WHvX64RegisterEfer;
    if (fWhat & CPUMCTX_EXTRN_KERNEL_GS_BASE)
        aenmNames[iReg++] = WHvX64RegisterKernelGsBase;
    if (fWhat & CPUMCTX_EXTRN_SYSENTER_MSRS)
    {
        aenmNames[iReg++] = WHvX64RegisterSysenterCs;
        aenmNames[iReg++] = WHvX64RegisterSysenterEip;
        aenmNames[iReg++] = WHvX64RegisterSysenterEsp;
    }
    if (fWhat & CPUMCTX_EXTRN_SYSCALL_MSRS)
    {
        aenmNames[iReg++] = WHvX64RegisterStar;
        aenmNames[iReg++] = WHvX64RegisterLstar;
        aenmNames[iReg++] = WHvX64RegisterCstar;
        aenmNames[iReg++] = WHvX64RegisterSfmask;
    }

//#ifdef LOG_ENABLED
//    const CPUMCPUVENDOR enmCpuVendor = CPUMGetHostCpuVendor(pVM);
//#endif
    if (fWhat & CPUMCTX_EXTRN_OTHER_MSRS)
    {
        aenmNames[iReg++] = WHvX64RegisterApicBase; /// @todo APIC BASE
        aenmNames[iReg++] = WHvX64RegisterPat;
#if 0 /*def LOG_ENABLED*/ /** @todo Check if WHvX64RegisterMsrMtrrCap works... */
        aenmNames[iReg++] = WHvX64RegisterMsrMtrrCap;
#endif
        aenmNames[iReg++] = WHvX64RegisterMsrMtrrDefType;
        aenmNames[iReg++] = WHvX64RegisterMsrMtrrFix64k00000;
        aenmNames[iReg++] = WHvX64RegisterMsrMtrrFix16k80000;
        aenmNames[iReg++] = WHvX64RegisterMsrMtrrFix16kA0000;
        aenmNames[iReg++] = WHvX64RegisterMsrMtrrFix4kC0000;
        aenmNames[iReg++] = WHvX64RegisterMsrMtrrFix4kC8000;
        aenmNames[iReg++] = WHvX64RegisterMsrMtrrFix4kD0000;
        aenmNames[iReg++] = WHvX64RegisterMsrMtrrFix4kD8000;
        aenmNames[iReg++] = WHvX64RegisterMsrMtrrFix4kE0000;
        aenmNames[iReg++] = WHvX64RegisterMsrMtrrFix4kE8000;
        aenmNames[iReg++] = WHvX64RegisterMsrMtrrFix4kF0000;
        aenmNames[iReg++] = WHvX64RegisterMsrMtrrFix4kF8000;
        aenmNames[iReg++] = WHvX64RegisterTscAux;
        /** @todo look for HvX64RegisterIa32MiscEnable and HvX64RegisterIa32FeatureControl? */
//#ifdef LOG_ENABLED
//        if (enmCpuVendor != CPUMCPUVENDOR_AMD)
//            aenmNames[iReg++] = HvX64RegisterIa32FeatureControl;
//#endif
    }

    /* Interruptibility. */
    if (fWhat & (CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT | CPUMCTX_EXTRN_NEM_WIN_INHIBIT_NMI))
    {
        aenmNames[iReg++] = WHvRegisterInterruptState;
        aenmNames[iReg++] = WHvX64RegisterRip;
    }

    /* event injection */
    aenmNames[iReg++] = WHvRegisterPendingInterruption;
    aenmNames[iReg++] = WHvRegisterPendingEvent0; /** @todo renamed to WHvRegisterPendingEvent */

    size_t const cRegs = iReg;
    Assert(cRegs < RT_ELEMENTS(aenmNames));

    /*
     * Get the registers.
     */
    WHV_REGISTER_VALUE aValues[128];
    RT_ZERO(aValues);
    Assert(RT_ELEMENTS(aValues) >= cRegs);
    Assert(RT_ELEMENTS(aenmNames) >= cRegs);
#  ifdef NEM_WIN_INTERCEPT_NT_IO_CTLS
    Log12(("Calling WHvGetVirtualProcessorRegisters(%p, %u, %p, %u, %p)\n",
          pVM->nem.s.hPartition, pVCpu->idCpu, aenmNames, cRegs, aValues));
#  endif
    HRESULT hrc = WHvGetVirtualProcessorRegisters(pVM->nem.s.hPartition, pVCpu->idCpu, aenmNames, (uint32_t)cRegs, aValues);
    AssertLogRelMsgReturn(SUCCEEDED(hrc),
                          ("WHvGetVirtualProcessorRegisters(%p, %u,,%u,) -> %Rhrc (Last=%#x/%u)\n",
                           pVM->nem.s.hPartition, pVCpu->idCpu, cRegs, hrc, RTNtLastStatusValue(), RTNtLastErrorValue())
                          , VERR_NEM_GET_REGISTERS_FAILED);

    iReg = 0;
#  define GET_REG64(a_DstVar, a_enmName) do { \
            Assert(aenmNames[iReg] == (a_enmName)); \
            (a_DstVar) = aValues[iReg].Reg64; \
            iReg++; \
        } while (0)
#  define GET_REG64_LOG7(a_DstVar, a_enmName, a_szLogName) do { \
            Assert(aenmNames[iReg] == (a_enmName)); \
            if ((a_DstVar) != aValues[iReg].Reg64) \
                Log7(("NEM/%u: " a_szLogName " changed %RX64 -> %RX64\n", pVCpu->idCpu, (a_DstVar), aValues[iReg].Reg64)); \
            (a_DstVar) = aValues[iReg].Reg64; \
            iReg++; \
        } while (0)
#  define GET_REG128(a_DstVarLo, a_DstVarHi, a_enmName) do { \
            Assert(aenmNames[iReg] == a_enmName); \
            (a_DstVarLo) = aValues[iReg].Reg128.Low64; \
            (a_DstVarHi) = aValues[iReg].Reg128.High64; \
            iReg++; \
        } while (0)
#  define GET_SEG(a_SReg, a_enmName) do { \
            Assert(aenmNames[iReg] == (a_enmName)); \
            NEM_WIN_COPY_BACK_SEG(a_SReg, aValues[iReg].Segment); \
            iReg++; \
        } while (0)

    /* GPRs */
    if (fWhat & CPUMCTX_EXTRN_GPRS_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_RAX)
            GET_REG64(pVCpu->cpum.GstCtx.rax, WHvX64RegisterRax);
        if (fWhat & CPUMCTX_EXTRN_RCX)
            GET_REG64(pVCpu->cpum.GstCtx.rcx, WHvX64RegisterRcx);
        if (fWhat & CPUMCTX_EXTRN_RDX)
            GET_REG64(pVCpu->cpum.GstCtx.rdx, WHvX64RegisterRdx);
        if (fWhat & CPUMCTX_EXTRN_RBX)
            GET_REG64(pVCpu->cpum.GstCtx.rbx, WHvX64RegisterRbx);
        if (fWhat & CPUMCTX_EXTRN_RSP)
            GET_REG64(pVCpu->cpum.GstCtx.rsp, WHvX64RegisterRsp);
        if (fWhat & CPUMCTX_EXTRN_RBP)
            GET_REG64(pVCpu->cpum.GstCtx.rbp, WHvX64RegisterRbp);
        if (fWhat & CPUMCTX_EXTRN_RSI)
            GET_REG64(pVCpu->cpum.GstCtx.rsi, WHvX64RegisterRsi);
        if (fWhat & CPUMCTX_EXTRN_RDI)
            GET_REG64(pVCpu->cpum.GstCtx.rdi, WHvX64RegisterRdi);
        if (fWhat & CPUMCTX_EXTRN_R8_R15)
        {
            GET_REG64(pVCpu->cpum.GstCtx.r8, WHvX64RegisterR8);
            GET_REG64(pVCpu->cpum.GstCtx.r9, WHvX64RegisterR9);
            GET_REG64(pVCpu->cpum.GstCtx.r10, WHvX64RegisterR10);
            GET_REG64(pVCpu->cpum.GstCtx.r11, WHvX64RegisterR11);
            GET_REG64(pVCpu->cpum.GstCtx.r12, WHvX64RegisterR12);
            GET_REG64(pVCpu->cpum.GstCtx.r13, WHvX64RegisterR13);
            GET_REG64(pVCpu->cpum.GstCtx.r14, WHvX64RegisterR14);
            GET_REG64(pVCpu->cpum.GstCtx.r15, WHvX64RegisterR15);
        }
    }

    /* RIP & Flags */
    if (fWhat & CPUMCTX_EXTRN_RIP)
        GET_REG64(pVCpu->cpum.GstCtx.rip, WHvX64RegisterRip);
    if (fWhat & CPUMCTX_EXTRN_RFLAGS)
        GET_REG64(pVCpu->cpum.GstCtx.rflags.u, WHvX64RegisterRflags);

    /* Segments */
    if (fWhat & CPUMCTX_EXTRN_SREG_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_ES)
            GET_SEG(pVCpu->cpum.GstCtx.es, WHvX64RegisterEs);
        if (fWhat & CPUMCTX_EXTRN_CS)
            GET_SEG(pVCpu->cpum.GstCtx.cs, WHvX64RegisterCs);
        if (fWhat & CPUMCTX_EXTRN_SS)
            GET_SEG(pVCpu->cpum.GstCtx.ss, WHvX64RegisterSs);
        if (fWhat & CPUMCTX_EXTRN_DS)
            GET_SEG(pVCpu->cpum.GstCtx.ds, WHvX64RegisterDs);
        if (fWhat & CPUMCTX_EXTRN_FS)
            GET_SEG(pVCpu->cpum.GstCtx.fs, WHvX64RegisterFs);
        if (fWhat & CPUMCTX_EXTRN_GS)
            GET_SEG(pVCpu->cpum.GstCtx.gs, WHvX64RegisterGs);
    }

    /* Descriptor tables and the task segment. */
    if (fWhat & CPUMCTX_EXTRN_TABLE_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_LDTR)
            GET_SEG(pVCpu->cpum.GstCtx.ldtr, WHvX64RegisterLdtr);

        if (fWhat & CPUMCTX_EXTRN_TR)
        {
            /* AMD-V likes loading TR with in AVAIL state, whereas intel insists on BUSY.  So,
               avoid to trigger sanity assertions around the code, always fix this. */
            GET_SEG(pVCpu->cpum.GstCtx.tr, WHvX64RegisterTr);
            switch (pVCpu->cpum.GstCtx.tr.Attr.n.u4Type)
            {
                case X86_SEL_TYPE_SYS_386_TSS_BUSY:
                case X86_SEL_TYPE_SYS_286_TSS_BUSY:
                    break;
                case X86_SEL_TYPE_SYS_386_TSS_AVAIL:
                    pVCpu->cpum.GstCtx.tr.Attr.n.u4Type = X86_SEL_TYPE_SYS_386_TSS_BUSY;
                    break;
                case X86_SEL_TYPE_SYS_286_TSS_AVAIL:
                    pVCpu->cpum.GstCtx.tr.Attr.n.u4Type = X86_SEL_TYPE_SYS_286_TSS_BUSY;
                    break;
            }
        }
        if (fWhat & CPUMCTX_EXTRN_IDTR)
        {
            Assert(aenmNames[iReg] == WHvX64RegisterIdtr);
            pVCpu->cpum.GstCtx.idtr.cbIdt = aValues[iReg].Table.Limit;
            pVCpu->cpum.GstCtx.idtr.pIdt  = aValues[iReg].Table.Base;
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_GDTR)
        {
            Assert(aenmNames[iReg] == WHvX64RegisterGdtr);
            pVCpu->cpum.GstCtx.gdtr.cbGdt = aValues[iReg].Table.Limit;
            pVCpu->cpum.GstCtx.gdtr.pGdt  = aValues[iReg].Table.Base;
            iReg++;
        }
    }

    /* Control registers. */
    bool fMaybeChangedMode = false;
    bool fUpdateCr3        = false;
    if (fWhat & CPUMCTX_EXTRN_CR_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_CR0)
        {
            Assert(aenmNames[iReg] == WHvX64RegisterCr0);
            if (pVCpu->cpum.GstCtx.cr0 != aValues[iReg].Reg64)
            {
                CPUMSetGuestCR0(pVCpu, aValues[iReg].Reg64);
                fMaybeChangedMode = true;
            }
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_CR2)
            GET_REG64(pVCpu->cpum.GstCtx.cr2, WHvX64RegisterCr2);
        if (fWhat & CPUMCTX_EXTRN_CR3)
        {
            if (pVCpu->cpum.GstCtx.cr3 != aValues[iReg].Reg64)
            {
                CPUMSetGuestCR3(pVCpu, aValues[iReg].Reg64);
                fUpdateCr3 = true;
            }
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_CR4)
        {
            if (pVCpu->cpum.GstCtx.cr4 != aValues[iReg].Reg64)
            {
                CPUMSetGuestCR4(pVCpu, aValues[iReg].Reg64);
                fMaybeChangedMode = true;
            }
            iReg++;
        }
    }
    if (fWhat & CPUMCTX_EXTRN_APIC_TPR)
    {
        Assert(aenmNames[iReg] == WHvX64RegisterCr8);
        APICSetTpr(pVCpu, (uint8_t)aValues[iReg].Reg64 << 4);
        iReg++;
    }

    /* Debug registers. */
    if (fWhat & CPUMCTX_EXTRN_DR7)
    {
        Assert(aenmNames[iReg] == WHvX64RegisterDr7);
        if (pVCpu->cpum.GstCtx.dr[7] != aValues[iReg].Reg64)
            CPUMSetGuestDR7(pVCpu, aValues[iReg].Reg64);
        pVCpu->cpum.GstCtx.fExtrn &= ~CPUMCTX_EXTRN_DR7; /* Hack alert! Avoids asserting when processing CPUMCTX_EXTRN_DR0_DR3. */
        iReg++;
    }
    if (fWhat & CPUMCTX_EXTRN_DR0_DR3)
    {
        Assert(aenmNames[iReg] == WHvX64RegisterDr0);
        Assert(aenmNames[iReg+3] == WHvX64RegisterDr3);
        if (pVCpu->cpum.GstCtx.dr[0] != aValues[iReg].Reg64)
            CPUMSetGuestDR0(pVCpu, aValues[iReg].Reg64);
        iReg++;
        if (pVCpu->cpum.GstCtx.dr[1] != aValues[iReg].Reg64)
            CPUMSetGuestDR1(pVCpu, aValues[iReg].Reg64);
        iReg++;
        if (pVCpu->cpum.GstCtx.dr[2] != aValues[iReg].Reg64)
            CPUMSetGuestDR2(pVCpu, aValues[iReg].Reg64);
        iReg++;
        if (pVCpu->cpum.GstCtx.dr[3] != aValues[iReg].Reg64)
            CPUMSetGuestDR3(pVCpu, aValues[iReg].Reg64);
        iReg++;
    }
    if (fWhat & CPUMCTX_EXTRN_DR6)
    {
        Assert(aenmNames[iReg] == WHvX64RegisterDr6);
        if (pVCpu->cpum.GstCtx.dr[6] != aValues[iReg].Reg64)
            CPUMSetGuestDR6(pVCpu, aValues[iReg].Reg64);
        iReg++;
    }

    /* Floating point state. */
    if (fWhat & CPUMCTX_EXTRN_X87)
    {
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[0].au64[0], pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[0].au64[1], WHvX64RegisterFpMmx0);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[1].au64[0], pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[1].au64[1], WHvX64RegisterFpMmx1);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[2].au64[0], pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[2].au64[1], WHvX64RegisterFpMmx2);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[3].au64[0], pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[3].au64[1], WHvX64RegisterFpMmx3);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[4].au64[0], pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[4].au64[1], WHvX64RegisterFpMmx4);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[5].au64[0], pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[5].au64[1], WHvX64RegisterFpMmx5);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[6].au64[0], pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[6].au64[1], WHvX64RegisterFpMmx6);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[7].au64[0], pVCpu->cpum.GstCtx.pXStateR3->x87.aRegs[7].au64[1], WHvX64RegisterFpMmx7);

        Assert(aenmNames[iReg] == WHvX64RegisterFpControlStatus);
        pVCpu->cpum.GstCtx.pXStateR3->x87.FCW        = aValues[iReg].FpControlStatus.FpControl;
        pVCpu->cpum.GstCtx.pXStateR3->x87.FSW        = aValues[iReg].FpControlStatus.FpStatus;
        pVCpu->cpum.GstCtx.pXStateR3->x87.FTW        = aValues[iReg].FpControlStatus.FpTag
                                        /*| (aValues[iReg].FpControlStatus.Reserved << 8)*/;
        pVCpu->cpum.GstCtx.pXStateR3->x87.FOP        = aValues[iReg].FpControlStatus.LastFpOp;
        pVCpu->cpum.GstCtx.pXStateR3->x87.FPUIP      = (uint32_t)aValues[iReg].FpControlStatus.LastFpRip;
        pVCpu->cpum.GstCtx.pXStateR3->x87.CS         = (uint16_t)(aValues[iReg].FpControlStatus.LastFpRip >> 32);
        pVCpu->cpum.GstCtx.pXStateR3->x87.Rsrvd1     = (uint16_t)(aValues[iReg].FpControlStatus.LastFpRip >> 48);
        iReg++;
    }

    if (fWhat & (CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX))
    {
        Assert(aenmNames[iReg] == WHvX64RegisterXmmControlStatus);
        if (fWhat & CPUMCTX_EXTRN_X87)
        {
            pVCpu->cpum.GstCtx.pXStateR3->x87.FPUDP  = (uint32_t)aValues[iReg].XmmControlStatus.LastFpRdp;
            pVCpu->cpum.GstCtx.pXStateR3->x87.DS     = (uint16_t)(aValues[iReg].XmmControlStatus.LastFpRdp >> 32);
            pVCpu->cpum.GstCtx.pXStateR3->x87.Rsrvd2 = (uint16_t)(aValues[iReg].XmmControlStatus.LastFpRdp >> 48);
        }
        pVCpu->cpum.GstCtx.pXStateR3->x87.MXCSR      = aValues[iReg].XmmControlStatus.XmmStatusControl;
        pVCpu->cpum.GstCtx.pXStateR3->x87.MXCSR_MASK = aValues[iReg].XmmControlStatus.XmmStatusControlMask; /** @todo ??? (Isn't this an output field?) */
        iReg++;
    }

    /* Vector state. */
    if (fWhat & CPUMCTX_EXTRN_SSE_AVX)
    {
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 0].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 0].uXmm.s.Hi, WHvX64RegisterXmm0);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 1].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 1].uXmm.s.Hi, WHvX64RegisterXmm1);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 2].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 2].uXmm.s.Hi, WHvX64RegisterXmm2);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 3].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 3].uXmm.s.Hi, WHvX64RegisterXmm3);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 4].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 4].uXmm.s.Hi, WHvX64RegisterXmm4);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 5].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 5].uXmm.s.Hi, WHvX64RegisterXmm5);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 6].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 6].uXmm.s.Hi, WHvX64RegisterXmm6);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 7].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 7].uXmm.s.Hi, WHvX64RegisterXmm7);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 8].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 8].uXmm.s.Hi, WHvX64RegisterXmm8);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 9].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[ 9].uXmm.s.Hi, WHvX64RegisterXmm9);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[10].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[10].uXmm.s.Hi, WHvX64RegisterXmm10);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[11].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[11].uXmm.s.Hi, WHvX64RegisterXmm11);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[12].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[12].uXmm.s.Hi, WHvX64RegisterXmm12);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[13].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[13].uXmm.s.Hi, WHvX64RegisterXmm13);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[14].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[14].uXmm.s.Hi, WHvX64RegisterXmm14);
        GET_REG128(pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[15].uXmm.s.Lo, pVCpu->cpum.GstCtx.pXStateR3->x87.aXMM[15].uXmm.s.Hi, WHvX64RegisterXmm15);
    }

    /* MSRs */
    // WHvX64RegisterTsc - don't touch
    if (fWhat & CPUMCTX_EXTRN_EFER)
    {
        Assert(aenmNames[iReg] == WHvX64RegisterEfer);
        if (aValues[iReg].Reg64 != pVCpu->cpum.GstCtx.msrEFER)
        {
            Log7(("NEM/%u: MSR EFER changed %RX64 -> %RX64\n", pVCpu->idCpu, pVCpu->cpum.GstCtx.msrEFER, aValues[iReg].Reg64));
            if ((aValues[iReg].Reg64 ^ pVCpu->cpum.GstCtx.msrEFER) & MSR_K6_EFER_NXE)
                PGMNotifyNxeChanged(pVCpu, RT_BOOL(aValues[iReg].Reg64 & MSR_K6_EFER_NXE));
            pVCpu->cpum.GstCtx.msrEFER = aValues[iReg].Reg64;
            fMaybeChangedMode = true;
        }
        iReg++;
    }
    if (fWhat & CPUMCTX_EXTRN_KERNEL_GS_BASE)
        GET_REG64_LOG7(pVCpu->cpum.GstCtx.msrKERNELGSBASE, WHvX64RegisterKernelGsBase, "MSR KERNEL_GS_BASE");
    if (fWhat & CPUMCTX_EXTRN_SYSENTER_MSRS)
    {
        GET_REG64_LOG7(pVCpu->cpum.GstCtx.SysEnter.cs,  WHvX64RegisterSysenterCs,  "MSR SYSENTER.CS");
        GET_REG64_LOG7(pVCpu->cpum.GstCtx.SysEnter.eip, WHvX64RegisterSysenterEip, "MSR SYSENTER.EIP");
        GET_REG64_LOG7(pVCpu->cpum.GstCtx.SysEnter.esp, WHvX64RegisterSysenterEsp, "MSR SYSENTER.ESP");
    }
    if (fWhat & CPUMCTX_EXTRN_SYSCALL_MSRS)
    {
        GET_REG64_LOG7(pVCpu->cpum.GstCtx.msrSTAR,   WHvX64RegisterStar,   "MSR STAR");
        GET_REG64_LOG7(pVCpu->cpum.GstCtx.msrLSTAR,  WHvX64RegisterLstar,  "MSR LSTAR");
        GET_REG64_LOG7(pVCpu->cpum.GstCtx.msrCSTAR,  WHvX64RegisterCstar,  "MSR CSTAR");
        GET_REG64_LOG7(pVCpu->cpum.GstCtx.msrSFMASK, WHvX64RegisterSfmask, "MSR SFMASK");
    }
    if (fWhat & CPUMCTX_EXTRN_OTHER_MSRS)
    {
        Assert(aenmNames[iReg] == WHvX64RegisterApicBase);
        const uint64_t uOldBase = APICGetBaseMsrNoCheck(pVCpu);
        if (aValues[iReg].Reg64 != uOldBase)
        {
            Log7(("NEM/%u: MSR APICBase changed %RX64 -> %RX64 (%RX64)\n",
                  pVCpu->idCpu, uOldBase, aValues[iReg].Reg64, aValues[iReg].Reg64 ^ uOldBase));
            int rc2 = APICSetBaseMsr(pVCpu, aValues[iReg].Reg64);
            AssertLogRelMsg(rc2 == VINF_SUCCESS, ("%Rrc %RX64\n", rc2, aValues[iReg].Reg64));
        }
        iReg++;

        GET_REG64_LOG7(pVCpu->cpum.GstCtx.msrPAT, WHvX64RegisterPat, "MSR PAT");
#if 0 /*def LOG_ENABLED*/ /** @todo something's wrong with HvX64RegisterMtrrCap? (AMD) */
        GET_REG64_LOG7(pVCpu->cpum.GstCtx.msrPAT, WHvX64RegisterMsrMtrrCap);
#endif
        PCPUMCTXMSRS pCtxMsrs = CPUMQueryGuestCtxMsrsPtr(pVCpu);
        GET_REG64_LOG7(pCtxMsrs->msr.MtrrDefType,      WHvX64RegisterMsrMtrrDefType,     "MSR MTRR_DEF_TYPE");
        GET_REG64_LOG7(pCtxMsrs->msr.MtrrFix64K_00000, WHvX64RegisterMsrMtrrFix64k00000, "MSR MTRR_FIX_64K_00000");
        GET_REG64_LOG7(pCtxMsrs->msr.MtrrFix16K_80000, WHvX64RegisterMsrMtrrFix16k80000, "MSR MTRR_FIX_16K_80000");
        GET_REG64_LOG7(pCtxMsrs->msr.MtrrFix16K_A0000, WHvX64RegisterMsrMtrrFix16kA0000, "MSR MTRR_FIX_16K_A0000");
        GET_REG64_LOG7(pCtxMsrs->msr.MtrrFix4K_C0000,  WHvX64RegisterMsrMtrrFix4kC0000,  "MSR MTRR_FIX_4K_C0000");
        GET_REG64_LOG7(pCtxMsrs->msr.MtrrFix4K_C8000,  WHvX64RegisterMsrMtrrFix4kC8000,  "MSR MTRR_FIX_4K_C8000");
        GET_REG64_LOG7(pCtxMsrs->msr.MtrrFix4K_D0000,  WHvX64RegisterMsrMtrrFix4kD0000,  "MSR MTRR_FIX_4K_D0000");
        GET_REG64_LOG7(pCtxMsrs->msr.MtrrFix4K_D8000,  WHvX64RegisterMsrMtrrFix4kD8000,  "MSR MTRR_FIX_4K_D8000");
        GET_REG64_LOG7(pCtxMsrs->msr.MtrrFix4K_E0000,  WHvX64RegisterMsrMtrrFix4kE0000,  "MSR MTRR_FIX_4K_E0000");
        GET_REG64_LOG7(pCtxMsrs->msr.MtrrFix4K_E8000,  WHvX64RegisterMsrMtrrFix4kE8000,  "MSR MTRR_FIX_4K_E8000");
        GET_REG64_LOG7(pCtxMsrs->msr.MtrrFix4K_F0000,  WHvX64RegisterMsrMtrrFix4kF0000,  "MSR MTRR_FIX_4K_F0000");
        GET_REG64_LOG7(pCtxMsrs->msr.MtrrFix4K_F8000,  WHvX64RegisterMsrMtrrFix4kF8000,  "MSR MTRR_FIX_4K_F8000");
        GET_REG64_LOG7(pCtxMsrs->msr.TscAux,           WHvX64RegisterTscAux,             "MSR TSC_AUX");
        /** @todo look for HvX64RegisterIa32MiscEnable and HvX64RegisterIa32FeatureControl? */
    }

    /* Interruptibility. */
    if (fWhat & (CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT | CPUMCTX_EXTRN_NEM_WIN_INHIBIT_NMI))
    {
        Assert(aenmNames[iReg] == WHvRegisterInterruptState);
        Assert(aenmNames[iReg + 1] == WHvX64RegisterRip);

        if (!(pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT))
        {
            pVCpu->nem.s.fLastInterruptShadow = aValues[iReg].InterruptState.InterruptShadow;
            if (aValues[iReg].InterruptState.InterruptShadow)
                EMSetInhibitInterruptsPC(pVCpu, aValues[iReg + 1].Reg64);
            else
                VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS);
        }

        if (!(pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_NEM_WIN_INHIBIT_NMI))
        {
            if (aValues[iReg].InterruptState.NmiMasked)
                VMCPU_FF_SET(pVCpu, VMCPU_FF_BLOCK_NMIS);
            else
                VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_BLOCK_NMIS);
        }

        fWhat |= CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT | CPUMCTX_EXTRN_NEM_WIN_INHIBIT_NMI;
        iReg += 2;
    }

    /* Event injection. */
    /// @todo WHvRegisterPendingInterruption
    Assert(aenmNames[iReg] == WHvRegisterPendingInterruption);
    if (aValues[iReg].PendingInterruption.InterruptionPending)
    {
        Log7(("PendingInterruption: type=%u vector=%#x errcd=%RTbool/%#x instr-len=%u nested=%u\n",
              aValues[iReg].PendingInterruption.InterruptionType, aValues[iReg].PendingInterruption.InterruptionVector,
              aValues[iReg].PendingInterruption.DeliverErrorCode, aValues[iReg].PendingInterruption.ErrorCode,
              aValues[iReg].PendingInterruption.InstructionLength, aValues[iReg].PendingInterruption.NestedEvent));
        AssertMsg((aValues[iReg].PendingInterruption.AsUINT64 & UINT64_C(0xfc00)) == 0,
                  ("%#RX64\n", aValues[iReg].PendingInterruption.AsUINT64));
    }

    /// @todo WHvRegisterPendingEvent0 (renamed to WHvRegisterPendingEvent).

    /* Almost done, just update extrn flags and maybe change PGM mode. */
    pVCpu->cpum.GstCtx.fExtrn &= ~fWhat;
    if (!(pVCpu->cpum.GstCtx.fExtrn & (CPUMCTX_EXTRN_ALL | (CPUMCTX_EXTRN_NEM_WIN_MASK & ~CPUMCTX_EXTRN_NEM_WIN_EVENT_INJECT))))
        pVCpu->cpum.GstCtx.fExtrn = 0;

    /* Typical. */
    if (!fMaybeChangedMode && !fUpdateCr3)
        return VINF_SUCCESS;

    /*
     * Slow.
     */
    if (fMaybeChangedMode)
    {
        int rc = PGMChangeMode(pVCpu, pVCpu->cpum.GstCtx.cr0, pVCpu->cpum.GstCtx.cr4, pVCpu->cpum.GstCtx.msrEFER);
        AssertMsgReturn(rc == VINF_SUCCESS, ("rc=%Rrc\n", rc), RT_FAILURE_NP(rc) ? rc : VERR_NEM_IPE_1);
    }

    if (fUpdateCr3)
    {
        int rc = PGMUpdateCR3(pVCpu, pVCpu->cpum.GstCtx.cr3);
        AssertMsgReturn(rc == VINF_SUCCESS, ("rc=%Rrc\n", rc), RT_FAILURE_NP(rc) ? rc : VERR_NEM_IPE_2);
    }

    return VINF_SUCCESS;
# endif /* !NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS */
}

#endif /* !IN_RING0 */


/**
 * Interface for importing state on demand (used by IEM).
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context CPU structure.
 * @param   fWhat       What to import, CPUMCTX_EXTRN_XXX.
 */
VMM_INT_DECL(int) NEMImportStateOnDemand(PVMCPUCC pVCpu, uint64_t fWhat)
{
    STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatImportOnDemand);

#ifdef IN_RING0
# ifdef NEM_WIN_WITH_RING0_RUNLOOP
    return nemR0WinImportState(pVCpu->pGVM, pVCpu, &pVCpu->cpum.GstCtx, fWhat, true /*fCanUpdateCr3*/);
# else
    RT_NOREF(pVCpu, fWhat);
    return VERR_NOT_IMPLEMENTED;
# endif
#else
    return nemHCWinCopyStateFromHyperV(pVCpu->pVMR3, pVCpu, fWhat);
#endif
}


/**
 * Query the CPU tick counter and optionally the TSC_AUX MSR value.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context CPU structure.
 * @param   pcTicks     Where to return the CPU tick count.
 * @param   puAux       Where to return the TSC_AUX register value.
 */
VMM_INT_DECL(int) NEMHCQueryCpuTick(PVMCPUCC pVCpu, uint64_t *pcTicks, uint32_t *puAux)
{
    STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatQueryCpuTick);

#ifdef IN_RING3
    PVMCC pVM = pVCpu->CTX_SUFF(pVM);
    VMCPU_ASSERT_EMT_RETURN(pVCpu, VERR_VM_THREAD_NOT_EMT);
    AssertReturn(VM_IS_NEM_ENABLED(pVM), VERR_NEM_IPE_9);

# if defined(NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS) || defined(NEM_WIN_WITH_RING0_RUNLOOP)
#  if !defined(NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS) && defined(NEM_WIN_WITH_RING0_RUNLOOP)
    if (pVM->nem.s.fUseRing0Runloop)
#  endif
    {
        /* Call ring-0 and get the values. */
        int rc = VMMR3CallR0Emt(pVM, pVCpu, VMMR0_DO_NEM_QUERY_CPU_TICK, 0, NULL);
        AssertLogRelRCReturn(rc, rc);
        *pcTicks = pVCpu->nem.s.Hypercall.QueryCpuTick.cTicks;
        if (puAux)
            *puAux = pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_TSC_AUX
                   ? pVCpu->nem.s.Hypercall.QueryCpuTick.uAux : CPUMGetGuestTscAux(pVCpu);
        return VINF_SUCCESS;
    }
# endif
# ifndef NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS
    /* Call the offical API. */
    WHV_REGISTER_NAME  aenmNames[2] = { WHvX64RegisterTsc, WHvX64RegisterTscAux };
    WHV_REGISTER_VALUE aValues[2]   = { {0, 0}, {0, 0} };
    Assert(RT_ELEMENTS(aenmNames) == RT_ELEMENTS(aValues));
    HRESULT hrc = WHvGetVirtualProcessorRegisters(pVM->nem.s.hPartition, pVCpu->idCpu, aenmNames, 2, aValues);
    AssertLogRelMsgReturn(SUCCEEDED(hrc),
                          ("WHvGetVirtualProcessorRegisters(%p, %u,{tsc,tsc_aux},2,) -> %Rhrc (Last=%#x/%u)\n",
                           pVM->nem.s.hPartition, pVCpu->idCpu, hrc, RTNtLastStatusValue(), RTNtLastErrorValue())
                          , VERR_NEM_GET_REGISTERS_FAILED);
    *pcTicks = aValues[0].Reg64;
    if (puAux)
        *pcTicks = pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_TSC_AUX ? aValues[0].Reg64 : CPUMGetGuestTscAux(pVCpu);
    return VINF_SUCCESS;
# endif /* !NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS */
#else  /* IN_RING0 */
# ifdef NEM_WIN_WITH_RING0_RUNLOOP
    int rc = nemR0WinQueryCpuTick(pVCpu->pGVM, pVCpu, pcTicks, puAux);
    if (RT_SUCCESS(rc) && puAux && !(pVCpu->cpum.GstCtx.fExtrn & CPUMCTX_EXTRN_TSC_AUX))
        *puAux = CPUMGetGuestTscAux(pVCpu);
    return rc;
# else
    RT_NOREF(pVCpu, pcTicks, puAux);
    return VERR_NOT_IMPLEMENTED;
# endif
#endif /* IN_RING0 */
}


/**
 * Resumes CPU clock (TSC) on all virtual CPUs.
 *
 * This is called by TM when the VM is started, restored, resumed or similar.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context CPU structure of the calling EMT.
 * @param   uPausedTscValue The TSC value at the time of pausing.
 */
VMM_INT_DECL(int) NEMHCResumeCpuTickOnAll(PVMCC pVM, PVMCPUCC pVCpu, uint64_t uPausedTscValue)
{
#ifdef IN_RING0
# ifdef NEM_WIN_WITH_RING0_RUNLOOP
    return nemR0WinResumeCpuTickOnAll(pVM, pVCpu, uPausedTscValue);
# else
    RT_NOREF(pVM, pVCpu, uPausedTscValue);
    return VERR_NOT_IMPLEMENTED;
# endif
#else  /* IN_RING3 */
    VMCPU_ASSERT_EMT_RETURN(pVCpu, VERR_VM_THREAD_NOT_EMT);
    AssertReturn(VM_IS_NEM_ENABLED(pVM), VERR_NEM_IPE_9);

# if defined(NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS) || defined(NEM_WIN_WITH_RING0_RUNLOOP)
#  if !defined(NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS) && defined(NEM_WIN_WITH_RING0_RUNLOOP)
    if (pVM->nem.s.fUseRing0Runloop)
#  endif
    {
        /* Call ring-0 and do it all there. */
        return VMMR3CallR0Emt(pVM, pVCpu, VMMR0_DO_NEM_RESUME_CPU_TICK_ON_ALL, uPausedTscValue, NULL);
    }
# endif
# ifndef NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS
    /*
     * Call the offical API to do the job.
     */
    if (pVM->cCpus > 1)
        RTThreadYield(); /* Try decrease the chance that we get rescheduled in the middle. */

    /* Start with the first CPU. */
    WHV_REGISTER_NAME  enmName   = WHvX64RegisterTsc;
    WHV_REGISTER_VALUE Value     = {0, 0};
    Value.Reg64 = uPausedTscValue;
    uint64_t const     uFirstTsc = ASMReadTSC();
    HRESULT hrc = WHvSetVirtualProcessorRegisters(pVM->nem.s.hPartition, 0 /*iCpu*/, &enmName, 1, &Value);
    AssertLogRelMsgReturn(SUCCEEDED(hrc),
                          ("WHvSetVirtualProcessorRegisters(%p, 0,{tsc},2,%#RX64) -> %Rhrc (Last=%#x/%u)\n",
                           pVM->nem.s.hPartition, uPausedTscValue, hrc, RTNtLastStatusValue(), RTNtLastErrorValue())
                          , VERR_NEM_SET_TSC);

    /* Do the other CPUs, adjusting for elapsed TSC and keeping finger crossed
       that we don't introduce too much drift here. */
    for (VMCPUID iCpu = 1; iCpu < pVM->cCpus; iCpu++)
    {
        Assert(enmName == WHvX64RegisterTsc);
        const uint64_t offDelta = (ASMReadTSC() - uFirstTsc);
        Value.Reg64 = uPausedTscValue + offDelta;
        hrc = WHvSetVirtualProcessorRegisters(pVM->nem.s.hPartition, iCpu, &enmName, 1, &Value);
        AssertLogRelMsgReturn(SUCCEEDED(hrc),
                              ("WHvSetVirtualProcessorRegisters(%p, 0,{tsc},2,%#RX64 + %#RX64) -> %Rhrc (Last=%#x/%u)\n",
                               pVM->nem.s.hPartition, iCpu, uPausedTscValue, offDelta, hrc, RTNtLastStatusValue(), RTNtLastErrorValue())
                              , VERR_NEM_SET_TSC);
    }

    return VINF_SUCCESS;
# endif /* !NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS */
#endif /* IN_RING3 */
}

#ifdef NEMWIN_NEED_GET_REGISTER
# if defined(IN_RING0) || defined(NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS)
/** Worker for assertion macro. */
NEM_TMPL_STATIC int nemHCWinGetRegister(PVMCPUCC pVCpu, PGVMCPU pGVCpu, uint32_t enmReg, HV_REGISTER_VALUE *pRetValue)
{
    RT_ZERO(*pRetValue);
#  ifdef IN_RING3
    RT_NOREF(pVCpu, pGVCpu, enmReg);
    return VERR_NOT_IMPLEMENTED;
#  else
    NOREF(pVCpu);

    /*
     * Hypercall parameters.
     */
    HV_INPUT_GET_VP_REGISTERS *pInput = (HV_INPUT_GET_VP_REGISTERS *)pGVCpu->nem.s.HypercallData.pbPage;
    AssertPtrReturn(pInput, VERR_INTERNAL_ERROR_3);
    AssertReturn(g_pfnHvlInvokeHypercall, VERR_NEM_MISSING_KERNEL_API);

    pInput->PartitionId = pVCpu->pGVM->nemr0.s.idHvPartition;
    pInput->VpIndex     = pVCpu->idCpu;
    pInput->fFlags      = 0;
    pInput->Names[0]    = (HV_REGISTER_NAME)enmReg;

    size_t const cbInput = RT_ALIGN_Z(RT_UOFFSETOF(HV_INPUT_GET_VP_REGISTERS, Names[1]), 32);
    HV_REGISTER_VALUE *paValues = (HV_REGISTER_VALUE *)((uint8_t *)pInput + cbInput);
    RT_BZERO(paValues, sizeof(paValues[0]) * 1);

    /*
     * Make the hypercall and copy out the value.
     */
    uint64_t uResult = g_pfnHvlInvokeHypercall(HV_MAKE_CALL_INFO(HvCallGetVpRegisters, 1),
                                               pGVCpu->nem.s.HypercallData.HCPhysPage,
                                               pGVCpu->nem.s.HypercallData.HCPhysPage + cbInput);
    AssertLogRelMsgReturn(uResult == HV_MAKE_CALL_REP_RET(1), ("uResult=%RX64 cRegs=%#x\n", uResult, 1),
                          VERR_NEM_GET_REGISTERS_FAILED);

    *pRetValue = paValues[0];
    return VINF_SUCCESS;
#  endif
}
# else
/** Worker for assertion macro. */
NEM_TMPL_STATIC int nemR3WinGetRegister(PVMCPUCC a_pVCpu, uint32_t a_enmReg, WHV_REGISTER_VALUE pValue)
{
    RT_ZERO(*pRetValue);
    RT_NOREF(pVCpu, pGVCpu, enmReg);
    return VERR_NOT_IMPLEMENTED;
}
# endif
#endif


#ifdef LOG_ENABLED
/**
 * Get the virtual processor running status.
 */
DECLINLINE(VID_PROCESSOR_STATUS) nemHCWinCpuGetRunningStatus(PVMCPUCC pVCpu)
{
# ifdef IN_RING0
    NOREF(pVCpu);
    return VidProcessorStatusUndefined;
# else
    RTERRVARS Saved;
    RTErrVarsSave(&Saved);

    /*
     * This API is disabled in release builds, it seems.  On build 17101 it requires
     * the following patch to be enabled (windbg): eb vid+12180 0f 84 98 00 00 00
     */
    VID_PROCESSOR_STATUS enmCpuStatus = VidProcessorStatusUndefined;
    NTSTATUS rcNt = g_pfnVidGetVirtualProcessorRunningStatus(pVCpu->pVMR3->nem.s.hPartitionDevice, pVCpu->idCpu, &enmCpuStatus);
    AssertRC(rcNt);

    RTErrVarsRestore(&Saved);
    return enmCpuStatus;
# endif
}
#endif /* LOG_ENABLED */


#if defined(NEM_WIN_USE_OUR_OWN_RUN_API) || defined(NEM_WIN_WITH_RING0_RUNLOOP)
# ifdef IN_RING3 /* hopefully not needed in ring-0, as we'd need KTHREADs and KeAlertThread. */
/**
 * Our own WHvCancelRunVirtualProcessor that can later be moved to ring-0.
 *
 * This is an experiment only.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure of the
 *                          calling EMT.
 */
NEM_TMPL_STATIC int nemHCWinCancelRunVirtualProcessor(PVMCC pVM, PVMCPUCC pVCpu)
{
    /*
     * Work the state.
     *
     * From the looks of things, we should let the EMT call VidStopVirtualProcessor.
     * So, we just need to modify the state and kick the EMT if it's waiting on
     * messages.  For the latter we use QueueUserAPC / KeAlterThread.
     */
    for (;;)
    {
        VMCPUSTATE enmState = VMCPU_GET_STATE(pVCpu);
        switch (enmState)
        {
            case VMCPUSTATE_STARTED_EXEC_NEM:
                if (VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC_NEM_CANCELED, VMCPUSTATE_STARTED_EXEC_NEM))
                {
                    DBGFTRACE_CUSTOM(pVM, "VMCPUSTATE_STARTED_EXEC_NEM -> CANCELED");
                    Log8(("nemHCWinCancelRunVirtualProcessor: Switched %u to canceled state\n", pVCpu->idCpu));
                    STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatCancelChangedState);
                    return VINF_SUCCESS;
                }
                break;

            case VMCPUSTATE_STARTED_EXEC_NEM_WAIT:
                if (VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC_NEM_CANCELED, VMCPUSTATE_STARTED_EXEC_NEM_WAIT))
                {
                    DBGFTRACE_CUSTOM(pVM, "VMCPUSTATE_STARTED_EXEC_NEM_WAIT -> CANCELED");
#  ifdef IN_RING0
                    NTSTATUS rcNt = KeAlertThread(??);
                    DBGFTRACE_CUSTOM(pVM, "KeAlertThread -> %#x", rcNt);
#  else
                    NTSTATUS rcNt = NtAlertThread(pVCpu->nem.s.hNativeThreadHandle);
                    DBGFTRACE_CUSTOM(pVM, "NtAlertThread -> %#x", rcNt);
#  endif
                    Log8(("nemHCWinCancelRunVirtualProcessor: Alerted %u: %#x\n", pVCpu->idCpu, rcNt));
                    Assert(rcNt == STATUS_SUCCESS);
                    if (NT_SUCCESS(rcNt))
                    {
                        STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatCancelAlertedThread);
                        return VINF_SUCCESS;
                    }
                    AssertLogRelMsgFailedReturn(("NtAlertThread failed: %#x\n", rcNt), RTErrConvertFromNtStatus(rcNt));
                }
                break;

            default:
                return VINF_SUCCESS;
        }

        ASMNopPause();
        RT_NOREF(pVM);
    }
}
# endif /* IN_RING3 */
#endif /* NEM_WIN_USE_OUR_OWN_RUN_API || NEM_WIN_WITH_RING0_RUNLOOP */


#ifdef LOG_ENABLED
/**
 * Logs the current CPU state.
 */
NEM_TMPL_STATIC void nemHCWinLogState(PVMCC pVM, PVMCPUCC pVCpu)
{
    if (LogIs3Enabled())
    {
# if 0 // def IN_RING3 - causes lazy state import assertions all over CPUM.
        char szRegs[4096];
        DBGFR3RegPrintf(pVM->pUVM, pVCpu->idCpu, &szRegs[0], sizeof(szRegs),
                        "rax=%016VR{rax} rbx=%016VR{rbx} rcx=%016VR{rcx} rdx=%016VR{rdx}\n"
                        "rsi=%016VR{rsi} rdi=%016VR{rdi} r8 =%016VR{r8} r9 =%016VR{r9}\n"
                        "r10=%016VR{r10} r11=%016VR{r11} r12=%016VR{r12} r13=%016VR{r13}\n"
                        "r14=%016VR{r14} r15=%016VR{r15} %VRF{rflags}\n"
                        "rip=%016VR{rip} rsp=%016VR{rsp} rbp=%016VR{rbp}\n"
                        "cs={%04VR{cs} base=%016VR{cs_base} limit=%08VR{cs_lim} flags=%04VR{cs_attr}} cr0=%016VR{cr0}\n"
                        "ds={%04VR{ds} base=%016VR{ds_base} limit=%08VR{ds_lim} flags=%04VR{ds_attr}} cr2=%016VR{cr2}\n"
                        "es={%04VR{es} base=%016VR{es_base} limit=%08VR{es_lim} flags=%04VR{es_attr}} cr3=%016VR{cr3}\n"
                        "fs={%04VR{fs} base=%016VR{fs_base} limit=%08VR{fs_lim} flags=%04VR{fs_attr}} cr4=%016VR{cr4}\n"
                        "gs={%04VR{gs} base=%016VR{gs_base} limit=%08VR{gs_lim} flags=%04VR{gs_attr}} cr8=%016VR{cr8}\n"
                        "ss={%04VR{ss} base=%016VR{ss_base} limit=%08VR{ss_lim} flags=%04VR{ss_attr}}\n"
                        "dr0=%016VR{dr0} dr1=%016VR{dr1} dr2=%016VR{dr2} dr3=%016VR{dr3}\n"
                        "dr6=%016VR{dr6} dr7=%016VR{dr7}\n"
                        "gdtr=%016VR{gdtr_base}:%04VR{gdtr_lim}  idtr=%016VR{idtr_base}:%04VR{idtr_lim}  rflags=%08VR{rflags}\n"
                        "ldtr={%04VR{ldtr} base=%016VR{ldtr_base} limit=%08VR{ldtr_lim} flags=%08VR{ldtr_attr}}\n"
                        "tr  ={%04VR{tr} base=%016VR{tr_base} limit=%08VR{tr_lim} flags=%08VR{tr_attr}}\n"
                        "    sysenter={cs=%04VR{sysenter_cs} eip=%08VR{sysenter_eip} esp=%08VR{sysenter_esp}}\n"
                        "        efer=%016VR{efer}\n"
                        "         pat=%016VR{pat}\n"
                        "     sf_mask=%016VR{sf_mask}\n"
                        "krnl_gs_base=%016VR{krnl_gs_base}\n"
                        "       lstar=%016VR{lstar}\n"
                        "        star=%016VR{star} cstar=%016VR{cstar}\n"
                        "fcw=%04VR{fcw} fsw=%04VR{fsw} ftw=%04VR{ftw} mxcsr=%04VR{mxcsr} mxcsr_mask=%04VR{mxcsr_mask}\n"
                        );

        char szInstr[256];
        DBGFR3DisasInstrEx(pVM->pUVM, pVCpu->idCpu, 0, 0,
                           DBGF_DISAS_FLAGS_CURRENT_GUEST | DBGF_DISAS_FLAGS_DEFAULT_MODE,
                           szInstr, sizeof(szInstr), NULL);
        Log3(("%s%s\n", szRegs, szInstr));
# else
        /** @todo stat logging in ring-0 */
        RT_NOREF(pVM, pVCpu);
# endif
    }
}
#endif /* LOG_ENABLED */


/** Macro used by nemHCWinExecStateToLogStr and nemR3WinExecStateToLogStr. */
#define SWITCH_IT(a_szPrefix) \
    do \
        switch (u)\
        { \
            case 0x00: return a_szPrefix ""; \
            case 0x01: return a_szPrefix ",Pnd"; \
            case 0x02: return a_szPrefix ",Dbg"; \
            case 0x03: return a_szPrefix ",Pnd,Dbg"; \
            case 0x04: return a_szPrefix ",Shw"; \
            case 0x05: return a_szPrefix ",Pnd,Shw"; \
            case 0x06: return a_szPrefix ",Shw,Dbg"; \
            case 0x07: return a_szPrefix ",Pnd,Shw,Dbg"; \
            default: AssertFailedReturn("WTF?"); \
        } \
    while (0)

#ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
/**
 * Translates the execution stat bitfield into a short log string, VID version.
 *
 * @returns Read-only log string.
 * @param   pMsgHdr       The header which state to summarize.
 */
static const char *nemHCWinExecStateToLogStr(HV_X64_INTERCEPT_MESSAGE_HEADER const *pMsgHdr)
{
    unsigned u = (unsigned)pMsgHdr->ExecutionState.InterruptionPending
               | ((unsigned)pMsgHdr->ExecutionState.DebugActive << 1)
               | ((unsigned)pMsgHdr->ExecutionState.InterruptShadow << 2);
    if (pMsgHdr->ExecutionState.EferLma)
        SWITCH_IT("LM");
    else if (pMsgHdr->ExecutionState.Cr0Pe)
        SWITCH_IT("PM");
    else
        SWITCH_IT("RM");
}
#elif defined(IN_RING3)
/**
 * Translates the execution stat bitfield into a short log string, WinHv version.
 *
 * @returns Read-only log string.
 * @param   pExitCtx        The exit context which state to summarize.
 */
static const char *nemR3WinExecStateToLogStr(WHV_VP_EXIT_CONTEXT const *pExitCtx)
{
    unsigned u = (unsigned)pExitCtx->ExecutionState.InterruptionPending
               | ((unsigned)pExitCtx->ExecutionState.DebugActive << 1)
               | ((unsigned)pExitCtx->ExecutionState.InterruptShadow << 2);
    if (pExitCtx->ExecutionState.EferLma)
        SWITCH_IT("LM");
    else if (pExitCtx->ExecutionState.Cr0Pe)
        SWITCH_IT("PM");
    else
        SWITCH_IT("RM");
}
#endif /* IN_RING3 && !NEM_WIN_TEMPLATE_MODE_OWN_RUN_API */
#undef SWITCH_IT


#ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
/**
 * Advances the guest RIP and clear EFLAGS.RF, VID version.
 *
 * This may clear VMCPU_FF_INHIBIT_INTERRUPTS.
 *
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pExitCtx        The exit context.
 * @param   cbMinInstr      The minimum instruction length, or 1 if not unknown.
 */
DECLINLINE(void)
nemHCWinAdvanceGuestRipAndClearRF(PVMCPUCC pVCpu, HV_X64_INTERCEPT_MESSAGE_HEADER const *pMsgHdr, uint8_t cbMinInstr)
{
    Assert(!(pVCpu->cpum.GstCtx.fExtrn & (CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_RFLAGS)));

    /* Advance the RIP. */
    Assert(pMsgHdr->InstructionLength >= cbMinInstr); RT_NOREF_PV(cbMinInstr);
    pVCpu->cpum.GstCtx.rip += pMsgHdr->InstructionLength;
    pVCpu->cpum.GstCtx.rflags.Bits.u1RF = 0;

    /* Update interrupt inhibition. */
    if (!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS))
    { /* likely */ }
    else if (pVCpu->cpum.GstCtx.rip != EMGetInhibitInterruptsPC(pVCpu))
        VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS);
}
#elif defined(IN_RING3)
/**
 * Advances the guest RIP and clear EFLAGS.RF, WinHv version.
 *
 * This may clear VMCPU_FF_INHIBIT_INTERRUPTS.
 *
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pExitCtx        The exit context.
 * @param   cbMinInstr      The minimum instruction length, or 1 if not unknown.
 */
DECLINLINE(void) nemR3WinAdvanceGuestRipAndClearRF(PVMCPUCC pVCpu, WHV_VP_EXIT_CONTEXT const *pExitCtx, uint8_t cbMinInstr)
{
    Assert(!(pVCpu->cpum.GstCtx.fExtrn & (CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_RFLAGS)));

    /* Advance the RIP. */
    Assert(pExitCtx->InstructionLength >= cbMinInstr); RT_NOREF_PV(cbMinInstr);
    pVCpu->cpum.GstCtx.rip += pExitCtx->InstructionLength;
    pVCpu->cpum.GstCtx.rflags.Bits.u1RF = 0;

    /* Update interrupt inhibition. */
    if (!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS))
    { /* likely */ }
    else if (pVCpu->cpum.GstCtx.rip != EMGetInhibitInterruptsPC(pVCpu))
        VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS);
}
#endif /* IN_RING3 && !NEM_WIN_TEMPLATE_MODE_OWN_RUN_API */



NEM_TMPL_STATIC DECLCALLBACK(int)
nemHCWinUnmapOnePageCallback(PVMCC pVM, PVMCPUCC pVCpu, RTGCPHYS GCPhys, uint8_t *pu2NemState, void *pvUser)
{
    RT_NOREF_PV(pvUser);
#ifdef NEM_WIN_USE_HYPERCALLS_FOR_PAGES
    int rc = nemHCWinHypercallUnmapPage(pVM, pVCpu, GCPhys);
    AssertRC(rc);
    if (RT_SUCCESS(rc))
#else
    RT_NOREF_PV(pVCpu);
    HRESULT hrc = WHvUnmapGpaRange(pVM->nem.s.hPartition, GCPhys, X86_PAGE_SIZE);
    if (SUCCEEDED(hrc))
#endif
    {
        Log5(("NEM GPA unmap all: %RGp (cMappedPages=%u)\n", GCPhys, pVM->nem.s.cMappedPages - 1));
        *pu2NemState = NEM_WIN_PAGE_STATE_UNMAPPED;
    }
    else
    {
#ifdef NEM_WIN_USE_HYPERCALLS_FOR_PAGES
        LogRel(("nemR3WinUnmapOnePageCallback: GCPhys=%RGp rc=%Rrc\n", GCPhys, rc));
#else
        LogRel(("nemR3WinUnmapOnePageCallback: GCPhys=%RGp %s hrc=%Rhrc (%#x) Last=%#x/%u (cMappedPages=%u)\n",
                GCPhys, g_apszPageStates[*pu2NemState], hrc, hrc, RTNtLastStatusValue(),
                RTNtLastErrorValue(), pVM->nem.s.cMappedPages));
#endif
        *pu2NemState = NEM_WIN_PAGE_STATE_NOT_SET;
    }
    if (pVM->nem.s.cMappedPages > 0)
        ASMAtomicDecU32(&pVM->nem.s.cMappedPages);
    return VINF_SUCCESS;
}


/**
 * State to pass between nemHCWinHandleMemoryAccess / nemR3WinWHvHandleMemoryAccess
 * and nemHCWinHandleMemoryAccessPageCheckerCallback.
 */
typedef struct NEMHCWINHMACPCCSTATE
{
    /** Input: Write access. */
    bool    fWriteAccess;
    /** Output: Set if we did something. */
    bool    fDidSomething;
    /** Output: Set it we should resume. */
    bool    fCanResume;
} NEMHCWINHMACPCCSTATE;

/**
 * @callback_method_impl{FNPGMPHYSNEMCHECKPAGE,
 *      Worker for nemR3WinHandleMemoryAccess; pvUser points to a
 *      NEMHCWINHMACPCCSTATE structure. }
 */
NEM_TMPL_STATIC DECLCALLBACK(int)
nemHCWinHandleMemoryAccessPageCheckerCallback(PVMCC pVM, PVMCPUCC pVCpu, RTGCPHYS GCPhys, PPGMPHYSNEMPAGEINFO pInfo, void *pvUser)
{
    NEMHCWINHMACPCCSTATE *pState = (NEMHCWINHMACPCCSTATE *)pvUser;
    pState->fDidSomething = false;
    pState->fCanResume    = false;

    /* If A20 is disabled, we may need to make another query on the masked
       page to get the correct protection information. */
    uint8_t  u2State = pInfo->u2NemState;
    RTGCPHYS GCPhysSrc;
    if (   pVM->nem.s.fA20Enabled
        || !NEM_WIN_IS_SUBJECT_TO_A20(GCPhys))
        GCPhysSrc = GCPhys;
    else
    {
        GCPhysSrc = GCPhys & ~(RTGCPHYS)RT_BIT_32(20);
        PGMPHYSNEMPAGEINFO Info2;
        int rc = PGMPhysNemPageInfoChecker(pVM, pVCpu, GCPhysSrc, pState->fWriteAccess, &Info2, NULL, NULL);
        AssertRCReturn(rc, rc);

        *pInfo = Info2;
        pInfo->u2NemState = u2State;
    }

    /*
     * Consolidate current page state with actual page protection and access type.
     * We don't really consider downgrades here, as they shouldn't happen.
     */
#ifndef NEM_WIN_USE_HYPERCALLS_FOR_PAGES
    /** @todo Someone at microsoft please explain:
     * I'm not sure WTF was going on, but I ended up in a loop if I remapped a
     * readonly page as writable (unmap, then map again).  Specifically, this was an
     * issue with the big VRAM mapping at 0xe0000000 when booing DSL 4.4.1.  So, in
     * a hope to work around that we no longer pre-map anything, just unmap stuff
     * and do it lazily here.  And here we will first unmap, restart, and then remap
     * with new protection or backing.
     */
#endif
    int rc;
    switch (u2State)
    {
        case NEM_WIN_PAGE_STATE_UNMAPPED:
        case NEM_WIN_PAGE_STATE_NOT_SET:
            if (pInfo->fNemProt == NEM_PAGE_PROT_NONE)
            {
                Log4(("nemHCWinHandleMemoryAccessPageCheckerCallback: %RGp - #1\n", GCPhys));
                return VINF_SUCCESS;
            }

            /* Don't bother remapping it if it's a write request to a non-writable page. */
            if (   pState->fWriteAccess
                && !(pInfo->fNemProt & NEM_PAGE_PROT_WRITE))
            {
                Log4(("nemHCWinHandleMemoryAccessPageCheckerCallback: %RGp - #1w\n", GCPhys));
                return VINF_SUCCESS;
            }

            /* Map the page. */
            rc = nemHCNativeSetPhysPage(pVM,
                                        pVCpu,
                                        GCPhysSrc & ~(RTGCPHYS)X86_PAGE_OFFSET_MASK,
                                        GCPhys & ~(RTGCPHYS)X86_PAGE_OFFSET_MASK,
                                        pInfo->fNemProt,
                                        &u2State,
                                        true /*fBackingState*/);
            pInfo->u2NemState = u2State;
            Log4(("nemHCWinHandleMemoryAccessPageCheckerCallback: %RGp - synced => %s + %Rrc\n",
                  GCPhys, g_apszPageStates[u2State], rc));
            pState->fDidSomething = true;
            pState->fCanResume    = true;
            return rc;

        case NEM_WIN_PAGE_STATE_READABLE:
            if (   !(pInfo->fNemProt & NEM_PAGE_PROT_WRITE)
                && (pInfo->fNemProt & (NEM_PAGE_PROT_READ | NEM_PAGE_PROT_EXECUTE)))
            {
                Log4(("nemHCWinHandleMemoryAccessPageCheckerCallback: %RGp - #2\n", GCPhys));
                return VINF_SUCCESS;
            }

#ifdef NEM_WIN_USE_HYPERCALLS_FOR_PAGES
            /* Upgrade page to writable. */
/** @todo test this*/
            if (   (pInfo->fNemProt & NEM_PAGE_PROT_WRITE)
                && pState->fWriteAccess)
            {
                rc = nemHCWinHypercallMapPage(pVM, pVCpu, GCPhysSrc, GCPhys,
                                              HV_MAP_GPA_READABLE   | HV_MAP_GPA_WRITABLE
                                              | HV_MAP_GPA_EXECUTABLE | HV_MAP_GPA_EXECUTABLE_AGAIN);
                AssertRC(rc);
                if (RT_SUCCESS(rc))
                {
                    pInfo->u2NemState = NEM_WIN_PAGE_STATE_WRITABLE;
                    pState->fDidSomething = true;
                    pState->fCanResume    = true;
                    Log5(("NEM GPA write-upgrade/exit: %RGp (was %s, cMappedPages=%u)\n",
                          GCPhys, g_apszPageStates[u2State], pVM->nem.s.cMappedPages));
                }
            }
            else
            {
                /* Need to emulate the acces. */
                AssertBreak(pInfo->fNemProt != NEM_PAGE_PROT_NONE); /* There should be no downgrades. */
                rc = VINF_SUCCESS;
            }
            return rc;
#else
            break;
#endif

        case NEM_WIN_PAGE_STATE_WRITABLE:
            if (pInfo->fNemProt & NEM_PAGE_PROT_WRITE)
            {
                if (pInfo->u2OldNemState == NEM_WIN_PAGE_STATE_WRITABLE)
                    Log4(("nemHCWinHandleMemoryAccessPageCheckerCallback: %RGp - #3a\n", GCPhys));
                else
                {
                    pState->fCanResume = true;
                    Log4(("nemHCWinHandleMemoryAccessPageCheckerCallback: %RGp - #3b (%s -> %s)\n",
                          GCPhys, g_apszPageStates[pInfo->u2OldNemState], g_apszPageStates[u2State]));
                }
                return VINF_SUCCESS;
            }
#ifdef NEM_WIN_USE_HYPERCALLS_FOR_PAGES
            AssertFailed(); /* There should be no downgrades. */
#endif
            break;

        default:
            AssertLogRelMsgFailedReturn(("u2State=%#x\n", u2State), VERR_NEM_IPE_4);
    }

    /*
     * Unmap and restart the instruction.
     * If this fails, which it does every so often, just unmap everything for now.
     */
#ifdef NEM_WIN_USE_HYPERCALLS_FOR_PAGES
    rc = nemHCWinHypercallUnmapPage(pVM, pVCpu, GCPhys);
    AssertRC(rc);
    if (RT_SUCCESS(rc))
#else
    /** @todo figure out whether we mess up the state or if it's WHv.   */
    HRESULT hrc = WHvUnmapGpaRange(pVM->nem.s.hPartition, GCPhys, X86_PAGE_SIZE);
    if (SUCCEEDED(hrc))
#endif
    {
        pState->fDidSomething = true;
        pState->fCanResume    = true;
        pInfo->u2NemState = NEM_WIN_PAGE_STATE_UNMAPPED;
        uint32_t cMappedPages = ASMAtomicDecU32(&pVM->nem.s.cMappedPages); NOREF(cMappedPages);
        Log5(("NEM GPA unmapped/exit: %RGp (was %s, cMappedPages=%u)\n", GCPhys, g_apszPageStates[u2State], cMappedPages));
        return VINF_SUCCESS;
    }
#ifdef NEM_WIN_USE_HYPERCALLS_FOR_PAGES
    LogRel(("nemHCWinHandleMemoryAccessPageCheckerCallback/unmap: GCPhysDst=%RGp rc=%Rrc\n", GCPhys, rc));
    return rc;
#else
    LogRel(("nemHCWinHandleMemoryAccessPageCheckerCallback/unmap: GCPhysDst=%RGp %s hrc=%Rhrc (%#x) Last=%#x/%u (cMappedPages=%u)\n",
            GCPhys, g_apszPageStates[u2State], hrc, hrc, RTNtLastStatusValue(), RTNtLastErrorValue(),
            pVM->nem.s.cMappedPages));

    PGMPhysNemEnumPagesByState(pVM, pVCpu, NEM_WIN_PAGE_STATE_READABLE, nemR3WinUnmapOnePageCallback, NULL);
    Log(("nemHCWinHandleMemoryAccessPageCheckerCallback: Unmapped all (cMappedPages=%u)\n", pVM->nem.s.cMappedPages));

    pState->fDidSomething = true;
    pState->fCanResume    = true;
    pInfo->u2NemState = NEM_WIN_PAGE_STATE_UNMAPPED;
    return VINF_SUCCESS;
#endif
}



#if defined(IN_RING0) && defined(NEM_WIN_TEMPLATE_MODE_OWN_RUN_API)
/**
 * Wrapper around nemR0WinImportState that converts VERR_NEM_FLUSH_TLB
 * into informational status codes and logs+asserts statuses.
 *
 * @returns VBox strict status code.
 * @param   pGVM            The global (ring-0) VM structure.
 * @param   pGVCpu          The global (ring-0) per CPU structure.
 * @param   fWhat           What to import.
 * @param   pszCaller       Who is doing the importing.
 */
DECLINLINE(VBOXSTRICTRC) nemR0WinImportStateStrict(PGVM pGVM, PGVMCPU pGVCpu, uint64_t fWhat, const char *pszCaller)
{
    int rc = nemR0WinImportState(pGVM, pGVCpu, &pGVCpu->cpum.GstCtx, fWhat, true /*fCanUpdateCr3*/);
    if (RT_SUCCESS(rc))
    {
        Assert(rc == VINF_SUCCESS);
        return VINF_SUCCESS;
    }

    if (rc == VERR_NEM_FLUSH_TLB)
    {
        Log4(("%s/%u: nemR0WinImportState -> %Rrc\n", pszCaller, pGVCpu->idCpu, -rc));
        return -rc;
    }
    RT_NOREF(pszCaller);
    AssertMsgFailedReturn(("%s/%u: nemR0WinImportState failed: %Rrc\n", pszCaller, pGVCpu->idCpu, rc), rc);
}
#endif /* IN_RING0 && NEM_WIN_TEMPLATE_MODE_OWN_RUN_API*/

#if defined(NEM_WIN_TEMPLATE_MODE_OWN_RUN_API) || defined(IN_RING3)
/**
 * Wrapper around nemR0WinImportStateStrict and nemHCWinCopyStateFromHyperV.
 *
 * Unlike the wrapped APIs, this checks whether it's necessary.
 *
 * @returns VBox strict status code.
 * @param   pVCpu           The cross context per CPU structure.
 * @param   fWhat           What to import.
 * @param   pszCaller       Who is doing the importing.
 */
DECLINLINE(VBOXSTRICTRC) nemHCWinImportStateIfNeededStrict(PVMCPUCC pVCpu, uint64_t fWhat, const char *pszCaller)
{
    if (pVCpu->cpum.GstCtx.fExtrn & fWhat)
    {
# ifdef IN_RING0
        return nemR0WinImportStateStrict(pVCpu->pGVM, pVCpu, fWhat, pszCaller);
# else
        RT_NOREF(pszCaller);
        int rc = nemHCWinCopyStateFromHyperV(pVCpu->pVMR3, pVCpu, fWhat);
        AssertRCReturn(rc, rc);
# endif
    }
    return VINF_SUCCESS;
}
#endif /* NEM_WIN_TEMPLATE_MODE_OWN_RUN_API || IN_RING3 */

#ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
/**
 * Copies register state from the X64 intercept message header.
 *
 * ASSUMES no state copied yet.
 *
 * @param   pVCpu           The cross context per CPU structure.
 * @param   pHdr            The X64 intercept message header.
 * @sa      nemR3WinCopyStateFromX64Header
 */
DECLINLINE(void) nemHCWinCopyStateFromX64Header(PVMCPUCC pVCpu, HV_X64_INTERCEPT_MESSAGE_HEADER const *pHdr)
{
    Assert(   (pVCpu->cpum.GstCtx.fExtrn & (CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_RFLAGS | CPUMCTX_EXTRN_CS | CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT))
           ==                              (CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_RFLAGS | CPUMCTX_EXTRN_CS | CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT));
    NEM_WIN_COPY_BACK_SEG(pVCpu->cpum.GstCtx.cs, pHdr->CsSegment);
    pVCpu->cpum.GstCtx.rip      = pHdr->Rip;
    pVCpu->cpum.GstCtx.rflags.u = pHdr->Rflags;

    pVCpu->nem.s.fLastInterruptShadow = pHdr->ExecutionState.InterruptShadow;
    if (!pHdr->ExecutionState.InterruptShadow)
    {
        if (!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS))
        { /* likely */ }
        else
            VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS);
    }
    else
        EMSetInhibitInterruptsPC(pVCpu, pHdr->Rip);

    pVCpu->cpum.GstCtx.fExtrn &= ~(CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_RFLAGS | CPUMCTX_EXTRN_CS | CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT);
}
#elif defined(IN_RING3)
/**
 * Copies register state from the (common) exit context.
 *
 * ASSUMES no state copied yet.
 *
 * @param   pVCpu           The cross context per CPU structure.
 * @param   pExitCtx        The common exit context.
 * @sa      nemHCWinCopyStateFromX64Header
 */
DECLINLINE(void) nemR3WinCopyStateFromX64Header(PVMCPUCC pVCpu, WHV_VP_EXIT_CONTEXT const *pExitCtx)
{
    Assert(   (pVCpu->cpum.GstCtx.fExtrn & (CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_RFLAGS | CPUMCTX_EXTRN_CS | CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT))
           ==                              (CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_RFLAGS | CPUMCTX_EXTRN_CS | CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT));
    NEM_WIN_COPY_BACK_SEG(pVCpu->cpum.GstCtx.cs, pExitCtx->Cs);
    pVCpu->cpum.GstCtx.rip      = pExitCtx->Rip;
    pVCpu->cpum.GstCtx.rflags.u = pExitCtx->Rflags;

    pVCpu->nem.s.fLastInterruptShadow = pExitCtx->ExecutionState.InterruptShadow;
    if (!pExitCtx->ExecutionState.InterruptShadow)
    {
        if (!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS))
        { /* likely */ }
        else
            VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS);
    }
    else
        EMSetInhibitInterruptsPC(pVCpu, pExitCtx->Rip);

    pVCpu->cpum.GstCtx.fExtrn &= ~(CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_RFLAGS | CPUMCTX_EXTRN_CS | CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT);
}
#endif /* IN_RING3 && !NEM_WIN_TEMPLATE_MODE_OWN_RUN_API */


#ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
/**
 * Deals with memory intercept message.
 *
 * @returns Strict VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context per CPU structure.
 * @param   pMsg            The message.
 * @sa      nemR3WinHandleExitMemory
 */
NEM_TMPL_STATIC VBOXSTRICTRC
nemHCWinHandleMessageMemory(PVMCC pVM, PVMCPUCC pVCpu, HV_X64_MEMORY_INTERCEPT_MESSAGE const *pMsg)
{
    uint64_t const uHostTsc = ASMReadTSC();
    Assert(   pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_READ
           || pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_WRITE
           || pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_EXECUTE);

    /*
     * Whatever we do, we must clear pending event injection upon resume.
     */
    if (pMsg->Header.ExecutionState.InterruptionPending)
        pVCpu->cpum.GstCtx.fExtrn &= ~CPUMCTX_EXTRN_NEM_WIN_EVENT_INJECT;

# if 0 /* Experiment: 20K -> 34K exit/s. */
    if (   pMsg->Header.ExecutionState.EferLma
        && pMsg->Header.CsSegment.Long
        && pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_WRITE)
    {
        if (   pMsg->Header.Rip - (uint64_t)0xf65a < (uint64_t)(0xf662 - 0xf65a)
            && pMsg->InstructionBytes[0] == 0x89
            && pMsg->InstructionBytes[1] == 0x03)
        {
            pVCpu->cpum.GstCtx.rip    = pMsg->Header.Rip + 2;
            pVCpu->cpum.GstCtx.fExtrn &= ~CPUMCTX_EXTRN_RIP;
            AssertMsg(pMsg->Header.InstructionLength == 2, ("%#x\n", pMsg->Header.InstructionLength));
            //Log(("%RX64 msg:\n%.80Rhxd\n", pVCpu->cpum.GstCtx.rip, pMsg));
            return VINF_SUCCESS;
        }
    }
# endif

    /*
     * Ask PGM for information about the given GCPhys.  We need to check if we're
     * out of sync first.
     */
    NEMHCWINHMACPCCSTATE State = { pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_WRITE, false, false };
    PGMPHYSNEMPAGEINFO   Info;
    int rc = PGMPhysNemPageInfoChecker(pVM, pVCpu, pMsg->GuestPhysicalAddress, State.fWriteAccess, &Info,
                                       nemHCWinHandleMemoryAccessPageCheckerCallback, &State);
    if (RT_SUCCESS(rc))
    {
        if (Info.fNemProt & (  pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_WRITE
                             ? NEM_PAGE_PROT_WRITE : NEM_PAGE_PROT_READ))
        {
            if (State.fCanResume)
            {
                Log4(("MemExit/%u: %04x:%08RX64/%s: %RGp (=>%RHp) %s fProt=%u%s%s%s; restarting (%s)\n",
                      pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
                      pMsg->GuestPhysicalAddress, Info.HCPhys, g_apszPageStates[Info.u2NemState], Info.fNemProt,
                      Info.fHasHandlers ? " handlers" : "", Info.fZeroPage    ? " zero-pg" : "",
                      State.fDidSomething ? "" : " no-change", g_apszHvInterceptAccessTypes[pMsg->Header.InterceptAccessType]));
                EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_MEMORY_ACCESS),
                                 pMsg->Header.Rip + pMsg->Header.CsSegment.Base, uHostTsc);
                return VINF_SUCCESS;
            }
        }
        Log4(("MemExit/%u: %04x:%08RX64/%s: %RGp (=>%RHp) %s fProt=%u%s%s%s; emulating (%s)\n",
              pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
              pMsg->GuestPhysicalAddress, Info.HCPhys, g_apszPageStates[Info.u2NemState], Info.fNemProt,
              Info.fHasHandlers ? " handlers" : "", Info.fZeroPage    ? " zero-pg" : "",
              State.fDidSomething ? "" : " no-change", g_apszHvInterceptAccessTypes[pMsg->Header.InterceptAccessType]));
    }
    else
        Log4(("MemExit/%u: %04x:%08RX64/%s: %RGp rc=%Rrc%s; emulating (%s)\n",
              pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
              pMsg->GuestPhysicalAddress, rc, State.fDidSomething ? " modified-backing" : "",
              g_apszHvInterceptAccessTypes[pMsg->Header.InterceptAccessType]));

    /*
     * Emulate the memory access, either access handler or special memory.
     */
    PCEMEXITREC pExitRec = EMHistoryAddExit(pVCpu,
                                              pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_WRITE
                                            ? EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_MMIO_WRITE)
                                            : EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_MMIO_READ),
                                            pMsg->Header.Rip + pMsg->Header.CsSegment.Base, uHostTsc);
    nemHCWinCopyStateFromX64Header(pVCpu, &pMsg->Header);
    VBOXSTRICTRC rcStrict;
# ifdef IN_RING0
    rcStrict = nemR0WinImportStateStrict(pVM, pVCpu,
                                         NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM | CPUMCTX_EXTRN_DS | CPUMCTX_EXTRN_ES, "MemExit");
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;
# else
    rc = nemHCWinCopyStateFromHyperV(pVM, pVCpu, NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM | CPUMCTX_EXTRN_DS | CPUMCTX_EXTRN_ES);
    AssertRCReturn(rc, rc);
# endif

    if (pMsg->Reserved1)
        Log(("MemExit/Reserved1=%#x\n", pMsg->Reserved1));
    if (pMsg->Header.ExecutionState.Reserved0 || pMsg->Header.ExecutionState.Reserved1)
        Log(("MemExit/Hdr/State: Reserved0=%#x Reserved1=%#x\n", pMsg->Header.ExecutionState.Reserved0, pMsg->Header.ExecutionState.Reserved1));

    if (!pExitRec)
    {
        //if (pMsg->InstructionByteCount > 0)
        //    Log4(("InstructionByteCount=%#x %.16Rhxs\n", pMsg->InstructionByteCount, pMsg->InstructionBytes));
        if (pMsg->InstructionByteCount > 0)
            rcStrict = IEMExecOneWithPrefetchedByPC(pVCpu, CPUMCTX2CORE(&pVCpu->cpum.GstCtx), pMsg->Header.Rip,
                                                    pMsg->InstructionBytes, pMsg->InstructionByteCount);
        else
            rcStrict = IEMExecOne(pVCpu);
        /** @todo do we need to do anything wrt debugging here?   */
    }
    else
    {
        /* Frequent access or probing. */
        rcStrict = EMHistoryExec(pVCpu, pExitRec, 0);
        Log4(("MemExit/%u: %04x:%08RX64/%s: EMHistoryExec -> %Rrc + %04x:%08RX64\n",
              pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
              VBOXSTRICTRC_VAL(rcStrict), pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip));
    }
    return rcStrict;
}
#elif defined(IN_RING3)
/**
 * Deals with memory access exits (WHvRunVpExitReasonMemoryAccess).
 *
 * @returns Strict VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context per CPU structure.
 * @param   pExit           The VM exit information to handle.
 * @sa      nemHCWinHandleMessageMemory
 */
NEM_TMPL_STATIC VBOXSTRICTRC
nemR3WinHandleExitMemory(PVMCC pVM, PVMCPUCC pVCpu, WHV_RUN_VP_EXIT_CONTEXT const *pExit)
{
    uint64_t const uHostTsc = ASMReadTSC();
    Assert(pExit->MemoryAccess.AccessInfo.AccessType != 3);

    /*
     * Whatever we do, we must clear pending event injection upon resume.
     */
    if (pExit->VpContext.ExecutionState.InterruptionPending)
        pVCpu->cpum.GstCtx.fExtrn &= ~CPUMCTX_EXTRN_NEM_WIN_EVENT_INJECT;

    /*
     * Ask PGM for information about the given GCPhys.  We need to check if we're
     * out of sync first.
     */
    NEMHCWINHMACPCCSTATE State = { pExit->MemoryAccess.AccessInfo.AccessType == WHvMemoryAccessWrite, false, false };
    PGMPHYSNEMPAGEINFO   Info;
    int rc = PGMPhysNemPageInfoChecker(pVM, pVCpu, pExit->MemoryAccess.Gpa, State.fWriteAccess, &Info,
                                       nemHCWinHandleMemoryAccessPageCheckerCallback, &State);
    if (RT_SUCCESS(rc))
    {
        if (Info.fNemProt & (  pExit->MemoryAccess.AccessInfo.AccessType == WHvMemoryAccessWrite
                             ? NEM_PAGE_PROT_WRITE : NEM_PAGE_PROT_READ))
        {
            if (State.fCanResume)
            {
                Log4(("MemExit/%u: %04x:%08RX64/%s: %RGp (=>%RHp) %s fProt=%u%s%s%s; restarting (%s)\n",
                      pVCpu->idCpu, pExit->VpContext.Cs.Selector, pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext),
                      pExit->MemoryAccess.Gpa, Info.HCPhys, g_apszPageStates[Info.u2NemState], Info.fNemProt,
                      Info.fHasHandlers ? " handlers" : "", Info.fZeroPage    ? " zero-pg" : "",
                      State.fDidSomething ? "" : " no-change", g_apszHvInterceptAccessTypes[pExit->MemoryAccess.AccessInfo.AccessType]));
                EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_MEMORY_ACCESS),
                                 pExit->VpContext.Rip + pExit->VpContext.Cs.Base, uHostTsc);
                return VINF_SUCCESS;
            }
        }
        Log4(("MemExit/%u: %04x:%08RX64/%s: %RGp (=>%RHp) %s fProt=%u%s%s%s; emulating (%s)\n",
              pVCpu->idCpu, pExit->VpContext.Cs.Selector, pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext),
              pExit->MemoryAccess.Gpa, Info.HCPhys, g_apszPageStates[Info.u2NemState], Info.fNemProt,
              Info.fHasHandlers ? " handlers" : "", Info.fZeroPage    ? " zero-pg" : "",
              State.fDidSomething ? "" : " no-change", g_apszHvInterceptAccessTypes[pExit->MemoryAccess.AccessInfo.AccessType]));
    }
    else
        Log4(("MemExit/%u: %04x:%08RX64/%s: %RGp rc=%Rrc%s; emulating (%s)\n",
              pVCpu->idCpu, pExit->VpContext.Cs.Selector, pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext),
              pExit->MemoryAccess.Gpa, rc, State.fDidSomething ? " modified-backing" : "",
              g_apszHvInterceptAccessTypes[pExit->MemoryAccess.AccessInfo.AccessType]));

    /*
     * Emulate the memory access, either access handler or special memory.
     */
    PCEMEXITREC pExitRec = EMHistoryAddExit(pVCpu,
                                              pExit->MemoryAccess.AccessInfo.AccessType == WHvMemoryAccessWrite
                                            ? EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_MMIO_WRITE)
                                            : EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_MMIO_READ),
                                            pExit->VpContext.Rip + pExit->VpContext.Cs.Base, uHostTsc);
    nemR3WinCopyStateFromX64Header(pVCpu, &pExit->VpContext);
    rc = nemHCWinCopyStateFromHyperV(pVM, pVCpu, NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM | CPUMCTX_EXTRN_DS | CPUMCTX_EXTRN_ES);
    AssertRCReturn(rc, rc);
    if (pExit->VpContext.ExecutionState.Reserved0 || pExit->VpContext.ExecutionState.Reserved1)
        Log(("MemExit/Hdr/State: Reserved0=%#x Reserved1=%#x\n", pExit->VpContext.ExecutionState.Reserved0, pExit->VpContext.ExecutionState.Reserved1));

    VBOXSTRICTRC rcStrict;
    if (!pExitRec)
    {
        //if (pMsg->InstructionByteCount > 0)
        //    Log4(("InstructionByteCount=%#x %.16Rhxs\n", pMsg->InstructionByteCount, pMsg->InstructionBytes));
        if (pExit->MemoryAccess.InstructionByteCount > 0)
            rcStrict = IEMExecOneWithPrefetchedByPC(pVCpu, CPUMCTX2CORE(&pVCpu->cpum.GstCtx), pExit->VpContext.Rip,
                                                    pExit->MemoryAccess.InstructionBytes, pExit->MemoryAccess.InstructionByteCount);
        else
            rcStrict = IEMExecOne(pVCpu);
        /** @todo do we need to do anything wrt debugging here?   */
    }
    else
    {
        /* Frequent access or probing. */
        rcStrict = EMHistoryExec(pVCpu, pExitRec, 0);
        Log4(("MemExit/%u: %04x:%08RX64/%s: EMHistoryExec -> %Rrc + %04x:%08RX64\n",
              pVCpu->idCpu, pExit->VpContext.Cs.Selector, pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext),
              VBOXSTRICTRC_VAL(rcStrict), pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip));
    }
    return rcStrict;
}
#endif /* IN_RING3 && !NEM_WIN_TEMPLATE_MODE_OWN_RUN_API */


#ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
/**
 * Deals with I/O port intercept message.
 *
 * @returns Strict VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context per CPU structure.
 * @param   pMsg            The message.
 */
NEM_TMPL_STATIC VBOXSTRICTRC
nemHCWinHandleMessageIoPort(PVMCC pVM, PVMCPUCC pVCpu, HV_X64_IO_PORT_INTERCEPT_MESSAGE const *pMsg)
{
    /*
     * Assert message sanity.
     */
    Assert(   pMsg->AccessInfo.AccessSize == 1
           || pMsg->AccessInfo.AccessSize == 2
           || pMsg->AccessInfo.AccessSize == 4);
    Assert(   pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_READ
           || pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_WRITE);
    NEMWIN_ASSERT_MSG_REG_SEG(  pVCpu, HvX64RegisterCs, pMsg->Header.CsSegment);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRip, pMsg->Header.Rip);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRflags, pMsg->Header.Rflags);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterCr8, (uint64_t)pMsg->Header.Cr8);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRax, pMsg->Rax);
    if (pMsg->AccessInfo.StringOp)
    {
        NEMWIN_ASSERT_MSG_REG_SEG(  pVCpu, HvX64RegisterDs,  pMsg->DsSegment);
        NEMWIN_ASSERT_MSG_REG_SEG(  pVCpu, HvX64RegisterEs,  pMsg->EsSegment);
        NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRcx, pMsg->Rcx);
        NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRsi, pMsg->Rsi);
        NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRdi, pMsg->Rdi);
    }

    /*
     * Whatever we do, we must clear pending event injection upon resume.
     */
    if (pMsg->Header.ExecutionState.InterruptionPending)
        pVCpu->cpum.GstCtx.fExtrn &= ~CPUMCTX_EXTRN_NEM_WIN_EVENT_INJECT;

    /*
     * Add history first to avoid two paths doing EMHistoryExec calls.
     */
    VBOXSTRICTRC rcStrict;
    PCEMEXITREC pExitRec = EMHistoryAddExit(pVCpu,
                                            !pMsg->AccessInfo.StringOp
                                            ? (  pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_WRITE
                                               ? EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_IO_PORT_WRITE)
                                               : EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_IO_PORT_READ))
                                            : (  pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_WRITE
                                               ? EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_IO_PORT_STR_WRITE)
                                               : EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_IO_PORT_STR_READ)),
                                            pMsg->Header.Rip + pMsg->Header.CsSegment.Base, ASMReadTSC());
    if (!pExitRec)
    {
        if (!pMsg->AccessInfo.StringOp)
        {
            /*
             * Simple port I/O.
             */
            static uint32_t const s_fAndMask[8] =
            {   UINT32_MAX, UINT32_C(0xff), UINT32_C(0xffff), UINT32_MAX,   UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX   };
            uint32_t const        fAndMask      = s_fAndMask[pMsg->AccessInfo.AccessSize];

            nemHCWinCopyStateFromX64Header(pVCpu, &pMsg->Header);
            if (pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_WRITE)
            {
                rcStrict = IOMIOPortWrite(pVM, pVCpu, pMsg->PortNumber, (uint32_t)pMsg->Rax & fAndMask, pMsg->AccessInfo.AccessSize);
                Log4(("IOExit/%u: %04x:%08RX64/%s: OUT %#x, %#x LB %u rcStrict=%Rrc\n",
                      pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
                      pMsg->PortNumber, (uint32_t)pMsg->Rax & fAndMask, pMsg->AccessInfo.AccessSize, VBOXSTRICTRC_VAL(rcStrict) ));
                if (IOM_SUCCESS(rcStrict))
                    nemHCWinAdvanceGuestRipAndClearRF(pVCpu, &pMsg->Header, 1);
# ifdef IN_RING0
                else if (   rcStrict == VINF_IOM_R3_IOPORT_WRITE
                         && !pVCpu->cpum.GstCtx.rflags.Bits.u1TF
                         /** @todo check for debug breakpoints */ )
                    return EMRZSetPendingIoPortWrite(pVCpu, pMsg->PortNumber, pMsg->Header.InstructionLength,
                                                     pMsg->AccessInfo.AccessSize, (uint32_t)pMsg->Rax & fAndMask);
# endif
                else
                {
                    pVCpu->cpum.GstCtx.rax = pMsg->Rax;
                    pVCpu->cpum.GstCtx.fExtrn &= ~CPUMCTX_EXTRN_RAX;
                }
            }
            else
            {
                uint32_t uValue = 0;
                rcStrict = IOMIOPortRead(pVM, pVCpu, pMsg->PortNumber, &uValue, pMsg->AccessInfo.AccessSize);
                Log4(("IOExit/%u: %04x:%08RX64/%s: IN %#x LB %u -> %#x, rcStrict=%Rrc\n",
                      pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
                      pMsg->PortNumber, pMsg->AccessInfo.AccessSize, uValue, VBOXSTRICTRC_VAL(rcStrict) ));
                if (IOM_SUCCESS(rcStrict))
                {
                    if (pMsg->AccessInfo.AccessSize != 4)
                        pVCpu->cpum.GstCtx.rax = (pMsg->Rax & ~(uint64_t)fAndMask) | (uValue & fAndMask);
                    else
                        pVCpu->cpum.GstCtx.rax = uValue;
                    pVCpu->cpum.GstCtx.fExtrn &= ~CPUMCTX_EXTRN_RAX;
                    Log4(("IOExit/%u: RAX %#RX64 -> %#RX64\n", pVCpu->idCpu, pMsg->Rax, pVCpu->cpum.GstCtx.rax));
                    nemHCWinAdvanceGuestRipAndClearRF(pVCpu, &pMsg->Header, 1);
                }
                else
                {
                    pVCpu->cpum.GstCtx.rax = pMsg->Rax;
                    pVCpu->cpum.GstCtx.fExtrn &= ~CPUMCTX_EXTRN_RAX;
# ifdef IN_RING0
                    if (   rcStrict == VINF_IOM_R3_IOPORT_READ
                        && !pVCpu->cpum.GstCtx.rflags.Bits.u1TF
                        /** @todo check for debug breakpoints */ )
                        return EMRZSetPendingIoPortRead(pVCpu, pMsg->PortNumber, pMsg->Header.InstructionLength,
                                                        pMsg->AccessInfo.AccessSize);
# endif
                }
            }
        }
        else
        {
            /*
             * String port I/O.
             */
            /** @todo Someone at Microsoft please explain how we can get the address mode
             * from the IoPortAccess.VpContext.  CS.Attributes is only sufficient for
             * getting the default mode, it can always be overridden by a prefix.   This
             * forces us to interpret the instruction from opcodes, which is suboptimal.
             * Both AMD-V and VT-x includes the address size in the exit info, at least on
             * CPUs that are reasonably new.
             *
             * Of course, it's possible this is an undocumented and we just need to do some
             * experiments to figure out how it's communicated.  Alternatively, we can scan
             * the opcode bytes for possible evil prefixes.
             */
            nemHCWinCopyStateFromX64Header(pVCpu, &pMsg->Header);
            pVCpu->cpum.GstCtx.fExtrn &= ~(  CPUMCTX_EXTRN_RAX | CPUMCTX_EXTRN_RCX | CPUMCTX_EXTRN_RDI | CPUMCTX_EXTRN_RSI
                              | CPUMCTX_EXTRN_DS  | CPUMCTX_EXTRN_ES);
            NEM_WIN_COPY_BACK_SEG(pVCpu->cpum.GstCtx.ds, pMsg->DsSegment);
            NEM_WIN_COPY_BACK_SEG(pVCpu->cpum.GstCtx.es, pMsg->EsSegment);
            pVCpu->cpum.GstCtx.rax = pMsg->Rax;
            pVCpu->cpum.GstCtx.rcx = pMsg->Rcx;
            pVCpu->cpum.GstCtx.rdi = pMsg->Rdi;
            pVCpu->cpum.GstCtx.rsi = pMsg->Rsi;
# ifdef IN_RING0
            rcStrict = nemR0WinImportStateStrict(pVM, pVCpu, NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM, "IOExit");
            if (rcStrict != VINF_SUCCESS)
                return rcStrict;
# else
            int rc = nemHCWinCopyStateFromHyperV(pVM, pVCpu, NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM);
            AssertRCReturn(rc, rc);
# endif

            Log4(("IOExit/%u: %04x:%08RX64/%s: %s%s %#x LB %u (emulating)\n",
                  pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
                  pMsg->AccessInfo.RepPrefix ? "REP " : "",
                  pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_WRITE ? "OUTS" : "INS",
                  pMsg->PortNumber, pMsg->AccessInfo.AccessSize ));
            rcStrict = IEMExecOne(pVCpu);
        }
        if (IOM_SUCCESS(rcStrict))
        {
            /*
             * Do debug checks.
             */
            if (   pMsg->Header.ExecutionState.DebugActive /** @todo Microsoft: Does DebugActive this only reflect DR7? */
                || (pMsg->Header.Rflags & X86_EFL_TF)
                || DBGFBpIsHwIoArmed(pVM) )
            {
                /** @todo Debugging. */
            }
        }
        return rcStrict;
    }

    /*
     * Frequent exit or something needing probing.
     * Get state and call EMHistoryExec.
     */
    nemHCWinCopyStateFromX64Header(pVCpu, &pMsg->Header);
    if (!pMsg->AccessInfo.StringOp)
        pVCpu->cpum.GstCtx.fExtrn &= ~CPUMCTX_EXTRN_RAX;
    else
    {
        pVCpu->cpum.GstCtx.fExtrn &= ~(  CPUMCTX_EXTRN_RAX | CPUMCTX_EXTRN_RCX | CPUMCTX_EXTRN_RDI | CPUMCTX_EXTRN_RSI
                          | CPUMCTX_EXTRN_DS  | CPUMCTX_EXTRN_ES);
        NEM_WIN_COPY_BACK_SEG(pVCpu->cpum.GstCtx.ds, pMsg->DsSegment);
        NEM_WIN_COPY_BACK_SEG(pVCpu->cpum.GstCtx.es, pMsg->EsSegment);
        pVCpu->cpum.GstCtx.rcx = pMsg->Rcx;
        pVCpu->cpum.GstCtx.rdi = pMsg->Rdi;
        pVCpu->cpum.GstCtx.rsi = pMsg->Rsi;
    }
    pVCpu->cpum.GstCtx.rax = pMsg->Rax;

# ifdef IN_RING0
    rcStrict = nemR0WinImportStateStrict(pVM, pVCpu, NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM, "IOExit");
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;
# else
    int rc = nemHCWinCopyStateFromHyperV(pVM, pVCpu, NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM);
    AssertRCReturn(rc, rc);
# endif

    Log4(("IOExit/%u: %04x:%08RX64/%s: %s%s%s %#x LB %u -> EMHistoryExec\n",
          pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
          pMsg->AccessInfo.RepPrefix ? "REP " : "",
          pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_WRITE ? "OUT" : "IN",
          pMsg->AccessInfo.StringOp ? "S" : "",
          pMsg->PortNumber, pMsg->AccessInfo.AccessSize));
    rcStrict = EMHistoryExec(pVCpu, pExitRec, 0);
    Log4(("IOExit/%u: %04x:%08RX64/%s: EMHistoryExec -> %Rrc + %04x:%08RX64\n",
          pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
          VBOXSTRICTRC_VAL(rcStrict), pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip));
    return rcStrict;
}
#elif defined(IN_RING3)
/**
 * Deals with I/O port access exits (WHvRunVpExitReasonX64IoPortAccess).
 *
 * @returns Strict VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context per CPU structure.
 * @param   pExit           The VM exit information to handle.
 * @sa      nemHCWinHandleMessageIoPort
 */
NEM_TMPL_STATIC VBOXSTRICTRC nemR3WinHandleExitIoPort(PVMCC pVM, PVMCPUCC pVCpu, WHV_RUN_VP_EXIT_CONTEXT const *pExit)
{
    Assert(   pExit->IoPortAccess.AccessInfo.AccessSize == 1
           || pExit->IoPortAccess.AccessInfo.AccessSize == 2
           || pExit->IoPortAccess.AccessInfo.AccessSize == 4);

    /*
     * Whatever we do, we must clear pending event injection upon resume.
     */
    if (pExit->VpContext.ExecutionState.InterruptionPending)
        pVCpu->cpum.GstCtx.fExtrn &= ~CPUMCTX_EXTRN_NEM_WIN_EVENT_INJECT;

    /*
     * Add history first to avoid two paths doing EMHistoryExec calls.
     */
    PCEMEXITREC pExitRec = EMHistoryAddExit(pVCpu,
                                            !pExit->IoPortAccess.AccessInfo.StringOp
                                            ? (  pExit->MemoryAccess.AccessInfo.AccessType == WHvMemoryAccessWrite
                                               ? EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_IO_PORT_WRITE)
                                               : EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_IO_PORT_READ))
                                            : (  pExit->MemoryAccess.AccessInfo.AccessType == WHvMemoryAccessWrite
                                               ? EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_IO_PORT_STR_WRITE)
                                               : EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_IO_PORT_STR_READ)),
                                            pExit->VpContext.Rip + pExit->VpContext.Cs.Base, ASMReadTSC());
    if (!pExitRec)
    {
        VBOXSTRICTRC rcStrict;
        if (!pExit->IoPortAccess.AccessInfo.StringOp)
        {
            /*
             * Simple port I/O.
             */
            static uint32_t const s_fAndMask[8] =
            {   UINT32_MAX, UINT32_C(0xff), UINT32_C(0xffff), UINT32_MAX,   UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX   };
            uint32_t const        fAndMask      = s_fAndMask[pExit->IoPortAccess.AccessInfo.AccessSize];
            if (pExit->IoPortAccess.AccessInfo.IsWrite)
            {
                rcStrict = IOMIOPortWrite(pVM, pVCpu, pExit->IoPortAccess.PortNumber,
                                          (uint32_t)pExit->IoPortAccess.Rax & fAndMask,
                                          pExit->IoPortAccess.AccessInfo.AccessSize);
                Log4(("IOExit/%u: %04x:%08RX64/%s: OUT %#x, %#x LB %u rcStrict=%Rrc\n",
                      pVCpu->idCpu, pExit->VpContext.Cs.Selector, pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext),
                      pExit->IoPortAccess.PortNumber, (uint32_t)pExit->IoPortAccess.Rax & fAndMask,
                      pExit->IoPortAccess.AccessInfo.AccessSize, VBOXSTRICTRC_VAL(rcStrict) ));
                if (IOM_SUCCESS(rcStrict))
                {
                    nemR3WinCopyStateFromX64Header(pVCpu, &pExit->VpContext);
                    nemR3WinAdvanceGuestRipAndClearRF(pVCpu, &pExit->VpContext, 1);
                }
            }
            else
            {
                uint32_t uValue = 0;
                rcStrict = IOMIOPortRead(pVM, pVCpu, pExit->IoPortAccess.PortNumber, &uValue,
                                         pExit->IoPortAccess.AccessInfo.AccessSize);
                Log4(("IOExit/%u: %04x:%08RX64/%s: IN %#x LB %u -> %#x, rcStrict=%Rrc\n",
                      pVCpu->idCpu, pExit->VpContext.Cs.Selector, pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext),
                      pExit->IoPortAccess.PortNumber, pExit->IoPortAccess.AccessInfo.AccessSize, uValue, VBOXSTRICTRC_VAL(rcStrict) ));
                if (IOM_SUCCESS(rcStrict))
                {
                    if (pExit->IoPortAccess.AccessInfo.AccessSize != 4)
                        pVCpu->cpum.GstCtx.rax = (pExit->IoPortAccess.Rax & ~(uint64_t)fAndMask) | (uValue & fAndMask);
                    else
                        pVCpu->cpum.GstCtx.rax = uValue;
                    pVCpu->cpum.GstCtx.fExtrn &= ~CPUMCTX_EXTRN_RAX;
                    Log4(("IOExit/%u: RAX %#RX64 -> %#RX64\n", pVCpu->idCpu, pExit->IoPortAccess.Rax, pVCpu->cpum.GstCtx.rax));
                    nemR3WinCopyStateFromX64Header(pVCpu, &pExit->VpContext);
                    nemR3WinAdvanceGuestRipAndClearRF(pVCpu, &pExit->VpContext, 1);
                }
            }
        }
        else
        {
            /*
             * String port I/O.
             */
            /** @todo Someone at Microsoft please explain how we can get the address mode
             * from the IoPortAccess.VpContext.  CS.Attributes is only sufficient for
             * getting the default mode, it can always be overridden by a prefix.   This
             * forces us to interpret the instruction from opcodes, which is suboptimal.
             * Both AMD-V and VT-x includes the address size in the exit info, at least on
             * CPUs that are reasonably new.
             *
             * Of course, it's possible this is an undocumented and we just need to do some
             * experiments to figure out how it's communicated.  Alternatively, we can scan
             * the opcode bytes for possible evil prefixes.
             */
            nemR3WinCopyStateFromX64Header(pVCpu, &pExit->VpContext);
            pVCpu->cpum.GstCtx.fExtrn &= ~(  CPUMCTX_EXTRN_RAX | CPUMCTX_EXTRN_RCX | CPUMCTX_EXTRN_RDI | CPUMCTX_EXTRN_RSI
                                           | CPUMCTX_EXTRN_DS  | CPUMCTX_EXTRN_ES);
            NEM_WIN_COPY_BACK_SEG(pVCpu->cpum.GstCtx.ds, pExit->IoPortAccess.Ds);
            NEM_WIN_COPY_BACK_SEG(pVCpu->cpum.GstCtx.es, pExit->IoPortAccess.Es);
            pVCpu->cpum.GstCtx.rax = pExit->IoPortAccess.Rax;
            pVCpu->cpum.GstCtx.rcx = pExit->IoPortAccess.Rcx;
            pVCpu->cpum.GstCtx.rdi = pExit->IoPortAccess.Rdi;
            pVCpu->cpum.GstCtx.rsi = pExit->IoPortAccess.Rsi;
            int rc = nemHCWinCopyStateFromHyperV(pVM, pVCpu, NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM);
            AssertRCReturn(rc, rc);

            Log4(("IOExit/%u: %04x:%08RX64/%s: %s%s %#x LB %u (emulating)\n",
                  pVCpu->idCpu, pExit->VpContext.Cs.Selector, pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext),
                  pExit->IoPortAccess.AccessInfo.RepPrefix ? "REP " : "",
                  pExit->IoPortAccess.AccessInfo.IsWrite ? "OUTS" : "INS",
                  pExit->IoPortAccess.PortNumber, pExit->IoPortAccess.AccessInfo.AccessSize ));
            rcStrict = IEMExecOne(pVCpu);
        }
        if (IOM_SUCCESS(rcStrict))
        {
            /*
             * Do debug checks.
             */
            if (   pExit->VpContext.ExecutionState.DebugActive /** @todo Microsoft: Does DebugActive this only reflect DR7? */
                || (pExit->VpContext.Rflags & X86_EFL_TF)
                || DBGFBpIsHwIoArmed(pVM) )
            {
                /** @todo Debugging. */
            }
        }
        return rcStrict;
    }

    /*
     * Frequent exit or something needing probing.
     * Get state and call EMHistoryExec.
     */
    nemR3WinCopyStateFromX64Header(pVCpu, &pExit->VpContext);
    if (!pExit->IoPortAccess.AccessInfo.StringOp)
        pVCpu->cpum.GstCtx.fExtrn &= ~CPUMCTX_EXTRN_RAX;
    else
    {
        pVCpu->cpum.GstCtx.fExtrn &= ~(  CPUMCTX_EXTRN_RAX | CPUMCTX_EXTRN_RCX | CPUMCTX_EXTRN_RDI | CPUMCTX_EXTRN_RSI
                          | CPUMCTX_EXTRN_DS  | CPUMCTX_EXTRN_ES);
        NEM_WIN_COPY_BACK_SEG(pVCpu->cpum.GstCtx.ds, pExit->IoPortAccess.Ds);
        NEM_WIN_COPY_BACK_SEG(pVCpu->cpum.GstCtx.es, pExit->IoPortAccess.Es);
        pVCpu->cpum.GstCtx.rcx = pExit->IoPortAccess.Rcx;
        pVCpu->cpum.GstCtx.rdi = pExit->IoPortAccess.Rdi;
        pVCpu->cpum.GstCtx.rsi = pExit->IoPortAccess.Rsi;
    }
    pVCpu->cpum.GstCtx.rax = pExit->IoPortAccess.Rax;
    int rc = nemHCWinCopyStateFromHyperV(pVM, pVCpu, NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM);
    AssertRCReturn(rc, rc);
    Log4(("IOExit/%u: %04x:%08RX64/%s: %s%s%s %#x LB %u -> EMHistoryExec\n",
          pVCpu->idCpu, pExit->VpContext.Cs.Selector, pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext),
          pExit->IoPortAccess.AccessInfo.RepPrefix ? "REP " : "",
          pExit->IoPortAccess.AccessInfo.IsWrite ? "OUT" : "IN",
          pExit->IoPortAccess.AccessInfo.StringOp ? "S" : "",
          pExit->IoPortAccess.PortNumber, pExit->IoPortAccess.AccessInfo.AccessSize));
    VBOXSTRICTRC rcStrict = EMHistoryExec(pVCpu, pExitRec, 0);
    Log4(("IOExit/%u: %04x:%08RX64/%s: EMHistoryExec -> %Rrc + %04x:%08RX64\n",
          pVCpu->idCpu, pExit->VpContext.Cs.Selector, pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext),
          VBOXSTRICTRC_VAL(rcStrict), pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip));
    return rcStrict;
}
#endif /* IN_RING3 && !NEM_WIN_TEMPLATE_MODE_OWN_RUN_API */


#ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
/**
 * Deals with interrupt window message.
 *
 * @returns Strict VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context per CPU structure.
 * @param   pMsg            The message.
 * @sa      nemR3WinHandleExitInterruptWindow
 */
NEM_TMPL_STATIC VBOXSTRICTRC
nemHCWinHandleMessageInterruptWindow(PVMCC pVM, PVMCPUCC pVCpu, HV_X64_INTERRUPT_WINDOW_MESSAGE const *pMsg)
{
    /*
     * Assert message sanity.
     */
    Assert(   pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_EXECUTE
           || pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_READ   // READ & WRITE are probably not used here
           || pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_WRITE);
    AssertMsg(pMsg->Type == HvX64PendingInterrupt || pMsg->Type == HvX64PendingNmi, ("%#x\n", pMsg->Type));

    /*
     * Just copy the state we've got and handle it in the loop for now.
     */
    EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_INTTERRUPT_WINDOW),
                     pMsg->Header.Rip + pMsg->Header.CsSegment.Base, ASMReadTSC());

    nemHCWinCopyStateFromX64Header(pVCpu, &pMsg->Header);
    Log4(("IntWinExit/%u: %04x:%08RX64/%s: %u IF=%d InterruptShadow=%d\n",
          pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip,  nemHCWinExecStateToLogStr(&pMsg->Header),
          pMsg->Type, RT_BOOL(pMsg->Header.Rflags & X86_EFL_IF), pMsg->Header.ExecutionState.InterruptShadow));

    /** @todo call nemHCWinHandleInterruptFF   */
    RT_NOREF(pVM);
    return VINF_SUCCESS;
}
#elif defined(IN_RING3)
/**
 * Deals with interrupt window exits (WHvRunVpExitReasonX64InterruptWindow).
 *
 * @returns Strict VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context per CPU structure.
 * @param   pExit           The VM exit information to handle.
 * @sa      nemHCWinHandleMessageInterruptWindow
 */
NEM_TMPL_STATIC VBOXSTRICTRC nemR3WinHandleExitInterruptWindow(PVMCC pVM, PVMCPUCC pVCpu, WHV_RUN_VP_EXIT_CONTEXT const *pExit)
{
    /*
     * Assert message sanity.
     */
    AssertMsg(   pExit->InterruptWindow.DeliverableType == WHvX64PendingInterrupt
              || pExit->InterruptWindow.DeliverableType == WHvX64PendingNmi,
              ("%#x\n", pExit->InterruptWindow.DeliverableType));

    /*
     * Just copy the state we've got and handle it in the loop for now.
     */
    EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_INTTERRUPT_WINDOW),
                     pExit->VpContext.Rip + pExit->VpContext.Cs.Base, ASMReadTSC());

    nemR3WinCopyStateFromX64Header(pVCpu, &pExit->VpContext);
    Log4(("IntWinExit/%u: %04x:%08RX64/%s: %u IF=%d InterruptShadow=%d\n",
          pVCpu->idCpu, pExit->VpContext.Cs.Selector, pExit->VpContext.Rip,  nemR3WinExecStateToLogStr(&pExit->VpContext),
          pExit->InterruptWindow.DeliverableType, RT_BOOL(pExit->VpContext.Rflags & X86_EFL_IF),
          pExit->VpContext.ExecutionState.InterruptShadow));

    /** @todo call nemHCWinHandleInterruptFF   */
    RT_NOREF(pVM);
    return VINF_SUCCESS;
}
#endif /* IN_RING3 && !NEM_WIN_TEMPLATE_MODE_OWN_RUN_API */


#ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
/**
 * Deals with CPUID intercept message.
 *
 * @returns Strict VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context per CPU structure.
 * @param   pMsg            The message.
 * @sa      nemR3WinHandleExitCpuId
 */
NEM_TMPL_STATIC VBOXSTRICTRC nemHCWinHandleMessageCpuId(PVMCC pVM, PVMCPUCC pVCpu, HV_X64_CPUID_INTERCEPT_MESSAGE const *pMsg)
{
    /* Check message register value sanity. */
    NEMWIN_ASSERT_MSG_REG_SEG(  pVCpu, HvX64RegisterCs, pMsg->Header.CsSegment);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRip, pMsg->Header.Rip);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRflags, pMsg->Header.Rflags);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterCr8, (uint64_t)pMsg->Header.Cr8);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRax, pMsg->Rax);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRcx, pMsg->Rcx);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRdx, pMsg->Rdx);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRbx, pMsg->Rbx);

    /* Do exit history. */
    PCEMEXITREC pExitRec = EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_CPUID),
                                            pMsg->Header.Rip + pMsg->Header.CsSegment.Base, ASMReadTSC());
    if (!pExitRec)
    {
        /*
         * Soak up state and execute the instruction.
         *
         * Note! If this grows slightly more complicated, combine into an IEMExecDecodedCpuId
         *       function and make everyone use it.
         */
        /** @todo Combine implementations into IEMExecDecodedCpuId as this will
         *        only get weirder with nested VT-x and AMD-V support. */
        nemHCWinCopyStateFromX64Header(pVCpu, &pMsg->Header);

        /* Copy in the low register values (top is always cleared). */
        pVCpu->cpum.GstCtx.rax = (uint32_t)pMsg->Rax;
        pVCpu->cpum.GstCtx.rcx = (uint32_t)pMsg->Rcx;
        pVCpu->cpum.GstCtx.rdx = (uint32_t)pMsg->Rdx;
        pVCpu->cpum.GstCtx.rbx = (uint32_t)pMsg->Rbx;
        pVCpu->cpum.GstCtx.fExtrn &= ~(CPUMCTX_EXTRN_RAX | CPUMCTX_EXTRN_RCX | CPUMCTX_EXTRN_RDX | CPUMCTX_EXTRN_RBX);

        /* Get the correct values. */
        CPUMGetGuestCpuId(pVCpu, pVCpu->cpum.GstCtx.eax, pVCpu->cpum.GstCtx.ecx,
                          &pVCpu->cpum.GstCtx.eax, &pVCpu->cpum.GstCtx.ebx, &pVCpu->cpum.GstCtx.ecx, &pVCpu->cpum.GstCtx.edx);

        Log4(("CpuIdExit/%u: %04x:%08RX64/%s: rax=%08RX64 / rcx=%08RX64 / rdx=%08RX64 / rbx=%08RX64 -> %08RX32 / %08RX32 / %08RX32 / %08RX32 (hv: %08RX64 / %08RX64 / %08RX64 / %08RX64)\n",
              pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
              pMsg->Rax,                           pMsg->Rcx,              pMsg->Rdx,              pMsg->Rbx,
              pVCpu->cpum.GstCtx.eax,              pVCpu->cpum.GstCtx.ecx, pVCpu->cpum.GstCtx.edx, pVCpu->cpum.GstCtx.ebx,
              pMsg->DefaultResultRax, pMsg->DefaultResultRcx, pMsg->DefaultResultRdx, pMsg->DefaultResultRbx));

        /* Move RIP and we're done. */
        nemHCWinAdvanceGuestRipAndClearRF(pVCpu, &pMsg->Header, 2);

        return VINF_SUCCESS;
    }

    /*
     * Frequent exit or something needing probing.
     * Get state and call EMHistoryExec.
     */
    nemHCWinCopyStateFromX64Header(pVCpu, &pMsg->Header);
    pVCpu->cpum.GstCtx.rax = pMsg->Rax;
    pVCpu->cpum.GstCtx.rcx = pMsg->Rcx;
    pVCpu->cpum.GstCtx.rdx = pMsg->Rdx;
    pVCpu->cpum.GstCtx.rbx = pMsg->Rbx;
    pVCpu->cpum.GstCtx.fExtrn &= ~(CPUMCTX_EXTRN_RAX | CPUMCTX_EXTRN_RCX | CPUMCTX_EXTRN_RDX | CPUMCTX_EXTRN_RBX);
    Log4(("CpuIdExit/%u: %04x:%08RX64/%s: rax=%08RX64 / rcx=%08RX64 / rdx=%08RX64 / rbx=%08RX64 (hv: %08RX64 / %08RX64 / %08RX64 / %08RX64) ==> EMHistoryExec\n",
          pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
          pMsg->Rax,                           pMsg->Rcx,              pMsg->Rdx,              pMsg->Rbx,
          pMsg->DefaultResultRax, pMsg->DefaultResultRcx, pMsg->DefaultResultRdx, pMsg->DefaultResultRbx));
# ifdef IN_RING0
    VBOXSTRICTRC rcStrict = nemR0WinImportStateStrict(pVM, pVCpu, NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM, "CpuIdExit");
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;
    RT_NOREF(pVM);
# else
    int rc = nemHCWinCopyStateFromHyperV(pVM, pVCpu, NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM);
    AssertRCReturn(rc, rc);
# endif
    VBOXSTRICTRC rcStrictExec = EMHistoryExec(pVCpu, pExitRec, 0);
    Log4(("CpuIdExit/%u: %04x:%08RX64/%s: EMHistoryExec -> %Rrc + %04x:%08RX64\n",
          pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
          VBOXSTRICTRC_VAL(rcStrictExec), pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip));
    return rcStrictExec;
}
#elif defined(IN_RING3)
/**
 * Deals with CPUID exits (WHvRunVpExitReasonX64Cpuid).
 *
 * @returns Strict VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context per CPU structure.
 * @param   pExit           The VM exit information to handle.
 * @sa      nemHCWinHandleMessageCpuId
 */
NEM_TMPL_STATIC VBOXSTRICTRC
nemR3WinHandleExitCpuId(PVMCC pVM, PVMCPUCC pVCpu, WHV_RUN_VP_EXIT_CONTEXT const *pExit)
{
    PCEMEXITREC pExitRec = EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_CPUID),
                                            pExit->VpContext.Rip + pExit->VpContext.Cs.Base, ASMReadTSC());
    if (!pExitRec)
    {
        /*
         * Soak up state and execute the instruction.
         *
         * Note! If this grows slightly more complicated, combine into an IEMExecDecodedCpuId
         *       function and make everyone use it.
         */
        /** @todo Combine implementations into IEMExecDecodedCpuId as this will
         *        only get weirder with nested VT-x and AMD-V support. */
        nemR3WinCopyStateFromX64Header(pVCpu, &pExit->VpContext);

        /* Copy in the low register values (top is always cleared). */
        pVCpu->cpum.GstCtx.rax = (uint32_t)pExit->CpuidAccess.Rax;
        pVCpu->cpum.GstCtx.rcx = (uint32_t)pExit->CpuidAccess.Rcx;
        pVCpu->cpum.GstCtx.rdx = (uint32_t)pExit->CpuidAccess.Rdx;
        pVCpu->cpum.GstCtx.rbx = (uint32_t)pExit->CpuidAccess.Rbx;
        pVCpu->cpum.GstCtx.fExtrn &= ~(CPUMCTX_EXTRN_RAX | CPUMCTX_EXTRN_RCX | CPUMCTX_EXTRN_RDX | CPUMCTX_EXTRN_RBX);

        /* Get the correct values. */
        CPUMGetGuestCpuId(pVCpu, pVCpu->cpum.GstCtx.eax, pVCpu->cpum.GstCtx.ecx,
                          &pVCpu->cpum.GstCtx.eax, &pVCpu->cpum.GstCtx.ebx, &pVCpu->cpum.GstCtx.ecx, &pVCpu->cpum.GstCtx.edx);

        Log4(("CpuIdExit/%u: %04x:%08RX64/%s: rax=%08RX64 / rcx=%08RX64 / rdx=%08RX64 / rbx=%08RX64 -> %08RX32 / %08RX32 / %08RX32 / %08RX32 (hv: %08RX64 / %08RX64 / %08RX64 / %08RX64)\n",
              pVCpu->idCpu, pExit->VpContext.Cs.Selector, pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext),
              pExit->CpuidAccess.Rax,                           pExit->CpuidAccess.Rcx,              pExit->CpuidAccess.Rdx,              pExit->CpuidAccess.Rbx,
              pVCpu->cpum.GstCtx.eax,                           pVCpu->cpum.GstCtx.ecx,              pVCpu->cpum.GstCtx.edx,              pVCpu->cpum.GstCtx.ebx,
              pExit->CpuidAccess.DefaultResultRax, pExit->CpuidAccess.DefaultResultRcx, pExit->CpuidAccess.DefaultResultRdx, pExit->CpuidAccess.DefaultResultRbx));

        /* Move RIP and we're done. */
        nemR3WinAdvanceGuestRipAndClearRF(pVCpu, &pExit->VpContext, 2);

        RT_NOREF_PV(pVM);
        return VINF_SUCCESS;
    }

    /*
     * Frequent exit or something needing probing.
     * Get state and call EMHistoryExec.
     */
    nemR3WinCopyStateFromX64Header(pVCpu, &pExit->VpContext);
    pVCpu->cpum.GstCtx.rax = pExit->CpuidAccess.Rax;
    pVCpu->cpum.GstCtx.rcx = pExit->CpuidAccess.Rcx;
    pVCpu->cpum.GstCtx.rdx = pExit->CpuidAccess.Rdx;
    pVCpu->cpum.GstCtx.rbx = pExit->CpuidAccess.Rbx;
    pVCpu->cpum.GstCtx.fExtrn &= ~(CPUMCTX_EXTRN_RAX | CPUMCTX_EXTRN_RCX | CPUMCTX_EXTRN_RDX | CPUMCTX_EXTRN_RBX);
    Log4(("CpuIdExit/%u: %04x:%08RX64/%s: rax=%08RX64 / rcx=%08RX64 / rdx=%08RX64 / rbx=%08RX64 (hv: %08RX64 / %08RX64 / %08RX64 / %08RX64) ==> EMHistoryExec\n",
          pVCpu->idCpu, pExit->VpContext.Cs.Selector, pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext),
          pExit->CpuidAccess.Rax,                           pExit->CpuidAccess.Rcx,              pExit->CpuidAccess.Rdx,              pExit->CpuidAccess.Rbx,
          pExit->CpuidAccess.DefaultResultRax, pExit->CpuidAccess.DefaultResultRcx, pExit->CpuidAccess.DefaultResultRdx, pExit->CpuidAccess.DefaultResultRbx));
    int rc = nemHCWinCopyStateFromHyperV(pVM, pVCpu, NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM);
    AssertRCReturn(rc, rc);
    VBOXSTRICTRC rcStrict = EMHistoryExec(pVCpu, pExitRec, 0);
    Log4(("CpuIdExit/%u: %04x:%08RX64/%s: EMHistoryExec -> %Rrc + %04x:%08RX64\n",
          pVCpu->idCpu, pExit->VpContext.Cs.Selector, pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext),
          VBOXSTRICTRC_VAL(rcStrict), pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip));
    return rcStrict;
}
#endif /* IN_RING3 && !NEM_WIN_TEMPLATE_MODE_OWN_RUN_API */


#ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
/**
 * Deals with MSR intercept message.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu           The cross context per CPU structure.
 * @param   pMsg            The message.
 * @sa      nemR3WinHandleExitMsr
 */
NEM_TMPL_STATIC VBOXSTRICTRC nemHCWinHandleMessageMsr(PVMCPUCC pVCpu, HV_X64_MSR_INTERCEPT_MESSAGE const *pMsg)
{
    /*
     * A wee bit of sanity first.
     */
    Assert(   pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_READ
           || pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_WRITE);
    NEMWIN_ASSERT_MSG_REG_SEG(  pVCpu, HvX64RegisterCs, pMsg->Header.CsSegment);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRip, pMsg->Header.Rip);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRflags, pMsg->Header.Rflags);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterCr8, (uint64_t)pMsg->Header.Cr8);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRax, pMsg->Rax);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRdx, pMsg->Rdx);

    /*
     * Check CPL as that's common to both RDMSR and WRMSR.
     */
    VBOXSTRICTRC rcStrict;
    if (pMsg->Header.ExecutionState.Cpl == 0)
    {
        /*
         * Get all the MSR state.  Since we're getting EFER, we also need to
         * get CR0, CR4 and CR3.
         */
        PCEMEXITREC pExitRec = EMHistoryAddExit(pVCpu,
                                                  pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_WRITE
                                                ? EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_MSR_WRITE)
                                                : EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_MSR_READ),
                                                pMsg->Header.Rip + pMsg->Header.CsSegment.Base, ASMReadTSC());

        nemHCWinCopyStateFromX64Header(pVCpu, &pMsg->Header);
        rcStrict = nemHCWinImportStateIfNeededStrict(pVCpu,
                                                     (!pExitRec ? 0 : IEM_CPUMCTX_EXTRN_MUST_MASK)
                                                     | CPUMCTX_EXTRN_ALL_MSRS | CPUMCTX_EXTRN_CR0
                                                     | CPUMCTX_EXTRN_CR3 | CPUMCTX_EXTRN_CR4,
                                                     "MSRs");
        if (rcStrict == VINF_SUCCESS)
        {
            if (!pExitRec)
            {
                /*
                 * Handle writes.
                 */
                if (pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_WRITE)
                {
                    rcStrict = CPUMSetGuestMsr(pVCpu, pMsg->MsrNumber, RT_MAKE_U64((uint32_t)pMsg->Rax, (uint32_t)pMsg->Rdx));
                    Log4(("MsrExit/%u: %04x:%08RX64/%s: WRMSR %08x, %08x:%08x -> %Rrc\n",
                          pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
                          pMsg->MsrNumber, (uint32_t)pMsg->Rax, (uint32_t)pMsg->Rdx, VBOXSTRICTRC_VAL(rcStrict) ));
                    if (rcStrict == VINF_SUCCESS)
                    {
                        nemHCWinAdvanceGuestRipAndClearRF(pVCpu, &pMsg->Header, 2);
                        return VINF_SUCCESS;
                    }
# ifndef IN_RING3
                    /* move to ring-3 and handle the trap/whatever there, as we want to LogRel this. */
                    if (rcStrict == VERR_CPUM_RAISE_GP_0)
                        rcStrict = VINF_CPUM_R3_MSR_WRITE;
                    return rcStrict;
# else
                    LogRel(("MsrExit/%u: %04x:%08RX64/%s: WRMSR %08x, %08x:%08x -> %Rrc!\n",
                            pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
                            pMsg->MsrNumber, (uint32_t)pMsg->Rax, (uint32_t)pMsg->Rdx, VBOXSTRICTRC_VAL(rcStrict) ));
# endif
                }
                /*
                 * Handle reads.
                 */
                else
                {
                    uint64_t uValue = 0;
                    rcStrict = CPUMQueryGuestMsr(pVCpu, pMsg->MsrNumber, &uValue);
                    Log4(("MsrExit/%u: %04x:%08RX64/%s: RDMSR %08x -> %08RX64 / %Rrc\n",
                          pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
                          pMsg->MsrNumber, uValue, VBOXSTRICTRC_VAL(rcStrict) ));
                    if (rcStrict == VINF_SUCCESS)
                    {
                        pVCpu->cpum.GstCtx.rax = (uint32_t)uValue;
                        pVCpu->cpum.GstCtx.rdx = uValue >> 32;
                        pVCpu->cpum.GstCtx.fExtrn &= ~(CPUMCTX_EXTRN_RAX | CPUMCTX_EXTRN_RDX);
                        nemHCWinAdvanceGuestRipAndClearRF(pVCpu, &pMsg->Header, 2);
                        return VINF_SUCCESS;
                    }
# ifndef IN_RING3
                    /* move to ring-3 and handle the trap/whatever there, as we want to LogRel this. */
                    if (rcStrict == VERR_CPUM_RAISE_GP_0)
                        rcStrict = VINF_CPUM_R3_MSR_READ;
                    return rcStrict;
# else
                    LogRel(("MsrExit/%u: %04x:%08RX64/%s: RDMSR %08x -> %08RX64 / %Rrc\n",
                            pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
                            pMsg->MsrNumber, uValue, VBOXSTRICTRC_VAL(rcStrict) ));
# endif
                }
            }
            else
            {
                /*
                 * Handle frequent exit or something needing probing.
                 */
                Log4(("MsrExit/%u: %04x:%08RX64/%s: %sMSR %#08x\n",
                      pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
                      pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_WRITE ? "WR" : "RD", pMsg->MsrNumber));
                rcStrict = EMHistoryExec(pVCpu, pExitRec, 0);
                Log4(("MsrExit/%u: %04x:%08RX64/%s: EMHistoryExec -> %Rrc + %04x:%08RX64\n",
                      pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
                      VBOXSTRICTRC_VAL(rcStrict), pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip));
                return rcStrict;
            }
        }
        else
        {
            LogRel(("MsrExit/%u: %04x:%08RX64/%s: %sMSR %08x -> %Rrc - msr state import\n",
                    pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
                    pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_WRITE ? "WR" : "RD",
                    pMsg->MsrNumber, VBOXSTRICTRC_VAL(rcStrict) ));
            return rcStrict;
        }
    }
    else if (pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_WRITE)
        Log4(("MsrExit/%u: %04x:%08RX64/%s: CPL %u -> #GP(0); WRMSR %08x, %08x:%08x\n",
              pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
              pMsg->Header.ExecutionState.Cpl, pMsg->MsrNumber, (uint32_t)pMsg->Rax, (uint32_t)pMsg->Rdx ));
    else
        Log4(("MsrExit/%u: %04x:%08RX64/%s: CPL %u -> #GP(0); RDMSR %08x\n",
              pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
              pMsg->Header.ExecutionState.Cpl, pMsg->MsrNumber));

    /*
     * If we get down here, we're supposed to #GP(0).
     */
    rcStrict = nemHCWinImportStateIfNeededStrict(pVCpu, NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM | CPUMCTX_EXTRN_ALL_MSRS, "MSR");
    if (rcStrict == VINF_SUCCESS)
    {
        rcStrict = IEMInjectTrap(pVCpu, X86_XCPT_GP, TRPM_TRAP, 0, 0, 0);
        if (rcStrict == VINF_IEM_RAISED_XCPT)
            rcStrict = VINF_SUCCESS;
        else if (rcStrict != VINF_SUCCESS)
            Log4(("MsrExit/%u: Injecting #GP(0) failed: %Rrc\n", VBOXSTRICTRC_VAL(rcStrict) ));
    }
    return rcStrict;
}
#elif defined(IN_RING3)
/**
 * Deals with MSR access exits (WHvRunVpExitReasonX64MsrAccess).
 *
 * @returns Strict VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context per CPU structure.
 * @param   pExit           The VM exit information to handle.
 * @sa      nemHCWinHandleMessageMsr
 */
NEM_TMPL_STATIC VBOXSTRICTRC nemR3WinHandleExitMsr(PVMCC pVM, PVMCPUCC pVCpu, WHV_RUN_VP_EXIT_CONTEXT const *pExit)
{
    /*
     * Check CPL as that's common to both RDMSR and WRMSR.
     */
    VBOXSTRICTRC rcStrict;
    if (pExit->VpContext.ExecutionState.Cpl == 0)
    {
        /*
         * Get all the MSR state.  Since we're getting EFER, we also need to
         * get CR0, CR4 and CR3.
         */
        PCEMEXITREC pExitRec = EMHistoryAddExit(pVCpu,
                                                  pExit->MsrAccess.AccessInfo.IsWrite
                                                ? EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_MSR_WRITE)
                                                : EMEXIT_MAKE_FT(EMEXIT_F_KIND_EM, EMEXITTYPE_MSR_READ),
                                                pExit->VpContext.Rip + pExit->VpContext.Cs.Base, ASMReadTSC());
        nemR3WinCopyStateFromX64Header(pVCpu, &pExit->VpContext);
        rcStrict = nemHCWinImportStateIfNeededStrict(pVCpu,
                                                     (!pExitRec ? 0 : IEM_CPUMCTX_EXTRN_MUST_MASK)
                                                     | CPUMCTX_EXTRN_ALL_MSRS | CPUMCTX_EXTRN_CR0
                                                     | CPUMCTX_EXTRN_CR3 | CPUMCTX_EXTRN_CR4,
                                                     "MSRs");
        if (rcStrict == VINF_SUCCESS)
        {
            if (!pExitRec)
            {
                /*
                 * Handle writes.
                 */
                if (pExit->MsrAccess.AccessInfo.IsWrite)
                {
                    rcStrict = CPUMSetGuestMsr(pVCpu, pExit->MsrAccess.MsrNumber,
                                               RT_MAKE_U64((uint32_t)pExit->MsrAccess.Rax, (uint32_t)pExit->MsrAccess.Rdx));
                    Log4(("MsrExit/%u: %04x:%08RX64/%s: WRMSR %08x, %08x:%08x -> %Rrc\n", pVCpu->idCpu, pExit->VpContext.Cs.Selector,
                          pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext), pExit->MsrAccess.MsrNumber,
                          (uint32_t)pExit->MsrAccess.Rax, (uint32_t)pExit->MsrAccess.Rdx, VBOXSTRICTRC_VAL(rcStrict) ));
                    if (rcStrict == VINF_SUCCESS)
                    {
                        nemR3WinAdvanceGuestRipAndClearRF(pVCpu, &pExit->VpContext, 2);
                        return VINF_SUCCESS;
                    }
                    LogRel(("MsrExit/%u: %04x:%08RX64/%s: WRMSR %08x, %08x:%08x -> %Rrc!\n", pVCpu->idCpu,
                            pExit->VpContext.Cs.Selector, pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext),
                            pExit->MsrAccess.MsrNumber, (uint32_t)pExit->MsrAccess.Rax, (uint32_t)pExit->MsrAccess.Rdx,
                            VBOXSTRICTRC_VAL(rcStrict) ));
                }
                /*
                 * Handle reads.
                 */
                else
                {
                    uint64_t uValue = 0;
                    rcStrict = CPUMQueryGuestMsr(pVCpu, pExit->MsrAccess.MsrNumber, &uValue);
                    Log4(("MsrExit/%u: %04x:%08RX64/%s: RDMSR %08x -> %08RX64 / %Rrc\n", pVCpu->idCpu,
                          pExit->VpContext.Cs.Selector, pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext),
                          pExit->MsrAccess.MsrNumber, uValue, VBOXSTRICTRC_VAL(rcStrict) ));
                    if (rcStrict == VINF_SUCCESS)
                    {
                        pVCpu->cpum.GstCtx.rax = (uint32_t)uValue;
                        pVCpu->cpum.GstCtx.rdx = uValue >> 32;
                        pVCpu->cpum.GstCtx.fExtrn &= ~(CPUMCTX_EXTRN_RAX | CPUMCTX_EXTRN_RDX);
                        nemR3WinAdvanceGuestRipAndClearRF(pVCpu, &pExit->VpContext, 2);
                        return VINF_SUCCESS;
                    }
                    LogRel(("MsrExit/%u: %04x:%08RX64/%s: RDMSR %08x -> %08RX64 / %Rrc\n", pVCpu->idCpu, pExit->VpContext.Cs.Selector,
                            pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext), pExit->MsrAccess.MsrNumber,
                            uValue, VBOXSTRICTRC_VAL(rcStrict) ));
                }
            }
            else
            {
                /*
                 * Handle frequent exit or something needing probing.
                 */
                Log4(("MsrExit/%u: %04x:%08RX64/%s: %sMSR %#08x\n",
                      pVCpu->idCpu, pExit->VpContext.Cs.Selector, pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext),
                      pExit->MsrAccess.AccessInfo.IsWrite ? "WR" : "RD", pExit->MsrAccess.MsrNumber));
                rcStrict = EMHistoryExec(pVCpu, pExitRec, 0);
                Log4(("MsrExit/%u: %04x:%08RX64/%s: EMHistoryExec -> %Rrc + %04x:%08RX64\n",
                      pVCpu->idCpu, pExit->VpContext.Cs.Selector, pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext),
                      VBOXSTRICTRC_VAL(rcStrict), pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip));
                return rcStrict;
            }
        }
        else
        {
            LogRel(("MsrExit/%u: %04x:%08RX64/%s: %sMSR %08x -> %Rrc - msr state import\n",
                    pVCpu->idCpu, pExit->VpContext.Cs.Selector, pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext),
                    pExit->MsrAccess.AccessInfo.IsWrite ? "WR" : "RD", pExit->MsrAccess.MsrNumber, VBOXSTRICTRC_VAL(rcStrict) ));
            return rcStrict;
        }
    }
    else if (pExit->MsrAccess.AccessInfo.IsWrite)
        Log4(("MsrExit/%u: %04x:%08RX64/%s: CPL %u -> #GP(0); WRMSR %08x, %08x:%08x\n", pVCpu->idCpu, pExit->VpContext.Cs.Selector,
              pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext), pExit->VpContext.ExecutionState.Cpl,
              pExit->MsrAccess.MsrNumber, (uint32_t)pExit->MsrAccess.Rax, (uint32_t)pExit->MsrAccess.Rdx ));
    else
        Log4(("MsrExit/%u: %04x:%08RX64/%s: CPL %u -> #GP(0); RDMSR %08x\n", pVCpu->idCpu, pExit->VpContext.Cs.Selector,
              pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext), pExit->VpContext.ExecutionState.Cpl,
              pExit->MsrAccess.MsrNumber));

    /*
     * If we get down here, we're supposed to #GP(0).
     */
    rcStrict = nemHCWinImportStateIfNeededStrict(pVCpu, NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM | CPUMCTX_EXTRN_ALL_MSRS, "MSR");
    if (rcStrict == VINF_SUCCESS)
    {
        rcStrict = IEMInjectTrap(pVCpu, X86_XCPT_GP, TRPM_TRAP, 0, 0, 0);
        if (rcStrict == VINF_IEM_RAISED_XCPT)
            rcStrict = VINF_SUCCESS;
        else if (rcStrict != VINF_SUCCESS)
            Log4(("MsrExit/%u: Injecting #GP(0) failed: %Rrc\n", VBOXSTRICTRC_VAL(rcStrict) ));
    }

    RT_NOREF_PV(pVM);
    return rcStrict;
}
#endif /* IN_RING3 && !NEM_WIN_TEMPLATE_MODE_OWN_RUN_API */


/**
 * Worker for nemHCWinHandleMessageException & nemR3WinHandleExitException that
 * checks if the given opcodes are of interest at all.
 *
 * @returns true if interesting, false if not.
 * @param   cbOpcodes           Number of opcode bytes available.
 * @param   pbOpcodes           The opcode bytes.
 * @param   f64BitMode          Whether we're in 64-bit mode.
 */
DECLINLINE(bool) nemHcWinIsInterestingUndefinedOpcode(uint8_t cbOpcodes, uint8_t const *pbOpcodes, bool f64BitMode)
{
    /*
     * Currently only interested in VMCALL and VMMCALL.
     */
    while (cbOpcodes >= 3)
    {
        switch (pbOpcodes[0])
        {
            case 0x0f:
                switch (pbOpcodes[1])
                {
                    case 0x01:
                        switch (pbOpcodes[2])
                        {
                            case 0xc1: /* 0f 01 c1  VMCALL */
                                return true;
                            case 0xd9: /* 0f 01 d9  VMMCALL */
                                return true;
                            default:
                                break;
                        }
                        break;
                }
                break;

            default:
                return false;

            /* prefixes */
            case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
            case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
                if (!f64BitMode)
                    return false;
                RT_FALL_THRU();
            case X86_OP_PRF_CS:
            case X86_OP_PRF_SS:
            case X86_OP_PRF_DS:
            case X86_OP_PRF_ES:
            case X86_OP_PRF_FS:
            case X86_OP_PRF_GS:
            case X86_OP_PRF_SIZE_OP:
            case X86_OP_PRF_SIZE_ADDR:
            case X86_OP_PRF_LOCK:
            case X86_OP_PRF_REPZ:
            case X86_OP_PRF_REPNZ:
                cbOpcodes--;
                pbOpcodes++;
                continue;
        }
        break;
    }
    return false;
}


#ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
/**
 * Copies state included in a exception intercept message.
 *
 * @param   pVCpu           The cross context per CPU structure.
 * @param   pMsg            The message.
 * @param   fClearXcpt      Clear pending exception.
 */
DECLINLINE(void)
nemHCWinCopyStateFromExceptionMessage(PVMCPUCC pVCpu, HV_X64_EXCEPTION_INTERCEPT_MESSAGE const *pMsg, bool fClearXcpt)
{
    nemHCWinCopyStateFromX64Header(pVCpu, &pMsg->Header);
    pVCpu->cpum.GstCtx.fExtrn &= ~(  CPUMCTX_EXTRN_GPRS_MASK | CPUMCTX_EXTRN_SS | CPUMCTX_EXTRN_DS
                      | (fClearXcpt ? CPUMCTX_EXTRN_NEM_WIN_EVENT_INJECT : 0) );
    pVCpu->cpum.GstCtx.rax = pMsg->Rax;
    pVCpu->cpum.GstCtx.rcx = pMsg->Rcx;
    pVCpu->cpum.GstCtx.rdx = pMsg->Rdx;
    pVCpu->cpum.GstCtx.rbx = pMsg->Rbx;
    pVCpu->cpum.GstCtx.rsp = pMsg->Rsp;
    pVCpu->cpum.GstCtx.rbp = pMsg->Rbp;
    pVCpu->cpum.GstCtx.rsi = pMsg->Rsi;
    pVCpu->cpum.GstCtx.rdi = pMsg->Rdi;
    pVCpu->cpum.GstCtx.r8  = pMsg->R8;
    pVCpu->cpum.GstCtx.r9  = pMsg->R9;
    pVCpu->cpum.GstCtx.r10 = pMsg->R10;
    pVCpu->cpum.GstCtx.r11 = pMsg->R11;
    pVCpu->cpum.GstCtx.r12 = pMsg->R12;
    pVCpu->cpum.GstCtx.r13 = pMsg->R13;
    pVCpu->cpum.GstCtx.r14 = pMsg->R14;
    pVCpu->cpum.GstCtx.r15 = pMsg->R15;
    NEM_WIN_COPY_BACK_SEG(pVCpu->cpum.GstCtx.ds, pMsg->DsSegment);
    NEM_WIN_COPY_BACK_SEG(pVCpu->cpum.GstCtx.ss, pMsg->SsSegment);
}
#elif defined(IN_RING3)
/**
 * Copies state included in a exception intercept exit.
 *
 * @param   pVCpu           The cross context per CPU structure.
 * @param   pExit           The VM exit information.
 * @param   fClearXcpt      Clear pending exception.
 */
DECLINLINE(void) nemR3WinCopyStateFromExceptionMessage(PVMCPUCC pVCpu, WHV_RUN_VP_EXIT_CONTEXT const *pExit, bool fClearXcpt)
{
    nemR3WinCopyStateFromX64Header(pVCpu, &pExit->VpContext);
    if (fClearXcpt)
        pVCpu->cpum.GstCtx.fExtrn &= ~CPUMCTX_EXTRN_NEM_WIN_EVENT_INJECT;
}
#endif /* IN_RING3 && !NEM_WIN_TEMPLATE_MODE_OWN_RUN_API */


/**
 * Advances the guest RIP by the number of bytes specified in @a cb.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   cb          RIP increment value in bytes.
 */
DECLINLINE(void) nemHcWinAdvanceRip(PVMCPUCC pVCpu, uint32_t cb)
{
    PCPUMCTX pCtx = &pVCpu->cpum.GstCtx;
    pCtx->rip += cb;

    /* Update interrupt shadow. */
    if (   VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS)
        && pCtx->rip != EMGetInhibitInterruptsPC(pVCpu))
        VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS);
}


/**
 * Hacks its way around the lovely mesa driver's backdoor accesses.
 *
 * @sa hmR0VmxHandleMesaDrvGp
 * @sa hmR0SvmHandleMesaDrvGp
 */
static int nemHcWinHandleMesaDrvGp(PVMCPUCC pVCpu, PCPUMCTX pCtx)
{
    Assert(!(pCtx->fExtrn & (CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_CS | CPUMCTX_EXTRN_RFLAGS | CPUMCTX_EXTRN_GPRS_MASK)));
    RT_NOREF(pCtx);

    /* For now we'll just skip the instruction. */
    nemHcWinAdvanceRip(pVCpu, 1);
    return VINF_SUCCESS;
}


/**
 * Checks if the \#GP'ing instruction is the mesa driver doing it's lovely
 * backdoor logging w/o checking what it is running inside.
 *
 * This recognizes an "IN EAX,DX" instruction executed in flat ring-3, with the
 * backdoor port and magic numbers loaded in registers.
 *
 * @returns true if it is, false if it isn't.
 * @sa      hmR0VmxIsMesaDrvGp
 * @sa      hmR0SvmIsMesaDrvGp
 */
DECLINLINE(bool) nemHcWinIsMesaDrvGp(PVMCPUCC pVCpu, PCPUMCTX pCtx, const uint8_t *pbInsn, uint32_t cbInsn)
{
    /* #GP(0) is already checked by caller. */

    /* Check magic and port. */
    Assert(!(pCtx->fExtrn & (CPUMCTX_EXTRN_RDX | CPUMCTX_EXTRN_RAX)));
    if (pCtx->dx != UINT32_C(0x5658))
        return false;
    if (pCtx->rax != UINT32_C(0x564d5868))
        return false;

    /* Flat ring-3 CS. */
    if (CPUMGetGuestCPL(pVCpu) != 3)
        return false;
    if (pCtx->cs.u64Base != 0)
        return false;

    /* 0xed:  IN eAX,dx */
    if (cbInsn < 1) /* Play safe (shouldn't happen). */
    {
        uint8_t abInstr[1];
        int rc = PGMPhysSimpleReadGCPtr(pVCpu, abInstr, pCtx->rip, sizeof(abInstr));
        if (RT_FAILURE(rc))
            return false;
        if (abInstr[0] != 0xed)
            return false;
    }
    else
    {
        if (pbInsn[0] != 0xed)
            return false;
    }

    return true;
}


#ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
/**
 * Deals with exception intercept message (HvMessageTypeX64ExceptionIntercept).
 *
 * @returns Strict VBox status code.
 * @param   pVCpu           The cross context per CPU structure.
 * @param   pMsg            The message.
 * @sa      nemR3WinHandleExitMsr
 */
NEM_TMPL_STATIC VBOXSTRICTRC
nemHCWinHandleMessageException(PVMCPUCC pVCpu, HV_X64_EXCEPTION_INTERCEPT_MESSAGE const *pMsg)
{
    /*
     * Assert sanity.
     */
    Assert(   pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_READ
           || pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_WRITE
           || pMsg->Header.InterceptAccessType == HV_INTERCEPT_ACCESS_EXECUTE);
    NEMWIN_ASSERT_MSG_REG_SEG(  pVCpu, HvX64RegisterCs, pMsg->Header.CsSegment);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRip, pMsg->Header.Rip);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRflags, pMsg->Header.Rflags);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterCr8, (uint64_t)pMsg->Header.Cr8);
    NEMWIN_ASSERT_MSG_REG_SEG(  pVCpu, HvX64RegisterDs,  pMsg->DsSegment);
    NEMWIN_ASSERT_MSG_REG_SEG(  pVCpu, HvX64RegisterSs,  pMsg->SsSegment);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRax, pMsg->Rax);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRcx, pMsg->Rcx);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRdx, pMsg->Rdx);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRbx, pMsg->Rbx);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRsp, pMsg->Rsp);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRbp, pMsg->Rbp);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRsi, pMsg->Rsi);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRdi, pMsg->Rdi);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterR8,  pMsg->R8);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterR9,  pMsg->R9);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterR10, pMsg->R10);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterR11, pMsg->R11);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterR12, pMsg->R12);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterR13, pMsg->R13);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterR14, pMsg->R14);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterR15, pMsg->R15);

    /*
     * Get most of the register state since we'll end up making IEM inject the
     * event.  The exception isn't normally flaged as a pending event, so duh.
     *
     * Note! We can optimize this later with event injection.
     */
    Log4(("XcptExit/%u: %04x:%08RX64/%s: %x errcd=%#x parm=%RX64\n",
          pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),
          pMsg->ExceptionVector, pMsg->ErrorCode, pMsg->ExceptionParameter));
    nemHCWinCopyStateFromExceptionMessage(pVCpu, pMsg, true /*fClearXcpt*/);
    uint64_t fWhat = NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM;
    if (pMsg->ExceptionVector == X86_XCPT_DB)
        fWhat |= CPUMCTX_EXTRN_DR0_DR3 | CPUMCTX_EXTRN_DR7 | CPUMCTX_EXTRN_DR6;
    VBOXSTRICTRC rcStrict = nemHCWinImportStateIfNeededStrict(pVCpu, fWhat, "Xcpt");
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /*
     * Handle the intercept.
     */
    TRPMEVENT enmEvtType = TRPM_TRAP;
    switch (pMsg->ExceptionVector)
    {
        /*
         * We get undefined opcodes on VMMCALL(AMD) & VMCALL(Intel) instructions
         * and need to turn them over to GIM.
         *
         * Note! We do not check fGIMTrapXcptUD here ASSUMING that GIM only wants
         *       #UD for handling non-native hypercall instructions.  (IEM will
         *       decode both and let the GIM provider decide whether to accept it.)
         */
        case X86_XCPT_UD:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitExceptionUd);
            EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_XCPT_UD),
                             pMsg->Header.Rip + pMsg->Header.CsSegment.Base, ASMReadTSC());

            if (nemHcWinIsInterestingUndefinedOpcode(pMsg->InstructionByteCount, pMsg->InstructionBytes,
                                                     pMsg->Header.ExecutionState.EferLma && pMsg->Header.CsSegment.Long ))
            {
                rcStrict = IEMExecOneWithPrefetchedByPC(pVCpu, CPUMCTX2CORE(&pVCpu->cpum.GstCtx), pMsg->Header.Rip,
                                                        pMsg->InstructionBytes, pMsg->InstructionByteCount);
                Log4(("XcptExit/%u: %04x:%08RX64/%s: #UD -> emulated -> %Rrc\n",
                      pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip,
                      nemHCWinExecStateToLogStr(&pMsg->Header), VBOXSTRICTRC_VAL(rcStrict) ));
                STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitExceptionUdHandled);
                return rcStrict;
            }
            Log4(("XcptExit/%u: %04x:%08RX64/%s: #UD [%.*Rhxs] -> re-injected\n", pVCpu->idCpu, pMsg->Header.CsSegment.Selector,
                  pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header),  pMsg->InstructionByteCount, pMsg->InstructionBytes ));
            break;

        /*
         * Workaround the lovely mesa driver assuming that vmsvga means vmware
         * hypervisor and tries to log stuff to the host.
         */
        case X86_XCPT_GP:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitExceptionGp);
            /** @todo r=bird: Need workaround in IEM for this, right?
            EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_XCPT_GP),
                             pMsg->Header.Rip + pMsg->Header.CsSegment.Base, ASMReadTSC()); */
            if (   !pVCpu->hm.s.fTrapXcptGpForLovelyMesaDrv
                || !nemHcWinIsMesaDrvGp(pVCpu, &pVCpu->cpum.GstCtx, pMsg->InstructionBytes, pMsg->InstructionByteCount))
            {
# if 1 /** @todo Need to emulate instruction or we get a triple fault when trying to inject the #GP... */
                rcStrict = IEMExecOneWithPrefetchedByPC(pVCpu, CPUMCTX2CORE(&pVCpu->cpum.GstCtx), pMsg->Header.Rip,
                                                        pMsg->InstructionBytes, pMsg->InstructionByteCount);
                Log4(("XcptExit/%u: %04x:%08RX64/%s: #GP -> emulated -> %Rrc\n",
                      pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip,
                      nemHCWinExecStateToLogStr(&pMsg->Header), VBOXSTRICTRC_VAL(rcStrict) ));
                return rcStrict;
# else
                break;
# endif
            }
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitExceptionGpMesa);
            return nemHcWinHandleMesaDrvGp(pVCpu, &pVCpu->cpum.GstCtx);

        /*
         * Filter debug exceptions.
         */
        case X86_XCPT_DB:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitExceptionDb);
            EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_XCPT_DB),
                             pMsg->Header.Rip + pMsg->Header.CsSegment.Base, ASMReadTSC());
            Log4(("XcptExit/%u: %04x:%08RX64/%s: #DB - TODO\n",
                  pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header) ));
            break;

        case X86_XCPT_BP:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitExceptionBp);
            EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_XCPT_BP),
                             pMsg->Header.Rip + pMsg->Header.CsSegment.Base, ASMReadTSC());
            Log4(("XcptExit/%u: %04x:%08RX64/%s: #BP - TODO - %u\n", pVCpu->idCpu, pMsg->Header.CsSegment.Selector,
                  pMsg->Header.Rip, nemHCWinExecStateToLogStr(&pMsg->Header), pMsg->Header.InstructionLength));
            enmEvtType = TRPM_SOFTWARE_INT; /* We're at the INT3 instruction, not after it. */
            break;

        /* This shouldn't happen. */
        default:
            AssertLogRelMsgFailedReturn(("ExceptionVector=%#x\n", pMsg->ExceptionVector),  VERR_IEM_IPE_6);
    }

    /*
     * Inject it.
     */
    rcStrict = IEMInjectTrap(pVCpu, pMsg->ExceptionVector, enmEvtType, pMsg->ErrorCode,
                             pMsg->ExceptionParameter /*??*/, pMsg->Header.InstructionLength);
    Log4(("XcptExit/%u: %04x:%08RX64/%s: %#u -> injected -> %Rrc\n",
          pVCpu->idCpu, pMsg->Header.CsSegment.Selector, pMsg->Header.Rip,
          nemHCWinExecStateToLogStr(&pMsg->Header), pMsg->ExceptionVector, VBOXSTRICTRC_VAL(rcStrict) ));
    return rcStrict;
}
#elif defined(IN_RING3)
/**
 * Deals with MSR access exits (WHvRunVpExitReasonException).
 *
 * @returns Strict VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context per CPU structure.
 * @param   pExit           The VM exit information to handle.
 * @sa      nemR3WinHandleExitException
 */
NEM_TMPL_STATIC VBOXSTRICTRC nemR3WinHandleExitException(PVMCC pVM, PVMCPUCC pVCpu, WHV_RUN_VP_EXIT_CONTEXT const *pExit)
{
    /*
     * Get most of the register state since we'll end up making IEM inject the
     * event.  The exception isn't normally flaged as a pending event, so duh.
     *
     * Note! We can optimize this later with event injection.
     */
    Log4(("XcptExit/%u: %04x:%08RX64/%s: %x errcd=%#x parm=%RX64\n", pVCpu->idCpu, pExit->VpContext.Cs.Selector,
          pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext), pExit->VpException.ExceptionType,
          pExit->VpException.ErrorCode, pExit->VpException.ExceptionParameter ));
    nemR3WinCopyStateFromExceptionMessage(pVCpu, pExit, true /*fClearXcpt*/);
    uint64_t fWhat = NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM;
    if (pExit->VpException.ExceptionType == X86_XCPT_DB)
        fWhat |= CPUMCTX_EXTRN_DR0_DR3 | CPUMCTX_EXTRN_DR7 | CPUMCTX_EXTRN_DR6;
    VBOXSTRICTRC rcStrict = nemHCWinImportStateIfNeededStrict(pVCpu, fWhat, "Xcpt");
    if (rcStrict != VINF_SUCCESS)
        return rcStrict;

    /*
     * Handle the intercept.
     */
    TRPMEVENT enmEvtType = TRPM_TRAP;
    switch (pExit->VpException.ExceptionType)
    {
        /*
         * We get undefined opcodes on VMMCALL(AMD) & VMCALL(Intel) instructions
         * and need to turn them over to GIM.
         *
         * Note! We do not check fGIMTrapXcptUD here ASSUMING that GIM only wants
         *       #UD for handling non-native hypercall instructions.  (IEM will
         *       decode both and let the GIM provider decide whether to accept it.)
         */
        case X86_XCPT_UD:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitExceptionUd);
            EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_XCPT_UD),
                             pExit->VpContext.Rip + pExit->VpContext.Cs.Base, ASMReadTSC());
            if (nemHcWinIsInterestingUndefinedOpcode(pExit->VpException.InstructionByteCount, pExit->VpException.InstructionBytes,
                                                     pExit->VpContext.ExecutionState.EferLma && pExit->VpContext.Cs.Long ))
            {
                rcStrict = IEMExecOneWithPrefetchedByPC(pVCpu, CPUMCTX2CORE(&pVCpu->cpum.GstCtx), pExit->VpContext.Rip,
                                                        pExit->VpException.InstructionBytes,
                                                        pExit->VpException.InstructionByteCount);
                Log4(("XcptExit/%u: %04x:%08RX64/%s: #UD -> emulated -> %Rrc\n",
                      pVCpu->idCpu, pExit->VpContext.Cs.Selector, pExit->VpContext.Rip,
                      nemR3WinExecStateToLogStr(&pExit->VpContext), VBOXSTRICTRC_VAL(rcStrict) ));
                STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitExceptionUdHandled);
                return rcStrict;
            }

            Log4(("XcptExit/%u: %04x:%08RX64/%s: #UD [%.*Rhxs] -> re-injected\n", pVCpu->idCpu,
                  pExit->VpContext.Cs.Selector, pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext),
                  pExit->VpException.InstructionByteCount, pExit->VpException.InstructionBytes ));
            break;

        /*
         * Workaround the lovely mesa driver assuming that vmsvga means vmware
         * hypervisor and tries to log stuff to the host.
         */
        case X86_XCPT_GP:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitExceptionGp);
            /** @todo r=bird: Need workaround in IEM for this, right?
            EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_XCPT_GP),
                             pExit->VpContext.Rip + pExit->VpContext.Cs.Base, ASMReadTSC()); */
            if (   !pVCpu->nem.s.fTrapXcptGpForLovelyMesaDrv
                || !nemHcWinIsMesaDrvGp(pVCpu, &pVCpu->cpum.GstCtx, pExit->VpException.InstructionBytes,
                                        pExit->VpException.InstructionByteCount))
            {
# if 1 /** @todo Need to emulate instruction or we get a triple fault when trying to inject the #GP... */
                rcStrict = IEMExecOneWithPrefetchedByPC(pVCpu, CPUMCTX2CORE(&pVCpu->cpum.GstCtx), pExit->VpContext.Rip,
                                                        pExit->VpException.InstructionBytes,
                                                        pExit->VpException.InstructionByteCount);
                Log4(("XcptExit/%u: %04x:%08RX64/%s: #GP -> emulated -> %Rrc\n",
                      pVCpu->idCpu, pExit->VpContext.Cs.Selector, pExit->VpContext.Rip,
                      nemR3WinExecStateToLogStr(&pExit->VpContext), VBOXSTRICTRC_VAL(rcStrict) ));
                STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitExceptionUdHandled);
                return rcStrict;
# else
                break;
# endif
            }
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitExceptionGpMesa);
            return nemHcWinHandleMesaDrvGp(pVCpu, &pVCpu->cpum.GstCtx);

        /*
         * Filter debug exceptions.
         */
        case X86_XCPT_DB:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitExceptionDb);
            EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_XCPT_DB),
                             pExit->VpContext.Rip + pExit->VpContext.Cs.Base, ASMReadTSC());
            Log4(("XcptExit/%u: %04x:%08RX64/%s: #DB - TODO\n",
                  pVCpu->idCpu, pExit->VpContext.Cs.Selector, pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext) ));
            break;

        case X86_XCPT_BP:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitExceptionBp);
            EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_XCPT_BP),
                             pExit->VpContext.Rip + pExit->VpContext.Cs.Base, ASMReadTSC());
            Log4(("XcptExit/%u: %04x:%08RX64/%s: #BP - TODO - %u\n", pVCpu->idCpu, pExit->VpContext.Cs.Selector,
                  pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext), pExit->VpContext.InstructionLength));
            enmEvtType = TRPM_SOFTWARE_INT; /* We're at the INT3 instruction, not after it. */
            break;

        /* This shouldn't happen. */
        default:
            AssertLogRelMsgFailedReturn(("ExceptionType=%#x\n", pExit->VpException.ExceptionType),  VERR_IEM_IPE_6);
    }

    /*
     * Inject it.
     */
    rcStrict = IEMInjectTrap(pVCpu, pExit->VpException.ExceptionType, enmEvtType, pExit->VpException.ErrorCode,
                             pExit->VpException.ExceptionParameter /*??*/, pExit->VpContext.InstructionLength);
    Log4(("XcptExit/%u: %04x:%08RX64/%s: %#u -> injected -> %Rrc\n",
          pVCpu->idCpu, pExit->VpContext.Cs.Selector, pExit->VpContext.Rip,
          nemR3WinExecStateToLogStr(&pExit->VpContext), pExit->VpException.ExceptionType, VBOXSTRICTRC_VAL(rcStrict) ));

    RT_NOREF_PV(pVM);
    return rcStrict;
}
#endif /* IN_RING3 && !NEM_WIN_TEMPLATE_MODE_OWN_RUN_API */


#ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
/**
 * Deals with unrecoverable exception (triple fault).
 *
 * Seen WRMSR 0x201 (IA32_MTRR_PHYSMASK0) writes from grub / debian9 ending up
 * here too.  So we'll leave it to IEM to decide.
 *
 * @returns Strict VBox status code.
 * @param   pVCpu           The cross context per CPU structure.
 * @param   pMsgHdr         The message header.
 * @sa      nemR3WinHandleExitUnrecoverableException
 */
NEM_TMPL_STATIC VBOXSTRICTRC
nemHCWinHandleMessageUnrecoverableException(PVMCPUCC pVCpu, HV_X64_INTERCEPT_MESSAGE_HEADER const *pMsgHdr)
{
    /* Check message register value sanity. */
    NEMWIN_ASSERT_MSG_REG_SEG(  pVCpu, HvX64RegisterCs, pMsgHdr->CsSegment);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRip, pMsgHdr->Rip);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterRflags, pMsgHdr->Rflags);
    NEMWIN_ASSERT_MSG_REG_VAL64(pVCpu, HvX64RegisterCr8, (uint64_t)pMsgHdr->Cr8);

# if 0
    /*
     * Just copy the state we've got and handle it in the loop for now.
     */
    nemHCWinCopyStateFromX64Header(pVCpu, pMsgHdr);
    Log(("TripleExit/%u: %04x:%08RX64/%s: RFL=%#RX64 -> VINF_EM_TRIPLE_FAULT\n",
         pVCpu->idCpu, pMsgHdr->CsSegment.Selector, pMsgHdr->Rip, nemHCWinExecStateToLogStr(&pMsg->Header), pMsgHdr->Rflags));
    return VINF_EM_TRIPLE_FAULT;
# else
    /*
     * Let IEM decide whether this is really it.
     */
    EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_UNRECOVERABLE_EXCEPTION),
                     pMsgHdr->Rip + pMsgHdr->CsSegment.Base, ASMReadTSC());
    nemHCWinCopyStateFromX64Header(pVCpu, pMsgHdr);
    VBOXSTRICTRC rcStrict = nemHCWinImportStateIfNeededStrict(pVCpu, NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM | CPUMCTX_EXTRN_ALL, "TripleExit");
    if (rcStrict == VINF_SUCCESS)
    {
        rcStrict = IEMExecOne(pVCpu);
        if (rcStrict == VINF_SUCCESS)
        {
            Log(("UnrecovExit/%u: %04x:%08RX64/%s: RFL=%#RX64 -> VINF_SUCCESS\n", pVCpu->idCpu, pMsgHdr->CsSegment.Selector,
                 pMsgHdr->Rip, nemHCWinExecStateToLogStr(pMsgHdr), pMsgHdr->Rflags ));
            pVCpu->cpum.GstCtx.fExtrn &= ~CPUMCTX_EXTRN_NEM_WIN_EVENT_INJECT; /* Make sure to reset pending #DB(0). */
            return VINF_SUCCESS;
        }
        if (rcStrict == VINF_EM_TRIPLE_FAULT)
            Log(("UnrecovExit/%u: %04x:%08RX64/%s: RFL=%#RX64 -> VINF_EM_TRIPLE_FAULT!\n", pVCpu->idCpu, pMsgHdr->CsSegment.Selector,
                 pMsgHdr->Rip, nemHCWinExecStateToLogStr(pMsgHdr), pMsgHdr->Rflags, VBOXSTRICTRC_VAL(rcStrict) ));
        else
            Log(("UnrecovExit/%u: %04x:%08RX64/%s: RFL=%#RX64 -> %Rrc (IEMExecOne)\n", pVCpu->idCpu, pMsgHdr->CsSegment.Selector,
                 pMsgHdr->Rip, nemHCWinExecStateToLogStr(pMsgHdr), pMsgHdr->Rflags, VBOXSTRICTRC_VAL(rcStrict) ));
    }
    else
        Log(("UnrecovExit/%u: %04x:%08RX64/%s: RFL=%#RX64 -> %Rrc (state import)\n", pVCpu->idCpu, pMsgHdr->CsSegment.Selector,
             pMsgHdr->Rip, nemHCWinExecStateToLogStr(pMsgHdr), pMsgHdr->Rflags, VBOXSTRICTRC_VAL(rcStrict) ));
    return rcStrict;
# endif
}
#elif defined(IN_RING3)
/**
 * Deals with MSR access exits (WHvRunVpExitReasonUnrecoverableException).
 *
 * @returns Strict VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context per CPU structure.
 * @param   pExit           The VM exit information to handle.
 * @sa      nemHCWinHandleMessageUnrecoverableException
 */
NEM_TMPL_STATIC VBOXSTRICTRC nemR3WinHandleExitUnrecoverableException(PVMCC pVM, PVMCPUCC pVCpu, WHV_RUN_VP_EXIT_CONTEXT const *pExit)
{
# if 0
    /*
     * Just copy the state we've got and handle it in the loop for now.
     */
    nemR3WinCopyStateFromX64Header(pVCpu, &pExit->VpContext);
    Log(("TripleExit/%u: %04x:%08RX64/%s: RFL=%#RX64 -> VINF_EM_TRIPLE_FAULT\n", pVCpu->idCpu, pExit->VpContext.Cs.Selector,
         pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext), pExit->VpContext.Rflags));
    RT_NOREF_PV(pVM);
    return VINF_EM_TRIPLE_FAULT;
# else
    /*
     * Let IEM decide whether this is really it.
     */
    EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_UNRECOVERABLE_EXCEPTION),
                     pExit->VpContext.Rip + pExit->VpContext.Cs.Base, ASMReadTSC());
    nemR3WinCopyStateFromX64Header(pVCpu, &pExit->VpContext);
    VBOXSTRICTRC rcStrict = nemHCWinImportStateIfNeededStrict(pVCpu, NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM | CPUMCTX_EXTRN_ALL, "TripleExit");
    if (rcStrict == VINF_SUCCESS)
    {
        rcStrict = IEMExecOne(pVCpu);
        if (rcStrict == VINF_SUCCESS)
        {
            Log(("UnrecovExit/%u: %04x:%08RX64/%s: RFL=%#RX64 -> VINF_SUCCESS\n", pVCpu->idCpu, pExit->VpContext.Cs.Selector,
                 pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext), pExit->VpContext.Rflags));
            pVCpu->cpum.GstCtx.fExtrn &= ~CPUMCTX_EXTRN_NEM_WIN_EVENT_INJECT; /* Make sure to reset pending #DB(0). */
            return VINF_SUCCESS;
        }
        if (rcStrict == VINF_EM_TRIPLE_FAULT)
            Log(("UnrecovExit/%u: %04x:%08RX64/%s: RFL=%#RX64 -> VINF_EM_TRIPLE_FAULT!\n", pVCpu->idCpu, pExit->VpContext.Cs.Selector,
                 pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext), pExit->VpContext.Rflags, VBOXSTRICTRC_VAL(rcStrict) ));
        else
            Log(("UnrecovExit/%u: %04x:%08RX64/%s: RFL=%#RX64 -> %Rrc (IEMExecOne)\n", pVCpu->idCpu, pExit->VpContext.Cs.Selector,
                 pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext), pExit->VpContext.Rflags, VBOXSTRICTRC_VAL(rcStrict) ));
    }
    else
        Log(("UnrecovExit/%u: %04x:%08RX64/%s: RFL=%#RX64 -> %Rrc (state import)\n", pVCpu->idCpu, pExit->VpContext.Cs.Selector,
             pExit->VpContext.Rip, nemR3WinExecStateToLogStr(&pExit->VpContext), pExit->VpContext.Rflags, VBOXSTRICTRC_VAL(rcStrict) ));
    RT_NOREF_PV(pVM);
    return rcStrict;
# endif

}
#endif /* IN_RING3 && !NEM_WIN_TEMPLATE_MODE_OWN_RUN_API */


#ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
/**
 * Handles messages (VM exits).
 *
 * @returns Strict VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context per CPU structure.
 * @param   pMappingHeader  The message slot mapping.
 * @sa      nemR3WinHandleExit
 */
NEM_TMPL_STATIC VBOXSTRICTRC
nemHCWinHandleMessage(PVMCC pVM, PVMCPUCC pVCpu, VID_MESSAGE_MAPPING_HEADER volatile *pMappingHeader)
{
    if (pMappingHeader->enmVidMsgType == VidMessageHypervisorMessage)
    {
        AssertMsg(pMappingHeader->cbMessage == HV_MESSAGE_SIZE, ("%#x\n", pMappingHeader->cbMessage));
        HV_MESSAGE const *pMsg = (HV_MESSAGE const *)(pMappingHeader + 1);
        switch (pMsg->Header.MessageType)
        {
            case HvMessageTypeUnmappedGpa:
                Assert(pMsg->Header.PayloadSize == RT_UOFFSETOF(HV_X64_MEMORY_INTERCEPT_MESSAGE, DsSegment));
                STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitMemUnmapped);
                return nemHCWinHandleMessageMemory(pVM, pVCpu, &pMsg->X64MemoryIntercept);

            case HvMessageTypeGpaIntercept:
                Assert(pMsg->Header.PayloadSize == RT_UOFFSETOF(HV_X64_MEMORY_INTERCEPT_MESSAGE, DsSegment));
                STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitMemIntercept);
                return nemHCWinHandleMessageMemory(pVM, pVCpu, &pMsg->X64MemoryIntercept);

            case HvMessageTypeX64IoPortIntercept:
                Assert(pMsg->Header.PayloadSize == sizeof(pMsg->X64IoPortIntercept));
                STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitPortIo);
                return nemHCWinHandleMessageIoPort(pVM, pVCpu, &pMsg->X64IoPortIntercept);

            case HvMessageTypeX64Halt:
                STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitHalt);
                EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_HALT),
                                 pMsg->X64InterceptHeader.Rip + pMsg->X64InterceptHeader.CsSegment.Base, ASMReadTSC());
                Log4(("HaltExit\n"));
                return VINF_EM_HALT;

            case HvMessageTypeX64InterruptWindow:
                Assert(pMsg->Header.PayloadSize == sizeof(pMsg->X64InterruptWindow));
                STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitInterruptWindow);
                return nemHCWinHandleMessageInterruptWindow(pVM, pVCpu, &pMsg->X64InterruptWindow);

            case HvMessageTypeX64CpuidIntercept:
                Assert(pMsg->Header.PayloadSize == sizeof(pMsg->X64CpuIdIntercept));
                STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitCpuId);
                return nemHCWinHandleMessageCpuId(pVM, pVCpu, &pMsg->X64CpuIdIntercept);

            case HvMessageTypeX64MsrIntercept:
                Assert(pMsg->Header.PayloadSize == sizeof(pMsg->X64MsrIntercept));
                STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitMsr);
                return nemHCWinHandleMessageMsr(pVCpu, &pMsg->X64MsrIntercept);

            case HvMessageTypeX64ExceptionIntercept:
                Assert(pMsg->Header.PayloadSize == sizeof(pMsg->X64ExceptionIntercept));
                STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitException);
                return nemHCWinHandleMessageException(pVCpu, &pMsg->X64ExceptionIntercept);

            case HvMessageTypeUnrecoverableException:
                Assert(pMsg->Header.PayloadSize == sizeof(pMsg->X64InterceptHeader));
                STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitUnrecoverable);
                return nemHCWinHandleMessageUnrecoverableException(pVCpu, &pMsg->X64InterceptHeader);

            case HvMessageTypeInvalidVpRegisterValue:
            case HvMessageTypeUnsupportedFeature:
            case HvMessageTypeTlbPageSizeMismatch:
                LogRel(("Unimplemented msg:\n%.*Rhxd\n", (int)sizeof(*pMsg), pMsg));
                AssertLogRelMsgFailedReturn(("Message type %#x not implemented!\n%.32Rhxd\n", pMsg->Header.MessageType, pMsg),
                                            VERR_NEM_IPE_3);

            case HvMessageTypeX64ApicEoi:
            case HvMessageTypeX64LegacyFpError:
            case HvMessageTypeX64RegisterIntercept:
            case HvMessageTypeApicEoi:
            case HvMessageTypeFerrAsserted:
            case HvMessageTypeEventLogBufferComplete:
            case HvMessageTimerExpired:
                LogRel(("Unexpected msg:\n%.*Rhxd\n", (int)sizeof(*pMsg), pMsg));
                AssertLogRelMsgFailedReturn(("Unexpected message on CPU #%u: %#x\n", pVCpu->idCpu, pMsg->Header.MessageType),
                                            VERR_NEM_IPE_3);

            default:
                LogRel(("Unknown msg:\n%.*Rhxd\n", (int)sizeof(*pMsg), pMsg));
                AssertLogRelMsgFailedReturn(("Unknown message on CPU #%u: %#x\n", pVCpu->idCpu, pMsg->Header.MessageType),
                                            VERR_NEM_IPE_3);
        }
    }
    else
        AssertLogRelMsgFailedReturn(("Unexpected VID message type on CPU #%u: %#x LB %u\n",
                                     pVCpu->idCpu, pMappingHeader->enmVidMsgType, pMappingHeader->cbMessage),
                                    VERR_NEM_IPE_4);
}
#elif defined(IN_RING3)
/**
 * Handles VM exits.
 *
 * @returns Strict VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context per CPU structure.
 * @param   pExit           The VM exit information to handle.
 * @sa      nemHCWinHandleMessage
 */
NEM_TMPL_STATIC VBOXSTRICTRC nemR3WinHandleExit(PVMCC pVM, PVMCPUCC pVCpu, WHV_RUN_VP_EXIT_CONTEXT const *pExit)
{
    switch (pExit->ExitReason)
    {
        case WHvRunVpExitReasonMemoryAccess:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitMemUnmapped);
            return nemR3WinHandleExitMemory(pVM, pVCpu, pExit);

        case WHvRunVpExitReasonX64IoPortAccess:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitPortIo);
            return nemR3WinHandleExitIoPort(pVM, pVCpu, pExit);

        case WHvRunVpExitReasonX64Halt:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitHalt);
            EMHistoryAddExit(pVCpu, EMEXIT_MAKE_FT(EMEXIT_F_KIND_NEM, NEMEXITTYPE_HALT),
                             pExit->VpContext.Rip + pExit->VpContext.Cs.Base, ASMReadTSC());
            Log4(("HaltExit\n"));
            return VINF_EM_HALT;

        case WHvRunVpExitReasonCanceled:
            return VINF_SUCCESS;

        case WHvRunVpExitReasonX64InterruptWindow:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitInterruptWindow);
            return nemR3WinHandleExitInterruptWindow(pVM, pVCpu, pExit);

        case WHvRunVpExitReasonX64Cpuid:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitCpuId);
            return nemR3WinHandleExitCpuId(pVM, pVCpu, pExit);

        case WHvRunVpExitReasonX64MsrAccess:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitMsr);
            return nemR3WinHandleExitMsr(pVM, pVCpu, pExit);

        case WHvRunVpExitReasonException:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitException);
            return nemR3WinHandleExitException(pVM, pVCpu, pExit);

        case WHvRunVpExitReasonUnrecoverableException:
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatExitUnrecoverable);
            return nemR3WinHandleExitUnrecoverableException(pVM, pVCpu, pExit);

        case WHvRunVpExitReasonUnsupportedFeature:
        case WHvRunVpExitReasonInvalidVpRegisterValue:
            LogRel(("Unimplemented exit:\n%.*Rhxd\n", (int)sizeof(*pExit), pExit));
            AssertLogRelMsgFailedReturn(("Unexpected exit on CPU #%u: %#x\n%.32Rhxd\n",
                                         pVCpu->idCpu, pExit->ExitReason, pExit), VERR_NEM_IPE_3);

        /* Undesired exits: */
        case WHvRunVpExitReasonNone:
        default:
            LogRel(("Unknown exit:\n%.*Rhxd\n", (int)sizeof(*pExit), pExit));
            AssertLogRelMsgFailedReturn(("Unknown exit on CPU #%u: %#x!\n", pVCpu->idCpu, pExit->ExitReason), VERR_NEM_IPE_3);
    }
}
#endif /* IN_RING3 && !NEM_WIN_TEMPLATE_MODE_OWN_RUN_API */


#ifdef IN_RING0
/**
 * Perform an I/O control operation on the partition handle (VID.SYS),
 * restarting on alert-like behaviour.
 *
 * @returns NT status code.
 * @param   pGVM            The ring-0 VM structure.
 * @param   pGVCpu          The global (ring-0) per CPU structure.
 * @param   fFlags          The wait flags.
 * @param   cMillies        The timeout in milliseconds
 */
static NTSTATUS nemR0NtPerformIoCtlMessageSlotHandleAndGetNext(PGVM pGVM, PGVMCPU pGVCpu, uint32_t fFlags, uint32_t cMillies)
{
    pGVCpu->nem.s.uIoCtlBuf.MsgSlotHandleAndGetNext.iCpu     = pGVCpu->idCpu;
    pGVCpu->nem.s.uIoCtlBuf.MsgSlotHandleAndGetNext.fFlags   = fFlags;
    pGVCpu->nem.s.uIoCtlBuf.MsgSlotHandleAndGetNext.cMillies = cMillies;
    NTSTATUS rcNt = nemR0NtPerformIoControl(pGVM, pGVCpu, pGVM->nemr0.s.IoCtlMessageSlotHandleAndGetNext.uFunction,
                                            &pGVCpu->nem.s.uIoCtlBuf.MsgSlotHandleAndGetNext,
                                            pGVM->nemr0.s.IoCtlMessageSlotHandleAndGetNext.cbInput,
                                            NULL, 0);
    if (rcNt == STATUS_SUCCESS)
    { /* likely */ }
    /*
     * Generally, if we get down here, we have been interrupted between ACK'ing
     * a message and waiting for the next due to a NtAlertThread call.  So, we
     * should stop ACK'ing the previous message and get on waiting on the next.
     * See similar stuff in nemHCWinRunGC().
     */
    else if (   rcNt == STATUS_TIMEOUT
             || rcNt == STATUS_ALERTED    /* just in case */
             || rcNt == STATUS_KERNEL_APC /* just in case */
             || rcNt == STATUS_USER_APC   /* just in case */)
    {
        DBGFTRACE_CUSTOM(pGVCpu->CTX_SUFF(pVM), "IoCtlMessageSlotHandleAndGetNextRestart/1 %#x (f=%#x)", rcNt, fFlags);
        STAM_REL_COUNTER_INC(&pGVCpu->nem.s.StatStopCpuPendingAlerts);
        Assert(fFlags & VID_MSHAGN_F_GET_NEXT_MESSAGE);

        pGVCpu->nem.s.uIoCtlBuf.MsgSlotHandleAndGetNext.iCpu     = pGVCpu->idCpu;
        pGVCpu->nem.s.uIoCtlBuf.MsgSlotHandleAndGetNext.fFlags   = fFlags & ~VID_MSHAGN_F_HANDLE_MESSAGE;
        pGVCpu->nem.s.uIoCtlBuf.MsgSlotHandleAndGetNext.cMillies = cMillies;
        rcNt = nemR0NtPerformIoControl(pGVM, pGVCpu, pGVM->nemr0.s.IoCtlMessageSlotHandleAndGetNext.uFunction,
                                       &pGVCpu->nem.s.uIoCtlBuf.MsgSlotHandleAndGetNext,
                                       pGVM->nemr0.s.IoCtlMessageSlotHandleAndGetNext.cbInput,
                                       NULL, 0);
        DBGFTRACE_CUSTOM(pGVM, "IoCtlMessageSlotHandleAndGetNextRestart/2 %#x", rcNt);
    }
    return rcNt;
}
#endif /* IN_RING0 */


#ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
/**
 * Worker for nemHCWinRunGC that stops the execution on the way out.
 *
 * The CPU was running the last time we checked, no there are no messages that
 * needs being marked handled/whatever.  Caller checks this.
 *
 * @returns rcStrict on success, error status on failure.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context per CPU structure.
 * @param   rcStrict        The nemHCWinRunGC return status.  This is a little
 *                          bit unnecessary, except in internal error cases,
 *                          since we won't need to stop the CPU if we took an
 *                          exit.
 * @param   pMappingHeader  The message slot mapping.
 */
NEM_TMPL_STATIC VBOXSTRICTRC nemHCWinStopCpu(PVMCC pVM, PVMCPUCC pVCpu, VBOXSTRICTRC rcStrict,
                                             VID_MESSAGE_MAPPING_HEADER volatile *pMappingHeader)
{
# ifdef DBGFTRACE_ENABLED
    HV_MESSAGE const volatile *pMsgForTrace = (HV_MESSAGE const volatile *)(pMappingHeader + 1);
# endif

    /*
     * Try stopping the processor.  If we're lucky we manage to do this before it
     * does another VM exit.
     */
    DBGFTRACE_CUSTOM(pVM, "nemStop#0");
# ifdef IN_RING0
    pVCpu->nem.s.uIoCtlBuf.idCpu = pVCpu->idCpu;
    NTSTATUS rcNt = nemR0NtPerformIoControl(pVM, pVCpu, pVM->nemr0.s.IoCtlStopVirtualProcessor.uFunction,
                                            &pVCpu->nem.s.uIoCtlBuf.idCpu, sizeof(pVCpu->nem.s.uIoCtlBuf.idCpu),
                                            NULL, 0);
    if (NT_SUCCESS(rcNt))
    {
        DBGFTRACE_CUSTOM(pVM, "nemStop#0: okay (%#x)", rcNt);
        Log8(("nemHCWinStopCpu: Stopping CPU succeeded (cpu status %u)\n", nemHCWinCpuGetRunningStatus(pVCpu) ));
        STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatStopCpuSuccess);
        return rcStrict;
    }
# else
    BOOL fRet = VidStopVirtualProcessor(pVM->nem.s.hPartitionDevice, pVCpu->idCpu);
    if (fRet)
    {
        DBGFTRACE_CUSTOM(pVM, "nemStop#0: okay");
        Log8(("nemHCWinStopCpu: Stopping CPU succeeded (cpu status %u)\n", nemHCWinCpuGetRunningStatus(pVCpu) ));
        STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatStopCpuSuccess);
        return rcStrict;
    }
# endif

    /*
     * Dang. The CPU stopped by itself and we got a couple of message to deal with.
     */
# ifdef IN_RING0
    DBGFTRACE_CUSTOM(pVM, "nemStop#0: pending (%#x)", rcNt);
    AssertLogRelMsgReturn(rcNt == ERROR_VID_STOP_PENDING, ("rcNt=%#x\n", rcNt),
                          RT_SUCCESS(rcStrict) ?  VERR_NEM_IPE_5 : rcStrict);
# else
    DWORD dwErr = RTNtLastErrorValue();
    DBGFTRACE_CUSTOM(pVM, "nemStop#0: pending (%#x)", dwErr);
    AssertLogRelMsgReturn(dwErr == ERROR_VID_STOP_PENDING, ("dwErr=%#u (%#x)\n", dwErr, dwErr),
                          RT_SUCCESS(rcStrict) ?  VERR_NEM_IPE_5 : rcStrict);
# endif
    Log8(("nemHCWinStopCpu: Stopping CPU #%u pending...\n", pVCpu->idCpu));
    STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatStopCpuPending);

    /*
     * First message: Exit or similar, sometimes VidMessageStopRequestComplete.
     * Note! We can safely ASSUME that rcStrict isn't an important information one.
     */
# ifdef IN_RING0
    rcNt = nemR0NtPerformIoCtlMessageSlotHandleAndGetNext(pVM, pVCpu, VID_MSHAGN_F_GET_NEXT_MESSAGE, 30000 /*ms*/);
    DBGFTRACE_CUSTOM(pVM, "nemStop#1: %#x / %#x %#x %#x", rcNt, pMappingHeader->enmVidMsgType, pMappingHeader->cbMessage,
                     pMsgForTrace->Header.MessageType);
    AssertLogRelMsgReturn(rcNt == STATUS_SUCCESS,
                          ("1st VidMessageSlotHandleAndGetNext after ERROR_VID_STOP_PENDING failed: %#x\n", rcNt),
                          RT_SUCCESS(rcStrict) ? VERR_NEM_IPE_5 : rcStrict);
# else
    BOOL fWait = g_pfnVidMessageSlotHandleAndGetNext(pVM->nem.s.hPartitionDevice, pVCpu->idCpu,
                                                     VID_MSHAGN_F_GET_NEXT_MESSAGE, 30000 /*ms*/);
    DBGFTRACE_CUSTOM(pVM, "nemStop#1: %d+%#x / %#x %#x %#x", fWait, RTNtLastErrorValue(), pMappingHeader->enmVidMsgType,
                     pMappingHeader->cbMessage, pMsgForTrace->Header.MessageType);
    AssertLogRelMsgReturn(fWait, ("1st VidMessageSlotHandleAndGetNext after ERROR_VID_STOP_PENDING failed: %u\n", RTNtLastErrorValue()),
                          RT_SUCCESS(rcStrict) ? VERR_NEM_IPE_5 : rcStrict);
# endif

    VID_MESSAGE_TYPE enmVidMsgType = pMappingHeader->enmVidMsgType;
    if (enmVidMsgType != VidMessageStopRequestComplete)
    {
        VBOXSTRICTRC rcStrict2 = nemHCWinHandleMessage(pVM, pVCpu, pMappingHeader);
        if (rcStrict2 != VINF_SUCCESS && RT_SUCCESS(rcStrict))
            rcStrict = rcStrict2;
        DBGFTRACE_CUSTOM(pVM, "nemStop#1: handled %#x -> %d", pMsgForTrace->Header.MessageType, VBOXSTRICTRC_VAL(rcStrict));

        /*
         * Mark it as handled and get the stop request completed message, then mark
         * that as handled too.  CPU is back into fully stopped stated then.
         */
# ifdef IN_RING0
        rcNt = nemR0NtPerformIoCtlMessageSlotHandleAndGetNext(pVM, pVCpu,
                                                              VID_MSHAGN_F_HANDLE_MESSAGE | VID_MSHAGN_F_GET_NEXT_MESSAGE,
                                                              30000 /*ms*/);
        DBGFTRACE_CUSTOM(pVM, "nemStop#2: %#x / %#x %#x %#x", rcNt, pMappingHeader->enmVidMsgType, pMappingHeader->cbMessage,
                         pMsgForTrace->Header.MessageType);
        AssertLogRelMsgReturn(rcNt == STATUS_SUCCESS,
                              ("2nd VidMessageSlotHandleAndGetNext after ERROR_VID_STOP_PENDING failed: %#x\n", rcNt),
                              RT_SUCCESS(rcStrict) ? VERR_NEM_IPE_5 : rcStrict);
# else
        fWait = g_pfnVidMessageSlotHandleAndGetNext(pVM->nem.s.hPartitionDevice, pVCpu->idCpu,
                                                    VID_MSHAGN_F_HANDLE_MESSAGE | VID_MSHAGN_F_GET_NEXT_MESSAGE, 30000 /*ms*/);
        DBGFTRACE_CUSTOM(pVM, "nemStop#2: %d+%#x / %#x %#x %#x", fWait, RTNtLastErrorValue(), pMappingHeader->enmVidMsgType,
                         pMappingHeader->cbMessage, pMsgForTrace->Header.MessageType);
        AssertLogRelMsgReturn(fWait, ("2nd VidMessageSlotHandleAndGetNext after ERROR_VID_STOP_PENDING failed: %u\n", RTNtLastErrorValue()),
                              RT_SUCCESS(rcStrict) ? VERR_NEM_IPE_5 : rcStrict);
# endif

        /* It should be a stop request completed message. */
        enmVidMsgType = pMappingHeader->enmVidMsgType;
        AssertLogRelMsgReturn(enmVidMsgType == VidMessageStopRequestComplete,
                              ("Unexpected 2nd message following ERROR_VID_STOP_PENDING: %#x LB %#x\n",
                               enmVidMsgType, pMappingHeader->cbMessage),
                              RT_SUCCESS(rcStrict) ? VERR_NEM_IPE_5 : rcStrict);

        /*
         * Mark the VidMessageStopRequestComplete message as handled.
         */
# ifdef IN_RING0
        rcNt = nemR0NtPerformIoCtlMessageSlotHandleAndGetNext(pVM, pVCpu, VID_MSHAGN_F_HANDLE_MESSAGE, 30000 /*ms*/);
        DBGFTRACE_CUSTOM(pVM, "nemStop#3: %#x / %#x %#x %#x", rcNt, pMappingHeader->enmVidMsgType,
                         pMsgForTrace->Header.MessageType, pMappingHeader->cbMessage, pMsgForTrace->Header.MessageType);
        AssertLogRelMsgReturn(rcNt == STATUS_SUCCESS,
                              ("3rd VidMessageSlotHandleAndGetNext after ERROR_VID_STOP_PENDING failed: %#x\n", rcNt),
                              RT_SUCCESS(rcStrict) ? VERR_NEM_IPE_5 : rcStrict);
# else
        fWait = g_pfnVidMessageSlotHandleAndGetNext(pVM->nem.s.hPartitionDevice, pVCpu->idCpu, VID_MSHAGN_F_HANDLE_MESSAGE, 30000 /*ms*/);
        DBGFTRACE_CUSTOM(pVM, "nemStop#3: %d+%#x / %#x %#x %#x", fWait, RTNtLastErrorValue(), pMappingHeader->enmVidMsgType,
                         pMsgForTrace->Header.MessageType, pMappingHeader->cbMessage, pMsgForTrace->Header.MessageType);
        AssertLogRelMsgReturn(fWait, ("3rd VidMessageSlotHandleAndGetNext after ERROR_VID_STOP_PENDING failed: %u\n", RTNtLastErrorValue()),
                              RT_SUCCESS(rcStrict) ? VERR_NEM_IPE_5 : rcStrict);
# endif
        Log8(("nemHCWinStopCpu: Stopped the CPU (rcStrict=%Rrc)\n", VBOXSTRICTRC_VAL(rcStrict) ));
    }
    else
    {
        /** @todo I'm not so sure about this now... */
        DBGFTRACE_CUSTOM(pVM, "nemStop#9: %#x %#x %#x", pMappingHeader->enmVidMsgType,
                         pMappingHeader->cbMessage, pMsgForTrace->Header.MessageType);
        STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatStopCpuPendingOdd);
        Log8(("nemHCWinStopCpu: Stopped the CPU (rcStrict=%Rrc) - 1st VidMessageSlotHandleAndGetNext got VidMessageStopRequestComplete.\n",
              VBOXSTRICTRC_VAL(rcStrict) ));
    }
    return rcStrict;
}
#endif /* NEM_WIN_TEMPLATE_MODE_OWN_RUN_API */

#if defined(NEM_WIN_TEMPLATE_MODE_OWN_RUN_API) || defined(IN_RING3)

/**
 * Deals with pending interrupt related force flags, may inject interrupt.
 *
 * @returns VBox strict status code.
 * @param   pVM                 The cross context VM structure.
 * @param   pVCpu               The cross context per CPU structure.
 * @param   pfInterruptWindows  Where to return interrupt window flags.
 */
NEM_TMPL_STATIC VBOXSTRICTRC nemHCWinHandleInterruptFF(PVMCC pVM, PVMCPUCC pVCpu, uint8_t *pfInterruptWindows)
{
    Assert(!TRPMHasTrap(pVCpu));
    RT_NOREF_PV(pVM);

    /*
     * First update APIC.  We ASSUME this won't need TPR/CR8.
     */
    if (VMCPU_FF_TEST_AND_CLEAR(pVCpu, VMCPU_FF_UPDATE_APIC))
    {
        APICUpdatePendingInterrupts(pVCpu);
        if (!VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC
                                      | VMCPU_FF_INTERRUPT_NMI  | VMCPU_FF_INTERRUPT_SMI))
            return VINF_SUCCESS;
    }

    /*
     * We don't currently implement SMIs.
     */
    AssertReturn(!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INTERRUPT_SMI), VERR_NEM_IPE_0);

    /*
     * Check if we've got the minimum of state required for deciding whether we
     * can inject interrupts and NMIs.  If we don't have it, get all we might require
     * for injection via IEM.
     */
    bool const fPendingNmi = VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INTERRUPT_NMI);
    uint64_t   fNeedExtrn  = CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT | CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_RFLAGS
                           | (fPendingNmi ? CPUMCTX_EXTRN_NEM_WIN_INHIBIT_NMI : 0);
    if (pVCpu->cpum.GstCtx.fExtrn & fNeedExtrn)
    {
        VBOXSTRICTRC rcStrict = nemHCWinImportStateIfNeededStrict(pVCpu, NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM_XCPT, "IntFF");
        if (rcStrict != VINF_SUCCESS)
            return rcStrict;
    }
    bool const fInhibitInterrupts = VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS)
                                 && EMGetInhibitInterruptsPC(pVCpu) == pVCpu->cpum.GstCtx.rip;

    /*
     * NMI? Try deliver it first.
     */
    if (fPendingNmi)
    {
        if (   !fInhibitInterrupts
            && !VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_BLOCK_NMIS))
        {
            VBOXSTRICTRC rcStrict = nemHCWinImportStateIfNeededStrict(pVCpu, NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM_XCPT, "NMI");
            if (rcStrict == VINF_SUCCESS)
            {
                VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INTERRUPT_NMI);
                rcStrict = IEMInjectTrap(pVCpu, X86_XCPT_NMI, TRPM_HARDWARE_INT, 0, 0, 0);
                Log8(("Injected NMI on %u (%d)\n", pVCpu->idCpu, VBOXSTRICTRC_VAL(rcStrict) ));
            }
            return rcStrict;
        }
        *pfInterruptWindows |= NEM_WIN_INTW_F_NMI;
        Log8(("NMI window pending on %u\n", pVCpu->idCpu));
    }

    /*
     * APIC or PIC interrupt?
     */
    if (VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC))
    {
        if (   !fInhibitInterrupts
            && pVCpu->cpum.GstCtx.rflags.Bits.u1IF)
        {
            AssertCompile(NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM_XCPT & CPUMCTX_EXTRN_APIC_TPR);
            VBOXSTRICTRC rcStrict = nemHCWinImportStateIfNeededStrict(pVCpu, NEM_WIN_CPUMCTX_EXTRN_MASK_FOR_IEM_XCPT, "NMI");
            if (rcStrict == VINF_SUCCESS)
            {
                uint8_t bInterrupt;
                int rc = PDMGetInterrupt(pVCpu, &bInterrupt);
                if (RT_SUCCESS(rc))
                {
                    rcStrict = IEMInjectTrap(pVCpu, bInterrupt, TRPM_HARDWARE_INT, 0, 0, 0);
                    Log8(("Injected interrupt %#x on %u (%d)\n", bInterrupt, pVCpu->idCpu, VBOXSTRICTRC_VAL(rcStrict) ));
                }
                else if (rc == VERR_APIC_INTR_MASKED_BY_TPR)
                {
                    *pfInterruptWindows |= (bInterrupt >> 4 /*??*/) << NEM_WIN_INTW_F_PRIO_SHIFT;
                    Log8(("VERR_APIC_INTR_MASKED_BY_TPR: *pfInterruptWindows=%#x\n", *pfInterruptWindows));
                }
                else
                    Log8(("PDMGetInterrupt failed -> %d\n", rc));
            }
            return rcStrict;
        }
        *pfInterruptWindows |= NEM_WIN_INTW_F_REGULAR;
        Log8(("Interrupt window pending on %u\n", pVCpu->idCpu));
    }

    return VINF_SUCCESS;
}


/**
 * Inner NEM runloop for windows.
 *
 * @returns Strict VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context per CPU structure.
 */
NEM_TMPL_STATIC VBOXSTRICTRC nemHCWinRunGC(PVMCC pVM, PVMCPUCC pVCpu)
{
    LogFlow(("NEM/%u: %04x:%08RX64 efl=%#08RX64 <=\n", pVCpu->idCpu, pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, pVCpu->cpum.GstCtx.rflags));
# ifdef LOG_ENABLED
    if (LogIs3Enabled())
        nemHCWinLogState(pVM, pVCpu);
# endif

    /*
     * Try switch to NEM runloop state.
     */
    if (VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC_NEM, VMCPUSTATE_STARTED))
    { /* likely */ }
    else
    {
        VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC_NEM, VMCPUSTATE_STARTED_EXEC_NEM_CANCELED);
        LogFlow(("NEM/%u: returning immediately because canceled\n", pVCpu->idCpu));
        return VINF_SUCCESS;
    }

    /*
     * The run loop.
     *
     * Current approach to state updating to use the sledgehammer and sync
     * everything every time.  This will be optimized later.
     */
# ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
    VID_MESSAGE_MAPPING_HEADER volatile *pMappingHeader = (VID_MESSAGE_MAPPING_HEADER volatile *)pVCpu->nem.s.pvMsgSlotMapping;
# endif
    const bool      fSingleStepping     = DBGFIsStepping(pVCpu);
//    const uint32_t  fCheckVmFFs         = !fSingleStepping ? VM_FF_HP_R0_PRE_HM_MASK
//                                                           : VM_FF_HP_R0_PRE_HM_STEP_MASK;
//    const uint32_t  fCheckCpuFFs        = !fSingleStepping ? VMCPU_FF_HP_R0_PRE_HM_MASK : VMCPU_FF_HP_R0_PRE_HM_STEP_MASK;
    VBOXSTRICTRC    rcStrict            = VINF_SUCCESS;
    for (unsigned iLoop = 0;; iLoop++)
    {
# ifndef NEM_WIN_USE_HYPERCALLS_FOR_PAGES
        /*
         * Hack alert!
         */
        uint32_t const cMappedPages = pVM->nem.s.cMappedPages;
        if (cMappedPages >= 4000)
        {
            PGMPhysNemEnumPagesByState(pVM, pVCpu, NEM_WIN_PAGE_STATE_READABLE, nemHCWinUnmapOnePageCallback, NULL);
            Log(("nemHCWinRunGC: Unmapped all; cMappedPages=%u -> %u\n", cMappedPages, pVM->nem.s.cMappedPages));
        }
# endif

        /*
         * Pending interrupts or such?  Need to check and deal with this prior
         * to the state syncing.
         */
        pVCpu->nem.s.fDesiredInterruptWindows = 0;
        if (VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_UPDATE_APIC | VMCPU_FF_INTERRUPT_PIC
                                     | VMCPU_FF_INTERRUPT_NMI  | VMCPU_FF_INTERRUPT_SMI))
        {
# ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
            /* Make sure the CPU isn't executing. */
            if (pVCpu->nem.s.fHandleAndGetFlags == VID_MSHAGN_F_GET_NEXT_MESSAGE)
            {
                pVCpu->nem.s.fHandleAndGetFlags = 0;
                rcStrict = nemHCWinStopCpu(pVM, pVCpu, rcStrict, pMappingHeader);
                if (rcStrict == VINF_SUCCESS)
                { /* likely */ }
                else
                {
                    LogFlow(("NEM/%u: breaking: nemHCWinStopCpu -> %Rrc\n", pVCpu->idCpu, VBOXSTRICTRC_VAL(rcStrict) ));
                    STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatBreakOnStatus);
                    break;
                }
            }
# endif

            /* Try inject interrupt. */
            rcStrict = nemHCWinHandleInterruptFF(pVM, pVCpu, &pVCpu->nem.s.fDesiredInterruptWindows);
            if (rcStrict == VINF_SUCCESS)
            { /* likely */ }
            else
            {
                LogFlow(("NEM/%u: breaking: nemHCWinHandleInterruptFF -> %Rrc\n", pVCpu->idCpu, VBOXSTRICTRC_VAL(rcStrict) ));
                STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatBreakOnStatus);
                break;
            }
        }

        /*
         * Ensure that hyper-V has the whole state.
         * (We always update the interrupt windows settings when active as hyper-V seems
         * to forget about it after an exit.)
         */
        if (      (pVCpu->cpum.GstCtx.fExtrn & (CPUMCTX_EXTRN_ALL | CPUMCTX_EXTRN_NEM_WIN_MASK))
               !=                              (CPUMCTX_EXTRN_ALL | CPUMCTX_EXTRN_NEM_WIN_MASK)
            || (  (   pVCpu->nem.s.fDesiredInterruptWindows
                   || pVCpu->nem.s.fCurrentInterruptWindows != pVCpu->nem.s.fDesiredInterruptWindows)
# ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
                && pVCpu->nem.s.fHandleAndGetFlags != VID_MSHAGN_F_GET_NEXT_MESSAGE /* not running */
# endif
               )
           )
        {
# ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
            AssertMsg(pVCpu->nem.s.fHandleAndGetFlags != VID_MSHAGN_F_GET_NEXT_MESSAGE /* not running */,
                      ("%#x fExtrn=%#RX64 (%#RX64) fDesiredInterruptWindows=%d fCurrentInterruptWindows=%#x vs %#x\n",
                       pVCpu->nem.s.fHandleAndGetFlags, pVCpu->cpum.GstCtx.fExtrn, ~pVCpu->cpum.GstCtx.fExtrn & (CPUMCTX_EXTRN_ALL | CPUMCTX_EXTRN_NEM_WIN_MASK),
                       pVCpu->nem.s.fDesiredInterruptWindows, pVCpu->nem.s.fCurrentInterruptWindows, pVCpu->nem.s.fDesiredInterruptWindows));
# endif
# ifdef IN_RING0
            int rc2 = nemR0WinExportState(pVM, pVCpu, &pVCpu->cpum.GstCtx);
# else
            int rc2 = nemHCWinCopyStateToHyperV(pVM, pVCpu);
# endif
            AssertRCReturn(rc2, rc2);
        }

        /*
         * Poll timers and run for a bit.
         *
         * With the VID approach (ring-0 or ring-3) we can specify a timeout here,
         * so we take the time of the next timer event and uses that as a deadline.
         * The rounding heuristics are "tuned" so that rhel5 (1K timer) will boot fine.
         */
        /** @todo See if we cannot optimize this TMTimerPollGIP by only redoing
         *        the whole polling job when timers have changed... */
        uint64_t       offDeltaIgnored;
        uint64_t const nsNextTimerEvt = TMTimerPollGIP(pVM, pVCpu, &offDeltaIgnored); NOREF(nsNextTimerEvt);
        if (   !VM_FF_IS_ANY_SET(pVM, VM_FF_EMT_RENDEZVOUS | VM_FF_TM_VIRTUAL_SYNC)
            && !VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_HM_TO_R3_MASK))
        {
# ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
            if (pVCpu->nem.s.fHandleAndGetFlags)
            { /* Very likely that the CPU does NOT need starting (pending msg, running). */ }
            else
            {
#  ifdef IN_RING0
                pVCpu->nem.s.uIoCtlBuf.idCpu = pVCpu->idCpu;
                NTSTATUS rcNt = nemR0NtPerformIoControl(pVM, pVCpu, pVM->nemr0.s.IoCtlStartVirtualProcessor.uFunction,
                                                        &pVCpu->nem.s.uIoCtlBuf.idCpu, sizeof(pVCpu->nem.s.uIoCtlBuf.idCpu),
                                                        NULL, 0);
                LogFlow(("NEM/%u: IoCtlStartVirtualProcessor -> %#x\n", pVCpu->idCpu, rcNt));
                AssertLogRelMsgReturn(NT_SUCCESS(rcNt), ("VidStartVirtualProcessor failed for CPU #%u: %#x\n", pVCpu->idCpu, rcNt),
                                      VERR_NEM_IPE_5);
#  else
                AssertLogRelMsgReturn(g_pfnVidStartVirtualProcessor(pVM->nem.s.hPartitionDevice, pVCpu->idCpu),
                                      ("VidStartVirtualProcessor failed for CPU #%u: %u (%#x, rcNt=%#x)\n",
                                       pVCpu->idCpu, RTNtLastErrorValue(), RTNtLastErrorValue(), RTNtLastStatusValue()),
                                      VERR_NEM_IPE_5);
#  endif
                pVCpu->nem.s.fHandleAndGetFlags = VID_MSHAGN_F_GET_NEXT_MESSAGE;
            }
# endif /* NEM_WIN_TEMPLATE_MODE_OWN_RUN_API */

            if (VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC_NEM_WAIT, VMCPUSTATE_STARTED_EXEC_NEM))
            {
# ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
                uint64_t const  nsNow           = RTTimeNanoTS();
                int64_t const   cNsNextTimerEvt = nsNow - nsNextTimerEvt;
                uint32_t        cMsWait;
                if (cNsNextTimerEvt < 100000 /* ns */)
                    cMsWait = 0;
                else if ((uint64_t)cNsNextTimerEvt < RT_NS_1SEC)
                {
                    if ((uint32_t)cNsNextTimerEvt < 2*RT_NS_1MS)
                        cMsWait = 1;
                    else
                        cMsWait = ((uint32_t)cNsNextTimerEvt - 100000 /*ns*/) / RT_NS_1MS;
                }
                else
                    cMsWait = RT_MS_1SEC;
#  ifdef IN_RING0
                pVCpu->nem.s.uIoCtlBuf.MsgSlotHandleAndGetNext.iCpu     = pVCpu->idCpu;
                pVCpu->nem.s.uIoCtlBuf.MsgSlotHandleAndGetNext.fFlags   = pVCpu->nem.s.fHandleAndGetFlags;
                pVCpu->nem.s.uIoCtlBuf.MsgSlotHandleAndGetNext.cMillies = cMsWait;
                NTSTATUS rcNt = nemR0NtPerformIoControl(pVM, pVCpu, pVM->nemr0.s.IoCtlMessageSlotHandleAndGetNext.uFunction,
                                                        &pVCpu->nem.s.uIoCtlBuf.MsgSlotHandleAndGetNext,
                                                        pVM->nemr0.s.IoCtlMessageSlotHandleAndGetNext.cbInput,
                                                        NULL, 0);
                VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC_NEM, VMCPUSTATE_STARTED_EXEC_NEM_WAIT);
                if (rcNt == STATUS_SUCCESS)
#  else
                BOOL fRet = VidMessageSlotHandleAndGetNext(pVM->nem.s.hPartitionDevice, pVCpu->idCpu,
                                                           pVCpu->nem.s.fHandleAndGetFlags, cMsWait);
                VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC_NEM, VMCPUSTATE_STARTED_EXEC_NEM_WAIT);
                if (fRet)
#  endif
# else
                WHV_RUN_VP_EXIT_CONTEXT ExitReason;
                RT_ZERO(ExitReason);
                HRESULT hrc = WHvRunVirtualProcessor(pVM->nem.s.hPartition, pVCpu->idCpu, &ExitReason, sizeof(ExitReason));
                VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC_NEM, VMCPUSTATE_STARTED_EXEC_NEM_WAIT);
                if (SUCCEEDED(hrc))
# endif
                {
                    /*
                     * Deal with the message.
                     */
# ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
                    rcStrict = nemHCWinHandleMessage(pVM, pVCpu, pMappingHeader);
                    pVCpu->nem.s.fHandleAndGetFlags |= VID_MSHAGN_F_HANDLE_MESSAGE;
# else
                    rcStrict = nemR3WinHandleExit(pVM, pVCpu, &ExitReason);
# endif
                    if (rcStrict == VINF_SUCCESS)
                    { /* hopefully likely */ }
                    else
                    {
                        LogFlow(("NEM/%u: breaking: nemHCWinHandleMessage -> %Rrc\n", pVCpu->idCpu, VBOXSTRICTRC_VAL(rcStrict) ));
                        STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatBreakOnStatus);
                        break;
                    }
                }
                else
                {
# ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API

                    /* VID.SYS merges STATUS_ALERTED and STATUS_USER_APC into STATUS_TIMEOUT,
                       so after NtAlertThread we end up here with a STATUS_TIMEOUT.  And yeah,
                       the error code conversion is into WAIT_XXX, i.e. NT status codes. */
#  ifndef IN_RING0
                    DWORD rcNt = GetLastError();
#  endif
                    LogFlow(("NEM/%u: VidMessageSlotHandleAndGetNext -> %#x\n", pVCpu->idCpu, rcNt));
                    AssertLogRelMsgReturn(   rcNt == STATUS_TIMEOUT
                                          || rcNt == STATUS_ALERTED    /* just in case */
                                          || rcNt == STATUS_USER_APC   /* ditto */
                                          || rcNt == STATUS_KERNEL_APC /* ditto */
                                          , ("VidMessageSlotHandleAndGetNext failed for CPU #%u: %#x (%u)\n",
                                             pVCpu->idCpu, rcNt, rcNt),
                                          VERR_NEM_IPE_0);
                    pVCpu->nem.s.fHandleAndGetFlags = VID_MSHAGN_F_GET_NEXT_MESSAGE;
                    STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatGetMsgTimeout);
# else
                    AssertLogRelMsgFailedReturn(("WHvRunVirtualProcessor failed for CPU #%u: %#x (%u)\n",
                                                 pVCpu->idCpu, hrc, GetLastError()),
                                                VERR_NEM_IPE_0);
# endif
                }

                /*
                 * If no relevant FFs are pending, loop.
                 */
                if (   !VM_FF_IS_ANY_SET(   pVM,   !fSingleStepping ? VM_FF_HP_R0_PRE_HM_MASK    : VM_FF_HP_R0_PRE_HM_STEP_MASK)
                    && !VMCPU_FF_IS_ANY_SET(pVCpu, !fSingleStepping ? VMCPU_FF_HP_R0_PRE_HM_MASK : VMCPU_FF_HP_R0_PRE_HM_STEP_MASK) )
                    continue;

                /** @todo Try handle pending flags, not just return to EM loops.  Take care
                 *        not to set important RCs here unless we've handled a message. */
                LogFlow(("NEM/%u: breaking: pending FF (%#x / %#RX64)\n",
                         pVCpu->idCpu, pVM->fGlobalForcedActions, (uint64_t)pVCpu->fLocalForcedActions));
                STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatBreakOnFFPost);
            }
            else
            {
                LogFlow(("NEM/%u: breaking: canceled %d (pre exec)\n", pVCpu->idCpu, VMCPU_GET_STATE(pVCpu) ));
                STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatBreakOnCancel);
            }
        }
        else
        {
            LogFlow(("NEM/%u: breaking: pending FF (pre exec)\n", pVCpu->idCpu));
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatBreakOnFFPre);
        }
        break;
    } /* the run loop */


    /*
     * If the CPU is running, make sure to stop it before we try sync back the
     * state and return to EM.  We don't sync back the whole state if we can help it.
     */
# ifdef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
    if (pVCpu->nem.s.fHandleAndGetFlags == VID_MSHAGN_F_GET_NEXT_MESSAGE)
    {
        pVCpu->nem.s.fHandleAndGetFlags = 0;
        rcStrict = nemHCWinStopCpu(pVM, pVCpu, rcStrict, pMappingHeader);
    }
# endif

    if (!VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED, VMCPUSTATE_STARTED_EXEC_NEM))
        VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED, VMCPUSTATE_STARTED_EXEC_NEM_CANCELED);

    if (pVCpu->cpum.GstCtx.fExtrn & (CPUMCTX_EXTRN_ALL | (CPUMCTX_EXTRN_NEM_WIN_MASK & ~CPUMCTX_EXTRN_NEM_WIN_EVENT_INJECT)))
    {
        /* Try anticipate what we might need. */
        uint64_t fImport = IEM_CPUMCTX_EXTRN_MUST_MASK | CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT | CPUMCTX_EXTRN_NEM_WIN_INHIBIT_NMI;
        if (   (rcStrict >= VINF_EM_FIRST && rcStrict <= VINF_EM_LAST)
            || RT_FAILURE(rcStrict))
            fImport = CPUMCTX_EXTRN_ALL | (CPUMCTX_EXTRN_NEM_WIN_MASK & ~CPUMCTX_EXTRN_NEM_WIN_EVENT_INJECT);
# ifdef IN_RING0 /* Ring-3 I/O port access optimizations: */
        else if (   rcStrict == VINF_IOM_R3_IOPORT_COMMIT_WRITE
                 || rcStrict == VINF_EM_PENDING_R3_IOPORT_WRITE)
            fImport = CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_CS | CPUMCTX_EXTRN_RFLAGS | CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT;
        else if (rcStrict == VINF_EM_PENDING_R3_IOPORT_READ)
            fImport = CPUMCTX_EXTRN_RAX | CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_CS | CPUMCTX_EXTRN_RFLAGS | CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT;
# endif
        else if (VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_INTERRUPT_PIC | VMCPU_FF_INTERRUPT_APIC
                                          | VMCPU_FF_INTERRUPT_NMI | VMCPU_FF_INTERRUPT_SMI))
            fImport |= IEM_CPUMCTX_EXTRN_XCPT_MASK;

        if (pVCpu->cpum.GstCtx.fExtrn & fImport)
        {
# ifdef IN_RING0
            int rc2 = nemR0WinImportState(pVM, pVCpu, &pVCpu->cpum.GstCtx, fImport | CPUMCTX_EXTRN_NEM_WIN_EVENT_INJECT,
                                          true /*fCanUpdateCr3*/);
            if (RT_SUCCESS(rc2))
                pVCpu->cpum.GstCtx.fExtrn &= ~fImport;
            else if (rc2 == VERR_NEM_FLUSH_TLB)
            {
                pVCpu->cpum.GstCtx.fExtrn &= ~fImport;
                if (rcStrict == VINF_SUCCESS || rcStrict == -rc2)
                    rcStrict = -rc2;
                else
                {
                    pVCpu->nem.s.rcPending = -rc2;
                    LogFlow(("NEM/%u: rcPending=%Rrc (rcStrict=%Rrc)\n", pVCpu->idCpu, rc2, VBOXSTRICTRC_VAL(rcStrict) ));
                }
            }
# else
            int rc2 = nemHCWinCopyStateFromHyperV(pVM, pVCpu, fImport | CPUMCTX_EXTRN_NEM_WIN_EVENT_INJECT);
            if (RT_SUCCESS(rc2))
                pVCpu->cpum.GstCtx.fExtrn &= ~fImport;
# endif
            else if (RT_SUCCESS(rcStrict))
                rcStrict = rc2;
            if (!(pVCpu->cpum.GstCtx.fExtrn & (CPUMCTX_EXTRN_ALL | (CPUMCTX_EXTRN_NEM_WIN_MASK & ~CPUMCTX_EXTRN_NEM_WIN_EVENT_INJECT))))
                pVCpu->cpum.GstCtx.fExtrn = 0;
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatImportOnReturn);
        }
        else
        {
            STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatImportOnReturnSkipped);
            pVCpu->cpum.GstCtx.fExtrn &= ~CPUMCTX_EXTRN_NEM_WIN_EVENT_INJECT;
        }
    }
    else
    {
        STAM_REL_COUNTER_INC(&pVCpu->nem.s.StatImportOnReturnSkipped);
        pVCpu->cpum.GstCtx.fExtrn = 0;
    }

    LogFlow(("NEM/%u: %04x:%08RX64 efl=%#08RX64 => %Rrc\n",
             pVCpu->idCpu, pVCpu->cpum.GstCtx.cs.Sel, pVCpu->cpum.GstCtx.rip, pVCpu->cpum.GstCtx.rflags, VBOXSTRICTRC_VAL(rcStrict) ));
    return rcStrict;
}

#endif /* defined(NEM_WIN_TEMPLATE_MODE_OWN_RUN_API) || defined(IN_RING3) */

/**
 * @callback_method_impl{FNPGMPHYSNEMCHECKPAGE}
 */
NEM_TMPL_STATIC DECLCALLBACK(int) nemHCWinUnsetForA20CheckerCallback(PVMCC pVM, PVMCPUCC pVCpu, RTGCPHYS GCPhys,
                                                                     PPGMPHYSNEMPAGEINFO pInfo, void *pvUser)
{
    /* We'll just unmap the memory. */
    if (pInfo->u2NemState > NEM_WIN_PAGE_STATE_UNMAPPED)
    {
#ifdef NEM_WIN_USE_HYPERCALLS_FOR_PAGES
        int rc = nemHCWinHypercallUnmapPage(pVM, pVCpu, GCPhys);
        AssertRC(rc);
        if (RT_SUCCESS(rc))
#else
        HRESULT hrc = WHvUnmapGpaRange(pVM->nem.s.hPartition, GCPhys, X86_PAGE_SIZE);
        if (SUCCEEDED(hrc))
#endif
        {
            uint32_t cMappedPages = ASMAtomicDecU32(&pVM->nem.s.cMappedPages); NOREF(cMappedPages);
            Log5(("NEM GPA unmapped/A20: %RGp (was %s, cMappedPages=%u)\n", GCPhys, g_apszPageStates[pInfo->u2NemState], cMappedPages));
            pInfo->u2NemState = NEM_WIN_PAGE_STATE_UNMAPPED;
        }
        else
        {
#ifdef NEM_WIN_USE_HYPERCALLS_FOR_PAGES
            LogRel(("nemHCWinUnsetForA20CheckerCallback/unmap: GCPhys=%RGp rc=%Rrc\n", GCPhys, rc));
            return rc;
#else
            LogRel(("nemHCWinUnsetForA20CheckerCallback/unmap: GCPhys=%RGp hrc=%Rhrc (%#x) Last=%#x/%u\n",
                    GCPhys, hrc, hrc, RTNtLastStatusValue(), RTNtLastErrorValue()));
            return VERR_NEM_IPE_2;
#endif
        }
    }
    RT_NOREF(pVCpu, pvUser);
    return VINF_SUCCESS;
}


/**
 * Unmaps a page from Hyper-V for the purpose of emulating A20 gate behavior.
 *
 * @returns The PGMPhysNemQueryPageInfo result.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   GCPhys          The page to unmap.
 */
NEM_TMPL_STATIC int nemHCWinUnmapPageForA20Gate(PVMCC pVM, PVMCPUCC pVCpu, RTGCPHYS GCPhys)
{
    PGMPHYSNEMPAGEINFO Info;
    return PGMPhysNemPageInfoChecker(pVM, pVCpu, GCPhys, false /*fMakeWritable*/, &Info,
                                     nemHCWinUnsetForA20CheckerCallback, NULL);
}


void nemHCNativeNotifyHandlerPhysicalRegister(PVMCC pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb)
{
    Log5(("nemHCNativeNotifyHandlerPhysicalRegister: %RGp LB %RGp enmKind=%d\n", GCPhys, cb, enmKind));
    NOREF(pVM); NOREF(enmKind); NOREF(GCPhys); NOREF(cb);
}


void nemHCNativeNotifyHandlerPhysicalDeregister(PVMCC pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhys, RTGCPHYS cb,
                                                int fRestoreAsRAM, bool fRestoreAsRAM2)
{
    Log5(("nemHCNativeNotifyHandlerPhysicalDeregister: %RGp LB %RGp enmKind=%d fRestoreAsRAM=%d fRestoreAsRAM2=%d\n",
          GCPhys, cb, enmKind, fRestoreAsRAM, fRestoreAsRAM2));
    NOREF(pVM); NOREF(enmKind); NOREF(GCPhys); NOREF(cb); NOREF(fRestoreAsRAM); NOREF(fRestoreAsRAM2);
}


void nemHCNativeNotifyHandlerPhysicalModify(PVMCC pVM, PGMPHYSHANDLERKIND enmKind, RTGCPHYS GCPhysOld,
                                            RTGCPHYS GCPhysNew, RTGCPHYS cb, bool fRestoreAsRAM)
{
    Log5(("nemHCNativeNotifyHandlerPhysicalModify: %RGp LB %RGp -> %RGp enmKind=%d fRestoreAsRAM=%d\n",
          GCPhysOld, cb, GCPhysNew, enmKind, fRestoreAsRAM));
    NOREF(pVM); NOREF(enmKind); NOREF(GCPhysOld); NOREF(GCPhysNew); NOREF(cb); NOREF(fRestoreAsRAM);
}


/**
 * Worker that maps pages into Hyper-V.
 *
 * This is used by the PGM physical page notifications as well as the memory
 * access VMEXIT handlers.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure of the
 *                          calling EMT.
 * @param   GCPhysSrc       The source page address.
 * @param   GCPhysDst       The hyper-V destination page.  This may differ from
 *                          GCPhysSrc when A20 is disabled.
 * @param   fPageProt       NEM_PAGE_PROT_XXX.
 * @param   pu2State        Our page state (input/output).
 * @param   fBackingChanged Set if the page backing is being changed.
 * @thread  EMT(pVCpu)
 */
NEM_TMPL_STATIC int nemHCNativeSetPhysPage(PVMCC pVM, PVMCPUCC pVCpu, RTGCPHYS GCPhysSrc, RTGCPHYS GCPhysDst,
                                           uint32_t fPageProt, uint8_t *pu2State, bool fBackingChanged)
{
#ifdef NEM_WIN_USE_HYPERCALLS_FOR_PAGES
    /*
     * When using the hypercalls instead of the ring-3 APIs, we don't need to
     * unmap memory before modifying it.  We still want to track the state though,
     * since unmap will fail when called an unmapped page and we don't want to redo
     * upgrades/downgrades.
     */
    uint8_t const u2OldState = *pu2State;
    int rc;
    if (fPageProt == NEM_PAGE_PROT_NONE)
    {
        if (u2OldState > NEM_WIN_PAGE_STATE_UNMAPPED)
        {
            rc = nemHCWinHypercallUnmapPage(pVM, pVCpu, GCPhysDst);
            if (RT_SUCCESS(rc))
            {
                *pu2State = NEM_WIN_PAGE_STATE_UNMAPPED;
                uint32_t cMappedPages = ASMAtomicDecU32(&pVM->nem.s.cMappedPages); NOREF(cMappedPages);
                Log5(("NEM GPA unmapped/set: %RGp (was %s, cMappedPages=%u)\n", GCPhysDst, g_apszPageStates[u2OldState], cMappedPages));
            }
            else
                AssertLogRelMsgFailed(("nemHCNativeSetPhysPage/unmap: GCPhysDst=%RGp rc=%Rrc\n", GCPhysDst, rc));
        }
        else
            rc = VINF_SUCCESS;
    }
    else if (fPageProt & NEM_PAGE_PROT_WRITE)
    {
        if (u2OldState != NEM_WIN_PAGE_STATE_WRITABLE || fBackingChanged)
        {
            rc = nemHCWinHypercallMapPage(pVM, pVCpu, GCPhysSrc, GCPhysDst,
                                            HV_MAP_GPA_READABLE   | HV_MAP_GPA_WRITABLE
                                          | HV_MAP_GPA_EXECUTABLE | HV_MAP_GPA_EXECUTABLE_AGAIN);
            if (RT_SUCCESS(rc))
            {
                *pu2State = NEM_WIN_PAGE_STATE_WRITABLE;
                uint32_t cMappedPages = u2OldState <= NEM_WIN_PAGE_STATE_UNMAPPED
                                      ? ASMAtomicIncU32(&pVM->nem.s.cMappedPages) : pVM->nem.s.cMappedPages;
                Log5(("NEM GPA writable/set: %RGp (was %s, cMappedPages=%u)\n", GCPhysDst, g_apszPageStates[u2OldState], cMappedPages));
                NOREF(cMappedPages);
            }
            else
                AssertLogRelMsgFailed(("nemHCNativeSetPhysPage/writable: GCPhysDst=%RGp rc=%Rrc\n", GCPhysDst, rc));
        }
        else
            rc = VINF_SUCCESS;
    }
    else
    {
        if (u2OldState != NEM_WIN_PAGE_STATE_READABLE || fBackingChanged)
        {
            rc = nemHCWinHypercallMapPage(pVM, pVCpu, GCPhysSrc, GCPhysDst,
                                          HV_MAP_GPA_READABLE | HV_MAP_GPA_EXECUTABLE | HV_MAP_GPA_EXECUTABLE_AGAIN);
            if (RT_SUCCESS(rc))
            {
                *pu2State = NEM_WIN_PAGE_STATE_READABLE;
                uint32_t cMappedPages = u2OldState <= NEM_WIN_PAGE_STATE_UNMAPPED
                                      ? ASMAtomicIncU32(&pVM->nem.s.cMappedPages) : pVM->nem.s.cMappedPages;
                Log5(("NEM GPA read+exec/set: %RGp (was %s, cMappedPages=%u)\n", GCPhysDst, g_apszPageStates[u2OldState], cMappedPages));
                NOREF(cMappedPages);
            }
            else
                AssertLogRelMsgFailed(("nemHCNativeSetPhysPage/writable: GCPhysDst=%RGp rc=%Rrc\n", GCPhysDst, rc));
        }
        else
            rc = VINF_SUCCESS;
    }

    return VINF_SUCCESS;

#else
    /*
     * Looks like we need to unmap a page before we can change the backing
     * or even modify the protection.  This is going to be *REALLY* efficient.
     * PGM lends us two bits to keep track of the state here.
     */
    uint8_t const u2OldState = *pu2State;
    uint8_t const u2NewState = fPageProt & NEM_PAGE_PROT_WRITE ? NEM_WIN_PAGE_STATE_WRITABLE
                             : fPageProt & NEM_PAGE_PROT_READ  ? NEM_WIN_PAGE_STATE_READABLE : NEM_WIN_PAGE_STATE_UNMAPPED;
    if (   fBackingChanged
        || u2NewState != u2OldState)
    {
        if (u2OldState > NEM_WIN_PAGE_STATE_UNMAPPED)
        {
# ifdef NEM_WIN_USE_HYPERCALLS_FOR_PAGES
            int rc = nemHCWinHypercallUnmapPage(pVM, pVCpu, GCPhysDst);
            AssertRC(rc);
            if (RT_SUCCESS(rc))
            {
                *pu2State = NEM_WIN_PAGE_STATE_UNMAPPED;
                uint32_t cMappedPages = ASMAtomicDecU32(&pVM->nem.s.cMappedPages); NOREF(cMappedPages);
                if (u2NewState == NEM_WIN_PAGE_STATE_UNMAPPED)
                {
                    Log5(("NEM GPA unmapped/set: %RGp (was %s, cMappedPages=%u)\n",
                          GCPhysDst, g_apszPageStates[u2OldState], cMappedPages));
                    return VINF_SUCCESS;
                }
            }
            else
            {
                LogRel(("nemHCNativeSetPhysPage/unmap: GCPhysDst=%RGp rc=%Rrc\n", GCPhysDst, rc));
                return rc;
            }
# else
            HRESULT hrc = WHvUnmapGpaRange(pVM->nem.s.hPartition, GCPhysDst, X86_PAGE_SIZE);
            if (SUCCEEDED(hrc))
            {
                *pu2State = NEM_WIN_PAGE_STATE_UNMAPPED;
                uint32_t cMappedPages = ASMAtomicDecU32(&pVM->nem.s.cMappedPages); NOREF(cMappedPages);
                if (u2NewState == NEM_WIN_PAGE_STATE_UNMAPPED)
                {
                    Log5(("NEM GPA unmapped/set: %RGp (was %s, cMappedPages=%u)\n",
                          GCPhysDst, g_apszPageStates[u2OldState], cMappedPages));
                    return VINF_SUCCESS;
                }
            }
            else
            {
                LogRel(("nemHCNativeSetPhysPage/unmap: GCPhysDst=%RGp hrc=%Rhrc (%#x) Last=%#x/%u\n",
                        GCPhysDst, hrc, hrc, RTNtLastStatusValue(), RTNtLastErrorValue()));
                return VERR_NEM_INIT_FAILED;
            }
# endif
        }
    }

    /*
     * Writeable mapping?
     */
    if (fPageProt & NEM_PAGE_PROT_WRITE)
    {
# ifdef NEM_WIN_USE_HYPERCALLS_FOR_PAGES
        int rc = nemHCWinHypercallMapPage(pVM, pVCpu, GCPhysSrc, GCPhysDst,
                                            HV_MAP_GPA_READABLE   | HV_MAP_GPA_WRITABLE
                                          | HV_MAP_GPA_EXECUTABLE | HV_MAP_GPA_EXECUTABLE_AGAIN);
        AssertRC(rc);
        if (RT_SUCCESS(rc))
        {
            *pu2State = NEM_WIN_PAGE_STATE_WRITABLE;
            uint32_t cMappedPages = ASMAtomicIncU32(&pVM->nem.s.cMappedPages); NOREF(cMappedPages);
            Log5(("NEM GPA mapped/set: %RGp %s (was %s, cMappedPages=%u)\n",
                  GCPhysDst, g_apszPageStates[u2NewState], g_apszPageStates[u2OldState], cMappedPages));
            return VINF_SUCCESS;
        }
        LogRel(("nemHCNativeSetPhysPage/writable: GCPhysDst=%RGp rc=%Rrc\n", GCPhysDst, rc));
        return rc;
# else
        void *pvPage;
        int rc = nemR3NativeGCPhys2R3PtrWriteable(pVM, GCPhysSrc, &pvPage);
        if (RT_SUCCESS(rc))
        {
            HRESULT hrc = WHvMapGpaRange(pVM->nem.s.hPartition, pvPage, GCPhysDst, X86_PAGE_SIZE,
                                         WHvMapGpaRangeFlagRead | WHvMapGpaRangeFlagExecute | WHvMapGpaRangeFlagWrite);
            if (SUCCEEDED(hrc))
            {
                *pu2State = NEM_WIN_PAGE_STATE_WRITABLE;
                uint32_t cMappedPages = ASMAtomicIncU32(&pVM->nem.s.cMappedPages); NOREF(cMappedPages);
                Log5(("NEM GPA mapped/set: %RGp %s (was %s, cMappedPages=%u)\n",
                      GCPhysDst, g_apszPageStates[u2NewState], g_apszPageStates[u2OldState], cMappedPages));
                return VINF_SUCCESS;
            }
            LogRel(("nemHCNativeSetPhysPage/writable: GCPhysDst=%RGp hrc=%Rhrc (%#x) Last=%#x/%u\n",
                    GCPhysDst, hrc, hrc, RTNtLastStatusValue(), RTNtLastErrorValue()));
            return VERR_NEM_INIT_FAILED;
        }
        LogRel(("nemHCNativeSetPhysPage/writable: GCPhysSrc=%RGp rc=%Rrc\n", GCPhysSrc, rc));
        return rc;
# endif
    }

    if (fPageProt & NEM_PAGE_PROT_READ)
    {
# ifdef NEM_WIN_USE_HYPERCALLS_FOR_PAGES
        int rc = nemHCWinHypercallMapPage(pVM, pVCpu, GCPhysSrc, GCPhysDst,
                                          HV_MAP_GPA_READABLE | HV_MAP_GPA_EXECUTABLE | HV_MAP_GPA_EXECUTABLE_AGAIN);
        AssertRC(rc);
        if (RT_SUCCESS(rc))
        {
            *pu2State = NEM_WIN_PAGE_STATE_READABLE;
            uint32_t cMappedPages = ASMAtomicIncU32(&pVM->nem.s.cMappedPages); NOREF(cMappedPages);
            Log5(("NEM GPA mapped/set: %RGp %s (was %s, cMappedPages=%u)\n",
                  GCPhysDst, g_apszPageStates[u2NewState], g_apszPageStates[u2OldState], cMappedPages));
            return VINF_SUCCESS;
        }
        LogRel(("nemHCNativeSetPhysPage/readonly: GCPhysDst=%RGp rc=%Rrc\n", GCPhysDst, rc));
        return rc;
# else
        const void *pvPage;
        int rc = nemR3NativeGCPhys2R3PtrReadOnly(pVM, GCPhysSrc, &pvPage);
        if (RT_SUCCESS(rc))
        {
            HRESULT hrc = WHvMapGpaRange(pVM->nem.s.hPartition, (void *)pvPage, GCPhysDst, X86_PAGE_SIZE,
                                         WHvMapGpaRangeFlagRead | WHvMapGpaRangeFlagExecute);
            if (SUCCEEDED(hrc))
            {
                *pu2State = NEM_WIN_PAGE_STATE_READABLE;
                uint32_t cMappedPages = ASMAtomicIncU32(&pVM->nem.s.cMappedPages); NOREF(cMappedPages);
                Log5(("NEM GPA mapped/set: %RGp %s (was %s, cMappedPages=%u)\n",
                      GCPhysDst, g_apszPageStates[u2NewState], g_apszPageStates[u2OldState], cMappedPages));
                return VINF_SUCCESS;
            }
            LogRel(("nemHCNativeSetPhysPage/readonly: GCPhysDst=%RGp hrc=%Rhrc (%#x) Last=%#x/%u\n",
                    GCPhysDst, hrc, hrc, RTNtLastStatusValue(), RTNtLastErrorValue()));
            return VERR_NEM_INIT_FAILED;
        }
        LogRel(("nemHCNativeSetPhysPage/readonly: GCPhysSrc=%RGp rc=%Rrc\n", GCPhysSrc, rc));
        return rc;
# endif
    }

    /* We already unmapped it above. */
    *pu2State = NEM_WIN_PAGE_STATE_UNMAPPED;
    return VINF_SUCCESS;
#endif /* !NEM_WIN_USE_HYPERCALLS_FOR_PAGES */
}


NEM_TMPL_STATIC int nemHCJustUnmapPageFromHyperV(PVMCC pVM, RTGCPHYS GCPhysDst, uint8_t *pu2State)
{
    if (*pu2State <= NEM_WIN_PAGE_STATE_UNMAPPED)
    {
        Log5(("nemHCJustUnmapPageFromHyperV: %RGp == unmapped\n", GCPhysDst));
        *pu2State = NEM_WIN_PAGE_STATE_UNMAPPED;
        return VINF_SUCCESS;
    }

#if defined(NEM_WIN_USE_HYPERCALLS_FOR_PAGES) || defined(IN_RING0)
    PVMCPUCC pVCpu = VMMGetCpu(pVM);
    int rc = nemHCWinHypercallUnmapPage(pVM, pVCpu, GCPhysDst);
    AssertRC(rc);
    if (RT_SUCCESS(rc))
    {
        uint32_t cMappedPages = ASMAtomicDecU32(&pVM->nem.s.cMappedPages); NOREF(cMappedPages);
        Log5(("NEM GPA unmapped/just: %RGp (was %s, cMappedPages=%u)\n", GCPhysDst, g_apszPageStates[*pu2State], cMappedPages));
        *pu2State = NEM_WIN_PAGE_STATE_UNMAPPED;
        return VINF_SUCCESS;
    }
    LogRel(("nemHCJustUnmapPageFromHyperV/unmap: GCPhysDst=%RGp rc=%Rrc\n", GCPhysDst, rc));
    return rc;
#else
    HRESULT hrc = WHvUnmapGpaRange(pVM->nem.s.hPartition, GCPhysDst & ~(RTGCPHYS)X86_PAGE_OFFSET_MASK, X86_PAGE_SIZE);
    if (SUCCEEDED(hrc))
    {
        uint32_t cMappedPages = ASMAtomicDecU32(&pVM->nem.s.cMappedPages); NOREF(cMappedPages);
        *pu2State = NEM_WIN_PAGE_STATE_UNMAPPED;
        Log5(("nemHCJustUnmapPageFromHyperV: %RGp => unmapped (total %u)\n", GCPhysDst, cMappedPages));
        return VINF_SUCCESS;
    }
    LogRel(("nemHCJustUnmapPageFromHyperV(%RGp): failed! hrc=%Rhrc (%#x) Last=%#x/%u\n",
            GCPhysDst, hrc, hrc, RTNtLastStatusValue(), RTNtLastErrorValue()));
    return VERR_NEM_IPE_6;
#endif
}


int nemHCNativeNotifyPhysPageAllocated(PVMCC pVM, RTGCPHYS GCPhys, RTHCPHYS HCPhys, uint32_t fPageProt,
                                       PGMPAGETYPE enmType, uint8_t *pu2State)
{
    Log5(("nemHCNativeNotifyPhysPageAllocated: %RGp HCPhys=%RHp fPageProt=%#x enmType=%d *pu2State=%d\n",
          GCPhys, HCPhys, fPageProt, enmType, *pu2State));
    RT_NOREF_PV(HCPhys); RT_NOREF_PV(enmType);

    int rc;
#if defined(NEM_WIN_USE_HYPERCALLS_FOR_PAGES) || defined(IN_RING0)
    PVMCPUCC pVCpu = VMMGetCpu(pVM);
    if (   pVM->nem.s.fA20Enabled
        || !NEM_WIN_IS_RELEVANT_TO_A20(GCPhys))
        rc = nemHCNativeSetPhysPage(pVM, pVCpu, GCPhys, GCPhys, fPageProt, pu2State, true /*fBackingChanged*/);
    else
    {
        /* To keep effort at a minimum, we unmap the HMA page alias and resync it lazily when needed. */
        rc = nemHCWinUnmapPageForA20Gate(pVM, pVCpu, GCPhys | RT_BIT_32(20));
        if (!NEM_WIN_IS_SUBJECT_TO_A20(GCPhys) && RT_SUCCESS(rc))
            rc = nemHCNativeSetPhysPage(pVM, pVCpu, GCPhys, GCPhys, fPageProt, pu2State, true /*fBackingChanged*/);

    }
#else
    RT_NOREF_PV(fPageProt);
    if (   pVM->nem.s.fA20Enabled
        || !NEM_WIN_IS_RELEVANT_TO_A20(GCPhys))
        rc = nemR3JustUnmapPageFromHyperV(pVM, GCPhys, pu2State);
    else if (!NEM_WIN_IS_SUBJECT_TO_A20(GCPhys))
        rc = nemR3JustUnmapPageFromHyperV(pVM, GCPhys, pu2State);
    else
        rc = VINF_SUCCESS; /* ignore since we've got the alias page at this address. */
#endif
    return rc;
}


void nemHCNativeNotifyPhysPageProtChanged(PVMCC pVM, RTGCPHYS GCPhys, RTHCPHYS HCPhys, uint32_t fPageProt,
                                          PGMPAGETYPE enmType, uint8_t *pu2State)
{
    Log5(("nemHCNativeNotifyPhysPageProtChanged: %RGp HCPhys=%RHp fPageProt=%#x enmType=%d *pu2State=%d\n",
          GCPhys, HCPhys, fPageProt, enmType, *pu2State));
    RT_NOREF_PV(HCPhys); RT_NOREF_PV(enmType);

#if defined(NEM_WIN_USE_HYPERCALLS_FOR_PAGES) || defined(IN_RING0)
    PVMCPUCC pVCpu = VMMGetCpu(pVM);
    if (   pVM->nem.s.fA20Enabled
        || !NEM_WIN_IS_RELEVANT_TO_A20(GCPhys))
        nemHCNativeSetPhysPage(pVM, pVCpu, GCPhys, GCPhys, fPageProt, pu2State, false /*fBackingChanged*/);
    else
    {
        /* To keep effort at a minimum, we unmap the HMA page alias and resync it lazily when needed. */
        nemHCWinUnmapPageForA20Gate(pVM, pVCpu, GCPhys | RT_BIT_32(20));
        if (!NEM_WIN_IS_SUBJECT_TO_A20(GCPhys))
            nemHCNativeSetPhysPage(pVM, pVCpu, GCPhys, GCPhys, fPageProt, pu2State, false /*fBackingChanged*/);
    }
#else
    RT_NOREF_PV(fPageProt);
    if (   pVM->nem.s.fA20Enabled
        || !NEM_WIN_IS_RELEVANT_TO_A20(GCPhys))
        nemR3JustUnmapPageFromHyperV(pVM, GCPhys, pu2State);
    else if (!NEM_WIN_IS_SUBJECT_TO_A20(GCPhys))
        nemR3JustUnmapPageFromHyperV(pVM, GCPhys, pu2State);
    /* else: ignore since we've got the alias page at this address. */
#endif
}


void nemHCNativeNotifyPhysPageChanged(PVMCC pVM, RTGCPHYS GCPhys, RTHCPHYS HCPhysPrev, RTHCPHYS HCPhysNew,
                                     uint32_t fPageProt, PGMPAGETYPE enmType, uint8_t *pu2State)
{
    Log5(("nemHCNativeNotifyPhysPageChanged: %RGp HCPhys=%RHp->%RHp fPageProt=%#x enmType=%d *pu2State=%d\n",
          GCPhys, HCPhysPrev, HCPhysNew, fPageProt, enmType, *pu2State));
    RT_NOREF_PV(HCPhysPrev); RT_NOREF_PV(HCPhysNew); RT_NOREF_PV(enmType);

#if defined(NEM_WIN_USE_HYPERCALLS_FOR_PAGES) || defined(IN_RING0)
    PVMCPUCC pVCpu = VMMGetCpu(pVM);
    if (   pVM->nem.s.fA20Enabled
        || !NEM_WIN_IS_RELEVANT_TO_A20(GCPhys))
        nemHCNativeSetPhysPage(pVM, pVCpu, GCPhys, GCPhys, fPageProt, pu2State, true /*fBackingChanged*/);
    else
    {
        /* To keep effort at a minimum, we unmap the HMA page alias and resync it lazily when needed. */
        nemHCWinUnmapPageForA20Gate(pVM, pVCpu, GCPhys | RT_BIT_32(20));
        if (!NEM_WIN_IS_SUBJECT_TO_A20(GCPhys))
            nemHCNativeSetPhysPage(pVM, pVCpu, GCPhys, GCPhys, fPageProt, pu2State, true /*fBackingChanged*/);
    }
#else
    RT_NOREF_PV(fPageProt);
    if (   pVM->nem.s.fA20Enabled
        || !NEM_WIN_IS_RELEVANT_TO_A20(GCPhys))
        nemR3JustUnmapPageFromHyperV(pVM, GCPhys, pu2State);
    else if (!NEM_WIN_IS_SUBJECT_TO_A20(GCPhys))
        nemR3JustUnmapPageFromHyperV(pVM, GCPhys, pu2State);
    /* else: ignore since we've got the alias page at this address. */
#endif
}


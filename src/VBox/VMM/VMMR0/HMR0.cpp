/* $Id: HMR0.cpp 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * Hardware Assisted Virtualization Manager (HM) - Host Context Ring-0.
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
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
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_HM
#define VMCPU_INCL_CPUM_GST_CTX
#include <VBox/vmm/hm.h>
#include <VBox/vmm/pgm.h>
#include "HMInternal.h"
#include <VBox/vmm/vmcc.h>
#include <VBox/vmm/hm_svm.h>
#include <VBox/vmm/hmvmxinline.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/asm-amd64-x86.h>
#include <iprt/cpuset.h>
#include <iprt/mem.h>
#include <iprt/memobj.h>
#include <iprt/once.h>
#include <iprt/param.h>
#include <iprt/power.h>
#include <iprt/string.h>
#include <iprt/thread.h>
#include <iprt/x86.h>
#include "HMVMXR0.h"
#include "HMSVMR0.h"


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static DECLCALLBACK(void) hmR0EnableCpuCallback(RTCPUID idCpu, void *pvUser1, void *pvUser2);
static DECLCALLBACK(void) hmR0DisableCpuCallback(RTCPUID idCpu, void *pvUser1, void *pvUser2);
static DECLCALLBACK(void) hmR0InitIntelCpu(RTCPUID idCpu, void *pvUser1, void *pvUser2);
static DECLCALLBACK(void) hmR0InitAmdCpu(RTCPUID idCpu, void *pvUser1, void *pvUser2);
static DECLCALLBACK(void) hmR0PowerCallback(RTPOWEREVENT enmEvent, void *pvUser);
static DECLCALLBACK(void) hmR0MpEventCallback(RTMPEVENT enmEvent, RTCPUID idCpu, void *pvData);


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * This is used to manage the status code of a RTMpOnAll in HM.
 */
typedef struct HMR0FIRSTRC
{
    /** The status code. */
    int32_t volatile    rc;
    /** The ID of the CPU reporting the first failure. */
    RTCPUID volatile    idCpu;
} HMR0FIRSTRC;
/** Pointer to a first return code structure. */
typedef HMR0FIRSTRC *PHMR0FIRSTRC;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/**
 * Global data.
 */
static struct
{
    /** Per CPU globals. */
    HMPHYSCPU                       aCpuInfo[RTCPUSET_MAX_CPUS];

    /** @name Ring-0 method table for AMD-V and VT-x specific operations.
     * @{ */
    DECLR0CALLBACKMEMBER(int,          pfnEnterSession, (PVMCPUCC pVCpu));
    DECLR0CALLBACKMEMBER(void,         pfnThreadCtxCallback, (RTTHREADCTXEVENT enmEvent, PVMCPUCC pVCpu, bool fGlobalInit));
    DECLR0CALLBACKMEMBER(int,          pfnCallRing3Callback, (PVMCPUCC pVCpu, VMMCALLRING3 enmOperation));
    DECLR0CALLBACKMEMBER(int,          pfnExportHostState, (PVMCPUCC pVCpu));
    DECLR0CALLBACKMEMBER(VBOXSTRICTRC, pfnRunGuestCode, (PVMCPUCC pVCpu));
    DECLR0CALLBACKMEMBER(int,          pfnEnableCpu, (PHMPHYSCPU pHostCpu, PVMCC pVM, void *pvCpuPage, RTHCPHYS HCPhysCpuPage,
                                                      bool fEnabledByHost, PCSUPHWVIRTMSRS pHwvirtMsrs));
    DECLR0CALLBACKMEMBER(int,          pfnDisableCpu, (PHMPHYSCPU pHostCpu, void *pvCpuPage, RTHCPHYS HCPhysCpuPage));
    DECLR0CALLBACKMEMBER(int,          pfnInitVM, (PVMCC pVM));
    DECLR0CALLBACKMEMBER(int,          pfnTermVM, (PVMCC pVM));
    DECLR0CALLBACKMEMBER(int,          pfnSetupVM, (PVMCC pVM));
    /** @} */

    /** Hardware-virtualization data. */
    struct
    {
        union
        {
            /** VT-x data. */
            struct
            {
                /** Host CR4 value (set by ring-0 VMX init) */
                uint64_t                    u64HostCr4;
                /** Host EFER value (set by ring-0 VMX init) */
                uint64_t                    u64HostMsrEfer;
                /** Host SMM monitor control (used for logging/diagnostics) */
                uint64_t                    u64HostSmmMonitorCtl;
                /** Last instruction error. */
                uint32_t                    ulLastInstrError;
                /** The shift mask employed by the VMX-Preemption timer. */
                uint8_t                     cPreemptTimerShift;
                /** Padding. */
                uint8_t                     abPadding[3];
                /** Whether we're using the preemption timer or not. */
                bool                        fUsePreemptTimer;
                /** Whether we're using SUPR0EnableVTx or not. */
                bool                        fUsingSUPR0EnableVTx;
                /** Set if we've called SUPR0EnableVTx(true) and should disable it during
                 * module termination. */
                bool                        fCalledSUPR0EnableVTx;
                /** Set to by us to indicate VMX is supported by the CPU. */
                bool                        fSupported;
            } vmx;

            /** AMD-V data. */
            struct
            {
                /** SVM revision. */
                uint32_t                    u32Rev;
                /** SVM feature bits from cpuid 0x8000000a */
                uint32_t                    u32Features;
                /** Padding. */
                bool                        afPadding[3];
                /** Set by us to indicate SVM is supported by the CPU. */
                bool                        fSupported;
            } svm;
        } u;
        /** Maximum allowed ASID/VPID (inclusive). */
        uint32_t                    uMaxAsid;
        /** MSRs. */
        SUPHWVIRTMSRS               Msrs;
    } hwvirt;

    /** Last recorded error code during HM ring-0 init. */
    int32_t                         rcInit;

    /** If set, VT-x/AMD-V is enabled globally at init time, otherwise it's
     * enabled and disabled each time it's used to execute guest code. */
    bool                            fGlobalInit;
    /** Indicates whether the host is suspending or not.  We'll refuse a few
     *  actions when the host is being suspended to speed up the suspending and
     *  avoid trouble. */
    bool volatile                   fSuspended;

    /** Whether we've already initialized all CPUs.
     * @remarks We could check the EnableAllCpusOnce state, but this is
     *          simpler and hopefully easier to understand. */
    bool                            fEnabled;
    /** Serialize initialization in HMR0EnableAllCpus. */
    RTONCE                          EnableAllCpusOnce;
} g_HmR0;


/**
 * Initializes a first return code structure.
 *
 * @param   pFirstRc            The structure to init.
 */
static void hmR0FirstRcInit(PHMR0FIRSTRC pFirstRc)
{
    pFirstRc->rc    = VINF_SUCCESS;
    pFirstRc->idCpu = NIL_RTCPUID;
}


/**
 * Try set the status code (success ignored).
 *
 * @param   pFirstRc            The first return code structure.
 * @param   rc                  The status code.
 */
static void hmR0FirstRcSetStatus(PHMR0FIRSTRC pFirstRc, int rc)
{
    if (   RT_FAILURE(rc)
        && ASMAtomicCmpXchgS32(&pFirstRc->rc, rc, VINF_SUCCESS))
        pFirstRc->idCpu = RTMpCpuId();
}


/**
 * Get the status code of a first return code structure.
 *
 * @returns The status code; VINF_SUCCESS or error status, no informational or
 *          warning errors.
 * @param   pFirstRc            The first return code structure.
 */
static int hmR0FirstRcGetStatus(PHMR0FIRSTRC pFirstRc)
{
    return pFirstRc->rc;
}


#ifdef VBOX_STRICT
# ifndef DEBUG_bird
/**
 * Get the CPU ID on which the failure status code was reported.
 *
 * @returns The CPU ID, NIL_RTCPUID if no failure was reported.
 * @param   pFirstRc            The first return code structure.
 */
static RTCPUID hmR0FirstRcGetCpuId(PHMR0FIRSTRC pFirstRc)
{
    return pFirstRc->idCpu;
}
# endif
#endif /* VBOX_STRICT */


/** @name Dummy callback handlers.
 * @{ */

static DECLCALLBACK(int) hmR0DummyEnter(PVMCPUCC pVCpu)
{
    RT_NOREF1(pVCpu);
    return VINF_SUCCESS;
}

static DECLCALLBACK(void) hmR0DummyThreadCtxCallback(RTTHREADCTXEVENT enmEvent, PVMCPUCC pVCpu, bool fGlobalInit)
{
    RT_NOREF3(enmEvent, pVCpu, fGlobalInit);
}

static DECLCALLBACK(int) hmR0DummyEnableCpu(PHMPHYSCPU pHostCpu, PVMCC pVM, void *pvCpuPage, RTHCPHYS HCPhysCpuPage,
                                            bool fEnabledBySystem, PCSUPHWVIRTMSRS pHwvirtMsrs)
{
    RT_NOREF6(pHostCpu, pVM, pvCpuPage, HCPhysCpuPage, fEnabledBySystem, pHwvirtMsrs);
    return VINF_SUCCESS;
}

static DECLCALLBACK(int) hmR0DummyDisableCpu(PHMPHYSCPU pHostCpu, void *pvCpuPage, RTHCPHYS HCPhysCpuPage)
{
    RT_NOREF3(pHostCpu, pvCpuPage, HCPhysCpuPage);
    return VINF_SUCCESS;
}

static DECLCALLBACK(int) hmR0DummyInitVM(PVMCC pVM)
{
    RT_NOREF1(pVM);
    return VINF_SUCCESS;
}

static DECLCALLBACK(int) hmR0DummyTermVM(PVMCC pVM)
{
    RT_NOREF1(pVM);
    return VINF_SUCCESS;
}

static DECLCALLBACK(int) hmR0DummySetupVM(PVMCC pVM)
{
    RT_NOREF1(pVM);
    return VINF_SUCCESS;
}

static DECLCALLBACK(int) hmR0DummyCallRing3Callback(PVMCPUCC pVCpu, VMMCALLRING3 enmOperation)
{
    RT_NOREF2(pVCpu, enmOperation);
    return VINF_SUCCESS;
}

static DECLCALLBACK(VBOXSTRICTRC) hmR0DummyRunGuestCode(PVMCPUCC pVCpu)
{
    RT_NOREF(pVCpu);
    return VINF_SUCCESS;
}

static DECLCALLBACK(int) hmR0DummyExportHostState(PVMCPUCC pVCpu)
{
    RT_NOREF1(pVCpu);
    return VINF_SUCCESS;
}

/** @} */


/**
 * Intel specific initialization code.
 *
 * @returns VBox status code (will only fail if out of memory).
 */
static int hmR0InitIntel(void)
{
    /* Read this MSR now as it may be useful for error reporting when initializing VT-x fails. */
    g_HmR0.hwvirt.Msrs.u.vmx.u64FeatCtrl = ASMRdMsr(MSR_IA32_FEATURE_CONTROL);

    /*
     * First try use native kernel API for controlling VT-x.
     * (This is only supported by some Mac OS X kernels atm.)
     */
    int rc = g_HmR0.rcInit = SUPR0EnableVTx(true /* fEnable */);
    g_HmR0.hwvirt.u.vmx.fUsingSUPR0EnableVTx = rc != VERR_NOT_SUPPORTED;
    if (g_HmR0.hwvirt.u.vmx.fUsingSUPR0EnableVTx)
    {
        AssertLogRelMsg(rc == VINF_SUCCESS || rc == VERR_VMX_IN_VMX_ROOT_MODE || rc == VERR_VMX_NO_VMX, ("%Rrc\n", rc));
        if (RT_SUCCESS(rc))
        {
            g_HmR0.hwvirt.u.vmx.fSupported = true;
            rc = SUPR0EnableVTx(false /* fEnable */);
            AssertLogRelRC(rc);
        }
    }
    else
    {
        HMR0FIRSTRC FirstRc;
        hmR0FirstRcInit(&FirstRc);
        g_HmR0.rcInit = RTMpOnAll(hmR0InitIntelCpu, &FirstRc, NULL);
        if (RT_SUCCESS(g_HmR0.rcInit))
            g_HmR0.rcInit = hmR0FirstRcGetStatus(&FirstRc);
    }

    if (RT_SUCCESS(g_HmR0.rcInit))
    {
        /* Read CR4 and EFER for logging/diagnostic purposes. */
        g_HmR0.hwvirt.u.vmx.u64HostCr4     = ASMGetCR4();
        g_HmR0.hwvirt.u.vmx.u64HostMsrEfer = ASMRdMsr(MSR_K6_EFER);

        /* Get VMX MSRs for determining VMX features we can ultimately use. */
        SUPR0GetHwvirtMsrs(&g_HmR0.hwvirt.Msrs, SUPVTCAPS_VT_X, false /* fForce */);

        /*
         * Nested KVM workaround: Intel SDM section 34.15.5 describes that
         * MSR_IA32_SMM_MONITOR_CTL depends on bit 49 of MSR_IA32_VMX_BASIC while
         * table 35-2 says that this MSR is available if either VMX or SMX is supported.
         */
        uint64_t const uVmxBasicMsr = g_HmR0.hwvirt.Msrs.u.vmx.u64Basic;
        if (RT_BF_GET(uVmxBasicMsr, VMX_BF_BASIC_DUAL_MON))
            g_HmR0.hwvirt.u.vmx.u64HostSmmMonitorCtl = ASMRdMsr(MSR_IA32_SMM_MONITOR_CTL);

        /* Initialize VPID - 16 bits ASID. */
        g_HmR0.hwvirt.uMaxAsid = 0x10000; /* exclusive */

        /*
         * If the host OS has not enabled VT-x for us, try enter VMX root mode
         * to really verify if VT-x is usable.
         */
        if (!g_HmR0.hwvirt.u.vmx.fUsingSUPR0EnableVTx)
        {
            /* Allocate a temporary VMXON region. */
            RTR0MEMOBJ hScatchMemObj;
            rc = RTR0MemObjAllocCont(&hScatchMemObj, PAGE_SIZE, false /* fExecutable */);
            if (RT_FAILURE(rc))
            {
                LogRel(("hmR0InitIntel: RTR0MemObjAllocCont(,PAGE_SIZE,false) -> %Rrc\n", rc));
                return rc;
            }
            void          *pvScatchPage      = RTR0MemObjAddress(hScatchMemObj);
            RTHCPHYS const HCPhysScratchPage = RTR0MemObjGetPagePhysAddr(hScatchMemObj, 0);
            ASMMemZeroPage(pvScatchPage);

            /* Set revision dword at the beginning of the VMXON structure. */
            *(uint32_t *)pvScatchPage = RT_BF_GET(uVmxBasicMsr, VMX_BF_BASIC_VMCS_ID);

            /* Make sure we don't get rescheduled to another CPU during this probe. */
            RTCCUINTREG const fEFlags = ASMIntDisableFlags();

            /* Enable CR4.VMXE if it isn't already set. */
            RTCCUINTREG const uOldCr4 = SUPR0ChangeCR4(X86_CR4_VMXE, RTCCUINTREG_MAX);

            /*
             * The only way of checking if we're in VMX root mode or not is to try and enter it.
             * There is no instruction or control bit that tells us if we're in VMX root mode.
             * Therefore, try and enter VMX root mode here.
             */
            rc = VMXEnable(HCPhysScratchPage);
            if (RT_SUCCESS(rc))
            {
                g_HmR0.hwvirt.u.vmx.fSupported = true;
                VMXDisable();
            }
            else
            {
                /*
                 * KVM leaves the CPU in VMX root mode. Not only is  this not allowed,
                 * it will crash the host when we enter raw mode, because:
                 *
                 *   (a) clearing X86_CR4_VMXE in CR4 causes a #GP (we no longer modify
                 *       this bit), and
                 *   (b) turning off paging causes a #GP  (unavoidable when switching
                 *       from long to 32 bits mode or 32 bits to PAE).
                 *
                 * They should fix their code, but until they do we simply refuse to run.
                 */
                g_HmR0.rcInit = VERR_VMX_IN_VMX_ROOT_MODE;
                Assert(g_HmR0.hwvirt.u.vmx.fSupported == false);
            }

            /* Restore CR4.VMXE if it wasn't set prior to us setting it above. */
            if (!(uOldCr4 & X86_CR4_VMXE))
                SUPR0ChangeCR4(0 /* fOrMask */, ~(uint64_t)X86_CR4_VMXE);

            /* Restore interrupts. */
            ASMSetFlags(fEFlags);

            RTR0MemObjFree(hScatchMemObj, false);
        }

        if (g_HmR0.hwvirt.u.vmx.fSupported)
        {
            rc = VMXR0GlobalInit();
            if (RT_FAILURE(rc))
                g_HmR0.rcInit = rc;

            /*
             * Install the VT-x methods.
             */
            g_HmR0.pfnEnterSession      = VMXR0Enter;
            g_HmR0.pfnThreadCtxCallback = VMXR0ThreadCtxCallback;
            g_HmR0.pfnCallRing3Callback = VMXR0CallRing3Callback;
            g_HmR0.pfnExportHostState   = VMXR0ExportHostState;
            g_HmR0.pfnRunGuestCode      = VMXR0RunGuestCode;
            g_HmR0.pfnEnableCpu         = VMXR0EnableCpu;
            g_HmR0.pfnDisableCpu        = VMXR0DisableCpu;
            g_HmR0.pfnInitVM            = VMXR0InitVM;
            g_HmR0.pfnTermVM            = VMXR0TermVM;
            g_HmR0.pfnSetupVM           = VMXR0SetupVM;

            /*
             * Check for the VMX-Preemption Timer and adjust for the "VMX-Preemption
             * Timer Does Not Count Down at the Rate Specified" CPU erratum.
             */
            VMXCTLSMSR PinCtls;
            PinCtls.u = g_HmR0.hwvirt.Msrs.u.vmx.u64PinCtls;
            if (PinCtls.n.allowed1 & VMX_PIN_CTLS_PREEMPT_TIMER)
            {
                uint64_t const uVmxMiscMsr = g_HmR0.hwvirt.Msrs.u.vmx.u64Misc;
                g_HmR0.hwvirt.u.vmx.fUsePreemptTimer   = true;
                g_HmR0.hwvirt.u.vmx.cPreemptTimerShift = RT_BF_GET(uVmxMiscMsr, VMX_BF_MISC_PREEMPT_TIMER_TSC);
                if (HMIsSubjectToVmxPreemptTimerErratum())
                    g_HmR0.hwvirt.u.vmx.cPreemptTimerShift = 0; /* This is about right most of the time here. */
            }
        }
    }
#ifdef LOG_ENABLED
    else
        SUPR0Printf("hmR0InitIntelCpu failed with rc=%Rrc\n", g_HmR0.rcInit);
#endif
    return VINF_SUCCESS;
}


/**
 * AMD-specific initialization code.
 *
 * @returns VBox status code (will only fail if out of memory).
 */
static int hmR0InitAmd(void)
{
    /* Call the global AMD-V initialization routine (should only fail in out-of-memory situations). */
    int rc = SVMR0GlobalInit();
    if (RT_FAILURE(rc))
    {
        g_HmR0.rcInit = rc;
        return rc;
    }

    /*
     * Install the AMD-V methods.
     */
    g_HmR0.pfnEnterSession      = SVMR0Enter;
    g_HmR0.pfnThreadCtxCallback = SVMR0ThreadCtxCallback;
    g_HmR0.pfnCallRing3Callback = SVMR0CallRing3Callback;
    g_HmR0.pfnExportHostState   = SVMR0ExportHostState;
    g_HmR0.pfnRunGuestCode      = SVMR0RunGuestCode;
    g_HmR0.pfnEnableCpu         = SVMR0EnableCpu;
    g_HmR0.pfnDisableCpu        = SVMR0DisableCpu;
    g_HmR0.pfnInitVM            = SVMR0InitVM;
    g_HmR0.pfnTermVM            = SVMR0TermVM;
    g_HmR0.pfnSetupVM           = SVMR0SetupVM;

    /* Query AMD features. */
    uint32_t u32Dummy;
    ASMCpuId(0x8000000a, &g_HmR0.hwvirt.u.svm.u32Rev, &g_HmR0.hwvirt.uMaxAsid, &u32Dummy, &g_HmR0.hwvirt.u.svm.u32Features);

    /*
     * We need to check if AMD-V has been properly initialized on all CPUs.
     * Some BIOSes might do a poor job.
     */
    HMR0FIRSTRC FirstRc;
    hmR0FirstRcInit(&FirstRc);
    rc = RTMpOnAll(hmR0InitAmdCpu, &FirstRc, NULL);
    AssertRC(rc);
    if (RT_SUCCESS(rc))
        rc = hmR0FirstRcGetStatus(&FirstRc);
#ifndef DEBUG_bird
    AssertMsg(rc == VINF_SUCCESS || rc == VERR_SVM_IN_USE,
              ("hmR0InitAmdCpu failed for cpu %d with rc=%Rrc\n", hmR0FirstRcGetCpuId(&FirstRc), rc));
#endif
    if (RT_SUCCESS(rc))
    {
        SUPR0GetHwvirtMsrs(&g_HmR0.hwvirt.Msrs, SUPVTCAPS_AMD_V, false /* fForce */);
        g_HmR0.hwvirt.u.svm.fSupported = true;
    }
    else
    {
        g_HmR0.rcInit = rc;
        if (rc == VERR_SVM_DISABLED || rc == VERR_SVM_IN_USE)
            rc = VINF_SUCCESS; /* Don't fail if AMD-V is disabled or in use. */
    }
    return rc;
}


/**
 * Does global Ring-0 HM initialization (at module init).
 *
 * @returns VBox status code.
 */
VMMR0_INT_DECL(int) HMR0Init(void)
{
    /*
     * Initialize the globals.
     */
    g_HmR0.fEnabled = false;
    static RTONCE s_OnceInit = RTONCE_INITIALIZER;
    g_HmR0.EnableAllCpusOnce = s_OnceInit;
    for (unsigned i = 0; i < RT_ELEMENTS(g_HmR0.aCpuInfo); i++)
    {
        g_HmR0.aCpuInfo[i].idCpu        = NIL_RTCPUID;
        g_HmR0.aCpuInfo[i].hMemObj      = NIL_RTR0MEMOBJ;
        g_HmR0.aCpuInfo[i].HCPhysMemObj = NIL_RTHCPHYS;
        g_HmR0.aCpuInfo[i].pvMemObj     = NULL;
#ifdef VBOX_WITH_NESTED_HWVIRT_SVM
        g_HmR0.aCpuInfo[i].n.svm.hNstGstMsrpm      = NIL_RTR0MEMOBJ;
        g_HmR0.aCpuInfo[i].n.svm.HCPhysNstGstMsrpm = NIL_RTHCPHYS;
        g_HmR0.aCpuInfo[i].n.svm.pvNstGstMsrpm     = NULL;
#endif
    }

    /* Fill in all callbacks with placeholders. */
    g_HmR0.pfnEnterSession      = hmR0DummyEnter;
    g_HmR0.pfnThreadCtxCallback = hmR0DummyThreadCtxCallback;
    g_HmR0.pfnCallRing3Callback = hmR0DummyCallRing3Callback;
    g_HmR0.pfnExportHostState   = hmR0DummyExportHostState;
    g_HmR0.pfnRunGuestCode      = hmR0DummyRunGuestCode;
    g_HmR0.pfnEnableCpu         = hmR0DummyEnableCpu;
    g_HmR0.pfnDisableCpu        = hmR0DummyDisableCpu;
    g_HmR0.pfnInitVM            = hmR0DummyInitVM;
    g_HmR0.pfnTermVM            = hmR0DummyTermVM;
    g_HmR0.pfnSetupVM           = hmR0DummySetupVM;

    /* Default is global VT-x/AMD-V init. */
    g_HmR0.fGlobalInit         = true;

    /*
     * Make sure aCpuInfo is big enough for all the CPUs on this system.
     */
    if (RTMpGetArraySize() > RT_ELEMENTS(g_HmR0.aCpuInfo))
    {
        LogRel(("HM: Too many real CPUs/cores/threads - %u, max %u\n", RTMpGetArraySize(), RT_ELEMENTS(g_HmR0.aCpuInfo)));
        return VERR_TOO_MANY_CPUS;
    }

    /*
     * Check for VT-x or AMD-V support.
     * Return failure only in out-of-memory situations.
     */
    uint32_t fCaps = 0;
    int rc = SUPR0GetVTSupport(&fCaps);
    if (RT_SUCCESS(rc))
    {
        if (fCaps & SUPVTCAPS_VT_X)
        {
            rc = hmR0InitIntel();
            if (RT_FAILURE(rc))
                return rc;
        }
        else
        {
            Assert(fCaps & SUPVTCAPS_AMD_V);
            rc = hmR0InitAmd();
            if (RT_FAILURE(rc))
                return rc;
        }
    }
    else
        g_HmR0.rcInit = VERR_UNSUPPORTED_CPU;

    /*
     * Register notification callbacks that we can use to disable/enable CPUs
     * when brought offline/online or suspending/resuming.
     */
    if (!g_HmR0.hwvirt.u.vmx.fUsingSUPR0EnableVTx)
    {
        rc = RTMpNotificationRegister(hmR0MpEventCallback, NULL);
        AssertRC(rc);

        rc = RTPowerNotificationRegister(hmR0PowerCallback, NULL);
        AssertRC(rc);
    }

    /* We return success here because module init shall not fail if HM fails to initialize. */
    return VINF_SUCCESS;
}


/**
 * Does global Ring-0 HM termination (at module termination).
 *
 * @returns VBox status code.
 */
VMMR0_INT_DECL(int) HMR0Term(void)
{
    int rc;
    if (   g_HmR0.hwvirt.u.vmx.fSupported
        && g_HmR0.hwvirt.u.vmx.fUsingSUPR0EnableVTx)
    {
        /*
         * Simple if the host OS manages VT-x.
         */
        Assert(g_HmR0.fGlobalInit);

        if (g_HmR0.hwvirt.u.vmx.fCalledSUPR0EnableVTx)
        {
            rc = SUPR0EnableVTx(false /* fEnable */);
            g_HmR0.hwvirt.u.vmx.fCalledSUPR0EnableVTx = false;
        }
        else
            rc = VINF_SUCCESS;

        for (unsigned iCpu = 0; iCpu < RT_ELEMENTS(g_HmR0.aCpuInfo); iCpu++)
        {
            g_HmR0.aCpuInfo[iCpu].fConfigured = false;
            Assert(g_HmR0.aCpuInfo[iCpu].hMemObj == NIL_RTR0MEMOBJ);
        }
    }
    else
    {
        Assert(!g_HmR0.hwvirt.u.vmx.fSupported || !g_HmR0.hwvirt.u.vmx.fUsingSUPR0EnableVTx);

        /* Doesn't really matter if this fails. */
        rc = RTMpNotificationDeregister(hmR0MpEventCallback, NULL);  AssertRC(rc);
        rc = RTPowerNotificationDeregister(hmR0PowerCallback, NULL); AssertRC(rc);

        /*
         * Disable VT-x/AMD-V on all CPUs if we enabled it before.
         */
        if (g_HmR0.fGlobalInit)
        {
            HMR0FIRSTRC FirstRc;
            hmR0FirstRcInit(&FirstRc);
            rc = RTMpOnAll(hmR0DisableCpuCallback, NULL /* pvUser 1 */, &FirstRc);
            Assert(RT_SUCCESS(rc) || rc == VERR_NOT_SUPPORTED);
            if (RT_SUCCESS(rc))
                rc = hmR0FirstRcGetStatus(&FirstRc);
        }

        /*
         * Free the per-cpu pages used for VT-x and AMD-V.
         */
        for (unsigned i = 0; i < RT_ELEMENTS(g_HmR0.aCpuInfo); i++)
        {
            if (g_HmR0.aCpuInfo[i].hMemObj != NIL_RTR0MEMOBJ)
            {
                RTR0MemObjFree(g_HmR0.aCpuInfo[i].hMemObj, false);
                g_HmR0.aCpuInfo[i].hMemObj      = NIL_RTR0MEMOBJ;
                g_HmR0.aCpuInfo[i].HCPhysMemObj = NIL_RTHCPHYS;
                g_HmR0.aCpuInfo[i].pvMemObj     = NULL;
            }
#ifdef VBOX_WITH_NESTED_HWVIRT_SVM
            if (g_HmR0.aCpuInfo[i].n.svm.hNstGstMsrpm != NIL_RTR0MEMOBJ)
            {
                RTR0MemObjFree(g_HmR0.aCpuInfo[i].n.svm.hNstGstMsrpm, false);
                g_HmR0.aCpuInfo[i].n.svm.hNstGstMsrpm      = NIL_RTR0MEMOBJ;
                g_HmR0.aCpuInfo[i].n.svm.HCPhysNstGstMsrpm = NIL_RTHCPHYS;
                g_HmR0.aCpuInfo[i].n.svm.pvNstGstMsrpm     = NULL;
            }
#endif
        }
    }

    /** @todo This needs cleaning up. There's no matching
     *        hmR0TermIntel()/hmR0TermAmd() and all the VT-x/AMD-V specific bits
     *        should move into their respective modules. */
    /* Finally, call global VT-x/AMD-V termination. */
    if (g_HmR0.hwvirt.u.vmx.fSupported)
        VMXR0GlobalTerm();
    else if (g_HmR0.hwvirt.u.svm.fSupported)
        SVMR0GlobalTerm();

    return rc;
}


/**
 * Worker function used by hmR0PowerCallback() and HMR0Init() to initalize VT-x
 * on a CPU.
 *
 * @param   idCpu       The identifier for the CPU the function is called on.
 * @param   pvUser1     Pointer to the first RC structure.
 * @param   pvUser2     Ignored.
 */
static DECLCALLBACK(void) hmR0InitIntelCpu(RTCPUID idCpu, void *pvUser1, void *pvUser2)
{
    PHMR0FIRSTRC pFirstRc = (PHMR0FIRSTRC)pvUser1;
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
    Assert(idCpu == (RTCPUID)RTMpCpuIdToSetIndex(idCpu)); /** @todo fix idCpu == index assumption (rainy day) */
    NOREF(idCpu); NOREF(pvUser2);

    int rc = SUPR0GetVmxUsability(NULL /* pfIsSmxModeAmbiguous */);
    hmR0FirstRcSetStatus(pFirstRc, rc);
}


/**
 * Worker function used by hmR0PowerCallback() and HMR0Init() to initalize AMD-V
 * on a CPU.
 *
 * @param   idCpu       The identifier for the CPU the function is called on.
 * @param   pvUser1     Pointer to the first RC structure.
 * @param   pvUser2     Ignored.
 */
static DECLCALLBACK(void) hmR0InitAmdCpu(RTCPUID idCpu, void *pvUser1, void *pvUser2)
{
    PHMR0FIRSTRC pFirstRc = (PHMR0FIRSTRC)pvUser1;
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
    Assert(idCpu == (RTCPUID)RTMpCpuIdToSetIndex(idCpu)); /** @todo fix idCpu == index assumption (rainy day) */
    NOREF(idCpu); NOREF(pvUser2);

    int rc = SUPR0GetSvmUsability(true /* fInitSvm */);
    hmR0FirstRcSetStatus(pFirstRc, rc);
}


/**
 * Enable VT-x or AMD-V on the current CPU
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure. Can be NULL.
 * @param   idCpu   The identifier for the CPU the function is called on.
 *
 * @remarks Maybe called with interrupts disabled!
 */
static int hmR0EnableCpu(PVMCC pVM, RTCPUID idCpu)
{
    PHMPHYSCPU pHostCpu = &g_HmR0.aCpuInfo[idCpu];

    Assert(idCpu == (RTCPUID)RTMpCpuIdToSetIndex(idCpu)); /** @todo fix idCpu == index assumption (rainy day) */
    Assert(idCpu < RT_ELEMENTS(g_HmR0.aCpuInfo));
    Assert(!pHostCpu->fConfigured);
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));

    pHostCpu->idCpu = idCpu;
    /* Do NOT reset cTlbFlushes here, see @bugref{6255}. */

    int rc;
    if (   g_HmR0.hwvirt.u.vmx.fSupported
        && g_HmR0.hwvirt.u.vmx.fUsingSUPR0EnableVTx)
        rc = g_HmR0.pfnEnableCpu(pHostCpu, pVM, NULL /* pvCpuPage */, NIL_RTHCPHYS, true, &g_HmR0.hwvirt.Msrs);
    else
    {
        AssertLogRelMsgReturn(pHostCpu->hMemObj != NIL_RTR0MEMOBJ, ("hmR0EnableCpu failed idCpu=%u.\n", idCpu), VERR_HM_IPE_1);
        rc = g_HmR0.pfnEnableCpu(pHostCpu, pVM, pHostCpu->pvMemObj, pHostCpu->HCPhysMemObj, false, &g_HmR0.hwvirt.Msrs);
    }
    if (RT_SUCCESS(rc))
        pHostCpu->fConfigured = true;
    return rc;
}


/**
 * Worker function passed to RTMpOnAll() that is to be called on all CPUs.
 *
 * @param   idCpu       The identifier for the CPU the function is called on.
 * @param   pvUser1     Opaque pointer to the VM (can be NULL!).
 * @param   pvUser2     The 2nd user argument.
 */
static DECLCALLBACK(void) hmR0EnableCpuCallback(RTCPUID idCpu, void *pvUser1, void *pvUser2)
{
    PVMCC           pVM      = (PVMCC)pvUser1;     /* can be NULL! */
    PHMR0FIRSTRC    pFirstRc = (PHMR0FIRSTRC)pvUser2;
    AssertReturnVoid(g_HmR0.fGlobalInit);
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
    hmR0FirstRcSetStatus(pFirstRc, hmR0EnableCpu(pVM, idCpu));
}


/**
 * RTOnce callback employed by HMR0EnableAllCpus.
 *
 * @returns VBox status code.
 * @param   pvUser          Pointer to the VM.
 */
static DECLCALLBACK(int32_t) hmR0EnableAllCpuOnce(void *pvUser)
{
    PVMCC pVM = (PVMCC)pvUser;

    /*
     * Indicate that we've initialized.
     *
     * Note! There is a potential race between this function and the suspend
     *       notification.  Kind of unlikely though, so ignored for now.
     */
    AssertReturn(!g_HmR0.fEnabled, VERR_HM_ALREADY_ENABLED_IPE);
    ASMAtomicWriteBool(&g_HmR0.fEnabled, true);

    /*
     * The global init variable is set by the first VM.
     */
    g_HmR0.fGlobalInit = pVM->hm.s.fGlobalInit;

#ifdef VBOX_STRICT
    for (unsigned i = 0; i < RT_ELEMENTS(g_HmR0.aCpuInfo); i++)
    {
        Assert(g_HmR0.aCpuInfo[i].hMemObj      == NIL_RTR0MEMOBJ);
        Assert(g_HmR0.aCpuInfo[i].HCPhysMemObj == NIL_RTHCPHYS);
        Assert(g_HmR0.aCpuInfo[i].pvMemObj     == NULL);
        Assert(!g_HmR0.aCpuInfo[i].fConfigured);
        Assert(!g_HmR0.aCpuInfo[i].cTlbFlushes);
        Assert(!g_HmR0.aCpuInfo[i].uCurrentAsid);
# ifdef VBOX_WITH_NESTED_HWVIRT_SVM
        Assert(g_HmR0.aCpuInfo[i].n.svm.hNstGstMsrpm      == NIL_RTR0MEMOBJ);
        Assert(g_HmR0.aCpuInfo[i].n.svm.HCPhysNstGstMsrpm == NIL_RTHCPHYS);
        Assert(g_HmR0.aCpuInfo[i].n.svm.pvNstGstMsrpm     == NULL);
# endif
    }
#endif

    int rc;
    if (   g_HmR0.hwvirt.u.vmx.fSupported
        && g_HmR0.hwvirt.u.vmx.fUsingSUPR0EnableVTx)
    {
        /*
         * Global VT-x initialization API (only darwin for now).
         */
        rc = SUPR0EnableVTx(true /* fEnable */);
        if (RT_SUCCESS(rc))
        {
            g_HmR0.hwvirt.u.vmx.fCalledSUPR0EnableVTx = true;
            /* If the host provides a VT-x init API, then we'll rely on that for global init. */
            g_HmR0.fGlobalInit = pVM->hm.s.fGlobalInit = true;
        }
        else
            AssertMsgFailed(("hmR0EnableAllCpuOnce/SUPR0EnableVTx: rc=%Rrc\n", rc));
    }
    else
    {
        /*
         * We're doing the job ourselves.
         */
        /* Allocate one page per cpu for the global VT-x and AMD-V pages */
        for (unsigned i = 0; i < RT_ELEMENTS(g_HmR0.aCpuInfo); i++)
        {
            Assert(g_HmR0.aCpuInfo[i].hMemObj == NIL_RTR0MEMOBJ);
#ifdef VBOX_WITH_NESTED_HWVIRT_SVM
            Assert(g_HmR0.aCpuInfo[i].n.svm.hNstGstMsrpm == NIL_RTR0MEMOBJ);
#endif
            if (RTMpIsCpuPossible(RTMpCpuIdFromSetIndex(i)))
            {
                /** @todo NUMA */
                rc = RTR0MemObjAllocCont(&g_HmR0.aCpuInfo[i].hMemObj, PAGE_SIZE, false /* executable R0 mapping */);
                AssertLogRelRCReturn(rc, rc);

                g_HmR0.aCpuInfo[i].HCPhysMemObj = RTR0MemObjGetPagePhysAddr(g_HmR0.aCpuInfo[i].hMemObj, 0);
                Assert(g_HmR0.aCpuInfo[i].HCPhysMemObj != NIL_RTHCPHYS);
                Assert(!(g_HmR0.aCpuInfo[i].HCPhysMemObj & PAGE_OFFSET_MASK));

                g_HmR0.aCpuInfo[i].pvMemObj     = RTR0MemObjAddress(g_HmR0.aCpuInfo[i].hMemObj);
                AssertPtr(g_HmR0.aCpuInfo[i].pvMemObj);
                ASMMemZeroPage(g_HmR0.aCpuInfo[i].pvMemObj);

#ifdef VBOX_WITH_NESTED_HWVIRT_SVM
                rc = RTR0MemObjAllocCont(&g_HmR0.aCpuInfo[i].n.svm.hNstGstMsrpm, SVM_MSRPM_PAGES << X86_PAGE_4K_SHIFT,
                                         false /* executable R0 mapping */);
                AssertLogRelRCReturn(rc, rc);

                g_HmR0.aCpuInfo[i].n.svm.HCPhysNstGstMsrpm = RTR0MemObjGetPagePhysAddr(g_HmR0.aCpuInfo[i].n.svm.hNstGstMsrpm, 0);
                Assert(g_HmR0.aCpuInfo[i].n.svm.HCPhysNstGstMsrpm != NIL_RTHCPHYS);
                Assert(!(g_HmR0.aCpuInfo[i].n.svm.HCPhysNstGstMsrpm & PAGE_OFFSET_MASK));

                g_HmR0.aCpuInfo[i].n.svm.pvNstGstMsrpm    = RTR0MemObjAddress(g_HmR0.aCpuInfo[i].n.svm.hNstGstMsrpm);
                AssertPtr(g_HmR0.aCpuInfo[i].n.svm.pvNstGstMsrpm);
                ASMMemFill32(g_HmR0.aCpuInfo[i].n.svm.pvNstGstMsrpm, SVM_MSRPM_PAGES << X86_PAGE_4K_SHIFT, UINT32_C(0xffffffff));
#endif
            }
        }

        rc = VINF_SUCCESS;
    }

    if (   RT_SUCCESS(rc)
        && g_HmR0.fGlobalInit)
    {
        /* First time, so initialize each cpu/core. */
        HMR0FIRSTRC FirstRc;
        hmR0FirstRcInit(&FirstRc);
        rc = RTMpOnAll(hmR0EnableCpuCallback, (void *)pVM, &FirstRc);
        if (RT_SUCCESS(rc))
            rc = hmR0FirstRcGetStatus(&FirstRc);
    }

    return rc;
}


/**
 * Sets up HM on all cpus.
 *
 * @returns VBox status code.
 * @param   pVM                 The cross context VM structure.
 */
VMMR0_INT_DECL(int) HMR0EnableAllCpus(PVMCC pVM)
{
    /* Make sure we don't touch HM after we've disabled HM in preparation of a suspend. */
    if (ASMAtomicReadBool(&g_HmR0.fSuspended))
        return VERR_HM_SUSPEND_PENDING;

    return RTOnce(&g_HmR0.EnableAllCpusOnce, hmR0EnableAllCpuOnce, pVM);
}


/**
 * Disable VT-x or AMD-V on the current CPU.
 *
 * @returns VBox status code.
 * @param   idCpu       The identifier for the CPU this function is called on.
 *
 * @remarks Must be called with preemption disabled.
 */
static int hmR0DisableCpu(RTCPUID idCpu)
{
    PHMPHYSCPU pHostCpu = &g_HmR0.aCpuInfo[idCpu];

    Assert(!g_HmR0.hwvirt.u.vmx.fSupported || !g_HmR0.hwvirt.u.vmx.fUsingSUPR0EnableVTx);
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
    Assert(idCpu == (RTCPUID)RTMpCpuIdToSetIndex(idCpu)); /** @todo fix idCpu == index assumption (rainy day) */
    Assert(idCpu < RT_ELEMENTS(g_HmR0.aCpuInfo));
    Assert(!pHostCpu->fConfigured || pHostCpu->hMemObj != NIL_RTR0MEMOBJ);
    AssertRelease(idCpu == RTMpCpuId());

    if (pHostCpu->hMemObj == NIL_RTR0MEMOBJ)
        return pHostCpu->fConfigured ? VERR_NO_MEMORY : VINF_SUCCESS /* not initialized. */;
    AssertPtr(pHostCpu->pvMemObj);
    Assert(pHostCpu->HCPhysMemObj != NIL_RTHCPHYS);

    int rc;
    if (pHostCpu->fConfigured)
    {
        rc = g_HmR0.pfnDisableCpu(pHostCpu, pHostCpu->pvMemObj, pHostCpu->HCPhysMemObj);
        AssertRCReturn(rc, rc);

        pHostCpu->fConfigured = false;
        pHostCpu->idCpu = NIL_RTCPUID;
    }
    else
        rc = VINF_SUCCESS; /* nothing to do */
    return rc;
}


/**
 * Worker function passed to RTMpOnAll() that is to be called on the target
 * CPUs.
 *
 * @param   idCpu       The identifier for the CPU the function is called on.
 * @param   pvUser1     The 1st user argument.
 * @param   pvUser2     Opaque pointer to the FirstRc.
 */
static DECLCALLBACK(void) hmR0DisableCpuCallback(RTCPUID idCpu, void *pvUser1, void *pvUser2)
{
    PHMR0FIRSTRC pFirstRc = (PHMR0FIRSTRC)pvUser2; NOREF(pvUser1);
    AssertReturnVoid(g_HmR0.fGlobalInit);
    hmR0FirstRcSetStatus(pFirstRc, hmR0DisableCpu(idCpu));
}


/**
 * Worker function passed to RTMpOnSpecific() that is to be called on the target
 * CPU.
 *
 * @param   idCpu       The identifier for the CPU the function is called on.
 * @param   pvUser1     Null, not used.
 * @param   pvUser2     Null, not used.
 */
static DECLCALLBACK(void) hmR0DisableCpuOnSpecificCallback(RTCPUID idCpu, void *pvUser1, void *pvUser2)
{
    NOREF(pvUser1);
    NOREF(pvUser2);
    hmR0DisableCpu(idCpu);
}


/**
 * Callback function invoked when a cpu goes online or offline.
 *
 * @param   enmEvent            The Mp event.
 * @param   idCpu               The identifier for the CPU the function is called on.
 * @param   pvData              Opaque data (PVMCC pointer).
 */
static DECLCALLBACK(void) hmR0MpEventCallback(RTMPEVENT enmEvent, RTCPUID idCpu, void *pvData)
{
    NOREF(pvData);
    Assert(!g_HmR0.hwvirt.u.vmx.fSupported || !g_HmR0.hwvirt.u.vmx.fUsingSUPR0EnableVTx);

    /*
     * We only care about uninitializing a CPU that is going offline. When a
     * CPU comes online, the initialization is done lazily in HMR0Enter().
     */
    switch (enmEvent)
    {
        case RTMPEVENT_OFFLINE:
        {
            RTTHREADPREEMPTSTATE PreemptState = RTTHREADPREEMPTSTATE_INITIALIZER;
            RTThreadPreemptDisable(&PreemptState);
            if (idCpu == RTMpCpuId())
            {
                int rc = hmR0DisableCpu(idCpu);
                AssertRC(rc);
                RTThreadPreemptRestore(&PreemptState);
            }
            else
            {
                RTThreadPreemptRestore(&PreemptState);
                RTMpOnSpecific(idCpu, hmR0DisableCpuOnSpecificCallback, NULL /* pvUser1 */, NULL /* pvUser2 */);
            }
            break;
        }

        default:
            break;
    }
}


/**
 * Called whenever a system power state change occurs.
 *
 * @param   enmEvent        The Power event.
 * @param   pvUser          User argument.
 */
static DECLCALLBACK(void) hmR0PowerCallback(RTPOWEREVENT enmEvent, void *pvUser)
{
    NOREF(pvUser);
    Assert(!g_HmR0.hwvirt.u.vmx.fSupported || !g_HmR0.hwvirt.u.vmx.fUsingSUPR0EnableVTx);

#ifdef LOG_ENABLED
    if (enmEvent == RTPOWEREVENT_SUSPEND)
        SUPR0Printf("hmR0PowerCallback RTPOWEREVENT_SUSPEND\n");
    else
        SUPR0Printf("hmR0PowerCallback RTPOWEREVENT_RESUME\n");
#endif

    if (enmEvent == RTPOWEREVENT_SUSPEND)
        ASMAtomicWriteBool(&g_HmR0.fSuspended, true);

    if (g_HmR0.fEnabled)
    {
        int         rc;
        HMR0FIRSTRC FirstRc;
        hmR0FirstRcInit(&FirstRc);

        if (enmEvent == RTPOWEREVENT_SUSPEND)
        {
            if (g_HmR0.fGlobalInit)
            {
                /* Turn off VT-x or AMD-V on all CPUs. */
                rc = RTMpOnAll(hmR0DisableCpuCallback, NULL /* pvUser 1 */, &FirstRc);
                Assert(RT_SUCCESS(rc) || rc == VERR_NOT_SUPPORTED);
            }
            /* else nothing to do here for the local init case */
        }
        else
        {
            /* Reinit the CPUs from scratch as the suspend state might have
               messed with the MSRs. (lousy BIOSes as usual) */
            if (g_HmR0.hwvirt.u.vmx.fSupported)
                rc = RTMpOnAll(hmR0InitIntelCpu, &FirstRc, NULL);
            else
                rc = RTMpOnAll(hmR0InitAmdCpu, &FirstRc, NULL);
            Assert(RT_SUCCESS(rc) || rc == VERR_NOT_SUPPORTED);
            if (RT_SUCCESS(rc))
                rc = hmR0FirstRcGetStatus(&FirstRc);
#ifdef LOG_ENABLED
            if (RT_FAILURE(rc))
                SUPR0Printf("hmR0PowerCallback hmR0InitXxxCpu failed with %Rc\n", rc);
#endif
            if (g_HmR0.fGlobalInit)
            {
                /* Turn VT-x or AMD-V back on on all CPUs. */
                rc = RTMpOnAll(hmR0EnableCpuCallback, NULL /* pVM */, &FirstRc /* output ignored */);
                Assert(RT_SUCCESS(rc) || rc == VERR_NOT_SUPPORTED);
            }
            /* else nothing to do here for the local init case */
        }
    }

    if (enmEvent == RTPOWEREVENT_RESUME)
        ASMAtomicWriteBool(&g_HmR0.fSuspended, false);
}


/**
 * Does ring-0 per-VM HM initialization.
 *
 * This will call the CPU specific init. routine which may initialize and allocate
 * resources for virtual CPUs.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 *
 * @remarks This is called after HMR3Init(), see vmR3CreateU() and
 *          vmR3InitRing3().
 */
VMMR0_INT_DECL(int) HMR0InitVM(PVMCC pVM)
{
    AssertReturn(pVM, VERR_INVALID_PARAMETER);

    /* Make sure we don't touch HM after we've disabled HM in preparation of a suspend. */
    if (ASMAtomicReadBool(&g_HmR0.fSuspended))
        return VERR_HM_SUSPEND_PENDING;

    /*
     * Copy globals to the VM structure.
     */
    Assert(!(pVM->hm.s.vmx.fSupported && pVM->hm.s.svm.fSupported));
    if (pVM->hm.s.vmx.fSupported)
    {
        pVM->hm.s.vmx.fUsePreemptTimer     &= g_HmR0.hwvirt.u.vmx.fUsePreemptTimer; /* Can be overridden by CFGM in HMR3Init(). */
        pVM->hm.s.vmx.cPreemptTimerShift    = g_HmR0.hwvirt.u.vmx.cPreemptTimerShift;
        pVM->hm.s.vmx.u64HostCr4            = g_HmR0.hwvirt.u.vmx.u64HostCr4;
        pVM->hm.s.vmx.u64HostMsrEfer        = g_HmR0.hwvirt.u.vmx.u64HostMsrEfer;
        pVM->hm.s.vmx.u64HostSmmMonitorCtl  = g_HmR0.hwvirt.u.vmx.u64HostSmmMonitorCtl;
        HMGetVmxMsrsFromHwvirtMsrs(&g_HmR0.hwvirt.Msrs, &pVM->hm.s.vmx.Msrs);
        /* If you need to tweak host MSRs for testing VMX R0 code, do it here. */

        /* Enable VPID if supported and configured. */
        if (pVM->hm.s.vmx.Msrs.ProcCtls2.n.allowed1 & VMX_PROC_CTLS2_VPID)
            pVM->hm.s.vmx.fVpid = pVM->hm.s.vmx.fAllowVpid; /* Can be overridden by CFGM in HMR3Init(). */

        /* Use VMCS shadowing if supported. */
        Assert(!pVM->hm.s.vmx.fUseVmcsShadowing);
        if (   pVM->cpum.ro.GuestFeatures.fVmx
            && (pVM->hm.s.vmx.Msrs.ProcCtls2.n.allowed1 & VMX_PROC_CTLS2_VMCS_SHADOWING))
            pVM->hm.s.vmx.fUseVmcsShadowing = true;

        /* Use the VMCS controls for swapping the EFER MSR if supported. */
        Assert(!pVM->hm.s.vmx.fSupportsVmcsEfer);
        if (   (pVM->hm.s.vmx.Msrs.EntryCtls.n.allowed1 & VMX_ENTRY_CTLS_LOAD_EFER_MSR)
            && (pVM->hm.s.vmx.Msrs.ExitCtls.n.allowed1  & VMX_EXIT_CTLS_LOAD_EFER_MSR)
            && (pVM->hm.s.vmx.Msrs.ExitCtls.n.allowed1  & VMX_EXIT_CTLS_SAVE_EFER_MSR))
            pVM->hm.s.vmx.fSupportsVmcsEfer = true;

#if 0
        /* Enable APIC register virtualization and virtual-interrupt delivery if supported. */
        if (   (pVM->hm.s.vmx.Msrs.ProcCtls2.n.allowed1 & VMX_PROC_CTLS2_APIC_REG_VIRT)
            && (pVM->hm.s.vmx.Msrs.ProcCtls2.n.allowed1 & VMX_PROC_CTLS2_VIRT_INTR_DELIVERY))
            pVM->hm.s.fVirtApicRegs = true;

        /* Enable posted-interrupt processing if supported. */
        /** @todo Add and query IPRT API for host OS support for posted-interrupt IPI
         *        here. */
        if (   (pVM->hm.s.vmx.Msrs.PinCtls.n.allowed1  & VMX_PIN_CTLS_POSTED_INT)
            && (pVM->hm.s.vmx.Msrs.ExitCtls.n.allowed1 & VMX_EXIT_CTLS_ACK_EXT_INT))
            pVM->hm.s.fPostedIntrs = true;
#endif
    }
    else if (pVM->hm.s.svm.fSupported)
    {
        pVM->hm.s.svm.u32Rev      = g_HmR0.hwvirt.u.svm.u32Rev;
        pVM->hm.s.svm.u32Features = g_HmR0.hwvirt.u.svm.u32Features;
        pVM->hm.s.svm.u64MsrHwcr  = g_HmR0.hwvirt.Msrs.u.svm.u64MsrHwcr;
        /* If you need to tweak host MSRs for testing SVM R0 code, do it here. */
    }
    pVM->hm.s.rcInit              = g_HmR0.rcInit;
    pVM->hm.s.uMaxAsid            = g_HmR0.hwvirt.uMaxAsid;

    /*
     * Set default maximum inner loops in ring-0 before returning to ring-3.
     * Can be overriden using CFGM.
     */
    if (!pVM->hm.s.cMaxResumeLoops)
    {
        pVM->hm.s.cMaxResumeLoops       = 1024;
        if (RTThreadPreemptIsPendingTrusty())
            pVM->hm.s.cMaxResumeLoops   = 8192;
    }

    /*
     * Initialize some per-VCPU fields.
     */
    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
    {
        PVMCPUCC pVCpu = VMCC_GET_CPU(pVM, idCpu);
        pVCpu->hm.s.idEnteredCpu   = NIL_RTCPUID;
        pVCpu->hm.s.idLastCpu      = NIL_RTCPUID;

        /* We'll aways increment this the first time (host uses ASID 0). */
        AssertReturn(!pVCpu->hm.s.uCurrentAsid, VERR_HM_IPE_3);
    }

    /*
     * Get host kernel features that HM might need to know in order
     * to co-operate and function properly with the host OS (e.g. SMAP).
     *
     * Technically, we could do this as part of the pre-init VM procedure
     * but it shouldn't be done later than this point so we do it here.
     */
    pVM->hm.s.fHostKernelFeatures = SUPR0GetKernelFeatures();

    /*
     * Call the hardware specific initialization method.
     */
    return g_HmR0.pfnInitVM(pVM);
}


/**
 * Does ring-0 per VM HM termination.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR0_INT_DECL(int) HMR0TermVM(PVMCC pVM)
{
    Log(("HMR0TermVM: %p\n", pVM));
    AssertReturn(pVM, VERR_INVALID_PARAMETER);

    /*
     * Call the hardware specific method.
     *
     * Note! We might be preparing for a suspend, so the pfnTermVM() functions should probably not
     * mess with VT-x/AMD-V features on the CPU, currently all they do is free memory so this is safe.
     */
    return g_HmR0.pfnTermVM(pVM);
}


/**
 * Sets up a VT-x or AMD-V session.
 *
 * This is mostly about setting up the hardware VM state.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR0_INT_DECL(int) HMR0SetupVM(PVMCC pVM)
{
    Log(("HMR0SetupVM: %p\n", pVM));
    AssertReturn(pVM, VERR_INVALID_PARAMETER);

    /* Make sure we don't touch HM after we've disabled HM in preparation of a suspend. */
    AssertReturn(!ASMAtomicReadBool(&g_HmR0.fSuspended), VERR_HM_SUSPEND_PENDING);

    /* On first entry we'll sync everything. */
    VMCC_FOR_EACH_VMCPU_STMT(pVM, pVCpu->hm.s.fCtxChanged |= HM_CHANGED_HOST_CONTEXT | HM_CHANGED_ALL_GUEST);

    /*
     * Call the hardware specific setup VM method. This requires the CPU to be
     * enabled for AMD-V/VT-x and preemption to be prevented.
     */
    RTTHREADPREEMPTSTATE PreemptState = RTTHREADPREEMPTSTATE_INITIALIZER;
    RTThreadPreemptDisable(&PreemptState);
    RTCPUID const idCpu = RTMpCpuId();

    /* Enable VT-x or AMD-V if local init is required. */
    int rc;
    if (!g_HmR0.fGlobalInit)
    {
        Assert(!g_HmR0.hwvirt.u.vmx.fSupported || !g_HmR0.hwvirt.u.vmx.fUsingSUPR0EnableVTx);
        rc = hmR0EnableCpu(pVM, idCpu);
        if (RT_FAILURE(rc))
        {
            RTThreadPreemptRestore(&PreemptState);
            return rc;
        }
    }

    /* Setup VT-x or AMD-V. */
    rc = g_HmR0.pfnSetupVM(pVM);

    /* Disable VT-x or AMD-V if local init was done before. */
    if (!g_HmR0.fGlobalInit)
    {
        Assert(!g_HmR0.hwvirt.u.vmx.fSupported || !g_HmR0.hwvirt.u.vmx.fUsingSUPR0EnableVTx);
        int rc2 = hmR0DisableCpu(idCpu);
        AssertRC(rc2);
    }

    RTThreadPreemptRestore(&PreemptState);
    return rc;
}


/**
 * Notification callback before performing a longjump to ring-3.
 *
 * @returns VBox status code.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   enmOperation    The operation causing the ring-3 longjump.
 * @param   pvUser          User argument, currently unused, NULL.
 */
static DECLCALLBACK(int) hmR0CallRing3Callback(PVMCPUCC pVCpu, VMMCALLRING3 enmOperation, void *pvUser)
{
    RT_NOREF(pvUser);
    Assert(pVCpu);
    Assert(g_HmR0.pfnCallRing3Callback);
    return g_HmR0.pfnCallRing3Callback(pVCpu, enmOperation);
}


/**
 * Turns on HM on the CPU if necessary and initializes the bare minimum state
 * required for entering HM context.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 *
 * @remarks No-long-jump zone!!!
 */
VMMR0_INT_DECL(int) hmR0EnterCpu(PVMCPUCC pVCpu)
{
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));

    int              rc       = VINF_SUCCESS;
    RTCPUID const    idCpu    = RTMpCpuId();
    PHMPHYSCPU       pHostCpu = &g_HmR0.aCpuInfo[idCpu];
    AssertPtr(pHostCpu);

    /* Enable VT-x or AMD-V if local init is required, or enable if it's a freshly onlined CPU. */
    if (!pHostCpu->fConfigured)
        rc = hmR0EnableCpu(pVCpu->CTX_SUFF(pVM), idCpu);

    /* Register a callback to fire prior to performing a longjmp to ring-3 so HM can disable VT-x/AMD-V if needed. */
    VMMRZCallRing3SetNotification(pVCpu, hmR0CallRing3Callback, NULL /* pvUser */);

    /* Reload host-state (back from ring-3/migrated CPUs) and shared guest/host bits. */
    if (g_HmR0.hwvirt.u.vmx.fSupported)
        pVCpu->hm.s.fCtxChanged |= HM_CHANGED_HOST_CONTEXT | HM_CHANGED_VMX_HOST_GUEST_SHARED_STATE;
    else
        pVCpu->hm.s.fCtxChanged |= HM_CHANGED_HOST_CONTEXT | HM_CHANGED_SVM_HOST_GUEST_SHARED_STATE;

    Assert(pHostCpu->idCpu == idCpu && pHostCpu->idCpu != NIL_RTCPUID);
    pVCpu->hm.s.idEnteredCpu = idCpu;
    return rc;
}


/**
 * Enters the VT-x or AMD-V session.
 *
 * @returns VBox status code.
 * @param   pVCpu      The cross context virtual CPU structure.
 *
 * @remarks This is called with preemption disabled.
 */
VMMR0_INT_DECL(int) HMR0Enter(PVMCPUCC pVCpu)
{
    /* Make sure we can't enter a session after we've disabled HM in preparation of a suspend. */
    AssertReturn(!ASMAtomicReadBool(&g_HmR0.fSuspended), VERR_HM_SUSPEND_PENDING);
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));

    /* Load the bare minimum state required for entering HM. */
    int rc = hmR0EnterCpu(pVCpu);
    if (RT_SUCCESS(rc))
    {
        if (g_HmR0.hwvirt.u.vmx.fSupported)
        {
            Assert((pVCpu->hm.s.fCtxChanged & (HM_CHANGED_HOST_CONTEXT | HM_CHANGED_VMX_HOST_GUEST_SHARED_STATE))
                                           == (HM_CHANGED_HOST_CONTEXT | HM_CHANGED_VMX_HOST_GUEST_SHARED_STATE));
        }
        else
        {
            Assert((pVCpu->hm.s.fCtxChanged & (HM_CHANGED_HOST_CONTEXT | HM_CHANGED_SVM_HOST_GUEST_SHARED_STATE))
                                           == (HM_CHANGED_HOST_CONTEXT | HM_CHANGED_SVM_HOST_GUEST_SHARED_STATE));
        }

#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
        AssertReturn(!VMMR0ThreadCtxHookIsEnabled(pVCpu), VERR_HM_IPE_5);
        bool const fStartedSet = PGMR0DynMapStartOrMigrateAutoSet(pVCpu);
#endif

        /* Keep track of the CPU owning the VMCS for debugging scheduling weirdness and ring-3 calls. */
        rc = g_HmR0.pfnEnterSession(pVCpu);
        AssertMsgRCReturnStmt(rc, ("rc=%Rrc pVCpu=%p\n", rc, pVCpu),  pVCpu->hm.s.idEnteredCpu = NIL_RTCPUID, rc);

        /* Exports the host-state as we may be resuming code after a longjmp and quite
           possibly now be scheduled on a different CPU. */
        rc = g_HmR0.pfnExportHostState(pVCpu);
        AssertMsgRCReturnStmt(rc, ("rc=%Rrc pVCpu=%p\n", rc, pVCpu),  pVCpu->hm.s.idEnteredCpu = NIL_RTCPUID, rc);

#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
        if (fStartedSet)
            PGMRZDynMapReleaseAutoSet(pVCpu);
#endif
    }
    return rc;
}


/**
 * Deinitializes the bare minimum state used for HM context and if necessary
 * disable HM on the CPU.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 *
 * @remarks No-long-jump zone!!!
 */
VMMR0_INT_DECL(int) HMR0LeaveCpu(PVMCPUCC pVCpu)
{
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
    VMCPU_ASSERT_EMT_RETURN(pVCpu, VERR_HM_WRONG_CPU);

    RTCPUID const idCpu    = RTMpCpuId();
    PCHMPHYSCPU   pHostCpu = &g_HmR0.aCpuInfo[idCpu];

    if (   !g_HmR0.fGlobalInit
        && pHostCpu->fConfigured)
    {
        int rc = hmR0DisableCpu(idCpu);
        AssertRCReturn(rc, rc);
        Assert(!pHostCpu->fConfigured);
        Assert(pHostCpu->idCpu == NIL_RTCPUID);

        /* For obtaining a non-zero ASID/VPID on next re-entry. */
        pVCpu->hm.s.idLastCpu = NIL_RTCPUID;
    }

    /* Clear it while leaving HM context, hmPokeCpuForTlbFlush() relies on this. */
    pVCpu->hm.s.idEnteredCpu = NIL_RTCPUID;

    /* De-register the longjmp-to-ring 3 callback now that we have reliquished hardware resources. */
    VMMRZCallRing3RemoveNotification(pVCpu);
    return VINF_SUCCESS;
}


/**
 * Thread-context hook for HM.
 *
 * @param   enmEvent        The thread-context event.
 * @param   pvUser          Opaque pointer to the VMCPU.
 */
VMMR0_INT_DECL(void) HMR0ThreadCtxCallback(RTTHREADCTXEVENT enmEvent, void *pvUser)
{
    PVMCPUCC pVCpu = (PVMCPUCC)pvUser;
    Assert(pVCpu);
    Assert(g_HmR0.pfnThreadCtxCallback);

    g_HmR0.pfnThreadCtxCallback(enmEvent, pVCpu, g_HmR0.fGlobalInit);
}


/**
 * Runs guest code in a hardware accelerated VM.
 *
 * @returns Strict VBox status code. (VBOXSTRICTRC isn't used because it's
 *          called from setjmp assembly.)
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 *
 * @remarks Can be called with preemption enabled if thread-context hooks are
 *          used!!!
 */
VMMR0_INT_DECL(int) HMR0RunGuestCode(PVMCC pVM, PVMCPUCC pVCpu)
{
    RT_NOREF(pVM);

#ifdef VBOX_STRICT
    /* With thread-context hooks we would be running this code with preemption enabled. */
    if (!RTThreadPreemptIsEnabled(NIL_RTTHREAD))
    {
        PCHMPHYSCPU pHostCpu = &g_HmR0.aCpuInfo[RTMpCpuId()];
        Assert(!VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3 | VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL));
        Assert(pHostCpu->fConfigured);
        AssertReturn(!ASMAtomicReadBool(&g_HmR0.fSuspended), VERR_HM_SUSPEND_PENDING);
    }
#endif

#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
    AssertReturn(!VMMR0ThreadCtxHookIsEnabled(pVCpu), VERR_HM_IPE_4);
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
    PGMRZDynMapStartAutoSet(pVCpu);
#endif

    VBOXSTRICTRC rcStrict = g_HmR0.pfnRunGuestCode(pVCpu);

#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
    PGMRZDynMapReleaseAutoSet(pVCpu);
#endif
    return VBOXSTRICTRC_VAL(rcStrict);
}


/**
 * Notification from CPUM that it has unloaded the guest FPU/SSE/AVX state from
 * the host CPU and that guest access to it must be intercepted.
 *
 * @param   pVCpu   The cross context virtual CPU structure of the calling EMT.
 */
VMMR0_INT_DECL(void) HMR0NotifyCpumUnloadedGuestFpuState(PVMCPUCC pVCpu)
{
    ASMAtomicUoOrU64(&pVCpu->hm.s.fCtxChanged, HM_CHANGED_GUEST_CR0);
}


/**
 * Notification from CPUM that it has modified the host CR0 (because of FPU).
 *
 * @param   pVCpu   The cross context virtual CPU structure of the calling EMT.
 */
VMMR0_INT_DECL(void) HMR0NotifyCpumModifiedHostCr0(PVMCPUCC pVCpu)
{
    ASMAtomicUoOrU64(&pVCpu->hm.s.fCtxChanged, HM_CHANGED_HOST_CONTEXT);
}


/**
 * Returns suspend status of the host.
 *
 * @returns Suspend pending or not.
 */
VMMR0_INT_DECL(bool) HMR0SuspendPending(void)
{
    return ASMAtomicReadBool(&g_HmR0.fSuspended);
}


/**
 * Invalidates a guest page from the host TLB.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCVirt      Page to invalidate.
 */
VMMR0_INT_DECL(int) HMR0InvalidatePage(PVMCPUCC pVCpu, RTGCPTR GCVirt)
{
    PVMCC pVM = pVCpu->CTX_SUFF(pVM);
    if (pVM->hm.s.vmx.fSupported)
        return VMXR0InvalidatePage(pVCpu, GCVirt);
    return SVMR0InvalidatePage(pVCpu, GCVirt);
}


/**
 * Returns the cpu structure for the current cpu.
 * Keep in mind that there is no guarantee it will stay the same (long jumps to ring 3!!!).
 *
 * @returns The cpu structure pointer.
 */
VMMR0_INT_DECL(PHMPHYSCPU) hmR0GetCurrentCpu(void)
{
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
    RTCPUID const idCpu = RTMpCpuId();
    Assert(idCpu < RT_ELEMENTS(g_HmR0.aCpuInfo));
    return &g_HmR0.aCpuInfo[idCpu];
}


/**
 * Interface for importing state on demand (used by IEM).
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context CPU structure.
 * @param   fWhat       What to import, CPUMCTX_EXTRN_XXX.
 */
VMMR0_INT_DECL(int) HMR0ImportStateOnDemand(PVMCPUCC pVCpu, uint64_t fWhat)
{
    if (pVCpu->CTX_SUFF(pVM)->hm.s.vmx.fSupported)
        return VMXR0ImportStateOnDemand(pVCpu, fWhat);
    return SVMR0ImportStateOnDemand(pVCpu, fWhat);
}

#ifdef VBOX_STRICT

/**
 * Dumps a descriptor.
 *
 * @param   pDesc    Descriptor to dump.
 * @param   Sel      The selector.
 * @param   pszSel   The name of the selector.
 */
VMMR0_INT_DECL(void) hmR0DumpDescriptor(PCX86DESCHC pDesc, RTSEL Sel, const char *pszSel)
{
    /*
     * Make variable description string.
     */
    static struct
    {
        unsigned    cch;
        const char *psz;
    } const s_aTypes[32] =
    {
# define STRENTRY(str) { sizeof(str) - 1, str }

        /* system */
# if HC_ARCH_BITS == 64
        STRENTRY("Reserved0 "),                  /* 0x00 */
        STRENTRY("Reserved1 "),                  /* 0x01 */
        STRENTRY("LDT "),                        /* 0x02 */
        STRENTRY("Reserved3 "),                  /* 0x03 */
        STRENTRY("Reserved4 "),                  /* 0x04 */
        STRENTRY("Reserved5 "),                  /* 0x05 */
        STRENTRY("Reserved6 "),                  /* 0x06 */
        STRENTRY("Reserved7 "),                  /* 0x07 */
        STRENTRY("Reserved8 "),                  /* 0x08 */
        STRENTRY("TSS64Avail "),                 /* 0x09 */
        STRENTRY("ReservedA "),                  /* 0x0a */
        STRENTRY("TSS64Busy "),                  /* 0x0b */
        STRENTRY("Call64 "),                     /* 0x0c */
        STRENTRY("ReservedD "),                  /* 0x0d */
        STRENTRY("Int64 "),                      /* 0x0e */
        STRENTRY("Trap64 "),                     /* 0x0f */
# else
        STRENTRY("Reserved0 "),                  /* 0x00 */
        STRENTRY("TSS16Avail "),                 /* 0x01 */
        STRENTRY("LDT "),                        /* 0x02 */
        STRENTRY("TSS16Busy "),                  /* 0x03 */
        STRENTRY("Call16 "),                     /* 0x04 */
        STRENTRY("Task "),                       /* 0x05 */
        STRENTRY("Int16 "),                      /* 0x06 */
        STRENTRY("Trap16 "),                     /* 0x07 */
        STRENTRY("Reserved8 "),                  /* 0x08 */
        STRENTRY("TSS32Avail "),                 /* 0x09 */
        STRENTRY("ReservedA "),                  /* 0x0a */
        STRENTRY("TSS32Busy "),                  /* 0x0b */
        STRENTRY("Call32 "),                     /* 0x0c */
        STRENTRY("ReservedD "),                  /* 0x0d */
        STRENTRY("Int32 "),                      /* 0x0e */
        STRENTRY("Trap32 "),                     /* 0x0f */
# endif
        /* non system */
        STRENTRY("DataRO "),                     /* 0x10 */
        STRENTRY("DataRO Accessed "),            /* 0x11 */
        STRENTRY("DataRW "),                     /* 0x12 */
        STRENTRY("DataRW Accessed "),            /* 0x13 */
        STRENTRY("DataDownRO "),                 /* 0x14 */
        STRENTRY("DataDownRO Accessed "),        /* 0x15 */
        STRENTRY("DataDownRW "),                 /* 0x16 */
        STRENTRY("DataDownRW Accessed "),        /* 0x17 */
        STRENTRY("CodeEO "),                     /* 0x18 */
        STRENTRY("CodeEO Accessed "),            /* 0x19 */
        STRENTRY("CodeER "),                     /* 0x1a */
        STRENTRY("CodeER Accessed "),            /* 0x1b */
        STRENTRY("CodeConfEO "),                 /* 0x1c */
        STRENTRY("CodeConfEO Accessed "),        /* 0x1d */
        STRENTRY("CodeConfER "),                 /* 0x1e */
        STRENTRY("CodeConfER Accessed ")         /* 0x1f */
# undef SYSENTRY
    };
# define ADD_STR(psz, pszAdd) do { strcpy(psz, pszAdd); psz += strlen(pszAdd); } while (0)
    char        szMsg[128];
    char       *psz = &szMsg[0];
    unsigned    i = pDesc->Gen.u1DescType << 4 | pDesc->Gen.u4Type;
    memcpy(psz, s_aTypes[i].psz, s_aTypes[i].cch);
    psz += s_aTypes[i].cch;

    if (pDesc->Gen.u1Present)
        ADD_STR(psz, "Present ");
    else
        ADD_STR(psz, "Not-Present ");
# if HC_ARCH_BITS == 64
    if (pDesc->Gen.u1Long)
        ADD_STR(psz, "64-bit ");
    else
        ADD_STR(psz, "Comp ");
# else
    if (pDesc->Gen.u1Granularity)
        ADD_STR(psz, "Page ");
    if (pDesc->Gen.u1DefBig)
        ADD_STR(psz, "32-bit ");
    else
        ADD_STR(psz, "16-bit ");
# endif
# undef ADD_STR
    *psz = '\0';

    /*
     * Limit and Base and format the output.
     */
#ifdef LOG_ENABLED
    uint32_t u32Limit = X86DESC_LIMIT_G(pDesc);

# if HC_ARCH_BITS == 64
    uint64_t const u64Base  = X86DESC64_BASE(pDesc);
    Log(("  %s { %#04x - %#RX64 %#RX64 - base=%#RX64 limit=%#08x dpl=%d } %s\n", pszSel,
         Sel, pDesc->au64[0], pDesc->au64[1], u64Base, u32Limit, pDesc->Gen.u2Dpl, szMsg));
# else
    uint32_t const u32Base  = X86DESC_BASE(pDesc);
    Log(("  %s { %#04x - %#08x %#08x - base=%#08x limit=%#08x dpl=%d } %s\n", pszSel,
         Sel, pDesc->au32[0], pDesc->au32[1], u32Base, u32Limit, pDesc->Gen.u2Dpl, szMsg));
# endif
#else
    NOREF(Sel); NOREF(pszSel);
#endif
}


/**
 * Formats a full register dump.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   fFlags  The dumping flags (HM_DUMP_REG_FLAGS_XXX).
 */
VMMR0_INT_DECL(void) hmR0DumpRegs(PVMCPUCC pVCpu, uint32_t fFlags)
{
    /*
     * Format the flags.
     */
    static struct
    {
        const char *pszSet;
        const char *pszClear;
        uint32_t    fFlag;
    } const s_aFlags[] =
    {
        { "vip", NULL, X86_EFL_VIP },
        { "vif", NULL, X86_EFL_VIF },
        { "ac",  NULL, X86_EFL_AC  },
        { "vm",  NULL, X86_EFL_VM  },
        { "rf",  NULL, X86_EFL_RF  },
        { "nt",  NULL, X86_EFL_NT  },
        { "ov",  "nv", X86_EFL_OF  },
        { "dn",  "up", X86_EFL_DF  },
        { "ei",  "di", X86_EFL_IF  },
        { "tf",  NULL, X86_EFL_TF  },
        { "nt",  "pl", X86_EFL_SF  },
        { "nz",  "zr", X86_EFL_ZF  },
        { "ac",  "na", X86_EFL_AF  },
        { "po",  "pe", X86_EFL_PF  },
        { "cy",  "nc", X86_EFL_CF  },
    };
    char szEFlags[80];
    char *psz = szEFlags;
    PCCPUMCTX pCtx = &pVCpu->cpum.GstCtx;
    uint32_t uEFlags = pCtx->eflags.u32;
    for (unsigned i = 0; i < RT_ELEMENTS(s_aFlags); i++)
    {
        const char *pszAdd = s_aFlags[i].fFlag & uEFlags ? s_aFlags[i].pszSet : s_aFlags[i].pszClear;
        if (pszAdd)
        {
            strcpy(psz, pszAdd);
            psz += strlen(pszAdd);
            *psz++ = ' ';
        }
    }
    psz[-1] = '\0';

    if (fFlags & HM_DUMP_REG_FLAGS_GPRS)
    {
        /*
         * Format the registers.
         */
        if (CPUMIsGuestIn64BitCode(pVCpu))
        {
            Log(("rax=%016RX64 rbx=%016RX64 rcx=%016RX64 rdx=%016RX64\n"
                 "rsi=%016RX64 rdi=%016RX64 r8 =%016RX64 r9 =%016RX64\n"
                 "r10=%016RX64 r11=%016RX64 r12=%016RX64 r13=%016RX64\n"
                 "r14=%016RX64 r15=%016RX64\n"
                 "rip=%016RX64 rsp=%016RX64 rbp=%016RX64 iopl=%d %*s\n"
                 "cs={%04x base=%016RX64 limit=%08x flags=%08x}\n"
                 "ds={%04x base=%016RX64 limit=%08x flags=%08x}\n"
                 "es={%04x base=%016RX64 limit=%08x flags=%08x}\n"
                 "fs={%04x base=%016RX64 limit=%08x flags=%08x}\n"
                 "gs={%04x base=%016RX64 limit=%08x flags=%08x}\n"
                 "ss={%04x base=%016RX64 limit=%08x flags=%08x}\n"
                 "cr0=%016RX64 cr2=%016RX64 cr3=%016RX64 cr4=%016RX64\n"
                 "dr0=%016RX64 dr1=%016RX64 dr2=%016RX64 dr3=%016RX64\n"
                 "dr4=%016RX64 dr5=%016RX64 dr6=%016RX64 dr7=%016RX64\n"
                 "gdtr=%016RX64:%04x  idtr=%016RX64:%04x  eflags=%08x\n"
                 "ldtr={%04x base=%08RX64 limit=%08x flags=%08x}\n"
                 "tr  ={%04x base=%08RX64 limit=%08x flags=%08x}\n"
                 "SysEnter={cs=%04llx eip=%08llx esp=%08llx}\n"
                 ,
                 pCtx->rax, pCtx->rbx, pCtx->rcx, pCtx->rdx, pCtx->rsi, pCtx->rdi,
                 pCtx->r8, pCtx->r9, pCtx->r10, pCtx->r11, pCtx->r12, pCtx->r13,
                 pCtx->r14, pCtx->r15,
                 pCtx->rip, pCtx->rsp, pCtx->rbp, X86_EFL_GET_IOPL(uEFlags), 31, szEFlags,
                 pCtx->cs.Sel, pCtx->cs.u64Base, pCtx->cs.u32Limit, pCtx->cs.Attr.u,
                 pCtx->ds.Sel, pCtx->ds.u64Base, pCtx->ds.u32Limit, pCtx->ds.Attr.u,
                 pCtx->es.Sel, pCtx->es.u64Base, pCtx->es.u32Limit, pCtx->es.Attr.u,
                 pCtx->fs.Sel, pCtx->fs.u64Base, pCtx->fs.u32Limit, pCtx->fs.Attr.u,
                 pCtx->gs.Sel, pCtx->gs.u64Base, pCtx->gs.u32Limit, pCtx->gs.Attr.u,
                 pCtx->ss.Sel, pCtx->ss.u64Base, pCtx->ss.u32Limit, pCtx->ss.Attr.u,
                 pCtx->cr0,  pCtx->cr2, pCtx->cr3,  pCtx->cr4,
                 pCtx->dr[0],  pCtx->dr[1], pCtx->dr[2],  pCtx->dr[3],
                 pCtx->dr[4],  pCtx->dr[5], pCtx->dr[6],  pCtx->dr[7],
                 pCtx->gdtr.pGdt, pCtx->gdtr.cbGdt, pCtx->idtr.pIdt, pCtx->idtr.cbIdt, uEFlags,
                 pCtx->ldtr.Sel, pCtx->ldtr.u64Base, pCtx->ldtr.u32Limit, pCtx->ldtr.Attr.u,
                 pCtx->tr.Sel, pCtx->tr.u64Base, pCtx->tr.u32Limit, pCtx->tr.Attr.u,
                 pCtx->SysEnter.cs, pCtx->SysEnter.eip, pCtx->SysEnter.esp));
        }
        else
            Log(("eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x\n"
                 "eip=%08x esp=%08x ebp=%08x iopl=%d %*s\n"
                 "cs={%04x base=%016RX64 limit=%08x flags=%08x} dr0=%08RX64 dr1=%08RX64\n"
                 "ds={%04x base=%016RX64 limit=%08x flags=%08x} dr2=%08RX64 dr3=%08RX64\n"
                 "es={%04x base=%016RX64 limit=%08x flags=%08x} dr4=%08RX64 dr5=%08RX64\n"
                 "fs={%04x base=%016RX64 limit=%08x flags=%08x} dr6=%08RX64 dr7=%08RX64\n"
                 "gs={%04x base=%016RX64 limit=%08x flags=%08x} cr0=%08RX64 cr2=%08RX64\n"
                 "ss={%04x base=%016RX64 limit=%08x flags=%08x} cr3=%08RX64 cr4=%08RX64\n"
                 "gdtr=%016RX64:%04x  idtr=%016RX64:%04x  eflags=%08x\n"
                 "ldtr={%04x base=%08RX64 limit=%08x flags=%08x}\n"
                 "tr  ={%04x base=%08RX64 limit=%08x flags=%08x}\n"
                 "SysEnter={cs=%04llx eip=%08llx esp=%08llx}\n"
                 ,
                 pCtx->eax, pCtx->ebx, pCtx->ecx, pCtx->edx, pCtx->esi, pCtx->edi,
                 pCtx->eip, pCtx->esp, pCtx->ebp, X86_EFL_GET_IOPL(uEFlags), 31, szEFlags,
                 pCtx->cs.Sel, pCtx->cs.u64Base, pCtx->cs.u32Limit, pCtx->cs.Attr.u, pCtx->dr[0],  pCtx->dr[1],
                 pCtx->ds.Sel, pCtx->ds.u64Base, pCtx->ds.u32Limit, pCtx->ds.Attr.u, pCtx->dr[2],  pCtx->dr[3],
                 pCtx->es.Sel, pCtx->es.u64Base, pCtx->es.u32Limit, pCtx->es.Attr.u, pCtx->dr[4],  pCtx->dr[5],
                 pCtx->fs.Sel, pCtx->fs.u64Base, pCtx->fs.u32Limit, pCtx->fs.Attr.u, pCtx->dr[6],  pCtx->dr[7],
                 pCtx->gs.Sel, pCtx->gs.u64Base, pCtx->gs.u32Limit, pCtx->gs.Attr.u, pCtx->cr0,  pCtx->cr2,
                 pCtx->ss.Sel, pCtx->ss.u64Base, pCtx->ss.u32Limit, pCtx->ss.Attr.u, pCtx->cr3,  pCtx->cr4,
                 pCtx->gdtr.pGdt, pCtx->gdtr.cbGdt, pCtx->idtr.pIdt, pCtx->idtr.cbIdt, uEFlags,
                 pCtx->ldtr.Sel, pCtx->ldtr.u64Base, pCtx->ldtr.u32Limit, pCtx->ldtr.Attr.u,
                 pCtx->tr.Sel, pCtx->tr.u64Base, pCtx->tr.u32Limit, pCtx->tr.Attr.u,
                 pCtx->SysEnter.cs, pCtx->SysEnter.eip, pCtx->SysEnter.esp));
    }

    if (fFlags & HM_DUMP_REG_FLAGS_FPU)
    {
        PCX86FXSTATE pFpuCtx = &pCtx->CTX_SUFF(pXState)->x87;
        Log(("FPU:\n"
            "FCW=%04x FSW=%04x FTW=%02x\n"
            "FOP=%04x FPUIP=%08x CS=%04x Rsrvd1=%04x\n"
            "FPUDP=%04x DS=%04x Rsvrd2=%04x MXCSR=%08x MXCSR_MASK=%08x\n"
            ,
            pFpuCtx->FCW,   pFpuCtx->FSW,   pFpuCtx->FTW,
            pFpuCtx->FOP,   pFpuCtx->FPUIP, pFpuCtx->CS, pFpuCtx->Rsrvd1,
            pFpuCtx->FPUDP, pFpuCtx->DS,    pFpuCtx->Rsrvd2,
            pFpuCtx->MXCSR, pFpuCtx->MXCSR_MASK));
        NOREF(pFpuCtx);
    }

    if (fFlags & HM_DUMP_REG_FLAGS_MSRS)
    {
        Log(("MSR:\n"
            "EFER         =%016RX64\n"
            "PAT          =%016RX64\n"
            "STAR         =%016RX64\n"
            "CSTAR        =%016RX64\n"
            "LSTAR        =%016RX64\n"
            "SFMASK       =%016RX64\n"
            "KERNELGSBASE =%016RX64\n",
            pCtx->msrEFER,
            pCtx->msrPAT,
            pCtx->msrSTAR,
            pCtx->msrCSTAR,
            pCtx->msrLSTAR,
            pCtx->msrSFMASK,
            pCtx->msrKERNELGSBASE));
    }
}

#endif /* VBOX_STRICT */


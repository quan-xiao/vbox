/* $Id: NEMR0Native-win.cpp 86055 2020-09-07 19:58:13Z vboxsync $ */
/** @file
 * NEM - Native execution manager, native ring-0 Windows backend.
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
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_NEM
#define VMCPU_INCL_CPUM_GST_CTX
#include <iprt/nt/nt.h>
#include <iprt/nt/hyperv.h>
#include <iprt/nt/vid.h>
#include <winerror.h>

#include <VBox/vmm/nem.h>
#include <VBox/vmm/iem.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/apic.h>
#include <VBox/vmm/pdm.h>
#include <VBox/vmm/dbgftrace.h>
#include "NEMInternal.h"
#include <VBox/vmm/gvm.h>
#include <VBox/vmm/vmcc.h>
#include <VBox/vmm/gvmm.h>
#include <VBox/param.h>

#include <iprt/dbg.h>
#include <iprt/memobj.h>
#include <iprt/string.h>
#include <iprt/time.h>


/* Assert compile context sanity. */
#ifndef RT_OS_WINDOWS
# error "Windows only file!"
#endif
#ifndef RT_ARCH_AMD64
# error "AMD64 only file!"
#endif


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
typedef uint32_t DWORD; /* for winerror.h constants */


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static uint64_t (*g_pfnHvlInvokeHypercall)(uint64_t uCallInfo, uint64_t HCPhysInput, uint64_t HCPhysOutput);

/**
 * WinHvr.sys!WinHvDepositMemory
 *
 * This API will try allocates cPages on IdealNode and deposit it to the
 * hypervisor for use with the given partition.  The memory will be freed when
 * VID.SYS calls WinHvWithdrawAllMemory when the partition is cleanedup.
 *
 * Apparently node numbers above 64 has a different meaning.
 */
static NTSTATUS (*g_pfnWinHvDepositMemory)(uintptr_t idPartition, size_t cPages, uintptr_t IdealNode, size_t *pcActuallyAdded);


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
NEM_TMPL_STATIC int  nemR0WinMapPages(PGVM pGVM, PGVMCPU pGVCpu, RTGCPHYS GCPhysSrc, RTGCPHYS GCPhysDst,
                                      uint32_t cPages, uint32_t fFlags);
NEM_TMPL_STATIC int  nemR0WinUnmapPages(PGVM pGVM, PGVMCPU pGVCpu, RTGCPHYS GCPhys, uint32_t cPages);
#if defined(NEM_WIN_WITH_RING0_RUNLOOP) || defined(NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS)
NEM_TMPL_STATIC int  nemR0WinExportState(PGVM pGVM, PGVMCPU pGVCpu, PCPUMCTX pCtx);
NEM_TMPL_STATIC int  nemR0WinImportState(PGVM pGVM, PGVMCPU pGVCpu, PCPUMCTX pCtx, uint64_t fWhat, bool fCanUpdateCr3);
NEM_TMPL_STATIC int  nemR0WinQueryCpuTick(PGVM pGVM, PGVMCPU pGVCpu, uint64_t *pcTicks, uint32_t *pcAux);
NEM_TMPL_STATIC int  nemR0WinResumeCpuTickOnAll(PGVM pGVM, PGVMCPU pGVCpu, uint64_t uPausedTscValue);
#endif
DECLINLINE(NTSTATUS) nemR0NtPerformIoControl(PGVM pGVM, PGVMCPU pGVCpu, uint32_t uFunction, void *pvInput, uint32_t cbInput,
                                             void *pvOutput, uint32_t cbOutput);


/*
 * Instantate the code we share with ring-0.
 */
#ifdef NEM_WIN_WITH_RING0_RUNLOOP
# define NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
#else
# undef NEM_WIN_TEMPLATE_MODE_OWN_RUN_API
#endif
#include "../VMMAll/NEMAllNativeTemplate-win.cpp.h"



/**
 * Worker for NEMR0InitVM that allocates a hypercall page.
 *
 * @returns VBox status code.
 * @param   pHypercallData  The hypercall data page to initialize.
 */
static int nemR0InitHypercallData(PNEMR0HYPERCALLDATA pHypercallData)
{
    int rc = RTR0MemObjAllocPage(&pHypercallData->hMemObj, PAGE_SIZE, false /*fExecutable*/);
    if (RT_SUCCESS(rc))
    {
        pHypercallData->HCPhysPage = RTR0MemObjGetPagePhysAddr(pHypercallData->hMemObj, 0 /*iPage*/);
        AssertStmt(pHypercallData->HCPhysPage != NIL_RTHCPHYS, rc = VERR_INTERNAL_ERROR_3);
        pHypercallData->pbPage     = (uint8_t *)RTR0MemObjAddress(pHypercallData->hMemObj);
        AssertStmt(pHypercallData->pbPage, rc = VERR_INTERNAL_ERROR_3);
        if (RT_SUCCESS(rc))
            return VINF_SUCCESS;

        /* bail out */
        RTR0MemObjFree(pHypercallData->hMemObj, true /*fFreeMappings*/);
    }
    pHypercallData->hMemObj     = NIL_RTR0MEMOBJ;
    pHypercallData->HCPhysPage  = NIL_RTHCPHYS;
    pHypercallData->pbPage      = NULL;
    return rc;
}

/**
 * Worker for NEMR0CleanupVM and NEMR0InitVM that cleans up a hypercall page.
 *
 * @param   pHypercallData  The hypercall data page to uninitialize.
 */
static void nemR0DeleteHypercallData(PNEMR0HYPERCALLDATA pHypercallData)
{
    /* Check pbPage here since it's NULL, whereas the hMemObj can be either
       NIL_RTR0MEMOBJ or 0 (they aren't necessarily the same). */
    if (pHypercallData->pbPage != NULL)
    {
        RTR0MemObjFree(pHypercallData->hMemObj, true /*fFreeMappings*/);
        pHypercallData->pbPage = NULL;
    }
    pHypercallData->hMemObj    = NIL_RTR0MEMOBJ;
    pHypercallData->HCPhysPage = NIL_RTHCPHYS;
}


/**
 * Called by NEMR3Init to make sure we've got what we need.
 *
 * @returns VBox status code.
 * @param   pGVM            The ring-0 VM handle.
 * @thread  EMT(0)
 */
VMMR0_INT_DECL(int) NEMR0InitVM(PGVM pGVM)
{
    AssertCompile(sizeof(pGVM->nemr0.s) <= sizeof(pGVM->nemr0.padding));
    AssertCompile(sizeof(pGVM->aCpus[0].nemr0.s) <= sizeof(pGVM->aCpus[0].nemr0.padding));

    int rc = GVMMR0ValidateGVMandEMT(pGVM, 0);
    AssertRCReturn(rc, rc);

    /*
     * We want to perform hypercalls here.  The NT kernel started to expose a very low
     * level interface to do this thru somewhere between build 14271 and 16299.  Since
     * we need build 17134 to get anywhere at all, the exact build is not relevant here.
     *
     * We also need to deposit memory to the hypervisor for use with partition (page
     * mapping structures, stuff).
     */
    RTDBGKRNLINFO hKrnlInfo;
    rc = RTR0DbgKrnlInfoOpen(&hKrnlInfo, 0);
    if (RT_SUCCESS(rc))
    {
        rc = RTR0DbgKrnlInfoQuerySymbol(hKrnlInfo, NULL, "HvlInvokeHypercall", (void **)&g_pfnHvlInvokeHypercall);
        if (RT_FAILURE(rc))
            rc = VERR_NEM_MISSING_KERNEL_API_1;
        if (RT_SUCCESS(rc))
        {
            rc = RTR0DbgKrnlInfoQuerySymbol(hKrnlInfo, "winhvr.sys", "WinHvDepositMemory", (void **)&g_pfnWinHvDepositMemory);
            if (RT_FAILURE(rc))
                rc = rc == VERR_MODULE_NOT_FOUND ? VERR_NEM_MISSING_KERNEL_API_2 : VERR_NEM_MISSING_KERNEL_API_3;
        }
        RTR0DbgKrnlInfoRelease(hKrnlInfo);
        if (RT_SUCCESS(rc))
        {
            /*
             * Allocate a page for non-EMT threads to use for hypercalls (update
             * statistics and such) and a critical section protecting it.
             */
            rc = RTCritSectInit(&pGVM->nemr0.s.HypercallDataCritSect);
            if (RT_SUCCESS(rc))
            {
                rc = nemR0InitHypercallData(&pGVM->nemr0.s.HypercallData);
                if (RT_SUCCESS(rc))
                {
                    /*
                     * Allocate a page for each VCPU to place hypercall data on.
                     */
                    for (VMCPUID i = 0; i < pGVM->cCpus; i++)
                    {
                        rc = nemR0InitHypercallData(&pGVM->aCpus[i].nemr0.s.HypercallData);
                        if (RT_FAILURE(rc))
                        {
                            while (i-- > 0)
                                nemR0DeleteHypercallData(&pGVM->aCpus[i].nemr0.s.HypercallData);
                            break;
                        }
                    }
                    if (RT_SUCCESS(rc))
                    {
                        /*
                         * So far, so good.
                         */
                        return rc;
                    }

                    /*
                     * Bail out.
                     */
                    nemR0DeleteHypercallData(&pGVM->nemr0.s.HypercallData);
                }
                RTCritSectDelete(&pGVM->nemr0.s.HypercallDataCritSect);
            }
        }
    }

    return rc;
}


/**
 * Perform an I/O control operation on the partition handle (VID.SYS).
 *
 * @returns NT status code.
 * @param   pGVM            The ring-0 VM structure.
 * @param   pGVCpu          The global (ring-0) CPU structure of the calling EMT.
 * @param   uFunction       The function to perform.
 * @param   pvInput         The input buffer.  This must point within the VM
 *                          structure so we can easily convert to a ring-3
 *                          pointer if necessary.
 * @param   cbInput         The size of the input.  @a pvInput must be NULL when
 *                          zero.
 * @param   pvOutput        The output buffer.  This must also point within the
 *                          VM structure for ring-3 pointer magic.
 * @param   cbOutput        The size of the output.  @a pvOutput must be NULL
 *                          when zero.
 * @thread  EMT(pGVCpu)
 */
DECLINLINE(NTSTATUS) nemR0NtPerformIoControl(PGVM pGVM, PGVMCPU pGVCpu, uint32_t uFunction, void *pvInput, uint32_t cbInput,
                                             void *pvOutput, uint32_t cbOutput)
{
#ifdef RT_STRICT
    /*
     * Input and output parameters are part of the VM CPU structure.
     */
    VMCPU_ASSERT_EMT(pGVCpu);
    if (pvInput)
        AssertReturn(((uintptr_t)pvInput + cbInput) - (uintptr_t)pGVCpu <= sizeof(*pGVCpu), VERR_INVALID_PARAMETER);
    if (pvOutput)
        AssertReturn(((uintptr_t)pvOutput + cbOutput) - (uintptr_t)pGVCpu <= sizeof(*pGVCpu), VERR_INVALID_PARAMETER);
#endif

    int32_t rcNt = STATUS_UNSUCCESSFUL;
    int rc = SUPR0IoCtlPerform(pGVM->nemr0.s.pIoCtlCtx, uFunction,
                               pvInput,
                               pvInput  ? (uintptr_t)pvInput  + pGVCpu->nemr0.s.offRing3ConversionDelta : NIL_RTR3PTR,
                               cbInput,
                               pvOutput,
                               pvOutput ? (uintptr_t)pvOutput + pGVCpu->nemr0.s.offRing3ConversionDelta : NIL_RTR3PTR,
                               cbOutput,
                               &rcNt);
    if (RT_SUCCESS(rc) || !NT_SUCCESS((NTSTATUS)rcNt))
        return (NTSTATUS)rcNt;
    return STATUS_UNSUCCESSFUL;
}


/**
 * 2nd part of the initialization, after we've got a partition handle.
 *
 * @returns VBox status code.
 * @param   pGVM            The ring-0 VM handle.
 * @thread  EMT(0)
 */
VMMR0_INT_DECL(int) NEMR0InitVMPart2(PGVM pGVM)
{
    int rc = GVMMR0ValidateGVMandEMT(pGVM, 0);
    AssertRCReturn(rc, rc);
    SUPR0Printf("NEMR0InitVMPart2\n"); LogRel(("2: NEMR0InitVMPart2\n"));
    Assert(pGVM->nemr0.s.fMayUseRing0Runloop == false);

    /*
     * Copy and validate the I/O control information from ring-3.
     */
    NEMWINIOCTL Copy = pGVM->nem.s.IoCtlGetHvPartitionId;
    AssertLogRelReturn(Copy.uFunction != 0, VERR_NEM_INIT_FAILED);
    AssertLogRelReturn(Copy.cbInput == 0, VERR_NEM_INIT_FAILED);
    AssertLogRelReturn(Copy.cbOutput == sizeof(HV_PARTITION_ID), VERR_NEM_INIT_FAILED);
    pGVM->nemr0.s.IoCtlGetHvPartitionId = Copy;

    pGVM->nemr0.s.fMayUseRing0Runloop = pGVM->nem.s.fUseRing0Runloop;

    Copy = pGVM->nem.s.IoCtlStartVirtualProcessor;
    AssertLogRelStmt(Copy.uFunction != 0, rc = VERR_NEM_INIT_FAILED);
    AssertLogRelStmt(Copy.cbInput == sizeof(HV_VP_INDEX), rc = VERR_NEM_INIT_FAILED);
    AssertLogRelStmt(Copy.cbOutput == 0, rc = VERR_NEM_INIT_FAILED);
    AssertLogRelStmt(Copy.uFunction != pGVM->nemr0.s.IoCtlGetHvPartitionId.uFunction, rc = VERR_NEM_INIT_FAILED);
    if (RT_SUCCESS(rc))
        pGVM->nemr0.s.IoCtlStartVirtualProcessor = Copy;

    Copy = pGVM->nem.s.IoCtlStopVirtualProcessor;
    AssertLogRelStmt(Copy.uFunction != 0, rc = VERR_NEM_INIT_FAILED);
    AssertLogRelStmt(Copy.cbInput == sizeof(HV_VP_INDEX), rc = VERR_NEM_INIT_FAILED);
    AssertLogRelStmt(Copy.cbOutput == 0, rc = VERR_NEM_INIT_FAILED);
    AssertLogRelStmt(Copy.uFunction != pGVM->nemr0.s.IoCtlGetHvPartitionId.uFunction, rc = VERR_NEM_INIT_FAILED);
    AssertLogRelStmt(Copy.uFunction != pGVM->nemr0.s.IoCtlStartVirtualProcessor.uFunction, rc = VERR_NEM_INIT_FAILED);
    if (RT_SUCCESS(rc))
        pGVM->nemr0.s.IoCtlStopVirtualProcessor = Copy;

    Copy = pGVM->nem.s.IoCtlMessageSlotHandleAndGetNext;
    AssertLogRelStmt(Copy.uFunction != 0, rc = VERR_NEM_INIT_FAILED);
    AssertLogRelStmt(   Copy.cbInput == sizeof(VID_IOCTL_INPUT_MESSAGE_SLOT_HANDLE_AND_GET_NEXT)
                     || Copy.cbInput == RT_OFFSETOF(VID_IOCTL_INPUT_MESSAGE_SLOT_HANDLE_AND_GET_NEXT, cMillies),
                     rc = VERR_NEM_INIT_FAILED);
    AssertLogRelStmt(Copy.cbOutput == 0, VERR_NEM_INIT_FAILED);
    AssertLogRelStmt(Copy.uFunction != pGVM->nemr0.s.IoCtlGetHvPartitionId.uFunction, rc = VERR_NEM_INIT_FAILED);
    AssertLogRelStmt(Copy.uFunction != pGVM->nemr0.s.IoCtlStartVirtualProcessor.uFunction, rc = VERR_NEM_INIT_FAILED);
    AssertLogRelStmt(Copy.uFunction != pGVM->nemr0.s.IoCtlStopVirtualProcessor.uFunction, rc = VERR_NEM_INIT_FAILED);
    if (RT_SUCCESS(rc))
        pGVM->nemr0.s.IoCtlMessageSlotHandleAndGetNext = Copy;

    if (   RT_SUCCESS(rc)
        || !pGVM->nem.s.fUseRing0Runloop)
    {
        /*
         * Setup of an I/O control context for the partition handle for later use.
         */
        rc = SUPR0IoCtlSetupForHandle(pGVM->pSession, pGVM->nem.s.hPartitionDevice, 0, &pGVM->nemr0.s.pIoCtlCtx);
        AssertLogRelRCReturn(rc, rc);
        for (VMCPUID idCpu = 0; idCpu < pGVM->cCpus; idCpu++)
        {
            PGVMCPU pGVCpu = &pGVM->aCpus[idCpu];
            pGVCpu->nemr0.s.offRing3ConversionDelta = (uintptr_t)pGVM->aCpus[idCpu].pVCpuR3 - (uintptr_t)pGVCpu;
        }

        /*
         * Get the partition ID.
         */
        PVMCPUCC pVCpu0 = &pGVM->aCpus[0];
        NTSTATUS rcNt = nemR0NtPerformIoControl(pGVM, pVCpu0, pGVM->nemr0.s.IoCtlGetHvPartitionId.uFunction, NULL, 0,
                                                &pVCpu0->nem.s.uIoCtlBuf.idPartition, sizeof(pVCpu0->nem.s.uIoCtlBuf.idPartition));
        AssertLogRelMsgReturn(NT_SUCCESS(rcNt), ("IoCtlGetHvPartitionId failed: %#x\n", rcNt), VERR_NEM_INIT_FAILED);
        pGVM->nemr0.s.idHvPartition = pVCpu0->nem.s.uIoCtlBuf.idPartition;
        AssertLogRelMsgReturn(pGVM->nemr0.s.idHvPartition == pGVM->nem.s.idHvPartition,
                              ("idHvPartition mismatch: r0=%#RX64, r3=%#RX64\n", pGVM->nemr0.s.idHvPartition, pGVM->nem.s.idHvPartition),
                              VERR_NEM_INIT_FAILED);
    }

    return rc;
}


/**
 * Cleanup the NEM parts of the VM in ring-0.
 *
 * This is always called and must deal the state regardless of whether
 * NEMR0InitVM() was called or not.  So, take care here.
 *
 * @param   pGVM            The ring-0 VM handle.
 */
VMMR0_INT_DECL(void) NEMR0CleanupVM(PGVM pGVM)
{
    pGVM->nemr0.s.idHvPartition = HV_PARTITION_ID_INVALID;

    /* Clean up I/O control context. */
    if (pGVM->nemr0.s.pIoCtlCtx)
    {
        int rc = SUPR0IoCtlCleanup(pGVM->nemr0.s.pIoCtlCtx);
        AssertRC(rc);
        pGVM->nemr0.s.pIoCtlCtx = NULL;
    }

    /* Free the hypercall pages. */
    VMCPUID i = pGVM->cCpus;
    while (i-- > 0)
        nemR0DeleteHypercallData(&pGVM->aCpus[i].nemr0.s.HypercallData);

    /* The non-EMT one too. */
    if (RTCritSectIsInitialized(&pGVM->nemr0.s.HypercallDataCritSect))
        RTCritSectDelete(&pGVM->nemr0.s.HypercallDataCritSect);
    nemR0DeleteHypercallData(&pGVM->nemr0.s.HypercallData);
}


#if 0 /* for debugging GPA unmapping.  */
static int nemR3WinDummyReadGpa(PGVM pGVM, PGVMCPU pGVCpu, RTGCPHYS GCPhys)
{
    PHV_INPUT_READ_GPA  pIn  = (PHV_INPUT_READ_GPA)pGVCpu->nemr0.s.pbHypercallData;
    PHV_OUTPUT_READ_GPA pOut = (PHV_OUTPUT_READ_GPA)(pIn + 1);
    pIn->PartitionId            = pGVM->nemr0.s.idHvPartition;
    pIn->VpIndex                = pGVCpu->idCpu;
    pIn->ByteCount              = 0x10;
    pIn->BaseGpa                = GCPhys;
    pIn->ControlFlags.AsUINT64  = 0;
    pIn->ControlFlags.CacheType = HvCacheTypeX64WriteCombining;
    memset(pOut, 0xfe, sizeof(*pOut));
    uint64_t volatile uResult = g_pfnHvlInvokeHypercall(HvCallReadGpa, pGVCpu->nemr0.s.HCPhysHypercallData,
                                                        pGVCpu->nemr0.s.HCPhysHypercallData + sizeof(*pIn));
    LogRel(("nemR3WinDummyReadGpa: %RGp -> %#RX64; code=%u rsvd=%u abData=%.16Rhxs\n",
            GCPhys, uResult, pOut->AccessResult.ResultCode, pOut->AccessResult.Reserved, pOut->Data));
    __debugbreak();

    return uResult != 0 ? VERR_READ_ERROR : VINF_SUCCESS;
}
#endif


/**
 * Worker for NEMR0MapPages and others.
 */
NEM_TMPL_STATIC int nemR0WinMapPages(PGVM pGVM, PGVMCPU pGVCpu, RTGCPHYS GCPhysSrc, RTGCPHYS GCPhysDst,
                                     uint32_t cPages, uint32_t fFlags)
{
    /*
     * Validate.
     */
    AssertReturn(g_pfnHvlInvokeHypercall, VERR_NEM_MISSING_KERNEL_API_1);

    AssertReturn(cPages > 0, VERR_OUT_OF_RANGE);
    AssertReturn(cPages <= NEM_MAX_MAP_PAGES, VERR_OUT_OF_RANGE);
    AssertReturn(!(fFlags & ~(HV_MAP_GPA_MAYBE_ACCESS_MASK & ~HV_MAP_GPA_DUNNO_ACCESS)), VERR_INVALID_FLAGS);
    AssertMsgReturn(!(GCPhysDst & X86_PAGE_OFFSET_MASK), ("GCPhysDst=%RGp\n", GCPhysDst), VERR_OUT_OF_RANGE);
    AssertReturn(GCPhysDst < _1E, VERR_OUT_OF_RANGE);
    if (GCPhysSrc != GCPhysDst)
    {
        AssertMsgReturn(!(GCPhysSrc & X86_PAGE_OFFSET_MASK), ("GCPhysSrc=%RGp\n", GCPhysSrc), VERR_OUT_OF_RANGE);
        AssertReturn(GCPhysSrc < _1E, VERR_OUT_OF_RANGE);
    }

    /*
     * Compose and make the hypercall.
     * Ring-3 is not allowed to fill in the host physical addresses of the call.
     */
    for (uint32_t iTries = 0;; iTries++)
    {
        RTGCPHYS GCPhysSrcTmp = GCPhysSrc;
        HV_INPUT_MAP_GPA_PAGES *pMapPages = (HV_INPUT_MAP_GPA_PAGES *)pGVCpu->nemr0.s.HypercallData.pbPage;
        AssertPtrReturn(pMapPages, VERR_INTERNAL_ERROR_3);
        pMapPages->TargetPartitionId    = pGVM->nemr0.s.idHvPartition;
        pMapPages->TargetGpaBase        = GCPhysDst >> X86_PAGE_SHIFT;
        pMapPages->MapFlags             = fFlags;
        pMapPages->u32ExplicitPadding   = 0;

        for (uint32_t iPage = 0; iPage < cPages; iPage++, GCPhysSrcTmp += X86_PAGE_SIZE)
        {
            RTHCPHYS HCPhys = NIL_RTGCPHYS;
            int rc = PGMPhysGCPhys2HCPhys(pGVM, GCPhysSrcTmp, &HCPhys);
            AssertRCReturn(rc, rc);
            pMapPages->PageList[iPage] = HCPhys >> X86_PAGE_SHIFT;
        }

        uint64_t uResult = g_pfnHvlInvokeHypercall(HvCallMapGpaPages | ((uint64_t)cPages << 32),
                                                   pGVCpu->nemr0.s.HypercallData.HCPhysPage, 0);
        Log6(("NEMR0MapPages: %RGp/%RGp L %u prot %#x -> %#RX64\n",
              GCPhysDst, GCPhysSrcTmp - cPages * X86_PAGE_SIZE, cPages, fFlags, uResult));
        if (uResult == ((uint64_t)cPages << 32))
            return VINF_SUCCESS;

        /*
         * If the partition is out of memory, try donate another 512 pages to
         * it (2MB).  VID.SYS does multiples of 512 pages, nothing smaller.
         */
        if (   uResult != HV_STATUS_INSUFFICIENT_MEMORY
            || iTries > 16
            || g_pfnWinHvDepositMemory == NULL)
        {
            LogRel(("g_pfnHvlInvokeHypercall/MapGpaPages -> %#RX64\n", uResult));
            return VERR_NEM_MAP_PAGES_FAILED;
        }

        size_t cPagesAdded = 0;
        NTSTATUS rcNt = g_pfnWinHvDepositMemory(pGVM->nemr0.s.idHvPartition, 512, 0, &cPagesAdded);
        if (!cPagesAdded)
        {
            LogRel(("g_pfnWinHvDepositMemory -> %#x / %#RX64\n", rcNt, uResult));
            return VERR_NEM_MAP_PAGES_FAILED;
        }
    }
}


/**
 * Maps pages into the guest physical address space.
 *
 * Generally the caller will be under the PGM lock already, so no extra effort
 * is needed to make sure all changes happens under it.
 *
 * @returns VBox status code.
 * @param   pGVM            The ring-0 VM handle.
 * @param   idCpu           The calling EMT.  Necessary for getting the
 *                          hypercall page and arguments.
 * @thread  EMT(idCpu)
 */
VMMR0_INT_DECL(int) NEMR0MapPages(PGVM pGVM, VMCPUID idCpu)
{
    /*
     * Unpack the call.
     */
    int rc = GVMMR0ValidateGVMandEMT(pGVM, idCpu);
    if (RT_SUCCESS(rc))
    {
        PGVMCPU pGVCpu = &pGVM->aCpus[idCpu];

        RTGCPHYS const          GCPhysSrc = pGVCpu->nem.s.Hypercall.MapPages.GCPhysSrc;
        RTGCPHYS const          GCPhysDst = pGVCpu->nem.s.Hypercall.MapPages.GCPhysDst;
        uint32_t const          cPages    = pGVCpu->nem.s.Hypercall.MapPages.cPages;
        HV_MAP_GPA_FLAGS const  fFlags    = pGVCpu->nem.s.Hypercall.MapPages.fFlags;

        /*
         * Do the work.
         */
        rc = nemR0WinMapPages(pGVM, pGVCpu, GCPhysSrc, GCPhysDst, cPages, fFlags);
    }
    return rc;
}


/**
 * Worker for NEMR0UnmapPages and others.
 */
NEM_TMPL_STATIC int nemR0WinUnmapPages(PGVM pGVM, PGVMCPU pGVCpu, RTGCPHYS GCPhys, uint32_t cPages)
{
    /*
     * Validate input.
     */
    AssertReturn(g_pfnHvlInvokeHypercall, VERR_NEM_MISSING_KERNEL_API_1);

    AssertReturn(cPages > 0, VERR_OUT_OF_RANGE);
    AssertReturn(cPages <= NEM_MAX_UNMAP_PAGES, VERR_OUT_OF_RANGE);
    AssertMsgReturn(!(GCPhys & X86_PAGE_OFFSET_MASK), ("%RGp\n", GCPhys), VERR_OUT_OF_RANGE);
    AssertReturn(GCPhys < _1E, VERR_OUT_OF_RANGE);

    /*
     * Compose and make the hypercall.
     */
    HV_INPUT_UNMAP_GPA_PAGES *pUnmapPages = (HV_INPUT_UNMAP_GPA_PAGES *)pGVCpu->nemr0.s.HypercallData.pbPage;
    AssertPtrReturn(pUnmapPages, VERR_INTERNAL_ERROR_3);
    pUnmapPages->TargetPartitionId    = pGVM->nemr0.s.idHvPartition;
    pUnmapPages->TargetGpaBase        = GCPhys >> X86_PAGE_SHIFT;
    pUnmapPages->fFlags               = 0;

    uint64_t uResult = g_pfnHvlInvokeHypercall(HvCallUnmapGpaPages | ((uint64_t)cPages << 32),
                                               pGVCpu->nemr0.s.HypercallData.HCPhysPage, 0);
    Log6(("NEMR0UnmapPages: %RGp L %u -> %#RX64\n", GCPhys, cPages, uResult));
    if (uResult == ((uint64_t)cPages << 32))
    {
#if 1       /* Do we need to do this? Hopefully not... */
        uint64_t volatile uR = g_pfnHvlInvokeHypercall(HvCallUncommitGpaPages | ((uint64_t)cPages << 32),
                                                       pGVCpu->nemr0.s.HypercallData.HCPhysPage, 0);
        AssertMsg(uR == ((uint64_t)cPages << 32), ("uR=%#RX64\n", uR)); NOREF(uR);
#endif
        return VINF_SUCCESS;
    }

    LogRel(("g_pfnHvlInvokeHypercall/UnmapGpaPages -> %#RX64\n", uResult));
    return VERR_NEM_UNMAP_PAGES_FAILED;
}


/**
 * Unmaps pages from the guest physical address space.
 *
 * Generally the caller will be under the PGM lock already, so no extra effort
 * is needed to make sure all changes happens under it.
 *
 * @returns VBox status code.
 * @param   pGVM            The ring-0 VM handle.
 * @param   idCpu           The calling EMT.  Necessary for getting the
 *                          hypercall page and arguments.
 * @thread  EMT(idCpu)
 */
VMMR0_INT_DECL(int) NEMR0UnmapPages(PGVM pGVM, VMCPUID idCpu)
{
    /*
     * Unpack the call.
     */
    int rc = GVMMR0ValidateGVMandEMT(pGVM, idCpu);
    if (RT_SUCCESS(rc))
    {
        PGVMCPU pGVCpu = &pGVM->aCpus[idCpu];

        RTGCPHYS const GCPhys = pGVCpu->nem.s.Hypercall.UnmapPages.GCPhys;
        uint32_t const cPages = pGVCpu->nem.s.Hypercall.UnmapPages.cPages;

        /*
         * Do the work.
         */
        rc = nemR0WinUnmapPages(pGVM, pGVCpu, GCPhys, cPages);
    }
    return rc;
}


#if defined(NEM_WIN_WITH_RING0_RUNLOOP) || defined(NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS)
/**
 * Worker for NEMR0ExportState.
 *
 * Intention is to use it internally later.
 *
 * @returns VBox status code.
 * @param   pGVM        The ring-0 VM handle.
 * @param   pGVCpu      The ring-0 VCPU handle.
 * @param   pCtx        The CPU context structure to import into.
 */
NEM_TMPL_STATIC int nemR0WinExportState(PGVM pGVM, PGVMCPU pGVCpu, PCPUMCTX pCtx)
{
    HV_INPUT_SET_VP_REGISTERS *pInput = (HV_INPUT_SET_VP_REGISTERS *)pGVCpu->nemr0.s.HypercallData.pbPage;
    AssertPtrReturn(pInput, VERR_INTERNAL_ERROR_3);
    AssertReturn(g_pfnHvlInvokeHypercall, VERR_NEM_MISSING_KERNEL_API_1);

    pInput->PartitionId = pGVM->nemr0.s.idHvPartition;
    pInput->VpIndex     = pGVCpu->idCpu;
    pInput->RsvdZ       = 0;

    uint64_t const fWhat = ~pCtx->fExtrn & (CPUMCTX_EXTRN_ALL | CPUMCTX_EXTRN_NEM_WIN_MASK);
    if (   !fWhat
        && pGVCpu->nem.s.fCurrentInterruptWindows == pGVCpu->nem.s.fDesiredInterruptWindows)
        return VINF_SUCCESS;
    uintptr_t iReg = 0;

    /* GPRs */
    if (fWhat & CPUMCTX_EXTRN_GPRS_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_RAX)
        {
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                = HvX64RegisterRax;
            pInput->Elements[iReg].Value.Reg64         = pCtx->rax;
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_RCX)
        {
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                = HvX64RegisterRcx;
            pInput->Elements[iReg].Value.Reg64         = pCtx->rcx;
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_RDX)
        {
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                = HvX64RegisterRdx;
            pInput->Elements[iReg].Value.Reg64         = pCtx->rdx;
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_RBX)
        {
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                = HvX64RegisterRbx;
            pInput->Elements[iReg].Value.Reg64         = pCtx->rbx;
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_RSP)
        {
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                = HvX64RegisterRsp;
            pInput->Elements[iReg].Value.Reg64         = pCtx->rsp;
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_RBP)
        {
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                = HvX64RegisterRbp;
            pInput->Elements[iReg].Value.Reg64         = pCtx->rbp;
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_RSI)
        {
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                = HvX64RegisterRsi;
            pInput->Elements[iReg].Value.Reg64         = pCtx->rsi;
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_RDI)
        {
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                = HvX64RegisterRdi;
            pInput->Elements[iReg].Value.Reg64         = pCtx->rdi;
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_R8_R15)
        {
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                = HvX64RegisterR8;
            pInput->Elements[iReg].Value.Reg64         = pCtx->r8;
            iReg++;
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                = HvX64RegisterR9;
            pInput->Elements[iReg].Value.Reg64         = pCtx->r9;
            iReg++;
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                = HvX64RegisterR10;
            pInput->Elements[iReg].Value.Reg64         = pCtx->r10;
            iReg++;
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                = HvX64RegisterR11;
            pInput->Elements[iReg].Value.Reg64         = pCtx->r11;
            iReg++;
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                = HvX64RegisterR12;
            pInput->Elements[iReg].Value.Reg64         = pCtx->r12;
            iReg++;
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                = HvX64RegisterR13;
            pInput->Elements[iReg].Value.Reg64         = pCtx->r13;
            iReg++;
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                = HvX64RegisterR14;
            pInput->Elements[iReg].Value.Reg64         = pCtx->r14;
            iReg++;
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                = HvX64RegisterR15;
            pInput->Elements[iReg].Value.Reg64         = pCtx->r15;
            iReg++;
        }
    }

    /* RIP & Flags */
    if (fWhat & CPUMCTX_EXTRN_RIP)
    {
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                = HvX64RegisterRip;
        pInput->Elements[iReg].Value.Reg64         = pCtx->rip;
        iReg++;
    }
    if (fWhat & CPUMCTX_EXTRN_RFLAGS)
    {
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                = HvX64RegisterRflags;
        pInput->Elements[iReg].Value.Reg64         = pCtx->rflags.u;
        iReg++;
    }

    /* Segments */
# define COPY_OUT_SEG(a_idx, a_enmName, a_SReg) \
        do { \
            HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[a_idx]); \
            pInput->Elements[a_idx].Name                     = a_enmName; \
            pInput->Elements[a_idx].Value.Segment.Base       = (a_SReg).u64Base; \
            pInput->Elements[a_idx].Value.Segment.Limit      = (a_SReg).u32Limit; \
            pInput->Elements[a_idx].Value.Segment.Selector   = (a_SReg).Sel; \
            pInput->Elements[a_idx].Value.Segment.Attributes = (a_SReg).Attr.u; \
        } while (0)
    if (fWhat & CPUMCTX_EXTRN_SREG_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_CS)
        {
            COPY_OUT_SEG(iReg, HvX64RegisterCs,   pCtx->cs);
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_ES)
        {
            COPY_OUT_SEG(iReg, HvX64RegisterEs,   pCtx->es);
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_SS)
        {
            COPY_OUT_SEG(iReg, HvX64RegisterSs,   pCtx->ss);
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_DS)
        {
            COPY_OUT_SEG(iReg, HvX64RegisterDs,   pCtx->ds);
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_FS)
        {
            COPY_OUT_SEG(iReg, HvX64RegisterFs,   pCtx->fs);
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_GS)
        {
            COPY_OUT_SEG(iReg, HvX64RegisterGs,   pCtx->gs);
            iReg++;
        }
    }

    /* Descriptor tables & task segment. */
    if (fWhat & CPUMCTX_EXTRN_TABLE_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_LDTR)
        {
            COPY_OUT_SEG(iReg, HvX64RegisterLdtr, pCtx->ldtr);
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_TR)
        {
            COPY_OUT_SEG(iReg, HvX64RegisterTr,   pCtx->tr);
            iReg++;
        }

        if (fWhat & CPUMCTX_EXTRN_IDTR)
        {
            HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Value.Table.Pad[0]   = 0;
            pInput->Elements[iReg].Value.Table.Pad[1]   = 0;
            pInput->Elements[iReg].Value.Table.Pad[2]   = 0;
            pInput->Elements[iReg].Name                 = HvX64RegisterIdtr;
            pInput->Elements[iReg].Value.Table.Limit    = pCtx->idtr.cbIdt;
            pInput->Elements[iReg].Value.Table.Base     = pCtx->idtr.pIdt;
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_GDTR)
        {
            HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Value.Table.Pad[0]   = 0;
            pInput->Elements[iReg].Value.Table.Pad[1]   = 0;
            pInput->Elements[iReg].Value.Table.Pad[2]   = 0;
            pInput->Elements[iReg].Name                 = HvX64RegisterGdtr;
            pInput->Elements[iReg].Value.Table.Limit    = pCtx->gdtr.cbGdt;
            pInput->Elements[iReg].Value.Table.Base     = pCtx->gdtr.pGdt;
            iReg++;
        }
    }

    /* Control registers. */
    if (fWhat & CPUMCTX_EXTRN_CR_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_CR0)
        {
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                 = HvX64RegisterCr0;
            pInput->Elements[iReg].Value.Reg64          = pCtx->cr0;
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_CR2)
        {
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                 = HvX64RegisterCr2;
            pInput->Elements[iReg].Value.Reg64          = pCtx->cr2;
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_CR3)
        {
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                 = HvX64RegisterCr3;
            pInput->Elements[iReg].Value.Reg64          = pCtx->cr3;
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_CR4)
        {
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                 = HvX64RegisterCr4;
            pInput->Elements[iReg].Value.Reg64          = pCtx->cr4;
            iReg++;
        }
    }
    if (fWhat & CPUMCTX_EXTRN_APIC_TPR)
    {
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterCr8;
        pInput->Elements[iReg].Value.Reg64          = CPUMGetGuestCR8(pGVCpu);
        iReg++;
    }

    /** @todo does HvX64RegisterXfem mean XCR0? What about the related MSR. */

    /* Debug registers. */
/** @todo fixme. Figure out what the hyper-v version of KVM_SET_GUEST_DEBUG would be. */
    if (fWhat & CPUMCTX_EXTRN_DR0_DR3)
    {
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterDr0;
        //pInput->Elements[iReg].Value.Reg64        = CPUMGetHyperDR0(pGVCpu);
        pInput->Elements[iReg].Value.Reg64          = pCtx->dr[0];
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterDr1;
        //pInput->Elements[iReg].Value.Reg64        = CPUMGetHyperDR1(pGVCpu);
        pInput->Elements[iReg].Value.Reg64          = pCtx->dr[1];
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterDr2;
        //pInput->Elements[iReg].Value.Reg64        = CPUMGetHyperDR2(pGVCpu);
        pInput->Elements[iReg].Value.Reg64          = pCtx->dr[2];
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterDr3;
        //pInput->Elements[iReg].Value.Reg64        = CPUMGetHyperDR3(pGVCpu);
        pInput->Elements[iReg].Value.Reg64          = pCtx->dr[3];
        iReg++;
    }
    if (fWhat & CPUMCTX_EXTRN_DR6)
    {
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterDr6;
        //pInput->Elements[iReg].Value.Reg64        = CPUMGetHyperDR6(pGVCpu);
        pInput->Elements[iReg].Value.Reg64          = pCtx->dr[6];
        iReg++;
    }
    if (fWhat & CPUMCTX_EXTRN_DR7)
    {
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterDr7;
        //pInput->Elements[iReg].Value.Reg64        = CPUMGetHyperDR7(pGVCpu);
        pInput->Elements[iReg].Value.Reg64          = pCtx->dr[7];
        iReg++;
    }

    /* Floating point state. */
    if (fWhat & CPUMCTX_EXTRN_X87)
    {
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                      = HvX64RegisterFpMmx0;
        pInput->Elements[iReg].Value.Fp.AsUINT128.Low64  = pCtx->pXStateR0->x87.aRegs[0].au64[0];
        pInput->Elements[iReg].Value.Fp.AsUINT128.High64 = pCtx->pXStateR0->x87.aRegs[0].au64[1];
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                      = HvX64RegisterFpMmx1;
        pInput->Elements[iReg].Value.Fp.AsUINT128.Low64  = pCtx->pXStateR0->x87.aRegs[1].au64[0];
        pInput->Elements[iReg].Value.Fp.AsUINT128.High64 = pCtx->pXStateR0->x87.aRegs[1].au64[1];
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                      = HvX64RegisterFpMmx2;
        pInput->Elements[iReg].Value.Fp.AsUINT128.Low64  = pCtx->pXStateR0->x87.aRegs[2].au64[0];
        pInput->Elements[iReg].Value.Fp.AsUINT128.High64 = pCtx->pXStateR0->x87.aRegs[2].au64[1];
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                      = HvX64RegisterFpMmx3;
        pInput->Elements[iReg].Value.Fp.AsUINT128.Low64  = pCtx->pXStateR0->x87.aRegs[3].au64[0];
        pInput->Elements[iReg].Value.Fp.AsUINT128.High64 = pCtx->pXStateR0->x87.aRegs[3].au64[1];
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                      = HvX64RegisterFpMmx4;
        pInput->Elements[iReg].Value.Fp.AsUINT128.Low64  = pCtx->pXStateR0->x87.aRegs[4].au64[0];
        pInput->Elements[iReg].Value.Fp.AsUINT128.High64 = pCtx->pXStateR0->x87.aRegs[4].au64[1];
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                      = HvX64RegisterFpMmx5;
        pInput->Elements[iReg].Value.Fp.AsUINT128.Low64  = pCtx->pXStateR0->x87.aRegs[5].au64[0];
        pInput->Elements[iReg].Value.Fp.AsUINT128.High64 = pCtx->pXStateR0->x87.aRegs[5].au64[1];
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                      = HvX64RegisterFpMmx6;
        pInput->Elements[iReg].Value.Fp.AsUINT128.Low64  = pCtx->pXStateR0->x87.aRegs[6].au64[0];
        pInput->Elements[iReg].Value.Fp.AsUINT128.High64 = pCtx->pXStateR0->x87.aRegs[6].au64[1];
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                      = HvX64RegisterFpMmx7;
        pInput->Elements[iReg].Value.Fp.AsUINT128.Low64  = pCtx->pXStateR0->x87.aRegs[7].au64[0];
        pInput->Elements[iReg].Value.Fp.AsUINT128.High64 = pCtx->pXStateR0->x87.aRegs[7].au64[1];
        iReg++;

        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                            = HvX64RegisterFpControlStatus;
        pInput->Elements[iReg].Value.FpControlStatus.FpControl = pCtx->pXStateR0->x87.FCW;
        pInput->Elements[iReg].Value.FpControlStatus.FpStatus  = pCtx->pXStateR0->x87.FSW;
        pInput->Elements[iReg].Value.FpControlStatus.FpTag     = pCtx->pXStateR0->x87.FTW;
        pInput->Elements[iReg].Value.FpControlStatus.Reserved  = pCtx->pXStateR0->x87.FTW >> 8;
        pInput->Elements[iReg].Value.FpControlStatus.LastFpOp  = pCtx->pXStateR0->x87.FOP;
        pInput->Elements[iReg].Value.FpControlStatus.LastFpRip = (pCtx->pXStateR0->x87.FPUIP)
                                                               | ((uint64_t)pCtx->pXStateR0->x87.CS << 32)
                                                               | ((uint64_t)pCtx->pXStateR0->x87.Rsrvd1 << 48);
        iReg++;
/** @todo we've got trouble if if we try write just SSE w/o X87.  */
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                                        = HvX64RegisterXmmControlStatus;
        pInput->Elements[iReg].Value.XmmControlStatus.LastFpRdp            = (pCtx->pXStateR0->x87.FPUDP)
                                                                           | ((uint64_t)pCtx->pXStateR0->x87.DS << 32)
                                                                           | ((uint64_t)pCtx->pXStateR0->x87.Rsrvd2 << 48);
        pInput->Elements[iReg].Value.XmmControlStatus.XmmStatusControl     = pCtx->pXStateR0->x87.MXCSR;
        pInput->Elements[iReg].Value.XmmControlStatus.XmmStatusControlMask = pCtx->pXStateR0->x87.MXCSR_MASK; /** @todo ??? (Isn't this an output field?) */
        iReg++;
    }

    /* Vector state. */
    if (fWhat & CPUMCTX_EXTRN_SSE_AVX)
    {
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterXmm0;
        pInput->Elements[iReg].Value.Reg128.Low64   = pCtx->pXStateR0->x87.aXMM[0].uXmm.s.Lo;
        pInput->Elements[iReg].Value.Reg128.High64  = pCtx->pXStateR0->x87.aXMM[0].uXmm.s.Hi;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterXmm1;
        pInput->Elements[iReg].Value.Reg128.Low64   = pCtx->pXStateR0->x87.aXMM[1].uXmm.s.Lo;
        pInput->Elements[iReg].Value.Reg128.High64  = pCtx->pXStateR0->x87.aXMM[1].uXmm.s.Hi;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterXmm2;
        pInput->Elements[iReg].Value.Reg128.Low64   = pCtx->pXStateR0->x87.aXMM[2].uXmm.s.Lo;
        pInput->Elements[iReg].Value.Reg128.High64  = pCtx->pXStateR0->x87.aXMM[2].uXmm.s.Hi;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterXmm3;
        pInput->Elements[iReg].Value.Reg128.Low64   = pCtx->pXStateR0->x87.aXMM[3].uXmm.s.Lo;
        pInput->Elements[iReg].Value.Reg128.High64  = pCtx->pXStateR0->x87.aXMM[3].uXmm.s.Hi;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterXmm4;
        pInput->Elements[iReg].Value.Reg128.Low64   = pCtx->pXStateR0->x87.aXMM[4].uXmm.s.Lo;
        pInput->Elements[iReg].Value.Reg128.High64  = pCtx->pXStateR0->x87.aXMM[4].uXmm.s.Hi;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterXmm5;
        pInput->Elements[iReg].Value.Reg128.Low64   = pCtx->pXStateR0->x87.aXMM[5].uXmm.s.Lo;
        pInput->Elements[iReg].Value.Reg128.High64  = pCtx->pXStateR0->x87.aXMM[5].uXmm.s.Hi;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterXmm6;
        pInput->Elements[iReg].Value.Reg128.Low64   = pCtx->pXStateR0->x87.aXMM[6].uXmm.s.Lo;
        pInput->Elements[iReg].Value.Reg128.High64  = pCtx->pXStateR0->x87.aXMM[6].uXmm.s.Hi;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterXmm7;
        pInput->Elements[iReg].Value.Reg128.Low64   = pCtx->pXStateR0->x87.aXMM[7].uXmm.s.Lo;
        pInput->Elements[iReg].Value.Reg128.High64  = pCtx->pXStateR0->x87.aXMM[7].uXmm.s.Hi;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterXmm8;
        pInput->Elements[iReg].Value.Reg128.Low64   = pCtx->pXStateR0->x87.aXMM[8].uXmm.s.Lo;
        pInput->Elements[iReg].Value.Reg128.High64  = pCtx->pXStateR0->x87.aXMM[8].uXmm.s.Hi;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterXmm9;
        pInput->Elements[iReg].Value.Reg128.Low64   = pCtx->pXStateR0->x87.aXMM[9].uXmm.s.Lo;
        pInput->Elements[iReg].Value.Reg128.High64  = pCtx->pXStateR0->x87.aXMM[9].uXmm.s.Hi;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterXmm10;
        pInput->Elements[iReg].Value.Reg128.Low64   = pCtx->pXStateR0->x87.aXMM[10].uXmm.s.Lo;
        pInput->Elements[iReg].Value.Reg128.High64  = pCtx->pXStateR0->x87.aXMM[10].uXmm.s.Hi;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterXmm11;
        pInput->Elements[iReg].Value.Reg128.Low64   = pCtx->pXStateR0->x87.aXMM[11].uXmm.s.Lo;
        pInput->Elements[iReg].Value.Reg128.High64  = pCtx->pXStateR0->x87.aXMM[11].uXmm.s.Hi;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterXmm12;
        pInput->Elements[iReg].Value.Reg128.Low64   = pCtx->pXStateR0->x87.aXMM[12].uXmm.s.Lo;
        pInput->Elements[iReg].Value.Reg128.High64  = pCtx->pXStateR0->x87.aXMM[12].uXmm.s.Hi;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterXmm13;
        pInput->Elements[iReg].Value.Reg128.Low64   = pCtx->pXStateR0->x87.aXMM[13].uXmm.s.Lo;
        pInput->Elements[iReg].Value.Reg128.High64  = pCtx->pXStateR0->x87.aXMM[13].uXmm.s.Hi;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterXmm14;
        pInput->Elements[iReg].Value.Reg128.Low64   = pCtx->pXStateR0->x87.aXMM[14].uXmm.s.Lo;
        pInput->Elements[iReg].Value.Reg128.High64  = pCtx->pXStateR0->x87.aXMM[14].uXmm.s.Hi;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterXmm15;
        pInput->Elements[iReg].Value.Reg128.Low64   = pCtx->pXStateR0->x87.aXMM[15].uXmm.s.Lo;
        pInput->Elements[iReg].Value.Reg128.High64  = pCtx->pXStateR0->x87.aXMM[15].uXmm.s.Hi;
        iReg++;
    }

    /* MSRs */
    // HvX64RegisterTsc - don't touch
    if (fWhat & CPUMCTX_EXTRN_EFER)
    {
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterEfer;
        pInput->Elements[iReg].Value.Reg64          = pCtx->msrEFER;
        iReg++;
    }
    if (fWhat & CPUMCTX_EXTRN_KERNEL_GS_BASE)
    {
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterKernelGsBase;
        pInput->Elements[iReg].Value.Reg64          = pCtx->msrKERNELGSBASE;
        iReg++;
    }
    if (fWhat & CPUMCTX_EXTRN_SYSENTER_MSRS)
    {
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterSysenterCs;
        pInput->Elements[iReg].Value.Reg64          = pCtx->SysEnter.cs;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterSysenterEip;
        pInput->Elements[iReg].Value.Reg64          = pCtx->SysEnter.eip;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterSysenterEsp;
        pInput->Elements[iReg].Value.Reg64          = pCtx->SysEnter.esp;
        iReg++;
    }
    if (fWhat & CPUMCTX_EXTRN_SYSCALL_MSRS)
    {
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterStar;
        pInput->Elements[iReg].Value.Reg64          = pCtx->msrSTAR;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterLstar;
        pInput->Elements[iReg].Value.Reg64          = pCtx->msrLSTAR;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterCstar;
        pInput->Elements[iReg].Value.Reg64          = pCtx->msrCSTAR;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterSfmask;
        pInput->Elements[iReg].Value.Reg64          = pCtx->msrSFMASK;
        iReg++;
    }
    if (fWhat & CPUMCTX_EXTRN_OTHER_MSRS)
    {
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterApicBase;
        pInput->Elements[iReg].Value.Reg64          = APICGetBaseMsrNoCheck(pGVCpu);
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterPat;
        pInput->Elements[iReg].Value.Reg64          = pCtx->msrPAT;
        iReg++;
# if 0 /** @todo HvX64RegisterMtrrCap is read only?  Seems it's not even readable. */
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterMtrrCap;
        pInput->Elements[iReg].Value.Reg64          = CPUMGetGuestIa32MtrrCap(pGVCpu);
        iReg++;
# endif

        PCPUMCTXMSRS pCtxMsrs = CPUMQueryGuestCtxMsrsPtr(pGVCpu);

        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterMtrrDefType;
        pInput->Elements[iReg].Value.Reg64          = pCtxMsrs->msr.MtrrDefType;
        iReg++;

        /** @todo we dont keep state for HvX64RegisterMtrrPhysBaseX and HvX64RegisterMtrrPhysMaskX */

        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterMtrrFix64k00000;
        pInput->Elements[iReg].Value.Reg64          = pCtxMsrs->msr.MtrrFix64K_00000;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterMtrrFix16k80000;
        pInput->Elements[iReg].Value.Reg64          = pCtxMsrs->msr.MtrrFix16K_80000;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterMtrrFix16kA0000;
        pInput->Elements[iReg].Value.Reg64          = pCtxMsrs->msr.MtrrFix16K_A0000;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterMtrrFix4kC0000;
        pInput->Elements[iReg].Value.Reg64          = pCtxMsrs->msr.MtrrFix4K_C0000;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterMtrrFix4kC8000;
        pInput->Elements[iReg].Value.Reg64          = pCtxMsrs->msr.MtrrFix4K_C8000;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterMtrrFix4kD0000;
        pInput->Elements[iReg].Value.Reg64          = pCtxMsrs->msr.MtrrFix4K_D0000;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterMtrrFix4kD8000;
        pInput->Elements[iReg].Value.Reg64          = pCtxMsrs->msr.MtrrFix4K_D8000;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterMtrrFix4kE0000;
        pInput->Elements[iReg].Value.Reg64          = pCtxMsrs->msr.MtrrFix4K_E0000;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterMtrrFix4kE8000;
        pInput->Elements[iReg].Value.Reg64          = pCtxMsrs->msr.MtrrFix4K_E8000;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterMtrrFix4kF0000;
        pInput->Elements[iReg].Value.Reg64          = pCtxMsrs->msr.MtrrFix4K_F0000;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterMtrrFix4kF8000;
        pInput->Elements[iReg].Value.Reg64          = pCtxMsrs->msr.MtrrFix4K_F8000;
        iReg++;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvX64RegisterTscAux;
        pInput->Elements[iReg].Value.Reg64          = pCtxMsrs->msr.TscAux;
        iReg++;

# if 0 /** @todo Why can't we write these on Intel systems? Not that we really care... */
        const CPUMCPUVENDOR enmCpuVendor = CPUMGetHostCpuVendor(pGVM);
        if (enmCpuVendor != CPUMCPUVENDOR_AMD)
        {
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                 = HvX64RegisterIa32MiscEnable;
            pInput->Elements[iReg].Value.Reg64          = pCtxMsrs->msr.MiscEnable;
            iReg++;
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                 = HvX64RegisterIa32FeatureControl;
            pInput->Elements[iReg].Value.Reg64          = CPUMGetGuestIa32FeatureControl(pGVCpu);
            iReg++;
        }
# endif
    }

    /* event injection (clear it). */
    if (fWhat & CPUMCTX_EXTRN_NEM_WIN_EVENT_INJECT)
    {
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvRegisterPendingInterruption;
        pInput->Elements[iReg].Value.Reg64          = 0;
        iReg++;
    }

    /* Interruptibility state.  This can get a little complicated since we get
       half of the state via HV_X64_VP_EXECUTION_STATE. */
    if (   (fWhat & (CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT | CPUMCTX_EXTRN_NEM_WIN_INHIBIT_NMI))
        ==          (CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT | CPUMCTX_EXTRN_NEM_WIN_INHIBIT_NMI) )
    {
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                 = HvRegisterInterruptState;
        pInput->Elements[iReg].Value.Reg64          = 0;
        if (   VMCPU_FF_IS_SET(pGVCpu, VMCPU_FF_INHIBIT_INTERRUPTS)
            && EMGetInhibitInterruptsPC(pGVCpu) == pCtx->rip)
            pInput->Elements[iReg].Value.InterruptState.InterruptShadow = 1;
        if (VMCPU_FF_IS_SET(pGVCpu, VMCPU_FF_BLOCK_NMIS))
            pInput->Elements[iReg].Value.InterruptState.NmiMasked = 1;
        iReg++;
    }
    else if (fWhat & CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT)
    {
        if (   pGVCpu->nem.s.fLastInterruptShadow
            || (   VMCPU_FF_IS_SET(pGVCpu, VMCPU_FF_INHIBIT_INTERRUPTS)
                && EMGetInhibitInterruptsPC(pGVCpu) == pCtx->rip))
        {
            HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
            pInput->Elements[iReg].Name                 = HvRegisterInterruptState;
            pInput->Elements[iReg].Value.Reg64          = 0;
            if (   VMCPU_FF_IS_SET(pGVCpu, VMCPU_FF_INHIBIT_INTERRUPTS)
                && EMGetInhibitInterruptsPC(pGVCpu) == pCtx->rip)
                pInput->Elements[iReg].Value.InterruptState.InterruptShadow = 1;
            /** @todo Retrieve NMI state, currently assuming it's zero. (yes this may happen on I/O) */
            //if (VMCPU_FF_IS_ANY_SET(pGVCpu, VMCPU_FF_BLOCK_NMIS))
            //    pInput->Elements[iReg].Value.InterruptState.NmiMasked = 1;
            iReg++;
        }
    }
    else
        Assert(!(fWhat & CPUMCTX_EXTRN_NEM_WIN_INHIBIT_NMI));

    /* Interrupt windows. Always set if active as Hyper-V seems to be forgetful. */
    uint8_t const fDesiredIntWin = pGVCpu->nem.s.fDesiredInterruptWindows;
    if (   fDesiredIntWin
        || pGVCpu->nem.s.fCurrentInterruptWindows != fDesiredIntWin)
    {
        pGVCpu->nem.s.fCurrentInterruptWindows = pGVCpu->nem.s.fDesiredInterruptWindows;
        HV_REGISTER_ASSOC_ZERO_PADDING_AND_HI64(&pInput->Elements[iReg]);
        pInput->Elements[iReg].Name                                         = HvX64RegisterDeliverabilityNotifications;
        pInput->Elements[iReg].Value.DeliverabilityNotifications.AsUINT64   = fDesiredIntWin;
        Assert(pInput->Elements[iReg].Value.DeliverabilityNotifications.NmiNotification == RT_BOOL(fDesiredIntWin & NEM_WIN_INTW_F_NMI));
        Assert(pInput->Elements[iReg].Value.DeliverabilityNotifications.InterruptNotification == RT_BOOL(fDesiredIntWin & NEM_WIN_INTW_F_REGULAR));
        Assert(pInput->Elements[iReg].Value.DeliverabilityNotifications.InterruptPriority == (fDesiredIntWin & NEM_WIN_INTW_F_PRIO_MASK) >> NEM_WIN_INTW_F_PRIO_SHIFT);
        iReg++;
    }

    /// @todo HvRegisterPendingEvent0
    /// @todo HvRegisterPendingEvent1

    /*
     * Set the registers.
     */
    Assert((uintptr_t)&pInput->Elements[iReg] - (uintptr_t)pGVCpu->nemr0.s.HypercallData.pbPage < PAGE_SIZE); /* max is 127 */

    /*
     * Make the hypercall.
     */
    uint64_t uResult = g_pfnHvlInvokeHypercall(HV_MAKE_CALL_INFO(HvCallSetVpRegisters, iReg),
                                               pGVCpu->nemr0.s.HypercallData.HCPhysPage, 0 /*GCPhysOutput*/);
    AssertLogRelMsgReturn(uResult == HV_MAKE_CALL_REP_RET(iReg),
                          ("uResult=%RX64 iRegs=%#x\n", uResult, iReg),
                          VERR_NEM_SET_REGISTERS_FAILED);
    //LogFlow(("nemR0WinExportState: uResult=%#RX64 iReg=%zu fWhat=%#018RX64 fExtrn=%#018RX64 -> %#018RX64\n", uResult, iReg, fWhat, pCtx->fExtrn,
    //         pCtx->fExtrn | CPUMCTX_EXTRN_ALL | CPUMCTX_EXTRN_NEM_WIN_MASK | CPUMCTX_EXTRN_KEEPER_NEM ));
    pCtx->fExtrn |= CPUMCTX_EXTRN_ALL | CPUMCTX_EXTRN_NEM_WIN_MASK | CPUMCTX_EXTRN_KEEPER_NEM;
    return VINF_SUCCESS;
}
#endif /* NEM_WIN_WITH_RING0_RUNLOOP || NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS */


/**
 * Export the state to the native API (out of CPUMCTX).
 *
 * @returns VBox status code
 * @param   pGVM        The ring-0 VM handle.
 * @param   idCpu       The calling EMT.  Necessary for getting the
 *                      hypercall page and arguments.
 */
VMMR0_INT_DECL(int)  NEMR0ExportState(PGVM pGVM, VMCPUID idCpu)
{
#if defined(NEM_WIN_WITH_RING0_RUNLOOP) || defined(NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS)
    /*
     * Validate the call.
     */
    int rc = GVMMR0ValidateGVMandEMT(pGVM, idCpu);
    if (RT_SUCCESS(rc))
    {
        PGVMCPU pGVCpu = &pGVM->aCpus[idCpu];
        AssertReturn(g_pfnHvlInvokeHypercall, VERR_NEM_MISSING_KERNEL_API_1);

        /*
         * Call worker.
         */
        rc = nemR0WinExportState(pGVM, pGVCpu, &pGVCpu->cpum.GstCtx);
    }
    return rc;
#else
    RT_NOREF(pGVM, idCpu);
    return VERR_NOT_IMPLEMENTED;
#endif
}


#if defined(NEM_WIN_WITH_RING0_RUNLOOP) || defined(NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS)
/**
 * Worker for NEMR0ImportState.
 *
 * Intention is to use it internally later.
 *
 * @returns VBox status code.
 * @param   pGVM            The ring-0 VM handle.
 * @param   pGVCpu          The ring-0 VCPU handle.
 * @param   pCtx            The CPU context structure to import into.
 * @param   fWhat           What to import, CPUMCTX_EXTRN_XXX.
 * @param   fCanUpdateCr3   Whether it's safe to update CR3 or not.
 */
NEM_TMPL_STATIC int nemR0WinImportState(PGVM pGVM, PGVMCPU pGVCpu, PCPUMCTX pCtx, uint64_t fWhat, bool fCanUpdateCr3)
{
    HV_INPUT_GET_VP_REGISTERS *pInput = (HV_INPUT_GET_VP_REGISTERS *)pGVCpu->nemr0.s.HypercallData.pbPage;
    AssertPtrReturn(pInput, VERR_INTERNAL_ERROR_3);
    AssertReturn(g_pfnHvlInvokeHypercall, VERR_NEM_MISSING_KERNEL_API_1);
    Assert(pCtx == &pGVCpu->cpum.GstCtx);

    fWhat &= pCtx->fExtrn;

    pInput->PartitionId = pGVM->nemr0.s.idHvPartition;
    pInput->VpIndex     = pGVCpu->idCpu;
    pInput->fFlags      = 0;

    /* GPRs */
    uintptr_t iReg = 0;
    if (fWhat & CPUMCTX_EXTRN_GPRS_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_RAX)
            pInput->Names[iReg++] = HvX64RegisterRax;
        if (fWhat & CPUMCTX_EXTRN_RCX)
            pInput->Names[iReg++] = HvX64RegisterRcx;
        if (fWhat & CPUMCTX_EXTRN_RDX)
            pInput->Names[iReg++] = HvX64RegisterRdx;
        if (fWhat & CPUMCTX_EXTRN_RBX)
            pInput->Names[iReg++] = HvX64RegisterRbx;
        if (fWhat & CPUMCTX_EXTRN_RSP)
            pInput->Names[iReg++] = HvX64RegisterRsp;
        if (fWhat & CPUMCTX_EXTRN_RBP)
            pInput->Names[iReg++] = HvX64RegisterRbp;
        if (fWhat & CPUMCTX_EXTRN_RSI)
            pInput->Names[iReg++] = HvX64RegisterRsi;
        if (fWhat & CPUMCTX_EXTRN_RDI)
            pInput->Names[iReg++] = HvX64RegisterRdi;
        if (fWhat & CPUMCTX_EXTRN_R8_R15)
        {
            pInput->Names[iReg++] = HvX64RegisterR8;
            pInput->Names[iReg++] = HvX64RegisterR9;
            pInput->Names[iReg++] = HvX64RegisterR10;
            pInput->Names[iReg++] = HvX64RegisterR11;
            pInput->Names[iReg++] = HvX64RegisterR12;
            pInput->Names[iReg++] = HvX64RegisterR13;
            pInput->Names[iReg++] = HvX64RegisterR14;
            pInput->Names[iReg++] = HvX64RegisterR15;
        }
    }

    /* RIP & Flags */
    if (fWhat & CPUMCTX_EXTRN_RIP)
        pInput->Names[iReg++]  = HvX64RegisterRip;
    if (fWhat & CPUMCTX_EXTRN_RFLAGS)
        pInput->Names[iReg++] = HvX64RegisterRflags;

    /* Segments */
    if (fWhat & CPUMCTX_EXTRN_SREG_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_CS)
            pInput->Names[iReg++] = HvX64RegisterCs;
        if (fWhat & CPUMCTX_EXTRN_ES)
            pInput->Names[iReg++] = HvX64RegisterEs;
        if (fWhat & CPUMCTX_EXTRN_SS)
            pInput->Names[iReg++] = HvX64RegisterSs;
        if (fWhat & CPUMCTX_EXTRN_DS)
            pInput->Names[iReg++] = HvX64RegisterDs;
        if (fWhat & CPUMCTX_EXTRN_FS)
            pInput->Names[iReg++] = HvX64RegisterFs;
        if (fWhat & CPUMCTX_EXTRN_GS)
            pInput->Names[iReg++] = HvX64RegisterGs;
    }

    /* Descriptor tables and the task segment. */
    if (fWhat & CPUMCTX_EXTRN_TABLE_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_LDTR)
            pInput->Names[iReg++] = HvX64RegisterLdtr;
        if (fWhat & CPUMCTX_EXTRN_TR)
            pInput->Names[iReg++] = HvX64RegisterTr;
        if (fWhat & CPUMCTX_EXTRN_IDTR)
            pInput->Names[iReg++] = HvX64RegisterIdtr;
        if (fWhat & CPUMCTX_EXTRN_GDTR)
            pInput->Names[iReg++] = HvX64RegisterGdtr;
    }

    /* Control registers. */
    if (fWhat & CPUMCTX_EXTRN_CR_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_CR0)
            pInput->Names[iReg++] = HvX64RegisterCr0;
        if (fWhat & CPUMCTX_EXTRN_CR2)
            pInput->Names[iReg++] = HvX64RegisterCr2;
        if (fWhat & CPUMCTX_EXTRN_CR3)
            pInput->Names[iReg++] = HvX64RegisterCr3;
        if (fWhat & CPUMCTX_EXTRN_CR4)
            pInput->Names[iReg++] = HvX64RegisterCr4;
    }
    if (fWhat & CPUMCTX_EXTRN_APIC_TPR)
        pInput->Names[iReg++] = HvX64RegisterCr8;

    /* Debug registers. */
    if (fWhat & CPUMCTX_EXTRN_DR7)
        pInput->Names[iReg++] = HvX64RegisterDr7;
    if (fWhat & CPUMCTX_EXTRN_DR0_DR3)
    {
        if (!(fWhat & CPUMCTX_EXTRN_DR7) && (pCtx->fExtrn & CPUMCTX_EXTRN_DR7))
        {
            fWhat |= CPUMCTX_EXTRN_DR7;
            pInput->Names[iReg++] = HvX64RegisterDr7;
        }
        pInput->Names[iReg++] = HvX64RegisterDr0;
        pInput->Names[iReg++] = HvX64RegisterDr1;
        pInput->Names[iReg++] = HvX64RegisterDr2;
        pInput->Names[iReg++] = HvX64RegisterDr3;
    }
    if (fWhat & CPUMCTX_EXTRN_DR6)
        pInput->Names[iReg++] = HvX64RegisterDr6;

    /* Floating point state. */
    if (fWhat & CPUMCTX_EXTRN_X87)
    {
        pInput->Names[iReg++] = HvX64RegisterFpMmx0;
        pInput->Names[iReg++] = HvX64RegisterFpMmx1;
        pInput->Names[iReg++] = HvX64RegisterFpMmx2;
        pInput->Names[iReg++] = HvX64RegisterFpMmx3;
        pInput->Names[iReg++] = HvX64RegisterFpMmx4;
        pInput->Names[iReg++] = HvX64RegisterFpMmx5;
        pInput->Names[iReg++] = HvX64RegisterFpMmx6;
        pInput->Names[iReg++] = HvX64RegisterFpMmx7;
        pInput->Names[iReg++] = HvX64RegisterFpControlStatus;
    }
    if (fWhat & (CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX))
        pInput->Names[iReg++] = HvX64RegisterXmmControlStatus;

    /* Vector state. */
    if (fWhat & CPUMCTX_EXTRN_SSE_AVX)
    {
        pInput->Names[iReg++] = HvX64RegisterXmm0;
        pInput->Names[iReg++] = HvX64RegisterXmm1;
        pInput->Names[iReg++] = HvX64RegisterXmm2;
        pInput->Names[iReg++] = HvX64RegisterXmm3;
        pInput->Names[iReg++] = HvX64RegisterXmm4;
        pInput->Names[iReg++] = HvX64RegisterXmm5;
        pInput->Names[iReg++] = HvX64RegisterXmm6;
        pInput->Names[iReg++] = HvX64RegisterXmm7;
        pInput->Names[iReg++] = HvX64RegisterXmm8;
        pInput->Names[iReg++] = HvX64RegisterXmm9;
        pInput->Names[iReg++] = HvX64RegisterXmm10;
        pInput->Names[iReg++] = HvX64RegisterXmm11;
        pInput->Names[iReg++] = HvX64RegisterXmm12;
        pInput->Names[iReg++] = HvX64RegisterXmm13;
        pInput->Names[iReg++] = HvX64RegisterXmm14;
        pInput->Names[iReg++] = HvX64RegisterXmm15;
    }

    /* MSRs */
    // HvX64RegisterTsc - don't touch
    if (fWhat & CPUMCTX_EXTRN_EFER)
        pInput->Names[iReg++] = HvX64RegisterEfer;
    if (fWhat & CPUMCTX_EXTRN_KERNEL_GS_BASE)
        pInput->Names[iReg++] = HvX64RegisterKernelGsBase;
    if (fWhat & CPUMCTX_EXTRN_SYSENTER_MSRS)
    {
        pInput->Names[iReg++] = HvX64RegisterSysenterCs;
        pInput->Names[iReg++] = HvX64RegisterSysenterEip;
        pInput->Names[iReg++] = HvX64RegisterSysenterEsp;
    }
    if (fWhat & CPUMCTX_EXTRN_SYSCALL_MSRS)
    {
        pInput->Names[iReg++] = HvX64RegisterStar;
        pInput->Names[iReg++] = HvX64RegisterLstar;
        pInput->Names[iReg++] = HvX64RegisterCstar;
        pInput->Names[iReg++] = HvX64RegisterSfmask;
    }

# ifdef LOG_ENABLED
    const CPUMCPUVENDOR enmCpuVendor = CPUMGetHostCpuVendor(pGVM);
# endif
    if (fWhat & CPUMCTX_EXTRN_OTHER_MSRS)
    {
        pInput->Names[iReg++] = HvX64RegisterApicBase; /// @todo APIC BASE
        pInput->Names[iReg++] = HvX64RegisterPat;
# if 0 /*def LOG_ENABLED*/ /** @todo something's wrong with HvX64RegisterMtrrCap? (AMD) */
        pInput->Names[iReg++] = HvX64RegisterMtrrCap;
# endif
        pInput->Names[iReg++] = HvX64RegisterMtrrDefType;
        pInput->Names[iReg++] = HvX64RegisterMtrrFix64k00000;
        pInput->Names[iReg++] = HvX64RegisterMtrrFix16k80000;
        pInput->Names[iReg++] = HvX64RegisterMtrrFix16kA0000;
        pInput->Names[iReg++] = HvX64RegisterMtrrFix4kC0000;
        pInput->Names[iReg++] = HvX64RegisterMtrrFix4kC8000;
        pInput->Names[iReg++] = HvX64RegisterMtrrFix4kD0000;
        pInput->Names[iReg++] = HvX64RegisterMtrrFix4kD8000;
        pInput->Names[iReg++] = HvX64RegisterMtrrFix4kE0000;
        pInput->Names[iReg++] = HvX64RegisterMtrrFix4kE8000;
        pInput->Names[iReg++] = HvX64RegisterMtrrFix4kF0000;
        pInput->Names[iReg++] = HvX64RegisterMtrrFix4kF8000;
        pInput->Names[iReg++] = HvX64RegisterTscAux;
# if 0 /** @todo why can't we read HvX64RegisterIa32MiscEnable? */
        if (enmCpuVendor != CPUMCPUVENDOR_AMD)
            pInput->Names[iReg++] = HvX64RegisterIa32MiscEnable;
# endif
# ifdef LOG_ENABLED
        if (enmCpuVendor != CPUMCPUVENDOR_AMD && enmCpuVendor != CPUMCPUVENDOR_HYGON)
            pInput->Names[iReg++] = HvX64RegisterIa32FeatureControl;
# endif
    }

    /* Interruptibility. */
    if (fWhat & (CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT | CPUMCTX_EXTRN_NEM_WIN_INHIBIT_NMI))
    {
        pInput->Names[iReg++] = HvRegisterInterruptState;
        pInput->Names[iReg++] = HvX64RegisterRip;
    }

    /* event injection */
    pInput->Names[iReg++] = HvRegisterPendingInterruption;
    pInput->Names[iReg++] = HvRegisterPendingEvent0;
    pInput->Names[iReg++] = HvRegisterPendingEvent1;
    size_t const cRegs   = iReg;
    size_t const cbInput = RT_ALIGN_Z(RT_UOFFSETOF_DYN(HV_INPUT_GET_VP_REGISTERS, Names[cRegs]), 32);

    HV_REGISTER_VALUE *paValues = (HV_REGISTER_VALUE *)((uint8_t *)pInput + cbInput);
    Assert((uintptr_t)&paValues[cRegs] - (uintptr_t)pGVCpu->nemr0.s.HypercallData.pbPage < PAGE_SIZE); /* (max is around 168 registers) */
    RT_BZERO(paValues, cRegs * sizeof(paValues[0]));

    /*
     * Make the hypercall.
     */
    uint64_t uResult = g_pfnHvlInvokeHypercall(HV_MAKE_CALL_INFO(HvCallGetVpRegisters, cRegs),
                                               pGVCpu->nemr0.s.HypercallData.HCPhysPage,
                                               pGVCpu->nemr0.s.HypercallData.HCPhysPage + cbInput);
    AssertLogRelMsgReturn(uResult == HV_MAKE_CALL_REP_RET(cRegs),
                          ("uResult=%RX64 cRegs=%#x\n", uResult, cRegs),
                          VERR_NEM_GET_REGISTERS_FAILED);
    //LogFlow(("nemR0WinImportState: uResult=%#RX64 iReg=%zu fWhat=%#018RX64 fExtr=%#018RX64\n", uResult, cRegs, fWhat, pCtx->fExtrn));

    /*
     * Copy information to the CPUM context.
     */
    iReg = 0;

    /* GPRs */
    if (fWhat & CPUMCTX_EXTRN_GPRS_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_RAX)
        {
            Assert(pInput->Names[iReg] == HvX64RegisterRax);
            pCtx->rax = paValues[iReg++].Reg64;
        }
        if (fWhat & CPUMCTX_EXTRN_RCX)
        {
            Assert(pInput->Names[iReg] == HvX64RegisterRcx);
            pCtx->rcx = paValues[iReg++].Reg64;
        }
        if (fWhat & CPUMCTX_EXTRN_RDX)
        {
            Assert(pInput->Names[iReg] == HvX64RegisterRdx);
            pCtx->rdx = paValues[iReg++].Reg64;
        }
        if (fWhat & CPUMCTX_EXTRN_RBX)
        {
            Assert(pInput->Names[iReg] == HvX64RegisterRbx);
            pCtx->rbx = paValues[iReg++].Reg64;
        }
        if (fWhat & CPUMCTX_EXTRN_RSP)
        {
            Assert(pInput->Names[iReg] == HvX64RegisterRsp);
            pCtx->rsp = paValues[iReg++].Reg64;
        }
        if (fWhat & CPUMCTX_EXTRN_RBP)
        {
            Assert(pInput->Names[iReg] == HvX64RegisterRbp);
            pCtx->rbp = paValues[iReg++].Reg64;
        }
        if (fWhat & CPUMCTX_EXTRN_RSI)
        {
            Assert(pInput->Names[iReg] == HvX64RegisterRsi);
            pCtx->rsi = paValues[iReg++].Reg64;
        }
        if (fWhat & CPUMCTX_EXTRN_RDI)
        {
            Assert(pInput->Names[iReg] == HvX64RegisterRdi);
            pCtx->rdi = paValues[iReg++].Reg64;
        }
        if (fWhat & CPUMCTX_EXTRN_R8_R15)
        {
            Assert(pInput->Names[iReg] == HvX64RegisterR8);
            Assert(pInput->Names[iReg + 7] == HvX64RegisterR15);
            pCtx->r8  = paValues[iReg++].Reg64;
            pCtx->r9  = paValues[iReg++].Reg64;
            pCtx->r10 = paValues[iReg++].Reg64;
            pCtx->r11 = paValues[iReg++].Reg64;
            pCtx->r12 = paValues[iReg++].Reg64;
            pCtx->r13 = paValues[iReg++].Reg64;
            pCtx->r14 = paValues[iReg++].Reg64;
            pCtx->r15 = paValues[iReg++].Reg64;
        }
    }

    /* RIP & Flags */
    if (fWhat & CPUMCTX_EXTRN_RIP)
    {
        Assert(pInput->Names[iReg] == HvX64RegisterRip);
        pCtx->rip      = paValues[iReg++].Reg64;
    }
    if (fWhat & CPUMCTX_EXTRN_RFLAGS)
    {
        Assert(pInput->Names[iReg] == HvX64RegisterRflags);
        pCtx->rflags.u = paValues[iReg++].Reg64;
    }

    /* Segments */
# define COPY_BACK_SEG(a_idx, a_enmName, a_SReg) \
        do { \
            Assert(pInput->Names[a_idx] == a_enmName); \
            (a_SReg).u64Base  = paValues[a_idx].Segment.Base; \
            (a_SReg).u32Limit = paValues[a_idx].Segment.Limit; \
            (a_SReg).ValidSel = (a_SReg).Sel = paValues[a_idx].Segment.Selector; \
            (a_SReg).Attr.u   = paValues[a_idx].Segment.Attributes; \
            (a_SReg).fFlags   = CPUMSELREG_FLAGS_VALID; \
        } while (0)
    if (fWhat & CPUMCTX_EXTRN_SREG_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_CS)
        {
            COPY_BACK_SEG(iReg, HvX64RegisterCs,   pCtx->cs);
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_ES)
        {
            COPY_BACK_SEG(iReg, HvX64RegisterEs,   pCtx->es);
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_SS)
        {
            COPY_BACK_SEG(iReg, HvX64RegisterSs,   pCtx->ss);
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_DS)
        {
            COPY_BACK_SEG(iReg, HvX64RegisterDs,   pCtx->ds);
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_FS)
        {
            COPY_BACK_SEG(iReg, HvX64RegisterFs,   pCtx->fs);
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_GS)
        {
            COPY_BACK_SEG(iReg, HvX64RegisterGs,   pCtx->gs);
            iReg++;
        }
    }
    /* Descriptor tables and the task segment. */
    if (fWhat & CPUMCTX_EXTRN_TABLE_MASK)
    {
        if (fWhat & CPUMCTX_EXTRN_LDTR)
        {
            COPY_BACK_SEG(iReg, HvX64RegisterLdtr, pCtx->ldtr);
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_TR)
        {
            /* AMD-V likes loading TR with in AVAIL state, whereas intel insists on BUSY.  So,
               avoid to trigger sanity assertions around the code, always fix this. */
            COPY_BACK_SEG(iReg, HvX64RegisterTr,   pCtx->tr);
            switch (pCtx->tr.Attr.n.u4Type)
            {
                case X86_SEL_TYPE_SYS_386_TSS_BUSY:
                case X86_SEL_TYPE_SYS_286_TSS_BUSY:
                    break;
                case X86_SEL_TYPE_SYS_386_TSS_AVAIL:
                    pCtx->tr.Attr.n.u4Type = X86_SEL_TYPE_SYS_386_TSS_BUSY;
                    break;
                case X86_SEL_TYPE_SYS_286_TSS_AVAIL:
                    pCtx->tr.Attr.n.u4Type = X86_SEL_TYPE_SYS_286_TSS_BUSY;
                    break;
            }
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_IDTR)
        {
            Assert(pInput->Names[iReg] == HvX64RegisterIdtr);
            pCtx->idtr.cbIdt = paValues[iReg].Table.Limit;
            pCtx->idtr.pIdt  = paValues[iReg].Table.Base;
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_GDTR)
        {
            Assert(pInput->Names[iReg] == HvX64RegisterGdtr);
            pCtx->gdtr.cbGdt = paValues[iReg].Table.Limit;
            pCtx->gdtr.pGdt  = paValues[iReg].Table.Base;
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
            Assert(pInput->Names[iReg] == HvX64RegisterCr0);
            if (pCtx->cr0 != paValues[iReg].Reg64)
            {
                CPUMSetGuestCR0(pGVCpu, paValues[iReg].Reg64);
                fMaybeChangedMode = true;
            }
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_CR2)
        {
            Assert(pInput->Names[iReg] == HvX64RegisterCr2);
            pCtx->cr2 = paValues[iReg].Reg64;
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_CR3)
        {
            Assert(pInput->Names[iReg] == HvX64RegisterCr3);
            if (pCtx->cr3 != paValues[iReg].Reg64)
            {
                CPUMSetGuestCR3(pGVCpu, paValues[iReg].Reg64);
                fUpdateCr3 = true;
            }
            iReg++;
        }
        if (fWhat & CPUMCTX_EXTRN_CR4)
        {
            Assert(pInput->Names[iReg] == HvX64RegisterCr4);
            if (pCtx->cr4 != paValues[iReg].Reg64)
            {
                CPUMSetGuestCR4(pGVCpu, paValues[iReg].Reg64);
                fMaybeChangedMode = true;
            }
            iReg++;
        }
    }
    if (fWhat & CPUMCTX_EXTRN_APIC_TPR)
    {
        Assert(pInput->Names[iReg] == HvX64RegisterCr8);
        APICSetTpr(pGVCpu, (uint8_t)paValues[iReg].Reg64 << 4);
        iReg++;
    }

    /* Debug registers. */
    if (fWhat & CPUMCTX_EXTRN_DR7)
    {
        Assert(pInput->Names[iReg] == HvX64RegisterDr7);
        if (pCtx->dr[7] != paValues[iReg].Reg64)
            CPUMSetGuestDR7(pGVCpu, paValues[iReg].Reg64);
        pCtx->fExtrn &= ~CPUMCTX_EXTRN_DR7; /* Hack alert! Avoids asserting when processing CPUMCTX_EXTRN_DR0_DR3. */
        iReg++;
    }
    if (fWhat & CPUMCTX_EXTRN_DR0_DR3)
    {
        Assert(pInput->Names[iReg] == HvX64RegisterDr0);
        Assert(pInput->Names[iReg+3] == HvX64RegisterDr3);
        if (pCtx->dr[0] != paValues[iReg].Reg64)
            CPUMSetGuestDR0(pGVCpu, paValues[iReg].Reg64);
        iReg++;
        if (pCtx->dr[1] != paValues[iReg].Reg64)
            CPUMSetGuestDR1(pGVCpu, paValues[iReg].Reg64);
        iReg++;
        if (pCtx->dr[2] != paValues[iReg].Reg64)
            CPUMSetGuestDR2(pGVCpu, paValues[iReg].Reg64);
        iReg++;
        if (pCtx->dr[3] != paValues[iReg].Reg64)
            CPUMSetGuestDR3(pGVCpu, paValues[iReg].Reg64);
        iReg++;
    }
    if (fWhat & CPUMCTX_EXTRN_DR6)
    {
        Assert(pInput->Names[iReg] == HvX64RegisterDr6);
        if (pCtx->dr[6] != paValues[iReg].Reg64)
            CPUMSetGuestDR6(pGVCpu, paValues[iReg].Reg64);
        iReg++;
    }

    /* Floating point state. */
    if (fWhat & CPUMCTX_EXTRN_X87)
    {
        Assert(pInput->Names[iReg] == HvX64RegisterFpMmx0);
        Assert(pInput->Names[iReg + 7] == HvX64RegisterFpMmx7);
        pCtx->pXStateR0->x87.aRegs[0].au64[0] = paValues[iReg].Fp.AsUINT128.Low64;
        pCtx->pXStateR0->x87.aRegs[0].au64[1] = paValues[iReg].Fp.AsUINT128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aRegs[1].au64[0] = paValues[iReg].Fp.AsUINT128.Low64;
        pCtx->pXStateR0->x87.aRegs[1].au64[1] = paValues[iReg].Fp.AsUINT128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aRegs[2].au64[0] = paValues[iReg].Fp.AsUINT128.Low64;
        pCtx->pXStateR0->x87.aRegs[2].au64[1] = paValues[iReg].Fp.AsUINT128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aRegs[3].au64[0] = paValues[iReg].Fp.AsUINT128.Low64;
        pCtx->pXStateR0->x87.aRegs[3].au64[1] = paValues[iReg].Fp.AsUINT128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aRegs[4].au64[0] = paValues[iReg].Fp.AsUINT128.Low64;
        pCtx->pXStateR0->x87.aRegs[4].au64[1] = paValues[iReg].Fp.AsUINT128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aRegs[5].au64[0] = paValues[iReg].Fp.AsUINT128.Low64;
        pCtx->pXStateR0->x87.aRegs[5].au64[1] = paValues[iReg].Fp.AsUINT128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aRegs[6].au64[0] = paValues[iReg].Fp.AsUINT128.Low64;
        pCtx->pXStateR0->x87.aRegs[6].au64[1] = paValues[iReg].Fp.AsUINT128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aRegs[7].au64[0] = paValues[iReg].Fp.AsUINT128.Low64;
        pCtx->pXStateR0->x87.aRegs[7].au64[1] = paValues[iReg].Fp.AsUINT128.High64;
        iReg++;

        Assert(pInput->Names[iReg] == HvX64RegisterFpControlStatus);
        pCtx->pXStateR0->x87.FCW        = paValues[iReg].FpControlStatus.FpControl;
        pCtx->pXStateR0->x87.FSW        = paValues[iReg].FpControlStatus.FpStatus;
        pCtx->pXStateR0->x87.FTW        = paValues[iReg].FpControlStatus.FpTag
                                        /*| (paValues[iReg].FpControlStatus.Reserved << 8)*/;
        pCtx->pXStateR0->x87.FOP        = paValues[iReg].FpControlStatus.LastFpOp;
        pCtx->pXStateR0->x87.FPUIP      = (uint32_t)paValues[iReg].FpControlStatus.LastFpRip;
        pCtx->pXStateR0->x87.CS         = (uint16_t)(paValues[iReg].FpControlStatus.LastFpRip >> 32);
        pCtx->pXStateR0->x87.Rsrvd1     = (uint16_t)(paValues[iReg].FpControlStatus.LastFpRip >> 48);
        iReg++;
    }

    if (fWhat & (CPUMCTX_EXTRN_X87 | CPUMCTX_EXTRN_SSE_AVX))
    {
        Assert(pInput->Names[iReg] == HvX64RegisterXmmControlStatus);
        if (fWhat & CPUMCTX_EXTRN_X87)
        {
            pCtx->pXStateR0->x87.FPUDP  = (uint32_t)paValues[iReg].XmmControlStatus.LastFpRdp;
            pCtx->pXStateR0->x87.DS     = (uint16_t)(paValues[iReg].XmmControlStatus.LastFpRdp >> 32);
            pCtx->pXStateR0->x87.Rsrvd2 = (uint16_t)(paValues[iReg].XmmControlStatus.LastFpRdp >> 48);
        }
        pCtx->pXStateR0->x87.MXCSR      = paValues[iReg].XmmControlStatus.XmmStatusControl;
        pCtx->pXStateR0->x87.MXCSR_MASK = paValues[iReg].XmmControlStatus.XmmStatusControlMask; /** @todo ??? (Isn't this an output field?) */
        iReg++;
    }

    /* Vector state. */
    if (fWhat & CPUMCTX_EXTRN_SSE_AVX)
    {
        Assert(pInput->Names[iReg] == HvX64RegisterXmm0);
        Assert(pInput->Names[iReg+15] == HvX64RegisterXmm15);
        pCtx->pXStateR0->x87.aXMM[0].uXmm.s.Lo  = paValues[iReg].Reg128.Low64;
        pCtx->pXStateR0->x87.aXMM[0].uXmm.s.Hi  = paValues[iReg].Reg128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aXMM[1].uXmm.s.Lo  = paValues[iReg].Reg128.Low64;
        pCtx->pXStateR0->x87.aXMM[1].uXmm.s.Hi  = paValues[iReg].Reg128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aXMM[2].uXmm.s.Lo  = paValues[iReg].Reg128.Low64;
        pCtx->pXStateR0->x87.aXMM[2].uXmm.s.Hi  = paValues[iReg].Reg128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aXMM[3].uXmm.s.Lo  = paValues[iReg].Reg128.Low64;
        pCtx->pXStateR0->x87.aXMM[3].uXmm.s.Hi  = paValues[iReg].Reg128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aXMM[4].uXmm.s.Lo  = paValues[iReg].Reg128.Low64;
        pCtx->pXStateR0->x87.aXMM[4].uXmm.s.Hi  = paValues[iReg].Reg128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aXMM[5].uXmm.s.Lo  = paValues[iReg].Reg128.Low64;
        pCtx->pXStateR0->x87.aXMM[5].uXmm.s.Hi  = paValues[iReg].Reg128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aXMM[6].uXmm.s.Lo  = paValues[iReg].Reg128.Low64;
        pCtx->pXStateR0->x87.aXMM[6].uXmm.s.Hi  = paValues[iReg].Reg128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aXMM[7].uXmm.s.Lo  = paValues[iReg].Reg128.Low64;
        pCtx->pXStateR0->x87.aXMM[7].uXmm.s.Hi  = paValues[iReg].Reg128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aXMM[8].uXmm.s.Lo  = paValues[iReg].Reg128.Low64;
        pCtx->pXStateR0->x87.aXMM[8].uXmm.s.Hi  = paValues[iReg].Reg128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aXMM[9].uXmm.s.Lo  = paValues[iReg].Reg128.Low64;
        pCtx->pXStateR0->x87.aXMM[9].uXmm.s.Hi  = paValues[iReg].Reg128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aXMM[10].uXmm.s.Lo = paValues[iReg].Reg128.Low64;
        pCtx->pXStateR0->x87.aXMM[10].uXmm.s.Hi = paValues[iReg].Reg128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aXMM[11].uXmm.s.Lo = paValues[iReg].Reg128.Low64;
        pCtx->pXStateR0->x87.aXMM[11].uXmm.s.Hi = paValues[iReg].Reg128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aXMM[12].uXmm.s.Lo = paValues[iReg].Reg128.Low64;
        pCtx->pXStateR0->x87.aXMM[12].uXmm.s.Hi = paValues[iReg].Reg128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aXMM[13].uXmm.s.Lo = paValues[iReg].Reg128.Low64;
        pCtx->pXStateR0->x87.aXMM[13].uXmm.s.Hi = paValues[iReg].Reg128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aXMM[14].uXmm.s.Lo = paValues[iReg].Reg128.Low64;
        pCtx->pXStateR0->x87.aXMM[14].uXmm.s.Hi = paValues[iReg].Reg128.High64;
        iReg++;
        pCtx->pXStateR0->x87.aXMM[15].uXmm.s.Lo = paValues[iReg].Reg128.Low64;
        pCtx->pXStateR0->x87.aXMM[15].uXmm.s.Hi = paValues[iReg].Reg128.High64;
        iReg++;
    }


    /* MSRs */
    // HvX64RegisterTsc - don't touch
    if (fWhat & CPUMCTX_EXTRN_EFER)
    {
        Assert(pInput->Names[iReg] == HvX64RegisterEfer);
        if (paValues[iReg].Reg64 != pCtx->msrEFER)
        {
            Log7(("NEM/%u: MSR EFER changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtx->msrEFER, paValues[iReg].Reg64));
            if ((paValues[iReg].Reg64 ^ pCtx->msrEFER) & MSR_K6_EFER_NXE)
                PGMNotifyNxeChanged(pGVCpu, RT_BOOL(paValues[iReg].Reg64 & MSR_K6_EFER_NXE));
            pCtx->msrEFER = paValues[iReg].Reg64;
            fMaybeChangedMode = true;
        }
        iReg++;
    }
    if (fWhat & CPUMCTX_EXTRN_KERNEL_GS_BASE)
    {
        Assert(pInput->Names[iReg] == HvX64RegisterKernelGsBase);
        if (pCtx->msrKERNELGSBASE != paValues[iReg].Reg64)
            Log7(("NEM/%u: MSR KERNELGSBASE changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtx->msrKERNELGSBASE, paValues[iReg].Reg64));
        pCtx->msrKERNELGSBASE = paValues[iReg].Reg64;
        iReg++;
    }
    if (fWhat & CPUMCTX_EXTRN_SYSENTER_MSRS)
    {
        Assert(pInput->Names[iReg] == HvX64RegisterSysenterCs);
        if (pCtx->SysEnter.cs != paValues[iReg].Reg64)
            Log7(("NEM/%u: MSR SYSENTER.CS changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtx->SysEnter.cs, paValues[iReg].Reg64));
        pCtx->SysEnter.cs = paValues[iReg].Reg64;
        iReg++;

        Assert(pInput->Names[iReg] == HvX64RegisterSysenterEip);
        if (pCtx->SysEnter.eip != paValues[iReg].Reg64)
            Log7(("NEM/%u: MSR SYSENTER.EIP changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtx->SysEnter.eip, paValues[iReg].Reg64));
        pCtx->SysEnter.eip = paValues[iReg].Reg64;
        iReg++;

        Assert(pInput->Names[iReg] == HvX64RegisterSysenterEsp);
        if (pCtx->SysEnter.esp != paValues[iReg].Reg64)
            Log7(("NEM/%u: MSR SYSENTER.ESP changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtx->SysEnter.esp, paValues[iReg].Reg64));
        pCtx->SysEnter.esp = paValues[iReg].Reg64;
        iReg++;
    }
    if (fWhat & CPUMCTX_EXTRN_SYSCALL_MSRS)
    {
        Assert(pInput->Names[iReg] == HvX64RegisterStar);
        if (pCtx->msrSTAR != paValues[iReg].Reg64)
            Log7(("NEM/%u: MSR STAR changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtx->msrSTAR, paValues[iReg].Reg64));
        pCtx->msrSTAR   = paValues[iReg].Reg64;
        iReg++;

        Assert(pInput->Names[iReg] == HvX64RegisterLstar);
        if (pCtx->msrLSTAR != paValues[iReg].Reg64)
            Log7(("NEM/%u: MSR LSTAR changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtx->msrLSTAR, paValues[iReg].Reg64));
        pCtx->msrLSTAR  = paValues[iReg].Reg64;
        iReg++;

        Assert(pInput->Names[iReg] == HvX64RegisterCstar);
        if (pCtx->msrCSTAR != paValues[iReg].Reg64)
            Log7(("NEM/%u: MSR CSTAR changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtx->msrCSTAR, paValues[iReg].Reg64));
        pCtx->msrCSTAR  = paValues[iReg].Reg64;
        iReg++;

        Assert(pInput->Names[iReg] == HvX64RegisterSfmask);
        if (pCtx->msrSFMASK != paValues[iReg].Reg64)
            Log7(("NEM/%u: MSR SFMASK changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtx->msrSFMASK, paValues[iReg].Reg64));
        pCtx->msrSFMASK = paValues[iReg].Reg64;
        iReg++;
    }
    if (fWhat & CPUMCTX_EXTRN_OTHER_MSRS)
    {
        Assert(pInput->Names[iReg] == HvX64RegisterApicBase);
        const uint64_t uOldBase = APICGetBaseMsrNoCheck(pGVCpu);
        if (paValues[iReg].Reg64 != uOldBase)
        {
            Log7(("NEM/%u: MSR APICBase changed %RX64 -> %RX64 (%RX64)\n",
                  pGVCpu->idCpu, uOldBase, paValues[iReg].Reg64, paValues[iReg].Reg64 ^ uOldBase));
            int rc2 = APICSetBaseMsr(pGVCpu, paValues[iReg].Reg64);
            AssertLogRelMsg(rc2 == VINF_SUCCESS, ("rc2=%Rrc [%#RX64]\n", rc2, paValues[iReg].Reg64));
        }
        iReg++;

        Assert(pInput->Names[iReg] == HvX64RegisterPat);
        if (pCtx->msrPAT != paValues[iReg].Reg64)
            Log7(("NEM/%u: MSR PAT changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtx->msrPAT, paValues[iReg].Reg64));
        pCtx->msrPAT    = paValues[iReg].Reg64;
        iReg++;

# if 0 /*def LOG_ENABLED*/ /** @todo something's wrong with HvX64RegisterMtrrCap? (AMD) */
        Assert(pInput->Names[iReg] == HvX64RegisterMtrrCap);
        if (paValues[iReg].Reg64 != CPUMGetGuestIa32MtrrCap(pGVCpu))
            Log7(("NEM/%u: MSR MTRR_CAP changed %RX64 -> %RX64 (!!)\n", pGVCpu->idCpu, CPUMGetGuestIa32MtrrCap(pGVCpu), paValues[iReg].Reg64));
        iReg++;
# endif

        PCPUMCTXMSRS pCtxMsrs = CPUMQueryGuestCtxMsrsPtr(pGVCpu);
        Assert(pInput->Names[iReg] == HvX64RegisterMtrrDefType);
        if (paValues[iReg].Reg64 != pCtxMsrs->msr.MtrrDefType )
            Log7(("NEM/%u: MSR MTRR_DEF_TYPE changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtxMsrs->msr.MtrrDefType, paValues[iReg].Reg64));
        pCtxMsrs->msr.MtrrDefType = paValues[iReg].Reg64;
        iReg++;

        /** @todo we dont keep state for HvX64RegisterMtrrPhysBaseX and HvX64RegisterMtrrPhysMaskX */

        Assert(pInput->Names[iReg] == HvX64RegisterMtrrFix64k00000);
        if (paValues[iReg].Reg64 != pCtxMsrs->msr.MtrrFix64K_00000 )
            Log7(("NEM/%u: MSR MTRR_FIX16K_00000 changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtxMsrs->msr.MtrrFix64K_00000, paValues[iReg].Reg64));
        pCtxMsrs->msr.MtrrFix64K_00000 = paValues[iReg].Reg64;
        iReg++;

        Assert(pInput->Names[iReg] == HvX64RegisterMtrrFix16k80000);
        if (paValues[iReg].Reg64 != pCtxMsrs->msr.MtrrFix16K_80000 )
            Log7(("NEM/%u: MSR MTRR_FIX16K_80000 changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtxMsrs->msr.MtrrFix16K_80000, paValues[iReg].Reg64));
        pCtxMsrs->msr.MtrrFix16K_80000 = paValues[iReg].Reg64;
        iReg++;

        Assert(pInput->Names[iReg] == HvX64RegisterMtrrFix16kA0000);
        if (paValues[iReg].Reg64 != pCtxMsrs->msr.MtrrFix16K_A0000 )
            Log7(("NEM/%u: MSR MTRR_FIX16K_A0000 changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtxMsrs->msr.MtrrFix16K_A0000, paValues[iReg].Reg64));
        pCtxMsrs->msr.MtrrFix16K_A0000 = paValues[iReg].Reg64;
        iReg++;

        Assert(pInput->Names[iReg] == HvX64RegisterMtrrFix4kC0000);
        if (paValues[iReg].Reg64 != pCtxMsrs->msr.MtrrFix4K_C0000 )
            Log7(("NEM/%u: MSR MTRR_FIX16K_C0000 changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtxMsrs->msr.MtrrFix4K_C0000, paValues[iReg].Reg64));
        pCtxMsrs->msr.MtrrFix4K_C0000 = paValues[iReg].Reg64;
        iReg++;

        Assert(pInput->Names[iReg] == HvX64RegisterMtrrFix4kC8000);
        if (paValues[iReg].Reg64 != pCtxMsrs->msr.MtrrFix4K_C8000 )
            Log7(("NEM/%u: MSR MTRR_FIX16K_C8000 changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtxMsrs->msr.MtrrFix4K_C8000, paValues[iReg].Reg64));
        pCtxMsrs->msr.MtrrFix4K_C8000 = paValues[iReg].Reg64;
        iReg++;

        Assert(pInput->Names[iReg] == HvX64RegisterMtrrFix4kD0000);
        if (paValues[iReg].Reg64 != pCtxMsrs->msr.MtrrFix4K_D0000 )
            Log7(("NEM/%u: MSR MTRR_FIX16K_D0000 changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtxMsrs->msr.MtrrFix4K_D0000, paValues[iReg].Reg64));
        pCtxMsrs->msr.MtrrFix4K_D0000 = paValues[iReg].Reg64;
        iReg++;

        Assert(pInput->Names[iReg] == HvX64RegisterMtrrFix4kD8000);
        if (paValues[iReg].Reg64 != pCtxMsrs->msr.MtrrFix4K_D8000 )
            Log7(("NEM/%u: MSR MTRR_FIX16K_D8000 changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtxMsrs->msr.MtrrFix4K_D8000, paValues[iReg].Reg64));
        pCtxMsrs->msr.MtrrFix4K_D8000 = paValues[iReg].Reg64;
        iReg++;

        Assert(pInput->Names[iReg] == HvX64RegisterMtrrFix4kE0000);
        if (paValues[iReg].Reg64 != pCtxMsrs->msr.MtrrFix4K_E0000 )
            Log7(("NEM/%u: MSR MTRR_FIX16K_E0000 changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtxMsrs->msr.MtrrFix4K_E0000, paValues[iReg].Reg64));
        pCtxMsrs->msr.MtrrFix4K_E0000 = paValues[iReg].Reg64;
        iReg++;

        Assert(pInput->Names[iReg] == HvX64RegisterMtrrFix4kE8000);
        if (paValues[iReg].Reg64 != pCtxMsrs->msr.MtrrFix4K_E8000 )
            Log7(("NEM/%u: MSR MTRR_FIX16K_E8000 changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtxMsrs->msr.MtrrFix4K_E8000, paValues[iReg].Reg64));
        pCtxMsrs->msr.MtrrFix4K_E8000 = paValues[iReg].Reg64;
        iReg++;

        Assert(pInput->Names[iReg] == HvX64RegisterMtrrFix4kF0000);
        if (paValues[iReg].Reg64 != pCtxMsrs->msr.MtrrFix4K_F0000 )
            Log7(("NEM/%u: MSR MTRR_FIX16K_F0000 changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtxMsrs->msr.MtrrFix4K_F0000, paValues[iReg].Reg64));
        pCtxMsrs->msr.MtrrFix4K_F0000 = paValues[iReg].Reg64;
        iReg++;

        Assert(pInput->Names[iReg] == HvX64RegisterMtrrFix4kF8000);
        if (paValues[iReg].Reg64 != pCtxMsrs->msr.MtrrFix4K_F8000 )
            Log7(("NEM/%u: MSR MTRR_FIX16K_F8000 changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtxMsrs->msr.MtrrFix4K_F8000, paValues[iReg].Reg64));
        pCtxMsrs->msr.MtrrFix4K_F8000 = paValues[iReg].Reg64;
        iReg++;

        Assert(pInput->Names[iReg] == HvX64RegisterTscAux);
        if (paValues[iReg].Reg64 != pCtxMsrs->msr.TscAux )
            Log7(("NEM/%u: MSR TSC_AUX changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtxMsrs->msr.TscAux, paValues[iReg].Reg64));
        pCtxMsrs->msr.TscAux = paValues[iReg].Reg64;
        iReg++;

# if 0 /** @todo why can't we even read HvX64RegisterIa32MiscEnable? */
        if (enmCpuVendor != CPUMCPUVENDOR_AMD)
        {
            Assert(pInput->Names[iReg] == HvX64RegisterIa32MiscEnable);
            if (paValues[iReg].Reg64 != pCtxMsrs->msr.MiscEnable)
                Log7(("NEM/%u: MSR MISC_ENABLE changed %RX64 -> %RX64\n", pGVCpu->idCpu, pCtxMsrs->msr.MiscEnable, paValues[iReg].Reg64));
            pCtxMsrs->msr.MiscEnable = paValues[iReg].Reg64;
            iReg++;
        }
# endif
# ifdef LOG_ENABLED
        if (enmCpuVendor != CPUMCPUVENDOR_AMD && enmCpuVendor != CPUMCPUVENDOR_HYGON)
        {
            Assert(pInput->Names[iReg] == HvX64RegisterIa32FeatureControl);
            if (paValues[iReg].Reg64 != pCtx->hwvirt.vmx.Msrs.u64FeatCtrl)
                Log7(("NEM/%u: MSR FEATURE_CONTROL changed %RX64 -> %RX64 (!!)\n", pGVCpu->idCpu, pCtx->hwvirt.vmx.Msrs.u64FeatCtrl, paValues[iReg].Reg64));
            iReg++;
        }
# endif
    }

    /* Interruptibility. */
    if (fWhat & (CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT | CPUMCTX_EXTRN_NEM_WIN_INHIBIT_NMI))
    {
        Assert(pInput->Names[iReg] == HvRegisterInterruptState);
        Assert(pInput->Names[iReg + 1] == HvX64RegisterRip);

        if (!(pCtx->fExtrn & CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT))
        {
            pGVCpu->nem.s.fLastInterruptShadow = paValues[iReg].InterruptState.InterruptShadow;
            if (paValues[iReg].InterruptState.InterruptShadow)
                EMSetInhibitInterruptsPC(pGVCpu, paValues[iReg + 1].Reg64);
            else
                VMCPU_FF_CLEAR(pGVCpu, VMCPU_FF_INHIBIT_INTERRUPTS);
        }

        if (!(pCtx->fExtrn & CPUMCTX_EXTRN_NEM_WIN_INHIBIT_NMI))
        {
            if (paValues[iReg].InterruptState.NmiMasked)
                VMCPU_FF_SET(pGVCpu, VMCPU_FF_BLOCK_NMIS);
            else
                VMCPU_FF_CLEAR(pGVCpu, VMCPU_FF_BLOCK_NMIS);
        }

        fWhat |= CPUMCTX_EXTRN_NEM_WIN_INHIBIT_INT | CPUMCTX_EXTRN_NEM_WIN_INHIBIT_NMI;
        iReg += 2;
    }

    /* Event injection. */
    /// @todo HvRegisterPendingInterruption
    Assert(pInput->Names[iReg] == HvRegisterPendingInterruption);
    if (paValues[iReg].PendingInterruption.InterruptionPending)
    {
        Log7(("PendingInterruption: type=%u vector=%#x errcd=%RTbool/%#x instr-len=%u nested=%u\n",
              paValues[iReg].PendingInterruption.InterruptionType, paValues[iReg].PendingInterruption.InterruptionVector,
              paValues[iReg].PendingInterruption.DeliverErrorCode, paValues[iReg].PendingInterruption.ErrorCode,
              paValues[iReg].PendingInterruption.InstructionLength, paValues[iReg].PendingInterruption.NestedEvent));
        AssertMsg((paValues[iReg].PendingInterruption.AsUINT64 & UINT64_C(0xfc00)) == 0,
                  ("%#RX64\n", paValues[iReg].PendingInterruption.AsUINT64));
    }

    /// @todo HvRegisterPendingEvent0
    /// @todo HvRegisterPendingEvent1

    /* Almost done, just update extrn flags and maybe change PGM mode. */
    pCtx->fExtrn &= ~fWhat;
    if (!(pCtx->fExtrn & (CPUMCTX_EXTRN_ALL | (CPUMCTX_EXTRN_NEM_WIN_MASK & ~CPUMCTX_EXTRN_NEM_WIN_EVENT_INJECT))))
        pCtx->fExtrn = 0;

    /* Typical. */
    if (!fMaybeChangedMode && !fUpdateCr3)
        return VINF_SUCCESS;

    /*
     * Slow.
     */
    int rc = VINF_SUCCESS;
    if (fMaybeChangedMode)
    {
        rc = PGMChangeMode(pGVCpu, pCtx->cr0, pCtx->cr4, pCtx->msrEFER);
        AssertMsgReturn(rc == VINF_SUCCESS, ("rc=%Rrc\n", rc), RT_FAILURE_NP(rc) ? rc : VERR_NEM_IPE_1);
    }

    if (fUpdateCr3)
    {
        if (fCanUpdateCr3)
        {
            LogFlow(("nemR0WinImportState: -> PGMUpdateCR3!\n"));
            rc = PGMUpdateCR3(pGVCpu, pCtx->cr3);
            AssertMsgReturn(rc == VINF_SUCCESS, ("rc=%Rrc\n", rc), RT_FAILURE_NP(rc) ? rc : VERR_NEM_IPE_2);
        }
        else
        {
            LogFlow(("nemR0WinImportState: -> VERR_NEM_FLUSH_TLB!\n"));
            rc = VERR_NEM_FLUSH_TLB; /* Calling PGMFlushTLB w/o long jump setup doesn't work, ring-3 does it. */
        }
    }

    return rc;
}
#endif /* NEM_WIN_WITH_RING0_RUNLOOP || NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS */


/**
 * Import the state from the native API (back to CPUMCTX).
 *
 * @returns VBox status code
 * @param   pGVM        The ring-0 VM handle.
 * @param   idCpu       The calling EMT.  Necessary for getting the
 *                      hypercall page and arguments.
 * @param   fWhat       What to import, CPUMCTX_EXTRN_XXX. Set
 *                      CPUMCTX_EXTERN_ALL for everything.
 */
VMMR0_INT_DECL(int) NEMR0ImportState(PGVM pGVM, VMCPUID idCpu, uint64_t fWhat)
{
#if defined(NEM_WIN_WITH_RING0_RUNLOOP) || defined(NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS)
    /*
     * Validate the call.
     */
    int rc = GVMMR0ValidateGVMandEMT(pGVM, idCpu);
    if (RT_SUCCESS(rc))
    {
        PGVMCPU pGVCpu = &pGVM->aCpus[idCpu];
        AssertReturn(g_pfnHvlInvokeHypercall, VERR_NEM_MISSING_KERNEL_API_1);

        /*
         * Call worker.
         */
        rc = nemR0WinImportState(pGVM, pGVCpu, &pGVCpu->cpum.GstCtx, fWhat, false /*fCanUpdateCr3*/);
    }
    return rc;
#else
    RT_NOREF(pGVM, idCpu, fWhat);
    return VERR_NOT_IMPLEMENTED;
#endif
}


#if defined(NEM_WIN_WITH_RING0_RUNLOOP) || defined(NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS)
/**
 * Worker for NEMR0QueryCpuTick and the ring-0 NEMHCQueryCpuTick.
 *
 * @returns VBox status code.
 * @param   pGVM        The ring-0 VM handle.
 * @param   pGVCpu      The ring-0 VCPU handle.
 * @param   pcTicks     Where to return the current CPU tick count.
 * @param   pcAux       Where to return the hyper-V TSC_AUX value.  Optional.
 */
NEM_TMPL_STATIC int nemR0WinQueryCpuTick(PGVM pGVM, PGVMCPU pGVCpu, uint64_t *pcTicks, uint32_t *pcAux)
{
    /*
     * Hypercall parameters.
     */
    HV_INPUT_GET_VP_REGISTERS *pInput = (HV_INPUT_GET_VP_REGISTERS *)pGVCpu->nemr0.s.HypercallData.pbPage;
    AssertPtrReturn(pInput, VERR_INTERNAL_ERROR_3);
    AssertReturn(g_pfnHvlInvokeHypercall, VERR_NEM_MISSING_KERNEL_API_1);

    pInput->PartitionId = pGVM->nemr0.s.idHvPartition;
    pInput->VpIndex     = pGVCpu->idCpu;
    pInput->fFlags      = 0;
    pInput->Names[0]    = HvX64RegisterTsc;
    pInput->Names[1]    = HvX64RegisterTscAux;

    size_t const cbInput = RT_ALIGN_Z(RT_UOFFSETOF(HV_INPUT_GET_VP_REGISTERS, Names[2]), 32);
    HV_REGISTER_VALUE *paValues = (HV_REGISTER_VALUE *)((uint8_t *)pInput + cbInput);
    RT_BZERO(paValues, sizeof(paValues[0]) * 2);

    /*
     * Make the hypercall.
     */
    uint64_t uResult = g_pfnHvlInvokeHypercall(HV_MAKE_CALL_INFO(HvCallGetVpRegisters, 2),
                                               pGVCpu->nemr0.s.HypercallData.HCPhysPage,
                                               pGVCpu->nemr0.s.HypercallData.HCPhysPage + cbInput);
    AssertLogRelMsgReturn(uResult == HV_MAKE_CALL_REP_RET(2), ("uResult=%RX64 cRegs=%#x\n", uResult, 2),
                          VERR_NEM_GET_REGISTERS_FAILED);

    /*
     * Get results.
     */
    *pcTicks = paValues[0].Reg64;
    if (pcAux)
        *pcAux = paValues[0].Reg32;
    return VINF_SUCCESS;
}
#endif /* NEM_WIN_WITH_RING0_RUNLOOP || NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS */


/**
 * Queries the TSC and TSC_AUX values, putting the results in .
 *
 * @returns VBox status code
 * @param   pGVM        The ring-0 VM handle.
 * @param   idCpu       The calling EMT.  Necessary for getting the
 *                      hypercall page and arguments.
 */
VMMR0_INT_DECL(int) NEMR0QueryCpuTick(PGVM pGVM, VMCPUID idCpu)
{
#if defined(NEM_WIN_WITH_RING0_RUNLOOP) || defined(NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS)
    /*
     * Validate the call.
     */
    int rc = GVMMR0ValidateGVMandEMT(pGVM, idCpu);
    if (RT_SUCCESS(rc))
    {
        PGVMCPU pGVCpu = &pGVM->aCpus[idCpu];
        AssertReturn(g_pfnHvlInvokeHypercall, VERR_NEM_MISSING_KERNEL_API_1);

        /*
         * Call worker.
         */
        pGVCpu->nem.s.Hypercall.QueryCpuTick.cTicks = 0;
        pGVCpu->nem.s.Hypercall.QueryCpuTick.uAux   = 0;
        rc = nemR0WinQueryCpuTick(pGVM, pGVCpu, &pGVCpu->nem.s.Hypercall.QueryCpuTick.cTicks,
                                  &pGVCpu->nem.s.Hypercall.QueryCpuTick.uAux);
    }
    return rc;
#else
    RT_NOREF(pGVM, idCpu);
    return VERR_NOT_IMPLEMENTED;
#endif
}


#if defined(NEM_WIN_WITH_RING0_RUNLOOP) || defined(NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS)
/**
 * Worker for NEMR0ResumeCpuTickOnAll and the ring-0 NEMHCResumeCpuTickOnAll.
 *
 * @returns VBox status code.
 * @param   pGVM            The ring-0 VM handle.
 * @param   pGVCpu          The ring-0 VCPU handle.
 * @param   uPausedTscValue The TSC value at the time of pausing.
 */
NEM_TMPL_STATIC int nemR0WinResumeCpuTickOnAll(PGVM pGVM, PGVMCPU pGVCpu, uint64_t uPausedTscValue)
{
    AssertReturn(g_pfnHvlInvokeHypercall, VERR_NEM_MISSING_KERNEL_API_1);

    /*
     * Set up the hypercall parameters.
     */
    HV_INPUT_SET_VP_REGISTERS *pInput = (HV_INPUT_SET_VP_REGISTERS *)pGVCpu->nemr0.s.HypercallData.pbPage;
    AssertPtrReturn(pInput, VERR_INTERNAL_ERROR_3);

    pInput->PartitionId = pGVM->nemr0.s.idHvPartition;
    pInput->VpIndex     = 0;
    pInput->RsvdZ       = 0;
    pInput->Elements[0].Name  = HvX64RegisterTsc;
    pInput->Elements[0].Pad0  = 0;
    pInput->Elements[0].Pad1  = 0;
    pInput->Elements[0].Value.Reg128.High64 = 0;
    pInput->Elements[0].Value.Reg64 = uPausedTscValue;

    /*
     * Disable interrupts and do the first virtual CPU.
     */
    RTCCINTREG const fSavedFlags = ASMIntDisableFlags();
    uint64_t const   uFirstTsc   = ASMReadTSC();
    uint64_t uResult = g_pfnHvlInvokeHypercall(HV_MAKE_CALL_INFO(HvCallSetVpRegisters, 1),
                                               pGVCpu->nemr0.s.HypercallData.HCPhysPage, 0 /* no output */);
    AssertLogRelMsgReturnStmt(uResult == HV_MAKE_CALL_REP_RET(1), ("uResult=%RX64 uTsc=%#RX64\n", uResult, uPausedTscValue),
                              ASMSetFlags(fSavedFlags), VERR_NEM_SET_TSC);

    /*
     * Do secondary processors, adjusting for elapsed TSC and keeping finger crossed
     * that we don't introduce too much drift here.
     */
    for (VMCPUID iCpu = 1; iCpu < pGVM->cCpus; iCpu++)
    {
        Assert(pInput->PartitionId == pGVM->nemr0.s.idHvPartition);
        Assert(pInput->RsvdZ       == 0);
        Assert(pInput->Elements[0].Name == HvX64RegisterTsc);
        Assert(pInput->Elements[0].Pad0 == 0);
        Assert(pInput->Elements[0].Pad1 == 0);
        Assert(pInput->Elements[0].Value.Reg128.High64 == 0);

        pInput->VpIndex = iCpu;
        const uint64_t offDelta = (ASMReadTSC() - uFirstTsc);
        pInput->Elements[0].Value.Reg64 = uPausedTscValue + offDelta;

        uResult = g_pfnHvlInvokeHypercall(HV_MAKE_CALL_INFO(HvCallSetVpRegisters, 1),
                                          pGVCpu->nemr0.s.HypercallData.HCPhysPage, 0 /* no output */);
        AssertLogRelMsgReturnStmt(uResult == HV_MAKE_CALL_REP_RET(1),
                                  ("uResult=%RX64 uTsc=%#RX64 + %#RX64\n", uResult, uPausedTscValue, offDelta),
                                  ASMSetFlags(fSavedFlags), VERR_NEM_SET_TSC);
    }

    /*
     * Done.
     */
    ASMSetFlags(fSavedFlags);
    return VINF_SUCCESS;
}
#endif /* NEM_WIN_WITH_RING0_RUNLOOP || NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS */


/**
 * Sets the TSC register to @a uPausedTscValue on all CPUs.
 *
 * @returns VBox status code
 * @param   pGVM            The ring-0 VM handle.
 * @param   idCpu           The calling EMT.  Necessary for getting the
 *                          hypercall page and arguments.
 * @param   uPausedTscValue The TSC value at the time of pausing.
 */
VMMR0_INT_DECL(int) NEMR0ResumeCpuTickOnAll(PGVM pGVM, VMCPUID idCpu, uint64_t uPausedTscValue)
{
#if defined(NEM_WIN_WITH_RING0_RUNLOOP) || defined(NEM_WIN_USE_HYPERCALLS_FOR_REGISTERS)
    /*
     * Validate the call.
     */
    int rc = GVMMR0ValidateGVMandEMT(pGVM, idCpu);
    if (RT_SUCCESS(rc))
    {
        PGVMCPU pGVCpu = &pGVM->aCpus[idCpu];
        AssertReturn(g_pfnHvlInvokeHypercall, VERR_NEM_MISSING_KERNEL_API_1);

        /*
         * Call worker.
         */
        pGVCpu->nem.s.Hypercall.QueryCpuTick.cTicks = 0;
        pGVCpu->nem.s.Hypercall.QueryCpuTick.uAux   = 0;
        rc = nemR0WinResumeCpuTickOnAll(pGVM, pGVCpu, uPausedTscValue);
    }
    return rc;
#else
    RT_NOREF(pGVM, idCpu, uPausedTscValue);
    return VERR_NOT_IMPLEMENTED;
#endif
}


VMMR0_INT_DECL(VBOXSTRICTRC) NEMR0RunGuestCode(PGVM pGVM, VMCPUID idCpu)
{
#ifdef NEM_WIN_WITH_RING0_RUNLOOP
    if (pGVM->nemr0.s.fMayUseRing0Runloop)
        return nemHCWinRunGC(pGVM, &pGVM->aCpus[idCpu]);
    return VERR_NEM_RING3_ONLY;
#else
    RT_NOREF(pGVM, idCpu);
    return VERR_NOT_IMPLEMENTED;
#endif
}


/**
 * Updates statistics in the VM structure.
 *
 * @returns VBox status code.
 * @param   pGVM        The ring-0 VM handle.
 * @param   idCpu       The calling EMT, or NIL.  Necessary for getting the hypercall
 *                      page and arguments.
 */
VMMR0_INT_DECL(int)  NEMR0UpdateStatistics(PGVM pGVM, VMCPUID idCpu)
{
    /*
     * Validate the call.
     */
    int rc;
    if (idCpu == NIL_VMCPUID)
        rc = GVMMR0ValidateGVM(pGVM);
    else
        rc = GVMMR0ValidateGVMandEMT(pGVM, idCpu);
    if (RT_SUCCESS(rc))
    {
        AssertReturn(g_pfnHvlInvokeHypercall, VERR_NEM_MISSING_KERNEL_API_1);

        PNEMR0HYPERCALLDATA pHypercallData = idCpu != NIL_VMCPUID
                                           ? &pGVM->aCpus[idCpu].nemr0.s.HypercallData
                                           : &pGVM->nemr0.s.HypercallData;
        if (   RT_VALID_PTR(pHypercallData->pbPage)
            && pHypercallData->HCPhysPage != NIL_RTHCPHYS)
        {
            if (idCpu == NIL_VMCPUID)
                rc = RTCritSectEnter(&pGVM->nemr0.s.HypercallDataCritSect);
            if (RT_SUCCESS(rc))
            {
                /*
                 * Query the memory statistics for the partition.
                 */
                HV_INPUT_GET_MEMORY_BALANCE  *pInput = (HV_INPUT_GET_MEMORY_BALANCE *)pHypercallData->pbPage;
                pInput->TargetPartitionId                               = pGVM->nemr0.s.idHvPartition;
                pInput->ProximityDomainInfo.Flags.ProximityPreferred    = 0;
                pInput->ProximityDomainInfo.Flags.ProxyimityInfoValid   = 0;
                pInput->ProximityDomainInfo.Flags.Reserved              = 0;
                pInput->ProximityDomainInfo.Id                          = 0;

                HV_OUTPUT_GET_MEMORY_BALANCE *pOutput = (HV_OUTPUT_GET_MEMORY_BALANCE *)(pInput + 1);
                RT_ZERO(*pOutput);

                uint64_t uResult = g_pfnHvlInvokeHypercall(HvCallGetMemoryBalance,
                                                           pHypercallData->HCPhysPage,
                                                           pHypercallData->HCPhysPage + sizeof(*pInput));
                if (uResult == HV_STATUS_SUCCESS)
                {
                    pGVM->nem.s.R0Stats.cPagesAvailable = pOutput->PagesAvailable;
                    pGVM->nem.s.R0Stats.cPagesInUse     = pOutput->PagesInUse;
                    rc = VINF_SUCCESS;
                }
                else
                {
                    LogRel(("HvCallGetMemoryBalance -> %#RX64 (%#RX64 %#RX64)!!\n",
                            uResult, pOutput->PagesAvailable, pOutput->PagesInUse));
                    rc = VERR_NEM_IPE_0;
                }

                if (idCpu == NIL_VMCPUID)
                    RTCritSectLeave(&pGVM->nemr0.s.HypercallDataCritSect);
            }
        }
        else
            rc = VERR_WRONG_ORDER;
    }
    return rc;
}


#if 1 && defined(DEBUG_bird)
/**
 * Debug only interface for poking around and exploring Hyper-V stuff.
 *
 * @param   pGVM        The ring-0 VM handle.
 * @param   idCpu       The calling EMT.
 * @param   u64Arg      What to query.  0 == registers.
 */
VMMR0_INT_DECL(int) NEMR0DoExperiment(PGVM pGVM, VMCPUID idCpu, uint64_t u64Arg)
{
    /*
     * Resolve CPU structures.
     */
    int rc = GVMMR0ValidateGVMandEMT(pGVM, idCpu);
    if (RT_SUCCESS(rc))
    {
        AssertReturn(g_pfnHvlInvokeHypercall, VERR_NEM_MISSING_KERNEL_API_1);

        PGVMCPU pGVCpu = &pGVM->aCpus[idCpu];
        if (u64Arg == 0)
        {
            /*
             * Query register.
             */
            HV_INPUT_GET_VP_REGISTERS *pInput = (HV_INPUT_GET_VP_REGISTERS *)pGVCpu->nemr0.s.HypercallData.pbPage;
            AssertPtrReturn(pInput, VERR_INTERNAL_ERROR_3);

            size_t const cbInput = RT_ALIGN_Z(RT_UOFFSETOF(HV_INPUT_GET_VP_REGISTERS, Names[1]), 32);
            HV_REGISTER_VALUE *paValues = (HV_REGISTER_VALUE *)((uint8_t *)pInput + cbInput);
            RT_BZERO(paValues, sizeof(paValues[0]) * 1);

            pInput->PartitionId = pGVM->nemr0.s.idHvPartition;
            pInput->VpIndex     = pGVCpu->idCpu;
            pInput->fFlags      = 0;
            pInput->Names[0]    = (HV_REGISTER_NAME)pGVCpu->nem.s.Hypercall.Experiment.uItem;

            uint64_t uResult = g_pfnHvlInvokeHypercall(HV_MAKE_CALL_INFO(HvCallGetVpRegisters, 1),
                                                       pGVCpu->nemr0.s.HypercallData.HCPhysPage,
                                                       pGVCpu->nemr0.s.HypercallData.HCPhysPage + cbInput);
            pGVCpu->nem.s.Hypercall.Experiment.fSuccess = uResult == HV_MAKE_CALL_REP_RET(1);
            pGVCpu->nem.s.Hypercall.Experiment.uStatus  = uResult;
            pGVCpu->nem.s.Hypercall.Experiment.uLoValue = paValues[0].Reg128.Low64;
            pGVCpu->nem.s.Hypercall.Experiment.uHiValue = paValues[0].Reg128.High64;
            rc = VINF_SUCCESS;
        }
        else if (u64Arg == 1)
        {
            /*
             * Query partition property.
             */
            HV_INPUT_GET_PARTITION_PROPERTY *pInput = (HV_INPUT_GET_PARTITION_PROPERTY *)pGVCpu->nemr0.s.HypercallData.pbPage;
            AssertPtrReturn(pInput, VERR_INTERNAL_ERROR_3);

            size_t const cbInput = RT_ALIGN_Z(sizeof(*pInput), 32);
            HV_OUTPUT_GET_PARTITION_PROPERTY *pOutput = (HV_OUTPUT_GET_PARTITION_PROPERTY *)((uint8_t *)pInput + cbInput);
            pOutput->PropertyValue = 0;

            pInput->PartitionId  = pGVM->nemr0.s.idHvPartition;
            pInput->PropertyCode = (HV_PARTITION_PROPERTY_CODE)pGVCpu->nem.s.Hypercall.Experiment.uItem;
            pInput->uPadding     = 0;

            uint64_t uResult = g_pfnHvlInvokeHypercall(HvCallGetPartitionProperty,
                                                       pGVCpu->nemr0.s.HypercallData.HCPhysPage,
                                                       pGVCpu->nemr0.s.HypercallData.HCPhysPage + cbInput);
            pGVCpu->nem.s.Hypercall.Experiment.fSuccess = uResult == HV_STATUS_SUCCESS;
            pGVCpu->nem.s.Hypercall.Experiment.uStatus  = uResult;
            pGVCpu->nem.s.Hypercall.Experiment.uLoValue = pOutput->PropertyValue;
            pGVCpu->nem.s.Hypercall.Experiment.uHiValue = 0;
            rc = VINF_SUCCESS;
        }
        else if (u64Arg == 2)
        {
            /*
             * Set register.
             */
            HV_INPUT_SET_VP_REGISTERS *pInput = (HV_INPUT_SET_VP_REGISTERS *)pGVCpu->nemr0.s.HypercallData.pbPage;
            AssertPtrReturn(pInput, VERR_INTERNAL_ERROR_3);
            RT_BZERO(pInput, RT_UOFFSETOF(HV_INPUT_SET_VP_REGISTERS, Elements[1]));

            pInput->PartitionId = pGVM->nemr0.s.idHvPartition;
            pInput->VpIndex     = pGVCpu->idCpu;
            pInput->RsvdZ      = 0;
            pInput->Elements[0].Name = (HV_REGISTER_NAME)pGVCpu->nem.s.Hypercall.Experiment.uItem;
            pInput->Elements[0].Value.Reg128.High64 = pGVCpu->nem.s.Hypercall.Experiment.uHiValue;
            pInput->Elements[0].Value.Reg128.Low64  = pGVCpu->nem.s.Hypercall.Experiment.uLoValue;

            uint64_t uResult = g_pfnHvlInvokeHypercall(HV_MAKE_CALL_INFO(HvCallSetVpRegisters, 1),
                                                       pGVCpu->nemr0.s.HypercallData.HCPhysPage, 0);
            pGVCpu->nem.s.Hypercall.Experiment.fSuccess = uResult == HV_MAKE_CALL_REP_RET(1);
            pGVCpu->nem.s.Hypercall.Experiment.uStatus  = uResult;
            rc = VINF_SUCCESS;
        }
        else
            rc = VERR_INVALID_FUNCTION;
    }
    return rc;
}
#endif /* DEBUG_bird */


/* $Id: internal-r0drv-nt.h 86175 2020-09-19 09:19:38Z vboxsync $ */
/** @file
 * IPRT - Internal Header for the NT Ring-0 Driver Code.
 */

/*
 * Copyright (C) 2008-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef IPRT_INCLUDED_SRC_r0drv_nt_internal_r0drv_nt_h
#define IPRT_INCLUDED_SRC_r0drv_nt_internal_r0drv_nt_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cpuset.h>
#include <iprt/nt/nt.h>

RT_C_DECLS_BEGIN

/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
typedef ULONG (__stdcall *PFNMYEXSETTIMERRESOLUTION)(ULONG, BOOLEAN);
typedef VOID (__stdcall *PFNMYKEFLUSHQUEUEDDPCS)(VOID);
typedef VOID (__stdcall *PFNHALSENDSOFTWAREINTERRUPT)(ULONG ProcessorNumber, KIRQL Irql);
typedef int (__stdcall *PFNRTSENDIPI)(RTCPUID idCpu);
typedef ULONG_PTR (__stdcall *PFNRTKEIPIGENERICCALL)(PKIPI_BROADCAST_WORKER BroadcastFunction, ULONG_PTR  Context);
typedef ULONG (__stdcall *PFNRTRTLGETVERSION)(PRTL_OSVERSIONINFOEXW pVerInfo);
#ifndef RT_ARCH_AMD64
typedef ULONGLONG (__stdcall *PFNRTKEQUERYINTERRUPTTIME)(VOID);
typedef VOID (__stdcall *PFNRTKEQUERYSYSTEMTIME)(PLARGE_INTEGER pTime);
#endif
typedef ULONG64 (__stdcall *PFNRTKEQUERYINTERRUPTTIMEPRECISE)(PULONG64 pQpcTS);
typedef VOID (__stdcall *PFNRTKEQUERYSYSTEMTIMEPRECISE)(PLARGE_INTEGER pTime);


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
extern RTCPUSET                                g_rtMpNtCpuSet;
extern uint32_t                                g_cRtMpNtMaxGroups;
extern uint32_t                                g_cRtMpNtMaxCpus;
extern RTCPUID                                 g_aidRtMpNtByCpuSetIdx[RTCPUSET_MAX_CPUS];

extern decltype(ExAllocatePoolWithTag)        *g_pfnrtExAllocatePoolWithTag;
extern decltype(ExFreePoolWithTag)            *g_pfnrtExFreePoolWithTag;
extern PFNMYEXSETTIMERRESOLUTION               g_pfnrtNtExSetTimerResolution;
extern PFNMYKEFLUSHQUEUEDDPCS                  g_pfnrtNtKeFlushQueuedDpcs;
extern PFNHALREQUESTIPI_W7PLUS                 g_pfnrtHalRequestIpiW7Plus;
extern PFNHALREQUESTIPI_PRE_W7                 g_pfnrtHalRequestIpiPreW7;
extern PFNHALSENDSOFTWAREINTERRUPT             g_pfnrtNtHalSendSoftwareInterrupt;
extern PFNRTSENDIPI                            g_pfnrtMpPokeCpuWorker;
extern PFNRTKEIPIGENERICCALL                   g_pfnrtKeIpiGenericCall;
extern PFNKESETTARGETPROCESSORDPCEX            g_pfnrtKeSetTargetProcessorDpcEx;
extern PFNKEINITIALIZEAFFINITYEX               g_pfnrtKeInitializeAffinityEx;
extern PFNKEADDPROCESSORAFFINITYEX             g_pfnrtKeAddProcessorAffinityEx;
extern PFNKEGETPROCESSORINDEXFROMNUMBER        g_pfnrtKeGetProcessorIndexFromNumber;
extern PFNKEGETPROCESSORNUMBERFROMINDEX        g_pfnrtKeGetProcessorNumberFromIndex;
extern PFNKEGETCURRENTPROCESSORNUMBEREX        g_pfnrtKeGetCurrentProcessorNumberEx;
extern PFNKEQUERYACTIVEPROCESSORS              g_pfnrtKeQueryActiveProcessors;
extern PFNKEQUERYMAXIMUMPROCESSORCOUNT         g_pfnrtKeQueryMaximumProcessorCount;
extern PFNKEQUERYMAXIMUMPROCESSORCOUNTEX       g_pfnrtKeQueryMaximumProcessorCountEx;
extern PFNKEQUERYMAXIMUMGROUPCOUNT             g_pfnrtKeQueryMaximumGroupCount;
extern PFNKEQUERYACTIVEPROCESSORCOUNT          g_pfnrtKeQueryActiveProcessorCount;
extern PFNKEQUERYACTIVEPROCESSORCOUNTEX        g_pfnrtKeQueryActiveProcessorCountEx;
extern PFNKEQUERYLOGICALPROCESSORRELATIONSHIP  g_pfnrtKeQueryLogicalProcessorRelationship;
extern PFNKEREGISTERPROCESSORCHANGECALLBACK    g_pfnrtKeRegisterProcessorChangeCallback;
extern PFNKEDEREGISTERPROCESSORCHANGECALLBACK  g_pfnrtKeDeregisterProcessorChangeCallback;
extern decltype(KeSetImportanceDpc)           *g_pfnrtKeSetImportanceDpc;
extern decltype(KeSetTargetProcessorDpc)      *g_pfnrtKeSetTargetProcessorDpc;
extern decltype(KeInitializeTimerEx)          *g_pfnrtKeInitializeTimerEx;
extern PFNKESHOULDYIELDPROCESSOR               g_pfnrtKeShouldYieldProcessor;
extern decltype(MmProtectMdlSystemAddress)    *g_pfnrtMmProtectMdlSystemAddress;
extern decltype(MmAllocatePagesForMdl)        *g_pfnrtMmAllocatePagesForMdl;
extern decltype(MmFreePagesFromMdl)           *g_pfnrtMmFreePagesFromMdl;
extern decltype(MmMapLockedPagesSpecifyCache) *g_pfnrtMmMapLockedPagesSpecifyCache;
extern decltype(MmAllocateContiguousMemorySpecifyCache) *g_pfnrtMmAllocateContiguousMemorySpecifyCache;
extern decltype(MmSecureVirtualMemory)        *g_pfnrtMmSecureVirtualMemory;
extern decltype(MmUnsecureVirtualMemory)      *g_pfnrtMmUnsecureVirtualMemory;

extern PFNRTRTLGETVERSION                      g_pfnrtRtlGetVersion;
#ifdef RT_ARCH_X86
extern PFNRTKEQUERYINTERRUPTTIME               g_pfnrtKeQueryInterruptTime;
#endif
extern PFNRTKEQUERYINTERRUPTTIMEPRECISE        g_pfnrtKeQueryInterruptTimePrecise;
extern PFNRTKEQUERYSYSTEMTIMEPRECISE           g_pfnrtKeQuerySystemTimePrecise;

extern uint32_t                                g_offrtNtPbQuantumEnd;
extern uint32_t                                g_cbrtNtPbQuantumEnd;
extern uint32_t                                g_offrtNtPbDpcQueueDepth;

/** Makes an NT version for checking against g_uRtNtVersion. */
#define RTNT_MAKE_VERSION(uMajor, uMinor)       RT_MAKE_U32(uMinor, uMajor)

extern uint32_t                                g_uRtNtVersion;
extern uint8_t                                 g_uRtNtMajorVer;
extern uint8_t                                 g_uRtNtMinorVer;
extern uint32_t                                g_uRtNtBuildNo;

extern uintptr_t const                        *g_puRtMmHighestUserAddress;
extern uintptr_t const                        *g_puRtMmSystemRangeStart;


int __stdcall rtMpPokeCpuUsingFailureNotSupported(RTCPUID idCpu);
int __stdcall rtMpPokeCpuUsingDpc(RTCPUID idCpu);
int __stdcall rtMpPokeCpuUsingBroadcastIpi(RTCPUID idCpu);
int __stdcall rtMpPokeCpuUsingHalRequestIpiW7Plus(RTCPUID idCpu);
int __stdcall rtMpPokeCpuUsingHalRequestIpiPreW7(RTCPUID idCpu);

struct RTNTSDBOSVER;
DECLHIDDEN(int)  rtR0MpNtInit(struct RTNTSDBOSVER const *pOsVerInfo);
DECLHIDDEN(void) rtR0MpNtTerm(void);
DECLHIDDEN(int)  rtMpNtSetTargetProcessorDpc(KDPC *pDpc, RTCPUID idCpu);
#if defined(RT_ARCH_X86) && defined(NIL_RTDBGKRNLINFO)
DECLHIDDEN(int)  rtR0Nt3InitSymbols(RTDBGKRNLINFO hKrnlInfo);
#endif

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_SRC_r0drv_nt_internal_r0drv_nt_h */


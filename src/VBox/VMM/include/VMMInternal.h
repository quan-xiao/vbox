/* $Id: VMMInternal.h 85121 2020-07-08 19:33:26Z vboxsync $ */
/** @file
 * VMM - Internal header file.
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

#ifndef VMM_INCLUDED_SRC_include_VMMInternal_h
#define VMM_INCLUDED_SRC_include_VMMInternal_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/cdefs.h>
#include <VBox/sup.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/vmm.h>
#include <VBox/log.h>
#include <iprt/critsect.h>

#if !defined(IN_VMM_R3) && !defined(IN_VMM_R0) && !defined(IN_VMM_RC)
# error "Not in VMM! This is an internal header!"
#endif
#if HC_ARCH_BITS == 32
# error "32-bit hosts are no longer supported. Go back to 6.0 or earlier!"
#endif



/** @defgroup grp_vmm_int   Internals
 * @ingroup grp_vmm
 * @internal
 * @{
 */

/** @def VBOX_WITH_RC_RELEASE_LOGGING
 * Enables RC release logging. */
#define VBOX_WITH_RC_RELEASE_LOGGING

/** @def VBOX_WITH_R0_LOGGING
 * Enables Ring-0 logging (non-release).
 *
 * Ring-0 logging isn't 100% safe yet (thread id reuse / process exit cleanup),
 * so you have to sign up here by adding your defined(DEBUG_<userid>) to the
 * \#if, or by adding VBOX_WITH_R0_LOGGING to your LocalConfig.kmk.
 */
#if defined(DEBUG_sandervl) || defined(DEBUG_frank) || defined(DEBUG_ramshankar) || defined(DOXYGEN_RUNNING)
# define VBOX_WITH_R0_LOGGING
#endif

/** @def VBOX_STRICT_VMM_STACK
 * Enables VMM stack guard pages to catch stack over- and underruns. */
#if defined(VBOX_STRICT) || defined(DOXYGEN_RUNNING)
# define VBOX_STRICT_VMM_STACK
#endif


/**
 * The ring-0 logger instance wrapper.
 *
 * We need to be able to find the VM handle from the logger instance, so we wrap
 * it in this structure.
 */
typedef struct VMMR0LOGGER
{
    /** Pointer to Pointer to the VM. */
    R0PTRTYPE(PVMCC)            pVM;
    /** Size of the allocated logger instance (Logger). */
    uint32_t                    cbLogger;
    /** Flag indicating whether we've create the logger Ring-0 instance yet. */
    bool                        fCreated;
    /** Flag indicating whether we've disabled flushing (world switch) or not. */
    bool                        fFlushingDisabled;
    /** Flag indicating whether we've registered the instance already. */
    bool                        fRegistered;
    bool                        a8Alignment;
    /** The CPU ID. */
    VMCPUID                     idCpu;
#if HC_ARCH_BITS == 64
    uint32_t                    u32Alignment;
#endif
    /** The ring-0 logger instance. This extends beyond the size.  */
    RTLOGGER                    Logger;
} VMMR0LOGGER;
/** Pointer to a ring-0 logger instance wrapper. */
typedef VMMR0LOGGER *PVMMR0LOGGER;


/**
 * Jump buffer for the setjmp/longjmp like constructs used to
 * quickly 'call' back into Ring-3.
 */
typedef struct VMMR0JMPBUF
{
    /** Traditional jmp_buf stuff
     * @{ */
#if HC_ARCH_BITS == 32
    uint32_t                    ebx;
    uint32_t                    esi;
    uint32_t                    edi;
    uint32_t                    ebp;
    uint32_t                    esp;
    uint32_t                    eip;
    uint32_t                    eflags;
#endif
#if HC_ARCH_BITS == 64
    uint64_t                    rbx;
# ifdef RT_OS_WINDOWS
    uint64_t                    rsi;
    uint64_t                    rdi;
# endif
    uint64_t                    rbp;
    uint64_t                    r12;
    uint64_t                    r13;
    uint64_t                    r14;
    uint64_t                    r15;
    uint64_t                    rsp;
    uint64_t                    rip;
# ifdef RT_OS_WINDOWS
    uint128_t                   xmm6;
    uint128_t                   xmm7;
    uint128_t                   xmm8;
    uint128_t                   xmm9;
    uint128_t                   xmm10;
    uint128_t                   xmm11;
    uint128_t                   xmm12;
    uint128_t                   xmm13;
    uint128_t                   xmm14;
    uint128_t                   xmm15;
# endif
    uint64_t                    rflags;
#endif
    /** @} */

    /** Flag that indicates that we've done a ring-3 call. */
    bool                        fInRing3Call;
    /** The number of bytes we've saved. */
    uint32_t                    cbSavedStack;
    /** Pointer to the buffer used to save the stack.
     * This is assumed to be 8KB. */
    RTR0PTR                     pvSavedStack;
    /** Esp we we match against esp on resume to make sure the stack wasn't relocated. */
    RTHCUINTREG                 SpCheck;
    /** The esp we should resume execution with after the restore. */
    RTHCUINTREG                 SpResume;
    /** ESP/RSP at the time of the jump to ring 3. */
    RTHCUINTREG                 SavedEsp;
    /** EBP/RBP at the time of the jump to ring 3. */
    RTHCUINTREG                 SavedEbp;
    /** EIP/RIP within vmmR0CallRing3LongJmp for assisting unwinding. */
    RTHCUINTREG                 SavedEipForUnwind;
    /** Unwind: The vmmR0CallRing3SetJmp return address value. */
    RTHCUINTREG                 UnwindRetPcValue;
    /** Unwind: The vmmR0CallRing3SetJmp return address stack location. */
    RTHCUINTREG                 UnwindRetPcLocation;
#if HC_ARCH_BITS == 32
    /** Alignment padding. */
    uint32_t                    uPadding;
#endif

    /** Stats: Max amount of stack used. */
    uint32_t                    cbUsedMax;
    /** Stats: Average stack usage. (Avg = cbUsedTotal / cUsedTotal) */
    uint32_t                    cbUsedAvg;
    /** Stats: Total amount of stack used. */
    uint64_t                    cbUsedTotal;
    /** Stats: Number of stack usages. */
    uint64_t                    cUsedTotal;
} VMMR0JMPBUF;
/** Pointer to a ring-0 jump buffer. */
typedef VMMR0JMPBUF *PVMMR0JMPBUF;


/**
 * VMM Data (part of VM)
 */
typedef struct VMM
{
    /** Whether we should use the periodic preemption timers. */
    bool                        fUsePeriodicPreemptionTimers;
    /** Alignment padding. */
    bool                        afPadding0[7];

    /** The EMT yield timer. */
    PTMTIMERR3                  pYieldTimer;
    /** The period to the next timeout when suspended or stopped.
     * This is 0 when running. */
    uint32_t                    cYieldResumeMillies;
    /** The EMT yield timer interval (milliseconds). */
    uint32_t                    cYieldEveryMillies;
    /** The timestamp of the previous yield. (nano) */
    uint64_t                    u64LastYield;

    /** @name EMT Rendezvous
     * @{ */
    /** Semaphore to wait on upon entering ordered execution. */
    R3PTRTYPE(PRTSEMEVENT)      pahEvtRendezvousEnterOrdered;
    /** Semaphore to wait on upon entering for one-by-one execution. */
    RTSEMEVENT                  hEvtRendezvousEnterOneByOne;
    /** Semaphore to wait on upon entering for all-at-once execution. */
    RTSEMEVENTMULTI             hEvtMulRendezvousEnterAllAtOnce;
    /** Semaphore to wait on when done. */
    RTSEMEVENTMULTI             hEvtMulRendezvousDone;
    /** Semaphore the VMMR3EmtRendezvous caller waits on at the end. */
    RTSEMEVENT                  hEvtRendezvousDoneCaller;
    /** Semaphore to wait on upon recursing. */
    RTSEMEVENTMULTI             hEvtMulRendezvousRecursionPush;
    /** Semaphore to wait on after done with recursion (caller restoring state). */
    RTSEMEVENTMULTI             hEvtMulRendezvousRecursionPop;
    /** Semaphore the initiator waits on while the EMTs are getting into position
     *  on hEvtMulRendezvousRecursionPush. */
    RTSEMEVENT                  hEvtRendezvousRecursionPushCaller;
    /** Semaphore the initiator waits on while the EMTs sitting on
     *  hEvtMulRendezvousRecursionPop wakes up and leave. */
    RTSEMEVENT                  hEvtRendezvousRecursionPopCaller;
    /** Callback. */
    R3PTRTYPE(PFNVMMEMTRENDEZVOUS) volatile pfnRendezvous;
    /** The user argument for the callback. */
    RTR3PTR volatile            pvRendezvousUser;
    /** Flags. */
    volatile uint32_t           fRendezvousFlags;
    /** The number of EMTs that has entered. */
    volatile uint32_t           cRendezvousEmtsEntered;
    /** The number of EMTs that has done their job. */
    volatile uint32_t           cRendezvousEmtsDone;
    /** The number of EMTs that has returned. */
    volatile uint32_t           cRendezvousEmtsReturned;
    /** The status code. */
    volatile int32_t            i32RendezvousStatus;
    /** Spin lock. */
    volatile uint32_t           u32RendezvousLock;
    /** The recursion depth. */
    volatile uint32_t           cRendezvousRecursions;
    /** The number of EMTs that have entered the recursion routine. */
    volatile uint32_t           cRendezvousEmtsRecursingPush;
    /** The number of EMTs that have leaft the recursion routine. */
    volatile uint32_t           cRendezvousEmtsRecursingPop;
    /** Triggers rendezvous recursion in the other threads. */
    volatile bool               fRendezvousRecursion;

    /** @} */

    /** RTThreadPreemptIsPendingTrusty() result, set by vmmR0InitVM() for
     * release logging purposes. */
    bool                        fIsPreemptPendingApiTrusty : 1;
    /** The RTThreadPreemptIsPossible() result,  set by vmmR0InitVM() for
     * release logging purposes.  */
    bool                        fIsPreemptPossible : 1;

    bool                        afAlignment2[2]; /**< Alignment padding. */

    /** Buffer for storing the standard assertion message for a ring-0 assertion.
     * Used for saving the assertion message text for the release log and guru
     * meditation dump. */
    char                        szRing0AssertMsg1[512];
    /** Buffer for storing the custom message for a ring-0 assertion. */
    char                        szRing0AssertMsg2[256];

    /** Number of VMMR0_DO_HM_RUN or VMMR0_DO_NEM_RUN calls. */
    STAMCOUNTER                 StatRunGC;

    /** Statistics for each of the RC/R0 return codes.
     * @{ */
    STAMCOUNTER                 StatRZRetNormal;
    STAMCOUNTER                 StatRZRetInterrupt;
    STAMCOUNTER                 StatRZRetInterruptHyper;
    STAMCOUNTER                 StatRZRetGuestTrap;
    STAMCOUNTER                 StatRZRetRingSwitch;
    STAMCOUNTER                 StatRZRetRingSwitchInt;
    STAMCOUNTER                 StatRZRetStaleSelector;
    STAMCOUNTER                 StatRZRetIRETTrap;
    STAMCOUNTER                 StatRZRetEmulate;
    STAMCOUNTER                 StatRZRetPatchEmulate;
    STAMCOUNTER                 StatRZRetIORead;
    STAMCOUNTER                 StatRZRetIOWrite;
    STAMCOUNTER                 StatRZRetIOCommitWrite;
    STAMCOUNTER                 StatRZRetMMIORead;
    STAMCOUNTER                 StatRZRetMMIOWrite;
    STAMCOUNTER                 StatRZRetMMIOCommitWrite;
    STAMCOUNTER                 StatRZRetMMIOPatchRead;
    STAMCOUNTER                 StatRZRetMMIOPatchWrite;
    STAMCOUNTER                 StatRZRetMMIOReadWrite;
    STAMCOUNTER                 StatRZRetMSRRead;
    STAMCOUNTER                 StatRZRetMSRWrite;
    STAMCOUNTER                 StatRZRetLDTFault;
    STAMCOUNTER                 StatRZRetGDTFault;
    STAMCOUNTER                 StatRZRetIDTFault;
    STAMCOUNTER                 StatRZRetTSSFault;
    STAMCOUNTER                 StatRZRetCSAMTask;
    STAMCOUNTER                 StatRZRetSyncCR3;
    STAMCOUNTER                 StatRZRetMisc;
    STAMCOUNTER                 StatRZRetPatchInt3;
    STAMCOUNTER                 StatRZRetPatchPF;
    STAMCOUNTER                 StatRZRetPatchGP;
    STAMCOUNTER                 StatRZRetPatchIretIRQ;
    STAMCOUNTER                 StatRZRetRescheduleREM;
    STAMCOUNTER                 StatRZRetToR3Total;
    STAMCOUNTER                 StatRZRetToR3FF;
    STAMCOUNTER                 StatRZRetToR3Unknown;
    STAMCOUNTER                 StatRZRetToR3TMVirt;
    STAMCOUNTER                 StatRZRetToR3HandyPages;
    STAMCOUNTER                 StatRZRetToR3PDMQueues;
    STAMCOUNTER                 StatRZRetToR3Rendezvous;
    STAMCOUNTER                 StatRZRetToR3Timer;
    STAMCOUNTER                 StatRZRetToR3DMA;
    STAMCOUNTER                 StatRZRetToR3CritSect;
    STAMCOUNTER                 StatRZRetToR3Iem;
    STAMCOUNTER                 StatRZRetToR3Iom;
    STAMCOUNTER                 StatRZRetTimerPending;
    STAMCOUNTER                 StatRZRetInterruptPending;
    STAMCOUNTER                 StatRZRetCallRing3;
    STAMCOUNTER                 StatRZRetPATMDuplicateFn;
    STAMCOUNTER                 StatRZRetPGMChangeMode;
    STAMCOUNTER                 StatRZRetPendingRequest;
    STAMCOUNTER                 StatRZRetPGMFlushPending;
    STAMCOUNTER                 StatRZRetPatchTPR;
    STAMCOUNTER                 StatRZCallPDMCritSectEnter;
    STAMCOUNTER                 StatRZCallPDMLock;
    STAMCOUNTER                 StatRZCallLogFlush;
    STAMCOUNTER                 StatRZCallPGMPoolGrow;
    STAMCOUNTER                 StatRZCallPGMMapChunk;
    STAMCOUNTER                 StatRZCallPGMAllocHandy;
    STAMCOUNTER                 StatRZCallVMSetError;
    STAMCOUNTER                 StatRZCallVMSetRuntimeError;
    STAMCOUNTER                 StatRZCallPGMLock;
    /** @} */
} VMM;
/** Pointer to VMM. */
typedef VMM *PVMM;


/**
 * VMMCPU Data (part of VMCPU)
 */
typedef struct VMMCPU
{
    /** The last RC/R0 return code. */
    int32_t                     iLastGZRc;
    /** Alignment padding. */
    uint32_t                    u32Padding0;

    /** VMM stack, pointer to the top of the stack in R3.
     * Stack is allocated from the hypervisor heap and is page aligned
     * and always writable in RC. */
    R3PTRTYPE(uint8_t *)        pbEMTStackR3;

    /** Pointer to the R0 logger instance - R3 Ptr.
     * This is NULL if logging is disabled. */
    R3PTRTYPE(PVMMR0LOGGER)     pR0LoggerR3;
    /** Pointer to the R0 logger instance - R0 Ptr.
     * This is NULL if logging is disabled. */
    R0PTRTYPE(PVMMR0LOGGER)     pR0LoggerR0;

    /** Pointer to the R0 release logger instance - R3 Ptr.
     * This is NULL if logging is disabled. */
    R3PTRTYPE(PVMMR0LOGGER)     pR0RelLoggerR3;
    /** Pointer to the R0 release instance - R0 Ptr.
     * This is NULL if logging is disabled. */
    R0PTRTYPE(PVMMR0LOGGER)     pR0RelLoggerR0;

    /** Thread context switching hook (ring-0). */
    RTTHREADCTXHOOK             hCtxHook;

    /** @name Rendezvous
     * @{ */
    /** Whether the EMT is executing a rendezvous right now. For detecting
     *  attempts at recursive rendezvous. */
    bool volatile               fInRendezvous;
    bool                        afPadding1[10];
    /** @} */

    /** Whether we can HLT in VMMR0 rather than having to return to EM.
     * Updated by vmR3SetHaltMethodU(). */
    bool                        fMayHaltInRing0;
    /** The minimum delta for which we can HLT in ring-0 for.
     * The deadlines we can calculate are  from TM, so, if it's too close
     * we should just return to ring-3 and run the timer wheel, no point
     * in spinning in ring-0.
     * Updated by vmR3SetHaltMethodU(). */
    uint32_t                    cNsSpinBlockThreshold;
    /** Number of ring-0 halts (used for depreciating following values). */
    uint32_t                    cR0Halts;
    /** Number of ring-0 halts succeeding (VINF_SUCCESS) recently. */
    uint32_t                    cR0HaltsSucceeded;
    /** Number of ring-0 halts failing (VINF_EM_HALT) recently. */
    uint32_t                    cR0HaltsToRing3;
    /** Padding   */
    uint32_t                    u32Padding2;

    /** @name Raw-mode context tracing data.
     * @{ */
    SUPDRVTRACERUSRCTX          TracerCtx;
    /** @} */

    /** Alignment padding, making sure u64CallRing3Arg and CallRing3JmpBufR0 are nicely aligned. */
    uint32_t                    au32Padding3[1];

    /** @name Call Ring-3
     * Formerly known as host calls.
     * @{ */
    /** The disable counter. */
    uint32_t                    cCallRing3Disabled;
    /** The pending operation. */
    VMMCALLRING3                enmCallRing3Operation;
    /** The result of the last operation. */
    int32_t                     rcCallRing3;
    /** The argument to the operation. */
    uint64_t                    u64CallRing3Arg;
    /** The Ring-0 notification callback. */
    R0PTRTYPE(PFNVMMR0CALLRING3NOTIFICATION)   pfnCallRing3CallbackR0;
    /** The Ring-0 notification callback user argument. */
    R0PTRTYPE(void *)           pvCallRing3CallbackUserR0;
    /** The Ring-0 jmp buffer.
     * @remarks The size of this type isn't stable in assembly, so don't put
     *          anything that needs to be accessed from assembly after it. */
    VMMR0JMPBUF                 CallRing3JmpBufR0;
    /** @} */

    STAMPROFILE                 StatR0HaltBlock;
    STAMPROFILE                 StatR0HaltBlockOnTime;
    STAMPROFILE                 StatR0HaltBlockOverslept;
    STAMPROFILE                 StatR0HaltBlockInsomnia;
    STAMCOUNTER                 StatR0HaltExec;
    STAMCOUNTER                 StatR0HaltExecFromBlock;
    STAMCOUNTER                 StatR0HaltExecFromSpin;
    STAMCOUNTER                 StatR0HaltToR3FromSpin;
} VMMCPU;
AssertCompileMemberAlignment(VMMCPU, TracerCtx, 8);
/** Pointer to VMMCPU. */
typedef VMMCPU *PVMMCPU;



RT_C_DECLS_BEGIN

int  vmmInitFormatTypes(void);
void vmmTermFormatTypes(void);
uint32_t vmmGetBuildType(void);

#ifdef IN_RING3
int  vmmR3SwitcherInit(PVM pVM);
void vmmR3SwitcherRelocate(PVM pVM, RTGCINTPTR offDelta);
#endif /* IN_RING3 */

#ifdef IN_RING0

/**
 * World switcher assembly routine.
 * It will call VMMRCEntry().
 *
 * @returns return code from VMMRCEntry().
 * @param   pVM         The cross context VM structure.
 * @param   uArg        See VMMRCEntry().
 * @internal
 */
DECLASM(int)    vmmR0WorldSwitch(PVM pVM, unsigned uArg);

/**
 * Callback function for vmmR0CallRing3SetJmp.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
typedef DECLCALLBACKTYPE(int, FNVMMR0SETJMP,(PVMCC pVM, PVMCPUCC pVCpu));
/** Pointer to FNVMMR0SETJMP(). */
typedef FNVMMR0SETJMP *PFNVMMR0SETJMP;

/**
 * The setjmp variant used for calling Ring-3.
 *
 * This differs from the normal setjmp in that it will resume VMMRZCallRing3 if we're
 * in the middle of a ring-3 call. Another differences is the function pointer and
 * argument. This has to do with resuming code and the stack frame of the caller.
 *
 * @returns VINF_SUCCESS on success or whatever is passed to vmmR0CallRing3LongJmp.
 * @param   pJmpBuf     The jmp_buf to set.
 * @param   pfn         The function to be called when not resuming.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 */
DECLASM(int)    vmmR0CallRing3SetJmp(PVMMR0JMPBUF pJmpBuf, PFNVMMR0SETJMP pfn, PVM pVM, PVMCPU pVCpu);


/**
 * Callback function for vmmR0CallRing3SetJmp2.
 *
 * @returns VBox status code.
 * @param   pvUser      The user argument.
 */
typedef DECLCALLBACKTYPE(int, FNVMMR0SETJMP2,(PGVM pGVM, VMCPUID idCpu));
/** Pointer to FNVMMR0SETJMP2(). */
typedef FNVMMR0SETJMP2 *PFNVMMR0SETJMP2;

/**
 * Same as vmmR0CallRing3SetJmp except for the function signature.
 *
 * @returns VINF_SUCCESS on success or whatever is passed to vmmR0CallRing3LongJmp.
 * @param   pJmpBuf     The jmp_buf to set.
 * @param   pfn         The function to be called when not resuming.
 * @param   pGVM        The ring-0 VM structure.
 * @param   idCpu       The ID of the calling EMT.
 */
DECLASM(int)    vmmR0CallRing3SetJmp2(PVMMR0JMPBUF pJmpBuf, PFNVMMR0SETJMP2 pfn, PGVM pGVM, VMCPUID idCpu);


/**
 * Callback function for vmmR0CallRing3SetJmpEx.
 *
 * @returns VBox status code.
 * @param   pvUser      The user argument.
 */
typedef DECLCALLBACKTYPE(int, FNVMMR0SETJMPEX,(void *pvUser));
/** Pointer to FNVMMR0SETJMPEX(). */
typedef FNVMMR0SETJMPEX *PFNVMMR0SETJMPEX;

/**
 * Same as vmmR0CallRing3SetJmp except for the function signature.
 *
 * @returns VINF_SUCCESS on success or whatever is passed to vmmR0CallRing3LongJmp.
 * @param   pJmpBuf     The jmp_buf to set.
 * @param   pfn         The function to be called when not resuming.
 * @param   pvUser      The argument of that function.
 */
DECLASM(int)    vmmR0CallRing3SetJmpEx(PVMMR0JMPBUF pJmpBuf, PFNVMMR0SETJMPEX pfn, void *pvUser);


/**
 * Worker for VMMRZCallRing3.
 * This will save the stack and registers.
 *
 * @returns rc.
 * @param   pJmpBuf     Pointer to the jump buffer.
 * @param   rc          The return code.
 */
DECLASM(int)    vmmR0CallRing3LongJmp(PVMMR0JMPBUF pJmpBuf, int rc);

/**
 * Internal R0 logger worker: Logger wrapper.
 */
VMMR0DECL(void) vmmR0LoggerWrapper(const char *pszFormat, ...);

/**
 * Internal R0 logger worker: Flush logger.
 *
 * @param   pLogger     The logger instance to flush.
 * @remark  This function must be exported!
 */
VMMR0DECL(void) vmmR0LoggerFlush(PRTLOGGER pLogger);

/**
 * Internal R0 logger worker: Custom prefix.
 *
 * @returns Number of chars written.
 *
 * @param   pLogger     The logger instance.
 * @param   pchBuf      The output buffer.
 * @param   cchBuf      The size of the buffer.
 * @param   pvUser      User argument (ignored).
 */
VMMR0DECL(size_t) vmmR0LoggerPrefix(PRTLOGGER pLogger, char *pchBuf, size_t cchBuf, void *pvUser);

# ifdef VBOX_WITH_TRIPLE_FAULT_HACK
int  vmmR0TripleFaultHackInit(void);
void vmmR0TripleFaultHackTerm(void);
# endif

#endif /* IN_RING0 */

RT_C_DECLS_END

/** @} */

#endif /* !VMM_INCLUDED_SRC_include_VMMInternal_h */

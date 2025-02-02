/* $Id: GVMMR0Internal.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * GVMM - The Global VM Manager, Internal header.
 */

/*
 * Copyright (C) 2007-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VMM_INCLUDED_SRC_VMMR0_GVMMR0Internal_h
#define VMM_INCLUDED_SRC_VMMR0_GVMMR0Internal_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/mem.h>

/**
 * The GVMM per VM data.
 */
typedef struct GVMMPERVCPU
{
    /** The time the halted EMT thread expires.
     * 0 if the EMT thread is blocked here. */
    uint64_t volatile   u64HaltExpire;
    /** The event semaphore the EMT thread is blocking on. */
    RTSEMEVENTMULTI     HaltEventMulti;
    /** The ring-3 mapping of the VMCPU structure. */
    RTR0MEMOBJ          VMCpuMapObj;
    /** The APIC ID of the CPU that EMT was scheduled on the last time we checked.
     * @todo Extend to 32-bit and use most suitable APIC ID function when we
     *       start using this for something sensible... */
    uint8_t             iCpuEmt;
} GVMMPERVCPU;
/** Pointer to the GVMM per VCPU data. */
typedef GVMMPERVCPU *PGVMMPERVCPU;

/**
 * The GVMM per VM data.
 */
typedef struct GVMMPERVM
{
    /** The shared VM data structure allocation object (PVMR0). */
    RTR0MEMOBJ          VMMemObj;
    /** The Ring-3 mapping of the shared VM data structure (PVMR3). */
    RTR0MEMOBJ          VMMapObj;
    /** The allocation object for the VM pages. */
    RTR0MEMOBJ          VMPagesMemObj;
    /** The ring-3 mapping of the VM pages. */
    RTR0MEMOBJ          VMPagesMapObj;

    /** The scheduler statistics. */
    GVMMSTATSSCHED      StatsSched;

    /** Whether the per-VM ring-0 initialization has been performed. */
    bool                fDoneVMMR0Init;
    /** Whether the per-VM ring-0 termination is being or has been performed. */
    bool                fDoneVMMR0Term;
} GVMMPERVM;
/** Pointer to the GVMM per VM data. */
typedef GVMMPERVM *PGVMMPERVM;


#endif /* !VMM_INCLUDED_SRC_VMMR0_GVMMR0Internal_h */


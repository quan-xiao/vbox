/* $Id: sched-generic.cpp 86525 2020-10-11 18:28:52Z vboxsync $ */
/** @file
 * IPRT - Scheduling, generic stubs.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP RTLOGGROUP_THREAD
#include <iprt/thread.h>
#include "internal/iprt.h"

#include <iprt/log.h>
#include <iprt/assert.h>
#include <iprt/errcore.h>
#include "internal/sched.h"


/**
 * Calculate the scheduling properties for all the threads in the default
 * process priority, assuming the current thread have the type enmType.
 *
 * @returns iprt status code.
 * @param   enmType     The thread type to be assumed for the current thread.
 */
DECLHIDDEN(int) rtSchedNativeCalcDefaultPriority(RTTHREADTYPE enmType)
{
    Assert(enmType > RTTHREADTYPE_INVALID && enmType < RTTHREADTYPE_END);
    return VINF_SUCCESS;
}


DECLHIDDEN(int) rtProcNativeSetPriority(RTPROCPRIORITY enmPriority)
{
    Assert(enmPriority > RTPROCPRIORITY_INVALID && enmPriority < RTPROCPRIORITY_LAST);
    return VINF_SUCCESS;
}


DECLHIDDEN(int) rtThreadNativeSetPriority(PRTTHREADINT pThread, RTTHREADTYPE enmType)
{
    Assert(enmType > RTTHREADTYPE_INVALID && enmType < RTTHREADTYPE_END);
    return VINF_SUCCESS;
}


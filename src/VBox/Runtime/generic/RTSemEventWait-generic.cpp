/* $Id: RTSemEventWait-generic.cpp 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * IPRT - RTSemEventWait, generic RTSemEventWaitNoResume wrapper.
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
#define LOG_GROUP RTLOGGROUP_SEM
#define RTSEMEVENT_WITHOUT_REMAPPING
#include <iprt/semaphore.h>
#include "internal/iprt.h"

#include <iprt/time.h>
#include <iprt/errcore.h>
#include <iprt/assert.h>


RTDECL(int) RTSemEventWait(RTSEMEVENT EventSem, RTMSINTERVAL cMillies)
{
    int rc;
    if (cMillies == RT_INDEFINITE_WAIT)
    {
        do rc = RTSemEventWaitNoResume(EventSem, cMillies);
        while (rc == VERR_INTERRUPTED);
    }
    else
    {
        const uint64_t u64Start = RTTimeMilliTS();
        rc = RTSemEventWaitNoResume(EventSem, cMillies);
        if (rc == VERR_INTERRUPTED)
        {
            do
            {
                uint64_t u64Elapsed = RTTimeMilliTS() - u64Start;
                if (u64Elapsed >= cMillies)
                    return VERR_TIMEOUT;
                rc = RTSemEventWaitNoResume(EventSem, cMillies - (RTMSINTERVAL)u64Elapsed);
            } while (rc == VERR_INTERRUPTED);
        }
    }
    return rc;
}
RT_EXPORT_SYMBOL(RTSemEventWait);


/* $Id: VBoxGuestR3LibTime.cpp 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBoxGuestR3Lib - Ring-3 Support Library for VirtualBox guest additions, Time.
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
#include <iprt/time.h>
#include "VBoxGuestR3LibInternal.h"


VBGLR3DECL(int) VbglR3GetHostTime(PRTTIMESPEC pTime)
{
    VMMDevReqHostTime Req;
    vmmdevInitRequest(&Req.header, VMMDevReq_GetHostTime);
    Req.time = UINT64_MAX;
    int rc = vbglR3GRPerform(&Req.header);
    if (RT_SUCCESS(rc))
        RTTimeSpecSetMilli(pTime, (int64_t)Req.time);
    return rc;
}


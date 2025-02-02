/* $Id: HBDMgmt-generic.cpp 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox storage devices: Host block device management API.
 */

/*
 * Copyright (C) 2015-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#include <VBox/cdefs.h>
#include <iprt/errcore.h>
#include <iprt/assert.h>

#include "HBDMgmt.h"

DECLHIDDEN(int) HBDMgrCreate(PHBDMGR phHbdMgr)
{
    AssertPtrReturn(phHbdMgr, VERR_INVALID_POINTER);
    *phHbdMgr = NIL_HBDMGR;
    return VINF_SUCCESS;
}

DECLHIDDEN(void) HBDMgrDestroy(HBDMGR hHbdMgr)
{
    NOREF(hHbdMgr);
}

DECLHIDDEN(bool) HBDMgrIsBlockDevice(const char *pszFilename)
{
    NOREF(pszFilename);
    return false;
}

DECLHIDDEN(int) HBDMgrClaimBlockDevice(HBDMGR hHbdMgr, const char *pszFilename)
{
    NOREF(hHbdMgr);
    NOREF(pszFilename);
    return VINF_SUCCESS;
}

DECLHIDDEN(int) HBDMgrUnclaimBlockDevice(HBDMGR hHbdMgr, const char *pszFilename)
{
    NOREF(hHbdMgr);
    NOREF(pszFilename);
    return VINF_SUCCESS;
}

DECLHIDDEN(bool) HBDMgrIsBlockDeviceClaimed(HBDMGR hHbdMgr, const char *pszFilename)
{
    NOREF(hHbdMgr);
    NOREF(pszFilename);
    return false;
}

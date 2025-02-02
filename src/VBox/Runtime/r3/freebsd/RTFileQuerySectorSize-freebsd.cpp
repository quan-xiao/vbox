/* $Id: RTFileQuerySectorSize-freebsd.cpp 85875 2020-08-24 16:22:01Z vboxsync $ */
/** @file
 * IPRT - RTFileQuerySectorSize, FreeBSD.
 */

/*
 * Copyright (C) 2017-2020 Oracle Corporation
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
#include "internal/iprt.h"
#include <iprt/file.h>

#include <iprt/assert.h>
#include <iprt/errcore.h>

#include <errno.h>
#include <sys/disk.h>
#include <sys/stat.h>
#include <sys/ioctl.h>



RTDECL(int) RTFileQuerySectorSize(RTFILE hFile, uint32_t *pcbSector)
{
    AssertPtrReturn(pcbSector, VERR_INVALID_PARAMETER);

    int rc;
    int const fd = (int)RTFileToNative(hFile);
    struct stat DevStat = { 0 };
    if (!fstat(fd, &DevStat))
    {
        if (S_ISCHR(DevStat.st_mode))
        {
            u_int cbSector = 0;
            if (!ioctl(fd, DIOCGSECTORSIZE, &cbSector))
            {
                AssertReturn(cbSector > 0, VERR_INVALID_FUNCTION);
                *pcbSector = cbSector;
                return VINF_SUCCESS;
            }
            rc = RTErrConvertFromErrno(errno);
            AssertMsgFailed(("ioctl failed: errno=%d / %Rrc\n", errno, rc));
        }
        else
        {
            AssertMsgFailed(("not a character device.\n"));
            rc = VERR_INVALID_FUNCTION;
        }
    }
    else
    {
        rc = RTErrConvertFromErrno(errno);
        AssertMsgFailed(("fstat failed: errno=%d / %Rrc\n", errno, rc));
    }
    return rc;
}


/* $Id: RTStrCopyPEx.cpp 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * IPRT - RTStrCopyPEx.
 */

/*
 * Copyright (C) 2010-2020 Oracle Corporation
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
#include <iprt/string.h>
#include "internal/iprt.h"

#include <iprt/errcore.h>


RTDECL(int) RTStrCopyPEx(char **ppszDst, size_t *pcbDst, const char *pszSrc, size_t cchMaxSrc)
{
    const char  *pszSrcEol = RTStrEnd(pszSrc, cchMaxSrc);
    size_t       cchSrc    = pszSrcEol ? (size_t)(pszSrcEol - pszSrc) : cchMaxSrc;
    size_t const cbDst     = *pcbDst;
    char        *pszDst    = *ppszDst;
    if (RT_LIKELY(cchSrc < cbDst))
    {
        memcpy(pszDst, pszSrc, cchSrc);
        *ppszDst = pszDst += cchSrc;
        *pszDst  = '\0';
        *pcbDst -= cchSrc;
        return VINF_SUCCESS;
    }

    if (cbDst != 0)
    {
        memcpy(*ppszDst, pszSrc, cbDst - 1);
        *ppszDst = pszDst += cbDst - 1;
        *pszDst  = '\0';
        *pcbDst  = 1;
    }
    return VERR_BUFFER_OVERFLOW;
}
RT_EXPORT_SYMBOL(RTStrCopyPEx);


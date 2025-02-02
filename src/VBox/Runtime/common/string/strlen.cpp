/* $Id: strlen.cpp 85075 2020-07-06 17:17:30Z vboxsync $ */
/** @file
 * IPRT - CRT Strings, strlen().
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
#include <iprt/string.h>


/**
 * Find the length of a zeroterminated byte string.
 *
 * @returns String length in bytes.
 * @param   pszString   Zero terminated string.
 */
#ifdef _MSC_VER
# if _MSC_VER >= 1400
__checkReturn size_t  __cdecl strlen(__in_z  const char *pszString)
# else
size_t strlen(const char *pszString)
# endif
#else
size_t strlen(const char *pszString)
#endif
{
    const char *psz = pszString;
    while (*psz)
        psz++;
    return psz - pszString;
}


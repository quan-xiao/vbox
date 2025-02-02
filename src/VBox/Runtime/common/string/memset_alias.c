/* $Id: memset_alias.c 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * IPRT - No-CRT memset() alias for gcc.
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
#include <iprt/nocrt/string.h>
#undef memset

#if defined(RT_OS_DARWIN) || defined(RT_OS_WINDOWS)
# ifndef __MINGW32__
#  pragma weak memset
# endif

/* No alias support here (yet in the ming case). */
extern void *(memset)(void *pvDst, int ch, size_t cb)
{
    return RT_NOCRT(memset)(pvDst, ch, cb);
}

#elif __GNUC__ >= 4
/* create a weak alias. */
__asm__(".weak memset\t\n"
        " .set memset," RT_NOCRT_STR(memset) "\t\n");
#else
/* create a weak alias. */
extern __typeof(RT_NOCRT(memset)) memset __attribute__((weak, alias(RT_NOCRT_STR(memset))));
#endif


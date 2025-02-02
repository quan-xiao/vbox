/* $Id: memrchr.cpp 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * IPRT - CRT Strings, memrchr().
 */

/*
 * Copyright (C) 2018-2020 Oracle Corporation
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
 * Search for a given byte starting at the end of the block.
 *
 * @returns Pointer on a match or NULL otherwise.
 * @param   pb      Pointer to the block.
 * @param   ch      The char to search for.
 * @param   cb      The size of the block.
 */
void *memrchr(const char *pb, int ch, size_t cb)
{
    if (cb)
    {
        const char *pbCur = pb + cb - 1;

        while (cb)
        {
            if (*pbCur == ch)
                return (void *)pbCur;
            pbCur--;
            cb--;
        }
    }

    return NULL;
}


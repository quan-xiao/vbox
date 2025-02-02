/* $Id: bs3-mode-TrapInit.c 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * BS3Kit - Bs3TrapInit
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
#include "bs3kit-template-header.h"


#undef Bs3TrapInit
BS3_MODE_DEF(void, Bs3TrapInit,(void))
{
#if BS3_MODE_IS_RM_SYS(TMPL_MODE)
    Bs3TrapRmV86Init();
#elif BS3_MODE_IS_16BIT_SYS(TMPL_MODE)
    Bs3TrapRmV86Init();
    Bs3Trap16Init();
#elif BS3_MODE_IS_32BIT_SYS(TMPL_MODE)
    Bs3TrapRmV86Init();
    Bs3Trap32Init();
#elif BS3_MODE_IS_64BIT_SYS(TMPL_MODE)
    Bs3Trap64Init();
#else
# error "TMPL_MODE"
#endif
}


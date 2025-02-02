/* $Id: GIMMinimalInternal.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * GIM - Minimal, Internal header file.
 */

/*
 * Copyright (C) 2014-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VMM_INCLUDED_SRC_include_GIMMinimalInternal_h
#define VMM_INCLUDED_SRC_include_GIMMinimalInternal_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <VBox/types.h>

RT_C_DECLS_BEGIN

#ifdef IN_RING3
VMMR3_INT_DECL(int)         gimR3MinimalInit(PVM pVM);
VMMR3_INT_DECL(int)         gimR3MinimalInitCompleted(PVM pVM);
VMMR3_INT_DECL(void)        gimR3MinimalRelocate(PVM pVM, RTGCINTPTR offDelta);
#endif /* IN_RING3 */

RT_C_DECLS_END

#endif /* !VMM_INCLUDED_SRC_include_GIMMinimalInternal_h */


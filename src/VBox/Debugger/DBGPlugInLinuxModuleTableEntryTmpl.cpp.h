/* $Id: DBGPlugInLinuxModuleTableEntryTmpl.cpp.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * DBGPlugInLinux - Table entry template for struct module processing.
 */

/*
 * Copyright (C) 2008-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

    { LNX_VER, LNX_64BIT, RT_CONCAT(dbgDiggerLinuxLoadModule,LNX_SUFFIX) },

#undef LNX_VER
#undef LNX_SUFFIX


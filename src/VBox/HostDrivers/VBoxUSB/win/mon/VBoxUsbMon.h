/* $Id: VBoxUsbMon.h 85121 2020-07-08 19:33:26Z vboxsync $ */
/** @file
 * VBox USB Monitor
 */
/*
 * Copyright (C) 2011-2020 Oracle Corporation
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

#ifndef VBOX_INCLUDED_SRC_VBoxUSB_win_mon_VBoxUsbMon_h
#define VBOX_INCLUDED_SRC_VBoxUSB_win_mon_VBoxUsbMon_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <iprt/assert.h>
#include <VBox/sup.h>
#include <iprt/asm.h>
#include <VBox/log.h>

#ifdef DEBUG
/* disables filters */
//#define VBOXUSBMON_DBG_NO_FILTERS
/* disables pnp hooking */
//#define VBOXUSBMON_DBG_NO_PNPHOOK
#endif

#include "../../../win/VBoxDbgLog.h"
#include "../cmn/VBoxDrvTool.h"
#include "../cmn/VBoxUsbTool.h"

#include "VBoxUsbHook.h"
#include "VBoxUsbFlt.h"

PVOID VBoxUsbMonMemAlloc(SIZE_T cbBytes);
PVOID VBoxUsbMonMemAllocZ(SIZE_T cbBytes);
VOID VBoxUsbMonMemFree(PVOID pvMem);

NTSTATUS VBoxUsbMonGetDescriptor(PDEVICE_OBJECT pDevObj, void *buffer, int size, int type, int index, int language_id);
NTSTATUS VBoxUsbMonQueryBusRelations(PDEVICE_OBJECT pDevObj, PFILE_OBJECT pFileObj, PDEVICE_RELATIONS *pDevRelations);

void vboxUsbDbgPrintUnicodeString(PUNICODE_STRING pUnicodeString);

typedef DECLCALLBACKTYPE(BOOLEAN, FNVBOXUSBMONDEVWALKER,(PFILE_OBJECT pHubFile, PDEVICE_OBJECT pHubDo, PVOID pvContext));
typedef FNVBOXUSBMONDEVWALKER *PFNVBOXUSBMONDEVWALKER;

VOID vboxUsbMonHubDevWalk(PFNVBOXUSBMONDEVWALKER pfnWalker, PVOID pvWalker);

#endif /* !VBOX_INCLUDED_SRC_VBoxUSB_win_mon_VBoxUsbMon_h */

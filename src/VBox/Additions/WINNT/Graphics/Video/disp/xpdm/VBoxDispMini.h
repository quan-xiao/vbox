/* $Id: VBoxDispMini.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox XPDM Display driver, helper functions which interacts with our miniport driver
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
 */

#ifndef GA_INCLUDED_SRC_WINNT_Graphics_Video_disp_xpdm_VBoxDispMini_h
#define GA_INCLUDED_SRC_WINNT_Graphics_Video_disp_xpdm_VBoxDispMini_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "VBoxDisp.h"

int VBoxDispMPGetVideoModes(HANDLE hDriver, PVIDEO_MODE_INFORMATION *ppModesTable, ULONG *cModes);
int VBoxDispMPGetPointerCaps(HANDLE hDriver, PVIDEO_POINTER_CAPABILITIES pCaps);
int VBoxDispMPSetCurrentMode(HANDLE hDriver, ULONG ulMode);
int VBoxDispMPMapMemory(PVBOXDISPDEV pDev, PVIDEO_MEMORY_INFORMATION pMemInfo);
int VBoxDispMPUnmapMemory(PVBOXDISPDEV pDev);
int VBoxDispMPQueryHGSMIInfo(HANDLE hDriver, QUERYHGSMIRESULT *pInfo);
int VBoxDispMPQueryHGSMICallbacks(HANDLE hDriver, HGSMIQUERYCALLBACKS *pCallbacks);
int VBoxDispMPHGSMIQueryPortProcs(HANDLE hDriver, HGSMIQUERYCPORTPROCS *pPortProcs);
#ifdef VBOX_WITH_VIDEOHWACCEL
int VBoxDispMPVHWAQueryInfo(HANDLE hDriver, VHWAQUERYINFO *pInfo);
#endif
int VBoxDispMPSetColorRegisters(HANDLE hDriver, PVIDEO_CLUT pClut, DWORD cbClut);
int VBoxDispMPDisablePointer(HANDLE hDriver);
int VBoxDispMPSetPointerPosition(HANDLE hDriver, PVIDEO_POINTER_POSITION pPos);
int VBoxDispMPSetPointerAttrs(PVBOXDISPDEV pDev);
int VBoxDispMPSetVisibleRegion(HANDLE hDriver, PRTRECT pRects, DWORD cRects);
int VBoxDispMPResetDevice(HANDLE hDriver);
int VBoxDispMPShareVideoMemory(HANDLE hDriver, PVIDEO_SHARE_MEMORY pSMem, PVIDEO_SHARE_MEMORY_INFORMATION pSMemInfo);
int VBoxDispMPUnshareVideoMemory(HANDLE hDriver, PVIDEO_SHARE_MEMORY pSMem);
int VBoxDispMPQueryRegistryFlags(HANDLE hDriver, ULONG *pulFlags);

#endif /* !GA_INCLUDED_SRC_WINNT_Graphics_Video_disp_xpdm_VBoxDispMini_h */

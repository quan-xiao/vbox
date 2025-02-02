/* $Id: VBoxHook.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBoxHook -- Global windows hook dll.
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
 */

#ifndef GA_INCLUDED_WINNT_VBoxHook_h
#define GA_INCLUDED_WINNT_VBoxHook_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* custom messages as we must install the hook from the main thread */
/** @todo r=andy Use WM_APP + n offsets here! */
#define WM_VBOX_SEAMLESS_ENABLE                     0x2001
#define WM_VBOX_SEAMLESS_DISABLE                    0x2002
#define WM_VBOX_SEAMLESS_UPDATE                     0x2003
#define WM_VBOX_GRAPHICS_SUPPORTED                  0x2004
#define WM_VBOX_GRAPHICS_UNSUPPORTED                0x2005


#define VBOXHOOK_DLL_NAME              "VBoxHook.dll"
#define VBOXHOOK_GLOBAL_DT_EVENT_NAME  "Local\\VBoxHookDtNotifyEvent"
#define VBOXHOOK_GLOBAL_WT_EVENT_NAME  "Local\\VBoxHookWtNotifyEvent"

BOOL VBoxHookInstallActiveDesktopTracker(HMODULE hDll);
BOOL VBoxHookRemoveActiveDesktopTracker();

BOOL VBoxHookInstallWindowTracker(HMODULE hDll);
BOOL VBoxHookRemoveWindowTracker();

#endif /* !GA_INCLUDED_WINNT_VBoxHook_h */


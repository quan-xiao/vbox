/* $Id: dllmain-win.cpp 83124 2020-02-20 17:23:23Z vboxsync $ */
/** @file
 * IPRT - Win32 DllMain (Ring-3).
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
#include <iprt/win/windows.h>
#include <iprt/thread.h>
#include <iprt/param.h>
#include "internal/thread.h"



/**
 * Increases the load count on the IPRT DLL so it won't unload.
 *
 * This is a separate function so as to not overflow the stack of threads with
 * very little of it.
 *
 * @param   hModule     The IPRT DLL module handle.
 */
DECL_NO_INLINE(static, void) EnsureNoUnload(HMODULE hModule)
{
    WCHAR wszName[RTPATH_MAX];
    SetLastError(NO_ERROR);
    if (   GetModuleFileNameW(hModule, wszName, RT_ELEMENTS(wszName)) > 0
        && GetLastError() == NO_ERROR)
    {
        int cExtraLoads = 32;
        while (cExtraLoads-- > 0)
            LoadLibraryW(wszName);
    }
}


/**
 * The Dll main entry point.
 */
BOOL __stdcall DllMain(HANDLE hModule, DWORD dwReason, PVOID pvReserved)
{
    RT_NOREF_PV(pvReserved);

    switch (dwReason)
    {
        /*
         * When attaching to a process, we'd like to make sure IPRT stays put
         * and doesn't get unloaded.
         */
        case DLL_PROCESS_ATTACH:
            EnsureNoUnload((HMODULE)hModule);
            break;

        case DLL_PROCESS_DETACH:
        case DLL_THREAD_ATTACH:
        default:
            /* ignore */
            break;

        case DLL_THREAD_DETACH:
            rtThreadWinTlsDestruction();
            rtThreadNativeDetach();
            break;
    }
    return TRUE;
}


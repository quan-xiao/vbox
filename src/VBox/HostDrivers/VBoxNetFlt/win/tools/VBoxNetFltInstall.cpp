/* $Id: VBoxNetFltInstall.cpp 85121 2020-07-08 19:33:26Z vboxsync $ */
/** @file
 * NetFltInstall - VBoxNetFlt installer command line tool
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

#include <VBox/VBoxNetCfg-win.h>
#include <devguid.h>
#include <stdio.h>

#define NETFLT_ID L"sun_VBoxNetFlt"
#define VBOX_NETCFG_APP_NAME L"NetFltInstall"
#define VBOX_NETFLT_PT_INF L".\\VBoxNetFlt.inf"
#define VBOX_NETFLT_MP_INF L".\\VBoxNetFltM.inf"
#define VBOX_NETFLT_RETRIES 10


static DECLCALLBACK(void) winNetCfgLogger(const char *pszString)
{
    printf("%s", pszString);
}

/** Wrapper around GetfullPathNameW that will try an alternative INF location.
 *
 * The default location is the current directory.  If not found there, the
 * alternative location is the executable directory.  If not found there either,
 * the first alternative is present to the caller.
 */
static DWORD MyGetfullPathNameW(LPCWSTR pwszName, size_t cchFull, LPWSTR pwszFull)
{
    LPWSTR pwszFilePart;
    DWORD dwSize = GetFullPathNameW(pwszName, (DWORD)cchFull, pwszFull, &pwszFilePart);
    if (dwSize <= 0)
        return dwSize;

    /* if it doesn't exist, see if the file exists in the same directory as the executable. */
    if (GetFileAttributesW(pwszFull) == INVALID_FILE_ATTRIBUTES)
    {
        WCHAR wsz[512];
        DWORD cch = GetModuleFileNameW(GetModuleHandle(NULL), &wsz[0], RT_ELEMENTS(wsz));
        if (cch > 0)
        {
            while (cch > 0 && wsz[cch - 1] != '/' && wsz[cch - 1] != '\\' && wsz[cch - 1] != ':')
                cch--;
            unsigned i = 0;
            while (cch < RT_ELEMENTS(wsz))
            {
                wsz[cch] = pwszFilePart[i++];
                if (!wsz[cch])
                {
                    dwSize = GetFullPathNameW(wsz, (DWORD)cchFull, pwszFull, NULL);
                    if (dwSize > 0 && GetFileAttributesW(pwszFull) != INVALID_FILE_ATTRIBUTES)
                        return dwSize;
                    break;
                }
                cch++;
            }
        }
    }

    /* fallback */
    return GetFullPathNameW(pwszName, (DWORD)cchFull, pwszFull, NULL);
}

static int VBoxNetFltInstall()
{
    WCHAR wszPtInf[MAX_PATH];
    WCHAR wszMpInf[MAX_PATH];
    INetCfg *pnc;
    int rcExit = RTEXITCODE_FAILURE;

    VBoxNetCfgWinSetLogging(winNetCfgLogger);

    HRESULT hr = CoInitialize(NULL);
    if (hr == S_OK)
    {
        for (int i = 0;; i++)
        {
            LPWSTR pwszLockedBy = NULL;
            hr = VBoxNetCfgWinQueryINetCfg(&pnc, TRUE, VBOX_NETCFG_APP_NAME, 10000, &pwszLockedBy);
            if (hr == S_OK)
            {
                DWORD dwSize;
                dwSize = MyGetfullPathNameW(VBOX_NETFLT_PT_INF, RT_ELEMENTS(wszPtInf), wszPtInf);
                if (dwSize > 0)
                {
                    /** @todo add size check for (RT_ELEMENTS(wszPtInf) == dwSize (string length in WCHARs) */

                    dwSize = MyGetfullPathNameW(VBOX_NETFLT_MP_INF, RT_ELEMENTS(wszMpInf), wszMpInf);
                    if (dwSize > 0)
                    {
                        /** @todo add size check for (RT_ELEMENTS(wszMpInf) == dwSize (string length in WHCARs) */

                        LPCWSTR apwszInfs[] = { wszPtInf, wszMpInf };
                        hr = VBoxNetCfgWinNetFltInstall(pnc, apwszInfs, 2);
                        if (hr == S_OK)
                        {
                            wprintf(L"installed successfully\n");
                            rcExit = RTEXITCODE_SUCCESS;
                        }
                        else
                            wprintf(L"error installing VBoxNetFlt (%#lx)\n", hr);
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        wprintf(L"error getting full inf path for VBoxNetFltM.inf (%#lx)\n", hr);
                    }
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    wprintf(L"error getting full inf path for VBoxNetFlt.inf (%#lx)\n", hr);
                }

                VBoxNetCfgWinReleaseINetCfg(pnc, TRUE);
                break;
            }

            if (hr == NETCFG_E_NO_WRITE_LOCK && pwszLockedBy)
            {
                if (i < VBOX_NETFLT_RETRIES && !wcscmp(pwszLockedBy, L"6to4svc.dll"))
                {
                    wprintf(L"6to4svc.dll is holding the lock, retrying %d out of %d\n", i + 1, VBOX_NETFLT_RETRIES);
                    CoTaskMemFree(pwszLockedBy);
                }
                else
                {
                    wprintf(L"Error: write lock is owned by another application (%s), close the application and retry installing\n", pwszLockedBy);
                    CoTaskMemFree(pwszLockedBy);
                    break;
                }
            }
            else
            {
                wprintf(L"Error getting the INetCfg interface (%#lx)\n", hr);
                break;
            }
        }

        CoUninitialize();
    }
    else
        wprintf(L"Error initializing COM (%#lx)\n", hr);

    VBoxNetCfgWinSetLogging(NULL);

    return rcExit;
}

int __cdecl main(int argc, char **argv)
{
    RT_NOREF2(argc, argv);
    return VBoxNetFltInstall();
}

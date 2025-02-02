/* $Id: VBoxNetAdpInstall.cpp 85121 2020-07-08 19:33:26Z vboxsync $ */
/** @file
 * NetAdpInstall - VBoxNetAdp installer command line tool.
 */

/*
 * Copyright (C) 2009-2020 Oracle Corporation
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
#include <VBox/VBoxNetCfg-win.h>
#include <VBox/VBoxDrvCfg-win.h>
#include <stdio.h>
#include <devguid.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#define VBOX_NETADP_APP_NAME L"NetAdpInstall"

#define VBOX_NETADP_HWID L"sun_VBoxNetAdp"
#ifdef NDIS60
# define VBOX_NETADP_INF L"VBoxNetAdp6.inf"
#else
# define VBOX_NETADP_INF L"VBoxNetAdp.inf"
#endif


static DECLCALLBACK(void) winNetCfgLogger(const char *pszString)
{
    printf("%s\n", pszString);
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
            while (cch < sizeof(wsz) / sizeof(wsz[0]))
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


static int VBoxNetAdpInstall(void)
{
    VBoxNetCfgWinSetLogging(winNetCfgLogger);

    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        wprintf(L"adding host-only interface..\n");

        WCHAR wszInfFile[MAX_PATH];
        DWORD cwcInfFile = MyGetfullPathNameW(VBOX_NETADP_INF, RT_ELEMENTS(wszInfFile), wszInfFile);
        if (cwcInfFile > 0)
        {
            INetCfg *pnc;
            LPWSTR lpszLockedBy = NULL;
            hr = VBoxNetCfgWinQueryINetCfg(&pnc, TRUE, VBOX_NETADP_APP_NAME, 10000, &lpszLockedBy);
            if (hr == S_OK)
            {

                hr = VBoxNetCfgWinNetAdpInstall(pnc, wszInfFile);

                if (hr == S_OK)
                    wprintf(L"installed successfully\n");
                else
                    wprintf(L"error installing VBoxNetAdp (%#lx)\n", hr);

                VBoxNetCfgWinReleaseINetCfg(pnc, TRUE);
            }
            else
                wprintf(L"VBoxNetCfgWinQueryINetCfg failed: hr=%#lx\n", hr);
            /*
            hr = VBoxDrvCfgInfInstall(MpInf);
            if (FAILED(hr))
                printf("VBoxDrvCfgInfInstall failed %#x\n", hr);

            GUID guid;
            BSTR name, errMsg;

            hr = VBoxNetCfgWinCreateHostOnlyNetworkInterface (MpInf, true, &guid, &name, &errMsg);
            if (SUCCEEDED(hr))
            {
                ULONG ip, mask;
                hr = VBoxNetCfgWinGenHostOnlyNetworkNetworkIp(&ip, &mask);
                if (SUCCEEDED(hr))
                {
                    // ip returned by VBoxNetCfgWinGenHostOnlyNetworkNetworkIp is a network ip,
                    // i.e. 192.168.xxx.0, assign  192.168.xxx.1 for the hostonly adapter
                    ip = ip | (1 << 24);
                    hr = VBoxNetCfgWinEnableStaticIpConfig(&guid, ip, mask);
                    if (SUCCEEDED(hr))
                        printf("installation successful\n");
                    else
                        printf("VBoxNetCfgWinEnableStaticIpConfig failed: hr=%#lx\n", hr);
                }
                else
                    printf("VBoxNetCfgWinGenHostOnlyNetworkNetworkIp failed: hr=%#lx\n", hr);
            }
            else
                printf("VBoxNetCfgWinCreateHostOnlyNetworkInterface failed: hr=%#lx\n", hr);
            */
        }
        else
        {
            DWORD dwErr = GetLastError();
            wprintf(L"GetFullPathNameW failed: winEr = %lu\n", dwErr);
            hr = HRESULT_FROM_WIN32(dwErr);
        }
        CoUninitialize();
    }
    else
        wprintf(L"Error initializing COM (%#lx)\n", hr);

    VBoxNetCfgWinSetLogging(NULL);

    return SUCCEEDED(hr) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

static int VBoxNetAdpUninstall(void)
{
    VBoxNetCfgWinSetLogging(winNetCfgLogger);

    printf("uninstalling all host-only interfaces..\n");

    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        hr = VBoxNetCfgWinRemoveAllNetDevicesOfId(VBOX_NETADP_HWID);
        if (SUCCEEDED(hr))
        {
            hr = VBoxDrvCfgInfUninstallAllSetupDi(&GUID_DEVCLASS_NET, L"Net", VBOX_NETADP_HWID, 0/* could be SUOI_FORCEDELETE */);
            if (SUCCEEDED(hr))
                printf("uninstallation successful\n");
            else
                printf("uninstalled successfully, but failed to remove infs\n");
        }
        else
            printf("uninstall failed, hr=%#lx\n", hr);
        CoUninitialize();
    }
    else
        printf("Error initializing COM (%#lx)\n", hr);

    VBoxNetCfgWinSetLogging(NULL);

    return SUCCEEDED(hr) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

static int VBoxNetAdpUpdate(void)
{
    VBoxNetCfgWinSetLogging(winNetCfgLogger);

    printf("uninstalling all host-only interfaces..\n");

    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        BOOL fRebootRequired = FALSE;
        /*
         * Before we can update the driver for existing adapters we need to remove
         * all old driver packages from the driver cache. Otherwise we may end up
         * with both NDIS5 and NDIS6 versions of VBoxNetAdp in the cache which
         * will cause all sorts of trouble.
         */
        VBoxDrvCfgInfUninstallAllF(L"Net", VBOX_NETADP_HWID, SUOI_FORCEDELETE);
        hr = VBoxNetCfgWinUpdateHostOnlyNetworkInterface(VBOX_NETADP_INF, &fRebootRequired, VBOX_NETADP_HWID);
        if (SUCCEEDED(hr))
        {
            if (fRebootRequired)
                printf("!!REBOOT REQUIRED!!\n");
            printf("updated successfully\n");
        }
        else
            printf("update failed, hr=%#lx\n", hr);

        CoUninitialize();
    }
    else
        printf("Error initializing COM (%#lx)\n", hr);

    VBoxNetCfgWinSetLogging(NULL);

    return SUCCEEDED(hr) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

static int VBoxNetAdpDisable(void)
{
    VBoxNetCfgWinSetLogging(winNetCfgLogger);

    printf("disabling all host-only interfaces..\n");

    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        hr = VBoxNetCfgWinPropChangeAllNetDevicesOfId(VBOX_NETADP_HWID, VBOXNECTFGWINPROPCHANGE_TYPE_DISABLE);
        if (SUCCEEDED(hr))
            printf("disabling successful\n");
        else
            printf("disable failed, hr=%#lx\n", hr);

        CoUninitialize();
    }
    else
        printf("Error initializing COM (%#lx)\n", hr);

    VBoxNetCfgWinSetLogging(NULL);

    return SUCCEEDED(hr) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

static int VBoxNetAdpEnable(void)
{
    VBoxNetCfgWinSetLogging(winNetCfgLogger);

    printf("enabling all host-only interfaces..\n");

    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        hr = VBoxNetCfgWinPropChangeAllNetDevicesOfId(VBOX_NETADP_HWID, VBOXNECTFGWINPROPCHANGE_TYPE_ENABLE);
        if (SUCCEEDED(hr))
            printf("enabling successful\n");
        else
            printf("enabling failed, hr=%#lx\n", hr);

        CoUninitialize();
    }
    else
        printf("Error initializing COM (%#lx)\n", hr);

    VBoxNetCfgWinSetLogging(NULL);

    return SUCCEEDED(hr) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

static void printUsage(void)
{
    printf("host-only network adapter configuration tool\n"
            "  Usage: VBoxNetAdpInstall [cmd]\n"
            "    cmd can be one of the following values:\n"
            "       i  - install a new host-only interface (default command)\n"
            "       u  - uninstall all host-only interfaces\n"
            "       a  - update the host-only driver\n"
            "       d  - disable all host-only interfaces\n"
            "       e  - enable all host-only interfaces\n"
            "       h  - print this message\n");
}

int __cdecl main(int argc, char **argv)
{
    if (argc < 2)
        return VBoxNetAdpInstall();
    if (argc > 2)
    {
        printUsage();
        return RTEXITCODE_SYNTAX;
    }

    if (!strcmp(argv[1], "i"))
        return VBoxNetAdpInstall();
    if (!strcmp(argv[1], "u"))
        return VBoxNetAdpUninstall();
    if (!strcmp(argv[1], "a"))
        return VBoxNetAdpUpdate();
    if (!strcmp(argv[1], "d"))
        return VBoxNetAdpDisable();
    if (!strcmp(argv[1], "e"))
        return VBoxNetAdpEnable();

    printUsage();
    return !strcmp(argv[1], "h") ? RTEXITCODE_SUCCESS : RTEXITCODE_SYNTAX;
}

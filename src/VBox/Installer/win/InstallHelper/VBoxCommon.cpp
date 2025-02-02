/* $Id: VBoxCommon.cpp 86775 2020-10-30 18:24:36Z vboxsync $ */
/** @file
 * VBoxCommon - Misc helper routines for install helper.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/win/windows.h>

#include <tchar.h>
#include <stdio.h>

#include <msi.h>
#include <msiquery.h>

#include <iprt/utf16.h>


#if (_MSC_VER < 1400) /* Provide swprintf_s to VC < 8.0. */
int swprintf_s(WCHAR *buffer, size_t cbBuffer, const WCHAR *format, ...)
{
    int ret;
    va_list va;
    va_start(va, format);
    ret = _vsnwprintf(buffer, cbBuffer, format, va);
    va_end(va);
    return ret;
}
#endif

UINT VBoxGetMsiProp(MSIHANDLE hMsi, WCHAR *pwszName, WCHAR *pwszValue, DWORD dwSize)
{
    DWORD dwBuffer = 0;
    UINT uiRet = MsiGetPropertyW(hMsi, pwszName, L"", &dwBuffer);
    if (uiRet == ERROR_MORE_DATA)
    {
        ++dwBuffer;     /* On output does not include terminating null, so add 1. */

        if (dwBuffer > dwSize)
            return ERROR_MORE_DATA;

        ZeroMemory(pwszValue, dwSize);
        uiRet = MsiGetPropertyW(hMsi, pwszName, pwszValue, &dwBuffer);
    }
    return uiRet;
}

/**
 * Retrieves a MSI property (in UTF-8).
 *
 * Convenience function for VBoxGetMsiProp().
 *
 * @returns VBox status code.
 * @param   hMsi                MSI handle to use.
 * @param   pcszName            Name of property to retrieve.
 * @param   ppszValue           Where to store the allocated value on success.
 *                              Must be free'd using RTStrFree() by the caller.
 */
int VBoxGetMsiPropUtf8(MSIHANDLE hMsi, const char *pcszName, char **ppszValue)
{
    PRTUTF16 pwszName;
    int rc = RTStrToUtf16(pcszName, &pwszName);
    if (RT_SUCCESS(rc))
    {
        WCHAR wszValue[1024]; /* 1024 should be enough for everybody (tm). */
        if (VBoxGetMsiProp(hMsi, pwszName, wszValue, sizeof(wszValue)) == ERROR_SUCCESS)
        {
            rc = RTUtf16ToUtf8(wszValue, ppszValue);
        }
        else
            rc = VERR_NOT_FOUND;

        RTUtf16Free(pwszName);
    }

    return rc;
}

UINT VBoxSetMsiProp(MSIHANDLE hMsi, WCHAR *pwszName, WCHAR *pwszValue)
{
    return MsiSetPropertyW(hMsi, pwszName, pwszValue);
}


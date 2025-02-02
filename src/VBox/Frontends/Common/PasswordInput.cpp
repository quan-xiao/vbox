/* $Id: PasswordInput.cpp 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * Frontend shared bits - Password file and console input helpers.
 */

/*
 * Copyright (C) 2012-2020 Oracle Corporation
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
#include "PasswordInput.h"

#include <iprt/ctype.h>
#include <iprt/message.h>
#include <iprt/stream.h>

#include <VBox/com/errorprint.h>


/**
 * Reads a password from the password file.
 *
 * Only first line is used. The passwords length must be less than 512 bytes
 *
 * @param   pszFilename  The path to file containing the password
 * @param   pPasswd      The string where password will be returned
 * @returns RTEXITCODE_SUCCESS or RTEXITCODE_FAILURE + msg.
 */
RTEXITCODE readPasswordFile(const char *pszFilename, com::Utf8Str *pPasswd)
{
    size_t cbFile;
    char szPasswd[512] = { 0 };
    int vrc = VINF_SUCCESS;
    RTEXITCODE rcExit = RTEXITCODE_SUCCESS;
    bool fStdIn = !strcmp(pszFilename, "stdin");
    PRTSTREAM pStrm;
    if (!fStdIn)
        vrc = RTStrmOpen(pszFilename, "r", &pStrm);
    else
        pStrm = g_pStdIn;
    if (RT_SUCCESS(vrc))
    {
        vrc = RTStrmReadEx(pStrm, szPasswd, sizeof(szPasswd)-1, &cbFile);
        if (RT_SUCCESS(vrc))
        {
            size_t cbSize = RT_MIN(sizeof(szPasswd)-1, cbFile);
            unsigned i;
            for (i = 0; i < cbSize && !RTLocCIsCntrl(szPasswd[i]); i++)
                ;
            szPasswd[i] = '\0';
            /* If the line containing password doesn't fit into buffer */
            if (i >= sizeof(szPasswd)-1 && cbFile >= sizeof(szPasswd))
                rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Provided password in file '%s' is too long", pszFilename);
            else
                *pPasswd = szPasswd;
        }
        else
            rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Cannot read password from file '%s': %Rrc", pszFilename, vrc);
        if (!fStdIn)
            RTStrmClose(pStrm);
    }
    else
        rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Cannot open password file '%s' (%Rrc)", pszFilename, vrc);

    return rcExit;
}


/**
 * Sets password for settings from password file
 *
 * Only first line is used. The passwords length must be less than 512 bytes
 *
 * @param virtualBox   The IVirtualBox interface the settings password will be set for
 * @param pszFilename  The path to file containing the password
 * @return RTEXITCODE_SUCCESS or RTEXITCODE_FAILURE + msg.
 */
RTEXITCODE settingsPasswordFile(ComPtr<IVirtualBox> virtualBox, const char *pszFilename)
{
    com::Utf8Str passwd;
    RTEXITCODE rcExit = readPasswordFile(pszFilename, &passwd);
    if (rcExit == RTEXITCODE_SUCCESS)
    {
        int rc;
        CHECK_ERROR(virtualBox, SetSettingsSecret(com::Bstr(passwd).raw()));
        if (FAILED(rc))
            rcExit = RTEXITCODE_FAILURE;
    }

    return rcExit;
}


/**
 * Gets the password from the user input
 * *
 * @param pPassword  The string where password will be returned
 * @param pszPrompt  The prompt string for user
 * @return RTEXITCODE_SUCCESS or RTEXITCODE_FAILURE + msg.
 */
RTEXITCODE readPasswordFromConsole(com::Utf8Str *pPassword, const char *pszPrompt, ...)
{
    RTEXITCODE rcExit = RTEXITCODE_SUCCESS;
    char aszPwdInput[_1K] = { 0 };
    va_list vaArgs;

    va_start(vaArgs, pszPrompt);
    int vrc = RTStrmPrintfV(g_pStdOut, pszPrompt, vaArgs);
    if (RT_SUCCESS(vrc))
    {
        bool fEchoOld = false;
        vrc = RTStrmInputGetEchoChars(g_pStdIn, &fEchoOld);
        if (RT_SUCCESS(vrc))
        {
            vrc = RTStrmInputSetEchoChars(g_pStdIn, false);
            if (RT_SUCCESS(vrc))
            {
                vrc = RTStrmGetLine(g_pStdIn, &aszPwdInput[0], sizeof(aszPwdInput));
                if (RT_SUCCESS(vrc))
                    *pPassword = aszPwdInput;
                else
                    rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Failed read password from command line (%Rrc)", vrc);

                int vrc2 = RTStrmInputSetEchoChars(g_pStdIn, fEchoOld);
                AssertRC(vrc2);
            }
            else
                rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Failed to disable echoing typed characters (%Rrc)", vrc);
        }
        else
            rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Failed to retrieve echo setting (%Rrc)", vrc);

        RTStrmPutStr(g_pStdOut, "\n");
    }
    else
        rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Failed to print prompt (%Rrc)", vrc);
    va_end(vaArgs);

    return rcExit;
}

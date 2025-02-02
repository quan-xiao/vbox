/* $Id: svchlp.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * Declaration of SVC Helper Process control routines.
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

#ifndef MAIN_INCLUDED_SRC_src_server_win_svchlp_h
#define MAIN_INCLUDED_SRC_src_server_win_svchlp_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "VBox/com/string.h"
#include "VBox/com/guid.h"

#include <VBox/err.h>

#include <iprt/win/windows.h>

struct SVCHlpMsg
{
    enum Code
    {
        Null = 0, /* no parameters */
        OK, /* no parameters */
        Error, /* Utf8Str string (may be null but must present) */

        CreateHostOnlyNetworkInterface = 100, /* see usage in code */
        CreateHostOnlyNetworkInterface_OK, /* see usage in code */
        RemoveHostOnlyNetworkInterface, /* see usage in code */
        EnableDynamicIpConfig, /* see usage in code */
        EnableStaticIpConfig, /* see usage in code */
        EnableStaticIpConfigV6, /* see usage in code */
        DhcpRediscover, /* see usage in code */
    };
};

class SVCHlpClient
{
public:

    SVCHlpClient();
    virtual ~SVCHlpClient();

    int create (const char *aName);
    int connect();
    int open (const char *aName);
    int close();

    bool isOpen() const { return mIsOpen; }
    bool isServer() const { return mIsServer; }
    const com::Utf8Str &name() const { return mName; }

    int write (const void *aVal, size_t aLen);
    template <typename Scalar>
    int write (Scalar aVal) { return write (&aVal, sizeof (aVal)); }
    int write (const com::Utf8Str &aVal);
    int write (const com::Guid &aGuid);

    int read (void *aVal, size_t aLen);
    template <typename Scalar>
    int read (Scalar &aVal) { return read (&aVal, sizeof (aVal)); }
    int read (com::Utf8Str &aVal);
    int read (com::Guid &aGuid);

private:

    bool mIsOpen : 1;
    bool mIsServer : 1;

    HANDLE mReadEnd;
    HANDLE mWriteEnd;
    com::Utf8Str mName;
};

class SVCHlpServer : public SVCHlpClient
{
public:

    SVCHlpServer();

    int run();
};

#endif /* !MAIN_INCLUDED_SRC_src_server_win_svchlp_h */


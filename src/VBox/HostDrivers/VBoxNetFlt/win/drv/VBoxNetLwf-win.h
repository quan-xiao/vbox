/* $Id: VBoxNetLwf-win.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBoxNetLwf-win.h - Bridged Networking Driver, Windows-specific code.
 */
/*
 * Copyright (C) 2014-2020 Oracle Corporation
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

#ifndef VBOX_INCLUDED_SRC_VBoxNetFlt_win_drv_VBoxNetLwf_win_h
#define VBOX_INCLUDED_SRC_VBoxNetFlt_win_drv_VBoxNetLwf_win_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#define VBOXNETLWF_VERSION_NDIS_MAJOR        6
#define VBOXNETLWF_VERSION_NDIS_MINOR        0

#define VBOXNETLWF_NAME_FRIENDLY             L"VirtualBox NDIS Light-Weight Filter"
#define VBOXNETLWF_NAME_UNIQUE               L"{7af6b074-048d-4444-bfce-1ecc8bc5cb76}"
#define VBOXNETLWF_NAME_SERVICE              L"VBoxNetLwf"

#define VBOXNETLWF_NAME_LINK                 L"\\DosDevices\\Global\\VBoxNetLwf"
#define VBOXNETLWF_NAME_DEVICE               L"\\Device\\VBoxNetLwf"

#define VBOXNETLWF_MEM_TAG                   'FLBV'
#define VBOXNETLWF_REQ_ID                    'fLBV'

#endif /* !VBOX_INCLUDED_SRC_VBoxNetFlt_win_drv_VBoxNetLwf_win_h */

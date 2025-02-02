/* $Id: VBoxNetFltP-win.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBoxNetFltP-win.h - Bridged Networking Driver, Windows Specific Code.
 * Protocol edge API
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

#ifndef VBOX_INCLUDED_SRC_VBoxNetFlt_win_drv_VBoxNetFltP_win_h
#define VBOX_INCLUDED_SRC_VBoxNetFlt_win_drv_VBoxNetFltP_win_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#ifdef VBOXNETADP
# error "No protocol edge"
#endif
DECLHIDDEN(NDIS_STATUS) vboxNetFltWinPtRegister(PVBOXNETFLTGLOBALS_PT pGlobalsPt, PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPathStr);
DECLHIDDEN(NDIS_STATUS) vboxNetFltWinPtDeregister(PVBOXNETFLTGLOBALS_PT pGlobalsPt);
DECLHIDDEN(NDIS_STATUS) vboxNetFltWinPtDoUnbinding(PVBOXNETFLTINS pNetFlt, bool bOnUnbind);
DECLHIDDEN(VOID) vboxNetFltWinPtRequestComplete(NDIS_HANDLE hContext, PNDIS_REQUEST pNdisRequest, NDIS_STATUS Status);
DECLHIDDEN(bool) vboxNetFltWinPtCloseInterface(PVBOXNETFLTINS pNetFlt, PNDIS_STATUS pStatus);
DECLHIDDEN(NDIS_STATUS) vboxNetFltWinPtDoBinding(PVBOXNETFLTINS pThis, PNDIS_STRING pOurDeviceName, PNDIS_STRING pBindToDeviceName);
#endif /* !VBOX_INCLUDED_SRC_VBoxNetFlt_win_drv_VBoxNetFltP_win_h */

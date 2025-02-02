/* $Id: GaDrvEnvKMT.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VirtualBox Windows Guest Mesa3D - Gallium driver interface to the WDDM miniport driver.
 */

/*
 * Copyright (C) 2016-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef GA_INCLUDED_SRC_3D_win_VBoxGL_GaDrvEnvKMT_h
#define GA_INCLUDED_SRC_3D_win_VBoxGL_GaDrvEnvKMT_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBoxGaDriver.h>
#include <VBoxWddmUmHlp.h>

RT_C_DECLS_BEGIN

const WDDMGalliumDriverEnv *GaDrvEnvKmtCreate(void);
void GaDrvEnvKmtDelete(const WDDMGalliumDriverEnv *pEnv);

D3DKMT_HANDLE GaDrvEnvKmtContextHandle(const WDDMGalliumDriverEnv *pEnv,
                                       uint32_t u32Cid);
D3DKMT_HANDLE GaDrvEnvKmtSurfaceHandle(const WDDMGalliumDriverEnv *pEnv,
                                       uint32_t u32Sid);
void GaDrvEnvKmtAdapterLUID(const WDDMGalliumDriverEnv *pEnv,
                            LUID *pAdapterLuid);
D3DKMT_HANDLE GaDrvEnvKmtAdapterHandle(const WDDMGalliumDriverEnv *pEnv);
D3DKMT_HANDLE GaDrvEnvKmtDeviceHandle(const WDDMGalliumDriverEnv *pEnv);
void GaDrvEnvKmtRenderCompose(const WDDMGalliumDriverEnv *pEnv,
                              uint32_t u32Cid,
                              void *pvCommands,
                              uint32_t cbCommands,
                              ULONGLONG PresentHistoryToken);

RT_C_DECLS_END

#endif /* !GA_INCLUDED_SRC_3D_win_VBoxGL_GaDrvEnvKMT_h */

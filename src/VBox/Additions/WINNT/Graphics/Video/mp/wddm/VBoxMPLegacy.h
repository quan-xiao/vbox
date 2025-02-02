/* $Id: VBoxMPLegacy.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VirtualBox Windows Guest Mesa3D - Gallium driver interface for WDDM kernel mode driver.
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

#ifndef GA_INCLUDED_SRC_WINNT_Graphics_Video_mp_wddm_VBoxMPLegacy_h
#define GA_INCLUDED_SRC_WINNT_Graphics_Video_mp_wddm_VBoxMPLegacy_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "common/VBoxMPDevExt.h"

NTSTATUS APIENTRY DxgkDdiBuildPagingBufferLegacy(CONST HANDLE hAdapter,
                                                 DXGKARG_BUILDPAGINGBUFFER *pBuildPagingBuffer);
NTSTATUS APIENTRY DxgkDdiPresentLegacy(CONST HANDLE hContext,
                                       DXGKARG_PRESENT *pPresent);
NTSTATUS APIENTRY DxgkDdiRenderLegacy(CONST HANDLE hContext,
                                      DXGKARG_RENDER *pRender);
NTSTATUS APIENTRY DxgkDdiPatchLegacy(CONST HANDLE hAdapter,
                                     CONST DXGKARG_PATCH *pPatch);
NTSTATUS APIENTRY DxgkDdiSubmitCommandLegacy(CONST HANDLE hAdapter,
                                             CONST DXGKARG_SUBMITCOMMAND *pSubmitCommand);
NTSTATUS APIENTRY DxgkDdiPreemptCommandLegacy(CONST HANDLE hAdapter,
                                              CONST DXGKARG_PREEMPTCOMMAND *pPreemptCommand);
NTSTATUS APIENTRY DxgkDdiQueryCurrentFenceLegacy(CONST HANDLE hAdapter,
                                                 DXGKARG_QUERYCURRENTFENCE *pCurrentFence);
BOOLEAN DxgkDdiInterruptRoutineLegacy(CONST PVOID MiniportDeviceContext,
                                      ULONG MessageNumber);
VOID DxgkDdiDpcRoutineLegacy(CONST PVOID MiniportDeviceContext);

VOID vboxVdmaDdiNodesInit(PVBOXMP_DEVEXT pDevExt);

NTSTATUS vboxVdmaGgDmaBltPerform(PVBOXMP_DEVEXT pDevExt, struct VBOXWDDM_ALLOC_DATA *pSrcAlloc, RECT *pSrcRect,
                                 struct VBOXWDDM_ALLOC_DATA *pDstAlloc, RECT *pDstRect);

#endif /* !GA_INCLUDED_SRC_WINNT_Graphics_Video_mp_wddm_VBoxMPLegacy_h */

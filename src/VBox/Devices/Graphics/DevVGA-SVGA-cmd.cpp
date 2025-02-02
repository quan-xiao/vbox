/* $Id: DevVGA-SVGA-cmd.cpp 86479 2020-10-08 06:50:49Z vboxsync $ */
/** @file
 * VMware SVGA device - implementation of VMSVGA commands.
 */

/*
 * Copyright (C) 2013-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef IN_RING3
# error "DevVGA-SVGA-cmd.cpp is only for ring-3 code"
#endif


#define LOG_GROUP LOG_GROUP_DEV_VMSVGA
#include <iprt/mem.h>
#include <VBox/AssertGuest.h>
#include <VBox/log.h>
#include <VBox/vmm/pdmdev.h>
#include <VBoxVideo.h>

/* should go BEFORE any other DevVGA include to make all DevVGA.h config defines be visible */
#include "DevVGA.h"

/* Should be included after DevVGA.h/DevVGA-SVGA.h to pick all defines. */
#include "DevVGA-SVGA-internal.h"
#ifdef VBOX_WITH_VMSVGA3D
# include "DevVGA-SVGA3d.h"
#endif


#if defined(LOG_ENABLED) || defined(VBOX_STRICT)
# define SVGA_CASE_ID2STR(idx) case idx: return #idx
/**
 * FIFO command name lookup
 *
 * @returns FIFO command string or "UNKNOWN"
 * @param   u32Cmd      FIFO command
 */
const char *vmsvgaR3FifoCmdToString(uint32_t u32Cmd)
{
    switch (u32Cmd)
    {
        SVGA_CASE_ID2STR(SVGA_CMD_INVALID_CMD);
        SVGA_CASE_ID2STR(SVGA_CMD_UPDATE);
        SVGA_CASE_ID2STR(SVGA_CMD_RECT_FILL);
        SVGA_CASE_ID2STR(SVGA_CMD_RECT_COPY);
        SVGA_CASE_ID2STR(SVGA_CMD_RECT_ROP_COPY);
        SVGA_CASE_ID2STR(SVGA_CMD_DEFINE_CURSOR);
        SVGA_CASE_ID2STR(SVGA_CMD_DISPLAY_CURSOR);
        SVGA_CASE_ID2STR(SVGA_CMD_MOVE_CURSOR);
        SVGA_CASE_ID2STR(SVGA_CMD_DEFINE_ALPHA_CURSOR);
        SVGA_CASE_ID2STR(SVGA_CMD_UPDATE_VERBOSE);
        SVGA_CASE_ID2STR(SVGA_CMD_FRONT_ROP_FILL);
        SVGA_CASE_ID2STR(SVGA_CMD_FENCE);
        SVGA_CASE_ID2STR(SVGA_CMD_ESCAPE);
        SVGA_CASE_ID2STR(SVGA_CMD_DEFINE_SCREEN);
        SVGA_CASE_ID2STR(SVGA_CMD_DESTROY_SCREEN);
        SVGA_CASE_ID2STR(SVGA_CMD_DEFINE_GMRFB);
        SVGA_CASE_ID2STR(SVGA_CMD_BLIT_GMRFB_TO_SCREEN);
        SVGA_CASE_ID2STR(SVGA_CMD_BLIT_SCREEN_TO_GMRFB);
        SVGA_CASE_ID2STR(SVGA_CMD_ANNOTATION_FILL);
        SVGA_CASE_ID2STR(SVGA_CMD_ANNOTATION_COPY);
        SVGA_CASE_ID2STR(SVGA_CMD_DEFINE_GMR2);
        SVGA_CASE_ID2STR(SVGA_CMD_REMAP_GMR2);
        SVGA_CASE_ID2STR(SVGA_CMD_DEAD);
        SVGA_CASE_ID2STR(SVGA_CMD_DEAD_2);
        SVGA_CASE_ID2STR(SVGA_CMD_NOP);
        SVGA_CASE_ID2STR(SVGA_CMD_NOP_ERROR);
        SVGA_CASE_ID2STR(SVGA_CMD_MAX);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SURFACE_DEFINE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SURFACE_DESTROY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SURFACE_COPY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SURFACE_STRETCHBLT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SURFACE_DMA);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_CONTEXT_DEFINE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_CONTEXT_DESTROY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SETTRANSFORM);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SETZRANGE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SETRENDERSTATE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SETRENDERTARGET);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SETTEXTURESTATE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SETMATERIAL);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SETLIGHTDATA);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SETLIGHTENABLED);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SETVIEWPORT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SETCLIPPLANE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_CLEAR);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_PRESENT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SHADER_DEFINE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SHADER_DESTROY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SET_SHADER);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SET_SHADER_CONST);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DRAW_PRIMITIVES);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SETSCISSORRECT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_BEGIN_QUERY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_END_QUERY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_WAIT_FOR_QUERY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_PRESENT_READBACK);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_BLIT_SURFACE_TO_SCREEN);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SURFACE_DEFINE_V2);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_GENERATE_MIPMAPS);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_VIDEO_CREATE_DECODER);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_VIDEO_DESTROY_DECODER);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_VIDEO_CREATE_PROCESSOR);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_VIDEO_DESTROY_PROCESSOR);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_VIDEO_DECODE_START_FRAME);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_VIDEO_DECODE_RENDER);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_VIDEO_DECODE_END_FRAME);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_VIDEO_PROCESS_FRAME);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_ACTIVATE_SURFACE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DEACTIVATE_SURFACE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SCREEN_DMA);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DEAD1);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DEAD2);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_LOGICOPS_BITBLT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_LOGICOPS_TRANSBLT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_LOGICOPS_STRETCHBLT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_LOGICOPS_COLORFILL);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_LOGICOPS_ALPHABLEND);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_LOGICOPS_CLEARTYPEBLEND);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SET_OTABLE_BASE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_READBACK_OTABLE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DEFINE_GB_MOB);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DESTROY_GB_MOB);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DEAD3);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_UPDATE_GB_MOB_MAPPING);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DEFINE_GB_SURFACE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DESTROY_GB_SURFACE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_BIND_GB_SURFACE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_COND_BIND_GB_SURFACE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_UPDATE_GB_IMAGE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_UPDATE_GB_SURFACE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_READBACK_GB_IMAGE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_READBACK_GB_SURFACE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_INVALIDATE_GB_IMAGE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_INVALIDATE_GB_SURFACE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DEFINE_GB_CONTEXT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DESTROY_GB_CONTEXT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_BIND_GB_CONTEXT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_READBACK_GB_CONTEXT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_INVALIDATE_GB_CONTEXT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DEFINE_GB_SHADER);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DESTROY_GB_SHADER);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_BIND_GB_SHADER);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SET_OTABLE_BASE64);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_BEGIN_GB_QUERY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_END_GB_QUERY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_WAIT_FOR_GB_QUERY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_NOP);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_ENABLE_GART);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DISABLE_GART);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_MAP_MOB_INTO_GART);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_UNMAP_GART_RANGE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DEFINE_GB_SCREENTARGET);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DESTROY_GB_SCREENTARGET);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_BIND_GB_SCREENTARGET);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_UPDATE_GB_SCREENTARGET);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_READBACK_GB_IMAGE_PARTIAL);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_INVALIDATE_GB_IMAGE_PARTIAL);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SET_GB_SHADERCONSTS_INLINE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_GB_SCREEN_DMA);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_BIND_GB_SURFACE_WITH_PITCH);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_GB_MOB_FENCE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DEFINE_GB_SURFACE_V2);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DEFINE_GB_MOB64);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_REDEFINE_GB_MOB64);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_NOP_ERROR);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SET_VERTEX_STREAMS);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SET_VERTEX_DECLS);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_SET_VERTEX_DIVISORS);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DRAW);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DRAW_INDEXED);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DEFINE_CONTEXT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DESTROY_CONTEXT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_BIND_CONTEXT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_READBACK_CONTEXT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_INVALIDATE_CONTEXT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_SINGLE_CONSTANT_BUFFER);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_SHADER_RESOURCES);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_SHADER);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_SAMPLERS);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DRAW);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DRAW_INDEXED);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DRAW_INSTANCED);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DRAW_INDEXED_INSTANCED);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DRAW_AUTO);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_INPUT_LAYOUT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_VERTEX_BUFFERS);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_INDEX_BUFFER);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_TOPOLOGY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_RENDERTARGETS);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_BLEND_STATE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_DEPTHSTENCIL_STATE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_RASTERIZER_STATE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DEFINE_QUERY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DESTROY_QUERY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_BIND_QUERY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_QUERY_OFFSET);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_BEGIN_QUERY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_END_QUERY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_READBACK_QUERY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_PREDICATION);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_SOTARGETS);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_VIEWPORTS);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_SCISSORRECTS);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_CLEAR_RENDERTARGET_VIEW);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_CLEAR_DEPTHSTENCIL_VIEW);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_PRED_COPY_REGION);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_PRED_COPY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_STRETCHBLT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_GENMIPS);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_UPDATE_SUBRESOURCE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_READBACK_SUBRESOURCE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_INVALIDATE_SUBRESOURCE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DEFINE_SHADERRESOURCE_VIEW);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DESTROY_SHADERRESOURCE_VIEW);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DEFINE_RENDERTARGET_VIEW);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DESTROY_RENDERTARGET_VIEW);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DEFINE_DEPTHSTENCIL_VIEW);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DESTROY_DEPTHSTENCIL_VIEW);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DEFINE_ELEMENTLAYOUT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DESTROY_ELEMENTLAYOUT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DEFINE_BLEND_STATE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DESTROY_BLEND_STATE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DEFINE_DEPTHSTENCIL_STATE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DESTROY_DEPTHSTENCIL_STATE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DEFINE_RASTERIZER_STATE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DESTROY_RASTERIZER_STATE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DEFINE_SAMPLER_STATE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DESTROY_SAMPLER_STATE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DEFINE_SHADER);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DESTROY_SHADER);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_BIND_SHADER);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DEFINE_STREAMOUTPUT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_DESTROY_STREAMOUTPUT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_STREAMOUTPUT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_COTABLE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_READBACK_COTABLE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_BUFFER_COPY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_TRANSFER_FROM_BUFFER);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SURFACE_COPY_AND_READBACK);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_MOVE_QUERY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_BIND_ALL_QUERY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_READBACK_ALL_QUERY);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_PRED_TRANSFER_FROM_BUFFER);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_MOB_FENCE_64);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_BIND_ALL_SHADER);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_HINT);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_BUFFER_UPDATE);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_VS_CONSTANT_BUFFER_OFFSET);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_PS_CONSTANT_BUFFER_OFFSET);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_SET_GS_CONSTANT_BUFFER_OFFSET);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_RESERVED1);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_RESERVED2);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_RESERVED3);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_COND_BIND_ALL_SHADER);
        SVGA_CASE_ID2STR(SVGA_3D_CMD_DX_MAX);
        default: return "UNKNOWN";
    }
}
# undef SVGA_CASE_ID2STR
#endif /* LOG_ENABLED || VBOX_STRICT */


VMSVGASCREENOBJECT *vmsvgaR3GetScreenObject(PVGASTATECC pThisCC, uint32_t idScreen)
{
    PVMSVGAR3STATE pSVGAState = pThisCC->svga.pSvgaR3State;
    if (   idScreen < (uint32_t)RT_ELEMENTS(pSVGAState->aScreens)
        && pSVGAState
        && pSVGAState->aScreens[idScreen].fDefined)
    {
        return &pSVGAState->aScreens[idScreen];
    }
    return NULL;
}

void vmsvgaR3ResetScreens(PVGASTATE pThis, PVGASTATECC pThisCC)
{
#ifdef VBOX_WITH_VMSVGA3D
    if (pThis->svga.f3DEnabled)
    {
        for (uint32_t idScreen = 0; idScreen < (uint32_t)RT_ELEMENTS(pThisCC->svga.pSvgaR3State->aScreens); ++idScreen)
        {
            VMSVGASCREENOBJECT *pScreen = vmsvgaR3GetScreenObject(pThisCC, idScreen);
            if (pScreen)
                vmsvga3dDestroyScreen(pThisCC, pScreen);
        }
    }
#else
    RT_NOREF(pThis, pThisCC);
#endif
}


/**
 * Copy a rectangle of pixels within guest VRAM.
 */
static void vmsvgaR3RectCopy(PVGASTATECC pThisCC, VMSVGASCREENOBJECT const *pScreen, uint32_t srcX, uint32_t srcY,
                             uint32_t dstX, uint32_t dstY, uint32_t width, uint32_t height, unsigned cbFrameBuffer)
{
    if (!width || !height)
        return; /* Nothing to do, don't even bother. */

    /*
     * The guest VRAM (aka GFB) is considered to be a bitmap in the format
     * corresponding to the current display mode.
     */
    uint32_t const  cbPixel = RT_ALIGN(pScreen->cBpp, 8) / 8;
    uint32_t const  cbScanline = pScreen->cbPitch ? pScreen->cbPitch : width * cbPixel;
    uint8_t const   *pSrc;
    uint8_t         *pDst;
    unsigned const  cbRectWidth = width * cbPixel;
    unsigned        uMaxOffset;

    uMaxOffset = (RT_MAX(srcY, dstY) + height) * cbScanline + (RT_MAX(srcX, dstX) + width) * cbPixel;
    if (uMaxOffset >= cbFrameBuffer)
    {
        Log(("Max offset (%u) too big for framebuffer (%u bytes), ignoring!\n", uMaxOffset, cbFrameBuffer));
        return; /* Just don't listen to a bad guest. */
    }

    pSrc = pDst = pThisCC->pbVRam;
    pSrc += srcY * cbScanline + srcX * cbPixel;
    pDst += dstY * cbScanline + dstX * cbPixel;

    if (srcY >= dstY)
    {
        /* Source below destination, copy top to bottom. */
        for (; height > 0; height--)
        {
            memmove(pDst, pSrc, cbRectWidth);
            pSrc += cbScanline;
            pDst += cbScanline;
        }
    }
    else
    {
        /* Source above destination, copy bottom to top. */
        pSrc += cbScanline * (height - 1);
        pDst += cbScanline * (height - 1);
        for (; height > 0; height--)
        {
            memmove(pDst, pSrc, cbRectWidth);
            pSrc -= cbScanline;
            pDst -= cbScanline;
        }
    }
}


/**
 * Common worker for changing the pointer shape.
 *
 * @param   pThisCC             The VGA/VMSVGA state for ring-3.
 * @param   pSVGAState          The VMSVGA ring-3 instance data.
 * @param   fAlpha              Whether there is alpha or not.
 * @param   xHot                Hotspot x coordinate.
 * @param   yHot                Hotspot y coordinate.
 * @param   cx                  Width.
 * @param   cy                  Height.
 * @param   pbData              Heap copy of the cursor data.  Consumed.
 * @param   cbData              The size of the data.
 */
static void vmsvgaR3InstallNewCursor(PVGASTATECC pThisCC, PVMSVGAR3STATE pSVGAState, bool fAlpha,
                                     uint32_t xHot, uint32_t yHot, uint32_t cx, uint32_t cy, uint8_t *pbData, uint32_t cbData)
{
    LogRel2(("vmsvgaR3InstallNewCursor: cx=%d cy=%d xHot=%d yHot=%d fAlpha=%d cbData=%#x\n", cx, cy, xHot, yHot, fAlpha, cbData));
#ifdef LOG_ENABLED
    if (LogIs2Enabled())
    {
        uint32_t cbAndLine = RT_ALIGN(cx, 8) / 8;
        if (!fAlpha)
        {
            Log2(("VMSVGA Cursor AND mask (%d,%d):\n", cx, cy));
            for (uint32_t y = 0; y < cy; y++)
            {
                Log2(("%3u:", y));
                uint8_t const *pbLine = &pbData[y * cbAndLine];
                for (uint32_t x = 0; x < cx; x += 8)
                {
                    uint8_t   b = pbLine[x / 8];
                    char      szByte[12];
                    szByte[0] = b & 0x80 ? '*' : ' '; /* most significant bit first */
                    szByte[1] = b & 0x40 ? '*' : ' ';
                    szByte[2] = b & 0x20 ? '*' : ' ';
                    szByte[3] = b & 0x10 ? '*' : ' ';
                    szByte[4] = b & 0x08 ? '*' : ' ';
                    szByte[5] = b & 0x04 ? '*' : ' ';
                    szByte[6] = b & 0x02 ? '*' : ' ';
                    szByte[7] = b & 0x01 ? '*' : ' ';
                    szByte[8] = '\0';
                    Log2(("%s", szByte));
                }
                Log2(("\n"));
            }
        }

        Log2(("VMSVGA Cursor XOR mask (%d,%d):\n", cx, cy));
        uint32_t const *pu32Xor = (uint32_t const *)&pbData[RT_ALIGN_32(cbAndLine * cy, 4)];
        for (uint32_t y = 0; y < cy; y++)
        {
            Log2(("%3u:", y));
            uint32_t const *pu32Line = &pu32Xor[y * cx];
            for (uint32_t x = 0; x < cx; x++)
                Log2((" %08x", pu32Line[x]));
            Log2(("\n"));
        }
    }
#endif

    int rc = pThisCC->pDrv->pfnVBVAMousePointerShape(pThisCC->pDrv, true /*fVisible*/, fAlpha, xHot, yHot, cx, cy, pbData);
    AssertRC(rc);

    if (pSVGAState->Cursor.fActive)
        RTMemFreeZ(pSVGAState->Cursor.pData, pSVGAState->Cursor.cbData);

    pSVGAState->Cursor.fActive  = true;
    pSVGAState->Cursor.xHotspot = xHot;
    pSVGAState->Cursor.yHotspot = yHot;
    pSVGAState->Cursor.width    = cx;
    pSVGAState->Cursor.height   = cy;
    pSVGAState->Cursor.cbData   = cbData;
    pSVGAState->Cursor.pData    = pbData;
}


#ifdef VBOX_WITH_VMSVGA3D
/** @def VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK
 * Check that the 3D command has at least a_cbMin of payload bytes after the
 * header.  Will break out of the switch if it doesn't.
 */
# define VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(a_cbMin) \
     if (1) { \
          AssertMsgBreak(cbCmd >= (a_cbMin), ("size=%#x a_cbMin=%#zx\n", cbCmd, (size_t)(a_cbMin))); \
          RT_UNTRUSTED_VALIDATED_FENCE(); \
     } else do {} while (0)

# define VMSVGA_3D_CMD_NOTIMPL() \
     if (1) { \
          AssertMsgFailed(("Not implemented %d %s\n", enmCmdId, vmsvgaR3FifoCmdToString(enmCmdId))); \
     } else do {} while (0)

/** SVGA_3D_CMD_* handler.
 * This function parses the command and calls the corresponding command handler.
 *
 * @param   pThis       The shared VGA/VMSVGA state.
 * @param   pThisCC     The VGA/VMSVGA state for the current context.
 * @param   enmCmdId    SVGA_3D_CMD_* command identifier.
 * @param   cbCmd       Size of the command in bytes.
 * @param   pvCmd       Pointer to the command.
 * @returns VBox status code if an error was detected parsing a command.
 */
int vmsvgaR3Process3dCmd(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAFifo3dCmdId enmCmdId, uint32_t cbCmd, void const *pvCmd)
{
    int rcParse = VINF_SUCCESS;
    PVMSVGAR3STATE pSvgaR3State = pThisCC->svga.pSvgaR3State;

    switch (enmCmdId)
    {
    case SVGA_3D_CMD_SURFACE_DEFINE:
    {
        SVGA3dCmdDefineSurface *pCmd = (SVGA3dCmdDefineSurface *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dSurfaceDefine);

        uint32_t const cMipLevels = (cbCmd - sizeof(*pCmd)) / sizeof(SVGA3dSize);
        vmsvga3dSurfaceDefine(pThisCC, pCmd->sid, (uint32_t)pCmd->surfaceFlags, pCmd->format, pCmd->face, 0,
                              SVGA3D_TEX_FILTER_NONE, cMipLevels, (SVGA3dSize *)(pCmd + 1));
# ifdef DEBUG_GMR_ACCESS
        VMR3ReqCallWaitU(PDMDevHlpGetUVM(pDevIns), VMCPUID_ANY, (PFNRT)vmsvgaR3ResetGmrHandlers, 1, pThis);
# endif
        break;
    }

    case SVGA_3D_CMD_SURFACE_DEFINE_V2:
    {
        SVGA3dCmdDefineSurface_v2 *pCmd = (SVGA3dCmdDefineSurface_v2 *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dSurfaceDefineV2);

        uint32_t const cMipLevels = (cbCmd - sizeof(*pCmd)) / sizeof(SVGA3dSize);
        vmsvga3dSurfaceDefine(pThisCC, pCmd->sid, pCmd->surfaceFlags, pCmd->format, pCmd->face,
                              pCmd->multisampleCount, pCmd->autogenFilter,
                              cMipLevels, (SVGA3dSize *)(pCmd + 1));
        break;
    }

    case SVGA_3D_CMD_SURFACE_DESTROY:
    {
        SVGA3dCmdDestroySurface *pCmd = (SVGA3dCmdDestroySurface *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dSurfaceDestroy);

        vmsvga3dSurfaceDestroy(pThisCC, pCmd->sid);
        break;
    }

    case SVGA_3D_CMD_SURFACE_COPY:
    {
        SVGA3dCmdSurfaceCopy *pCmd = (SVGA3dCmdSurfaceCopy *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dSurfaceCopy);

        uint32_t const cCopyBoxes = (cbCmd - sizeof(pCmd)) / sizeof(SVGA3dCopyBox);
        vmsvga3dSurfaceCopy(pThisCC, pCmd->dest, pCmd->src, cCopyBoxes, (SVGA3dCopyBox *)(pCmd + 1));
        break;
    }

    case SVGA_3D_CMD_SURFACE_STRETCHBLT:
    {
        SVGA3dCmdSurfaceStretchBlt *pCmd = (SVGA3dCmdSurfaceStretchBlt *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dSurfaceStretchBlt);

        vmsvga3dSurfaceStretchBlt(pThis, pThisCC, &pCmd->dest, &pCmd->boxDest,
                                  &pCmd->src, &pCmd->boxSrc, pCmd->mode);
        break;
    }

    case SVGA_3D_CMD_SURFACE_DMA:
    {
        SVGA3dCmdSurfaceDMA *pCmd = (SVGA3dCmdSurfaceDMA *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dSurfaceDma);

        uint64_t u64NanoTS = 0;
        if (LogRelIs3Enabled())
            u64NanoTS = RTTimeNanoTS();
        uint32_t const cCopyBoxes = (cbCmd - sizeof(*pCmd)) / sizeof(SVGA3dCopyBox);
        STAM_PROFILE_START(&pSvgaR3State->StatR3Cmd3dSurfaceDmaProf, a);
        vmsvga3dSurfaceDMA(pThis, pThisCC, pCmd->guest, pCmd->host, pCmd->transfer,
                           cCopyBoxes, (SVGA3dCopyBox *)(pCmd + 1));
        STAM_PROFILE_STOP(&pSvgaR3State->StatR3Cmd3dSurfaceDmaProf, a);
        if (LogRelIs3Enabled())
        {
            if (cCopyBoxes)
            {
                SVGA3dCopyBox *pFirstBox = (SVGA3dCopyBox *)(pCmd + 1);
                LogRel3(("VMSVGA: SURFACE_DMA: %d us %d boxes %d,%d %dx%d%s\n",
                         (RTTimeNanoTS() - u64NanoTS) / 1000ULL, cCopyBoxes,
                         pFirstBox->x, pFirstBox->y, pFirstBox->w, pFirstBox->h,
                         pCmd->transfer == SVGA3D_READ_HOST_VRAM ? " readback!!!" : ""));
            }
        }
        break;
    }

    case SVGA_3D_CMD_BLIT_SURFACE_TO_SCREEN:
    {
        SVGA3dCmdBlitSurfaceToScreen *pCmd = (SVGA3dCmdBlitSurfaceToScreen *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dSurfaceScreen);

        static uint64_t u64FrameStartNanoTS = 0;
        static uint64_t u64ElapsedPerSecNano = 0;
        static int cFrames = 0;
        uint64_t u64NanoTS = 0;
        if (LogRelIs3Enabled())
            u64NanoTS = RTTimeNanoTS();
        uint32_t const cRects = (cbCmd - sizeof(*pCmd)) / sizeof(SVGASignedRect);
        STAM_REL_PROFILE_START(&pSvgaR3State->StatR3Cmd3dBlitSurfaceToScreenProf, a);
        vmsvga3dSurfaceBlitToScreen(pThis, pThisCC, pCmd->destScreenId, pCmd->destRect, pCmd->srcImage,
                                    pCmd->srcRect, cRects, (SVGASignedRect *)(pCmd + 1));
        STAM_REL_PROFILE_STOP(&pSvgaR3State->StatR3Cmd3dBlitSurfaceToScreenProf, a);
        if (LogRelIs3Enabled())
        {
            uint64_t u64ElapsedNano = RTTimeNanoTS() - u64NanoTS;
            u64ElapsedPerSecNano += u64ElapsedNano;

            SVGASignedRect *pFirstRect = cRects ? (SVGASignedRect *)(pCmd + 1) : &pCmd->destRect;
            LogRel3(("VMSVGA: SURFACE_TO_SCREEN: %d us %d rects %d,%d %dx%d\n",
                     (u64ElapsedNano) / 1000ULL, cRects,
                     pFirstRect->left, pFirstRect->top,
                     pFirstRect->right - pFirstRect->left, pFirstRect->bottom - pFirstRect->top));

            ++cFrames;
            if (u64NanoTS - u64FrameStartNanoTS >= UINT64_C(1000000000))
            {
                LogRel3(("VMSVGA: SURFACE_TO_SCREEN: FPS %d, elapsed %llu us\n",
                         cFrames, u64ElapsedPerSecNano / 1000ULL));
                u64FrameStartNanoTS = u64NanoTS;
                cFrames = 0;
                u64ElapsedPerSecNano = 0;
            }
        }
        break;
    }

    case SVGA_3D_CMD_CONTEXT_DEFINE:
    {
        SVGA3dCmdDefineContext *pCmd = (SVGA3dCmdDefineContext *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dContextDefine);

        vmsvga3dContextDefine(pThisCC, pCmd->cid);
        break;
    }

    case SVGA_3D_CMD_CONTEXT_DESTROY:
    {
        SVGA3dCmdDestroyContext *pCmd = (SVGA3dCmdDestroyContext *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dContextDestroy);

        vmsvga3dContextDestroy(pThisCC, pCmd->cid);
        break;
    }

    case SVGA_3D_CMD_SETTRANSFORM:
    {
        SVGA3dCmdSetTransform *pCmd = (SVGA3dCmdSetTransform *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dSetTransform);

        vmsvga3dSetTransform(pThisCC, pCmd->cid, pCmd->type, pCmd->matrix);
        break;
    }

    case SVGA_3D_CMD_SETZRANGE:
    {
        SVGA3dCmdSetZRange *pCmd = (SVGA3dCmdSetZRange *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dSetZRange);

        vmsvga3dSetZRange(pThisCC, pCmd->cid, pCmd->zRange);
        break;
    }

    case SVGA_3D_CMD_SETRENDERSTATE:
    {
        SVGA3dCmdSetRenderState *pCmd = (SVGA3dCmdSetRenderState *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dSetRenderState);

        uint32_t const cRenderStates = (cbCmd - sizeof(*pCmd)) / sizeof(SVGA3dRenderState);
        vmsvga3dSetRenderState(pThisCC, pCmd->cid, cRenderStates, (SVGA3dRenderState *)(pCmd + 1));
        break;
    }

    case SVGA_3D_CMD_SETRENDERTARGET:
    {
        SVGA3dCmdSetRenderTarget *pCmd = (SVGA3dCmdSetRenderTarget *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dSetRenderTarget);

        vmsvga3dSetRenderTarget(pThisCC, pCmd->cid, pCmd->type, pCmd->target);
        break;
    }

    case SVGA_3D_CMD_SETTEXTURESTATE:
    {
        SVGA3dCmdSetTextureState *pCmd = (SVGA3dCmdSetTextureState *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dSetTextureState);

        uint32_t const cTextureStates = (cbCmd - sizeof(*pCmd)) / sizeof(SVGA3dTextureState);
        vmsvga3dSetTextureState(pThisCC, pCmd->cid, cTextureStates, (SVGA3dTextureState *)(pCmd + 1));
        break;
    }

    case SVGA_3D_CMD_SETMATERIAL:
    {
        SVGA3dCmdSetMaterial *pCmd = (SVGA3dCmdSetMaterial *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dSetMaterial);

        vmsvga3dSetMaterial(pThisCC, pCmd->cid, pCmd->face, &pCmd->material);
        break;
    }

    case SVGA_3D_CMD_SETLIGHTDATA:
    {
        SVGA3dCmdSetLightData *pCmd = (SVGA3dCmdSetLightData *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dSetLightData);

        vmsvga3dSetLightData(pThisCC, pCmd->cid, pCmd->index, &pCmd->data);
        break;
    }

    case SVGA_3D_CMD_SETLIGHTENABLED:
    {
        SVGA3dCmdSetLightEnabled *pCmd = (SVGA3dCmdSetLightEnabled *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dSetLightEnable);

        vmsvga3dSetLightEnabled(pThisCC, pCmd->cid, pCmd->index, pCmd->enabled);
        break;
    }

    case SVGA_3D_CMD_SETVIEWPORT:
    {
        SVGA3dCmdSetViewport *pCmd = (SVGA3dCmdSetViewport *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dSetViewPort);

        vmsvga3dSetViewPort(pThisCC, pCmd->cid, &pCmd->rect);
        break;
    }

    case SVGA_3D_CMD_SETCLIPPLANE:
    {
        SVGA3dCmdSetClipPlane *pCmd = (SVGA3dCmdSetClipPlane *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dSetClipPlane);

        vmsvga3dSetClipPlane(pThisCC, pCmd->cid, pCmd->index, pCmd->plane);
        break;
    }

    case SVGA_3D_CMD_CLEAR:
    {
        SVGA3dCmdClear  *pCmd = (SVGA3dCmdClear *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dClear);

        uint32_t const cRects = (cbCmd - sizeof(*pCmd)) / sizeof(SVGA3dRect);
        vmsvga3dCommandClear(pThisCC, pCmd->cid, pCmd->clearFlag, pCmd->color, pCmd->depth, pCmd->stencil, cRects, (SVGA3dRect *)(pCmd + 1));
        break;
    }

    case SVGA_3D_CMD_PRESENT:
    case SVGA_3D_CMD_PRESENT_READBACK: /** @todo SVGA_3D_CMD_PRESENT_READBACK isn't quite the same as present... */
    {
        SVGA3dCmdPresent *pCmd = (SVGA3dCmdPresent *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        if (enmCmdId == SVGA_3D_CMD_PRESENT)
            STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dPresent);
        else
            STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dPresentReadBack);

        uint32_t const cRects = (cbCmd - sizeof(*pCmd)) / sizeof(SVGA3dCopyRect);
        STAM_PROFILE_START(&pSvgaR3State->StatR3Cmd3dPresentProf, a);
        vmsvga3dCommandPresent(pThis, pThisCC, pCmd->sid, cRects, (SVGA3dCopyRect *)(pCmd + 1));
        STAM_PROFILE_STOP(&pSvgaR3State->StatR3Cmd3dPresentProf, a);
        break;
    }

    case SVGA_3D_CMD_SHADER_DEFINE:
    {
        SVGA3dCmdDefineShader *pCmd = (SVGA3dCmdDefineShader *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dShaderDefine);

        uint32_t const cbData = (cbCmd - sizeof(*pCmd));
        vmsvga3dShaderDefine(pThisCC, pCmd->cid, pCmd->shid, pCmd->type, cbData, (uint32_t *)(pCmd + 1));
        break;
    }

    case SVGA_3D_CMD_SHADER_DESTROY:
    {
        SVGA3dCmdDestroyShader *pCmd = (SVGA3dCmdDestroyShader *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dShaderDestroy);

        vmsvga3dShaderDestroy(pThisCC, pCmd->cid, pCmd->shid, pCmd->type);
        break;
    }

    case SVGA_3D_CMD_SET_SHADER:
    {
        SVGA3dCmdSetShader *pCmd = (SVGA3dCmdSetShader *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dSetShader);

        vmsvga3dShaderSet(pThisCC, NULL, pCmd->cid, pCmd->type, pCmd->shid);
        break;
    }

    case SVGA_3D_CMD_SET_SHADER_CONST:
    {
        SVGA3dCmdSetShaderConst *pCmd = (SVGA3dCmdSetShaderConst *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dSetShaderConst);

        uint32_t const cRegisters = (cbCmd - sizeof(*pCmd)) / sizeof(pCmd->values) + 1;
        vmsvga3dShaderSetConst(pThisCC, pCmd->cid, pCmd->reg, pCmd->type, pCmd->ctype, cRegisters, pCmd->values);
        break;
    }

    case SVGA_3D_CMD_DRAW_PRIMITIVES:
    {
        SVGA3dCmdDrawPrimitives *pCmd = (SVGA3dCmdDrawPrimitives *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dDrawPrimitives);

        ASSERT_GUEST_STMT_BREAK(pCmd->numRanges <= SVGA3D_MAX_DRAW_PRIMITIVE_RANGES, rcParse = VERR_INVALID_PARAMETER);
        ASSERT_GUEST_STMT_BREAK(pCmd->numVertexDecls <= SVGA3D_MAX_VERTEX_ARRAYS, rcParse = VERR_INVALID_PARAMETER);
        uint32_t const cbRangesAndVertexDecls = pCmd->numVertexDecls * sizeof(SVGA3dVertexDecl)
                                              + pCmd->numRanges * sizeof(SVGA3dPrimitiveRange);
        ASSERT_GUEST_STMT_BREAK(cbRangesAndVertexDecls <= cbCmd - sizeof(*pCmd), rcParse = VERR_INVALID_PARAMETER);

        uint32_t const cVertexDivisor = (cbCmd - sizeof(*pCmd) - cbRangesAndVertexDecls) / sizeof(uint32_t);
        ASSERT_GUEST_STMT_BREAK(!cVertexDivisor || cVertexDivisor == pCmd->numVertexDecls, rcParse = VERR_INVALID_PARAMETER);
        RT_UNTRUSTED_VALIDATED_FENCE();

        SVGA3dVertexDecl     *pVertexDecl    = (SVGA3dVertexDecl *)(pCmd + 1);
        SVGA3dPrimitiveRange *pNumRange      = (SVGA3dPrimitiveRange *)&pVertexDecl[pCmd->numVertexDecls];
        SVGA3dVertexDivisor  *pVertexDivisor = cVertexDivisor ? (SVGA3dVertexDivisor *)&pNumRange[pCmd->numRanges] : NULL;

        STAM_PROFILE_START(&pSvgaR3State->StatR3Cmd3dDrawPrimitivesProf, a);
        vmsvga3dDrawPrimitives(pThisCC, pCmd->cid, pCmd->numVertexDecls, pVertexDecl, pCmd->numRanges,
                               pNumRange, cVertexDivisor, pVertexDivisor);
        STAM_PROFILE_STOP(&pSvgaR3State->StatR3Cmd3dDrawPrimitivesProf, a);
        break;
    }

    case SVGA_3D_CMD_SETSCISSORRECT:
    {
        SVGA3dCmdSetScissorRect *pCmd = (SVGA3dCmdSetScissorRect *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dSetScissorRect);

        vmsvga3dSetScissorRect(pThisCC, pCmd->cid, &pCmd->rect);
        break;
    }

    case SVGA_3D_CMD_BEGIN_QUERY:
    {
        SVGA3dCmdBeginQuery *pCmd = (SVGA3dCmdBeginQuery *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dBeginQuery);

        vmsvga3dQueryBegin(pThisCC, pCmd->cid, pCmd->type);
        break;
    }

    case SVGA_3D_CMD_END_QUERY:
    {
        SVGA3dCmdEndQuery *pCmd = (SVGA3dCmdEndQuery *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dEndQuery);

        vmsvga3dQueryEnd(pThisCC, pCmd->cid, pCmd->type, pCmd->guestResult);
        break;
    }

    case SVGA_3D_CMD_WAIT_FOR_QUERY:
    {
        SVGA3dCmdWaitForQuery *pCmd = (SVGA3dCmdWaitForQuery *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dWaitForQuery);

        vmsvga3dQueryWait(pThis, pThisCC, pCmd->cid, pCmd->type, pCmd->guestResult);
        break;
    }

    case SVGA_3D_CMD_GENERATE_MIPMAPS:
    {
        SVGA3dCmdGenerateMipmaps *pCmd = (SVGA3dCmdGenerateMipmaps *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dGenerateMipmaps);

        vmsvga3dGenerateMipmaps(pThisCC, pCmd->sid, pCmd->filter);
        break;
    }

    case SVGA_3D_CMD_ACTIVATE_SURFACE:
        /* context id + surface id? */
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dActivateSurface);
        break;

    case SVGA_3D_CMD_DEACTIVATE_SURFACE:
        /* context id + surface id? */
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3Cmd3dDeactivateSurface);
        break;

    /*
     *
     * VPGU10: SVGA_CAP_GBOBJECTS+ commands.
     *
     */
    case SVGA_3D_CMD_SCREEN_DMA:
    {
        SVGA3dCmdScreenDMA *pCmd = (SVGA3dCmdScreenDMA *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_DEAD1:
    case SVGA_3D_CMD_DEAD2:
    {
        VMSVGA_3D_CMD_NOTIMPL();
        break;
    }

    case SVGA_3D_CMD_LOGICOPS_BITBLT:
    {
        SVGA3dCmdLogicOpsBitBlt *pCmd = (SVGA3dCmdLogicOpsBitBlt *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_LOGICOPS_TRANSBLT:
    {
        SVGA3dCmdLogicOpsTransBlt *pCmd = (SVGA3dCmdLogicOpsTransBlt *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_LOGICOPS_STRETCHBLT:
    {
        SVGA3dCmdLogicOpsStretchBlt *pCmd = (SVGA3dCmdLogicOpsStretchBlt *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_LOGICOPS_COLORFILL:
    {
        SVGA3dCmdLogicOpsColorFill *pCmd = (SVGA3dCmdLogicOpsColorFill *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_LOGICOPS_ALPHABLEND:
    {
        SVGA3dCmdLogicOpsAlphaBlend *pCmd = (SVGA3dCmdLogicOpsAlphaBlend *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_LOGICOPS_CLEARTYPEBLEND:
    {
        SVGA3dCmdLogicOpsClearTypeBlend *pCmd = (SVGA3dCmdLogicOpsClearTypeBlend *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_SET_OTABLE_BASE:
    {
        SVGA3dCmdSetOTableBase *pCmd = (SVGA3dCmdSetOTableBase *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_READBACK_OTABLE:
    {
        SVGA3dCmdReadbackOTable *pCmd = (SVGA3dCmdReadbackOTable *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_DEFINE_GB_MOB:
    {
        SVGA3dCmdDefineGBMob *pCmd = (SVGA3dCmdDefineGBMob *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_DESTROY_GB_MOB:
    {
        SVGA3dCmdDestroyGBMob *pCmd = (SVGA3dCmdDestroyGBMob *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_DEAD3:
    {
        VMSVGA_3D_CMD_NOTIMPL();
        break;
    }

    case SVGA_3D_CMD_UPDATE_GB_MOB_MAPPING:
    {
        SVGA3dCmdUpdateGBMobMapping *pCmd = (SVGA3dCmdUpdateGBMobMapping *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_DEFINE_GB_SURFACE:
    {
        SVGA3dCmdDefineGBSurface *pCmd = (SVGA3dCmdDefineGBSurface *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_DESTROY_GB_SURFACE:
    {
        SVGA3dCmdDestroyGBSurface *pCmd = (SVGA3dCmdDestroyGBSurface *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_BIND_GB_SURFACE:
    {
        SVGA3dCmdBindGBSurface *pCmd = (SVGA3dCmdBindGBSurface *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_COND_BIND_GB_SURFACE:
    {
        SVGA3dCmdCondBindGBSurface *pCmd = (SVGA3dCmdCondBindGBSurface *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_UPDATE_GB_IMAGE:
    {
        SVGA3dCmdUpdateGBImage *pCmd = (SVGA3dCmdUpdateGBImage *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_UPDATE_GB_SURFACE:
    {
        SVGA3dCmdUpdateGBSurface *pCmd = (SVGA3dCmdUpdateGBSurface *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_READBACK_GB_IMAGE:
    {
        SVGA3dCmdReadbackGBImage *pCmd = (SVGA3dCmdReadbackGBImage *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_READBACK_GB_SURFACE:
    {
        SVGA3dCmdReadbackGBSurface *pCmd = (SVGA3dCmdReadbackGBSurface *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_INVALIDATE_GB_IMAGE:
    {
        SVGA3dCmdInvalidateGBImage *pCmd = (SVGA3dCmdInvalidateGBImage *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_INVALIDATE_GB_SURFACE:
    {
        SVGA3dCmdInvalidateGBSurface *pCmd = (SVGA3dCmdInvalidateGBSurface *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_DEFINE_GB_CONTEXT:
    {
        SVGA3dCmdDefineGBContext *pCmd = (SVGA3dCmdDefineGBContext *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_DESTROY_GB_CONTEXT:
    {
        SVGA3dCmdDestroyGBContext *pCmd = (SVGA3dCmdDestroyGBContext *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_BIND_GB_CONTEXT:
    {
        SVGA3dCmdBindGBContext *pCmd = (SVGA3dCmdBindGBContext *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_READBACK_GB_CONTEXT:
    {
        SVGA3dCmdReadbackGBContext *pCmd = (SVGA3dCmdReadbackGBContext *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_INVALIDATE_GB_CONTEXT:
    {
        SVGA3dCmdInvalidateGBContext *pCmd = (SVGA3dCmdInvalidateGBContext *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_DEFINE_GB_SHADER:
    {
        SVGA3dCmdDefineGBShader *pCmd = (SVGA3dCmdDefineGBShader *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_DESTROY_GB_SHADER:
    {
        SVGA3dCmdDestroyGBShader *pCmd = (SVGA3dCmdDestroyGBShader *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_BIND_GB_SHADER:
    {
        SVGA3dCmdBindGBShader *pCmd = (SVGA3dCmdBindGBShader *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_SET_OTABLE_BASE64:
    {
        SVGA3dCmdSetOTableBase64 *pCmd = (SVGA3dCmdSetOTableBase64 *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_BEGIN_GB_QUERY:
    {
        SVGA3dCmdBeginGBQuery *pCmd = (SVGA3dCmdBeginGBQuery *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_END_GB_QUERY:
    {
        SVGA3dCmdEndGBQuery *pCmd = (SVGA3dCmdEndGBQuery *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_WAIT_FOR_GB_QUERY:
    {
        SVGA3dCmdWaitForGBQuery *pCmd = (SVGA3dCmdWaitForGBQuery *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_NOP:
    {
        /* Apparently there is nothing to do. */
        break;
    }

    case SVGA_3D_CMD_ENABLE_GART:
    {
        SVGA3dCmdEnableGart *pCmd = (SVGA3dCmdEnableGart *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_DISABLE_GART:
    {
        /* No corresponding SVGA3dCmd structure. */
        VMSVGA_3D_CMD_NOTIMPL();
        break;
    }

    case SVGA_3D_CMD_MAP_MOB_INTO_GART:
    {
        SVGA3dCmdMapMobIntoGart *pCmd = (SVGA3dCmdMapMobIntoGart *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_UNMAP_GART_RANGE:
    {
        SVGA3dCmdUnmapGartRange *pCmd = (SVGA3dCmdUnmapGartRange *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_DEFINE_GB_SCREENTARGET:
    {
        SVGA3dCmdDefineGBScreenTarget *pCmd = (SVGA3dCmdDefineGBScreenTarget *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_DESTROY_GB_SCREENTARGET:
    {
        SVGA3dCmdDestroyGBScreenTarget *pCmd = (SVGA3dCmdDestroyGBScreenTarget *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_BIND_GB_SCREENTARGET:
    {
        SVGA3dCmdBindGBScreenTarget *pCmd = (SVGA3dCmdBindGBScreenTarget *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_UPDATE_GB_SCREENTARGET:
    {
        SVGA3dCmdUpdateGBScreenTarget *pCmd = (SVGA3dCmdUpdateGBScreenTarget *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_READBACK_GB_IMAGE_PARTIAL:
    {
        SVGA3dCmdReadbackGBImagePartial *pCmd = (SVGA3dCmdReadbackGBImagePartial *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_INVALIDATE_GB_IMAGE_PARTIAL:
    {
        SVGA3dCmdInvalidateGBImagePartial *pCmd = (SVGA3dCmdInvalidateGBImagePartial *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_SET_GB_SHADERCONSTS_INLINE:
    {
        SVGA3dCmdSetGBShaderConstInline *pCmd = (SVGA3dCmdSetGBShaderConstInline *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_GB_SCREEN_DMA:
    {
        SVGA3dCmdGBScreenDMA *pCmd = (SVGA3dCmdGBScreenDMA *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_BIND_GB_SURFACE_WITH_PITCH:
    {
        SVGA3dCmdBindGBSurfaceWithPitch *pCmd = (SVGA3dCmdBindGBSurfaceWithPitch *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_GB_MOB_FENCE:
    {
        SVGA3dCmdGBMobFence *pCmd = (SVGA3dCmdGBMobFence *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_DEFINE_GB_SURFACE_V2:
    {
        /// @todo SVGA3dCmdDefineGBSurface_v2 is not defined in Mesa 17 header. Mesa 20 has it.
        //SVGA3dCmdDefineGBSurface_v2 *pCmd = (SVGA3dCmdDefineGBSurface_v2 *)pvCmd;
        //VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL();
        break;
    }

    case SVGA_3D_CMD_DEFINE_GB_MOB64:
    {
        SVGA3dCmdDefineGBMob64 *pCmd = (SVGA3dCmdDefineGBMob64 *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_REDEFINE_GB_MOB64:
    {
        SVGA3dCmdRedefineGBMob64 *pCmd = (SVGA3dCmdRedefineGBMob64 *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_NOP_ERROR:
    {
        /* Apparently there is nothing to do. */
        break;
    }

    case SVGA_3D_CMD_SET_VERTEX_STREAMS:
    {
        SVGA3dCmdSetVertexStreams *pCmd = (SVGA3dCmdSetVertexStreams *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_SET_VERTEX_DECLS:
    {
        SVGA3dCmdSetVertexDecls *pCmd = (SVGA3dCmdSetVertexDecls *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_SET_VERTEX_DIVISORS:
    {
        SVGA3dCmdSetVertexDivisors *pCmd = (SVGA3dCmdSetVertexDivisors *)pvCmd;
        VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK(sizeof(*pCmd));
        VMSVGA_3D_CMD_NOTIMPL(); RT_NOREF(pCmd);
        break;
    }

    case SVGA_3D_CMD_DRAW:
    {
        /* No corresponding SVGA3dCmd structure. */
        VMSVGA_3D_CMD_NOTIMPL();
        break;
    }

    case SVGA_3D_CMD_DRAW_INDEXED:
    {
        /* No corresponding SVGA3dCmd structure. */
        VMSVGA_3D_CMD_NOTIMPL();
        break;
    }

    default:
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatFifoUnkCmds);
        ASSERT_GUEST_MSG_FAILED(("enmCmdId=%d\n", enmCmdId));
        rcParse = VERR_NOT_IMPLEMENTED;
        break;
    }

    return rcParse;
}
# undef VMSVGAFIFO_CHECK_3D_CMD_MIN_SIZE_BREAK
#endif /* VBOX_WITH_VMSVGA3D */


/*
 *
 * Handlers for FIFO commands.
 *
 * Every handler takes the following parameters:
 *
 *    pThis               The shared VGA/VMSVGA state.
 *    pThisCC             The VGA/VMSVGA state for ring-3.
 *    pCmd                The command data.
 */


/* SVGA_CMD_UPDATE */
void vmsvgaR3CmdUpdate(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAFifoCmdUpdate const *pCmd)
{
    RT_NOREF(pThis);
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;

    STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdUpdate);
    Log(("SVGA_CMD_UPDATE %d,%d %dx%d\n", pCmd->x, pCmd->y, pCmd->width, pCmd->height));

    /** @todo Multiple screens? */
    VMSVGASCREENOBJECT *pScreen = vmsvgaR3GetScreenObject(pThisCC, 0);
    if (!pScreen) /* Can happen if screen is not defined (aScreens[idScreen].fDefined == false) yet. */
        return;

    vmsvgaR3UpdateScreen(pThisCC, pScreen, pCmd->x, pCmd->y, pCmd->width, pCmd->height);
}


/* SVGA_CMD_UPDATE_VERBOSE */
void vmsvgaR3CmdUpdateVerbose(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAFifoCmdUpdateVerbose const *pCmd)
{
    RT_NOREF(pThis);
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;

    STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdUpdateVerbose);
    Log(("SVGA_CMD_UPDATE_VERBOSE %d,%d %dx%d reason %#x\n", pCmd->x, pCmd->y, pCmd->width, pCmd->height, pCmd->reason));

    /** @todo Multiple screens? */
    VMSVGASCREENOBJECT *pScreen = vmsvgaR3GetScreenObject(pThisCC, 0);
    if (!pScreen) /* Can happen if screen is not defined (aScreens[idScreen].fDefined == false) yet. */
        return;

    vmsvgaR3UpdateScreen(pThisCC, pScreen, pCmd->x, pCmd->y, pCmd->width, pCmd->height);
}


/* SVGA_CMD_RECT_FILL */
void vmsvgaR3CmdRectFill(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAFifoCmdRectFill const *pCmd)
{
    RT_NOREF(pThis, pCmd);
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;

    STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdRectFill);
    Log(("SVGA_CMD_RECT_FILL %08X @ %d,%d (%dx%d)\n", pCmd->pixel, pCmd->destX, pCmd->destY, pCmd->width, pCmd->height));
    LogRelMax(4, ("VMSVGA: Unsupported SVGA_CMD_RECT_FILL command ignored.\n"));
}


/* SVGA_CMD_RECT_COPY */
void vmsvgaR3CmdRectCopy(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAFifoCmdRectCopy const *pCmd)
{
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;

    STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdRectCopy);
    Log(("SVGA_CMD_RECT_COPY %d,%d -> %d,%d %dx%d\n", pCmd->srcX, pCmd->srcY, pCmd->destX, pCmd->destY, pCmd->width, pCmd->height));

    VMSVGASCREENOBJECT *pScreen = vmsvgaR3GetScreenObject(pThisCC, 0);
    AssertPtrReturnVoid(pScreen);

    /* Check that arguments aren't complete junk. A precise check is done in vmsvgaR3RectCopy(). */
    ASSERT_GUEST_RETURN_VOID(pCmd->srcX < pThis->svga.u32MaxWidth);
    ASSERT_GUEST_RETURN_VOID(pCmd->destX < pThis->svga.u32MaxWidth);
    ASSERT_GUEST_RETURN_VOID(pCmd->width < pThis->svga.u32MaxWidth);
    ASSERT_GUEST_RETURN_VOID(pCmd->srcY < pThis->svga.u32MaxHeight);
    ASSERT_GUEST_RETURN_VOID(pCmd->destY < pThis->svga.u32MaxHeight);
    ASSERT_GUEST_RETURN_VOID(pCmd->height < pThis->svga.u32MaxHeight);

    vmsvgaR3RectCopy(pThisCC, pScreen, pCmd->srcX, pCmd->srcY, pCmd->destX, pCmd->destY,
                     pCmd->width, pCmd->height, pThis->vram_size);
    vmsvgaR3UpdateScreen(pThisCC, pScreen, pCmd->destX, pCmd->destY, pCmd->width, pCmd->height);
}


/* SVGA_CMD_RECT_ROP_COPY */
void vmsvgaR3CmdRectRopCopy(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAFifoCmdRectRopCopy const *pCmd)
{
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;

    STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdRectRopCopy);
    Log(("SVGA_CMD_RECT_ROP_COPY %d,%d -> %d,%d %dx%d ROP %#X\n", pCmd->srcX, pCmd->srcY, pCmd->destX, pCmd->destY, pCmd->width, pCmd->height, pCmd->rop));

    if (pCmd->rop != SVGA_ROP_COPY)
    {
        /* We only support the plain copy ROP which makes SVGA_CMD_RECT_ROP_COPY exactly the same
         * as SVGA_CMD_RECT_COPY. XFree86 4.1.0 and 4.2.0 drivers (driver version 10.4.0 and 10.7.0,
         * respectively) issue SVGA_CMD_RECT_ROP_COPY when SVGA_CAP_RECT_COPY is present even when
         * SVGA_CAP_RASTER_OP is not. However, the ROP will always be SVGA_ROP_COPY.
         */
        LogRelMax(4, ("VMSVGA: SVGA_CMD_RECT_ROP_COPY %d,%d -> %d,%d (%dx%d) ROP %X unsupported\n",
                      pCmd->srcX, pCmd->srcY, pCmd->destX, pCmd->destY, pCmd->width, pCmd->height, pCmd->rop));
        return;
    }

    VMSVGASCREENOBJECT *pScreen = vmsvgaR3GetScreenObject(pThisCC, 0);
    AssertPtrReturnVoid(pScreen);

    /* Check that arguments aren't complete junk. A precise check is done in vmsvgaR3RectCopy(). */
    ASSERT_GUEST_RETURN_VOID(pCmd->srcX < pThis->svga.u32MaxWidth);
    ASSERT_GUEST_RETURN_VOID(pCmd->destX < pThis->svga.u32MaxWidth);
    ASSERT_GUEST_RETURN_VOID(pCmd->width < pThis->svga.u32MaxWidth);
    ASSERT_GUEST_RETURN_VOID(pCmd->srcY < pThis->svga.u32MaxHeight);
    ASSERT_GUEST_RETURN_VOID(pCmd->destY < pThis->svga.u32MaxHeight);
    ASSERT_GUEST_RETURN_VOID(pCmd->height < pThis->svga.u32MaxHeight);

    vmsvgaR3RectCopy(pThisCC, pScreen, pCmd->srcX, pCmd->srcY, pCmd->destX, pCmd->destY,
                     pCmd->width, pCmd->height, pThis->vram_size);
    vmsvgaR3UpdateScreen(pThisCC, pScreen, pCmd->destX, pCmd->destY, pCmd->width, pCmd->height);
}


/* SVGA_CMD_DISPLAY_CURSOR */
void vmsvgaR3CmdDisplayCursor(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAFifoCmdDisplayCursor const *pCmd)
{
    RT_NOREF(pThis, pCmd);
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;

    STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdDisplayCursor);
    Log(("SVGA_CMD_DISPLAY_CURSOR id=%d state=%d\n", pCmd->id, pCmd->state));
    LogRelMax(4, ("VMSVGA: Unsupported SVGA_CMD_DISPLAY_CURSOR command ignored.\n"));
}


/* SVGA_CMD_MOVE_CURSOR */
void vmsvgaR3CmdMoveCursor(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAFifoCmdMoveCursor const *pCmd)
{
    RT_NOREF(pThis, pCmd);
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;

    STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdMoveCursor);
    Log(("SVGA_CMD_MOVE_CURSOR to %d,%d\n", pCmd->pos.x, pCmd->pos.y));
    LogRelMax(4, ("VMSVGA: Unsupported SVGA_CMD_MOVE_CURSOR command ignored.\n"));
}


/* SVGA_CMD_DEFINE_CURSOR */
void vmsvgaR3CmdDefineCursor(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAFifoCmdDefineCursor const *pCmd)
{
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;

    STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdDefineCursor);
    Log(("SVGA_CMD_DEFINE_CURSOR id=%d size (%dx%d) hotspot (%d,%d) andMaskDepth=%d xorMaskDepth=%d\n",
             pCmd->id, pCmd->width, pCmd->height, pCmd->hotspotX, pCmd->hotspotY, pCmd->andMaskDepth, pCmd->xorMaskDepth));

    ASSERT_GUEST_RETURN_VOID(pCmd->height < 2048 && pCmd->width < 2048);
    ASSERT_GUEST_RETURN_VOID(pCmd->andMaskDepth <= 32);
    ASSERT_GUEST_RETURN_VOID(pCmd->xorMaskDepth <= 32);
    RT_UNTRUSTED_VALIDATED_FENCE();

    uint32_t const cbSrcAndLine = RT_ALIGN_32(pCmd->width * (pCmd->andMaskDepth + (pCmd->andMaskDepth == 15)), 32) / 8;
    uint32_t const cbSrcAndMask = cbSrcAndLine * pCmd->height;
    uint32_t const cbSrcXorLine = RT_ALIGN_32(pCmd->width * (pCmd->xorMaskDepth + (pCmd->xorMaskDepth == 15)), 32) / 8;

    uint8_t const *pbSrcAndMask = (uint8_t const *)(pCmd + 1);
    uint8_t const *pbSrcXorMask = (uint8_t const *)(pCmd + 1) + cbSrcAndMask;

    uint32_t const cx = pCmd->width;
    uint32_t const cy = pCmd->height;

    /*
     * Convert the input to 1-bit AND mask and a 32-bit BRGA XOR mask.
     * The AND data uses 8-bit aligned scanlines.
     * The XOR data must be starting on a 32-bit boundrary.
     */
    uint32_t cbDstAndLine = RT_ALIGN_32(cx, 8) / 8;
    uint32_t cbDstAndMask = cbDstAndLine          * cy;
    uint32_t cbDstXorMask = cx * sizeof(uint32_t) * cy;
    uint32_t cbCopy = RT_ALIGN_32(cbDstAndMask, 4) + cbDstXorMask;

    uint8_t *pbCopy = (uint8_t *)RTMemAlloc(cbCopy);
    AssertReturnVoid(pbCopy);

    /* Convert the AND mask. */
    uint8_t       *pbDst     = pbCopy;
    uint8_t const *pbSrc     = pbSrcAndMask;
    switch (pCmd->andMaskDepth)
    {
        case 1:
            if (cbSrcAndLine == cbDstAndLine)
                memcpy(pbDst, pbSrc, cbSrcAndLine * cy);
            else
            {
                Assert(cbSrcAndLine > cbDstAndLine); /* lines are dword alined in source, but only byte in destination. */
                for (uint32_t y = 0; y < cy; y++)
                {
                    memcpy(pbDst, pbSrc, cbDstAndLine);
                    pbDst += cbDstAndLine;
                    pbSrc += cbSrcAndLine;
                }
            }
            break;
        /* Should take the XOR mask into account for the multi-bit AND mask. */
        case 8:
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; )
                {
                    uint8_t bDst = 0;
                    uint8_t fBit = 0x80;
                    do
                    {
                        uintptr_t const idxPal = pbSrc[x] * 3;
                        if (((   pThis->last_palette[idxPal]
                              | (pThis->last_palette[idxPal] >>  8)
                              | (pThis->last_palette[idxPal] >> 16)) & 0xff) > 0xfc)
                            bDst |= fBit;
                        fBit >>= 1;
                        x++;
                    } while (x < cx && (x & 7));
                    pbDst[(x - 1) / 8] = bDst;
                }
                pbDst += cbDstAndLine;
                pbSrc += cbSrcAndLine;
            }
            break;
        case 15:
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; )
                {
                    uint8_t bDst = 0;
                    uint8_t fBit = 0x80;
                    do
                    {
                        if ((pbSrc[x * 2] | (pbSrc[x * 2 + 1] & 0x7f)) >= 0xfc)
                            bDst |= fBit;
                        fBit >>= 1;
                        x++;
                    } while (x < cx && (x & 7));
                    pbDst[(x - 1) / 8] = bDst;
                }
                pbDst += cbDstAndLine;
                pbSrc += cbSrcAndLine;
            }
            break;
        case 16:
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; )
                {
                    uint8_t bDst = 0;
                    uint8_t fBit = 0x80;
                    do
                    {
                        if ((pbSrc[x * 2] | pbSrc[x * 2 + 1]) >= 0xfc)
                            bDst |= fBit;
                        fBit >>= 1;
                        x++;
                    } while (x < cx && (x & 7));
                    pbDst[(x - 1) / 8] = bDst;
                }
                pbDst += cbDstAndLine;
                pbSrc += cbSrcAndLine;
            }
            break;
        case 24:
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; )
                {
                    uint8_t bDst = 0;
                    uint8_t fBit = 0x80;
                    do
                    {
                        if ((pbSrc[x * 3] | pbSrc[x * 3 + 1] | pbSrc[x * 3 + 2]) >= 0xfc)
                            bDst |= fBit;
                        fBit >>= 1;
                        x++;
                    } while (x < cx && (x & 7));
                    pbDst[(x - 1) / 8] = bDst;
                }
                pbDst += cbDstAndLine;
                pbSrc += cbSrcAndLine;
            }
            break;
        case 32:
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; )
                {
                    uint8_t bDst = 0;
                    uint8_t fBit = 0x80;
                    do
                    {
                        if ((pbSrc[x * 4] | pbSrc[x * 4 + 1] | pbSrc[x * 4 + 2] | pbSrc[x * 4 + 3]) >= 0xfc)
                            bDst |= fBit;
                        fBit >>= 1;
                        x++;
                    } while (x < cx && (x & 7));
                    pbDst[(x - 1) / 8] = bDst;
                }
                pbDst += cbDstAndLine;
                pbSrc += cbSrcAndLine;
            }
            break;
        default:
            RTMemFreeZ(pbCopy, cbCopy);
            AssertFailedReturnVoid();
    }

    /* Convert the XOR mask. */
    uint32_t *pu32Dst = (uint32_t *)(pbCopy + RT_ALIGN_32(cbDstAndMask, 4));
    pbSrc  = pbSrcXorMask;
    switch (pCmd->xorMaskDepth)
    {
        case 1:
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; )
                {
                    /* most significant bit is the left most one. */
                    uint8_t bSrc = pbSrc[x / 8];
                    do
                    {
                        *pu32Dst++ = bSrc & 0x80 ? UINT32_C(0x00ffffff) : 0;
                        bSrc <<= 1;
                        x++;
                    } while ((x & 7) && x < cx);
                }
                pbSrc += cbSrcXorLine;
            }
            break;
        case 8:
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; x++)
                {
                    uint32_t u = pThis->last_palette[pbSrc[x]];
                    *pu32Dst++ = u;//RT_MAKE_U32_FROM_U8(RT_BYTE1(u), RT_BYTE2(u), RT_BYTE3(u), 0);
                }
                pbSrc += cbSrcXorLine;
            }
            break;
        case 15: /* Src: RGB-5-5-5 */
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; x++)
                {
                    uint32_t const uValue = RT_MAKE_U16(pbSrc[x * 2], pbSrc[x * 2 + 1]);
                    *pu32Dst++ = RT_MAKE_U32_FROM_U8(( uValue        & 0x1f) << 3,
                                                     ((uValue >>  5) & 0x1f) << 3,
                                                     ((uValue >> 10) & 0x1f) << 3, 0);
                }
                pbSrc += cbSrcXorLine;
            }
            break;
        case 16: /* Src: RGB-5-6-5 */
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; x++)
                {
                    uint32_t const uValue = RT_MAKE_U16(pbSrc[x * 2], pbSrc[x * 2 + 1]);
                    *pu32Dst++ = RT_MAKE_U32_FROM_U8(( uValue        & 0x1f) << 3,
                                                     ((uValue >>  5) & 0x3f) << 2,
                                                     ((uValue >> 11) & 0x1f) << 3, 0);
                }
                pbSrc += cbSrcXorLine;
            }
            break;
        case 24:
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; x++)
                    *pu32Dst++ = RT_MAKE_U32_FROM_U8(pbSrc[x*3], pbSrc[x*3 + 1], pbSrc[x*3 + 2], 0);
                pbSrc += cbSrcXorLine;
            }
            break;
        case 32:
            for (uint32_t y = 0; y < cy; y++)
            {
                for (uint32_t x = 0; x < cx; x++)
                    *pu32Dst++ = RT_MAKE_U32_FROM_U8(pbSrc[x*4], pbSrc[x*4 + 1], pbSrc[x*4 + 2], 0);
                pbSrc += cbSrcXorLine;
            }
            break;
        default:
            RTMemFreeZ(pbCopy, cbCopy);
            AssertFailedReturnVoid();
    }

    /*
     * Pass it to the frontend/whatever.
     */
    vmsvgaR3InstallNewCursor(pThisCC, pSvgaR3State, false /*fAlpha*/, pCmd->hotspotX, pCmd->hotspotY,
                             cx, cy, pbCopy, cbCopy);
}


/* SVGA_CMD_DEFINE_ALPHA_CURSOR */
void vmsvgaR3CmdDefineAlphaCursor(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAFifoCmdDefineAlphaCursor const *pCmd)
{
    RT_NOREF(pThis);
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;

    STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdDefineAlphaCursor);
    Log(("VMSVGA cmd: SVGA_CMD_DEFINE_ALPHA_CURSOR id=%d size (%dx%d) hotspot (%d,%d)\n", pCmd->id, pCmd->width, pCmd->height, pCmd->hotspotX, pCmd->hotspotY));

    /* Check against a reasonable upper limit to prevent integer overflows in the sanity checks below. */
    ASSERT_GUEST_RETURN_VOID(pCmd->height < 2048 && pCmd->width < 2048);
    RT_UNTRUSTED_VALIDATED_FENCE();

    /* The mouse pointer interface always expects an AND mask followed by the color data (XOR mask). */
    uint32_t cbAndMask     = (pCmd->width + 7) / 8 * pCmd->height;          /* size of the AND mask */
    cbAndMask              = ((cbAndMask + 3) & ~3);                        /* + gap for alignment */
    uint32_t cbXorMask     = pCmd->width * sizeof(uint32_t) * pCmd->height; /* + size of the XOR mask (32-bit BRGA format) */
    uint32_t cbCursorShape = cbAndMask + cbXorMask;

    uint8_t *pCursorCopy = (uint8_t *)RTMemAlloc(cbCursorShape);
    AssertPtrReturnVoid(pCursorCopy);

    /* Transparency is defined by the alpha bytes, so make the whole bitmap visible. */
    memset(pCursorCopy, 0xff, cbAndMask);
    /* Colour data */
    memcpy(pCursorCopy + cbAndMask, pCmd + 1, cbXorMask);

    vmsvgaR3InstallNewCursor(pThisCC, pSvgaR3State, true /*fAlpha*/, pCmd->hotspotX, pCmd->hotspotY,
                             pCmd->width, pCmd->height, pCursorCopy, cbCursorShape);
}


/* SVGA_CMD_ESCAPE */
void vmsvgaR3CmdEscape(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAFifoCmdEscape const *pCmd)
{
    RT_NOREF(pThis);
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;

    STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdEscape);

    if (pCmd->nsid == SVGA_ESCAPE_NSID_VMWARE)
    {
        ASSERT_GUEST_RETURN_VOID(pCmd->size >= sizeof(uint32_t));
        RT_UNTRUSTED_VALIDATED_FENCE();

        uint32_t const cmd = *(uint32_t *)(pCmd + 1);
        Log(("SVGA_CMD_ESCAPE (%#x %#x) VMWARE cmd=%#x\n", pCmd->nsid, pCmd->size, cmd));

        switch (cmd)
        {
            case SVGA_ESCAPE_VMWARE_VIDEO_SET_REGS:
            {
                SVGAEscapeVideoSetRegs *pVideoCmd = (SVGAEscapeVideoSetRegs *)(pCmd + 1);
                ASSERT_GUEST_RETURN_VOID(pCmd->size >= sizeof(pVideoCmd->header));
                RT_UNTRUSTED_VALIDATED_FENCE();

                uint32_t const cRegs = (pCmd->size - sizeof(pVideoCmd->header)) / sizeof(pVideoCmd->items[0]);

                Log(("SVGA_ESCAPE_VMWARE_VIDEO_SET_REGS: stream %#x\n", pVideoCmd->header.streamId));
                for (uint32_t iReg = 0; iReg < cRegs; iReg++)
                    Log(("SVGA_ESCAPE_VMWARE_VIDEO_SET_REGS: reg %#x val %#x\n", pVideoCmd->items[iReg].registerId, pVideoCmd->items[iReg].value));
                RT_NOREF_PV(pVideoCmd);
                break;
            }

            case SVGA_ESCAPE_VMWARE_VIDEO_FLUSH:
            {
                SVGAEscapeVideoFlush *pVideoCmd = (SVGAEscapeVideoFlush *)(pCmd + 1);
                ASSERT_GUEST_RETURN_VOID(pCmd->size >= sizeof(*pVideoCmd));
                Log(("SVGA_ESCAPE_VMWARE_VIDEO_FLUSH: stream %#x\n", pVideoCmd->streamId));
                RT_NOREF_PV(pVideoCmd);
                break;
            }

            default:
                Log(("SVGA_CMD_ESCAPE: Unknown vmware escape: %#x\n", cmd));
                break;
        }
    }
    else
        Log(("SVGA_CMD_ESCAPE %#x %#x\n", pCmd->nsid, pCmd->size));
}


/* SVGA_CMD_DEFINE_SCREEN */
void vmsvgaR3CmdDefineScreen(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAFifoCmdDefineScreen const *pCmd)
{
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;

    STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdDefineScreen);
    Log(("SVGA_CMD_DEFINE_SCREEN id=%x flags=%x size=(%d,%d) root=(%d,%d) %d:0x%x 0x%x\n",
             pCmd->screen.id, pCmd->screen.flags, pCmd->screen.size.width, pCmd->screen.size.height, pCmd->screen.root.x, pCmd->screen.root.y,
             pCmd->screen.backingStore.ptr.gmrId, pCmd->screen.backingStore.ptr.offset, pCmd->screen.backingStore.pitch));

    uint32_t const idScreen = pCmd->screen.id;
    ASSERT_GUEST_RETURN_VOID(idScreen < RT_ELEMENTS(pSvgaR3State->aScreens));

    uint32_t const uWidth = pCmd->screen.size.width;
    ASSERT_GUEST_RETURN_VOID(uWidth <= pThis->svga.u32MaxWidth);

    uint32_t const uHeight = pCmd->screen.size.height;
    ASSERT_GUEST_RETURN_VOID(uHeight <= pThis->svga.u32MaxHeight);

    uint32_t const cbWidth = uWidth * ((32 + 7) / 8); /** @todo 32? */
    uint32_t const cbPitch = pCmd->screen.backingStore.pitch ? pCmd->screen.backingStore.pitch : cbWidth;
    ASSERT_GUEST_RETURN_VOID(cbWidth <= cbPitch);

    uint32_t const uScreenOffset = pCmd->screen.backingStore.ptr.offset;
    ASSERT_GUEST_RETURN_VOID(uScreenOffset < pThis->vram_size);

    uint32_t const cbVram = pThis->vram_size - uScreenOffset;
    /* If we have a not zero pitch, then height can't exceed the available VRAM. */
    ASSERT_GUEST_RETURN_VOID(   (uHeight == 0 && cbPitch == 0)
                             || (cbPitch > 0 && uHeight <= cbVram / cbPitch));
    RT_UNTRUSTED_VALIDATED_FENCE();

    VMSVGASCREENOBJECT *pScreen = &pSvgaR3State->aScreens[idScreen];
    pScreen->fDefined  = true;
    pScreen->fModified = true;
    pScreen->fuScreen  = pCmd->screen.flags;
    pScreen->idScreen  = idScreen;
    if (!RT_BOOL(pCmd->screen.flags & (SVGA_SCREEN_DEACTIVATE | SVGA_SCREEN_BLANKING)))
    {
        /* Not blanked. */
        ASSERT_GUEST_RETURN_VOID(uWidth > 0 && uHeight > 0);
        RT_UNTRUSTED_VALIDATED_FENCE();

        pScreen->xOrigin = pCmd->screen.root.x;
        pScreen->yOrigin = pCmd->screen.root.y;
        pScreen->cWidth  = uWidth;
        pScreen->cHeight = uHeight;
        pScreen->offVRAM = uScreenOffset;
        pScreen->cbPitch = cbPitch;
        pScreen->cBpp    = 32;
    }
    else
    {
        /* Screen blanked. Keep old values. */
    }

    pThis->svga.fGFBRegisters = false;
    vmsvgaR3ChangeMode(pThis, pThisCC);

#ifdef VBOX_WITH_VMSVGA3D
    if (RT_LIKELY(pThis->svga.f3DEnabled))
        vmsvga3dDefineScreen(pThis, pThisCC, pScreen);
#endif
}


/* SVGA_CMD_DESTROY_SCREEN */
void vmsvgaR3CmdDestroyScreen(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAFifoCmdDestroyScreen const *pCmd)
{
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;

    STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdDestroyScreen);
    Log(("SVGA_CMD_DESTROY_SCREEN id=%x\n", pCmd->screenId));

    uint32_t const idScreen = pCmd->screenId;
    ASSERT_GUEST_RETURN_VOID(idScreen < RT_ELEMENTS(pSvgaR3State->aScreens));
    RT_UNTRUSTED_VALIDATED_FENCE();

    VMSVGASCREENOBJECT *pScreen = &pSvgaR3State->aScreens[idScreen];
    pScreen->fModified = true;
    pScreen->fDefined  = false;
    pScreen->idScreen  = idScreen;

#ifdef VBOX_WITH_VMSVGA3D
    if (RT_LIKELY(pThis->svga.f3DEnabled))
        vmsvga3dDestroyScreen(pThisCC, pScreen);
#endif
    vmsvgaR3ChangeMode(pThis, pThisCC);
}


/* SVGA_CMD_DEFINE_GMRFB */
void vmsvgaR3CmdDefineGMRFB(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAFifoCmdDefineGMRFB const *pCmd)
{
    RT_NOREF(pThis);
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;

    STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdDefineGmrFb);
    Log(("SVGA_CMD_DEFINE_GMRFB gmr=%x offset=%x bytesPerLine=%x bpp=%d color depth=%d\n",
             pCmd->ptr.gmrId, pCmd->ptr.offset, pCmd->bytesPerLine, pCmd->format.bitsPerPixel, pCmd->format.colorDepth));

    pSvgaR3State->GMRFB.ptr          = pCmd->ptr;
    pSvgaR3State->GMRFB.bytesPerLine = pCmd->bytesPerLine;
    pSvgaR3State->GMRFB.format       = pCmd->format;
}


/* SVGA_CMD_BLIT_GMRFB_TO_SCREEN */
void vmsvgaR3CmdBlitGMRFBToScreen(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAFifoCmdBlitGMRFBToScreen const *pCmd)
{
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;

    STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdBlitGmrFbToScreen);
    Log(("SVGA_CMD_BLIT_GMRFB_TO_SCREEN src=(%d,%d) dest id=%d (%d,%d)(%d,%d)\n",
             pCmd->srcOrigin.x, pCmd->srcOrigin.y, pCmd->destScreenId, pCmd->destRect.left, pCmd->destRect.top, pCmd->destRect.right, pCmd->destRect.bottom));

    ASSERT_GUEST_RETURN_VOID(pCmd->destScreenId < RT_ELEMENTS(pSvgaR3State->aScreens));
    RT_UNTRUSTED_VALIDATED_FENCE();

    VMSVGASCREENOBJECT *pScreen = vmsvgaR3GetScreenObject(pThisCC, pCmd->destScreenId);
    AssertPtrReturnVoid(pScreen);

    /** @todo Support GMRFB.format.s.bitsPerPixel != pThis->svga.uBpp ?  */
    AssertReturnVoid(pSvgaR3State->GMRFB.format.bitsPerPixel == pScreen->cBpp);

    /* Clip destRect to the screen dimensions. */
    SVGASignedRect screenRect;
    screenRect.left   = 0;
    screenRect.top    = 0;
    screenRect.right  = pScreen->cWidth;
    screenRect.bottom = pScreen->cHeight;
    SVGASignedRect clipRect = pCmd->destRect;
    vmsvgaR3ClipRect(&screenRect, &clipRect);
    RT_UNTRUSTED_VALIDATED_FENCE();

    uint32_t const width  = clipRect.right - clipRect.left;
    uint32_t const height = clipRect.bottom - clipRect.top;

    if (   width == 0
        || height == 0)
        return;  /* Nothing to do. */

    int32_t const srcx = pCmd->srcOrigin.x + (clipRect.left - pCmd->destRect.left);
    int32_t const srcy = pCmd->srcOrigin.y + (clipRect.top - pCmd->destRect.top);

    /* Copy the defined by GMRFB image to the screen 0 VRAM area.
     * Prepare parameters for vmsvgaR3GmrTransfer.
     */
    AssertReturnVoid(pScreen->offVRAM < pThis->vram_size); /* Paranoia. Ensured by SVGA_CMD_DEFINE_SCREEN. */

    /* Destination: host buffer which describes the screen 0 VRAM.
     * Important are pbHstBuf and cbHstBuf. offHst and cbHstPitch are verified by vmsvgaR3GmrTransfer.
     */
    uint8_t * const pbHstBuf = (uint8_t *)pThisCC->pbVRam + pScreen->offVRAM;
    uint32_t const cbScanline = pScreen->cbPitch ? pScreen->cbPitch :
                                                   width * (RT_ALIGN(pScreen->cBpp, 8) / 8);
    uint32_t cbHstBuf = cbScanline * pScreen->cHeight;
    if (cbHstBuf > pThis->vram_size - pScreen->offVRAM)
       cbHstBuf = pThis->vram_size - pScreen->offVRAM; /* Paranoia. */
    uint32_t const offHst =   (clipRect.left * RT_ALIGN(pScreen->cBpp, 8)) / 8
                            + cbScanline * clipRect.top;
    int32_t const cbHstPitch = cbScanline;

    /* Source: GMRFB. vmsvgaR3GmrTransfer ensures that no memory outside the GMR is read. */
    SVGAGuestPtr const gstPtr = pSvgaR3State->GMRFB.ptr;
    uint32_t const offGst =  (srcx * RT_ALIGN(pSvgaR3State->GMRFB.format.bitsPerPixel, 8)) / 8
                           + pSvgaR3State->GMRFB.bytesPerLine * srcy;
    int32_t const cbGstPitch = pSvgaR3State->GMRFB.bytesPerLine;

    int rc = vmsvgaR3GmrTransfer(pThis, pThisCC, SVGA3D_WRITE_HOST_VRAM,
                                 pbHstBuf, cbHstBuf, offHst, cbHstPitch,
                                 gstPtr, offGst, cbGstPitch,
                                 (width * RT_ALIGN(pScreen->cBpp, 8)) / 8, height);
    AssertRC(rc);
    vmsvgaR3UpdateScreen(pThisCC, pScreen, clipRect.left, clipRect.top, width, height);
}


/* SVGA_CMD_BLIT_SCREEN_TO_GMRFB */
void vmsvgaR3CmdBlitScreenToGMRFB(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAFifoCmdBlitScreenToGMRFB const *pCmd)
{
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;

    STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdBlitScreentoGmrFb);
    /* Note! This can fetch 3d render results as well!! */
    Log(("SVGA_CMD_BLIT_SCREEN_TO_GMRFB dest=(%d,%d) src id=%d (%d,%d)(%d,%d)\n",
             pCmd->destOrigin.x, pCmd->destOrigin.y, pCmd->srcScreenId, pCmd->srcRect.left, pCmd->srcRect.top, pCmd->srcRect.right, pCmd->srcRect.bottom));

    ASSERT_GUEST_RETURN_VOID(pCmd->srcScreenId < RT_ELEMENTS(pSvgaR3State->aScreens));
    RT_UNTRUSTED_VALIDATED_FENCE();

    VMSVGASCREENOBJECT *pScreen = vmsvgaR3GetScreenObject(pThisCC, pCmd->srcScreenId);
    AssertPtrReturnVoid(pScreen);

    /** @todo Support GMRFB.format.bitsPerPixel != pThis->svga.uBpp ? */
    AssertReturnVoid(pSvgaR3State->GMRFB.format.bitsPerPixel == pScreen->cBpp);

    /* Clip destRect to the screen dimensions. */
    SVGASignedRect screenRect;
    screenRect.left   = 0;
    screenRect.top    = 0;
    screenRect.right  = pScreen->cWidth;
    screenRect.bottom = pScreen->cHeight;
    SVGASignedRect clipRect = pCmd->srcRect;
    vmsvgaR3ClipRect(&screenRect, &clipRect);
    RT_UNTRUSTED_VALIDATED_FENCE();

    uint32_t const width  = clipRect.right - clipRect.left;
    uint32_t const height = clipRect.bottom - clipRect.top;

    if (   width == 0
        || height == 0)
        return;  /* Nothing to do. */

    int32_t const dstx = pCmd->destOrigin.x + (clipRect.left - pCmd->srcRect.left);
    int32_t const dsty = pCmd->destOrigin.y + (clipRect.top - pCmd->srcRect.top);

    /* Copy the defined by GMRFB image to the screen 0 VRAM area.
     * Prepare parameters for vmsvgaR3GmrTransfer.
     */
    AssertReturnVoid(pScreen->offVRAM < pThis->vram_size); /* Paranoia. Ensured by SVGA_CMD_DEFINE_SCREEN. */

    /* Source: host buffer which describes the screen 0 VRAM.
     * Important are pbHstBuf and cbHstBuf. offHst and cbHstPitch are verified by vmsvgaR3GmrTransfer.
     */
    uint8_t * const pbHstBuf = (uint8_t *)pThisCC->pbVRam + pScreen->offVRAM;
    uint32_t const cbScanline = pScreen->cbPitch ? pScreen->cbPitch :
                                                   width * (RT_ALIGN(pScreen->cBpp, 8) / 8);
    uint32_t cbHstBuf = cbScanline * pScreen->cHeight;
    if (cbHstBuf > pThis->vram_size - pScreen->offVRAM)
       cbHstBuf = pThis->vram_size - pScreen->offVRAM; /* Paranoia. */
    uint32_t const offHst =   (clipRect.left * RT_ALIGN(pScreen->cBpp, 8)) / 8
                            + cbScanline * clipRect.top;
    int32_t const cbHstPitch = cbScanline;

    /* Destination: GMRFB. vmsvgaR3GmrTransfer ensures that no memory outside the GMR is read. */
    SVGAGuestPtr const gstPtr = pSvgaR3State->GMRFB.ptr;
    uint32_t const offGst =  (dstx * RT_ALIGN(pSvgaR3State->GMRFB.format.bitsPerPixel, 8)) / 8
                           + pSvgaR3State->GMRFB.bytesPerLine * dsty;
    int32_t const cbGstPitch = pSvgaR3State->GMRFB.bytesPerLine;

    int rc = vmsvgaR3GmrTransfer(pThis, pThisCC, SVGA3D_READ_HOST_VRAM,
                                 pbHstBuf, cbHstBuf, offHst, cbHstPitch,
                                 gstPtr, offGst, cbGstPitch,
                                 (width * RT_ALIGN(pScreen->cBpp, 8)) / 8, height);
    AssertRC(rc);
}


/* SVGA_CMD_ANNOTATION_FILL */
void vmsvgaR3CmdAnnotationFill(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAFifoCmdAnnotationFill const *pCmd)
{
    RT_NOREF(pThis);
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;

    STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdAnnotationFill);
    Log(("SVGA_CMD_ANNOTATION_FILL red=%x green=%x blue=%x\n", pCmd->color.r, pCmd->color.g, pCmd->color.b));

    pSvgaR3State->colorAnnotation = pCmd->color;
}


/* SVGA_CMD_ANNOTATION_COPY */
void vmsvgaR3CmdAnnotationCopy(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAFifoCmdAnnotationCopy const *pCmd)
{
    RT_NOREF(pThis, pCmd);
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;

    STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdAnnotationCopy);
    Log(("SVGA_CMD_ANNOTATION_COPY srcOrigin %d,%d, srcScreenId %u\n", pCmd->srcOrigin.x, pCmd->srcOrigin.y, pCmd->srcScreenId));

    AssertFailed();
}


#ifdef VBOX_WITH_VMSVGA3D
/* SVGA_CMD_DEFINE_GMR2 */
void vmsvgaR3CmdDefineGMR2(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAFifoCmdDefineGMR2 const *pCmd)
{
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;

    STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdDefineGmr2);
    Log(("SVGA_CMD_DEFINE_GMR2 id=%#x %#x pages\n", pCmd->gmrId, pCmd->numPages));

    /* Validate current GMR id. */
    ASSERT_GUEST_RETURN_VOID(pCmd->gmrId < pThis->svga.cGMR);
    ASSERT_GUEST_RETURN_VOID(pCmd->numPages <= VMSVGA_MAX_GMR_PAGES);
    RT_UNTRUSTED_VALIDATED_FENCE();

    if (!pCmd->numPages)
    {
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdDefineGmr2Free);
        vmsvgaR3GmrFree(pThisCC, pCmd->gmrId);
    }
    else
    {
        PGMR pGMR = &pSvgaR3State->paGMR[pCmd->gmrId];
        if (pGMR->cMaxPages)
            STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdDefineGmr2Modify);

        /* Not sure if we should always free the descriptor, but for simplicity
           we do so if the new size is smaller than the current. */
        /** @todo always free the descriptor in SVGA_CMD_DEFINE_GMR2? */
        if (pGMR->cbTotal / X86_PAGE_SIZE > pGMR->cMaxPages)
            vmsvgaR3GmrFree(pThisCC, pCmd->gmrId);

        pGMR->cMaxPages = pCmd->numPages;
        /* The rest is done by the REMAP_GMR2 command. */
    }
}


/* SVGA_CMD_REMAP_GMR2 */
void vmsvgaR3CmdRemapGMR2(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAFifoCmdRemapGMR2 const *pCmd)
{
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;

    STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdRemapGmr2);
    Log(("SVGA_CMD_REMAP_GMR2 id=%#x flags=%#x offset=%#x npages=%#x\n", pCmd->gmrId, pCmd->flags, pCmd->offsetPages, pCmd->numPages));

    /* Validate current GMR id and size. */
    ASSERT_GUEST_RETURN_VOID(pCmd->gmrId < pThis->svga.cGMR);
    RT_UNTRUSTED_VALIDATED_FENCE();
    PGMR pGMR = &pSvgaR3State->paGMR[pCmd->gmrId];
    ASSERT_GUEST_RETURN_VOID(   (uint64_t)pCmd->offsetPages + pCmd->numPages
                             <= RT_MIN(pGMR->cMaxPages, RT_MIN(VMSVGA_MAX_GMR_PAGES, UINT32_MAX / X86_PAGE_SIZE)));
    ASSERT_GUEST_RETURN_VOID(!pCmd->offsetPages || pGMR->paDesc); /** @todo */

    if (pCmd->numPages == 0)
        return;
    RT_UNTRUSTED_VALIDATED_FENCE();

    /* Calc new total page count so we can use it instead of cMaxPages for allocations below. */
    uint32_t const cNewTotalPages = RT_MAX(pGMR->cbTotal >> X86_PAGE_SHIFT, pCmd->offsetPages + pCmd->numPages);

    /*
     * We flatten the existing descriptors into a page array, overwrite the
     * pages specified in this command and then recompress the descriptor.
     */
    /** @todo Optimize the GMR remap algorithm! */

    /* Save the old page descriptors as an array of page frame numbers (address >> X86_PAGE_SHIFT) */
    uint64_t *paNewPage64 = NULL;
    if (pGMR->paDesc)
    {
        STAM_REL_COUNTER_INC(&pSvgaR3State->StatR3CmdRemapGmr2Modify);

        paNewPage64 = (uint64_t *)RTMemAllocZ(cNewTotalPages * sizeof(uint64_t));
        AssertPtrReturnVoid(paNewPage64);

        uint32_t idxPage = 0;
        for (uint32_t i = 0; i < pGMR->numDescriptors; i++)
            for (uint32_t j = 0; j < pGMR->paDesc[i].numPages; j++)
                paNewPage64[idxPage++] = (pGMR->paDesc[i].GCPhys + j * X86_PAGE_SIZE) >> X86_PAGE_SHIFT;
        AssertReturnVoidStmt(idxPage == pGMR->cbTotal >> X86_PAGE_SHIFT, RTMemFree(paNewPage64));
        RT_UNTRUSTED_VALIDATED_FENCE();
    }

    /* Free the old GMR if present. */
    if (pGMR->paDesc)
        RTMemFree(pGMR->paDesc);

    /* Allocate the maximum amount possible (everything non-continuous) */
    PVMSVGAGMRDESCRIPTOR paDescs;
    pGMR->paDesc = paDescs = (PVMSVGAGMRDESCRIPTOR)RTMemAllocZ(cNewTotalPages * sizeof(VMSVGAGMRDESCRIPTOR));
    AssertReturnVoidStmt(paDescs, RTMemFree(paNewPage64));

    if (pCmd->flags & SVGA_REMAP_GMR2_VIA_GMR)
    {
        /** @todo */
        AssertFailed();
        pGMR->numDescriptors = 0;
    }
    else
    {
        uint32_t  *paPages32 = (uint32_t *)(pCmd + 1);
        uint64_t  *paPages64 = (uint64_t *)(pCmd + 1);
        bool       fGCPhys64 = RT_BOOL(pCmd->flags & SVGA_REMAP_GMR2_PPN64);

        uint32_t cPages;
        if (paNewPage64)
        {
            /* Overwrite the old page array with the new page values. */
            if (fGCPhys64)
                for (uint32_t i = pCmd->offsetPages; i < pCmd->offsetPages + pCmd->numPages; i++)
                    paNewPage64[i] = paPages64[i - pCmd->offsetPages];
            else
                for (uint32_t i = pCmd->offsetPages; i < pCmd->offsetPages + pCmd->numPages; i++)
                    paNewPage64[i] = paPages32[i - pCmd->offsetPages];

            /* Use the updated page array instead of the command data. */
            fGCPhys64      = true;
            paPages64      = paNewPage64;
            cPages = cNewTotalPages;
        }
        else
            cPages = pCmd->numPages;

        /* The first page. */
        /** @todo The 0x00000FFFFFFFFFFF mask limits to 44 bits and should not be
         *        applied to paNewPage64. */
        RTGCPHYS GCPhys;
        if (fGCPhys64)
            GCPhys = (paPages64[0] << X86_PAGE_SHIFT) & UINT64_C(0x00000FFFFFFFFFFF); /* Seeing rubbish in the top bits with certain linux guests. */
        else
            GCPhys = (RTGCPHYS)paPages32[0] << PAGE_SHIFT;
        paDescs[0].GCPhys    = GCPhys;
        paDescs[0].numPages  = 1;

        /* Subsequent pages. */
        uint32_t iDescriptor = 0;
        for (uint32_t i = 1; i < cPages; i++)
        {
            if (pCmd->flags & SVGA_REMAP_GMR2_PPN64)
                GCPhys = (paPages64[i] << X86_PAGE_SHIFT) & UINT64_C(0x00000FFFFFFFFFFF); /* Seeing rubbish in the top bits with certain linux guests. */
            else
                GCPhys = (RTGCPHYS)paPages32[i] << X86_PAGE_SHIFT;

            /* Continuous physical memory? */
            if (GCPhys == paDescs[iDescriptor].GCPhys + paDescs[iDescriptor].numPages * X86_PAGE_SIZE)
            {
                Assert(paDescs[iDescriptor].numPages);
                paDescs[iDescriptor].numPages++;
                Log5Func(("Page %x GCPhys=%RGp successor\n", i, GCPhys));
            }
            else
            {
                iDescriptor++;
                paDescs[iDescriptor].GCPhys   = GCPhys;
                paDescs[iDescriptor].numPages = 1;
                Log5Func(("Page %x GCPhys=%RGp\n", i, paDescs[iDescriptor].GCPhys));
            }
        }

        pGMR->cbTotal = cNewTotalPages << X86_PAGE_SHIFT;
        Log5Func(("Nr of descriptors %x; cbTotal=%#x\n", iDescriptor + 1, cNewTotalPages));
        pGMR->numDescriptors = iDescriptor + 1;
    }

    if (paNewPage64)
        RTMemFree(paNewPage64);
}


/**
 * Free the specified GMR
 *
 * @param   pThisCC         The VGA/VMSVGA state for ring-3.
 * @param   idGMR           GMR id
 */
void vmsvgaR3GmrFree(PVGASTATECC pThisCC, uint32_t idGMR)
{
    PVMSVGAR3STATE pSVGAState = pThisCC->svga.pSvgaR3State;

    /* Free the old descriptor if present. */
    PGMR pGMR = &pSVGAState->paGMR[idGMR];
    if (   pGMR->numDescriptors
        || pGMR->paDesc /* needed till we implement SVGA_REMAP_GMR2_VIA_GMR */)
    {
# ifdef DEBUG_GMR_ACCESS
        VMR3ReqCallWaitU(PDMDevHlpGetUVM(pThisCC->pDevIns), VMCPUID_ANY, (PFNRT)vmsvgaR3DeregisterGmr, 2, pDevIns, idGMR);
# endif

        Assert(pGMR->paDesc);
        RTMemFree(pGMR->paDesc);
        pGMR->paDesc         = NULL;
        pGMR->numDescriptors = 0;
        pGMR->cbTotal        = 0;
        pGMR->cMaxPages      = 0;
    }
    Assert(!pGMR->cMaxPages);
    Assert(!pGMR->cbTotal);
}
#endif /* VBOX_WITH_VMSVGA3D */


/**
 * Copy between a GMR and a host memory buffer.
 *
 * @returns VBox status code.
 * @param   pThis           The shared VGA/VMSVGA instance data.
 * @param   pThisCC         The VGA/VMSVGA state for ring-3.
 * @param   enmTransferType Transfer type (read/write)
 * @param   pbHstBuf        Host buffer pointer (valid)
 * @param   cbHstBuf        Size of host buffer (valid)
 * @param   offHst          Host buffer offset of the first scanline
 * @param   cbHstPitch      Destination buffer pitch
 * @param   gstPtr          GMR description
 * @param   offGst          Guest buffer offset of the first scanline
 * @param   cbGstPitch      Guest buffer pitch
 * @param   cbWidth         Width in bytes to copy
 * @param   cHeight         Number of scanllines to copy
 */
int vmsvgaR3GmrTransfer(PVGASTATE pThis, PVGASTATECC pThisCC, const SVGA3dTransferType enmTransferType,
                        uint8_t *pbHstBuf, uint32_t cbHstBuf, uint32_t offHst, int32_t cbHstPitch,
                        SVGAGuestPtr gstPtr, uint32_t offGst, int32_t cbGstPitch,
                        uint32_t cbWidth, uint32_t cHeight)
{
    PVMSVGAR3STATE  pSVGAState = pThisCC->svga.pSvgaR3State;
    PPDMDEVINS      pDevIns = pThisCC->pDevIns; /* simpler */
    int             rc;

    LogFunc(("%s host %p size=%d offset %d pitch=%d; guest gmr=%#x:%#x offset=%d pitch=%d cbWidth=%d cHeight=%d\n",
             enmTransferType == SVGA3D_READ_HOST_VRAM ? "WRITE" : "READ", /* GMR op: READ host VRAM means WRITE GMR */
             pbHstBuf, cbHstBuf, offHst, cbHstPitch,
             gstPtr.gmrId, gstPtr.offset, offGst, cbGstPitch, cbWidth, cHeight));
    AssertReturn(cbWidth && cHeight, VERR_INVALID_PARAMETER);

    PGMR pGMR;
    uint32_t cbGmr; /* The GMR size in bytes. */
    if (gstPtr.gmrId == SVGA_GMR_FRAMEBUFFER)
    {
        pGMR = NULL;
        cbGmr = pThis->vram_size;
    }
    else
    {
        AssertReturn(gstPtr.gmrId < pThis->svga.cGMR, VERR_INVALID_PARAMETER);
        RT_UNTRUSTED_VALIDATED_FENCE();
        pGMR = &pSVGAState->paGMR[gstPtr.gmrId];
        cbGmr = pGMR->cbTotal;
    }

    /*
     * GMR
     */
    /* Calculate GMR offset of the data to be copied. */
    AssertMsgReturn(gstPtr.offset < cbGmr,
                    ("gmr=%#x:%#x offGst=%#x cbGstPitch=%#x cHeight=%#x cbWidth=%#x cbGmr=%#x\n",
                     gstPtr.gmrId, gstPtr.offset, offGst, cbGstPitch, cHeight, cbWidth, cbGmr),
                    VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();
    AssertMsgReturn(offGst < cbGmr - gstPtr.offset,
                    ("gmr=%#x:%#x offGst=%#x cbGstPitch=%#x cHeight=%#x cbWidth=%#x cbGmr=%#x\n",
                     gstPtr.gmrId, gstPtr.offset, offGst, cbGstPitch, cHeight, cbWidth, cbGmr),
                    VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();
    uint32_t const offGmr = offGst + gstPtr.offset; /* Offset in the GMR, where the first scanline is located. */

    /* Verify that cbWidth is less than scanline and fits into the GMR. */
    uint32_t const cbGmrScanline = cbGstPitch > 0 ? cbGstPitch : -cbGstPitch;
    AssertMsgReturn(cbGmrScanline != 0,
                    ("gmr=%#x:%#x offGst=%#x cbGstPitch=%#x cHeight=%#x cbWidth=%#x cbGmr=%#x\n",
                     gstPtr.gmrId, gstPtr.offset, offGst, cbGstPitch, cHeight, cbWidth, cbGmr),
                    VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();
    AssertMsgReturn(cbWidth <= cbGmrScanline,
                    ("gmr=%#x:%#x offGst=%#x cbGstPitch=%#x cHeight=%#x cbWidth=%#x cbGmr=%#x\n",
                     gstPtr.gmrId, gstPtr.offset, offGst, cbGstPitch, cHeight, cbWidth, cbGmr),
                    VERR_INVALID_PARAMETER);
    AssertMsgReturn(cbWidth <= cbGmr - offGmr,
                    ("gmr=%#x:%#x offGst=%#x cbGstPitch=%#x cHeight=%#x cbWidth=%#x cbGmr=%#x\n",
                     gstPtr.gmrId, gstPtr.offset, offGst, cbGstPitch, cHeight, cbWidth, cbGmr),
                    VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    /* How many bytes are available for the data in the GMR. */
    uint32_t const cbGmrLeft = cbGstPitch > 0 ? cbGmr - offGmr : offGmr + cbWidth;

    /* How many scanlines would fit into the available data. */
    uint32_t cGmrScanlines = cbGmrLeft / cbGmrScanline;
    uint32_t const cbGmrLastScanline = cbGmrLeft - cGmrScanlines * cbGmrScanline; /* Slack space. */
    if (cbWidth <= cbGmrLastScanline)
        ++cGmrScanlines;

    if (cHeight > cGmrScanlines)
        cHeight = cGmrScanlines;

    AssertMsgReturn(cHeight > 0,
                    ("gmr=%#x:%#x offGst=%#x cbGstPitch=%#x cHeight=%#x cbWidth=%#x cbGmr=%#x\n",
                     gstPtr.gmrId, gstPtr.offset, offGst, cbGstPitch, cHeight, cbWidth, cbGmr),
                    VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    /*
     * Host buffer.
     */
    AssertMsgReturn(offHst < cbHstBuf,
                    ("buffer=%p size %d offHst=%d cbHstPitch=%d cHeight=%d cbWidth=%d\n",
                     pbHstBuf, cbHstBuf, offHst, cbHstPitch, cHeight, cbWidth),
                    VERR_INVALID_PARAMETER);

    /* Verify that cbWidth is less than scanline and fits into the buffer. */
    uint32_t const cbHstScanline = cbHstPitch > 0 ? cbHstPitch : -cbHstPitch;
    AssertMsgReturn(cbHstScanline != 0,
                    ("buffer=%p size %d offHst=%d cbHstPitch=%d cHeight=%d cbWidth=%d\n",
                     pbHstBuf, cbHstBuf, offHst, cbHstPitch, cHeight, cbWidth),
                    VERR_INVALID_PARAMETER);
    AssertMsgReturn(cbWidth <= cbHstScanline,
                    ("buffer=%p size %d offHst=%d cbHstPitch=%d cHeight=%d cbWidth=%d\n",
                     pbHstBuf, cbHstBuf, offHst, cbHstPitch, cHeight, cbWidth),
                    VERR_INVALID_PARAMETER);
    AssertMsgReturn(cbWidth <= cbHstBuf - offHst,
                    ("buffer=%p size %d offHst=%d cbHstPitch=%d cHeight=%d cbWidth=%d\n",
                     pbHstBuf, cbHstBuf, offHst, cbHstPitch, cHeight, cbWidth),
                    VERR_INVALID_PARAMETER);

    /* How many bytes are available for the data in the buffer. */
    uint32_t const cbHstLeft = cbHstPitch > 0 ? cbHstBuf - offHst : offHst + cbWidth;

    /* How many scanlines would fit into the available data. */
    uint32_t cHstScanlines = cbHstLeft / cbHstScanline;
    uint32_t const cbHstLastScanline = cbHstLeft - cHstScanlines * cbHstScanline; /* Slack space. */
    if (cbWidth <= cbHstLastScanline)
        ++cHstScanlines;

    if (cHeight > cHstScanlines)
        cHeight = cHstScanlines;

    AssertMsgReturn(cHeight > 0,
                    ("buffer=%p size %d offHst=%d cbHstPitch=%d cHeight=%d cbWidth=%d\n",
                     pbHstBuf, cbHstBuf, offHst, cbHstPitch, cHeight, cbWidth),
                    VERR_INVALID_PARAMETER);

    uint8_t *pbHst = pbHstBuf + offHst;

    /* Shortcut for the framebuffer. */
    if (gstPtr.gmrId == SVGA_GMR_FRAMEBUFFER)
    {
        uint8_t *pbGst = pThisCC->pbVRam + offGmr;

        uint8_t const *pbSrc;
        int32_t cbSrcPitch;
        uint8_t *pbDst;
        int32_t cbDstPitch;

        if (enmTransferType == SVGA3D_READ_HOST_VRAM)
        {
            pbSrc      = pbHst;
            cbSrcPitch = cbHstPitch;
            pbDst      = pbGst;
            cbDstPitch = cbGstPitch;
        }
        else
        {
            pbSrc      = pbGst;
            cbSrcPitch = cbGstPitch;
            pbDst      = pbHst;
            cbDstPitch = cbHstPitch;
        }

        if (   cbWidth == (uint32_t)cbGstPitch
            && cbGstPitch == cbHstPitch)
        {
            /* Entire scanlines, positive pitch. */
            memcpy(pbDst, pbSrc, cbWidth * cHeight);
        }
        else
        {
            for (uint32_t i = 0; i < cHeight; ++i)
            {
                memcpy(pbDst, pbSrc, cbWidth);

                pbDst += cbDstPitch;
                pbSrc += cbSrcPitch;
            }
        }
        return VINF_SUCCESS;
    }

    AssertPtrReturn(pGMR, VERR_INVALID_PARAMETER);
    AssertReturn(pGMR->numDescriptors > 0, VERR_INVALID_PARAMETER);

    PVMSVGAGMRDESCRIPTOR const paDesc = pGMR->paDesc; /* Local copy of the pointer. */
    uint32_t iDesc = 0;                               /* Index in the descriptor array. */
    uint32_t offDesc = 0;                             /* GMR offset of the current descriptor. */
    uint32_t offGmrScanline = offGmr;                 /* GMR offset of the scanline which is being copied. */
    uint8_t *pbHstScanline = pbHst;                   /* Host address of the scanline which is being copied. */
    for (uint32_t i = 0; i < cHeight; ++i)
    {
        uint32_t cbCurrentWidth = cbWidth;
        uint32_t offGmrCurrent  = offGmrScanline;
        uint8_t *pbCurrentHost  = pbHstScanline;

        /* Find the right descriptor */
        while (offDesc + paDesc[iDesc].numPages * PAGE_SIZE <= offGmrCurrent)
        {
            offDesc += paDesc[iDesc].numPages * PAGE_SIZE;
            AssertReturn(offDesc < pGMR->cbTotal, VERR_INTERNAL_ERROR); /* overflow protection */
            ++iDesc;
            AssertReturn(iDesc < pGMR->numDescriptors, VERR_INTERNAL_ERROR);
        }

        while (cbCurrentWidth)
        {
            uint32_t cbToCopy;

            if (offGmrCurrent + cbCurrentWidth <= offDesc + paDesc[iDesc].numPages * PAGE_SIZE)
            {
                cbToCopy = cbCurrentWidth;
            }
            else
            {
                cbToCopy = (offDesc + paDesc[iDesc].numPages * PAGE_SIZE - offGmrCurrent);
                AssertReturn(cbToCopy <= cbCurrentWidth, VERR_INVALID_PARAMETER);
            }

            RTGCPHYS const GCPhys = paDesc[iDesc].GCPhys + offGmrCurrent - offDesc;

            Log5Func(("%s phys=%RGp\n", (enmTransferType == SVGA3D_WRITE_HOST_VRAM) ? "READ" : "WRITE", GCPhys));

            if (enmTransferType == SVGA3D_WRITE_HOST_VRAM)
                rc = PDMDevHlpPCIPhysRead(pDevIns, GCPhys, pbCurrentHost, cbToCopy);
            else
                rc = PDMDevHlpPCIPhysWrite(pDevIns, GCPhys, pbCurrentHost, cbToCopy);
            AssertRCBreak(rc);

            cbCurrentWidth -= cbToCopy;
            offGmrCurrent  += cbToCopy;
            pbCurrentHost  += cbToCopy;

            /* Go to the next descriptor if there's anything left. */
            if (cbCurrentWidth)
            {
                offDesc += paDesc[iDesc].numPages * PAGE_SIZE;
                AssertReturn(offDesc < pGMR->cbTotal, VERR_INTERNAL_ERROR);
                ++iDesc;
                AssertReturn(iDesc < pGMR->numDescriptors, VERR_INTERNAL_ERROR);
            }
        }

        offGmrScanline += cbGstPitch;
        pbHstScanline  += cbHstPitch;
    }

    return VINF_SUCCESS;
}


/**
 * Unsigned coordinates in pBox. Clip to [0; pSizeSrc), [0; pSizeDest).
 *
 * @param   pSizeSrc    Source surface dimensions.
 * @param   pSizeDest   Destination surface dimensions.
 * @param   pBox        Coordinates to be clipped.
 */
void vmsvgaR3ClipCopyBox(const SVGA3dSize *pSizeSrc, const SVGA3dSize *pSizeDest, SVGA3dCopyBox *pBox)
{
    /* Src x, w */
    if (pBox->srcx > pSizeSrc->width)
        pBox->srcx = pSizeSrc->width;
    if (pBox->w > pSizeSrc->width - pBox->srcx)
        pBox->w = pSizeSrc->width - pBox->srcx;

    /* Src y, h */
    if (pBox->srcy > pSizeSrc->height)
        pBox->srcy = pSizeSrc->height;
    if (pBox->h > pSizeSrc->height - pBox->srcy)
        pBox->h = pSizeSrc->height - pBox->srcy;

    /* Src z, d */
    if (pBox->srcz > pSizeSrc->depth)
        pBox->srcz = pSizeSrc->depth;
    if (pBox->d > pSizeSrc->depth - pBox->srcz)
        pBox->d = pSizeSrc->depth - pBox->srcz;

    /* Dest x, w */
    if (pBox->x > pSizeDest->width)
        pBox->x = pSizeDest->width;
    if (pBox->w > pSizeDest->width - pBox->x)
        pBox->w = pSizeDest->width - pBox->x;

    /* Dest y, h */
    if (pBox->y > pSizeDest->height)
        pBox->y = pSizeDest->height;
    if (pBox->h > pSizeDest->height - pBox->y)
        pBox->h = pSizeDest->height - pBox->y;

    /* Dest z, d */
    if (pBox->z > pSizeDest->depth)
        pBox->z = pSizeDest->depth;
    if (pBox->d > pSizeDest->depth - pBox->z)
        pBox->d = pSizeDest->depth - pBox->z;
}


/**
 * Unsigned coordinates in pBox. Clip to [0; pSize).
 *
 * @param   pSize   Source surface dimensions.
 * @param   pBox    Coordinates to be clipped.
 */
void vmsvgaR3ClipBox(const SVGA3dSize *pSize, SVGA3dBox *pBox)
{
    /* x, w */
    if (pBox->x > pSize->width)
        pBox->x = pSize->width;
    if (pBox->w > pSize->width - pBox->x)
        pBox->w = pSize->width - pBox->x;

    /* y, h */
    if (pBox->y > pSize->height)
        pBox->y = pSize->height;
    if (pBox->h > pSize->height - pBox->y)
        pBox->h = pSize->height - pBox->y;

    /* z, d */
    if (pBox->z > pSize->depth)
        pBox->z = pSize->depth;
    if (pBox->d > pSize->depth - pBox->z)
        pBox->d = pSize->depth - pBox->z;
}


/**
 * Clip.
 *
 * @param   pBound  Bounding rectangle.
 * @param   pRect   Rectangle to be clipped.
 */
void vmsvgaR3ClipRect(SVGASignedRect const *pBound, SVGASignedRect *pRect)
{
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;

    /* Right order. */
    Assert(pBound->left <= pBound->right && pBound->top <= pBound->bottom);
    if (pRect->left < pRect->right)
    {
        left = pRect->left;
        right = pRect->right;
    }
    else
    {
        left = pRect->right;
        right = pRect->left;
    }
    if (pRect->top < pRect->bottom)
    {
        top = pRect->top;
        bottom = pRect->bottom;
    }
    else
    {
        top = pRect->bottom;
        bottom = pRect->top;
    }

    if (left < pBound->left)
        left = pBound->left;
    if (right < pBound->left)
        right = pBound->left;

    if (left > pBound->right)
        left = pBound->right;
    if (right > pBound->right)
        right = pBound->right;

    if (top < pBound->top)
        top = pBound->top;
    if (bottom < pBound->top)
        bottom = pBound->top;

    if (top > pBound->bottom)
        top = pBound->bottom;
    if (bottom > pBound->bottom)
        bottom = pBound->bottom;

    pRect->left = left;
    pRect->right = right;
    pRect->top = top;
    pRect->bottom = bottom;
}


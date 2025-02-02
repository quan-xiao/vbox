/* $Id: VBoxDispDrawCmd.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox XPDM Display driver
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
 */

#ifndef GA_INCLUDED_SRC_WINNT_Graphics_Video_disp_xpdm_VBoxDispDrawCmd_h
#define GA_INCLUDED_SRC_WINNT_Graphics_Video_disp_xpdm_VBoxDispDrawCmd_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#define VBVA_DECL_OP(__fn, __args) \
    void vbvaDrv##__fn __args;     \
    void vrdpDrv##__fn __args;

VBVA_DECL_OP(BitBlt, (                 \
    SURFOBJ  *psoTrg,                  \
    SURFOBJ  *psoSrc,                  \
    SURFOBJ  *psoMask,                 \
    CLIPOBJ  *pco,                     \
    XLATEOBJ *pxlo,                    \
    RECTL    *prclTrg,                 \
    POINTL   *pptlSrc,                 \
    POINTL   *pptlMask,                \
    BRUSHOBJ *pbo,                     \
    POINTL   *pptlBrush,               \
    ROP4      rop4                     \
    ));

VBVA_DECL_OP(TextOut, (                \
    SURFOBJ  *pso,                     \
    STROBJ   *pstro,                   \
    FONTOBJ  *pfo,                     \
    CLIPOBJ  *pco,                     \
    RECTL    *prclExtra,               \
    RECTL    *prclOpaque,              \
    BRUSHOBJ *pboFore,                 \
    BRUSHOBJ *pboOpaque,               \
    POINTL   *pptlOrg,                 \
    MIX       mix                      \
    ));

VBVA_DECL_OP(LineTo, (                 \
    SURFOBJ   *pso,                    \
    CLIPOBJ   *pco,                    \
    BRUSHOBJ  *pbo,                    \
    LONG       x1,                     \
    LONG       y1,                     \
    LONG       x2,                     \
    LONG       y2,                     \
    RECTL     *prclBounds,             \
    MIX        mix                     \
    ));

VBVA_DECL_OP(StretchBlt, (             \
    SURFOBJ         *psoDest,          \
    SURFOBJ         *psoSrc,           \
    SURFOBJ         *psoMask,          \
    CLIPOBJ         *pco,              \
    XLATEOBJ        *pxlo,             \
    COLORADJUSTMENT *pca,              \
    POINTL          *pptlHTOrg,        \
    RECTL           *prclDest,         \
    RECTL           *prclSrc,          \
    POINTL          *pptlMask,         \
    ULONG            iMode             \
    ));

VBVA_DECL_OP(CopyBits, (               \
    SURFOBJ  *psoDest,                 \
    SURFOBJ  *psoSrc,                  \
    CLIPOBJ  *pco,                     \
    XLATEOBJ *pxlo,                    \
    RECTL    *prclDest,                \
    POINTL   *pptlSrc                  \
    ));

VBVA_DECL_OP(Paint, (                  \
    SURFOBJ  *pso,                     \
    CLIPOBJ  *pco,                     \
    BRUSHOBJ *pbo,                     \
    POINTL   *pptlBrushOrg,            \
    MIX       mix                      \
    ));

VBVA_DECL_OP(FillPath, (               \
    SURFOBJ  *pso,                     \
    PATHOBJ  *ppo,                     \
    CLIPOBJ  *pco,                     \
    BRUSHOBJ *pbo,                     \
    POINTL   *pptlBrushOrg,            \
    MIX       mix,                     \
    FLONG     flOptions                \
    ));

VBVA_DECL_OP(StrokePath, (             \
    SURFOBJ   *pso,                    \
    PATHOBJ   *ppo,                    \
    CLIPOBJ   *pco,                    \
    XFORMOBJ  *pxo,                    \
    BRUSHOBJ  *pbo,                    \
    POINTL    *pptlBrushOrg,           \
    LINEATTRS *plineattrs,             \
    MIX        mix                     \
    ));

VBVA_DECL_OP(StrokeAndFillPath, (      \
    SURFOBJ   *pso,                    \
    PATHOBJ   *ppo,                    \
    CLIPOBJ   *pco,                    \
    XFORMOBJ  *pxo,                    \
    BRUSHOBJ  *pboStroke,              \
    LINEATTRS *plineattrs,             \
    BRUSHOBJ  *pboFill,                \
    POINTL    *pptlBrushOrg,           \
    MIX        mixFill,                \
    FLONG      flOptions               \
    ))

VBVA_DECL_OP(SaveScreenBits, (         \
    SURFOBJ  *pso,                     \
    ULONG    iMode,                    \
    ULONG_PTR ident,                   \
    RECTL    *prcl                     \
    ))

#undef VBVA_DECL_OP

#endif /* !GA_INCLUDED_SRC_WINNT_Graphics_Video_disp_xpdm_VBoxDispDrawCmd_h */

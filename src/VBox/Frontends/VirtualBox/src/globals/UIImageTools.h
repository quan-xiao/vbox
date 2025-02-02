/* $Id: UIImageTools.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - Declarations of utility classes and functions for image manipulation.
 */

/*
 * Copyright (C) 2010-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_globals_UIImageTools_h
#define FEQT_INCLUDED_SRC_globals_UIImageTools_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QImage>
#include <QPixmap>

/* GUI includes: */
#include "UILibraryDefs.h"

/** Image operation namespace. */
namespace UIImageTools
{
    /** Converts @a image to gray-scale. */
    SHARED_LIBRARY_STUFF QImage toGray(const QImage &image);

    /** Makes @a image more dark and dim. */
    SHARED_LIBRARY_STUFF void dimImage(QImage &image);

    /** Blurs passed @a source image to @a destination cropping by certain @a iRadius. */
    SHARED_LIBRARY_STUFF void blurImage(const QImage &source, QImage &destination, int iRadius);
    /** Blurs passed @a source image horizontally to @a destination cropping by certain @a iRadius. */
    SHARED_LIBRARY_STUFF void blurImageHorizontal(const QImage &source, QImage &destination, int iRadius);
    /** Blurs passed @a source image vertically to @a destination cropping by certain @a iRadius. */
    SHARED_LIBRARY_STUFF void blurImageVertical(const QImage &source, QImage &destination, int iRadius);

    /** Applies BET-label of passed @a size. */
    SHARED_LIBRARY_STUFF QPixmap betaLabel(const QSize &size = QSize(80, 16));
}
using namespace UIImageTools /* if header included */;

#endif /* !FEQT_INCLUDED_SRC_globals_UIImageTools_h */

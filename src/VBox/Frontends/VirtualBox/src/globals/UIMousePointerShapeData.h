/* $Id: UIMousePointerShapeData.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIMousePointerShapeData class declaration.
 */

/*
 * Copyright (C) 2019-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_globals_UIMousePointerShapeData_h
#define FEQT_INCLUDED_SRC_globals_UIMousePointerShapeData_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QMetaType>
#include <QPoint>
#include <QSize>
#include <QVector>

/* GUI includes: */
#include "UILibraryDefs.h"

/* Other VBox inlcudes: */
#include <VBox/com/defs.h>

/** Holds the mouse shape data to be able
  * to pass it through signal-slot mechanism. */
class SHARED_LIBRARY_STUFF UIMousePointerShapeData
{
public:

    /** Constructs mouse pointer shape data.
      * @param  fVisible   Brings whether mouse pointer should be visible.
      * @param  fAlpha     Brings whether mouse pointer chape has alpha channel.
      * @param  hotSpot    Brings the mouse pointer hot-spot.
      * @param  shapeSize  Brings the mouse pointer shape size.
      * @param  shape      Brings the mouse pointer shape byte array. */
    UIMousePointerShapeData(bool fVisible = false,
                            bool fAlpha = false,
                            const QPoint &hotSpot = QPoint(),
                            const QSize &shapeSize = QSize(),
                            const QVector<BYTE> &shape = QVector<BYTE>());

    /** Constructs mouse pointer shape data on the basis of another. */
    UIMousePointerShapeData(const UIMousePointerShapeData &another);

    /** Assigns this mouse pointer shape data with values of @a another. */
    UIMousePointerShapeData &operator=(const UIMousePointerShapeData &another);

    /** Returns whether mouse pointer should be visible. */
    bool isVisible() const { return m_fVisible; }
    /** Returns whether mouse pointer chape has alpha channel. */
    bool hasAlpha() const { return m_fAlpha; }
    /** Returns the mouse pointer hot-spot. */
    const QPoint &hotSpot() const { return m_hotSpot; }
    /** Returns the mouse pointer shape size. */
    const QSize &shapeSize() const { return m_shapeSize; }
    /** Returns the mouse pointer shape byte array. */
    const QVector<BYTE> &shape() const { return m_shape; }

private:

    /** Holds whether mouse pointer should be visible. */
    bool           m_fVisible;
    /** Holds whether mouse pointer chape has alpha channel. */
    bool           m_fAlpha;
    /** Holds the mouse pointer hot-spot. */
    QPoint         m_hotSpot;
    /** Holds the mouse pointer shape size. */
    QSize          m_shapeSize;
    /** Holds the mouse pointer shape byte array. */
    QVector<BYTE>  m_shape;
};
Q_DECLARE_METATYPE(UIMousePointerShapeData);

#endif /* !FEQT_INCLUDED_SRC_globals_UIMousePointerShapeData_h */

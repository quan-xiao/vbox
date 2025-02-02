/* $Id: QIGraphicsView.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - Qt extensions: QIGraphicsView class declaration.
 */

/*
 * Copyright (C) 2015-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_extensions_graphics_QIGraphicsView_h
#define FEQT_INCLUDED_SRC_extensions_graphics_QIGraphicsView_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QGraphicsView>

/* GUI includes: */
#include "UILibraryDefs.h"

/* Forward declarations: */
class QEvent;
class QWidget;

/** QGraphicsView extension with advanced functionality. */
class SHARED_LIBRARY_STUFF QIGraphicsView : public QGraphicsView
{
    Q_OBJECT;

public:

    /** Constructs graphics-view passing @a pParent to the base-class. */
    QIGraphicsView(QWidget *pParent = 0);

protected:

    /** Handles any Qt @a pEvent. */
    virtual bool event(QEvent *pEvent) /* override */;

private:

    /** Holds the vertical scroll-bar position. */
    int m_iVerticalScrollBarPosition;
};

#endif /* !FEQT_INCLUDED_SRC_extensions_graphics_QIGraphicsView_h */

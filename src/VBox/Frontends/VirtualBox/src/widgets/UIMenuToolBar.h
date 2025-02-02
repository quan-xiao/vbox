/* $Id: UIMenuToolBar.h 86233 2020-09-23 12:10:51Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIMenuToolBar class declaration.
 */

/*
 * Copyright (C) 2017-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_widgets_UIMenuToolBar_h
#define FEQT_INCLUDED_SRC_widgets_UIMenuToolBar_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QWidget>

/* Forward declarations: */
class UIMenuToolBarPrivate;


/** QWidget wrapper for QIToolBar extension
  * holding single drop-down menu of actions. */
class UIMenuToolBar : public QWidget
{
    Q_OBJECT;

public:

    /** Menu toolbar alignment types. */
    enum AlignmentType
    {
        AlignmentType_TopLeft,
        AlignmentType_TopRight,
        AlignmentType_BottomLeft,
        AlignmentType_BottomRight,
    };

    /** Constructs menu-toolbar wrapper. */
    UIMenuToolBar(QWidget *pParent = 0);

    /** Defines toolbar alignment @a enmType. */
    void setAlignmentType(AlignmentType enmType);

    /** Defines toolbar icon @a size. */
    void setIconSize(const QSize &size);

    /** Defines toolbar menu action. */
    void setMenuAction(QAction *pAction);

    /** Defines toolbar tool button @a enmStyle. */
    void setToolButtonStyle(Qt::ToolButtonStyle enmStyle);

    /** Returns toolbar widget for passed @a pAction. */
    QWidget *widgetForAction(QAction *pAction) const;

private:

    /** Prepares all. */
    void prepare();

    /** Holds the menu-toolbar instance. */
    UIMenuToolBarPrivate *m_pToolbar;
};

#endif /* !FEQT_INCLUDED_SRC_widgets_UIMenuToolBar_h */


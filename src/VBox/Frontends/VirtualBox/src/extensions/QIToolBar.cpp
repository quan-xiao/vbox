/* $Id: QIToolBar.cpp 86233 2020-09-23 12:10:51Z vboxsync $ */
/** @file
 * VBox Qt GUI - QIToolBar class implementation.
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Qt includes: */
#include <QLayout>
#include <QMainWindow>
#include <QResizeEvent>
#ifdef VBOX_WS_MAC
# include <QPainter>
#endif

/* GUI includes: */
#include "QIToolBar.h"
#ifdef VBOX_WS_MAC
# include "VBoxUtils.h"
#endif


QIToolBar::QIToolBar(QWidget *pParent /* = 0 */)
    : QToolBar(pParent)
    , m_pMainWindow(qobject_cast<QMainWindow*>(pParent))
#ifdef VBOX_WS_MAC
    , m_fEmulateUnifiedToolbar(false)
#endif
{
    /* Prepare: */
    prepare();
}

void QIToolBar::setUseTextLabels(bool fEnable)
{
    /* Determine tool-button style on the basis of passed flag: */
    Qt::ToolButtonStyle tbs = fEnable ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly;

    /* Depending on parent, assign this style: */
    if (m_pMainWindow)
        m_pMainWindow->setToolButtonStyle(tbs);
    else
        setToolButtonStyle(tbs);
}

#ifdef VBOX_WS_MAC
void QIToolBar::enableMacToolbar()
{
    /* Depending on parent, enable unified title/tool-bar: */
    if (m_pMainWindow)
        m_pMainWindow->setUnifiedTitleAndToolBarOnMac(true);
}

void QIToolBar::emulateMacToolbar()
{
    /* Remember request, to be used in paintEvent: */
    m_fEmulateUnifiedToolbar = true;
}

void QIToolBar::setShowToolBarButton(bool fShow)
{
    ::darwinSetShowsToolbarButton(this, fShow);
}

void QIToolBar::updateLayout()
{
    // WORKAROUND:
    // There is a bug in Qt Cocoa which result in showing a "more arrow" when
    // the necessary size of the tool-bar is increased. Also for some languages
    // the with doesn't match if the text increase. So manually adjust the size
    // after changing the text.
    QSizePolicy sp = sizePolicy();
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    adjustSize();
    setSizePolicy(sp);
    layout()->invalidate();
    layout()->activate();
}
#endif /* VBOX_WS_MAC */

void QIToolBar::resizeEvent(QResizeEvent *pEvent)
{
    /* Call to base-class: */
    QToolBar::resizeEvent(pEvent);

    /* Notify listeners about new size: */
    emit sigResized(pEvent->size());
}

#ifdef VBOX_WS_MAC
void QIToolBar::paintEvent(QPaintEvent *pEvent)
{
    /* Call to base-class: */
    QToolBar::paintEvent(pEvent);

    /* If we have request to emulate unified tool-bar: */
    if (m_fEmulateUnifiedToolbar)
    {
        /* Acquire rectangle: */
        const QRect rectangle = pEvent->rect();

        /* Prepare gradient: */
        const QColor backgroundColor = palette().color(QPalette::Active, QPalette::Mid);
        QLinearGradient gradient(rectangle.topLeft(), rectangle.bottomLeft());
        gradient.setColorAt(0,   backgroundColor.lighter(130));
        gradient.setColorAt(1,   backgroundColor.lighter(125));

        /* Fill background: */
        QPainter painter(this);
        painter.fillRect(rectangle, gradient);
    }
}
#endif /* VBOX_WS_MAC */

void QIToolBar::prepare()
{
    /* Configure tool-bar: */
    setFloatable(false);
    setMovable(false);

#ifdef VBOX_WS_MAC
    setStyleSheet("QToolBar { border: 0px none black; }");
#endif

    /* Configure tool-bar' layout: */
    if (layout())
        layout()->setContentsMargins(0, 0, 0, 0);

    /* Configure tool-bar' context-menu policy: */
    setContextMenuPolicy(Qt::NoContextMenu);
}

/* $Id: QIGraphicsView.cpp 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - Qt extensions: QIGraphicsView class implementation.
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

/* Qt includes: */
#include <QScrollBar>
#include <QTouchEvent>

/* GUI includes: */
#include "QIGraphicsView.h"

/* Other VBox includes: */
#include "iprt/assert.h"


QIGraphicsView::QIGraphicsView(QWidget *pParent /* = 0 */)
    : QGraphicsView(pParent)
    , m_iVerticalScrollBarPosition(0)
{
    /* Enable multi-touch support: */
    setAttribute(Qt::WA_AcceptTouchEvents);
    viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
}

bool QIGraphicsView::event(QEvent *pEvent)
{
    /* Handle known event types: */
    switch (pEvent->type())
    {
        case QEvent::TouchBegin:
        {
            /* Parse the touch event: */
            QTouchEvent *pTouchEvent = static_cast<QTouchEvent*>(pEvent);
            AssertPtrReturn(pTouchEvent, QGraphicsView::event(pEvent));
            /* For touch-screen event we have something special: */
            if (pTouchEvent->device()->type() == QTouchDevice::TouchScreen)
            {
                /* Remember where the scrolling was started: */
                m_iVerticalScrollBarPosition = verticalScrollBar()->value();
                /* Allow further touch events: */
                pEvent->accept();
                /* Mark event handled: */
                return true;
            }
            break;
        }
        case QEvent::TouchUpdate:
        {
            /* Parse the touch-event: */
            QTouchEvent *pTouchEvent = static_cast<QTouchEvent*>(pEvent);
            AssertPtrReturn(pTouchEvent, QGraphicsView::event(pEvent));
            /* For touch-screen event we have something special: */
            if (pTouchEvent->device()->type() == QTouchDevice::TouchScreen)
            {
                /* Determine vertical shift (inverted): */
                const QTouchEvent::TouchPoint point = pTouchEvent->touchPoints().first();
                const int iShift = (int)(point.startPos().y() - point.pos().y());
                /* Calculate new scroll-bar value according calculated shift: */
                int iNewScrollBarValue = m_iVerticalScrollBarPosition + iShift;
                /* Make sure new scroll-bar value is within the minimum/maximum bounds: */
                iNewScrollBarValue = qMax(verticalScrollBar()->minimum(), iNewScrollBarValue);
                iNewScrollBarValue = qMin(verticalScrollBar()->maximum(), iNewScrollBarValue);
                /* Apply calculated scroll-bar shift finally: */
                verticalScrollBar()->setValue(iNewScrollBarValue);
                /* Mark event handled: */
                return true;
            }
            break;
        }
        case QEvent::TouchEnd:
        {
            /* Parse the touch event: */
            QTouchEvent *pTouchEvent = static_cast<QTouchEvent*>(pEvent);
            AssertPtrReturn(pTouchEvent, QGraphicsView::event(pEvent));
            /* For touch-screen event we have something special: */
            if (pTouchEvent->device()->type() == QTouchDevice::TouchScreen)
            {
                /* Reset the scrolling start position: */
                m_iVerticalScrollBarPosition = 0;
                /* Mark event handled: */
                return true;
            }
            break;
        }
        default:
            break;
    }
    /* Call to base-class: */
    return QGraphicsView::event(pEvent);
}

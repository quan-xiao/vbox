/* $Id: UIDetailsView.cpp 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIDetailsView class implementation.
 */

/*
 * Copyright (C) 2012-2020 Oracle Corporation
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
#include <QAccessibleWidget>
#include <QScrollBar>

/* GUI includes: */
#include "UIDetails.h"
#include "UIDetailsModel.h"
#include "UIDetailsView.h"
#include "UIDetailsItem.h"

/* Other VBox includes: */
#include <iprt/assert.h>


/** QAccessibleWidget extension used as an accessibility interface for Details-view. */
class UIAccessibilityInterfaceForUIDetailsView : public QAccessibleWidget
{
public:

    /** Returns an accessibility interface for passed @a strClassname and @a pObject. */
    static QAccessibleInterface *pFactory(const QString &strClassname, QObject *pObject)
    {
        /* Creating Details-view accessibility interface: */
        if (pObject && strClassname == QLatin1String("UIDetailsView"))
            return new UIAccessibilityInterfaceForUIDetailsView(qobject_cast<QWidget*>(pObject));

        /* Null by default: */
        return 0;
    }

    /** Constructs an accessibility interface passing @a pWidget to the base-class. */
    UIAccessibilityInterfaceForUIDetailsView(QWidget *pWidget)
        : QAccessibleWidget(pWidget, QAccessible::List)
    {}

    /** Returns the number of children. */
    virtual int childCount() const /* override */
    {
        /* Make sure view still alive: */
        AssertPtrReturn(view(), 0);

        /* What amount of children root has? */
        const int cChildCount = view()->details()->model()->root()->items().size();

        /* Return amount of children root has (if there are many of children): */
        if (cChildCount > 1)
            return cChildCount;

        /* Return the number of children lone root child has (otherwise): */
        return view()->details()->model()->root()->items().first()->items().size();
    }

    /** Returns the child with the passed @a iIndex. */
    virtual QAccessibleInterface *child(int iIndex) const /* override */
    {
        /* Make sure view still alive: */
        AssertPtrReturn(view(), 0);
        /* Make sure index is valid: */
        AssertReturn(iIndex >= 0 && iIndex < childCount(), 0);

        /* What amount of children root has? */
        const int cChildCount = view()->details()->model()->root()->items().size();

        /* Return the root child with the passed iIndex (if there are many of children): */
        if (cChildCount > 1)
            return QAccessible::queryAccessibleInterface(view()->details()->model()->root()->items().at(iIndex));

        /* Return the lone root child's child with the passed iIndex (otherwise): */
        return QAccessible::queryAccessibleInterface(view()->details()->model()->root()->items().first()->items().at(iIndex));
    }

    /** Returns a text for the passed @a enmTextRole. */
    virtual QString text(QAccessible::Text enmTextRole) const /* override */
    {
        /* Make sure view still alive: */
        AssertPtrReturn(view(), QString());

        /* Return view tool-tip: */
        Q_UNUSED(enmTextRole);
        return view()->toolTip();
    }

private:

    /** Returns corresponding Details-view. */
    UIDetailsView *view() const { return qobject_cast<UIDetailsView*>(widget()); }
};


UIDetailsView::UIDetailsView(UIDetails *pParent)
    : QIWithRetranslateUI<QIGraphicsView>(pParent)
    , m_pDetails(pParent)
    , m_iMinimumWidthHint(0)
{
    prepare();
}

void UIDetailsView::sltMinimumWidthHintChanged(int iHint)
{
    /* Is there something changed? */
    if (m_iMinimumWidthHint == iHint)
        return;

    /* Remember new value: */
    m_iMinimumWidthHint = iHint;
    if (m_iMinimumWidthHint <= 0)
        m_iMinimumWidthHint = 1;

    /* Set minimum view width according passed width-hint: */
    setMinimumWidth(2 * frameWidth() + m_iMinimumWidthHint + verticalScrollBar()->sizeHint().width());

    /* Update scene-rect: */
    updateSceneRect();
}

void UIDetailsView::retranslateUi()
{
    /* Translate this: */
#if 0 /* we will leave that for accessibility needs. */
    setToolTip(tr("Contains a list of Virtual Machine details"));
#endif  /* to be integrated to accessibility interface. */
}

void UIDetailsView::resizeEvent(QResizeEvent *pEvent)
{
    /* Call to base-class: */
    QIWithRetranslateUI<QIGraphicsView>::resizeEvent(pEvent);
    /* Notify listeners: */
    emit sigResized();

    /* Update scene-rect: */
    updateSceneRect();
}

void UIDetailsView::prepare()
{
    /* Install Details-view accessibility interface factory: */
    QAccessible::installFactory(UIAccessibilityInterfaceForUIDetailsView::pFactory);

    /* Setup frame: */
    setFrameShape(QFrame::NoFrame);
    setFrameShadow(QFrame::Plain);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);

    /* Setup scroll-bars policy: */
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    /* Update scene-rect: */
    updateSceneRect();

    /* Translate finally: */
    retranslateUi();
}

void UIDetailsView::updateSceneRect()
{
    setSceneRect(0, 0, m_iMinimumWidthHint, height());
}

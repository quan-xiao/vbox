/* $Id: QIStatusBar.cpp 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - Qt extensions: QIStatusBar class implementation.
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
#include <QAccessibleWidget>

/* GUI includes: */
#include "QIStatusBar.h"


/** QAccessibleWidget extension used as an accessibility interface for QIStatusBar. */
class QIAccessibilityInterfaceForQIStatusBar : public QAccessibleWidget
{
public:

    /** Returns an accessibility interface for passed @a strClassname and @a pObject. */
    static QAccessibleInterface *pFactory(const QString &strClassname, QObject *pObject)
    {
        /* Creating QIStatusBar accessibility interface: */
        if (pObject && strClassname == QLatin1String("QIStatusBar"))
            return new QIAccessibilityInterfaceForQIStatusBar(qobject_cast<QWidget*>(pObject));

        /* Null by default: */
        return 0;
    }

    /** Constructs an accessibility interface passing @a pWidget to the base-class. */
    QIAccessibilityInterfaceForQIStatusBar(QWidget *pWidget)
        : QAccessibleWidget(pWidget, QAccessible::ToolBar)
    {
        // We are not interested in status-bar text as it's a mean of
        // accessibility in case when accessibility is disabled.
        // Since accessibility is enabled in our case, we wish
        // to pass control token to our sub-elements.
        // So we are using QAccessible::ToolBar.
    }
};


/*********************************************************************************************************************************
*   Class QIStatusBar implementation.                                                                                            *
*********************************************************************************************************************************/

QIStatusBar::QIStatusBar(QWidget *pParent)
    : QStatusBar(pParent)
{
    /* Install QIStatusBar accessibility interface factory: */
    QAccessible::installFactory(QIAccessibilityInterfaceForQIStatusBar::pFactory);

    /* Make sure we remember the last one status message: */
    connect(this, &QIStatusBar::messageChanged,
            this, &QIStatusBar::sltRememberLastMessage);

    /* Remove that ugly border around the status-bar items on every platform: */
    setStyleSheet("QStatusBar::item { border: 0px none black; }");
}

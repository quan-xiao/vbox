/* $Id: UIMultiScreenLayout.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIMultiScreenLayout class declaration.
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

#ifndef FEQT_INCLUDED_SRC_runtime_UIMultiScreenLayout_h
#define FEQT_INCLUDED_SRC_runtime_UIMultiScreenLayout_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QObject>
#include <QMap>

/* Forward declarations: */
class UIMachineLogic;
class QMenu;
class QAction;

/* Multi-screen layout manager: */
class UIMultiScreenLayout : public QObject
{
    Q_OBJECT;

signals:

    /** Notifies about layout change. */
    void sigScreenLayoutChange();

public:

    /* Constructor/destructor: */
    UIMultiScreenLayout(UIMachineLogic *pMachineLogic);

    /* API: Update stuff: */
    void update();
    void rebuild();

    /* API: Getters: */
    int hostScreenCount() const;
    int guestScreenCount() const;
    int hostScreenForGuestScreen(int iScreenId) const;
    bool hasHostScreenForGuestScreen(int iScreenId) const;
    quint64 memoryRequirements() const;

private slots:

    /* Handler: Screen change stuff: */
    void sltHandleScreenLayoutChange(int iRequestedGuestScreen, int iRequestedHostScreen);

private:

    /* Helpers: Prepare stuff: */
    void calculateHostMonitorCount();
    void calculateGuestScreenCount();
    void prepareConnections();

    /* Other helpers: */
    void saveScreenMapping();
    quint64 memoryRequirements(const QMap<int, int> &screenLayout) const;

    /* Variables: */
    UIMachineLogic *m_pMachineLogic;
    QList<int> m_guestScreens;
    QList<int> m_disabledGuestScreens;
    const uint m_cGuestScreens;
    int m_cHostScreens;
    QMap<int, int> m_screenMap;
    QList<QMenu*> m_screenMenuList;
};

#endif /* !FEQT_INCLUDED_SRC_runtime_UIMultiScreenLayout_h */


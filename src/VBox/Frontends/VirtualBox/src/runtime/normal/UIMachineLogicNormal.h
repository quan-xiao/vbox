/* $Id: UIMachineLogicNormal.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIMachineLogicNormal class declaration.
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

#ifndef FEQT_INCLUDED_SRC_runtime_normal_UIMachineLogicNormal_h
#define FEQT_INCLUDED_SRC_runtime_normal_UIMachineLogicNormal_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Local includes: */
#include "UIMachineLogic.h"

/* Normal machine logic implementation: */
class UIMachineLogicNormal : public UIMachineLogic
{
    Q_OBJECT;

protected:

    /* Constructor: */
    UIMachineLogicNormal(QObject *pParent, UISession *pSession);

    /* Check if this logic is available: */
    bool checkAvailability();

    /** Returns machine-window flags for 'Normal' machine-logic and passed @a uScreenId. */
    virtual Qt::WindowFlags windowFlags(ulong uScreenId) const { Q_UNUSED(uScreenId); return Qt::Window; }

private slots:

    /** Checks if some visual-state type was requested. */
    void sltCheckForRequestedVisualStateType();

#ifndef RT_OS_DARWIN
    /** Invokes popup-menu. */
    void sltInvokePopupMenu();
#endif /* RT_OS_DARWIN */

    /** Opens menu-bar editor.*/
    void sltOpenMenuBarSettings();
    /** Handles menu-bar editor closing.*/
    void sltMenuBarSettingsClosed();
#ifndef RT_OS_DARWIN
    /** Toggles menu-bar presence.*/
    void sltToggleMenuBar();
#endif /* !RT_OS_DARWIN */

    /** Opens status-bar editor.*/
    void sltOpenStatusBarSettings();
    /** Handles status-bar editor closing.*/
    void sltStatusBarSettingsClosed();
    /** Toggles status-bar presence.*/
    void sltToggleStatusBar();

    /** Handles guest-screen toggle requests. */
    void sltHandleActionTriggerViewScreenToggle(int iIndex, bool fEnabled);
    /** Handles guest-screen resize requests. */
    void sltHandleActionTriggerViewScreenResize(int iIndex, const QSize &size);

    /** Handles host-screen available-area change. */
    virtual void sltHostScreenAvailableAreaChange() /* override */;

private:

    /* Prepare helpers: */
    virtual void prepareActionGroups() /* override */;
    void prepareActionConnections();
    void prepareMachineWindows();
#ifndef VBOX_WS_MAC
    void prepareMenu();
#endif /* !VBOX_WS_MAC */

    /* Cleanup helpers: */
#ifndef VBOX_WS_MAC
    void cleanupMenu();
#endif /* !VBOX_WS_MAC */
    void cleanupMachineWindows();
    void cleanupActionConnections();

#ifndef VBOX_WS_MAC
    /** Holds the popup-menu instance. */
    QMenu *m_pPopupMenu;
#endif /* !VBOX_WS_MAC */

    /* Friend classes: */
    friend class UIMachineLogic;
};

#endif /* !FEQT_INCLUDED_SRC_runtime_normal_UIMachineLogicNormal_h */

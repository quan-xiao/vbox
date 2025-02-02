/* $Id: UIMachineWindowNormal.h 86061 2020-09-08 10:25:36Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIMachineWindowNormal class declaration.
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

#ifndef FEQT_INCLUDED_SRC_runtime_normal_UIMachineWindowNormal_h
#define FEQT_INCLUDED_SRC_runtime_normal_UIMachineWindowNormal_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIMachineWindow.h"

/* Forward declarations: */
class CMediumAttachment;
class UIIndicatorsPool;
class UIAction;

/** UIMachineWindow reimplementation,
  * providing GUI with machine-window for the normal mode. */
class UIMachineWindowNormal : public UIMachineWindow
{
    Q_OBJECT;

signals:

    /** Notifies about geometry change. */
    void sigGeometryChange(const QRect &rect);

protected:

    /** Constructor, passes @a pMachineLogic and @a uScreenId to the UIMachineWindow constructor. */
    UIMachineWindowNormal(UIMachineLogic *pMachineLogic, ulong uScreenId);

private slots:

    /** Handles machine state change event. */
    void sltMachineStateChanged();
    /** Handles medium change event. */
    void sltMediumChange(const CMediumAttachment &attachment);
    /** Handles USB controller change event. */
    void sltUSBControllerChange();
    /** Handles USB device state change event. */
    void sltUSBDeviceStateChange();
    /** Handles audio adapter change event. */
    void sltAudioAdapterChange();
    /** Handles network adapter change event. */
    void sltNetworkAdapterChange();
    /** Handles shared folder change event. */
    void sltSharedFolderChange();
    /** Handles recording change event. */
    void sltRecordingChange();
    /** Handles CPU execution cap change event. */
    void sltCPUExecutionCapChange();
    /** Handles UISession initialized event. */
    void sltHandleSessionInitialized();

#ifndef RT_OS_DARWIN
    /** Handles menu-bar configuration-change. */
    void sltHandleMenuBarConfigurationChange(const QUuid &uMachineID);
    /** Handles menu-bar context-menu-request. */
    void sltHandleMenuBarContextMenuRequest(const QPoint &position);
#endif /* !RT_OS_DARWIN */

    /** Handles status-bar configuration-change. */
    void sltHandleStatusBarConfigurationChange(const QUuid &uMachineID);
    /** Handles status-bar context-menu-request. */
    void sltHandleStatusBarContextMenuRequest(const QPoint &position);
    /** Handles status-bar indicator context-menu-request. */
    void sltHandleIndicatorContextMenuRequest(IndicatorType enmIndicatorType, const QPoint &indicatorPosition);

#ifdef VBOX_WS_MAC
    /** Handles signal about some @a pAction hovered. */
    void sltActionHovered(UIAction *pAction);
#endif /* VBOX_WS_MAC */

private:

    /** Prepare session connections routine. */
    void prepareSessionConnections();
#ifndef VBOX_WS_MAC
    /** Prepare menu routine. */
    void prepareMenu();
#endif /* !VBOX_WS_MAC */
    /** Prepare status-bar routine. */
    void prepareStatusBar();
    /** Prepare visual-state routine. */
    void prepareVisualState();
    /** Load settings routine. */
    void loadSettings();

    /** Save settings routine. */
    void saveSettings();
    /** Cleanup visual-state routine. */
    void cleanupVisualState();
    /** Cleanup session connections routine. */
    void cleanupSessionConnections();

    /** Updates visibility according to visual-state. */
    void showInNecessaryMode();

    /** Restores cached window geometry. */
    virtual void restoreCachedGeometry() /* override */;

    /** Performs window geometry normalization according to guest-size and host's available geometry.
      * @param  fAdjustPosition        Determines whether is it necessary to adjust position as well.
      * @param  fResizeToGuestDisplay  Determines whether is it necessary to resize the window to fit to guest display size. */
    virtual void normalizeGeometry(bool fAdjustPosition, bool fResizeToGuestDisplay) /* override */;

    /** Common update routine. */
    void updateAppearanceOf(int aElement);

#ifndef VBOX_WS_MAC
    /** Updates menu-bar content. */
    void updateMenu();
#endif /* !VBOX_WS_MAC */

    /** Common @a pEvent handler. */
    bool event(QEvent *pEvent);

    /** Returns whether this window is maximized. */
    bool isMaximizedChecked();

    /** Holds the indicator-pool instance. */
    UIIndicatorsPool *m_pIndicatorsPool;

    /** Holds the current window geometry. */
    QRect m_normalGeometry;

    /** Factory support. */
    friend class UIMachineWindow;
};

#endif /* !FEQT_INCLUDED_SRC_runtime_normal_UIMachineWindowNormal_h */

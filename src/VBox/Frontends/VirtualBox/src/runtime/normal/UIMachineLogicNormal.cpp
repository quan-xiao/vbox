/* $Id: UIMachineLogicNormal.cpp 84698 2020-06-05 15:18:19Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIMachineLogicNormal class implementation.
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

/* Qt includes: */
#ifndef VBOX_WS_MAC
# include <QTimer>
#endif /* !VBOX_WS_MAC */

/* GUI includes: */
#include "UICommon.h"
#include "UIDesktopWidgetWatchdog.h"
#include "UIMessageCenter.h"
#include "UISession.h"
#include "UIActionPoolRuntime.h"
#include "UIMachineLogicNormal.h"
#include "UIMachineWindow.h"
#include "UIMenuBarEditorWindow.h"
#include "UIStatusBarEditorWindow.h"
#include "UIExtraDataManager.h"
#include "UIFrameBuffer.h"
#ifndef VBOX_WS_MAC
# include "QIMenu.h"
#else  /* VBOX_WS_MAC */
# include "VBoxUtils.h"
#endif /* VBOX_WS_MAC */

/* COM includes: */
#include "CConsole.h"
#include "CDisplay.h"
#include "CGraphicsAdapter.h"


UIMachineLogicNormal::UIMachineLogicNormal(QObject *pParent, UISession *pSession)
    : UIMachineLogic(pParent, pSession, UIVisualStateType_Normal)
#ifndef VBOX_WS_MAC
    , m_pPopupMenu(0)
#endif /* !VBOX_WS_MAC */
{
}

bool UIMachineLogicNormal::checkAvailability()
{
    /* Normal mode is always available: */
    return true;
}

void UIMachineLogicNormal::sltCheckForRequestedVisualStateType()
{
    LogRel(("GUI: UIMachineLogicNormal::sltCheckForRequestedVisualStateType: Requested-state=%d, Machine-state=%d\n",
            uisession()->requestedVisualState(), uisession()->machineState()));

    /* Do not try to change visual-state type if machine was not started yet: */
    if (!uisession()->isRunning() && !uisession()->isPaused())
        return;

    /* Do not try to change visual-state type in 'manual override' mode: */
    if (isManualOverrideMode())
        return;

    /* Check requested visual-state types: */
    switch (uisession()->requestedVisualState())
    {
        /* If 'seamless' visual-state type is requested: */
        case UIVisualStateType_Seamless:
        {
            /* And supported: */
            if (uisession()->isGuestSupportsSeamless())
            {
                LogRel(("GUI: UIMachineLogicNormal::sltCheckForRequestedVisualStateType: "
                        "Going 'seamless' as requested...\n"));
                uisession()->setRequestedVisualState(UIVisualStateType_Invalid);
                uisession()->changeVisualState(UIVisualStateType_Seamless);
            }
            else
                LogRel(("GUI: UIMachineLogicNormal::sltCheckForRequestedVisualStateType: "
                        "Rejecting 'seamless' as is it not yet supported...\n"));
            break;
        }
        default:
            break;
    }
}

#ifndef RT_OS_DARWIN
void UIMachineLogicNormal::sltInvokePopupMenu()
{
    /* Popup main-menu if present: */
    if (m_pPopupMenu && !m_pPopupMenu->isEmpty())
    {
        m_pPopupMenu->popup(activeMachineWindow()->geometry().center());
        QTimer::singleShot(0, m_pPopupMenu, SLOT(sltHighlightFirstAction()));
    }
}
#endif /* RT_OS_DARWIN */

void UIMachineLogicNormal::sltOpenMenuBarSettings()
{
    /* Do not process if window(s) missed! */
    AssertReturnVoid(isMachineWindowsCreated());

#ifndef VBOX_WS_MAC
    /* Make sure menu-bar is enabled: */
    const bool fEnabled = actionPool()->action(UIActionIndexRT_M_View_M_MenuBar_T_Visibility)->isChecked();
    AssertReturnVoid(fEnabled);
#endif /* !VBOX_WS_MAC */

    /* Prevent user from opening another one editor or toggle menu-bar: */
    actionPool()->action(UIActionIndexRT_M_View_M_MenuBar_S_Settings)->setEnabled(false);
#ifndef VBOX_WS_MAC
    actionPool()->action(UIActionIndexRT_M_View_M_MenuBar_T_Visibility)->setEnabled(false);
#endif /* !VBOX_WS_MAC */
    /* Create menu-bar editor: */
    UIMenuBarEditorWindow *pMenuBarEditor = new UIMenuBarEditorWindow(activeMachineWindow(), actionPool());
    AssertPtrReturnVoid(pMenuBarEditor);
    {
        /* Configure menu-bar editor: */
        connect(pMenuBarEditor, &UIMenuBarEditorWindow::destroyed,
                this, &UIMachineLogicNormal::sltMenuBarSettingsClosed);
#ifdef VBOX_WS_MAC
        connect(this, &UIMachineLogicNormal::sigNotifyAbout3DOverlayVisibilityChange,
                pMenuBarEditor, &UIMenuBarEditorWindow::sltActivateWindow);
#endif /* VBOX_WS_MAC */
        /* Show window: */
        pMenuBarEditor->show();
    }
}

void UIMachineLogicNormal::sltMenuBarSettingsClosed()
{
#ifndef VBOX_WS_MAC
    /* Make sure menu-bar is enabled: */
    const bool fEnabled = actionPool()->action(UIActionIndexRT_M_View_M_MenuBar_T_Visibility)->isChecked();
    AssertReturnVoid(fEnabled);
#endif /* !VBOX_WS_MAC */

    /* Allow user to open editor and toggle menu-bar again: */
    actionPool()->action(UIActionIndexRT_M_View_M_MenuBar_S_Settings)->setEnabled(true);
#ifndef VBOX_WS_MAC
    actionPool()->action(UIActionIndexRT_M_View_M_MenuBar_T_Visibility)->setEnabled(true);
#endif /* !VBOX_WS_MAC */
}

#ifndef RT_OS_DARWIN
void UIMachineLogicNormal::sltToggleMenuBar()
{
    /* Do not process if window(s) missed! */
    AssertReturnVoid(isMachineWindowsCreated());

    /* Invert menu-bar availability option: */
    const bool fEnabled = gEDataManager->menuBarEnabled(uiCommon().managedVMUuid());
    gEDataManager->setMenuBarEnabled(!fEnabled, uiCommon().managedVMUuid());
}
#endif /* !RT_OS_DARWIN */

void UIMachineLogicNormal::sltOpenStatusBarSettings()
{
    /* Do not process if window(s) missed! */
    AssertReturnVoid(isMachineWindowsCreated());

    /* Make sure status-bar is enabled: */
    const bool fEnabled = actionPool()->action(UIActionIndexRT_M_View_M_StatusBar_T_Visibility)->isChecked();
    AssertReturnVoid(fEnabled);

    /* Prevent user from opening another one editor or toggle status-bar: */
    actionPool()->action(UIActionIndexRT_M_View_M_StatusBar_S_Settings)->setEnabled(false);
    actionPool()->action(UIActionIndexRT_M_View_M_StatusBar_T_Visibility)->setEnabled(false);
    /* Create status-bar editor: */
    UIStatusBarEditorWindow *pStatusBarEditor = new UIStatusBarEditorWindow(activeMachineWindow());
    AssertPtrReturnVoid(pStatusBarEditor);
    {
        /* Configure status-bar editor: */
        connect(pStatusBarEditor, &UIStatusBarEditorWindow::destroyed,
                this, &UIMachineLogicNormal::sltStatusBarSettingsClosed);
#ifdef VBOX_WS_MAC
        connect(this, &UIMachineLogicNormal::sigNotifyAbout3DOverlayVisibilityChange,
                pStatusBarEditor, &UIStatusBarEditorWindow::sltActivateWindow);
#endif /* VBOX_WS_MAC */
        /* Show window: */
        pStatusBarEditor->show();
    }
}

void UIMachineLogicNormal::sltStatusBarSettingsClosed()
{
    /* Make sure status-bar is enabled: */
    const bool fEnabled = actionPool()->action(UIActionIndexRT_M_View_M_StatusBar_T_Visibility)->isChecked();
    AssertReturnVoid(fEnabled);

    /* Allow user to open editor and toggle status-bar again: */
    actionPool()->action(UIActionIndexRT_M_View_M_StatusBar_S_Settings)->setEnabled(true);
    actionPool()->action(UIActionIndexRT_M_View_M_StatusBar_T_Visibility)->setEnabled(true);
}

void UIMachineLogicNormal::sltToggleStatusBar()
{
    /* Do not process if window(s) missed! */
    AssertReturnVoid(isMachineWindowsCreated());

    /* Invert status-bar availability option: */
    const bool fEnabled = gEDataManager->statusBarEnabled(uiCommon().managedVMUuid());
    gEDataManager->setStatusBarEnabled(!fEnabled, uiCommon().managedVMUuid());
}

void UIMachineLogicNormal::sltHandleActionTriggerViewScreenToggle(int iIndex, bool fEnabled)
{
    /* Enable/disable guest keeping current size: */
    ULONG uWidth, uHeight, uBitsPerPixel;
    LONG uOriginX, uOriginY;
    KGuestMonitorStatus monitorStatus = KGuestMonitorStatus_Enabled;
    display().GetScreenResolution(iIndex, uWidth, uHeight, uBitsPerPixel, uOriginX, uOriginY, monitorStatus);
    if (!fEnabled)
    {
        uisession()->setScreenVisibleHostDesires(iIndex, false);
        display().SetVideoModeHint(iIndex, false, false, 0, 0, 0, 0, 0, true);
    }
    else
    {
        /* Defaults: */
        if (!uWidth)
            uWidth = 800;
        if (!uHeight)
            uHeight = 600;
        uisession()->setScreenVisibleHostDesires(iIndex, true);
        display().SetVideoModeHint(iIndex, true, false, 0, 0, uWidth, uHeight, 32, true);
    }
}

void UIMachineLogicNormal::sltHandleActionTriggerViewScreenResize(int iIndex, const QSize &size)
{
    /* Resize guest to required size: */
    display().SetVideoModeHint(iIndex, uisession()->isScreenVisible(iIndex),
                             false, 0, 0, size.width(), size.height(), 0, true);
}

void UIMachineLogicNormal::sltHostScreenAvailableAreaChange()
{
#if defined(VBOX_WS_X11) && !defined(VBOX_GUI_WITH_CUSTOMIZATIONS1)
    /* Prevent handling if fake screen detected: */
    if (gpDesktop->isFakeScreenDetected())
        return;

    /* Make sure all machine-window(s) have previous but normalized geometry: */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        if (!pMachineWindow->isMaximized())
            pMachineWindow->restoreCachedGeometry();
#endif /* VBOX_WS_X11 && !VBOX_GUI_WITH_CUSTOMIZATIONS1 */

    /* Call to base-class: */
    UIMachineLogic::sltHostScreenAvailableAreaChange();
}

void UIMachineLogicNormal::prepareActionGroups()
{
    /* Call to base-class: */
    UIMachineLogic::prepareActionGroups();

    /* Restrict 'Remap' actions for 'View' menu: */
    actionPool()->toRuntime()->setRestrictionForMenuView(UIActionRestrictionLevel_Logic,
                                                         (UIExtraDataMetaDefs::RuntimeMenuViewActionType)
                                                         (UIExtraDataMetaDefs::RuntimeMenuViewActionType_Remap));
}

void UIMachineLogicNormal::prepareActionConnections()
{
    /* Call to base-class: */
    UIMachineLogic::prepareActionConnections();

    /* Prepare 'View' actions connections: */
    connect(actionPool()->action(UIActionIndexRT_M_View_T_Fullscreen), &UIAction::triggered,
            this, &UIMachineLogicNormal::sltChangeVisualStateToFullscreen);
    connect(actionPool()->action(UIActionIndexRT_M_View_T_Seamless), &UIAction::triggered,
            this, &UIMachineLogicNormal::sltChangeVisualStateToSeamless);
    connect(actionPool()->action(UIActionIndexRT_M_View_T_Scale), &UIAction::triggered,
            this, &UIMachineLogicNormal::sltChangeVisualStateToScale);
    connect(actionPool()->action(UIActionIndexRT_M_View_M_MenuBar_S_Settings), &UIAction::triggered,
            this, &UIMachineLogicNormal::sltOpenMenuBarSettings);
#ifndef VBOX_WS_MAC
    connect(actionPool()->action(UIActionIndexRT_M_View_M_MenuBar_T_Visibility), &UIAction::triggered,
            this, &UIMachineLogicNormal::sltToggleMenuBar);
#endif /* !VBOX_WS_MAC */
    connect(actionPool()->action(UIActionIndexRT_M_View_M_StatusBar_S_Settings), &UIAction::triggered,
            this, &UIMachineLogicNormal::sltOpenStatusBarSettings);
    connect(actionPool()->action(UIActionIndexRT_M_View_M_StatusBar_T_Visibility), &UIAction::triggered,
            this, &UIMachineLogicNormal::sltToggleStatusBar);
    UIActionPoolRuntime* pActionPoolRuntime = qobject_cast<UIActionPoolRuntime*>(actionPool());
    AssertPtrReturnVoid(pActionPoolRuntime);
    {
        connect(pActionPoolRuntime, &UIActionPoolRuntime::sigNotifyAboutTriggeringViewScreenToggle,
                this, &UIMachineLogicNormal::sltHandleActionTriggerViewScreenToggle);
        connect(pActionPoolRuntime, &UIActionPoolRuntime::sigNotifyAboutTriggeringViewScreenResize,
                this, &UIMachineLogicNormal::sltHandleActionTriggerViewScreenResize);
    }
}

void UIMachineLogicNormal::prepareMachineWindows()
{
    /* Do not create machine-window(s) if they created already: */
    if (isMachineWindowsCreated())
        return;

#ifdef VBOX_WS_MAC /// @todo Is that really need here?
    /* We have to make sure that we are getting the front most process.
     * This is necessary for Qt versions > 4.3.3: */
    ::darwinSetFrontMostProcess();
#endif /* VBOX_WS_MAC */

    /* Get monitors count: */
    ulong uMonitorCount = machine().GetGraphicsAdapter().GetMonitorCount();
    /* Create machine window(s): */
    for (ulong uScreenId = 0; uScreenId < uMonitorCount; ++ uScreenId)
        addMachineWindow(UIMachineWindow::create(this, uScreenId));
    /* Order machine window(s): */
    for (ulong uScreenId = uMonitorCount; uScreenId > 0; -- uScreenId)
        machineWindows()[uScreenId - 1]->raise();

    /* Listen for frame-buffer resize: */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        connect(pMachineWindow, &UIMachineWindow::sigFrameBufferResize,
                this, &UIMachineLogicNormal::sigFrameBufferResize);
    emit sigFrameBufferResize();

    /* Mark machine-window(s) created: */
    setMachineWindowsCreated(true);
}

#ifndef VBOX_WS_MAC
void UIMachineLogicNormal::prepareMenu()
{
    /* Prepare popup-menu: */
    m_pPopupMenu = new QIMenu;
    AssertPtrReturnVoid(m_pPopupMenu);
    {
        /* Prepare popup-menu: */
        foreach (QMenu *pMenu, actionPool()->menus())
            m_pPopupMenu->addMenu(pMenu);
    }
}
#endif /* !VBOX_WS_MAC */

#ifndef VBOX_WS_MAC
void UIMachineLogicNormal::cleanupMenu()
{
    /* Cleanup popup-menu: */
    delete m_pPopupMenu;
    m_pPopupMenu = 0;
}
#endif /* !VBOX_WS_MAC */

void UIMachineLogicNormal::cleanupMachineWindows()
{
    /* Do not destroy machine-window(s) if they destroyed already: */
    if (!isMachineWindowsCreated())
        return;

    /* Mark machine-window(s) destroyed: */
    setMachineWindowsCreated(false);

    /* Cleanup machine-window(s): */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        UIMachineWindow::destroy(pMachineWindow);
}

void UIMachineLogicNormal::cleanupActionConnections()
{
    /* "View" actions disconnections: */
    disconnect(actionPool()->action(UIActionIndexRT_M_View_T_Fullscreen), &UIAction::triggered,
               this, &UIMachineLogicNormal::sltChangeVisualStateToFullscreen);
    disconnect(actionPool()->action(UIActionIndexRT_M_View_T_Seamless), &UIAction::triggered,
               this, &UIMachineLogicNormal::sltChangeVisualStateToSeamless);
    disconnect(actionPool()->action(UIActionIndexRT_M_View_T_Scale), &UIAction::triggered,
               this, &UIMachineLogicNormal::sltChangeVisualStateToScale);
    disconnect(actionPool()->action(UIActionIndexRT_M_View_M_MenuBar_S_Settings), &UIAction::triggered,
               this, &UIMachineLogicNormal::sltOpenMenuBarSettings);
#ifndef VBOX_WS_MAC
    disconnect(actionPool()->action(UIActionIndexRT_M_View_M_MenuBar_T_Visibility), &UIAction::triggered,
               this, &UIMachineLogicNormal::sltToggleMenuBar);
#endif /* !VBOX_WS_MAC */
    disconnect(actionPool()->action(UIActionIndexRT_M_View_M_StatusBar_S_Settings), &UIAction::triggered,
               this, &UIMachineLogicNormal::sltOpenStatusBarSettings);
    disconnect(actionPool()->action(UIActionIndexRT_M_View_M_StatusBar_T_Visibility), &UIAction::triggered,
               this, &UIMachineLogicNormal::sltToggleStatusBar);

    /* Call to base-class: */
    UIMachineLogic::cleanupActionConnections();
}

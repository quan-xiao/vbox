/* $Id: UIMachineSettingsInterface.cpp 86045 2020-09-07 14:58:04Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsInterface class implementation.
 */

/*
 * Copyright (C) 2008-2020 Oracle Corporation
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
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>

/* GUI includes: */
#include "UIActionPool.h"
#include "UIExtraDataManager.h"
#include "UIMachineSettingsInterface.h"
#include "UIStatusBarEditorWindow.h"
#include "UIMenuBarEditorWindow.h"
#include "UIVisualStateEditor.h"

/** Machine settings: User Interface page data structure. */
struct UIDataSettingsMachineInterface
{
    /** Constructs data. */
    UIDataSettingsMachineInterface()
        : m_fStatusBarEnabled(false)
#ifndef VBOX_WS_MAC
        , m_fMenuBarEnabled(false)
#endif /* !VBOX_WS_MAC */
        , m_restrictionsOfMenuBar(UIExtraDataMetaDefs::MenuType_Invalid)
        , m_restrictionsOfMenuApplication(UIExtraDataMetaDefs::MenuApplicationActionType_Invalid)
        , m_restrictionsOfMenuMachine(UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Invalid)
        , m_restrictionsOfMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_Invalid)
        , m_restrictionsOfMenuInput(UIExtraDataMetaDefs::RuntimeMenuInputActionType_Invalid)
        , m_restrictionsOfMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Invalid)
#ifdef VBOX_WITH_DEBUGGER_GUI
        , m_restrictionsOfMenuDebug(UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_Invalid)
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
        , m_restrictionsOfMenuWindow(UIExtraDataMetaDefs::MenuWindowActionType_Invalid)
#endif /* VBOX_WS_MAC */
        , m_restrictionsOfMenuHelp(UIExtraDataMetaDefs::MenuHelpActionType_Invalid)
#ifndef VBOX_WS_MAC
        , m_fShowMiniToolBar(false)
        , m_fMiniToolBarAtTop(false)
#endif /* !VBOX_WS_MAC */
        , m_enmVisualState(UIVisualStateType_Invalid)
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsMachineInterface &other) const
    {
        return true
               && (m_fStatusBarEnabled == other.m_fStatusBarEnabled)
               && (m_statusBarRestrictions == other.m_statusBarRestrictions)
               && (m_statusBarOrder == other.m_statusBarOrder)
#ifndef VBOX_WS_MAC
               && (m_fMenuBarEnabled == other.m_fMenuBarEnabled)
#endif /* !VBOX_WS_MAC */
               && (m_restrictionsOfMenuBar == other.m_restrictionsOfMenuBar)
               && (m_restrictionsOfMenuApplication == other.m_restrictionsOfMenuApplication)
               && (m_restrictionsOfMenuMachine == other.m_restrictionsOfMenuMachine)
               && (m_restrictionsOfMenuView == other.m_restrictionsOfMenuView)
               && (m_restrictionsOfMenuInput == other.m_restrictionsOfMenuInput)
               && (m_restrictionsOfMenuDevices == other.m_restrictionsOfMenuDevices)
#ifdef VBOX_WITH_DEBUGGER_GUI
               && (m_restrictionsOfMenuDebug == other.m_restrictionsOfMenuDebug)
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
               && (m_restrictionsOfMenuWindow == other.m_restrictionsOfMenuWindow)
#endif /* VBOX_WS_MAC */
               && (m_restrictionsOfMenuHelp == other.m_restrictionsOfMenuHelp)
#ifndef VBOX_WS_MAC
               && (m_fShowMiniToolBar == other.m_fShowMiniToolBar)
               && (m_fMiniToolBarAtTop == other.m_fMiniToolBarAtTop)
#endif /* !VBOX_WS_MAC */
               && (m_enmVisualState == other.m_enmVisualState)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsMachineInterface &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsMachineInterface &other) const { return !equal(other); }

    /** Holds whether the status-bar is enabled. */
    bool                  m_fStatusBarEnabled;
    /** Holds the status-bar indicator restrictions. */
    QList<IndicatorType>  m_statusBarRestrictions;
    /** Holds the status-bar indicator order. */
    QList<IndicatorType>  m_statusBarOrder;

#ifndef VBOX_WS_MAC
    /** Holds whether the menu-bar is enabled. */
    bool                                                m_fMenuBarEnabled;
#endif /* !VBOX_WS_MAC */
    /** Holds the menu-bar menu restrictions. */
    UIExtraDataMetaDefs::MenuType                       m_restrictionsOfMenuBar;
    /** Holds the Application menu restrictions. */
    UIExtraDataMetaDefs::MenuApplicationActionType      m_restrictionsOfMenuApplication;
    /** Holds the Machine menu restrictions. */
    UIExtraDataMetaDefs::RuntimeMenuMachineActionType   m_restrictionsOfMenuMachine;
    /** Holds the View menu restrictions. */
    UIExtraDataMetaDefs::RuntimeMenuViewActionType      m_restrictionsOfMenuView;
    /** Holds the Input menu restrictions. */
    UIExtraDataMetaDefs::RuntimeMenuInputActionType     m_restrictionsOfMenuInput;
    /** Holds the Devices menu restrictions. */
    UIExtraDataMetaDefs::RuntimeMenuDevicesActionType   m_restrictionsOfMenuDevices;
#ifdef VBOX_WITH_DEBUGGER_GUI
    /** Holds the Debug menu restrictions. */
    UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType  m_restrictionsOfMenuDebug;
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
    /** Holds the Window menu restrictions. */
    UIExtraDataMetaDefs::MenuWindowActionType           m_restrictionsOfMenuWindow;
#endif /* VBOX_WS_MAC */
    /** Holds the Help menu restrictions. */
    UIExtraDataMetaDefs::MenuHelpActionType             m_restrictionsOfMenuHelp;

#ifndef VBOX_WS_MAC
    /** Holds whether the mini-toolbar is enabled. */
    bool  m_fShowMiniToolBar;
    /** Holds whether the mini-toolbar should be aligned at top of screen. */
    bool  m_fMiniToolBarAtTop;
#endif /* !VBOX_WS_MAC */

    /** Holds the visual state. */
    UIVisualStateType  m_enmVisualState;
};


UIMachineSettingsInterface::UIMachineSettingsInterface(const QUuid &uMachineId)
    : m_uMachineId(uMachineId)
    , m_pActionPool(0)
    , m_pCache(0)
    , m_pEditorMenuBar(0)
    , m_pLabelVisualState(0)
    , m_pEditorVisualState(0)
    , m_pLabelMiniToolBar(0)
    , m_pCheckBoxShowMiniToolBar(0)
    , m_pCheckBoxMiniToolBarAlignment(0)
    , m_pEditorStatusBar(0)
{
    /* Prepare: */
    prepare();
}

UIMachineSettingsInterface::~UIMachineSettingsInterface()
{
    /* Cleanup: */
    cleanup();
}

bool UIMachineSettingsInterface::changed() const
{
    return m_pCache->wasChanged();
}

void UIMachineSettingsInterface::loadToCacheFrom(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Clear cache initially: */
    m_pCache->clear();

    /* Prepare old interface data: */
    UIDataSettingsMachineInterface oldInterfaceData;

    /* Gather old interface data: */
    oldInterfaceData.m_fStatusBarEnabled = gEDataManager->statusBarEnabled(m_machine.GetId());
    oldInterfaceData.m_statusBarRestrictions = gEDataManager->restrictedStatusBarIndicators(m_machine.GetId());
    oldInterfaceData.m_statusBarOrder = gEDataManager->statusBarIndicatorOrder(m_machine.GetId());
#ifndef VBOX_WS_MAC
    oldInterfaceData.m_fMenuBarEnabled = gEDataManager->menuBarEnabled(m_machine.GetId());
#endif
    oldInterfaceData.m_restrictionsOfMenuBar = gEDataManager->restrictedRuntimeMenuTypes(m_machine.GetId());
    oldInterfaceData.m_restrictionsOfMenuApplication = gEDataManager->restrictedRuntimeMenuApplicationActionTypes(m_machine.GetId());
    oldInterfaceData.m_restrictionsOfMenuMachine = gEDataManager->restrictedRuntimeMenuMachineActionTypes(m_machine.GetId());
    oldInterfaceData.m_restrictionsOfMenuView = gEDataManager->restrictedRuntimeMenuViewActionTypes(m_machine.GetId());
    oldInterfaceData.m_restrictionsOfMenuInput = gEDataManager->restrictedRuntimeMenuInputActionTypes(m_machine.GetId());
    oldInterfaceData.m_restrictionsOfMenuDevices = gEDataManager->restrictedRuntimeMenuDevicesActionTypes(m_machine.GetId());
#ifdef VBOX_WITH_DEBUGGER_GUI
    oldInterfaceData.m_restrictionsOfMenuDebug = gEDataManager->restrictedRuntimeMenuDebuggerActionTypes(m_machine.GetId());
#endif
#ifdef VBOX_WS_MAC
    oldInterfaceData.m_restrictionsOfMenuWindow = gEDataManager->restrictedRuntimeMenuWindowActionTypes(m_machine.GetId());
#endif
    oldInterfaceData.m_restrictionsOfMenuHelp = gEDataManager->restrictedRuntimeMenuHelpActionTypes(m_machine.GetId());
#ifndef VBOX_WS_MAC
    oldInterfaceData.m_fShowMiniToolBar = gEDataManager->miniToolbarEnabled(m_machine.GetId());
    oldInterfaceData.m_fMiniToolBarAtTop = gEDataManager->miniToolbarAlignment(m_machine.GetId()) == Qt::AlignTop;
#endif
    oldInterfaceData.m_enmVisualState = gEDataManager->requestedVisualState(m_machine.GetId());

    /* Cache old interface data: */
    m_pCache->cacheInitialData(oldInterfaceData);

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

void UIMachineSettingsInterface::getFromCache()
{
    /* Get old interface data from the cache: */
    const UIDataSettingsMachineInterface &oldInterfaceData = m_pCache->base();

    /* Load old interface data from the cache: */
    m_pEditorStatusBar->setStatusBarEnabled(oldInterfaceData.m_fStatusBarEnabled);
    m_pEditorStatusBar->setStatusBarConfiguration(oldInterfaceData.m_statusBarRestrictions,
                                                  oldInterfaceData.m_statusBarOrder);
#ifndef VBOX_WS_MAC
    m_pEditorMenuBar->setMenuBarEnabled(oldInterfaceData.m_fMenuBarEnabled);
#endif
    m_pEditorMenuBar->setRestrictionsOfMenuBar(oldInterfaceData.m_restrictionsOfMenuBar);
    m_pEditorMenuBar->setRestrictionsOfMenuApplication(oldInterfaceData.m_restrictionsOfMenuApplication);
    m_pEditorMenuBar->setRestrictionsOfMenuMachine(oldInterfaceData.m_restrictionsOfMenuMachine);
    m_pEditorMenuBar->setRestrictionsOfMenuView(oldInterfaceData.m_restrictionsOfMenuView);
    m_pEditorMenuBar->setRestrictionsOfMenuInput(oldInterfaceData.m_restrictionsOfMenuInput);
    m_pEditorMenuBar->setRestrictionsOfMenuDevices(oldInterfaceData.m_restrictionsOfMenuDevices);
#ifdef VBOX_WITH_DEBUGGER_GUI
    m_pEditorMenuBar->setRestrictionsOfMenuDebug(oldInterfaceData.m_restrictionsOfMenuDebug);
#endif
#ifdef VBOX_WS_MAC
    m_pEditorMenuBar->setRestrictionsOfMenuWindow(oldInterfaceData.m_restrictionsOfMenuWindow);
#endif
    m_pEditorMenuBar->setRestrictionsOfMenuHelp(oldInterfaceData.m_restrictionsOfMenuHelp);
#ifndef VBOX_WS_MAC
    m_pCheckBoxShowMiniToolBar->setChecked(oldInterfaceData.m_fShowMiniToolBar);
    m_pCheckBoxMiniToolBarAlignment->setChecked(oldInterfaceData.m_fMiniToolBarAtTop);
#endif
    m_pEditorVisualState->setMachineId(m_machine.GetId());
    m_pEditorVisualState->setValue(oldInterfaceData.m_enmVisualState);

    /* Polish page finally: */
    polishPage();

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsInterface::putToCache()
{
    /* Prepare new interface data: */
    UIDataSettingsMachineInterface newInterfaceData;

    /* Gather new interface data: */
    newInterfaceData.m_fStatusBarEnabled = m_pEditorStatusBar->isStatusBarEnabled();
    newInterfaceData.m_statusBarRestrictions = m_pEditorStatusBar->statusBarIndicatorRestrictions();
    newInterfaceData.m_statusBarOrder = m_pEditorStatusBar->statusBarIndicatorOrder();
#ifndef VBOX_WS_MAC
    newInterfaceData.m_fMenuBarEnabled = m_pEditorMenuBar->isMenuBarEnabled();
#endif
    newInterfaceData.m_restrictionsOfMenuBar = m_pEditorMenuBar->restrictionsOfMenuBar();
    newInterfaceData.m_restrictionsOfMenuApplication = m_pEditorMenuBar->restrictionsOfMenuApplication();
    newInterfaceData.m_restrictionsOfMenuMachine = m_pEditorMenuBar->restrictionsOfMenuMachine();
    newInterfaceData.m_restrictionsOfMenuView = m_pEditorMenuBar->restrictionsOfMenuView();
    newInterfaceData.m_restrictionsOfMenuInput = m_pEditorMenuBar->restrictionsOfMenuInput();
    newInterfaceData.m_restrictionsOfMenuDevices = m_pEditorMenuBar->restrictionsOfMenuDevices();
#ifdef VBOX_WITH_DEBUGGER_GUI
    newInterfaceData.m_restrictionsOfMenuDebug = m_pEditorMenuBar->restrictionsOfMenuDebug();
#endif
#ifdef VBOX_WS_MAC
    newInterfaceData.m_restrictionsOfMenuWindow = m_pEditorMenuBar->restrictionsOfMenuWindow();
#endif
    newInterfaceData.m_restrictionsOfMenuHelp = m_pEditorMenuBar->restrictionsOfMenuHelp();
#ifndef VBOX_WS_MAC
    newInterfaceData.m_fShowMiniToolBar = m_pCheckBoxShowMiniToolBar->isChecked();
    newInterfaceData.m_fMiniToolBarAtTop = m_pCheckBoxMiniToolBarAlignment->isChecked();
#endif
    newInterfaceData.m_enmVisualState = m_pEditorVisualState->value();

    /* Cache new interface data: */
    m_pCache->cacheCurrentData(newInterfaceData);
}

void UIMachineSettingsInterface::saveFromCacheTo(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Update interface data and failing state: */
    setFailed(!saveInterfaceData());

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

void UIMachineSettingsInterface::retranslateUi()
{
    m_pEditorMenuBar->setWhatsThis(tr("Allows to modify VM menu-bar contents."));
    m_pLabelVisualState->setText(tr("Visual State:"));
    m_pEditorVisualState->setWhatsThis(tr("Selects the visual state. If machine is running it will be applied as soon as possible,"
                                          "otherwise desired one will be defined."));
    m_pLabelMiniToolBar->setText(tr("Mini ToolBar:"));
    m_pCheckBoxShowMiniToolBar->setWhatsThis(tr("When checked, show the Mini ToolBar in full-screen and seamless modes."));
    m_pCheckBoxShowMiniToolBar->setText(tr("Show in &Full-screen/Seamless"));
    m_pCheckBoxMiniToolBarAlignment->setWhatsThis(tr("When checked, show the Mini ToolBar at the top of the screen, rather than in its"
                                              "default position at the bottom of the screen."));
    m_pCheckBoxMiniToolBarAlignment->setText(tr("Show at &Top of Screen"));
    m_pEditorStatusBar->setWhatsThis(tr("Allows to modify VM status-bar contents."));
}

void UIMachineSettingsInterface::polishPage()
{
    /* Polish interface page availability: */
    m_pEditorMenuBar->setEnabled(isMachineInValidMode());
#ifdef VBOX_WS_MAC
    m_pLabelMiniToolBar->hide();
    m_pCheckBoxShowMiniToolBar->hide();
    m_pCheckBoxMiniToolBarAlignment->hide();
#else /* !VBOX_WS_MAC */
    m_pLabelMiniToolBar->setEnabled(isMachineInValidMode());
    m_pCheckBoxShowMiniToolBar->setEnabled(isMachineInValidMode());
    m_pCheckBoxMiniToolBarAlignment->setEnabled(isMachineInValidMode() && m_pCheckBoxShowMiniToolBar->isChecked());
#endif /* !VBOX_WS_MAC */
    m_pEditorStatusBar->setEnabled(isMachineInValidMode());
}

void UIMachineSettingsInterface::prepare()
{
    /* Prepare action-pool: */
    m_pActionPool = UIActionPool::create(UIActionPoolType_Runtime);

    /* Prepare cache: */
    m_pCache = new UISettingsCacheMachineInterface;
    AssertPtrReturnVoid(m_pCache);

    /* Prepare everything: */
    prepareWidgets();
    prepareConnections();

    /* Apply language settings: */
    retranslateUi();
}

void UIMachineSettingsInterface::prepareWidgets()
{
    /* Prepare main layout: */
    QGridLayout *pLayoutMain = new QGridLayout(this);
    if (pLayoutMain)
    {
        pLayoutMain->setColumnStretch(1, 1);
        pLayoutMain->setRowStretch(4, 1);

        /* Prepare menu-bar editor: */
        m_pEditorMenuBar = new UIMenuBarEditorWidget(this);
        if (m_pEditorMenuBar)
        {
            m_pEditorMenuBar->setActionPool(m_pActionPool);
            m_pEditorMenuBar->setMachineID(m_uMachineId);

            pLayoutMain->addWidget(m_pEditorMenuBar, 0, 0, 1, 3);
        }

        /* Prepare visual-state label: */
        m_pLabelVisualState = new QLabel(this);
        if (m_pLabelVisualState)
        {
            m_pLabelVisualState->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pLayoutMain->addWidget(m_pLabelVisualState, 1, 0);
        }
        /* Prepare visual-state editor: */
        m_pEditorVisualState = new UIVisualStateEditor(this);
        if (m_pEditorVisualState)
            pLayoutMain->addWidget(m_pEditorVisualState, 1, 1);

        /* Prepare mini-toolbar label: */
        m_pLabelMiniToolBar = new QLabel(this);
        if (m_pLabelMiniToolBar)
        {
            m_pLabelMiniToolBar->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pLayoutMain->addWidget(m_pLabelMiniToolBar, 2, 0);
        }
        /* Prepare 'show mini-toolbar' check-box: */
        m_pCheckBoxShowMiniToolBar = new QCheckBox(this);
        if (m_pCheckBoxShowMiniToolBar)
            pLayoutMain->addWidget(m_pCheckBoxShowMiniToolBar, 2, 1);
        /* Prepare 'mini-toolbar alignment' check-box: */
        m_pCheckBoxMiniToolBarAlignment = new QCheckBox(this);
        if (m_pCheckBoxMiniToolBarAlignment)
            pLayoutMain->addWidget(m_pCheckBoxMiniToolBarAlignment, 3, 1);

        /* Prepare status-bar editor: */
        m_pEditorStatusBar = new UIStatusBarEditorWidget(this);
        if (m_pEditorStatusBar)
        {
            m_pEditorStatusBar->setMachineID(m_uMachineId);
            pLayoutMain->addWidget(m_pEditorStatusBar, 5, 0, 1, 3);
        }
    }
}

void UIMachineSettingsInterface::prepareConnections()
{
    connect(m_pCheckBoxShowMiniToolBar, &QCheckBox::toggled,
            m_pCheckBoxMiniToolBarAlignment, &UIMachineSettingsInterface::setEnabled);
}

void UIMachineSettingsInterface::cleanup()
{
    /* Cleanup action-pool: */
    UIActionPool::destroy(m_pActionPool);

    /* Cleanup cache: */
    delete m_pCache;
    m_pCache = 0;
}

bool UIMachineSettingsInterface::saveInterfaceData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save display settings from the cache: */
    if (fSuccess && isMachineInValidMode() && m_pCache->wasChanged())
    {
        /* Save 'Menu-bar' data from the cache: */
        if (fSuccess)
            fSuccess = saveMenuBarData();
        /* Save 'Status-bar' data from the cache: */
        if (fSuccess)
            fSuccess = saveStatusBarData();
        /* Save 'Mini-toolbar' data from the cache: */
        if (fSuccess)
            fSuccess = saveMiniToolbarData();
        /* Save 'Visual State' data from the cache: */
        if (fSuccess)
            fSuccess = saveVisualStateData();
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsInterface::saveMenuBarData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save 'Menu-bar' data from the cache: */
    if (fSuccess)
    {
        /* Get old interface data from the cache: */
        const UIDataSettingsMachineInterface &oldInterfaceData = m_pCache->base();
        /* Get new interface data from the cache: */
        const UIDataSettingsMachineInterface &newInterfaceData = m_pCache->data();

#ifndef VBOX_WS_MAC
        /* Save whether menu-bar is enabled: */
        if (fSuccess && newInterfaceData.m_fMenuBarEnabled != oldInterfaceData.m_fMenuBarEnabled)
            /* fSuccess = */ gEDataManager->setMenuBarEnabled(newInterfaceData.m_fMenuBarEnabled, m_machine.GetId());
#endif
        /* Save menu-bar restrictions: */
        if (fSuccess && newInterfaceData.m_restrictionsOfMenuBar != oldInterfaceData.m_restrictionsOfMenuBar)
            /* fSuccess = */ gEDataManager->setRestrictedRuntimeMenuTypes(newInterfaceData.m_restrictionsOfMenuBar, m_machine.GetId());
        /* Save menu-bar Application menu restrictions: */
        if (fSuccess && newInterfaceData.m_restrictionsOfMenuApplication != oldInterfaceData.m_restrictionsOfMenuApplication)
            /* fSuccess = */ gEDataManager->setRestrictedRuntimeMenuApplicationActionTypes(newInterfaceData.m_restrictionsOfMenuApplication, m_machine.GetId());
        /* Save menu-bar Machine menu restrictions: */
        if (fSuccess && newInterfaceData.m_restrictionsOfMenuMachine != oldInterfaceData.m_restrictionsOfMenuMachine)
           /* fSuccess = */  gEDataManager->setRestrictedRuntimeMenuMachineActionTypes(newInterfaceData.m_restrictionsOfMenuMachine, m_machine.GetId());
        /* Save menu-bar View menu restrictions: */
        if (fSuccess && newInterfaceData.m_restrictionsOfMenuView != oldInterfaceData.m_restrictionsOfMenuView)
           /* fSuccess = */  gEDataManager->setRestrictedRuntimeMenuViewActionTypes(newInterfaceData.m_restrictionsOfMenuView, m_machine.GetId());
        /* Save menu-bar Input menu restrictions: */
        if (fSuccess && newInterfaceData.m_restrictionsOfMenuInput != oldInterfaceData.m_restrictionsOfMenuInput)
            /* fSuccess = */ gEDataManager->setRestrictedRuntimeMenuInputActionTypes(newInterfaceData.m_restrictionsOfMenuInput, m_machine.GetId());
        /* Save menu-bar Devices menu restrictions: */
        if (fSuccess && newInterfaceData.m_restrictionsOfMenuDevices != oldInterfaceData.m_restrictionsOfMenuDevices)
            /* fSuccess = */ gEDataManager->setRestrictedRuntimeMenuDevicesActionTypes(newInterfaceData.m_restrictionsOfMenuDevices, m_machine.GetId());
#ifdef VBOX_WITH_DEBUGGER_GUI
        /* Save menu-bar Debug menu restrictions: */
        if (fSuccess && newInterfaceData.m_restrictionsOfMenuDebug != oldInterfaceData.m_restrictionsOfMenuDebug)
            /* fSuccess = */ gEDataManager->setRestrictedRuntimeMenuDebuggerActionTypes(newInterfaceData.m_restrictionsOfMenuDebug, m_machine.GetId());
#endif
#ifdef VBOX_WS_MAC
        /* Save menu-bar Window menu restrictions: */
        if (fSuccess && newInterfaceData.m_restrictionsOfMenuWindow != oldInterfaceData.m_restrictionsOfMenuWindow)
            /* fSuccess = */ gEDataManager->setRestrictedRuntimeMenuWindowActionTypes(newInterfaceData.m_restrictionsOfMenuWindow, m_machine.GetId());
#endif
        /* Save menu-bar Help menu restrictions: */
        if (fSuccess && newInterfaceData.m_restrictionsOfMenuHelp != oldInterfaceData.m_restrictionsOfMenuHelp)
            /* fSuccess = */ gEDataManager->setRestrictedRuntimeMenuHelpActionTypes(newInterfaceData.m_restrictionsOfMenuHelp, m_machine.GetId());
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsInterface::saveStatusBarData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save 'Status-bar' data from the cache: */
    if (fSuccess)
    {
        /* Get old interface data from the cache: */
        const UIDataSettingsMachineInterface &oldInterfaceData = m_pCache->base();
        /* Get new interface data from the cache: */
        const UIDataSettingsMachineInterface &newInterfaceData = m_pCache->data();

        /* Save whether status-bar is enabled: */
        if (fSuccess && newInterfaceData.m_fStatusBarEnabled != oldInterfaceData.m_fStatusBarEnabled)
            /* fSuccess = */ gEDataManager->setStatusBarEnabled(newInterfaceData.m_fStatusBarEnabled, m_machine.GetId());
        /* Save status-bar restrictions: */
        if (fSuccess && newInterfaceData.m_statusBarRestrictions != oldInterfaceData.m_statusBarRestrictions)
            /* fSuccess = */ gEDataManager->setRestrictedStatusBarIndicators(newInterfaceData.m_statusBarRestrictions, m_machine.GetId());
        /* Save status-bar order: */
        if (fSuccess && newInterfaceData.m_statusBarOrder != oldInterfaceData.m_statusBarOrder)
            /* fSuccess = */ gEDataManager->setStatusBarIndicatorOrder(newInterfaceData.m_statusBarOrder, m_machine.GetId());
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsInterface::saveMiniToolbarData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save 'Mini-toolbar' data from the cache: */
    if (fSuccess)
    {
        /* Get old interface data from the cache: */
        const UIDataSettingsMachineInterface &oldInterfaceData = m_pCache->base(); Q_UNUSED(oldInterfaceData);
        /* Get new interface data from the cache: */
        const UIDataSettingsMachineInterface &newInterfaceData = m_pCache->data(); Q_UNUSED(newInterfaceData);

#ifndef VBOX_WS_MAC
        /* Save whether mini-toolbar is enabled: */
        if (fSuccess && newInterfaceData.m_fShowMiniToolBar != oldInterfaceData.m_fShowMiniToolBar)
            /* fSuccess = */ gEDataManager->setMiniToolbarEnabled(newInterfaceData.m_fShowMiniToolBar, m_machine.GetId());
        /* Save whether mini-toolbar should be location at top of screen: */
        if (fSuccess && newInterfaceData.m_fMiniToolBarAtTop != oldInterfaceData.m_fMiniToolBarAtTop)
            /* fSuccess = */ gEDataManager->setMiniToolbarAlignment(newInterfaceData.m_fMiniToolBarAtTop ? Qt::AlignTop : Qt::AlignBottom, m_machine.GetId());
#endif
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsInterface::saveVisualStateData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save 'Visual State' data from the cache: */
    if (fSuccess)
    {
        /* Get old interface data from the cache: */
        const UIDataSettingsMachineInterface &oldInterfaceData = m_pCache->base();
        /* Get new interface data from the cache: */
        const UIDataSettingsMachineInterface &newInterfaceData = m_pCache->data();

        /* Save desired visual state: */
        if (fSuccess && newInterfaceData.m_enmVisualState != oldInterfaceData.m_enmVisualState)
            /* fSuccess = */ gEDataManager->setRequestedVisualState(newInterfaceData.m_enmVisualState, m_machine.GetId());
    }
    /* Return result: */
    return fSuccess;
}

/* $Id: UIActionPool.h 85661 2020-08-10 12:04:07Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIActionPool class declaration.
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

#ifndef FEQT_INCLUDED_SRC_globals_UIActionPool_h
#define FEQT_INCLUDED_SRC_globals_UIActionPool_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QAction>
#include <QMenu>
#include <QVector>

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "UIExtraDataDefs.h"
#include "UILibraryDefs.h"

/* Forward declarations: */
class QKeySequence;
class QString;
class UIActionPool;
class UIActionPoolRuntime;
class UIActionPoolManager;


/** Action-pool types. */
enum UIActionPoolType
{
    UIActionPoolType_Manager,
    UIActionPoolType_Runtime
};

/** Action types. */
enum UIActionType
{
    UIActionType_Menu,
    UIActionType_Simple,
    UIActionType_Toggle
};

/** Action indexes. */
enum UIActionIndex
{
    /* 'Application' menu actions: */
    UIActionIndex_M_Application,
#ifdef VBOX_WS_MAC
    UIActionIndex_M_Application_S_About,
#endif
    UIActionIndex_M_Application_S_Preferences,
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    UIActionIndex_M_Application_S_NetworkAccessManager,
    UIActionIndex_M_Application_S_CheckForUpdates,
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
    UIActionIndex_M_Application_S_ResetWarnings,
    UIActionIndex_M_Application_S_Close,

#ifdef VBOX_WS_MAC
    /* 'Window' menu actions: */
    UIActionIndex_M_Window,
    UIActionIndex_M_Window_S_Minimize,
#endif

    /* 'Help' menu actions: */
    UIActionIndex_Menu_Help,
    UIActionIndex_Simple_Contents,
    UIActionIndex_Simple_WebSite,
    UIActionIndex_Simple_BugTracker,
    UIActionIndex_Simple_Forums,
    UIActionIndex_Simple_Oracle,
#ifndef VBOX_WS_MAC
    UIActionIndex_Simple_About,
#endif

    /* 'Log' menu actions: */
    UIActionIndex_M_LogWindow,
    UIActionIndex_M_Log,
    UIActionIndex_M_Log_T_Find,
    UIActionIndex_M_Log_T_Filter,
    UIActionIndex_M_Log_T_Bookmark,
    UIActionIndex_M_Log_T_Options,
    UIActionIndex_M_Log_S_Refresh,
    UIActionIndex_M_Log_S_Save,

    /* 'Performance' menu actions: */
    UIActionIndex_M_Performance,
    UIActionIndex_M_Performance_S_Export,

    /* File Manager actions: */
    UIActionIndex_M_FileManager,
    UIActionIndex_M_FileManager_M_HostSubmenu,
    UIActionIndex_M_FileManager_M_GuestSubmenu,
    UIActionIndex_M_FileManager_S_CopyToGuest,
    UIActionIndex_M_FileManager_S_CopyToHost,
    UIActionIndex_M_FileManager_T_Options,
    UIActionIndex_M_FileManager_T_Log,
    UIActionIndex_M_FileManager_T_Operations,
    UIActionIndex_M_FileManager_T_Session,
    UIActionIndex_M_FileManager_S_Host_GoUp,
    UIActionIndex_M_FileManager_S_Guest_GoUp,
    UIActionIndex_M_FileManager_S_Host_GoHome,
    UIActionIndex_M_FileManager_S_Guest_GoHome,
    UIActionIndex_M_FileManager_S_Host_Refresh,
    UIActionIndex_M_FileManager_S_Guest_Refresh,
    UIActionIndex_M_FileManager_S_Host_Delete,
    UIActionIndex_M_FileManager_S_Guest_Delete,
    UIActionIndex_M_FileManager_S_Host_Rename,
    UIActionIndex_M_FileManager_S_Guest_Rename,
    UIActionIndex_M_FileManager_S_Host_CreateNewDirectory,
    UIActionIndex_M_FileManager_S_Guest_CreateNewDirectory,
    UIActionIndex_M_FileManager_S_Host_Copy,
    UIActionIndex_M_FileManager_S_Guest_Copy,
    UIActionIndex_M_FileManager_S_Host_Cut,
    UIActionIndex_M_FileManager_S_Guest_Cut,
    UIActionIndex_M_FileManager_S_Host_Paste,
    UIActionIndex_M_FileManager_S_Guest_Paste,
    UIActionIndex_M_FileManager_S_Host_SelectAll,
    UIActionIndex_M_FileManager_S_Guest_SelectAll,
    UIActionIndex_M_FileManager_S_Host_InvertSelection,
    UIActionIndex_M_FileManager_S_Guest_InvertSelection,
    UIActionIndex_M_FileManager_S_Host_ShowProperties,
    UIActionIndex_M_FileManager_S_Guest_ShowProperties,

    /* Maximum index: */
    UIActionIndex_Max
};

/** Action restriction levels. */
enum UIActionRestrictionLevel
{
    UIActionRestrictionLevel_Base,
    UIActionRestrictionLevel_Session,
    UIActionRestrictionLevel_Logic
};


/** QMenu extension. */
class SHARED_LIBRARY_STUFF UIMenu : public QMenu
{
    Q_OBJECT;

public:

    /** Constructs menu. */
    UIMenu();

    /** Defines whether tool-tip should be shown. */
    void setShowToolTip(bool fShowToolTips) { m_fShowToolTip = fShowToolTips; }

#ifdef VBOX_WS_MAC
    /** Mac OS X: Returns whether this menu is consumable by the menu-bar. */
    bool isConsumable() const { return m_fConsumable; }
    /** Mac OS X: Defines whether this menu is @a fConsumable by the menu-bar. */
    void setConsumable(bool fConsumable) { m_fConsumable = fConsumable; }

    /** Mac OS X: Returns whether this menu is consumed by the menu-bar. */
    bool isConsumed() const { return m_fConsumed; }
    /** Mac OS X: Defines whether this menu is @a fConsumed by the menu-bar. */
    void setConsumed(bool fConsumed) { m_fConsumed = fConsumed; }
#endif /* VBOX_WS_MAC */

protected:

    /** Handles any Qt @a pEvent. */
    virtual bool event(QEvent *pEvent);

private:

    /** Holds whether tool-tip should be shown. */
    bool m_fShowToolTip;

#ifdef VBOX_WS_MAC
    /** Mac OS X: Holds whether this menu can be consumed by the menu-bar. */
    bool m_fConsumable;
    /** Mac OS X: Holds whether this menu is consumed by the menu-bar. */
    bool m_fConsumed;
#endif /* VBOX_WS_MAC */
};


/** Abstract QAction extension. */
class SHARED_LIBRARY_STUFF UIAction : public QAction
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class.
      * @param  enmType  Brings the action type. */
    UIAction(UIActionPool *pParent, UIActionType enmType, bool fMachineMenuAction = false);
    /** Destructs action. */
    virtual ~UIAction() /* override */ { delete menu(); }

    /** Returns action-pool this action belongs to. */
    UIActionPool *actionPool() const { return m_pActionPool; }
    /** Returns action type. */
    UIActionType type() const { return m_enmType; }

    /** Returns menu contained by this action. */
    UIMenu *menu() const;

    /** Returns current action state. */
    int state() const { return m_iState; }
    /** Defines current action @a iState. */
    void setState(int iState);

    /** Defines @a icon for certain @a iState. */
    void setIcon(int iState, const QIcon &icon);
    /** Defines @a icon. */
    void setIcon(const QIcon &icon);

    /** Returns current action name. */
    QString name() const { return m_strName; }
    /** Defines current action name. */
    void setName(const QString &strName);

    /** Returns action shortcut scope. */
    QString shortcutScope() const { return m_strShortcutScope; }
    /** Defines action @a strShortcutScope. */
    void setShortcutScope(const QString &strShortcutScope) { m_strShortcutScope = strShortcutScope; }

    /** Defines current keyboard shortcuts for this action. */
    void setShortcuts(const QList<QKeySequence> &shortcuts);
    /** Make action show keyboard shortcut. */
    void showShortcut();
    /** Make action hide keyboard shortcut. */
    void hideShortcut();

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return 0; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return QString(); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return true; }

    /** Returns extra-data ID to save keyboard shortcut under. */
    virtual QString shortcutExtraDataID() const { return QString(); }
    /** Returns default keyboard shortcut for this action. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const { return QKeySequence(); }
    /** Returns standard keyboard shortcut for this action. */
    virtual QKeySequence standardShortcut(UIActionPoolType) const { return QKeySequence(); }

    /** Retranslates action. */
    virtual void retranslateUi() = 0;

protected:

    /** Handles state change. */
    virtual void handleStateChange() {}

    /** Returns current action name in menu. */
    QString nameInMenu() const;

    /** Updates action icon. */
    void updateIcon();
    /** Updates action text. */
    void updateText();

    /** Simplifies passed @a strText by removing dots and ampersands.
      * @note Used to simplify action names for tool-tip needs. */
    static QString simplifyText(QString strText);

    /** Holds the reference to the action-pool this action belongs to. */
    UIActionPool           *m_pActionPool;
    /** Holds the type of the action-pool this action belongs to. */
    const UIActionPoolType  m_enmActionPoolType;

    /** Holds the action type. */
    const UIActionType  m_enmType;
    /** Holds whether this is machine-menu action. */
    const bool          m_fMachineMenuAction;

    /** Holds current action state. */
    int             m_iState;
    /** Holds action icons. */
    QVector<QIcon>  m_icons;

    /** Holds the action name. */
    QString  m_strName;

    /** Holds the action shortcut scope. */
    QString              m_strShortcutScope;
    /** Holds the action shortcuts. */
    QList<QKeySequence>  m_shortcuts;
    /** Holds whether action shortcut hidden. */
    bool                 m_fShortcutHidden;
};


/** Abstract UIAction extension for 'Menu' action type. */
class SHARED_LIBRARY_STUFF UIActionMenu : public UIAction
{
    Q_OBJECT;

protected:

    /** Constructs menu action passing @a pParent to the base-class.
      * @param  strIcon          Brings the normal-icon name.
      * @param  strIconDisabled  Brings the disabled-icon name. */
    UIActionMenu(UIActionPool *pParent,
                 const QString &strIcon = QString(), const QString &strIconDisabled = QString());
    /** Constructs menu action passing @a pParent to the base-class.
      * @param  strIconNormal          Brings the normal-icon name.
      * @param  strIconSmall           Brings the small-icon name.
      * @param  strIconNormalDisabled  Brings the normal-disabled-icon name.
      * @param  strIconSmallDisabled   Brings the small-disabled-icon name. */
    UIActionMenu(UIActionPool *pParent,
                 const QString &strIconNormal, const QString &strIconSmall,
                 const QString &strIconNormalDisabled, const QString &strIconSmallDisabled);
    /** Constructs menu action passing @a pParent to the base-class.
      * @param  icon  Brings the icon. */
    UIActionMenu(UIActionPool *pParent,
                 const QIcon &icon);

    /** Destructs menu action. */
    virtual ~UIActionMenu() /* override */;

    /** Defines whether tool-tip should be shown. */
    void setShowToolTip(bool fShowToolTip);

    /** Shows menu. */
    void showMenu();
    /** Hides menu. */
    void hideMenu();

private:

    /** Prepares all. */
    void prepare();

    /** Holds the menu instance. */
    UIMenu *m_pMenu;
};


/** Abstract UIAction extension for 'Simple' action type. */
class SHARED_LIBRARY_STUFF UIActionSimple : public UIAction
{
    Q_OBJECT;

protected:

    /** Constructs simple action passing @a pParent to the base-class.
      * @param  fMachineMenuAction  Brings whether this action is a part of machine menu. */
    UIActionSimple(UIActionPool *pParent,
                   bool fMachineMenuAction = false);
    /** Constructs simple action passing @a pParent to the base-class.
      * @param  strIcon             Brings the normal-icon name.
      * @param  strIconDisabled     Brings the disabled-icon name.
      * @param  fMachineMenuAction  Brings whether this action is a part of machine menu. */
    UIActionSimple(UIActionPool *pParent,
                   const QString &strIcon, const QString &strIconDisabled,
                   bool fMachineMenuAction = false);
    /** Constructs simple action passing @a pParent to the base-class.
      * @param  strIconNormal          Brings the normal-icon name.
      * @param  strIconSmall           Brings the small-icon name.
      * @param  strIconNormalDisabled  Brings the normal-disabled-icon name.
      * @param  strIconSmallDisabled   Brings the small-disabled-icon name.
      * @param  fMachineMenuAction     Brings whether this action is a part of machine menu. */
    UIActionSimple(UIActionPool *pParent,
                   const QString &strIconNormal, const QString &strIconSmall,
                   const QString &strIconNormalDisabled, const QString &strIconSmallDisabled,
                   bool fMachineMenuAction = false);
    /** Constructs simple action passing @a pParent to the base-class.
      * @param  icon                Brings the icon.
      * @param  fMachineMenuAction  Brings whether this action is a part of machine menu. */
    UIActionSimple(UIActionPool *pParent,
                   const QIcon &icon,
                   bool fMachineMenuAction = false);
};


/** Abstract UIAction extension for 'Toggle' action type. */
class SHARED_LIBRARY_STUFF UIActionToggle : public UIAction
{
    Q_OBJECT;

protected:

    /** Constructs toggle action passing @a pParent to the base-class.
      * @param  fMachineMenuAction  Brings whether this action is a part of machine menu. */
    UIActionToggle(UIActionPool *pParent,
                   bool fMachineMenuAction = false);
    /** Constructs toggle action passing @a pParent to the base-class.
      * @param  strIcon             Brings the normal-icon name.
      * @param  strIconDisabled     Brings the disabled-icon name.
      * @param  fMachineMenuAction  Brings whether this action is a part of machine menu. */
    UIActionToggle(UIActionPool *pParent,
                   const QString &strIcon, const QString &strIconDisabled,
                   bool fMachineMenuAction = false);
    /** Constructs toggle action passing @a pParent to the base-class.
      * @param  strIconOn           Brings the on-icon name.
      * @param  strIconOff          Brings the off-icon name.
      * @param  strIconOnDisabled   Brings the on-disabled-icon name.
      * @param  strIconOffDisabled  Brings the off-disabled-icon name.
      * @param  fMachineMenuAction  Brings whether this action is a part of machine menu. */
    UIActionToggle(UIActionPool *pParent,
                   const QString &strIconOn, const QString &strIconOff,
                   const QString &strIconOnDisabled, const QString &strIconOffDisabled,
                   bool fMachineMenuAction = false);
    /** Constructs toggle action passing @a pParent to the base-class.
      * @param  icon                Brings the icon.
      * @param  fMachineMenuAction  Brings whether this action is a part of machine menu. */
    UIActionToggle(UIActionPool *pParent,
                   const QIcon &icon,
                   bool fMachineMenuAction = false);

private:

    /** Prepares all. */
    void prepare();
};


/** Abstract QObject extension
  * representing action-pool interface and factory. */
class SHARED_LIBRARY_STUFF UIActionPool : public QIWithRetranslateUI3<QObject>
{
    Q_OBJECT;

    /** Pointer to menu update-handler for this class. */
    typedef void (UIActionPool::*PTFActionPool)();
    /** Pointer to menu update-handler for Manager sub-class. */
    typedef void (UIActionPoolManager::*PTFActionPoolManager)();
    /** Pointer to menu update-handler for Runtime sub-class. */
    typedef void (UIActionPoolRuntime::*PTFActionPoolRuntime)();
    /** Union for three defines above. */
    union PointerToFunction
    {
        PTFActionPool ptf;
        PTFActionPoolManager ptfm;
        PTFActionPoolRuntime ptfr;
    };

signals:

    /** Notifies about menu prepare. */
    void sigNotifyAboutMenuPrepare(int iIndex, QMenu *pMenu);

#ifdef VBOX_WS_MAC
    /** Notifies about @a pAction hovered. */
    void sigActionHovered(UIAction *pAction);
#endif

public:

    /** Creates singleton instance. */
    static UIActionPool *create(UIActionPoolType enmType);
    /** Destroys singleton instance. */
    static void destroy(UIActionPool *pActionPool);

    /** Creates temporary singleton instance,
      * used to initialize shortcuts-pool from action-pool of passed @a enmType. */
    static void createTemporary(UIActionPoolType enmType);

    /** Cast action-pool to Manager one. */
    UIActionPoolManager *toManager();
    /** Cast action-pool to Runtime one. */
    UIActionPoolRuntime *toRuntime();

    /** Returns action-pool type. */
    UIActionPoolType type() const { return m_enmType; }
    /** Returns whether this action-pool is temporary. */
    bool isTemporary() const { return m_fTemporary; }

    /** Returns the action for the passed @a iIndex. */
    UIAction *action(int iIndex) const;
    /** Returns all the actions action-pool contains. */
    QList<UIAction*> actions() const;

    /** Returns the action group for the passed @a iIndex.
      * @note Only menu actions can have action groups. */
    QActionGroup *actionGroup(int iIndex) const;

    /** Returns the list of main menus. */
    QList<QMenu*> menus() const { return m_mainMenus; }

    /** Returns whether the menu with passed @a type is allowed in menu-bar. */
    bool isAllowedInMenuBar(UIExtraDataMetaDefs::MenuType type) const;
    /** Defines menu-bar @a enmRestriction for passed @a level. */
    void setRestrictionForMenuBar(UIActionRestrictionLevel level, UIExtraDataMetaDefs::MenuType enmRestriction);

    /** Returns whether the action with passed @a type is allowed in the 'Application' menu. */
    bool isAllowedInMenuApplication(UIExtraDataMetaDefs::MenuApplicationActionType type) const;
    /** Defines 'Application' menu @a enmRestriction for passed @a level. */
    void setRestrictionForMenuApplication(UIActionRestrictionLevel level, UIExtraDataMetaDefs::MenuApplicationActionType enmRestriction);

#ifdef VBOX_WS_MAC
    /** Mac OS X: Returns whether the action with passed @a type is allowed in the 'Window' menu. */
    bool isAllowedInMenuWindow(UIExtraDataMetaDefs::MenuWindowActionType type) const;
    /** Mac OS X: Defines 'Window' menu @a enmRestriction for passed @a level. */
    void setRestrictionForMenuWindow(UIActionRestrictionLevel level, UIExtraDataMetaDefs::MenuWindowActionType enmRestriction);
#endif /* VBOX_WS_MAC */

    /** Returns whether the action with passed @a type is allowed in the 'Help' menu. */
    bool isAllowedInMenuHelp(UIExtraDataMetaDefs::MenuHelpActionType enmType) const;
    /** Defines 'Help' menu @a enmRestriction for passed @a level. */
    void setRestrictionForMenuHelp(UIActionRestrictionLevel enmLevel, UIExtraDataMetaDefs::MenuHelpActionType enmRestriction);

    /** Hot-key processing delegate. */
    bool processHotKey(const QKeySequence &key);

    /** Defines whether shortcuts of menu actions with specified @a iIndex should be visible. */
    virtual void setShortcutsVisible(int iIndex, bool fVisible) { Q_UNUSED(iIndex); Q_UNUSED(fVisible); }
    /** Returns extra-data ID to save keyboard shortcuts under. */
    virtual QString shortcutsExtraDataID() const = 0;

public slots:

    /** Handles menu prepare. */
    void sltHandleMenuPrepare();

#ifdef VBOX_WS_MAC
    /** Handles action hovered signal. */
    void sltActionHovered();
#endif

protected slots:

    /** Loads keyboard shortcuts of action-pool into shortcuts-pool. */
    void sltApplyShortcuts() { updateShortcuts(); }

protected:

    /** Constructs probably @a fTemporary action-pool of passed @a enmType. */
    UIActionPool(UIActionPoolType enmType, bool fTemporary = false);

    /** Prepares pool. */
    virtual void preparePool();
    /** Prepares connections. */
    virtual void prepareConnections();
    /** Cleanups connections. */
    virtual void cleanupConnections();
    /** Cleanups pool. */
    virtual void cleanupPool();

    /** Updates configuration. */
    virtual void updateConfiguration();

    /** Updates menu with certain @a iIndex. */
    virtual void updateMenu(int iIndex);
    /** Updates menus. */
    virtual void updateMenus() = 0;

    /** Updates shortcuts. */
    virtual void updateShortcuts();

    /** Handles any Qt @a pEvent */
    virtual bool event(QEvent *pEvent) /* override */;

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /** Adds action into corresponding menu. */
    bool addAction(UIMenu *pMenu, UIAction *pAction, bool fReallyAdd = true);
    /** Adds action's menu into corresponding menu list. */
    bool addMenu(QList<QMenu*> &menuList, UIAction *pAction, bool fReallyAdd = true);

    /** Updates 'Application' menu. */
    void updateMenuApplication();
#ifdef VBOX_WS_MAC
    /** Mac OS X: Updates 'Window' menu. */
    void updateMenuWindow();
#endif
    /** Updates 'Help' menu. */
    void updateMenuHelp();
    /** Updates 'Log Viewer Window' menu. */
    void updateMenuLogViewerWindow();
    /** Updates 'Log Viewer' menu. */
    void updateMenuLogViewer();
    /** Updates 'Log Viewer' @a pMenu. */
    void updateMenuLogViewerWrapper(UIMenu *pMenu);
    /** Updates 'Performance Monitor' menu. */
    void updateMenuPerformanceMonitor();
    /** Updates 'File Manager' menu. */
    void updateMenuFileManager();
    /** Updates 'File Manager' @a pMenu. */
    void updateMenuFileManagerWrapper(UIMenu *pMenu);

    /** Holds the map of actions. */
    QMap<int, UIAction*>          m_pool;
    /** Holds the map of action groups.
      * @note Only menu actions can have action groups. */
    QMap<int, QActionGroup*>      m_groupPool;
    /** Holds the map of validation handlers. */
    QMap<int, PointerToFunction>  m_menuUpdateHandlers;
    /** Holds the set of invalidated action indexes. */
    QSet<int>                     m_invalidations;

    /** Holds the list of main menus. */
    QList<QMenu*>  m_mainMenus;

    /** Holds restricted menu types. */
    QMap<UIActionRestrictionLevel, UIExtraDataMetaDefs::MenuType>                   m_restrictedMenus;
    /** Holds restricted action types of the 'Application' menu. */
    QMap<UIActionRestrictionLevel, UIExtraDataMetaDefs::MenuApplicationActionType>  m_restrictedActionsMenuApplication;
#ifdef VBOX_WS_MAC
    /** Mac OS X: Holds restricted action types of the 'Window' menu. */
    QMap<UIActionRestrictionLevel, UIExtraDataMetaDefs::MenuWindowActionType>       m_restrictedActionsMenuWindow;
#endif
    /** Holds restricted action types of the Help menu. */
    QMap<UIActionRestrictionLevel, UIExtraDataMetaDefs::MenuHelpActionType>         m_restrictedActionsMenuHelp;

private:

    /** Prepares all. */
    void prepare();
    /** Cleanups all. */
    void cleanup();

    /** Holds the action-pool type. */
    const UIActionPoolType  m_enmType;
    /** Holds whether this action-pool is temporary. */
    const bool              m_fTemporary;
};


#endif /* !FEQT_INCLUDED_SRC_globals_UIActionPool_h */

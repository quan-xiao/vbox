/* $Id: UIActionPool.cpp 85907 2020-08-27 14:38:18Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIActionPool class implementation.
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
#include <QHelpEvent>
#include <QToolTip>

/* GUI includes: */
#include "UICommon.h"
#include "UIActionPool.h"
#include "UIActionPoolManager.h"
#include "UIActionPoolRuntime.h"
#include "UIConverter.h"
#include "UIIconPool.h"
#include "UIMessageCenter.h"
#include "UIShortcutPool.h"
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
# include "UIExtraDataManager.h"
# include "UINetworkManager.h"
# include "UIUpdateManager.h"
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */


/** QEvent extension
  * representing action-activation event. */
class ActivateActionEvent : public QEvent
{
public:

    /** Constructs @a pAction event. */
    ActivateActionEvent(QAction *pAction)
        : QEvent((QEvent::Type)ActivateActionEventType)
        , m_pAction(pAction)
    {}

    /** Returns the action this event corresponds to. */
    QAction *action() const { return m_pAction; }

private:

    /** Holds the action this event corresponds to. */
    QAction *m_pAction;
};


/*********************************************************************************************************************************
*   Class UIMenu implementation.                                                                                                 *
*********************************************************************************************************************************/

UIMenu::UIMenu()
    : m_fShowToolTip(false)
#ifdef VBOX_WS_MAC
    , m_fConsumable(false)
    , m_fConsumed(false)
#endif
{
}

bool UIMenu::event(QEvent *pEvent)
{
    /* Handle particular event-types: */
    switch (pEvent->type())
    {
        /* Tool-tip request handler: */
        case QEvent::ToolTip:
        {
            /* Get current help-event: */
            QHelpEvent *pHelpEvent = static_cast<QHelpEvent*>(pEvent);
            /* Get action which caused help-event: */
            QAction *pAction = actionAt(pHelpEvent->pos());
            /* If action present => show action's tool-tip if needed: */
            if (pAction && m_fShowToolTip)
                QToolTip::showText(pHelpEvent->globalPos(), pAction->toolTip());
            break;
        }
        default:
            break;
    }
    /* Call to base-class: */
    return QMenu::event(pEvent);
}


/*********************************************************************************************************************************
*   Class UIAction implementation.                                                                                               *
*********************************************************************************************************************************/

UIAction::UIAction(UIActionPool *pParent, UIActionType enmType, bool fMachineMenuAction /* = false */)
    : QAction(pParent)
    , m_pActionPool(pParent)
    , m_enmActionPoolType(pParent->type())
    , m_enmType(enmType)
    , m_fMachineMenuAction(fMachineMenuAction)
    , m_iState(0)
    , m_fShortcutHidden(false)
{
    /* By default there is no specific menu role.
     * It will be set explicitly later. */
    setMenuRole(QAction::NoRole);

#ifdef VBOX_WS_MAC
    /* Make sure each action notifies it's parent about hovering: */
    connect(this, &UIAction::hovered,
            static_cast<UIActionPool*>(parent()), &UIActionPool::sltActionHovered);
#endif
}

UIMenu *UIAction::menu() const
{
    return QAction::menu() ? qobject_cast<UIMenu*>(QAction::menu()) : 0;
}

void UIAction::setState(int iState)
{
    m_iState = iState;
    updateIcon();
    retranslateUi();
    handleStateChange();
}

void UIAction::setIcon(int iState, const QIcon &icon)
{
    m_icons.resize(iState + 1);
    m_icons[iState] = icon;
    updateIcon();
}

void UIAction::setIcon(const QIcon &icon)
{
    setIcon(0, icon);
}

void UIAction::setName(const QString &strName)
{
    /* Remember internal name: */
    m_strName = strName;
    /* Update text according new name: */
    updateText();
}

void UIAction::setShortcuts(const QList<QKeySequence> &shortcuts)
{
    /* Only for manager's action-pool: */
    if (m_enmActionPoolType == UIActionPoolType_Manager)
    {
        /* If primary shortcut should be visible: */
        if (!m_fShortcutHidden)
            /* Call to base-class: */
            QAction::setShortcuts(shortcuts);
        /* Remember shortcuts: */
        m_shortcuts = shortcuts;
    }
    /* Update text according to new primary shortcut: */
    updateText();
}

void UIAction::showShortcut()
{
    m_fShortcutHidden = false;
    if (!m_shortcuts.isEmpty())
        QAction::setShortcuts(m_shortcuts);
}

void UIAction::hideShortcut()
{
    m_fShortcutHidden = true;
    if (!shortcut().isEmpty())
        QAction::setShortcuts(QList<QKeySequence>());
}

QString UIAction::nameInMenu() const
{
    /* Action-name format depends on action-pool type: */
    switch (m_enmActionPoolType)
    {
        /* Unchanged name for Manager UI: */
        case UIActionPoolType_Manager: return name();
        /* Filtered name for Runtime UI: */
        case UIActionPoolType_Runtime: return UICommon::removeAccelMark(name());
    }
    /* Nothing by default: */
    return QString();
}

void UIAction::updateIcon()
{
    QAction::setIcon(m_icons.value(m_iState, m_icons.value(0)));
}

void UIAction::updateText()
{
    /* First of all, action-text depends on action type: */
    switch (m_enmType)
    {
        case UIActionType_Menu:
        {
            /* For menu types it's very easy: */
            setText(nameInMenu());
            break;
        }
        default:
        {
            /* For rest of action types it depends on action-pool type: */
            switch (m_enmActionPoolType)
            {
                /* The same as menu name for Manager UI: */
                case UIActionPoolType_Manager:
                {
                    setText(nameInMenu());
                    break;
                }
                /* With shortcut appended for Runtime UI: */
                case UIActionPoolType_Runtime:
                {
                    if (m_fMachineMenuAction)
                        setText(uiCommon().insertKeyToActionText(nameInMenu(),
                                                                 gShortcutPool->shortcut(actionPool(), this).primaryToPortableText()));
                    else
                        setText(nameInMenu());
                    break;
                }
            }
            break;
        }
    }
}

/* static */
QString UIAction::simplifyText(QString strText)
{
    return strText.remove('.').remove('&');
}


/*********************************************************************************************************************************
*   Class UIActionMenu implementation.                                                                                           *
*********************************************************************************************************************************/

UIActionMenu::UIActionMenu(UIActionPool *pParent,
                           const QString &strIcon, const QString &strIconDisabled)
    : UIAction(pParent, UIActionType_Menu)
    , m_pMenu(0)
{
    if (!strIcon.isNull())
        setIcon(UIIconPool::iconSet(strIcon, strIconDisabled));
    prepare();
}

UIActionMenu::UIActionMenu(UIActionPool *pParent,
                           const QString &strIconNormal, const QString &strIconSmall,
                           const QString &strIconNormalDisabled, const QString &strIconSmallDisabled)
    : UIAction(pParent, UIActionType_Menu)
    , m_pMenu(0)
{
    if (!strIconNormal.isNull())
        setIcon(UIIconPool::iconSetFull(strIconNormal, strIconSmall, strIconNormalDisabled, strIconSmallDisabled));
    prepare();
}

UIActionMenu::UIActionMenu(UIActionPool *pParent,
                           const QIcon &icon)
    : UIAction(pParent, UIActionType_Menu)
    , m_pMenu(0)
{
    if (!icon.isNull())
        setIcon(icon);
    prepare();
}

UIActionMenu::~UIActionMenu()
{
    /* Hide menu: */
    hideMenu();
    /* Delete menu: */
    delete m_pMenu;
    m_pMenu = 0;
}

void UIActionMenu::setShowToolTip(bool fShowToolTip)
{
    AssertPtrReturnVoid(m_pMenu);
    m_pMenu->setShowToolTip(fShowToolTip);
}

void UIActionMenu::showMenu()
{
    /* Show menu if necessary: */
    if (!menu())
        setMenu(m_pMenu);
}

void UIActionMenu::hideMenu()
{
    /* Hide menu if necessary: */
    if (menu())
        setMenu(0);
}

void UIActionMenu::prepare()
{
    /* Create menu: */
    m_pMenu = new UIMenu;
    AssertPtrReturnVoid(m_pMenu);
    {
        /* Prepare menu: */
        connect(m_pMenu, &UIMenu::aboutToShow,
                actionPool(), &UIActionPool::sltHandleMenuPrepare);
        /* Show menu: */
        showMenu();
    }
}


/*********************************************************************************************************************************
*   Class UIActionSimple implementation.                                                                                         *
*********************************************************************************************************************************/

UIActionSimple::UIActionSimple(UIActionPool *pParent,
                               bool fMachineMenuAction /* = false */)
    : UIAction(pParent, UIActionType_Simple, fMachineMenuAction)
{
}

UIActionSimple::UIActionSimple(UIActionPool *pParent,
                               const QString &strIcon, const QString &strIconDisabled,
                               bool fMachineMenuAction /* = false */)
    : UIAction(pParent, UIActionType_Simple, fMachineMenuAction)
{
    if (!strIcon.isNull())
        setIcon(UIIconPool::iconSet(strIcon, strIconDisabled));
}

UIActionSimple::UIActionSimple(UIActionPool *pParent,
                               const QString &strIconNormal, const QString &strIconSmall,
                               const QString &strIconNormalDisabled, const QString &strIconSmallDisabled,
                               bool fMachineMenuAction /* = false */)
    : UIAction(pParent, UIActionType_Simple, fMachineMenuAction)
{
    if (!strIconNormal.isNull())
        setIcon(UIIconPool::iconSetFull(strIconNormal, strIconSmall, strIconNormalDisabled, strIconSmallDisabled));
}

UIActionSimple::UIActionSimple(UIActionPool *pParent,
                               const QIcon &icon,
                               bool fMachineMenuAction /* = false */)
    : UIAction(pParent, UIActionType_Simple, fMachineMenuAction)
{
    if (!icon.isNull())
        setIcon(icon);
}


/*********************************************************************************************************************************
*   Class UIActionToggle implementation.                                                                                         *
*********************************************************************************************************************************/

UIActionToggle::UIActionToggle(UIActionPool *pParent,
                               bool fMachineMenuAction /* = false */)
    : UIAction(pParent, UIActionType_Toggle, fMachineMenuAction)
{
    prepare();
}

UIActionToggle::UIActionToggle(UIActionPool *pParent,
                               const QString &strIcon, const QString &strIconDisabled,
                               bool fMachineMenuAction /* = false */)
    : UIAction(pParent, UIActionType_Toggle, fMachineMenuAction)
{
    if (!strIcon.isNull())
        setIcon(UIIconPool::iconSet(strIcon, strIconDisabled));
    prepare();
}

UIActionToggle::UIActionToggle(UIActionPool *pParent,
                               const QString &strIconOn, const QString &strIconOff,
                               const QString &strIconOnDisabled, const QString &strIconOffDisabled,
                               bool fMachineMenuAction /* = false */)
    : UIAction(pParent, UIActionType_Toggle, fMachineMenuAction)
{
    if (!strIconOn.isNull())
        setIcon(UIIconPool::iconSetOnOff(strIconOn, strIconOff, strIconOnDisabled, strIconOffDisabled));
    prepare();
}

UIActionToggle::UIActionToggle(UIActionPool *pParent,
                               const QIcon &icon,
                               bool fMachineMenuAction /* = false */)
    : UIAction(pParent, UIActionType_Toggle, fMachineMenuAction)
{
    if (!icon.isNull())
        setIcon(icon);
    prepare();
}

void UIActionToggle::prepare()
{
    setCheckable(true);
}


/** Menu action extension, used as 'Application' menu class. */
class UIActionMenuApplication : public UIActionMenu
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuApplication(UIActionPool *pParent)
        : UIActionMenu(pParent)
    {
#ifdef VBOX_WS_MAC
        menu()->setConsumable(true);
#endif
        retranslateUi();
    }

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const /* override */
    {
        return UIExtraDataMetaDefs::MenuType_Application;
    }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const /* override */
    {
        return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuType_Application);
    }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const /* override */
    {
        return actionPool()->isAllowedInMenuBar(UIExtraDataMetaDefs::MenuType_Application);
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
#ifdef VBOX_WS_MAC
        setName(QApplication::translate("UIActionPool", "&VirtualBox"));
#else
        setName(QApplication::translate("UIActionPool", "&File"));
#endif
    }
};

/** Simple action extension, used as 'Close' action class. */
class UIActionSimplePerformClose : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionSimplePerformClose(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/exit_16px.png", ":/exit_16px.png", true)
    {
        setMenuRole(QAction::QuitRole);
    }

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const /* override */
    {
        return UIExtraDataMetaDefs::MenuApplicationActionType_Close;
    }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const /* override */
    {
        return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuApplicationActionType_Close);
    }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const /* override */
    {
        return actionPool()->isAllowedInMenuApplication(UIExtraDataMetaDefs::MenuApplicationActionType_Close);
    }

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("Close");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType actionPoolType) const /* override */
    {
        switch (actionPoolType)
        {
            case UIActionPoolType_Manager: break;
            case UIActionPoolType_Runtime: return QKeySequence("Q");
        }
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&Close..."));
        setStatusTip(QApplication::translate("UIActionPool", "Close the virtual machine"));
    }
};

#ifdef VBOX_WS_MAC
/** Menu action extension, used as 'Window' menu class. */
class UIActionMenuWindow : public UIActionMenu
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuWindow(UIActionPool *pParent)
        : UIActionMenu(pParent)
    {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const /* override */
    {
        return UIExtraDataMetaDefs::MenuType_Window;
    }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const /* override */
    {
        return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuType_Window);
    }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const /* override */
    {
        return actionPool()->isAllowedInMenuBar(UIExtraDataMetaDefs::MenuType_Window);
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&Window"));
    }
};

/** Simple action extension, used as 'Minimize' action class. */
class UIActionSimpleMinimize : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionSimpleMinimize(UIActionPool *pParent)
        : UIActionSimple(pParent)
    {}

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const /* override */
    {
        return UIExtraDataMetaDefs::MenuWindowActionType_Minimize;
    }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const /* override */
    {
        return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuWindowActionType_Minimize);
    }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const /* override */
    {
        return actionPool()->isAllowedInMenuWindow(UIExtraDataMetaDefs::MenuWindowActionType_Minimize);
    }

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("Minimize");
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&Minimize"));
        setStatusTip(QApplication::translate("UIActionPool", "Minimize active window"));
    }
};
#endif /* VBOX_WS_MAC */

/** Menu action extension, used as 'Help' menu class. */
class UIActionMenuHelp : public UIActionMenu
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuHelp(UIActionPool *pParent)
        : UIActionMenu(pParent)
    {
        retranslateUi();
    }

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const /* override */
    {
        return UIExtraDataMetaDefs::MenuType_Help;
    }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const /* override */
    {
        return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuType_Help);
    }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const /* override */
    {
        return actionPool()->isAllowedInMenuBar(UIExtraDataMetaDefs::MenuType_Help);
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&Help"));
    }
};

/** Simple action extension, used as 'Contents' action class. */
class UIActionSimpleContents : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionSimpleContents(UIActionPool *pParent)
        : UIActionSimple(pParent, UIIconPool::defaultIcon(UIIconPool::UIDefaultIconType_DialogHelp), true)
    {
        retranslateUi();
    }

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const /* override */
    {
        return UIExtraDataMetaDefs::MenuHelpActionType_Contents;
    }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const /* override */
    {
        return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuHelpActionType_Contents);
    }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const /* override */
    {
        return actionPool()->isAllowedInMenuHelp(UIExtraDataMetaDefs::MenuHelpActionType_Contents);
    }

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("Help");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType actionPoolType) const /* override */
    {
        switch (actionPoolType)
        {
            case UIActionPoolType_Manager: return QKeySequence(QKeySequence::HelpContents);
            case UIActionPoolType_Runtime: break;
        }
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&Contents..."));
        setStatusTip(QApplication::translate("UIActionPool", "Show help contents"));
    }
};

/** Simple action extension, used as 'Web Site' action class. */
class UIActionSimpleWebSite : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionSimpleWebSite(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/site_16px.png", ":/site_16px.png", true)
    {
        retranslateUi();
    }

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const /* override */
    {
        return UIExtraDataMetaDefs::MenuHelpActionType_WebSite;
    }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const /* override */
    {
        return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuHelpActionType_WebSite);
    }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const /* override */
    {
        return actionPool()->isAllowedInMenuHelp(UIExtraDataMetaDefs::MenuHelpActionType_WebSite);
    }

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("Web");
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&VirtualBox Web Site..."));
        setStatusTip(QApplication::translate("UIActionPool", "Open the browser and go to the VirtualBox product web site"));
    }
};

/** Simple action extension, used as 'Bug Tracker' action class. */
class UIActionSimpleBugTracker : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionSimpleBugTracker(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/site_bugtracker_16px.png", ":/site_bugtracker_16px.png", true)
    {
        retranslateUi();
    }

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const /* override */
    {
        return UIExtraDataMetaDefs::MenuHelpActionType_BugTracker;
    }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const /* override */
    {
        return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuHelpActionType_BugTracker);
    }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const /* override */
    {
        return actionPool()->isAllowedInMenuHelp(UIExtraDataMetaDefs::MenuHelpActionType_BugTracker);
    }

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("BugTracker");
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&VirtualBox Bug Tracker..."));
        setStatusTip(QApplication::translate("UIActionPool", "Open the browser and go to the VirtualBox product bug tracker"));
    }
};

/** Simple action extension, used as 'Forums' action class. */
class UIActionSimpleForums : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionSimpleForums(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/site_forum_16px.png", ":/site_forum_16px.png", true)
    {
        retranslateUi();
    }

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const /* override */
    {
        return UIExtraDataMetaDefs::MenuHelpActionType_Forums;
    }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const /* override */
    {
        return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuHelpActionType_Forums);
    }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const /* override */
    {
        return actionPool()->isAllowedInMenuHelp(UIExtraDataMetaDefs::MenuHelpActionType_Forums);
    }

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("Forums");
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&VirtualBox Forums..."));
        setStatusTip(QApplication::translate("UIActionPool", "Open the browser and go to the VirtualBox product forums"));
    }
};

/** Simple action extension, used as 'Oracle' action class. */
class UIActionSimpleOracle : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionSimpleOracle(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/site_oracle_16px.png", ":/site_oracle_16px.png", true)
    {
        retranslateUi();
    }

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const /* override */
    {
        return UIExtraDataMetaDefs::MenuHelpActionType_Oracle;
    }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const /* override */
    {
        return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuHelpActionType_Oracle);
    }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const /* override */
    {
        return actionPool()->isAllowedInMenuHelp(UIExtraDataMetaDefs::MenuHelpActionType_Oracle);
    }

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("Oracle");
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&Oracle Web Site..."));
        setStatusTip(QApplication::translate("UIActionPool", "Open the browser and go to the Oracle web site"));
    }
};

/** Simple action extension, used as 'Reset Warnings' action class. */
class UIActionSimpleResetWarnings : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionSimpleResetWarnings(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/reset_warnings_16px.png", ":/reset_warnings_16px.png", true)
    {
        setMenuRole(QAction::ApplicationSpecificRole);
        retranslateUi();
    }

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const /* override */
    {
        return UIExtraDataMetaDefs::MenuApplicationActionType_ResetWarnings;
    }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const /* override */
    {
        return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuApplicationActionType_ResetWarnings);
    }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const /* override */
    {
        return actionPool()->isAllowedInMenuApplication(UIExtraDataMetaDefs::MenuApplicationActionType_ResetWarnings);
    }

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("ResetWarnings");
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&Reset All Warnings"));
        setStatusTip(QApplication::translate("UIActionPool", "Go back to showing all suppressed warnings and messages"));
    }
};

#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
/** Simple action extension, used as 'Network Access Manager' action class. */
class UIActionSimpleNetworkAccessManager : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionSimpleNetworkAccessManager(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/download_manager_16px.png", ":/download_manager_16px.png", true)
    {
        setMenuRole(QAction::ApplicationSpecificRole);
        retranslateUi();
    }

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const /* override */
    {
        return UIExtraDataMetaDefs::MenuApplicationActionType_NetworkAccessManager;
    }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const /* override */
    {
        return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuApplicationActionType_NetworkAccessManager);
    }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const /* override */
    {
        return actionPool()->isAllowedInMenuApplication(UIExtraDataMetaDefs::MenuApplicationActionType_NetworkAccessManager);
    }

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("NetworkAccessManager");
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&Network Operations Manager..."));
        setStatusTip(QApplication::translate("UIActionPool", "Display the Network Operations Manager window"));
    }
};

/** Simple action extension, used as 'Check for Updates' action class. */
class UIActionSimpleCheckForUpdates : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionSimpleCheckForUpdates(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/refresh_16px.png", ":/refresh_disabled_16px.png", true)
    {
        setMenuRole(QAction::ApplicationSpecificRole);
        retranslateUi();
    }

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const /* override */
    {
        return UIExtraDataMetaDefs::MenuApplicationActionType_CheckForUpdates;
    }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const /* override */
    {
        return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuApplicationActionType_CheckForUpdates);
    }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const /* override */
    {
        return actionPool()->isAllowedInMenuApplication(UIExtraDataMetaDefs::MenuApplicationActionType_CheckForUpdates);
    }

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("Update");
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "C&heck for Updates..."));
        setStatusTip(QApplication::translate("UIActionPool", "Check for a new VirtualBox version"));
    }
};
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */

/** Simple action extension, used as 'About' action class. */
class UIActionSimpleAbout : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionSimpleAbout(UIActionPool *pParent)
        : UIActionSimple(pParent, ":/about_16px.png", ":/about_16px.png", true)
    {
        setMenuRole(QAction::AboutRole);
        retranslateUi();
    }

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const /* override */
    {
#ifdef VBOX_WS_MAC
        return UIExtraDataMetaDefs::MenuApplicationActionType_About;
#else
        return UIExtraDataMetaDefs::MenuHelpActionType_About;
#endif
    }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const /* override */
    {
#ifdef VBOX_WS_MAC
        return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuApplicationActionType_About);
#else
        return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuHelpActionType_About);
#endif
    }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const /* override */
    {
#ifdef VBOX_WS_MAC
        return actionPool()->isAllowedInMenuApplication(UIExtraDataMetaDefs::MenuApplicationActionType_About);
#else
        return actionPool()->isAllowedInMenuHelp(UIExtraDataMetaDefs::MenuHelpActionType_About);
#endif
    }

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("About");
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&About VirtualBox..."));
        setStatusTip(QApplication::translate("UIActionPool", "Display a window with product information"));
    }
};

/** Simple action extension, used as 'Preferences' action class. */
class UIActionSimplePreferences : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionSimplePreferences(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/global_settings_32px.png", ":/global_settings_16px.png",
                         ":/global_settings_disabled_32px.png", ":/global_settings_disabled_16px.png",
                         true)
    {
        setMenuRole(QAction::PreferencesRole);
        retranslateUi();
    }

protected:

    /** Returns action extra-data ID. */
    virtual int extraDataID() const /* override */
    {
        return UIExtraDataMetaDefs::MenuApplicationActionType_Preferences;
    }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const /* override */
    {
        return gpConverter->toInternalString(UIExtraDataMetaDefs::MenuApplicationActionType_Preferences);
    }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const /* override */
    {
        return actionPool()->isAllowedInMenuApplication(UIExtraDataMetaDefs::MenuApplicationActionType_Preferences);
    }

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("Preferences");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        switch (actionPool()->type())
        {
            case UIActionPoolType_Manager: return QKeySequence("Ctrl+G");
            case UIActionPoolType_Runtime: break;
        }
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&Preferences...", "global preferences window"));
        setStatusTip(QApplication::translate("UIActionPool", "Display the global preferences window"));
        setToolTip(  QApplication::translate("UIActionPool", "Display Global Preferences")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Menu action extension, used as 'Log' menu class. */
class UIActionMenuSelectorLog : public UIActionMenu
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuSelectorLog(UIActionPool *pParent)
        : UIActionMenu(pParent)
    {}

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("LogViewerMenu");
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&Log"));
    }
};

/** Simple action extension, used as 'Toggle Pane Find' action class. */
class UIActionMenuSelectorLogTogglePaneFind : public UIActionToggle
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuSelectorLogTogglePaneFind(UIActionPool *pParent)
        : UIActionToggle(pParent)
    {
        setShortcutContext(Qt::WidgetWithChildrenShortcut);
        setIcon(UIIconPool::iconSetFull(":/log_viewer_find_32px.png",          ":/log_viewer_find_16px.png",
                                        ":/log_viewer_find_disabled_32px.png", ":/log_viewer_find_disabled_16px.png"));
    }

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("ToggleLogFind");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence("Ctrl+Shift+F");
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&Find"));
        setShortcutScope(QApplication::translate("UIActionPool", "Log Viewer"));
        setStatusTip(QApplication::translate("UIActionPool", "Open pane with searching options"));
        setToolTip(  QApplication::translate("UIActionPool", "Open Find Pane")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Simple action extension, used as 'Toggle Pane Filter' action class. */
class UIActionMenuSelectorLogTogglePaneFilter : public UIActionToggle
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuSelectorLogTogglePaneFilter(UIActionPool *pParent)
        : UIActionToggle(pParent)
    {
        setShortcutContext(Qt::WidgetWithChildrenShortcut);
        setIcon(UIIconPool::iconSetFull(":/log_viewer_filter_32px.png",          ":/log_viewer_filter_16px.png",
                                        ":/log_viewer_filter_disabled_32px.png", ":/log_viewer_filter_disabled_16px.png"));
    }

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("ToggleLogFilter");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence("Ctrl+Shift+T");
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&Filter"));
        setShortcutScope(QApplication::translate("UIActionPool", "Log Viewer"));
        setStatusTip(QApplication::translate("UIActionPool", "Open pane with filtering options"));
        setToolTip(  QApplication::translate("UIActionPool", "Open Filter Pane")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Simple action extension, used as 'Toggle Pane Bookmark' action class. */
class UIActionMenuSelectorLogTogglePaneBookmark : public UIActionToggle
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuSelectorLogTogglePaneBookmark(UIActionPool *pParent)
        : UIActionToggle(pParent)
    {
        setShortcutContext(Qt::WidgetWithChildrenShortcut);
        setIcon(UIIconPool::iconSetFull(":/log_viewer_bookmark_32px.png",          ":/log_viewer_bookmark_16px.png",
                                        ":/log_viewer_bookmark_disabled_32px.png", ":/log_viewer_bookmark_disabled_16px.png"));
    }

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("ToggleLogBookmark");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence("Ctrl+Shift+D");
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&Bookmark"));
        setShortcutScope(QApplication::translate("UIActionPool", "Log Viewer"));
        setStatusTip(QApplication::translate("UIActionPool", "Open pane with bookmarking options"));
        setToolTip(  QApplication::translate("UIActionPool", "Open Bookmark Pane")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Simple action extension, used as 'Toggle Pane Options' action class. */
class UIActionMenuSelectorLogTogglePaneOptions : public UIActionToggle
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuSelectorLogTogglePaneOptions(UIActionPool *pParent)
        : UIActionToggle(pParent)
    {
        setShortcutContext(Qt::WidgetWithChildrenShortcut);
        setIcon(UIIconPool::iconSetFull(":/log_viewer_options_32px.png",          ":/log_viewer_options_16px.png",
                                        ":/log_viewer_options_disabled_32px.png", ":/log_viewer_options_disabled_16px.png"));
    }

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("ToggleLogOptions");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence("Ctrl+Shift+P");
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&Options"));
        setShortcutScope(QApplication::translate("UIActionPool", "Log Viewer"));
        setStatusTip(QApplication::translate("UIActionPool", "Open pane with log viewer options"));
        setToolTip(  QApplication::translate("UIActionPool", "Open Options Pane")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Simple action extension, used as 'Perform Refresh' action class. */
class UIActionMenuSelectorLogPerformRefresh : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuSelectorLogPerformRefresh(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/log_viewer_refresh_32px.png", ":/log_viewer_refresh_16px.png",
                         ":/log_viewer_refresh_disabled_32px.png", ":/log_viewer_refresh_disabled_16px.png")
    {
        setShortcutContext(Qt::WidgetWithChildrenShortcut);
    }

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("RefreshLog");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence("Ctrl+Shift+R");
    }

    /** Returns standard shortcut. */
    virtual QKeySequence standardShortcut(UIActionPoolType) const /* override */
    {
        return actionPool()->isTemporary() ? QKeySequence() : QKeySequence(QKeySequence::Refresh);
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&Refresh"));
        setShortcutScope(QApplication::translate("UIActionPool", "Log Viewer"));
        setStatusTip(QApplication::translate("UIActionPool", "Refresh selected virtual machine log"));
        setToolTip(  QApplication::translate("UIActionPool", "Refresh Virtual Machine Log")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Simple action extension, used as 'Perform Save' action class. */
class UIActionMenuSelectorLogPerformSave : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuSelectorLogPerformSave(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/log_viewer_save_32px.png", ":/log_viewer_save_16px.png",
                         ":/log_viewer_save_disabled_32px.png", ":/log_viewer_save_disabled_16px.png")
    {
        setShortcutContext(Qt::WidgetWithChildrenShortcut);
    }

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("SaveLog");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence("Ctrl+Shift+S");
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&Save..."));
        setShortcutScope(QApplication::translate("UIActionPool", "Log Viewer"));
        setStatusTip(QApplication::translate("UIActionPool", "Save selected virtual machine log"));
        setToolTip(  QApplication::translate("UIActionPool", "Save Virtual Machine Log")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Menu action extension, used as 'File Manager' menu class. */
class UIActionMenuFileManager : public UIActionMenu
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManager(UIActionPool *pParent)
        : UIActionMenu(pParent)
    {}

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("FileManagerMenu");
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "File Manager"));
    }
};

class UIActionMenuFileManagerHostSubmenu : public UIActionMenu
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManagerHostSubmenu(UIActionPool *pParent)
        : UIActionMenu(pParent)
    {}

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("FileManagerHostSubmenu");
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "Host"));
    }
};

class UIActionMenuFileManagerGuestSubmenu : public UIActionMenu
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManagerGuestSubmenu(UIActionPool *pParent)
        : UIActionMenu(pParent)
    {}

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("FileManagerGuestSubmenu");
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "Guest"));
    }
};

/** Simple action extension, used as 'Copy to Guest' in file manager action class. */
class UIActionMenuFileManagerCopyToGuest : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManagerCopyToGuest(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/file_manager_copy_to_guest_24px.png", ":/file_manager_copy_to_guest_16px.png",
                         ":/file_manager_copy_to_guest_disabled_24px.png", ":/file_manager_copy_to_guest_disabled_16px.png"){}

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("FileManagerCopyToGuest");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "Copy to guest"));
        setShortcutScope(QApplication::translate("UIActionPool", "File Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Copy the selected object(s) from host to guest"));
        setToolTip(  QApplication::translate("UIActionPool", "Copy from Host to Guest")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Simple action extension, used as 'Copy to Host' in file manager action class. */
class UIActionMenuFileManagerCopyToHost : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManagerCopyToHost(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/file_manager_copy_to_host_24px.png", ":/file_manager_copy_to_host_16px.png",
                         ":/file_manager_copy_to_host_disabled_24px.png", ":/file_manager_copy_to_host_disabled_16px.png"){}

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("FileManagerCopyToHost");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "Copy to host"));
        setShortcutScope(QApplication::translate("UIActionPool", "File Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Copy the selected object(s) from guest to host"));
        setToolTip(  QApplication::translate("UIActionPool", "Copy from Guest to Host")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Toggle action extension, used to toggle 'File Manager Options' panel in file manager. */
class UIActionMenuFileManagerOptions : public UIActionToggle
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManagerOptions(UIActionPool *pParent)
        : UIActionToggle(pParent)
    {
        setShortcutContext(Qt::WidgetWithChildrenShortcut);
        setIcon(UIIconPool::iconSetFull(":/file_manager_options_32px.png",          ":/file_manager_options_16px.png",
                                        ":/file_manager_options_disabled_32px.png", ":/file_manager_options_disabled_16px.png"));
    }

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("ToggleFileManagerOptionsPanel");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "Options"));
        setShortcutScope(QApplication::translate("UIActionPool", "File Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Open panel with file manager options"));
        setToolTip(  QApplication::translate("UIActionPool", "Open Options Pane")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Toggle action extension, used to toggle 'File Manager Log' panel in file manager. */
class UIActionMenuFileManagerLog : public UIActionToggle
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManagerLog(UIActionPool *pParent)
        : UIActionToggle(pParent)
    {
        setShortcutContext(Qt::WidgetWithChildrenShortcut);
        setIcon(UIIconPool::iconSetFull(":/file_manager_log_32px.png",          ":/file_manager_log_16px.png",
                                        ":/file_manager_log_disabled_32px.png", ":/file_manager_log_disabled_16px.png"));
    }

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("ToggleFileManagerLogPanel");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "Log"));
        setShortcutScope(QApplication::translate("UIActionPool", "File Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Open panel with file manager log"));
        setToolTip(  QApplication::translate("UIActionPool", "Open Log Pane")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Toggle action extension, used to toggle 'File Manager Operations' panel in file manager. */
class UIActionMenuFileManagerOperations : public UIActionToggle
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManagerOperations(UIActionPool *pParent)
        : UIActionToggle(pParent)
    {
        setShortcutContext(Qt::WidgetWithChildrenShortcut);
        setIcon(UIIconPool::iconSetFull(":/file_manager_operations_32px.png",          ":/file_manager_operations_16px.png",
                                        ":/file_manager_operations_disabled_32px.png", ":/file_manager_operations_disabled_16px.png"));
    }

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("ToggleFileManagerOperationsPanel");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "Operations"));
        setShortcutScope(QApplication::translate("UIActionPool", "File Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Open panel with file manager operations"));
        setToolTip(  QApplication::translate("UIActionPool", "Open Operations Pane")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Toggle action extension, used to toggle 'File Manager Session' panel in file manager. */
class UIActionMenuFileManagerSession : public UIActionToggle
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManagerSession(UIActionPool *pParent)
        : UIActionToggle(pParent)
    {
        setShortcutContext(Qt::WidgetWithChildrenShortcut);
        setIcon(UIIconPool::iconSetFull(":/file_manager_session_32px.png",          ":/file_manager_session_16px.png",
                                        ":/file_manager_session_disabled_32px.png", ":/file_manager_session_disabled_16px.png"));
    }

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("ToggleFileManagerSessionPanel");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "Session"));
        setShortcutScope(QApplication::translate("UIActionPool", "File Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Open panel with file manager session"));
        setToolTip(  QApplication::translate("UIActionPool", "Open Session Pane")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Simple action extension, used as 'Perform GoUp' in file manager action class. */
class UIActionMenuFileManagerGoUp : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManagerGoUp(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/file_manager_go_up_24px.png", ":/file_manager_go_up_16px.png",
                         ":/file_manager_go_up_disabled_24px.png", ":/file_manager_go_up_disabled_16px.png")
    {}

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("FileManagerGoUp");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "Go Up"));
        setShortcutScope(QApplication::translate("UIActionPool", "File Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Go one level up to parent folder"));
        setToolTip(  QApplication::translate("UIActionPool", "Go One Level Up")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Simple action extension, used as 'Perform GoHome' in file manager action class. */
class UIActionMenuFileManagerGoHome : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManagerGoHome(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/file_manager_go_home_24px.png", ":/file_manager_go_home_16px.png",
                         ":/file_manager_go_home_disabled_24px.png", ":/file_manager_go_home_disabled_16px.png")
    {}

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("FileManagerGoHome");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "Go Home"));
        setShortcutScope(QApplication::translate("UIActionPool", "File Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Go to home folder"));
        setToolTip(  QApplication::translate("UIActionPool", "Go to Home Folder")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Simple action extension, used as 'Perform Delete' in file manager action class. */
class UIActionMenuFileManagerDelete : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManagerDelete(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/file_manager_delete_24px.png", ":/file_manager_delete_16px.png",
                         ":/file_manager_delete_disabled_24px.png", ":/file_manager_delete_disabled_16px.png")
    {}

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("FileManagerDelete");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "Delete"));
        setShortcutScope(QApplication::translate("UIActionPool", "File Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Delete selected file object(s)"));
        setToolTip(  QApplication::translate("UIActionPool", "Delete Selected Object(s)")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Simple action extension, used as 'Perform Refresh' in file manager action class. */
class UIActionMenuFileManagerRefresh : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManagerRefresh(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/file_manager_refresh_24px.png", ":/file_manager_refresh_16px.png",
                         ":/file_manager_refresh_disabled_24px.png", ":/file_manager_refresh_disabled_16px.png")
    {}

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("FileManagerRefresh");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "Refresh"));
        setShortcutScope(QApplication::translate("UIActionPool", "File Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Refresh"));
        setToolTip(  QApplication::translate("UIActionPool", "Refresh Contents")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Simple action extension, used as 'Perform Rename' in file manager action class. */
class UIActionMenuFileManagerRename : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManagerRename(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/file_manager_rename_24px.png", ":/file_manager_rename_16px.png",
                         ":/file_manager_rename_disabled_24px.png", ":/file_manager_rename_disabled_16px.png"){}

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("FileManagerRename");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "Rename"));
        setShortcutScope(QApplication::translate("UIActionPool", "File Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Rename selected file object"));
        setToolTip(  QApplication::translate("UIActionPool", "Rename Selected Object")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Simple action extension, used as 'Perform Rename' in file manager action class. */
class UIActionMenuFileManagerCreateNewDirectory : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManagerCreateNewDirectory(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/file_manager_new_directory_24px.png", ":/file_manager_new_directory_16px.png",
                         ":/file_manager_new_directory_disabled_24px.png", ":/file_manager_new_directory_disabled_16px.png"){}

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("FileManagerCreateNewDirectory");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "Create New Directory"));
        setShortcutScope(QApplication::translate("UIActionPool", "File Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Create New Directory"));
        setToolTip(  QApplication::translate("UIActionPool", "Create New Directory")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Simple action extension, used as 'Perform Copy' in file manager action class. */
class UIActionMenuFileManagerCopy : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManagerCopy(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/file_manager_copy_24px.png", ":/file_manager_copy_16px.png",
                         ":/file_manager_copy_disabled_24px.png", ":/file_manager_copy_disabled_16px.png"){}

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("FileManagerCopy");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "Copy"));
        setShortcutScope(QApplication::translate("UIActionPool", "File Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Copy selected file object(s)"));
        setToolTip(  QApplication::translate("UIActionPool", "Copy Selected Object(s)")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Simple action extension, used as 'Perform Cut' in file manager action class. */
class UIActionMenuFileManagerCut : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManagerCut(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/file_manager_cut_24px.png", ":/file_manager_cut_16px.png",
                         ":/file_manager_cut_disabled_24px.png", ":/file_manager_cut_disabled_16px.png"){}

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("FileManagerCut");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "Cut"));
        setShortcutScope(QApplication::translate("UIActionPool", "File Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Cut selected file object(s)"));
        setToolTip(  QApplication::translate("UIActionPool", "Cut Selected Object(s)")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Simple action extension, used as 'Perform Paste' in file manager action class. */
class UIActionMenuFileManagerPaste : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManagerPaste(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/file_manager_paste_24px.png", ":/file_manager_paste_16px.png",
                         ":/file_manager_paste_disabled_24px.png", ":/file_manager_paste_disabled_16px.png"){}

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("FileManagerPaste");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "Paste"));
        setShortcutScope(QApplication::translate("UIActionPool", "File Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Paste copied/cut file object(s)"));
        setToolTip(  QApplication::translate("UIActionPool", "Paste Copied/Cut Object(s)")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Simple action extension, used as 'Select All' in file manager action class. */
class UIActionMenuFileManagerSelectAll : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManagerSelectAll(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/file_manager_select_all_24px.png", ":/file_manager_select_all_16px.png",
                         ":/file_manager_select_all_disabled_24px.png", ":/file_manager_select_all_disabled_16px.png"){}

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("FileManagerSelectAll");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "Select All"));
        setShortcutScope(QApplication::translate("UIActionPool", "File Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Select all files objects"));
        setToolTip(  QApplication::translate("UIActionPool", "Select All Objects")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Simple action extension, used as 'Invert Selection' in file manager action class. */
class UIActionMenuFileManagerInvertSelection : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManagerInvertSelection(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/file_manager_invert_selection_24px.png", ":/file_manager_invert_selection_16px.png",
                         ":/file_manager_invert_selection_disabled_24px.png", ":/file_manager_invert_selection_disabled_16px.png"){}

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("FileManagerInvertSelection");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "Invert Selection"));
        setShortcutScope(QApplication::translate("UIActionPool", "File Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Invert the current selection"));
        setToolTip(  QApplication::translate("UIActionPool", "Invert Current Selection")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Simple action extension, used as 'Show Properties' in file manager action class. */
class UIActionMenuFileManagerShowProperties : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuFileManagerShowProperties(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/file_manager_properties_24px.png", ":/file_manager_properties_16px.png",
                         ":/file_manager_properties_disabled_24px.png", ":/file_manager_properties_disabled_16px.png"){}

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("FileManagerShowProperties");
    }

    /** Returns default shortcut. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const /* override */
    {
        return QKeySequence();
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "Show Properties"));
        setShortcutScope(QApplication::translate("UIActionPool", "File Manager"));
        setStatusTip(QApplication::translate("UIActionPool", "Show the properties of currently selected file object(s)"));
        setToolTip(  QApplication::translate("UIActionPool", "Show Properties of Current Object(s)")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};

/** Menu action extension, used as 'Performance' menu class. */
class UIActionMenuSelectorPerformance : public UIActionMenu
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuSelectorPerformance(UIActionPool *pParent)
        : UIActionMenu(pParent)
    {}

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("PerformanceMonitorMenu");
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&Performance"));
    }
};

/** Simple action extension, used as 'Perform Export' action class. */
class UIActionMenuSelectorPerformancePerformExport : public UIActionSimple
{
    Q_OBJECT;

public:

    /** Constructs action passing @a pParent to the base-class. */
    UIActionMenuSelectorPerformancePerformExport(UIActionPool *pParent)
        : UIActionSimple(pParent,
                         ":/performance_monitor_export_32px.png", ":/performance_monitor_export_16px.png",
                         ":/performance_monitor_export_disabled_32px.png", ":/performance_monitor_export_disabled_16px.png")
    {
        setShortcutContext(Qt::WidgetWithChildrenShortcut);
    }

protected:

    /** Returns shortcut extra-data ID. */
    virtual QString shortcutExtraDataID() const /* override */
    {
        return QString("ExportCharts");
    }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */
    {
        setName(QApplication::translate("UIActionPool", "&Export..."));
        setShortcutScope(QApplication::translate("UIActionPool", "Performance Monitor"));
        setStatusTip(QApplication::translate("UIActionPool", "Export the chart data into a text file"));
        setToolTip(  QApplication::translate("UIActionPool", "Export Data to File")
                   + (shortcut().isEmpty() ? QString() : QString(" (%1)").arg(shortcut().toString())));
    }
};


/*********************************************************************************************************************************
*   Class UIActionPool implementation.                                                                                           *
*********************************************************************************************************************************/

/* static */
UIActionPool *UIActionPool::create(UIActionPoolType enmType)
{
    UIActionPool *pActionPool = 0;
    switch (enmType)
    {
        case UIActionPoolType_Manager: pActionPool = new UIActionPoolManager; break;
        case UIActionPoolType_Runtime: pActionPool = new UIActionPoolRuntime; break;
        default: AssertFailedReturn(0);
    }
    AssertPtrReturn(pActionPool, 0);
    pActionPool->prepare();
    return pActionPool;
}

/* static */
void UIActionPool::destroy(UIActionPool *pActionPool)
{
    AssertPtrReturnVoid(pActionPool);
    pActionPool->cleanup();
    delete pActionPool;
}

/* static */
void UIActionPool::createTemporary(UIActionPoolType enmType)
{
    UIActionPool *pActionPool = 0;
    switch (enmType)
    {
        case UIActionPoolType_Manager: pActionPool = new UIActionPoolManager(true); break;
        case UIActionPoolType_Runtime: pActionPool = new UIActionPoolRuntime(true); break;
        default: AssertFailedReturnVoid();
    }
    AssertPtrReturnVoid(pActionPool);
    pActionPool->prepare();
    pActionPool->cleanup();
    delete pActionPool;
}

UIActionPoolManager *UIActionPool::toManager()
{
    return qobject_cast<UIActionPoolManager*>(this);
}

UIActionPoolRuntime *UIActionPool::toRuntime()
{
    return qobject_cast<UIActionPoolRuntime*>(this);
}

UIAction *UIActionPool::action(int iIndex) const
{
    AssertReturn(m_pool.contains(iIndex), 0);
    return m_pool.value(iIndex);
}

QList<UIAction*> UIActionPool::actions() const
{
    return m_pool.values();
}

QActionGroup *UIActionPool::actionGroup(int iIndex) const
{
    AssertReturn(m_groupPool.contains(iIndex), 0);
    return m_groupPool.value(iIndex);
}

bool UIActionPool::isAllowedInMenuBar(UIExtraDataMetaDefs::MenuType enmType) const
{
    foreach (const UIExtraDataMetaDefs::MenuType &enmRestriction, m_restrictedMenus.values())
        if (enmRestriction & enmType)
            return false;
    return true;
}

void UIActionPool::setRestrictionForMenuBar(UIActionRestrictionLevel enmLevel, UIExtraDataMetaDefs::MenuType enmRestriction)
{
    m_restrictedMenus[enmLevel] = enmRestriction;
    updateMenus();
}

bool UIActionPool::isAllowedInMenuApplication(UIExtraDataMetaDefs::MenuApplicationActionType enmType) const
{
    foreach (const UIExtraDataMetaDefs::MenuApplicationActionType &enmRestriction, m_restrictedActionsMenuApplication.values())
        if (enmRestriction & enmType)
            return false;
    return true;
}

void UIActionPool::setRestrictionForMenuApplication(UIActionRestrictionLevel enmLevel, UIExtraDataMetaDefs::MenuApplicationActionType enmRestriction)
{
    m_restrictedActionsMenuApplication[enmLevel] = enmRestriction;
    m_invalidations << UIActionIndex_M_Application;
}

#ifdef VBOX_WS_MAC
bool UIActionPool::isAllowedInMenuWindow(UIExtraDataMetaDefs::MenuWindowActionType enmType) const
{
    foreach (const UIExtraDataMetaDefs::MenuWindowActionType &enmRestriction, m_restrictedActionsMenuWindow.values())
        if (enmRestriction & enmType)
            return false;
    return true;
}

void UIActionPool::setRestrictionForMenuWindow(UIActionRestrictionLevel enmLevel, UIExtraDataMetaDefs::MenuWindowActionType enmRestriction)
{
    m_restrictedActionsMenuWindow[enmLevel] = enmRestriction;
    m_invalidations << UIActionIndex_M_Window;
}
#endif /* VBOX_WS_MAC */

bool UIActionPool::isAllowedInMenuHelp(UIExtraDataMetaDefs::MenuHelpActionType enmType) const
{
    foreach (const UIExtraDataMetaDefs::MenuHelpActionType &enmRestriction, m_restrictedActionsMenuHelp.values())
        if (enmRestriction & enmType)
            return false;
    return true;
}

void UIActionPool::setRestrictionForMenuHelp(UIActionRestrictionLevel enmLevel, UIExtraDataMetaDefs::MenuHelpActionType enmRestriction)
{
    m_restrictedActionsMenuHelp[enmLevel] = enmRestriction;
    m_invalidations << UIActionIndex_Menu_Help;
}

bool UIActionPool::processHotKey(const QKeySequence &key)
{
    /* Iterate through the whole list of keys: */
    foreach (const int &iKey, m_pool.keys())
    {
        /* Get current action: */
        UIAction *pAction = m_pool.value(iKey);
        /* Skip menus/separators: */
        if (pAction->type() == UIActionType_Menu)
            continue;
        /* Get the hot-key of the current action: */
        const QString strHotKey = gShortcutPool->shortcut(this, pAction).primaryToPortableText();
        if (pAction->isEnabled() && pAction->isAllowed() && !strHotKey.isEmpty())
        {
            if (key.matches(QKeySequence(strHotKey)) == QKeySequence::ExactMatch)
            {
                /* We asynchronously post a special event instead of calling
                 * pAction->trigger() directly, to let key presses and
                 * releases be processed correctly by Qt first.
                 * Note: we assume that nobody will delete the menu item
                 * corresponding to the key sequence, so that the pointer to
                 * menu data posted along with the event will remain valid in
                 * the event handler, at least until the main window is closed. */
                QApplication::postEvent(this, new ActivateActionEvent(pAction));
                return true;
            }
        }
    }
    return false;
}

void UIActionPool::sltHandleMenuPrepare()
{
    /* Make sure menu is valid: */
    AssertPtrReturnVoid(sender());
    UIMenu *pMenu = qobject_cast<UIMenu*>(sender());
    AssertPtrReturnVoid(pMenu);
    /* Make sure action is valid: */
    AssertPtrReturnVoid(pMenu->menuAction());
    UIAction *pAction = qobject_cast<UIAction*>(pMenu->menuAction());
    AssertPtrReturnVoid(pAction);

    /* Determine action index: */
    const int iIndex = m_pool.key(pAction);

    /* Update menu if necessary: */
    updateMenu(iIndex);

    /* Notify listeners about menu prepared: */
    emit sigNotifyAboutMenuPrepare(iIndex, pMenu);
}

#ifdef VBOX_WS_MAC
void UIActionPool::sltActionHovered()
{
    /* Acquire sender action: */
    UIAction *pAction = qobject_cast<UIAction*>(sender());
    AssertPtrReturnVoid(pAction);
    //printf("Action hovered: {%s}\n", pAction->name().toUtf8().constData());

    /* Notify listener about action hevering: */
    emit sigActionHovered(pAction);
}
#endif /* VBOX_WS_MAC */

UIActionPool::UIActionPool(UIActionPoolType enmType, bool fTemporary /* = false */)
    : m_enmType(enmType)
    , m_fTemporary(fTemporary)
{
}

void UIActionPool::preparePool()
{
    /* Create 'Application' actions: */
    m_pool[UIActionIndex_M_Application] = new UIActionMenuApplication(this);
#ifdef VBOX_WS_MAC
    m_pool[UIActionIndex_M_Application_S_About] = new UIActionSimpleAbout(this);
#endif
    m_pool[UIActionIndex_M_Application_S_Preferences] = new UIActionSimplePreferences(this);
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    m_pool[UIActionIndex_M_Application_S_NetworkAccessManager] = new UIActionSimpleNetworkAccessManager(this);
    m_pool[UIActionIndex_M_Application_S_CheckForUpdates] = new UIActionSimpleCheckForUpdates(this);
#endif
    m_pool[UIActionIndex_M_Application_S_ResetWarnings] = new UIActionSimpleResetWarnings(this);
    m_pool[UIActionIndex_M_Application_S_Close] = new UIActionSimplePerformClose(this);

#ifdef VBOX_WS_MAC
    /* Create 'Window' actions: */
    m_pool[UIActionIndex_M_Window] = new UIActionMenuWindow(this);
    m_pool[UIActionIndex_M_Window_S_Minimize] = new UIActionSimpleMinimize(this);
#endif

    /* Create 'Help' actions: */
    m_pool[UIActionIndex_Menu_Help] = new UIActionMenuHelp(this);
    m_pool[UIActionIndex_Simple_Contents] = new UIActionSimpleContents(this);
    m_pool[UIActionIndex_Simple_WebSite] = new UIActionSimpleWebSite(this);
    m_pool[UIActionIndex_Simple_BugTracker] = new UIActionSimpleBugTracker(this);
    m_pool[UIActionIndex_Simple_Forums] = new UIActionSimpleForums(this);
    m_pool[UIActionIndex_Simple_Oracle] = new UIActionSimpleOracle(this);
#ifndef VBOX_WS_MAC
    m_pool[UIActionIndex_Simple_About] = new UIActionSimpleAbout(this);
#endif

    /* Create 'Log Viewer' actions: */
    m_pool[UIActionIndex_M_LogWindow] = new UIActionMenuSelectorLog(this);
    m_pool[UIActionIndex_M_Log] = new UIActionMenuSelectorLog(this);
    m_pool[UIActionIndex_M_Log_T_Find] = new UIActionMenuSelectorLogTogglePaneFind(this);
    m_pool[UIActionIndex_M_Log_T_Filter] = new UIActionMenuSelectorLogTogglePaneFilter(this);
    m_pool[UIActionIndex_M_Log_T_Bookmark] = new UIActionMenuSelectorLogTogglePaneBookmark(this);
    m_pool[UIActionIndex_M_Log_T_Options] = new UIActionMenuSelectorLogTogglePaneOptions(this);
    m_pool[UIActionIndex_M_Log_S_Refresh] = new UIActionMenuSelectorLogPerformRefresh(this);
    m_pool[UIActionIndex_M_Log_S_Save] = new UIActionMenuSelectorLogPerformSave(this);

    /* Create 'Performance Monitor' actions: */
    m_pool[UIActionIndex_M_Performance] = new UIActionMenuSelectorPerformance(this);
    m_pool[UIActionIndex_M_Performance_S_Export] = new UIActionMenuSelectorPerformancePerformExport(this);

    /* Create 'File Manager' actions: */
    m_pool[UIActionIndex_M_FileManager] = new UIActionMenuFileManager(this);
    m_pool[UIActionIndex_M_FileManager_M_HostSubmenu] = new UIActionMenuFileManagerHostSubmenu(this);
    m_pool[UIActionIndex_M_FileManager_M_GuestSubmenu] = new UIActionMenuFileManagerGuestSubmenu(this);
    m_pool[UIActionIndex_M_FileManager_S_CopyToGuest] = new  UIActionMenuFileManagerCopyToGuest(this);
    m_pool[UIActionIndex_M_FileManager_S_CopyToHost] = new  UIActionMenuFileManagerCopyToHost(this);
    m_pool[UIActionIndex_M_FileManager_T_Options] = new UIActionMenuFileManagerOptions(this);
    m_pool[UIActionIndex_M_FileManager_T_Log] = new UIActionMenuFileManagerLog(this);
    m_pool[UIActionIndex_M_FileManager_T_Operations] = new UIActionMenuFileManagerOperations(this);
    m_pool[UIActionIndex_M_FileManager_T_Session] = new UIActionMenuFileManagerSession(this);
    m_pool[UIActionIndex_M_FileManager_S_Host_GoUp] = new UIActionMenuFileManagerGoUp(this);
    m_pool[UIActionIndex_M_FileManager_S_Guest_GoUp] = new UIActionMenuFileManagerGoUp(this);
    m_pool[UIActionIndex_M_FileManager_S_Host_GoHome] = new UIActionMenuFileManagerGoHome(this);
    m_pool[UIActionIndex_M_FileManager_S_Guest_GoHome] = new UIActionMenuFileManagerGoHome(this);
    m_pool[UIActionIndex_M_FileManager_S_Host_Refresh] = new UIActionMenuFileManagerRefresh(this);
    m_pool[UIActionIndex_M_FileManager_S_Guest_Refresh] = new UIActionMenuFileManagerRefresh(this);
    m_pool[UIActionIndex_M_FileManager_S_Host_Delete] = new UIActionMenuFileManagerDelete(this);
    m_pool[UIActionIndex_M_FileManager_S_Guest_Delete] = new UIActionMenuFileManagerDelete(this);
    m_pool[UIActionIndex_M_FileManager_S_Host_Rename] = new UIActionMenuFileManagerRename(this);
    m_pool[UIActionIndex_M_FileManager_S_Guest_Rename] = new UIActionMenuFileManagerRename(this);
    m_pool[UIActionIndex_M_FileManager_S_Host_CreateNewDirectory] = new UIActionMenuFileManagerCreateNewDirectory(this);
    m_pool[UIActionIndex_M_FileManager_S_Guest_CreateNewDirectory] = new UIActionMenuFileManagerCreateNewDirectory(this);
    m_pool[UIActionIndex_M_FileManager_S_Host_Copy] = new UIActionMenuFileManagerCopy(this);
    m_pool[UIActionIndex_M_FileManager_S_Guest_Copy] = new UIActionMenuFileManagerCopy(this);
    m_pool[UIActionIndex_M_FileManager_S_Host_Cut] = new UIActionMenuFileManagerCut(this);
    m_pool[UIActionIndex_M_FileManager_S_Guest_Cut] = new UIActionMenuFileManagerCut(this);
    m_pool[UIActionIndex_M_FileManager_S_Host_Paste] = new UIActionMenuFileManagerPaste(this);
    m_pool[UIActionIndex_M_FileManager_S_Guest_Paste] = new UIActionMenuFileManagerPaste(this);
    m_pool[UIActionIndex_M_FileManager_S_Host_SelectAll] = new UIActionMenuFileManagerSelectAll(this);
    m_pool[UIActionIndex_M_FileManager_S_Guest_SelectAll] = new UIActionMenuFileManagerSelectAll(this);
    m_pool[UIActionIndex_M_FileManager_S_Host_InvertSelection] = new UIActionMenuFileManagerInvertSelection(this);
    m_pool[UIActionIndex_M_FileManager_S_Guest_InvertSelection] = new UIActionMenuFileManagerInvertSelection(this);
    m_pool[UIActionIndex_M_FileManager_S_Host_ShowProperties] = new UIActionMenuFileManagerShowProperties(this);
    m_pool[UIActionIndex_M_FileManager_S_Guest_ShowProperties] = new UIActionMenuFileManagerShowProperties(this);

    /* Prepare update-handlers for known menus: */
#ifdef VBOX_WS_MAC
    m_menuUpdateHandlers[UIActionIndex_M_Application].ptf = &UIActionPool::updateMenuApplication;
    m_menuUpdateHandlers[UIActionIndex_M_Window].ptf = &UIActionPool::updateMenuWindow;
#endif
    m_menuUpdateHandlers[UIActionIndex_Menu_Help].ptf = &UIActionPool::updateMenuHelp;
    m_menuUpdateHandlers[UIActionIndex_M_LogWindow].ptf = &UIActionPool::updateMenuLogViewerWindow;
    m_menuUpdateHandlers[UIActionIndex_M_Log].ptf = &UIActionPool::updateMenuLogViewer;
    m_menuUpdateHandlers[UIActionIndex_M_Performance].ptf = &UIActionPool::updateMenuPerformanceMonitor;
    m_menuUpdateHandlers[UIActionIndex_M_FileManager].ptf = &UIActionPool::updateMenuFileManager;

    /* Invalidate all known menus: */
    m_invalidations.unite(m_menuUpdateHandlers.keys().toSet());

    /* Apply language settings: */
    retranslateUi();
}

void UIActionPool::prepareConnections()
{
    /* 'Application' menu connections: */
#ifdef VBOX_WS_MAC
    connect(action(UIActionIndex_M_Application_S_About), &UIAction::triggered,
            &msgCenter(), &UIMessageCenter::sltShowHelpAboutDialog, Qt::UniqueConnection);
#endif
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    connect(action(UIActionIndex_M_Application_S_NetworkAccessManager), &UIAction::triggered,
            gNetworkManager, &UINetworkManager::show, Qt::UniqueConnection);
    connect(action(UIActionIndex_M_Application_S_CheckForUpdates), &UIAction::triggered,
            gUpdateManager, &UIUpdateManager::sltForceCheck, Qt::UniqueConnection);
#endif
    connect(action(UIActionIndex_M_Application_S_ResetWarnings), &UIAction::triggered,
            &msgCenter(), &UIMessageCenter::sltResetSuppressedMessages, Qt::UniqueConnection);

    /* 'Help' menu connections: */
    connect(action(UIActionIndex_Simple_Contents), &UIAction::triggered,
            &msgCenter(), &UIMessageCenter::sltShowHelpHelpDialog, Qt::UniqueConnection);
    connect(action(UIActionIndex_Simple_WebSite), &UIAction::triggered,
            &msgCenter(), &UIMessageCenter::sltShowHelpWebDialog, Qt::UniqueConnection);
    connect(action(UIActionIndex_Simple_BugTracker), &UIAction::triggered,
            &msgCenter(), &UIMessageCenter::sltShowBugTracker, Qt::UniqueConnection);
    connect(action(UIActionIndex_Simple_Forums), &UIAction::triggered,
            &msgCenter(), &UIMessageCenter::sltShowForums, Qt::UniqueConnection);
    connect(action(UIActionIndex_Simple_Oracle), &UIAction::triggered,
            &msgCenter(), &UIMessageCenter::sltShowOracle, Qt::UniqueConnection);
#ifndef VBOX_WS_MAC
    connect(action(UIActionIndex_Simple_About), &UIAction::triggered,
            &msgCenter(), &UIMessageCenter::sltShowHelpAboutDialog, Qt::UniqueConnection);
#endif
}

void UIActionPool::cleanupConnections()
{
    /* Nothing for now.. */
}

void UIActionPool::cleanupPool()
{
    qDeleteAll(m_groupPool);
    qDeleteAll(m_pool);
}

void UIActionPool::updateConfiguration()
{
    /* Recache common action restrictions: */
    // Nothing here for now..

#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    /* Recache update action restrictions: */
    bool fUpdateAllowed = gEDataManager->applicationUpdateEnabled();
    if (!fUpdateAllowed)
    {
        m_restrictedActionsMenuApplication[UIActionRestrictionLevel_Base] = (UIExtraDataMetaDefs::MenuApplicationActionType)
            (m_restrictedActionsMenuApplication[UIActionRestrictionLevel_Base] | UIExtraDataMetaDefs::MenuApplicationActionType_CheckForUpdates);
    }
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */

    /* Update menus: */
    updateMenus();
}

void UIActionPool::updateMenu(int iIndex)
{
    /* Make sure index belongs to this class: */
    AssertReturnVoid(iIndex < UIActionIndex_Max);

    /* If menu with such index is invalidated
     * and there is update-handler => handle it here: */
    if (   m_invalidations.contains(iIndex)
        && m_menuUpdateHandlers.contains(iIndex))
        (this->*(m_menuUpdateHandlers.value(iIndex).ptf))();
}

void UIActionPool::updateShortcuts()
{
    gShortcutPool->applyShortcuts(this);
}

bool UIActionPool::event(QEvent *pEvent)
{
    /* Depending on event-type: */
    switch ((UIEventType)pEvent->type())
    {
        case ActivateActionEventType:
        {
            /* Process specific event: */
            ActivateActionEvent *pActionEvent = static_cast<ActivateActionEvent*>(pEvent);
            pActionEvent->action()->trigger();
            pEvent->accept();
            return true;
        }
        default:
            break;
    }
    /* Pass to the base-class: */
    return QObject::event(pEvent);
}

void UIActionPool::retranslateUi()
{
    /* Translate all the actions: */
    foreach (const int iActionPoolKey, m_pool.keys())
        m_pool[iActionPoolKey]->retranslateUi();
    /* Update shortcuts: */
    updateShortcuts();
}

bool UIActionPool::addAction(UIMenu *pMenu, UIAction *pAction, bool fReallyAdd /* = true */)
{
    /* Check if action is allowed: */
    const bool fIsActionAllowed = pAction->isAllowed();

#ifdef VBOX_WS_MAC
    /* Check if menu is consumable: */
    const bool fIsMenuConsumable = pMenu->isConsumable();
    /* Check if menu is NOT yet consumed: */
    const bool fIsMenuConsumed = pMenu->isConsumed();
#endif

    /* Make this action visible
     * depending on clearance state. */
    pAction->setVisible(fIsActionAllowed);

#ifdef VBOX_WS_MAC
    /* If menu is consumable: */
    if (fIsMenuConsumable)
    {
        /* Add action only if menu was not yet consumed: */
        if (!fIsMenuConsumed)
            pMenu->addAction(pAction);
    }
    /* If menu is NOT consumable: */
    else
#endif
    {
        /* Add action only if is allowed: */
        if (fIsActionAllowed && fReallyAdd)
            pMenu->addAction(pAction);
    }

    /* Return if action is allowed: */
    return fIsActionAllowed;
}

bool UIActionPool::addMenu(QList<QMenu*> &menuList, UIAction *pAction, bool fReallyAdd /* = true */)
{
    /* Check if action is allowed: */
    const bool fIsActionAllowed = pAction->isAllowed();

    /* Get action's menu: */
    UIMenu *pMenu = pAction->menu();

#ifdef VBOX_WS_MAC
    /* Check if menu is consumable: */
    const bool fIsMenuConsumable = pMenu->isConsumable();
    /* Check if menu is NOT yet consumed: */
    const bool fIsMenuConsumed = pMenu->isConsumed();
#endif

    /* Make this action visible
     * depending on clearance state. */
    pAction->setVisible(   fIsActionAllowed
#ifdef VBOX_WS_MAC
                        && !fIsMenuConsumable
#endif
                        );

#ifdef VBOX_WS_MAC
    /* If menu is consumable: */
    if (fIsMenuConsumable)
    {
        /* Add action's menu only if menu was not yet consumed: */
        if (!fIsMenuConsumed)
            menuList << pMenu;
    }
    /* If menu is NOT consumable: */
    else
#endif
    {
        /* Add action only if is allowed: */
        if (fIsActionAllowed && fReallyAdd)
            menuList << pMenu;
    }

    /* Return if action is allowed: */
    return fIsActionAllowed;
}

void UIActionPool::updateMenuApplication()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndex_M_Application)->menu();
    AssertPtrReturnVoid(pMenu);
#ifdef VBOX_WS_MAC
    AssertReturnVoid(pMenu->isConsumable());
#endif
    /* Clear contents: */
#ifdef VBOX_WS_MAC
    if (!pMenu->isConsumed())
#endif
        pMenu->clear();

    /* Separator: */
    bool fSeparator = false;

#ifdef VBOX_WS_MAC
    /* 'About' action: */
    fSeparator = addAction(pMenu, action(UIActionIndex_M_Application_S_About)) || fSeparator;
#endif

    /* 'Preferences' action: */
    fSeparator = addAction(pMenu, action(UIActionIndex_M_Application_S_Preferences)) || fSeparator;

#ifndef VBOX_WS_MAC
    /* Separator: */
    if (fSeparator)
    {
        pMenu->addSeparator();
        fSeparator = false;
    }
#endif

#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    /* 'Network Manager' action: */
    fSeparator = addAction(pMenu, action(UIActionIndex_M_Application_S_NetworkAccessManager)) || fSeparator;
#endif
    /* 'Reset Warnings' action: */
    fSeparator = addAction(pMenu, action(UIActionIndex_M_Application_S_ResetWarnings)) || fSeparator;

#ifndef VBOX_WS_MAC
    /* Separator: */
    if (fSeparator)
    {
        pMenu->addSeparator();
        fSeparator = false;
    }
#endif

    /* 'Close' action: */
    fSeparator = addAction(pMenu, action(UIActionIndex_M_Application_S_Close)) || fSeparator;

    /* Mark menu as valid: */
    m_invalidations.remove(UIActionIndex_M_Application);
}

#ifdef VBOX_WS_MAC
void UIActionPool::updateMenuWindow()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndex_M_Window)->menu();
    AssertPtrReturnVoid(pMenu);
    /* Clear contents: */
    pMenu->clear();

    /* Separator: */
    bool fSeparator = false;

    /* 'Minimize' action: */
    fSeparator = addAction(pMenu, action(UIActionIndex_M_Window_S_Minimize)) || fSeparator;

    /* Separator: */
    if (fSeparator)
    {
        pMenu->addSeparator();
        fSeparator = false;
    }

    /* This menu always remains invalid.. */
}
#endif /* VBOX_WS_MAC */

void UIActionPool::updateMenuHelp()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndex_Menu_Help)->menu();
    AssertPtrReturnVoid(pMenu);
    /* Clear contents: */
    pMenu->clear();

    /* Separator? */
    bool fSeparator = false;

    /* 'Contents' action: */
    fSeparator = addAction(pMenu, action(UIActionIndex_Simple_Contents)) || fSeparator;
    /* 'Web Site' action: */
    fSeparator = addAction(pMenu, action(UIActionIndex_Simple_WebSite)) || fSeparator;
    /* 'Bug Tracker' action: */
    fSeparator = addAction(pMenu, action(UIActionIndex_Simple_BugTracker)) || fSeparator;
    /* 'Forums' action: */
    fSeparator = addAction(pMenu, action(UIActionIndex_Simple_Forums)) || fSeparator;
    /* 'Oracle' action: */
    fSeparator = addAction(pMenu, action(UIActionIndex_Simple_Oracle)) || fSeparator;

    /* Separator? */
    if (fSeparator)
    {
        pMenu->addSeparator();
        fSeparator = false;
    }

#ifndef VBOX_WS_MAC
    /* 'About' action: */
    fSeparator = addAction(pMenu, action(UIActionIndex_Simple_About)) || fSeparator;
#endif

    /* Mark menu as valid: */
    m_invalidations.remove(UIActionIndex_Menu_Help);
}

void UIActionPool::updateMenuLogViewerWindow()
{
    /* Update corresponding menu: */
    updateMenuLogViewerWrapper(action(UIActionIndex_M_LogWindow)->menu());

    /* Mark menu as valid: */
    m_invalidations.remove(UIActionIndex_M_LogWindow);
}

void UIActionPool::updateMenuLogViewer()
{
    /* Update corresponding menu: */
    updateMenuLogViewerWrapper(action(UIActionIndex_M_Log)->menu());

    /* Mark menu as valid: */
    m_invalidations.remove(UIActionIndex_M_Log);
}

void UIActionPool::updateMenuLogViewerWrapper(UIMenu *pMenu)
{
    /* Clear contents: */
    pMenu->clear();

    /* Separator? */
    bool fSeparator = false;

    /* 'Save' action: */
    fSeparator = addAction(pMenu, action(UIActionIndex_M_Log_S_Save)) || fSeparator;

    /* Separator? */
    if (fSeparator)
    {
        pMenu->addSeparator();
        fSeparator = false;
    }

    /* 'Find' action: */
    fSeparator = addAction(pMenu, action(UIActionIndex_M_Log_T_Find)) || fSeparator;
    /* 'Filter' action: */
    fSeparator = addAction(pMenu, action(UIActionIndex_M_Log_T_Filter)) || fSeparator;
    /* 'Bookmarks' action: */
    fSeparator = addAction(pMenu, action(UIActionIndex_M_Log_T_Bookmark)) || fSeparator;
    /* 'Options' action: */
    fSeparator = addAction(pMenu, action(UIActionIndex_M_Log_T_Options)) || fSeparator;

    /* Separator? */
    if (fSeparator)
    {
        pMenu->addSeparator();
        fSeparator = false;
    }

    /* 'Refresh' action: */
    fSeparator = addAction(pMenu, action(UIActionIndex_M_Log_S_Refresh)) || fSeparator;
}

void UIActionPool::updateMenuPerformanceMonitor()
{
    /* Get corresponding menu: */
    UIMenu *pMenu = action(UIActionIndex_M_Performance)->menu();
    AssertPtrReturnVoid(pMenu);
    /* Clear contents: */
    pMenu->clear();

    /* 'Export' action: */
    pMenu->addAction(action(UIActionIndex_M_Performance_S_Export));

    /* Mark menu as valid: */
    m_invalidations.remove(UIActionIndex_M_Performance);
}

void UIActionPool::updateMenuFileManager()
{
    updateMenuFileManagerWrapper(action(UIActionIndex_M_FileManager)->menu());

    /* Mark menu as valid: */
    m_invalidations.remove(UIActionIndex_M_FileManager);
}

void UIActionPool::updateMenuFileManagerWrapper(UIMenu *pMenu)
{
    addAction(pMenu, action(UIActionIndex_M_FileManager_T_Session));
    addAction(pMenu, action(UIActionIndex_M_FileManager_T_Options));
    addAction(pMenu, action(UIActionIndex_M_FileManager_T_Operations));
    addAction(pMenu, action(UIActionIndex_M_FileManager_T_Log));

    addAction(pMenu, action(UIActionIndex_M_FileManager_M_HostSubmenu));
    addAction(pMenu, action(UIActionIndex_M_FileManager_M_GuestSubmenu));

    UIMenu *pHostSubmenu = action(UIActionIndex_M_FileManager_M_HostSubmenu)->menu();
    if (pHostSubmenu)
    {
        addAction(pHostSubmenu, action(UIActionIndex_M_FileManager_S_Host_GoUp));
        addAction(pHostSubmenu, action(UIActionIndex_M_FileManager_S_Host_GoHome));
        addAction(pHostSubmenu, action(UIActionIndex_M_FileManager_S_Host_Refresh));
        addAction(pHostSubmenu, action(UIActionIndex_M_FileManager_S_Host_Delete));
        addAction(pHostSubmenu, action(UIActionIndex_M_FileManager_S_Host_Rename));
        addAction(pHostSubmenu, action(UIActionIndex_M_FileManager_S_Host_CreateNewDirectory));
        addAction(pHostSubmenu, action(UIActionIndex_M_FileManager_S_Host_Copy));
        addAction(pHostSubmenu, action(UIActionIndex_M_FileManager_S_Host_Cut));
        addAction(pHostSubmenu, action(UIActionIndex_M_FileManager_S_Host_Paste));
        addAction(pHostSubmenu, action(UIActionIndex_M_FileManager_S_Host_SelectAll));
        addAction(pHostSubmenu, action(UIActionIndex_M_FileManager_S_Host_InvertSelection));
        addAction(pHostSubmenu, action(UIActionIndex_M_FileManager_S_Host_ShowProperties));
    }

    UIMenu *pGuestSubmenu = action(UIActionIndex_M_FileManager_M_GuestSubmenu)->menu();
    if (pGuestSubmenu)
    {
        addAction(pGuestSubmenu, action(UIActionIndex_M_FileManager_S_Host_GoUp));
        addAction(pGuestSubmenu, action(UIActionIndex_M_FileManager_S_Guest_GoHome));
        addAction(pGuestSubmenu, action(UIActionIndex_M_FileManager_S_Guest_Refresh));
        addAction(pGuestSubmenu, action(UIActionIndex_M_FileManager_S_Guest_Delete));
        addAction(pGuestSubmenu, action(UIActionIndex_M_FileManager_S_Guest_Rename));
        addAction(pGuestSubmenu, action(UIActionIndex_M_FileManager_S_Guest_CreateNewDirectory));
        addAction(pGuestSubmenu, action(UIActionIndex_M_FileManager_S_Guest_Copy));
        addAction(pGuestSubmenu, action(UIActionIndex_M_FileManager_S_Guest_Cut));
        addAction(pGuestSubmenu, action(UIActionIndex_M_FileManager_S_Guest_Paste));
        addAction(pGuestSubmenu, action(UIActionIndex_M_FileManager_S_Guest_SelectAll));
        addAction(pGuestSubmenu, action(UIActionIndex_M_FileManager_S_Guest_InvertSelection));
        addAction(pGuestSubmenu, action(UIActionIndex_M_FileManager_S_Guest_ShowProperties));
    }
}

void UIActionPool::prepare()
{
    /* Prepare pool: */
    preparePool();
    /* Prepare connections: */
    prepareConnections();

    /* Update configuration: */
    updateConfiguration();
    /* Update shortcuts: */
    updateShortcuts();
}

void UIActionPool::cleanup()
{
    /* Cleanup connections: */
    cleanupConnections();
    /* Cleanup pool: */
    cleanupPool();
}


#include "UIActionPool.moc"

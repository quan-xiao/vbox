/* $Id: UIVirtualBoxManagerWidget.h 86734 2020-10-28 11:43:13Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIVirtualBoxManagerWidget class declaration.
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

#ifndef FEQT_INCLUDED_SRC_manager_UIVirtualBoxManagerWidget_h
#define FEQT_INCLUDED_SRC_manager_UIVirtualBoxManagerWidget_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QWidget>

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "UISlidingAnimation.h"
#include "UIToolPaneGlobal.h"
#include "UIToolPaneMachine.h"

/* Forward declarations: */
class QStackedWidget;
class QISplitter;
class UIActionPool;
class UIChooser;
class UITabBar;
class QIToolBar;
class UITools;
class UIVirtualBoxManager;
class UIVirtualMachineItem;

/** QWidget extension used as VirtualBox Manager Widget instance. */
class UIVirtualBoxManagerWidget : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

    /** Possible selection types. */
    enum SelectionType
    {
        SelectionType_Invalid,
        SelectionType_SingleGroupItem,
        SelectionType_FirstIsGlobalItem,
        SelectionType_FirstIsMachineItem
    };

signals:

    /** @name Tool-bar stuff.
      * @{ */
        /* Notifies listeners about tool-bar height change. */
        void sigToolBarHeightChange(int iHeight);
    /** @} */

    /** @name Chooser pane stuff.
      * @{ */
        /** Notifies about Chooser-pane index change. */
        void sigChooserPaneIndexChange();
        /** Notifies about Chooser-pane group saving change. */
        void sigGroupSavingStateChanged();
        /** Notifies about Chooser-pane cloud update change. */
        void sigCloudUpdateStateChanged();

        /** Notifies about state change for cloud machine with certain @a uId. */
        void sigCloudMachineStateChange(const QUuid &uId);

        /** Notify listeners about start or show request. */
        void sigStartOrShowRequest();
        /** Notifies listeners about machine search widget visibility changed to @a fVisible. */
        void sigMachineSearchWidgetVisibilityChanged(bool fVisible);
    /** @} */

    /** @name Tools pane stuff.
      * @{ */
        /** Notifies about Tool type change. */
        void sigToolTypeChange();
    /** @} */

    /** @name Tools / Details pane stuff.
      * @{ */
        /** Notifies aboud Details-pane link clicked. */
        void sigMachineSettingsLinkClicked(const QString &strCategory, const QString &strControl, const QUuid &uId);
    /** @} */

    /** @name Tools / Snapshots pane stuff.
      * @{ */
        /** Notifies listeners about current Snapshots pane item change. */
        void sigCurrentSnapshotItemChange();
    /** @} */

public:

    /** Constructs VirtualBox Manager widget. */
    UIVirtualBoxManagerWidget(UIVirtualBoxManager *pParent);
    /** Destructs VirtualBox Manager widget. */
    virtual ~UIVirtualBoxManagerWidget() /* override */;

    /** @name Common stuff.
      * @{ */
        /** Returns the action-pool instance. */
        UIActionPool *actionPool() const { return m_pActionPool; }
    /** @} */

    /** @name Chooser pane stuff.
      * @{ */
        /** Returns current-item. */
        UIVirtualMachineItem *currentItem() const;
        /** Returns a list of current-items. */
        QList<UIVirtualMachineItem*> currentItems() const;

        /** Returns whether group item is selected. */
        bool isGroupItemSelected() const;
        /** Returns whether global item is selected. */
        bool isGlobalItemSelected() const;
        /** Returns whether machine item is selected. */
        bool isMachineItemSelected() const;

        /** Returns whether single group is selected. */
        bool isSingleGroupSelected() const;
        /** Returns whether single local group is selected. */
        bool isSingleLocalGroupSelected() const;
        /** Returns whether single cloud profile group is selected. */
        bool isSingleCloudProfileGroupSelected() const;
        /** Returns whether all items of one group are selected. */
        bool isAllItemsOfOneGroupSelected() const;

        /** Returns full name of currently selected group. */
        QString fullGroupName() const;

        /** Returns whether group saving is in progress. */
        bool isGroupSavingInProgress() const;
        /** Returns whether at least one cloud profile currently being updated. */
        bool isCloudProfileUpdateInProgress() const;

        /** Opens group name editor. */
        void openGroupNameEditor();
        /** Disbands group. */
        void disbandGroup();
        /** Removes machine. */
        void removeMachine();
        /** Moves machine to a group with certain @a strName. */
        void moveMachineToGroup(const QString &strName = QString());
        /** Returns possible groups for machine with passed @a uId to move to. */
        QStringList possibleGroupsForMachineToMove(const QUuid &uId);
        /** Returns possible groups for group with passed @a strFullName to move to. */
        QStringList possibleGroupsForGroupToMove(const QString &strFullName);
        /** Refreshes machine. */
        void refreshMachine();
        /** Sorts group. */
        void sortGroup();
        /** Toggle machine search widget to be @a fVisible. */
        void setMachineSearchWidgetVisibility(bool fVisible);
    /** @} */

    /** @name Tools pane stuff.
      * @{ */
        /** Defines tools @a enmType. */
        void setToolsType(UIToolType enmType);
        /** Returns tools type. */
        UIToolType toolsType() const;

        /** Returns a type of curent Global tool. */
        UIToolType currentGlobalTool() const;
        /** Returns a type of curent Machine tool. */
        UIToolType currentMachineTool() const;
        /** Returns whether Global tool of passed @a enmType is opened. */
        bool isGlobalToolOpened(UIToolType enmType) const;
        /** Returns whether Machine tool of passed @a enmType is opened. */
        bool isMachineToolOpened(UIToolType enmType) const;
        /** Switches to Global tool of passed @a enmType. */
        void switchToGlobalTool(UIToolType enmType);
        /** Switches to Machine tool of passed @a enmType. */
        void switchToMachineTool(UIToolType enmType);
        /** Closes Global tool of passed @a enmType. */
        void closeGlobalTool(UIToolType enmType);
        /** Closes Machine tool of passed @a enmType. */
        void closeMachineTool(UIToolType enmType);
    /** @} */

    /** @name Tools / Snapshot pane stuff.
      * @{ */
        /** Returns whether current-state item of Snapshot pane is selected. */
        bool isCurrentStateItemSelected() const;
    /** @} */

    /** @name Tool-bar stuff.
      * @{ */
        /** Updates tool-bar menu buttons. */
        void updateToolBarMenuButtons(bool fSeparateMenuSection);
    /** @} */

public slots:

    /** @name Tool-bar stuff.
      * @{ */
        /** Handles tool-bar context-menu request for passed @a position. */
        void sltHandleToolBarContextMenuRequest(const QPoint &position);
    /** @} */

protected:

    /** @name Event handling stuff.
      * @{ */
        /** Handles translation event. */
        virtual void retranslateUi() /* override */;
    /** @} */

private slots:

    /** @name CVirtualBox event handling stuff.
      * @{ */
        /** Handles CVirtualBox event about state change for machine with @a uId. */
        void sltHandleStateChange(const QUuid &uId);
    /** @} */

    /** @name Tool-bar stuff.
      * @{ */
        /** Handles signal about tool-bar resize to @a newSize. */
        void sltHandleToolBarResize(const QSize &newSize);
    /** @} */

    /** @name Chooser pane stuff.
      * @{ */
        /** Handles signal about Chooser-pane index change. */
        void sltHandleChooserPaneIndexChange();

        /** Handles signal about Chooser-pane selection invalidated. */
        void sltHandleChooserPaneSelectionInvalidated() { recacheCurrentItemInformation(true /* fDontRaiseErrorPane */); }

        /** Handles sliding animation complete signal.
          * @param  enmDirection  Brings which direction was animation finished for. */
        void sltHandleSlidingAnimationComplete(SlidingDirection enmDirection);

        /** Handles state change for cloud machine with certain @a uId. */
        void sltHandleCloudMachineStateChange(const QUuid &uId);
    /** @} */

    /** @name Tools pane stuff.
      * @{ */
        /** Handles tool menu request. */
        void sltHandleToolMenuRequested(UIToolClass enmClass, const QPoint &position);

        /** Handles signal about Tools-pane index change. */
        void sltHandleToolsPaneIndexChange();

        /** Handles signal requesting switch to Performance pane of machine with @a uMachineId. */
        void sltSwitchToMachinePerformancePane(const QUuid &uMachineId);
    /** @} */

private:

    /** @name Prepare/Cleanup cascade.
      * @{ */
        /** Prepares all. */
        void prepare();
        /** Prepares palette. */
        void preparePalette();
        /** Prepares widgets. */
        void prepareWidgets();
        /** Prepares connections. */
        void prepareConnections();
        /** Loads settings. */
        void loadSettings();

        /** Updates toolbar. */
        void updateToolbar();

        /** Saves settings. */
        void saveSettings();
        /** Cleanups connections. */
        void cleanupConnections();
        /** Cleanups all. */
        void cleanup();
    /** @} */

    /** @name Tools / Common stuff.
      * @{ */
        /** Recaches current item information.
          * @param  fDontRaiseErrorPane  Brings whether we should not raise error-pane. */
        void recacheCurrentItemInformation(bool fDontRaiseErrorPane = false);
    /** @} */

    /** Holds the action-pool instance. */
    UIActionPool *m_pActionPool;

    /** Holds the central splitter instance. */
    QISplitter *m_pSplitter;

    /** Holds the main toolbar instance. */
    QIToolBar *m_pToolBar;

    /** Holds the Chooser-pane instance. */
    UIChooser          *m_pPaneChooser;
    /** Holds the stacked-widget. */
    QStackedWidget     *m_pStackedWidget;
    /** Holds the Global Tools-pane instance. */
    UIToolPaneGlobal   *m_pPaneToolsGlobal;
    /** Holds the Machine Tools-pane instance. */
    UIToolPaneMachine  *m_pPaneToolsMachine;
    /** Holds the sliding-animation widget instance. */
    UISlidingAnimation *m_pSlidingAnimation;
    /** Holds the Tools-pane instance. */
    UITools            *m_pPaneTools;

    /** Holds the last selection type. */
    SelectionType  m_enmSelectionType;
    /** Holds whether the last selected item was accessible. */
    bool           m_fSelectedMachineItemAccessible;
};

#endif /* !FEQT_INCLUDED_SRC_manager_UIVirtualBoxManagerWidget_h */

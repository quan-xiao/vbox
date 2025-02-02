/* $Id: UIChooserModel.h 86705 2020-10-26 13:12:07Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIChooserModel class declaration.
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

#ifndef FEQT_INCLUDED_SRC_manager_chooser_UIChooserModel_h
#define FEQT_INCLUDED_SRC_manager_chooser_UIChooserModel_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QPointer>

/* GUI includes: */
#include "UIChooserAbstractModel.h"
#include "UIExtraDataDefs.h"

/* COM includes: */
#include "COMEnums.h"
#include "CCloudMachine.h"
#include "CMachine.h"

/* Forward declaration: */
class QDrag;
class UIActionPool;
class UIChooser;
class UIChooserHandlerMouse;
class UIChooserHandlerKeyboard;
class UIChooserItem;
class UIChooserItemMachine;
class UIChooserNode;
class UIChooserView;
class UIVirtualMachineItem;

/** UIChooserAbstractModel extension used as VM Chooser-pane model.
  * This class is used to operate on tree of visible tree items
  * representing VMs and their groups. */
class UIChooserModel : public UIChooserAbstractModel
{
    Q_OBJECT;

signals:

    /** @name Tool stuff.
      * @{ */
        /** Notifies listeners about tool popup-menu request for certain @a enmClass and @a position. */
        void sigToolMenuRequested(UIToolClass enmClass, const QPoint &position);
    /** @} */

    /** @name Selection stuff.
      * @{ */
        /** Notifies listeners about selection changed. */
        void sigSelectionChanged();
        /** Notifies listeners about selection invalidated. */
        void sigSelectionInvalidated();

        /** Notifies listeners about group toggling started. */
        void sigToggleStarted();
        /** Notifies listeners about group toggling finished. */
        void sigToggleFinished();
    /** @} */

    /** @name Layout stuff.
      * @{ */
        /** Notifies listeners about root item minimum width @a iHint changed. */
        void sigRootItemMinimumWidthHintChanged(int iHint);
    /** @} */

    /** @name Action stuff.
      * @{ */
        /** Notifies listeners about start or show request. */
        void sigStartOrShowRequest();
    /** @} */

public:

    /** Constructs Chooser-model passing @a pParent to the base-class. */
    UIChooserModel(UIChooser *pParent, UIActionPool *pActionPool);
    /** Destructs Chooser-model. */
    virtual ~UIChooserModel() /* override */;

    /** @name General stuff.
      * @{ */
        /** Inits model. */
        virtual void init() /* override */;
        /** Deinits model. */
        virtual void deinit() /* override */;

        /** Returns the action-pool reference. */
        UIActionPool *actionPool() const;
        /** Returns the scene reference. */
        QGraphicsScene *scene() const;
        /** Returns the reference of the first view of the scene(). */
        UIChooserView *view() const;
        /** Returns the paint device reference. */
        QPaintDevice *paintDevice() const;

        /** Returns item at @a position, taking into account possible @a deviceTransform. */
        QGraphicsItem *itemAt(const QPointF &position, const QTransform &deviceTransform = QTransform()) const;

        /** Handles tool button click for certain @a pItem. */
        void handleToolButtonClick(UIChooserItem *pItem);
        /** Handles pin button click for certain @a pItem. */
        void handlePinButtonClick(UIChooserItem *pItem);
    /** @} */

    /** @name Selection stuff.
      * @{ */
        /** Sets a list of selected @a items. */
        void setSelectedItems(const QList<UIChooserItem*> &items);
        /** Defines selected @a pItem. */
        void setSelectedItem(UIChooserItem *pItem);
        /** Defines selected-item by @a definition. */
        void setSelectedItem(const QString &strDefinition);
        /** Clear selected-items list. */
        void clearSelectedItems();

        /** Returns a list of selected-items. */
        const QList<UIChooserItem*> &selectedItems() const;

        /** Adds @a pItem to list of selected. */
        void addToSelectedItems(UIChooserItem *pItem);
        /** Removes @a pItem from list of selected. */
        void removeFromSelectedItems(UIChooserItem *pItem);

        /** Returns first selected-item. */
        UIChooserItem *firstSelectedItem() const;
        /** Returns first selected machine item. */
        UIVirtualMachineItem *firstSelectedMachineItem() const;
        /** Returns a list of selected machine items. */
        QList<UIVirtualMachineItem*> selectedMachineItems() const;

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
        /** Returns whether all machine items of one group is selected. */
        bool isAllItemsOfOneGroupSelected() const;

        /** Returns full name of currently selected group. */
        QString fullGroupName() const;

        /** Finds closest non-selected-item. */
        UIChooserItem *findClosestUnselectedItem() const;
        /** Makes sure selection doesn't contain item with certain @a uId. */
        void makeSureNoItemWithCertainIdSelected(const QUuid &uId);
        /** Makes sure at least one item selected. */
        void makeSureAtLeastOneItemSelected();

        /** Defines current @a pItem. */
        void setCurrentItem(UIChooserItem *pItem);
        /** Returns current-item. */
        UIChooserItem *currentItem() const;
    /** @} */

    /** @name Navigation stuff.
      * @{ */
        /** Returns a list of navigation-items. */
        const QList<UIChooserItem*> &navigationItems() const;
        /** Removes @a pItem from navigation list. */
        void removeFromNavigationItems(UIChooserItem *pItem);
        /** Updates navigation list. */
        void updateNavigationItemList();
    /** @} */

    /** @name Search stuff.
      * @{ */
        /** Performs a search for an item matching @a strDefinition. */
        UIChooserItem *searchItemByDefinition(const QString &strDefinition) const;

        /** Performs a search using @a strSearchTerm and @a iSearchFlags specified. */
        virtual void performSearch(const QString &strSearchTerm, int iSearchFlags) /* override */;
        /** Resets the search result data members and disables item's visual effects.
          * Also returns a list of all nodes which may be utilized by the calling code. */
        virtual QList<UIChooserNode*> resetSearch() /* override */;

        /** Selects next/prev (wrt. @a fIsNext) search result. */
        void selectSearchResult(bool fIsNext);
        /** Shows/hides machine search widget. */
        void setSearchWidgetVisible(bool fVisible);
    /** @} */

    /** @name Children stuff.
      * @{ */
        /** Returns the root instance. */
        UIChooserItem *root() const;

        /** Starts editing selected group item name. */
        void startEditingSelectedGroupItemName();
        /** Disbands selected group item. */
        void disbandSelectedGroupItem();
        /** Removes selected machine items. */
        void removeSelectedMachineItems();
        /** Moves selected machine items to group item.
          * @param  strName  Holds the group item name to move items to, if
          *                  that name isn't specified, new top-level group
          *                  item will be created. */
        void moveSelectedMachineItemsToGroupItem(const QString &strName);
        /** Starts or shows selected items. */
        void startOrShowSelectedItems();
        /** Refreshes selected machine items. */
        void refreshSelectedMachineItems();
        /** Sorts selected [parent] group item. */
        void sortSelectedGroupItem();
        /** Changes current machine item to the one with certain @a uId. */
        void setCurrentMachineItem(const QUuid &uId);

        /** Defines current @a pDragObject. */
        void setCurrentDragObject(QDrag *pDragObject);

        /** Looks for item with certain @a strLookupText. */
        void lookFor(const QString &strLookupText);
    /** @} */

    /** @name Layout stuff.
      * @{ */
        /** Updates layout. */
        void updateLayout();

        /** Defines global item height @a iHint. */
        void setGlobalItemHeightHint(int iHint);
    /** @} */

public slots:

    /** @name General stuff.
      * @{ */
        /** Handles Chooser-view resize. */
        void sltHandleViewResized();
    /** @} */

protected:

    /** @name Event handling stuff.
      * @{ */
        /** Preprocesses Qt @a pEvent for passed @a pObject. */
        virtual bool eventFilter(QObject *pObject, QEvent *pEvent) /* override */;
    /** @} */

protected slots:

    /** @name Main event handling stuff.
      * @{ */
        /** Handles local machine registering/unregistering for machine with certain @a uMachineId. */
        virtual void sltLocalMachineRegistrationChanged(const QUuid &uMachineId, const bool fRegistered) /* override */;

        /** Handles event about cloud provider with @a uProviderId being uninstalled. */
        virtual void sltHandleCloudProviderUninstall(const QUuid &uProviderId) /* override */;
    /** @} */

    /** @name Children stuff.
      * @{ */
        /** Handles reload machine with certain @a uMachineId request. */
        virtual void sltReloadMachine(const QUuid &uMachineId) /* override */;
    /** @} */

    /** @name Cloud stuff.
      * @{ */
        /** Handles cloud machine unregistering for @a uId.
          * @param  strProviderShortName  Brings provider short name.
          * @param  strProfileName        Brings profile name. */
        virtual void sltCloudMachineUnregistered(const QString &strProviderShortName,
                                                 const QString &strProfileName,
                                                 const QUuid &uId) /* override */;
        /** Handles cloud machine unregistering for a list of @a ids.
          * @param  strProviderShortName  Brings provider short name.
          * @param  strProfileName        Brings profile name. */
        virtual void sltCloudMachinesUnregistered(const QString &strProviderShortName,
                                                  const QString &strProfileName,
                                                  const QList<QUuid> &ids) /* override */;
        /** Handles cloud machine registering for @a comMachine.
          * @param  strProviderShortName  Brings provider short name.
          * @param  strProfileName        Brings profile name. */
        virtual void sltCloudMachineRegistered(const QString &strProviderShortName,
                                               const QString &strProfileName,
                                               const CCloudMachine &comMachine) /* override */;
        /** Handles cloud machine registering for a list of @a machines.
          * @param  strProviderShortName  Brings provider short name.
          * @param  strProfileName        Brings profile name. */
        virtual void sltCloudMachinesRegistered(const QString &strProviderShortName,
                                                const QString &strProfileName,
                                                const QVector<CCloudMachine> &machines) /* override */;

        /** Handles list cloud machines task complete signal. */
        virtual void sltHandleCloudListMachinesTaskComplete(UITask *pTask) /* override */;

        /** Handles Cloud Profile Manager cumulative changes. */
        virtual void sltHandleCloudProfileManagerCumulativeChange() /* override */;
    /** @} */

private slots:

    /** @name Selection stuff.
      * @{ */
        /** Makes sure current item is visible. */
        void sltMakeSureCurrentItemVisible();

        /** Handles current-item destruction. */
        void sltCurrentItemDestroyed();
    /** @} */

    /** @name Children stuff.
      * @{ */
        /** Handles D&D scrolling. */
        void sltStartScrolling();
        /** Handles D&D object destruction. */
        void sltCurrentDragObjectDestroyed();
    /** @} */

    /** @name Cloud stuff.
      * @{ */
        /** Updates selected cloud profiles. */
        void sltUpdateSelectedCloudProfiles();
    /** @} */

private:

    /** @name Prepare/Cleanup cascade.
      * @{ */
        /** Prepares all. */
        void prepare();
        /** Prepares scene. */
        void prepareScene();
        /** Prepares context-menu. */
        void prepareContextMenu();
        /** Prepares handlers. */
        void prepareHandlers();
        /** Prepares cloud update timer. */
        void prepareCloudUpdateTimer();
        /** Prepares connections. */
        void prepareConnections();
        /** Loads last selected-items. */
        void loadLastSelectedItem();

        /** Saves last selected-items. */
        void saveLastSelectedItem();
        /** Cleanups connections. */
        void cleanupConnections();
        /** Cleanups cloud update timer.*/
        void cleanupCloudUpdateTimer();
        /** Cleanups handlers. */
        void cleanupHandlers();
        /** Cleanups context-menu. */
        void cleanupContextMenu();
        /** Cleanups scene. */
        void cleanupScene();
        /** Cleanups all. */
        void cleanup();
    /** @} */

    /** @name General stuff.
      * @{ */
        /** Handles context-menu @a pEvent. */
        bool processContextMenuEvent(QGraphicsSceneContextMenuEvent *pEvent);
    /** @} */

    /** @name Selection stuff.
      * @{ */
        /** Clears real focus. */
        void clearRealFocus();
    /** @} */

    /** @name Navigation stuff.
      * @{ */
        /** Creates navigation list for passed root @a pItem. */
        QList<UIChooserItem*> createNavigationItemList(UIChooserItem *pItem);
    /** @} */

    /** @name Children stuff.
      * @{ */
        /** [Re]builds tree for main root, preserves selection if requested. */
        void buildTreeForMainRoot(bool fPreserveSelection = false);
        /** Update tree for main root. */
        void updateTreeForMainRoot();

        /** Removes a list of local virtual @a machineItems. */
        void removeLocalMachineItems(const QList<UIChooserItemMachine*> &machineItems);
        /** Unregisters a list of local virtual @a machines. */
        void unregisterLocalMachines(const QList<CMachine> &machines);
        /** Unregisters a list of cloud virtual @a machineItems. */
        void unregisterCloudMachineItems(const QList<UIChooserItemMachine*> &machineItems);

        /** Processes drag move @a pEvent. */
        bool processDragMoveEvent(QGraphicsSceneDragDropEvent *pEvent);
        /** Processes drag leave @a pEvent. */
        bool processDragLeaveEvent(QGraphicsSceneDragDropEvent *pEvent);
    /** @} */

    /** @name General stuff.
      * @{ */
        /** Holds the action-pool reference. */
        UIActionPool *m_pActionPool;

        /** Holds the scene reference. */
        QGraphicsScene *m_pScene;

        /** Holds the mouse handler instance. */
        UIChooserHandlerMouse    *m_pMouseHandler;
        /** Holds the keyboard handler instance. */
        UIChooserHandlerKeyboard *m_pKeyboardHandler;

        /** Holds the map of local context-menu instances. */
        QMap<UIChooserNodeType, QMenu*>  m_localMenus;
        /** Holds the map of cloud context-menu instances. */
        QMap<UIChooserNodeType, QMenu*>  m_cloudMenus;
    /** @} */

    /** @name Selection stuff.
      * @{ */
        /** Holds the current-item reference. */
        QPointer<UIChooserItem>  m_pCurrentItem;
    /** @} */

    /** @name Search stuff.
      * @{ */
        /** Stores the index (within the m_searchResults) of the currently selected found item. */
        int  m_iCurrentSearchResultIndex;
    /** @} */

    /** @name Children stuff.
      * @{ */
        /** Holds the root instance. */
        QPointer<UIChooserItem>  m_pRoot;

        /** Holds the navigation-items. */
        QList<UIChooserItem*>  m_navigationItems;
        /** Holds the selected-items. */
        QList<UIChooserItem*>  m_selectedItems;

        /** Holds the current drag object instance. */
        QPointer<QDrag>  m_pCurrentDragObject;
        /** Holds the drag scrolling token size. */
        int              m_iScrollingTokenSize;
        /** Holds whether drag scrolling is in progress. */
        bool             m_fIsScrollingInProgress;
    /** @} */

    /** @name Cloud stuff.
      * @{ */
        /** Holds cloud profile update timer instance. */
        QTimer *m_pTimerCloudProfileUpdate;
    /** @} */
};

#endif /* !FEQT_INCLUDED_SRC_manager_chooser_UIChooserModel_h */

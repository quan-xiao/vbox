/* $Id: UICloudProfileManager.h 86690 2020-10-23 14:34:40Z vboxsync $ */
/** @file
 * VBox Qt GUI - UICloudProfileManager class declaration.
 */

/*
 * Copyright (C) 2009-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_cloud_profilemanager_UICloudProfileManager_h
#define FEQT_INCLUDED_SRC_cloud_profilemanager_UICloudProfileManager_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "QIManagerDialog.h"
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class QAbstractButton;
class QTreeWidgetItem;
class QITreeWidget;
class UIActionPool;
class UICloudProfileDetailsWidget;
class UIItemCloudProfile;
class UIItemCloudProvider;
class QIToolBar;
struct UIDataCloudProfile;
struct UIDataCloudProvider;
class CCloudProfile;
class CCloudProvider;


/** QWidget extension providing GUI with the pane to control cloud profile related functionality. */
class UICloudProfileManagerWidget : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

signals:

    /** Notifies listeners about cloud profile details-widget @a fVisible. */
    void sigCloudProfileDetailsVisibilityChanged(bool fVisible);
    /** Notifies listeners about cloud profile details data @a fDiffers. */
    void sigCloudProfileDetailsDataChanged(bool fDiffers);

public:

    /** Constructs Cloud Profile Manager widget.
      * @param  enmEmbedding  Brings the type of widget embedding.
      * @param  pActionPool   Brings the action-pool reference.
      * @param  fShowToolbar  Brings whether we should create/show toolbar. */
    UICloudProfileManagerWidget(EmbedTo enmEmbedding, UIActionPool *pActionPool,
                                bool fShowToolbar = true, QWidget *pParent = 0);

    /** Returns the menu. */
    QMenu *menu() const;

#ifdef VBOX_WS_MAC
    /** Returns the toolbar. */
    QIToolBar *toolbar() const { return m_pToolBar; }
#endif

    /** Check for changes committed.
      * @returns Whether changes were resolved (accepted or discarded) or still a problem otherwise. */
    bool makeSureChangesResolved();

protected:

    /** @name Event-handling stuff.
      * @{ */
        /** Handles translation event. */
        virtual void retranslateUi() /* override */;
    /** @} */

public slots:

    /** @name Details-widget stuff.
      * @{ */
        /** Handles command to reset cloud profile details changes. */
        void sltResetCloudProfileDetailsChanges();
        /** Handles command to apply cloud profile details changes. */
        void sltApplyCloudProfileDetailsChanges();
    /** @} */

private slots:

    /** @name Menu/action stuff.
      * @{ */
        /** Handles command to add cloud profile. */
        void sltAddCloudProfile();
        /** Handles command to import cloud profiles. */
        void sltImportCloudProfiles();
        /** Handles command to remove cloud profile. */
        void sltRemoveCloudProfile();
        /** Handles command to make cloud profile details @a fVisible. */
        void sltToggleCloudProfileDetailsVisibility(bool fVisible);
        /** Handles command to show cloud profile try page. */
        void sltShowCloudProfileTryPage();
        /** Handles command to show cloud profile help. */
        void sltShowCloudProfileHelp();
    /** @} */

    /** @name Tree-widget stuff.
      * @{ */
        /** Handles request to load cloud stuff. */
        void sltLoadCloudStuff() { loadCloudStuff(); }
        /** Adjusts tree-widget according content. */
        void sltPerformTableAdjustment();
        /** Handles tree-widget current item change. */
        void sltHandleCurrentItemChange();
        /** Handles context-menu request for tree-widget @a position. */
        void sltHandleContextMenuRequest(const QPoint &position);
        /** Handles tree-widget @a pItem change. */
        void sltHandleItemChange(QTreeWidgetItem *pItem);
    /** @} */

private:

    /** @name Prepare/cleanup cascade.
      * @{ */
        /** Prepares all. */
        void prepare();
        /** Prepares actions. */
        void prepareActions();
        /** Prepares widgets. */
        void prepareWidgets();
        /** Prepares toolbar. */
        void prepareToolBar();
        /** Prepares tree-widget. */
        void prepareTreeWidget();
        /** Prepares details-widget. */
        void prepareDetailsWidget();
        /** Prepares connections. */
        void prepareConnections();
        /** Load settings: */
        void loadSettings();
    /** @} */

    /** @name Loading stuff.
      * @{ */
        /** Loads cloud stuff. */
        void loadCloudStuff();
        /** Loads cloud @a comProvider data to passed @a providerData container,
          * using @a restrictions as hint. */
        void loadCloudProvider(const CCloudProvider &comProvider,
                               const QStringList &restrictions,
                               UIDataCloudProvider &providerData);
        /** Loads cloud @a comProfile data to passed @a profileData container,
          * using @a restrictions & @a providerData as hint. */
        void loadCloudProfile(const CCloudProfile &comProfile,
                              const QStringList &restrictions,
                              const UIDataCloudProvider &providerData,
                              UIDataCloudProfile &profileData);
    /** @} */

    /** @name Tree-widget stuff.
      * @{ */
        /** Recursively searches for an item with specified @a strDefinition,
          * using @a pParentItem as an item to start search from. */
        QTreeWidgetItem *searchItem(const QString &strDefinition,
                                    QTreeWidgetItem *pParentItem = 0) const;

        /** Creates a new tree-widget item
          * on the basis of passed @a providerData. */
        void createItemForCloudProvider(const UIDataCloudProvider &providerData);
        /** Creates a new tree-widget item as a child of certain @a pParent,
          * on the basis of passed @a profileData. */
        void createItemForCloudProfile(QTreeWidgetItem *pParent, const UIDataCloudProfile &profileData);

        /* Gathers a list of Cloud Profile Manager restrictions starting from @a pParentItem. */
        QStringList gatherCloudProfileManagerRestrictions(QTreeWidgetItem *pParentItem);
    /** @} */

    /** @name General variables.
      * @{ */
        /** Holds the widget embedding type. */
        const EmbedTo  m_enmEmbedding;
        /** Holds the action-pool reference. */
        UIActionPool  *m_pActionPool;
        /** Holds whether we should create/show toolbar. */
        const bool     m_fShowToolbar;
    /** @} */

    /** @name Toolbar and menu variables.
      * @{ */
        /** Holds the toolbar instance. */
        QIToolBar *m_pToolBar;
    /** @} */

    /** @name Splitter variables.
      * @{ */
        /** Holds the tree-widget instance. */
        QITreeWidget                *m_pTreeWidget;
        /** Holds the details-widget instance. */
        UICloudProfileDetailsWidget *m_pDetailsWidget;
    /** @} */
};


/** QIManagerDialogFactory extension used as a factory for Cloud Profile Manager dialog. */
class UICloudProfileManagerFactory : public QIManagerDialogFactory
{
public:

    /** Constructs Cloud Profile Manager factory acquiring additional arguments.
      * @param  pActionPool  Brings the action-pool reference. */
    UICloudProfileManagerFactory(UIActionPool *pActionPool = 0);

protected:

    /** Creates derived @a pDialog instance.
      * @param  pCenterWidget  Brings the widget reference to center according to. */
    virtual void create(QIManagerDialog *&pDialog, QWidget *pCenterWidget) /* override */;

    /** Holds the action-pool reference. */
    UIActionPool *m_pActionPool;
};


/** QIManagerDialog extension providing GUI with the dialog to control cloud profile related functionality. */
class UICloudProfileManager : public QIWithRetranslateUI<QIManagerDialog>
{
    Q_OBJECT;

signals:

    /** Notifies listeners about data change rejected and should be reseted. */
    void sigDataChangeRejected();
    /** Notifies listeners about data change accepted and should be applied. */
    void sigDataChangeAccepted();

private slots:

    /** @name Button-box stuff.
      * @{ */
        /** Handles button-box button click. */
        void sltHandleButtonBoxClick(QAbstractButton *pButton);
    /** @} */

private:

    /** Constructs Cloud Profile Manager dialog.
      * @param  pCenterWidget  Brings the widget reference to center according to.
      * @param  pActionPool    Brings the action-pool reference. */
    UICloudProfileManager(QWidget *pCenterWidget, UIActionPool *pActionPool);

    /** @name Event-handling stuff.
      * @{ */
        /** Handles translation event. */
        virtual void retranslateUi() /* override */;
    /** @} */

    /** @name Prepare/cleanup cascade.
      * @{ */
        /** Configures all. */
        virtual void configure() /* override */;
        /** Configures central-widget. */
        virtual void configureCentralWidget() /* override */;
        /** Configures button-box. */
        virtual void configureButtonBox() /* override */;
        /** Perform final preparations. */
        virtual void finalize() /* override */;
    /** @} */

    /** @name Widget stuff.
      * @{ */
        /** Returns the widget. */
        virtual UICloudProfileManagerWidget *widget() /* override */;
    /** @} */

    /** @name Event-handling stuff.
      * @{ */
        /** Handles close @a pEvent. */
        virtual void closeEvent(QCloseEvent *pEvent) /* override */;
    /** @} */

    /** @name Action related variables.
      * @{ */
        /** Holds the action-pool reference. */
        UIActionPool *m_pActionPool;
    /** @} */

    /** Allow factory access to private/protected members: */
    friend class UICloudProfileManagerFactory;
};

#endif /* !FEQT_INCLUDED_SRC_cloud_profilemanager_UICloudProfileManager_h */

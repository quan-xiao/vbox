/* $Id: UIHostNetworkManager.h 86233 2020-09-23 12:10:51Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIHostNetworkManager class declaration.
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

#ifndef FEQT_INCLUDED_SRC_hostnetwork_UIHostNetworkManager_h
#define FEQT_INCLUDED_SRC_hostnetwork_UIHostNetworkManager_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QMainWindow>

/* GUI includes: */
#include "QIManagerDialog.h"
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class CHostNetworkInterface;
class QAbstractButton;
class QTreeWidgetItem;
class QIDialogButtonBox;
class QITreeWidget;
class UIActionPool;
class UIHostNetworkDetailsWidget;
class UIItemHostNetwork;
class QIToolBar;
struct UIDataHostNetwork;


/** QWidget extension providing GUI with the pane to control host network related functionality. */
class UIHostNetworkManagerWidget : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

signals:

    /** Notifies listeners about host network details-widget @a fVisible. */
    void sigHostNetworkDetailsVisibilityChanged(bool fVisible);
    /** Notifies listeners about host network details data @a fDiffers. */
    void sigHostNetworkDetailsDataChanged(bool fDiffers);

public:

    /** Constructs Host Network Manager widget.
      * @param  enmEmbedding  Brings the type of widget embedding.
      * @param  pActionPool   Brings the action-pool reference.
      * @param  fShowToolbar  Brings whether we should create/show toolbar. */
    UIHostNetworkManagerWidget(EmbedTo enmEmbedding, UIActionPool *pActionPool,
                               bool fShowToolbar = true, QWidget *pParent = 0);

    /** Returns the menu. */
    QMenu *menu() const;

#ifdef VBOX_WS_MAC
    /** Returns the toolbar. */
    QIToolBar *toolbar() const { return m_pToolBar; }
#endif

protected:

    /** @name Event-handling stuff.
      * @{ */
        /** Handles translation event. */
        virtual void retranslateUi() /* override */;

        /** Handles resize @a pEvent. */
        virtual void resizeEvent(QResizeEvent *pEvent) /* override */;

        /** Handles show @a pEvent. */
        virtual void showEvent(QShowEvent *pEvent) /* override */;
    /** @} */

public slots:

    /** @name Details-widget stuff.
      * @{ */
        /** Handles command to reset host network details changes. */
        void sltResetHostNetworkDetailsChanges();
        /** Handles command to apply host network details changes. */
        void sltApplyHostNetworkDetailsChanges();
    /** @} */

private slots:

    /** @name Menu/action stuff.
      * @{ */
        /** Handles command to create host network. */
        void sltCreateHostNetwork();
        /** Handles command to remove host network. */
        void sltRemoveHostNetwork();
        /** Handles command to make host network details @a fVisible. */
        void sltToggleHostNetworkDetailsVisibility(bool fVisible);
        /** Handles command to refresh host networks. */
        void sltRefreshHostNetworks();
    /** @} */

    /** @name Tree-widget stuff.
      * @{ */
        /** Handles command to adjust tree-widget. */
        void sltAdjustTreeWidget();

        /** Handles tree-widget @a pItem change. */
        void sltHandleItemChange(QTreeWidgetItem *pItem);
        /** Handles tree-widget current item change. */
        void sltHandleCurrentItemChange();
        /** Handles context menu request for tree-widget @a position. */
        void sltHandleContextMenuRequest(const QPoint &position);
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
        /** Load settings: */
        void loadSettings();
    /** @} */

    /** @name Loading stuff.
      * @{ */
        /** Loads host networks. */
        void loadHostNetworks();
        /** Loads host @a comInterface data to passed @a data container. */
        void loadHostNetwork(const CHostNetworkInterface &comInterface, UIDataHostNetwork &data);
    /** @} */

    /** @name Tree-widget stuff.
      * @{ */
        /** Creates a new tree-widget item on the basis of passed @a data, @a fChooseItem if requested. */
        void createItemForNetworkHost(const UIDataHostNetwork &data, bool fChooseItem);
        /** Updates the passed tree-widget item on the basis of passed @a data, @a fChooseItem if requested. */
        void updateItemForNetworkHost(const UIDataHostNetwork &data, bool fChooseItem, UIItemHostNetwork *pItem);
    /** @} */

    /** @name General variables.
      * @{ */
        /** Holds the widget embedding type. */
        const EmbedTo m_enmEmbedding;
        /** Holds the action-pool reference. */
        UIActionPool *m_pActionPool;
        /** Holds whether we should create/show toolbar. */
        const bool    m_fShowToolbar;
    /** @} */

    /** @name Toolbar and menu variables.
      * @{ */
        /** Holds the toolbar instance. */
        QIToolBar *m_pToolBar;
    /** @} */

    /** @name Splitter variables.
      * @{ */
        /** Holds the tree-widget instance. */
        QITreeWidget *m_pTreeWidget;
        /** Holds the details-widget instance. */
        UIHostNetworkDetailsWidget *m_pDetailsWidget;
    /** @} */
};


/** QIManagerDialogFactory extension used as a factory for Host Network Manager dialog. */
class UIHostNetworkManagerFactory : public QIManagerDialogFactory
{
public:

    /** Constructs Media Manager factory acquiring additional arguments.
      * @param  pActionPool  Brings the action-pool reference. */
    UIHostNetworkManagerFactory(UIActionPool *pActionPool = 0);

protected:

    /** Creates derived @a pDialog instance.
      * @param  pCenterWidget  Brings the widget reference to center according to. */
    virtual void create(QIManagerDialog *&pDialog, QWidget *pCenterWidget) /* override */;

    /** Holds the action-pool reference. */
    UIActionPool *m_pActionPool;
};


/** QIManagerDialog extension providing GUI with the dialog to control host network related functionality. */
class UIHostNetworkManager : public QIWithRetranslateUI<QIManagerDialog>
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

    /** Constructs Host Network Manager dialog.
      * @param  pCenterWidget  Brings the widget reference to center according to.
      * @param  pActionPool    Brings the action-pool reference. */
    UIHostNetworkManager(QWidget *pCenterWidget, UIActionPool *pActionPool);

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
        virtual UIHostNetworkManagerWidget *widget() /* override */;
    /** @} */

    /** @name Action related variables.
      * @{ */
        /** Holds the action-pool reference. */
        UIActionPool *m_pActionPool;
    /** @} */

    /** Allow factory access to private/protected members: */
    friend class UIHostNetworkManagerFactory;
};

#endif /* !FEQT_INCLUDED_SRC_hostnetwork_UIHostNetworkManager_h */


/* $Id: UIVisoCreator.h 86233 2020-09-23 12:10:51Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIVisoCreator class declaration.
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

#ifndef FEQT_INCLUDED_SRC_medium_viso_UIVisoCreator_h
#define FEQT_INCLUDED_SRC_medium_viso_UIVisoCreator_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QModelIndex>

/* GUI includes: */
#include "QIMainDialog.h"
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class QMenu;
class QGridLayout;
class QIDialogButtonBox;
class UIDialogPanel;
class QIToolBar;
class UIVisoHostBrowser;
class UIVisoContentBrowser;
class UIVisoCreatorOptionsPanel;
class UIVisoConfigurationPanel;

/** A QIMainDialog extension. It hosts two UIVisoBrowserBase extensions, one for host and one
  * for VISO file system. It has the main menu, main toolbar, and a vertical toolbar and corresponding
  * actions. */
class UIVisoCreator : public QIWithRetranslateUI<QIMainDialog>
{
    Q_OBJECT;

public:

    UIVisoCreator(QWidget *pParent = 0, const QString& strMachineName = QString());
    /** Returns the content of the .viso file. Each element of the list corresponds to a line in the .viso file. */
    QStringList       entryList() const;
    const QString     &visoName() const;
    /** Returns custom ISO options (if any). */
    const QStringList &customOptions() const;
    /** Returns the current path that the host browser is listing. */
    QString currentPath() const;
    void    setCurrentPath(const QString &strPath);

#ifdef VBOX_WS_MAC
    /** Returns the toolbar. */
    QIToolBar *toolbar() const { return m_pToolBar; }
#endif

protected:

    virtual void retranslateUi() /* override */;

private slots:

    void sltHandleAddObjectsToViso(QStringList pathList);
    void sltPanelActionToggled(bool fChecked);
    void sltHandleVisoNameChanged(const QString& strVisoName);
    void sltHandleCustomVisoOptionsChanged(const QStringList &customVisoOptions);
    void sltHandleShowHiddenObjectsChange(bool fShow);
    void sltHandleHidePanel(UIDialogPanel *pPanel);
    void sltHandleBrowserTreeViewVisibilityChanged(bool fVisible);
    void sltHandleHostBrowserTableSelectionChanged(bool fIsSelectionEmpty);
    void sltHandleContentBrowserTableSelectionChanged(bool fIsSelectionEmpty);
    void sltHandleShowContextMenu(const QWidget *pContextMenuRequester, const QPoint &point);

private:

    struct VisoOptions
    {
        VisoOptions()
            :m_strVisoName("ad-hoc-viso"){}
        QString m_strVisoName;
        /** Additions viso options to be inserted to the viso file as separate lines. */
        QStringList m_customOptions;
    };

    struct BrowserOptions
    {
        BrowserOptions()
            :m_fShowHiddenObjects(true){}
        bool m_fShowHiddenObjects;
    };

    void prepareWidgets();
    void prepareConnections();
    void prepareActions();
    /** Creates and configures the vertical toolbar. Should be called after prepareActions() */
    void prepareVerticalToolBar();
    /* Populates the main menu and toolbard with already created actions.
     * Leave out the vertical toolbar which is handled in prepareVerticalToolBar. */
    void populateMenuMainToolbar();
    /** Set the root index of the m_pTableModel to the current index of m_pTreeModel. */
    void setTableRootIndex(QModelIndex index = QModelIndex() );
    void setTreeCurrentIndex(QModelIndex index = QModelIndex() );
    void hidePanel(UIDialogPanel *panel);
    void showPanel(UIDialogPanel *panel);
    /** Makes sure escape key is assigned to only a single widget. This is done by checking
      *  several things in the following order:
      *  - when (drop-down) tree views of browser panes are visible esc. key used to close those. thus it is taken from the dialog and panels
      *  - when there are no more panels visible assign it to the parent dialog
      *  - grab it from the dialog as soon as a panel becomes visible again
      *  - assign it to the most recently "unhidden" panel */
    void manageEscapeShortCut();

    /** @name Main toolbar (and main menu) actions
      * @{ */
        QAction         *m_pActionConfiguration;
        QAction         *m_pActionOptions;
    /** @} */

    /** @name These actions are addded to vertical toolbar, context menus, and the main menu.
      * @{ */
        QAction              *m_pAddAction;
        QAction              *m_pRemoveAction;
        QAction              *m_pNewDirectoryAction;
        QAction              *m_pRenameAction;
        QAction              *m_pResetAction;
    /** @} */

    QGridLayout          *m_pMainLayout;
    UIVisoHostBrowser    *m_pHostBrowser;
    UIVisoContentBrowser *m_pVisoBrowser;
    QIDialogButtonBox    *m_pButtonBox;
    QIToolBar            *m_pToolBar;
    QIToolBar            *m_pVerticalToolBar;
    VisoOptions           m_visoOptions;
    BrowserOptions        m_browserOptions;
    QWidget              *m_pCentralWidget;
    QMenu                *m_pMainMenu;
    QString               m_strMachineName;
    UIVisoCreatorOptionsPanel *m_pCreatorOptionsPanel;
    UIVisoConfigurationPanel  *m_pConfigurationPanel;
    QMap<UIDialogPanel*, QAction*> m_panelActionMap;
    QList<UIDialogPanel*>          m_visiblePanelsList;
};

#endif /* !FEQT_INCLUDED_SRC_medium_viso_UIVisoCreator_h */

/* $Id: UIVMLogViewerBookmarksPanel.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIVMLogViewer class declaration.
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

#ifndef FEQT_INCLUDED_SRC_logviewer_UIVMLogViewerBookmarksPanel_h
#define FEQT_INCLUDED_SRC_logviewer_UIVMLogViewerBookmarksPanel_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIVMLogViewerPanel.h"

/* Forward declarations: */
class QComboBox;
class QWidget;
class QIToolButton;

/** UIVMLogViewerPanel extension providing GUI for bookmark management. Show a list of bookmarks currently set
 *  for displayed log page. It has controls to navigate and clear bookmarks. */
class UIVMLogViewerBookmarksPanel : public UIVMLogViewerPanel
{
    Q_OBJECT;

public:

    UIVMLogViewerBookmarksPanel(QWidget *pParent, UIVMLogViewerWidget *pViewer);

    /** Adds a single bookmark to an existing list of bookmarks. Possibly called
     *  by UIVMLogViewerWidget when user adds a bookmark thru context menu etc. */
    void addBookmark(const QPair<int, QString> &newBookmark);
    /** Clear the bookmark list and show this list instead. Probably done after
     *  user switches to another log page tab etc. */
    void setBookmarksList(const QVector<QPair<int, QString> > &bookmarkList);
    void updateBookmarkList(const QVector<QPair<int, QString> > &bookmarkVector);
    /** Disable/enable all the widget except the close button */
    void disableEnableBookmarking(bool flag);
    virtual QString panelName() const /* override */;
signals:

    void sigDeleteBookmark(int bookmarkIndex);
    void sigDeleteAllBookmarks();
    void sigBookmarkSelected(int index);

protected:

    virtual void prepareWidgets() /* override */;
    virtual void prepareConnections() /* override */;

    /** Handles the translation event. */
    void retranslateUi();

private slots:

    void sltDeleteCurrentBookmark();
    void sltBookmarkSelected(int index);
    void sltGotoNextBookmark();
    void sltGotoPreviousBookmark();
    void sltGotoSelectedBookmark();

private:

    /** @a index is the index of the curent bookmark. */
    void setBookmarkIndex(int index);

    const int     m_iMaxBookmarkTextLength;
    QComboBox    *m_pBookmarksComboBox;
    QIToolButton *m_pGotoSelectedBookmark;
    QIToolButton *m_pDeleteAllButton;
    QIToolButton *m_pDeleteCurrentButton;
    QIToolButton *m_pNextButton;
    QIToolButton *m_pPreviousButton;
};

#endif /* !FEQT_INCLUDED_SRC_logviewer_UIVMLogViewerBookmarksPanel_h */

/* $Id: UIGuestProcessControlWidget.h 86233 2020-09-23 12:10:51Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIGuestProcessControlWidget class declaration.
 */

/*
 * Copyright (C) 2016-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_guestctrl_UIGuestProcessControlWidget_h
#define FEQT_INCLUDED_SRC_guestctrl_UIGuestProcessControlWidget_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QWidget>

/* COM includes: */
#include "COMEnums.h"
#include "CGuest.h"
#include "CEventListener.h"

/* GUI includes: */
#include "QIManagerDialog.h"
#include "QIWithRetranslateUI.h"
#include "UIMainEventListener.h"

/* Forward declarations: */
class QITreeWidget;
class QVBoxLayout;
class QSplitter;
class UIGuestControlConsole;
class UIGuestControlInterface;
class UIGuestSessionsEventHandler;
class UIGuestControlTreeWidget;
class QIToolBar;

/** QWidget extension
  * providing GUI with guest session information and control tab in session-information window. */
class UIGuestProcessControlWidget : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

public:

    UIGuestProcessControlWidget(EmbedTo enmEmbedding, const CGuest &comGuest, QWidget *pParent,
                                QString strMachineName = QString(), bool fShowToolbar = false);
    ~UIGuestProcessControlWidget();
    /** When true we delete the corresponding tree item as soon as the guest session/process is unregistered. */
    static const bool           m_fDeleteAfterUnregister;

protected:

    void retranslateUi();

private slots:

    void sltGuestSessionsUpdated();
    void sltGuestSessionRegistered(CGuestSession guestSession);
    void sltGuestSessionUnregistered(CGuestSession guestSession);
    void sltTreeItemUpdated();
    void sltCloseSessionOrProcess();
    void sltShowProperties();

private:

    void prepareObjects();
    void prepareConnections();
    void prepareToolBar();
    void prepareListener();
    void initGuestSessionTree();
    void updateTreeWidget();
    void cleanupListener();
    void addGuestSession(CGuestSession guestSession);
    void saveSettings();
    void loadSettings();

    CGuest                    m_comGuest;
    QVBoxLayout              *m_pMainLayout;
    QSplitter                *m_pSplitter;
    UIGuestControlTreeWidget *m_pTreeWidget;
    const EmbedTo             m_enmEmbedding;
    QIToolBar                *m_pToolBar;

    /** Holds the Qt event listener instance. */
    ComObjPtr<UIMainEventListenerImpl> m_pQtListener;
    /** Holds the COM event listener instance. */
    CEventListener m_comEventListener;
    const bool     m_fShowToolbar;
    QString        m_strMachineName;
};

#endif /* !FEQT_INCLUDED_SRC_guestctrl_UIGuestProcessControlWidget_h */

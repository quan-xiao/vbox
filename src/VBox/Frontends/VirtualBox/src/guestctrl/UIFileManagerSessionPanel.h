/* $Id: UIFileManagerSessionPanel.h 82968 2020-02-04 10:35:17Z vboxsync $ */
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

#ifndef FEQT_INCLUDED_SRC_guestctrl_UIFileManagerSessionPanel_h
#define FEQT_INCLUDED_SRC_guestctrl_UIFileManagerSessionPanel_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIDialogPanel.h"

/* Forward declarations: */
class UIGuestSessionCreateWidget;

/** UIDialogPanel extension providing GUI for creating/stopping a guest session. */
class UIFileManagerSessionPanel : public UIDialogPanel
{
    Q_OBJECT;

signals:

    void sigCreateSession(QString strUserName, QString strPassword);
    void sigCloseSession();

public:

    UIFileManagerSessionPanel(QWidget *pParent = 0);
    /** @name Enable/disable member widget wrt. guest session status.
      * @{ */
        void switchSessionCloseMode();
        void switchSessionCreateMode();
    /** @} */
    virtual QString panelName() const /* override */;
    void markForError(bool fMarkForError);

protected:

    virtual void prepareWidgets() /* override */;
    virtual void prepareConnections() /* override */;
    void retranslateUi();
    void showEvent(QShowEvent *pEvent) /* override */;

private:

    UIGuestSessionCreateWidget *m_pSessionCreateWidget;
};

#endif /* !FEQT_INCLUDED_SRC_guestctrl_UIFileManagerSessionPanel_h */

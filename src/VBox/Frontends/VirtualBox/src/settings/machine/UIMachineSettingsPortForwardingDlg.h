/* $Id: UIMachineSettingsPortForwardingDlg.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsPortForwardingDlg class declaration.
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

#ifndef FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsPortForwardingDlg_h
#define FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsPortForwardingDlg_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "QIDialog.h"
#include "UIPortForwardingTable.h"

/* Forward declarations: */
class QIDialogButtonBox;

/* Machine settings / Network page / NAT attachment / Port forwarding dialog: */
class SHARED_LIBRARY_STUFF UIMachineSettingsPortForwardingDlg : public QIWithRetranslateUI<QIDialog>
{
    Q_OBJECT;

public:

    /* Constructor/destructor: */
    UIMachineSettingsPortForwardingDlg(QWidget *pParent, const UIPortForwardingDataList &rules);

    /* API: Rules stuff: */
    const UIPortForwardingDataList rules() const;

private slots:

    /* Handlers: Dialog stuff: */
    void accept();
    void reject();

private:

    /* Handler: Translation stuff: */
    void retranslateUi();

    /* Widgets: */
    UIPortForwardingTable *m_pTable;
    QIDialogButtonBox *m_pButtonBox;
};

#endif /* !FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsPortForwardingDlg_h */

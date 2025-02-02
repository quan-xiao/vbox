/* $Id: UICloudMachineSettingsDialog.cpp 84014 2020-04-27 14:57:32Z vboxsync $ */
/** @file
 * VBox Qt GUI - UICloudMachineSettingsDialog class implementation.
 */

/*
 * Copyright (C) 2020 Oracle Corporation
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
#include <QPushButton>
#include <QVBoxLayout>

/* GUI includes: */
#include "QIDialogButtonBox.h"
#include "UIMessageCenter.h"
#include "UICloudMachineSettingsDialog.h"
#include "UICloudMachineSettingsDialogPage.h"
#include "UICloudNetworkingStuff.h"

/* COM includes: */
#include "CProgress.h"


UICloudMachineSettingsDialog::UICloudMachineSettingsDialog(QWidget *pParent, const CCloudMachine &comCloudMachine)
    : QIWithRetranslateUI<QDialog>(pParent)
    , m_comCloudMachine(comCloudMachine)
    , m_pPage(0)
    , m_pButtonBox(0)
{
    prepare();
}

int UICloudMachineSettingsDialog::exec()
{
    /* Request dialog initialization: */
    QMetaObject::invokeMethod(this, "sltRefresh", Qt::QueuedConnection);

    /* Call to base-class: */
    return QIWithRetranslateUI<QDialog>::exec();
}

void UICloudMachineSettingsDialog::accept()
{
    /* Makes sure page data committed: */
    if (m_pPage)
        m_pPage->makeSureDataCommitted();

    /* Apply form: */
    AssertReturnVoid(m_comForm.isNotNull());
    if (!applyCloudMachineSettingsForm(m_comCloudMachine, m_comForm, this))
        return;

    /* Call to base-class: */
    QIWithRetranslateUI<QDialog>::accept();
}

void UICloudMachineSettingsDialog::retranslateUi()
{
    /* Translate title: */
    const QString strCaption = tr("Settings");
    if (m_strName.isNull())
        setWindowTitle(strCaption);
    else
        setWindowTitle(tr("%1 - %2").arg(m_strName, strCaption));
}

void UICloudMachineSettingsDialog::setOkButtonEnabled(bool fEnabled)
{
    AssertPtrReturnVoid(m_pButtonBox);
    AssertPtrReturnVoid(m_pButtonBox->button(QDialogButtonBox::Ok));
    m_pButtonBox->button(QDialogButtonBox::Ok)->setEnabled(fEnabled);
}

void UICloudMachineSettingsDialog::sltRefresh()
{
    /* Update name: */
    if (!cloudMachineName(m_comCloudMachine, m_strName, this))
        reject();

    /* Retranslate title: */
    retranslateUi();

    /* Update form: */
    if (!cloudMachineSettingsForm(m_comCloudMachine, m_comForm, this))
        reject();

    /* Assign page with form: */
    m_pPage->setForm(m_comForm);
}

void UICloudMachineSettingsDialog::prepare()
{
    /* Prepare layout: */
    QVBoxLayout *pLayout = new QVBoxLayout(this);
    if (pLayout)
    {
        /* Prepare page: */
        m_pPage = new UICloudMachineSettingsDialogPage(this);
        if (m_pPage)
        {
            connect(m_pPage.data(), &UICloudMachineSettingsDialogPage::sigValidChanged,
                    this, &UICloudMachineSettingsDialog::setOkButtonEnabled);
            /* Add into layout: */
            pLayout->addWidget(m_pPage);
        }

        /* Prepare button-box: */
        m_pButtonBox = new QIDialogButtonBox;
        if (m_pButtonBox)
        {
            m_pButtonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
            m_pButtonBox->button(QDialogButtonBox::Cancel)->setShortcut(Qt::Key_Escape);
            connect(m_pButtonBox, &QIDialogButtonBox::accepted, this, &UICloudMachineSettingsDialog::accept);
            connect(m_pButtonBox, &QIDialogButtonBox::rejected, this, &UICloudMachineSettingsDialog::reject);
            setOkButtonEnabled(false);
            /* Add into layout: */
            pLayout->addWidget(m_pButtonBox);
        }
    }

    /* Apply language settings: */
    retranslateUi();
}

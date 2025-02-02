/* $Id: UIWizardFirstRunPageBasic.cpp 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardFirstRunPageBasic class implementation.
 */

/*
 * Copyright (C) 2008-2020 Oracle Corporation
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
#include <QHBoxLayout>
#include <QVBoxLayout>

/* GUI includes: */
#include "QIRichTextLabel.h"
#include "QIToolButton.h"
#include "UICommon.h"
#include "UIIconPool.h"
#include "UIMediaComboBox.h"
#include "UIMedium.h"
#include "UIMessageCenter.h"
#include "UIWizardFirstRun.h"
#include "UIWizardFirstRunPageBasic.h"


UIWizardFirstRunPage::UIWizardFirstRunPage(bool fBootHardDiskWasSet)
    : m_fBootHardDiskWasSet(fBootHardDiskWasSet)
{
}

void UIWizardFirstRunPage::onOpenMediumWithFileOpenDialog()
{
    /* Get opened vboxMedium id: */
    QUuid uMediumId;
    uiCommon().openMediumSelectorDialog(thisImp(), UIMediumDeviceType_DVD, uMediumId, "", "", "", true);
    /* Update medium-combo if necessary: */
    if (!uMediumId.isNull())
        m_pMediaSelector->setCurrentItem(uMediumId);
}

QUuid UIWizardFirstRunPage::id() const
{
    return m_pMediaSelector->id();
}

void UIWizardFirstRunPage::setId(const QUuid &uId)
{
    m_pMediaSelector->setCurrentItem(uId);
}

UIWizardFirstRunPageBasic::UIWizardFirstRunPageBasic(const QUuid &uMachineId, bool fBootHardDiskWasSet)
    : UIWizardFirstRunPage(fBootHardDiskWasSet)
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        m_pLabel = new QIRichTextLabel(this);
        QHBoxLayout *pSourceDiskLayout = new QHBoxLayout;
        {
            m_pMediaSelector = new UIMediaComboBox(this);
            {
                m_pMediaSelector->setMachineId(uMachineId);
                m_pMediaSelector->setType(UIMediumDeviceType_DVD);
                m_pMediaSelector->repopulate();
            }
            m_pSelectMediaButton = new QIToolButton(this);
            {
                m_pSelectMediaButton->setIcon(UIIconPool::iconSet(":/select_file_16px.png", ":/select_file_disabled_16px.png"));
                m_pSelectMediaButton->setAutoRaise(true);
            }
            pSourceDiskLayout->addWidget(m_pMediaSelector);
            pSourceDiskLayout->addWidget(m_pSelectMediaButton);
        }
        pMainLayout->addWidget(m_pLabel);
        pMainLayout->addLayout(pSourceDiskLayout);
        pMainLayout->addStretch();
    }

    /* Setup connections: */
    connect(m_pMediaSelector, static_cast<void(UIMediaComboBox::*)(int)>(&UIMediaComboBox::currentIndexChanged),
            this, &UIWizardFirstRunPageBasic::completeChanged);
    connect(m_pSelectMediaButton, &QIToolButton::clicked,
            this, &UIWizardFirstRunPageBasic::sltOpenMediumWithFileOpenDialog);

    /* Register fields: */
    registerField("source", this, "source");
    registerField("id", this, "id");
}

void UIWizardFirstRunPageBasic::sltOpenMediumWithFileOpenDialog()
{
    /* Call to base-class: */
    onOpenMediumWithFileOpenDialog();
}

void UIWizardFirstRunPageBasic::retranslateUi()
{
    /* Translate widgets: */
    if (m_fBootHardDiskWasSet)
        m_pLabel->setText(UIWizardFirstRun::tr("<p>Please select a virtual optical disk file "
                                               "or a physical optical drive containing a disk "
                                               "to start your new virtual machine from.</p>"
                                               "<p>The disk should be suitable for starting a computer from "
                                               "and should contain the operating system you wish to install "
                                               "on the virtual machine if you want to do that now. "
                                               "The disk will be ejected from the virtual drive "
                                               "automatically next time you switch the virtual machine off, "
                                               "but you can also do this yourself if needed using the Devices menu.</p>"));
    else
        m_pLabel->setText(UIWizardFirstRun::tr("<p>Please select a virtual optical disk file "
                                               "or a physical optical drive containing a disk "
                                               "to start your new virtual machine from.</p>"
                                               "<p>The disk should be suitable for starting a computer from. "
                                               "As this virtual machine has no hard drive "
                                               "you will not be able to install an operating system on it at the moment.</p>"));
    m_pSelectMediaButton->setToolTip(UIWizardFirstRun::tr("Choose a virtual optical disk file..."));
}

void UIWizardFirstRunPageBasic::initializePage()
{
    /* Translate page: */
    retranslateUi();
}

bool UIWizardFirstRunPageBasic::isComplete() const
{
    /* Make sure valid medium chosen: */
    return !uiCommon().medium(id()).isNull();
}

bool UIWizardFirstRunPageBasic::validatePage()
{
    /* Initial result: */
    bool fResult = true;

    /* Lock finish button: */
    startProcessing();

    /* Try to insert chosen medium: */
    if (fResult)
        fResult = qobject_cast<UIWizardFirstRun*>(wizard())->insertMedium();

    /* Unlock finish button: */
    endProcessing();

    /* Return result: */
    return fResult;
}

QString UIWizardFirstRunPageBasic::source() const
{
    return m_pMediaSelector->currentText();
}

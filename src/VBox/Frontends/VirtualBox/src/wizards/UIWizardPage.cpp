/* $Id: UIWizardPage.cpp 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardPage class implementation.
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

/* Qt includes: */
#include <QAbstractButton>

/* GUI includes: */
#include "UICommon.h"
#include "UIWizard.h"
#include "UIWizardPage.h"


/*********************************************************************************************************************************
*   Class UIWizardPageBase implementation.                                                                                       *
*********************************************************************************************************************************/

UIWizard *UIWizardPageBase::wizardImp() const
{
    /* Should be reimplemented in sub-class to enable access to wizard! */
    AssertMsgFailed(("UIWizardPageBase::wizardImp() should be reimplemented!"));
    return 0;
}

UIWizardPage *UIWizardPageBase::thisImp()
{
    /* Should be reimplemented in sub-class to enable access to wizard page! */
    AssertMsgFailed(("UIWizardPageBase::thisImp() should be reimplemented!"));
    return 0;
}

QVariant UIWizardPageBase::fieldImp(const QString &) const
{
    /* Should be reimplemented in sub-class to enable access to wizard field! */
    AssertMsgFailed(("UIWizardPageBase::fieldImp(const QString &) should be reimplemented!"));
    return QVariant();
}


/*********************************************************************************************************************************
*   Class UIWizardPage implementation.                                                                                           *
*********************************************************************************************************************************/

UIWizardPage::UIWizardPage()
    : m_fReady(false)
{
}

void UIWizardPage::markReady()
{
    m_fReady = true;
    QWizardPage::setTitle(m_strTitle);
}

void UIWizardPage::setTitle(const QString &strTitle)
{
    m_strTitle = strTitle;
    if (m_fReady)
        QWizardPage::setTitle(m_strTitle);
}

UIWizard *UIWizardPage::wizard() const
{
    return qobject_cast<UIWizard*>(QWizardPage::wizard());
}

void UIWizardPage::startProcessing()
{
    if (isFinalPage())
        wizard()->button(QWizard::FinishButton)->setEnabled(false);
}

void UIWizardPage::endProcessing()
{
    if (isFinalPage())
        wizard()->button(QWizard::FinishButton)->setEnabled(true);
}

/* $Id: UIWizardNewCloudVM.cpp 86573 2020-10-14 12:06:06Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardNewCloudVM class implementation.
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
#include "UIMessageCenter.h"
#include "UIWizardNewCloudVM.h"
#include "UIWizardNewCloudVMPageBasic1.h"
#include "UIWizardNewCloudVMPageBasic2.h"
#include "UIWizardNewCloudVMPageExpert.h"

/* COM includes: */
#include "CCloudMachine.h"
#include "CProgress.h"


UIWizardNewCloudVM::UIWizardNewCloudVM(QWidget *pParent,
                                       const QString &strFullGroupName /* = QString() */,
                                       const CCloudClient &comClient /* = CCloudClient() */,
                                       const CVirtualSystemDescription &comDescription /* = CVirtualSystemDescription() */,
                                       WizardMode enmMode /* = WizardMode_Auto */)
    : UIWizard(pParent, WizardType_NewCloudVM, enmMode)
    , m_strFullGroupName(strFullGroupName)
    , m_comClient(comClient)
    , m_comVSD(comDescription)
    , m_fFullWizard(m_comClient.isNull() || m_comVSD.isNull())
    , m_fFinalStepPrevented(false)
{
#ifndef VBOX_WS_MAC
    /* Assign watermark: */
    assignWatermark(":/wizard_new_cloud_vm.png");
#else
    /* Assign background image: */
    assignBackground(":/wizard_new_cloud_vm_bg.png");
#endif
}

void UIWizardNewCloudVM::prepare()
{
    /* Create corresponding pages: */
    switch (mode())
    {
        case WizardMode_Basic:
        {
            if (m_fFullWizard)
                setPage(Page1, new UIWizardNewCloudVMPageBasic1);
            setPage(Page2, new UIWizardNewCloudVMPageBasic2(m_fFullWizard));
            break;
        }
        case WizardMode_Expert:
        {
            setPage(PageExpert, new UIWizardNewCloudVMPageExpert(m_fFullWizard));
            break;
        }
        default:
        {
            AssertMsgFailed(("Invalid mode: %d", mode()));
            break;
        }
    }
    /* Call to base-class: */
    UIWizard::prepare();
}

bool UIWizardNewCloudVM::createVSDForm()
{
    /* Prepare result: */
    bool fResult = false;

    /* Main API request sequence, can be interrupted after any step: */
    do
    {
        /* Acquire prepared client and description: */
        CCloudClient comClient = client();
        CVirtualSystemDescription comDescription = vsd();
        AssertReturn(comClient.isNotNull() && comDescription.isNotNull(), false);

        /* Read Cloud Client description form: */
        CVirtualSystemDescriptionForm comForm;
        CProgress comProgress = comClient.GetLaunchDescriptionForm(comDescription, comForm);
        if (!comClient.isOk())
        {
            msgCenter().cannotAcquireCloudClientParameter(comClient);
            break;
        }

        /* Show "Acquire launch form" progress: */
        msgCenter().showModalProgressDialog(comProgress, QString(), ":/progress_refresh_90px.png", this, 0);
        if (!comProgress.isOk() || comProgress.GetResultCode() != 0)
        {
            msgCenter().cannotAcquireCloudClientParameter(comProgress);
            break;
        }

        /* Remember Virtual System Description Form: */
        setVSDForm(comForm);

        /* Finally, success: */
        fResult = true;
    }
    while (0);

    /* Return result: */
    return fResult;
}

bool UIWizardNewCloudVM::createCloudVM()
{
    /* Prepare result: */
    bool fResult = false;

    /* Main API request sequence, can be interrupted after any step: */
    do
    {
        /* Do nothing if prevented: */
        if (m_fFinalStepPrevented)
        {
            fResult = true;
            break;
        }

        /* Acquire prepared client and description: */
        CCloudClient comClient = client();
        CVirtualSystemDescription comDescription = vsd();
        AssertReturn(comClient.isNotNull() && comDescription.isNotNull(), false);

        /* Initiate cloud VM creation procedure: */
        CCloudMachine comMachine;
        CProgress comProgress = comClient.CreateCloudMachine(comDescription, comMachine);
        if (!comClient.isOk())
        {
            msgCenter().cannotCreateCloudMachine(comClient, this);
            break;
        }

        /* Show "Create Cloud Machine" progress: */
        msgCenter().showModalProgressDialog(comProgress, QString(), ":/progress_new_cloud_vm_90px.png", this, 0);
        if (comProgress.GetCanceled())
            break;
        if (!comProgress.isOk() || comProgress.GetResultCode() != 0)
        {
            msgCenter().cannotCreateCloudMachine(comProgress, this);
            break;
        }

        /* Check whether VM really added: */
        if (comMachine.isNotNull())
            uiCommon().notifyCloudMachineRegistered(field("location").toString(),
                                                    field("profileName").toString(),
                                                    comMachine);

        /* Finally, success: */
        fResult = true;
    }
    while (0);

    /* Return result: */
    return fResult;
}

void UIWizardNewCloudVM::scheduleAutoFinish()
{
    QMetaObject::invokeMethod(this, "sltTriggerFinishButton", Qt::QueuedConnection);
}

void UIWizardNewCloudVM::retranslateUi()
{
    /* Call to base-class: */
    UIWizard::retranslateUi();

    /* Translate wizard: */
    setWindowTitle(tr("Create Cloud Virtual Machine"));
    setButtonText(QWizard::FinishButton, tr("Create"));
}

void UIWizardNewCloudVM::sltTriggerFinishButton()
{
    button(QWizard::FinishButton)->click();
}

/* $Id: UIVirtualMachineItemCloud.cpp 86779 2020-11-02 11:35:12Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIVirtualMachineItemCloud class implementation.
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

/* Qt includes: */
#include <QPointer>
#include <QTimer>

/* GUI includes: */
#include "UICloudNetworkingStuff.h"
#include "UICommon.h"
#include "UIConverter.h"
#include "UIErrorString.h"
#include "UIIconPool.h"
#include "UIMessageCenter.h"
#include "UIProgressTask.h"
#include "UIThreadPool.h"
#include "UIVirtualMachineItemCloud.h"

/* COM includes: */
#include "CProgress.h"
#include "CVirtualBoxErrorInfo.h"


/** UIProgressTask extension performing cloud machine refresh task. */
class UIProgressTaskRefreshCloudMachine : public UIProgressTask
{
    Q_OBJECT;

public:

    /** Constructs @a comCloudMachine refresh task passing @a pParent to the base-class. */
    UIProgressTaskRefreshCloudMachine(QObject *pParent, const CCloudMachine &comCloudMachine);

protected:

    /** Creates and returns started progress-wrapper required to init UIProgressObject. */
    virtual CProgress createProgress() /* override */;
    /** Handles finished @a comProgress wrapper. */
    virtual void handleProgressFinished(CProgress &comProgress) /* override */;

private:

    /** Holds the cloud machine wrapper. */
    CCloudMachine  m_comCloudMachine;
};


/*********************************************************************************************************************************
*   Class UIProgressTaskRefreshCloudMachine implementation.                                                                      *
*********************************************************************************************************************************/

UIProgressTaskRefreshCloudMachine::UIProgressTaskRefreshCloudMachine(QObject *pParent, const CCloudMachine &comCloudMachine)
    : UIProgressTask(pParent)
    , m_comCloudMachine(comCloudMachine)
{
}

CProgress UIProgressTaskRefreshCloudMachine::createProgress()
{
    CProgress comProgress = m_comCloudMachine.Refresh();
    if (!m_comCloudMachine.isOk())
        msgCenter().cannotAcquireCloudMachineParameter(m_comCloudMachine);
    return comProgress;
}

void UIProgressTaskRefreshCloudMachine::handleProgressFinished(CProgress &comProgress)
{
    if (!comProgress.GetCanceled() && (!comProgress.isOk() || comProgress.GetResultCode() != 0))
        msgCenter().cannotAcquireCloudMachineParameter(comProgress);
}


/*********************************************************************************************************************************
*   Class UIVirtualMachineItemCloud implementation.                                                                              *
*********************************************************************************************************************************/

UIVirtualMachineItemCloud::UIVirtualMachineItemCloud(UIFakeCloudVirtualMachineItemState enmState)
    : UIVirtualMachineItem(UIVirtualMachineItemType_CloudFake)
    , m_enmMachineState(KCloudMachineState_Invalid)
    , m_enmFakeCloudItemState(enmState)
    , m_fRefreshScheduled(false)
    , m_pProgressTaskRefresh(0)
{
    prepare();
}

UIVirtualMachineItemCloud::UIVirtualMachineItemCloud(const CCloudMachine &comCloudMachine)
    : UIVirtualMachineItem(UIVirtualMachineItemType_CloudReal)
    , m_comCloudMachine(comCloudMachine)
    , m_enmMachineState(KCloudMachineState_Invalid)
    , m_enmFakeCloudItemState(UIFakeCloudVirtualMachineItemState_NotApplicable)
    , m_fRefreshScheduled(false)
    , m_pProgressTaskRefresh(0)
{
    prepare();
}

UIVirtualMachineItemCloud::~UIVirtualMachineItemCloud()
{
    cleanup();
}

void UIVirtualMachineItemCloud::setFakeCloudItemState(UIFakeCloudVirtualMachineItemState enmState)
{
    m_enmFakeCloudItemState = enmState;
    recache();
}

void UIVirtualMachineItemCloud::setFakeCloudItemErrorMessage(const QString &strErrorMessage)
{
    m_strFakeCloudItemErrorMessage = strErrorMessage;
    recache();
}

void UIVirtualMachineItemCloud::updateInfoAsync(bool fDelayed, bool fSubscribe /* = false */)
{
    /* Ignore refresh request if progress-task is absent: */
    if (!m_pProgressTaskRefresh)
        return;

    /* Mark update scheduled if requested: */
    if (fSubscribe)
        m_fRefreshScheduled = true;

    /* Schedule refresh request in a 10 or 0 seconds
     * if progress-task isn't already scheduled or running: */
    if (   !m_pProgressTaskRefresh->isScheduled()
        && !m_pProgressTaskRefresh->isRunning())
        m_pProgressTaskRefresh->schedule(fDelayed ? 10000 : 0);
}

void UIVirtualMachineItemCloud::stopAsyncUpdates()
{
    /* Ignore cancel request if progress-task is absent: */
    if (!m_pProgressTaskRefresh)
        return;

    /* Mark update canceled in any case: */
    m_fRefreshScheduled = false;
}

void UIVirtualMachineItemCloud::waitForAsyncInfoUpdateFinished()
{
    /* Ignore cancel request if progress-task is absent: */
    if (!m_pProgressTaskRefresh)
        return;

    /* Mark update canceled in any case: */
    m_fRefreshScheduled = false;

    /* Cancel refresh request
     * if progress-task already running: */
    if (m_pProgressTaskRefresh->isRunning())
        m_pProgressTaskRefresh->cancel();
}

void UIVirtualMachineItemCloud::recache()
{
    switch (itemType())
    {
        case UIVirtualMachineItemType_CloudFake:
        {
            /* Make sure cloud VM is NOT set: */
            AssertReturnVoid(m_comCloudMachine.isNull());

            /* Determine ID/name: */
            m_uId = QUuid();
            m_strName = QString();

            /* Determine whether VM is accessible: */
            m_fAccessible = m_strFakeCloudItemErrorMessage.isNull();
            m_strAccessError = m_strFakeCloudItemErrorMessage;

            /* Determine VM OS type: */
            m_strOSTypeId = "Other";

            /* Determine VM states: */
            m_enmMachineState = KCloudMachineState_Stopped;
            m_strMachineStateName = gpConverter->toString(m_enmMachineState);
            switch (m_enmFakeCloudItemState)
            {
                case UIFakeCloudVirtualMachineItemState_Loading:
                    m_machineStateIcon = UIIconPool::iconSet(":/state_loading_16px.png");
                    break;
                case UIFakeCloudVirtualMachineItemState_Done:
                    m_machineStateIcon = UIIconPool::iconSet(":/vm_new_16px.png");
                    break;
                default:
                    break;
            }

            /* Determine configuration access level: */
            m_enmConfigurationAccessLevel = ConfigurationAccessLevel_Null;

            /* Determine whether we should show this VM details: */
            m_fHasDetails = true;

            break;
        }
        case UIVirtualMachineItemType_CloudReal:
        {
            /* Make sure cloud VM is set: */
            AssertReturnVoid(m_comCloudMachine.isNotNull());

            /* Determine ID/name: */
            m_uId = m_comCloudMachine.GetId();
            m_strName = m_comCloudMachine.GetName();

            /* Determine whether VM is accessible: */
            m_fAccessible = m_comCloudMachine.GetAccessible();
            m_strAccessError = !m_fAccessible ? UIErrorString::formatErrorInfo(m_comCloudMachine.GetAccessError()) : QString();

            /* Determine VM OS type: */
            m_strOSTypeId = m_fAccessible ? m_comCloudMachine.GetOSTypeId() : "Other";

            /* Determine VM states: */
            m_enmMachineState = m_fAccessible ? m_comCloudMachine.GetState() : KCloudMachineState_Stopped;
            m_strMachineStateName = gpConverter->toString(m_enmMachineState);
            m_machineStateIcon = gpConverter->toIcon(m_enmMachineState);

            /* Determine configuration access level: */
            m_enmConfigurationAccessLevel = m_fAccessible ? ConfigurationAccessLevel_Full : ConfigurationAccessLevel_Null;

            /* Determine whether we should show this VM details: */
            m_fHasDetails = true;

            break;
        }
        default:
        {
            AssertFailed();
            break;
        }
    }

    /* Recache item pixmap: */
    recachePixmap();

    /* Retranslate finally: */
    retranslateUi();
}

void UIVirtualMachineItemCloud::recachePixmap()
{
    /* We are using icon corresponding to cached guest OS type: */
    if (   itemType() == UIVirtualMachineItemType_CloudFake
        && fakeCloudItemState() == UIFakeCloudVirtualMachineItemState_Loading)
        m_pixmap = uiCommon().vmGuestOSTypePixmapDefault("Cloud", &m_logicalPixmapSize);
    else
        m_pixmap = uiCommon().vmGuestOSTypePixmapDefault(m_strOSTypeId, &m_logicalPixmapSize);
}

bool UIVirtualMachineItemCloud::isItemEditable() const
{
    return    accessible()
           && itemType() == UIVirtualMachineItemType_CloudReal;
}

bool UIVirtualMachineItemCloud::isItemRemovable() const
{
    return    accessible()
           && itemType() == UIVirtualMachineItemType_CloudReal;
}

bool UIVirtualMachineItemCloud::isItemSaved() const
{
    return    accessible()
           && machineState() == KCloudMachineState_Stopped;
}

bool UIVirtualMachineItemCloud::isItemPoweredOff() const
{
    return    accessible()
           && (   machineState() == KCloudMachineState_Stopped
               || machineState() == KCloudMachineState_Terminated);
}

bool UIVirtualMachineItemCloud::isItemStarted() const
{
    return    isItemRunning()
           || isItemPaused();
}

bool UIVirtualMachineItemCloud::isItemRunning() const
{
    return    accessible()
           && machineState() == KCloudMachineState_Running;
}

bool UIVirtualMachineItemCloud::isItemRunningHeadless() const
{
    return isItemRunning();
}

bool UIVirtualMachineItemCloud::isItemPaused() const
{
    return false;
}

bool UIVirtualMachineItemCloud::isItemStuck() const
{
    return false;
}

bool UIVirtualMachineItemCloud::isItemCanBeSwitchedTo() const
{
    return false;
}

void UIVirtualMachineItemCloud::retranslateUi()
{
    /* If machine is accessible: */
    if (accessible())
    {
        if (itemType() == UIVirtualMachineItemType_CloudFake)
        {
            /* Update machine/state name: */
            switch (m_enmFakeCloudItemState)
            {
                case UIFakeCloudVirtualMachineItemState_Loading:
                    m_strMachineStateName = tr("Loading ...");
                    break;
                case UIFakeCloudVirtualMachineItemState_Done:
                    m_strMachineStateName = tr("Empty");
                    break;
                default:
                    break;
            }

            /* Update tool-tip: */
            m_strToolTipText = m_strMachineStateName;
        }
        else
        {
            /* Update tool-tip: */
            m_strToolTipText = QString("<nobr><b>%1</b></nobr><br>"
                                       "<nobr>%2</nobr>")
                                       .arg(m_strName)
                                       .arg(gpConverter->toString(m_enmMachineState));
        }
    }
    /* Otherwise: */
    else
    {
        /* Update tool-tip: */
        m_strToolTipText = tr("<nobr><b>%1</b></nobr><br>"
                              "<nobr>Inaccessible</nobr>",
                              "Inaccessible VM tooltip (name)")
                              .arg(m_strName);

        /* We have our own translation for Null states: */
        m_strMachineStateName = tr("Inaccessible");
    }
}

void UIVirtualMachineItemCloud::sltHandleRefreshCloudMachineInfoDone()
{
    /* Recache: */
    recache();

    /* Notify listeners: */
    emit sigRefreshFinished();

    /* Refresh again if scheduled: */
    if (m_fRefreshScheduled)
        updateInfoAsync(true /* async? */);
}

void UIVirtualMachineItemCloud::prepare()
{
    /* Prepare progress-task if necessary: */
    if (itemType() == UIVirtualMachineItemType_CloudReal)
    {
        m_pProgressTaskRefresh = new UIProgressTaskRefreshCloudMachine(this, machine());
        if (m_pProgressTaskRefresh)
        {
            connect(m_pProgressTaskRefresh, &UIProgressTaskRefreshCloudMachine::sigProgressStarted,
                    this, &UIVirtualMachineItemCloud::sigRefreshStarted);
            connect(m_pProgressTaskRefresh, &UIProgressTaskRefreshCloudMachine::sigProgressFinished,
                    this, &UIVirtualMachineItemCloud::sltHandleRefreshCloudMachineInfoDone);
        }
    }

    /* Recache finally: */
    recache();
}

void UIVirtualMachineItemCloud::cleanup()
{
    /* Cleanup progress-task: */
    delete m_pProgressTaskRefresh;
    m_pProgressTaskRefresh = 0;
}


#include "UIVirtualMachineItemCloud.moc"

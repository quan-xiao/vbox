/* $Id: UIVirtualMachineItemCloud.h 86779 2020-11-02 11:35:12Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIVirtualMachineItemCloud class declaration.
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

#ifndef FEQT_INCLUDED_SRC_manager_UIVirtualMachineItemCloud_h
#define FEQT_INCLUDED_SRC_manager_UIVirtualMachineItemCloud_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIVirtualMachineItem.h"

/* COM includes: */
#include "COMEnums.h"
#include "CCloudMachine.h"

/* Forward declarations: */
class UIProgressTask;

/** UIVirtualMachineItem sub-class used as cloud Virtual Machine item interface. */
class UIVirtualMachineItemCloud : public UIVirtualMachineItem
{
    Q_OBJECT;

signals:

    /** Notifies listeners about refresh started. */
    void sigRefreshStarted();
    /** Notifies listeners about refresh finished. */
    void sigRefreshFinished();

public:

    /** Constructs fake cloud VM item of certain @a enmState. */
    UIVirtualMachineItemCloud(UIFakeCloudVirtualMachineItemState enmState);
    /** Constructs real cloud VM item on the basis of taken @a comCloudMachine. */
    UIVirtualMachineItemCloud(const CCloudMachine &comCloudMachine);
    /** Destructs cloud VM item. */
    virtual ~UIVirtualMachineItemCloud() /* override */;

    /** @name Arguments.
      * @{ */
        /** Returns cached cloud machine object. */
        CCloudMachine machine() const { return m_comCloudMachine; }
    /** @} */

    /** @name Data attributes.
      * @{ */
        /** Returns cached machine state. */
        KCloudMachineState machineState() const { return m_enmMachineState; }

        /** Defines fake cloud item @a enmState. */
        void setFakeCloudItemState(UIFakeCloudVirtualMachineItemState enmState);
        /** Returns fake cloud item state. */
        UIFakeCloudVirtualMachineItemState fakeCloudItemState() const { return m_enmFakeCloudItemState; }

        /** Defines fake cloud item @a strErrorMessage. */
        void setFakeCloudItemErrorMessage(const QString &strErrorMessage);
        /** Returns fake cloud item error message. */
        QString fakeCloudItemErrorMessage() const { return m_strFakeCloudItemErrorMessage; }

        /** Updates cloud VM info async way, @a fDelayed if requested or instant otherwise.
          * @param  fSubscribe  Brings whether this update should be performed periodically. */
        void updateInfoAsync(bool fDelayed, bool fSubscribe = false);
        /** Stop periodical updates previously requested. */
        void stopAsyncUpdates();
        /** Makes sure async info update is finished.
          * @note  This method creates own event-loop to avoid blocking calling thread event processing,
          *        so it's safe to call it from the GUI thread, ofc the method itself will be blocked. */
        void waitForAsyncInfoUpdateFinished();
    /** @} */

    /** @name Update stuff.
      * @{ */
        /** Recaches machine data. */
        virtual void recache() /* override */;
        /** Recaches machine item pixmap. */
        virtual void recachePixmap() /* override */;
    /** @} */

    /** @name Validation stuff.
      * @{ */
        /** Returns whether this item is editable. */
        virtual bool isItemEditable() const /* override */;
        /** Returns whether this item is removable. */
        virtual bool isItemRemovable() const /* override */;
        /** Returns whether this item is saved. */
        virtual bool isItemSaved() const /* override */;
        /** Returns whether this item is powered off. */
        virtual bool isItemPoweredOff() const /* override */;
        /** Returns whether this item is started. */
        virtual bool isItemStarted() const /* override */;
        /** Returns whether this item is running. */
        virtual bool isItemRunning() const /* override */;
        /** Returns whether this item is running headless. */
        virtual bool isItemRunningHeadless() const /* override */;
        /** Returns whether this item is paused. */
        virtual bool isItemPaused() const /* override */;
        /** Returns whether this item is stuck. */
        virtual bool isItemStuck() const /* override */;
        /** Returns whether this item can be switched to. */
        virtual bool isItemCanBeSwitchedTo() const /* override */;
    /** @} */

protected:

    /** @name Event handling.
      * @{ */
        /** Handles translation event. */
        virtual void retranslateUi() /* override */;
    /** @} */

private slots:

        /** Handles signal about cloud VM info refresh progress is done. */
        void sltHandleRefreshCloudMachineInfoDone();

private:

    /** @name Prepare/Cleanup cascade.
      * @{ */
        /** Prepares all. */
        void prepare();
        /** Cleanups all. */
        void cleanup();
    /** @} */

    /** @name Arguments.
      * @{ */
        /** Holds cached cloud machine object. */
        CCloudMachine  m_comCloudMachine;
    /** @} */

    /** @name Data attributes.
      * @{ */
        /** Holds cached machine state. */
        KCloudMachineState  m_enmMachineState;

        /** Holds fake cloud item state. */
        UIFakeCloudVirtualMachineItemState  m_enmFakeCloudItemState;
        /** Holds fake cloud item error message. */
        QString                             m_strFakeCloudItemErrorMessage;

        /** Holds whether we plan to refresh info. */
        bool            m_fRefreshScheduled;
        /** Holds the refresh progress-task instance. */
        UIProgressTask *m_pProgressTaskRefresh;
    /** @} */
};

#endif /* !FEQT_INCLUDED_SRC_manager_UIVirtualMachineItemCloud_h */

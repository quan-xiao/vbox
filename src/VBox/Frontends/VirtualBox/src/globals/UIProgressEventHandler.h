/* $Id: UIProgressEventHandler.h 86324 2020-09-28 15:25:28Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIProgressEventHandler class declaration.
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

#ifndef FEQT_INCLUDED_SRC_globals_UIProgressEventHandler_h
#define FEQT_INCLUDED_SRC_globals_UIProgressEventHandler_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
 #include "UIMainEventListener.h"

/* COM includes: */
# include "CEventListener.h"
# include "CEventSource.h"
# include "CProgress.h"


/** Private QObject extension
  * providing UIExtraDataManager with the CVirtualBox event-source. */
class SHARED_LIBRARY_STUFF UIProgressEventHandler : public QObject
{
    Q_OBJECT;

signals:

    /** Notifies about @a iPercent change for progress with @a uProgressId. */
    void sigProgressPercentageChange(const QUuid &uProgressId, const int iPercent);
    /** Notifies about task complete for progress with @a uProgressId. */
    void sigProgressTaskComplete(const QUuid &uProgressId);
    /** Notifies about handling has finished. */
    void sigHandlingFinished();

public:

    /** Constructs event proxy object on the basis of passed @a pParent. */
    UIProgressEventHandler(QObject *pParent, const CProgress &comProgress);
    /** Destructs event proxy object. */
    virtual ~UIProgressEventHandler() /* override */;

protected:

    /** @name Prepare/Cleanup cascade.
      * @{ */
        /** Prepares all. */
        void prepare();
        /** Prepares listener. */
        void prepareListener();
        /** Prepares connections. */
        void prepareConnections();

        /** Cleanups connections. */
        void cleanupConnections();
        /** Cleanups listener. */
        void cleanupListener();
        /** Cleanups all. */
        void cleanup();
    /** @} */

private:

    /** Holds the progress wrapper. */
    CProgress  m_comProgress;

    /** Holds the Qt event listener instance. */
    ComObjPtr<UIMainEventListenerImpl>  m_pQtListener;
    /** Holds the COM event listener instance. */
    CEventListener                      m_comEventListener;
};


#endif /* !FEQT_INCLUDED_SRC_globals_UIProgressEventHandler_h */

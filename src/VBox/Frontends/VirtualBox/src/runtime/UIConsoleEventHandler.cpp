/* $Id: UIConsoleEventHandler.cpp 86541 2020-10-12 12:30:09Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIConsoleEventHandler class implementation.
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

/* GUI includes: */
#include "UIConsoleEventHandler.h"
#include "UIMainEventListener.h"
#include "UIMousePointerShapeData.h"
#include "UIExtraDataManager.h"
#include "UICommon.h"
#include "UISession.h"
#ifdef VBOX_WS_MAC
# include "VBoxUtils.h"
#endif

/* COM includes: */
#include "COMEnums.h"
#include "CEventListener.h"
#include "CEventSource.h"
#include "CConsole.h"


/** Private QObject extension
  * providing UIConsoleEventHandler with the CConsole event-source. */
class UIConsoleEventHandlerProxy : public QObject
{
    Q_OBJECT;

signals:

    /** Notifies about mouse pointer @a shapeData change. */
    void sigMousePointerShapeChange(const UIMousePointerShapeData &shapeData);
    /** Notifies about mouse capability change to @a fSupportsAbsolute, @a fSupportsRelative, @a fSupportsMultiTouch and @a fNeedsHostCursor. */
    void sigMouseCapabilityChange(bool fSupportsAbsolute, bool fSupportsRelative, bool fSupportsMultiTouch, bool fNeedsHostCursor);
    /** Notifies about guest request to change the cursor position to @a uX * @a uY.
      * @param  fContainsData  Brings whether the @a uX and @a uY values are valid and could be used by the GUI now. */
    void sigCursorPositionChange(bool fContainsData, unsigned long uX, unsigned long uY);
    /** Notifies about keyboard LEDs change for @a fNumLock, @a fCapsLock and @a fScrollLock. */
    void sigKeyboardLedsChangeEvent(bool fNumLock, bool fCapsLock, bool fScrollLock);
    /** Notifies about machine @a state change. */
    void sigStateChange(KMachineState state);
    /** Notifies about guest additions state change. */
    void sigAdditionsChange();
    /** Notifies about network @a adapter state change. */
    void sigNetworkAdapterChange(CNetworkAdapter adapter);
    /** Notifies about storage device change for @a attachment, which was @a fRemoved and it was @a fSilent for guest. */
    void sigStorageDeviceChange(CMediumAttachment attachment, bool fRemoved, bool fSilent);
    /** Notifies about storage medium @a attachment state change. */
    void sigMediumChange(CMediumAttachment attachment);
    /** Notifies about VRDE device state change. */
    void sigVRDEChange();
    /** Notifies about recording state change. */
    void sigRecordingChange();
    /** Notifies about USB controller state change. */
    void sigUSBControllerChange();
    /** Notifies about USB @a device state change to @a fAttached, holding additional @a error information. */
    void sigUSBDeviceStateChange(CUSBDevice device, bool fAttached, CVirtualBoxErrorInfo error);
    /** Notifies about shared folder state change. */
    void sigSharedFolderChange();
    /** Notifies about CPU execution-cap change. */
    void sigCPUExecutionCapChange();
    /** Notifies about guest-screen configuration change of @a type for @a uScreenId with @a screenGeo. */
    void sigGuestMonitorChange(KGuestMonitorChangedEventType type, ulong uScreenId, QRect screenGeo);
    /** Notifies about Runtime error with @a strErrorId which is @a fFatal and have @a strMessage. */
    void sigRuntimeError(bool fFatal, QString strErrorId, QString strMessage);
#ifdef RT_OS_DARWIN
    /** Notifies about VM window should be shown. */
    void sigShowWindow();
#endif /* RT_OS_DARWIN */
    /** Notifies about audio adapter state change. */
    void sigAudioAdapterChange();
    /** Notifies clipboard mode change. */
    void sigClipboardModeChange(KClipboardMode enmMode);
    /** Notifies drag and drop mode change. */
    void sigDnDModeChange(KDnDMode enmMode);

public:

    /** Constructs event proxy object on the basis of passed @a pParent and @a pSession. */
    UIConsoleEventHandlerProxy(QObject *pParent, UISession *pSession);
    /** Destructs event proxy object. */
    ~UIConsoleEventHandlerProxy();

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

private slots:

    /** @name Slots for waitable signals.
      * @{ */
        /** Returns whether VM window can be shown. */
        void sltCanShowWindow(bool &fVeto, QString &strReason);
        /** Shows VM window if possible. */
        void sltShowWindow(qint64 &winId);
    /** @} */

private:

    /** Holds the UI session reference. */
    UISession *m_pSession;

    /** Holds the Qt event listener instance. */
    ComObjPtr<UIMainEventListenerImpl> m_pQtListener;
    /** Holds the COM event listener instance. */
    CEventListener m_comEventListener;
};


/*********************************************************************************************************************************
*   Class UIConsoleEventHandlerProxy implementation.                                                                             *
*********************************************************************************************************************************/

UIConsoleEventHandlerProxy::UIConsoleEventHandlerProxy(QObject *pParent, UISession *pSession)
    : QObject(pParent)
    , m_pSession(pSession)
{
    /* Prepare: */
    prepare();
}

UIConsoleEventHandlerProxy::~UIConsoleEventHandlerProxy()
{
    /* Cleanup: */
    cleanup();
}

void UIConsoleEventHandlerProxy::prepare()
{
    /* Prepare: */
    prepareListener();
    prepareConnections();
}

void UIConsoleEventHandlerProxy::prepareListener()
{
    /* Make sure session is passed: */
    AssertPtrReturnVoid(m_pSession);

    /* Create event listener instance: */
    m_pQtListener.createObject();
    m_pQtListener->init(new UIMainEventListener, this);
    m_comEventListener = CEventListener(m_pQtListener);

    /* Get console: */
    const CConsole comConsole = m_pSession->session().GetConsole();
    AssertReturnVoid(!comConsole.isNull() && comConsole.isOk());
    /* Get console event source: */
    CEventSource comEventSourceConsole = comConsole.GetEventSource();
    AssertReturnVoid(!comEventSourceConsole.isNull() && comEventSourceConsole.isOk());

    /* Enumerate all the required event-types: */
    QVector<KVBoxEventType> eventTypes;
    eventTypes
        << KVBoxEventType_OnMousePointerShapeChanged
        << KVBoxEventType_OnMouseCapabilityChanged
        << KVBoxEventType_OnCursorPositionChanged
        << KVBoxEventType_OnKeyboardLedsChanged
        << KVBoxEventType_OnStateChanged
        << KVBoxEventType_OnAdditionsStateChanged
        << KVBoxEventType_OnNetworkAdapterChanged
        << KVBoxEventType_OnStorageDeviceChanged
        << KVBoxEventType_OnMediumChanged
        << KVBoxEventType_OnVRDEServerChanged
        << KVBoxEventType_OnVRDEServerInfoChanged
        << KVBoxEventType_OnRecordingChanged
        << KVBoxEventType_OnUSBControllerChanged
        << KVBoxEventType_OnUSBDeviceStateChanged
        << KVBoxEventType_OnSharedFolderChanged
        << KVBoxEventType_OnCPUExecutionCapChanged
        << KVBoxEventType_OnGuestMonitorChanged
        << KVBoxEventType_OnRuntimeError
        << KVBoxEventType_OnCanShowWindow
        << KVBoxEventType_OnShowWindow
        << KVBoxEventType_OnAudioAdapterChanged
        << KVBoxEventType_OnClipboardModeChanged
        << KVBoxEventType_OnDnDModeChanged;

    /* Register event listener for console event source: */
    comEventSourceConsole.RegisterListener(m_comEventListener, eventTypes, FALSE /* active? */);
    AssertWrapperOk(comEventSourceConsole);

    /* Register event sources in their listeners as well: */
    m_pQtListener->getWrapped()->registerSource(comEventSourceConsole, m_comEventListener);
}

void UIConsoleEventHandlerProxy::prepareConnections()
{
    /* Create direct (sync) connections for signals of main listener: */
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigMousePointerShapeChange,
        this, &UIConsoleEventHandlerProxy::sigMousePointerShapeChange,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigMouseCapabilityChange,
           this, &UIConsoleEventHandlerProxy::sigMouseCapabilityChange,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigCursorPositionChange,
           this, &UIConsoleEventHandlerProxy::sigCursorPositionChange,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigKeyboardLedsChangeEvent,
            this, &UIConsoleEventHandlerProxy::sigKeyboardLedsChangeEvent,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigStateChange,
        this, &UIConsoleEventHandlerProxy::sigStateChange,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigAdditionsChange,
            this, &UIConsoleEventHandlerProxy::sigAdditionsChange,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigNetworkAdapterChange,
            this, &UIConsoleEventHandlerProxy::sigNetworkAdapterChange,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigStorageDeviceChange,
            this, &UIConsoleEventHandlerProxy::sigStorageDeviceChange,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigMediumChange,
            this, &UIConsoleEventHandlerProxy::sigMediumChange,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigVRDEChange,
            this, &UIConsoleEventHandlerProxy::sigVRDEChange,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigRecordingChange,
            this, &UIConsoleEventHandlerProxy::sigRecordingChange,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigUSBControllerChange,
            this, &UIConsoleEventHandlerProxy::sigUSBControllerChange,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigUSBDeviceStateChange,
            this, &UIConsoleEventHandlerProxy::sigUSBDeviceStateChange,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigSharedFolderChange,
            this, &UIConsoleEventHandlerProxy::sigSharedFolderChange,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigCPUExecutionCapChange,
            this, &UIConsoleEventHandlerProxy::sigCPUExecutionCapChange,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigGuestMonitorChange,
            this, &UIConsoleEventHandlerProxy::sigGuestMonitorChange,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigRuntimeError,
            this, &UIConsoleEventHandlerProxy::sigRuntimeError,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigCanShowWindow,
            this, &UIConsoleEventHandlerProxy::sltCanShowWindow,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigShowWindow,
            this, &UIConsoleEventHandlerProxy::sltShowWindow,
            Qt::DirectConnection);
    connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigAudioAdapterChange,
            this, &UIConsoleEventHandlerProxy::sigAudioAdapterChange,
            Qt::DirectConnection);
   connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigClipboardModeChange,
            this, &UIConsoleEventHandlerProxy::sigClipboardModeChange,
            Qt::DirectConnection);
   connect(m_pQtListener->getWrapped(), &UIMainEventListener::sigDnDModeChange,
            this, &UIConsoleEventHandlerProxy::sigDnDModeChange,
            Qt::DirectConnection);
}

void UIConsoleEventHandlerProxy::cleanupConnections()
{
    /* Nothing for now. */
}

void UIConsoleEventHandlerProxy::cleanupListener()
{
    /* Make sure session is passed: */
    AssertPtrReturnVoid(m_pSession);

    /* Unregister everything: */
    m_pQtListener->getWrapped()->unregisterSources();

    /* Get console: */
    const CConsole comConsole = m_pSession->session().GetConsole();
    if (comConsole.isNull() || !comConsole.isOk())
        return;
    /* Get console event source: */
    CEventSource comEventSourceConsole = comConsole.GetEventSource();
    AssertWrapperOk(comEventSourceConsole);

    /* Unregister event listener for console event source: */
    comEventSourceConsole.UnregisterListener(m_comEventListener);
}

void UIConsoleEventHandlerProxy::cleanup()
{
    /* Cleanup: */
    cleanupConnections();
    cleanupListener();
}

void UIConsoleEventHandlerProxy::sltCanShowWindow(bool & /* fVeto */, QString & /* strReason */)
{
    /* Nothing for now. */
}

void UIConsoleEventHandlerProxy::sltShowWindow(qint64 &winId)
{
#ifdef VBOX_WS_MAC
    /* First of all, just ask the GUI thread to show the machine-window: */
    winId = 0;
    if (::darwinSetFrontMostProcess())
        emit sigShowWindow();
    else
    {
        /* If it's failed for some reason, send the other process our PSN so it can try: */
        winId = ::darwinGetCurrentProcessId();
    }
#else /* !VBOX_WS_MAC */
    /* Return the ID of the top-level machine-window. */
    winId = (ULONG64)m_pSession->mainMachineWindowId();
#endif /* !VBOX_WS_MAC */
}


/*********************************************************************************************************************************
*   Class UIConsoleEventHandler implementation.                                                                                  *
*********************************************************************************************************************************/

/* static */
UIConsoleEventHandler *UIConsoleEventHandler::m_spInstance = 0;

/* static */
void UIConsoleEventHandler::create(UISession *pSession)
{
    if (!m_spInstance)
        m_spInstance = new UIConsoleEventHandler(pSession);
}

/* static */
void UIConsoleEventHandler::destroy()
{
    if (m_spInstance)
    {
        delete m_spInstance;
        m_spInstance = 0;
    }
}

UIConsoleEventHandler::UIConsoleEventHandler(UISession *pSession)
    : m_pProxy(new UIConsoleEventHandlerProxy(this, pSession))
{
    /* Prepare: */
    prepare();
}

void UIConsoleEventHandler::prepare()
{
    /* Prepare: */
    prepareConnections();
}

void UIConsoleEventHandler::prepareConnections()
{
    /* Create queued (async) connections for signals of event proxy object: */
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigMousePointerShapeChange,
            this, &UIConsoleEventHandler::sigMousePointerShapeChange,
            Qt::QueuedConnection);
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigMouseCapabilityChange,
            this, &UIConsoleEventHandler::sigMouseCapabilityChange,
            Qt::QueuedConnection);
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigCursorPositionChange,
            this, &UIConsoleEventHandler::sigCursorPositionChange,
            Qt::QueuedConnection);
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigKeyboardLedsChangeEvent,
            this, &UIConsoleEventHandler::sigKeyboardLedsChangeEvent,
            Qt::QueuedConnection);
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigStateChange,
        this, &UIConsoleEventHandler::sigStateChange,
            Qt::QueuedConnection);
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigAdditionsChange,
            this, &UIConsoleEventHandler::sigAdditionsChange,
            Qt::QueuedConnection);
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigNetworkAdapterChange,
            this, &UIConsoleEventHandler::sigNetworkAdapterChange,
            Qt::QueuedConnection);
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigStorageDeviceChange,
        this, &UIConsoleEventHandler::sigStorageDeviceChange,
            Qt::QueuedConnection);
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigMediumChange,
            this, &UIConsoleEventHandler::sigMediumChange,
            Qt::QueuedConnection);
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigVRDEChange,
            this, &UIConsoleEventHandler::sigVRDEChange,
            Qt::QueuedConnection);
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigRecordingChange,
            this, &UIConsoleEventHandler::sigRecordingChange,
            Qt::QueuedConnection);
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigUSBControllerChange,
            this, &UIConsoleEventHandler::sigUSBControllerChange,
            Qt::QueuedConnection);
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigUSBDeviceStateChange,
            this, &UIConsoleEventHandler::sigUSBDeviceStateChange,
            Qt::QueuedConnection);
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigSharedFolderChange,
            this, &UIConsoleEventHandler::sigSharedFolderChange,
            Qt::QueuedConnection);
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigCPUExecutionCapChange,
            this, &UIConsoleEventHandler::sigCPUExecutionCapChange,
            Qt::QueuedConnection);
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigGuestMonitorChange,
            this, &UIConsoleEventHandler::sigGuestMonitorChange,
            Qt::QueuedConnection);
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigRuntimeError,
            this, &UIConsoleEventHandler::sigRuntimeError,
            Qt::QueuedConnection);
#ifdef RT_OS_DARWIN
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigShowWindow,
            this, &UIConsoleEventHandler::sigShowWindow,
            Qt::QueuedConnection);
#endif /* RT_OS_DARWIN */
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigAudioAdapterChange,
            this, &UIConsoleEventHandler::sigAudioAdapterChange,
            Qt::QueuedConnection);
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigClipboardModeChange,
            this, &UIConsoleEventHandler::sigClipboardModeChange,
            Qt::QueuedConnection);
    connect(m_pProxy, &UIConsoleEventHandlerProxy::sigDnDModeChange,
            this, &UIConsoleEventHandler::sigDnDModeChange,
            Qt::QueuedConnection);
}

#include "UIConsoleEventHandler.moc"

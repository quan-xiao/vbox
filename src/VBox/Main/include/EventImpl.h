/* $Id: EventImpl.h 85293 2020-07-13 08:55:08Z vboxsync $ */
/** @file
 * VirtualBox COM IEvent implementation
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

#ifndef MAIN_INCLUDED_EventImpl_h
#define MAIN_INCLUDED_EventImpl_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "EventWrap.h"
#include "EventSourceWrap.h"
#include "VetoEventWrap.h"


class ATL_NO_VTABLE VBoxEvent
    : public EventWrap
{
public:
    DECLARE_EMPTY_CTOR_DTOR(VBoxEvent)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(IEventSource *aSource, VBoxEventType_T aType, BOOL aWaitable);
    void uninit();

private:
    // wrapped IEvent properties
    HRESULT getType(VBoxEventType_T *aType);
    HRESULT getSource(ComPtr<IEventSource> &aSource);
    HRESULT getWaitable(BOOL *aWaitable);

    // wrapped IEvent methods
    HRESULT setProcessed();
    HRESULT waitProcessed(LONG aTimeout, BOOL *aResult);

    struct Data;
    Data* m;
};


class ATL_NO_VTABLE VBoxVetoEvent
    : public VetoEventWrap
{
public:
    DECLARE_EMPTY_CTOR_DTOR(VBoxVetoEvent)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(IEventSource *aSource, VBoxEventType_T aType);
    void uninit();

private:
    // wrapped IEvent properties
    HRESULT getType(VBoxEventType_T *aType);
    HRESULT getSource(ComPtr<IEventSource> &aSource);
    HRESULT getWaitable(BOOL *aWaitable);

    // wrapped IEvent methods
    HRESULT setProcessed();
    HRESULT waitProcessed(LONG aTimeout, BOOL *aResult);

    // wrapped IVetoEvent methods
    HRESULT addVeto(const com::Utf8Str &aReason);
    HRESULT isVetoed(BOOL *aResult);
    HRESULT getVetos(std::vector<com::Utf8Str> &aResult);
    HRESULT addApproval(const com::Utf8Str &aReason);
    HRESULT isApproved(BOOL *aResult);
    HRESULT getApprovals(std::vector<com::Utf8Str> &aResult);

    struct Data;
    Data* m;
};

class ATL_NO_VTABLE EventSource :
    public EventSourceWrap
{
public:
    DECLARE_EMPTY_CTOR_DTOR(EventSource)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init();
    void uninit();

private:
    // wrapped IEventSource methods
    HRESULT createListener(ComPtr<IEventListener> &aListener);
    HRESULT createAggregator(const std::vector<ComPtr<IEventSource> > &aSubordinates,
                             ComPtr<IEventSource> &aResult);
    HRESULT registerListener(const ComPtr<IEventListener> &aListener,
                             const std::vector<VBoxEventType_T> &aInteresting,
                             BOOL aActive);
    HRESULT unregisterListener(const ComPtr<IEventListener> &aListener);
    HRESULT fireEvent(const ComPtr<IEvent> &aEvent,
                      LONG aTimeout,
                      BOOL *aResult);
    HRESULT getEvent(const ComPtr<IEventListener> &aListener,
                     LONG aTimeout,
                     ComPtr<IEvent> &aEvent);
    HRESULT eventProcessed(const ComPtr<IEventListener> &aListener,
                           const ComPtr<IEvent> &aEvent);


    struct Data;
    Data *m;

    friend class ListenerRecord;
};

class VBoxEventDesc
{
public:
    VBoxEventDesc() : mEvent(0), mEventSource(0)
    {}

    VBoxEventDesc(IEvent *aEvent, IEventSource *aSource)
        : mEvent(aEvent), mEventSource(aSource)
    {}

    ~VBoxEventDesc()
    {}

    void init(IEvent *aEvent, IEventSource *aSource)
    {
        mEvent       = aEvent;
        mEventSource = aSource;
    }

    void uninit()
    {
        mEvent.setNull();
        mEventSource.setNull();
    }

    void getEvent(IEvent **aEvent)
    {
        mEvent.queryInterfaceTo(aEvent);
    }

    BOOL fire(LONG aTimeout)
    {
        if (mEventSource && mEvent)
        {
            BOOL fDelivered = FALSE;
            HRESULT hrc = mEventSource->FireEvent(mEvent, aTimeout, &fDelivered);
            AssertComRCReturn(hrc, FALSE);
            return fDelivered;
        }
        return FALSE;
    }

private:
    ComPtr<IEvent>          mEvent;
    ComPtr<IEventSource>    mEventSource;
};


#endif /* !MAIN_INCLUDED_EventImpl_h */

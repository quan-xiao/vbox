/* $Id: GuestProcessImpl.cpp 85300 2020-07-13 10:04:45Z vboxsync $ */
/** @file
 * VirtualBox Main - Guest process handling.
 */

/*
 * Copyright (C) 2012-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/**
 * Locking rules:
 * - When the main dispatcher (callbackDispatcher) is called it takes the
 *   WriteLock while dispatching to the various on* methods.
 * - All other outer functions (accessible by Main) must not own a lock
 *   while waiting for a callback or for an event.
 * - Only keep Read/WriteLocks as short as possible and only when necessary.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_MAIN_GUESTPROCESS
#include "LoggingNew.h"

#ifndef VBOX_WITH_GUEST_CONTROL
# error "VBOX_WITH_GUEST_CONTROL must defined in this file"
#endif
#include "GuestImpl.h"
#include "GuestProcessImpl.h"
#include "GuestSessionImpl.h"
#include "GuestCtrlImplPrivate.h"
#include "ConsoleImpl.h"
#include "VirtualBoxErrorInfoImpl.h"

#include "Global.h"
#include "AutoCaller.h"
#include "VBoxEvents.h"
#include "ThreadTask.h"

#include <memory> /* For auto_ptr. */

#include <iprt/asm.h>
#include <iprt/cpp/utils.h> /* For unconst(). */
#include <iprt/getopt.h>

#include <VBox/com/listeners.h>

#include <VBox/com/array.h>


class GuestProcessTask : public ThreadTask
{
public:

    GuestProcessTask(GuestProcess *pProcess)
        : ThreadTask("GenericGuestProcessTask")
        , mProcess(pProcess)
        , mRC(VINF_SUCCESS) { }

    virtual ~GuestProcessTask(void) { }

    int i_rc(void) const { return mRC; }
    bool i_isOk(void) const { return RT_SUCCESS(mRC); }
    const ComObjPtr<GuestProcess> &i_process(void) const { return mProcess; }

protected:

    const ComObjPtr<GuestProcess>    mProcess;
    int                              mRC;
};

class GuestProcessStartTask : public GuestProcessTask
{
public:

    GuestProcessStartTask(GuestProcess *pProcess)
        : GuestProcessTask(pProcess)
    {
        m_strTaskName = "gctlPrcStart";
    }

    void handler()
    {
        GuestProcess::i_startProcessThreadTask(this);
    }
};

/**
 * Internal listener class to serve events in an
 * active manner, e.g. without polling delays.
 */
class GuestProcessListener
{
public:

    GuestProcessListener(void)
    {
    }

    virtual ~GuestProcessListener(void)
    {
    }

    HRESULT init(GuestProcess *pProcess)
    {
        AssertPtrReturn(pProcess, E_POINTER);
        mProcess = pProcess;
        return S_OK;
    }

    void uninit(void)
    {
        mProcess = NULL;
    }

    STDMETHOD(HandleEvent)(VBoxEventType_T aType, IEvent *aEvent)
    {
        switch (aType)
        {
            case VBoxEventType_OnGuestProcessStateChanged:
            case VBoxEventType_OnGuestProcessInputNotify:
            case VBoxEventType_OnGuestProcessOutput:
            {
                AssertPtrReturn(mProcess, E_POINTER);
                int rc2 = mProcess->signalWaitEvent(aType, aEvent);
                RT_NOREF(rc2);
#ifdef LOG_ENABLED
                LogFlowThisFunc(("Signalling events of type=%RU32, pProcess=%p resulted in rc=%Rrc\n",
                                 aType, &mProcess, rc2));
#endif
                break;
            }

            default:
                AssertMsgFailed(("Unhandled event %RU32\n", aType));
                break;
        }

        return S_OK;
    }

private:

    GuestProcess *mProcess;
};
typedef ListenerImpl<GuestProcessListener, GuestProcess*> GuestProcessListenerImpl;

VBOX_LISTENER_DECLARE(GuestProcessListenerImpl)

// constructor / destructor
/////////////////////////////////////////////////////////////////////////////

DEFINE_EMPTY_CTOR_DTOR(GuestProcess)

HRESULT GuestProcess::FinalConstruct(void)
{
    LogFlowThisFuncEnter();
    return BaseFinalConstruct();
}

void GuestProcess::FinalRelease(void)
{
    LogFlowThisFuncEnter();
    uninit();
    BaseFinalRelease();
    LogFlowThisFuncLeave();
}

// public initializer/uninitializer for internal purposes only
/////////////////////////////////////////////////////////////////////////////

int GuestProcess::init(Console *aConsole, GuestSession *aSession, ULONG aObjectID,
                       const GuestProcessStartupInfo &aProcInfo, const GuestEnvironment *pBaseEnv)
{
    LogFlowThisFunc(("aConsole=%p, aSession=%p, aObjectID=%RU32, pBaseEnv=%p\n",
                     aConsole, aSession, aObjectID, pBaseEnv));

    AssertPtrReturn(aConsole, VERR_INVALID_POINTER);
    AssertPtrReturn(aSession, VERR_INVALID_POINTER);

    /* Enclose the state transition NotReady->InInit->Ready. */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), VERR_OBJECT_DESTROYED);

    HRESULT hr;

    int vrc = bindToSession(aConsole, aSession, aObjectID);
    if (RT_SUCCESS(vrc))
    {
        hr = unconst(mEventSource).createObject();
        if (FAILED(hr))
            vrc = VERR_NO_MEMORY;
        else
        {
            hr = mEventSource->init();
            if (FAILED(hr))
                vrc = VERR_COM_UNEXPECTED;
        }
    }

    if (RT_SUCCESS(vrc))
    {
        try
        {
            GuestProcessListener *pListener = new GuestProcessListener();
            ComObjPtr<GuestProcessListenerImpl> thisListener;
            hr = thisListener.createObject();
            if (SUCCEEDED(hr))
                hr = thisListener->init(pListener, this);

            if (SUCCEEDED(hr))
            {
                com::SafeArray <VBoxEventType_T> eventTypes;
                eventTypes.push_back(VBoxEventType_OnGuestProcessStateChanged);
                eventTypes.push_back(VBoxEventType_OnGuestProcessInputNotify);
                eventTypes.push_back(VBoxEventType_OnGuestProcessOutput);
                hr = mEventSource->RegisterListener(thisListener,
                                                    ComSafeArrayAsInParam(eventTypes),
                                                    TRUE /* Active listener */);
                if (SUCCEEDED(hr))
                {
                    vrc = baseInit();
                    if (RT_SUCCESS(vrc))
                    {
                        mLocalListener = thisListener;
                    }
                }
                else
                    vrc = VERR_COM_UNEXPECTED;
            }
            else
                vrc = VERR_COM_UNEXPECTED;
        }
        catch(std::bad_alloc &)
        {
            vrc = VERR_NO_MEMORY;
        }
    }

    if (RT_SUCCESS(vrc))
    {
        mData.mProcess = aProcInfo;
        mData.mpSessionBaseEnv = pBaseEnv;
        if (pBaseEnv)
            pBaseEnv->retainConst();
        mData.mExitCode = 0;
        mData.mPID = 0;
        mData.mLastError = VINF_SUCCESS;
        mData.mStatus = ProcessStatus_Undefined;
        /* Everything else will be set by the actual starting routine. */

        /* Confirm a successful initialization when it's the case. */
        autoInitSpan.setSucceeded();

        return vrc;
    }

    autoInitSpan.setFailed();
    return vrc;
}

/**
 * Uninitializes the instance.
 * Called from FinalRelease() or IGuestSession::uninit().
 */
void GuestProcess::uninit(void)
{
    /* Enclose the state transition Ready->InUninit->NotReady. */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;

    LogFlowThisFunc(("mExe=%s, PID=%RU32\n", mData.mProcess.mExecutable.c_str(), mData.mPID));

    if (mData.mpSessionBaseEnv)
    {
        mData.mpSessionBaseEnv->releaseConst();
        mData.mpSessionBaseEnv = NULL;
    }

    baseUninit();

    LogFlowFuncLeave();
}

// implementation of public getters/setters for attributes
/////////////////////////////////////////////////////////////////////////////
HRESULT GuestProcess::getArguments(std::vector<com::Utf8Str> &aArguments)
{
    LogFlowThisFuncEnter();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    aArguments = mData.mProcess.mArguments;
    return S_OK;
}

HRESULT GuestProcess::getEnvironment(std::vector<com::Utf8Str> &aEnvironment)
{
#ifndef VBOX_WITH_GUEST_CONTROL
    ReturnComNotImplemented();
#else
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);  /* (Paranoia since both environment objects are immutable.) */
    HRESULT hrc;
    if (mData.mpSessionBaseEnv)
    {
        int vrc;
        if (mData.mProcess.mEnvironmentChanges.count() == 0)
            vrc = mData.mpSessionBaseEnv->queryPutEnvArray(&aEnvironment);
        else
        {
            GuestEnvironment TmpEnv;
            vrc = TmpEnv.copy(*mData.mpSessionBaseEnv);
            if (RT_SUCCESS(vrc))
            {
                vrc = TmpEnv.applyChanges(mData.mProcess.mEnvironmentChanges);
                if (RT_SUCCESS(vrc))
                    vrc = TmpEnv.queryPutEnvArray(&aEnvironment);
            }
        }
        hrc = Global::vboxStatusCodeToCOM(vrc);
    }
    else
        hrc = setError(VBOX_E_NOT_SUPPORTED, tr("The base environment feature is not supported by installed Guest Additions"));
    LogFlowThisFuncLeave();
    return hrc;
#endif
}

HRESULT GuestProcess::getEventSource(ComPtr<IEventSource> &aEventSource)
{
    LogFlowThisFuncEnter();

    // no need to lock - lifetime constant
    mEventSource.queryInterfaceTo(aEventSource.asOutParam());

    LogFlowThisFuncLeave();
    return S_OK;
}

HRESULT GuestProcess::getExecutablePath(com::Utf8Str &aExecutablePath)
{
    LogFlowThisFuncEnter();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aExecutablePath = mData.mProcess.mExecutable;

    return S_OK;
}

HRESULT GuestProcess::getExitCode(LONG *aExitCode)
{
    LogFlowThisFuncEnter();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aExitCode = mData.mExitCode;

    return S_OK;
}

HRESULT GuestProcess::getName(com::Utf8Str &aName)
{
    LogFlowThisFuncEnter();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aName = mData.mProcess.mName;

    return S_OK;
}

HRESULT GuestProcess::getPID(ULONG *aPID)
{
    LogFlowThisFuncEnter();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aPID = mData.mPID;

    return S_OK;
}

HRESULT GuestProcess::getStatus(ProcessStatus_T *aStatus)
{
    LogFlowThisFuncEnter();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aStatus = mData.mStatus;

    return S_OK;
}

// private methods
/////////////////////////////////////////////////////////////////////////////

int GuestProcess::i_callbackDispatcher(PVBOXGUESTCTRLHOSTCBCTX pCbCtx, PVBOXGUESTCTRLHOSTCALLBACK pSvcCb)
{
    AssertPtrReturn(pCbCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pSvcCb, VERR_INVALID_POINTER);
#ifdef DEBUG
    LogFlowThisFunc(("uPID=%RU32, uContextID=%RU32, uMessage=%RU32, pSvcCb=%p\n",
                     mData.mPID, pCbCtx->uContextID, pCbCtx->uMessage, pSvcCb));
#endif

    int vrc;
    switch (pCbCtx->uMessage)
    {
        case GUEST_MSG_DISCONNECTED:
        {
            vrc = i_onGuestDisconnected(pCbCtx, pSvcCb);
            break;
        }

        case GUEST_MSG_EXEC_STATUS:
        {
            vrc = i_onProcessStatusChange(pCbCtx, pSvcCb);
            break;
        }

        case GUEST_MSG_EXEC_OUTPUT:
        {
            vrc = i_onProcessOutput(pCbCtx, pSvcCb);
            break;
        }

        case GUEST_MSG_EXEC_INPUT_STATUS:
        {
            vrc = i_onProcessInputStatus(pCbCtx, pSvcCb);
            break;
        }

        default:
            /* Silently ignore not implemented functions. */
            vrc = VERR_NOT_SUPPORTED;
            break;
    }

#ifdef DEBUG
    LogFlowFuncLeaveRC(vrc);
#endif
    return vrc;
}

/**
 * Checks if the current assigned PID matches another PID (from a callback).
 *
 * In protocol v1 we don't have the possibility to terminate/kill
 * processes so it can happen that a formerly started process A
 * (which has the context ID 0 (session=0, process=0, count=0) will
 * send a delayed message to the host if this process has already
 * been discarded there and the same context ID was reused by
 * a process B. Process B in turn then has a different guest PID.
 *
 * Note: This also can happen when restoring from a saved state which
 *       had a guest process running.
 *
 * @return  IPRT status code.
 * @param   uPID                    PID to check.
 */
inline int GuestProcess::i_checkPID(uint32_t uPID)
{
    int rc = VINF_SUCCESS;

    /* Was there a PID assigned yet? */
    if (mData.mPID)
    {
        if (RT_UNLIKELY(mData.mPID != uPID))
        {
            LogFlowFunc(("Stale guest process (PID=%RU32) sent data to a newly started process (pProcesS=%p, PID=%RU32, status=%RU32)\n",
                         uPID, this, mData.mPID, mData.mStatus));
            rc = VERR_NOT_FOUND;
        }
    }

    return rc;
}

/**
 * Converts a given guest process error to a string.
 *
 * @returns Error as a string.
 * @param   rcGuest             Guest process error to return string for.
 * @param   pcszWhat            Hint of what was involved when the error occurred.
 */
/* static */
Utf8Str GuestProcess::i_guestErrorToString(int rcGuest, const char *pcszWhat)
{
    AssertPtrReturn(pcszWhat, "");

    Utf8Str strErr;

#define CASE_MSG(a_iRc, ...) \
    case a_iRc: strErr = Utf8StrFmt( __VA_ARGS__); break;

    /** @todo pData->u32Flags: int vs. uint32 -- IPRT errors are *negative* !!! */
    switch (rcGuest)
    {
        CASE_MSG(VERR_FILE_NOT_FOUND,                 tr("No such file or directory \"%s\" on guest"), pcszWhat); /* This is the most likely error. */
        CASE_MSG(VERR_PATH_NOT_FOUND,                 tr("No such file or directory \"%s\" on guest"), pcszWhat);
        CASE_MSG(VERR_INVALID_VM_HANDLE,              tr("VMM device is not available (is the VM running?)"));
        CASE_MSG(VERR_HGCM_SERVICE_NOT_FOUND,         tr("The guest execution service is not available"));
        CASE_MSG(VERR_BAD_EXE_FORMAT,                 tr("The file \"%s\" is not an executable format on guest"), pcszWhat);
        CASE_MSG(VERR_AUTHENTICATION_FAILURE,         tr("The user \"%s\" was not able to logon on guest"), pcszWhat);
        CASE_MSG(VERR_INVALID_NAME,                   tr("The file \"%s\" is an invalid name"), pcszWhat);
        CASE_MSG(VERR_TIMEOUT,                        tr("The guest did not respond within time"));
        CASE_MSG(VERR_CANCELLED,                      tr("The execution operation for \"%s\" was canceled"), pcszWhat);
        CASE_MSG(VERR_GSTCTL_MAX_CID_OBJECTS_REACHED, tr("Maximum number of concurrent guest processes has been reached"));
        CASE_MSG(VERR_NOT_FOUND,                      tr("The guest execution service is not ready (yet)"));
        default:
        {
            char szDefine[80];
            RTErrQueryDefine(rcGuest, szDefine, sizeof(szDefine), false /*fFailIfUnknown*/);
            strErr = Utf8StrFmt(tr("Error %s for guest process \"%s\" occurred\n"), szDefine, pcszWhat);
            break;
        }
    }

#undef CASE_MSG

    return strErr;
}

/**
 * Returns @c true if the passed in error code indicates an error which came
 * from the guest side, or @c false if not.
 *
 * @return  bool                @c true if the passed in error code indicates an error which came
 *                              from the guest side, or @c false if not.
 * @param   rc                  Error code to check.
 */
/* static */
bool GuestProcess::i_isGuestError(int rc)
{
    return (   rc == VERR_GSTCTL_GUEST_ERROR
            || rc == VERR_GSTCTL_PROCESS_EXIT_CODE);
}

inline bool GuestProcess::i_isAlive(void)
{
    return (   mData.mStatus == ProcessStatus_Started
            || mData.mStatus == ProcessStatus_Paused
            || mData.mStatus == ProcessStatus_Terminating);
}

inline bool GuestProcess::i_hasEnded(void)
{
    return (   mData.mStatus == ProcessStatus_TerminatedNormally
            || mData.mStatus == ProcessStatus_TerminatedSignal
            || mData.mStatus == ProcessStatus_TerminatedAbnormally
            || mData.mStatus == ProcessStatus_TimedOutKilled
            || mData.mStatus == ProcessStatus_TimedOutAbnormally
            || mData.mStatus == ProcessStatus_Down
            || mData.mStatus == ProcessStatus_Error);
}

int GuestProcess::i_onGuestDisconnected(PVBOXGUESTCTRLHOSTCBCTX pCbCtx, PVBOXGUESTCTRLHOSTCALLBACK pSvcCbData)
{
    AssertPtrReturn(pCbCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pSvcCbData, VERR_INVALID_POINTER);

    int vrc = i_setProcessStatus(ProcessStatus_Down, VINF_SUCCESS);

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

int GuestProcess::i_onProcessInputStatus(PVBOXGUESTCTRLHOSTCBCTX pCbCtx, PVBOXGUESTCTRLHOSTCALLBACK pSvcCbData)
{
    AssertPtrReturn(pCbCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pSvcCbData, VERR_INVALID_POINTER);
    /* pCallback is optional. */

    if (pSvcCbData->mParms < 5)
        return VERR_INVALID_PARAMETER;

    CALLBACKDATA_PROC_INPUT dataCb;
    /* pSvcCb->mpaParms[0] always contains the context ID. */
    int vrc = HGCMSvcGetU32(&pSvcCbData->mpaParms[1], &dataCb.uPID);
    AssertRCReturn(vrc, vrc);
    vrc = HGCMSvcGetU32(&pSvcCbData->mpaParms[2], &dataCb.uStatus);
    AssertRCReturn(vrc, vrc);
    vrc = HGCMSvcGetU32(&pSvcCbData->mpaParms[3], &dataCb.uFlags);
    AssertRCReturn(vrc, vrc);
    vrc = HGCMSvcGetU32(&pSvcCbData->mpaParms[4], &dataCb.uProcessed);
    AssertRCReturn(vrc, vrc);

    LogFlowThisFunc(("uPID=%RU32, uStatus=%RU32, uFlags=%RI32, cbProcessed=%RU32\n",
                     dataCb.uPID, dataCb.uStatus, dataCb.uFlags, dataCb.uProcessed));

    vrc = i_checkPID(dataCb.uPID);
    if (RT_SUCCESS(vrc))
    {
        ProcessInputStatus_T inputStatus = ProcessInputStatus_Undefined;
        switch (dataCb.uStatus)
        {
            case INPUT_STS_WRITTEN:
                inputStatus = ProcessInputStatus_Written;
                break;
            case INPUT_STS_ERROR:
                inputStatus = ProcessInputStatus_Broken;
                break;
            case INPUT_STS_TERMINATED:
                inputStatus = ProcessInputStatus_Broken;
                break;
            case INPUT_STS_OVERFLOW:
                inputStatus = ProcessInputStatus_Overflow;
                break;
            case INPUT_STS_UNDEFINED:
                /* Fall through is intentional. */
            default:
                AssertMsg(!dataCb.uProcessed, ("Processed data is not 0 in undefined input state\n"));
                break;
        }

        if (inputStatus != ProcessInputStatus_Undefined)
        {
            AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

            /* Copy over necessary data before releasing lock again. */
            uint32_t uPID = mData.mPID;
            /** @todo Also handle mSession? */

            alock.release(); /* Release lock before firing off event. */

            ::FireGuestProcessInputNotifyEvent(mEventSource, mSession, this, uPID, 0 /* StdIn */, dataCb.uProcessed, inputStatus);
        }
    }

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

int GuestProcess::i_onProcessNotifyIO(PVBOXGUESTCTRLHOSTCBCTX pCbCtx, PVBOXGUESTCTRLHOSTCALLBACK pSvcCbData)
{
    AssertPtrReturn(pCbCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pSvcCbData, VERR_INVALID_POINTER);

    return VERR_NOT_IMPLEMENTED;
}

int GuestProcess::i_onProcessStatusChange(PVBOXGUESTCTRLHOSTCBCTX pCbCtx, PVBOXGUESTCTRLHOSTCALLBACK pSvcCbData)
{
    AssertPtrReturn(pCbCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pSvcCbData, VERR_INVALID_POINTER);

    if (pSvcCbData->mParms < 5)
        return VERR_INVALID_PARAMETER;

    CALLBACKDATA_PROC_STATUS dataCb;
    /* pSvcCb->mpaParms[0] always contains the context ID. */
    int vrc = HGCMSvcGetU32(&pSvcCbData->mpaParms[1], &dataCb.uPID);
    AssertRCReturn(vrc, vrc);
    vrc = HGCMSvcGetU32(&pSvcCbData->mpaParms[2], &dataCb.uStatus);
    AssertRCReturn(vrc, vrc);
    vrc = HGCMSvcGetU32(&pSvcCbData->mpaParms[3], &dataCb.uFlags);
    AssertRCReturn(vrc, vrc);
    vrc = HGCMSvcGetPv(&pSvcCbData->mpaParms[4], &dataCb.pvData, &dataCb.cbData);
    AssertRCReturn(vrc, vrc);

    LogFlowThisFunc(("uPID=%RU32, uStatus=%RU32, uFlags=%RU32\n",
                     dataCb.uPID, dataCb.uStatus, dataCb.uFlags));

    vrc = i_checkPID(dataCb.uPID);
    if (RT_SUCCESS(vrc))
    {
        ProcessStatus_T procStatus = ProcessStatus_Undefined;
        int procRc = VINF_SUCCESS;

        switch (dataCb.uStatus)
        {
            case PROC_STS_STARTED:
            {
                procStatus = ProcessStatus_Started;

                AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
                mData.mPID = dataCb.uPID; /* Set the process PID. */
                break;
            }

            case PROC_STS_TEN:
            {
                procStatus = ProcessStatus_TerminatedNormally;

                AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
                mData.mExitCode = dataCb.uFlags; /* Contains the exit code. */
                break;
            }

            case PROC_STS_TES:
            {
                procStatus = ProcessStatus_TerminatedSignal;

                AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
                mData.mExitCode = dataCb.uFlags; /* Contains the signal. */
                break;
            }

            case PROC_STS_TEA:
            {
                procStatus = ProcessStatus_TerminatedAbnormally;
                break;
            }

            case PROC_STS_TOK:
            {
                procStatus = ProcessStatus_TimedOutKilled;
                break;
            }

            case PROC_STS_TOA:
            {
                procStatus = ProcessStatus_TimedOutAbnormally;
                break;
            }

            case PROC_STS_DWN:
            {
                procStatus = ProcessStatus_Down;
                break;
            }

            case PROC_STS_ERROR:
            {
                procRc = dataCb.uFlags; /* mFlags contains the IPRT error sent from the guest. */
                procStatus = ProcessStatus_Error;
                break;
            }

            case PROC_STS_UNDEFINED:
            default:
            {
                /* Silently skip this request. */
                procStatus = ProcessStatus_Undefined;
                break;
            }
        }

        LogFlowThisFunc(("Got rc=%Rrc, procSts=%RU32, procRc=%Rrc\n",
                         vrc, procStatus, procRc));

        /* Set the process status. */
        int rc2 = i_setProcessStatus(procStatus, procRc);
        if (RT_SUCCESS(vrc))
            vrc = rc2;
    }

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

int GuestProcess::i_onProcessOutput(PVBOXGUESTCTRLHOSTCBCTX pCbCtx, PVBOXGUESTCTRLHOSTCALLBACK pSvcCbData)
{
    RT_NOREF(pCbCtx);
    AssertPtrReturn(pSvcCbData, VERR_INVALID_POINTER);

    if (pSvcCbData->mParms < 5)
        return VERR_INVALID_PARAMETER;

    CALLBACKDATA_PROC_OUTPUT dataCb;
    /* pSvcCb->mpaParms[0] always contains the context ID. */
    int vrc = HGCMSvcGetU32(&pSvcCbData->mpaParms[1], &dataCb.uPID);
    AssertRCReturn(vrc, vrc);
    vrc = HGCMSvcGetU32(&pSvcCbData->mpaParms[2], &dataCb.uHandle);
    AssertRCReturn(vrc, vrc);
    vrc = HGCMSvcGetU32(&pSvcCbData->mpaParms[3], &dataCb.uFlags);
    AssertRCReturn(vrc, vrc);
    vrc = HGCMSvcGetPv(&pSvcCbData->mpaParms[4], &dataCb.pvData, &dataCb.cbData);
    AssertRCReturn(vrc, vrc);

    LogFlowThisFunc(("uPID=%RU32, uHandle=%RU32, uFlags=%RI32, pvData=%p, cbData=%RU32\n",
                     dataCb.uPID, dataCb.uHandle, dataCb.uFlags, dataCb.pvData, dataCb.cbData));

    vrc = i_checkPID(dataCb.uPID);
    if (RT_SUCCESS(vrc))
    {
        com::SafeArray<BYTE> data((size_t)dataCb.cbData);
        if (dataCb.cbData)
            data.initFrom((BYTE*)dataCb.pvData, dataCb.cbData);

        ::FireGuestProcessOutputEvent(mEventSource, mSession, this,
                                      mData.mPID, dataCb.uHandle, dataCb.cbData, ComSafeArrayAsInParam(data));
    }

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/**
 * @copydoc GuestObject::i_onUnregister
 */
int GuestProcess::i_onUnregister(void)
{
    LogFlowThisFuncEnter();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    int vrc = VINF_SUCCESS;

    /*
     * Note: The event source stuff holds references to this object,
     *       so make sure that this is cleaned up *before* calling uninit().
     */
    if (!mEventSource.isNull())
    {
        mEventSource->UnregisterListener(mLocalListener);

        mLocalListener.setNull();
        unconst(mEventSource).setNull();
    }

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/**
 * @copydoc GuestObject::i_onSessionStatusChange
 */
int GuestProcess::i_onSessionStatusChange(GuestSessionStatus_T enmSessionStatus)
{
    LogFlowThisFuncEnter();

    int vrc = VINF_SUCCESS;

    /* If the session now is in a terminated state, set the process status
     * to "down", as there is not much else we can do now. */
    if (GuestSession::i_isTerminated(enmSessionStatus))
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

        vrc = i_setProcessStatus(ProcessStatus_Down, 0 /* rc, ignored */);
    }

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

int GuestProcess::i_readData(uint32_t uHandle, uint32_t uSize, uint32_t uTimeoutMS,
                             void *pvData, size_t cbData, uint32_t *pcbRead, int *prcGuest)
{
    LogFlowThisFunc(("uPID=%RU32, uHandle=%RU32, uSize=%RU32, uTimeoutMS=%RU32, pvData=%p, cbData=%RU32, prcGuest=%p\n",
                     mData.mPID, uHandle, uSize, uTimeoutMS, pvData, cbData, prcGuest));
    AssertReturn(uSize, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pvData, VERR_INVALID_POINTER);
    AssertReturn(cbData >= uSize, VERR_INVALID_PARAMETER);
    /* pcbRead is optional. */

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (   mData.mStatus != ProcessStatus_Started
        /* Skip reading if the process wasn't started with the appropriate
         * flags. */
        || (   (   uHandle == OUTPUT_HANDLE_ID_STDOUT
                || uHandle == OUTPUT_HANDLE_ID_STDOUT_DEPRECATED)
            && !(mData.mProcess.mFlags & ProcessCreateFlag_WaitForStdOut))
        || (   uHandle == OUTPUT_HANDLE_ID_STDERR
            && !(mData.mProcess.mFlags & ProcessCreateFlag_WaitForStdErr))
       )
    {
        if (pcbRead)
            *pcbRead = 0;
        if (prcGuest)
            *prcGuest = VINF_SUCCESS;
        return VINF_SUCCESS; /* Nothing to read anymore. */
    }

    int vrc;

    GuestWaitEvent *pEvent = NULL;
    GuestEventTypes eventTypes;
    try
    {
        /*
         * On Guest Additions < 4.3 there is no guarantee that the process status
         * change arrives *after* the output event, e.g. if this was the last output
         * block being read and the process will report status "terminate".
         * So just skip checking for process status change and only wait for the
         * output event.
         */
        if (mSession->i_getProtocolVersion() >= 2)
            eventTypes.push_back(VBoxEventType_OnGuestProcessStateChanged);
        eventTypes.push_back(VBoxEventType_OnGuestProcessOutput);

        vrc = registerWaitEvent(eventTypes, &pEvent);
    }
    catch (std::bad_alloc &)
    {
        vrc = VERR_NO_MEMORY;
    }

    if (RT_FAILURE(vrc))
        return vrc;

    if (RT_SUCCESS(vrc))
    {
        VBOXHGCMSVCPARM paParms[8];
        int i = 0;
        HGCMSvcSetU32(&paParms[i++], pEvent->ContextID());
        HGCMSvcSetU32(&paParms[i++], mData.mPID);
        HGCMSvcSetU32(&paParms[i++], uHandle);
        HGCMSvcSetU32(&paParms[i++], 0 /* Flags, none set yet. */);

        alock.release(); /* Drop the write lock before sending. */

        vrc = sendMessage(HOST_MSG_EXEC_GET_OUTPUT, i, paParms);
    }

    if (RT_SUCCESS(vrc))
        vrc = i_waitForOutput(pEvent, uHandle, uTimeoutMS,
                              pvData, cbData, pcbRead);

    unregisterWaitEvent(pEvent);

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/* Does not do locking; caller is responsible for that! */
int GuestProcess::i_setProcessStatus(ProcessStatus_T procStatus, int procRc)
{
    LogFlowThisFuncEnter();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    LogFlowThisFunc(("oldStatus=%RU32, newStatus=%RU32, procRc=%Rrc\n",
                     mData.mStatus, procStatus, procRc));

    if (procStatus == ProcessStatus_Error)
    {
        AssertMsg(RT_FAILURE(procRc), ("Guest rc must be an error (%Rrc)\n", procRc));
        /* Do not allow overwriting an already set error. If this happens
         * this means we forgot some error checking/locking somewhere. */
        AssertMsg(RT_SUCCESS(mData.mLastError), ("Guest rc already set (to %Rrc)\n", mData.mLastError));
    }
    else
        AssertMsg(RT_SUCCESS(procRc), ("Guest rc must not be an error (%Rrc)\n", procRc));

    int rc = VINF_SUCCESS;

    if (mData.mStatus != procStatus) /* Was there a process status change? */
    {
        mData.mStatus    = procStatus;
        mData.mLastError = procRc;

        ComObjPtr<VirtualBoxErrorInfo> errorInfo;
        HRESULT hr = errorInfo.createObject();
        ComAssertComRC(hr);
        if (RT_FAILURE(mData.mLastError))
        {
            hr = errorInfo->initEx(VBOX_E_IPRT_ERROR, mData.mLastError,
                                   COM_IIDOF(IGuestProcess), getComponentName(),
                                   i_guestErrorToString(mData.mLastError, mData.mProcess.mExecutable.c_str()));
            ComAssertComRC(hr);
        }

        /* Copy over necessary data before releasing lock again. */
        uint32_t uPID =  mData.mPID;
        /** @todo Also handle mSession? */

        alock.release(); /* Release lock before firing off event. */

        ::FireGuestProcessStateChangedEvent(mEventSource, mSession, this, uPID, procStatus, errorInfo);
#if 0
        /*
         * On Guest Additions < 4.3 there is no guarantee that outstanding
         * requests will be delivered to the host after the process has ended,
         * so just cancel all waiting events here to not let clients run
         * into timeouts.
         */
        if (   mSession->getProtocolVersion() < 2
            && hasEnded())
        {
            LogFlowThisFunc(("Process ended, canceling outstanding wait events ...\n"));
            rc = cancelWaitEvents();
        }
#endif
    }

    return rc;
}

int GuestProcess::i_startProcess(uint32_t cMsTimeout, int *prcGuest)
{
    LogFlowThisFunc(("cMsTimeout=%RU32, procExe=%s, procTimeoutMS=%RU32, procFlags=%x, sessionID=%RU32\n",
                     cMsTimeout, mData.mProcess.mExecutable.c_str(), mData.mProcess.mTimeoutMS, mData.mProcess.mFlags,
                     mSession->i_getId()));

    /* Wait until the caller function (if kicked off by a thread)
     * has returned and continue operation. */
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    mData.mStatus = ProcessStatus_Starting;

    int vrc;

    GuestWaitEvent *pEvent = NULL;
    GuestEventTypes eventTypes;
    try
    {
        eventTypes.push_back(VBoxEventType_OnGuestProcessStateChanged);
        vrc = registerWaitEvent(eventTypes, &pEvent);
    }
    catch (std::bad_alloc &)
    {
        vrc = VERR_NO_MEMORY;
    }
    if (RT_FAILURE(vrc))
        return vrc;

    vrc = i_startProcessInner(cMsTimeout, alock, pEvent, prcGuest);

    unregisterWaitEvent(pEvent);

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

int GuestProcess::i_startProcessInner(uint32_t cMsTimeout, AutoWriteLock &rLock, GuestWaitEvent *pEvent, int *prcGuest)
{
    GuestSession *pSession = mSession;
    AssertPtr(pSession);
    uint32_t const uProtocol = pSession->i_getProtocolVersion();

    const GuestCredentials &sessionCreds = pSession->i_getCredentials();

    /* Prepare arguments. */
    size_t cArgs = mData.mProcess.mArguments.size();
    if (cArgs >= 128*1024)
        return VERR_BUFFER_OVERFLOW;

    size_t cbArgs = 0;
    char *pszArgs = NULL;
    int vrc = VINF_SUCCESS;
    if (cArgs)
    {
        char const **papszArgv = (char const **)RTMemAlloc((cArgs + 1) * sizeof(papszArgv[0]));
        AssertReturn(papszArgv, VERR_NO_MEMORY);

        for (size_t i = 0; i < cArgs; i++)
        {
            papszArgv[i] = mData.mProcess.mArguments[i].c_str();
            AssertPtr(papszArgv[i]);
        }
        papszArgv[cArgs] = NULL;

        Guest *pGuest = mSession->i_getParent();
        AssertPtr(pGuest);

        const uint64_t fGuestControlFeatures0 = pGuest->i_getGuestControlFeatures0();

        /* If the Guest Additions don't support using argv[0] correctly (< 6.1.x), don't supply it. */
        if (!(fGuestControlFeatures0 & VBOX_GUESTCTRL_GF_0_PROCESS_ARGV0))
            vrc = RTGetOptArgvToString(&pszArgs, papszArgv + 1, RTGETOPTARGV_CNV_QUOTE_BOURNE_SH);
        else /* ... else send the whole argv, including argv[0]. */
            vrc = RTGetOptArgvToString(&pszArgs, papszArgv, RTGETOPTARGV_CNV_QUOTE_BOURNE_SH);

        RTMemFree(papszArgv);
        if (RT_FAILURE(vrc))
            return vrc;

        /* Note! No direct returns after this. */
    }

    /* Calculate arguments size (in bytes). */
    AssertPtr(pszArgs);
    cbArgs = strlen(pszArgs) + 1; /* Include terminating zero. */

    /* Prepare environment.  The guest service dislikes the empty string at the end, so drop it. */
    size_t  cbEnvBlock   = 0;    /* Shut up MSVC. */
    char   *pszzEnvBlock = NULL; /* Ditto. */
    vrc = mData.mProcess.mEnvironmentChanges.queryUtf8Block(&pszzEnvBlock, &cbEnvBlock);
    if (RT_SUCCESS(vrc))
    {
        Assert(cbEnvBlock > 0);
        cbEnvBlock--;
        AssertPtr(pszzEnvBlock);

        /* Prepare HGCM call. */
        VBOXHGCMSVCPARM paParms[16];
        int i = 0;
        HGCMSvcSetU32(&paParms[i++], pEvent->ContextID());
        HGCMSvcSetRTCStr(&paParms[i++], mData.mProcess.mExecutable);
        HGCMSvcSetU32(&paParms[i++], mData.mProcess.mFlags);
        HGCMSvcSetU32(&paParms[i++], (uint32_t)mData.mProcess.mArguments.size());
        HGCMSvcSetPv(&paParms[i++], pszArgs, (uint32_t)cbArgs);
        HGCMSvcSetU32(&paParms[i++], mData.mProcess.mEnvironmentChanges.count());
        HGCMSvcSetU32(&paParms[i++], (uint32_t)cbEnvBlock);
        HGCMSvcSetPv(&paParms[i++], pszzEnvBlock, (uint32_t)cbEnvBlock);
        if (uProtocol < 2)
        {
            /* In protocol v1 (VBox < 4.3) the credentials were part of the execution
             * call. In newer protocols these credentials are part of the opened guest
             * session, so not needed anymore here. */
            HGCMSvcSetRTCStr(&paParms[i++], sessionCreds.mUser);
            HGCMSvcSetRTCStr(&paParms[i++], sessionCreds.mPassword);
        }
        /*
         * If the WaitForProcessStartOnly flag is set, we only want to define and wait for a timeout
         * until the process was started - the process itself then gets an infinite timeout for execution.
         * This is handy when we want to start a process inside a worker thread within a certain timeout
         * but let the started process perform lengthly operations then.
         */
        if (mData.mProcess.mFlags & ProcessCreateFlag_WaitForProcessStartOnly)
            HGCMSvcSetU32(&paParms[i++], UINT32_MAX /* Infinite timeout */);
        else
            HGCMSvcSetU32(&paParms[i++], mData.mProcess.mTimeoutMS);
        if (uProtocol >= 2)
        {
            HGCMSvcSetU32(&paParms[i++], mData.mProcess.mPriority);
            /* CPU affinity: We only support one CPU affinity block at the moment,
             * so that makes up to 64 CPUs total. This can be more in the future. */
            HGCMSvcSetU32(&paParms[i++], 1);
            /* The actual CPU affinity blocks. */
            HGCMSvcSetPv(&paParms[i++], (void *)&mData.mProcess.mAffinity, sizeof(mData.mProcess.mAffinity));
        }

        rLock.release(); /* Drop the write lock before sending. */

        vrc = sendMessage(HOST_MSG_EXEC_CMD, i, paParms);
        if (RT_FAILURE(vrc))
        {
            int rc2 = i_setProcessStatus(ProcessStatus_Error, vrc);
            AssertRC(rc2);
        }

        mData.mProcess.mEnvironmentChanges.freeUtf8Block(pszzEnvBlock);
    }

    RTStrFree(pszArgs);

    if (RT_SUCCESS(vrc))
        vrc = i_waitForStatusChange(pEvent, cMsTimeout,
                                    NULL /* Process status */, prcGuest);
    return vrc;
}

int GuestProcess::i_startProcessAsync(void)
{
    LogFlowThisFuncEnter();

    /* Create the task: */
    GuestProcessStartTask *pTask = NULL;
    try
    {
        pTask = new GuestProcessStartTask(this);
    }
    catch (std::bad_alloc &)
    {
        LogFlowThisFunc(("out of memory\n"));
        return VERR_NO_MEMORY;
    }
    AssertReturnStmt(pTask->i_isOk(), delete pTask, E_FAIL); /* cannot fail for GuestProcessStartTask. */
    LogFlowThisFunc(("Successfully created GuestProcessStartTask object\n"));

    /* Start the thread (always consumes the task): */
    HRESULT hrc = pTask->createThread();
    pTask = NULL;
    if (SUCCEEDED(hrc))
        return VINF_SUCCESS;
    LogFlowThisFunc(("Failed to create thread for GuestProcessStartTask\n"));
    return VERR_GENERAL_FAILURE;
}

/* static */
int GuestProcess::i_startProcessThreadTask(GuestProcessStartTask *pTask)
{
    LogFlowFunc(("pTask=%p\n", pTask));

    const ComObjPtr<GuestProcess> pProcess(pTask->i_process());
    Assert(!pProcess.isNull());

    AutoCaller autoCaller(pProcess);
    if (FAILED(autoCaller.rc()))
        return VERR_COM_UNEXPECTED;

    int vrc = pProcess->i_startProcess(30 * 1000 /* 30s timeout */, NULL /* Guest rc, ignored */);
    /* Nothing to do here anymore. */

    LogFlowFunc(("pProcess=%p, vrc=%Rrc\n", (GuestProcess *)pProcess, vrc));
    return vrc;
}

int GuestProcess::i_terminateProcess(uint32_t uTimeoutMS, int *prcGuest)
{
    /* prcGuest is optional. */
    LogFlowThisFunc(("uTimeoutMS=%RU32\n", uTimeoutMS));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    int vrc = VINF_SUCCESS;

    if (mData.mStatus != ProcessStatus_Started)
    {
        LogFlowThisFunc(("Process not in started state (state is %RU32), skipping termination\n",
                         mData.mStatus));
    }
    else
    {
        AssertPtr(mSession);
        /* Note: VBox < 4.3 (aka protocol version 1) does not
         *       support this, so just skip. */
        if (mSession->i_getProtocolVersion() < 2)
            vrc = VERR_NOT_SUPPORTED;

        if (RT_SUCCESS(vrc))
        {
            GuestWaitEvent *pEvent = NULL;
            GuestEventTypes eventTypes;
            try
            {
                eventTypes.push_back(VBoxEventType_OnGuestProcessStateChanged);

                vrc = registerWaitEvent(eventTypes, &pEvent);
            }
            catch (std::bad_alloc &)
            {
                vrc = VERR_NO_MEMORY;
            }

            if (RT_FAILURE(vrc))
                return vrc;

            VBOXHGCMSVCPARM paParms[4];
            int i = 0;
            HGCMSvcSetU32(&paParms[i++], pEvent->ContextID());
            HGCMSvcSetU32(&paParms[i++], mData.mPID);

            alock.release(); /* Drop the write lock before sending. */

            vrc = sendMessage(HOST_MSG_EXEC_TERMINATE, i, paParms);
            if (RT_SUCCESS(vrc))
                vrc = i_waitForStatusChange(pEvent, uTimeoutMS,
                                            NULL /* ProcessStatus */, prcGuest);
            unregisterWaitEvent(pEvent);
        }
    }

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/* static */
ProcessWaitResult_T GuestProcess::i_waitFlagsToResultEx(uint32_t fWaitFlags,
                                                        ProcessStatus_T oldStatus, ProcessStatus_T newStatus,
                                                        uint32_t uProcFlags, uint32_t uProtocol)
{
    ProcessWaitResult_T waitResult = ProcessWaitResult_None;

    switch (newStatus)
    {
        case ProcessStatus_TerminatedNormally:
        case ProcessStatus_TerminatedSignal:
        case ProcessStatus_TerminatedAbnormally:
        case ProcessStatus_Down:
            /* Nothing to wait for anymore. */
            waitResult = ProcessWaitResult_Terminate;
            break;

        case ProcessStatus_TimedOutKilled:
        case ProcessStatus_TimedOutAbnormally:
            /* Dito. */
            waitResult = ProcessWaitResult_Timeout;
            break;

        case ProcessStatus_Started:
            switch (oldStatus)
            {
                case ProcessStatus_Undefined:
                case ProcessStatus_Starting:
                    /* Also wait for process start. */
                    if (fWaitFlags & ProcessWaitForFlag_Start)
                        waitResult = ProcessWaitResult_Start;
                    else
                    {
                        /*
                         * If ProcessCreateFlag_WaitForProcessStartOnly was specified on process creation the
                         * caller is not interested in getting further process statuses -- so just don't notify
                         * anything here anymore and return.
                         */
                        if (uProcFlags & ProcessCreateFlag_WaitForProcessStartOnly)
                            waitResult = ProcessWaitResult_Start;
                    }
                    break;

                case ProcessStatus_Started:
                    /* Only wait for process start. */
                    if (fWaitFlags & ProcessWaitForFlag_Start)
                        waitResult = ProcessWaitResult_Start;
                    break;

                default:
                    AssertMsgFailed(("Unhandled old status %RU32 before new status 'started'\n",
                                     oldStatus));
                    if (fWaitFlags & ProcessWaitForFlag_Start)
                        waitResult = ProcessWaitResult_Start;
                    break;
            }
            break;

        case ProcessStatus_Error:
            /* Nothing to wait for anymore. */
            waitResult = ProcessWaitResult_Error;
            break;

        case ProcessStatus_Undefined:
        case ProcessStatus_Starting:
        case ProcessStatus_Terminating:
        case ProcessStatus_Paused:
            /* No result available yet, leave wait
             * flags untouched. */
            break;
#ifdef VBOX_WITH_XPCOM_CPP_ENUM_HACK
        case ProcessStatus_32BitHack: AssertFailedBreak(); /* (compiler warnings) */
#endif
    }

    if (newStatus == ProcessStatus_Started)
    {
        /*
         * Filter out waits which are *not* supported using
         * older guest control Guest Additions.
         *
         */
        /** @todo ProcessWaitForFlag_Std* flags are not implemented yet. */
        if (uProtocol < 99) /* See @todo above. */
        {
            if (   waitResult == ProcessWaitResult_None
                /* We don't support waiting for stdin, out + err,
                 * just skip waiting then. */
                && (   (fWaitFlags & ProcessWaitForFlag_StdIn)
                    || (fWaitFlags & ProcessWaitForFlag_StdOut)
                    || (fWaitFlags & ProcessWaitForFlag_StdErr)
                   )
               )
            {
                /* Use _WaitFlagNotSupported because we don't know what to tell the caller. */
                waitResult = ProcessWaitResult_WaitFlagNotSupported;
            }
        }
    }

#ifdef DEBUG
    LogFlowFunc(("oldStatus=%RU32, newStatus=%RU32, fWaitFlags=0x%x, waitResult=%RU32\n",
                 oldStatus, newStatus, fWaitFlags, waitResult));
#endif
    return waitResult;
}

ProcessWaitResult_T GuestProcess::i_waitFlagsToResult(uint32_t fWaitFlags)
{
    AssertPtr(mSession);
    return GuestProcess::i_waitFlagsToResultEx(fWaitFlags,
                                               mData.mStatus /* oldStatus */, mData.mStatus /* newStatus */,
                                               mData.mProcess.mFlags, mSession->i_getProtocolVersion());
}

int GuestProcess::i_waitFor(uint32_t fWaitFlags, ULONG uTimeoutMS,
                            ProcessWaitResult_T &waitResult, int *prcGuest)
{
    AssertReturn(fWaitFlags, VERR_INVALID_PARAMETER);

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    LogFlowThisFunc(("fWaitFlags=0x%x, uTimeoutMS=%RU32, procStatus=%RU32, procRc=%Rrc, prcGuest=%p\n",
                     fWaitFlags, uTimeoutMS, mData.mStatus, mData.mLastError, prcGuest));

    /* Did some error occur before? Then skip waiting and return. */
    ProcessStatus_T curStatus = mData.mStatus;
    if (curStatus == ProcessStatus_Error)
    {
        waitResult = ProcessWaitResult_Error;
        AssertMsg(RT_FAILURE(mData.mLastError),
                             ("No error rc (%Rrc) set when guest process indicated an error\n", mData.mLastError));
        if (prcGuest)
            *prcGuest = mData.mLastError; /* Return last set error. */
        LogFlowThisFunc(("Process is in error state (rcGuest=%Rrc)\n", mData.mLastError));
        return VERR_GSTCTL_GUEST_ERROR;
    }

    waitResult = i_waitFlagsToResult(fWaitFlags);

    /* No waiting needed? Return immediately using the last set error. */
    if (waitResult != ProcessWaitResult_None)
    {
        if (prcGuest)
            *prcGuest = mData.mLastError; /* Return last set error (if any). */
        LogFlowThisFunc(("Nothing to wait for (rcGuest=%Rrc)\n", mData.mLastError));
        return RT_SUCCESS(mData.mLastError) ? VINF_SUCCESS : VERR_GSTCTL_GUEST_ERROR;
    }

    /* Adjust timeout. Passing 0 means RT_INDEFINITE_WAIT. */
    if (!uTimeoutMS)
        uTimeoutMS = RT_INDEFINITE_WAIT;

    int vrc;

    GuestWaitEvent *pEvent = NULL;
    GuestEventTypes eventTypes;
    try
    {
        eventTypes.push_back(VBoxEventType_OnGuestProcessStateChanged);

        vrc = registerWaitEvent(eventTypes, &pEvent);
    }
    catch (std::bad_alloc &)
    {
        vrc = VERR_NO_MEMORY;
    }

    if (RT_FAILURE(vrc))
        return vrc;

    alock.release(); /* Release lock before waiting. */

    /*
     * Do the actual waiting.
     */
    ProcessStatus_T newStatus = ProcessStatus_Undefined;
    uint64_t u64StartMS = RTTimeMilliTS();
    for (;;)
    {
        uint64_t u64ElapsedMS = RTTimeMilliTS() - u64StartMS;
        if (   uTimeoutMS   != RT_INDEFINITE_WAIT
            && u64ElapsedMS >= uTimeoutMS)
        {
            vrc = VERR_TIMEOUT;
            break;
        }

        vrc = i_waitForStatusChange(pEvent,
                                    uTimeoutMS == RT_INDEFINITE_WAIT
                                    ? RT_INDEFINITE_WAIT : uTimeoutMS - (uint32_t)u64ElapsedMS,
                                    &newStatus, prcGuest);
        if (RT_SUCCESS(vrc))
        {
            alock.acquire();

            waitResult = i_waitFlagsToResultEx(fWaitFlags, curStatus, newStatus,
                                               mData.mProcess.mFlags, mSession->i_getProtocolVersion());
#ifdef DEBUG
            LogFlowThisFunc(("Got new status change: fWaitFlags=0x%x, newStatus=%RU32, waitResult=%RU32\n",
                             fWaitFlags, newStatus, waitResult));
#endif
            if (ProcessWaitResult_None != waitResult) /* We got a waiting result. */
                break;
        }
        else /* Waiting failed, bail out. */
            break;

        alock.release(); /* Don't hold lock in next waiting round. */
    }

    unregisterWaitEvent(pEvent);

    LogFlowThisFunc(("Returned waitResult=%RU32, newStatus=%RU32, rc=%Rrc\n",
                     waitResult, newStatus, vrc));
    return vrc;
}

int GuestProcess::i_waitForInputNotify(GuestWaitEvent *pEvent, uint32_t uHandle, uint32_t uTimeoutMS,
                                       ProcessInputStatus_T *pInputStatus, uint32_t *pcbProcessed)
{
    RT_NOREF(uHandle);
    AssertPtrReturn(pEvent, VERR_INVALID_POINTER);

    VBoxEventType_T evtType;
    ComPtr<IEvent> pIEvent;
    int vrc = waitForEvent(pEvent, uTimeoutMS,
                           &evtType, pIEvent.asOutParam());
    if (RT_SUCCESS(vrc))
    {
        if (evtType == VBoxEventType_OnGuestProcessInputNotify)
        {
            ComPtr<IGuestProcessInputNotifyEvent> pProcessEvent = pIEvent;
            Assert(!pProcessEvent.isNull());

            if (pInputStatus)
            {
                HRESULT hr2 = pProcessEvent->COMGETTER(Status)(pInputStatus);
                ComAssertComRC(hr2);
            }
            if (pcbProcessed)
            {
                HRESULT hr2 = pProcessEvent->COMGETTER(Processed)((ULONG*)pcbProcessed);
                ComAssertComRC(hr2);
            }
        }
        else
            vrc = VWRN_GSTCTL_OBJECTSTATE_CHANGED;
    }

    LogFlowThisFunc(("Returning pEvent=%p, uHandle=%RU32, rc=%Rrc\n",
                     pEvent, uHandle, vrc));
    return vrc;
}

int GuestProcess::i_waitForOutput(GuestWaitEvent *pEvent, uint32_t uHandle, uint32_t uTimeoutMS,
                                  void *pvData, size_t cbData, uint32_t *pcbRead)
{
    AssertPtrReturn(pEvent, VERR_INVALID_POINTER);
    /* pvData is optional. */
    /* cbData is optional. */
    /* pcbRead is optional. */

    LogFlowThisFunc(("cEventTypes=%zu, pEvent=%p, uHandle=%RU32, uTimeoutMS=%RU32, pvData=%p, cbData=%zu, pcbRead=%p\n",
                     pEvent->TypeCount(), pEvent, uHandle, uTimeoutMS, pvData, cbData, pcbRead));

    int vrc;

    VBoxEventType_T evtType;
    ComPtr<IEvent> pIEvent;
    do
    {
        vrc = waitForEvent(pEvent, uTimeoutMS,
                           &evtType, pIEvent.asOutParam());
        if (RT_SUCCESS(vrc))
        {
            if (evtType == VBoxEventType_OnGuestProcessOutput)
            {
                ComPtr<IGuestProcessOutputEvent> pProcessEvent = pIEvent;
                Assert(!pProcessEvent.isNull());

                ULONG uHandleEvent;
                HRESULT hr = pProcessEvent->COMGETTER(Handle)(&uHandleEvent);
                if (   SUCCEEDED(hr)
                    && uHandleEvent == uHandle)
                {
                    if (pvData)
                    {
                        com::SafeArray <BYTE> data;
                        hr = pProcessEvent->COMGETTER(Data)(ComSafeArrayAsOutParam(data));
                        ComAssertComRC(hr);
                        size_t cbRead = data.size();
                        if (cbRead)
                        {
                            if (cbRead <= cbData)
                            {
                                /* Copy data from event into our buffer. */
                                memcpy(pvData, data.raw(), data.size());
                            }
                            else
                                vrc = VERR_BUFFER_OVERFLOW;

                            LogFlowThisFunc(("Read %zu bytes (uHandle=%RU32), rc=%Rrc\n",
                                             cbRead, uHandleEvent, vrc));
                        }
                    }

                    if (   RT_SUCCESS(vrc)
                        && pcbRead)
                    {
                        ULONG cbRead;
                        hr = pProcessEvent->COMGETTER(Processed)(&cbRead);
                        ComAssertComRC(hr);
                        *pcbRead = (uint32_t)cbRead;
                    }

                    break;
                }
                else if (FAILED(hr))
                    vrc = VERR_COM_UNEXPECTED;
            }
            else
                vrc = VWRN_GSTCTL_OBJECTSTATE_CHANGED;
        }

    } while (vrc == VINF_SUCCESS);

    if (   vrc != VINF_SUCCESS
        && pcbRead)
    {
        *pcbRead = 0;
    }

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/**
 * Undocumented, you guess what it does.
 *
 * @note Similar code in GuestFile::i_waitForStatusChange() and
 *       GuestSession::i_waitForStatusChange().
 */
int GuestProcess::i_waitForStatusChange(GuestWaitEvent *pEvent, uint32_t uTimeoutMS,
                                        ProcessStatus_T *pProcessStatus, int *prcGuest)
{
    AssertPtrReturn(pEvent, VERR_INVALID_POINTER);
    /* pProcessStatus is optional. */
    /* prcGuest is optional. */

    VBoxEventType_T evtType;
    ComPtr<IEvent> pIEvent;
    int vrc = waitForEvent(pEvent, uTimeoutMS,
                           &evtType, pIEvent.asOutParam());
    if (RT_SUCCESS(vrc))
    {
        Assert(evtType == VBoxEventType_OnGuestProcessStateChanged);
        ComPtr<IGuestProcessStateChangedEvent> pProcessEvent = pIEvent;
        Assert(!pProcessEvent.isNull());

        ProcessStatus_T procStatus;
        HRESULT hr = pProcessEvent->COMGETTER(Status)(&procStatus);
        ComAssertComRC(hr);
        if (pProcessStatus)
            *pProcessStatus = procStatus;

        ComPtr<IVirtualBoxErrorInfo> errorInfo;
        hr = pProcessEvent->COMGETTER(Error)(errorInfo.asOutParam());
        ComAssertComRC(hr);

        LONG lGuestRc;
        hr = errorInfo->COMGETTER(ResultDetail)(&lGuestRc);
        ComAssertComRC(hr);

        LogFlowThisFunc(("Got procStatus=%RU32, rcGuest=%RI32 (%Rrc)\n",
                         procStatus, lGuestRc, lGuestRc));

        if (RT_FAILURE((int)lGuestRc))
            vrc = VERR_GSTCTL_GUEST_ERROR;

        if (prcGuest)
            *prcGuest = (int)lGuestRc;
    }
    /* waitForEvent may also return VERR_GSTCTL_GUEST_ERROR like we do above, so make prcGuest is set. */
    else if (vrc == VERR_GSTCTL_GUEST_ERROR && prcGuest)
        *prcGuest = pEvent->GuestResult();
    Assert(vrc != VERR_GSTCTL_GUEST_ERROR || !prcGuest || *prcGuest != (int)0xcccccccc);

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

#if 0 /* Unused */
/* static */
bool GuestProcess::i_waitResultImpliesEx(ProcessWaitResult_T waitResult, ProcessStatus_T procStatus, uint32_t uProtocol)
{
    RT_NOREF(uProtocol);

    bool fImplies;

    switch (waitResult)
    {
        case ProcessWaitResult_Start:
            fImplies = procStatus == ProcessStatus_Started;
            break;

        case ProcessWaitResult_Terminate:
            fImplies = (   procStatus == ProcessStatus_TerminatedNormally
                        || procStatus == ProcessStatus_TerminatedSignal
                        || procStatus == ProcessStatus_TerminatedAbnormally
                        || procStatus == ProcessStatus_TimedOutKilled
                        || procStatus == ProcessStatus_TimedOutAbnormally
                        || procStatus == ProcessStatus_Down
                        || procStatus == ProcessStatus_Error);
            break;

        default:
            fImplies = false;
            break;
    }

    return fImplies;
}
#endif /* unused */

int GuestProcess::i_writeData(uint32_t uHandle, uint32_t uFlags,
                              void *pvData, size_t cbData, uint32_t uTimeoutMS, uint32_t *puWritten, int *prcGuest)
{
    LogFlowThisFunc(("uPID=%RU32, uHandle=%RU32, uFlags=%RU32, pvData=%p, cbData=%RU32, uTimeoutMS=%RU32, puWritten=%p, prcGuest=%p\n",
                     mData.mPID, uHandle, uFlags, pvData, cbData, uTimeoutMS, puWritten, prcGuest));
    /* All is optional. There can be 0 byte writes. */
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (mData.mStatus != ProcessStatus_Started)
    {
        if (puWritten)
            *puWritten = 0;
        if (prcGuest)
            *prcGuest = VINF_SUCCESS;
        return VINF_SUCCESS; /* Not available for writing (anymore). */
    }

    int vrc;

    GuestWaitEvent *pEvent = NULL;
    GuestEventTypes eventTypes;
    try
    {
        /*
         * On Guest Additions < 4.3 there is no guarantee that the process status
         * change arrives *after* the input event, e.g. if this was the last input
         * block being written and the process will report status "terminate".
         * So just skip checking for process status change and only wait for the
         * input event.
         */
        if (mSession->i_getProtocolVersion() >= 2)
            eventTypes.push_back(VBoxEventType_OnGuestProcessStateChanged);
        eventTypes.push_back(VBoxEventType_OnGuestProcessInputNotify);

        vrc = registerWaitEvent(eventTypes, &pEvent);
    }
    catch (std::bad_alloc &)
    {
        vrc = VERR_NO_MEMORY;
    }

    if (RT_FAILURE(vrc))
        return vrc;

    VBOXHGCMSVCPARM paParms[5];
    int i = 0;
    HGCMSvcSetU32(&paParms[i++], pEvent->ContextID());
    HGCMSvcSetU32(&paParms[i++], mData.mPID);
    HGCMSvcSetU32(&paParms[i++], uFlags);
    HGCMSvcSetPv(&paParms[i++], pvData, (uint32_t)cbData);
    HGCMSvcSetU32(&paParms[i++], (uint32_t)cbData);

    alock.release(); /* Drop the write lock before sending. */

    uint32_t cbProcessed = 0;
    vrc = sendMessage(HOST_MSG_EXEC_SET_INPUT, i, paParms);
    if (RT_SUCCESS(vrc))
    {
        ProcessInputStatus_T inputStatus;
        vrc = i_waitForInputNotify(pEvent, uHandle, uTimeoutMS,
                                   &inputStatus, &cbProcessed);
        if (RT_SUCCESS(vrc))
        {
            /** @todo Set rcGuest. */

            if (puWritten)
                *puWritten = cbProcessed;
        }
        /** @todo Error handling. */
    }

    unregisterWaitEvent(pEvent);

    LogFlowThisFunc(("Returning cbProcessed=%RU32, rc=%Rrc\n",
                     cbProcessed, vrc));
    return vrc;
}

// implementation of public methods
/////////////////////////////////////////////////////////////////////////////

HRESULT GuestProcess::read(ULONG aHandle, ULONG aToRead, ULONG aTimeoutMS, std::vector<BYTE> &aData)
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    if (aToRead == 0)
        return setError(E_INVALIDARG, tr("The size to read is zero"));

    LogFlowThisFuncEnter();

    aData.resize(aToRead);

    HRESULT hr = S_OK;

    uint32_t cbRead;
    int rcGuest = VERR_IPE_UNINITIALIZED_STATUS;
    int vrc = i_readData(aHandle, aToRead, aTimeoutMS, &aData.front(), aToRead, &cbRead, &rcGuest);
    if (RT_SUCCESS(vrc))
    {
        if (aData.size() != cbRead)
            aData.resize(cbRead);
    }
    else
    {
        aData.resize(0);

        switch (vrc)
        {
            case VERR_GSTCTL_GUEST_ERROR:
                hr = setErrorExternal(this, Utf8StrFmt("Reading %RU32 bytes from guest process handle %RU32 failed", aToRead, aHandle),
                                      GuestErrorInfo(GuestErrorInfo::Type_Process, rcGuest, mData.mProcess.mExecutable.c_str()));
                break;

            default:
                hr = setErrorBoth(VBOX_E_IPRT_ERROR, vrc, tr("Reading from guest process \"%s\" (PID %RU32) failed: %Rrc"),
                                  mData.mProcess.mExecutable.c_str(), mData.mPID, vrc);
                break;
        }
    }

    LogFlowThisFunc(("rc=%Rrc, cbRead=%RU32\n", vrc, cbRead));

    LogFlowFuncLeaveRC(vrc);
    return hr;
}

HRESULT GuestProcess::terminate()
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    LogFlowThisFuncEnter();

    HRESULT hr = S_OK;

    int rcGuest = VERR_IPE_UNINITIALIZED_STATUS;
    int vrc = i_terminateProcess(30 * 1000 /* Timeout in ms */, &rcGuest);
    if (RT_FAILURE(vrc))
    {
        switch (vrc)
        {
           case VERR_GSTCTL_GUEST_ERROR:
                hr = setErrorExternal(this, "Terminating guest process failed",
                                      GuestErrorInfo(GuestErrorInfo::Type_Process, rcGuest, mData.mProcess.mExecutable.c_str()));
                break;

            case VERR_NOT_SUPPORTED:
                hr = setErrorBoth(VBOX_E_IPRT_ERROR, vrc,
                                  tr("Terminating guest process \"%s\" (PID %RU32) not supported by installed Guest Additions"),
                                  mData.mProcess.mExecutable.c_str(), mData.mPID);
                break;

            default:
                hr = setErrorBoth(VBOX_E_IPRT_ERROR, vrc, tr("Terminating guest process \"%s\" (PID %RU32) failed: %Rrc"),
                                  mData.mProcess.mExecutable.c_str(), mData.mPID, vrc);
                break;
        }
    }

    /* Remove process from guest session list. Now only API clients
     * still can hold references to it. */
    AssertPtr(mSession);
    int rc2 = mSession->i_processUnregister(this);
    if (RT_SUCCESS(vrc))
        vrc = rc2;

    LogFlowFuncLeaveRC(vrc);
    return hr;
}

HRESULT GuestProcess::waitFor(ULONG aWaitFor, ULONG aTimeoutMS, ProcessWaitResult_T *aReason)
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    LogFlowThisFuncEnter();

    /* Validate flags: */
    static ULONG const s_fValidFlags = ProcessWaitForFlag_None   | ProcessWaitForFlag_Start  | ProcessWaitForFlag_Terminate
                                     | ProcessWaitForFlag_StdIn  | ProcessWaitForFlag_StdOut | ProcessWaitForFlag_StdErr;
    if (aWaitFor & ~s_fValidFlags)
        return setErrorBoth(E_INVALIDARG, VERR_INVALID_FLAGS, tr("Flags value %#x, invalid: %#x"),
                            aWaitFor, aWaitFor & ~s_fValidFlags);

    /*
     * Note: Do not hold any locks here while waiting!
     */
    HRESULT hr = S_OK;

    int rcGuest = VERR_IPE_UNINITIALIZED_STATUS;
    ProcessWaitResult_T waitResult;
    int vrc = i_waitFor(aWaitFor, aTimeoutMS, waitResult, &rcGuest);
    if (RT_SUCCESS(vrc))
    {
        *aReason = waitResult;
    }
    else
    {
        switch (vrc)
        {
            case VERR_GSTCTL_GUEST_ERROR:
                hr = setErrorExternal(this, Utf8StrFmt("Waiting for guest process (flags %#x) failed", aWaitFor),
                                      GuestErrorInfo(GuestErrorInfo::Type_Process, rcGuest, mData.mProcess.mExecutable.c_str()));
                break;

            case VERR_TIMEOUT:
                *aReason = ProcessWaitResult_Timeout;
                break;

            default:
                hr = setErrorBoth(VBOX_E_IPRT_ERROR, vrc, tr("Waiting for guest process \"%s\" (PID %RU32) failed: %Rrc"),
                                  mData.mProcess.mExecutable.c_str(), mData.mPID, vrc);
                break;
        }
    }

    LogFlowFuncLeaveRC(vrc);
    return hr;
}

HRESULT GuestProcess::waitForArray(const std::vector<ProcessWaitForFlag_T> &aWaitFor,
                                   ULONG aTimeoutMS, ProcessWaitResult_T *aReason)
{
    uint32_t fWaitFor = ProcessWaitForFlag_None;
    for (size_t i = 0; i < aWaitFor.size(); i++)
        fWaitFor |= aWaitFor[i];

    return WaitFor(fWaitFor, aTimeoutMS, aReason);
}

HRESULT GuestProcess::write(ULONG aHandle, ULONG aFlags, const std::vector<BYTE> &aData,
                            ULONG aTimeoutMS, ULONG *aWritten)
{
    static ULONG const s_fValidFlags = ProcessInputFlag_None | ProcessInputFlag_EndOfFile;
    if (aFlags & ~s_fValidFlags)
        return setErrorBoth(E_INVALIDARG, VERR_INVALID_FLAGS, tr("Flags value %#x, invalid: %#x"),
                            aFlags, aFlags & ~s_fValidFlags);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    LogFlowThisFuncEnter();

    HRESULT hr = S_OK;

    uint32_t cbWritten;
    int      rcGuest = VERR_IPE_UNINITIALIZED_STATUS;
    uint32_t cbData  = (uint32_t)aData.size();
    void    *pvData  = cbData > 0 ? (void *)&aData.front() : NULL;
    int vrc = i_writeData(aHandle, aFlags, pvData, cbData, aTimeoutMS, &cbWritten, &rcGuest);
    if (RT_FAILURE(vrc))
    {
        switch (vrc)
        {
            case VERR_GSTCTL_GUEST_ERROR:
                hr = setErrorExternal(this, Utf8StrFmt("Writing %RU32 bytes (flags %#x) to guest process failed", cbData, aFlags),
                                      GuestErrorInfo(GuestErrorInfo::Type_Process, rcGuest, mData.mProcess.mExecutable.c_str()));
                break;

            default:
                hr = setErrorBoth(VBOX_E_IPRT_ERROR, vrc, tr("Writing to guest process \"%s\" (PID %RU32) failed: %Rrc"),
                                  mData.mProcess.mExecutable.c_str(), mData.mPID, vrc);
                break;
        }
    }

    LogFlowThisFunc(("rc=%Rrc, aWritten=%RU32\n", vrc, cbWritten));

    *aWritten = (ULONG)cbWritten;

    LogFlowFuncLeaveRC(vrc);
    return hr;
}

HRESULT GuestProcess::writeArray(ULONG aHandle, const std::vector<ProcessInputFlag_T> &aFlags,
                                 const std::vector<BYTE> &aData, ULONG aTimeoutMS, ULONG *aWritten)
{
    LogFlowThisFuncEnter();

    ULONG fWrite = ProcessInputFlag_None;
    for (size_t i = 0; i < aFlags.size(); i++)
        fWrite |= aFlags[i];

    return write(aHandle, fWrite, aData, aTimeoutMS, aWritten);
}

///////////////////////////////////////////////////////////////////////////////

GuestProcessTool::GuestProcessTool(void)
    : pSession(NULL),
      pProcess(NULL)
{
}

GuestProcessTool::~GuestProcessTool(void)
{
    uninit();
}

int GuestProcessTool::init(GuestSession *pGuestSession, const GuestProcessStartupInfo &startupInfo,
                           bool fAsync, int *prcGuest)
{
    LogFlowThisFunc(("pGuestSession=%p, exe=%s, fAsync=%RTbool\n",
                     pGuestSession, startupInfo.mExecutable.c_str(), fAsync));

    AssertPtrReturn(pGuestSession, VERR_INVALID_POINTER);
    Assert(startupInfo.mArguments[0] == startupInfo.mExecutable);

    pSession = pGuestSession;
    mStartupInfo = startupInfo;

    /* Make sure the process is hidden. */
    mStartupInfo.mFlags |= ProcessCreateFlag_Hidden;

    int vrc = pSession->i_processCreateEx(mStartupInfo, pProcess);
    if (RT_SUCCESS(vrc))
    {
        int vrcGuest = VINF_SUCCESS;
        vrc = fAsync
            ? pProcess->i_startProcessAsync()
            : pProcess->i_startProcess(30 * 1000 /* 30s timeout */, &vrcGuest);

        if (   RT_SUCCESS(vrc)
            && !fAsync
            && RT_FAILURE(vrcGuest)
           )
        {
            vrc = VERR_GSTCTL_GUEST_ERROR;
        }

        if (prcGuest)
            *prcGuest = vrcGuest;
    }

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

void GuestProcessTool::uninit(void)
{
    /* Make sure the process is terminated and unregistered from the guest session. */
    int rcGuestIgnored;
    terminate(30 * 1000 /* 30s timeout */, &rcGuestIgnored);

    /* Unregister the process from the process (and the session's object) list. */
    if (   pSession
        && pProcess)
        pSession->i_processUnregister(pProcess);

    /* Release references. */
    pProcess.setNull();
    pSession.setNull();
}

int GuestProcessTool::getCurrentBlock(uint32_t uHandle, GuestProcessStreamBlock &strmBlock)
{
    const GuestProcessStream *pStream = NULL;
    if (uHandle == OUTPUT_HANDLE_ID_STDOUT)
        pStream = &mStdOut;
    else if (uHandle == OUTPUT_HANDLE_ID_STDERR)
        pStream = &mStdErr;

    if (!pStream)
        return VERR_INVALID_PARAMETER;

    int vrc;
    do
    {
        /* Try parsing the data to see if the current block is complete. */
        vrc = mStdOut.ParseBlock(strmBlock);
        if (strmBlock.GetCount())
            break;
    } while (RT_SUCCESS(vrc));

    LogFlowThisFunc(("rc=%Rrc, %RU64 pairs\n",
                      vrc, strmBlock.GetCount()));
    return vrc;
}

int GuestProcessTool::getRc(void) const
{
    LONG exitCode = -1;
    HRESULT hr = pProcess->COMGETTER(ExitCode(&exitCode));
    AssertComRC(hr);

    return GuestProcessTool::exitCodeToRc(mStartupInfo, exitCode);
}

bool GuestProcessTool::isRunning(void)
{
    AssertReturn(!pProcess.isNull(), false);

    ProcessStatus_T procStatus = ProcessStatus_Undefined;
    HRESULT hr = pProcess->COMGETTER(Status(&procStatus));
    AssertComRC(hr);

    if (   procStatus == ProcessStatus_Started
        || procStatus == ProcessStatus_Paused
        || procStatus == ProcessStatus_Terminating)
    {
        return true;
    }

    return false;
}

/**
 * Returns whether the tool has been run correctly or not, based on it's internal process
 * status and reported exit status.
 *
 * @return @c true if the tool has been run correctly (exit status 0), or @c false if some error
 *         occurred (exit status <> 0 or wrong process state).
 */
bool GuestProcessTool::isTerminatedOk(void)
{
    return getTerminationStatus() == VINF_SUCCESS ? true : false;
}

/**
 * Static helper function to start and wait for a certain toolbox tool.
 *
 * This function most likely is the one you want to use in the first place if you
 * want to just use a toolbox tool and wait for its result. See runEx() if you also
 * needs its output.
 *
 * @return  VBox status code.
 * @param   pGuestSession           Guest control session to use for starting the toolbox tool in.
 * @param   startupInfo             Startup information about the toolbox tool.
 * @param   prcGuest                Where to store the toolbox tool's specific error code in case
 *                                  VERR_GSTCTL_GUEST_ERROR is returned.
 */
/* static */
int GuestProcessTool::run(      GuestSession              *pGuestSession,
                          const GuestProcessStartupInfo   &startupInfo,
                                int                       *prcGuest /* = NULL */)
{
    int rcGuest = VERR_IPE_UNINITIALIZED_STATUS;

    GuestProcessToolErrorInfo errorInfo = { VERR_IPE_UNINITIALIZED_STATUS, INT32_MAX };
    int vrc = runErrorInfo(pGuestSession, startupInfo, errorInfo);
    if (RT_SUCCESS(vrc))
    {
        /* Make sure to check the error information we got from the guest tool. */
        if (GuestProcess::i_isGuestError(errorInfo.rcGuest))
        {
            if (errorInfo.rcGuest == VERR_GSTCTL_PROCESS_EXIT_CODE) /* Translate exit code to a meaningful error code. */
                rcGuest = GuestProcessTool::exitCodeToRc(startupInfo, errorInfo.iExitCode);
            else /* At least return something. */
                rcGuest = errorInfo.rcGuest;

            if (prcGuest)
                *prcGuest = rcGuest;

            vrc = VERR_GSTCTL_GUEST_ERROR;
        }
    }

    LogFlowFunc(("Returned rc=%Rrc, rcGuest=%Rrc, iExitCode=%d\n", vrc, errorInfo.rcGuest, errorInfo.iExitCode));
    return vrc;
}

/**
 * Static helper function to start and wait for a certain toolbox tool, returning
 * extended error information from the guest.
 *
 * @return  VBox status code.
 * @param   pGuestSession           Guest control session to use for starting the toolbox tool in.
 * @param   startupInfo             Startup information about the toolbox tool.
 * @param   errorInfo               Error information returned for error handling.
 */
/* static */
int GuestProcessTool::runErrorInfo(      GuestSession              *pGuestSession,
                                   const GuestProcessStartupInfo   &startupInfo,
                                         GuestProcessToolErrorInfo &errorInfo)
{
    return runExErrorInfo(pGuestSession, startupInfo,
                          NULL /* paStrmOutObjects */, 0 /* cStrmOutObjects */, errorInfo);
}

/**
 * Static helper function to start and wait for output of a certain toolbox tool.
 *
 * @return  IPRT status code.
 * @param   pGuestSession           Guest control session to use for starting the toolbox tool in.
 * @param   startupInfo             Startup information about the toolbox tool.
 * @param   paStrmOutObjects        Pointer to stream objects array to use for retrieving the output of the toolbox tool.
 *                                  Optional.
 * @param   cStrmOutObjects         Number of stream objects passed in. Optional.
 * @param   prcGuest                Error code returned from the guest side if VERR_GSTCTL_GUEST_ERROR is returned. Optional.
 */
/* static */
int GuestProcessTool::runEx(      GuestSession              *pGuestSession,
                            const GuestProcessStartupInfo   &startupInfo,
                                  GuestCtrlStreamObjects    *paStrmOutObjects,
                                  uint32_t                   cStrmOutObjects,
                                  int                       *prcGuest /* = NULL */)
{
    int rcGuest = VERR_IPE_UNINITIALIZED_STATUS;

    GuestProcessToolErrorInfo errorInfo = { VERR_IPE_UNINITIALIZED_STATUS, INT32_MAX };
    int vrc = GuestProcessTool::runExErrorInfo(pGuestSession, startupInfo, paStrmOutObjects, cStrmOutObjects, errorInfo);
    if (RT_SUCCESS(vrc))
    {
        /* Make sure to check the error information we got from the guest tool. */
        if (GuestProcess::i_isGuestError(errorInfo.rcGuest))
        {
            if (errorInfo.rcGuest == VERR_GSTCTL_PROCESS_EXIT_CODE) /* Translate exit code to a meaningful error code. */
                rcGuest = GuestProcessTool::exitCodeToRc(startupInfo, errorInfo.iExitCode);
            else /* At least return something. */
                rcGuest = errorInfo.rcGuest;

            if (prcGuest)
                *prcGuest = rcGuest;

            vrc = VERR_GSTCTL_GUEST_ERROR;
        }
    }

    LogFlowFunc(("Returned rc=%Rrc, rcGuest=%Rrc, iExitCode=%d\n", vrc, errorInfo.rcGuest, errorInfo.iExitCode));
    return vrc;
}

/**
 * Static helper function to start and wait for output of a certain toolbox tool.
 *
 * This is the extended version, which addds the possibility of retrieving parsable so-called guest stream
 * objects. Those objects are issued on the guest side as part of VBoxService's toolbox tools (think of a BusyBox-like approach)
 * on stdout and can be used on the host side to retrieve more information about the actual command issued on the guest side.
 *
 * @return  VBox status code.
 * @param   pGuestSession           Guest control session to use for starting the toolbox tool in.
 * @param   startupInfo             Startup information about the toolbox tool.
 * @param   paStrmOutObjects        Pointer to stream objects array to use for retrieving the output of the toolbox tool.
 *                                  Optional.
 * @param   cStrmOutObjects         Number of stream objects passed in. Optional.
 * @param   errorInfo               Error information returned for error handling.
 */
/* static */
int GuestProcessTool::runExErrorInfo(      GuestSession              *pGuestSession,
                                     const GuestProcessStartupInfo   &startupInfo,
                                           GuestCtrlStreamObjects    *paStrmOutObjects,
                                           uint32_t                   cStrmOutObjects,
                                           GuestProcessToolErrorInfo &errorInfo)
{
    AssertPtrReturn(pGuestSession, VERR_INVALID_POINTER);
    /* paStrmOutObjects is optional. */

    /** @todo Check if this is a valid toolbox. */

    GuestProcessTool procTool;
    int vrc = procTool.init(pGuestSession, startupInfo, false /* Async */, &errorInfo.rcGuest);
    if (RT_SUCCESS(vrc))
    {
        while (cStrmOutObjects--)
        {
            try
            {
                GuestProcessStreamBlock strmBlk;
                vrc = procTool.waitEx(  paStrmOutObjects
                                        ? GUESTPROCESSTOOL_WAIT_FLAG_STDOUT_BLOCK
                                        : GUESTPROCESSTOOL_WAIT_FLAG_NONE, &strmBlk, &errorInfo.rcGuest);
                if (paStrmOutObjects)
                    paStrmOutObjects->push_back(strmBlk);
            }
            catch (std::bad_alloc &)
            {
                vrc = VERR_NO_MEMORY;
            }

            if (RT_FAILURE(vrc))
                break;
        }
    }

    if (RT_SUCCESS(vrc))
    {
        /* Make sure the process runs until completion. */
        vrc = procTool.wait(GUESTPROCESSTOOL_WAIT_FLAG_NONE, &errorInfo.rcGuest);
        if (RT_SUCCESS(vrc))
            errorInfo.rcGuest = procTool.getTerminationStatus(&errorInfo.iExitCode);
    }

    LogFlowFunc(("Returned rc=%Rrc, rcGuest=%Rrc, iExitCode=%d\n", vrc, errorInfo.rcGuest, errorInfo.iExitCode));
    return vrc;
}

/**
 * Reports if the tool has been run correctly.
 *
 * @return  Will return VERR_GSTCTL_PROCESS_EXIT_CODE if the tool process returned an exit code <> 0,
 *          VERR_GSTCTL_PROCESS_WRONG_STATE if the tool process is in a wrong state (e.g. still running),
 *          or VINF_SUCCESS otherwise.
 *
 * @param   piExitCode      Exit code of the tool. Optional.
 */
int GuestProcessTool::getTerminationStatus(int32_t *piExitCode /* = NULL */)
{
    Assert(!pProcess.isNull());
    /* pExitCode is optional. */

    int vrc;
    if (!isRunning())
    {
        LONG iExitCode = -1;
        HRESULT hr = pProcess->COMGETTER(ExitCode(&iExitCode));
        AssertComRC(hr);

        if (piExitCode)
            *piExitCode = iExitCode;

        vrc = iExitCode != 0 ? VERR_GSTCTL_PROCESS_EXIT_CODE : VINF_SUCCESS;
    }
    else
        vrc = VERR_GSTCTL_PROCESS_WRONG_STATE;

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

int GuestProcessTool::wait(uint32_t fToolWaitFlags, int *prcGuest)
{
    return waitEx(fToolWaitFlags, NULL /* pStrmBlkOut */, prcGuest);
}

int GuestProcessTool::waitEx(uint32_t fToolWaitFlags, GuestProcessStreamBlock *pStrmBlkOut, int *prcGuest)
{
    LogFlowThisFunc(("fToolWaitFlags=0x%x, pStreamBlock=%p, prcGuest=%p\n", fToolWaitFlags, pStrmBlkOut, prcGuest));

    /* Can we parse the next block without waiting? */
    int vrc;
    if (fToolWaitFlags & GUESTPROCESSTOOL_WAIT_FLAG_STDOUT_BLOCK)
    {
        AssertPtr(pStrmBlkOut);
        vrc = getCurrentBlock(OUTPUT_HANDLE_ID_STDOUT, *pStrmBlkOut);
        if (RT_SUCCESS(vrc))
            return vrc;
        /* else do the waiting below. */
    }

    /* Do the waiting. */
    uint32_t fProcWaitForFlags = ProcessWaitForFlag_Terminate;
    if (mStartupInfo.mFlags & ProcessCreateFlag_WaitForStdOut)
        fProcWaitForFlags |= ProcessWaitForFlag_StdOut;
    if (mStartupInfo.mFlags & ProcessCreateFlag_WaitForStdErr)
        fProcWaitForFlags |= ProcessWaitForFlag_StdErr;

    /** @todo Decrease timeout while running. */
    uint64_t u64StartMS = RTTimeMilliTS();
    uint32_t uTimeoutMS = mStartupInfo.mTimeoutMS;

    int vrcGuest = VINF_SUCCESS;
    bool fDone = false;

    BYTE byBuf[_64K];
    uint32_t cbRead;

    bool fHandleStdOut = false;
    bool fHandleStdErr = false;

    /**
     * Updates the elapsed time and checks if a
     * timeout happened, then breaking out of the loop.
     */
#define UPDATE_AND_CHECK_ELAPSED_TIME()          \
    u64ElapsedMS = RTTimeMilliTS() - u64StartMS; \
    if (   uTimeoutMS   != RT_INDEFINITE_WAIT    \
        && u64ElapsedMS >= uTimeoutMS)           \
    {                                            \
        vrc = VERR_TIMEOUT;                      \
        break;                                   \
    }

    /**
     * Returns the remaining time (in ms).
     */
#define GET_REMAINING_TIME                                     \
      uTimeoutMS == RT_INDEFINITE_WAIT                         \
    ? RT_INDEFINITE_WAIT : uTimeoutMS - (uint32_t)u64ElapsedMS \

    ProcessWaitResult_T waitRes = ProcessWaitResult_None;
    do
    {
        uint64_t u64ElapsedMS;
        UPDATE_AND_CHECK_ELAPSED_TIME();

        vrc = pProcess->i_waitFor(fProcWaitForFlags, GET_REMAINING_TIME, waitRes, &vrcGuest);
        if (RT_FAILURE(vrc))
            break;

        switch (waitRes)
        {
            case ProcessWaitResult_StdIn:
                vrc = VERR_NOT_IMPLEMENTED;
                break;

            case ProcessWaitResult_StdOut:
                fHandleStdOut = true;
                break;

            case ProcessWaitResult_StdErr:
                fHandleStdErr = true;
                break;

            case ProcessWaitResult_WaitFlagNotSupported:
                if (fProcWaitForFlags & ProcessWaitForFlag_StdOut)
                    fHandleStdOut = true;
                if (fProcWaitForFlags & ProcessWaitForFlag_StdErr)
                    fHandleStdErr = true;
                /* Since waiting for stdout / stderr is not supported by the guest,
                 * wait a bit to not hog the CPU too much when polling for data. */
                RTThreadSleep(1); /* Optional, don't check rc. */
                break;

            case ProcessWaitResult_Error:
                vrc = VERR_GSTCTL_GUEST_ERROR;
                break;

            case ProcessWaitResult_Terminate:
                fDone = true;
                break;

            case ProcessWaitResult_Timeout:
                vrc = VERR_TIMEOUT;
                break;

            case ProcessWaitResult_Start:
            case ProcessWaitResult_Status:
                /* Not used here, just skip. */
                break;

            default:
                AssertMsgFailed(("Unhandled process wait result %RU32\n", waitRes));
                break;
        }

        if (RT_FAILURE(vrc))
            break;

        if (fHandleStdOut)
        {
            UPDATE_AND_CHECK_ELAPSED_TIME();

            cbRead = 0;
            vrc = pProcess->i_readData(OUTPUT_HANDLE_ID_STDOUT, sizeof(byBuf),
                                       GET_REMAINING_TIME,
                                       byBuf, sizeof(byBuf),
                                       &cbRead, &vrcGuest);
            if (   RT_FAILURE(vrc)
                || vrc == VWRN_GSTCTL_OBJECTSTATE_CHANGED)
                break;

            if (cbRead)
            {
                LogFlowThisFunc(("Received %RU32 bytes from stdout\n", cbRead));
                vrc = mStdOut.AddData(byBuf, cbRead);

                if (   RT_SUCCESS(vrc)
                    && (fToolWaitFlags & GUESTPROCESSTOOL_WAIT_FLAG_STDOUT_BLOCK))
                {
                    AssertPtr(pStrmBlkOut);
                    vrc = getCurrentBlock(OUTPUT_HANDLE_ID_STDOUT, *pStrmBlkOut);

                    /* When successful, break out of the loop because we're done
                     * with reading the first stream block. */
                    if (RT_SUCCESS(vrc))
                        fDone = true;
                }
            }

            fHandleStdOut = false;
        }

        if (fHandleStdErr)
        {
            UPDATE_AND_CHECK_ELAPSED_TIME();

            cbRead = 0;
            vrc = pProcess->i_readData(OUTPUT_HANDLE_ID_STDERR, sizeof(byBuf),
                                       GET_REMAINING_TIME,
                                       byBuf, sizeof(byBuf),
                                       &cbRead, &vrcGuest);
            if (   RT_FAILURE(vrc)
                || vrc == VWRN_GSTCTL_OBJECTSTATE_CHANGED)
                break;

            if (cbRead)
            {
                LogFlowThisFunc(("Received %RU32 bytes from stderr\n", cbRead));
                vrc = mStdErr.AddData(byBuf, cbRead);
            }

            fHandleStdErr = false;
        }

    } while (!fDone && RT_SUCCESS(vrc));

#undef UPDATE_AND_CHECK_ELAPSED_TIME
#undef GET_REMAINING_TIME

    if (RT_FAILURE(vrcGuest))
        vrc = VERR_GSTCTL_GUEST_ERROR;

    LogFlowThisFunc(("Loop ended with rc=%Rrc, vrcGuest=%Rrc, waitRes=%RU32\n",
                     vrc, vrcGuest, waitRes));
    if (prcGuest)
        *prcGuest = vrcGuest;

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

int GuestProcessTool::terminate(uint32_t uTimeoutMS, int *prcGuest)
{
    LogFlowThisFuncEnter();

    int rc;
    if (!pProcess.isNull())
        rc = pProcess->i_terminateProcess(uTimeoutMS, prcGuest);
    else
        rc = VERR_NOT_FOUND;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Converts a toolbox tool's exit code to an IPRT error code.
 *
 * @return  int         Returned IPRT error for the particular tool.
 * @param   startupInfo Startup info of the toolbox tool to lookup error code for.
 * @param   iExitCode   The toolbox tool's exit code to lookup IPRT error for.
 */
/* static */
int GuestProcessTool::exitCodeToRc(const GuestProcessStartupInfo &startupInfo, int32_t iExitCode)
{
    if (startupInfo.mArguments.size() == 0)
    {
        AssertFailed();
        return VERR_GENERAL_FAILURE; /* Should not happen. */
    }

    return exitCodeToRc(startupInfo.mArguments[0].c_str(), iExitCode);
}

/**
 * Converts a toolbox tool's exit code to an IPRT error code.
 *
 * @return  Returned IPRT error for the particular tool.
 * @param   pszTool     Name of toolbox tool to lookup error code for.
 * @param   iExitCode   The toolbox tool's exit code to lookup IPRT error for.
 */
/* static */
int GuestProcessTool::exitCodeToRc(const char *pszTool, int32_t iExitCode)
{
    AssertPtrReturn(pszTool, VERR_INVALID_POINTER);

    LogFlowFunc(("%s: %d\n", pszTool, iExitCode));

    if (iExitCode == 0) /* No error? Bail out early. */
        return VINF_SUCCESS;

    if (!RTStrICmp(pszTool, VBOXSERVICE_TOOL_CAT))
    {
        switch (iExitCode)
        {
            case VBOXSERVICETOOLBOX_CAT_EXITCODE_ACCESS_DENIED:     return VERR_ACCESS_DENIED;
            case VBOXSERVICETOOLBOX_CAT_EXITCODE_FILE_NOT_FOUND:    return VERR_FILE_NOT_FOUND;
            case VBOXSERVICETOOLBOX_CAT_EXITCODE_PATH_NOT_FOUND:    return VERR_PATH_NOT_FOUND;
            case VBOXSERVICETOOLBOX_CAT_EXITCODE_SHARING_VIOLATION: return VERR_SHARING_VIOLATION;
            case VBOXSERVICETOOLBOX_CAT_EXITCODE_IS_A_DIRECTORY:    return VERR_IS_A_DIRECTORY;
            default:                                                break;
        }
    }
    else if (!RTStrICmp(pszTool, VBOXSERVICE_TOOL_LS))
    {
        switch (iExitCode)
        {
            /** @todo Handle access denied? */
            case RTEXITCODE_FAILURE: return VERR_PATH_NOT_FOUND;
            default:                 break;
        }
    }
    else if (!RTStrICmp(pszTool, VBOXSERVICE_TOOL_STAT))
    {
        switch (iExitCode)
        {
            case VBOXSERVICETOOLBOX_STAT_EXITCODE_ACCESS_DENIED:      return VERR_ACCESS_DENIED;
            case VBOXSERVICETOOLBOX_STAT_EXITCODE_FILE_NOT_FOUND:     return VERR_FILE_NOT_FOUND;
            case VBOXSERVICETOOLBOX_STAT_EXITCODE_PATH_NOT_FOUND:     return VERR_PATH_NOT_FOUND;
            case VBOXSERVICETOOLBOX_STAT_EXITCODE_NET_PATH_NOT_FOUND: return VERR_NET_PATH_NOT_FOUND;
            default:                                                  break;
        }
    }
    else if (!RTStrICmp(pszTool, VBOXSERVICE_TOOL_MKDIR))
    {
        switch (iExitCode)
        {
            case RTEXITCODE_FAILURE: return VERR_CANT_CREATE;
            default:                 break;
        }
    }
    else if (!RTStrICmp(pszTool, VBOXSERVICE_TOOL_MKTEMP))
    {
        switch (iExitCode)
        {
            case RTEXITCODE_FAILURE: return VERR_CANT_CREATE;
            default:                 break;
        }
    }
    else if (!RTStrICmp(pszTool, VBOXSERVICE_TOOL_RM))
    {
        switch (iExitCode)
        {
            case RTEXITCODE_FAILURE: return VERR_FILE_NOT_FOUND;
            /** @todo RTPathRmCmd does not yet distinguish between not found and access denied yet. */
            default:                 break;
        }
    }

    LogFunc(("Warning: Exit code %d not handled for tool '%s', returning VERR_GENERAL_FAILURE\n", iExitCode, pszTool));

    if (iExitCode == RTEXITCODE_SYNTAX)
        return VERR_INTERNAL_ERROR_5;
    return VERR_GENERAL_FAILURE;
}

/* static */
Utf8Str GuestProcessTool::guestErrorToString(const char *pszTool, const GuestErrorInfo& guestErrorInfo)
{
    Utf8Str strErr;

    /** @todo pData->u32Flags: int vs. uint32 -- IPRT errors are *negative* !!! */
    switch (guestErrorInfo.getRc())
    {
        case VERR_ACCESS_DENIED:
            strErr = Utf8StrFmt(Guest::tr("Access to \"%s\" denied"), guestErrorInfo.getWhat().c_str());
            break;

        case VERR_FILE_NOT_FOUND: /* This is the most likely error. */
            RT_FALL_THROUGH();
        case VERR_PATH_NOT_FOUND:
            strErr = Utf8StrFmt(Guest::tr("No such file or directory \"%s\""), guestErrorInfo.getWhat().c_str());
            break;

        case VERR_INVALID_VM_HANDLE:
            strErr = Utf8StrFmt(Guest::tr("VMM device is not available (is the VM running?)"));
            break;

        case VERR_HGCM_SERVICE_NOT_FOUND:
            strErr = Utf8StrFmt(Guest::tr("The guest execution service is not available"));
            break;

        case VERR_BAD_EXE_FORMAT:
            strErr = Utf8StrFmt(Guest::tr("The file \"%s\" is not an executable format"),
                                guestErrorInfo.getWhat().c_str());
            break;

        case VERR_AUTHENTICATION_FAILURE:
            strErr = Utf8StrFmt(Guest::tr("The user \"%s\" was not able to logon"), guestErrorInfo.getWhat().c_str());
            break;

        case VERR_INVALID_NAME:
            strErr = Utf8StrFmt(Guest::tr("The file \"%s\" is an invalid name"), guestErrorInfo.getWhat().c_str());
            break;

        case VERR_TIMEOUT:
            strErr = Utf8StrFmt(Guest::tr("The guest did not respond within time"));
            break;

        case VERR_CANCELLED:
            strErr = Utf8StrFmt(Guest::tr("The execution operation was canceled"));
            break;

        case VERR_GSTCTL_MAX_CID_OBJECTS_REACHED:
            strErr = Utf8StrFmt(Guest::tr("Maximum number of concurrent guest processes has been reached"));
            break;

        case VERR_NOT_FOUND:
            strErr = Utf8StrFmt(Guest::tr("The guest execution service is not ready (yet)"));
            break;

        default:
            strErr = Utf8StrFmt(Guest::tr("Unhandled error %Rrc for \"%s\" occurred for tool \"%s\" on guest -- please file a bug report"),
                                guestErrorInfo.getRc(), guestErrorInfo.getWhat().c_str(), pszTool);
            break;
    }

    return strErr;
}


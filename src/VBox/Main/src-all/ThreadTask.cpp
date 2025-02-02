/* $Id: ThreadTask.cpp 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * Implementation of ThreadTask
 */

/*
 * Copyright (C) 2015-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <iprt/errcore.h>
#include <iprt/thread.h>

#include "VirtualBoxBase.h"
#include "ThreadTask.h"

#define LOG_GROUP LOG_GROUP_MAIN_THREAD_TASK
#include "LoggingNew.h"

/**
 * Starts the task (on separate thread), consuming @a this.
 *
 * The function takes ownership of "this" instance (object instance which calls
 * this function). And the function is responsible for deletion of "this"
 * pointer in all cases.
 *
 * Possible way of usage:
 * @code{.cpp}
        HRESULT hr;
        SomeTaskInheritedFromThreadTask *pTask = NULL;
        try
        {
            pTask = new SomeTaskInheritedFromThreadTask(this);
            if (!pTask->Init()) // some init procedure
                throw E_FAIL;
        }
        catch (...)
        {
            if (pTask);
                delete pTask;
            return E_FAIL;
        }
        return pTask->createThread(); // pTask is always consumed
   @endcode
 *
 * @sa createThreadWithType
 *
 * @note Always consumes @a this!
 */
HRESULT ThreadTask::createThread(void)
{
    return createThreadInternal(RTTHREADTYPE_MAIN_WORKER);
}


/**
 * Same ThreadTask::createThread(), except it takes a thread type parameter.
 *
 * @param   enmType     The thread type.
 *
 * @note Always consumes @a this!
 */
HRESULT ThreadTask::createThreadWithType(RTTHREADTYPE enmType)
{
    return createThreadInternal(enmType);
}


/**
 * Internal worker for ThreadTask::createThread,
 * ThreadTask::createThreadWithType.
 *
 * @note Always consumes @a this!
 */
HRESULT ThreadTask::createThreadInternal(RTTHREADTYPE enmType)
{
    LogThisFunc(("Created \"%s\"\n", m_strTaskName.c_str()));

    mAsync = true;
    int vrc = RTThreadCreate(NULL,
                             taskHandlerThreadProc,
                             (void *)this,
                             0,
                             enmType,
                             0,
                             m_strTaskName.c_str());
    if (RT_SUCCESS(vrc))
        return S_OK;

    mAsync = false;
    delete this;
    return E_FAIL;
}


/**
 * Static method that can get passed to RTThreadCreate to have a
 * thread started for a Task.
 */
/* static */ DECLCALLBACK(int) ThreadTask::taskHandlerThreadProc(RTTHREAD /* thread */, void *pvUser)
{
    if (pvUser == NULL)
        return VERR_INVALID_POINTER; /* nobody cares */

    ThreadTask *pTask = static_cast<ThreadTask *>(pvUser);

    LogFunc(("Started \"%s\"\n", pTask->m_strTaskName.c_str()));

    /*
     *  handler shall catch and process all possible cases as errors and exceptions.
     */
    pTask->handler();

    LogFunc(("Ended \"%s\"\n", pTask->m_strTaskName.c_str()));

    delete pTask;
    return VINF_SUCCESS;
}


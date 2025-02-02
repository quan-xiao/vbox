/* $Id: QIProcess.cpp 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - Qt extensions: QIProcess class implementation.
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

/* GUI includes: */
#include "QIProcess.h"

/* External includes: */
#ifdef VBOX_WS_X11
# include <sys/wait.h>
#endif


QIProcess::QIProcess(QObject *pParent /* = 0 */)
    : QProcess(pParent)
{
}

/* static */
QByteArray QIProcess::singleShot(const QString &strProcessName, int iTimeout /* = 5000 */)
{
    // WORKAROUND:
    // Why is it really needed is because of Qt4.3 bug with QProcess.
    // This bug is about QProcess sometimes (~70%) do not receive
    // notification about process was finished, so this makes
    // 'bool QProcess::waitForFinished (int)' block the GUI thread and
    // never dismissed with 'true' result even if process was really
    // started&finished. So we just waiting for some information
    // on process output and destroy the process with force. Due to
    // QProcess::~QProcess() has the same 'waitForFinished (int)' blocker
    // we have to change process state to QProcess::NotRunning.

    /// @todo Do we still need this?
    QByteArray result;
    QIProcess process;
    process.start(strProcessName);
    bool firstShotReady = process.waitForReadyRead(iTimeout);
    if (firstShotReady)
        result = process.readAllStandardOutput();
    process.setProcessState(QProcess::NotRunning);
#ifdef VBOX_WS_X11
    int iStatus;
    if (process.pid() > 0)
        waitpid(process.pid(), &iStatus, 0);
#endif /* VBOX_WS_X11 */
    return result;
}

/* $Id: QIProcess.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - Qt extensions: QIProcess class declaration.
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

#ifndef FEQT_INCLUDED_SRC_extensions_QIProcess_h
#define FEQT_INCLUDED_SRC_extensions_QIProcess_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QProcess>

/* GUI includes: */
#include "UILibraryDefs.h"

/** QProcess extension for VBox GUI needs. */
class SHARED_LIBRARY_STUFF QIProcess : public QProcess
{
    Q_OBJECT;

    /** Constructs our own file-dialog passing @a pParent to the base-class.
      * Doesn't mean to be used directly, cause this subclass is a bunch of statics. */
    QIProcess(QObject *pParent = 0);

public:

    /** Execute certain script specified by @a strProcessName
      * and wait up to specified @a iTimeout amount of time for responce. */
    static QByteArray singleShot(const QString &strProcessName,
                                 int iTimeout = 5000);
};

#endif /* !FEQT_INCLUDED_SRC_extensions_QIProcess_h */

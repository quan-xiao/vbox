/* $Id: UIDownloaderUserManual.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIDownloaderUserManual class declaration.
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

#ifndef FEQT_INCLUDED_SRC_net_UIDownloaderUserManual_h
#define FEQT_INCLUDED_SRC_net_UIDownloaderUserManual_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIDownloader.h"

/** UIDownloader extension for background user-manual downloading. */
class SHARED_LIBRARY_STUFF UIDownloaderUserManual : public UIDownloader
{
    Q_OBJECT;

signals:

    /** Notifies listeners about downloading finished.
      * @param  strFile  Brings the downloaded file-name. */
    void sigDownloadFinished(const QString &strFile);

public:

    /** Creates downloader instance. */
    static UIDownloaderUserManual *create();
    /** Returns current downloader instance. */
    static UIDownloaderUserManual *current() { return s_pInstance; }

private:

    /** Constructs downloader. */
    UIDownloaderUserManual();
    /** Destructs downloader. */
    ~UIDownloaderUserManual();

    /** Returns description of the current network operation. */
    virtual const QString description() const /* override */;

    /** Asks user for downloading confirmation for passed @a pReply. */
    virtual bool askForDownloadingConfirmation(UINetworkReply *pReply) /* override */;
    /** Handles downloaded object for passed @a pReply. */
    virtual void handleDownloadedObject(UINetworkReply *pReply) /* override */;

    /** Holds the static singleton instance. */
    static UIDownloaderUserManual *s_pInstance;
};

#endif /* !FEQT_INCLUDED_SRC_net_UIDownloaderUserManual_h */


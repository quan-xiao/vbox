/* $Id: UIDownloaderUserManual.cpp 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIDownloaderUserManual class implementation.
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
#include <QDir>
#include <QFile>
#include <QVariant>

/* GUI includes: */
#include "QIFileDialog.h"
#include "UICommon.h"
#include "UIDownloaderUserManual.h"
#include "UIMessageCenter.h"
#include "UIModalWindowManager.h"
#include "UINetworkReply.h"
#include "UIVersion.h"


/* static */
UIDownloaderUserManual* UIDownloaderUserManual::s_pInstance = 0;

/* static */
UIDownloaderUserManual* UIDownloaderUserManual::create()
{
    if (!s_pInstance)
        s_pInstance = new UIDownloaderUserManual;
    return s_pInstance;
}

UIDownloaderUserManual::UIDownloaderUserManual()
{
    /* Prepare instance: */
    if (!s_pInstance)
        s_pInstance = this;

    /* Get version number and adjust it for test and trunk builds. The server only has official releases. */
    const QString strVersion = UIVersion(uiCommon().vboxVersionStringNormalized()).effectiveReleasedVersion().toString();

    /* Compose User Manual filename: */
    QString strUserManualFullFileName = uiCommon().helpFile();
    QString strUserManualShortFileName = QFileInfo(strUserManualFullFileName).fileName();

    /* Add sources: */
    QString strSource1 = QString("https://download.virtualbox.org/virtualbox/%1/").arg(strVersion) + strUserManualShortFileName;
    QString strSource2 = QString("https://download.virtualbox.org/virtualbox/") + strUserManualShortFileName;
    addSource(strSource1);
    addSource(strSource2);

    /* Set target: */
    QString strUserManualDestination = QDir(uiCommon().homeFolder()).absoluteFilePath(strUserManualShortFileName);
    setTarget(strUserManualDestination);
}

UIDownloaderUserManual::~UIDownloaderUserManual()
{
    /* Cleanup instance: */
    if (s_pInstance == this)
        s_pInstance = 0;
}

const QString UIDownloaderUserManual::description() const
{
    return UIDownloader::description().arg(tr("VirtualBox User Manual"));
}

bool UIDownloaderUserManual::askForDownloadingConfirmation(UINetworkReply *pReply)
{
    return msgCenter().confirmDownloadUserManual(source().toString(), pReply->header(UINetworkReply::ContentLengthHeader).toInt());
}

void UIDownloaderUserManual::handleDownloadedObject(UINetworkReply *pReply)
{
    /* Read received data into the buffer: */
    QByteArray receivedData(pReply->readAll());
    /* Serialize that buffer into the file: */
    while (true)
    {
        /* Make sure the file already exists.  If we reached
         * this place, it's already written and checked. */
        QFile file(target());
        bool fSuccess = false;
        /* Check step. Try to open file for reading first. */
        if (file.open(QIODevice::ReadOnly))
            fSuccess = true;
        /* Failsafe step. Try to open file for writing otherwise. */
        if (!fSuccess && file.open(QIODevice::WriteOnly))
        {
            /* Write buffer into the file: */
            file.write(receivedData);
            file.close();
            fSuccess = true;
        }
        /* If the file already exists or was just written: */
        if (fSuccess)
        {
            /* Warn the user about user-manual loaded and saved: */
            msgCenter().warnAboutUserManualDownloaded(source().toString(), QDir::toNativeSeparators(target()));
            /* Warn the listener about user-manual was downloaded: */
            emit sigDownloadFinished(target());
            break;
        }

        /* Warn user about user-manual was downloaded but was NOT saved: */
        msgCenter().cannotSaveUserManual(source().toString(), QDir::toNativeSeparators(target()));

        /* Ask the user for another location for the user-manual file: */
        QString strTarget = QIFileDialog::getExistingDirectory(QFileInfo(target()).absolutePath(),
                                                               windowManager().networkManagerOrMainWindowShown(),
                                                               tr("Select folder to save User Manual to"), true);

        /* Check if user had really set a new target: */
        if (!strTarget.isNull())
            setTarget(QDir(strTarget).absoluteFilePath(QFileInfo(target()).fileName()));
        else
            break;
    }
}


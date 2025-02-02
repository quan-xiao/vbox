/* $Id: UIHelpBrowserDialog.cpp 86764 2020-10-30 10:08:52Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIHelpBrowserDialog class implementation.
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

/* Qt includes: */
#if defined(RT_OS_SOLARIS)
# include <QFontDatabase>
#endif
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>

/* GUI includes: */
#include "UIDesktopWidgetWatchdog.h"
#include "UIExtraDataManager.h"
#include "UIIconPool.h"
#include "UIHelpBrowserDialog.h"
#include "UIHelpBrowserWidget.h"
#include "UICommon.h"
#ifdef VBOX_WS_MAC
# include "VBoxUtils-darwin.h"
#endif


/*********************************************************************************************************************************
*   Class UIHelpBrowserDialogFactory implementation.                                                                             *
*********************************************************************************************************************************/

UIHelpBrowserDialogFactory::UIHelpBrowserDialogFactory(const QString &strHelpFilePath /*  = QString() */)
    :m_strHelpFilePath(strHelpFilePath)
{
}

void UIHelpBrowserDialogFactory::create(QIManagerDialog *&pDialog, QWidget *pCenterWidget)
{
    pDialog = new UIHelpBrowserDialog(pCenterWidget, m_strHelpFilePath);
}


/*********************************************************************************************************************************
*   Class UIHelpBrowserDialog implementation.                                                                                    *
*********************************************************************************************************************************/

UIHelpBrowserDialog::UIHelpBrowserDialog(QWidget *pCenterWidget, const QString &strHelpFilePath)
    : QIWithRetranslateUI<QIManagerDialog>(pCenterWidget)
    , m_strHelpFilePath(strHelpFilePath)
{
}

void UIHelpBrowserDialog::retranslateUi()
{
    setWindowTitle(UIHelpBrowserWidget::tr("User Manual"));

    /* Translate buttons: */
    button(ButtonType_Close)->setText(UIHelpBrowserWidget::tr("Close"));
}

void UIHelpBrowserDialog::configure()
{
    /* Apply window icons: */
    setWindowIcon(UIIconPool::iconSetFull(":/vm_show_logs_32px.png", ":/vm_show_logs_16px.png"));
}

void UIHelpBrowserDialog::configureCentralWidget()
{
    /* Create widget: */
    UIHelpBrowserWidget *pWidget = 0;

#ifdef RT_OS_LINUX
    pWidget = new UIHelpBrowserWidget(EmbedTo_Dialog, m_strHelpFilePath, true /* show toolbar */, this);
#endif

    if (pWidget)
    {
        /* Configure widget: */
        setWidget(pWidget);
        setWidgetMenu(pWidget->menu());
#ifdef VBOX_WS_MAC
        setWidgetToolbar(pWidget->toolbar());
#endif
        connect(pWidget, &UIHelpBrowserWidget::sigSetCloseButtonShortCut,
                this, &UIHelpBrowserDialog::sltSetCloseButtonShortCut);

        /* Add into layout: */
        centralWidget()->layout()->addWidget(pWidget);
    }
}

void UIHelpBrowserDialog::finalize()
{
    /* Apply language settings: */
    retranslateUi();
}

void UIHelpBrowserDialog::loadSettings()
{
    const QRect availableGeo = gpDesktop->availableGeometry(this);
    int iDefaultWidth = availableGeo.width() / 2;
    int iDefaultHeight = availableGeo.height() * 3 / 4;
    QRect defaultGeo(0, 0, iDefaultWidth, iDefaultHeight);

    /* Load geometry from extradata: */
    const QRect geo = gEDataManager->helpBrowserDialogGeometry(this, centerWidget(), defaultGeo);
    LogRel2(("GUI: UIHelpBrowserDialog: Restoring geometry to: Origin=%dx%d, Size=%dx%d\n",
             geo.x(), geo.y(), geo.width(), geo.height()));
    restoreGeometry(geo);
}

void UIHelpBrowserDialog::saveSettings()
{
    const QRect geo = currentGeometry();
    LogRel2(("GUI: UIHelpBrowserDialog: Saving geometry as: Origin=%dx%d, Size=%dx%d\n",
             geo.x(), geo.y(), geo.width(), geo.height()));
    gEDataManager->setHelpBrowserDialogGeometry(geo, isCurrentlyMaximized());
}

bool UIHelpBrowserDialog::shouldBeMaximized() const
{
    return gEDataManager->helpBrowserDialogShouldBeMaximized();
}

void UIHelpBrowserDialog::sltSetCloseButtonShortCut(QKeySequence shortcut)
{
    if (button(ButtonType_Close))
        button(ButtonType_Close)->setShortcut(shortcut);
}

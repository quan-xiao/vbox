/* $Id: UIVirtualBoxManager.cpp 86734 2020-10-28 11:43:13Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIVirtualBoxManager class implementation.
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
#include <QClipboard>
#include <QFile>
#include <QGuiApplication>
#include <QMenuBar>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTextEdit>
#ifndef VBOX_WS_WIN
# include <QRegExp>
#endif
# include <QVBoxLayout>

/* GUI includes: */
#include "QIDialogButtonBox.h"
#include "QIFileDialog.h"
#include "UIActionPoolManager.h"
#include "UICloudConsoleManager.h"
#include "UICloudMachineSettingsDialog.h"
#include "UICloudNetworkingStuff.h"
#include "UICloudProfileManager.h"
#include "UIDesktopServices.h"
#include "UIErrorString.h"
#include "UIExtraDataManager.h"
#include "UIHostNetworkManager.h"
#include "UIIconPool.h"
#include "UIMedium.h"
#include "UIMediumManager.h"
#include "UIMessageCenter.h"
#include "UIModalWindowManager.h"
#include "UIQObjectStuff.h"
#include "UISettingsDialogSpecific.h"
#include "UIVirtualBoxManager.h"
#include "UIVirtualBoxManagerWidget.h"
#include "UIVirtualMachineItemCloud.h"
#include "UIVirtualMachineItemLocal.h"
#include "UIVMLogViewerDialog.h"
#include "UIVirtualBoxEventHandler.h"
#include "UIWizardAddCloudVM.h"
#include "UIWizardCloneVM.h"
#include "UIWizardExportApp.h"
#include "UIWizardImportApp.h"
#include "UIWizardNewCloudVM.h"
#include "UIWizardNewVM.h"
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
# include "UIUpdateManager.h"
#endif
#ifdef VBOX_WS_MAC
# include "UIImageTools.h"
# include "UIWindowMenuManager.h"
# include "VBoxUtils.h"
#else
# include "UIMenuBar.h"
#endif
#ifdef VBOX_WS_X11
# include "UIDesktopWidgetWatchdog.h"
#endif

/* COM includes: */
#include "CSystemProperties.h"
#include "CUnattended.h"
#include "CVirtualBoxErrorInfo.h"
#ifdef VBOX_WS_MAC
# include "CVirtualBox.h"
#endif

/* Other VBox stuff: */
#include <iprt/buildconfig.h>
#include <VBox/version.h>
#ifdef VBOX_WS_X11
# include <iprt/env.h>
#endif /* VBOX_WS_X11 */

#define checkUnattendedInstallError(comUnattendedInstaller) \
    do { \
        if (!comUnattendedInstaller.isOk())      \
        { \
        COMErrorInfo comErrorInfo =  comUnattendedInstaller.errorInfo(); \
        QString strErrorInfo = UIErrorString::formatErrorInfo(comErrorInfo); \
        msgCenter().cannotRunUnattendedGuestInstall(comUnattendedInstaller); \
        return; \
        } \
    } while (0)


/** QDialog extension used to ask for a public key for console connection needs. */
class UIAcquirePublicKeyDialog : public QIWithRetranslateUI<QDialog>
{
    Q_OBJECT;

public:

    /** Constructs dialog passing @a pParent to the base-class. */
    UIAcquirePublicKeyDialog(QWidget *pParent = 0);

    /** Return public key. */
    QString publicKey() const;

private slots:

    /** Handles abstract @a pButton click. */
    void sltHandleButtonClicked(QAbstractButton *pButton);
    /** Handles Open button click. */
    void sltHandleOpenButtonClick();

    /** Performs revalidation. */
    void sltRevalidate();

protected:

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

private:

    /** Prepares all. */
    void prepare();

    /** Loads file contents. */
    void loadFileContents(const QString &strPath, bool fIgnoreErrors = false);

    /** Holds the text-editor instance. */
    QTextEdit         *m_pTextEditor;
    /** Holds the button-box instance. */
    QIDialogButtonBox *m_pButtonBox;
};


/*********************************************************************************************************************************
*   Class UIAcquirePublicKeyDialog implementation.                                                                               *
*********************************************************************************************************************************/

UIAcquirePublicKeyDialog::UIAcquirePublicKeyDialog(QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QDialog>(pParent)
    , m_pTextEditor(0)
    , m_pButtonBox(0)
{
    prepare();
    sltRevalidate();
}

QString UIAcquirePublicKeyDialog::publicKey() const
{
    return m_pTextEditor->toPlainText();
}

void UIAcquirePublicKeyDialog::sltHandleButtonClicked(QAbstractButton *pButton)
{
    const QDialogButtonBox::StandardButton enmStandardButton = m_pButtonBox->standardButton(pButton);
    switch (enmStandardButton)
    {
        case QDialogButtonBox::Ok:     return accept();
        case QDialogButtonBox::Cancel: return reject();
        case QDialogButtonBox::Open:   return sltHandleOpenButtonClick();
        default: break;
    }
}

void UIAcquirePublicKeyDialog::sltHandleOpenButtonClick()
{
    CVirtualBox comVBox = uiCommon().virtualBox();
    const QString strFileName = QIFileDialog::getOpenFileName(comVBox.GetHomeFolder(), QString(),
                                                              this, tr("Choose a public key file"));
    if (!strFileName.isEmpty())
    {
        gEDataManager->setCloudConsolePublicKeyPath(strFileName);
        loadFileContents(strFileName);
    }
}

void UIAcquirePublicKeyDialog::sltRevalidate()
{
    m_pButtonBox->button(QDialogButtonBox::Ok)->setEnabled(!m_pTextEditor->toPlainText().isEmpty());
}

void UIAcquirePublicKeyDialog::retranslateUi()
{
    setWindowTitle(tr("Public key"));
    m_pTextEditor->setPlaceholderText(tr("Paste public key"));
    m_pButtonBox->button(QDialogButtonBox::Open)->setText(tr("Browse"));
}

void UIAcquirePublicKeyDialog::prepare()
{
    /* Prepare layout: */
    QVBoxLayout *pLayout = new QVBoxLayout(this);
    if (pLayout)
    {
        /* Prepare text-editor: */
        m_pTextEditor = new QTextEdit(this);
        if (m_pTextEditor)
        {
            connect(m_pTextEditor, &QTextEdit::textChanged, this, &UIAcquirePublicKeyDialog::sltRevalidate);
            pLayout->addWidget(m_pTextEditor);
        }

        /* Prepare button-box: */
        m_pButtonBox = new QIDialogButtonBox(this);
        if (m_pButtonBox)
        {
            m_pButtonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Open);
            connect(m_pButtonBox, &QIDialogButtonBox::clicked, this, &UIAcquirePublicKeyDialog::sltHandleButtonClicked);
            pLayout->addWidget(m_pButtonBox);
        }
    }

    /* Apply language settings: */
    retranslateUi();

    /* Load last remembered file contents: */
    loadFileContents(gEDataManager->cloudConsolePublicKeyPath(), true /* ignore errors */);

    /* Resize to suitable size: */
    const int iMinimumHeightHint = minimumSizeHint().height();
    resize(iMinimumHeightHint * 2, iMinimumHeightHint);
}

void UIAcquirePublicKeyDialog::loadFileContents(const QString &strPath, bool fIgnoreErrors /* = false */)
{
    if (strPath.isEmpty())
        return;
    QFile file(strPath);
    if (file.open(QIODevice::ReadOnly))
        m_pTextEditor->setPlainText(file.readAll());
    else if (!fIgnoreErrors)
        msgCenter().cannotOpenPublicKeyFile(strPath);
}


/*********************************************************************************************************************************
*   Class UIVirtualBoxManager implementation.                                                                                    *
*********************************************************************************************************************************/

/* static */
UIVirtualBoxManager *UIVirtualBoxManager::s_pInstance = 0;

/* static */
void UIVirtualBoxManager::create()
{
    /* Make sure VirtualBox Manager isn't created: */
    AssertReturnVoid(s_pInstance == 0);

    /* Create VirtualBox Manager: */
    new UIVirtualBoxManager;
    /* Prepare VirtualBox Manager: */
    s_pInstance->prepare();
    /* Show VirtualBox Manager: */
    s_pInstance->show();
    /* Register in the modal window manager: */
    windowManager().setMainWindowShown(s_pInstance);
}

/* static */
void UIVirtualBoxManager::destroy()
{
    /* Make sure VirtualBox Manager is created: */
    AssertPtrReturnVoid(s_pInstance);

    /* Unregister in the modal window manager: */
    windowManager().setMainWindowShown(0);
    /* Cleanup VirtualBox Manager: */
    s_pInstance->cleanup();
    /* Destroy machine UI: */
    delete s_pInstance;
}

UIVirtualBoxManager::UIVirtualBoxManager()
    : m_fPolished(false)
    , m_fFirstMediumEnumerationHandled(false)
    , m_pActionPool(0)
    , m_pManagerVirtualMedia(0)
    , m_pManagerHostNetwork(0)
    , m_pManagerCloudProfile(0)
    , m_pManagerCloudConsole(0)
{
    s_pInstance = this;
}

UIVirtualBoxManager::~UIVirtualBoxManager()
{
    s_pInstance = 0;
}

bool UIVirtualBoxManager::shouldBeMaximized() const
{
    return gEDataManager->selectorWindowShouldBeMaximized();
}

#ifdef VBOX_WS_MAC
bool UIVirtualBoxManager::eventFilter(QObject *pObject, QEvent *pEvent)
{
    /* Ignore for non-active window except for FileOpen event which should be always processed: */
    if (!isActiveWindow() && pEvent->type() != QEvent::FileOpen)
        return QMainWindowWithRestorableGeometryAndRetranslateUi::eventFilter(pObject, pEvent);

    /* Ignore for other objects: */
    if (qobject_cast<QWidget*>(pObject) &&
        qobject_cast<QWidget*>(pObject)->window() != this)
        return QMainWindowWithRestorableGeometryAndRetranslateUi::eventFilter(pObject, pEvent);

    /* Which event do we have? */
    switch (pEvent->type())
    {
        case QEvent::FileOpen:
        {
            sltHandleOpenUrlCall(QList<QUrl>() << static_cast<QFileOpenEvent*>(pEvent)->url());
            pEvent->accept();
            return true;
            break;
        }
        default:
            break;
    }

    /* Call to base-class: */
    return QMainWindowWithRestorableGeometryAndRetranslateUi::eventFilter(pObject, pEvent);
}
#endif /* VBOX_WS_MAC */

void UIVirtualBoxManager::retranslateUi()
{
    /* Set window title: */
    QString strTitle(VBOX_PRODUCT);
    strTitle += " " + tr("Manager", "Note: main window title which is prepended by the product name.");
#ifdef VBOX_BLEEDING_EDGE
    strTitle += QString(" EXPERIMENTAL build ")
             +  QString(RTBldCfgVersion())
             +  QString(" r")
             +  QString(RTBldCfgRevisionStr())
             +  QString(" - " VBOX_BLEEDING_EDGE);
#endif /* VBOX_BLEEDING_EDGE */
    setWindowTitle(strTitle);
}

bool UIVirtualBoxManager::event(QEvent *pEvent)
{
    /* Which event do we have? */
    switch (pEvent->type())
    {
        /* Handle every ScreenChangeInternal event to notify listeners: */
        case QEvent::ScreenChangeInternal:
        {
            emit sigWindowRemapped();
            break;
        }
        default:
            break;
    }
    /* Call to base-class: */
    return QMainWindowWithRestorableGeometryAndRetranslateUi::event(pEvent);
}

void UIVirtualBoxManager::showEvent(QShowEvent *pEvent)
{
    /* Call to base-class: */
    QMainWindowWithRestorableGeometryAndRetranslateUi::showEvent(pEvent);

    /* Is polishing required? */
    if (!m_fPolished)
    {
        /* Pass the show-event to polish-event: */
        polishEvent(pEvent);
        /* Mark as polished: */
        m_fPolished = true;
    }
}

void UIVirtualBoxManager::polishEvent(QShowEvent *)
{
    /* Make sure user warned about inaccessible media: */
    QMetaObject::invokeMethod(this, "sltHandleMediumEnumerationFinish", Qt::QueuedConnection);
}

void UIVirtualBoxManager::closeEvent(QCloseEvent *pEvent)
{
    /* Call to base-class: */
    QMainWindowWithRestorableGeometryAndRetranslateUi::closeEvent(pEvent);

    /* Quit application: */
    QApplication::quit();
}

#ifdef VBOX_WS_X11
void UIVirtualBoxManager::sltHandleHostScreenAvailableAreaChange()
{
    /* Prevent handling if fake screen detected: */
    if (gpDesktop->isFakeScreenDetected())
        return;

    /* Restore the geometry cached by the window: */
    const QRect geo = currentGeometry();
    resize(geo.size());
    move(geo.topLeft());
}
#endif /* VBOX_WS_X11 */

void UIVirtualBoxManager::sltHandleMediumEnumerationFinish()
{
#if 0 // ohh, come on!
    /* To avoid annoying the user, we check for inaccessible media just once, after
     * the first media emumeration [started from main() at startup] is complete. */
    if (m_fFirstMediumEnumerationHandled)
        return;
    m_fFirstMediumEnumerationHandled = true;

    /* Make sure MM window/tool is not opened,
     * otherwise user sees everything himself: */
    if (   m_pManagerVirtualMedia
        || m_pWidget->isGlobalToolOpened(UIToolType_Media))
        return;

    /* Look for at least one inaccessible medium: */
    bool fIsThereAnyInaccessibleMedium = false;
    foreach (const QUuid &uMediumID, uiCommon().mediumIDs())
    {
        if (uiCommon().medium(uMediumID).state() == KMediumState_Inaccessible)
        {
            fIsThereAnyInaccessibleMedium = true;
            break;
        }
    }
    /* Warn the user about inaccessible medium, propose to open MM window/tool: */
    if (fIsThereAnyInaccessibleMedium && msgCenter().warnAboutInaccessibleMedia())
    {
        /* Open the MM window: */
        sltOpenVirtualMediumManagerWindow();
    }
#endif
}

void UIVirtualBoxManager::sltHandleOpenUrlCall(QList<QUrl> list /* = QList<QUrl>() */)
{
    /* If passed list is empty, we take the one from UICommon: */
    if (list.isEmpty())
        list = uiCommon().takeArgumentUrls();

    /* Check if we are can handle the dropped urls: */
    for (int i = 0; i < list.size(); ++i)
    {
#ifdef VBOX_WS_MAC
        const QString strFile = ::darwinResolveAlias(list.at(i).toLocalFile());
#else
        const QString strFile = list.at(i).toLocalFile();
#endif
        /* If there is such file exists: */
        if (!strFile.isEmpty() && QFile::exists(strFile))
        {
            /* And has allowed VBox config file extension: */
            if (UICommon::hasAllowedExtension(strFile, VBoxFileExts))
            {
                /* Handle VBox config file: */
                CVirtualBox comVBox = uiCommon().virtualBox();
                CMachine comMachine = comVBox.FindMachine(strFile);
                if (comVBox.isOk() && comMachine.isNotNull())
                    uiCommon().launchMachine(comMachine);
                else
                    openAddMachineDialog(strFile);
            }
            /* And has allowed VBox OVF file extension: */
            else if (UICommon::hasAllowedExtension(strFile, OVFFileExts))
            {
                /* Allow only one file at the time: */
                sltOpenImportApplianceWizard(strFile);
                break;
            }
            /* And has allowed VBox extension pack file extension: */
            else if (UICommon::hasAllowedExtension(strFile, VBoxExtPackFileExts))
            {
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
                /* Prevent update manager from proposing us to update EP: */
                gUpdateManager->setEPInstallationRequested(true);
#endif
                /* Propose the user to install EP described by the arguments @a list. */
                uiCommon().doExtPackInstallation(strFile, QString(), this, NULL);
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
                /* Allow update manager to propose us to update EP: */
                gUpdateManager->setEPInstallationRequested(false);
#endif
            }
        }
    }
}

void UIVirtualBoxManager::sltHandleChooserPaneIndexChange()
{
    updateActionsVisibility();
    updateActionsAppearance();
}

void UIVirtualBoxManager::sltHandleGroupSavingProgressChange()
{
    updateActionsAppearance();
}

void UIVirtualBoxManager::sltHandleCloudUpdateProgressChange()
{
    updateActionsAppearance();
}

void UIVirtualBoxManager::sltHandleToolTypeChange()
{
    updateActionsVisibility();
    updateActionsAppearance();

    /* Make sure separate dialogs are closed when corresponding tools are opened: */
    switch (m_pWidget->toolsType())
    {
        case UIToolType_Media:       sltCloseVirtualMediumManagerWindow(); break;
        case UIToolType_Network:     sltCloseHostNetworkManagerWindow(); break;
        case UIToolType_Cloud:       sltCloseCloudProfileManagerWindow(); break;
        case UIToolType_Logs:        sltCloseLogViewerWindow(); break;
        case UIToolType_Performance: sltClosePerformanceMonitorWindow(); break;
        default: break;
    }
}

void UIVirtualBoxManager::sltCurrentSnapshotItemChange()
{
    updateActionsAppearance();
}

void UIVirtualBoxManager::sltHandleCloudMachineStateChange(const QUuid & /* uId */)
{
    updateActionsAppearance();
}

void UIVirtualBoxManager::sltHandleStateChange(const QUuid &)
{
    updateActionsAppearance();
}

void UIVirtualBoxManager::sltHandleMenuPrepare(int iIndex, QMenu *pMenu)
{
    /* Update if there is update-handler: */
    if (m_menuUpdateHandlers.contains(iIndex))
        (this->*(m_menuUpdateHandlers.value(iIndex)))(pMenu);
}

void UIVirtualBoxManager::sltOpenVirtualMediumManagerWindow()
{
    /* First check if instance of widget opened the embedded way: */
    if (m_pWidget->isGlobalToolOpened(UIToolType_Media))
    {
        m_pWidget->setToolsType(UIToolType_Welcome);
        m_pWidget->closeGlobalTool(UIToolType_Media);
    }

    /* Create instance if not yet created: */
    if (!m_pManagerVirtualMedia)
    {
        UIMediumManagerFactory(m_pActionPool).prepare(m_pManagerVirtualMedia, this);
        connect(m_pManagerVirtualMedia, &QIManagerDialog::sigClose,
                this, &UIVirtualBoxManager::sltCloseVirtualMediumManagerWindow);
    }

    /* Show instance: */
    m_pManagerVirtualMedia->show();
    m_pManagerVirtualMedia->setWindowState(m_pManagerVirtualMedia->windowState() & ~Qt::WindowMinimized);
    m_pManagerVirtualMedia->activateWindow();
}

void UIVirtualBoxManager::sltCloseVirtualMediumManagerWindow()
{
    /* Destroy instance if still exists: */
    if (m_pManagerVirtualMedia)
        UIMediumManagerFactory().cleanup(m_pManagerVirtualMedia);
}

void UIVirtualBoxManager::sltOpenHostNetworkManagerWindow()
{
    /* First check if instance of widget opened the embedded way: */
    if (m_pWidget->isGlobalToolOpened(UIToolType_Network))
    {
        m_pWidget->setToolsType(UIToolType_Welcome);
        m_pWidget->closeGlobalTool(UIToolType_Network);
    }

    /* Create instance if not yet created: */
    if (!m_pManagerHostNetwork)
    {
        UIHostNetworkManagerFactory(m_pActionPool).prepare(m_pManagerHostNetwork, this);
        connect(m_pManagerHostNetwork, &QIManagerDialog::sigClose,
                this, &UIVirtualBoxManager::sltCloseHostNetworkManagerWindow);
    }

    /* Show instance: */
    m_pManagerHostNetwork->show();
    m_pManagerHostNetwork->setWindowState(m_pManagerHostNetwork->windowState() & ~Qt::WindowMinimized);
    m_pManagerHostNetwork->activateWindow();
}

void UIVirtualBoxManager::sltCloseHostNetworkManagerWindow()
{
    /* Destroy instance if still exists: */
    if (m_pManagerHostNetwork)
        UIHostNetworkManagerFactory().cleanup(m_pManagerHostNetwork);
}

void UIVirtualBoxManager::sltOpenCloudProfileManagerWindow()
{
    /* First check if instance of widget opened the embedded way: */
    if (m_pWidget->isGlobalToolOpened(UIToolType_Cloud))
    {
        m_pWidget->setToolsType(UIToolType_Welcome);
        m_pWidget->closeGlobalTool(UIToolType_Cloud);
    }

    /* Create instance if not yet created: */
    if (!m_pManagerCloudProfile)
    {
        UICloudProfileManagerFactory(m_pActionPool).prepare(m_pManagerCloudProfile, this);
        connect(m_pManagerCloudProfile, &QIManagerDialog::sigClose,
                this, &UIVirtualBoxManager::sltCloseCloudProfileManagerWindow);
    }

    /* Show instance: */
    m_pManagerCloudProfile->show();
    m_pManagerCloudProfile->setWindowState(m_pManagerCloudProfile->windowState() & ~Qt::WindowMinimized);
    m_pManagerCloudProfile->activateWindow();
}

void UIVirtualBoxManager::sltCloseCloudProfileManagerWindow()
{
    /* Destroy instance if still exists: */
    if (m_pManagerCloudProfile)
        UIHostNetworkManagerFactory().cleanup(m_pManagerCloudProfile);
}

void UIVirtualBoxManager::sltOpenCloudConsoleManagerWindow()
{
    /* Create instance if not yet created: */
    if (!m_pManagerCloudConsole)
    {
        UICloudConsoleManagerFactory(m_pActionPool).prepare(m_pManagerCloudConsole, this);
        connect(m_pManagerCloudConsole, &QIManagerDialog::sigClose,
                this, &UIVirtualBoxManager::sltCloseCloudConsoleManagerWindow);
    }

    /* Show instance: */
    m_pManagerCloudConsole->show();
    m_pManagerCloudConsole->setWindowState(m_pManagerCloudConsole->windowState() & ~Qt::WindowMinimized);
    m_pManagerCloudConsole->activateWindow();
}

void UIVirtualBoxManager::sltCloseCloudConsoleManagerWindow()
{
    /* Destroy instance if still exists: */
    if (m_pManagerCloudConsole)
        UIHostNetworkManagerFactory().cleanup(m_pManagerCloudConsole);
}

void UIVirtualBoxManager::sltOpenImportApplianceWizard(const QString &strFileName /* = QString() */)
{
    /* Initialize variables: */
#ifdef VBOX_WS_MAC
    const QString strTmpFile = ::darwinResolveAlias(strFileName);
#else
    const QString strTmpFile = strFileName;
#endif

    /* Lock the action preventing cascade calls: */
    UIQObjectPropertySetter guardBlock(actionPool()->action(UIActionIndexMN_M_File_S_ImportAppliance), "opened", true);
    connect(&guardBlock, &UIQObjectPropertySetter::sigAboutToBeDestroyed,
            this, &UIVirtualBoxManager::sltHandleUpdateActionAppearanceRequest);
    updateActionsAppearance();

    /* Use the "safe way" to open stack of Mac OS X Sheets: */
    QWidget *pWizardParent = windowManager().realParentWindow(this);
    UISafePointerWizardImportApp pWizard = new UIWizardImportApp(pWizardParent, false /* OCI by default? */, strTmpFile);
    windowManager().registerNewParent(pWizard, pWizardParent);
    pWizard->prepare();
    if (strFileName.isEmpty() || pWizard->isValid())
        pWizard->exec();
    delete pWizard;
}

void UIVirtualBoxManager::sltOpenExportApplianceWizard()
{
    /* Get selected items: */
    QList<UIVirtualMachineItem*> items = currentItems();

    /* Populate the list of VM names: */
    QStringList names;
    for (int i = 0; i < items.size(); ++i)
        names << items.at(i)->name();

    /* Lock the actions preventing cascade calls: */
    UIQObjectPropertySetter guardBlock(QList<QObject*>() << actionPool()->action(UIActionIndexMN_M_File_S_ExportAppliance)
                                                         << actionPool()->action(UIActionIndexMN_M_Machine_S_ExportToOCI),
                                       "opened", true);
    connect(&guardBlock, &UIQObjectPropertySetter::sigAboutToBeDestroyed,
            this, &UIVirtualBoxManager::sltHandleUpdateActionAppearanceRequest);
    updateActionsAppearance();

    /* Check what was the action invoked us: */
    UIAction *pAction = qobject_cast<UIAction*>(sender());

    /* Use the "safe way" to open stack of Mac OS X Sheets: */
    QWidget *pWizardParent = windowManager().realParentWindow(this);
    UISafePointerWizard pWizard = new UIWizardExportApp(pWizardParent, names,
                                                        pAction &&
                                                        pAction == actionPool()->action(UIActionIndexMN_M_Machine_S_ExportToOCI));
    windowManager().registerNewParent(pWizard, pWizardParent);
    pWizard->prepare();
    pWizard->exec();
    delete pWizard;
}

#ifdef VBOX_GUI_WITH_EXTRADATA_MANAGER_UI
void UIVirtualBoxManager::sltOpenExtraDataManagerWindow()
{
    gEDataManager->openWindow(this);
}
#endif /* VBOX_GUI_WITH_EXTRADATA_MANAGER_UI */

void UIVirtualBoxManager::sltOpenPreferencesDialog()
{
    /* Don't show the inaccessible warning
     * if the user tries to open global settings: */
    m_fFirstMediumEnumerationHandled = true;

    /* Lock the action preventing cascade calls: */
    UIQObjectPropertySetter guardBlock(actionPool()->action(UIActionIndex_M_Application_S_Preferences), "opened", true);
    connect(&guardBlock, &UIQObjectPropertySetter::sigAboutToBeDestroyed,
            this, &UIVirtualBoxManager::sltHandleUpdateActionAppearanceRequest);
    updateActionsAppearance();

    /* Use the "safe way" to open stack of Mac OS X Sheets: */
    QWidget *pDialogParent = windowManager().realParentWindow(this);
    UISafePointerSettingsDialogGlobal pDialog = new UISettingsDialogGlobal(pDialogParent);
    windowManager().registerNewParent(pDialog, pDialogParent);

    /* Execute dialog: */
    pDialog->execute();
    delete pDialog;
}

void UIVirtualBoxManager::sltPerformExit()
{
    close();
}

void UIVirtualBoxManager::sltOpenNewMachineWizard()
{
    /* Lock the actions preventing cascade calls: */
    UIQObjectPropertySetter guardBlock(QList<QObject*>() << actionPool()->action(UIActionIndexMN_M_Welcome_S_New)
                                                         << actionPool()->action(UIActionIndexMN_M_Machine_S_New)
                                                         << actionPool()->action(UIActionIndexMN_M_Group_S_New),
                                       "opened", true);
    connect(&guardBlock, &UIQObjectPropertySetter::sigAboutToBeDestroyed,
            this, &UIVirtualBoxManager::sltHandleUpdateActionAppearanceRequest);
    updateActionsAppearance();

    /* Get first selected item: */
    UIVirtualMachineItem *pItem = currentItem();

    /* For global item or local machine: */
    if (   !pItem
        || pItem->itemType() == UIVirtualMachineItemType_Local)
    {
        /* Use the "safe way" to open stack of Mac OS X Sheets: */
        QWidget *pWizardParent = windowManager().realParentWindow(this);
        UISafePointerWizardNewVM pWizard = new UIWizardNewVM(pWizardParent, m_pWidget->fullGroupName());
        windowManager().registerNewParent(pWizard, pWizardParent);
        pWizard->prepare();

        CUnattended comUnattendedInstaller = uiCommon().virtualBox().CreateUnattendedInstaller();
        AssertMsg(!comUnattendedInstaller.isNull(), ("Could not create unattended installer!\n"));

        UIUnattendedInstallData unattendedInstallData;
        unattendedInstallData.m_strUserName = comUnattendedInstaller.GetUser();
        unattendedInstallData.m_strPassword = comUnattendedInstaller.GetPassword();
        unattendedInstallData.m_strHostname = comUnattendedInstaller.GetHostname();
        unattendedInstallData.m_fInstallGuestAdditions = comUnattendedInstaller.GetInstallGuestAdditions();
        unattendedInstallData.m_strGuestAdditionsISOPath = comUnattendedInstaller.GetAdditionsIsoPath();
        pWizard->setDefaultUnattendedInstallData(unattendedInstallData);

        /* Execute wizard: */
        pWizard->exec();

        /* Cache unattended install related info and delete the wizard before handling the unattended install stuff: */
        unattendedInstallData = pWizard->unattendedInstallData();

        delete pWizard;
        /* Handle unattended install stuff: */
        if (unattendedInstallData.m_fUnattendedEnabled)
            startUnattendedInstall(comUnattendedInstaller, unattendedInstallData);
    }
    /* For cloud machine: */
    else
    {
        /* Use the "safe way" to open stack of Mac OS X Sheets: */
        QWidget *pWizardParent = windowManager().realParentWindow(this);
        UISafePointerWizardNewCloudVM pWizard = new UIWizardNewCloudVM(pWizardParent, m_pWidget->fullGroupName());
        windowManager().registerNewParent(pWizard, pWizardParent);
        pWizard->prepare();

        /* Execute wizard: */
        pWizard->exec();
        delete pWizard;
    }
}

void UIVirtualBoxManager::sltOpenAddMachineDialog()
{
    /* Lock the actions preventing cascade calls: */
    UIQObjectPropertySetter guardBlock(QList<QObject*>() << actionPool()->action(UIActionIndexMN_M_Welcome_S_Add)
                                                         << actionPool()->action(UIActionIndexMN_M_Machine_S_Add)
                                                         << actionPool()->action(UIActionIndexMN_M_Group_S_Add),
                                       "opened", true);
    connect(&guardBlock, &UIQObjectPropertySetter::sigAboutToBeDestroyed,
            this, &UIVirtualBoxManager::sltHandleUpdateActionAppearanceRequest);
    updateActionsAppearance();

    /* Get first selected item: */
    UIVirtualMachineItem *pItem = currentItem();

    /* For global item or local machine: */
    if (   !pItem
        || pItem->itemType() == UIVirtualMachineItemType_Local)
    {
        /* Open add machine dialog: */
        openAddMachineDialog();
    }
    /* For cloud machine: */
    else
    {
        /* Use the "safe way" to open stack of Mac OS X Sheets: */
        QWidget *pWizardParent = windowManager().realParentWindow(this);
        UISafePointerWizardAddCloudVM pWizard = new UIWizardAddCloudVM(pWizardParent, m_pWidget->fullGroupName());
        windowManager().registerNewParent(pWizard, pWizardParent);
        pWizard->prepare();

        /* Execute wizard: */
        pWizard->exec();
        delete pWizard;
    }
}

void UIVirtualBoxManager::sltOpenGroupNameEditor()
{
    m_pWidget->openGroupNameEditor();
}

void UIVirtualBoxManager::sltDisbandGroup()
{
    m_pWidget->disbandGroup();
}

void UIVirtualBoxManager::sltOpenMachineSettingsDialog(QString strCategory /* = QString() */,
                                                       QString strControl /* = QString() */,
                                                       const QUuid &uID /* = QString() */)
{
    /* Lock the action preventing cascade calls: */
    UIQObjectPropertySetter guardBlock(actionPool()->action(UIActionIndexMN_M_Machine_S_Settings), "opened", true);
    connect(&guardBlock, &UIQObjectPropertySetter::sigAboutToBeDestroyed,
            this, &UIVirtualBoxManager::sltHandleUpdateActionAppearanceRequest);
    updateActionsAppearance();

    /* Get current item: */
    UIVirtualMachineItem *pItem = currentItem();
    AssertMsgReturnVoid(pItem, ("Current item should be selected!\n"));

    /* For local machine: */
    if (pItem->itemType() == UIVirtualMachineItemType_Local)
    {
        /* Process href from VM details / description: */
        if (!strCategory.isEmpty() && strCategory[0] != '#')
        {
            uiCommon().openURL(strCategory);
        }
        else
        {
            /* Check if control is coded into the URL by %%: */
            if (strControl.isEmpty())
            {
                QStringList parts = strCategory.split("%%");
                if (parts.size() == 2)
                {
                    strCategory = parts.at(0);
                    strControl = parts.at(1);
                }
            }

            /* Don't show the inaccessible warning
             * if the user tries to open VM settings: */
            m_fFirstMediumEnumerationHandled = true;

            /* Use the "safe way" to open stack of Mac OS X Sheets: */
            QWidget *pDialogParent = windowManager().realParentWindow(this);
            UISafePointerSettingsDialogMachine pDialog = new UISettingsDialogMachine(pDialogParent,
                                                                                     uID.isNull() ? pItem->id() : uID,
                                                                                     strCategory, strControl);
            windowManager().registerNewParent(pDialog, pDialogParent);

            /* Execute dialog: */
            pDialog->execute();
            delete pDialog;
        }
    }
    /* For cloud machine: */
    else
    {
        /* Use the "safe way" to open stack of Mac OS X Sheets: */
        QWidget *pDialogParent = windowManager().realParentWindow(this);
        UISafePointerCloudMachineSettingsDialog pDialog = new UICloudMachineSettingsDialog(pDialogParent,
                                                                                           pItem->toCloud()->machine());
        windowManager().registerNewParent(pDialog, pDialogParent);

        /* Execute dialog: */
        pDialog->exec();
        delete pDialog;
    }
}

void UIVirtualBoxManager::sltOpenCloneMachineWizard()
{
    /* Get current item: */
    UIVirtualMachineItem *pItem = currentItem();
    AssertMsgReturnVoid(pItem, ("Current item should be selected!\n"));
    /* Make sure current item is local one: */
    UIVirtualMachineItemLocal *pItemLocal = pItem->toLocal();
    AssertMsgReturnVoid(pItemLocal, ("Current item should be local one!\n"));

    /* Use the "safe way" to open stack of Mac OS X Sheets: */
    QWidget *pWizardParent = windowManager().realParentWindow(this);
    const QStringList &machineGroupNames = pItemLocal->groups();
    const QString strGroup = !machineGroupNames.isEmpty() ? machineGroupNames.at(0) : QString();
    UISafePointerWizard pWizard = new UIWizardCloneVM(pWizardParent, pItemLocal->machine(), strGroup);
    windowManager().registerNewParent(pWizard, pWizardParent);
    pWizard->prepare();
    pWizard->exec();
    delete pWizard;
}

void UIVirtualBoxManager::sltPerformMachineMove()
{
    /* Get current item: */
    UIVirtualMachineItem *pItem = currentItem();
    AssertMsgReturnVoid(pItem, ("Current item should be selected!\n"));

    /* Open a session thru which we will modify the machine: */
    CSession comSession = uiCommon().openSession(pItem->id(), KLockType_Write);
    if (comSession.isNull())
        return;

    /* Get session machine: */
    CMachine comMachine = comSession.GetMachine();
    AssertMsgReturnVoid(comSession.isOk() && comMachine.isNotNull(), ("Unable to acquire machine!\n"));

    /* Open a file dialog for the user to select a destination folder. Start with the default machine folder: */
    CVirtualBox comVBox = uiCommon().virtualBox();
    QString strBaseFolder = comVBox.GetSystemProperties().GetDefaultMachineFolder();
    QString strTitle = tr("Select a destination folder to move the selected virtual machine");
    QString strDestinationFolder = QIFileDialog::getExistingDirectory(strBaseFolder, this, strTitle);
    if (!strDestinationFolder.isEmpty())
    {
        /* Prepare machine move progress: */
        CProgress comProgress = comMachine.MoveTo(strDestinationFolder, "basic");
        if (comMachine.isOk() && comProgress.isNotNull())
        {
            /* Show machine move progress: */
            msgCenter().showModalProgressDialog(comProgress, comMachine.GetName(), ":/progress_dnd_hg_90px.png");
            if (!comProgress.isOk() || comProgress.GetResultCode() != 0)
                msgCenter().cannotMoveMachine(comProgress, comMachine.GetName());
        }
        else
            msgCenter().cannotMoveMachine(comMachine);
    }
    comSession.UnlockMachine();
}

void UIVirtualBoxManager::sltPerformMachineRemove()
{
    m_pWidget->removeMachine();
}

void UIVirtualBoxManager::sltPerformMachineMoveToNewGroup()
{
    m_pWidget->moveMachineToGroup();
}

void UIVirtualBoxManager::sltPerformMachineMoveToSpecificGroup()
{
    AssertPtrReturnVoid(sender());
    QAction *pAction = qobject_cast<QAction*>(sender());
    AssertPtrReturnVoid(pAction);
    m_pWidget->moveMachineToGroup(pAction->property("actual_group_name").toString());
}

void UIVirtualBoxManager::sltPerformStartOrShowMachine()
{
    /* Start selected VMs in corresponding mode: */
    QList<UIVirtualMachineItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));
    performStartOrShowVirtualMachines(items, UICommon::LaunchMode_Invalid);
}

void UIVirtualBoxManager::sltPerformStartMachineNormal()
{
    /* Start selected VMs in corresponding mode: */
    QList<UIVirtualMachineItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));
    performStartOrShowVirtualMachines(items, UICommon::LaunchMode_Default);
}

void UIVirtualBoxManager::sltPerformStartMachineHeadless()
{
    /* Start selected VMs in corresponding mode: */
    QList<UIVirtualMachineItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));
    performStartOrShowVirtualMachines(items, UICommon::LaunchMode_Headless);
}

void UIVirtualBoxManager::sltPerformStartMachineDetachable()
{
    /* Start selected VMs in corresponding mode: */
    QList<UIVirtualMachineItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));
    performStartOrShowVirtualMachines(items, UICommon::LaunchMode_Separate);
}

void UIVirtualBoxManager::sltPerformCreateConsoleConnectionForGroup()
{
    /* Get selected items: */
    QList<UIVirtualMachineItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* Create input dialog to pass public key to newly created console connection: */
    QPointer<UIAcquirePublicKeyDialog> pDialog = new UIAcquirePublicKeyDialog(this);
    if (pDialog)
    {
        if (pDialog->exec() == QDialog::Accepted)
        {
            foreach (UIVirtualMachineItem *pItem, items)
            {
                /* Make sure the item exists: */
                AssertPtr(pItem);
                if (pItem)
                {
                    /* Make sure the item is of cloud type: */
                    UIVirtualMachineItemCloud *pCloudItem = pItem->toCloud();
                    if (pCloudItem)
                    {
                        /* Acquire current machine: */
                        CCloudMachine comMachine = pCloudItem->machine();

                        /* Acquire machine console connection fingerprint: */
                        QString strConsoleConnectionFingerprint;
                        if (cloudMachineConsoleConnectionFingerprint(comMachine, strConsoleConnectionFingerprint))
                        {
                            /* Only if no fingerprint exist: */
                            if (strConsoleConnectionFingerprint.isEmpty())
                            {
                                /* Acquire machine name: */
                                QString strName;
                                if (cloudMachineName(comMachine, strName))
                                {
                                    /* Prepare "create console connection" progress: */
                                    CProgress comProgress = comMachine.CreateConsoleConnection(pDialog->publicKey());
                                    if (!comMachine.isOk())
                                        msgCenter().cannotCreateConsoleConnection(comMachine);
                                    else
                                    {
                                        /* Show "create console connection" progress: */
                                        msgCenter().showModalProgressDialog(comProgress, strName, ":/progress_media_delete_90px.png", 0, 0); /// @todo use proper icon
                                        if (!comProgress.isOk() || comProgress.GetResultCode() != 0)
                                            msgCenter().cannotCreateConsoleConnection(comProgress, strName);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        delete pDialog;
    }
}

void UIVirtualBoxManager::sltPerformCreateConsoleConnectionForMachine()
{
    /* Get current item: */
    UIVirtualMachineItem *pItem = currentItem();
    AssertMsgReturnVoid(pItem, ("Current item should be selected!\n"));

    /* Create input dialog to pass public key to newly created console connection: */
    QPointer<UIAcquirePublicKeyDialog> pDialog = new UIAcquirePublicKeyDialog(this);
    if (pDialog)
    {
        if (pDialog->exec() == QDialog::Accepted)
        {
            /* Make sure the item is of cloud type: */
            UIVirtualMachineItemCloud *pCloudItem = pItem->toCloud();
            AssertPtr(pCloudItem);
            if (pCloudItem)
            {
                /* Acquire current machine: */
                CCloudMachine comMachine = pCloudItem->machine();

                /* Acquire machine console connection fingerprint: */
                QString strConsoleConnectionFingerprint;
                if (cloudMachineConsoleConnectionFingerprint(comMachine, strConsoleConnectionFingerprint))
                {
                    /* Only if no fingerprint exist: */
                    if (strConsoleConnectionFingerprint.isEmpty())
                    {
                        /* Acquire machine name: */
                        QString strName;
                        if (cloudMachineName(comMachine, strName))
                        {
                            /* Prepare "create console connection" progress: */
                            CProgress comProgress = comMachine.CreateConsoleConnection(pDialog->publicKey());
                            if (!comMachine.isOk())
                                msgCenter().cannotCreateConsoleConnection(comMachine);
                            else
                            {
                                /* Show "create console connection" progress: */
                                msgCenter().showModalProgressDialog(comProgress, strName, ":/progress_media_delete_90px.png", 0, 0); /// @todo use proper icon
                                if (!comProgress.isOk() || comProgress.GetResultCode() != 0)
                                    msgCenter().cannotCreateConsoleConnection(comProgress, strName);
                            }
                        }
                    }
                }
            }
        }
        delete pDialog;
    }
}

void UIVirtualBoxManager::sltPerformDeleteConsoleConnectionForGroup()
{
    /* Get selected items: */
    QList<UIVirtualMachineItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    foreach (UIVirtualMachineItem *pItem, items)
    {
        /* Make sure the item exists: */
        AssertPtr(pItem);
        if (pItem)
        {
            /* Make sure the item is of cloud type: */
            UIVirtualMachineItemCloud *pCloudItem = pItem->toCloud();
            if (pCloudItem)
            {
                /* Acquire current machine: */
                CCloudMachine comMachine = pCloudItem->machine();

                /* Acquire machine console connection fingerprint: */
                QString strConsoleConnectionFingerprint;
                if (cloudMachineConsoleConnectionFingerprint(comMachine, strConsoleConnectionFingerprint))
                {
                    /* Only if fingerprint exists: */
                    if (!strConsoleConnectionFingerprint.isEmpty())
                    {
                        /* Acquire machine name: */
                        QString strName;
                        if (cloudMachineName(comMachine, strName))
                        {
                            /* Prepare "delete console connection" progress: */
                            CProgress comProgress = comMachine.DeleteConsoleConnection();
                            if (!comMachine.isOk())
                                msgCenter().cannotDeleteConsoleConnection(comMachine);
                            else
                            {
                                /* Show "delete console connection" progress: */
                                msgCenter().showModalProgressDialog(comProgress, strName, ":/progress_media_delete_90px.png", 0, 0); /// @todo use proper icon
                                if (!comProgress.isOk() || comProgress.GetResultCode() != 0)
                                    msgCenter().cannotDeleteConsoleConnection(comProgress, strName);
                            }
                        }
                    }
                }
            }
        }
    }
}

void UIVirtualBoxManager::sltPerformDeleteConsoleConnectionForMachine()
{
    /* Get current item: */
    UIVirtualMachineItem *pItem = currentItem();
    AssertMsgReturnVoid(pItem, ("Current item should be selected!\n"));

    /* Make sure the item is of cloud type: */
    UIVirtualMachineItemCloud *pCloudItem = pItem->toCloud();
    AssertPtr(pCloudItem);
    if (pCloudItem)
    {
        /* Acquire current machine: */
        CCloudMachine comMachine = pCloudItem->machine();

        /* Acquire machine console connection fingerprint: */
        QString strConsoleConnectionFingerprint;
        if (cloudMachineConsoleConnectionFingerprint(comMachine, strConsoleConnectionFingerprint))
        {
            /* Only if fingerprint exists: */
            if (!strConsoleConnectionFingerprint.isEmpty())
            {
                /* Acquire machine name: */
                QString strName;
                if (cloudMachineName(comMachine, strName))
                {
                    /* Prepare "delete console connection" progress: */
                    CProgress comProgress = comMachine.DeleteConsoleConnection();
                    if (!comMachine.isOk())
                        msgCenter().cannotDeleteConsoleConnection(comMachine);
                    else
                    {
                        /* Show "delete console connection" progress: */
                        msgCenter().showModalProgressDialog(comProgress, strName, ":/progress_media_delete_90px.png", 0, 0); /// @todo use proper icon
                        if (!comProgress.isOk() || comProgress.GetResultCode() != 0)
                            msgCenter().cannotDeleteConsoleConnection(comProgress, strName);
                    }
                }
            }
        }
    }
}

void UIVirtualBoxManager::sltCopyConsoleConnectionFingerprint()
{
    QAction *pAction = qobject_cast<QAction*>(sender());
    AssertPtrReturnVoid(pAction);
    QClipboard *pClipboard = QGuiApplication::clipboard();
    AssertPtrReturnVoid(pClipboard);
    pClipboard->setText(pAction->property("fingerprint").toString());
}

void UIVirtualBoxManager::sltExecuteExternalApplication()
{
    /* Acquire passed path and argument strings: */
    QAction *pAction = qobject_cast<QAction*>(sender());
    AssertMsgReturnVoid(pAction, ("This slot should be called by action only!\n"));
    const QString strPath = pAction->property("path").toString();
    const QString strArguments = pAction->property("arguments").toString();

    /* Get current-item: */
    UIVirtualMachineItem *pItem = currentItem();
    AssertMsgReturnVoid(pItem, ("Current item should be selected!\n"));
    UIVirtualMachineItemCloud *pCloudItem = pItem->toCloud();
    AssertPtrReturnVoid(pCloudItem);

    /* Get cloud machine to acquire serial command: */
    const CCloudMachine comMachine = pCloudItem->machine();

#if defined(VBOX_WS_MAC)
    /* Gather arguments: */
    QStringList arguments;
    arguments << parseShellArguments(strArguments);

    /* Make sure that isn't a request to start Open command: */
    if (strPath != "open" && strPath != "/usr/bin/open")
    {
        /* In that case just add the command we have as simple argument: */
        arguments << comMachine.GetSerialConsoleCommand();
    }
    else
    {
        /* Otherwise upload command to external file which can be opened with Open command: */
        QDir uiHomeFolder(uiCommon().virtualBox().GetHomeFolder());
        const QString strAbsoluteCommandName = uiHomeFolder.absoluteFilePath("last.command");
        QFile file(strAbsoluteCommandName);
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
        if (!file.open(QIODevice::WriteOnly))
            AssertFailedReturnVoid();
        file.write(comMachine.GetSerialConsoleCommand().toUtf8());
        file.close();
        arguments << strAbsoluteCommandName;
    }

    /* Execute console application finally: */
    QProcess::startDetached(strPath, arguments);
#elif defined(VBOX_WS_WIN)
    /* Gather arguments: */
    QStringList arguments;
    arguments << strArguments;
    arguments << comMachine.GetSerialConsoleCommandWindows();

    /* Execute console application finally: */
    QProcess::startDetached(QString("%1 %2").arg(strPath, arguments.join(' ')));
#elif defined(VBOX_WS_X11)
    /* Gather arguments: */
    QStringList arguments;
    arguments << parseShellArguments(strArguments);
    arguments << comMachine.GetSerialConsoleCommand();

    /* Execute console application finally: */
    QProcess::startDetached(strPath, arguments);
#endif /* VBOX_WS_X11 */
}

void UIVirtualBoxManager::sltPerformCopyCommandSerialUnix()
{
    /* Get current item: */
    UIVirtualMachineItem *pItem = currentItem();
    AssertMsgReturnVoid(pItem, ("Current item should be selected!\n"));
    UIVirtualMachineItemCloud *pCloudItem = pItem->toCloud();
    AssertPtrReturnVoid(pCloudItem);

    /* Acquire cloud machine: */
    CCloudMachine comMachine = pCloudItem->machine();

    /* Put copied serial command to clipboard: */
    QClipboard *pClipboard = QGuiApplication::clipboard();
    AssertPtrReturnVoid(pClipboard);
    pClipboard->setText(comMachine.GetSerialConsoleCommand());
}

void UIVirtualBoxManager::sltPerformCopyCommandSerialWindows()
{
    /* Get current item: */
    UIVirtualMachineItem *pItem = currentItem();
    AssertMsgReturnVoid(pItem, ("Current item should be selected!\n"));
    UIVirtualMachineItemCloud *pCloudItem = pItem->toCloud();
    AssertPtrReturnVoid(pCloudItem);

    /* Acquire cloud machine: */
    CCloudMachine comMachine = pCloudItem->machine();

    /* Put copied serial command to clipboard: */
    QClipboard *pClipboard = QGuiApplication::clipboard();
    AssertPtrReturnVoid(pClipboard);
    pClipboard->setText(comMachine.GetSerialConsoleCommandWindows());
}

void UIVirtualBoxManager::sltPerformCopyCommandVNCUnix()
{
    /* Get current item: */
    UIVirtualMachineItem *pItem = currentItem();
    AssertMsgReturnVoid(pItem, ("Current item should be selected!\n"));
    UIVirtualMachineItemCloud *pCloudItem = pItem->toCloud();
    AssertPtrReturnVoid(pCloudItem);

    /* Acquire cloud machine: */
    CCloudMachine comMachine = pCloudItem->machine();

    /* Put copied VNC command to clipboard: */
    QClipboard *pClipboard = QGuiApplication::clipboard();
    AssertPtrReturnVoid(pClipboard);
    pClipboard->setText(comMachine.GetVNCConsoleCommand());
}

void UIVirtualBoxManager::sltPerformCopyCommandVNCWindows()
{
    /* Get current item: */
    UIVirtualMachineItem *pItem = currentItem();
    AssertMsgReturnVoid(pItem, ("Current item should be selected!\n"));
    UIVirtualMachineItemCloud *pCloudItem = pItem->toCloud();
    AssertPtrReturnVoid(pCloudItem);

    /* Acquire cloud machine: */
    CCloudMachine comMachine = pCloudItem->machine();

    /* Put copied VNC command to clipboard: */
    QClipboard *pClipboard = QGuiApplication::clipboard();
    AssertPtrReturnVoid(pClipboard);
    pClipboard->setText(comMachine.GetVNCConsoleCommandWindows());
}

void UIVirtualBoxManager::sltPerformDiscardMachineState()
{
    /* Get selected items: */
    QList<UIVirtualMachineItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* Prepare the list of the machines to be discarded/terminated: */
    QStringList machinesToDiscard;
    QStringList machinesToTerminate;
    QList<UIVirtualMachineItem*> itemsToDiscard;
    QList<UIVirtualMachineItem*> itemsToTerminate;
    foreach (UIVirtualMachineItem *pItem, items)
    {
        if (isActionEnabled(UIActionIndexMN_M_Group_S_Discard, QList<UIVirtualMachineItem*>() << pItem))
        {
            if (pItem->itemType() == UIVirtualMachineItemType_Local)
            {
                machinesToDiscard << pItem->name();
                itemsToDiscard << pItem;
            }
            else if (pItem->itemType() == UIVirtualMachineItemType_CloudReal)
            {
                machinesToTerminate << pItem->name();
                itemsToTerminate << pItem;
            }
        }
    }
    AssertMsg(!machinesToDiscard.isEmpty() || !machinesToTerminate.isEmpty(), ("This action should not be allowed!"));

    /* Confirm discarding/terminating: */
    if (   (machinesToDiscard.isEmpty() || !msgCenter().confirmDiscardSavedState(machinesToDiscard.join(", ")))
        && (machinesToTerminate.isEmpty() || !msgCenter().confirmTerminateCloudInstance(machinesToTerminate.join(", "))))
        return;

    /* For every confirmed item to discard: */
    foreach (UIVirtualMachineItem *pItem, itemsToDiscard)
    {
        /* Open a session to modify VM: */
        AssertPtrReturnVoid(pItem);
        CSession comSession = uiCommon().openSession(pItem->id());
        if (comSession.isNull())
            return;

        /* Get session machine: */
        CMachine comMachine = comSession.GetMachine();
        comMachine.DiscardSavedState(true);
        if (!comMachine.isOk())
            msgCenter().cannotDiscardSavedState(comMachine);

        /* Unlock machine finally: */
        comSession.UnlockMachine();
    }

    /* For every confirmed item to terminate: */
    foreach (UIVirtualMachineItem *pItem, itemsToTerminate)
    {
        /* Get cloud machine: */
        AssertPtrReturnVoid(pItem);
        UIVirtualMachineItemCloud *pCloudItem = pItem->toCloud();
        AssertPtrReturnVoid(pCloudItem);
        CCloudMachine comMachine = pCloudItem->machine();

        /* Acquire machine name: */
        QString strName;
        if (!cloudMachineName(comMachine, strName))
            continue;

        /* Prepare terminate cloud instance progress: */
        CProgress comProgress = comMachine.Terminate();
        if (!comMachine.isOk())
        {
            msgCenter().cannotTerminateCloudInstance(comMachine);
            continue;
        }

        /* Show terminate cloud instance progress: */
        msgCenter().showModalProgressDialog(comProgress, strName, ":/progress_media_delete_90px.png", 0, 0); /// @todo use proper icon
        if (!comProgress.isOk() || comProgress.GetResultCode() != 0)
            msgCenter().cannotTerminateCloudInstance(comProgress, strName);
    }
}

void UIVirtualBoxManager::sltPerformPauseOrResumeMachine(bool fPause)
{
    /* Get selected items: */
    QList<UIVirtualMachineItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* For every selected item: */
    foreach (UIVirtualMachineItem *pItem, items)
    {
        /* But for local machine items only: */
        AssertPtrReturnVoid(pItem);
        if (pItem->itemType() != UIVirtualMachineItemType_Local)
            continue;

        /* Get local machine item state: */
        UIVirtualMachineItemLocal *pLocalItem = pItem->toLocal();
        AssertPtrReturnVoid(pLocalItem);
        const KMachineState enmState = pLocalItem->machineState();

        /* Check if current item could be paused/resumed: */
        if (!isActionEnabled(UIActionIndexMN_M_Group_T_Pause, QList<UIVirtualMachineItem*>() << pItem))
            continue;

        /* Check if current item already paused: */
        if (fPause &&
            (enmState == KMachineState_Paused ||
             enmState == KMachineState_TeleportingPausedVM))
            continue;

        /* Check if current item already resumed: */
        if (!fPause &&
            (enmState == KMachineState_Running ||
             enmState == KMachineState_Teleporting ||
             enmState == KMachineState_LiveSnapshotting))
            continue;

        /* Open a session to modify VM state: */
        CSession comSession = uiCommon().openExistingSession(pItem->id());
        if (comSession.isNull())
            return;

        /* Get session console: */
        CConsole comConsole = comSession.GetConsole();
        /* Pause/resume VM: */
        if (fPause)
            comConsole.Pause();
        else
            comConsole.Resume();
        if (!comConsole.isOk())
        {
            if (fPause)
                msgCenter().cannotPauseMachine(comConsole);
            else
                msgCenter().cannotResumeMachine(comConsole);
        }

        /* Unlock machine finally: */
        comSession.UnlockMachine();
    }
}

void UIVirtualBoxManager::sltPerformResetMachine()
{
    /* Get selected items: */
    QList<UIVirtualMachineItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* Prepare the list of the machines to be reseted: */
    QStringList machineNames;
    QList<UIVirtualMachineItem*> itemsToReset;
    foreach (UIVirtualMachineItem *pItem, items)
    {
        if (isActionEnabled(UIActionIndexMN_M_Group_S_Reset, QList<UIVirtualMachineItem*>() << pItem))
        {
            machineNames << pItem->name();
            itemsToReset << pItem;
        }
    }
    AssertMsg(!machineNames.isEmpty(), ("This action should not be allowed!"));

    /* Confirm reseting VM: */
    if (!msgCenter().confirmResetMachine(machineNames.join(", ")))
        return;

    /* For each selected item: */
    foreach (UIVirtualMachineItem *pItem, itemsToReset)
    {
        /* Open a session to modify VM state: */
        CSession comSession = uiCommon().openExistingSession(pItem->id());
        if (comSession.isNull())
            return;

        /* Get session console: */
        CConsole comConsole = comSession.GetConsole();
        /* Reset VM: */
        comConsole.Reset();

        /* Unlock machine finally: */
        comSession.UnlockMachine();
    }
}

void UIVirtualBoxManager::sltPerformDetachMachineUI()
{
    /* Get selected items: */
    QList<UIVirtualMachineItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* For each selected item: */
    foreach (UIVirtualMachineItem *pItem, items)
    {
        /* Check if current item could be detached: */
        if (!isActionEnabled(UIActionIndexMN_M_Machine_M_Close_S_Detach, QList<UIVirtualMachineItem*>() << pItem))
            continue;

        /// @todo Detach separate UI process..
        AssertFailed();
    }
}

void UIVirtualBoxManager::sltPerformSaveMachineState()
{
    /* Get selected items: */
    QList<UIVirtualMachineItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* For each selected item: */
    foreach (UIVirtualMachineItem *pItem, items)
    {
        /* Check if current item could be saved: */
        AssertPtrReturnVoid(pItem);
        if (!isActionEnabled(UIActionIndexMN_M_Machine_M_Close_S_SaveState, QList<UIVirtualMachineItem*>() << pItem))
            continue;

        /* Open a session to modify VM state: */
        CSession comSession = uiCommon().openExistingSession(pItem->id());
        if (comSession.isNull())
            return;

        /* Get session console: */
        CConsole comConsole = comSession.GetConsole();
        /* Get session machine: */
        CMachine comMachine = comSession.GetMachine();

        /* Get local machine item state: */
        UIVirtualMachineItemLocal *pLocalItem = pItem->toLocal();
        AssertPtrReturnVoid(pLocalItem);
        const KMachineState enmState = pLocalItem->machineState();

        /* Pause VM first if necessary: */
        if (enmState != KMachineState_Paused)
            comConsole.Pause();
        if (comConsole.isOk())
        {
            /* Prepare machine state saving progress: */
            CProgress comProgress = comMachine.SaveState();
            if (comMachine.isOk())
            {
                /* Show machine state saving progress: */
                msgCenter().showModalProgressDialog(comProgress, comMachine.GetName(), ":/progress_state_save_90px.png");
                if (!comProgress.isOk() || comProgress.GetResultCode() != 0)
                    msgCenter().cannotSaveMachineState(comProgress, comMachine.GetName());
            }
            else
                msgCenter().cannotSaveMachineState(comMachine);
        }
        else
            msgCenter().cannotPauseMachine(comConsole);

        /* Unlock machine finally: */
        comSession.UnlockMachine();
    }
}

void UIVirtualBoxManager::sltPerformShutdownMachine()
{
    /* Get selected items: */
    QList<UIVirtualMachineItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* Prepare the list of the machines to be shutdowned: */
    QStringList machineNames;
    QList<UIVirtualMachineItem*> itemsToShutdown;
    foreach (UIVirtualMachineItem *pItem, items)
    {
        if (isActionEnabled(UIActionIndexMN_M_Machine_M_Close_S_Shutdown, QList<UIVirtualMachineItem*>() << pItem))
        {
            machineNames << pItem->name();
            itemsToShutdown << pItem;
        }
    }
    AssertMsg(!machineNames.isEmpty(), ("This action should not be allowed!"));

    /* Confirm ACPI shutdown current VM: */
    if (!msgCenter().confirmACPIShutdownMachine(machineNames.join(", ")))
        return;

    /* For each selected item: */
    foreach (UIVirtualMachineItem *pItem, itemsToShutdown)
    {
        /* For local machine: */
        if (pItem->itemType() == UIVirtualMachineItemType_Local)
        {
            /* Open a session to modify VM state: */
            CSession comSession = uiCommon().openExistingSession(pItem->id());
            if (comSession.isNull())
                return;

            /* Get session console: */
            CConsole comConsole = comSession.GetConsole();
            /* ACPI Shutdown: */
            comConsole.PowerButton();
            if (!comConsole.isOk())
                msgCenter().cannotACPIShutdownMachine(comConsole);

            /* Unlock machine finally: */
            comSession.UnlockMachine();
        }
        /* For real cloud machine: */
        else if (pItem->itemType() == UIVirtualMachineItemType_CloudReal)
        {
            /* Acquire cloud machine: */
            CCloudMachine comCloudMachine = pItem->toCloud()->machine();
            /* Prepare machine ACPI shutdown: */
            CProgress comProgress = comCloudMachine.Shutdown();
            if (!comCloudMachine.isOk())
                msgCenter().cannotACPIShutdownCloudMachine(comCloudMachine);
            else
            {
                /* Show machine ACPI shutdown progress: */
                msgCenter().showModalProgressDialog(comProgress, pItem->name(), ":/progress_poweroff_90px.png", 0, 0);
                if (!comProgress.isOk() || comProgress.GetResultCode() != 0)
                    msgCenter().cannotACPIShutdownCloudMachine(comProgress, pItem->name());
                /* Update info in any case: */
                pItem->toCloud()->updateInfoAsync(false /* delayed? */);
            }
        }
    }
}

void UIVirtualBoxManager::sltPerformPowerOffMachine()
{
    /* Get selected items: */
    QList<UIVirtualMachineItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* Prepare the list of the machines to be powered off: */
    QStringList machineNames;
    QList<UIVirtualMachineItem*> itemsToPowerOff;
    foreach (UIVirtualMachineItem *pItem, items)
    {
        if (isActionEnabled(UIActionIndexMN_M_Machine_M_Close_S_PowerOff, QList<UIVirtualMachineItem*>() << pItem))
        {
            machineNames << pItem->name();
            itemsToPowerOff << pItem;
        }
    }
    AssertMsg(!machineNames.isEmpty(), ("This action should not be allowed!"));

    /* Confirm Power Off current VM: */
    if (!msgCenter().confirmPowerOffMachine(machineNames.join(", ")))
        return;

    /* For each selected item: */
    foreach (UIVirtualMachineItem *pItem, itemsToPowerOff)
    {
        /* For local machine: */
        if (pItem->itemType() == UIVirtualMachineItemType_Local)
        {
            /* Open a session to modify VM state: */
            CSession comSession = uiCommon().openExistingSession(pItem->id());
            if (comSession.isNull())
                break;

            /* Get session console: */
            CConsole comConsole = comSession.GetConsole();
            /* Prepare machine power down: */
            CProgress comProgress = comConsole.PowerDown();
            if (!comConsole.isOk())
                msgCenter().cannotPowerDownMachine(comConsole);
            else
            {
                /* Show machine power down progress: */
                msgCenter().showModalProgressDialog(comProgress, pItem->name(), ":/progress_poweroff_90px.png");
                if (!comProgress.isOk() || comProgress.GetResultCode() != 0)
                    msgCenter().cannotPowerDownMachine(comProgress, pItem->name());
            }

            /* Unlock machine finally: */
            comSession.UnlockMachine();
        }
        /* For real cloud machine: */
        else if (pItem->itemType() == UIVirtualMachineItemType_CloudReal)
        {
            /* Acquire cloud machine: */
            CCloudMachine comCloudMachine = pItem->toCloud()->machine();
            /* Prepare machine power down: */
            CProgress comProgress = comCloudMachine.PowerDown();
            if (!comCloudMachine.isOk())
                msgCenter().cannotPowerDownCloudMachine(comCloudMachine);
            else
            {
                /* Show machine power down progress: */
                msgCenter().showModalProgressDialog(comProgress, pItem->name(), ":/progress_poweroff_90px.png", 0, 0);
                if (!comProgress.isOk() || comProgress.GetResultCode() != 0)
                    msgCenter().cannotPowerDownCloudMachine(comProgress, pItem->name());
                /* Update info in any case: */
                pItem->toCloud()->updateInfoAsync(false /* delayed? */);
            }
        }
    }
}

void UIVirtualBoxManager::sltPerformShowMachineTool(QAction *pAction)
{
    AssertPtrReturnVoid(pAction);
    AssertPtrReturnVoid(m_pWidget);
    m_pWidget->setToolsType(pAction->property("UIToolType").value<UIToolType>());
}

void UIVirtualBoxManager::sltOpenLogViewerWindow()
{
    /* Get selected items: */
    QList<UIVirtualMachineItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* First check if instance of widget opened the embedded way: */
    if (m_pWidget->isMachineToolOpened(UIToolType_Logs))
    {
        m_pWidget->setToolsType(UIToolType_Details);
        m_pWidget->closeMachineTool(UIToolType_Logs);
    }

    /* For each selected item: */
    foreach (UIVirtualMachineItem *pItem, items)
    {
        /* Make sure current item is local one: */
        UIVirtualMachineItemLocal *pItemLocal = pItem->toLocal();
        if (!pItemLocal)
            continue;

        /* Check if log could be show for the current item: */
        if (!isActionEnabled(UIActionIndexMN_M_Group_S_ShowLogDialog, QList<UIVirtualMachineItem*>() << pItem))
            continue;

        QIManagerDialog *pLogViewerDialog = 0;
        /* Create and Show VM Log Viewer: */
        if (!m_logViewers[pItemLocal->machine().GetHardwareUUID().toString()])
        {
            UIVMLogViewerDialogFactory dialogFactory(actionPool(), pItemLocal->machine());
            dialogFactory.prepare(pLogViewerDialog, this);
            if (pLogViewerDialog)
            {
                m_logViewers[pItemLocal->machine().GetHardwareUUID().toString()] = pLogViewerDialog;
                connect(pLogViewerDialog, &QIManagerDialog::sigClose,
                        this, &UIVirtualBoxManager::sltCloseLogViewerWindow);
            }
        }
        else
        {
            pLogViewerDialog = m_logViewers[pItemLocal->machine().GetHardwareUUID().toString()];
        }
        if (pLogViewerDialog)
        {
            /* Show instance: */
            pLogViewerDialog->show();
            pLogViewerDialog->setWindowState(pLogViewerDialog->windowState() & ~Qt::WindowMinimized);
            pLogViewerDialog->activateWindow();
        }
    }
}

void UIVirtualBoxManager::sltCloseLogViewerWindow()
{
    /* If there is a proper sender: */
    if (qobject_cast<QIManagerDialog*>(sender()))
    {
        /* Search for the sender of the signal within the m_logViewers map: */
        QMap<QString, QIManagerDialog*>::iterator sendersIterator = m_logViewers.begin();
        while (sendersIterator != m_logViewers.end() && sendersIterator.value() != sender())
            ++sendersIterator;
        /* Do nothing if we cannot find it with the map: */
        if (sendersIterator == m_logViewers.end())
            return;

        /* Check whether we have found the proper dialog: */
        QIManagerDialog *pDialog = qobject_cast<QIManagerDialog*>(sendersIterator.value());
        if (!pDialog)
            return;

        /* First remove this log-viewer dialog from the map.
         * This should be done before closing the dialog which will incur
         * a second call to this function and result in double delete!!! */
        m_logViewers.erase(sendersIterator);
        UIVMLogViewerDialogFactory().cleanup(pDialog);
    }
    /* Otherwise: */
    else
    {
        /* Just wipe out everything: */
        foreach (const QString &strKey, m_logViewers.keys())
        {
            /* First remove each log-viewer dialog from the map.
             * This should be done before closing the dialog which will incur
             * a second call to this function and result in double delete!!! */
            QIManagerDialog *pDialog = m_logViewers.value(strKey);
            m_logViewers.remove(strKey);
            UIVMLogViewerDialogFactory().cleanup(pDialog);
        }
    }
}

void UIVirtualBoxManager::sltOpenPerformanceMonitorWindow()
{
}

void UIVirtualBoxManager::sltClosePerformanceMonitorWindow()
{
}

void UIVirtualBoxManager::sltPerformRefreshMachine()
{
    m_pWidget->refreshMachine();
}

void UIVirtualBoxManager::sltShowMachineInFileManager()
{
    /* Get selected items: */
    QList<UIVirtualMachineItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* For each selected item: */
    foreach (UIVirtualMachineItem *pItem, items)
    {
        /* Make sure current item is local one: */
        UIVirtualMachineItemLocal *pItemLocal = pItem->toLocal();
        if (!pItemLocal)
            continue;

        /* Check if that item could be shown in file-browser: */
        if (!isActionEnabled(UIActionIndexMN_M_Group_S_ShowInFileManager, QList<UIVirtualMachineItem*>() << pItem))
            continue;

        /* Show VM in filebrowser: */
        UIDesktopServices::openInFileManager(pItemLocal->machine().GetSettingsFilePath());
    }
}

void UIVirtualBoxManager::sltPerformCreateMachineShortcut()
{
    /* Get selected items: */
    QList<UIVirtualMachineItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* For each selected item: */
    foreach (UIVirtualMachineItem *pItem, items)
    {
        /* Make sure current item is local one: */
        UIVirtualMachineItemLocal *pItemLocal = pItem->toLocal();
        if (!pItemLocal)
            continue;

        /* Check if shortcuts could be created for this item: */
        if (!isActionEnabled(UIActionIndexMN_M_Group_S_CreateShortcut, QList<UIVirtualMachineItem*>() << pItem))
            continue;

        /* Create shortcut for this VM: */
        const CMachine &comMachine = pItemLocal->machine();
        UIDesktopServices::createMachineShortcut(comMachine.GetSettingsFilePath(),
                                                 QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                                                 comMachine.GetName(), comMachine.GetId());
    }
}

void UIVirtualBoxManager::sltPerformGroupSorting()
{
    m_pWidget->sortGroup();
}

void UIVirtualBoxManager::sltPerformMachineSearchWidgetVisibilityToggling(bool fVisible)
{
    m_pWidget->setMachineSearchWidgetVisibility(fVisible);
}

void UIVirtualBoxManager::prepare()
{
#ifdef VBOX_WS_X11
    /* Assign same name to both WM_CLASS name & class for now: */
    UICommon::setWMClass(this, "VirtualBox Manager", "VirtualBox Manager");
#endif

#ifdef VBOX_WS_MAC
    /* We have to make sure that we are getting the front most process: */
    ::darwinSetFrontMostProcess();
    /* Install global event-filter, since vmstarter.app can send us FileOpen events,
     * see UIVirtualBoxManager::eventFilter for handler implementation. */
    qApp->installEventFilter(this);
#endif

    /* Cache media data early if necessary: */
    if (uiCommon().agressiveCaching())
        uiCommon().enumerateMedia();

    /* Prepare: */
    prepareIcon();
    prepareMenuBar();
    prepareStatusBar();
    prepareWidgets();
    prepareConnections();

    /* Update actions initially: */
    updateActionsVisibility();
    updateActionsAppearance();

    /* Load settings: */
    loadSettings();

    /* Translate UI: */
    retranslateUi();

#ifdef VBOX_WS_MAC
    /* Beta label? */
    if (uiCommon().isBeta())
    {
        QPixmap betaLabel = ::betaLabel(QSize(100, 16));
        ::darwinLabelWindow(this, &betaLabel, true);
    }
#endif /* VBOX_WS_MAC */

    /* If there are unhandled URLs we should handle them after manager is shown: */
    if (uiCommon().argumentUrlsPresent())
        QMetaObject::invokeMethod(this, "sltHandleOpenUrlCall", Qt::QueuedConnection);
}

void UIVirtualBoxManager::prepareIcon()
{
    /* Prepare application icon.
     * On Win host it's built-in to the executable.
     * On Mac OS X the icon referenced in info.plist is used.
     * On X11 we will provide as much icons as we can. */
#if !defined(VBOX_WS_WIN) && !defined(VBOX_WS_MAC)
    QIcon icon(":/VirtualBox.svg");
    icon.addFile(":/VirtualBox_48px.png");
    icon.addFile(":/VirtualBox_64px.png");
    setWindowIcon(icon);
#endif /* !VBOX_WS_WIN && !VBOX_WS_MAC */
}

void UIVirtualBoxManager::prepareMenuBar()
{
#ifndef VBOX_WS_MAC
    /* Create menu-bar: */
    setMenuBar(new UIMenuBar);
    if (menuBar())
    {
        /* Make sure menu-bar fills own solid background: */
        menuBar()->setAutoFillBackground(true);
        QPalette pal = menuBar()->palette();
        const QColor color = pal.color(QPalette::Active, QPalette::Mid).lighter(160);
        pal.setColor(QPalette::Active, QPalette::Button, color);
        menuBar()->setPalette(pal);
    }
#endif

    /* Create action-pool: */
    m_pActionPool = UIActionPool::create(UIActionPoolType_Manager);

    /* Prepare menu update-handlers: */
    m_menuUpdateHandlers[UIActionIndexMN_M_Group] = &UIVirtualBoxManager::updateMenuGroup;
    m_menuUpdateHandlers[UIActionIndexMN_M_Machine] = &UIVirtualBoxManager::updateMenuMachine;
    m_menuUpdateHandlers[UIActionIndexMN_M_Group_M_MoveToGroup] = &UIVirtualBoxManager::updateMenuGroupMoveToGroup;
    m_menuUpdateHandlers[UIActionIndexMN_M_Group_M_Console] = &UIVirtualBoxManager::updateMenuGroupConsole;
    m_menuUpdateHandlers[UIActionIndexMN_M_Group_M_Close] = &UIVirtualBoxManager::updateMenuGroupClose;
    m_menuUpdateHandlers[UIActionIndexMN_M_Machine_M_MoveToGroup] = &UIVirtualBoxManager::updateMenuMachineMoveToGroup;
    m_menuUpdateHandlers[UIActionIndexMN_M_Machine_M_Console] = &UIVirtualBoxManager::updateMenuMachineConsole;
    m_menuUpdateHandlers[UIActionIndexMN_M_Machine_M_Close] = &UIVirtualBoxManager::updateMenuMachineClose;

    /* Build menu-bar: */
    foreach (QMenu *pMenu, actionPool()->menus())
    {
#ifdef VBOX_WS_MAC
        /* Before 'Help' menu we should: */
        if (pMenu == actionPool()->action(UIActionIndex_Menu_Help)->menu())
        {
            /* Insert 'Window' menu: */
            UIWindowMenuManager::create();
            menuBar()->addMenu(gpWindowMenuManager->createMenu(this));
            gpWindowMenuManager->addWindow(this);
        }
#endif
        menuBar()->addMenu(pMenu);
    }

    /* Setup menu-bar policy: */
    menuBar()->setContextMenuPolicy(Qt::CustomContextMenu);
}

void UIVirtualBoxManager::prepareStatusBar()
{
    /* We are not using status-bar anymore: */
    statusBar()->setHidden(true);
}

void UIVirtualBoxManager::prepareWidgets()
{
    /* Prepare central-widget: */
    m_pWidget = new UIVirtualBoxManagerWidget(this);
    if (m_pWidget)
        setCentralWidget(m_pWidget);
}

void UIVirtualBoxManager::prepareConnections()
{
#ifdef VBOX_WS_X11
    /* Desktop event handlers: */
    connect(gpDesktop, &UIDesktopWidgetWatchdog::sigHostScreenWorkAreaResized,
            this, &UIVirtualBoxManager::sltHandleHostScreenAvailableAreaChange);
#endif

    /* Medium enumeration connections: */
    connect(&uiCommon(), &UICommon::sigMediumEnumerationFinished,
            this, &UIVirtualBoxManager::sltHandleMediumEnumerationFinish);

    /* Widget connections: */
    connect(m_pWidget, &UIVirtualBoxManagerWidget::sigChooserPaneIndexChange,
            this, &UIVirtualBoxManager::sltHandleChooserPaneIndexChange);
    connect(m_pWidget, &UIVirtualBoxManagerWidget::sigGroupSavingStateChanged,
            this, &UIVirtualBoxManager::sltHandleGroupSavingProgressChange);
    connect(m_pWidget, &UIVirtualBoxManagerWidget::sigCloudUpdateStateChanged,
            this, &UIVirtualBoxManager::sltHandleCloudUpdateProgressChange);
    connect(m_pWidget, &UIVirtualBoxManagerWidget::sigStartOrShowRequest,
            this, &UIVirtualBoxManager::sltPerformStartOrShowMachine);
    connect(m_pWidget, &UIVirtualBoxManagerWidget::sigCloudMachineStateChange,
            this, &UIVirtualBoxManager::sltHandleCloudMachineStateChange);
    connect(m_pWidget, &UIVirtualBoxManagerWidget::sigToolTypeChange,
            this, &UIVirtualBoxManager::sltHandleToolTypeChange);
    connect(m_pWidget, &UIVirtualBoxManagerWidget::sigMachineSettingsLinkClicked,
            this, &UIVirtualBoxManager::sltOpenMachineSettingsDialog);
    connect(m_pWidget, &UIVirtualBoxManagerWidget::sigCurrentSnapshotItemChange,
            this, &UIVirtualBoxManager::sltCurrentSnapshotItemChange);
    connect(menuBar(), &QMenuBar::customContextMenuRequested,
            m_pWidget, &UIVirtualBoxManagerWidget::sltHandleToolBarContextMenuRequest);

    /* Global VBox event handlers: */
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigMachineStateChange,
            this, &UIVirtualBoxManager::sltHandleStateChange);
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigSessionStateChange,
            this, &UIVirtualBoxManager::sltHandleStateChange);

    /* General action-pool connections: */
    connect(actionPool(), &UIActionPool::sigNotifyAboutMenuPrepare, this, &UIVirtualBoxManager::sltHandleMenuPrepare);

    /* 'File' menu connections: */
    connect(actionPool()->action(UIActionIndexMN_M_File_S_ShowVirtualMediumManager), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenVirtualMediumManagerWindow);
    connect(actionPool()->action(UIActionIndexMN_M_File_S_ShowHostNetworkManager), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenHostNetworkManagerWindow);
    connect(actionPool()->action(UIActionIndexMN_M_File_S_ShowCloudProfileManager), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenCloudProfileManagerWindow);
    connect(actionPool()->action(UIActionIndexMN_M_File_S_ImportAppliance), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenImportApplianceWizardDefault);
    connect(actionPool()->action(UIActionIndexMN_M_File_S_ExportAppliance), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenExportApplianceWizard);
#ifdef VBOX_GUI_WITH_EXTRADATA_MANAGER_UI
    connect(actionPool()->action(UIActionIndexMN_M_File_S_ShowExtraDataManager), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenExtraDataManagerWindow);
#endif /* VBOX_GUI_WITH_EXTRADATA_MANAGER_UI */
    connect(actionPool()->action(UIActionIndex_M_Application_S_Preferences), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenPreferencesDialog);
    connect(actionPool()->action(UIActionIndexMN_M_File_S_Close), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformExit);

    /* 'Welcome' menu connections: */
    connect(actionPool()->action(UIActionIndexMN_M_Welcome_S_New), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenNewMachineWizard);
    connect(actionPool()->action(UIActionIndexMN_M_Welcome_S_Add), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenAddMachineDialog);

    /* 'Group' menu connections: */
    connect(actionPool()->action(UIActionIndexMN_M_Group_S_New), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenNewMachineWizard);
    connect(actionPool()->action(UIActionIndexMN_M_Group_S_Add), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenAddMachineDialog);
    connect(actionPool()->action(UIActionIndexMN_M_Group_S_Rename), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenGroupNameEditor);
    connect(actionPool()->action(UIActionIndexMN_M_Group_S_Remove), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltDisbandGroup);
    connect(actionPool()->action(UIActionIndexMN_M_Group_M_StartOrShow), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformStartOrShowMachine);
    connect(actionPool()->action(UIActionIndexMN_M_Group_T_Pause), &UIAction::toggled,
            this, &UIVirtualBoxManager::sltPerformPauseOrResumeMachine);
    connect(actionPool()->action(UIActionIndexMN_M_Group_S_Reset), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformResetMachine);
    connect(actionPool()->action(UIActionIndexMN_M_Group_S_Discard), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformDiscardMachineState);
    connect(actionPool()->action(UIActionIndexMN_M_Group_S_ShowLogDialog), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenLogViewerWindow);
    connect(actionPool()->action(UIActionIndexMN_M_Group_S_Refresh), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformRefreshMachine);
    connect(actionPool()->action(UIActionIndexMN_M_Group_S_ShowInFileManager), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltShowMachineInFileManager);
    connect(actionPool()->action(UIActionIndexMN_M_Group_S_CreateShortcut), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformCreateMachineShortcut);
    connect(actionPool()->action(UIActionIndexMN_M_Group_S_Sort), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformGroupSorting);
    connect(actionPool()->action(UIActionIndexMN_M_Group_T_Search), &UIAction::toggled,
            this, &UIVirtualBoxManager::sltPerformMachineSearchWidgetVisibilityToggling);
    connect(m_pWidget, &UIVirtualBoxManagerWidget::sigMachineSearchWidgetVisibilityChanged,
            actionPool()->action(UIActionIndexMN_M_Group_T_Search), &QAction::setChecked);

    /* 'Machine' menu connections: */
    connect(actionPool()->action(UIActionIndexMN_M_Machine_S_New), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenNewMachineWizard);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_S_Add), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenAddMachineDialog);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_S_Settings), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenMachineSettingsDialogDefault);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_S_Clone), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenCloneMachineWizard);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_S_Move), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformMachineMove);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_S_ExportToOCI), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenExportApplianceWizard);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_S_Remove), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformMachineRemove);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_M_MoveToGroup_S_New), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformMachineMoveToNewGroup);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_M_StartOrShow), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformStartOrShowMachine);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_T_Pause), &UIAction::toggled,
            this, &UIVirtualBoxManager::sltPerformPauseOrResumeMachine);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_S_Reset), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformResetMachine);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_S_Discard), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformDiscardMachineState);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_S_ShowLogDialog), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenLogViewerWindow);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_S_Refresh), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformRefreshMachine);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_S_ShowInFileManager), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltShowMachineInFileManager);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_S_CreateShortcut), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformCreateMachineShortcut);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_S_SortParent), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformGroupSorting);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_T_Search), &UIAction::toggled,
            this, &UIVirtualBoxManager::sltPerformMachineSearchWidgetVisibilityToggling);
    connect(m_pWidget, &UIVirtualBoxManagerWidget::sigMachineSearchWidgetVisibilityChanged,
            actionPool()->action(UIActionIndexMN_M_Machine_T_Search), &QAction::setChecked);

    /* 'Group/Start or Show' menu connections: */
    connect(actionPool()->action(UIActionIndexMN_M_Group_M_StartOrShow_S_StartNormal), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformStartMachineNormal);
    connect(actionPool()->action(UIActionIndexMN_M_Group_M_StartOrShow_S_StartHeadless), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformStartMachineHeadless);
    connect(actionPool()->action(UIActionIndexMN_M_Group_M_StartOrShow_S_StartDetachable), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformStartMachineDetachable);

    /* 'Machine/Start or Show' menu connections: */
    connect(actionPool()->action(UIActionIndexMN_M_Machine_M_StartOrShow_S_StartNormal), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformStartMachineNormal);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_M_StartOrShow_S_StartHeadless), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformStartMachineHeadless);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_M_StartOrShow_S_StartDetachable), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformStartMachineDetachable);

    /* 'Group/Console' menu connections: */
    connect(actionPool()->action(UIActionIndexMN_M_Group_M_Console_S_CreateConnection), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformCreateConsoleConnectionForGroup);
    connect(actionPool()->action(UIActionIndexMN_M_Group_M_Console_S_DeleteConnection), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformDeleteConsoleConnectionForGroup);
    connect(actionPool()->action(UIActionIndexMN_M_Group_M_Console_S_ConfigureApplications), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenCloudConsoleManagerWindow);

    /* 'Machine/Console' menu connections: */
    connect(actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_CreateConnection), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformCreateConsoleConnectionForMachine);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_DeleteConnection), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformDeleteConsoleConnectionForMachine);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_CopyCommandSerialUnix), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformCopyCommandSerialUnix);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_CopyCommandSerialWindows), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformCopyCommandSerialWindows);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_CopyCommandVNCUnix), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformCopyCommandVNCUnix);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_CopyCommandVNCWindows), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformCopyCommandVNCWindows);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_ConfigureApplications), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltOpenCloudConsoleManagerWindow);

    /* 'Group/Close' menu connections: */
    connect(actionPool()->action(UIActionIndexMN_M_Group_M_Close_S_Detach), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformDetachMachineUI);
    connect(actionPool()->action(UIActionIndexMN_M_Group_M_Close_S_SaveState), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformSaveMachineState);
    connect(actionPool()->action(UIActionIndexMN_M_Group_M_Close_S_Shutdown), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformShutdownMachine);
    connect(actionPool()->action(UIActionIndexMN_M_Group_M_Close_S_PowerOff), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformPowerOffMachine);

    /* 'Machine/Close' menu connections: */
    connect(actionPool()->action(UIActionIndexMN_M_Machine_M_Close_S_Detach), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformDetachMachineUI);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_M_Close_S_SaveState), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformSaveMachineState);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_M_Close_S_Shutdown), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformShutdownMachine);
    connect(actionPool()->action(UIActionIndexMN_M_Machine_M_Close_S_PowerOff), &UIAction::triggered,
            this, &UIVirtualBoxManager::sltPerformPowerOffMachine);

    /* 'Group/Tools' menu connections: */
    connect(actionPool()->actionGroup(UIActionIndexMN_M_Group_M_Tools), &QActionGroup::triggered,
            this, &UIVirtualBoxManager::sltPerformShowMachineTool);

    /* 'Machine/Tools' menu connections: */
    connect(actionPool()->actionGroup(UIActionIndexMN_M_Machine_M_Tools), &QActionGroup::triggered,
            this, &UIVirtualBoxManager::sltPerformShowMachineTool);
}

void UIVirtualBoxManager::loadSettings()
{
    /* Load window geometry: */
    {
        const QRect geo = gEDataManager->selectorWindowGeometry(this);
        LogRel2(("GUI: UIVirtualBoxManager: Restoring geometry to: Origin=%dx%d, Size=%dx%d\n",
                 geo.x(), geo.y(), geo.width(), geo.height()));
        restoreGeometry(geo);
    }
}

void UIVirtualBoxManager::saveSettings()
{
    /* Save window geometry: */
    {
        const QRect geo = currentGeometry();
        LogRel2(("GUI: UIVirtualBoxManager: Saving geometry as: Origin=%dx%d, Size=%dx%d\n",
                 geo.x(), geo.y(), geo.width(), geo.height()));
        gEDataManager->setSelectorWindowGeometry(geo, isCurrentlyMaximized());
    }
}

void UIVirtualBoxManager::cleanupConnections()
{
    /* Honestly we should disconnect everything here,
     * but for now it's enough to disconnect the most critical. */
    m_pWidget->disconnect(this);
}

void UIVirtualBoxManager::cleanupWidgets()
{
    /* Deconfigure central-widget: */
    setCentralWidget(0);
    /* Destroy central-widget: */
    delete m_pWidget;
    m_pWidget = 0;
}

void UIVirtualBoxManager::cleanupMenuBar()
{
#ifdef VBOX_WS_MAC
    /* Cleanup 'Window' menu: */
    UIWindowMenuManager::destroy();
#endif

    /* Destroy action-pool: */
    UIActionPool::destroy(m_pActionPool);
    m_pActionPool = 0;
}

void UIVirtualBoxManager::cleanup()
{
    /* Close the sub-dialogs first: */
    sltCloseVirtualMediumManagerWindow();
    sltCloseHostNetworkManagerWindow();
    sltCloseCloudProfileManagerWindow();
    sltCloseCloudConsoleManagerWindow();

    /* Save settings: */
    saveSettings();

    /* Cleanup: */
    cleanupConnections();
    cleanupWidgets();
    cleanupMenuBar();
}

UIVirtualMachineItem *UIVirtualBoxManager::currentItem() const
{
    return m_pWidget->currentItem();
}

QList<UIVirtualMachineItem*> UIVirtualBoxManager::currentItems() const
{
    return m_pWidget->currentItems();
}

bool UIVirtualBoxManager::isGroupSavingInProgress() const
{
    return m_pWidget->isGroupSavingInProgress();
}

bool UIVirtualBoxManager::isAllItemsOfOneGroupSelected() const
{
    return m_pWidget->isAllItemsOfOneGroupSelected();
}

bool UIVirtualBoxManager::isSingleGroupSelected() const
{
    return m_pWidget->isSingleGroupSelected();
}

bool UIVirtualBoxManager::isSingleLocalGroupSelected() const
{
    return m_pWidget->isSingleLocalGroupSelected();
}

bool UIVirtualBoxManager::isSingleCloudProfileGroupSelected() const
{
    return m_pWidget->isSingleCloudProfileGroupSelected();
}

bool UIVirtualBoxManager::isCloudProfileUpdateInProgress() const
{
    return m_pWidget->isCloudProfileUpdateInProgress();
}

void UIVirtualBoxManager::openAddMachineDialog(const QString &strFileName /* = QString() */)
{
    /* Initialize variables: */
#ifdef VBOX_WS_MAC
    QString strTmpFile = ::darwinResolveAlias(strFileName);
#else
    QString strTmpFile = strFileName;
#endif
    CVirtualBox comVBox = uiCommon().virtualBox();

    /* No file specified: */
    if (strTmpFile.isEmpty())
    {
        QString strBaseFolder = comVBox.GetSystemProperties().GetDefaultMachineFolder();
        QString strTitle = tr("Select a virtual machine file");
        QStringList extensions;
        for (int i = 0; i < VBoxFileExts.size(); ++i)
            extensions << QString("*.%1").arg(VBoxFileExts[i]);
        QString strFilter = tr("Virtual machine files (%1)").arg(extensions.join(" "));
        /* Create open file dialog: */
        QStringList fileNames = QIFileDialog::getOpenFileNames(strBaseFolder, strFilter, this, strTitle, 0, true, true);
        if (!fileNames.isEmpty())
            strTmpFile = fileNames.at(0);
    }

    /* Nothing was chosen? */
    if (strTmpFile.isEmpty())
        return;

    /* Make sure this machine can be opened: */
    CMachine comMachineNew = comVBox.OpenMachine(strTmpFile);
    if (!comVBox.isOk())
    {
        msgCenter().cannotOpenMachine(comVBox, strTmpFile);
        return;
    }

    /* Make sure this machine was NOT registered already: */
    CMachine comMachineOld = comVBox.FindMachine(comMachineNew.GetId().toString());
    if (!comMachineOld.isNull())
    {
        msgCenter().cannotReregisterExistingMachine(strTmpFile, comMachineOld.GetName());
        return;
    }

    /* Register that machine: */
    comVBox.RegisterMachine(comMachineNew);
}

void UIVirtualBoxManager::startUnattendedInstall(CUnattended &comUnattendedInstaller, const UIUnattendedInstallData &unattendedData)
{
    CVirtualBox comVBox = uiCommon().virtualBox();
    CMachine comMachine = comVBox.FindMachine(unattendedData.m_uMachineUid.toString());
    if (comMachine.isNull())
        return;

    if (!QFileInfo(unattendedData.m_strISOPath).exists())
    {
        /// @todo Show a relavant error message here
        return;
    }

    comUnattendedInstaller.SetIsoPath(unattendedData.m_strISOPath);
    checkUnattendedInstallError(comUnattendedInstaller);
    comUnattendedInstaller.SetMachine(comMachine);
    checkUnattendedInstallError(comUnattendedInstaller);
    comUnattendedInstaller.SetUser(unattendedData.m_strUserName);
    comUnattendedInstaller.SetPassword(unattendedData.m_strPassword);
    comUnattendedInstaller.SetHostname(unattendedData.m_strHostname);
    comUnattendedInstaller.SetProductKey(unattendedData.m_strProductKey);
    comUnattendedInstaller.SetInstallGuestAdditions(unattendedData.m_fInstallGuestAdditions);
    comUnattendedInstaller.SetAdditionsIsoPath(unattendedData.m_strGuestAdditionsISOPath);

    comUnattendedInstaller.Prepare();
    checkUnattendedInstallError(comUnattendedInstaller);
    comUnattendedInstaller.ConstructMedia();
    checkUnattendedInstallError(comUnattendedInstaller);
    comUnattendedInstaller.ReconfigureVM();
    checkUnattendedInstallError(comUnattendedInstaller);

    UICommon::LaunchMode enmLaunchMode = UICommon::LaunchMode_Default;
    if (unattendedData.m_fStartHeadless)
        enmLaunchMode = UICommon::LaunchMode_Headless;
    uiCommon().launchMachine(comMachine, enmLaunchMode);
}

void UIVirtualBoxManager::performStartOrShowVirtualMachines(const QList<UIVirtualMachineItem*> &items, UICommon::LaunchMode enmLaunchMode)
{
    /* Do nothing while group saving is in progress: */
    if (isGroupSavingInProgress())
        return;

    /* Compose the list of startable items: */
    QStringList startableMachineNames;
    QList<UIVirtualMachineItem*> startableItems;
    foreach (UIVirtualMachineItem *pItem, items)
    {
        if (isAtLeastOneItemCanBeStarted(QList<UIVirtualMachineItem*>() << pItem))
        {
            startableItems << pItem;
            startableMachineNames << pItem->name();
        }
    }

    /* Initially we have start auto-confirmed: */
    bool fStartConfirmed = true;
    /* But if we have more than one item to start =>
     * We should still ask user for a confirmation: */
    if (startableItems.size() > 1)
        fStartConfirmed = msgCenter().confirmStartMultipleMachines(startableMachineNames.join(", "));

    /* For every item => check if it could be launched: */
    foreach (UIVirtualMachineItem *pItem, items)
    {
        if (   isAtLeastOneItemCanBeShown(QList<UIVirtualMachineItem*>() << pItem)
            || (   isAtLeastOneItemCanBeStarted(QList<UIVirtualMachineItem*>() << pItem)
                && fStartConfirmed))
        {
            /* For local machine: */
            if (pItem->itemType() == UIVirtualMachineItemType_Local)
            {
                /* Fetch item launch mode: */
                UICommon::LaunchMode enmItemLaunchMode = enmLaunchMode;
                if (enmItemLaunchMode == UICommon::LaunchMode_Invalid)
                    enmItemLaunchMode = pItem->isItemRunningHeadless()
                                      ? UICommon::LaunchMode_Separate
                                      : qApp->keyboardModifiers() == Qt::ShiftModifier
                                      ? UICommon::LaunchMode_Headless
                                      : UICommon::LaunchMode_Default;

                /* Launch current VM: */
                CMachine machine = pItem->toLocal()->machine();
                uiCommon().launchMachine(machine, enmItemLaunchMode);
            }
            /* For real cloud machine: */
            else if (pItem->itemType() == UIVirtualMachineItemType_CloudReal)
            {
                /* Acquire cloud machine: */
                CCloudMachine comCloudMachine = pItem->toCloud()->machine();
                /* Launch current VM: */
                uiCommon().launchMachine(comCloudMachine);
                /* Update info in any case: */
                pItem->toCloud()->updateInfoAsync(false /* delayed? */);
            }
        }
    }
}

#ifndef VBOX_WS_WIN
QStringList UIVirtualBoxManager::parseShellArguments(const QString &strArguments)
{
    //printf("start processing arguments\n");

    /* Parse argument string: */
    QStringList arguments;
    QRegExp re("(\"[^\"]+\")|('[^']+')|([^\\s\"']+)");
    int iPosition = 0;
    int iIndex = re.indexIn(strArguments, iPosition);
    while (iIndex != -1)
    {
        /* Get what's the sequence we have: */
        const QString strCap0 = re.cap(0);
        /* Get what's the double-quoted sequence we have: */
        const QString strCap1 = re.cap(1);
        /* Get what's the single-quoted sequence we have: */
        const QString strCap2 = re.cap(2);
        /* Get what's the unquoted sequence we have: */
        const QString strCap3 = re.cap(3);

        /* If new sequence starts where previous ended
         * we are appending new value to previous one, otherwise
         * we are appending new value to argument list itself.. */

        /* Do we have double-quoted sequence? */
        if (!strCap1.isEmpty())
        {
            //printf(" [D] double-quoted sequence starting at: %d\n", iIndex);
            /* Unquote the value and add it to the list: */
            const QString strValue = strCap1.mid(1, strCap1.size() - 2);
            if (!arguments.isEmpty() && iIndex == iPosition)
                arguments.last() += strValue;
            else
                arguments << strValue;
        }
        /* Do we have single-quoted sequence? */
        else if (!strCap2.isEmpty())
        {
            //printf(" [S] single-quoted sequence starting at: %d\n", iIndex);
            /* Unquote the value and add it to the list: */
            const QString strValue = strCap2.mid(1, strCap2.size() - 2);
            if (!arguments.isEmpty() && iIndex == iPosition)
                arguments.last() += strValue;
            else
                arguments << strValue;
        }
        /* Do we have unquoted sequence? */
        else if (!strCap3.isEmpty())
        {
            //printf(" [U] unquoted sequence starting at: %d\n", iIndex);
            /* Value wasn't unquoted, add it to the list: */
            if (!arguments.isEmpty() && iIndex == iPosition)
                arguments.last() += strCap3;
            else
                arguments << strCap3;
        }

        /* Advance position: */
        iPosition = iIndex + strCap0.size();
        /* Search for a next sequence: */
        iIndex = re.indexIn(strArguments, iPosition);
    }

    //printf("arguments processed:\n");
    //foreach (const QString &strArgument, arguments)
    //    printf(" %s\n", strArgument.toUtf8().constData());

    /* Return parsed arguments: */
    return arguments;
}
#endif /* !VBOX_WS_WIN */

void UIVirtualBoxManager::updateMenuGroup(QMenu *pMenu)
{
    /* For single local group selected: */
    if (isSingleLocalGroupSelected())
    {
        /* Populate Group-menu: */
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_S_New));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_S_Add));
        pMenu->addSeparator();
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_S_Rename));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_S_Remove));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_M_MoveToGroup));
        pMenu->addSeparator();
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_M_StartOrShow));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_T_Pause));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_S_Reset));
        pMenu->addMenu(actionPool()->action(UIActionIndexMN_M_Group_M_Close)->menu());
        pMenu->addSeparator();
        pMenu->addMenu(actionPool()->action(UIActionIndexMN_M_Group_M_Tools)->menu());
        pMenu->addSeparator();
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_S_Discard));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_S_ShowLogDialog));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_S_Refresh));
        pMenu->addSeparator();
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_S_ShowInFileManager));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_S_CreateShortcut));
        pMenu->addSeparator();
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_S_Sort));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_T_Search));
    }
    else
    {
        /* Populate Group-menu: */
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_S_New));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_S_Add));
        pMenu->addSeparator();
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_M_StartOrShow));
        pMenu->addMenu(actionPool()->action(UIActionIndexMN_M_Group_M_Console)->menu());
        pMenu->addMenu(actionPool()->action(UIActionIndexMN_M_Group_M_Close)->menu());
        pMenu->addSeparator();
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_S_Discard));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_S_Refresh));
        pMenu->addSeparator();
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_S_Sort));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_T_Search));
    }
}

void UIVirtualBoxManager::updateMenuMachine(QMenu *pMenu)
{
    /* Get first selected item: */
    UIVirtualMachineItem *pItem = currentItem();
    AssertPtrReturnVoid(pItem);

    /* For local machine: */
    if (pItem->itemType() == UIVirtualMachineItemType_Local)
    {
        /* Populate Machine-menu: */
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_New));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Add));
        pMenu->addSeparator();
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Settings));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Clone));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Move));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_ExportToOCI));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Remove));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_M_MoveToGroup));
        pMenu->addSeparator();
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_M_StartOrShow));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_T_Pause));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Reset));
        pMenu->addMenu(actionPool()->action(UIActionIndexMN_M_Machine_M_Close)->menu());
        pMenu->addSeparator();
        pMenu->addMenu(actionPool()->action(UIActionIndexMN_M_Machine_M_Tools)->menu());
        pMenu->addSeparator();
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Discard));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_ShowLogDialog));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Refresh));
        pMenu->addSeparator();
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_ShowInFileManager));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_CreateShortcut));
        pMenu->addSeparator();
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_SortParent));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_T_Search));
    }
    else
    {
        /* Populate Machine-menu: */
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_New));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Add));
        pMenu->addSeparator();
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Settings));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Remove));
        pMenu->addSeparator();
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_M_StartOrShow));
        pMenu->addMenu(actionPool()->action(UIActionIndexMN_M_Machine_M_Console)->menu());
        pMenu->addMenu(actionPool()->action(UIActionIndexMN_M_Machine_M_Close)->menu());
        pMenu->addSeparator();
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Discard));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_Refresh));
        pMenu->addSeparator();
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_S_SortParent));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_T_Search));
    }
}

void UIVirtualBoxManager::updateMenuGroupMoveToGroup(QMenu *pMenu)
{
    const QStringList groups = m_pWidget->possibleGroupsForGroupToMove(m_pWidget->fullGroupName());
    if (!groups.isEmpty())
        pMenu->addSeparator();
    foreach (const QString &strGroupName, groups)
    {
        QString strVisibleGroupName = strGroupName;
        if (strVisibleGroupName.startsWith('/'))
            strVisibleGroupName.remove(0, 1);
        if (strVisibleGroupName.isEmpty())
            strVisibleGroupName = QApplication::translate("UIActionPool", "[Root]", "group");
        QAction *pAction = pMenu->addAction(strVisibleGroupName, this, &UIVirtualBoxManager::sltPerformMachineMoveToSpecificGroup);
        pAction->setProperty("actual_group_name", strGroupName);
    }
}

void UIVirtualBoxManager::updateMenuGroupConsole(QMenu *pMenu)
{
    /* Populate 'Group' / 'Console' menu: */
    pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_M_Console_S_CreateConnection));
    pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_M_Console_S_DeleteConnection));
    pMenu->addSeparator();
    pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Group_M_Console_S_ConfigureApplications));
}

void UIVirtualBoxManager::updateMenuGroupClose(QMenu *)
{
    /* Get selected items: */
    QList<UIVirtualMachineItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    actionPool()->action(UIActionIndexMN_M_Group_M_Close_S_Shutdown)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_M_Close_S_Shutdown, items));
}

void UIVirtualBoxManager::updateMenuMachineMoveToGroup(QMenu *pMenu)
{
    /* Get current item: */
    UIVirtualMachineItem *pItem = currentItem();
    AssertMsgReturnVoid(pItem, ("Current item should be selected!\n"));

    const QStringList groups = m_pWidget->possibleGroupsForMachineToMove(pItem->id());
    if (!groups.isEmpty())
        pMenu->addSeparator();
    foreach (const QString &strGroupName, groups)
    {
        QString strVisibleGroupName = strGroupName;
        if (strVisibleGroupName.startsWith('/'))
            strVisibleGroupName.remove(0, 1);
        if (strVisibleGroupName.isEmpty())
            strVisibleGroupName = QApplication::translate("UIActionPool", "[Root]", "group");
        QAction *pAction = pMenu->addAction(strVisibleGroupName, this, &UIVirtualBoxManager::sltPerformMachineMoveToSpecificGroup);
        pAction->setProperty("actual_group_name", strGroupName);
    }
}

void UIVirtualBoxManager::updateMenuMachineConsole(QMenu *pMenu)
{
    /* Get current item: */
    UIVirtualMachineItem *pItem = currentItem();
    AssertMsgReturnVoid(pItem, ("Current item should be selected!\n"));
    UIVirtualMachineItemCloud *pCloudItem = pItem->toCloud();
    AssertPtrReturnVoid(pCloudItem);

    /* Acquire current cloud machine: */
    CCloudMachine comMachine = pCloudItem->machine();
    const QString strFingerprint = comMachine.GetConsoleConnectionFingerprint();

    /* Populate 'Group' / 'Console' menu: */
    if (strFingerprint.isEmpty())
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_CreateConnection));
    else
    {
        /* Copy fingerprint to clipboard action: */
        const QString strFingerprintCompressed = strFingerprint.size() <= 12
                                               ? strFingerprint
                                               : QString("%1...%2").arg(strFingerprint.left(6), strFingerprint.right(6));
        QAction *pAction = pMenu->addAction(UIIconPool::iconSet(":/cloud_machine_console_copy_connection_fingerprint_16px.png",
                                                                ":/cloud_machine_console_copy_connection_fingerprint_disabled_16px.png"),
                                            QApplication::translate("UIActionPool", "Copy Key Fingerprint (%1)").arg(strFingerprintCompressed),
                                            this, &UIVirtualBoxManager::sltCopyConsoleConnectionFingerprint);
        pAction->setProperty("fingerprint", strFingerprint);

        /* Copy command to clipboard actions: */
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_CopyCommandSerialUnix));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_CopyCommandSerialWindows));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_CopyCommandVNCUnix));
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_CopyCommandVNCWindows));
        pMenu->addSeparator();

        /* Default Connect action: */
        QAction *pDefaultAction = pMenu->addAction(QApplication::translate("UIActionPool", "Connect", "to cloud VM"),
                                                   this, &UIVirtualBoxManager::sltExecuteExternalApplication);
#if defined(VBOX_WS_MAC)
        pDefaultAction->setProperty("path", "open");
#elif defined(VBOX_WS_WIN)
        pDefaultAction->setProperty("path", "powershell");
#elif defined(VBOX_WS_X11)
        const QPair<QString, QString> terminalData = defaultTerminalData();
        pDefaultAction->setProperty("path", terminalData.first);
        pDefaultAction->setProperty("arguments", QString("%1 sh -c").arg(terminalData.second));
#endif

        /* Terminal application/profile action list: */
        const QStringList restrictions = gEDataManager->cloudConsoleManagerRestrictions();
        foreach (const QString strApplicationId, gEDataManager->cloudConsoleManagerApplications())
        {
            const QString strApplicationDefinition = QString("/%1").arg(strApplicationId);
            if (restrictions.contains(strApplicationDefinition))
                continue;
            const QString strApplicationOptions = gEDataManager->cloudConsoleManagerApplication(strApplicationId);
            const QStringList applicationValues = strApplicationOptions.split(',');
            bool fAtLeastOneProfileListed = false;
            foreach (const QString strProfileId, gEDataManager->cloudConsoleManagerProfiles(strApplicationId))
            {
                const QString strProfileDefinition = QString("/%1/%2").arg(strApplicationId, strProfileId);
                if (restrictions.contains(strProfileDefinition))
                    continue;
                const QString strProfileOptions = gEDataManager->cloudConsoleManagerProfile(strApplicationId, strProfileId);
                const QStringList profileValues = strProfileOptions.split(',');
                QAction *pAction = pMenu->addAction(QApplication::translate("UIActionPool",
                                                                            "Connect with %1 (%2)",
                                                                            "with terminal application (profile)")
                                                        .arg(applicationValues.value(0), profileValues.value(0)),
                                                    this, &UIVirtualBoxManager::sltExecuteExternalApplication);
                pAction->setProperty("path", applicationValues.value(1));
                pAction->setProperty("arguments", profileValues.value(1));
                fAtLeastOneProfileListed = true;
            }
            if (!fAtLeastOneProfileListed)
            {
                QAction *pAction = pMenu->addAction(QApplication::translate("UIActionPool",
                                                                            "Connect with %1",
                                                                            "with terminal application")
                                                        .arg(applicationValues.value(0)),
                                                    this, &UIVirtualBoxManager::sltExecuteExternalApplication);
                pAction->setProperty("path", applicationValues.value(1));
                pAction->setProperty("arguments", applicationValues.value(2));
            }
        }
        /* Terminal application configuration tool: */
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_ConfigureApplications));
        pMenu->addSeparator();

        /* Delete connection action finally: */
        pMenu->addAction(actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_DeleteConnection));
    }
}

void UIVirtualBoxManager::updateMenuMachineClose(QMenu *)
{
    /* Get selected items: */
    QList<UIVirtualMachineItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    actionPool()->action(UIActionIndexMN_M_Machine_M_Close_S_Shutdown)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_M_Close_S_Shutdown, items));
}

void UIVirtualBoxManager::updateActionsVisibility()
{
    /* Determine whether Machine or Group menu should be shown at all: */
    const bool fGlobalMenuShown  = m_pWidget->isGlobalItemSelected();
    const bool fGroupMenuShown   = m_pWidget->isGroupItemSelected()   &&  isSingleGroupSelected();
    const bool fMachineMenuShown = m_pWidget->isMachineItemSelected() && !isSingleGroupSelected();
    actionPool()->action(UIActionIndexMN_M_Welcome)->setVisible(fGlobalMenuShown);
    actionPool()->action(UIActionIndexMN_M_Group)->setVisible(fGroupMenuShown);
    actionPool()->action(UIActionIndexMN_M_Machine)->setVisible(fMachineMenuShown);

    /* Determine whether Media menu should be visible: */
    const bool fMediumMenuShown = fGlobalMenuShown && m_pWidget->currentGlobalTool() == UIToolType_Media;
    actionPool()->action(UIActionIndexMN_M_Medium)->setVisible(fMediumMenuShown);
    /* Determine whether Network menu should be visible: */
    const bool fNetworkMenuShown = fGlobalMenuShown && m_pWidget->currentGlobalTool() == UIToolType_Network;
    actionPool()->action(UIActionIndexMN_M_Network)->setVisible(fNetworkMenuShown);
    /* Determine whether Cloud menu should be visible: */
    const bool fCloudMenuShown = fGlobalMenuShown && m_pWidget->currentGlobalTool() == UIToolType_Cloud;
    actionPool()->action(UIActionIndexMN_M_Cloud)->setVisible(fCloudMenuShown);
    /* Determine whether Resources menu should be visible: */
    const bool fResourcesMenuShown = fGlobalMenuShown && m_pWidget->currentGlobalTool() == UIToolType_Resources;
    actionPool()->action(UIActionIndexMN_M_VMResourceMonitor)->setVisible(fResourcesMenuShown);

    /* Determine whether Snapshots menu should be visible: */
    const bool fSnapshotMenuShown = (fMachineMenuShown || fGroupMenuShown) &&
                                    m_pWidget->currentMachineTool() == UIToolType_Snapshots;
    actionPool()->action(UIActionIndexMN_M_Snapshot)->setVisible(fSnapshotMenuShown);
    /* Determine whether Logs menu should be visible: */
    const bool fLogViewerMenuShown = (fMachineMenuShown || fGroupMenuShown) &&
                                     m_pWidget->currentMachineTool() == UIToolType_Logs;
    actionPool()->action(UIActionIndex_M_Log)->setVisible(fLogViewerMenuShown);
    /* Determine whether Performance menu should be visible: */
    const bool fPerformanceMenuShown = (fMachineMenuShown || fGroupMenuShown) &&
                                       m_pWidget->currentMachineTool() == UIToolType_Performance;
    actionPool()->action(UIActionIndex_M_Performance)->setVisible(fPerformanceMenuShown);

    /* Hide action shortcuts: */
    if (!fGlobalMenuShown)
        actionPool()->setShortcutsVisible(UIActionIndexMN_M_Welcome, false);
    if (!fGroupMenuShown)
        actionPool()->setShortcutsVisible(UIActionIndexMN_M_Group, false);
    if (!fMachineMenuShown)
        actionPool()->setShortcutsVisible(UIActionIndexMN_M_Machine, false);

    /* Show action shortcuts: */
    if (fGlobalMenuShown)
        actionPool()->setShortcutsVisible(UIActionIndexMN_M_Welcome, true);
    if (fGroupMenuShown)
        actionPool()->setShortcutsVisible(UIActionIndexMN_M_Group, true);
    if (fMachineMenuShown)
        actionPool()->setShortcutsVisible(UIActionIndexMN_M_Machine, true);
}

void UIVirtualBoxManager::updateActionsAppearance()
{
    /* Get current items: */
    QList<UIVirtualMachineItem*> items = currentItems();

    /* Enable/disable File/Application actions: */
    actionPool()->action(UIActionIndex_M_Application_S_Preferences)->setEnabled(isActionEnabled(UIActionIndex_M_Application_S_Preferences, items));
    actionPool()->action(UIActionIndexMN_M_File_S_ExportAppliance)->setEnabled(isActionEnabled(UIActionIndexMN_M_File_S_ExportAppliance, items));
    actionPool()->action(UIActionIndexMN_M_File_S_ImportAppliance)->setEnabled(isActionEnabled(UIActionIndexMN_M_File_S_ImportAppliance, items));

    /* Enable/disable welcome actions: */
    actionPool()->action(UIActionIndexMN_M_Welcome_S_New)->setEnabled(isActionEnabled(UIActionIndexMN_M_Welcome_S_New, items));
    actionPool()->action(UIActionIndexMN_M_Welcome_S_Add)->setEnabled(isActionEnabled(UIActionIndexMN_M_Welcome_S_Add, items));

    /* Enable/disable group actions: */
    actionPool()->action(UIActionIndexMN_M_Group_S_New)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_S_New, items));
    actionPool()->action(UIActionIndexMN_M_Group_S_Add)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_S_Add, items));
    actionPool()->action(UIActionIndexMN_M_Group_S_Rename)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_S_Rename, items));
    actionPool()->action(UIActionIndexMN_M_Group_S_Remove)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_S_Remove, items));
    actionPool()->action(UIActionIndexMN_M_Group_M_MoveToGroup)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_M_MoveToGroup, items));
    actionPool()->action(UIActionIndexMN_M_Group_T_Pause)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_T_Pause, items));
    actionPool()->action(UIActionIndexMN_M_Group_S_Reset)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_S_Reset, items));
    actionPool()->action(UIActionIndexMN_M_Group_S_Discard)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_S_Discard, items));
    actionPool()->action(UIActionIndexMN_M_Group_S_ShowLogDialog)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_S_ShowLogDialog, items));
    actionPool()->action(UIActionIndexMN_M_Group_S_Refresh)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_S_Refresh, items));
    actionPool()->action(UIActionIndexMN_M_Group_S_ShowInFileManager)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_S_ShowInFileManager, items));
    actionPool()->action(UIActionIndexMN_M_Group_S_CreateShortcut)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_S_CreateShortcut, items));
    actionPool()->action(UIActionIndexMN_M_Group_S_Sort)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_S_Sort, items));

    /* Enable/disable machine actions: */
    actionPool()->action(UIActionIndexMN_M_Machine_S_New)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_S_New, items));
    actionPool()->action(UIActionIndexMN_M_Machine_S_Add)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_S_Add, items));
    actionPool()->action(UIActionIndexMN_M_Machine_S_Settings)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_S_Settings, items));
    actionPool()->action(UIActionIndexMN_M_Machine_S_Clone)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_S_Clone, items));
    actionPool()->action(UIActionIndexMN_M_Machine_S_Move)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_S_Move, items));
    actionPool()->action(UIActionIndexMN_M_Machine_S_ExportToOCI)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_S_ExportToOCI, items));
    actionPool()->action(UIActionIndexMN_M_Machine_S_Remove)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_S_Remove, items));
    actionPool()->action(UIActionIndexMN_M_Machine_M_MoveToGroup)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_M_MoveToGroup, items));
    actionPool()->action(UIActionIndexMN_M_Machine_M_MoveToGroup_S_New)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_M_MoveToGroup_S_New, items));
    actionPool()->action(UIActionIndexMN_M_Machine_T_Pause)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_T_Pause, items));
    actionPool()->action(UIActionIndexMN_M_Machine_S_Reset)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_S_Reset, items));
    actionPool()->action(UIActionIndexMN_M_Machine_S_Discard)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_S_Discard, items));
    actionPool()->action(UIActionIndexMN_M_Machine_S_ShowLogDialog)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_S_ShowLogDialog, items));
    actionPool()->action(UIActionIndexMN_M_Machine_S_Refresh)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_S_Refresh, items));
    actionPool()->action(UIActionIndexMN_M_Machine_S_ShowInFileManager)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_S_ShowInFileManager, items));
    actionPool()->action(UIActionIndexMN_M_Machine_S_CreateShortcut)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_S_CreateShortcut, items));
    actionPool()->action(UIActionIndexMN_M_Machine_S_SortParent)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_S_SortParent, items));

    /* Enable/disable group-start-or-show actions: */
    actionPool()->action(UIActionIndexMN_M_Group_M_StartOrShow)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_M_StartOrShow, items));
    actionPool()->action(UIActionIndexMN_M_Group_M_StartOrShow_S_StartNormal)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_M_StartOrShow_S_StartNormal, items));
    actionPool()->action(UIActionIndexMN_M_Group_M_StartOrShow_S_StartHeadless)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_M_StartOrShow_S_StartHeadless, items));
    actionPool()->action(UIActionIndexMN_M_Group_M_StartOrShow_S_StartDetachable)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_M_StartOrShow_S_StartDetachable, items));

    /* Enable/disable machine-start-or-show actions: */
    actionPool()->action(UIActionIndexMN_M_Machine_M_StartOrShow)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_M_StartOrShow, items));
    actionPool()->action(UIActionIndexMN_M_Machine_M_StartOrShow_S_StartNormal)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_M_StartOrShow_S_StartNormal, items));
    actionPool()->action(UIActionIndexMN_M_Machine_M_StartOrShow_S_StartHeadless)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_M_StartOrShow_S_StartHeadless, items));
    actionPool()->action(UIActionIndexMN_M_Machine_M_StartOrShow_S_StartDetachable)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_M_StartOrShow_S_StartDetachable, items));

    /* Enable/disable group-console actions: */
    actionPool()->action(UIActionIndexMN_M_Group_M_Console)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_M_Console, items));
    actionPool()->action(UIActionIndexMN_M_Group_M_Console_S_CreateConnection)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_M_Console_S_CreateConnection, items));
    actionPool()->action(UIActionIndexMN_M_Group_M_Console_S_DeleteConnection)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_M_Console_S_DeleteConnection, items));
    actionPool()->action(UIActionIndexMN_M_Group_M_Console_S_ConfigureApplications)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_M_Console_S_ConfigureApplications, items));

    /* Enable/disable machine-console actions: */
    actionPool()->action(UIActionIndexMN_M_Machine_M_Console)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_M_Console, items));
    actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_CreateConnection)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_M_Console_S_CreateConnection, items));
    actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_DeleteConnection)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_M_Console_S_DeleteConnection, items));
    actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_CopyCommandSerialUnix)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_M_Console_S_CopyCommandSerialUnix, items));
    actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_CopyCommandSerialWindows)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_M_Console_S_CopyCommandSerialWindows, items));
    actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_CopyCommandVNCUnix)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_M_Console_S_CopyCommandVNCUnix, items));
    actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_CopyCommandVNCWindows)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_M_Console_S_CopyCommandVNCWindows, items));
    actionPool()->action(UIActionIndexMN_M_Machine_M_Console_S_ConfigureApplications)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_M_Console_S_ConfigureApplications, items));

    /* Enable/disable group-close actions: */
    actionPool()->action(UIActionIndexMN_M_Group_M_Close)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_M_Close, items));
    actionPool()->action(UIActionIndexMN_M_Group_M_Close_S_Detach)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_M_Close_S_Detach, items));
    actionPool()->action(UIActionIndexMN_M_Group_M_Close_S_SaveState)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_M_Close_S_SaveState, items));
    actionPool()->action(UIActionIndexMN_M_Group_M_Close_S_Shutdown)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_M_Close_S_Shutdown, items));
    actionPool()->action(UIActionIndexMN_M_Group_M_Close_S_PowerOff)->setEnabled(isActionEnabled(UIActionIndexMN_M_Group_M_Close_S_PowerOff, items));

    /* Enable/disable machine-close actions: */
    actionPool()->action(UIActionIndexMN_M_Machine_M_Close)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_M_Close, items));
    actionPool()->action(UIActionIndexMN_M_Machine_M_Close_S_Detach)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_M_Close_S_Detach, items));
    actionPool()->action(UIActionIndexMN_M_Machine_M_Close_S_SaveState)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_M_Close_S_SaveState, items));
    actionPool()->action(UIActionIndexMN_M_Machine_M_Close_S_Shutdown)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_M_Close_S_Shutdown, items));
    actionPool()->action(UIActionIndexMN_M_Machine_M_Close_S_PowerOff)->setEnabled(isActionEnabled(UIActionIndexMN_M_Machine_M_Close_S_PowerOff, items));

    /* Get current item: */
    UIVirtualMachineItem *pItem = currentItem();

    /* Discard/Terminate action is deremined by 1st item: */
    if (   pItem
        && (   pItem->itemType() == UIVirtualMachineItemType_CloudFake
            || pItem->itemType() == UIVirtualMachineItemType_CloudReal))
    {
        actionPool()->action(UIActionIndexMN_M_Group_S_Discard)->setState(1);
        actionPool()->action(UIActionIndexMN_M_Machine_S_Discard)->setState(1);
    }
    else
    {
        actionPool()->action(UIActionIndexMN_M_Group_S_Discard)->setState(0);
        actionPool()->action(UIActionIndexMN_M_Machine_S_Discard)->setState(0);
    }

    /* Start/Show action is deremined by 1st item: */
    if (pItem && pItem->accessible())
    {
        actionPool()->action(UIActionIndexMN_M_Group_M_StartOrShow)->setState(pItem->isItemPoweredOff() ? 0 : 1);
        actionPool()->action(UIActionIndexMN_M_Machine_M_StartOrShow)->setState(pItem->isItemPoweredOff() ? 0 : 1);
        m_pWidget->updateToolBarMenuButtons(pItem->isItemPoweredOff());
    }
    else
    {
        actionPool()->action(UIActionIndexMN_M_Group_M_StartOrShow)->setState(0);
        actionPool()->action(UIActionIndexMN_M_Machine_M_StartOrShow)->setState(0);
        m_pWidget->updateToolBarMenuButtons(true /* separate menu section? */);
    }

    /* Pause/Resume action is deremined by 1st started item: */
    UIVirtualMachineItem *pFirstStartedAction = 0;
    foreach (UIVirtualMachineItem *pSelectedItem, items)
    {
        if (pSelectedItem->isItemStarted())
        {
            pFirstStartedAction = pSelectedItem;
            break;
        }
    }
    /* Update the group Pause/Resume action appearance: */
    actionPool()->action(UIActionIndexMN_M_Group_T_Pause)->blockSignals(true);
    actionPool()->action(UIActionIndexMN_M_Group_T_Pause)->setChecked(pFirstStartedAction && pFirstStartedAction->isItemPaused());
    actionPool()->action(UIActionIndexMN_M_Group_T_Pause)->retranslateUi();
    actionPool()->action(UIActionIndexMN_M_Group_T_Pause)->blockSignals(false);
    /* Update the machine Pause/Resume action appearance: */
    actionPool()->action(UIActionIndexMN_M_Machine_T_Pause)->blockSignals(true);
    actionPool()->action(UIActionIndexMN_M_Machine_T_Pause)->setChecked(pFirstStartedAction && pFirstStartedAction->isItemPaused());
    actionPool()->action(UIActionIndexMN_M_Machine_T_Pause)->retranslateUi();
    actionPool()->action(UIActionIndexMN_M_Machine_T_Pause)->blockSignals(false);

    /* Update action toggle states: */
    if (m_pWidget)
    {
        switch (m_pWidget->currentMachineTool())
        {
            case UIToolType_Details:
            {
                actionPool()->action(UIActionIndexMN_M_Group_M_Tools_T_Details)->setChecked(true);
                actionPool()->action(UIActionIndexMN_M_Machine_M_Tools_T_Details)->setChecked(true);
                break;
            }
            case UIToolType_Snapshots:
            {
                actionPool()->action(UIActionIndexMN_M_Group_M_Tools_T_Snapshots)->setChecked(true);
                actionPool()->action(UIActionIndexMN_M_Machine_M_Tools_T_Snapshots)->setChecked(true);
                break;
            }
            case UIToolType_Logs:
            {
                actionPool()->action(UIActionIndexMN_M_Group_M_Tools_T_Logs)->setChecked(true);
                actionPool()->action(UIActionIndexMN_M_Machine_M_Tools_T_Logs)->setChecked(true);
                break;
            }
            case UIToolType_Performance:
            {
                actionPool()->action(UIActionIndexMN_M_Group_M_Tools_T_Performance)->setChecked(true);
                actionPool()->action(UIActionIndexMN_M_Machine_M_Tools_T_Performance)->setChecked(true);
                break;
            }
            default:
                break;
        }
    }
}

bool UIVirtualBoxManager::isActionEnabled(int iActionIndex, const QList<UIVirtualMachineItem*> &items)
{
    /* Make sure action pool exists: */
    AssertPtrReturn(actionPool(), false);

    /* Any "opened" action is by definition disabled: */
    if (   actionPool()->action(iActionIndex)
        && actionPool()->action(iActionIndex)->property("opened").toBool())
        return false;

    /* For known *global* action types: */
    switch (iActionIndex)
    {
        case UIActionIndex_M_Application_S_Preferences:
        case UIActionIndexMN_M_File_S_ExportAppliance:
        case UIActionIndexMN_M_File_S_ImportAppliance:
        case UIActionIndexMN_M_Welcome_S_New:
        case UIActionIndexMN_M_Welcome_S_Add:
            return true;
        default:
            break;
    }

    /* No *machine* actions enabled for empty item list: */
    if (items.isEmpty())
        return false;

    /* Get first item: */
    UIVirtualMachineItem *pItem = items.first();

    /* For known *machine* action types: */
    switch (iActionIndex)
    {
        case UIActionIndexMN_M_Group_S_New:
        case UIActionIndexMN_M_Group_S_Add:
        {
            return !isGroupSavingInProgress() &&
                   (isSingleLocalGroupSelected() ||
                    isSingleCloudProfileGroupSelected());
        }
        case UIActionIndexMN_M_Group_S_Sort:
        {
            return !isGroupSavingInProgress() &&
                   isSingleGroupSelected() &&
                   isItemsLocal(items);
        }
        case UIActionIndexMN_M_Group_S_Rename:
        case UIActionIndexMN_M_Group_S_Remove:
        {
            return !isGroupSavingInProgress() &&
                   isSingleGroupSelected() &&
                   isItemsLocal(items) &&
                   isItemsPoweredOff(items);
        }
        case UIActionIndexMN_M_Machine_S_New:
        case UIActionIndexMN_M_Machine_S_Add:
        {
            return !isGroupSavingInProgress();
        }
        case UIActionIndexMN_M_Machine_S_Settings:
        {
            return !isGroupSavingInProgress() &&
                   items.size() == 1 &&
                   pItem->configurationAccessLevel() != ConfigurationAccessLevel_Null &&
                   (m_pWidget->currentMachineTool() != UIToolType_Snapshots ||
                    m_pWidget->isCurrentStateItemSelected());
        }
        case UIActionIndexMN_M_Machine_S_Clone:
        case UIActionIndexMN_M_Machine_S_Move:
        {
            return !isGroupSavingInProgress() &&
                   items.size() == 1 &&
                   pItem->toLocal() &&
                   pItem->isItemEditable();
        }
        case UIActionIndexMN_M_Machine_S_ExportToOCI:
        {
            return items.size() == 1 &&
                   pItem->toLocal();
        }
        case UIActionIndexMN_M_Machine_S_Remove:
        {
            return !isGroupSavingInProgress() &&
                   (isItemsLocal(items) || !isCloudProfileUpdateInProgress()) &&
                   isAtLeastOneItemRemovable(items);
        }
        case UIActionIndexMN_M_Group_M_MoveToGroup:
        case UIActionIndexMN_M_Machine_M_MoveToGroup:
        case UIActionIndexMN_M_Machine_M_MoveToGroup_S_New:
        {
            return !isGroupSavingInProgress() &&
                   isItemsLocal(items) &&
                   isItemsPoweredOff(items);
        }
        case UIActionIndexMN_M_Group_M_StartOrShow:
        case UIActionIndexMN_M_Group_M_StartOrShow_S_StartNormal:
        case UIActionIndexMN_M_Machine_M_StartOrShow:
        case UIActionIndexMN_M_Machine_M_StartOrShow_S_StartNormal:
        {
            return !isGroupSavingInProgress() &&
                   isAtLeastOneItemCanBeStartedOrShown(items) &&
                    (m_pWidget->currentMachineTool() != UIToolType_Snapshots ||
                     m_pWidget->isCurrentStateItemSelected());
        }
        case UIActionIndexMN_M_Group_M_StartOrShow_S_StartHeadless:
        case UIActionIndexMN_M_Group_M_StartOrShow_S_StartDetachable:
        case UIActionIndexMN_M_Machine_M_StartOrShow_S_StartHeadless:
        case UIActionIndexMN_M_Machine_M_StartOrShow_S_StartDetachable:
        {
            return !isGroupSavingInProgress() &&
                   isItemsLocal(items) &&
                   isAtLeastOneItemCanBeStartedOrShown(items) &&
                    (m_pWidget->currentMachineTool() != UIToolType_Snapshots ||
                     m_pWidget->isCurrentStateItemSelected());
        }
        case UIActionIndexMN_M_Group_S_Discard:
        case UIActionIndexMN_M_Machine_S_Discard:
        {
            return !isGroupSavingInProgress() &&
                   isAtLeastOneItemDiscardable(items) &&
                    (m_pWidget->currentMachineTool() != UIToolType_Snapshots ||
                     m_pWidget->isCurrentStateItemSelected());
        }
        case UIActionIndexMN_M_Group_S_ShowLogDialog:
        case UIActionIndexMN_M_Machine_S_ShowLogDialog:
        {
            return isItemsLocal(items) &&
                   isAtLeastOneItemAccessible(items);
        }
        case UIActionIndexMN_M_Group_T_Pause:
        case UIActionIndexMN_M_Machine_T_Pause:
        {
            return isItemsLocal(items) &&
                   isAtLeastOneItemStarted(items);
        }
        case UIActionIndexMN_M_Group_S_Reset:
        case UIActionIndexMN_M_Machine_S_Reset:
        {
            return isItemsLocal(items) &&
                   isAtLeastOneItemRunning(items);
        }
        case UIActionIndexMN_M_Group_S_Refresh:
        case UIActionIndexMN_M_Machine_S_Refresh:
        {
            return isAtLeastOneItemInaccessible(items);
        }
        case UIActionIndexMN_M_Group_S_ShowInFileManager:
        case UIActionIndexMN_M_Machine_S_ShowInFileManager:
        {
            return isItemsLocal(items) &&
                   isAtLeastOneItemAccessible(items);
        }
        case UIActionIndexMN_M_Machine_S_SortParent:
        {
            return !isGroupSavingInProgress() &&
                   isItemsLocal(items);
        }
        case UIActionIndexMN_M_Group_S_CreateShortcut:
        case UIActionIndexMN_M_Machine_S_CreateShortcut:
        {
            return isAtLeastOneItemSupportsShortcuts(items);
        }
        case UIActionIndexMN_M_Group_M_Console:
        case UIActionIndexMN_M_Group_M_Console_S_CreateConnection:
        case UIActionIndexMN_M_Group_M_Console_S_DeleteConnection:
        case UIActionIndexMN_M_Group_M_Console_S_ConfigureApplications:
        case UIActionIndexMN_M_Machine_M_Console:
        case UIActionIndexMN_M_Machine_M_Console_S_CreateConnection:
        case UIActionIndexMN_M_Machine_M_Console_S_DeleteConnection:
        case UIActionIndexMN_M_Machine_M_Console_S_CopyCommandSerialUnix:
        case UIActionIndexMN_M_Machine_M_Console_S_CopyCommandSerialWindows:
        case UIActionIndexMN_M_Machine_M_Console_S_CopyCommandVNCUnix:
        case UIActionIndexMN_M_Machine_M_Console_S_CopyCommandVNCWindows:
        case UIActionIndexMN_M_Machine_M_Console_S_ConfigureApplications:
        {
            return isAtLeastOneItemStarted(items);
        }
        case UIActionIndexMN_M_Group_M_Close:
        case UIActionIndexMN_M_Machine_M_Close:
        {
            return isAtLeastOneItemStarted(items);
        }
        case UIActionIndexMN_M_Group_M_Close_S_Detach:
        case UIActionIndexMN_M_Machine_M_Close_S_Detach:
        {
            return isItemsLocal(items) &&
                   isActionEnabled(UIActionIndexMN_M_Machine_M_Close, items);
        }
        case UIActionIndexMN_M_Group_M_Close_S_SaveState:
        case UIActionIndexMN_M_Machine_M_Close_S_SaveState:
        {
            return isItemsLocal(items) &&
                   isActionEnabled(UIActionIndexMN_M_Machine_M_Close, items);
        }
        case UIActionIndexMN_M_Group_M_Close_S_Shutdown:
        case UIActionIndexMN_M_Machine_M_Close_S_Shutdown:
        {
            return isActionEnabled(UIActionIndexMN_M_Machine_M_Close, items) &&
                   isAtLeastOneItemAbleToShutdown(items);
        }
        case UIActionIndexMN_M_Group_M_Close_S_PowerOff:
        case UIActionIndexMN_M_Machine_M_Close_S_PowerOff:
        {
            return isActionEnabled(UIActionIndexMN_M_Machine_M_Close, items);
        }
        default:
            break;
    }

    /* Unknown actions are disabled: */
    return false;
}

/* static */
bool UIVirtualBoxManager::isItemsLocal(const QList<UIVirtualMachineItem*> &items)
{
    foreach (UIVirtualMachineItem *pItem, items)
        if (!pItem->toLocal())
            return false;
    return true;
}

/* static */
bool UIVirtualBoxManager::isItemsPoweredOff(const QList<UIVirtualMachineItem*> &items)
{
    foreach (UIVirtualMachineItem *pItem, items)
        if (!pItem->isItemPoweredOff())
            return false;
    return true;
}

/* static */
bool UIVirtualBoxManager::isAtLeastOneItemAbleToShutdown(const QList<UIVirtualMachineItem*> &items)
{
    /* Enumerate all the passed items: */
    foreach (UIVirtualMachineItem *pItem, items)
    {
        /* Skip non-running machines: */
        if (!pItem->isItemRunning())
            continue;

        /* For local machine: */
        if (pItem->itemType() == UIVirtualMachineItemType_Local)
        {
            /* Skip session failures: */
            CSession session = uiCommon().openExistingSession(pItem->id());
            if (session.isNull())
                continue;
            /* Skip console failures: */
            CConsole console = session.GetConsole();
            if (console.isNull())
            {
                /* Do not forget to release machine: */
                session.UnlockMachine();
                continue;
            }
            /* Is the guest entered ACPI mode? */
            bool fGuestEnteredACPIMode = console.GetGuestEnteredACPIMode();
            /* Do not forget to release machine: */
            session.UnlockMachine();
            /* True if the guest entered ACPI mode: */
            if (fGuestEnteredACPIMode)
                return true;
        }
        /* For real cloud machine: */
        else if (pItem->itemType() == UIVirtualMachineItemType_CloudReal)
        {
            /* Running cloud VM has it by definition: */
            return true;
        }
    }
    /* False by default: */
    return false;
}

/* static */
bool UIVirtualBoxManager::isAtLeastOneItemSupportsShortcuts(const QList<UIVirtualMachineItem*> &items)
{
    foreach (UIVirtualMachineItem *pItem, items)
    {
        if (   pItem->accessible()
            && pItem->toLocal()
#ifdef VBOX_WS_MAC
            /* On Mac OS X this are real alias files, which don't work with the old legacy xml files. */
            && pItem->toLocal()->settingsFile().endsWith(".vbox", Qt::CaseInsensitive)
#endif
            )
            return true;
    }
    return false;
}

/* static */
bool UIVirtualBoxManager::isAtLeastOneItemAccessible(const QList<UIVirtualMachineItem*> &items)
{
    foreach (UIVirtualMachineItem *pItem, items)
        if (pItem->accessible())
            return true;
    return false;
}

/* static */
bool UIVirtualBoxManager::isAtLeastOneItemInaccessible(const QList<UIVirtualMachineItem*> &items)
{
    foreach (UIVirtualMachineItem *pItem, items)
        if (!pItem->accessible())
            return true;
    return false;
}

/* static */
bool UIVirtualBoxManager::isAtLeastOneItemRemovable(const QList<UIVirtualMachineItem*> &items)
{
    foreach (UIVirtualMachineItem *pItem, items)
        if (pItem->isItemRemovable())
            return true;
    return false;
}

/* static */
bool UIVirtualBoxManager::isAtLeastOneItemCanBeStarted(const QList<UIVirtualMachineItem*> &items)
{
    foreach (UIVirtualMachineItem *pItem, items)
    {
        if (pItem->isItemPoweredOff() && pItem->isItemEditable())
            return true;
    }
    return false;
}

/* static */
bool UIVirtualBoxManager::isAtLeastOneItemCanBeShown(const QList<UIVirtualMachineItem*> &items)
{
    foreach (UIVirtualMachineItem *pItem, items)
    {
        if (   pItem->isItemStarted()
            && pItem->isItemCanBeSwitchedTo())
            return true;
    }
    return false;
}

/* static */
bool UIVirtualBoxManager::isAtLeastOneItemCanBeStartedOrShown(const QList<UIVirtualMachineItem*> &items)
{
    foreach (UIVirtualMachineItem *pItem, items)
    {
        if (   (   pItem->isItemPoweredOff()
                && pItem->isItemEditable())
            || (   pItem->isItemStarted()
                && pItem->isItemCanBeSwitchedTo()))
            return true;
    }
    return false;
}

/* static */
bool UIVirtualBoxManager::isAtLeastOneItemDiscardable(const QList<UIVirtualMachineItem*> &items)
{
    foreach (UIVirtualMachineItem *pItem, items)
        if (   (   pItem->isItemSaved()
                || pItem->itemType() == UIVirtualMachineItemType_CloudReal)
            && pItem->isItemEditable())
            return true;
    return false;
}

/* static */
bool UIVirtualBoxManager::isAtLeastOneItemStarted(const QList<UIVirtualMachineItem*> &items)
{
    foreach (UIVirtualMachineItem *pItem, items)
        if (pItem->isItemStarted())
            return true;
    return false;
}

/* static */
bool UIVirtualBoxManager::isAtLeastOneItemRunning(const QList<UIVirtualMachineItem*> &items)
{
    foreach (UIVirtualMachineItem *pItem, items)
        if (pItem->isItemRunning())
            return true;
    return false;
}

#ifdef VBOX_WS_X11
/* static */
QPair<QString, QString> UIVirtualBoxManager::defaultTerminalData()
{
    /* List known terminals: */
    QStringList knownTerminalNames;
    knownTerminalNames << "gnome-terminal"
                       << "terminator"
                       << "konsole"
                       << "xfce4-terminal"
                       << "mate-terminal"
                       << "lxterminal"
                       << "tilda"
                       << "xterm"
                       << "aterm"
                       << "rxvt-unicode"
                       << "rxvt";

    /* Fill map of known terminal --execute argument exceptions,
     * keep in mind, terminals doesn't mentioned here will be
     * used with default `-e` argument: */
    QMap<QString, QString> knownTerminalArguments;
    knownTerminalArguments["gnome-terminal"] = "--";
    knownTerminalArguments["terminator"] = "-x";
    knownTerminalArguments["xfce4-terminal"] = "-x";
    knownTerminalArguments["mate-terminal"] = "-x";
    knownTerminalArguments["tilda"] = "-c";

    /* Search for a first one suitable through shell command -v test: */
    foreach (const QString &strTerminalName, knownTerminalNames)
    {
        const QString strPath = "sh";
        const QStringList arguments = QStringList() << "-c" << QString("command -v '%1'").arg(strTerminalName);
        QProcess process;
        process.start(strPath, arguments, QIODevice::ReadOnly);
        process.waitForFinished(3000);
        if (process.exitCode() == 0)
        {
            const QString strResult = process.readAllStandardOutput();
            if (strResult.startsWith('/'))
                return qMakePair(strResult.trimmed(), knownTerminalArguments.value(strTerminalName, "-e"));
        }
    }
    return QPair<QString, QString>();
}
#endif


#include "UIVirtualBoxManager.moc"

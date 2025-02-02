/* $Id: UIWizardExportAppPageBasic2.cpp 86687 2020-10-23 13:46:16Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardExportAppPageBasic2 class implementation.
 */

/*
 * Copyright (C) 2009-2020 Oracle Corporation
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
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QRadioButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QVBoxLayout>

/* GUI includes: */
#include "QIRichTextLabel.h"
#include "QIToolButton.h"
#include "UICommon.h"
#include "UIConverter.h"
#include "UIEmptyFilePathSelector.h"
#include "UIIconPool.h"
#include "UIMessageCenter.h"
#include "UIVirtualBoxEventHandler.h"
#include "UIVirtualBoxManager.h"
#include "UIWizardExportApp.h"
#include "UIWizardExportAppDefs.h"
#include "UIWizardExportAppPageBasic2.h"

/* COM includes: */
#include "CMachine.h"
#include "CSystemProperties.h"


/*********************************************************************************************************************************
*   Class UIWizardExportAppPage2 implementation.                                                                                 *
*********************************************************************************************************************************/

UIWizardExportAppPage2::UIWizardExportAppPage2(bool fExportToOCIByDefault)
    : m_fExportToOCIByDefault(fExportToOCIByDefault)
    , m_pFormatLayout(0)
    , m_pSettingsLayout1(0)
    , m_pSettingsLayout2(0)
    , m_pFormatComboBoxLabel(0)
    , m_pFormatComboBox(0)
    , m_pSettingsWidget(0)
    , m_pFileSelectorLabel(0)
    , m_pFileSelector(0)
    , m_pMACComboBoxLabel(0)
    , m_pMACComboBox(0)
    , m_pAdditionalLabel(0)
    , m_pManifestCheckbox(0)
    , m_pIncludeISOsCheckbox(0)
    , m_pProfileLabel(0)
    , m_pProfileComboBox(0)
    , m_pProfileToolButton(0)
    , m_pMachineLabel(0)
    , m_pRadioDoNotAsk(0)
    , m_pRadioAskThenExport(0)
    , m_pRadioExportThenAsk(0)
{
}

void UIWizardExportAppPage2::populateFormats()
{
    AssertReturnVoid(m_pFormatComboBox->count() == 0);

    /* Compose hardcoded format list: */
    QStringList formats;
    formats << "ovf-0.9";
    formats << "ovf-1.0";
    formats << "ovf-2.0";
    /* Add that list to combo: */
    foreach (const QString &strShortName, formats)
    {
        /* Compose empty item, fill it's data: */
        m_pFormatComboBox->addItem(QString());
        m_pFormatComboBox->setItemData(m_pFormatComboBox->count() - 1, strShortName, FormatData_ShortName);
    }

    /* Initialize Cloud Provider Manager: */
    bool fOCIPresent = false;
    CVirtualBox comVBox = uiCommon().virtualBox();
    m_comCloudProviderManager = comVBox.GetCloudProviderManager();
    /* Show error message if necessary: */
    if (!comVBox.isOk())
        msgCenter().cannotAcquireCloudProviderManager(comVBox);
    else
    {
        /* Acquire existing providers: */
        const QVector<CCloudProvider> providers = m_comCloudProviderManager.GetProviders();
        /* Show error message if necessary: */
        if (!m_comCloudProviderManager.isOk())
            msgCenter().cannotAcquireCloudProviderManagerParameter(m_comCloudProviderManager);
        else
        {
            /* Iterate through existing providers: */
            foreach (const CCloudProvider &comProvider, providers)
            {
                /* Skip if we have nothing to populate (file missing?): */
                if (comProvider.isNull())
                    continue;

                /* Compose empty item, fill it's data: */
                m_pFormatComboBox->addItem(QString());
                m_pFormatComboBox->setItemData(m_pFormatComboBox->count() - 1, comProvider.GetId(),        FormatData_ID);
                m_pFormatComboBox->setItemData(m_pFormatComboBox->count() - 1, comProvider.GetName(),      FormatData_Name);
                m_pFormatComboBox->setItemData(m_pFormatComboBox->count() - 1, comProvider.GetShortName(), FormatData_ShortName);
                m_pFormatComboBox->setItemData(m_pFormatComboBox->count() - 1, true,                       FormatData_IsItCloudFormat);
                if (m_pFormatComboBox->itemData(m_pFormatComboBox->count() - 1, FormatData_ShortName).toString() == "OCI")
                    fOCIPresent = true;
            }
        }
    }

    /* Set default: */
    if (m_fExportToOCIByDefault && fOCIPresent)
        setFormat("OCI");
    else
        setFormat("ovf-1.0");
}

void UIWizardExportAppPage2::populateMACAddressPolicies()
{
    AssertReturnVoid(m_pMACComboBox->count() == 0);

    /* Map known export options to known MAC address export policies: */
    QMap<KExportOptions, MACAddressExportPolicy> knownOptions;
    knownOptions[KExportOptions_StripAllMACs] = MACAddressExportPolicy_StripAllMACs;
    knownOptions[KExportOptions_StripAllNonNATMACs] = MACAddressExportPolicy_StripAllNonNATMACs;

    /* Load currently supported export options: */
    CSystemProperties comProperties = uiCommon().virtualBox().GetSystemProperties();
    const QVector<KExportOptions> supportedOptions = comProperties.GetSupportedExportOptions();

    /* Check which of supported options/policies are known: */
    QList<MACAddressExportPolicy> supportedPolicies;
    foreach (const KExportOptions &enmOption, supportedOptions)
        if (knownOptions.contains(enmOption))
            supportedPolicies << knownOptions.value(enmOption);

    /* Add supported policies first: */
    foreach (const MACAddressExportPolicy &enmPolicy, supportedPolicies)
        m_pMACComboBox->addItem(QString(), QVariant::fromValue(enmPolicy));

    /* Add hardcoded policy finally: */
    m_pMACComboBox->addItem(QString(), QVariant::fromValue(MACAddressExportPolicy_KeepAllMACs));

    /* Set default: */
    if (supportedPolicies.contains(MACAddressExportPolicy_StripAllNonNATMACs))
        setMACAddressExportPolicy(MACAddressExportPolicy_StripAllNonNATMACs);
    else
        setMACAddressExportPolicy(MACAddressExportPolicy_KeepAllMACs);
}

void UIWizardExportAppPage2::populateProfiles()
{
    /* Block signals while updating: */
    m_pProfileComboBox->blockSignals(true);

    /* Remember current item data to be able to restore it: */
    QString strOldData;
    if (m_pProfileComboBox->currentIndex() != -1)
        strOldData = m_pProfileComboBox->itemData(m_pProfileComboBox->currentIndex(), ProfileData_Name).toString();

    /* Clear combo initially: */
    m_pProfileComboBox->clear();
    /* Clear Cloud Provider: */
    m_comCloudProvider = CCloudProvider();

    /* If provider chosen: */
    if (!providerId().isNull())
    {
        /* (Re)initialize Cloud Provider: */
        m_comCloudProvider = m_comCloudProviderManager.GetProviderById(providerId());
        /* Show error message if necessary: */
        if (!m_comCloudProviderManager.isOk())
            msgCenter().cannotFindCloudProvider(m_comCloudProviderManager, providerId());
        else
        {
            /* Acquire existing profile names: */
            const QVector<QString> profileNames = m_comCloudProvider.GetProfileNames();
            /* Show error message if necessary: */
            if (!m_comCloudProvider.isOk())
                msgCenter().cannotAcquireCloudProviderParameter(m_comCloudProvider);
            else
            {
                /* Iterate through existing profile names: */
                foreach (const QString &strProfileName, profileNames)
                {
                    /* Skip if we have nothing to show (wtf happened?): */
                    if (strProfileName.isEmpty())
                        continue;

                    /* Compose item, fill it's data: */
                    m_pProfileComboBox->addItem(strProfileName);
                    m_pProfileComboBox->setItemData(m_pProfileComboBox->count() - 1, strProfileName, ProfileData_Name);
                }
            }
        }

        /* Set previous/default item if possible: */
        int iNewIndex = -1;
        if (   iNewIndex == -1
            && !strOldData.isNull())
            iNewIndex = m_pProfileComboBox->findData(strOldData, ProfileData_Name);
        if (   iNewIndex == -1
            && m_pProfileComboBox->count() > 0)
            iNewIndex = 0;
        if (iNewIndex != -1)
            m_pProfileComboBox->setCurrentIndex(iNewIndex);
    }

    /* Unblock signals after update: */
    m_pProfileComboBox->blockSignals(false);
}

void UIWizardExportAppPage2::populateProfile()
{
    /* Clear Cloud Profile: */
    m_comCloudProfile = CCloudProfile();

    /* If both provider and profile chosen: */
    if (!m_comCloudProvider.isNull() && !profileName().isNull())
    {
        /* Acquire Cloud Profile: */
        m_comCloudProfile = m_comCloudProvider.GetProfileByName(profileName());
        /* Show error message if necessary: */
        if (!m_comCloudProvider.isOk())
            msgCenter().cannotFindCloudProfile(m_comCloudProvider, profileName());
    }
}

void UIWizardExportAppPage2::populateFormProperties()
{
    /* Clear appliance: */
    m_comAppliance = CAppliance();
    /* Clear cloud client: */
    m_comClient = CCloudClient();
    /* Clear description: */
    m_comVSD = CVirtualSystemDescription();
    /* Clear description form: */
    m_comVSDExportForm = CVirtualSystemDescriptionForm();

    /* If profile chosen: */
    if (m_comCloudProfile.isNotNull())
    {
        /* Main API request sequence, can be interrupted after any step: */
        do
        {
            /* Perform cloud export procedure for first uuid only: */
            const QList<QUuid> uuids = fieldImp("machineIDs").value<QList<QUuid> >();
            AssertReturnVoid(!uuids.isEmpty());
            const QUuid uMachineId = uuids.first();

            /* Get the machine with the uMachineId: */
            CVirtualBox comVBox = uiCommon().virtualBox();
            CMachine comMachine = comVBox.FindMachine(uMachineId.toString());
            if (!comVBox.isOk())
            {
                msgCenter().cannotFindMachineById(comVBox, uMachineId);
                break;
            }

            /* Create appliance: */
            CAppliance comAppliance = comVBox.CreateAppliance();
            if (!comVBox.isOk())
            {
                msgCenter().cannotCreateAppliance(comVBox);
                break;
            }

            /* Remember appliance: */
            m_comAppliance = comAppliance;

            /* Add the export virtual system description to our appliance object: */
            CVirtualSystemDescription comVSD = comMachine.ExportTo(m_comAppliance, qobject_cast<UIWizardExportApp*>(wizardImp())->uri());
            if (!comMachine.isOk())
            {
                msgCenter().cannotExportAppliance(comMachine, m_comAppliance.GetPath(), thisImp());
                break;
            }

            /* Remember description: */
            m_comVSD = comVSD;

            /* Add Launch Instance flag to virtual system description: */
            switch (cloudExportMode())
            {
                case CloudExportMode_AskThenExport:
                case CloudExportMode_ExportThenAsk:
                    m_comVSD.AddDescription(KVirtualSystemDescriptionType_CloudLaunchInstance, "true", QString());
                    break;
                default:
                    m_comVSD.AddDescription(KVirtualSystemDescriptionType_CloudLaunchInstance, "false", QString());
                    break;
            }
            if (!m_comVSD.isOk())
            {
                msgCenter().cannotAddVirtualSystemDescriptionValue(m_comVSD);
                break;
            }

            /* Create Cloud Client: */
            CCloudClient comClient = m_comCloudProfile.CreateCloudClient();
            if (!m_comCloudProfile.isOk())
            {
                msgCenter().cannotCreateCloudClient(m_comCloudProfile);
                break;
            }

            /* Remember client: */
            m_comClient = comClient;

            /* Read Cloud Client Export description form: */
            CVirtualSystemDescriptionForm comExportForm;
            CProgress comExportDescriptionFormProgress = m_comClient.GetExportDescriptionForm(m_comVSD, comExportForm);
            if (!m_comClient.isOk())
            {
                msgCenter().cannotAcquireCloudClientParameter(m_comClient);
                break;
            }

            /* Show "Acquire export form" progress: */
            msgCenter().showModalProgressDialog(comExportDescriptionFormProgress, UIWizardExportApp::tr("Acquire export form ..."),
                                                ":/progress_refresh_90px.png", 0, 0);
            if (!comExportDescriptionFormProgress.isOk() || comExportDescriptionFormProgress.GetResultCode() != 0)
            {
                msgCenter().cannotAcquireCloudClientParameter(comExportDescriptionFormProgress);
                break;
            }

            /* Remember description form: */
            m_comVSDExportForm = comExportForm;
        }
        while (0);
    }
}

void UIWizardExportAppPage2::updatePageAppearance()
{
    /* Update page appearance according to chosen format: */
    m_pSettingsWidget->setCurrentIndex((int)isFormatCloudOne());
}

void UIWizardExportAppPage2::refreshFileSelectorName()
{
    /* If it's one VM only, we use the VM name as file-name: */
    if (fieldImp("machineNames").toStringList().size() == 1)
        m_strFileSelectorName = fieldImp("machineNames").toStringList()[0];
    /* Otherwise => we use the default file-name: */
    else
        m_strFileSelectorName = m_strDefaultApplianceName;

    /* Cascade update for file selector path: */
    refreshFileSelectorPath();
}

void UIWizardExportAppPage2::refreshFileSelectorExtension()
{
    /* Save old extension to compare afterwards: */
    const QString strOldExtension = m_strFileSelectorExt;

    /* If format is cloud one: */
    if (isFormatCloudOne())
    {
        /* We use no extension: */
        m_strFileSelectorExt.clear();

        /* Update file chooser accordingly: */
        m_pFileSelector->setFileFilters(QString());
    }
    /* Otherwise: */
    else
    {
        /* We use the default (.ova) extension: */
        m_strFileSelectorExt = ".ova";

        /* Update file chooser accordingly: */
        m_pFileSelector->setFileFilters(UIWizardExportApp::tr("Open Virtualization Format Archive (%1)").arg("*.ova") + ";;" +
                                        UIWizardExportApp::tr("Open Virtualization Format (%1)").arg("*.ovf"));
    }

    /* Cascade update for file selector path if necessary: */
    if (m_strFileSelectorExt != strOldExtension)
        refreshFileSelectorPath();
}

void UIWizardExportAppPage2::refreshFileSelectorPath()
{
    /* If format is cloud one: */
    if (isFormatCloudOne())
    {
        /* Clear file selector path: */
        m_pFileSelector->setPath(QString());
    }
    else
    {
        /* Compose file selector path: */
        const QString strPath = QDir::toNativeSeparators(QString("%1/%2")
                                                         .arg(uiCommon().documentsPath())
                                                         .arg(m_strFileSelectorName + m_strFileSelectorExt));
        m_pFileSelector->setPath(strPath);
    }
}

void UIWizardExportAppPage2::refreshManifestCheckBoxAccess()
{
    /* If format is cloud one: */
    if (isFormatCloudOne())
    {
        /* Disable manifest check-box: */
        m_pManifestCheckbox->setChecked(false);
        m_pManifestCheckbox->setEnabled(false);
    }
    /* Otherwise: */
    else
    {
        /* Enable manifest check-box: */
        m_pManifestCheckbox->setChecked(true);
        m_pManifestCheckbox->setEnabled(true);
    }
}

void UIWizardExportAppPage2::refreshIncludeISOsCheckBoxAccess()
{
    /* If format is cloud one: */
    if (isFormatCloudOne())
    {
        /* Disable include ISO check-box: */
        m_pIncludeISOsCheckbox->setChecked(false);
        m_pIncludeISOsCheckbox->setEnabled(false);
    }
    /* Otherwise: */
    else
    {
        /* Enable include ISO check-box: */
        m_pIncludeISOsCheckbox->setEnabled(true);
    }
}

void UIWizardExportAppPage2::updateFormatComboToolTip()
{
    const int iCurrentIndex = m_pFormatComboBox->currentIndex();
    const QString strCurrentToolTip = m_pFormatComboBox->itemData(iCurrentIndex, Qt::ToolTipRole).toString();
    AssertMsg(!strCurrentToolTip.isEmpty(), ("Data not found!"));
    m_pFormatComboBox->setToolTip(strCurrentToolTip);
}

void UIWizardExportAppPage2::updateMACAddressExportPolicyComboToolTip()
{
    const QString strCurrentToolTip = m_pMACComboBox->currentData(Qt::ToolTipRole).toString();
    AssertMsg(!strCurrentToolTip.isEmpty(), ("Data not found!"));
    m_pMACComboBox->setToolTip(strCurrentToolTip);
}

void UIWizardExportAppPage2::setFormat(const QString &strFormat)
{
    const int iIndex = m_pFormatComboBox->findData(strFormat, FormatData_ShortName);
    AssertMsg(iIndex != -1, ("Data not found!"));
    m_pFormatComboBox->setCurrentIndex(iIndex);
}

QString UIWizardExportAppPage2::format() const
{
    const int iIndex = m_pFormatComboBox->currentIndex();
    return m_pFormatComboBox->itemData(iIndex, FormatData_ShortName).toString();
}

bool UIWizardExportAppPage2::isFormatCloudOne(int iIndex /* = -1 */) const
{
    if (iIndex == -1)
        iIndex = m_pFormatComboBox->currentIndex();
    return m_pFormatComboBox->itemData(iIndex, FormatData_IsItCloudFormat).toBool();
}

void UIWizardExportAppPage2::setPath(const QString &strPath)
{
    m_pFileSelector->setPath(strPath);
}

QString UIWizardExportAppPage2::path() const
{
    return m_pFileSelector->path();
}

void UIWizardExportAppPage2::setMACAddressExportPolicy(MACAddressExportPolicy enmMACAddressExportPolicy)
{
    const int iIndex = m_pMACComboBox->findData(enmMACAddressExportPolicy);
    AssertMsg(iIndex != -1, ("Data not found!"));
    m_pMACComboBox->setCurrentIndex(iIndex);
}

MACAddressExportPolicy UIWizardExportAppPage2::macAddressExportPolicy() const
{
    return m_pMACComboBox->currentData().value<MACAddressExportPolicy>();
}

void UIWizardExportAppPage2::setManifestSelected(bool fChecked)
{
    m_pManifestCheckbox->setChecked(fChecked);
}

bool UIWizardExportAppPage2::isManifestSelected() const
{
    return m_pManifestCheckbox->isChecked();
}

void UIWizardExportAppPage2::setIncludeISOsSelected(bool fChecked)
{
    m_pIncludeISOsCheckbox->setChecked(fChecked);
}

bool UIWizardExportAppPage2::isIncludeISOsSelected() const
{
    return m_pIncludeISOsCheckbox->isChecked();
}

void UIWizardExportAppPage2::setProviderById(const QUuid &uId)
{
    const int iIndex = m_pFormatComboBox->findData(uId, FormatData_ID);
    AssertMsg(iIndex != -1, ("Data not found!"));
    m_pFormatComboBox->setCurrentIndex(iIndex);
}

QUuid UIWizardExportAppPage2::providerId() const
{
    const int iIndex = m_pFormatComboBox->currentIndex();
    return m_pFormatComboBox->itemData(iIndex, FormatData_ID).toUuid();
}

QString UIWizardExportAppPage2::providerShortName() const
{
    const int iIndex = m_pFormatComboBox->currentIndex();
    return m_pFormatComboBox->itemData(iIndex, FormatData_ShortName).toString();
}

QString UIWizardExportAppPage2::profileName() const
{
    const int iIndex = m_pProfileComboBox->currentIndex();
    return m_pProfileComboBox->itemData(iIndex, ProfileData_Name).toString();
}

CAppliance UIWizardExportAppPage2::appliance() const
{
    return m_comAppliance;
}

CCloudClient UIWizardExportAppPage2::client() const
{
    return m_comClient;
}

CVirtualSystemDescription UIWizardExportAppPage2::vsd() const
{
    return m_comVSD;
}

CVirtualSystemDescriptionForm UIWizardExportAppPage2::vsdExportForm() const
{
    return m_comVSDExportForm;
}

CloudExportMode UIWizardExportAppPage2::cloudExportMode() const
{
    if (m_pRadioAskThenExport->isChecked())
        return CloudExportMode_AskThenExport;
    else if (m_pRadioExportThenAsk->isChecked())
        return CloudExportMode_ExportThenAsk;
    return CloudExportMode_DoNotAsk;
}


/*********************************************************************************************************************************
*   Class UIWizardExportAppPageBasic2 implementation.                                                                            *
*********************************************************************************************************************************/

UIWizardExportAppPageBasic2::UIWizardExportAppPageBasic2(bool fExportToOCIByDefault)
    : UIWizardExportAppPage2(fExportToOCIByDefault)
    , m_pLabelFormat(0)
    , m_pLabelSettings(0)
{
    /* Create main layout: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    if (pMainLayout)
    {
        /* Create format label: */
        m_pLabelFormat = new QIRichTextLabel;
        if (m_pLabelFormat)
        {
            /* Add into layout: */
            pMainLayout->addWidget(m_pLabelFormat);
        }

        /* Create format layout: */
        m_pFormatLayout = new QGridLayout;
        if (m_pFormatLayout)
        {
#ifdef VBOX_WS_MAC
            m_pFormatLayout->setContentsMargins(0, 10, 0, 10);
            m_pFormatLayout->setSpacing(10);
#else
            m_pFormatLayout->setContentsMargins(0, qApp->style()->pixelMetric(QStyle::PM_LayoutTopMargin),
                                                0, qApp->style()->pixelMetric(QStyle::PM_LayoutBottomMargin));
#endif
            m_pFormatLayout->setColumnStretch(0, 0);
            m_pFormatLayout->setColumnStretch(1, 1);

            /* Create format combo-box: */
            m_pFormatComboBox = new QComboBox;
            if (m_pFormatComboBox)
            {
                /* Add into layout: */
                m_pFormatLayout->addWidget(m_pFormatComboBox, 0, 1);
            }
            /* Create format combo-box label: */
            m_pFormatComboBoxLabel = new QLabel;
            if (m_pFormatComboBoxLabel)
            {
                m_pFormatComboBoxLabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
                m_pFormatComboBoxLabel->setBuddy(m_pFormatComboBox);

                /* Add into layout: */
                m_pFormatLayout->addWidget(m_pFormatComboBoxLabel, 0, 0);
            }

            /* Add into layout: */
            pMainLayout->addLayout(m_pFormatLayout);
        }

        /* Create settings label: */
        m_pLabelSettings = new QIRichTextLabel;
        if (m_pLabelSettings)
        {
            /* Add into layout: */
            pMainLayout->addWidget(m_pLabelSettings);
        }

        /* Create settings layout: */
        m_pSettingsWidget = new QStackedWidget;
        if (m_pSettingsWidget)
        {
            /* Create settings pane 1: */
            QWidget *pSettingsPane1 = new QWidget;
            if (pSettingsPane1)
            {
                /* Create settings layout 1: */
                m_pSettingsLayout1 = new QGridLayout(pSettingsPane1);
                if (m_pSettingsLayout1)
                {
#ifdef VBOX_WS_MAC
                    m_pSettingsLayout1->setContentsMargins(0, 10, 0, 10);
                    m_pSettingsLayout1->setSpacing(10);
#else
                    m_pSettingsLayout1->setContentsMargins(0, qApp->style()->pixelMetric(QStyle::PM_LayoutTopMargin),
                                                           0, qApp->style()->pixelMetric(QStyle::PM_LayoutBottomMargin));
#endif
                    m_pSettingsLayout1->setColumnStretch(0, 0);
                    m_pSettingsLayout1->setColumnStretch(1, 1);

                    /* Create file selector: */
                    m_pFileSelector = new UIEmptyFilePathSelector;
                    if (m_pFileSelector)
                    {
                        m_pFileSelector->setMode(UIEmptyFilePathSelector::Mode_File_Save);
                        m_pFileSelector->setEditable(true);
                        m_pFileSelector->setButtonPosition(UIEmptyFilePathSelector::RightPosition);
                        m_pFileSelector->setDefaultSaveExt("ova");

                        /* Add into layout: */
                        m_pSettingsLayout1->addWidget(m_pFileSelector, 0, 1, 1, 2);
                    }
                    /* Create file selector label: */
                    m_pFileSelectorLabel = new QLabel;
                    if (m_pFileSelectorLabel)
                    {
                        m_pFileSelectorLabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
                        m_pFileSelectorLabel->setBuddy(m_pFileSelector);

                        /* Add into layout: */
                        m_pSettingsLayout1->addWidget(m_pFileSelectorLabel, 0, 0);
                    }

                    /* Create MAC policy combo-box: */
                    m_pMACComboBox = new QComboBox;
                    if (m_pMACComboBox)
                    {
                        /* Add into layout: */
                        m_pSettingsLayout1->addWidget(m_pMACComboBox, 1, 1, 1, 2);
                    }
                    /* Create format combo-box label: */
                    m_pMACComboBoxLabel = new QLabel;
                    if (m_pMACComboBoxLabel)
                    {
                        m_pMACComboBoxLabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
                        m_pMACComboBoxLabel->setBuddy(m_pMACComboBox);

                        /* Add into layout: */
                        m_pSettingsLayout1->addWidget(m_pMACComboBoxLabel, 1, 0);
                    }

                    /* Create advanced label: */
                    m_pAdditionalLabel = new QLabel;
                    if (m_pAdditionalLabel)
                    {
                        m_pAdditionalLabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);

                        /* Add into layout: */
                        m_pSettingsLayout1->addWidget(m_pAdditionalLabel, 2, 0);
                    }
                    /* Create manifest check-box: */
                    m_pManifestCheckbox = new QCheckBox;
                    if (m_pManifestCheckbox)
                    {
                        /* Add into layout: */
                        m_pSettingsLayout1->addWidget(m_pManifestCheckbox, 2, 1);
                    }
                    /* Create include ISOs check-box: */
                    m_pIncludeISOsCheckbox = new QCheckBox;
                    if (m_pIncludeISOsCheckbox)
                    {
                        /* Add into layout: */
                        m_pSettingsLayout1->addWidget(m_pIncludeISOsCheckbox, 3, 1);
                    }

                    /* Create placeholder: */
                    QWidget *pPlaceholder = new QWidget;
                    if (pPlaceholder)
                    {
                        /* Add into layout: */
                        m_pSettingsLayout1->addWidget(pPlaceholder, 4, 0, 1, 3);
                    }
                }

                /* Add into layout: */
                m_pSettingsWidget->addWidget(pSettingsPane1);
            }

            /* Create settings pane 2: */
            QWidget *pSettingsPane2 = new QWidget;
            if (pSettingsPane2)
            {
                /* Create settings layout 2: */
                m_pSettingsLayout2 = new QGridLayout(pSettingsPane2);
                if (m_pSettingsLayout2)
                {
#ifdef VBOX_WS_MAC
                    m_pSettingsLayout2->setContentsMargins(0, 10, 0, 10);
                    m_pSettingsLayout2->setSpacing(10);

#else
                    m_pSettingsLayout2->setContentsMargins(0, qApp->style()->pixelMetric(QStyle::PM_LayoutTopMargin),
                                                           0, qApp->style()->pixelMetric(QStyle::PM_LayoutBottomMargin));
#endif
                    m_pSettingsLayout2->setColumnStretch(0, 0);
                    m_pSettingsLayout2->setColumnStretch(1, 1);
                    m_pSettingsLayout2->setRowStretch(4, 1);

                    /* Create profile label: */
                    m_pProfileLabel = new QLabel;
                    if (m_pProfileLabel)
                    {
                        m_pProfileLabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);

                        /* Add into layout: */
                        m_pSettingsLayout2->addWidget(m_pProfileLabel, 0, 0);
                    }
                    /* Create sub-layout: */
                    QHBoxLayout *pSubLayout = new QHBoxLayout;
                    if (pSubLayout)
                    {
                        pSubLayout->setContentsMargins(0, 0, 0, 0);
                        pSubLayout->setSpacing(1);

                        /* Create profile combo-box: */
                        m_pProfileComboBox = new QComboBox;
                        if (m_pProfileComboBox)
                        {
                            m_pProfileLabel->setBuddy(m_pProfileComboBox);

                            /* Add into layout: */
                            pSubLayout->addWidget(m_pProfileComboBox);
                        }
                        /* Create profile tool-button: */
                        m_pProfileToolButton = new QIToolButton;
                        if (m_pProfileToolButton)
                        {
                            m_pProfileToolButton->setIcon(UIIconPool::iconSet(":/cloud_profile_manager_16px.png",
                                                                              ":/cloud_profile_manager_disabled_16px.png"));

                            /* Add into layout: */
                            pSubLayout->addWidget(m_pProfileToolButton);
                        }

                        /* Add into layout: */
                        m_pSettingsLayout2->addLayout(pSubLayout, 0, 1);
                    }

                    /* Create profile label: */
                    m_pMachineLabel = new QLabel;
                    if (m_pMachineLabel)
                    {
                        m_pMachineLabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);

                        /* Add into layout: */
                        m_pSettingsLayout2->addWidget(m_pMachineLabel, 1, 0);
                    }
                    /* Create Do Not Ask button: */
                    m_pRadioDoNotAsk = new QRadioButton;
                    if (m_pRadioDoNotAsk)
                    {
                        /* Add into layout: */
                        m_pSettingsLayout2->addWidget(m_pRadioDoNotAsk, 1, 1);
                    }
                    /* Create Ask Then Export button: */
                    m_pRadioAskThenExport = new QRadioButton;
                    if (m_pRadioAskThenExport)
                    {
                        /* Add into layout: */
                        m_pSettingsLayout2->addWidget(m_pRadioAskThenExport, 2, 1);
                    }
                    /* Create Export Then Ask button: */
                    m_pRadioExportThenAsk = new QRadioButton;
                    if (m_pRadioExportThenAsk)
                    {
                        /* Add into layout: */
                        m_pSettingsLayout2->addWidget(m_pRadioExportThenAsk, 3, 1);
                    }
                }

                /* Add into layout: */
                m_pSettingsWidget->addWidget(pSettingsPane2);
            }

            /* Add into layout: */
            pMainLayout->addWidget(m_pSettingsWidget);
        }
    }

    /* Populate formats: */
    populateFormats();
    /* Populate MAC address policies: */
    populateMACAddressPolicies();
    /* Populate profiles: */
    populateProfiles();
    /* Populate profile: */
    populateProfile();

    /* Setup connections: */
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigCloudProfileRegistered,
            this, &UIWizardExportAppPageBasic2::sltHandleFormatComboChange);
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigCloudProfileChanged,
            this, &UIWizardExportAppPageBasic2::sltHandleFormatComboChange);
    connect(m_pFileSelector, &UIEmptyFilePathSelector::pathChanged,
            this, &UIWizardExportAppPageBasic2::sltHandleFileSelectorChange);
    connect(m_pFormatComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &UIWizardExportAppPageBasic2::sltHandleFormatComboChange);
    connect(m_pMACComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &UIWizardExportAppPageBasic2::sltHandleMACAddressExportPolicyComboChange);
    connect(m_pProfileComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &UIWizardExportAppPageBasic2::sltHandleProfileComboChange);
    connect(m_pProfileToolButton, &QIToolButton::clicked,
            this, &UIWizardExportAppPageBasic2::sltHandleProfileButtonClick);

    /* Register fields: */
    registerField("format", this, "format");
    registerField("isFormatCloudOne", this, "isFormatCloudOne");
    registerField("path", this, "path");
    registerField("macAddressExportPolicy", this, "macAddressExportPolicy");
    registerField("manifestSelected", this, "manifestSelected");
    registerField("includeISOsSelected", this, "includeISOsSelected");
    registerField("providerShortName", this, "providerShortName");
    registerField("appliance", this, "appliance");
    registerField("client", this, "client");
    registerField("vsd", this, "vsd");
    registerField("vsdExportForm", this, "vsdExportForm");
    registerField("cloudExportMode", this, "cloudExportMode");
}

void UIWizardExportAppPageBasic2::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardExportApp::tr("Appliance settings"));

    /* Translate objects: */
    m_strDefaultApplianceName = UIWizardExportApp::tr("Appliance");

    /* Translate format label: */
    m_pLabelFormat->setText(UIWizardExportApp::
                            tr("<p>Please choose a format to export the virtual appliance to.</p>"
                               "<p>The <b>Open Virtualization Format</b> supports only <b>ovf</b> or <b>ova</b> extensions. "
                               "If you use the <b>ovf</b> extension, several files will be written separately. "
                               "If you use the <b>ova</b> extension, all the files will be combined into one Open "
                               "Virtualization Format archive.</p>"
                               "<p>The <b>Oracle Cloud Infrastructure</b> format supports exporting to remote cloud servers only. "
                               "Main virtual disk of each selected machine will be uploaded to remote server.</p>"));

    /* Translate file selector: */
    m_pFileSelectorLabel->setText(UIWizardExportApp::tr("&File:"));
    m_pFileSelector->setChooseButtonToolTip(UIWizardExportApp::tr("Choose a file to export the virtual appliance to..."));
    m_pFileSelector->setFileDialogTitle(UIWizardExportApp::tr("Please choose a file to export the virtual appliance to"));

    /* Translate hardcoded values of Format combo-box: */
    m_pFormatComboBoxLabel->setText(UIWizardExportApp::tr("F&ormat:"));
    m_pFormatComboBox->setItemText(0, UIWizardExportApp::tr("Open Virtualization Format 0.9"));
    m_pFormatComboBox->setItemText(1, UIWizardExportApp::tr("Open Virtualization Format 1.0"));
    m_pFormatComboBox->setItemText(2, UIWizardExportApp::tr("Open Virtualization Format 2.0"));
    m_pFormatComboBox->setItemData(0, UIWizardExportApp::tr("Write in legacy OVF 0.9 format for compatibility "
                                                            "with other virtualization products."), Qt::ToolTipRole);
    m_pFormatComboBox->setItemData(1, UIWizardExportApp::tr("Write in standard OVF 1.0 format."), Qt::ToolTipRole);
    m_pFormatComboBox->setItemData(2, UIWizardExportApp::tr("Write in new OVF 2.0 format."), Qt::ToolTipRole);
    /* Translate received values of Format combo-box.
     * We are enumerating starting from 0 for simplicity: */
    for (int i = 0; i < m_pFormatComboBox->count(); ++i)
        if (isFormatCloudOne(i))
        {
            m_pFormatComboBox->setItemText(i, m_pFormatComboBox->itemData(i, FormatData_Name).toString());
            m_pFormatComboBox->setItemData(i, UIWizardExportApp::tr("Export to cloud service provider."), Qt::ToolTipRole);
        }

    /* Translate MAC address policy combo-box: */
    m_pMACComboBoxLabel->setText(UIWizardExportApp::tr("MAC Address &Policy:"));
    for (int i = 0; i < m_pMACComboBox->count(); ++i)
    {
        const MACAddressExportPolicy enmPolicy = m_pMACComboBox->itemData(i).value<MACAddressExportPolicy>();
        switch (enmPolicy)
        {
            case MACAddressExportPolicy_KeepAllMACs:
            {
                m_pMACComboBox->setItemText(i, UIWizardExportApp::tr("Include all network adapter MAC addresses"));
                m_pMACComboBox->setItemData(i, UIWizardExportApp::tr("Include all network adapter MAC addresses in exported appliance archive."), Qt::ToolTipRole);
                break;
            }
            case MACAddressExportPolicy_StripAllNonNATMACs:
            {
                m_pMACComboBox->setItemText(i, UIWizardExportApp::tr("Include only NAT network adapter MAC addresses"));
                m_pMACComboBox->setItemData(i, UIWizardExportApp::tr("Include only NAT network adapter MAC addresses in exported appliance archive."), Qt::ToolTipRole);
                break;
            }
            case MACAddressExportPolicy_StripAllMACs:
            {
                m_pMACComboBox->setItemText(i, UIWizardExportApp::tr("Strip all network adapter MAC addresses"));
                m_pMACComboBox->setItemData(i, UIWizardExportApp::tr("Strip all network adapter MAC addresses from exported appliance archive."), Qt::ToolTipRole);
                break;
            }
            default:
                break;
        }
    }

    /* Translate addtional stuff: */
    m_pAdditionalLabel->setText(UIWizardExportApp::tr("Additionally:"));
    m_pManifestCheckbox->setToolTip(UIWizardExportApp::tr("Create a Manifest file for automatic data integrity checks on import."));
    m_pManifestCheckbox->setText(UIWizardExportApp::tr("&Write Manifest file"));
    m_pIncludeISOsCheckbox->setToolTip(UIWizardExportApp::tr("Include ISO image files in exported VM archive."));
    m_pIncludeISOsCheckbox->setText(UIWizardExportApp::tr("&Include ISO image files"));

    /* Translate profile stuff: */
    m_pProfileLabel->setText(UIWizardExportApp::tr("&Profile:"));
    m_pProfileToolButton->setToolTip(UIWizardExportApp::tr("Open Cloud Profile Manager..."));

    /* Translate option label: */
    m_pMachineLabel->setText(UIWizardExportApp::tr("Machine Creation:"));
    m_pRadioDoNotAsk->setText(UIWizardExportApp::tr("Do not ask me about it, leave custom &image for future usage"));
    m_pRadioAskThenExport->setText(UIWizardExportApp::tr("Ask me about it &before exporting disk as custom image"));
    m_pRadioExportThenAsk->setText(UIWizardExportApp::tr("Ask me about it &after exporting disk as custom image"));

    /* Adjust label widths: */
    QList<QWidget*> labels;
    labels << m_pFormatComboBoxLabel;
    labels << m_pFileSelectorLabel;
    labels << m_pMACComboBoxLabel;
    labels << m_pAdditionalLabel;
    labels << m_pProfileLabel;
    labels << m_pMachineLabel;
    int iMaxWidth = 0;
    foreach (QWidget *pLabel, labels)
        iMaxWidth = qMax(iMaxWidth, pLabel->minimumSizeHint().width());
    m_pFormatLayout->setColumnMinimumWidth(0, iMaxWidth);
    m_pSettingsLayout1->setColumnMinimumWidth(0, iMaxWidth);
    m_pSettingsLayout2->setColumnMinimumWidth(0, iMaxWidth);

    /* Refresh file selector name: */
    refreshFileSelectorName();

    /* Update page appearance: */
    updatePageAppearance();

    /* Update tool-tips: */
    updateFormatComboToolTip();
    updateMACAddressExportPolicyComboToolTip();
}

void UIWizardExportAppPageBasic2::initializePage()
{
    /* Translate page: */
    retranslateUi();

    /* Refresh file selector name: */
    // refreshFileSelectorName(); already called from retranslateUi();
    /* Refresh file selector extension: */
    refreshFileSelectorExtension();
    /* Refresh manifest check-box access: */
    refreshManifestCheckBoxAccess();
    /* Refresh include ISOs check-box access: */
    refreshIncludeISOsCheckBoxAccess();

    /* Choose default cloud export option: */
    m_pRadioExportThenAsk->setChecked(true);
}

bool UIWizardExportAppPageBasic2::isComplete() const
{
    /* Initial result: */
    bool fResult = true;

    /* Check whether there was cloud target selected: */
    if (isFormatCloudOne())
        fResult = m_comCloudProfile.isNotNull();
    else
        fResult = UICommon::hasAllowedExtension(path().toLower(), OVFFileExts);

    /* Return result: */
    return fResult;
}

bool UIWizardExportAppPageBasic2::validatePage()
{
    /* Initial result: */
    bool fResult = true;

    /* Check whether there was cloud target selected: */
    if (isFormatCloudOne())
    {
        /* Create appliance and populate form properties: */
        populateFormProperties();
        /* Which are required to continue to the next page: */
        fResult =    field("appliance").value<CAppliance>().isNotNull()
                  && field("client").value<CCloudClient>().isNotNull()
                  && field("vsd").value<CVirtualSystemDescription>().isNotNull()
                  && field("vsdExportForm").value<CVirtualSystemDescriptionForm>().isNotNull();
    }

    /* Return result: */
    return fResult;
}

void UIWizardExportAppPageBasic2::updatePageAppearance()
{
    /* Call to base-class: */
    UIWizardExportAppPage2::updatePageAppearance();

    /* Update page appearance according to chosen storage-type: */
    if (isFormatCloudOne())
    {
        m_pLabelSettings->setText(UIWizardExportApp::
                                  tr("<p>Please choose one of cloud service profiles you have registered to export virtual "
                                     "machines to. It will be used to establish network connection required to upload your "
                                     "virtual machine files to a remote cloud facility.</p>"));
        m_pProfileComboBox->setFocus();
    }
    else
    {
        m_pLabelSettings->setText(UIWizardExportApp::
                                  tr("<p>Please choose a filename to export the virtual appliance to. Besides that you can "
                                     "specify a certain amount of options which affects the size and content of resulting "
                                     "archive.</p>"));
        m_pFileSelector->setFocus();
    }
}

void UIWizardExportAppPageBasic2::sltHandleFormatComboChange()
{
    /* Update tool-tip: */
    updateFormatComboToolTip();

    /* Refresh required settings: */
    updatePageAppearance();
    refreshFileSelectorExtension();
    refreshManifestCheckBoxAccess();
    refreshIncludeISOsCheckBoxAccess();
    populateProfiles();
    populateProfile();
    emit completeChanged();
}

void UIWizardExportAppPageBasic2::sltHandleFileSelectorChange()
{
    /* Remember changed name, except empty one: */
    if (!m_pFileSelector->path().isEmpty())
        m_strFileSelectorName = QFileInfo(m_pFileSelector->path()).completeBaseName();

    /* Refresh required settings: */
    emit completeChanged();
}

void UIWizardExportAppPageBasic2::sltHandleMACAddressExportPolicyComboChange()
{
    /* Update tool-tip: */
    updateMACAddressExportPolicyComboToolTip();
}

void UIWizardExportAppPageBasic2::sltHandleProfileComboChange()
{
    /* Refresh required settings: */
    populateProfile();
}

void UIWizardExportAppPageBasic2::sltHandleProfileButtonClick()
{
    /* Open Cloud Profile Manager: */
    if (gpManager)
        gpManager->openCloudProfileManager();
}

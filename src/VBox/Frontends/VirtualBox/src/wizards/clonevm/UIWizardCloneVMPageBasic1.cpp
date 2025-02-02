/* $Id: UIWizardCloneVMPageBasic1.cpp 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVMPageBasic1 class implementation.
 */

/*
 * Copyright (C) 2011-2020 Oracle Corporation
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
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>

/* GUI includes: */
#include "QIRichTextLabel.h"
#include "QILineEdit.h"
#include "UIFilePathSelector.h"
#include "UIWizardCloneVM.h"
#include "UIWizardCloneVMPageBasic1.h"
#include "UICommon.h"

/* COM includes: */
#include "CSystemProperties.h"
#include "CVirtualBox.h"


UIWizardCloneVMPage1::UIWizardCloneVMPage1(const QString &strOriginalName, const QString &strDefaultPath, const QString &strGroup)
    : m_strOriginalName(strOriginalName)
    , m_strDefaultPath(strDefaultPath)
    , m_strGroup(strGroup)
    , m_pNameLineEdit(0)
    , m_pPathSelector(0)
    , m_pNameLabel(0)
    , m_pPathLabel(0)
    , m_pMACComboBoxLabel(0)
    , m_pMACComboBox(0)
    , m_pKeepDiskNamesCheckBox(0)
    , m_pKeepHWUUIDsCheckBox(0)
{
}

QString UIWizardCloneVMPage1::cloneName() const
{
    if (!m_pNameLineEdit)
        return QString();
    return m_pNameLineEdit->text();
}

void UIWizardCloneVMPage1::setCloneName(const QString &strName)
{
    if (!m_pNameLineEdit)
        return;
    m_pNameLineEdit->setText(strName);
}

QString UIWizardCloneVMPage1::clonePath() const
{
    if (!m_pPathSelector)
        return QString();
    return m_pPathSelector->path();
}

void UIWizardCloneVMPage1::setClonePath(const QString &strPath)
{
    if (!m_pPathSelector)
        m_pPathSelector->setPath(strPath);
}

QString UIWizardCloneVMPage1::cloneFilePath() const
{
    return m_strCloneFilePath;
}

void UIWizardCloneVMPage1::setCloneFilePath(const QString &path)
{
    if (m_strCloneFilePath == path)
        return;
    m_strCloneFilePath = path;
}

void UIWizardCloneVMPage1::composeCloneFilePath()
{
    CVirtualBox vbox = uiCommon().virtualBox();
    setCloneFilePath(vbox.ComposeMachineFilename(m_pNameLineEdit ? m_pNameLineEdit->text() : QString(),
                                                 m_strGroup,
                                                 QString::null,
                                                 m_pPathSelector ? m_pPathSelector->path() : QString()));
    const QFileInfo fileInfo(m_strCloneFilePath);
    m_strCloneFolder = fileInfo.absolutePath();
}

void UIWizardCloneVMPage1::updateMACAddressClonePolicyComboToolTip()
{
    const QString strCurrentToolTip = m_pMACComboBox->currentData(Qt::ToolTipRole).toString();
    AssertMsg(!strCurrentToolTip.isEmpty(), ("Tool-tip data not found!"));
    m_pMACComboBox->setToolTip(strCurrentToolTip);
}

MACAddressClonePolicy UIWizardCloneVMPage1::macAddressClonePolicy() const
{
    return m_pMACComboBox->currentData().value<MACAddressClonePolicy>();
}

void UIWizardCloneVMPage1::setMACAddressClonePolicy(MACAddressClonePolicy enmMACAddressClonePolicy)
{
    const int iIndex = m_pMACComboBox->findData(enmMACAddressClonePolicy);
    AssertMsg(iIndex != -1, ("Data not found!"));
    m_pMACComboBox->setCurrentIndex(iIndex);
}

void UIWizardCloneVMPage1::populateMACAddressClonePolicies()
{
    AssertReturnVoid(m_pMACComboBox->count() == 0);

    /* Map known clone options to known MAC address export policies: */
    QMap<KCloneOptions, MACAddressClonePolicy> knownOptions;
    knownOptions[KCloneOptions_KeepAllMACs] = MACAddressClonePolicy_KeepAllMACs;
    knownOptions[KCloneOptions_KeepNATMACs] = MACAddressClonePolicy_KeepNATMACs;

    /* Load currently supported clone options: */
    CSystemProperties comProperties = uiCommon().virtualBox().GetSystemProperties();
    const QVector<KCloneOptions> supportedOptions = comProperties.GetSupportedCloneOptions();

    /* Check which of supported options/policies are known: */
    QList<MACAddressClonePolicy> supportedPolicies;
    foreach (const KCloneOptions &enmOption, supportedOptions)
        if (knownOptions.contains(enmOption))
            supportedPolicies << knownOptions.value(enmOption);

    /* Add supported policies first: */
    foreach (const MACAddressClonePolicy &enmPolicy, supportedPolicies)
        m_pMACComboBox->addItem(QString(), QVariant::fromValue(enmPolicy));

    /* Add hardcoded policy finally: */
    m_pMACComboBox->addItem(QString(), QVariant::fromValue(MACAddressClonePolicy_StripAllMACs));

    /* Set default: */
    if (supportedPolicies.contains(MACAddressClonePolicy_KeepNATMACs))
        setMACAddressClonePolicy(MACAddressClonePolicy_KeepNATMACs);
    else
        setMACAddressClonePolicy(MACAddressClonePolicy_StripAllMACs);
}

bool UIWizardCloneVMPage1::keepDiskNames() const
{
    if (!m_pKeepDiskNamesCheckBox)
        return false;
    return m_pKeepDiskNamesCheckBox->isChecked();
}

void UIWizardCloneVMPage1::setKeepDiskNames(bool fKeepDiskNames)
{
    if (!m_pKeepDiskNamesCheckBox)
        return;
    if (m_pKeepDiskNamesCheckBox->isChecked() == fKeepDiskNames)
        return;
    m_pKeepDiskNamesCheckBox->setChecked(fKeepDiskNames);
}

bool UIWizardCloneVMPage1::keepHWUUIDs() const
{
    if (!m_pKeepHWUUIDsCheckBox)
        return false;
    return m_pKeepHWUUIDsCheckBox->isChecked();
}

void UIWizardCloneVMPage1::setKeepHWUUIDs(bool fKeepHWUUIDs)
{
    if (!m_pKeepHWUUIDsCheckBox)
        return;
    if (m_pKeepHWUUIDsCheckBox->isChecked() == fKeepHWUUIDs)
        return;
    m_pKeepHWUUIDsCheckBox->setChecked(fKeepHWUUIDs);
}


UIWizardCloneVMPageBasic1::UIWizardCloneVMPageBasic1(const QString &strOriginalName, const QString &strDefaultPath, const QString &strGroup)
    : UIWizardCloneVMPage1(strOriginalName, strDefaultPath, strGroup)
    , m_pMainLabel(0)
    , m_pContainerLayout(0)
    , m_pAdditionalOptionsLabel(0)
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    if (!pMainLayout)
        return;

    m_pMainLabel = new QIRichTextLabel(this);
    if (m_pMainLabel)
    {
        pMainLayout->addWidget(m_pMainLabel);
    }

    QWidget *pContainerWidget = new QWidget(this);
    if (pContainerWidget)
    {
        pMainLayout->addWidget(pContainerWidget);
        m_pContainerLayout = new QGridLayout(pContainerWidget);
        m_pContainerLayout->setContentsMargins(0, 0, 0, 0);

        m_pNameLabel = new QLabel;
        if (m_pNameLabel)
        {
            m_pNameLabel->setAlignment(Qt::AlignRight);
            m_pNameLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
            m_pContainerLayout->addWidget(m_pNameLabel, 0, 0, 1, 1);
        }

        m_pNameLineEdit = new QILineEdit();
        if (m_pNameLineEdit)
        {
            m_pContainerLayout->addWidget(m_pNameLineEdit, 0, 1, 1, 1);
            m_pNameLineEdit->setText(UIWizardCloneVM::tr("%1 Clone").arg(m_strOriginalName));
        }

        m_pPathLabel = new QLabel(this);
        if (m_pPathLabel)
        {
            m_pPathLabel->setAlignment(Qt::AlignRight);
            m_pPathLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
            m_pContainerLayout->addWidget(m_pPathLabel, 1, 0, 1, 1);
        }

        m_pPathSelector = new UIFilePathSelector(this);
        if (m_pPathSelector)
        {
            m_pContainerLayout->addWidget(m_pPathSelector, 1, 1, 1, 1);
            m_pPathSelector->setPath(m_strDefaultPath);
        }

        /* Create MAC policy combo-box: */
        m_pMACComboBox = new QComboBox;
        if (m_pMACComboBox)
        {
            /* Add into layout: */
            m_pContainerLayout->addWidget(m_pMACComboBox, 2, 1, 1, 1);
        }

        /* Create format combo-box label: */
        m_pMACComboBoxLabel = new QLabel;
        if (m_pMACComboBoxLabel)
        {
            m_pMACComboBoxLabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
            m_pMACComboBoxLabel->setBuddy(m_pMACComboBox);
            /* Add into layout: */
            m_pContainerLayout->addWidget(m_pMACComboBoxLabel, 2, 0, 1, 1);
        }

        /* Load currently supported clone options: */
        CSystemProperties comProperties = uiCommon().virtualBox().GetSystemProperties();
        const QVector<KCloneOptions> supportedOptions = comProperties.GetSupportedCloneOptions();
        /* Check whether we support additional clone options at all: */
        int iVerticalPosition = 3;
        const bool fSupportedKeepDiskNames = supportedOptions.contains(KCloneOptions_KeepDiskNames);
        const bool fSupportedKeepHWUUIDs = supportedOptions.contains(KCloneOptions_KeepHwUUIDs);
        if (fSupportedKeepDiskNames || fSupportedKeepHWUUIDs)
        {
            m_pAdditionalOptionsLabel = new QLabel;
            if (m_pAdditionalOptionsLabel)
            {
                m_pAdditionalOptionsLabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
                m_pContainerLayout->addWidget(m_pAdditionalOptionsLabel, iVerticalPosition, 0, 1, 1);
            }
        }
        if (fSupportedKeepDiskNames)
        {
            m_pKeepDiskNamesCheckBox = new QCheckBox;
            if (m_pKeepDiskNamesCheckBox)
                m_pContainerLayout->addWidget(m_pKeepDiskNamesCheckBox, iVerticalPosition++, 1, 1, 1);
        }
        if (fSupportedKeepHWUUIDs)
        {
            m_pKeepHWUUIDsCheckBox = new QCheckBox;
            if (m_pKeepHWUUIDsCheckBox)
                m_pContainerLayout->addWidget(m_pKeepHWUUIDsCheckBox, iVerticalPosition++, 1, 1, 1);
        }
    }
    pMainLayout->addStretch();

    /* Populate MAC address policies: */
    populateMACAddressClonePolicies();

    /* Register fields: */
    registerField("cloneName", this, "cloneName");
    registerField("cloneFilePath", this, "cloneFilePath");
    registerField("macAddressClonePolicy", this, "macAddressClonePolicy");
    registerField("keepDiskNames", this, "keepDiskNames");
    registerField("keepHWUUIDs", this, "keepHWUUIDs");

    composeCloneFilePath();

    /* Setup connections: */
    connect(m_pNameLineEdit, &QILineEdit::textChanged, this, &UIWizardCloneVMPageBasic1::completeChanged);
    connect(m_pPathSelector, &UIFilePathSelector::pathChanged, this, &UIWizardCloneVMPageBasic1::completeChanged);

    connect(m_pNameLineEdit, &QILineEdit::textChanged, this, &UIWizardCloneVMPageBasic1::sltNameChanged);
    connect(m_pPathSelector, &UIFilePathSelector::pathChanged, this, &UIWizardCloneVMPageBasic1::sltPathChanged);
    connect(m_pMACComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &UIWizardCloneVMPageBasic1::sltHandleMACAddressClonePolicyComboChange);
}

void UIWizardCloneVMPageBasic1::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardCloneVM::tr("New machine name and path"));

    /* Translate widgets: */
    if (m_pMainLabel)
        m_pMainLabel->setText(UIWizardCloneVM::tr("<p>Please choose a name and optionally a folder for the new virtual machine. "
                                                  "The new machine will be a clone of the machine <b>%1</b>.</p>")
                              .arg(m_strOriginalName));

    if (m_pNameLabel)
        m_pNameLabel->setText(UIWizardCloneVM::tr("Name:"));

    if (m_pPathLabel)
        m_pPathLabel->setText(UIWizardCloneVM::tr("Path:"));

    /* Translate MAC address policy combo-box: */
    m_pMACComboBoxLabel->setText(UIWizardCloneVM::tr("MAC Address &Policy:"));
    for (int i = 0; i < m_pMACComboBox->count(); ++i)
    {
        const MACAddressClonePolicy enmPolicy = m_pMACComboBox->itemData(i).value<MACAddressClonePolicy>();
        switch (enmPolicy)
        {
            case MACAddressClonePolicy_KeepAllMACs:
            {
                m_pMACComboBox->setItemText(i, UIWizardCloneVM::tr("Include all network adapter MAC addresses"));
                m_pMACComboBox->setItemData(i, UIWizardCloneVM::tr("Include all network adapter MAC addresses during cloning."), Qt::ToolTipRole);
                break;
            }
            case MACAddressClonePolicy_KeepNATMACs:
            {
                m_pMACComboBox->setItemText(i, UIWizardCloneVM::tr("Include only NAT network adapter MAC addresses"));
                m_pMACComboBox->setItemData(i, UIWizardCloneVM::tr("Include only NAT network adapter MAC addresses during cloning."), Qt::ToolTipRole);
                break;
            }
            case MACAddressClonePolicy_StripAllMACs:
            {
                m_pMACComboBox->setItemText(i, UIWizardCloneVM::tr("Generate new MAC addresses for all network adapters"));
                m_pMACComboBox->setItemData(i, UIWizardCloneVM::tr("Generate new MAC addresses for all network adapters during cloning."), Qt::ToolTipRole);
                break;
            }
            default:
                break;
        }
    }

    if (m_pAdditionalOptionsLabel)
        m_pAdditionalOptionsLabel->setText(UIWizardCloneVM::tr("Additional Options:"));
    if (m_pKeepDiskNamesCheckBox)
    {
        m_pKeepDiskNamesCheckBox->setToolTip(UIWizardCloneVM::tr("Don't change the disk names during cloning."));
        m_pKeepDiskNamesCheckBox->setText(UIWizardCloneVM::tr("Keep &Disk Names"));
    }
    if (m_pKeepHWUUIDsCheckBox)
    {
        m_pKeepHWUUIDsCheckBox->setToolTip(UIWizardCloneVM::tr("Don't change hardware UUIDs during cloning."));
        m_pKeepHWUUIDsCheckBox->setText(UIWizardCloneVM::tr("Keep &Hardware UUIDs"));
    }
}

void UIWizardCloneVMPageBasic1::initializePage()
{
    /* Translate page: */
    retranslateUi();
    if (m_pNameLineEdit)
        m_pNameLineEdit->setFocus();
}

bool UIWizardCloneVMPageBasic1::isComplete() const
{
    if (!m_pPathSelector)
        return false;

    QString path = m_pPathSelector->path();
    if (path.isEmpty())
        return false;
    /* Make sure VM name feat the rules: */
    QString strName = m_pNameLineEdit->text().trimmed();
    return !strName.isEmpty() && strName != m_strOriginalName;
}

void UIWizardCloneVMPageBasic1::sltNameChanged()
{
    composeCloneFilePath();
}

void UIWizardCloneVMPageBasic1::sltPathChanged()
{
    composeCloneFilePath();
}

void UIWizardCloneVMPageBasic1::sltHandleMACAddressClonePolicyComboChange()
{
    /* Update tool-tip: */
    updateMACAddressClonePolicyComboToolTip();
}

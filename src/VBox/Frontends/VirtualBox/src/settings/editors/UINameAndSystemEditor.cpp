/* $Id: UINameAndSystemEditor.cpp 86085 2020-09-10 13:57:52Z vboxsync $ */
/** @file
 * VBox Qt GUI - UINameAndSystemEditor class implementation.
 */

/*
 * Copyright (C) 2008-2020 Oracle Corporation
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
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>

/* GUI includes: */
#include "QILineEdit.h"
#include "UICommon.h"
#include "UIFilePathSelector.h"
#include "UINameAndSystemEditor.h"

/* COM includes: */
#include "CSystemProperties.h"


/** Defines the VM OS type ID. */
enum
{
    TypeID = Qt::UserRole + 1
};


UINameAndSystemEditor::UINameAndSystemEditor(QWidget *pParent,
                                             bool fChooseName /* = true */,
                                             bool fChoosePath /* = false */,
                                             bool fChooseType /* = true */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_fChooseName(fChooseName)
    , m_fChoosePath(fChoosePath)
    , m_fChooseType(fChooseType)
    , m_fSupportsHWVirtEx(false)
    , m_fSupportsLongMode(false)
    , m_pMainLayout(0)
    , m_pNameLabel(0)
    , m_pPathLabel(0)
    , m_pLabelFamily(0)
    , m_pLabelType(0)
    , m_pIconType(0)
    , m_pNameLineEdit(0)
    , m_pPathSelector(0)
    , m_pComboFamily(0)
    , m_pComboType(0)
{
    prepare();
}

void UINameAndSystemEditor::setMinimumLayoutIndent(int iIndent)
{
    if (m_pMainLayout)
        m_pMainLayout->setColumnMinimumWidth(0, iIndent);
}

void UINameAndSystemEditor::setNameStuffEnabled(bool fEnabled)
{
    if (m_pNameLabel)
        m_pNameLabel->setEnabled(fEnabled);
    if (m_pNameLineEdit)
        m_pNameLineEdit->setEnabled(fEnabled);
}

void UINameAndSystemEditor::setPathStuffEnabled(bool fEnabled)
{
    if (m_pPathLabel)
        m_pPathLabel->setEnabled(fEnabled);
    if (m_pPathSelector)
        m_pPathSelector->setEnabled(fEnabled);
}

void UINameAndSystemEditor::setOSTypeStuffEnabled(bool fEnabled)
{
    if (m_pLabelFamily)
        m_pLabelFamily->setEnabled(fEnabled);
    if (m_pLabelType)
        m_pLabelType->setEnabled(fEnabled);
    if (m_pIconType)
        m_pIconType->setEnabled(fEnabled);
    if (m_pComboFamily)
        m_pComboFamily->setEnabled(fEnabled);
    if (m_pComboType)
        m_pComboType->setEnabled(fEnabled);
}

void UINameAndSystemEditor::setName(const QString &strName)
{
    if (!m_pNameLineEdit)
        return;
    m_pNameLineEdit->setText(strName);
}

QString UINameAndSystemEditor::name() const
{
    if (!m_pNameLineEdit)
        return QString();
    return m_pNameLineEdit->text();
}

void UINameAndSystemEditor::setPath(const QString &strPath)
{
    if (!m_pPathSelector)
        return;
    m_pPathSelector->setPath(strPath);
}

QString UINameAndSystemEditor::path() const
{
    if (!m_pPathSelector)
        return uiCommon().virtualBox().GetSystemProperties().GetDefaultMachineFolder();
    return m_pPathSelector->path();
}

void UINameAndSystemEditor::setTypeId(QString strTypeId, QString strFamilyId /* = QString() */)
{
    if (!m_pComboType)
        return;
    AssertMsgReturnVoid(!strTypeId.isNull(), ("Null guest OS type ID"));

    /* Initialize indexes: */
    int iTypeIndex = -1;
    int iFamilyIndex = -1;

    /* If family ID isn't empty: */
    if (!strFamilyId.isEmpty())
    {
        /* Search for corresponding family ID index: */
        iFamilyIndex = m_pComboFamily->findData(strFamilyId, TypeID);

        /* If that family ID isn't present, we have to add it: */
        if (iFamilyIndex == -1)
        {
            /* Append family ID to corresponding combo: */
            m_pComboFamily->addItem(strFamilyId);
            m_pComboFamily->setItemData(m_pComboFamily->count() - 1, strFamilyId, TypeID);
            /* Append family ID to type cache: */
            m_types[strFamilyId] = QList<UIGuestOSType>();

            /* Search for corresponding family ID index finally: */
            iFamilyIndex = m_pComboFamily->findData(strFamilyId, TypeID);
        }
    }
    /* If family ID is empty: */
    else
    {
        /* We'll try to find it by type ID: */
        foreach (const QString &strKnownFamilyId, m_types.keys())
        {
            foreach (const UIGuestOSType &guiType, m_types.value(strKnownFamilyId))
            {
                if (guiType.typeId == strTypeId)
                    strFamilyId = strKnownFamilyId;
                if (!strFamilyId.isNull())
                    break;
            }
            if (!strFamilyId.isNull())
                break;
        }

        /* If we were unable to find it => use "Other": */
        if (strFamilyId.isNull())
            strFamilyId = "Other";

        /* Search for corresponding family ID index finally: */
        iFamilyIndex = m_pComboFamily->findData(strFamilyId, TypeID);
    }

    /* To that moment family ID index should be always found: */
    AssertReturnVoid(iFamilyIndex != -1);
    /* So we choose it: */
    m_pComboFamily->setCurrentIndex(iFamilyIndex);
    sltFamilyChanged(m_pComboFamily->currentIndex());

    /* Search for corresponding type ID index: */
    iTypeIndex = m_pComboType->findData(strTypeId, TypeID);

    /* If that type ID isn't present, we have to add it: */
    if (iTypeIndex == -1)
    {
        /* Append type ID to type cache: */
        UIGuestOSType guiType;
        guiType.typeId = strTypeId;
        guiType.typeDescription = strTypeId;
        guiType.is64bit = false;
        m_types[strFamilyId] << guiType;

        /* So we re-choose family again: */
        m_pComboFamily->setCurrentIndex(iFamilyIndex);
        sltFamilyChanged(m_pComboFamily->currentIndex());

        /* Search for corresponding type ID index finally: */
        iTypeIndex = m_pComboType->findData(strTypeId, TypeID);
    }

    /* To that moment type ID index should be always found: */
    AssertReturnVoid(iTypeIndex != -1);
    /* So we choose it: */
    m_pComboType->setCurrentIndex(iTypeIndex);
    sltTypeChanged(m_pComboType->currentIndex());
}

QString UINameAndSystemEditor::typeId() const
{
    if (!m_pComboType)
        return QString();
    return m_strTypeId;
}

QString UINameAndSystemEditor::familyId() const
{
    if (!m_pComboFamily)
        return QString();
    return m_strFamilyId;
}

void UINameAndSystemEditor::setType(const CGuestOSType &enmType)
{
    // WORKAROUND:
    // We're getting here with a NULL enmType when creating new VMs.
    // Very annoying, so just workarounded for now.
    /** @todo find out the reason and way to fix that.. */
    if (enmType.isNull())
        return;

    /* Pass to function above: */
    setTypeId(enmType.GetId(), enmType.GetFamilyId());
}

CGuestOSType UINameAndSystemEditor::type() const
{
    return uiCommon().vmGuestOSType(typeId(), familyId());
}

void UINameAndSystemEditor::setNameFieldValidator(const QString &strValidator)
{
    if (!m_pNameLineEdit)
        return;
    m_pNameLineEdit->setValidator(new QRegExpValidator(QRegExp(strValidator), this));
}

void UINameAndSystemEditor::markNameLineEdit(bool fError)
{
    if (m_pNameLineEdit)
        m_pNameLineEdit->mark(fError);
}

void UINameAndSystemEditor::retranslateUi()
{
    if (m_pNameLabel)
        m_pNameLabel->setText(tr("Name:"));
    if (m_pPathLabel)
        m_pPathLabel->setText(tr("Folder:"));
    if (m_pLabelFamily)
        m_pLabelFamily->setText(tr("&Type:"));
    if (m_pLabelType)
        m_pLabelType->setText(tr("&Version:"));

    if (m_pComboFamily)
        m_pComboFamily->setWhatsThis(tr("Selects the operating system family that "
                                        "you plan to install into this virtual machine."));
    if (m_pComboType)
        m_pComboType->setWhatsThis(tr("Selects the operating system type that "
                                      "you plan to install into this virtual machine "
                                      "(called a guest operating system)."));
}

void UINameAndSystemEditor::sltFamilyChanged(int iIndex)
{
    AssertPtrReturnVoid(m_pComboFamily);

    /* Lock the signals of m_pComboType to prevent it's reaction on clearing: */
    m_pComboType->blockSignals(true);
    m_pComboType->clear();

    /* Acquire family ID: */
    m_strFamilyId = m_pComboFamily->itemData(iIndex, TypeID).toString();

    /* Populate combo-box with OS types related to currently selected family id: */
    foreach (const UIGuestOSType &guiType, m_types.value(m_strFamilyId))
    {
        /* Skip 64bit OS types if hardware virtualization or long mode is not supported: */
        if (guiType.is64bit && (!m_fSupportsHWVirtEx || !m_fSupportsLongMode))
            continue;
        const int idxItem = m_pComboType->count();
        m_pComboType->insertItem(idxItem, guiType.typeDescription);
        m_pComboType->setItemData(idxItem, guiType.typeId, TypeID);
    }

    /* Select the most recently chosen item: */
    if (m_currentIds.contains(m_strFamilyId))
    {
        const QString strTypeId = m_currentIds.value(m_strFamilyId);
        const int iTypeIndex = m_pComboType->findData(strTypeId, TypeID);
        if (iTypeIndex != -1)
            m_pComboType->setCurrentIndex(iTypeIndex);
    }
    /* Or select Windows 7 item for Windows family as default: */
    else if (m_strFamilyId == "Windows")
    {
        QString strDefaultID = "Windows7";
        if (ARCH_BITS == 64 && m_fSupportsHWVirtEx && m_fSupportsLongMode)
            strDefaultID += "_64";
        const int iIndexWin7 = m_pComboType->findData(strDefaultID, TypeID);
        if (iIndexWin7 != -1)
            m_pComboType->setCurrentIndex(iIndexWin7);
    }
    /* Or select Oracle Linux item for Linux family as default: */
    else if (m_strFamilyId == "Linux")
    {
        QString strDefaultID = "Oracle";
        if (ARCH_BITS == 64 && m_fSupportsHWVirtEx && m_fSupportsLongMode)
            strDefaultID += "_64";
        const int iIndexUbuntu = m_pComboType->findData(strDefaultID, TypeID);
        if (iIndexUbuntu != -1)
            m_pComboType->setCurrentIndex(iIndexUbuntu);
    }
    /* Else simply select the first one present: */
    else
        m_pComboType->setCurrentIndex(0);

    /* Update all the stuff: */
    sltTypeChanged(m_pComboType->currentIndex());

    /* Unlock the signals of m_pComboType: */
    m_pComboType->blockSignals(false);

    /* Notify listeners about this change: */
    emit sigOSFamilyChanged();
}

void UINameAndSystemEditor::sltTypeChanged(int iIndex)
{
    AssertPtrReturnVoid(m_pComboType);

    /* Acquire type ID: */
    m_strTypeId = m_pComboType->itemData(iIndex, TypeID).toString();

    /* Update selected type pixmap: */
    m_pIconType->setPixmap(uiCommon().vmGuestOSTypePixmapDefault(m_strTypeId));

    /* Save the most recently used item: */
    m_currentIds[m_strFamilyId] = m_strTypeId;

    /* Notifies listeners about OS type change: */
    emit sigOsTypeChanged();
}

void UINameAndSystemEditor::prepare()
{
    prepareThis();
    prepareWidgets();
    prepareConnections();
    retranslateUi();
}

void UINameAndSystemEditor::prepareThis()
{
    if (m_fChooseType)
    {
        /* Check if host supports (AMD-V or VT-x) and long mode: */
        CHost comHost = uiCommon().host();
        m_fSupportsHWVirtEx = comHost.GetProcessorFeature(KProcessorFeature_HWVirtEx);
        m_fSupportsLongMode = comHost.GetProcessorFeature(KProcessorFeature_LongMode);
    }
}

void UINameAndSystemEditor::prepareWidgets()
{
    /* Create main-layout: */
    m_pMainLayout = new QGridLayout(this);
    if (m_pMainLayout)
    {
        m_pMainLayout->setContentsMargins(0, 0, 0, 0);
        m_pMainLayout->setColumnStretch(0, 0);
        m_pMainLayout->setColumnStretch(1, 1);

        int iRow = 0;

        if (m_fChooseName)
        {
            /* Create name label: */
            m_pNameLabel = new QLabel;
            if (m_pNameLabel)
            {
                m_pNameLabel->setAlignment(Qt::AlignRight);
                m_pNameLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

                /* Add into layout: */
                m_pMainLayout->addWidget(m_pNameLabel, iRow, 0, 1, 1);
            }
            /* Create name editor: */
            m_pNameLineEdit = new QILineEdit;
            if (m_pNameLineEdit)
            {
                /* Add into layout: */
                m_pMainLayout->addWidget(m_pNameLineEdit, iRow, 1, 1, 2);
            }

            ++iRow;
        }

        if (m_fChoosePath)
        {
            /* Create path label: */
            m_pPathLabel = new QLabel;
            if (m_pPathLabel)
            {
                m_pPathLabel->setAlignment(Qt::AlignRight);
                m_pPathLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

                /* Add into layout: */
                m_pMainLayout->addWidget(m_pPathLabel, iRow, 0, 1, 1);
            }
            /* Create path selector: */
            m_pPathSelector = new UIFilePathSelector;
            if (m_pPathSelector)
            {
                QString strDefaultMachineFolder = uiCommon().virtualBox().GetSystemProperties().GetDefaultMachineFolder();
                m_pPathSelector->setPath(strDefaultMachineFolder);
                m_pPathSelector->setDefaultPath(strDefaultMachineFolder);

                /* Add into layout: */
                m_pMainLayout->addWidget(m_pPathSelector, iRow, 1, 1, 2);
            }

            ++iRow;
        }

        if (m_fChooseType)
        {
            /* Create VM OS family label: */
            m_pLabelFamily = new QLabel;
            if (m_pLabelFamily)
            {
                m_pLabelFamily->setAlignment(Qt::AlignRight);
                m_pLabelFamily->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

                /* Add into layout: */
                m_pMainLayout->addWidget(m_pLabelFamily, iRow, 0);
            }

            int iIconRow = iRow;

            /* Create VM OS family combo: */
            m_pComboFamily = new QComboBox;
            if (m_pComboFamily)
            {
                m_pComboFamily->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
                m_pLabelFamily->setBuddy(m_pComboFamily);

                /* Add into layout: */
                m_pMainLayout->addWidget(m_pComboFamily, iRow, 1);
            }

            ++iRow;

            /* Create VM OS type label: */
            m_pLabelType = new QLabel;
            if (m_pLabelType)
            {
                m_pLabelType->setAlignment(Qt::AlignRight);
                m_pLabelType->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

                /* Add into layout: */
                m_pMainLayout->addWidget(m_pLabelType, iRow, 0);
            }
            /* Create VM OS type combo: */
            m_pComboType = new QComboBox;
            if (m_pComboType)
            {
                m_pComboType->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
                m_pLabelType->setBuddy(m_pComboType);

                /* Add into layout: */
                m_pMainLayout->addWidget(m_pComboType, iRow, 1);
            }

            ++iRow;

            /* Create sub-layout: */
            QVBoxLayout *pLayoutIcon = new QVBoxLayout;
            if (pLayoutIcon)
            {
                /* Create VM OS type icon: */
                m_pIconType = new QLabel;
                if (m_pIconType)
                {
                    m_pIconType->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

                    /* Add into layout: */
                    pLayoutIcon->addWidget(m_pIconType);
                }

                /* Add stretch to sub-layout: */
                pLayoutIcon->addStretch();

                /* Add into layout: */
                m_pMainLayout->addLayout(pLayoutIcon, iIconRow, 2, 2, 1);
            }

            /* Initialize VM OS family combo
             * after all widgets were created: */
            prepareFamilyCombo();
        }
    }
}

void UINameAndSystemEditor::prepareFamilyCombo()
{
    AssertPtrReturnVoid(m_pComboFamily);

    /* Acquire family IDs: */
    m_familyIDs = uiCommon().vmGuestOSFamilyIDs();

    /* For each known family ID: */
    for (int i = 0; i < m_familyIDs.size(); ++i)
    {
        const QString &strFamilyId = m_familyIDs.at(i);

        /* Append VM OS family combo: */
        m_pComboFamily->insertItem(i, uiCommon().vmGuestOSFamilyDescription(strFamilyId));
        m_pComboFamily->setItemData(i, strFamilyId, TypeID);

        /* Fill in the type cache: */
        m_types[strFamilyId] = QList<UIGuestOSType>();
        foreach (const CGuestOSType &comType, uiCommon().vmGuestOSTypeList(strFamilyId))
        {
            UIGuestOSType guiType;
            guiType.typeId = comType.GetId();
            guiType.typeDescription = comType.GetDescription();
            guiType.is64bit = comType.GetIs64Bit();
            m_types[strFamilyId] << guiType;
        }
    }

    /* Choose the 1st item to be the current: */
    m_pComboFamily->setCurrentIndex(0);
    /* And update the linked widgets accordingly: */
    sltFamilyChanged(m_pComboFamily->currentIndex());
}

void UINameAndSystemEditor::prepareConnections()
{
    /* Prepare connections: */
    if (m_pNameLineEdit)
        connect(m_pNameLineEdit, &QILineEdit::textChanged,
                this, &UINameAndSystemEditor::sigNameChanged);
    if (m_pPathSelector)
        connect(m_pPathSelector, &UIFilePathSelector::pathChanged,
                this, &UINameAndSystemEditor::sigPathChanged);
    if (m_pComboFamily)
        connect(m_pComboFamily, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, &UINameAndSystemEditor::sltFamilyChanged);
    if (m_pComboType)
        connect(m_pComboType, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, &UINameAndSystemEditor::sltTypeChanged);
}

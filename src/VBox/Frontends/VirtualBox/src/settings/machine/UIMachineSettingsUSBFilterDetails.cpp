/* $Id: UIMachineSettingsUSBFilterDetails.cpp 85960 2020-08-31 16:27:47Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsUSBFilterDetails class implementation.
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
#include <QLineEdit>

/* GUI includes: */
#include "QIDialogButtonBox.h"
#include "UIMachineSettingsUSBFilterDetails.h"
#include "UIConverter.h"


UIMachineSettingsUSBFilterDetails::UIMachineSettingsUSBFilterDetails(QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI2<QIDialog>(pParent, Qt::Sheet)
    , m_pLabelName(0)
    , m_pEditorName(0)
    , m_pLabelVendorID(0)
    , m_pEditorVendorID(0)
    , m_pLabelProductID(0)
    , m_pEditorProductID(0)
    , m_pLabelRevision(0)
    , m_pEditorRevision(0)
    , m_pLabelManufacturer(0)
    , m_pEditorManufacturer(0)
    , m_pLabelProduct(0)
    , m_pEditorProduct(0)
    , m_pLabelSerialNo(0)
    , m_pEditorSerialNo(0)
    , m_pLabelPort(0)
    , m_pEditorPort(0)
    , m_pLabelRemote(0)
    , m_pComboRemote(0)
    , m_pButtonBox(0)
{
    prepare();
}

void UIMachineSettingsUSBFilterDetails::retranslateUi()
{
    setWindowTitle(tr("USB Filter Details"));

    m_pLabelName->setText(tr("&Name:"));
    m_pEditorName->setToolTip(tr("Holds the filter name."));
    m_pLabelVendorID->setText(tr("&Vendor ID:"));
    m_pEditorVendorID->setToolTip(tr("Holds the vendor ID filter. The "
                                       "<i>exact match</i> string format is <tt>XXXX</tt> where <tt>X</tt> is a "
                                       "hexadecimal digit. An empty string will match any value."));
    m_pLabelProductID->setText(tr("&Product ID:"));
    m_pEditorProductID->setToolTip(tr("Holds the product ID filter. The "
                                        "<i>exact match</i> string format is <tt>XXXX</tt> where <tt>X</tt> is a "
                                        "hexadecimal digit. An empty string will match any value."));
    m_pLabelRevision->setText(tr("&Revision:"));
    m_pEditorRevision->setToolTip(tr("Holds the revision number filter. The "
                                       "<i>exact match</i> string format is <tt>IIFF</tt> where <tt>I</tt> is a decimal "
                                       "digit of the integer part and <tt>F</tt> is a decimal digit of the fractional "
                                       "part. An empty string will match any value."));
    m_pLabelManufacturer->setText(tr("&Manufacturer:"));
    m_pEditorManufacturer->setToolTip(tr("Holds the manufacturer filter as an "
                                           "<i>exact match</i> string. An empty string will match any value."));
    m_pLabelProduct->setText(tr("Pro&duct:"));
    m_pEditorProduct->setToolTip(tr("Holds the product name filter as an "
                                      "<i>exact match</i> string. An empty string will match any value."));
    m_pLabelSerialNo->setText(tr("&Serial No.:"));
    m_pEditorSerialNo->setToolTip(tr("Holds the serial number filter as an "
                                       "<i>exact match</i> string. An empty string will match any value."));
    m_pLabelPort->setText(tr("Por&t:"));
    m_pEditorPort->setToolTip(tr("Holds the host USB port filter as an "
                                   "<i>exact match</i> string. An empty string will match any value."));
    m_pLabelRemote->setText(tr("R&emote:"));
    m_pComboRemote->setToolTip(tr("Holds whether this filter applies to USB "
                                     "devices attached locally to the host computer (<i>No</i>), to a VRDP client's "
                                     "computer (<i>Yes</i>), or both (<i>Any</i>)."));

    m_pComboRemote->setItemText(UIMachineSettingsUSB::ModeAny, tr("Any", "remote"));
    m_pComboRemote->setItemText(UIMachineSettingsUSB::ModeOn,  tr("Yes", "remote"));
    m_pComboRemote->setItemText(UIMachineSettingsUSB::ModeOff, tr("No",  "remote"));
}

void UIMachineSettingsUSBFilterDetails::prepare()
{
    /* Prepare everything: */
    prepareWidgets();
    prepareConnections();

    /* Apply language settings: */
    retranslateUi();

    /* Adjust dialog size: */
    adjustSize();

#ifdef VBOX_WS_MAC
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFixedSize(minimumSize());
#endif /* VBOX_WS_MAC */
}

void UIMachineSettingsUSBFilterDetails::prepareWidgets()
{
    /* Prepare main layout: */
    QGridLayout *pLayoutMain = new QGridLayout(this);
    if (pLayoutMain)
    {
        pLayoutMain->setRowStretch(9, 1);

        /* Prepare name label: */
        m_pLabelName = new QLabel(this);
        if (m_pLabelName)
        {
            m_pLabelName->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pLayoutMain->addWidget(m_pLabelName, 0, 0);
        }
        /* Prepare name editor: */
        m_pEditorName = new QLineEdit(this);
        if (m_pEditorName)
        {
            if (m_pLabelName)
                m_pLabelName->setBuddy(m_pEditorName);
            m_pEditorName->setValidator(new QRegExpValidator(QRegExp(".+"), this));

            pLayoutMain->addWidget(m_pEditorName, 0, 1);
        }

        /* Prepare vendor ID label: */
        m_pLabelVendorID = new QLabel(this);
        if (m_pLabelVendorID)
        {
            m_pLabelVendorID->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pLayoutMain->addWidget(m_pLabelVendorID, 1, 0);
        }
        /* Prepare vendor ID editor: */
        m_pEditorVendorID = new QLineEdit(this);
        if (m_pEditorVendorID)
        {
            if (m_pLabelVendorID)
                m_pLabelVendorID->setBuddy(m_pEditorVendorID);
            m_pEditorVendorID->setValidator(new QRegExpValidator(QRegExp("[0-9a-fA-F]{0,4}"), this));

            pLayoutMain->addWidget(m_pEditorVendorID, 1, 1);
        }

        /* Prepare product ID label: */
        m_pLabelProductID = new QLabel(this);
        if (m_pLabelProductID)
        {
            m_pLabelProductID->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pLayoutMain->addWidget(m_pLabelProductID, 2, 0);
        }
        /* Prepare product ID editor: */
        m_pEditorProductID = new QLineEdit(this);
        if (m_pEditorProductID)
        {
            if (m_pLabelProductID)
                m_pLabelProductID->setBuddy(m_pEditorProductID);
            m_pEditorProductID->setValidator(new QRegExpValidator(QRegExp("[0-9a-fA-F]{0,4}"), this));

            pLayoutMain->addWidget(m_pEditorProductID, 2, 1);
        }

        /* Prepare revision label: */
        m_pLabelRevision = new QLabel(this);
        if (m_pLabelRevision)
        {
            m_pLabelRevision->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pLayoutMain->addWidget(m_pLabelRevision, 3, 0);
        }
        /* Prepare revision editor: */
        m_pEditorRevision = new QLineEdit(this);
        if (m_pEditorRevision)
        {
            if (m_pLabelRevision)
                m_pLabelRevision->setBuddy(m_pEditorRevision);
            m_pEditorRevision->setValidator(new QRegExpValidator(QRegExp("[0-9a-fA-F]{0,4}"), this));

            pLayoutMain->addWidget(m_pEditorRevision, 3, 1);
        }

        /* Prepare manufacturer label: */
        m_pLabelManufacturer = new QLabel(this);
        if (m_pLabelManufacturer)
        {
            m_pLabelManufacturer->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pLayoutMain->addWidget(m_pLabelManufacturer, 4, 0);
        }
        /* Prepare manufacturer editor: */
        m_pEditorManufacturer = new QLineEdit(this);
        if (m_pEditorManufacturer)
        {
            if (m_pLabelManufacturer)
                m_pLabelManufacturer->setBuddy(m_pEditorManufacturer);
            pLayoutMain->addWidget(m_pEditorManufacturer, 4, 1);
        }

        /* Prepare product label: */
        m_pLabelProduct = new QLabel(this);
        if (m_pLabelProduct)
        {
            m_pLabelProduct->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pLayoutMain->addWidget(m_pLabelProduct, 5, 0);
        }
        /* Prepare product editor: */
        m_pEditorProduct = new QLineEdit(this);
        if (m_pEditorProduct)
        {
            if (m_pLabelProduct)
                m_pLabelProduct->setBuddy(m_pEditorProduct);
            pLayoutMain->addWidget(m_pEditorProduct, 5, 1);
        }

        /* Prepare serial NO label: */
        m_pLabelSerialNo = new QLabel(this);
        if (m_pLabelSerialNo)
        {
            m_pLabelSerialNo->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pLayoutMain->addWidget(m_pLabelSerialNo, 6, 0);
        }
        /* Prepare serial NO editor: */
        m_pEditorSerialNo = new QLineEdit(this);
        if (m_pEditorSerialNo)
        {
            if (m_pLabelSerialNo)
                m_pLabelSerialNo->setBuddy(m_pEditorSerialNo);
            pLayoutMain->addWidget(m_pEditorSerialNo, 6, 1);
        }

        /* Prepare port label: */
        m_pLabelPort = new QLabel(this);
        if (m_pLabelPort)
        {
            m_pLabelPort->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pLayoutMain->addWidget(m_pLabelPort, 7, 0);
        }
        /* Prepare port editor: */
        m_pEditorPort = new QLineEdit(this);
        if (m_pEditorPort)
        {
            if (m_pLabelPort)
                m_pLabelPort->setBuddy(m_pEditorPort);
            m_pEditorPort->setValidator(new QRegExpValidator(QRegExp("[0-9]*"), this));

            pLayoutMain->addWidget(m_pEditorPort, 7, 1);
        }

        /* Prepare remote label: */
        m_pLabelRemote = new QLabel(this);
        if (m_pLabelRemote)
        {
            m_pLabelRemote->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            pLayoutMain->addWidget(m_pLabelRemote, 8, 0);
        }
        /* Prepare remote combo: */
        m_pComboRemote = new QComboBox(this);
        if (m_pComboRemote)
        {
            if (m_pLabelRemote)
                m_pLabelRemote->setBuddy(m_pComboRemote);
            m_pComboRemote->insertItem(UIMachineSettingsUSB::ModeAny, QString()); /* Any */
            m_pComboRemote->insertItem(UIMachineSettingsUSB::ModeOn,  QString()); /* Yes */
            m_pComboRemote->insertItem(UIMachineSettingsUSB::ModeOff, QString()); /* No */

            pLayoutMain->addWidget(m_pComboRemote, 8, 1);
        }

        /* Prepare button-box: */
        m_pButtonBox = new QIDialogButtonBox(this);
        if (m_pButtonBox)
        {
            m_pButtonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
            pLayoutMain->addWidget(m_pButtonBox, 10, 0, 1, 2);
        }
    }
}

void UIMachineSettingsUSBFilterDetails::prepareConnections()
{
    connect(m_pButtonBox, &QIDialogButtonBox::accepted, this, &UIMachineSettingsUSBFilterDetails::accept);
    connect(m_pButtonBox, &QIDialogButtonBox::rejected, this, &UIMachineSettingsUSBFilterDetails::reject);
}

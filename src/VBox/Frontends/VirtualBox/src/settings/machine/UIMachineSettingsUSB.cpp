/* $Id: UIMachineSettingsUSB.cpp 86233 2020-09-23 12:10:51Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsUSB class implementation.
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
#include <QCheckBox>
#include <QComboBox>
#include <QHeaderView>
#include <QHelpEvent>
#include <QLineEdit>
#include <QMenu>
#include <QRadioButton>
#include <QSpacerItem>
#include <QToolTip>
#include <QVBoxLayout>

/* GUI includes: */
#include "QILabelSeparator.h"
#include "QITreeWidget.h"
#include "QIWidgetValidator.h"
#include "UIConverter.h"
#include "UIIconPool.h"
#include "UIMachineSettingsUSB.h"
#include "UIMachineSettingsUSBFilterDetails.h"
#include "UIErrorString.h"
#include "QIToolBar.h"
#include "UICommon.h"

/* COM includes: */
#include "CConsole.h"
#include "CExtPack.h"
#include "CExtPackManager.h"
#include "CHostUSBDevice.h"
#include "CHostUSBDeviceFilter.h"
#include "CUSBController.h"
#include "CUSBDevice.h"
#include "CUSBDeviceFilter.h"
#include "CUSBDeviceFilters.h"

/* VirtualBox interface declarations: */
#include <VBox/com/VirtualBox.h>


/** Machine settings: USB filter data structure. */
struct UIDataSettingsMachineUSBFilter
{
    /** Constructs data. */
    UIDataSettingsMachineUSBFilter()
        : m_fActive(false)
        , m_strName(QString())
        , m_strVendorId(QString())
        , m_strProductId(QString())
        , m_strRevision(QString())
        , m_strManufacturer(QString())
        , m_strProduct(QString())
        , m_strSerialNumber(QString())
        , m_strPort(QString())
        , m_strRemote(QString())
        , m_enmAction(KUSBDeviceFilterAction_Null)
        , m_enmHostUSBDeviceState(KUSBDeviceState_NotSupported)
        , m_fHostUSBDevice(false)
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsMachineUSBFilter &other) const
    {
        return true
               && (m_fActive == other.m_fActive)
               && (m_strName == other.m_strName)
               && (m_strVendorId == other.m_strVendorId)
               && (m_strProductId == other.m_strProductId)
               && (m_strRevision == other.m_strRevision)
               && (m_strManufacturer == other.m_strManufacturer)
               && (m_strProduct == other.m_strProduct)
               && (m_strSerialNumber == other.m_strSerialNumber)
               && (m_strPort == other.m_strPort)
               && (m_strRemote == other.m_strRemote)
               && (m_enmAction == other.m_enmAction)
               && (m_enmHostUSBDeviceState == other.m_enmHostUSBDeviceState)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsMachineUSBFilter &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsMachineUSBFilter &other) const { return !equal(other); }

    /** Holds whether the USB filter is enabled. */
    bool     m_fActive;
    /** Holds the USB filter name. */
    QString  m_strName;
    /** Holds the USB filter vendor ID. */
    QString  m_strVendorId;
    /** Holds the USB filter product ID. */
    QString  m_strProductId;
    /** Holds the USB filter revision. */
    QString  m_strRevision;
    /** Holds the USB filter manufacturer. */
    QString  m_strManufacturer;
    /** Holds the USB filter product. */
    QString  m_strProduct;
    /** Holds the USB filter serial number. */
    QString  m_strSerialNumber;
    /** Holds the USB filter port. */
    QString  m_strPort;
    /** Holds the USB filter remote. */
    QString  m_strRemote;

    /** Holds the USB filter action. */
    KUSBDeviceFilterAction  m_enmAction;
    /** Holds the USB device state. */
    KUSBDeviceState         m_enmHostUSBDeviceState;
    /** Holds whether the USB filter is host USB device. */
    bool                    m_fHostUSBDevice;
};


/** Machine settings: USB page data structure. */
struct UIDataSettingsMachineUSB
{
    /** Constructs data. */
    UIDataSettingsMachineUSB()
        : m_fUSBEnabled(false)
        , m_USBControllerType(KUSBControllerType_Null)
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsMachineUSB &other) const
    {
        return true
               && (m_fUSBEnabled == other.m_fUSBEnabled)
               && (m_USBControllerType == other.m_USBControllerType)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsMachineUSB &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsMachineUSB &other) const { return !equal(other); }

    /** Holds whether the USB is enabled. */
    bool m_fUSBEnabled;
    /** Holds the USB controller type. */
    KUSBControllerType m_USBControllerType;
};


/** Machine settings: USB Filter popup menu. */
class VBoxUSBMenu : public QMenu
{
    Q_OBJECT;

public:

    /* Constructor: */
    VBoxUSBMenu(QWidget *)
    {
        connect(this, &VBoxUSBMenu::aboutToShow, this, &VBoxUSBMenu::processAboutToShow);
    }

    /* Returns USB device related to passed action: */
    const CUSBDevice& getUSB(QAction *pAction)
    {
        return m_usbDeviceMap[pAction];
    }

    /* Console setter: */
    void setConsole(const CConsole &console)
    {
        m_console = console;
    }

private slots:

    /* Prepare menu appearance: */
    void processAboutToShow()
    {
        clear();
        m_usbDeviceMap.clear();

        CHost host = uiCommon().host();

        bool fIsUSBEmpty = host.GetUSBDevices().size() == 0;
        if (fIsUSBEmpty)
        {
            QAction *pAction = addAction(tr("<no devices available>", "USB devices"));
            pAction->setEnabled(false);
            pAction->setToolTip(tr("No supported devices connected to the host PC", "USB device tooltip"));
        }
        else
        {
            CHostUSBDeviceVector devvec = host.GetUSBDevices();
            for (int i = 0; i < devvec.size(); ++i)
            {
                CHostUSBDevice dev = devvec[i];
                CUSBDevice usb(dev);
                QAction *pAction = addAction(uiCommon().details(usb));
                pAction->setCheckable(true);
                m_usbDeviceMap[pAction] = usb;
                /* Check if created item was already attached to this session: */
                if (!m_console.isNull())
                {
                    CUSBDevice attachedUSB = m_console.FindUSBDeviceById(usb.GetId());
                    pAction->setChecked(!attachedUSB.isNull());
                    pAction->setEnabled(dev.GetState() != KUSBDeviceState_Unavailable);
                }
            }
        }
    }

private:

    /* Event handler: */
    bool event(QEvent *pEvent)
    {
        /* We provide dynamic tooltips for the usb devices: */
        if (pEvent->type() == QEvent::ToolTip)
        {
            QHelpEvent *pHelpEvent = static_cast<QHelpEvent*>(pEvent);
            QAction *pAction = actionAt(pHelpEvent->pos());
            if (pAction)
            {
                CUSBDevice usb = m_usbDeviceMap[pAction];
                if (!usb.isNull())
                {
                    QToolTip::showText(pHelpEvent->globalPos(), uiCommon().toolTip(usb));
                    return true;
                }
            }
        }
        /* Call to base-class: */
        return QMenu::event(pEvent);
    }

    QMap<QAction*, CUSBDevice> m_usbDeviceMap;
    CConsole m_console;
};


/** Machine settings: USB Filter tree-widget item. */
class UIUSBFilterItem : public QITreeWidgetItem, public UIDataSettingsMachineUSBFilter
{
    Q_OBJECT;

public:

    /** Constructs USB filter (root) item.
      * @param  pParent  Brings the item parent. */
    UIUSBFilterItem(QITreeWidget *pParent)
        : QITreeWidgetItem(pParent)
    {
    }

    /** Updates item fields. */
    void updateFields()
    {
        setText(0, m_strName);
        setToolTip(0, toolTipFor());
    }

protected:

    /** Returns default text. */
    virtual QString defaultText() const /* override */
    {
        return checkState(0) == Qt::Checked ?
               tr("%1, Active", "col.1 text, col.1 state").arg(text(0)) :
               tr("%1",         "col.1 text")             .arg(text(0));
    }

private:

    /** Returns tool-tip generated from item data. */
    QString toolTipFor()
    {
        /* Prepare tool-tip: */
        QString strToolTip;

        const QString strVendorId = m_strVendorId;
        if (!strVendorId.isEmpty())
            strToolTip += tr("<nobr>Vendor ID: %1</nobr>", "USB filter tooltip").arg(strVendorId);

        const QString strProductId = m_strProductId;
        if (!strProductId.isEmpty())
            strToolTip += strToolTip.isEmpty() ? "":"<br/>" + tr("<nobr>Product ID: %2</nobr>", "USB filter tooltip").arg(strProductId);

        const QString strRevision = m_strRevision;
        if (!strRevision.isEmpty())
            strToolTip += strToolTip.isEmpty() ? "":"<br/>" + tr("<nobr>Revision: %3</nobr>", "USB filter tooltip").arg(strRevision);

        const QString strProduct = m_strProduct;
        if (!strProduct.isEmpty())
            strToolTip += strToolTip.isEmpty() ? "":"<br/>" + tr("<nobr>Product: %4</nobr>", "USB filter tooltip").arg(strProduct);

        const QString strManufacturer = m_strManufacturer;
        if (!strManufacturer.isEmpty())
            strToolTip += strToolTip.isEmpty() ? "":"<br/>" + tr("<nobr>Manufacturer: %5</nobr>", "USB filter tooltip").arg(strManufacturer);

        const QString strSerial = m_strSerialNumber;
        if (!strSerial.isEmpty())
            strToolTip += strToolTip.isEmpty() ? "":"<br/>" + tr("<nobr>Serial No.: %1</nobr>", "USB filter tooltip").arg(strSerial);

        const QString strPort = m_strPort;
        if (!strPort.isEmpty())
            strToolTip += strToolTip.isEmpty() ? "":"<br/>" + tr("<nobr>Port: %1</nobr>", "USB filter tooltip").arg(strPort);

        /* Add the state field if it's a host USB device: */
        if (m_fHostUSBDevice)
        {
            strToolTip += strToolTip.isEmpty() ? "":"<br/>" + tr("<nobr>State: %1</nobr>", "USB filter tooltip")
                                                                 .arg(gpConverter->toString(m_enmHostUSBDeviceState));
        }

        /* Return tool-tip: */
        return strToolTip;
    }
};


UIMachineSettingsUSB::UIMachineSettingsUSB()
    : m_pCache(0)
    , m_pCheckBoxUSB(0)
    , m_pWidgetUSBSettings(0)
    , m_pRadioButtonUSB1(0)
    , m_pRadioButtonUSB2(0)
    , m_pRadioButtonUSB3(0)
    , m_pLabelSeparatorFilters(0)
    , m_pLayoutFilters(0)
    , m_pTreeWidgetFilters(0)
    , m_pToolbarFilters(0)
    , m_pActionNew(0)
    , m_pActionAdd(0)
    , m_pActionEdit(0)
    , m_pActionRemove(0)
    , m_pActionMoveUp(0)
    , m_pActionMoveDown(0)
    , m_pMenuUSBDevices(0)
{
    /* Prepare: */
    prepare();
}

UIMachineSettingsUSB::~UIMachineSettingsUSB()
{
    /* Cleanup: */
    cleanup();
}

bool UIMachineSettingsUSB::isUSBEnabled() const
{
    return m_pCheckBoxUSB->isChecked();
}

bool UIMachineSettingsUSB::changed() const
{
    return m_pCache->wasChanged();
}

void UIMachineSettingsUSB::loadToCacheFrom(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Clear cache initially: */
    m_pCache->clear();

    /* Prepare old USB data: */
    UIDataSettingsMachineUSB oldUsbData;

    /* Gather old USB data: */
    oldUsbData.m_fUSBEnabled = !m_machine.GetUSBControllers().isEmpty();
    oldUsbData.m_USBControllerType = m_machine.GetUSBControllerCountByType(KUSBControllerType_XHCI) > 0 ? KUSBControllerType_XHCI :
                                     m_machine.GetUSBControllerCountByType(KUSBControllerType_EHCI) > 0 ? KUSBControllerType_EHCI :
                                     m_machine.GetUSBControllerCountByType(KUSBControllerType_OHCI) > 0 ? KUSBControllerType_OHCI :
                                     KUSBControllerType_Null;

    /* Check whether controller is valid: */
    const CUSBDeviceFilters &comFiltersObject = m_machine.GetUSBDeviceFilters();
    if (!comFiltersObject.isNull())
    {
        /* For each filter: */
        const CUSBDeviceFilterVector &filters = comFiltersObject.GetDeviceFilters();
        for (int iFilterIndex = 0; iFilterIndex < filters.size(); ++iFilterIndex)
        {
            /* Prepare old filter data: */
            UIDataSettingsMachineUSBFilter oldFilterData;

            /* Check whether filter is valid: */
            const CUSBDeviceFilter &filter = filters.at(iFilterIndex);
            if (!filter.isNull())
            {
                /* Gather old filter data: */
                oldFilterData.m_fActive = filter.GetActive();
                oldFilterData.m_strName = filter.GetName();
                oldFilterData.m_strVendorId = filter.GetVendorId();
                oldFilterData.m_strProductId = filter.GetProductId();
                oldFilterData.m_strRevision = filter.GetRevision();
                oldFilterData.m_strManufacturer = filter.GetManufacturer();
                oldFilterData.m_strProduct = filter.GetProduct();
                oldFilterData.m_strSerialNumber = filter.GetSerialNumber();
                oldFilterData.m_strPort = filter.GetPort();
                oldFilterData.m_strRemote = filter.GetRemote();
            }

            /* Cache old filter data: */
            m_pCache->child(iFilterIndex).cacheInitialData(oldFilterData);
        }
    }

    /* Cache old USB data: */
    m_pCache->cacheInitialData(oldUsbData);

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

void UIMachineSettingsUSB::getFromCache()
{
    /* Clear list initially: */
    m_pTreeWidgetFilters->clear();

    /* Get old USB data from the cache: */
    const UIDataSettingsMachineUSB &oldUsbData = m_pCache->base();

    /* Load currently supported USB controllers: */
    CSystemProperties comProperties = uiCommon().virtualBox().GetSystemProperties();
    QVector<KUSBControllerType> supportedTypes = comProperties.GetSupportedUSBControllerTypes();
    /* Take currently requested type into account if it's sane: */
    if (!supportedTypes.contains(oldUsbData.m_USBControllerType) && oldUsbData.m_USBControllerType != KUSBControllerType_Null)
        supportedTypes.prepend(oldUsbData.m_USBControllerType);

    /* Adjust radio-button visibility: */
    m_pRadioButtonUSB1->setVisible(supportedTypes.contains(KUSBControllerType_OHCI));
    m_pRadioButtonUSB2->setVisible(supportedTypes.contains(KUSBControllerType_EHCI));
    m_pRadioButtonUSB3->setVisible(supportedTypes.contains(KUSBControllerType_XHCI));

    /* Load old USB data from the cache: */
    m_pCheckBoxUSB->setChecked(oldUsbData.m_fUSBEnabled);
    switch (oldUsbData.m_USBControllerType)
    {
        default:
        case KUSBControllerType_OHCI: m_pRadioButtonUSB1->setChecked(true); break;
        case KUSBControllerType_EHCI: m_pRadioButtonUSB2->setChecked(true); break;
        case KUSBControllerType_XHCI: m_pRadioButtonUSB3->setChecked(true); break;
    }

    /* For each filter => load it from the cache: */
    for (int iFilterIndex = 0; iFilterIndex < m_pCache->childCount(); ++iFilterIndex)
        addUSBFilterItem(m_pCache->child(iFilterIndex).base(), false /* its new? */);

    /* Choose first filter as current: */
    m_pTreeWidgetFilters->setCurrentItem(m_pTreeWidgetFilters->topLevelItem(0));
    sltHandleUsbAdapterToggle(m_pCheckBoxUSB->isChecked());

    /* Polish page finally: */
    polishPage();

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsUSB::putToCache()
{
    /* Prepare new USB data: */
    UIDataSettingsMachineUSB newUsbData;

    /* Gather new USB data: */
    newUsbData.m_fUSBEnabled = m_pCheckBoxUSB->isChecked();
    if (!newUsbData.m_fUSBEnabled)
        newUsbData.m_USBControllerType = KUSBControllerType_Null;
    else
    {
        if (m_pRadioButtonUSB1->isChecked())
            newUsbData.m_USBControllerType = KUSBControllerType_OHCI;
        else if (m_pRadioButtonUSB2->isChecked())
            newUsbData.m_USBControllerType = KUSBControllerType_EHCI;
        else if (m_pRadioButtonUSB3->isChecked())
            newUsbData.m_USBControllerType = KUSBControllerType_XHCI;
    }

    /* For each filter: */
    QTreeWidgetItem *pMainRootItem = m_pTreeWidgetFilters->invisibleRootItem();
    for (int iFilterIndex = 0; iFilterIndex < pMainRootItem->childCount(); ++iFilterIndex)
    {
        /* Gather and cache new filter data: */
        const UIUSBFilterItem *pItem = static_cast<UIUSBFilterItem*>(pMainRootItem->child(iFilterIndex));
        m_pCache->child(iFilterIndex).cacheCurrentData(*pItem);
    }

    /* Cache new USB data: */
    m_pCache->cacheCurrentData(newUsbData);
}

void UIMachineSettingsUSB::saveFromCacheTo(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Update USB data and failing state: */
    setFailed(!saveUSBData());

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

bool UIMachineSettingsUSB::validate(QList<UIValidationMessage> &messages)
{
    Q_UNUSED(messages);

    /* Pass by default: */
    bool fPass = true;

#ifdef VBOX_WITH_EXTPACK
    /* USB 2.0/3.0 Extension Pack presence test: */
    const CExtPack extPack = uiCommon().virtualBox().GetExtensionPackManager().Find(GUI_ExtPackName);
    if (   m_pCheckBoxUSB->isChecked()
        && (m_pRadioButtonUSB2->isChecked() || m_pRadioButtonUSB3->isChecked())
        && (extPack.isNull() || !extPack.GetUsable()))
    {
        /* Prepare message: */
        UIValidationMessage message;
        message.second << tr("USB 2.0/3.0 is currently enabled for this virtual machine. "
                             "However, this requires the <i>%1</i> to be installed. "
                             "Please install the Extension Pack from the VirtualBox download site "
                             "or disable USB 2.0/3.0 to be able to start the machine.")
                             .arg(GUI_ExtPackName);
        /* Serialize message: */
        if (!message.second.isEmpty())
            messages << message;
    }
#endif /* VBOX_WITH_EXTPACK */

    /* Return result: */
    return fPass;
}

void UIMachineSettingsUSB::setOrderAfter(QWidget *pWidget)
{
    setTabOrder(pWidget, m_pCheckBoxUSB);
    setTabOrder(m_pCheckBoxUSB, m_pRadioButtonUSB1);
    setTabOrder(m_pRadioButtonUSB1, m_pRadioButtonUSB2);
    setTabOrder(m_pRadioButtonUSB2, m_pRadioButtonUSB3);
    setTabOrder(m_pRadioButtonUSB3, m_pTreeWidgetFilters);
}

void UIMachineSettingsUSB::retranslateUi()
{
    m_pCheckBoxUSB->setWhatsThis(tr("When checked, enables the virtual USB controller of this machine."));
    m_pCheckBoxUSB->setText(tr("Enable &USB Controller"));
    m_pRadioButtonUSB1->setWhatsThis(tr("When chosen, enables the virtual USB OHCI controller of "
                                        "this machine. The USB OHCI controller provides USB 1.0 support."));
    m_pRadioButtonUSB1->setText(tr("USB &1.1 (OHCI) Controller"));
    m_pRadioButtonUSB2->setWhatsThis(tr("When chosen, enables the virtual USB OHCI and EHCI "
                                        "controllers of this machine. Together they provide USB 2.0 support."));
    m_pRadioButtonUSB2->setText(tr("USB &2.0 (OHCI + EHCI) Controller"));
    m_pRadioButtonUSB3->setWhatsThis(tr("When chosen, enables the virtual USB xHCI controller of "
                                        "this machine. The USB xHCI controller provides USB 3.0 support."));
    m_pRadioButtonUSB3->setText(tr("USB &3.0 (xHCI) Controller"));
    m_pLabelSeparatorFilters->setText(tr("USB Device &Filters"));
    QTreeWidgetItem *pQtreewidgetitem = m_pTreeWidgetFilters->headerItem();
    pQtreewidgetitem->setText(0, tr("[filter]"));
    m_pTreeWidgetFilters->setWhatsThis(tr("Lists all USB filters of this machine. The checkbox "
                                          "to the left defines whether the particular filter is enabled or not. Use the "
                                          "context menu or buttons to the right to add or remove USB filters."));

    m_pActionNew->setText(tr("Add Empty Filter"));
    m_pActionAdd->setText(tr("Add Filter From Device"));
    m_pActionEdit->setText(tr("Edit Filter"));
    m_pActionRemove->setText(tr("Remove Filter"));
    m_pActionMoveUp->setText(tr("Move Filter Up"));
    m_pActionMoveDown->setText(tr("Move Filter Down"));

    m_pActionNew->setWhatsThis(tr("Adds new USB filter with all fields initially set to empty strings. "
                                "Note that such a filter will match any attached USB device."));
    m_pActionAdd->setWhatsThis(tr("Adds new USB filter with all fields set to the values of the "
                                "selected USB device attached to the host PC."));
    m_pActionEdit->setWhatsThis(tr("Edits selected USB filter."));
    m_pActionRemove->setWhatsThis(tr("Removes selected USB filter."));
    m_pActionMoveUp->setWhatsThis(tr("Moves selected USB filter up."));
    m_pActionMoveDown->setWhatsThis(tr("Moves selected USB filter down."));

    m_pActionNew->setToolTip(m_pActionNew->whatsThis());
    m_pActionAdd->setToolTip(m_pActionAdd->whatsThis());
    m_pActionEdit->setToolTip(m_pActionEdit->whatsThis());
    m_pActionRemove->setToolTip(m_pActionRemove->whatsThis());
    m_pActionMoveUp->setToolTip(m_pActionMoveUp->whatsThis());
    m_pActionMoveDown->setToolTip(m_pActionMoveDown->whatsThis());

    m_strTrUSBFilterName = tr("New Filter %1", "usb");
}

void UIMachineSettingsUSB::polishPage()
{
    /* Polish USB page availability: */
    m_pCheckBoxUSB->setEnabled(isMachineOffline());
    m_pWidgetUSBSettings->setEnabled(isMachineInValidMode() && m_pCheckBoxUSB->isChecked());
    m_pRadioButtonUSB1->setEnabled(isMachineOffline() && m_pCheckBoxUSB->isChecked());
    m_pRadioButtonUSB2->setEnabled(isMachineOffline() && m_pCheckBoxUSB->isChecked());
    m_pRadioButtonUSB3->setEnabled(isMachineOffline() && m_pCheckBoxUSB->isChecked());
}

void UIMachineSettingsUSB::sltHandleUsbAdapterToggle(bool fEnabled)
{
    /* Enable/disable USB children: */
    m_pWidgetUSBSettings->setEnabled(isMachineInValidMode() && fEnabled);
    m_pRadioButtonUSB1->setEnabled(isMachineOffline() && fEnabled);
    m_pRadioButtonUSB2->setEnabled(isMachineOffline() && fEnabled);
    m_pRadioButtonUSB3->setEnabled(isMachineOffline() && fEnabled);
    if (fEnabled)
    {
        /* If there is no chosen item but there is something to choose => choose it: */
        if (m_pTreeWidgetFilters->currentItem() == 0 && m_pTreeWidgetFilters->topLevelItemCount() != 0)
            m_pTreeWidgetFilters->setCurrentItem(m_pTreeWidgetFilters->topLevelItem(0));
    }
    /* Update current item: */
    sltHandleCurrentItemChange(m_pTreeWidgetFilters->currentItem());
}

void UIMachineSettingsUSB::sltHandleCurrentItemChange(QTreeWidgetItem *pCurrentItem)
{
    /* Get selected items: */
    QList<QTreeWidgetItem*> selectedItems = m_pTreeWidgetFilters->selectedItems();
    /* Deselect all selected items first: */
    for (int iItemIndex = 0; iItemIndex < selectedItems.size(); ++iItemIndex)
        selectedItems[iItemIndex]->setSelected(false);

    /* If tree-widget is NOT enabled => we should NOT select anything: */
    if (!m_pTreeWidgetFilters->isEnabled())
        return;

    /* Select item if requested: */
    if (pCurrentItem)
        pCurrentItem->setSelected(true);

    /* Update corresponding action states: */
    m_pActionEdit->setEnabled(pCurrentItem);
    m_pActionRemove->setEnabled(pCurrentItem);
    m_pActionMoveUp->setEnabled(pCurrentItem && m_pTreeWidgetFilters->itemAbove(pCurrentItem));
    m_pActionMoveDown->setEnabled(pCurrentItem && m_pTreeWidgetFilters->itemBelow(pCurrentItem));
}

void UIMachineSettingsUSB::sltHandleContextMenuRequest(const QPoint &pos)
{
    QMenu menu;
    if (m_pTreeWidgetFilters->isEnabled())
    {
        menu.addAction(m_pActionNew);
        menu.addAction(m_pActionAdd);
        menu.addSeparator();
        menu.addAction(m_pActionEdit);
        menu.addSeparator();
        menu.addAction(m_pActionRemove);
        menu.addSeparator();
        menu.addAction(m_pActionMoveUp);
        menu.addAction(m_pActionMoveDown);
    }
    if (!menu.isEmpty())
        menu.exec(m_pTreeWidgetFilters->mapToGlobal(pos));
}

void UIMachineSettingsUSB::sltHandleActivityStateChange(QTreeWidgetItem *pChangedItem)
{
    /* Check changed USB filter item: */
    UIUSBFilterItem *pItem = static_cast<UIUSBFilterItem*>(pChangedItem);
    AssertPtrReturnVoid(pItem);

    /* Update corresponding item: */
    pItem->m_fActive = pItem->checkState(0) == Qt::Checked;
}

void UIMachineSettingsUSB::sltNewFilter()
{
    /* Search for the max available filter index: */
    int iMaxFilterIndex = 0;
    const QRegExp regExp(QString("^") + m_strTrUSBFilterName.arg("([0-9]+)") + QString("$"));
    QTreeWidgetItemIterator iterator(m_pTreeWidgetFilters);
    while (*iterator)
    {
        const QString filterName = (*iterator)->text(0);
        const int pos = regExp.indexIn(filterName);
        if (pos != -1)
            iMaxFilterIndex = regExp.cap(1).toInt() > iMaxFilterIndex ?
                              regExp.cap(1).toInt() : iMaxFilterIndex;
        ++iterator;
    }

    /* Prepare new USB filter data: */
    UIDataSettingsMachineUSBFilter filterData;
    filterData.m_fActive = true;
    filterData.m_strName = m_strTrUSBFilterName.arg(iMaxFilterIndex + 1);
    filterData.m_fHostUSBDevice = false;

    /* Add new USB filter item: */
    addUSBFilterItem(filterData, true /* its new? */);

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsUSB::sltAddFilter()
{
    m_pMenuUSBDevices->exec(QCursor::pos());
}

void UIMachineSettingsUSB::sltAddFilterConfirmed(QAction *pAction)
{
    /* Get USB device: */
    const CUSBDevice usb = m_pMenuUSBDevices->getUSB(pAction);
    if (usb.isNull())
        return;

    /* Prepare new USB filter data: */
    UIDataSettingsMachineUSBFilter filterData;
    filterData.m_fActive = true;
    filterData.m_strName = uiCommon().details(usb);
    filterData.m_fHostUSBDevice = false;
    filterData.m_strVendorId = QString().sprintf("%04hX", usb.GetVendorId());
    filterData.m_strProductId = QString().sprintf("%04hX", usb.GetProductId());
    filterData.m_strRevision = QString().sprintf("%04hX", usb.GetRevision());
    /* The port property depends on the host computer rather than on the USB
     * device itself; for this reason only a few people will want to use it
     * in the filter since the same device plugged into a different socket
     * will not match the filter in this case. */
#if 0
    filterData.m_strPort = QString().sprintf("%04hX", usb.GetPort());
#endif
    filterData.m_strManufacturer = usb.GetManufacturer();
    filterData.m_strProduct = usb.GetProduct();
    filterData.m_strSerialNumber = usb.GetSerialNumber();
    filterData.m_strRemote = QString::number(usb.GetRemote());

    /* Add new USB filter item: */
    addUSBFilterItem(filterData, true /* its new? */);

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsUSB::sltEditFilter()
{
    /* Check current USB filter item: */
    UIUSBFilterItem *pItem = static_cast<UIUSBFilterItem*>(m_pTreeWidgetFilters->currentItem());
    AssertPtrReturnVoid(pItem);

    /* Configure USB filter details dialog: */
    UIMachineSettingsUSBFilterDetails dlgFilterDetails(this);
    dlgFilterDetails.m_pEditorName->setText(pItem->m_strName);
    dlgFilterDetails.m_pEditorVendorID->setText(pItem->m_strVendorId);
    dlgFilterDetails.m_pEditorProductID->setText(pItem->m_strProductId);
    dlgFilterDetails.m_pEditorRevision->setText(pItem->m_strRevision);
    dlgFilterDetails.m_pEditorPort->setText(pItem->m_strPort);
    dlgFilterDetails.m_pEditorManufacturer->setText(pItem->m_strManufacturer);
    dlgFilterDetails.m_pEditorProduct->setText(pItem->m_strProduct);
    dlgFilterDetails.m_pEditorSerialNo->setText(pItem->m_strSerialNumber);
    const QString strRemote = pItem->m_strRemote.toLower();
    if (strRemote == "yes" || strRemote == "true" || strRemote == "1")
        dlgFilterDetails.m_pComboRemote->setCurrentIndex(ModeOn);
    else if (strRemote == "no" || strRemote == "false" || strRemote == "0")
        dlgFilterDetails.m_pComboRemote->setCurrentIndex(ModeOff);
    else
        dlgFilterDetails.m_pComboRemote->setCurrentIndex(ModeAny);

    /* Run USB filter details dialog: */
    if (dlgFilterDetails.exec() == QDialog::Accepted)
    {
        /* Update edited tree-widget item: */
        pItem->m_strName = dlgFilterDetails.m_pEditorName->text().isEmpty() ? QString() : dlgFilterDetails.m_pEditorName->text();
        pItem->m_strVendorId = dlgFilterDetails.m_pEditorVendorID->text().isEmpty() ? QString() : dlgFilterDetails.m_pEditorVendorID->text();
        pItem->m_strProductId = dlgFilterDetails.m_pEditorProductID->text().isEmpty() ? QString() : dlgFilterDetails.m_pEditorProductID->text();
        pItem->m_strRevision = dlgFilterDetails.m_pEditorRevision->text().isEmpty() ? QString() : dlgFilterDetails.m_pEditorRevision->text();
        pItem->m_strManufacturer = dlgFilterDetails.m_pEditorManufacturer->text().isEmpty() ? QString() : dlgFilterDetails.m_pEditorManufacturer->text();
        pItem->m_strProduct = dlgFilterDetails.m_pEditorProduct->text().isEmpty() ? QString() : dlgFilterDetails.m_pEditorProduct->text();
        pItem->m_strSerialNumber = dlgFilterDetails.m_pEditorSerialNo->text().isEmpty() ? QString() : dlgFilterDetails.m_pEditorSerialNo->text();
        pItem->m_strPort = dlgFilterDetails.m_pEditorPort->text().isEmpty() ? QString() : dlgFilterDetails.m_pEditorPort->text();
        switch (dlgFilterDetails.m_pComboRemote->currentIndex())
        {
            case ModeAny: pItem->m_strRemote = QString(); break;
            case ModeOn:  pItem->m_strRemote = QString::number(1); break;
            case ModeOff: pItem->m_strRemote = QString::number(0); break;
            default: AssertMsgFailed(("Invalid combo box index"));
        }
        pItem->updateFields();
    }
}

void UIMachineSettingsUSB::sltRemoveFilter()
{
    /* Check current USB filter item: */
    QTreeWidgetItem *pItem = m_pTreeWidgetFilters->currentItem();
    AssertPtrReturnVoid(pItem);

    /* Delete corresponding item: */
    delete pItem;

    /* Update current item: */
    sltHandleCurrentItemChange(m_pTreeWidgetFilters->currentItem());

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsUSB::sltMoveFilterUp()
{
    /* Check current USB filter item: */
    QTreeWidgetItem *pItem = m_pTreeWidgetFilters->currentItem();
    AssertPtrReturnVoid(pItem);

    /* Move the item up: */
    const int iIndex = m_pTreeWidgetFilters->indexOfTopLevelItem(pItem);
    QTreeWidgetItem *pTakenItem = m_pTreeWidgetFilters->takeTopLevelItem(iIndex);
    Assert(pItem == pTakenItem);
    m_pTreeWidgetFilters->insertTopLevelItem(iIndex - 1, pTakenItem);

    /* Make sure moved item still chosen: */
    m_pTreeWidgetFilters->setCurrentItem(pTakenItem);
}

void UIMachineSettingsUSB::sltMoveFilterDown()
{
    /* Check current USB filter item: */
    QTreeWidgetItem *pItem = m_pTreeWidgetFilters->currentItem();
    AssertPtrReturnVoid(pItem);

    /* Move the item down: */
    const int iIndex = m_pTreeWidgetFilters->indexOfTopLevelItem(pItem);
    QTreeWidgetItem *pTakenItem = m_pTreeWidgetFilters->takeTopLevelItem(iIndex);
    Assert(pItem == pTakenItem);
    m_pTreeWidgetFilters->insertTopLevelItem(iIndex + 1, pTakenItem);

    /* Make sure moved item still chosen: */
    m_pTreeWidgetFilters->setCurrentItem(pTakenItem);
}

void UIMachineSettingsUSB::prepare()
{
    /* Prepare cache: */
    m_pCache = new UISettingsCacheMachineUSB;
    AssertPtrReturnVoid(m_pCache);

    /* Prepare everything: */
    prepareWidgets();
    prepareConnections();

    /* Apply language settings: */
    retranslateUi();
}

void UIMachineSettingsUSB::prepareWidgets()
{
    /* Prepare main layout: */
    QGridLayout *pLayoutMain = new QGridLayout(this);
    if (pLayoutMain)
    {
        pLayoutMain->setRowStretch(2, 1);
        pLayoutMain->setColumnStretch(1, 1);

        /* Prepare USB check-box: */
        m_pCheckBoxUSB = new QCheckBox;
        if (m_pCheckBoxUSB)
            pLayoutMain->addWidget(m_pCheckBoxUSB, 0, 0, 1, 2);

        /* Prepare 20-px shifting spacer: */
        QSpacerItem *pSpacerItem = new QSpacerItem(20, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
        if (pSpacerItem)
            pLayoutMain->addItem(pSpacerItem, 1, 0);

        /* Prepare USB settings widget: */
        m_pWidgetUSBSettings = new QWidget(this);
        if (m_pWidgetUSBSettings)
        {
            /* Prepare USB settings widget layout: */
            m_pLayoutUSBSettings = new QVBoxLayout(m_pWidgetUSBSettings);
            if (m_pLayoutUSBSettings)
            {
                m_pLayoutUSBSettings->setContentsMargins(0, 0, 0, 0);

                /* Prepare USB radio-buttons: */
                prepareRadioButtons();

                /* Prepare separator: */
                m_pLabelSeparatorFilters = new QILabelSeparator(m_pWidgetUSBSettings);
                if (m_pLabelSeparatorFilters)
                    m_pLayoutUSBSettings->addWidget(m_pLabelSeparatorFilters);

                /* Prepare USB filters layout: */
                m_pLayoutFilters = new QHBoxLayout;
                if (m_pLayoutFilters)
                {
                    m_pLayoutFilters->setContentsMargins(0, 0, 0, 0);
                    m_pLayoutFilters->setSpacing(3);

                    /* Prepare USB filters tree-widget: */
                    prepareFiltersTreeWidget();
                    /* Prepare USB filters toolbar: */
                    prepareFiltersToolbar();

                    m_pLayoutUSBSettings->addLayout(m_pLayoutFilters);
                }
            }

            pLayoutMain->addWidget(m_pWidgetUSBSettings, 1, 1, 1, 2);
        }
    }
}

void UIMachineSettingsUSB::prepareRadioButtons()
{
    /* Prepare USB1 radio-button: */
    m_pRadioButtonUSB1 = new QRadioButton(m_pWidgetUSBSettings);
    if (m_pRadioButtonUSB1)
    {
        m_pRadioButtonUSB1->setVisible(false);
        m_pLayoutUSBSettings->addWidget(m_pRadioButtonUSB1);
    }
    /* Prepare USB2 radio-button: */
    m_pRadioButtonUSB2 = new QRadioButton(m_pWidgetUSBSettings);
    if (m_pRadioButtonUSB2)
    {
        m_pRadioButtonUSB2->setVisible(false);
        m_pLayoutUSBSettings->addWidget(m_pRadioButtonUSB2);
    }
    /* Prepare USB3 radio-button: */
    m_pRadioButtonUSB3 = new QRadioButton(m_pWidgetUSBSettings);
    if (m_pRadioButtonUSB3)
    {
        m_pRadioButtonUSB3->setVisible(false);
        m_pLayoutUSBSettings->addWidget(m_pRadioButtonUSB3);
    }
}

void UIMachineSettingsUSB::prepareFiltersTreeWidget()
{
    /* Prepare USB filters tree-widget: */
    m_pTreeWidgetFilters = new QITreeWidget(m_pWidgetUSBSettings);
    if (m_pTreeWidgetFilters)
    {
        if (m_pLabelSeparatorFilters)
            m_pLabelSeparatorFilters->setBuddy(m_pTreeWidgetFilters);
        m_pTreeWidgetFilters->header()->hide();
        m_pTreeWidgetFilters->setRootIsDecorated(false);
        m_pTreeWidgetFilters->setContextMenuPolicy(Qt::CustomContextMenu);

        m_pLayoutFilters->addWidget(m_pTreeWidgetFilters);
    }
}

void UIMachineSettingsUSB::prepareFiltersToolbar()
{
    /* Prepare USB filters toolbar: */
    m_pToolbarFilters = new QIToolBar(m_pWidgetUSBSettings);
    if (m_pToolbarFilters)
    {
        const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
        m_pToolbarFilters->setIconSize(QSize(iIconMetric, iIconMetric));
        m_pToolbarFilters->setOrientation(Qt::Vertical);

        /* Prepare 'New USB Filter' action: */
        m_pActionNew = m_pToolbarFilters->addAction(UIIconPool::iconSet(":/usb_new_16px.png",
                                                                        ":/usb_new_disabled_16px.png"),
                                                    QString(), this, SLOT(sltNewFilter()));
        if (m_pActionNew)
            m_pActionNew->setShortcuts(QList<QKeySequence>() << QKeySequence("Ins") << QKeySequence("Ctrl+N"));

        /* Prepare 'Add USB Filter' action: */
        m_pActionAdd = m_pToolbarFilters->addAction(UIIconPool::iconSet(":/usb_add_16px.png",
                                                                        ":/usb_add_disabled_16px.png"),
                                                    QString(), this, SLOT(sltAddFilter()));
        if (m_pActionAdd)
            m_pActionAdd->setShortcuts(QList<QKeySequence>() << QKeySequence("Alt+Ins") << QKeySequence("Ctrl+A"));

        /* Prepare 'Edit USB Filter' action: */
        m_pActionEdit = m_pToolbarFilters->addAction(UIIconPool::iconSet(":/usb_filter_edit_16px.png",
                                                                         ":/usb_filter_edit_disabled_16px.png"),
                                                     QString(), this, SLOT(sltEditFilter()));
        if (m_pActionEdit)
            m_pActionEdit->setShortcuts(QList<QKeySequence>() << QKeySequence("Alt+Return") << QKeySequence("Ctrl+Return"));

        /* Prepare 'Remove USB Filter' action: */
        m_pActionRemove = m_pToolbarFilters->addAction(UIIconPool::iconSet(":/usb_remove_16px.png",
                                                                           ":/usb_remove_disabled_16px.png"),
                                                       QString(), this, SLOT(sltRemoveFilter()));
        if (m_pActionRemove)
            m_pActionRemove->setShortcuts(QList<QKeySequence>() << QKeySequence("Del") << QKeySequence("Ctrl+R"));

        /* Prepare 'Move USB Filter Up' action: */
        m_pActionMoveUp = m_pToolbarFilters->addAction(UIIconPool::iconSet(":/usb_moveup_16px.png",
                                                                           ":/usb_moveup_disabled_16px.png"),
                                                       QString(), this, SLOT(sltMoveFilterUp()));
        if (m_pActionMoveUp)
            m_pActionMoveUp->setShortcuts(QList<QKeySequence>() << QKeySequence("Alt+Up") << QKeySequence("Ctrl+Up"));

        /* Prepare 'Move USB Filter Down' action: */
        m_pActionMoveDown = m_pToolbarFilters->addAction(UIIconPool::iconSet(":/usb_movedown_16px.png",
                                                                             ":/usb_movedown_disabled_16px.png"),
                                                         QString(), this, SLOT(sltMoveFilterDown()));
        if (m_pActionMoveDown)
            m_pActionMoveDown->setShortcuts(QList<QKeySequence>() << QKeySequence("Alt+Down") << QKeySequence("Ctrl+Down"));

        /* Prepare USB devices menu: */
        m_pMenuUSBDevices = new VBoxUSBMenu(this);

        m_pLayoutFilters->addWidget(m_pToolbarFilters);
    }
}

void UIMachineSettingsUSB::prepareConnections()
{
    /* Configure validation connections: */
    connect(m_pCheckBoxUSB, &QCheckBox::stateChanged, this, &UIMachineSettingsUSB::revalidate);
    connect(m_pRadioButtonUSB1, &QRadioButton::toggled, this, &UIMachineSettingsUSB::revalidate);
    connect(m_pRadioButtonUSB2, &QRadioButton::toggled, this, &UIMachineSettingsUSB::revalidate);
    connect(m_pRadioButtonUSB3, &QRadioButton::toggled, this, &UIMachineSettingsUSB::revalidate);

    /* Configure widget connections: */
    connect(m_pCheckBoxUSB, &QCheckBox::toggled,
            this, &UIMachineSettingsUSB::sltHandleUsbAdapterToggle);
    connect(m_pTreeWidgetFilters, &QITreeWidget::currentItemChanged,
            this, &UIMachineSettingsUSB::sltHandleCurrentItemChange);
    connect(m_pTreeWidgetFilters, &QITreeWidget::customContextMenuRequested,
            this, &UIMachineSettingsUSB::sltHandleContextMenuRequest);
    connect(m_pTreeWidgetFilters, &QITreeWidget::itemDoubleClicked,
            this, &UIMachineSettingsUSB::sltEditFilter);
    connect(m_pTreeWidgetFilters, &QITreeWidget::itemChanged,
            this, &UIMachineSettingsUSB::sltHandleActivityStateChange);

    /* Configure USB device menu connections: */
    connect(m_pMenuUSBDevices, &VBoxUSBMenu::triggered,
            this, &UIMachineSettingsUSB::sltAddFilterConfirmed);
}

void UIMachineSettingsUSB::cleanup()
{
    /* Cleanup USB devices menu: */
    delete m_pMenuUSBDevices;
    m_pMenuUSBDevices = 0;

    /* Cleanup cache: */
    delete m_pCache;
    m_pCache = 0;
}

void UIMachineSettingsUSB::addUSBFilterItem(const UIDataSettingsMachineUSBFilter &filterData, bool fChoose)
{
    /* Create USB filter item: */
    UIUSBFilterItem *pItem = new UIUSBFilterItem(m_pTreeWidgetFilters);
    AssertPtrReturnVoid(pItem);
    {
        /* Configure item: */
        pItem->setCheckState(0, filterData.m_fActive ? Qt::Checked : Qt::Unchecked);
        pItem->m_strName = filterData.m_strName;
        pItem->m_strVendorId = filterData.m_strVendorId;
        pItem->m_strProductId = filterData.m_strProductId;
        pItem->m_strRevision = filterData.m_strRevision;
        pItem->m_strManufacturer = filterData.m_strManufacturer;
        pItem->m_strProduct = filterData.m_strProduct;
        pItem->m_strSerialNumber = filterData.m_strSerialNumber;
        pItem->m_strPort = filterData.m_strPort;
        pItem->m_strRemote = filterData.m_strRemote;
        pItem->m_enmAction = filterData.m_enmAction;
        pItem->m_fHostUSBDevice = filterData.m_fHostUSBDevice;
        pItem->m_enmHostUSBDeviceState = filterData.m_enmHostUSBDeviceState;
        pItem->updateFields();

        /* Select this item if it's new: */
        if (fChoose)
        {
            m_pTreeWidgetFilters->scrollToItem(pItem);
            m_pTreeWidgetFilters->setCurrentItem(pItem);
            sltHandleCurrentItemChange(pItem);
        }
    }
}

bool UIMachineSettingsUSB::saveUSBData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save USB settings from the cache: */
    if (fSuccess && isMachineInValidMode() && m_pCache->wasChanged())
    {
        /* Get new USB data from the cache: */
        const UIDataSettingsMachineUSB &newUsbData = m_pCache->data();

        /* Save USB data: */
        if (fSuccess && isMachineOffline())
        {
            /* Remove USB controllers: */
            if (!newUsbData.m_fUSBEnabled)
                fSuccess = removeUSBControllers();

            else

            /* Create/update USB controllers: */
            if (newUsbData.m_fUSBEnabled)
                fSuccess = createUSBControllers(newUsbData.m_USBControllerType);
        }

        /* Save USB filters data: */
        if (fSuccess)
        {
            /* Make sure filters object really exists: */
            CUSBDeviceFilters comFiltersObject = m_machine.GetUSBDeviceFilters();
            fSuccess = m_machine.isOk() && comFiltersObject.isNotNull();

            /* Show error message if necessary: */
            if (!fSuccess)
                notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
            else
            {
                /* For each filter data set: */
                int iOperationPosition = 0;
                for (int iFilterIndex = 0; fSuccess && iFilterIndex < m_pCache->childCount(); ++iFilterIndex)
                {
                    /* Check if USB filter data was changed: */
                    const UISettingsCacheMachineUSBFilter &filterCache = m_pCache->child(iFilterIndex);

                    /* Remove filter marked for 'remove' or 'update': */
                    if (fSuccess && (filterCache.wasRemoved() || filterCache.wasUpdated()))
                    {
                        fSuccess = removeUSBFilter(comFiltersObject, iOperationPosition);
                        if (fSuccess && filterCache.wasRemoved())
                            --iOperationPosition;
                    }

                    /* Create filter marked for 'create' or 'update': */
                    if (fSuccess && (filterCache.wasCreated() || filterCache.wasUpdated()))
                        fSuccess = createUSBFilter(comFiltersObject, iOperationPosition, filterCache.data());

                    /* Advance operation position: */
                    ++iOperationPosition;
                }
            }
        }
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsUSB::removeUSBControllers(const QSet<KUSBControllerType> &types /* = QSet<KUSBControllerType>() */)
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Remove controllers: */
    if (fSuccess && isMachineOffline())
    {
        /* Get controllers for further activities: */
        const CUSBControllerVector &controllers = m_machine.GetUSBControllers();
        fSuccess = m_machine.isOk();

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));

        /* For each controller: */
        for (int iControllerIndex = 0; fSuccess && iControllerIndex < controllers.size(); ++iControllerIndex)
        {
            /* Get current controller: */
            const CUSBController &comController = controllers.at(iControllerIndex);

            /* Get controller type for further activities: */
            KUSBControllerType enmType = KUSBControllerType_Null;
            if (fSuccess)
            {
                enmType = comController.GetType();
                fSuccess = comController.isOk();
            }
            /* Get controller name for further activities: */
            QString strName;
            if (fSuccess)
            {
                strName = comController.GetName();
                fSuccess = comController.isOk();
            }

            /* Show error message if necessary: */
            if (!fSuccess)
                notifyOperationProgressError(UIErrorString::formatErrorInfo(comController));
            else
            {
                /* Pass only if requested types were not defined or contains the one we found: */
                if (!types.isEmpty() && !types.contains(enmType))
                    continue;

                /* Remove controller: */
                if (fSuccess)
                {
                    m_machine.RemoveUSBController(comController.GetName());
                    fSuccess = m_machine.isOk();
                }

                /* Show error message if necessary: */
                if (!fSuccess)
                    notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
            }
        }
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsUSB::createUSBControllers(KUSBControllerType enmType)
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Add controllers: */
    if (fSuccess && isMachineOffline())
    {
        /* Get each controller count for further activities: */
        ULONG cOhciCtls = 0;
        if (fSuccess)
        {
            cOhciCtls = m_machine.GetUSBControllerCountByType(KUSBControllerType_OHCI);
            fSuccess = m_machine.isOk();
        }
        ULONG cEhciCtls = 0;
        if (fSuccess)
        {
            cEhciCtls = m_machine.GetUSBControllerCountByType(KUSBControllerType_EHCI);
            fSuccess = m_machine.isOk();
        }
        ULONG cXhciCtls = 0;
        if (fSuccess)
        {
            cXhciCtls = m_machine.GetUSBControllerCountByType(KUSBControllerType_XHCI);
            fSuccess = m_machine.isOk();
        }

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
        else
        {
            /* For requested controller type: */
            switch (enmType)
            {
                case KUSBControllerType_OHCI:
                {
                    /* Remove excessive controllers: */
                    if (cXhciCtls || cEhciCtls)
                        fSuccess = removeUSBControllers(QSet<KUSBControllerType>()
                                                        << KUSBControllerType_XHCI
                                                        << KUSBControllerType_EHCI);

                    /* Add required controller: */
                    if (fSuccess && !cOhciCtls)
                    {
                        m_machine.AddUSBController("OHCI", KUSBControllerType_OHCI);
                        fSuccess = m_machine.isOk();

                        /* Show error message if necessary: */
                        if (!fSuccess)
                            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
                    }

                    break;
                }
                case KUSBControllerType_EHCI:
                {
                    /* Remove excessive controllers: */
                    if (cXhciCtls)
                        fSuccess = removeUSBControllers(QSet<KUSBControllerType>()
                                                        << KUSBControllerType_XHCI);

                    /* Add required controllers: */
                    if (fSuccess)
                    {
                        if (fSuccess && !cOhciCtls)
                        {
                            m_machine.AddUSBController("OHCI", KUSBControllerType_OHCI);
                            fSuccess = m_machine.isOk();
                        }
                        if (fSuccess && !cEhciCtls)
                        {
                            m_machine.AddUSBController("EHCI", KUSBControllerType_EHCI);
                            fSuccess = m_machine.isOk();
                        }

                        /* Show error message if necessary: */
                        if (!fSuccess)
                            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
                    }

                    break;
                }
                case KUSBControllerType_XHCI:
                {
                    /* Remove excessive controllers: */
                    if (cEhciCtls || cOhciCtls)
                        fSuccess = removeUSBControllers(QSet<KUSBControllerType>()
                                                        << KUSBControllerType_EHCI
                                                        << KUSBControllerType_OHCI);

                    /* Add required controller: */
                    if (fSuccess && !cXhciCtls)
                    {
                        m_machine.AddUSBController("xHCI", KUSBControllerType_XHCI);
                        fSuccess = m_machine.isOk();

                        /* Show error message if necessary: */
                        if (!fSuccess)
                            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
                    }

                    break;
                }
                default:
                    break;
            }
        }
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsUSB::removeUSBFilter(CUSBDeviceFilters &comFiltersObject, int iPosition)
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Remove filter: */
    if (fSuccess)
    {
        /* Remove filter: */
        comFiltersObject.RemoveDeviceFilter(iPosition);
        fSuccess = comFiltersObject.isOk();

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(comFiltersObject));
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsUSB::createUSBFilter(CUSBDeviceFilters &comFiltersObject, int iPosition, const UIDataSettingsMachineUSBFilter &filterData)
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Add filter: */
    if (fSuccess)
    {
        /* Create filter: */
        CUSBDeviceFilter comFilter = comFiltersObject.CreateDeviceFilter(filterData.m_strName);
        fSuccess = comFiltersObject.isOk() && comFilter.isNotNull();

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(comFiltersObject));
        else
        {
            /* Save whether filter is active: */
            if (fSuccess)
            {
                comFilter.SetActive(filterData.m_fActive);
                fSuccess = comFilter.isOk();
            }
            /* Save filter Vendor ID: */
            if (fSuccess)
            {
                comFilter.SetVendorId(filterData.m_strVendorId);
                fSuccess = comFilter.isOk();
            }
            /* Save filter Product ID: */
            if (fSuccess)
            {
                comFilter.SetProductId(filterData.m_strProductId);
                fSuccess = comFilter.isOk();
            }
            /* Save filter revision: */
            if (fSuccess)
            {
                comFilter.SetRevision(filterData.m_strRevision);
                fSuccess = comFilter.isOk();
            }
            /* Save filter manufacturer: */
            if (fSuccess)
            {
                comFilter.SetManufacturer(filterData.m_strManufacturer);
                fSuccess = comFilter.isOk();
            }
            /* Save filter product: */
            if (fSuccess)
            {
                comFilter.SetProduct(filterData.m_strProduct);
                fSuccess = comFilter.isOk();
            }
            /* Save filter serial number: */
            if (fSuccess)
            {
                comFilter.SetSerialNumber(filterData.m_strSerialNumber);
                fSuccess = comFilter.isOk();
            }
            /* Save filter port: */
            if (fSuccess)
            {
                comFilter.SetPort(filterData.m_strPort);
                fSuccess = comFilter.isOk();
            }
            /* Save filter remote mode: */
            if (fSuccess)
            {
                comFilter.SetRemote(filterData.m_strRemote);
                fSuccess = comFilter.isOk();
            }

            /* Show error message if necessary: */
            if (!fSuccess)
                notifyOperationProgressError(UIErrorString::formatErrorInfo(comFilter));
            else
            {
                /* Insert filter onto corresponding position: */
                comFiltersObject.InsertDeviceFilter(iPosition, comFilter);
                fSuccess = comFiltersObject.isOk();

                /* Show error message if necessary: */
                if (!fSuccess)
                    notifyOperationProgressError(UIErrorString::formatErrorInfo(comFiltersObject));
            }
        }
    }
    /* Return result: */
    return fSuccess;
}


#include "UIMachineSettingsUSB.moc"

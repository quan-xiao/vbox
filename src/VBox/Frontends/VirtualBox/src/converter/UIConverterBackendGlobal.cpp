/* $Id: UIConverterBackendGlobal.cpp 86541 2020-10-12 12:30:09Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIConverterBackendGlobal implementation.
 */

/*
 * Copyright (C) 2012-2020 Oracle Corporation
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
#include <QApplication>
#include <QHash>
#include <QRegularExpression>

/* GUI includes: */
#include "UICommon.h"
#include "UIConverterBackend.h"
#include "UIIconPool.h"

/* COM includes: */
#include "CSystemProperties.h"


/* Determines if <Object of type X> can be converted to object of other type.
 * These functions returns 'true' for all allowed conversions. */
template<> bool canConvert<SizeSuffix>() { return true; }
template<> bool canConvert<StorageSlot>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::DialogType>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::MenuType>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::MenuApplicationActionType>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::MenuHelpActionType>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::RuntimeMenuMachineActionType>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::RuntimeMenuViewActionType>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::RuntimeMenuInputActionType>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::RuntimeMenuDevicesActionType>() { return true; }
#ifdef VBOX_WITH_DEBUGGER_GUI
template<> bool canConvert<UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType>() { return true; }
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
template<> bool canConvert<UIExtraDataMetaDefs::MenuWindowActionType>() { return true; }
#endif /* VBOX_WS_MAC */
template<> bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeSystem>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeStorage>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeAudio>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeSerial>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeUsb>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeSharedFolders>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeDescription>() { return true; }
template<> bool canConvert<UIToolType>() { return true; }
template<> bool canConvert<UIVisualStateType>() { return true; }
template<> bool canConvert<DetailsElementType>() { return true; }
template<> bool canConvert<PreviewUpdateIntervalType>() { return true; }
template<> bool canConvert<GUIFeatureType>() { return true; }
template<> bool canConvert<GlobalSettingsPageType>() { return true; }
template<> bool canConvert<MachineSettingsPageType>() { return true; }
template<> bool canConvert<WizardType>() { return true; }
template<> bool canConvert<IndicatorType>() { return true; }
template<> bool canConvert<MachineCloseAction>() { return true; }
template<> bool canConvert<MouseCapturePolicy>() { return true; }
template<> bool canConvert<GuruMeditationHandlerType>() { return true; }
template<> bool canConvert<ScalingOptimizationType>() { return true; }
#ifndef VBOX_WS_MAC
template<> bool canConvert<MiniToolbarAlignment>() { return true; }
#endif
template<> bool canConvert<InformationElementType>() { return true; }
template<> bool canConvert<MaxGuestResolutionPolicy>() { return true; }
template<> bool canConvert<UIMediumFormat>() { return true; }
template<> bool canConvert<UISettingsDefs::RecordingMode>() { return true; }
template<> bool canConvert<VMResourceMonitorColumn>(){ return true; };


/* QString <= SizeSuffix: */
template<> QString toString(const SizeSuffix &sizeSuffix)
{
    QString strResult;
    switch (sizeSuffix)
    {
        case SizeSuffix_Byte:     strResult = QApplication::translate("UICommon", "B", "size suffix Bytes"); break;
        case SizeSuffix_KiloByte: strResult = QApplication::translate("UICommon", "KB", "size suffix KBytes=1024 Bytes"); break;
        case SizeSuffix_MegaByte: strResult = QApplication::translate("UICommon", "MB", "size suffix MBytes=1024 KBytes"); break;
        case SizeSuffix_GigaByte: strResult = QApplication::translate("UICommon", "GB", "size suffix GBytes=1024 MBytes"); break;
        case SizeSuffix_TeraByte: strResult = QApplication::translate("UICommon", "TB", "size suffix TBytes=1024 GBytes"); break;
        case SizeSuffix_PetaByte: strResult = QApplication::translate("UICommon", "PB", "size suffix PBytes=1024 TBytes"); break;
        default:
        {
            AssertMsgFailed(("No text for size suffix=%d", sizeSuffix));
            break;
        }
    }
    return strResult;
}

/* SizeSuffix <= QString: */
template<> SizeSuffix fromString<SizeSuffix>(const QString &strSizeSuffix)
{
    QHash<QString, SizeSuffix> list;
    list.insert(QApplication::translate("UICommon", "B", "size suffix Bytes"),               SizeSuffix_Byte);
    list.insert(QApplication::translate("UICommon", "KB", "size suffix KBytes=1024 Bytes"),  SizeSuffix_KiloByte);
    list.insert(QApplication::translate("UICommon", "MB", "size suffix MBytes=1024 KBytes"), SizeSuffix_MegaByte);
    list.insert(QApplication::translate("UICommon", "GB", "size suffix GBytes=1024 MBytes"), SizeSuffix_GigaByte);
    list.insert(QApplication::translate("UICommon", "TB", "size suffix TBytes=1024 GBytes"), SizeSuffix_TeraByte);
    list.insert(QApplication::translate("UICommon", "PB", "size suffix PBytes=1024 TBytes"), SizeSuffix_PetaByte);
    if (!list.contains(strSizeSuffix))
    {
        AssertMsgFailed(("No value for '%s'", strSizeSuffix.toUtf8().constData()));
    }
    return list.value(strSizeSuffix);
}

/* QString <= StorageSlot: */
template<> QString toString(const StorageSlot &storageSlot)
{
    QString strResult;
    switch (storageSlot.bus)
    {
        case KStorageBus_IDE:
        {
            int iMaxPort = uiCommon().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(storageSlot.bus);
            int iMaxDevice = uiCommon().virtualBox().GetSystemProperties().GetMaxDevicesPerPortForStorageBus(storageSlot.bus);
            if (storageSlot.port < 0 || storageSlot.port > iMaxPort)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d", storageSlot.bus, storageSlot.port));
                break;
            }
            if (storageSlot.device < 0 || storageSlot.device > iMaxDevice)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d & device=%d", storageSlot.bus, storageSlot.port, storageSlot.device));
                break;
            }
            if (storageSlot.port == 0 && storageSlot.device == 0)
                strResult = QApplication::translate("UICommon", "IDE Primary Master", "StorageSlot");
            else if (storageSlot.port == 0 && storageSlot.device == 1)
                strResult = QApplication::translate("UICommon", "IDE Primary Slave", "StorageSlot");
            else if (storageSlot.port == 1 && storageSlot.device == 0)
                strResult = QApplication::translate("UICommon", "IDE Secondary Master", "StorageSlot");
            else if (storageSlot.port == 1 && storageSlot.device == 1)
                strResult = QApplication::translate("UICommon", "IDE Secondary Slave", "StorageSlot");
            break;
        }
        case KStorageBus_SATA:
        {
            int iMaxPort = uiCommon().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(storageSlot.bus);
            if (storageSlot.port < 0 || storageSlot.port > iMaxPort)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d", storageSlot.bus, storageSlot.port));
                break;
            }
            if (storageSlot.device != 0)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d & device=%d", storageSlot.bus, storageSlot.port, storageSlot.device));
                break;
            }
            strResult = QApplication::translate("UICommon", "SATA Port %1", "StorageSlot").arg(storageSlot.port);
            break;
        }
        case KStorageBus_SCSI:
        {
            int iMaxPort = uiCommon().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(storageSlot.bus);
            if (storageSlot.port < 0 || storageSlot.port > iMaxPort)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d", storageSlot.bus, storageSlot.port));
                break;
            }
            if (storageSlot.device != 0)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d & device=%d", storageSlot.bus, storageSlot.port, storageSlot.device));
                break;
            }
            strResult = QApplication::translate("UICommon", "SCSI Port %1", "StorageSlot").arg(storageSlot.port);
            break;
        }
        case KStorageBus_SAS:
        {
            int iMaxPort = uiCommon().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(storageSlot.bus);
            if (storageSlot.port < 0 || storageSlot.port > iMaxPort)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d", storageSlot.bus, storageSlot.port));
                break;
            }
            if (storageSlot.device != 0)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d & device=%d", storageSlot.bus, storageSlot.port, storageSlot.device));
                break;
            }
            strResult = QApplication::translate("UICommon", "SAS Port %1", "StorageSlot").arg(storageSlot.port);
            break;
        }
        case KStorageBus_Floppy:
        {
            int iMaxDevice = uiCommon().virtualBox().GetSystemProperties().GetMaxDevicesPerPortForStorageBus(storageSlot.bus);
            if (storageSlot.port != 0)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d", storageSlot.bus, storageSlot.port));
                break;
            }
            if (storageSlot.device < 0 || storageSlot.device > iMaxDevice)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d & device=%d", storageSlot.bus, storageSlot.port, storageSlot.device));
                break;
            }
            strResult = QApplication::translate("UICommon", "Floppy Device %1", "StorageSlot").arg(storageSlot.device);
            break;
        }
        case KStorageBus_USB:
        {
            int iMaxPort = uiCommon().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(storageSlot.bus);
            if (storageSlot.port < 0 || storageSlot.port > iMaxPort)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d", storageSlot.bus, storageSlot.port));
                break;
            }
            if (storageSlot.device != 0)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d & device=%d", storageSlot.bus, storageSlot.port, storageSlot.device));
                break;
            }
            strResult = QApplication::translate("UICommon", "USB Port %1", "StorageSlot").arg(storageSlot.port);
            break;
        }
        case KStorageBus_PCIe:
        {
            int iMaxPort = uiCommon().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(storageSlot.bus);
            if (storageSlot.port < 0 || storageSlot.port > iMaxPort)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d", storageSlot.bus, storageSlot.port));
                break;
            }
            if (storageSlot.device != 0)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d & device=%d", storageSlot.bus, storageSlot.port, storageSlot.device));
                break;
            }
            strResult = QApplication::translate("UICommon", "NVMe Port %1", "StorageSlot").arg(storageSlot.port);
            break;
        }
        case KStorageBus_VirtioSCSI:
        {
            int iMaxPort = uiCommon().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(storageSlot.bus);
            if (storageSlot.port < 0 || storageSlot.port > iMaxPort)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d", storageSlot.bus, storageSlot.port));
                break;
            }
            if (storageSlot.device != 0)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d & device=%d", storageSlot.bus, storageSlot.port, storageSlot.device));
                break;
            }
            strResult = QApplication::translate("UICommon", "virtio-scsi Port %1", "StorageSlot").arg(storageSlot.port);
            break;
        }
        default:
        {
            AssertMsgFailed(("No text for bus=%d & port=%d & device=%d", storageSlot.bus, storageSlot.port, storageSlot.device));
            break;
        }
    }
    return strResult;
}

/* StorageSlot <= QString: */
template<> StorageSlot fromString<StorageSlot>(const QString &strStorageSlot)
{
    /* Prepare a hash of known port templates: */
    QHash<int, QString> templates;
    templates[0] = QApplication::translate("UICommon", "IDE Primary Master", "StorageSlot");
    templates[1] = QApplication::translate("UICommon", "IDE Primary Slave", "StorageSlot");
    templates[2] = QApplication::translate("UICommon", "IDE Secondary Master", "StorageSlot");
    templates[3] = QApplication::translate("UICommon", "IDE Secondary Slave", "StorageSlot");
    templates[4] = QApplication::translate("UICommon", "SATA Port %1", "StorageSlot");
    templates[5] = QApplication::translate("UICommon", "SCSI Port %1", "StorageSlot");
    templates[6] = QApplication::translate("UICommon", "SAS Port %1", "StorageSlot");
    templates[7] = QApplication::translate("UICommon", "Floppy Device %1", "StorageSlot");
    templates[8] = QApplication::translate("UICommon", "USB Port %1", "StorageSlot");
    templates[9] = QApplication::translate("UICommon", "NVMe Port %1", "StorageSlot");
    templates[10] = QApplication::translate("UICommon", "virtio-scsi Port %1", "StorageSlot");

    /* Search for a template index strStorageSlot corresponds to: */
    int iIndex = -1;
    QRegExp regExp;
    for (int i = 0; i < templates.size(); ++i)
    {
        regExp = QRegExp(i >= 0 && i <= 3 ? templates.value(i) : templates.value(i).arg("(\\d+)"));
        if (regExp.indexIn(strStorageSlot) != -1)
        {
            iIndex = i;
            break;
        }
    }

    /* Compose result: */
    StorageSlot result;

    /* First we determine bus type: */
    switch (iIndex)
    {
        case 0:
        case 1:
        case 2:
        case 3:  result.bus = KStorageBus_IDE; break;
        case 4:  result.bus = KStorageBus_SATA; break;
        case 5:  result.bus = KStorageBus_SCSI; break;
        case 6:  result.bus = KStorageBus_SAS; break;
        case 7:  result.bus = KStorageBus_Floppy; break;
        case 8:  result.bus = KStorageBus_USB; break;
        case 9:  result.bus = KStorageBus_PCIe; break;
        case 10: result.bus = KStorageBus_VirtioSCSI; break;
        default: AssertMsgFailed(("No storage bus for text='%s'", strStorageSlot.toUtf8().constData())); break;
    }

    /* Second we determine port/device pair: */
    switch (iIndex)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        {
            if (result.bus == KStorageBus_Null)
                break;
            const int iMaxPort = uiCommon().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(result.bus);
            const int iMaxDevice = uiCommon().virtualBox().GetSystemProperties().GetMaxDevicesPerPortForStorageBus(result.bus);
            const LONG iPort = iIndex / iMaxPort;
            const LONG iDevice = iIndex % iMaxPort;
            if (iPort < 0 || iPort > iMaxPort)
            {
                AssertMsgFailed(("No storage port for text='%s'", strStorageSlot.toUtf8().constData()));
                break;
            }
            if (iDevice < 0 || iDevice > iMaxDevice)
            {
                AssertMsgFailed(("No storage device for text='%s'", strStorageSlot.toUtf8().constData()));
                break;
            }
            result.port = iPort;
            result.device = iDevice;
            break;
        }
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        {
            if (result.bus == KStorageBus_Null)
                break;
            const int iMaxPort = uiCommon().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(result.bus);
            const LONG iPort = regExp.cap(1).toInt();
            const LONG iDevice = 0;
            if (iPort < 0 || iPort > iMaxPort)
            {
                AssertMsgFailed(("No storage port for text='%s'", strStorageSlot.toUtf8().constData()));
                break;
            }
            result.port = iPort;
            result.device = iDevice;
            break;
        }
        default:
        {
            AssertMsgFailed(("No storage slot for text='%s'", strStorageSlot.toUtf8().constData()));
            break;
        }
    }

    /* Return result: */
    return result;
}

/* QString <= UIExtraDataMetaDefs::DialogType: */
template<> QString toInternalString(const UIExtraDataMetaDefs::DialogType &enmDialogType)
{
    QString strResult;
    switch (enmDialogType)
    {
        case UIExtraDataMetaDefs::DialogType_VISOCreator: strResult = "VISOCreator"; break;
        case UIExtraDataMetaDefs::DialogType_All:         strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for dialog type=%d", enmDialogType));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::DialogType <= QString: */
template<> UIExtraDataMetaDefs::DialogType fromInternalString<UIExtraDataMetaDefs::DialogType>(const QString &strDialogType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;      QList<UIExtraDataMetaDefs::DialogType> values;
    keys << "VISOCreator"; values << UIExtraDataMetaDefs::DialogType_VISOCreator;
    keys << "All";         values << UIExtraDataMetaDefs::DialogType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strDialogType, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::DialogType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strDialogType, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::MenuType: */
template<> QString toInternalString(const UIExtraDataMetaDefs::MenuType &menuType)
{
    QString strResult;
    switch (menuType)
    {
        case UIExtraDataMetaDefs::MenuType_Application: strResult = "Application"; break;
        case UIExtraDataMetaDefs::MenuType_Machine:     strResult = "Machine"; break;
        case UIExtraDataMetaDefs::MenuType_View:        strResult = "View"; break;
        case UIExtraDataMetaDefs::MenuType_Input:       strResult = "Input"; break;
        case UIExtraDataMetaDefs::MenuType_Devices:     strResult = "Devices"; break;
#ifdef VBOX_WITH_DEBUGGER_GUI
        case UIExtraDataMetaDefs::MenuType_Debug:       strResult = "Debug"; break;
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef RT_OS_DARWIN
        case UIExtraDataMetaDefs::MenuType_Window:      strResult = "Window"; break;
#endif /* RT_OS_DARWIN */
        case UIExtraDataMetaDefs::MenuType_Help:        strResult = "Help"; break;
        case UIExtraDataMetaDefs::MenuType_All:         strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for indicator type=%d", menuType));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::MenuType <= QString: */
template<> UIExtraDataMetaDefs::MenuType fromInternalString<UIExtraDataMetaDefs::MenuType>(const QString &strMenuType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;      QList<UIExtraDataMetaDefs::MenuType> values;
    keys << "Application"; values << UIExtraDataMetaDefs::MenuType_Application;
    keys << "Machine";     values << UIExtraDataMetaDefs::MenuType_Machine;
    keys << "View";        values << UIExtraDataMetaDefs::MenuType_View;
    keys << "Input";       values << UIExtraDataMetaDefs::MenuType_Input;
    keys << "Devices";     values << UIExtraDataMetaDefs::MenuType_Devices;
#ifdef VBOX_WITH_DEBUGGER_GUI
    keys << "Debug";       values << UIExtraDataMetaDefs::MenuType_Debug;
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef RT_OS_DARWIN
    keys << "Window";      values << UIExtraDataMetaDefs::MenuType_Window;
#endif /* RT_OS_DARWIN */
    keys << "Help";        values << UIExtraDataMetaDefs::MenuType_Help;
    keys << "All";         values << UIExtraDataMetaDefs::MenuType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strMenuType, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::MenuType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strMenuType, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::MenuApplicationActionType: */
template<> QString toInternalString(const UIExtraDataMetaDefs::MenuApplicationActionType &menuApplicationActionType)
{
    QString strResult;
    switch (menuApplicationActionType)
    {
#ifdef VBOX_WS_MAC
        case UIExtraDataMetaDefs::MenuApplicationActionType_About:                strResult = "About"; break;
#endif /* VBOX_WS_MAC */
        case UIExtraDataMetaDefs::MenuApplicationActionType_Preferences:          strResult = "Preferences"; break;
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
        case UIExtraDataMetaDefs::MenuApplicationActionType_NetworkAccessManager: strResult = "NetworkAccessManager"; break;
        case UIExtraDataMetaDefs::MenuApplicationActionType_CheckForUpdates:      strResult = "CheckForUpdates"; break;
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
        case UIExtraDataMetaDefs::MenuApplicationActionType_ResetWarnings:        strResult = "ResetWarnings"; break;
        case UIExtraDataMetaDefs::MenuApplicationActionType_Close:                strResult = "Close"; break;
        case UIExtraDataMetaDefs::MenuApplicationActionType_All:                  strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for action type=%d", menuApplicationActionType));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::MenuApplicationActionType <= QString: */
template<> UIExtraDataMetaDefs::MenuApplicationActionType fromInternalString<UIExtraDataMetaDefs::MenuApplicationActionType>(const QString &strMenuApplicationActionType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;               QList<UIExtraDataMetaDefs::MenuApplicationActionType> values;
#ifdef VBOX_WS_MAC
    keys << "About";                values << UIExtraDataMetaDefs::MenuApplicationActionType_About;
#endif /* VBOX_WS_MAC */
    keys << "Preferences";          values << UIExtraDataMetaDefs::MenuApplicationActionType_Preferences;
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    keys << "NetworkAccessManager"; values << UIExtraDataMetaDefs::MenuApplicationActionType_NetworkAccessManager;
    keys << "CheckForUpdates";      values << UIExtraDataMetaDefs::MenuApplicationActionType_CheckForUpdates;
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
    keys << "ResetWarnings";        values << UIExtraDataMetaDefs::MenuApplicationActionType_ResetWarnings;
    keys << "Close";                values << UIExtraDataMetaDefs::MenuApplicationActionType_Close;
    keys << "All";                  values << UIExtraDataMetaDefs::MenuApplicationActionType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strMenuApplicationActionType, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::MenuApplicationActionType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strMenuApplicationActionType, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::MenuHelpActionType: */
template<> QString toInternalString(const UIExtraDataMetaDefs::MenuHelpActionType &menuHelpActionType)
{
    QString strResult;
    switch (menuHelpActionType)
    {
        case UIExtraDataMetaDefs::MenuHelpActionType_Contents:             strResult = "Contents"; break;
        case UIExtraDataMetaDefs::MenuHelpActionType_WebSite:              strResult = "WebSite"; break;
        case UIExtraDataMetaDefs::MenuHelpActionType_BugTracker:           strResult = "BugTracker"; break;
        case UIExtraDataMetaDefs::MenuHelpActionType_Forums:               strResult = "Forums"; break;
        case UIExtraDataMetaDefs::MenuHelpActionType_Oracle:               strResult = "Oracle"; break;
#ifndef VBOX_WS_MAC
        case UIExtraDataMetaDefs::MenuHelpActionType_About:                strResult = "About"; break;
#endif /* !VBOX_WS_MAC */
        case UIExtraDataMetaDefs::MenuHelpActionType_All:                  strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for action type=%d", menuHelpActionType));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::MenuHelpActionType <= QString: */
template<> UIExtraDataMetaDefs::MenuHelpActionType fromInternalString<UIExtraDataMetaDefs::MenuHelpActionType>(const QString &strMenuHelpActionType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;               QList<UIExtraDataMetaDefs::MenuHelpActionType> values;
    keys << "Contents";             values << UIExtraDataMetaDefs::MenuHelpActionType_Contents;
    keys << "WebSite";              values << UIExtraDataMetaDefs::MenuHelpActionType_WebSite;
    keys << "BugTracker";           values << UIExtraDataMetaDefs::MenuHelpActionType_BugTracker;
    keys << "Forums";               values << UIExtraDataMetaDefs::MenuHelpActionType_Forums;
    keys << "Oracle";               values << UIExtraDataMetaDefs::MenuHelpActionType_Oracle;
#ifndef VBOX_WS_MAC
    keys << "About";                values << UIExtraDataMetaDefs::MenuHelpActionType_About;
#endif /* !VBOX_WS_MAC */
    keys << "All";                  values << UIExtraDataMetaDefs::MenuHelpActionType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strMenuHelpActionType, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::MenuHelpActionType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strMenuHelpActionType, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::RuntimeMenuMachineActionType: */
template<> QString toInternalString(const UIExtraDataMetaDefs::RuntimeMenuMachineActionType &runtimeMenuMachineActionType)
{
    QString strResult;
    switch (runtimeMenuMachineActionType)
    {
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_SettingsDialog:                strResult = "SettingsDialog"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_TakeSnapshot:                  strResult = "TakeSnapshot"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_InformationDialog:             strResult = "InformationDialog"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_FileManagerDialog:             strResult = "FileManagerDialog"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_GuestProcessControlDialog:     strResult = "GuestProcessControlDialog"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Pause:                         strResult = "Pause"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Reset:                         strResult = "Reset"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Detach:                        strResult = "Detach"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_SaveState:                     strResult = "SaveState"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Shutdown:                      strResult = "Shutdown"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_PowerOff:                      strResult = "PowerOff"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Nothing:                       strResult = "Nothing"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_All:                           strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for action type=%d", runtimeMenuMachineActionType));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::RuntimeMenuMachineActionType <= QString: */
template<> UIExtraDataMetaDefs::RuntimeMenuMachineActionType fromInternalString<UIExtraDataMetaDefs::RuntimeMenuMachineActionType>(const QString &strRuntimeMenuMachineActionType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;             QList<UIExtraDataMetaDefs::RuntimeMenuMachineActionType> values;
    keys << "SettingsDialog";                values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_SettingsDialog;
    keys << "TakeSnapshot";                  values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_TakeSnapshot;
    keys << "InformationDialog";             values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_InformationDialog;
    keys << "FileManagerDialog";             values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_FileManagerDialog;
    keys << "GuestProcessControlDialog";     values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_GuestProcessControlDialog;
    keys << "Pause";                         values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Pause;
    keys << "Reset";                         values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Reset;
    keys << "Detach";                        values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Detach;
    keys << "SaveState";                     values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_SaveState;
    keys << "Shutdown";                      values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Shutdown;
    keys << "PowerOff";                      values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_PowerOff;
    keys << "Nothing";                       values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Nothing;
    keys << "All";                           values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strRuntimeMenuMachineActionType, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strRuntimeMenuMachineActionType, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::RuntimeMenuViewActionType: */
template<> QString toInternalString(const UIExtraDataMetaDefs::RuntimeMenuViewActionType &runtimeMenuViewActionType)
{
    QString strResult;
    switch (runtimeMenuViewActionType)
    {
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_Fullscreen:           strResult = "Fullscreen"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_Seamless:             strResult = "Seamless"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_Scale:                strResult = "Scale"; break;
#ifndef VBOX_WS_MAC
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_MinimizeWindow:       strResult = "MinimizeWindow"; break;
#endif /* !VBOX_WS_MAC */
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_AdjustWindow:         strResult = "AdjustWindow"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_GuestAutoresize:      strResult = "GuestAutoresize"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_TakeScreenshot:       strResult = "TakeScreenshot"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_Recording:            strResult = "Recording"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_RecordingSettings:    strResult = "RecordingSettings"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_StartRecording:       strResult = "StartRecording"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_VRDEServer:           strResult = "VRDEServer"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_MenuBar:              strResult = "MenuBar"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_MenuBarSettings:      strResult = "MenuBarSettings"; break;
#ifndef VBOX_WS_MAC
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_ToggleMenuBar:        strResult = "ToggleMenuBar"; break;
#endif /* !VBOX_WS_MAC */
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_StatusBar:            strResult = "StatusBar"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_StatusBarSettings:    strResult = "StatusBarSettings"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_ToggleStatusBar:      strResult = "ToggleStatusBar"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_Resize:               strResult = "Resize"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_Remap:                strResult = "Remap"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_Rescale:              strResult = "Rescale"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_All:                  strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for action type=%d", runtimeMenuViewActionType));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::RuntimeMenuViewActionType <= QString: */
template<> UIExtraDataMetaDefs::RuntimeMenuViewActionType fromInternalString<UIExtraDataMetaDefs::RuntimeMenuViewActionType>(const QString &strRuntimeMenuViewActionType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;               QList<UIExtraDataMetaDefs::RuntimeMenuViewActionType> values;
    keys << "Fullscreen";           values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_Fullscreen;
    keys << "Seamless";             values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_Seamless;
    keys << "Scale";                values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_Scale;
#ifndef VBOX_WS_MAC
    keys << "MinimizeWindow";       values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_MinimizeWindow;
#endif /* !VBOX_WS_MAC */
    keys << "AdjustWindow";         values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_AdjustWindow;
    keys << "GuestAutoresize";      values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_GuestAutoresize;
    keys << "TakeScreenshot";       values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_TakeScreenshot;
    keys << "Recording";            values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_Recording;
    keys << "RecordingSettings";    values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_RecordingSettings;
    keys << "StartRecording";       values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_StartRecording;
    keys << "VRDEServer";           values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_VRDEServer;
    keys << "MenuBar";              values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_MenuBar;
    keys << "MenuBarSettings";      values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_MenuBarSettings;
#ifndef VBOX_WS_MAC
    keys << "ToggleMenuBar";        values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_ToggleMenuBar;
#endif /* !VBOX_WS_MAC */
    keys << "StatusBar";            values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_StatusBar;
    keys << "StatusBarSettings";    values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_StatusBarSettings;
    keys << "ToggleStatusBar";      values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_ToggleStatusBar;
    keys << "Resize";               values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_Resize;
    keys << "Remap";                values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_Remap;
    keys << "Rescale";              values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_Rescale;
    keys << "All";                  values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strRuntimeMenuViewActionType, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::RuntimeMenuViewActionType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strRuntimeMenuViewActionType, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::RuntimeMenuInputActionType: */
template<> QString toInternalString(const UIExtraDataMetaDefs::RuntimeMenuInputActionType &runtimeMenuInputActionType)
{
    QString strResult;
    switch (runtimeMenuInputActionType)
    {
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_Keyboard:           strResult = "Keyboard"; break;
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_KeyboardSettings:   strResult = "KeyboardSettings"; break;
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_SoftKeyboard:       strResult = "SoftKeyboard"; break;
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCAD:            strResult = "TypeCAD"; break;
#ifdef VBOX_WS_X11
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCABS:           strResult = "TypeCABS"; break;
#endif /* VBOX_WS_X11 */
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCtrlBreak:      strResult = "TypeCtrlBreak"; break;
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeInsert:         strResult = "TypeInsert"; break;
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypePrintScreen:    strResult = "TypePrintScreen"; break;
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeAltPrintScreen: strResult = "TypeAltPrintScreen"; break;
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_Mouse:              strResult = "Mouse"; break;
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_MouseIntegration:   strResult = "MouseIntegration"; break;
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeHostKeyCombo:   strResult = "TypeHostKeyCombo"; break;
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_All:                strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for action type=%d", runtimeMenuInputActionType));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::RuntimeMenuInputActionType <= QString: */
template<> UIExtraDataMetaDefs::RuntimeMenuInputActionType fromInternalString<UIExtraDataMetaDefs::RuntimeMenuInputActionType>(const QString &strRuntimeMenuInputActionType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;             QList<UIExtraDataMetaDefs::RuntimeMenuInputActionType> values;
    keys << "Keyboard";           values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_Keyboard;
    keys << "KeyboardSettings";   values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_KeyboardSettings;
    keys << "SoftKeyboard";       values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_SoftKeyboard;
    keys << "TypeCAD";            values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCAD;
#ifdef VBOX_WS_X11
    keys << "TypeCABS";           values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCABS;
#endif /* VBOX_WS_X11 */
    keys << "TypeCtrlBreak";      values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCtrlBreak;
    keys << "TypeInsert";         values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeInsert;
    keys << "TypePrintScreen";    values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypePrintScreen;
    keys << "TypeAltPrintScreen"; values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeAltPrintScreen;
    keys << "Mouse";              values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_Mouse;
    keys << "MouseIntegration";   values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_MouseIntegration;
    keys << "TypeHostKeyCombo";   values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeHostKeyCombo;
    keys << "All";                values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strRuntimeMenuInputActionType, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::RuntimeMenuInputActionType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strRuntimeMenuInputActionType, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::RuntimeMenuDevicesActionType: */
template<> QString toInternalString(const UIExtraDataMetaDefs::RuntimeMenuDevicesActionType &runtimeMenuDevicesActionType)
{
    QString strResult;
    switch (runtimeMenuDevicesActionType)
    {
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_HardDrives:            strResult = "HardDrives"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_HardDrivesSettings:    strResult = "HardDrivesSettings"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_OpticalDevices:        strResult = "OpticalDevices"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_FloppyDevices:         strResult = "FloppyDevices"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Audio:                 strResult = "Audio"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_AudioOutput:           strResult = "AudioOutput"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_AudioInput:            strResult = "AudioInput"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Network:               strResult = "Network"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_NetworkSettings:       strResult = "NetworkSettings"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_USBDevices:            strResult = "USBDevices"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_USBDevicesSettings:    strResult = "USBDevicesSettings"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_WebCams:               strResult = "WebCams"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedClipboard:       strResult = "SharedClipboard"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_DragAndDrop:           strResult = "DragAndDrop"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedFolders:         strResult = "SharedFolders"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedFoldersSettings: strResult = "SharedFoldersSettings"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_InstallGuestTools:     strResult = "InstallGuestTools"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Nothing:               strResult = "Nothing"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_All:                   strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for action type=%d", runtimeMenuDevicesActionType));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::RuntimeMenuDevicesActionType <= QString: */
template<> UIExtraDataMetaDefs::RuntimeMenuDevicesActionType fromInternalString<UIExtraDataMetaDefs::RuntimeMenuDevicesActionType>(const QString &strRuntimeMenuDevicesActionType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;                QList<UIExtraDataMetaDefs::RuntimeMenuDevicesActionType> values;
    keys << "HardDrives";            values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_HardDrives;
    keys << "HardDrivesSettings";    values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_HardDrivesSettings;
    keys << "OpticalDevices";        values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_OpticalDevices;
    keys << "FloppyDevices";         values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_FloppyDevices;
    keys << "Audio";                 values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Audio;
    keys << "AudioOutput";           values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_AudioOutput;
    keys << "AudioInput";            values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_AudioInput;
    keys << "Network";               values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Network;
    keys << "NetworkSettings";       values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_NetworkSettings;
    keys << "USBDevices";            values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_USBDevices;
    keys << "USBDevicesSettings";    values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_USBDevicesSettings;
    keys << "WebCams";               values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_WebCams;
    keys << "SharedClipboard";       values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedClipboard;
    keys << "DragAndDrop";           values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_DragAndDrop;
    keys << "SharedFolders";         values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedFolders;
    keys << "SharedFoldersSettings"; values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedFoldersSettings;
    keys << "InstallGuestTools";     values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_InstallGuestTools;
    keys << "Nothing";               values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Nothing;
    keys << "All";                   values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strRuntimeMenuDevicesActionType, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strRuntimeMenuDevicesActionType, Qt::CaseInsensitive)));
}

#ifdef VBOX_WITH_DEBUGGER_GUI
/* QString <= UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType: */
template<> QString toInternalString(const UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType &runtimeMenuDebuggerActionType)
{
    QString strResult;
    switch (runtimeMenuDebuggerActionType)
    {
        case UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_Statistics:            strResult = "Statistics"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_CommandLine:           strResult = "CommandLine"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_Logging:               strResult = "Logging"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_LogDialog:             strResult = "LogDialog"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_GuestControlConsole:   strResult = "GuestControlConsole"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_All:                   strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for action type=%d", runtimeMenuDebuggerActionType));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType <= QString: */
template<> UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType fromInternalString<UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType>(const QString &strRuntimeMenuDebuggerActionType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;      QList<UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType> values;
    keys << "Statistics";           values << UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_Statistics;
    keys << "CommandLine";          values << UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_CommandLine;
    keys << "Logging";              values << UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_Logging;
    keys << "LogDialog";            values << UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_LogDialog;
    keys << "GuestControlConsole";  values << UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_GuestControlConsole;
    keys << "All";                  values << UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strRuntimeMenuDebuggerActionType, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strRuntimeMenuDebuggerActionType, Qt::CaseInsensitive)));
}
#endif /* VBOX_WITH_DEBUGGER_GUI */

#ifdef VBOX_WS_MAC
/* QString <= UIExtraDataMetaDefs::MenuWindowActionType: */
template<> QString toInternalString(const UIExtraDataMetaDefs::MenuWindowActionType &menuWindowActionType)
{
    QString strResult;
    switch (menuWindowActionType)
    {
        case UIExtraDataMetaDefs::MenuWindowActionType_Minimize: strResult = "Minimize"; break;
        case UIExtraDataMetaDefs::MenuWindowActionType_Switch:   strResult = "Switch"; break;
        case UIExtraDataMetaDefs::MenuWindowActionType_All:      strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for action type=%d", menuWindowActionType));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::MenuWindowActionType <= QString: */
template<> UIExtraDataMetaDefs::MenuWindowActionType fromInternalString<UIExtraDataMetaDefs::MenuWindowActionType>(const QString &strMenuWindowActionType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;   QList<UIExtraDataMetaDefs::MenuWindowActionType> values;
    keys << "Minimize"; values << UIExtraDataMetaDefs::MenuWindowActionType_Minimize;
    keys << "Switch";   values << UIExtraDataMetaDefs::MenuWindowActionType_Switch;
    keys << "All";      values << UIExtraDataMetaDefs::MenuWindowActionType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strMenuWindowActionType, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::MenuWindowActionType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strMenuWindowActionType, Qt::CaseInsensitive)));
}
#endif /* VBOX_WS_MAC */

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral: */
template<> QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral &enmDetailsElementOptionTypeGeneral)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeGeneral)
    {
        case UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral_Name:     strResult = QApplication::translate("UICommon", "Name"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral_OS:       strResult = QApplication::translate("UICommon", "OS"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral_Location: strResult = QApplication::translate("UICommon", "Location"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral_Groups:   strResult = QApplication::translate("UICommon", "Groups"); break;
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeGeneral));
            break;
        }
    }
    return strResult;
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral: */
template<> QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral &enmDetailsElementOptionTypeGeneral)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeGeneral)
    {
        case UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral_Name:     strResult = "Name"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral_OS:       strResult = "OS"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral_Location: strResult = "Location"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral_Groups:   strResult = "Groups"; break;
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeGeneral));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral <= QString: */
template<> UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral>(const QString &strDetailsElementOptionTypeGeneral)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;   QList<UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral> values;
    keys << "Name";     values << UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral_Name;
    keys << "OS";       values << UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral_OS;
    keys << "Location"; values << UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral_Location;
    keys << "Groups";   values << UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral_Groups;
    /* Invalid type for unknown words: */
    if (!keys.contains(strDetailsElementOptionTypeGeneral, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strDetailsElementOptionTypeGeneral, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeSystem: */
template<> QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeSystem &enmDetailsElementOptionTypeSystem)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeSystem)
    {
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_RAM:             strResult = QApplication::translate("UICommon", "RAM"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_CPUCount:        strResult = QApplication::translate("UICommon", "CPU Count"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_CPUExecutionCap: strResult = QApplication::translate("UICommon", "CPU Execution Cap"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_BootOrder:       strResult = QApplication::translate("UICommon", "Boot Order"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_ChipsetType:     strResult = QApplication::translate("UICommon", "Chipset Type"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_Firmware:        strResult = QApplication::translate("UICommon", "Firmware"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_Acceleration:    strResult = QApplication::translate("UICommon", "Acceleration"); break;
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeSystem));
            break;
        }
    }
    return strResult;
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeSystem: */
template<> QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeSystem &enmDetailsElementOptionTypeSystem)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeSystem)
    {
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_RAM:             strResult = "RAM"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_CPUCount:        strResult = "CPUCount"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_CPUExecutionCap: strResult = "CPUExecutionCap"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_BootOrder:       strResult = "BootOrder"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_ChipsetType:     strResult = "ChipsetType"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_Firmware:        strResult = "Firmware"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_Acceleration:    strResult = "Acceleration"; break;
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeSystem));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::DetailsElementOptionTypeSystem <= QString: */
template<> UIExtraDataMetaDefs::DetailsElementOptionTypeSystem fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeSystem>(const QString &strDetailsElementOptionTypeSystem)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;          QList<UIExtraDataMetaDefs::DetailsElementOptionTypeSystem> values;
    keys << "RAM";             values << UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_RAM;
    keys << "CPUCount";        values << UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_CPUCount;
    keys << "CPUExecutionCap"; values << UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_CPUExecutionCap;
    keys << "BootOrder";       values << UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_BootOrder;
    keys << "ChipsetType";     values << UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_ChipsetType;
    keys << "Firmware";        values << UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_Firmware;
    keys << "Acceleration";    values << UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_Acceleration;
    /* Invalid type for unknown words: */
    if (!keys.contains(strDetailsElementOptionTypeSystem, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strDetailsElementOptionTypeSystem, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay: */
template<> QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay &enmDetailsElementOptionTypeDisplay)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeDisplay)
    {
        case UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_VRAM:               strResult = QApplication::translate("UICommon", "VRAM"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_ScreenCount:        strResult = QApplication::translate("UICommon", "Screen Count"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_ScaleFactor:        strResult = QApplication::translate("UICommon", "Scale Factor"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_GraphicsController: strResult = QApplication::translate("UICommon", "Graphics Controller"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_Acceleration:       strResult = QApplication::translate("UICommon", "Acceleration"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_VRDE:               strResult = QApplication::translate("UICommon", "VRDE"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_Recording:          strResult = QApplication::translate("UICommon", "Recording"); break;
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeDisplay));
            break;
        }
    }
    return strResult;
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay: */
template<> QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay &enmDetailsElementOptionTypeDisplay)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeDisplay)
    {
        case UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_VRAM:               strResult = "VRAM"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_ScreenCount:        strResult = "ScreenCount"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_ScaleFactor:        strResult = "ScaleFactor"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_GraphicsController: strResult = "GraphicsController"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_Acceleration:       strResult = "Acceleration"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_VRDE:               strResult = "VRDE"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_Recording:          strResult = "Recording"; break;
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeDisplay));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay <= QString: */
template<> UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay>(const QString &strDetailsElementOptionTypeDisplay)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;             QList<UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay> values;
    keys << "VRAM";               values << UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_VRAM;
    keys << "ScreenCount";        values << UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_ScreenCount;
    keys << "ScaleFactor";        values << UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_ScaleFactor;
    keys << "GraphicsController"; values << UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_GraphicsController;
    keys << "Acceleration";       values << UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_Acceleration;
    keys << "VRDE";               values << UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_VRDE;
    keys << "Recording";          values << UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_Recording;
    /* Invalid type for unknown words: */
    if (!keys.contains(strDetailsElementOptionTypeDisplay, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strDetailsElementOptionTypeDisplay, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeStorage: */
template<> QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeStorage &enmDetailsElementOptionTypeStorage)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeStorage)
    {
        case UIExtraDataMetaDefs::DetailsElementOptionTypeStorage_HardDisks:      strResult = QApplication::translate("UICommon", "Hard Disks"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeStorage_OpticalDevices: strResult = QApplication::translate("UICommon", "Optical Devices"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeStorage_FloppyDevices:  strResult = QApplication::translate("UICommon", "Floppy Devices"); break;
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeStorage));
            break;
        }
    }
    return strResult;
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeStorage: */
template<> QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeStorage &enmDetailsElementOptionTypeStorage)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeStorage)
    {
        case UIExtraDataMetaDefs::DetailsElementOptionTypeStorage_HardDisks:      strResult = "HardDisks"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeStorage_OpticalDevices: strResult = "OpticalDevices"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeStorage_FloppyDevices:  strResult = "FloppyDevices"; break;
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeStorage));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::DetailsElementOptionTypeStorage <= QString: */
template<> UIExtraDataMetaDefs::DetailsElementOptionTypeStorage fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeStorage>(const QString &strDetailsElementOptionTypeStorage)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;         QList<UIExtraDataMetaDefs::DetailsElementOptionTypeStorage> values;
    keys << "HardDisks";      values << UIExtraDataMetaDefs::DetailsElementOptionTypeStorage_HardDisks;
    keys << "OpticalDevices"; values << UIExtraDataMetaDefs::DetailsElementOptionTypeStorage_OpticalDevices;
    keys << "FloppyDevices";  values << UIExtraDataMetaDefs::DetailsElementOptionTypeStorage_FloppyDevices;
    /* Invalid type for unknown words: */
    if (!keys.contains(strDetailsElementOptionTypeStorage, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::DetailsElementOptionTypeStorage_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strDetailsElementOptionTypeStorage, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeAudio: */
template<> QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeAudio &enmDetailsElementOptionTypeAudio)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeAudio)
    {
        case UIExtraDataMetaDefs::DetailsElementOptionTypeAudio_Driver:     strResult = QApplication::translate("UICommon", "Driver"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeAudio_Controller: strResult = QApplication::translate("UICommon", "Controller"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeAudio_IO:         strResult = QApplication::translate("UICommon", "Input/Output"); break;
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeAudio));
            break;
        }
    }
    return strResult;
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeAudio: */
template<> QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeAudio &enmDetailsElementOptionTypeAudio)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeAudio)
    {
        case UIExtraDataMetaDefs::DetailsElementOptionTypeAudio_Driver:     strResult = "Driver"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeAudio_Controller: strResult = "Controller"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeAudio_IO:         strResult = "IO"; break;
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeAudio));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::DetailsElementOptionTypeAudio <= QString: */
template<> UIExtraDataMetaDefs::DetailsElementOptionTypeAudio fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeAudio>(const QString &strDetailsElementOptionTypeAudio)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;     QList<UIExtraDataMetaDefs::DetailsElementOptionTypeAudio> values;
    keys << "Driver";     values << UIExtraDataMetaDefs::DetailsElementOptionTypeAudio_Driver;
    keys << "Controller"; values << UIExtraDataMetaDefs::DetailsElementOptionTypeAudio_Controller;
    keys << "IO";         values << UIExtraDataMetaDefs::DetailsElementOptionTypeAudio_IO;
    /* Invalid type for unknown words: */
    if (!keys.contains(strDetailsElementOptionTypeAudio, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::DetailsElementOptionTypeAudio_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strDetailsElementOptionTypeAudio, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork: */
template<> QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork &enmDetailsElementOptionTypeNetwork)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeNetwork)
    {
        case UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_NotAttached:     strResult = QApplication::translate("UICommon", "Not Attached", "network adapter"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_NAT:             strResult = QApplication::translate("UICommon", "NAT"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_BridgetAdapter:  strResult = QApplication::translate("UICommon", "Bridget Adapter"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_InternalNetwork: strResult = QApplication::translate("UICommon", "Internal Network"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_HostOnlyAdapter: strResult = QApplication::translate("UICommon", "Host Only Adapter"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_GenericDriver:   strResult = QApplication::translate("UICommon", "Generic Driver"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_NATNetwork:      strResult = QApplication::translate("UICommon", "NAT Network"); break;
#ifdef VBOX_WITH_CLOUD_NET
        case UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_CloudNetwork:    strResult = QApplication::translate("UICommon", "Cloud Network"); break;
#endif /* VBOX_WITH_CLOUD_NET */
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeNetwork));
            break;
        }
    }
    return strResult;
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork: */
template<> QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork &enmDetailsElementOptionTypeNetwork)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeNetwork)
    {
        case UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_NotAttached:     strResult = "NotAttached"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_NAT:             strResult = "NAT"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_BridgetAdapter:  strResult = "BridgetAdapter"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_InternalNetwork: strResult = "InternalNetwork"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_HostOnlyAdapter: strResult = "HostOnlyAdapter"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_GenericDriver:   strResult = "GenericDriver"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_NATNetwork:      strResult = "NATNetwork"; break;
#ifdef VBOX_WITH_CLOUD_NET
        case UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_CloudNetwork:    strResult = "CloudNetwork"; break;
#endif /* VBOX_WITH_CLOUD_NET */
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeNetwork));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork <= QString: */
template<> UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork>(const QString &strDetailsElementOptionTypeNetwork)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;          QList<UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork> values;
    keys << "NotAttached";     values << UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_NotAttached;
    keys << "NAT";             values << UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_NAT;
    keys << "BridgetAdapter";  values << UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_BridgetAdapter;
    keys << "InternalNetwork"; values << UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_InternalNetwork;
    keys << "HostOnlyAdapter"; values << UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_HostOnlyAdapter;
    keys << "GenericDriver";   values << UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_GenericDriver;
    keys << "NATNetwork";      values << UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_NATNetwork;
#ifdef VBOX_WITH_CLOUD_NET
    keys << "CloudNetwork";    values << UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_CloudNetwork;
#endif /* VBOX_WITH_CLOUD_NET */
    /* Invalid type for unknown words: */
    if (!keys.contains(strDetailsElementOptionTypeNetwork, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strDetailsElementOptionTypeNetwork, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeSerial: */
template<> QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeSerial &enmDetailsElementOptionTypeSerial)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeSerial)
    {
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSerial_Disconnected: strResult = QApplication::translate("UICommon", "Disconnected", "serial port"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSerial_HostPipe:     strResult = QApplication::translate("UICommon", "Host Pipe"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSerial_HostDevice:   strResult = QApplication::translate("UICommon", "Host Device"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSerial_RawFile:      strResult = QApplication::translate("UICommon", "Raw File"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSerial_TCP:          strResult = QApplication::translate("UICommon", "TCP"); break;
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeSerial));
            break;
        }
    }
    return strResult;
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeSerial: */
template<> QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeSerial &enmDetailsElementOptionTypeSerial)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeSerial)
    {
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSerial_Disconnected: strResult = "Disconnected"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSerial_HostPipe:     strResult = "HostPipe"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSerial_HostDevice:   strResult = "HostDevice"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSerial_RawFile:      strResult = "RawFile"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeSerial_TCP:          strResult = "TCP"; break;
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeSerial));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::DetailsElementOptionTypeSerial <= QString: */
template<> UIExtraDataMetaDefs::DetailsElementOptionTypeSerial fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeSerial>(const QString &strDetailsElementOptionTypeSerial)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;       QList<UIExtraDataMetaDefs::DetailsElementOptionTypeSerial> values;
    keys << "Disconnected"; values << UIExtraDataMetaDefs::DetailsElementOptionTypeSerial_Disconnected;
    keys << "HostPipe";     values << UIExtraDataMetaDefs::DetailsElementOptionTypeSerial_HostPipe;
    keys << "HostDevice";   values << UIExtraDataMetaDefs::DetailsElementOptionTypeSerial_HostDevice;
    keys << "RawFile";      values << UIExtraDataMetaDefs::DetailsElementOptionTypeSerial_RawFile;
    keys << "TCP";          values << UIExtraDataMetaDefs::DetailsElementOptionTypeSerial_TCP;
    /* Invalid type for unknown words: */
    if (!keys.contains(strDetailsElementOptionTypeSerial, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::DetailsElementOptionTypeSerial_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strDetailsElementOptionTypeSerial, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeUsb: */
template<> QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeUsb &enmDetailsElementOptionTypeUsb)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeUsb)
    {
        case UIExtraDataMetaDefs::DetailsElementOptionTypeUsb_Controller:    strResult = QApplication::translate("UICommon", "Controller"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeUsb_DeviceFilters: strResult = QApplication::translate("UICommon", "Device Filters"); break;
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeUsb));
            break;
        }
    }
    return strResult;
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeUsb: */
template<> QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeUsb &enmDetailsElementOptionTypeUsb)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeUsb)
    {
        case UIExtraDataMetaDefs::DetailsElementOptionTypeUsb_Controller:    strResult = "Controller"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeUsb_DeviceFilters: strResult = "DeviceFilters"; break;
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeUsb));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::DetailsElementOptionTypeUsb <= QString: */
template<> UIExtraDataMetaDefs::DetailsElementOptionTypeUsb fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeUsb>(const QString &strDetailsElementOptionTypeUsb)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;        QList<UIExtraDataMetaDefs::DetailsElementOptionTypeUsb> values;
    keys << "Controller";    values << UIExtraDataMetaDefs::DetailsElementOptionTypeUsb_Controller;
    keys << "DeviceFilters"; values << UIExtraDataMetaDefs::DetailsElementOptionTypeUsb_DeviceFilters;
    /* Invalid type for unknown words: */
    if (!keys.contains(strDetailsElementOptionTypeUsb, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::DetailsElementOptionTypeUsb_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strDetailsElementOptionTypeUsb, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeSharedFolders: */
template<> QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeSharedFolders &enmDetailsElementOptionTypeSharedFolders)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeSharedFolders)
    {
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeSharedFolders));
            break;
        }
    }
    return strResult;
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeSharedFolders: */
template<> QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeSharedFolders &enmDetailsElementOptionTypeSharedFolders)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeSharedFolders)
    {
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeSharedFolders));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::DetailsElementOptionTypeSharedFolders <= QString: */
template<> UIExtraDataMetaDefs::DetailsElementOptionTypeSharedFolders fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeSharedFolders>(const QString &strDetailsElementOptionTypeSharedFolders)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;  QList<UIExtraDataMetaDefs::DetailsElementOptionTypeSharedFolders> values;
    /* Invalid type for unknown words: */
    if (!keys.contains(strDetailsElementOptionTypeSharedFolders, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::DetailsElementOptionTypeSharedFolders_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strDetailsElementOptionTypeSharedFolders, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface: */
template<> QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface &enmDetailsElementOptionTypeUserInterface)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeUserInterface)
    {
        case UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface_VisualState: strResult = QApplication::translate("UICommon", "Visual State"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface_MenuBar:     strResult = QApplication::translate("UICommon", "Menu Bar"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface_StatusBar:   strResult = QApplication::translate("UICommon", "Status Bar"); break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface_MiniToolbar: strResult = QApplication::translate("UICommon", "Mini Toolbar"); break;
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeUserInterface));
            break;
        }
    }
    return strResult;
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface: */
template<> QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface &enmDetailsElementOptionTypeUserInterface)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeUserInterface)
    {
        case UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface_VisualState: strResult = "VisualState"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface_MenuBar:     strResult = "MenuBar"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface_StatusBar:   strResult = "StatusBar"; break;
        case UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface_MiniToolbar: strResult = "MiniToolbar"; break;
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeUserInterface));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface <= QString: */
template<> UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface>(const QString &strDetailsElementOptionTypeUserInterface)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;      QList<UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface> values;
    keys << "VisualState"; values << UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface_VisualState;
    keys << "MenuBar";     values << UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface_MenuBar;
    keys << "StatusBar";   values << UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface_StatusBar;
    keys << "MiniToolbar"; values << UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface_MiniToolbar;
    /* Invalid type for unknown words: */
    if (!keys.contains(strDetailsElementOptionTypeUserInterface, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strDetailsElementOptionTypeUserInterface, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeDescription: */
template<> QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeDescription &enmDetailsElementOptionTypeDescription)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeDescription)
    {
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeDescription));
            break;
        }
    }
    return strResult;
}

/* QString <= UIExtraDataMetaDefs::DetailsElementOptionTypeDescription: */
template<> QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeDescription &enmDetailsElementOptionTypeDescription)
{
    QString strResult;
    switch (enmDetailsElementOptionTypeDescription)
    {
        default:
        {
            AssertMsgFailed(("No text for details element option type=%d", enmDetailsElementOptionTypeDescription));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::DetailsElementOptionTypeDescription <= QString: */
template<> UIExtraDataMetaDefs::DetailsElementOptionTypeDescription fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeDescription>(const QString &strDetailsElementOptionTypeDescription)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;  QList<UIExtraDataMetaDefs::DetailsElementOptionTypeDescription> values;
    /* Invalid type for unknown words: */
    if (!keys.contains(strDetailsElementOptionTypeDescription, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::DetailsElementOptionTypeDescription_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strDetailsElementOptionTypeDescription, Qt::CaseInsensitive)));
}

/* QString <= UIToolType: */
template<> QString toInternalString(const UIToolType &enmToolType)
{
    QString strResult;
    switch (enmToolType)
    {
        case UIToolType_Welcome:     strResult = "Welcome"; break;
        case UIToolType_Media:       strResult = "Media"; break;
        case UIToolType_Network:     strResult = "Network"; break;
        case UIToolType_Cloud:       strResult = "Cloud"; break;
        case UIToolType_Resources:   strResult = "Resources"; break;
        case UIToolType_Details:     strResult = "Details"; break;
        case UIToolType_Snapshots:   strResult = "Snapshots"; break;
        case UIToolType_Logs:        strResult = "Logs"; break;
        case UIToolType_Performance: strResult = "Performance"; break;
        default:
        {
            AssertMsgFailed(("No text for tool type=%d", enmToolType));
            break;
        }
    }
    return strResult;
}

/* UIToolType <= QString: */
template<> UIToolType fromInternalString<UIToolType>(const QString &strToolType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;      QList<UIToolType> values;
    keys << "Welcome";     values << UIToolType_Welcome;
    keys << "Media";       values << UIToolType_Media;
    keys << "Network";     values << UIToolType_Network;
    keys << "Cloud";       values << UIToolType_Cloud;
    keys << "Resources";   values << UIToolType_Resources;
    keys << "Details";     values << UIToolType_Details;
    keys << "Snapshots";   values << UIToolType_Snapshots;
    keys << "Logs";        values << UIToolType_Logs;
    keys << "Performance"; values << UIToolType_Performance;
    /* Invalid type for unknown words: */
    if (!keys.contains(strToolType, Qt::CaseInsensitive))
        return UIToolType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strToolType, Qt::CaseInsensitive)));
}

/* QString <= UIVisualStateType: */
template<> QString toString(const UIVisualStateType &visualStateType)
{
    QString strResult;
    switch (visualStateType)
    {
        case UIVisualStateType_Normal:     strResult = QApplication::translate("UICommon", "Normal (window)", "visual state"); break;
        case UIVisualStateType_Fullscreen: strResult = QApplication::translate("UICommon", "Full-screen", "visual state"); break;
        case UIVisualStateType_Seamless:   strResult = QApplication::translate("UICommon", "Seamless", "visual state"); break;
        case UIVisualStateType_Scale:      strResult = QApplication::translate("UICommon", "Scaled", "visual state"); break;
        default:
        {
            AssertMsgFailed(("No text for visual state type=%d", visualStateType));
            break;
        }
    }
    return strResult;
}

/* QString <= UIVisualStateType: */
template<> QString toInternalString(const UIVisualStateType &visualStateType)
{
    QString strResult;
    switch (visualStateType)
    {
        case UIVisualStateType_Normal:     strResult = "Normal"; break;
        case UIVisualStateType_Fullscreen: strResult = "Fullscreen"; break;
        case UIVisualStateType_Seamless:   strResult = "Seamless"; break;
        case UIVisualStateType_Scale:      strResult = "Scale"; break;
        case UIVisualStateType_All:        strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for visual state type=%d", visualStateType));
            break;
        }
    }
    return strResult;
}

/* UIVisualStateType <= QString: */
template<> UIVisualStateType fromInternalString<UIVisualStateType>(const QString &strVisualStateType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;     QList<UIVisualStateType> values;
    keys << "Normal";     values << UIVisualStateType_Normal;
    keys << "Fullscreen"; values << UIVisualStateType_Fullscreen;
    keys << "Seamless";   values << UIVisualStateType_Seamless;
    keys << "Scale";      values << UIVisualStateType_Scale;
    keys << "All";        values << UIVisualStateType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strVisualStateType, Qt::CaseInsensitive))
        return UIVisualStateType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strVisualStateType, Qt::CaseInsensitive)));
}

/* QString <= DetailsElementType: */
template<> QString toString(const DetailsElementType &detailsElementType)
{
    QString strResult;
    switch (detailsElementType)
    {
        case DetailsElementType_General:     strResult = QApplication::translate("UICommon", "General", "DetailsElementType"); break;
        case DetailsElementType_Preview:     strResult = QApplication::translate("UICommon", "Preview", "DetailsElementType"); break;
        case DetailsElementType_System:      strResult = QApplication::translate("UICommon", "System", "DetailsElementType"); break;
        case DetailsElementType_Display:     strResult = QApplication::translate("UICommon", "Display", "DetailsElementType"); break;
        case DetailsElementType_Storage:     strResult = QApplication::translate("UICommon", "Storage", "DetailsElementType"); break;
        case DetailsElementType_Audio:       strResult = QApplication::translate("UICommon", "Audio", "DetailsElementType"); break;
        case DetailsElementType_Network:     strResult = QApplication::translate("UICommon", "Network", "DetailsElementType"); break;
        case DetailsElementType_Serial:      strResult = QApplication::translate("UICommon", "Serial ports", "DetailsElementType"); break;
        case DetailsElementType_USB:         strResult = QApplication::translate("UICommon", "USB", "DetailsElementType"); break;
        case DetailsElementType_SF:          strResult = QApplication::translate("UICommon", "Shared folders", "DetailsElementType"); break;
        case DetailsElementType_UI:          strResult = QApplication::translate("UICommon", "User interface", "DetailsElementType"); break;
        case DetailsElementType_Description: strResult = QApplication::translate("UICommon", "Description", "DetailsElementType"); break;
        default:
        {
            AssertMsgFailed(("No text for details element type=%d", detailsElementType));
            break;
        }
    }
    return strResult;
}

/* DetailsElementType <= QString: */
template<> DetailsElementType fromString<DetailsElementType>(const QString &strDetailsElementType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;                                                                      QList<DetailsElementType> values;
    keys << QApplication::translate("UICommon", "General", "DetailsElementType");        values << DetailsElementType_General;
    keys << QApplication::translate("UICommon", "Preview", "DetailsElementType");        values << DetailsElementType_Preview;
    keys << QApplication::translate("UICommon", "System", "DetailsElementType");         values << DetailsElementType_System;
    keys << QApplication::translate("UICommon", "Display", "DetailsElementType");        values << DetailsElementType_Display;
    keys << QApplication::translate("UICommon", "Storage", "DetailsElementType");        values << DetailsElementType_Storage;
    keys << QApplication::translate("UICommon", "Audio", "DetailsElementType");          values << DetailsElementType_Audio;
    keys << QApplication::translate("UICommon", "Network", "DetailsElementType");        values << DetailsElementType_Network;
    keys << QApplication::translate("UICommon", "Serial ports", "DetailsElementType");   values << DetailsElementType_Serial;
    keys << QApplication::translate("UICommon", "USB", "DetailsElementType");            values << DetailsElementType_USB;
    keys << QApplication::translate("UICommon", "Shared folders", "DetailsElementType"); values << DetailsElementType_SF;
    keys << QApplication::translate("UICommon", "User interface", "DetailsElementType"); values << DetailsElementType_UI;
    keys << QApplication::translate("UICommon", "Description", "DetailsElementType");    values << DetailsElementType_Description;
    /* Invalid type for unknown words: */
    if (!keys.contains(strDetailsElementType, Qt::CaseInsensitive))
        return DetailsElementType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strDetailsElementType, Qt::CaseInsensitive)));
}

/* QString <= DetailsElementType: */
template<> QString toInternalString(const DetailsElementType &detailsElementType)
{
    QString strResult;
    switch (detailsElementType)
    {
        case DetailsElementType_General:     strResult = "general"; break;
        case DetailsElementType_Preview:     strResult = "preview"; break;
        case DetailsElementType_System:      strResult = "system"; break;
        case DetailsElementType_Display:     strResult = "display"; break;
        case DetailsElementType_Storage:     strResult = "storage"; break;
        case DetailsElementType_Audio:       strResult = "audio"; break;
        case DetailsElementType_Network:     strResult = "network"; break;
        case DetailsElementType_Serial:      strResult = "serialPorts"; break;
        case DetailsElementType_USB:         strResult = "usb"; break;
        case DetailsElementType_SF:          strResult = "sharedFolders"; break;
        case DetailsElementType_UI:          strResult = "userInterface"; break;
        case DetailsElementType_Description: strResult = "description"; break;
        default:
        {
            AssertMsgFailed(("No text for details element type=%d", detailsElementType));
            break;
        }
    }
    return strResult;
}

/* DetailsElementType <= QString: */
template<> DetailsElementType fromInternalString<DetailsElementType>(const QString &strDetailsElementType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;        QList<DetailsElementType> values;
    keys << "general";       values << DetailsElementType_General;
    keys << "preview";       values << DetailsElementType_Preview;
    keys << "system";        values << DetailsElementType_System;
    keys << "display";       values << DetailsElementType_Display;
    keys << "storage";       values << DetailsElementType_Storage;
    keys << "audio";         values << DetailsElementType_Audio;
    keys << "network";       values << DetailsElementType_Network;
    keys << "serialPorts";   values << DetailsElementType_Serial;
    keys << "usb";           values << DetailsElementType_USB;
    keys << "sharedFolders"; values << DetailsElementType_SF;
    keys << "userInterface"; values << DetailsElementType_UI;
    keys << "description";   values << DetailsElementType_Description;
    /* Invalid type for unknown words: */
    if (!keys.contains(strDetailsElementType, Qt::CaseInsensitive))
        return DetailsElementType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strDetailsElementType, Qt::CaseInsensitive)));
}

/* QIcon <= DetailsElementType: */
template<> QIcon toIcon(const DetailsElementType &detailsElementType)
{
    switch (detailsElementType)
    {
        case DetailsElementType_General:     return UIIconPool::iconSet(":/machine_16px.png");
        case DetailsElementType_Preview:     return UIIconPool::iconSet(":/machine_16px.png");
        case DetailsElementType_System:      return UIIconPool::iconSet(":/chipset_16px.png");
        case DetailsElementType_Display:     return UIIconPool::iconSet(":/vrdp_16px.png");
        case DetailsElementType_Storage:     return UIIconPool::iconSet(":/hd_16px.png");
        case DetailsElementType_Audio:       return UIIconPool::iconSet(":/sound_16px.png");
        case DetailsElementType_Network:     return UIIconPool::iconSet(":/nw_16px.png");
        case DetailsElementType_Serial:      return UIIconPool::iconSet(":/serial_port_16px.png");
        case DetailsElementType_USB:         return UIIconPool::iconSet(":/usb_16px.png");
        case DetailsElementType_SF:          return UIIconPool::iconSet(":/sf_16px.png");
        case DetailsElementType_UI:          return UIIconPool::iconSet(":/interface_16px.png");
        case DetailsElementType_Description: return UIIconPool::iconSet(":/description_16px.png");
        default:
        {
            AssertMsgFailed(("No icon for details element type=%d", detailsElementType));
            break;
        }
    }
    return QIcon();
}

/* QString <= PreviewUpdateIntervalType: */
template<> QString toInternalString(const PreviewUpdateIntervalType &previewUpdateIntervalType)
{
    /* Return corresponding QString representation for passed enum value: */
    switch (previewUpdateIntervalType)
    {
        case PreviewUpdateIntervalType_Disabled: return "disabled";
        case PreviewUpdateIntervalType_500ms:    return "500";
        case PreviewUpdateIntervalType_1000ms:   return "1000";
        case PreviewUpdateIntervalType_2000ms:   return "2000";
        case PreviewUpdateIntervalType_5000ms:   return "5000";
        case PreviewUpdateIntervalType_10000ms:  return "10000";
        default: AssertMsgFailed(("No text for '%d'", previewUpdateIntervalType)); break;
    }
    /* Return QString() by default: */
    return QString();
}

/* PreviewUpdateIntervalType <= QString: */
template<> PreviewUpdateIntervalType fromInternalString<PreviewUpdateIntervalType>(const QString &strPreviewUpdateIntervalType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;   QList<PreviewUpdateIntervalType> values;
    keys << "disabled"; values << PreviewUpdateIntervalType_Disabled;
    keys << "500";      values << PreviewUpdateIntervalType_500ms;
    keys << "1000";     values << PreviewUpdateIntervalType_1000ms;
    keys << "2000";     values << PreviewUpdateIntervalType_2000ms;
    keys << "5000";     values << PreviewUpdateIntervalType_5000ms;
    keys << "10000";    values << PreviewUpdateIntervalType_10000ms;
    /* 1000ms type for unknown words: */
    if (!keys.contains(strPreviewUpdateIntervalType, Qt::CaseInsensitive))
        return PreviewUpdateIntervalType_1000ms;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strPreviewUpdateIntervalType, Qt::CaseInsensitive)));
}

/* int <= PreviewUpdateIntervalType: */
template<> int toInternalInteger(const PreviewUpdateIntervalType &previewUpdateIntervalType)
{
    /* Return corresponding integer representation for passed enum value: */
    switch (previewUpdateIntervalType)
    {
        case PreviewUpdateIntervalType_Disabled: return 0;
        case PreviewUpdateIntervalType_500ms:    return 500;
        case PreviewUpdateIntervalType_1000ms:   return 1000;
        case PreviewUpdateIntervalType_2000ms:   return 2000;
        case PreviewUpdateIntervalType_5000ms:   return 5000;
        case PreviewUpdateIntervalType_10000ms:  return 10000;
        default: AssertMsgFailed(("No value for '%d'", previewUpdateIntervalType)); break;
    }
    /* Return 0 by default: */
    return 0;
}

/* PreviewUpdateIntervalType <= int: */
template<> PreviewUpdateIntervalType fromInternalInteger<PreviewUpdateIntervalType>(const int &iPreviewUpdateIntervalType)
{
    /* Add all the enum values into the hash: */
    QHash<int, PreviewUpdateIntervalType> hash;
    hash.insert(0,     PreviewUpdateIntervalType_Disabled);
    hash.insert(500,   PreviewUpdateIntervalType_500ms);
    hash.insert(1000,  PreviewUpdateIntervalType_1000ms);
    hash.insert(2000,  PreviewUpdateIntervalType_2000ms);
    hash.insert(5000,  PreviewUpdateIntervalType_5000ms);
    hash.insert(10000, PreviewUpdateIntervalType_10000ms);
    /* Make sure hash contains incoming integer representation: */
    if (!hash.contains(iPreviewUpdateIntervalType))
        AssertMsgFailed(("No value for '%d'", iPreviewUpdateIntervalType));
    /* Return corresponding enum value for passed integer representation: */
    return hash.value(iPreviewUpdateIntervalType);
}

/* QString <= GUIFeatureType: */
template<> QString toInternalString(const GUIFeatureType &guiFeatureType)
{
    QString strResult;
    switch (guiFeatureType)
    {
        case GUIFeatureType_NoSelector:     strResult = "noSelector"; break;
#ifdef VBOX_WS_MAC
        case GUIFeatureType_NoUserElements: strResult = "noUserElements"; break;
#else
        case GUIFeatureType_NoMenuBar:      strResult = "noMenuBar"; break;
#endif
        case GUIFeatureType_NoStatusBar:    strResult = "noStatusBar"; break;
        default:
        {
            AssertMsgFailed(("No text for GUI feature type=%d", guiFeatureType));
            break;
        }
    }
    return strResult;
}

/* GUIFeatureType <= QString: */
template<> GUIFeatureType fromInternalString<GUIFeatureType>(const QString &strGuiFeatureType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;         QList<GUIFeatureType> values;
    keys << "noSelector";     values << GUIFeatureType_NoSelector;
#ifdef VBOX_WS_MAC
    keys << "noUserElements"; values << GUIFeatureType_NoUserElements;
#else
    keys << "noMenuBar";      values << GUIFeatureType_NoMenuBar;
#endif
    keys << "noStatusBar";    values << GUIFeatureType_NoStatusBar;
    /* None type for unknown words: */
    if (!keys.contains(strGuiFeatureType, Qt::CaseInsensitive))
        return GUIFeatureType_None;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strGuiFeatureType, Qt::CaseInsensitive)));
}

/* QString <= GlobalSettingsPageType: */
template<> QString toInternalString(const GlobalSettingsPageType &globalSettingsPageType)
{
    QString strResult;
    switch (globalSettingsPageType)
    {
        case GlobalSettingsPageType_General:    strResult = "General"; break;
        case GlobalSettingsPageType_Input:      strResult = "Input"; break;
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
        case GlobalSettingsPageType_Update:     strResult = "Update"; break;
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
        case GlobalSettingsPageType_Language:   strResult = "Language"; break;
        case GlobalSettingsPageType_Display:    strResult = "Display"; break;
        case GlobalSettingsPageType_Network:    strResult = "Network"; break;
        case GlobalSettingsPageType_Extensions: strResult = "Extensions"; break;
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
        case GlobalSettingsPageType_Proxy:      strResult = "Proxy"; break;
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
        default:
        {
            AssertMsgFailed(("No text for settings page type=%d", globalSettingsPageType));
            break;
        }
    }
    return strResult;
}

/* GlobalSettingsPageType <= QString: */
template<> GlobalSettingsPageType fromInternalString<GlobalSettingsPageType>(const QString &strGlobalSettingsPageType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;     QList<GlobalSettingsPageType> values;
    keys << "General";    values << GlobalSettingsPageType_General;
    keys << "Input";      values << GlobalSettingsPageType_Input;
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    keys << "Update";     values << GlobalSettingsPageType_Update;
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
    keys << "Language";   values << GlobalSettingsPageType_Language;
    keys << "Display";    values << GlobalSettingsPageType_Display;
    keys << "Network";    values << GlobalSettingsPageType_Network;
    keys << "Extensions"; values << GlobalSettingsPageType_Extensions;
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    keys << "Proxy";      values << GlobalSettingsPageType_Proxy;
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
    /* Invalid type for unknown words: */
    if (!keys.contains(strGlobalSettingsPageType, Qt::CaseInsensitive))
        return GlobalSettingsPageType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strGlobalSettingsPageType, Qt::CaseInsensitive)));
}

/* QPixmap <= GlobalSettingsPageType: */
template<> QPixmap toWarningPixmap(const GlobalSettingsPageType &type)
{
    switch (type)
    {
        case GlobalSettingsPageType_General:    return UIIconPool::pixmap(":/machine_warning_16px.png");
        case GlobalSettingsPageType_Input:      return UIIconPool::pixmap(":/hostkey_warning_16px.png");
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
        case GlobalSettingsPageType_Update:     return UIIconPool::pixmap(":/refresh_warning_16px.png");
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
        case GlobalSettingsPageType_Language:   return UIIconPool::pixmap(":/site_warning_16px.png");
        case GlobalSettingsPageType_Display:    return UIIconPool::pixmap(":/vrdp_warning_16px.png");
        case GlobalSettingsPageType_Network:    return UIIconPool::pixmap(":/nw_warning_16px.png");
        case GlobalSettingsPageType_Extensions: return UIIconPool::pixmap(":/extension_pack_warning_16px.png");
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
        case GlobalSettingsPageType_Proxy:      return UIIconPool::pixmap(":/proxy_warning_16px.png");
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
        default: AssertMsgFailed(("No pixmap for %d", type)); break;
    }
    return QPixmap();
}

/* QString <= MachineSettingsPageType: */
template<> QString toInternalString(const MachineSettingsPageType &machineSettingsPageType)
{
    QString strResult;
    switch (machineSettingsPageType)
    {
        case MachineSettingsPageType_General:   strResult = "General"; break;
        case MachineSettingsPageType_System:    strResult = "System"; break;
        case MachineSettingsPageType_Display:   strResult = "Display"; break;
        case MachineSettingsPageType_Storage:   strResult = "Storage"; break;
        case MachineSettingsPageType_Audio:     strResult = "Audio"; break;
        case MachineSettingsPageType_Network:   strResult = "Network"; break;
        case MachineSettingsPageType_Ports:     strResult = "Ports"; break;
        case MachineSettingsPageType_Serial:    strResult = "Serial"; break;
        case MachineSettingsPageType_USB:       strResult = "USB"; break;
        case MachineSettingsPageType_SF:        strResult = "SharedFolders"; break;
        case MachineSettingsPageType_Interface: strResult = "Interface"; break;
        default:
        {
            AssertMsgFailed(("No text for settings page type=%d", machineSettingsPageType));
            break;
        }
    }
    return strResult;
}

/* MachineSettingsPageType <= QString: */
template<> MachineSettingsPageType fromInternalString<MachineSettingsPageType>(const QString &strMachineSettingsPageType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;        QList<MachineSettingsPageType> values;
    keys << "General";       values << MachineSettingsPageType_General;
    keys << "System";        values << MachineSettingsPageType_System;
    keys << "Display";       values << MachineSettingsPageType_Display;
    keys << "Storage";       values << MachineSettingsPageType_Storage;
    keys << "Audio";         values << MachineSettingsPageType_Audio;
    keys << "Network";       values << MachineSettingsPageType_Network;
    keys << "Ports";         values << MachineSettingsPageType_Ports;
    keys << "Serial";        values << MachineSettingsPageType_Serial;
    keys << "USB";           values << MachineSettingsPageType_USB;
    keys << "SharedFolders"; values << MachineSettingsPageType_SF;
    keys << "Interface";     values << MachineSettingsPageType_Interface;
    /* Invalid type for unknown words: */
    if (!keys.contains(strMachineSettingsPageType, Qt::CaseInsensitive))
        return MachineSettingsPageType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strMachineSettingsPageType, Qt::CaseInsensitive)));
}

/* QPixmap <= MachineSettingsPageType: */
template<> QPixmap toWarningPixmap(const MachineSettingsPageType &type)
{
    switch (type)
    {
        case MachineSettingsPageType_General:   return UIIconPool::pixmap(":/machine_warning_16px.png");
        case MachineSettingsPageType_System:    return UIIconPool::pixmap(":/chipset_warning_16px.png");
        case MachineSettingsPageType_Display:   return UIIconPool::pixmap(":/vrdp_warning_16px.png");
        case MachineSettingsPageType_Storage:   return UIIconPool::pixmap(":/hd_warning_16px.png");
        case MachineSettingsPageType_Audio:     return UIIconPool::pixmap(":/sound_warning_16px.png");
        case MachineSettingsPageType_Network:   return UIIconPool::pixmap(":/nw_warning_16px.png");
        case MachineSettingsPageType_Ports:     return UIIconPool::pixmap(":/serial_port_warning_16px.png");
        case MachineSettingsPageType_Serial:    return UIIconPool::pixmap(":/serial_port_warning_16px.png");
        case MachineSettingsPageType_USB:       return UIIconPool::pixmap(":/usb_warning_16px.png");
        case MachineSettingsPageType_SF:        return UIIconPool::pixmap(":/sf_warning_16px.png");
        case MachineSettingsPageType_Interface: return UIIconPool::pixmap(":/interface_warning_16px.png");
        default: AssertMsgFailed(("No pixmap for %d", type)); break;
    }
    return QPixmap();
}

/* QString <= WizardType: */
template<> QString toInternalString(const WizardType &wizardType)
{
    QString strResult;
    switch (wizardType)
    {
        case WizardType_NewVM:           strResult = "NewVM"; break;
        case WizardType_CloneVM:         strResult = "CloneVM"; break;
        case WizardType_ExportAppliance: strResult = "ExportAppliance"; break;
        case WizardType_ImportAppliance: strResult = "ImportAppliance"; break;
        case WizardType_NewCloudVM:      strResult = "NewCloudVM"; break;
        case WizardType_AddCloudVM:      strResult = "AddCloudVM"; break;
        case WizardType_FirstRun:        strResult = "FirstRun"; break;
        case WizardType_NewVD:           strResult = "NewVD"; break;
        case WizardType_CloneVD:         strResult = "CloneVD"; break;
        default:
        {
            AssertMsgFailed(("No text for wizard type=%d", wizardType));
            break;
        }
    }
    return strResult;
}

/* WizardType <= QString: */
template<> WizardType fromInternalString<WizardType>(const QString &strWizardType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;          QList<WizardType> values;
    keys << "NewVM";           values << WizardType_NewVM;
    keys << "CloneVM";         values << WizardType_CloneVM;
    keys << "ExportAppliance"; values << WizardType_ExportAppliance;
    keys << "ImportAppliance"; values << WizardType_ImportAppliance;
    keys << "NewCloudVM";      values << WizardType_NewCloudVM;
    keys << "AddCloudVM";      values << WizardType_AddCloudVM;
    keys << "FirstRun";        values << WizardType_FirstRun;
    keys << "NewVD";           values << WizardType_NewVD;
    keys << "CloneVD";         values << WizardType_CloneVD;
    /* Invalid type for unknown words: */
    if (!keys.contains(strWizardType, Qt::CaseInsensitive))
        return WizardType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strWizardType, Qt::CaseInsensitive)));
}

/* QString <= IndicatorType: */
template<> QString toInternalString(const IndicatorType &indicatorType)
{
    QString strResult;
    switch (indicatorType)
    {
        case IndicatorType_HardDisks:     strResult = "HardDisks"; break;
        case IndicatorType_OpticalDisks:  strResult = "OpticalDisks"; break;
        case IndicatorType_FloppyDisks:   strResult = "FloppyDisks"; break;
        case IndicatorType_Audio:         strResult = "Audio"; break;
        case IndicatorType_Network:       strResult = "Network"; break;
        case IndicatorType_USB:           strResult = "USB"; break;
        case IndicatorType_SharedFolders: strResult = "SharedFolders"; break;
        case IndicatorType_Display:       strResult = "Display"; break;
        case IndicatorType_Recording:     strResult = "Recording"; break;
        case IndicatorType_Features:      strResult = "Features"; break;
        case IndicatorType_Mouse:         strResult = "Mouse"; break;
        case IndicatorType_Keyboard:      strResult = "Keyboard"; break;
        default:
        {
            AssertMsgFailed(("No text for indicator type=%d", indicatorType));
            break;
        }
    }
    return strResult;
}

/* IndicatorType <= QString: */
template<> IndicatorType fromInternalString<IndicatorType>(const QString &strIndicatorType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;        QList<IndicatorType> values;
    keys << "HardDisks";     values << IndicatorType_HardDisks;
    keys << "OpticalDisks";  values << IndicatorType_OpticalDisks;
    keys << "FloppyDisks";   values << IndicatorType_FloppyDisks;
    keys << "Audio";         values << IndicatorType_Audio;
    keys << "Network";       values << IndicatorType_Network;
    keys << "USB";           values << IndicatorType_USB;
    keys << "SharedFolders"; values << IndicatorType_SharedFolders;
    keys << "Display";       values << IndicatorType_Display;
    keys << "Recording";     values << IndicatorType_Recording;
    keys << "Features";      values << IndicatorType_Features;
    keys << "Mouse";         values << IndicatorType_Mouse;
    keys << "Keyboard";      values << IndicatorType_Keyboard;
    /* Invalid type for unknown words: */
    if (!keys.contains(strIndicatorType, Qt::CaseInsensitive))
        return IndicatorType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strIndicatorType, Qt::CaseInsensitive)));
}

/* QString <= IndicatorType: */
template<> QString toString(const IndicatorType &indicatorType)
{
    QString strResult;
    switch (indicatorType)
    {
        case IndicatorType_HardDisks:     strResult = QApplication::translate("UICommon", "Hard Disks", "IndicatorType"); break;
        case IndicatorType_OpticalDisks:  strResult = QApplication::translate("UICommon", "Optical Disks", "IndicatorType"); break;
        case IndicatorType_FloppyDisks:   strResult = QApplication::translate("UICommon", "Floppy Disks", "IndicatorType"); break;
        case IndicatorType_Audio:         strResult = QApplication::translate("UICommon", "Audio", "IndicatorType"); break;
        case IndicatorType_Network:       strResult = QApplication::translate("UICommon", "Network", "IndicatorType"); break;
        case IndicatorType_USB:           strResult = QApplication::translate("UICommon", "USB", "IndicatorType"); break;
        case IndicatorType_SharedFolders: strResult = QApplication::translate("UICommon", "Shared Folders", "IndicatorType"); break;
        case IndicatorType_Display:       strResult = QApplication::translate("UICommon", "Display", "IndicatorType"); break;
        case IndicatorType_Recording:     strResult = QApplication::translate("UICommon", "Recording", "IndicatorType"); break;
        case IndicatorType_Features:      strResult = QApplication::translate("UICommon", "Features", "IndicatorType"); break;
        case IndicatorType_Mouse:         strResult = QApplication::translate("UICommon", "Mouse", "IndicatorType"); break;
        case IndicatorType_Keyboard:      strResult = QApplication::translate("UICommon", "Keyboard", "IndicatorType"); break;
        default:
        {
            AssertMsgFailed(("No text for indicator type=%d", indicatorType));
            break;
        }
    }
    return strResult;
}

/* QIcon <= IndicatorType: */
template<> QIcon toIcon(const IndicatorType &indicatorType)
{
    switch (indicatorType)
    {
        case IndicatorType_HardDisks:     return UIIconPool::iconSet(":/hd_16px.png");
        case IndicatorType_OpticalDisks:  return UIIconPool::iconSet(":/cd_16px.png");
        case IndicatorType_FloppyDisks:   return UIIconPool::iconSet(":/fd_16px.png");
        case IndicatorType_Audio:         return UIIconPool::iconSet(":/audio_16px.png");
        case IndicatorType_Network:       return UIIconPool::iconSet(":/nw_16px.png");
        case IndicatorType_USB:           return UIIconPool::iconSet(":/usb_16px.png");
        case IndicatorType_SharedFolders: return UIIconPool::iconSet(":/sf_16px.png");
        case IndicatorType_Display:       return UIIconPool::iconSet(":/display_software_16px.png");
        case IndicatorType_Recording:     return UIIconPool::iconSet(":/video_capture_16px.png");
        case IndicatorType_Features:      return UIIconPool::iconSet(":/vtx_amdv_16px.png");
        case IndicatorType_Mouse:         return UIIconPool::iconSet(":/mouse_16px.png");
        case IndicatorType_Keyboard:      return UIIconPool::iconSet(":/hostkey_16px.png");
        default:
        {
            AssertMsgFailed(("No icon for indicator type=%d", indicatorType));
            break;
        }
    }
    return QIcon();
}

/* QString <= MachineCloseAction: */
template<> QString toInternalString(const MachineCloseAction &machineCloseAction)
{
    QString strResult;
    switch (machineCloseAction)
    {
        case MachineCloseAction_Detach:                     strResult = "Detach"; break;
        case MachineCloseAction_SaveState:                  strResult = "SaveState"; break;
        case MachineCloseAction_Shutdown:                   strResult = "Shutdown"; break;
        case MachineCloseAction_PowerOff:                   strResult = "PowerOff"; break;
        case MachineCloseAction_PowerOff_RestoringSnapshot: strResult = "PowerOffRestoringSnapshot"; break;
        default:
        {
            AssertMsgFailed(("No text for indicator type=%d", machineCloseAction));
            break;
        }
    }
    return strResult;
}

/* MachineCloseAction <= QString: */
template<> MachineCloseAction fromInternalString<MachineCloseAction>(const QString &strMachineCloseAction)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;                    QList<MachineCloseAction> values;
    keys << "Detach";                    values << MachineCloseAction_Detach;
    keys << "SaveState";                 values << MachineCloseAction_SaveState;
    keys << "Shutdown";                  values << MachineCloseAction_Shutdown;
    keys << "PowerOff";                  values << MachineCloseAction_PowerOff;
    keys << "PowerOffRestoringSnapshot"; values << MachineCloseAction_PowerOff_RestoringSnapshot;
    /* Invalid type for unknown words: */
    if (!keys.contains(strMachineCloseAction, Qt::CaseInsensitive))
        return MachineCloseAction_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strMachineCloseAction, Qt::CaseInsensitive)));
}

/* QString <= MouseCapturePolicy: */
template<> QString toInternalString(const MouseCapturePolicy &mouseCapturePolicy)
{
    /* Return corresponding QString representation for passed enum value: */
    switch (mouseCapturePolicy)
    {
        case MouseCapturePolicy_Default:       return "Default";
        case MouseCapturePolicy_HostComboOnly: return "HostComboOnly";
        case MouseCapturePolicy_Disabled:      return "Disabled";
        default: AssertMsgFailed(("No text for '%d'", mouseCapturePolicy)); break;
    }
    /* Return QString() by default: */
    return QString();
}

/* MouseCapturePolicy <= QString: */
template<> MouseCapturePolicy fromInternalString<MouseCapturePolicy>(const QString &strMouseCapturePolicy)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;        QList<MouseCapturePolicy> values;
    keys << "Default";       values << MouseCapturePolicy_Default;
    keys << "HostComboOnly"; values << MouseCapturePolicy_HostComboOnly;
    keys << "Disabled";      values << MouseCapturePolicy_Disabled;
    /* Default type for unknown words: */
    if (!keys.contains(strMouseCapturePolicy, Qt::CaseInsensitive))
        return MouseCapturePolicy_Default;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strMouseCapturePolicy, Qt::CaseInsensitive)));
}

/* QString <= GuruMeditationHandlerType: */
template<> QString toInternalString(const GuruMeditationHandlerType &guruMeditationHandlerType)
{
    QString strResult;
    switch (guruMeditationHandlerType)
    {
        case GuruMeditationHandlerType_Default:  strResult = "Default"; break;
        case GuruMeditationHandlerType_PowerOff: strResult = "PowerOff"; break;
        case GuruMeditationHandlerType_Ignore:   strResult = "Ignore"; break;
        default:
        {
            AssertMsgFailed(("No text for indicator type=%d", guruMeditationHandlerType));
            break;
        }
    }
    return strResult;
}

/* GuruMeditationHandlerType <= QString: */
template<> GuruMeditationHandlerType fromInternalString<GuruMeditationHandlerType>(const QString &strGuruMeditationHandlerType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;   QList<GuruMeditationHandlerType> values;
    keys << "Default";  values << GuruMeditationHandlerType_Default;
    keys << "PowerOff"; values << GuruMeditationHandlerType_PowerOff;
    keys << "Ignore";   values << GuruMeditationHandlerType_Ignore;
    /* Default type for unknown words: */
    if (!keys.contains(strGuruMeditationHandlerType, Qt::CaseInsensitive))
        return GuruMeditationHandlerType_Default;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strGuruMeditationHandlerType, Qt::CaseInsensitive)));
}

/* QString <= ScalingOptimizationType: */
template<> QString toInternalString(const ScalingOptimizationType &optimizationType)
{
    QString strResult;
    switch (optimizationType)
    {
        case ScalingOptimizationType_None:        strResult = "None"; break;
        case ScalingOptimizationType_Performance: strResult = "Performance"; break;
        default:
        {
            AssertMsgFailed(("No text for type=%d", optimizationType));
            break;
        }
    }
    return strResult;
}

/* ScalingOptimizationType <= QString: */
template<> ScalingOptimizationType fromInternalString<ScalingOptimizationType>(const QString &strOptimizationType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;      QList<ScalingOptimizationType> values;
    keys << "None";        values << ScalingOptimizationType_None;
    keys << "Performance"; values << ScalingOptimizationType_Performance;
    /* 'None' type for empty/unknown words: */
    if (!keys.contains(strOptimizationType, Qt::CaseInsensitive))
        return ScalingOptimizationType_None;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strOptimizationType, Qt::CaseInsensitive)));
}

#ifndef VBOX_WS_MAC
/* QString <= MiniToolbarAlignment: */
template<> QString toInternalString(const MiniToolbarAlignment &miniToolbarAlignment)
{
    /* Return corresponding QString representation for passed enum value: */
    switch (miniToolbarAlignment)
    {
        case MiniToolbarAlignment_Bottom: return "Bottom";
        case MiniToolbarAlignment_Top:    return "Top";
        default: AssertMsgFailed(("No text for '%d'", miniToolbarAlignment)); break;
    }
    /* Return QString() by default: */
    return QString();
}

/* MiniToolbarAlignment <= QString: */
template<> MiniToolbarAlignment fromInternalString<MiniToolbarAlignment>(const QString &strMiniToolbarAlignment)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys; QList<MiniToolbarAlignment> values;
    keys << "Bottom"; values << MiniToolbarAlignment_Bottom;
    keys << "Top";    values << MiniToolbarAlignment_Top;
    /* Bottom type for unknown words: */
    if (!keys.contains(strMiniToolbarAlignment, Qt::CaseInsensitive))
        return MiniToolbarAlignment_Bottom;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strMiniToolbarAlignment, Qt::CaseInsensitive)));
}
#endif /* !VBOX_WS_MAC */

/* QString <= InformationElementType: */
template<> QString toString(const InformationElementType &informationElementType)
{
    QString strResult;
    switch (informationElementType)
    {
        case InformationElementType_General:           strResult = QApplication::translate("UICommon", "General", "InformationElementType"); break;
        case InformationElementType_Preview:           strResult = QApplication::translate("UICommon", "Preview", "InformationElementType"); break;
        case InformationElementType_System:            strResult = QApplication::translate("UICommon", "System", "InformationElementType"); break;
        case InformationElementType_Display:           strResult = QApplication::translate("UICommon", "Display", "InformationElementType"); break;
        case InformationElementType_Storage:           strResult = QApplication::translate("UICommon", "Storage", "InformationElementType"); break;
        case InformationElementType_Audio:             strResult = QApplication::translate("UICommon", "Audio", "InformationElementType"); break;
        case InformationElementType_Network:           strResult = QApplication::translate("UICommon", "Network", "InformationElementType"); break;
        case InformationElementType_Serial:            strResult = QApplication::translate("UICommon", "Serial ports", "InformationElementType"); break;
        case InformationElementType_USB:               strResult = QApplication::translate("UICommon", "USB", "InformationElementType"); break;
        case InformationElementType_SharedFolders:     strResult = QApplication::translate("UICommon", "Shared folders", "InformationElementType"); break;
        case InformationElementType_UI:                strResult = QApplication::translate("UICommon", "User interface", "InformationElementType"); break;
        case InformationElementType_Description:       strResult = QApplication::translate("UICommon", "Description", "InformationElementType"); break;
        case InformationElementType_RuntimeAttributes: strResult = QApplication::translate("UICommon", "Runtime attributes", "InformationElementType"); break;
        case InformationElementType_StorageStatistics: strResult = QApplication::translate("UICommon", "Storage statistics", "InformationElementType"); break;
        case InformationElementType_NetworkStatistics: strResult = QApplication::translate("UICommon", "Network statistics", "InformationElementType"); break;
        default:
        {
            AssertMsgFailed(("No text for information element type=%d", informationElementType));
            break;
        }
    }
    return strResult;
}

/* InformationElementType <= QString: */
template<> InformationElementType fromString<InformationElementType>(const QString &strInformationElementType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;                                                                              QList<InformationElementType> values;
    keys << QApplication::translate("UICommon", "General", "InformationElementType");            values << InformationElementType_General;
    keys << QApplication::translate("UICommon", "Preview", "InformationElementType");            values << InformationElementType_Preview;
    keys << QApplication::translate("UICommon", "System", "InformationElementType");             values << InformationElementType_System;
    keys << QApplication::translate("UICommon", "Display", "InformationElementType");            values << InformationElementType_Display;
    keys << QApplication::translate("UICommon", "Storage", "InformationElementType");            values << InformationElementType_Storage;
    keys << QApplication::translate("UICommon", "Audio", "InformationElementType");              values << InformationElementType_Audio;
    keys << QApplication::translate("UICommon", "Network", "InformationElementType");            values << InformationElementType_Network;
    keys << QApplication::translate("UICommon", "Serial ports", "InformationElementType");       values << InformationElementType_Serial;
    keys << QApplication::translate("UICommon", "USB", "InformationElementType");                values << InformationElementType_USB;
    keys << QApplication::translate("UICommon", "Shared folders", "InformationElementType");     values << InformationElementType_SharedFolders;
    keys << QApplication::translate("UICommon", "User interface", "InformationElementType");     values << InformationElementType_UI;
    keys << QApplication::translate("UICommon", "Description", "InformationElementType");        values << InformationElementType_Description;
    keys << QApplication::translate("UICommon", "Runtime attributes", "InformationElementType"); values << InformationElementType_RuntimeAttributes;
    keys << QApplication::translate("UICommon", "Storage statistics", "InformationElementType"); values << InformationElementType_StorageStatistics;
    keys << QApplication::translate("UICommon", "Network statistics", "InformationElementType"); values << InformationElementType_NetworkStatistics;
    /* Invalid type for unknown words: */
    if (!keys.contains(strInformationElementType, Qt::CaseInsensitive))
        return InformationElementType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strInformationElementType, Qt::CaseInsensitive)));
}

/* QString <= InformationElementType: */
template<> QString toInternalString(const InformationElementType &informationElementType)
{
    QString strResult;
    switch (informationElementType)
    {
        case InformationElementType_General:           strResult = "general"; break;
        case InformationElementType_Preview:           strResult = "preview"; break;
        case InformationElementType_System:            strResult = "system"; break;
        case InformationElementType_Display:           strResult = "display"; break;
        case InformationElementType_Storage:           strResult = "storage"; break;
        case InformationElementType_Audio:             strResult = "audio"; break;
        case InformationElementType_Network:           strResult = "network"; break;
        case InformationElementType_Serial:            strResult = "serialPorts"; break;
        case InformationElementType_USB:               strResult = "usb"; break;
        case InformationElementType_SharedFolders:     strResult = "sharedFolders"; break;
        case InformationElementType_UI:                strResult = "userInterface"; break;
        case InformationElementType_Description:       strResult = "description"; break;
        case InformationElementType_RuntimeAttributes: strResult = "runtime-attributes"; break;
        default:
        {
            AssertMsgFailed(("No text for information element type=%d", informationElementType));
            break;
        }
    }
    return strResult;
}

/* InformationElementType <= QString: */
template<> InformationElementType fromInternalString<InformationElementType>(const QString &strInformationElementType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;             QList<InformationElementType> values;
    keys << "general";            values << InformationElementType_General;
    keys << "preview";            values << InformationElementType_Preview;
    keys << "system";             values << InformationElementType_System;
    keys << "display";            values << InformationElementType_Display;
    keys << "storage";            values << InformationElementType_Storage;
    keys << "audio";              values << InformationElementType_Audio;
    keys << "network";            values << InformationElementType_Network;
    keys << "serialPorts";        values << InformationElementType_Serial;
    keys << "usb";                values << InformationElementType_USB;
    keys << "sharedFolders";      values << InformationElementType_SharedFolders;
    keys << "userInterface";      values << InformationElementType_UI;
    keys << "description";        values << InformationElementType_Description;
    keys << "runtime-attributes"; values << InformationElementType_RuntimeAttributes;
    /* Invalid type for unknown words: */
    if (!keys.contains(strInformationElementType, Qt::CaseInsensitive))
        return InformationElementType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strInformationElementType, Qt::CaseInsensitive)));
}

/* QIcon <= InformationElementType: */
template<> QIcon toIcon(const InformationElementType &informationElementType)
{
    switch (informationElementType)
    {
        case InformationElementType_General:           return UIIconPool::iconSet(":/machine_16px.png");
        case InformationElementType_Preview:           return UIIconPool::iconSet(":/machine_16px.png");
        case InformationElementType_System:            return UIIconPool::iconSet(":/chipset_16px.png");
        case InformationElementType_Display:           return UIIconPool::iconSet(":/vrdp_16px.png");
        case InformationElementType_Storage:           return UIIconPool::iconSet(":/hd_16px.png");
        case InformationElementType_Audio:             return UIIconPool::iconSet(":/sound_16px.png");
        case InformationElementType_Network:           return UIIconPool::iconSet(":/nw_16px.png");
        case InformationElementType_Serial:            return UIIconPool::iconSet(":/serial_port_16px.png");
        case InformationElementType_USB:               return UIIconPool::iconSet(":/usb_16px.png");
        case InformationElementType_SharedFolders:     return UIIconPool::iconSet(":/sf_16px.png");
        case InformationElementType_UI:                return UIIconPool::iconSet(":/interface_16px.png");
        case InformationElementType_Description:       return UIIconPool::iconSet(":/description_16px.png");
        case InformationElementType_RuntimeAttributes: return UIIconPool::iconSet(":/state_running_16px.png");
        case InformationElementType_StorageStatistics: return UIIconPool::iconSet(":/hd_16px.png");
        case InformationElementType_NetworkStatistics: return UIIconPool::iconSet(":/nw_16px.png");
        default:
        {
            AssertMsgFailed(("No icon for information element type=%d", informationElementType));
            break;
        }
    }
    return QIcon();
}

/* QString <= MaxGuestResolutionPolicy: */
template<> QString toInternalString(const MaxGuestResolutionPolicy &enmMaxGuestResolutionPolicy)
{
    QString strResult;
    switch (enmMaxGuestResolutionPolicy)
    {
        case MaxGuestResolutionPolicy_Automatic: strResult = ""; break;
        case MaxGuestResolutionPolicy_Any:       strResult = "any"; break;
        default:
        {
            AssertMsgFailed(("No text for max guest resolution policy=%d", enmMaxGuestResolutionPolicy));
            break;
        }
    }
    return strResult;
}

/* MaxGuestResolutionPolicy <= QString: */
template<> MaxGuestResolutionPolicy fromInternalString<MaxGuestResolutionPolicy>(const QString &strMaxGuestResolutionPolicy)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys; QList<MaxGuestResolutionPolicy> values;
    keys << "auto";   values << MaxGuestResolutionPolicy_Automatic;
    /* Auto type for empty value: */
    if (strMaxGuestResolutionPolicy.isEmpty())
        return MaxGuestResolutionPolicy_Automatic;
    /* Fixed type for value which can be parsed: */
    if (QRegularExpression("[1-9]\\d*,[1-9]\\d*").match(strMaxGuestResolutionPolicy).hasMatch())
        return MaxGuestResolutionPolicy_Fixed;
    /* Any type for unknown words: */
    if (!keys.contains(strMaxGuestResolutionPolicy, Qt::CaseInsensitive))
        return MaxGuestResolutionPolicy_Any;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strMaxGuestResolutionPolicy, Qt::CaseInsensitive)));
}

/* QString <= UIMediumFormat: */
template<> QString toString(const UIMediumFormat &enmUIMediumFormat)
{
    QString strResult;
    switch (enmUIMediumFormat)
    {
        case UIMediumFormat_VDI:       strResult = QApplication::translate("UICommon", "VDI (VirtualBox Disk Image)", "UIMediumFormat"); break;
        case UIMediumFormat_VMDK:      strResult = QApplication::translate("UICommon", "VMDK (Virtual Machine Disk)", "UIMediumFormat"); break;
        case UIMediumFormat_VHD:       strResult = QApplication::translate("UICommon", "VHD (Virtual Hard Disk)", "UIMediumFormat"); break;
        case UIMediumFormat_Parallels: strResult = QApplication::translate("UICommon", "HDD (Parallels Hard Disk)", "UIMediumFormat"); break;
        case UIMediumFormat_QED:       strResult = QApplication::translate("UICommon", "QED (QEMU enhanced disk)", "UIMediumFormat"); break;
        case UIMediumFormat_QCOW:      strResult = QApplication::translate("UICommon", "QCOW (QEMU Copy-On-Write)", "UIMediumFormat"); break;
        default:
        {
            AssertMsgFailed(("No text for medium format=%d", enmUIMediumFormat));
            break;
        }
    }
    return strResult;
}

/* QString <= UIMediumFormat: */
template<> QString toInternalString(const UIMediumFormat &enmUIMediumFormat)
{
    QString strResult;
    switch (enmUIMediumFormat)
    {
        case UIMediumFormat_VDI:       strResult = "VDI"; break;
        case UIMediumFormat_VMDK:      strResult = "VMDK"; break;
        case UIMediumFormat_VHD:       strResult = "VHD"; break;
        case UIMediumFormat_Parallels: strResult = "Parallels"; break;
        case UIMediumFormat_QED:       strResult = "QED"; break;
        case UIMediumFormat_QCOW:      strResult = "QCOW"; break;
        default:
        {
            AssertMsgFailed(("No text for medium format=%d", enmUIMediumFormat));
            break;
        }
    }
    return strResult;
}

/* UIMediumFormat <= QString: */
template<> UIMediumFormat fromInternalString<UIMediumFormat>(const QString &strUIMediumFormat)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;    QList<UIMediumFormat> values;
    keys << "VDI";       values << UIMediumFormat_VDI;
    keys << "VMDK";      values << UIMediumFormat_VMDK;
    keys << "VHD";       values << UIMediumFormat_VHD;
    keys << "Parallels"; values << UIMediumFormat_Parallels;
    keys << "QED";       values << UIMediumFormat_QED;
    keys << "QCOW";      values << UIMediumFormat_QCOW;
    /* VDI format for unknown words: */
    if (!keys.contains(strUIMediumFormat, Qt::CaseInsensitive))
        return UIMediumFormat_VDI;
    /* Corresponding format for known words: */
    return values.at(keys.indexOf(QRegExp(strUIMediumFormat, Qt::CaseInsensitive)));
}

/* QString <= UISettingsDefs::RecordingMode: */
template<> QString toString(const UISettingsDefs::RecordingMode &enmRecordingMode)
{
    QString strResult;
    switch (enmRecordingMode)
    {
        case UISettingsDefs::RecordingMode_VideoAudio: strResult = QApplication::translate("UICommon", "Video/Audio", "UISettingsDefs::RecordingMode"); break;
        case UISettingsDefs::RecordingMode_VideoOnly:  strResult = QApplication::translate("UICommon", "Video Only",  "UISettingsDefs::RecordingMode"); break;
        case UISettingsDefs::RecordingMode_AudioOnly:  strResult = QApplication::translate("UICommon", "Audio Only",  "UISettingsDefs::RecordingMode"); break;
        default:
        {
            AssertMsgFailed(("No text for recording mode format=%d", enmRecordingMode));
            break;
        }
    }
    return strResult;
}

/* UISettingsDefs::RecordingMode <= QString: */
template<> UISettingsDefs::RecordingMode fromString<UISettingsDefs::RecordingMode>(const QString &strRecordingMode)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;      QList<UISettingsDefs::RecordingMode> values;
    keys << "Video/Audio"; values << UISettingsDefs::RecordingMode_VideoAudio;
    keys << "Video Only";  values << UISettingsDefs::RecordingMode_VideoOnly;
    keys << "Audio Only";  values << UISettingsDefs::RecordingMode_AudioOnly;
    /* Video/Audio for unknown words: */
    if (!keys.contains(strRecordingMode, Qt::CaseInsensitive))
        return UISettingsDefs::RecordingMode_VideoAudio;
    /* Corresponding format for known words: */
    return values.at(keys.indexOf(QRegExp(strRecordingMode, Qt::CaseInsensitive)));
}

template<> QString toInternalString(const VMResourceMonitorColumn &enmVMResourceMonitorColumn)
{
    QString strResult;
    switch (enmVMResourceMonitorColumn)
    {
        case VMResourceMonitorColumn_Name:              strResult = "VMName"; break;
        case VMResourceMonitorColumn_CPUGuestLoad:      strResult = "CPUGuestLoad"; break;
        case VMResourceMonitorColumn_CPUVMMLoad:        strResult = "CPUVMMLoad"; break;
        case VMResourceMonitorColumn_RAMUsedAndTotal:   strResult = "RAMUsedAndTotal"; break;
        case VMResourceMonitorColumn_RAMUsedPercentage: strResult = "RAMUsedPercentage"; break;
        case VMResourceMonitorColumn_NetworkUpRate:     strResult = "NetworkUpRate"; break;
        case VMResourceMonitorColumn_NetworkDownRate:   strResult = "NetworkDownRate"; break;
        case VMResourceMonitorColumn_NetworkUpTotal:    strResult = "NetworkUpTotal"; break;
        case VMResourceMonitorColumn_NetworkDownTotal:  strResult = "NetworkDownTotal"; break;
        case VMResourceMonitorColumn_DiskIOReadRate:    strResult = "DiskIOReadRate"; break;
        case VMResourceMonitorColumn_DiskIOWriteRate:   strResult = "DiskIOWriteRate"; break;
        case VMResourceMonitorColumn_DiskIOReadTotal:   strResult = "DiskIOReadTotal"; break;
        case VMResourceMonitorColumn_DiskIOWriteTotal:  strResult = "DiskIOWriteTotal"; break;
        case VMResourceMonitorColumn_VMExits:           strResult = "VMExits"; break;
        default:
            {
                AssertMsgFailed(("No text for VM Resource Monitor Column=%d", enmVMResourceMonitorColumn));
                break;
            }
    }
    return strResult;
}

template<> VMResourceMonitorColumn fromInternalString<VMResourceMonitorColumn>(const QString &strVMResourceMonitorColumn)
{
    QStringList keys;    QList<VMResourceMonitorColumn> values;
    keys << "VMName";             values << VMResourceMonitorColumn_Name;
    keys << "CPUGuestLoad";       values << VMResourceMonitorColumn_CPUGuestLoad;
    keys << "CPUVMMLoad";         values << VMResourceMonitorColumn_CPUVMMLoad;
    keys << "RAMUsedAndTotal";    values << VMResourceMonitorColumn_RAMUsedAndTotal;
    keys << "RAMUsedPercentage";  values << VMResourceMonitorColumn_RAMUsedPercentage;
    keys << "NetworkUpRate";      values << VMResourceMonitorColumn_NetworkUpRate;
    keys << "NetworkDownRate";    values << VMResourceMonitorColumn_NetworkDownRate;
    keys << "NetworkUpTotal";     values << VMResourceMonitorColumn_NetworkUpTotal;
    keys << "NetworkDownTotal";   values << VMResourceMonitorColumn_NetworkDownTotal;
    keys << "DiskIOReadRate";     values << VMResourceMonitorColumn_DiskIOReadRate;
    keys << "DiskIOWriteRate";    values << VMResourceMonitorColumn_DiskIOWriteRate;
    keys << "DiskIOReadTotal";    values << VMResourceMonitorColumn_DiskIOReadTotal;
    keys << "DiskIOWriteTotal";   values << VMResourceMonitorColumn_DiskIOWriteTotal;
    keys << "VMExits";            values << VMResourceMonitorColumn_VMExits;
    if (!keys.contains(strVMResourceMonitorColumn, Qt::CaseInsensitive))
        return VMResourceMonitorColumn_Max;
    /* Corresponding format for known words: */
    return values.at(keys.indexOf(QRegExp(strVMResourceMonitorColumn, Qt::CaseInsensitive)));
}

/* $Id: Cpu.c 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * Cpu.c - VirtualBox CPU descriptors
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
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/HiiLib.h>
#include <Library/BaseLib.h>

#include <Protocol/Cpu.h>

#include "DataHub.h"

#define EFI_CPU_DATA_MAXIMUM_LENGTH 0x100

EFI_GUID gEfiAppleMagicHubGuid = {
    0x64517cc8, 0x6561, 0x4051, {0xb0, 0x3c, 0x59, 0x64, 0xb6, 0x0f, 0x4c, 0x7a }
};

EFI_GUID gEfiProcessorSubClassGuid = {
    0x26fdeb7e, 0xb8af, 0x4ccf, { 0xaa, 0x97, 0x02, 0x63, 0x3c, 0xe4, 0x8c, 0xa7 }
};

#pragma pack(1)
typedef struct {
    UINT8          Pad0[0x10];      /* 0x48 */
    UINT32         NameLen;         /* 0x58 , in bytes */
    UINT32         ValLen;          /* 0x5c */
    UINT8          Data[1];         /* 0x60 Name Value */
} MAGIC_HUB_DATA;
#pragma pack()

UINT32
CopyRecord(MAGIC_HUB_DATA* Rec, const CHAR16* Name, VOID* Val, UINT32 ValLen)
{
    Rec->NameLen = (UINT32)StrLen(Name) * sizeof(CHAR16);
    Rec->ValLen = ValLen;
    CopyMem(Rec->Data, Name, Rec->NameLen);
    CopyMem(Rec->Data + Rec->NameLen, Val, ValLen);

    return 0x10 + 4 + 4 + Rec->NameLen + Rec->ValLen;
}

EFI_STATUS EFIAPI
LogData(EFI_DATA_HUB_PROTOCOL       *DataHub,
        MAGIC_HUB_DATA              *MagicData,
        CHAR16                      *Name,
        VOID                        *Data,
        UINT32                       DataSize)
{
    UINT32                      RecordSize;
    EFI_STATUS                  Status;

    RecordSize = CopyRecord(MagicData, Name, Data, DataSize);
    Status = DataHub->LogData (
        DataHub,
        &gEfiProcessorSubClassGuid, /* DataRecordGuid */
        &gEfiAppleMagicHubGuid,     /* ProducerName */
        EFI_DATA_CLASS_DATA,
        MagicData,
        RecordSize
                               );
    ASSERT_EFI_ERROR (Status);

    return Status;
}

EFI_STATUS EFIAPI
CpuUpdateDataHub(EFI_BOOT_SERVICES * bs,
                 UINT64              FSBFrequency,
                 UINT64              TSCFrequency,
                 UINT64              CPUFrequency)
{
    EFI_STATUS                  Status;
    EFI_DATA_HUB_PROTOCOL       *DataHub;
    MAGIC_HUB_DATA              *MagicData;
    UINT32                      Supported = 1;
    //
    // Locate DataHub protocol.
    //
    Status = bs->LocateProtocol (&gEfiDataHubProtocolGuid, NULL, (VOID**)&DataHub);
    if (EFI_ERROR (Status)) {
        return Status;
    }

    MagicData = (MAGIC_HUB_DATA*)AllocatePool (0x200);
    if (MagicData == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }

    // Log data in format some OSes like
    LogData(DataHub, MagicData, L"FSBFrequency", &FSBFrequency, sizeof(FSBFrequency));
    // do that twice, as last variable read not really accounted for
    LogData(DataHub, MagicData, L"FSBFrequency", &FSBFrequency, sizeof(FSBFrequency));
    LogData(DataHub, MagicData, L"TSCFrequency", &TSCFrequency, sizeof(TSCFrequency));
    LogData(DataHub, MagicData, L"CPUFrequency", &CPUFrequency, sizeof(CPUFrequency));

    // The following is required for OS X to construct a SATA boot path. UEFI 2.0 (published
    // in Jan 2006, same time as the first Intel Macs) did not standardize SATA device paths;
    // if DevicePathsSupported is not set, OS X will create ATA boot paths which will fail
    // to boot
    LogData(DataHub, MagicData, L"DevicePathsSupported", &Supported, sizeof(Supported));

    FreePool (MagicData);

    return EFI_SUCCESS;
}

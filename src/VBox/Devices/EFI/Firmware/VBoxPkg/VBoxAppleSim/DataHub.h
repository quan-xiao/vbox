/* $Id: DataHub.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * DataHub.h
 */

/*
 * Copyright (C) 2019-2020 Oracle Corporation
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

#ifndef __DATA_HUB_H__
#define __DATA_HUB_H__

#define EFI_DATA_HUB_PROTOCOL_GUID \
  { 0xae80d021, 0x618e, 0x11d4, {0xbc, 0xd7, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81} }

typedef struct _EFI_DATA_HUB_PROTOCOL   EFI_DATA_HUB_PROTOCOL;


#define EFI_DATA_RECORD_HEADER_VERSION 0x0100

#define EFI_DATA_CLASS_DEBUG           0x1ULL
#define EFI_DATA_CLASS_ERROR           0x2ULL
#define EFI_DATA_CLASS_DATA            0x4ULL
#define EFI_DATA_CLASS_PROGRESS_CODE   0x8ULL

typedef struct {
    UINT16    Version;
    UINT16    HeaderSize;
    UINT32    RecordSize;
    EFI_GUID  DataRecordGuid;
    EFI_GUID  ProducerName;
    UINT64    DataRecordClass;
    EFI_TIME  LogTime;
    UINT64    LogMonotonicCount;
} EFI_DATA_RECORD_HEADER;

typedef
EFI_STATUS
(EFIAPI *EFI_DATA_HUB_LOG_DATA) (
  IN EFI_DATA_HUB_PROTOCOL             *This,
  IN EFI_GUID                          *DataRecordGuid,
  IN EFI_GUID                          *ProducerName,
  IN UINT64                            DataRecordClass,
  IN VOID                              *RawData,
  IN UINT32                            RawDataSize
  )
/*++

  Routine Description:
    Logs a data record to the system event log.

  Arguments:
    This         - Protocol instance pointer.


  Returns:
    EFI_SUCCESS     - Data was logged.

--*/
;


typedef
EFI_STATUS
(EFIAPI *EFI_DATA_HUB_GET_NEXT_DATA_RECORD) (
  IN EFI_DATA_HUB_PROTOCOL             *This,
  IN OUT UINT64                        *MonotonicCount,
  IN EFI_EVENT                         *FilterDriver OPTIONAL,
  OUT EFI_DATA_RECORD_HEADER           **Record
  )
/*++

  Routine Description:
    Allows the system data log to be searched.

  Arguments:
    This         - Protocol instance pointer.


  Returns:
    EFI_SUCCESS     - Data was logged.

--*/
;


typedef
EFI_STATUS
(EFIAPI *EFI_DATA_HUB_REGISTER_DATA_FILTER_DRIVER) (
  IN EFI_DATA_HUB_PROTOCOL             *This,
  IN EFI_EVENT                         FilterEvent,
  IN EFI_TPL                           FilterTpl,
  IN UINT64                            FilterClass,
  IN EFI_GUID                          *FilterDataRecordGui OPTIONAL
  )
/*++

  Routine Description:
    Registers an event to be signaled evey time a data record is logged in the system.

  Arguments:
    This         - Protocol instance pointer.


  Returns:
    EFI_SUCCESS     - The filter driver event was registered.

--*/
;


typedef
EFI_STATUS
(EFIAPI *EFI_DATA_HUB_UNREGISTER_DATA_FILTER_DRIVER) (
  IN EFI_DATA_HUB_PROTOCOL             *This,
  IN EFI_EVENT                         FilterEvent
  )
/*++

  Routine Description:
    Stops a filter driver from being notified when data records are logged.

  Arguments:
    This         - Protocol instance pointer.


  Returns:
    EFI_SUCCESS     - The filter driver represented by FilterEvent was shut off.

--*/
;


struct _EFI_DATA_HUB_PROTOCOL {
  EFI_DATA_HUB_LOG_DATA                        LogData;
  EFI_DATA_HUB_GET_NEXT_DATA_RECORD            GetNextDataRecord;
  EFI_DATA_HUB_REGISTER_DATA_FILTER_DRIVER     RegisterFilterDriver;
  EFI_DATA_HUB_UNREGISTER_DATA_FILTER_DRIVER   UnregisterFilterDriver;
};

extern EFI_GUID gEfiDataHubProtocolGuid;


EFI_STATUS
EFIAPI
InitializeDataHub (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );

#endif

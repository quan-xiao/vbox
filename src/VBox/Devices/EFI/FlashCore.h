/* $Id: FlashCore.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * A simple Flash device
 *
 * A simple non-volatile byte-wide (x8) memory device modeled after Intel 28F008
 * FlashFile. See 28F008SA datasheet, Intel order number 290429-007.
 *
 * Implemented as an MMIO device attached directly to the CPU, not behind any
 * bus. Typically mapped as part of the firmware image.
 */

/*
 * Copyright (C) 2018-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBOX_INCLUDED_SRC_EFI_FlashCore_h
#define VBOX_INCLUDED_SRC_EFI_FlashCore_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <VBox/vmm/pdmdev.h>
#include <VBox/log.h>
#include <VBox/err.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/file.h>

#include "VBoxDD.h"

RT_C_DECLS_BEGIN

/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** The current version of the saved state. */
#define FLASH_SAVED_STATE_VERSION           1

#if 0
/** Enables the ring-0/raw-mode read cache optimization, giving the size in
 *  uint64_t units. */
#define FLASH_WITH_RZ_READ_CACHE_SIZE       32
#endif


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * The flash device core structure.
 */
typedef struct FLASHCORE
{
    /** The current command. */
    uint8_t                 bCmd;
    /** The status register. */
    uint8_t                 bStatus;
    /** Current bus cycle. */
    uint8_t                 cBusCycle;

    /** @name The following state does not change at runtime
     * @{ */
    /** When set, indicates the state was saved. */
    bool                    fStateSaved;
    /** Manufacturer (high byte) and device (low byte) ID. */
    uint16_t                u16FlashId;
    /** The configured block size of the device. */
    uint16_t                cbBlockSize;
    /** The actual flash memory data.  */
    R3PTRTYPE(uint8_t *)    pbFlash;
    /** The flash memory region size.  */
    uint32_t                cbFlashSize;
    /** @} */

#ifdef FLASH_WITH_RZ_READ_CACHE_SIZE
    /** @name Read cache for non-ring-3 code.
     * @{ */
    /** The cache offset, UINT32_MAX if invalid. */
    uint32_t                offCache;
# if ARCH_BITS == 32
    uint32_t                uPadding;
# endif
    /** The cache data. */
    union
    {
        uint64_t            au64[FLASH_WITH_RZ_READ_CACHE_SIZE];
        uint8_t             ab[FLASH_WITH_RZ_READ_CACHE_SIZE * 8];
    } CacheData;
    /** @} */
#endif
} FLASHCORE;

/** Pointer to the Flash device state. */
typedef FLASHCORE *PFLASHCORE;

#ifndef VBOX_DEVICE_STRUCT_TESTCASE

DECLHIDDEN(VBOXSTRICTRC) flashWrite(PFLASHCORE pThis, uint32_t off, const void *pv, size_t cb);
DECLHIDDEN(VBOXSTRICTRC) flashRead(PFLASHCORE pThis, uint32_t off, void *pv, size_t cb);

# ifdef IN_RING3
DECLHIDDEN(int) flashR3Init(PFLASHCORE pThis, PPDMDEVINS pDevIns, uint16_t idFlashDev, uint32_t cbFlash, uint16_t cbBlock);
DECLHIDDEN(void) flashR3Destruct(PFLASHCORE pThis, PPDMDEVINS pDevIns);
DECLHIDDEN(int) flashR3LoadFromFile(PFLASHCORE pThis, PPDMDEVINS pDevIns, const char *pszFilename);
DECLHIDDEN(int) flashR3LoadFromBuf(PFLASHCORE pThis, void const *pvBuf, size_t cbBuf);
DECLHIDDEN(int) flashR3SaveToFile(PFLASHCORE pThis, PPDMDEVINS pDevIns, const char *pszFilename);
DECLHIDDEN(int) flashR3SaveToBuf(PFLASHCORE pThis, void *pvBuf, size_t cbBuf);
DECLHIDDEN(void) flashR3Reset(PFLASHCORE pThis);
DECLHIDDEN(int) flashR3SaveExec(PFLASHCORE pThis, PPDMDEVINS pDevIns, PSSMHANDLE pSSM);
DECLHIDDEN(int) flashR3LoadExec(PFLASHCORE pThis, PPDMDEVINS pDevIns, PSSMHANDLE pSSM);
# endif /* IN_RING3 */

#endif /* VBOX_DEVICE_STRUCT_TESTCASE */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_SRC_EFI_FlashCore_h */


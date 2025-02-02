/* $Id: DBGCIoProvInternal.h 86327 2020-09-28 16:20:50Z vboxsync $ */
/** @file
 * DBGC - Debugger Console, Internal I/O provider header file.
 */

/*
 * Copyright (C) 2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef DEBUGGER_INCLUDED_SRC_DBGCIoProvInternal_h
#define DEBUGGER_INCLUDED_SRC_DBGCIoProvInternal_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <VBox/dbg.h>
#include <VBox/err.h>
#include <VBox/vmm/cfgm.h>


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/

/** An Opaque I/O provider handle. */
typedef struct DBGCIOPROVINT *DBGCIOPROV;
/** Pointer to an opaque I/O provider handle. */
typedef DBGCIOPROV *PDBGCIOPROV;


/**
 * I/O provider registration record.
 */
typedef struct DBGCIOPROVREG
{
    /** Unique name for the I/O provider. */
    const char                  *pszName;
    /** I/O provider description. */
    const char                  *pszDesc;

    /**
     * Creates an I/O provider instance from the given config.
     *
     * @returns VBox status code.
     * @param   phDbgcIoProv    Where to store the handle to the I/O provider instance on success.
     * @param   pCfg            The config to use.
     */
    DECLCALLBACKMEMBER(int, pfnCreate, (PDBGCIOPROV phDbgcIoProv, PCFGMNODE pCfg));

    /**
     * Destroys the given I/O provider instance.
     *
     * @returns nothing.
     * @param   hDbgcIoProv     The I/O provider instance handle to destroy.
     */
    DECLCALLBACKMEMBER(void, pfnDestroy, (DBGCIOPROV hDbgcIoProv));

    /**
     * Waits for someone to connect to the provider instance.
     *
     * @returns VBox status code.
     * @retval  VERR_TIMEOUT if the waiting time was exceeded without anyone connecting.
     * @retval  VERR_INTERRUPTED if the waiting was interrupted by DBGCIOPROVREG::pfnWaitInterrupt.
     * @param   hDbgcIoProv     The I/O provider instance handle.
     * @param   cMsTimeout      Number of milliseconds to wait, use RT_INDEFINITE_WAIT to wait indefinitely.
     * @param   ppDbgcIo        Where to return the I/O connection callback table upon a succesful return.
     */
    DECLCALLBACKMEMBER(int, pfnWaitForConnect, (DBGCIOPROV hDbgcIoProv, RTMSINTERVAL cMsTimeout, PCDBGCIO *ppDbgcIo));

    /**
     * Interrupts the thread waiting in DBGCIOPROVREG::pfnWaitForConnect.
     *
     * @returns VBox status code.
     * @param   hDbgcIoProv     The I/O provider instance handle.
     */
    DECLCALLBACKMEMBER(int, pfnWaitInterrupt, (DBGCIOPROV hDbgcIoProv));

} DBGCIOPROVREG;
/** Pointer to an I/O provider registration record. */
typedef DBGCIOPROVREG *PDBGCIOPROVREG;
/** Pointer toa const I/O provider registration record. */
typedef const DBGCIOPROVREG *PCDBGCIOPROVREG;


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
extern const DBGCIOPROVREG    g_DbgcIoProvTcp;
extern const DBGCIOPROVREG    g_DbgcIoProvIpc;


#endif /* !DEBUGGER_INCLUDED_SRC_DBGCIoProvInternal_h */


/* $Id: PCIRawDevImpl.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VirtualBox Driver interface to raw PCI device
 */

/*
 * Copyright (C) 2010-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef MAIN_INCLUDED_PCIRawDevImpl_h
#define MAIN_INCLUDED_PCIRawDevImpl_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/vmm/pdmdrv.h>

class Console;
struct DRVMAINPCIRAWDEV;

class PCIRawDev
{
  public:
    PCIRawDev(Console *console);
    virtual ~PCIRawDev();

    static const PDMDRVREG DrvReg;

    Console *getParent() const
    {
        return mParent;
    }

  private:
    static DECLCALLBACK(void *) drvQueryInterface(PPDMIBASE pInterface, const char *pszIID);
    static DECLCALLBACK(int)    drvConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags);
    static DECLCALLBACK(void)   drvDestruct(PPDMDRVINS pDrvIns);
    static DECLCALLBACK(int)    drvDeviceConstructComplete(PPDMIPCIRAWCONNECTOR pInterface, const char *pcszName,
                                                           uint32_t uHostPCIAddress, uint32_t uGuestPCIAddress,
                                                           int rc);

    Console * const mParent;
    struct DRVMAINPCIRAWDEV *mpDrv;
};

#endif /* !MAIN_INCLUDED_PCIRawDevImpl_h */

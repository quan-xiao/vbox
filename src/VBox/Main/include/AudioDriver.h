/* $Id: AudioDriver.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VirtualBox audio base class for Main audio drivers.
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

#ifndef MAIN_INCLUDED_AudioDriver_h
#define MAIN_INCLUDED_AudioDriver_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/com/ptr.h>
#include <VBox/com/string.h>
#include <VBox/com/AutoLock.h>

using namespace com;

/**
 * Audio driver configuration for audio drivers implemented
 * in Main.
 */
struct AudioDriverCfg
{
    AudioDriverCfg(Utf8Str a_strDev = "", unsigned a_uInst = 0, unsigned a_uLUN = 0, Utf8Str a_strName = "")
        : strDev(a_strDev)
        , uInst(a_uInst)
        , uLUN(a_uLUN)
        , strName(a_strName) { }

    /** Copy assignment operator. */
    AudioDriverCfg& operator=(AudioDriverCfg const &a_rThat) RT_NOEXCEPT
    {
        this->strDev  = a_rThat.strDev;
        this->uInst   = a_rThat.uInst;
        this->uLUN    = a_rThat.uLUN;
        this->strName = a_rThat.strName;

        return *this;
    }

    /** The device name. */
    Utf8Str              strDev;
    /** The device instance. */
    unsigned             uInst;
    /** The LUN the driver is attached to.
     *  Set the UINT8_MAX if not attached. */
    unsigned             uLUN;
    /** The driver name. */
    Utf8Str              strName;
};

class Console;

/**
 * Base class for all audio drivers implemented in Main.
 */
class AudioDriver
{

public:
    AudioDriver(Console *pConsole);
    virtual ~AudioDriver();

    /** Copy assignment operator. */
    AudioDriver &operator=(AudioDriver const &a_rThat) RT_NOEXCEPT;

    Console *GetParent(void) { return mpConsole; }

    AudioDriverCfg *GetConfig(void) { return &mCfg; }
    int InitializeConfig(AudioDriverCfg *pCfg);

    /** Checks if audio is configured or not. */
    bool isConfigured() const { return mCfg.strName.isNotEmpty(); }

    bool IsAttached(void) { return mfAttached; }

    int doAttachDriverViaEmt(PUVM pUVM, util::AutoWriteLock *pAutoLock);
    int doDetachDriverViaEmt(PUVM pUVM, util::AutoWriteLock *pAutoLock);

protected:
    static DECLCALLBACK(int) attachDriverOnEmt(AudioDriver *pThis);
    static DECLCALLBACK(int) detachDriverOnEmt(AudioDriver *pThis);

    int configure(unsigned uLUN, bool fAttach);

    /**
     * Optional (virtual) function to give the derived audio driver
     * class the ability to add (or change) the driver configuration
     * entries when setting up.
     *
     * @return VBox status code.
     * @param  pLunCfg          CFGM configuration node of the driver.
     */
    virtual int configureDriver(PCFGMNODE pLunCfg) { RT_NOREF(pLunCfg); return VINF_SUCCESS; }

protected:

    /** Pointer to parent. */
    Console             *mpConsole;
    /** The driver's configuration. */
    AudioDriverCfg       mCfg;
    /** Whether the driver is attached or not. */
    bool                 mfAttached;
};

#endif /* !MAIN_INCLUDED_AudioDriver_h */


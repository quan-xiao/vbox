/* $Id: BIOSSettingsImpl.h 82968 2020-02-04 10:35:17Z vboxsync $ */

/** @file
 *
 * VirtualBox COM class implementation - Machine BIOS settings.
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

#ifndef MAIN_INCLUDED_BIOSSettingsImpl_h
#define MAIN_INCLUDED_BIOSSettingsImpl_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "BIOSSettingsWrap.h"

class GuestOSType;

namespace settings
{
    struct BIOSSettings;
}

class ATL_NO_VTABLE BIOSSettings :
    public BIOSSettingsWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(BIOSSettings)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(Machine *parent);
    HRESULT init(Machine *parent, BIOSSettings *that);
    HRESULT initCopy(Machine *parent, BIOSSettings *that);
    void uninit();

    // public methods for internal purposes only
    HRESULT i_loadSettings(const settings::BIOSSettings &data);
    HRESULT i_saveSettings(settings::BIOSSettings &data);

    void i_rollback();
    void i_commit();
    void i_copyFrom(BIOSSettings *aThat);
    void i_applyDefaults(GuestOSType *aOsType);

    com::Utf8Str i_getNonVolatileStorageFile();
    void i_updateNonVolatileStorageFile(const com::Utf8Str &aNonVolatileStorageFile);

private:

    // wrapped IBIOSettings properties
    HRESULT getLogoFadeIn(BOOL *enabled);
    HRESULT setLogoFadeIn(BOOL enable);
    HRESULT getLogoFadeOut(BOOL *enabled);
    HRESULT setLogoFadeOut(BOOL enable);
    HRESULT getLogoDisplayTime(ULONG *displayTime);
    HRESULT setLogoDisplayTime(ULONG displayTime);
    HRESULT getLogoImagePath(com::Utf8Str &imagePath);
    HRESULT setLogoImagePath(const com::Utf8Str &imagePath);
    HRESULT getBootMenuMode(BIOSBootMenuMode_T *bootMenuMode);
    HRESULT setBootMenuMode(BIOSBootMenuMode_T bootMenuMode);
    HRESULT getACPIEnabled(BOOL *enabled);
    HRESULT setACPIEnabled(BOOL enable);
    HRESULT getIOAPICEnabled(BOOL *aIOAPICEnabled);
    HRESULT setIOAPICEnabled(BOOL aIOAPICEnabled);
    HRESULT getAPICMode(APICMode_T *aAPICMode);
    HRESULT setAPICMode(APICMode_T aAPICMode);
    HRESULT getTimeOffset(LONG64 *offset);
    HRESULT setTimeOffset(LONG64 offset);
    HRESULT getPXEDebugEnabled(BOOL *enabled);
    HRESULT setPXEDebugEnabled(BOOL enable);
    HRESULT getNonVolatileStorageFile(com::Utf8Str &aNonVolatileStorageFile);
    HRESULT getSMBIOSUuidLittleEndian(BOOL *enabled);
    HRESULT setSMBIOSUuidLittleEndian(BOOL enable);

    struct Data;
    Data *m;
};

#endif /* !MAIN_INCLUDED_BIOSSettingsImpl_h */

/* vi: set tabstop=4 shiftwidth=4 expandtab: */

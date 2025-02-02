/* $Id: CloudNetworkImpl.cpp 85360 2020-07-16 09:18:27Z vboxsync $ */
/** @file
 * ICloudNetwork  COM class implementations.
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
 */


#define LOG_GROUP LOG_GROUP_MAIN_CLOUDNETWORK
#include <VBox/settings.h>
#include <iprt/cpp/utils.h>

#include "VirtualBoxImpl.h"
#include "CloudNetworkImpl.h"
#include "AutoCaller.h"
#include "LoggingNew.h"


struct CloudNetwork::Data
{
    Data() : pVirtualBox(NULL) {}
    virtual ~Data() {}

    /** weak VirtualBox parent */
    VirtualBox * const pVirtualBox;

    /** CloudNetwork settings */
    settings::CloudNetwork s;
};

////////////////////////////////////////////////////////////////////////////////
//
// CloudNetwork constructor / destructor
//
// ////////////////////////////////////////////////////////////////////////////////
CloudNetwork::CloudNetwork() : m(NULL)
{
}

CloudNetwork::~CloudNetwork()
{
}


HRESULT CloudNetwork::FinalConstruct()
{
    return BaseFinalConstruct();
}

void CloudNetwork::FinalRelease()
{
    uninit();

    BaseFinalRelease();
}

HRESULT CloudNetwork::init(VirtualBox *aVirtualBox, Utf8Str aName)
{
    // Enclose the state transition NotReady->InInit->Ready.
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    m = new Data();
    /* share VirtualBox weakly */
    unconst(m->pVirtualBox) = aVirtualBox;

    m->s.strNetworkName = aName;
    m->s.fEnabled = true;
    m->s.strProviderShortName = "OCI";
    m->s.strProfileName = "Default";

    autoInitSpan.setSucceeded();
    return S_OK;
}

void CloudNetwork::uninit()
{
    // Enclose the state transition Ready->InUninit->NotReady.
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;
}

HRESULT CloudNetwork::i_loadSettings(const settings::CloudNetwork &data)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    m->s = data;

    return S_OK;
}

HRESULT CloudNetwork::i_saveSettings(settings::CloudNetwork &data)
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    AssertReturn(!m->s.strNetworkName.isEmpty(), E_FAIL);
    data = m->s;

    return S_OK;
}

Utf8Str CloudNetwork::i_getProvider()
{
    return m->s.strProviderShortName;
}

Utf8Str CloudNetwork::i_getProfile()
{
    return m->s.strProfileName;
}

Utf8Str CloudNetwork::i_getNetworkId()
{
    return m->s.strNetworkId;
}

Utf8Str CloudNetwork::i_getNetworkName()
{
    return m->s.strNetworkName;
}


HRESULT CloudNetwork::getNetworkName(com::Utf8Str &aNetworkName)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    AssertReturn(!m->s.strNetworkName.isEmpty(), E_FAIL);
    aNetworkName = m->s.strNetworkName;
    return S_OK;
}

HRESULT CloudNetwork::setNetworkName(const com::Utf8Str &aNetworkName)
{
    if (aNetworkName.isEmpty())
        return setError(E_INVALIDARG,
                        tr("Network name cannot be empty"));
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (aNetworkName == m->s.strNetworkName)
            return S_OK;

        m->s.strNetworkName = aNetworkName;
    }

    AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = m->pVirtualBox->i_saveSettings();
    ComAssertComRCRetRC(rc);
    return S_OK;
}

HRESULT CloudNetwork::getEnabled(BOOL *aEnabled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    *aEnabled = m->s.fEnabled;
    return S_OK;
}

HRESULT CloudNetwork::setEnabled(BOOL aEnabled)
{
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (RT_BOOL(aEnabled) == m->s.fEnabled)
            return S_OK;
        m->s.fEnabled = RT_BOOL(aEnabled);
    }

    AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = m->pVirtualBox->i_saveSettings();
    ComAssertComRCRetRC(rc);
    return S_OK;
}

HRESULT CloudNetwork::getProvider(com::Utf8Str &aProvider)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    aProvider = m->s.strProviderShortName;
    return S_OK;
}

HRESULT CloudNetwork::setProvider(const com::Utf8Str &aProvider)
{
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (aProvider == m->s.strProviderShortName)
            return S_OK;
        m->s.strProviderShortName = aProvider;
    }

    AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = m->pVirtualBox->i_saveSettings();
    ComAssertComRCRetRC(rc);
    return S_OK;
}

HRESULT CloudNetwork::getProfile(com::Utf8Str &aProfile)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    aProfile = m->s.strProfileName;
    return S_OK;
}

HRESULT CloudNetwork::setProfile(const com::Utf8Str &aProfile)
{
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (aProfile == m->s.strProfileName)
            return S_OK;
        m->s.strProfileName = aProfile;
    }

    AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = m->pVirtualBox->i_saveSettings();
    ComAssertComRCRetRC(rc);
    return S_OK;
}

HRESULT CloudNetwork::getNetworkId(com::Utf8Str &aNetworkId)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    aNetworkId = m->s.strNetworkId;
    return S_OK;
}

HRESULT CloudNetwork::setNetworkId(const com::Utf8Str &aNetworkId)
{
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (aNetworkId == m->s.strNetworkId)
            return S_OK;
        m->s.strNetworkId = aNetworkId;
    }

    AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = m->pVirtualBox->i_saveSettings();
    ComAssertComRCRetRC(rc);
    return S_OK;
}


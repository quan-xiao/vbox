/* $Id: UIDnDEnumFormat_win.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIDnDEnumFormat class declaration.
 */

/*
 * Copyright (C) 2014-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_runtime_UIDnDEnumFormat_win_h
#define FEQT_INCLUDED_SRC_runtime_UIDnDEnumFormat_win_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


class UIDnDEnumFormatEtc : public IEnumFORMATETC
{
public:

    UIDnDEnumFormatEtc(FORMATETC *pFormatEtc, ULONG cFormats);
    virtual ~UIDnDEnumFormatEtc(void);

public:

    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    STDMETHOD(Next)(ULONG cFormats, FORMATETC *pFormatEtc, ULONG *pcFetched);
    STDMETHOD(Skip)(ULONG cFormats);
    STDMETHOD(Reset)(void);
    STDMETHOD(Clone)(IEnumFORMATETC ** ppEnumFormatEtc);

public:

    static void CopyFormat(FORMATETC *pFormatDest, FORMATETC *pFormatSource);
    static HRESULT CreateEnumFormatEtc(UINT cFormats, FORMATETC *pFormatEtc, IEnumFORMATETC **ppEnumFormatEtc);

private:

    LONG        m_lRefCount;
    ULONG       m_nIndex;
    ULONG       m_nNumFormats;
    FORMATETC * m_pFormatEtc;
};

#endif /* !FEQT_INCLUDED_SRC_runtime_UIDnDEnumFormat_win_h */


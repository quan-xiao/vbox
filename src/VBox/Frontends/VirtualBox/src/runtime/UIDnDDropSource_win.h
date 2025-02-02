/* $Id: UIDnDDropSource_win.h 85681 2020-08-11 09:36:37Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIDnDDropSource class declaration.
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

#ifndef FEQT_INCLUDED_SRC_runtime_UIDnDDropSource_win_h
#define FEQT_INCLUDED_SRC_runtime_UIDnDDropSource_win_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* COM includes: */
#include "COMEnums.h"

class UIDnDDataObject;

/**
 * Implementation of IDropSource for drag and drop on the host.
 */
class UIDnDDropSource : public IDropSource
{
public:

    UIDnDDropSource(QWidget *pParent, UIDnDDataObject *pDataObject);
    virtual ~UIDnDDropSource(void);

public:

    uint32_t GetCurrentAction(void) const { return m_uCurAction; }

public: /* IUnknown methods. */

    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

public: /* IDropSource methods. */

    STDMETHOD(QueryContinueDrag)(BOOL fEscapePressed, DWORD dwKeyState);
    STDMETHOD(GiveFeedback)(DWORD dwEffect);

protected:

    /** Pointer to parent widget. */
    QWidget         *m_pParent;
    /** Pointer to current data object. */
    UIDnDDataObject *m_pDataObject;
    /** The current reference count. */
    LONG             m_cRefCount;
    /** Current (last) drop effect issued. */
    DWORD            m_dwCurEffect;
    /** Current drop action to perform in case of a successful drop. */
    Qt::DropActions  m_uCurAction;
};

#endif /* !FEQT_INCLUDED_SRC_runtime_UIDnDDropSource_win_h */


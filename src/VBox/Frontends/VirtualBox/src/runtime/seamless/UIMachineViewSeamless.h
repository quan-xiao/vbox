/* $Id: UIMachineViewSeamless.h 84790 2020-06-11 10:30:36Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIMachineViewSeamless class declaration.
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

#ifndef FEQT_INCLUDED_SRC_runtime_seamless_UIMachineViewSeamless_h
#define FEQT_INCLUDED_SRC_runtime_seamless_UIMachineViewSeamless_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Local includes */
#include "UIMachineView.h"

class UIMachineViewSeamless : public UIMachineView
{
    Q_OBJECT;

protected:

    /* Seamless machine-view constructor: */
    UIMachineViewSeamless(UIMachineWindow *pMachineWindow, ulong uScreenId);
    /* Seamless machine-view destructor: */
    virtual ~UIMachineViewSeamless() { cleanupSeamless(); }

private slots:

    /* Handler: Console callback stuff: */
    void sltAdditionsStateChanged();

    /* Handler: Frame-buffer SetVisibleRegion stuff: */
    virtual void sltHandleSetVisibleRegion(QRegion region);

private:

    /* Event handlers: */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);

    /* Prepare helpers: */
    void prepareCommon();
    void prepareFilters();
    void prepareConsoleConnections();
    void prepareSeamless();

    /* Cleanup helpers: */
    void cleanupSeamless();
    //void cleanupConsoleConnections() {}
    //void cleanupFilters() {}
    //void cleanupCommon() {}

    /** Adjusts guest-screen size to correspond current <i>working area</i> size. */
    void adjustGuestScreenSize();

    /* Helpers: Geometry stuff: */
    QRect workingArea() const;
    QSize calculateMaxGuestSize() const;

    /* Friend classes: */
    friend class UIMachineView;
};

#endif /* !FEQT_INCLUDED_SRC_runtime_seamless_UIMachineViewSeamless_h */


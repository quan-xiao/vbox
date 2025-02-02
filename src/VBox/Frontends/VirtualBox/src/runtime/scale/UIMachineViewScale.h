/* $Id: UIMachineViewScale.h 84790 2020-06-11 10:30:36Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIMachineViewScale class declaration.
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

#ifndef FEQT_INCLUDED_SRC_runtime_scale_UIMachineViewScale_h
#define FEQT_INCLUDED_SRC_runtime_scale_UIMachineViewScale_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Local includes */
#include "UIMachineView.h"

class UIMachineViewScale : public UIMachineView
{
    Q_OBJECT;

protected:

    /* Scale machine-view constructor: */
    UIMachineViewScale(UIMachineWindow *pMachineWindow, ulong uScreenId);
    /* Scale machine-view destructor: */
    virtual ~UIMachineViewScale() {}

private slots:

    /* Slot to perform guest resize: */
    void sltPerformGuestScale();

private:

    /* Event handlers: */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);

    /** Applies machine-view scale-factor. */
    void applyMachineViewScaleFactor();

    /** Resends guest size-hint. */
    void resendSizeHint();

    /* Private helpers: */
    QSize sizeHint() const;
    QRect workingArea() const;
    QSize calculateMaxGuestSize() const;
    void updateSliders();

    /* Friend classes: */
    friend class UIMachineView;
};

#endif /* !FEQT_INCLUDED_SRC_runtime_scale_UIMachineViewScale_h */


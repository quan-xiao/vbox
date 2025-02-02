/* $Id: UIKeyboardHandlerFullscreen.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIKeyboardHandlerFullscreen class declaration.
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

#ifndef FEQT_INCLUDED_SRC_runtime_fullscreen_UIKeyboardHandlerFullscreen_h
#define FEQT_INCLUDED_SRC_runtime_fullscreen_UIKeyboardHandlerFullscreen_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIKeyboardHandler.h"

/** UIKeyboardHandler reimplementation
  * providing machine-logic with PopupMenu keyboard handler. */
class UIKeyboardHandlerFullscreen : public UIKeyboardHandler
{
    Q_OBJECT;

protected:

    /** Fullscreen keyboard-handler constructor. */
    UIKeyboardHandlerFullscreen(UIMachineLogic *pMachineLogic);
    /** Fullscreen keyboard-handler destructor. */
    virtual ~UIKeyboardHandlerFullscreen();

private:

    /** General event-filter. */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);

    /* Friend class: */
    friend class UIKeyboardHandler;
};

#endif /* !FEQT_INCLUDED_SRC_runtime_fullscreen_UIKeyboardHandlerFullscreen_h */

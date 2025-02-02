/* $Id: UIToolsHandlerKeyboard.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIToolsHandlerKeyboard class declaration.
 */

/*
 * Copyright (C) 2012-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_manager_tools_UIToolsHandlerKeyboard_h
#define FEQT_INCLUDED_SRC_manager_tools_UIToolsHandlerKeyboard_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QMap>
#include <QObject>

/* Forward declarations: */
class QKeyEvent;
class UIToolsModel;


/** Keyboard event types. */
enum UIKeyboardEventType
{
    UIKeyboardEventType_Press,
    UIKeyboardEventType_Release
};


/** QObject extension used as keyboard handler for graphics tools selector. */
class UIToolsHandlerKeyboard : public QObject
{
    Q_OBJECT;

public:

    /** Constructs keyboard handler passing @a pParent to the base-class. */
    UIToolsHandlerKeyboard(UIToolsModel *pParent);

    /** Handles keyboard @a pEvent of certain @a enmType. */
    bool handle(QKeyEvent *pEvent, UIKeyboardEventType enmType) const;

private:

    /** Returns the parent model reference. */
    UIToolsModel *model() const;

    /** Handles keyboard press @a pEvent. */
    bool handleKeyPress(QKeyEvent *pEvent) const;
    /** Handles keyboard release @a pEvent. */
    bool handleKeyRelease(QKeyEvent *pEvent) const;

    /** Holds the parent model reference. */
    UIToolsModel *m_pModel;
};


#endif /* !FEQT_INCLUDED_SRC_manager_tools_UIToolsHandlerKeyboard_h */

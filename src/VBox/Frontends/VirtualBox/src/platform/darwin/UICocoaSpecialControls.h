/* $Id: UICocoaSpecialControls.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UICocoaSpecialControls class declaration.
 */

/*
 * Copyright (C) 2009-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_platform_darwin_UICocoaSpecialControls_h
#define FEQT_INCLUDED_SRC_platform_darwin_UICocoaSpecialControls_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QMacCocoaViewContainer>
#include <QWidget>

/* GUI includes: */
#include "VBoxCocoaHelper.h"
#include "UILibraryDefs.h"

/* Add typedefs for Cocoa types: */
ADD_COCOA_NATIVE_REF(NSButton);

/** QMacCocoaViewContainer extension,
  * used as cocoa button container. */
class SHARED_LIBRARY_STUFF UICocoaButton : public QMacCocoaViewContainer
{
    Q_OBJECT

signals:

    /** Notifies about button click and whether it's @a fChecked. */
    void clicked(bool fChecked = false);

public:

    /** Cocoa button types. */
    enum CocoaButtonType
    {
        HelpButton,
        CancelButton,
        ResetButton
    };

    /** Constructs cocoa button passing @a pParent to the base-class.
      * @param  enmType  Brings the button type. */
    UICocoaButton(QWidget *pParent, CocoaButtonType enmType);
    /** Destructs cocoa button. */
    ~UICocoaButton();

    /** Returns size-hint. */
    QSize sizeHint() const;

    /** Defines button @a strText. */
    void setText(const QString &strText);
    /** Defines button @a strToolTip. */
    void setToolTip(const QString &strToolTip);

    /** Handles button click. */
    void onClicked();

private:

    /** Returns native cocoa button reference. */
    NativeNSButtonRef nativeRef() const { return static_cast<NativeNSButtonRef>(cocoaView()); }
};

#endif /* !FEQT_INCLUDED_SRC_platform_darwin_UICocoaSpecialControls_h */


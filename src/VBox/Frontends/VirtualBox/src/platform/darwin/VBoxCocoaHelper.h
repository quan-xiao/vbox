/* $Id: VBoxCocoaHelper.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - VBoxCocoa Helper.
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

#ifndef FEQT_INCLUDED_SRC_platform_darwin_VBoxCocoaHelper_h
#define FEQT_INCLUDED_SRC_platform_darwin_VBoxCocoaHelper_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Global includes */
#include <VBox/VBoxCocoa.h>

#ifdef __OBJC__

/* System includes */
#import <AppKit/NSImage.h>
#import <Foundation/NSAutoreleasePool.h>
#import <CoreFoundation/CFString.h>

/* Qt includes */
#include <QString>
#include <QVarLengthArray>

inline NSString *darwinQStringToNSString(const QString &aString)
{
    const UniChar *chars = reinterpret_cast<const UniChar *>(aString.unicode());
    CFStringRef str = CFStringCreateWithCharacters(0, chars, aString.length());
    return [(NSString*)CFStringCreateMutableCopy(0, 0, str) autorelease];
}

inline QString darwinNSStringToQString(const NSString *aString)
{
    CFStringRef str = reinterpret_cast<const CFStringRef>(aString);
    if(!str)
        return QString();
    CFIndex length = CFStringGetLength(str);
    const UniChar *chars = CFStringGetCharactersPtr(str);
    if (chars)
        return QString(reinterpret_cast<const QChar *>(chars), length);

    QVarLengthArray<UniChar> buffer(length);
    CFStringGetCharacters(str, CFRangeMake(0, length), buffer.data());
    return QString(reinterpret_cast<const QChar *>(buffer.constData()), length);
}

#endif /* __OBJC__ */

#endif /* !FEQT_INCLUDED_SRC_platform_darwin_VBoxCocoaHelper_h */


/* $Id: vboximgMedia.h 82968 2020-02-04 10:35:17Z vboxsync $ $Revision: 82968 $ $Date: 2020-02-04 18:35:17 +0800 (二, 2020-02-04) $ $Author: vboxsync $ */

/** @file
 * vboximgMedia.h
 */

/*
 * Copyright (C) 2008-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBOX_INCLUDED_SRC_vboximg_mount_vboximgMedia_h
#define VBOX_INCLUDED_SRC_vboximg_mount_vboximgMedia_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

typedef struct MEDIUMINFO
{
    char *name;
    char *uuid;
    char *location;
    char *description;
    char *state;
    char *size;
    char *format;
    int ro;
} MEDIUMINFO;

int vboximgListVMs(IVirtualBox *pVirtualBox);
char *vboximgScaledSize(size_t size);

#endif /* !VBOX_INCLUDED_SRC_vboximg_mount_vboximgMedia_h */

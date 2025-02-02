/* $Id: vbsfmount.c 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * vbsfmount - Commonly used code to mount shared folders on Linux-based
 *             systems.  Currently used by mount.vboxsf and VBoxService.
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <ctype.h>
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mount.h>

#include "vbsfmount.h"


/** @todo Use defines for return values! */
int vbsfmount_complete(const char *host_name, const char *mount_point,
                       unsigned long flags, struct vbsf_mount_opts *opts)
{
    FILE *f, *m;
    char *buf;
    size_t size;
    struct mntent e;
    int rc = 0;

    m = open_memstream(&buf, &size);
    if (!m)
        return 1; /* Could not update mount table (failed to create memstream). */

    if (opts->ttl != -1)
        fprintf(m, "ttl=%d,", opts->ttl);
    if (opts->msDirCacheTTL >= 0)
        fprintf(m, "dcachettl=%d,", opts->msDirCacheTTL);
    if (opts->msInodeTTL >= 0)
        fprintf(m, "inodettl=%d,", opts->msInodeTTL);
    if (opts->cMaxIoPages)
        fprintf(m, "maxiopages=%u,", opts->cMaxIoPages);
    if (opts->cbDirBuf)
        fprintf(m, "dirbuf=%u,", opts->cbDirBuf);
    switch (opts->enmCacheMode)
    {
        default:
        case kVbsfCacheMode_Default:
            break;
        case kVbsfCacheMode_None:       fprintf(m, "cache=none,"); break;
        case kVbsfCacheMode_Strict:     fprintf(m, "cache=strict,"); break;
        case kVbsfCacheMode_Read:       fprintf(m, "cache=read,"); break;
        case kVbsfCacheMode_ReadWrite:  fprintf(m, "cache=readwrite,"); break;
    }
    if (opts->uid)
        fprintf(m, "uid=%d,", opts->uid);
    if (opts->gid)
        fprintf(m, "gid=%d,", opts->gid);
    if (*opts->nls_name)
        fprintf(m, "iocharset=%s,", opts->nls_name);
    if (flags & MS_NOSUID)
        fprintf(m, "%s,", MNTOPT_NOSUID);
    if (flags & MS_RDONLY)
        fprintf(m, "%s,", MNTOPT_RO);
    else
        fprintf(m, "%s,", MNTOPT_RW);

    fclose(m);

    if (size > 0)
        buf[size - 1] = 0;
    else
        buf = "defaults";

    f = setmntent(MOUNTED, "a+");
    if (!f)
    {
        rc = 2; /* Could not open mount table for update. */
    }
    else
    {
        e.mnt_fsname = (char*)host_name;
        e.mnt_dir = (char*)mount_point;
        e.mnt_type = "vboxsf";
        e.mnt_opts = buf;
        e.mnt_freq = 0;
        e.mnt_passno = 0;

        if (addmntent(f, &e))
            rc = 3;  /* Could not add an entry to the mount table. */

        endmntent(f);
    }

    if (size > 0)
    {
        memset(buf, 0, size);
        free(buf);
    }

    return rc;
}

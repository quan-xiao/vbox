/* $Id: tstmmap.c 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * vboxsf - Simple writable mmap testcase.
 */

/*
 * Copyright (C) 2019-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>


int main(int argc, char **argv)
{
    uint8_t     abBuf[4096];
    int         fd;
    size_t      cErrors = 0;
    size_t      cbFile;
    size_t      offFile;
    uint8_t    *pbMapping;
    const char *pszFile = "tstmmap-file1";
    if (argc > 1)
        pszFile = argv[1];

    fd = open(pszFile, O_CREAT | O_TRUNC | O_RDWR, 0660);
    if (fd < 0)
    {
        fprintf(stderr, "error creating file: %s\n", pszFile);
        return 1;
    }

    /* write 64 KB to the file: */
    memset(abBuf, 0xf6, sizeof(abBuf));
    for (cbFile = 0; cbFile < 0x10000; cbFile += sizeof(abBuf))
        if (write(fd, abBuf, sizeof(abBuf)) != sizeof(abBuf))
        {
            fprintf(stderr, "error writing file: %s\n", pszFile);
            return 1;
        }
    fsync(fd);

    /* Map the file: */
    pbMapping = (uint8_t *)mmap(NULL, cbFile, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (pbMapping == (void *)-1)
    {
        fprintf(stderr, "error mapping file: %s\n", pszFile);
        return 1;
    }

    /* Modify the mapping and sync it: */
    memset(pbMapping, 0xf7, cbFile);
    if (msync(pbMapping, cbFile, MS_SYNC) != 0)
    {
        fprintf(stderr, "error msync'ing file: %s\n", pszFile);
        return 1;
    }

    /* Unmap and close it: */
    if (munmap(pbMapping, cbFile) != 0)
        fprintf(stderr, "error munmap'ing file: %s\n", pszFile);
    close(fd);

    /*
     * Open it again and check the content.
     */
    fd = open(pszFile, O_RDWR, 0);
    if (fd < 0)
    {
        fprintf(stderr, "error reopening file: %s\n", pszFile);
        return 1;
    }

    while (offFile < cbFile && cErrors < 42)
    {
        size_t offBuf;
        ssize_t cbRead = read(fd, abBuf, sizeof(abBuf));
        if (cbRead != (ssize_t)sizeof(abBuf))
        {
            fprintf(stderr, "error reading file: %zd, off %#zx (%s)\n", cbRead, offFile, pszFile);
            return 1;
        }

        for (offBuf = 0; offBuf < sizeof(abBuf); offBuf++)
            if (abBuf[offBuf] != 0xf7)
            {
                fprintf(stderr, "mismatch at %#zx: %#x, expected %#x\n", offFile + offBuf, abBuf[offBuf], 0xf7);
                cErrors++;
                if (cErrors > 42)
                    break;
            }

        offFile += sizeof(abBuf);
    }

    close(fd);

    return cErrors == 0 ? 0 : 1;
}


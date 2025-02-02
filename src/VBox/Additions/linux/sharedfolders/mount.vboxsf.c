/* $Id: mount.vboxsf.c 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VirtualBox Guest Additions for Linux - mount(8) helper.
 *
 * Parses options provided by mount (or user directly)
 * Packs them into struct vbsfmount and passes to mount(2)
 * Optionally adds entries to mtab
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
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
# define _GNU_SOURCE
#endif

/* #define DEBUG */
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <getopt.h>
#include <mntent.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <mntent.h>
#include <limits.h>
#include <iconv.h>

#include "vbsfmount.h"

#include <iprt/assert.h>


#define PANIC_ATTR __attribute ((noreturn, __format__ (__printf__, 1, 2)))

static void PANIC_ATTR
panic(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

static void PANIC_ATTR
panic_err(const char *fmt, ...)
{
    va_list ap;
    int errno_code = errno;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, ": %s\n", strerror(errno_code));
    exit(EXIT_FAILURE);
}

static int
safe_atoi(const char *s, size_t size, int base)
{
    char *endptr;
    long long int val = strtoll(s, &endptr, base);

    if (   val < INT_MIN
        || (   val > INT_MAX
            && (base != 8 || val != UINT_MAX) ) /* hack for printf("%o", -1) - 037777777777 */
        || endptr < s + size)
    {
        errno = ERANGE;
        panic_err("could not convert %.*s to integer, result = %lld (%d)",
                  (int)size, s, val, (int)val);
    }
    return (int)val;
}

static unsigned
safe_atoiu(const char *s, size_t size, int base)
{
    char *endptr;
    long long int val = strtoll(s, &endptr, base);

    if (   val < 0
        || val > UINT_MAX
        || endptr < s + size)
    {
        errno = ERANGE;
        panic_err("could not convert %.*s to unsigned integer, result = %lld (%#llx)",
                  (int)size, s, val, val);
    }
    return (unsigned)val;
}

static void
process_mount_opts(const char *s, struct vbsf_mount_opts *opts)
{
    const char *next = s;
    size_t len;
    typedef enum handler_opt
    {
        HO_RW,
        HO_RO,
        HO_UID,
        HO_GID,
        HO_TTL,
        HO_DENTRY_TTL,
        HO_INODE_TTL,
        HO_MAX_IO_PAGES,
        HO_DIR_BUF,
        HO_CACHE,
        HO_DMODE,
        HO_FMODE,
        HO_UMASK,
        HO_DMASK,
        HO_FMASK,
        HO_IOCHARSET,
        HO_CONVERTCP,
        HO_NOEXEC,
        HO_EXEC,
        HO_NODEV,
        HO_DEV,
        HO_NOSUID,
        HO_SUID,
        HO_REMOUNT,
        HO_NOAUTO,
        HO_NIGNORE
    } handler_opt;
    struct
    {
        const char *name;
        handler_opt opt;
        int has_arg;
        const char *desc;
    } handlers[] =
    {
        {"rw",          HO_RW,              0, "mount read write (default)"},
        {"ro",          HO_RO,              0, "mount read only"},
        {"uid",         HO_UID,             1, "default file owner user id"},
        {"gid",         HO_GID,             1, "default file owner group id"},
        {"ttl",         HO_TTL,             1, "time to live for dentries & inode info"},
        {"dcachettl",   HO_DENTRY_TTL,      1, "time to live for dentries"},
        {"inodettl",    HO_INODE_TTL,       1, "time to live for inode info"},
        {"maxiopages",  HO_MAX_IO_PAGES,    1, "max buffer size for I/O with host"},
        {"dirbuf",      HO_DIR_BUF,         1, "directory buffer size (0 for default)"},
        {"cache",       HO_CACHE,           1, "cache mode: none, strict (default), read, readwrite"},
        {"iocharset",   HO_IOCHARSET,       1, "i/o charset (default utf8)"},
        {"convertcp",   HO_CONVERTCP,       1, "convert share name from given charset to utf8"},
        {"dmode",       HO_DMODE,           1, "mode of all directories"},
        {"fmode",       HO_FMODE,           1, "mode of all regular files"},
        {"umask",       HO_UMASK,           1, "umask of directories and regular files"},
        {"dmask",       HO_DMASK,           1, "umask of directories"},
        {"fmask",       HO_FMASK,           1, "umask of regular files"},
        {"noexec",      HO_NOEXEC,          0, NULL}, /* don't document these options directly here */
        {"exec",        HO_EXEC,            0, NULL}, /* as they are well known and described in the */
        {"nodev",       HO_NODEV,           0, NULL}, /* usual manpages */
        {"dev",         HO_DEV,             0, NULL},
        {"nosuid",      HO_NOSUID,          0, NULL},
        {"suid",        HO_SUID,            0, NULL},
        {"remount",     HO_REMOUNT,         0, NULL},
        {"noauto",      HO_NOAUTO,          0, NULL},
        {"_netdev",     HO_NIGNORE,         0, NULL},
        {"relatime",    HO_NIGNORE,         0, NULL},
        {NULL,          0,                  0, NULL}
    }, *handler;

    while (next)
    {
        const char *val;
        size_t key_len, val_len;

        s = next;
        next = strchr(s, ',');
        if (!next)
        {
            len = strlen(s);
        }
        else
        {
            len = next - s;
            next += 1;
            if (!*next)
                next = 0;
        }

        val = NULL;
        val_len = 0;
        for (key_len = 0; key_len < len; ++key_len)
        {
            if (s[key_len] == '=')
            {
                if (key_len + 1 < len)
                {
                    val = s + key_len + 1;
                    val_len = len - key_len - 1;
                }
                break;
            }
        }

        for (handler = handlers; handler->name; ++handler)
        {
            size_t j;
            for (j = 0; j < key_len && handler->name[j] == s[j]; ++j)
                ;

            if (j == key_len && !handler->name[j])
            {
                if (handler->has_arg)
                {
                    if (!(val && *val))
                    {
                        panic("%.*s requires an argument (i.e. %.*s=<arg>)\n",
                               (int)len, s, (int)len, s);
                    }
                }

                switch (handler->opt)
                {
                    case HO_RW:
                        opts->ronly = 0;
                        break;
                    case HO_RO:
                        opts->ronly = 1;
                        break;
                    case HO_NOEXEC:
                        opts->noexec = 1;
                        break;
                    case HO_EXEC:
                        opts->noexec = 0;
                        break;
                    case HO_NODEV:
                        opts->nodev = 1;
                        break;
                    case HO_DEV:
                        opts->nodev = 0;
                        break;
                    case HO_NOSUID:
                        opts->nosuid = 1;
                        break;
                    case HO_SUID:
                        opts->nosuid = 0;
                        break;
                    case HO_REMOUNT:
                        opts->remount = 1;
                        break;
                    case HO_TTL:
                        opts->ttl = safe_atoi(val, val_len, 10);
                        break;
                    case HO_DENTRY_TTL:
                        opts->msDirCacheTTL = safe_atoi(val, val_len, 10);
                        break;
                    case HO_INODE_TTL:
                        opts->msInodeTTL = safe_atoi(val, val_len, 10);
                        break;
                    case HO_MAX_IO_PAGES:
                        opts->cMaxIoPages = safe_atoiu(val, val_len, 10);
                        break;
                    case HO_DIR_BUF:
                        opts->cbDirBuf = safe_atoiu(val, val_len, 10);
                        break;
                    case HO_CACHE:
#define IS_EQUAL(a_sz) (val_len == sizeof(a_sz) - 1U && strncmp(val, a_sz, sizeof(a_sz) - 1U) == 0)
                        if (IS_EQUAL("default"))
                            opts->enmCacheMode = kVbsfCacheMode_Default;
                        else if (IS_EQUAL("none"))
                            opts->enmCacheMode = kVbsfCacheMode_None;
                        else if (IS_EQUAL("strict"))
                            opts->enmCacheMode = kVbsfCacheMode_Strict;
                        else if (IS_EQUAL("read"))
                            opts->enmCacheMode = kVbsfCacheMode_Read;
                        else if (IS_EQUAL("readwrite"))
                            opts->enmCacheMode = kVbsfCacheMode_ReadWrite;
                        else
                            panic("invalid cache mode '%.*s'\n"
                                  "Valid cache modes are: default, none, strict, read, readwrite\n",
                                  (int)val_len, val);
                        break;
                    case HO_UID:
                        /** @todo convert string to id. */
                        opts->uid = safe_atoi(val, val_len, 10);
                        break;
                    case HO_GID:
                        /** @todo convert string to id. */
                        opts->gid = safe_atoi(val, val_len, 10);
                        break;
                    case HO_DMODE:
                        opts->dmode = safe_atoi(val, val_len, 8);
                        break;
                    case HO_FMODE:
                        opts->fmode = safe_atoi(val, val_len, 8);
                        break;
                    case HO_UMASK:
                        opts->dmask = opts->fmask = safe_atoi(val, val_len, 8);
                        break;
                    case HO_DMASK:
                        opts->dmask = safe_atoi(val, val_len, 8);
                        break;
                    case HO_FMASK:
                        opts->fmask = safe_atoi(val, val_len, 8);
                        break;
                    case HO_IOCHARSET:
                        if (val_len + 1 > sizeof(opts->nls_name))
                        {
                            panic("iocharset name too long\n");
                        }
                        memcpy(opts->nls_name, val, val_len);
                        opts->nls_name[val_len] = 0;
                        break;
                    case HO_CONVERTCP:
                        opts->convertcp = malloc(val_len + 1);
                        if (!opts->convertcp)
                        {
                            panic_err("could not allocate memory");
                        }
                        memcpy(opts->convertcp, val, val_len);
                        opts->convertcp[val_len] = 0;
                        break;
                    case HO_NOAUTO:
                    case HO_NIGNORE:
                        break;
                }
                break;
            }
            continue;
        }

        if (   !handler->name
            && !opts->sloppy)
        {
            fprintf(stderr, "unknown mount option `%.*s'\n", (int)len, s);
            fprintf(stderr, "valid options:\n");

            for (handler = handlers; handler->name; ++handler)
            {
                if (handler->desc)
                    fprintf(stderr, "  %-10s%s %s\n", handler->name,
                         handler->has_arg ? "=<arg>" : "", handler->desc);
            }
            exit(EXIT_FAILURE);
        }
    }
}

static void
convertcp(char *in_codeset, char *host_name, struct vbsf_mount_info_new *info)
{
    char *i = host_name;
    char *o = info->name;
    size_t ib = strlen(host_name);
    size_t ob = sizeof(info->name) - 1;
    iconv_t cd;

    cd = iconv_open("UTF-8", in_codeset);
    if (cd == (iconv_t) -1)
    {
        panic_err("could not convert share name, iconv_open `%s' failed",
                   in_codeset);
    }

    while (ib)
    {
        size_t c = iconv(cd, &i, &ib, &o, &ob);
        if (c == (size_t) -1)
        {
            panic_err("could not convert share name(%s) at %d",
                      host_name, (int)(strlen (host_name) - ib));
        }
    }
    *o = 0;
}


/**
  * Print out a usage message and exit.
  *
  * @returns 1
  * @param   argv0      The name of the application
  */
static int usage(char *argv0)
{
    printf("Usage: %s [OPTIONS] NAME MOUNTPOINT\n"
           "Mount the VirtualBox shared folder NAME from the host system to MOUNTPOINT.\n"
           "\n"
           "  -w                    mount the shared folder writable (the default)\n"
           "  -r                    mount the shared folder read-only\n"
           "  -n                    do not create an mtab entry\n"
           "  -s                    sloppy parsing, ignore unrecognized mount options\n"
           "  -o OPTION[,OPTION...] use the mount options specified\n"
           "\n", argv0);
    printf("Available mount options are:\n"
           "     rw                 mount writable (the default)\n"
           "     ro                 mount read only\n"
           "     uid=UID            set the default file owner user id to UID\n"
           "     gid=GID            set the default file owner group id to GID\n");
    printf("     ttl=MILLIESECSONDS set the \"time to live\" for both the directory cache\n"
           "                        and inode info.  -1 for kernel default, 0 disables it.\n"
           "     dcachettl=MILLIES  set the \"time to live\" for the directory cache,\n"
           "                        overriding the 'ttl' option.  Ignored if negative.\n"
           "     inodettl=MILLIES   set the \"time to live\" for the inode information,\n"
           "                        overriding the 'ttl' option.  Ignored if negative.\n");
    printf("     maxiopages=PAGES   set the max host I/O buffers size in pages. Uses\n"
           "                        default if zero.\n"
           "     dirbuf=BYTES       set the directory enumeration buffer size in bytes.\n"
           "                        Uses default size if zero.\n");
    printf("     cache=MODE         set the caching mode for the mount.  Allowed values:\n"
           "                          default: use the kernel default (strict)\n"
           "                             none: no caching; may experience guest side\n"
           "                                   coherence issues between mmap and read.\n");
    printf("                           strict: no caching, except for writably mapped\n"
           "                                   files (for guest side coherence)\n"
           "                             read: read via the page cache; host changes\n"
           "                                   may be completely ignored\n");
    printf("                        readwrite: read and write via the page cache; host\n"
           "                                   changes may be completely ignored and\n"
           "                                   guest changes takes a while to reach the host\n");
    printf("     dmode=MODE         override the mode of all directories to (octal) MODE\n"
           "     fmode=MODE         override the mode of all regular files to (octal) MODE\n"
           "     umask=UMASK        set the umask to (octal) UMASK\n");
    printf("     dmask=UMASK        set the umask applied to directories only\n"
           "     fmask=UMASK        set the umask applied to regular files only\n"
           "     iocharset CHARSET  use the character set CHARSET for I/O operations\n"
           "                        (default set is utf8)\n"
           "     convertcp CHARSET  convert the folder name from CHARSET to utf8\n"
           "\n");
    printf("Less common used options:\n"
           "     noexec,exec,nodev,dev,nosuid,suid\n");
    return EXIT_FAILURE;
}

int
main(int argc, char **argv)
{
    int c;
    int err;
    int saved_errno;
    int nomtab = 0;
    unsigned long flags = MS_NODEV;
    char *host_name;
    char *mount_point;
    struct vbsf_mount_info_new mntinf;
    struct vbsf_mount_opts opts =
    {
        -1,    /* ttl */
        -1,    /* msDirCacheTTL */
        -1,    /* msInodeTTL */
        0,     /* cMaxIoPages */
        0,     /* cbDirBuf */
        kVbsfCacheMode_Default,
        0,     /* uid */
        0,     /* gid */
       ~0U,    /* dmode */
       ~0U,    /* fmode*/
        0,     /* dmask */
        0,     /* fmask */
        0,     /* ronly */
        0,     /* sloppy */
        0,     /* noexec */
        0,     /* nodev */
        0,     /* nosuid */
        0,     /* remount */
        "\0",  /* nls_name */
        NULL,  /* convertcp */
    };
    AssertCompile(sizeof(uid_t) == sizeof(int));
    AssertCompile(sizeof(gid_t) == sizeof(int));

    memset(&mntinf, 0, sizeof(mntinf));
    mntinf.nullchar = '\0';
    mntinf.signature[0] = VBSF_MOUNT_SIGNATURE_BYTE_0;
    mntinf.signature[1] = VBSF_MOUNT_SIGNATURE_BYTE_1;
    mntinf.signature[2] = VBSF_MOUNT_SIGNATURE_BYTE_2;
    mntinf.length       = sizeof(mntinf);
    mntinf.szTag[0] = '\0';

    if (getuid())
        panic("Only root can mount shared folders from the host.\n");

    if (!argv[0])
        argv[0] = "mount.vboxsf";

    while ((c = getopt(argc, argv, "rwsno:h")) != -1)
    {
        switch (c)
        {
            default:
                fprintf(stderr, "unknown option `%c:%#x'\n", c, c);
                RT_FALL_THRU();
            case '?':
            case 'h':
                return usage(argv[0]);

            case 'r':
                opts.ronly = 1;
                break;

            case 'w':
                opts.ronly = 0;
                break;

            case 's':
                opts.sloppy = 1;
                break;

            case 'o':
                process_mount_opts(optarg, &opts);
                break;

            case 'n':
                nomtab = 1;
                break;
        }
    }

    if (argc - optind < 2)
        return usage(argv[0]);

    host_name = argv[optind];
    mount_point = argv[optind + 1];

    if (opts.convertcp)
        convertcp(opts.convertcp, host_name, &mntinf);
    else
    {
        if (strlen(host_name) > MAX_HOST_NAME - 1)
            panic("host name is too big\n");

        strcpy(mntinf.name, host_name);
    }

    if (strlen(opts.nls_name) > MAX_NLS_NAME - 1)
        panic("%s: the character set name for I/O is too long.\n", argv[0]);

    strcpy(mntinf.nls_name, opts.nls_name);

    if (opts.ronly)
        flags |= MS_RDONLY;
    if (opts.noexec)
        flags |= MS_NOEXEC;
    if (opts.nodev)
        flags |= MS_NODEV;
    if (opts.remount)
        flags |= MS_REMOUNT;

    mntinf.ttl              = opts.ttl;
    mntinf.msDirCacheTTL    = opts.msDirCacheTTL;
    mntinf.msInodeTTL       = opts.msInodeTTL;
    mntinf.cMaxIoPages      = opts.cMaxIoPages;
    mntinf.cbDirBuf         = opts.cbDirBuf;
    mntinf.enmCacheMode     = opts.enmCacheMode;

    mntinf.uid   = opts.uid;
    mntinf.gid   = opts.gid;
    mntinf.dmode = opts.dmode;
    mntinf.fmode = opts.fmode;
    mntinf.dmask = opts.dmask;
    mntinf.fmask = opts.fmask;

    /*
     * Note: When adding and/or modifying parameters of the vboxsf mounting
     *       options you also would have to adjust VBoxServiceAutoMount.cpp
     *       to keep this code here slick without having VbglR3.
     */
    err = mount(host_name, mount_point, "vboxsf", flags, &mntinf);
    saved_errno = errno;

    /* Some versions of the mount utility (unknown which, if any) will turn the
       shared folder name into an absolute path.  So, we check if it starts with
       the CWD and removes it.  We must do this after failing, because there is
       not actual restrictions on the shared folder name wrt to slashes and such. */
    if (err == -1 && errno == ENXIO && host_name[0] == '/')
    {
        char szCWD[4096];
        if (getcwd(szCWD, sizeof(szCWD)) != NULL)
        {
            size_t cchCWD = strlen(szCWD);
            if (!strncmp(host_name, szCWD, cchCWD))
            {
                while (host_name[cchCWD] == '/')
                    ++cchCWD;
                if (host_name[cchCWD])
                {
                    /* We checked before that we have enough space. */
                    strcpy(mntinf.name, host_name + cchCWD);
                    err = mount(host_name, mount_point, "vboxsf", flags, &mntinf);
                    saved_errno = errno;
                }
            }
        }
        else
            fprintf(stderr, "%s: failed to get the current working directory: %s", argv[0], strerror(errno));
        errno = saved_errno;
    }
    if (err)
    {
        if (saved_errno == ENXIO)
            panic("%s: shared folder '%s' was not found (check VM settings / spelling)\n", argv[0], host_name);
        else
            panic_err("%s: mounting failed with the error", argv[0]);
    }

    if (!nomtab)
    {
        err = vbsfmount_complete(host_name, mount_point, flags, &opts);
        switch (err)
        {
            case 0: /* Success. */
                break;

            case 1:
                panic_err("%s: Could not update mount table (failed to create memstream).", argv[0]);
                break;

            case 2:
                panic_err("%s: Could not open mount table for update.", argv[0]);
                break;

            case 3:
                /* panic_err("%s: Could not add an entry to the mount table.", argv[0]); */
                break;

            default:
                panic_err("%s: Unknown error while completing mount operation: %d", argv[0], err);
                break;
        }
    }

    exit(EXIT_SUCCESS);
}


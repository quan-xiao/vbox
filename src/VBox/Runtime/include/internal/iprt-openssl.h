/* $Id: iprt-openssl.h 84248 2020-05-11 11:46:40Z vboxsync $ */
/** @file
 * IPRT - Internal header for the OpenSSL helpers.
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
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef IPRT_INCLUDED_INTERNAL_iprt_openssl_h
#define IPRT_INCLUDED_INTERNAL_iprt_openssl_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/crypto/x509.h>

RT_C_DECLS_BEGIN
struct evp_md_st;
struct evp_pkey_st;

DECLHIDDEN(void) rtCrOpenSslInit(void);
DECLHIDDEN(int)  rtCrOpenSslErrInfoCallback(const char *pach, size_t cch, void *pvUser);
DECLHIDDEN(int)  rtCrOpenSslConvertX509Cert(void **ppvOsslCert, PCRTCRX509CERTIFICATE pCert, PRTERRINFO pErrInfo);
DECLHIDDEN(void) rtCrOpenSslFreeConvertedX509Cert(void *pvOsslCert);
DECLHIDDEN(int)  rtCrOpenSslAddX509CertToStack(void *pvOsslStack, PCRTCRX509CERTIFICATE pCert, PRTERRINFO pErrInfo);
DECLHIDDEN(const void /*EVP_MD*/ *) rtCrOpenSslConvertDigestType(RTDIGESTTYPE enmDigestType, PRTERRINFO pErrInfo);

DECLHIDDEN(int)  rtCrKeyToOpenSslKey(RTCRKEY hKey, bool fNeedPublic, void /*EVP_PKEY*/ **ppEvpKey, PRTERRINFO pErrInfo);
DECLHIDDEN(int)  rtCrKeyToOpenSslKeyEx(RTCRKEY hKey, bool fNeedPublic, const char *pszAlgoObjId,
                                       void /*EVP_PKEY*/ **ppEvpKey, const void /*EVP_MD*/ **ppEvpMdType, PRTERRINFO pErrInfo);

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_INTERNAL_iprt_openssl_h */


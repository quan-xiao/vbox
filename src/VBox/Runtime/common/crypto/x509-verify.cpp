/* $Id: x509-verify.cpp 85627 2020-08-06 00:43:00Z vboxsync $ */
/** @file
 * IPRT - Crypto - X.509, Signature verification.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "internal/iprt.h"
#include <iprt/crypto/x509.h>
#include <iprt/crypto/pkix.h>
#include <iprt/crypto/key.h>

#include <iprt/err.h>
#include <iprt/mem.h>
#include <iprt/string.h>


RTDECL(int) RTCrX509Certificate_VerifySignature(PCRTCRX509CERTIFICATE pThis, PCRTASN1OBJID pAlgorithm,
                                                PCRTASN1DYNTYPE pParameters, PCRTASN1BITSTRING pPublicKey,
                                                PRTERRINFO pErrInfo)
{
    /*
     * Validate the input a little.
     */
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);
    AssertReturn(RTCrX509Certificate_IsPresent(pThis), VERR_INVALID_PARAMETER);

    AssertPtrReturn(pAlgorithm, VERR_INVALID_POINTER);
    AssertReturn(RTAsn1ObjId_IsPresent(pAlgorithm), VERR_INVALID_POINTER);

    if (pParameters)
    {
        AssertPtrReturn(pParameters, VERR_INVALID_POINTER);
        if (pParameters->enmType == RTASN1TYPE_NULL)
            pParameters = NULL;
    }

    AssertPtrReturn(pPublicKey, VERR_INVALID_POINTER);
    AssertReturn(RTAsn1BitString_IsPresent(pPublicKey), VERR_INVALID_POINTER);

    /*
     * Check if the algorithm matches.
     */
    const char *pszCipherOid = RTCrPkixGetCiperOidFromSignatureAlgorithm(&pThis->SignatureAlgorithm.Algorithm);
    if (!pszCipherOid)
        return RTErrInfoSetF(pErrInfo, VERR_CR_X509_UNKNOWN_CERT_SIGN_ALGO,
                             "Certificate signature algorithm not known: %s",
                             pThis->SignatureAlgorithm.Algorithm.szObjId);

    if (RTAsn1ObjId_CompareWithString(pAlgorithm, pszCipherOid) != 0)
        return RTErrInfoSetF(pErrInfo, VERR_CR_X509_CERT_SIGN_ALGO_MISMATCH,
                             "Certificate signature cipher algorithm mismatch: cert uses %s (%s) while key uses %s",
                             pszCipherOid, pThis->SignatureAlgorithm.Algorithm.szObjId, pAlgorithm->szObjId);

    /*
     * Wrap up the public key.
     */
    RTCRKEY hPubKey;
    int rc = RTCrKeyCreateFromPublicAlgorithmAndBits(&hPubKey, pAlgorithm, pPublicKey, pErrInfo, NULL);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Here we should recode the to-be-signed part as DER, but we'll ASSUME
     * that it's already in DER encoding and only does this if there the
     * encoded bits are missing.
     */
    const uint8_t  *pbRaw;
    uint32_t        cbRaw;
    void           *pvFree = NULL;
    rc = RTAsn1EncodeQueryRawBits(RTCrX509TbsCertificate_GetAsn1Core(&pThis->TbsCertificate), &pbRaw, &cbRaw, &pvFree, pErrInfo);
    if (RT_SUCCESS(rc))
    {
        rc = RTCrPkixPubKeyVerifySignature(&pThis->SignatureAlgorithm.Algorithm, hPubKey, pParameters, &pThis->SignatureValue,
                                           pbRaw, cbRaw, pErrInfo);
        RTMemTmpFree(pvFree);
    }

    /* Free the public key. */
    uint32_t cRefs = RTCrKeyRelease(hPubKey);
    Assert(cRefs == 0); NOREF(cRefs);

    return rc;
}


RTDECL(int) RTCrX509Certificate_VerifySignatureSelfSigned(PCRTCRX509CERTIFICATE pThis, PRTERRINFO pErrInfo)
{
    /*
     * Validate the input a little.
     */
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);
    AssertReturn(RTCrX509Certificate_IsPresent(pThis), VERR_INVALID_PARAMETER);

    /*
     * Assemble parameters for the generic verification call.
     */
    PCRTCRX509TBSCERTIFICATE const pTbsCert    = &pThis->TbsCertificate;
    PCRTASN1DYNTYPE                pParameters = NULL;
    if (   RTASN1CORE_IS_PRESENT(&pTbsCert->SubjectPublicKeyInfo.Algorithm.Parameters.u.Core)
        && pTbsCert->SubjectPublicKeyInfo.Algorithm.Parameters.enmType != RTASN1TYPE_NULL)
        pParameters = &pTbsCert->SubjectPublicKeyInfo.Algorithm.Parameters;
    return RTCrX509Certificate_VerifySignature(pThis, &pTbsCert->SubjectPublicKeyInfo.Algorithm.Algorithm, pParameters,
                                               &pTbsCert->SubjectPublicKeyInfo.SubjectPublicKey, pErrInfo);
}


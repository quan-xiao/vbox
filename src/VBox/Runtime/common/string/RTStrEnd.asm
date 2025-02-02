; $Id: RTStrEnd.asm 83886 2020-04-21 11:14:20Z vboxsync $
;; @file
; IPRT - RTStrEnd - AMD64 & X86.
;

;
; Copyright (C) 2006-2020 Oracle Corporation
;
; This file is part of VirtualBox Open Source Edition (OSE), as
; available from http://www.virtualbox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the GNU
; General Public License (GPL) as published by the Free Software
; Foundation, in version 2 as it comes in the "COPYING" file of the
; VirtualBox OSE distribution. VirtualBox OSE is distributed in the
; hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;
; The contents of this file may alternatively be used under the terms
; of the Common Development and Distribution License Version 1.0
; (CDDL) only, as it comes in the "COPYING.CDDL" file of the
; VirtualBox OSE distribution, in which case the provisions of the
; CDDL are applicable instead of those of the GPL.
;
; You may elect to license modified versions of this file under the
; terms and conditions of either the GPL or the CDDL or both.
;

%include "iprt/asmdefs.mac"

BEGINCODE

;;
; @param    pszString   gcc: rdi  msc: rcx  x86:[esp+4]   wcall: eax
; @param    cchMax      gcc: rsi  msc: rdx  x86:[esp+8]   wcall: edx
;
BEGINPROC_EXPORTED RTStrEnd
        cld
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_MSC
        or      rdx, rdx
        jz      .not_found_early

        mov     r9, rdi                 ; save rdi
        mov     rdi, rcx
        mov     rcx, rdx
 %else
        mov     rcx, rsi
        jrcxz   .not_found_early
 %endif

%else
 %ifdef ASM_CALL32_WATCOM
        mov     ecx, edx
        jecxz   .not_found_early
        mov     edx, edi                ; save rdi
        mov     edi, eax
 %else
        mov     ecx, [esp + 8]
        jecxz   .not_found_early
        mov     edx, edi                ; save edi
        mov     edi, [esp + 4]
 %endif
%endif
        xor     eax, eax                ; we're searching for zero

        ; do the search
        repne   scasb
        jne     .not_found

        ; found it
        lea     xAX, [xDI - 1]
%ifdef ASM_CALL64_MSC
        mov     rdi, r9
%endif
%ifdef RT_ARCH_X86
        mov     edi, edx
%endif
        ret

.not_found:
%ifdef ASM_CALL64_MSC
        mov     rdi, r9
%endif
%ifdef RT_ARCH_X86
        mov     edi, edx
%endif
.not_found_early:
        xor     eax, eax
        ret
ENDPROC RTStrEnd


; $Id: ASMFxSave.asm 82968 2020-02-04 10:35:17Z vboxsync $
;; @file
; IPRT - ASMFxSave().
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

;*******************************************************************************
;* Header Files                                                                *
;*******************************************************************************
%define RT_ASM_WITH_SEH64
%include "iprt/asmdefs.mac"

BEGINCODE

;;
; Saves extended CPU state.
; @param    pFxState    Pointer to the XSAVE state area.
;                       msc=rcx, gcc=rdi, x86=[esp+4]
;
BEGINPROC_EXPORTED ASMFxSave
SEH64_END_PROLOGUE
%ifdef ASM_CALL64_MSC
        o64 fxsave [rcx]
%elifdef ASM_CALL64_GCC
        o64 fxsave [rdi]
%elif ARCH_BITS == 32
        mov     ecx, [esp + 4]
        fxsave  [ecx]
%elif ARCH_BITS == 16
        push    bp
        mov     bp, sp
        push    es
        push    bx
        les     bx, [bp + 4]
        fxsave  [es:bx]
        pop     bx
        pop     es
        pop     bp
%else
 %error "Undefined arch?"
%endif
        ret
ENDPROC ASMFxSave


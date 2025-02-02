; $Id: alloca-x86-r0drv-nt.asm 86190 2020-09-21 08:54:23Z vboxsync $
;; @file
; IPRT - Visual C++ __alloca__probe_16.
;

;
; Copyright (C) 2020 Oracle Corporation
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
%include "iprt/asmdefs.mac"

BEGINCODE

;;
; @returns  esp with eax subtracted and aligned to 16 bytes.
; @param    eax allocation size.
;
BEGINPROC _alloca_probe_16
        ; Sanitycheck the input.
%ifdef DEBUG
        cmp     eax, 0
        jne     .not_zero
        int3
.not_zero:
        cmp     eax, 4096
        jbe     .four_kb_or_less
        int3
.four_kb_or_less:
%endif

        ; Don't bother probing the stack as the allocation is supposed to be
        ; a lot smaller than 4KB.
        neg     eax
        lea     eax, [esp + eax + 4]
        and     eax, 0fffffff0h
        xchg    eax, esp
        jmp     [eax]
ENDPROC _alloca_probe_16


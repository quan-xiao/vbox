; $Id: strncpy.asm 82968 2020-02-04 10:35:17Z vboxsync $
;; @file
; IPRT - No-CRT strncpy - AMD64 & X86.
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
; @param    pszDst   gcc: rdi  msc: rcx  x86:[esp+4]   wcall: eax
; @param    pszSrc   gcc: rsi  msc: rdx  x86:[esp+8]   wcall: edx
; @param    cbMax    gcc: rdx  msc: r8   x86:[esp+12]  wcall: ebx
RT_NOCRT_BEGINPROC strncpy
        ; input
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_MSC
  %define pszDst rcx
  %define pszSrc rdx
  %define cbMax  r8
 %else
  %define pszDst rdi
  %define pszSrc rsi
  %define cbMax  rdx
 %endif
        mov     r9, pszDst
%else
 %ifdef ASM_CALL32_WATCOM
  mov   ecx, eax
  %define pszDst ecx
  %define pszSrc edx
  %define cbMax  ebx
 %else
        mov     ecx, [esp + 4]
        mov     edx, [esp + 8]
        push    ebx
        mov     ebx, [esp + 12 + 4]
  %define pszDst ecx
  %define pszSrc edx
  %define cbMax  ebx
 %endif
        push    pszDst
%endif

        ;
        ; The rolled out loop.
        ;
.next:
        cmp     cbMax, 4
        jb      .simple_intro

        mov     al, [pszSrc]
        mov     [pszDst], al
        test    al, al
        jz      .done

        mov     al, [pszSrc + 1]
        mov     [pszDst + 1], al
        test    al, al
        jz      .done

        mov     al, [pszSrc + 2]
        mov     [pszDst + 2], al
        test    al, al
        jz      .done

        mov     al, [pszSrc + 3]
        mov     [pszDst + 3], al
        test    al, al
        jz      .done

        add     pszDst, 4
        add     pszSrc, 4
        sub     cbMax,  4
        jmp     .next

        ;
        ; Char by char.
        ;
.simple_intro:
        test    cbMax, cbMax
        jz      .done

.simple_next:
        mov     al, [pszSrc]
        mov     [pszDst], al
        test    al, al
        jz      .done

        dec     cbMax
        jz      .done

        inc     pszSrc
        inc     pszDst
        jmp     .simple_next

.done:
%ifdef RT_ARCH_AMD64
        mov     rax, r9
%else
 %ifndef ASM_CALL32_WATCOM
        pop     ebx
 %endif
        pop     eax
%endif
        ret
ENDPROC RT_NOCRT(strncpy)


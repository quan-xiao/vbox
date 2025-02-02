; $Id: bs3-wc16-U8RS.asm 82968 2020-02-04 10:35:17Z vboxsync $
;; @file
; BS3Kit - 16-bit Watcom C/C++, 64-bit unsigned integer right shift.
;

;
; Copyright (C) 2007-2020 Oracle Corporation
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

%include "bs3kit-template-header.mac"


;;
; 64-bit unsigned integer left shift.
;
; @returns  AX:BX:CX:DX (AX is the most significant, DX the least)
; @param    AX:BX:CX:DX Value to shift.
; @param    SI          Shift count.
;
%ifdef BS3KIT_WITH_REAL_WATCOM_INTRINSIC_NAMES
global __U8RS
__U8RS:
%endif
global $_?U8RS
$_?U8RS:
        push    si

        ;
        ; The 16-bit watcom code differs from the 32-bit one in the way it
        ; handles the shift count. All 16-bit bits are used in the 16-bit
        ; code, we do the same as the 32-bit one as we don't want to wast
        ; time in the below loop.
        ;
        ; Using 8086 comatible approach here as it's less hazzle to write
        ; and smaller.
        ;
        and     si, 3fh
        jz      .return

.next_shift:
        shr     ax, 1
        rcr     bx, 1
        rcr     cx, 1
        rcr     dx, 1
        dec     si
        jnz     .next_shift

.return:
        pop     si
%ifdef ASM_MODEL_FAR_CODE
        retf
%else
        ret
%endif


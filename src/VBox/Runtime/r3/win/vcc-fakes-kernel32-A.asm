; $Id: vcc-fakes-kernel32-A.asm 84405 2020-05-20 14:27:07Z vboxsync $
;; @file
; IPRT - Wrappers for kernel32 APIs missing in NT4 and earlier.
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


%include "vcc-fakes.mac"

%define FAKE_MODULE_NAME kernel32

BEGINDATA
GLOBALNAME vcc100_kernel32_fakes_asm

%ifdef VCC_FAKES_TARGET_VCC100
 %include "vcc-fakes-kernel32-100.h"
%elifdef VCC_FAKES_TARGET_VCC140
 %include "vcc-fakes-kernel32-141.h"
%elifdef VCC_FAKES_TARGET_VCC141
 %include "vcc-fakes-kernel32-141.h"
%elifdef VCC_FAKES_TARGET_VCC142
 %include "vcc-fakes-kernel32-141.h"
%else
 %error "PORT ME!"
%endif


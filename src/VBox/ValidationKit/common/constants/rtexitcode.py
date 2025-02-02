# -*- coding: utf-8 -*-
# $Id: rtexitcode.py 82968 2020-02-04 10:35:17Z vboxsync $

"""
RTEXITCODE from iprt/types.h.
"""

__copyright__ = \
"""
Copyright (C) 2012-2020 Oracle Corporation

This file is part of VirtualBox Open Source Edition (OSE), as
available from http://www.virtualbox.org. This file is free software;
you can redistribute it and/or modify it under the terms of the GNU
General Public License (GPL) as published by the Free Software
Foundation, in version 2 as it comes in the "COPYING" file of the
VirtualBox OSE distribution. VirtualBox OSE is distributed in the
hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.

The contents of this file may alternatively be used under the terms
of the Common Development and Distribution License Version 1.0
(CDDL) only, as it comes in the "COPYING.CDDL" file of the
VirtualBox OSE distribution, in which case the provisions of the
CDDL are applicable instead of those of the GPL.

You may elect to license modified versions of this file under the
terms and conditions of either the GPL or the CDDL or both.
"""
__version__ = "$Revision: 82968 $"


## Success.
RTEXITCODE_SUCCESS = 0;
SUCCESS = RTEXITCODE_SUCCESS;
## General failure.
RTEXITCODE_FAILURE = 1;
FAILURE = RTEXITCODE_FAILURE;
## Invalid arguments.
RTEXITCODE_SYNTAX = 2;
SYNTAX  = RTEXITCODE_SYNTAX;
##  Initialization failure.
RTEXITCODE_INIT = 3;
INIT    = RTEXITCODE_INIT;
## Test skipped.
RTEXITCODE_SKIPPED = 4;
SKIPPED = RTEXITCODE_SKIPPED;
## Bad-testbox.
RTEXITCODE_BAD_TESTBOX = 32;
## Bad-testbox.
BAD_TESTBOX = RTEXITCODE_BAD_TESTBOX;


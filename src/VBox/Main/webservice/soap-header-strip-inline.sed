# $Id: soap-header-strip-inline.sed 83223 2020-03-06 14:41:03Z vboxsync $
## @file
# WebService - SED script for stripping inlined bodies from soapH.h.
#

#
# Copyright (C) 2020 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

/^inline /,/^}/ {
    /^inline/,/^{/ {
        s/^inline/\/\*noinline\*\//
        s/^{.*/;/
        p
    }
    d
}

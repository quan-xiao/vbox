#!/usr/bin/env python
# -*- coding: utf-8 -*-
# $Id: logout.py 82968 2020-02-04 10:35:17Z vboxsync $

"""
VirtualBox Validation Kit - CGI - Log out page.
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


# Standard python imports.
import os
import sys

# Only the main script needs to modify the path.
g_ksValidationKitDir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))));
sys.path.append(g_ksValidationKitDir);

# Validation Kit imports.
from testmanager.core.webservergluecgi import WebServerGlueCgi


def main():
    """
    Main function a la C/C++. Returns exit code.
    """

    oSrvGlue = WebServerGlueCgi(g_ksValidationKitDir, fHtmlOutput = True)
    sUser = oSrvGlue.getLoginName()
    if sUser not in (oSrvGlue.ksUnknownUser, 'logout'):
        oSrvGlue.write('<p>Broken apache config!\n'
                       'The logout.py script should be configured with .htaccess-logout and require user logout!</p>')
    else:
        oSrvGlue.write('<p>Successfully logged out!</p>')
        oSrvGlue.write('<p><a href="%sadmin.py">Log in</a> under another user name.</p>' %
                       (oSrvGlue.getBaseUrl(),))


        oSrvGlue.write('<hr/><p>debug info:</p>')
        oSrvGlue.debugInfoPage()
    oSrvGlue.flush()

    return 0

if __name__ == '__main__':
    sys.exit(main())


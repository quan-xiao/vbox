#!/usr/bin/env python
# -*- coding: utf-8 -*-
# $Id: tdPython1.py 82968 2020-02-04 10:35:17Z vboxsync $

"""
VirtualBox Validation Kit - Python Bindings Test #1
"""

__copyright__ = \
"""
Copyright (C) 2010-2020 Oracle Corporation

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


# Standard Python imports.
import os
import sys
import time
import threading

# Only the main script needs to modify the path.
try:    __file__
except: __file__ = sys.argv[0];
g_ksValidationKitDir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))));
sys.path.append(g_ksValidationKitDir);

# Validation Kit imports.
from testdriver import base;
from testdriver import reporter;


class SubTstDrvPython1(base.SubTestDriverBase):
    """
    Sub-test driver for Python Bindings Test #1.
    """

    def __init__(self, oTstDrv):
        base.SubTestDriverBase.__init__(self, oTstDrv, 'python-binding', 'Python bindings');

    def testIt(self):
        """
        Execute the sub-testcase.
        """
        return  self.testEventQueueWaiting() \
            and self.testEventQueueInterrupt();

    #
    # Test execution helpers.
    #

    def testEventQueueWaitingThreadProc(self):
        """ Thread procedure for checking that waitForEvents fails when not called by the main thread. """
        try:
            rc2 = self.oTstDrv.oVBoxMgr.waitForEvents(0);
        except:
            return True;
        reporter.error('waitForEvents() returned "%s" when called on a worker thread, expected exception.' % (rc2,));
        return False;

    def testEventQueueWaiting(self):
        """
        Test event queue waiting.
        """
        reporter.testStart('waitForEvents');

        # Check return values and such.
        for cMsTimeout in (0, 1, 2, 3, 256, 1000, 0):
            iLoop = 0;
            while True:
                try:
                    rc = self.oTstDrv.oVBoxMgr.waitForEvents(cMsTimeout);
                except:
                    reporter.errorXcpt();
                    break;
                if not isinstance(rc, int):
                    reporter.error('waitForEvents returns non-integer type');
                    break;
                if rc == 1:
                    break;
                if rc != 0:
                    reporter.error('waitForEvents returns "%s", expected 0 or 1' % (rc,));
                    break;
                iLoop += 1;
                if iLoop > 10240:
                    reporter.error('waitForEvents returns 0 (success) %u times. '
                                   'Expected 1 (timeout/interrupt) after a call or two.'
                                   % (iLoop,));
                    break;
            if reporter.testErrorCount() != 0:
                break;

        # Check that we get an exception when trying to call the method from
        # a different thread.
        reporter.log('If running a debug build, you will see an ignored assertion now. Please ignore it.')
        sVBoxAssertSaved = os.environ.get('VBOX_ASSERT', 'breakpoint');
        os.environ['VBOX_ASSERT'] = 'ignore';
        oThread = threading.Thread(target=self.testEventQueueWaitingThreadProc);
        oThread.start();
        oThread.join();
        os.environ['VBOX_ASSERT'] = sVBoxAssertSaved;

        return reporter.testDone()[1] == 0;

    def interruptWaitEventsThreadProc(self):
        """ Thread procedure that's used for waking up the main thread. """
        time.sleep(2);
        try:
            rc2 = self.oTstDrv.oVBoxMgr.interruptWaitEvents();
        except:
            reporter.errorXcpt();
        else:
            if rc2 is True:
                return True;
            reporter.error('interruptWaitEvents returned "%s" when called from other thread, expected True' % (rc2,));
        return False;

    def testEventQueueInterrupt(self):
        """
        Test interrupting an event queue wait.
        """
        reporter.testStart('interruptWait');

        # interrupt ourselves first and check the return value.
        for i in range(0, 10):
            try:
                rc = self.oTstDrv.oVBoxMgr.interruptWaitEvents();
            except:
                reporter.errorXcpt();
                break;
            if rc is not True:
                reporter.error('interruptWaitEvents returned "%s" expected True' % (rc,));
                break

        if reporter.testErrorCount() == 0:
            #
            # Interrupt a waitForEvents call.
            #
            # This test ASSUMES that no other events are posted to the thread's
            # event queue once we've drained it.  Also ASSUMES the box is
            # relatively fast and not too busy because we're timing sensitive.
            #
            for i in range(0, 4):
                # Try quiesce the event queue.
                for _ in range(1, 100):
                    self.oTstDrv.oVBoxMgr.waitForEvents(0);

                # Create a thread that will interrupt us in 2 seconds.
                try:
                    oThread = threading.Thread(target=self.interruptWaitEventsThreadProc);
                    oThread.setDaemon(False);
                except:
                    reporter.errorXcpt();
                    break;

                cMsTimeout = 20000;
                if i == 2:
                    cMsTimeout = -1;
                elif i == 3:
                    cMsTimeout = -999999;

                # Do the wait.
                oThread.start();
                msNow = base.timestampMilli();
                try:
                    rc = self.oTstDrv.oVBoxMgr.waitForEvents(cMsTimeout);
                except:
                    reporter.errorXcpt();
                else:
                    msElapsed = base.timestampMilli() - msNow;

                    # Check the return code and elapsed time.
                    if not isinstance(rc, int):
                        reporter.error('waitForEvents returns non-integer type after %u ms, expected 1' % (msElapsed,));
                    elif rc != 1:
                        reporter.error('waitForEvents returned "%s" after %u ms, expected 1' % (rc, msElapsed));
                    if msElapsed > 15000:
                        reporter.error('waitForEvents after %u ms, expected just above 2-3 seconds' % (msElapsed,));
                    elif msElapsed < 100:
                        reporter.error('waitForEvents after %u ms, expected more than 100 ms.' % (msElapsed,));

                oThread.join();
                oThread = None;
                if reporter.testErrorCount() != 0:
                    break;
                reporter.log('Iteration %u was successful...' % (i + 1,));
        return reporter.testDone()[1] == 0;


if __name__ == '__main__':
    from tests.api.tdApi1 import tdApi1;
    sys.exit(tdApi1([SubTstDrvPython1]).main(sys.argv));


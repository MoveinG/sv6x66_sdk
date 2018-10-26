#!/usr/bin/python

#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

import sys
import os

sys.path.append(os.environ['PYLIB'])
from TestCaseAda import *


class TestAdaSched(TestCaseAda):
    """
    Test Cases for ADA Schedule API
    """
#    port = ''
#    report = 1

    def test_01_C610702(self):
        """C610702"""
        # test ada sched init
        TestAdaSched.ada.cmdlogdisable()
        TestAdaSched.ada.cmdappup()
        input("Set schedule sched1 in dashboard, then press <Enter>")
        time.sleep(3)
        self.updateTres()
        res = self.parseTres([0])

    def test_02_C610703(self):
        """C610703"""
        # test ada sched enable
        res = self.parseTres([0])

    def test_03_C610704(self):
        """C610704"""
        # test ada sched set name
        res = self.parseTres([0])

    def test_04_C610705(self):
        """C610705"""
        # test ada sched get index
        res = self.parseTres([0])

    def test_05_C610706(self):
        """C610706"""
        # test ada sched set index
        res = self.parseTres([0])


def main():
    unittest2.main(testRunner=xmlrunner.XMLTestRunner(output='results'))


# Run the test case
if __name__ == '__main__':
    main()

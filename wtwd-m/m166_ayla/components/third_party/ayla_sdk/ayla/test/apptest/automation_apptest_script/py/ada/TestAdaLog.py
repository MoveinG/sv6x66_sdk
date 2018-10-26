#!/usr/bin/python

#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

import sys
import os

sys.path.append(os.environ['PYLIB'])
from TestCaseAda import *


class TestAdaLog(TestCaseAda):
    """
    Test Cases for ADA LOG API
    """
#    port = ''
#    report = 1

    def test_01_C610710(self):
        """C610710"""
        # test log info
        TestAdaLog.ada.cmdlogdisable()
        TestAdaLog.ada.cmdappup()
        time.sleep(3)
        self.updateTres()
        res = self.parseTres([0])

    def test_02_C610711(self):
        """C610711"""
        # test log put
        res = self.parseTres([0])


def main():
    unittest2.main(testRunner=xmlrunner.XMLTestRunner(output='results'))


# Run the test case
if __name__ == '__main__':
    main()

#!/usr/bin/python

#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

import sys
import os

sys.path.append(os.environ['PYLIB'])
from TestCaseAda import *


class TestAdaClient(TestCaseAda):
    """
    Test Cases for ADA Client API
    """
#    port = ''
#    report = 1

    def test_01_C610701(self):
        """C610701"""
        # test client reg window start
        TestAdaClient.ada.cmdlogdisable()
        TestAdaClient.ada.cmdappup()
        time.sleep(5)
        TestAdaClient.ada.cmdappdown()
        time.sleep(5)
        TestAdaClient.ada.cmdappup()
        time.sleep(5)
        self.updateTres()
        res = self.parseTres([0])

    def test_02_C610698(self):
        """C610698"""
        # test ada init
        res = self.parseTres([0])

    def test_03_C610699(self):
        """C610699"""
        # test ada client up
        res = self.parseTres([0])

    def test_03_C610700(self):
        """C610700"""
        # test ada client down
        res = self.parseTres([0])

def main():
    unittest2.main(testRunner=xmlrunner.XMLTestRunner(output='results'))


# Run the test case
if __name__ == '__main__':
    main()

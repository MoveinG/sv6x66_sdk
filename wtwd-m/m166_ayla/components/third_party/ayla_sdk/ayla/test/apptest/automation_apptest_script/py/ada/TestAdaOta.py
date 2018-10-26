#!/usr/bin/python

#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

import sys
import os

sys.path.append(os.environ['PYLIB'])
from TestCaseAda import *


class TestAdaOta(TestCaseAda):
    """
    Test Cases for ADA OTA API
    """
#    port = ''
#    report = 1

    def test_01_C610707(self):
        """C610707"""
        # test ada ota register
        TestAdaOta.ada.cmdlogdisable()
        TestAdaOta.ada.cmdappup()
        input("Perform OTA in dashboard, then press <Enter>")
        self.logger.info('Wait 30 seconds for ota to complete...')
        time.sleep(30)
        self.updateTres()
        res = self.parseTres([1])

    def test_02_C610708(self):
        """C610708"""
        # test ada ota start
        res = self.parseTres([1])

    def test_03_C610709(self):
        """C610708"""
        # test ada ota report
        res = self.parseTres([1])


def main():
    unittest2.main(testRunner=xmlrunner.XMLTestRunner(output='results'))


# Run the test case
if __name__ == '__main__':
    main()

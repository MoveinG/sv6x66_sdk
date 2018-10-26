#!/usr/bin/python

#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

import sys
import os

sys.path.append(os.environ['PYLIB'])
from TestCaseAda import *


class TestAdaProp(TestCaseAda):
    """
    Test Cases for ADA Properties API
    """
#    port = ''
#    report = 0

#    def __init__(self, port='', report=''):
#        super(TestAdaProp, self).__init__(port, report)

    # Test Cases
    def test_01_C610678(self):
        """C610678"""
        # test sprop manager register
        TestAdaProp.ada.cmdlogdisable()
        TestAdaProp.ada.cmdappup()
        time.sleep(3)
        self.updateTres()
        res = self.parseTres([0])
     
    def test_02_C610679(self):
        """C610679"""
        # test sprop send
        TestAdaProp.ada.cmdpropsetint()
        time.sleep(1)
        self.updateTres() 
        res = self.parseTres([0, -7])
     
    def test_03_C610680(self):
        """C610680"""
        # test sprop send by name
        res = self.parseTres([0, -7])
     
    def test_04_C610681(self):
        """C610681"""
        # test sprop set bool
        TestAdaProp.ada.cmdpropsetblueled()
        time.sleep(1)
        self.updateTres() 
        res = self.parseTres([0])

    def test_05_C610682(self):
        """C610682"""
        # test sprop get bool
        TestAdaProp.ada.cmdpropgetblueled()
        time.sleep(1)
        self.updateTres() 
        res = self.parseTres([1])

    def test_05_C610683(self):
        """C610683"""
        # test sprop set int
        TestAdaProp.ada.cmdpropsetint()
        time.sleep(1)
        self.updateTres() 
        res = self.parseTres([0])

    def test_06_C610684(self):
        """C610684"""
        # test sprop get int
        TestAdaProp.ada.cmdpropgetint()
        time.sleep(1)
        self.updateTres() 
        res = self.parseTres([4])

    def test_08_C610685(self):
        """C610685"""
        # test sprop set uint
        TestAdaProp.ada.cmdpropsetuint()
        time.sleep(1)
        self.updateTres()
        res = self.parseTres([0])

    def test_09_C610686(self):
        """C610686"""
        # test sprop get uint
        TestAdaProp.ada.cmdpropgetuint()
        time.sleep(1)
        self.updateTres()
        res = self.parseTres([4])

    def test_10_C610687(self):
        """C610687"""
        # test sprop set string
        TestAdaProp.ada.cmdpropsetstring()
        time.sleep(1)
        self.updateTres()
        res = self.parseTres([0])

    def test_11_C610688(self):
        """C610688"""
        # test sprop get string
        TestAdaProp.ada.cmdpropgetstring()
        time.sleep(1)
        self.updateTres()
        res = self.parseTres(list(range(1, 1024)))

    def test_12_610689(self):
        """C610689"""
        # test sprop dest mask
        res = self.parseTres(list(range(0, 4)))

    def test_13_610690(self):
        """C610690"""
        input("Create a new datapoint for property uinput_pmgr in dashboard, then press <Enter>")
        # test prop mgr register
        res = self.parseTres([0])

    def test_14_610691(self):
        """C610691"""
        # test prop mgr ready
        res = self.parseTres([0, -7])

    def test_15_610692(self):
        """C610692"""
        # test prop mgr request
        res = self.parseTres([0])

    def test_16_610693(self):
        """C610693"""
        # test prop mgr send
        res = self.parseTres([0, -7])

    def test_17_610694(self):
        """C610694"""
        # test prop mgr send done
        res = self.parseTres([2, 3])

    def test_18_610695(self):
        """C610695"""
        # test prop mgr connect status
        res = self.parseTres(list(range(0, 4)))

def main():
#    TestAdaProp(port='', report=0)
    unittest2.main(testRunner=xmlrunner.XMLTestRunner(output='results'))

# Run the test case
if __name__ == '__main__':
    main()

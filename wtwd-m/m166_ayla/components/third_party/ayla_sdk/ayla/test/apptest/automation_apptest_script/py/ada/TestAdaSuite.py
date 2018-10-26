#!/usr/bin/python

#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

import sys
import os
import unittest2

sys.path.append(os.environ['PYLIB'])

from unittest2 import TestLoader, TextTestRunner, TestSuite
from TestAdaProp import *
from TestAdaClient import *
from TestAdaOta import *
from TestAdaSched import *
from TestAdaLog import *


def main():
    loader = unittest2.TestLoader()
    suite = unittest2.TestSuite((
        loader.loadTestsFromTestCase(TestAdaProp),
        loader.loadTestsFromTestCase(TestAdaLog),
        loader.loadTestsFromTestCase(TestAdaSched),
        loader.loadTestsFromTestCase(TestAdaClient),
        loader.loadTestsFromTestCase(TestAdaOta),
       ))

    runner = TextTestRunner(verbosity = 2)
    runner.run(suite)
    

# Run the test case
if __name__ == '__main__':
    main()

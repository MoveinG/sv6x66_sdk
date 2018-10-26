#!/usr/bin/python

#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

import unittest2
import xmlrunner
from CmdAdwQca import *
from CmdAdwWmsdk import *
from CmdAdwAmeba import *
from CmdAdwMod import *
from CmdAdwLinux import *
from Util import *

chkenv


class TestCaseAda(unittest2.TestCase):
    """
    Base ADA Test Class
    """
    port = '/dev/ttyUSB0'
    # apptest program is located in local
    # if automation script interacte with device via serial port, please set localCmd to ''
    localCmd = 'apptest'
    tres = {}
    tlist = []
    plat = ''
    proj = 'device_ada'
    cmdSpecialExec = {}


    @classmethod
    def setUpClass(cls):
        """ SetUp Class"""
        cls.suite = cls.__name__
        cls.logger = loggerinit('TestCaseAda')
        cls.logger.info("{0}: setup".format(cls.suite))
        try:
            cls.ada
        except AttributeError:
            cls.ada = CmdAda(cls.suite, cls.port, localCmd=cls.localCmd)
            cls.plat = prompt2plat(cls.ada.getprompt(), port=cls.port)
            cls.ada = None
            #cls.plat = 'linux'
            cls.logger.info("Platform = {0}".format(cls.plat))
            cls.logger.info("wait 3 seconds before switching to platform")
            time.sleep(3)
            if cls.plat == 'qca4010':
                cls.ada = CmdAdwQca(cls.suite, cls.port, localCmd=cls.localCmd)
            elif cls.plat == 'wmsdk':
                cls.ada = CmdAdwWmsdk(cls.suite, cls.port, localCmd=cls.localCmd)
            elif cls.plat == 'ameba':
                cls.ada = CmdAdwAmeba(cls.suite, cls.port, localCmd=cls.localCmd)
            elif cls.plat == 'module':
                cls.ada = CmdAdwMod(cls.suite, cls.port, localCmd=cls.localCmd)
                cls.proj = 'device'
            elif cls.plat == 'linux':
                cls.ada = CmdAdwLinux(cls.suite, cls.port, localCmd=cls.localCmd)
            else:
                self.logger.error('Unknown platform {0}'.format(cls.plat))
                sys.exit(0)
        cls.ada.dcls['log'].makedirs()
        cls.tres = {}
        cls.tlist = []
        cls.envIsClean()


    @classmethod
    def tearDownClass(cls):
        """ TearDown Class"""
        cls.logger.info("{0}: teardown".format(cls.suite))
        dp = cls.ada.getsys()
        dp['suite'] = cls.suite
        for e in ['tr', 'txt']:
            cls.ada.dcls['log'].writelog(e, cls.tlist, dp)

# check test environment
    @classmethod
    def envIsClean(cls):
        pass

# preparing to test
    def setUp(self):
        """Setting up for the test"""
        test = self.shortDescription()
#
        self.tres[test] = self.ada.getsys(test, name='res')
        print()
##        self.logger.info(">>> {0}".format(test))
        name = test
        dt = {'name': test, 'descr': name, 'exp': None, 'got': None, 'res': None, 'tres': None, 'elapsed': None,
              'comment': None}
        self.tlist.append(dt)
        self.startTime = time.time()
        self.logger.info(">>> {0}: {1}".format(test, name))

# ending the test
    def tearDown(self):
        """Cleaning up after the test"""
        test = self.shortDescription()
        self.logger.info("<<< {0}".format(test))


#
# Test Cases
#

    def updateTres(self):
        name = self.shortDescription()
        self.tres[name] = self.ada.getsys(name, refresh=1, name='res')
        self.logger.info('update tres {0} : {1}'.format(name, self.tres[name]))

    def parseTres(self, exp):
        name = self.shortDescription()
#
        d = self.tres[name]
        got = d['res']
        res = d['verb']
#
        self.tlist[-1]['res'] = res
        self.tlist[-1]['exp'] = exp
        self.tlist[-1]['got'] = got
        self.tlist[-1]['tres'] = 'SKIPPED'
        self.tlist[-1]['elapsed'] = sec2hms(time.time() - self.startTime, valid=True)
        self.tlist[-1]['comment'] = "Expected: <{0}>, got: <{1}>".format(exp, got)

        self.logger.info("{0} was executed {1} times, last result {2} ({3}) : {4}".format(name, self.tres[name]['cnt'],
                                       got, exp, self.tlist[-1]['elapsed']))

        assert self.tlist[-1]['name'] == name, "{0} doesn't match {1}".format(self.tlist[-1]['name'], name)
        if got == '-99':
            self.tlist[-1]['tres'] = 'SKIPPED'
            self.skipTest("{0} not executed".format(name))

        try:
            self.assertIn(str(got), str(exp), '{0}'.format(name))
            self.tlist[-1]['tres'] = 'PASSED'
        except:
            self.tlist[-1]['tres'] = 'FAILED'
            self.assertIn(str(got), str(exp), '{0}'.format(name))

        return res


def main():
    unittest2.main(testRunner=xmlrunner.XMLTestRunner(output='results'))


# Run the test case
if __name__ == '__main__':
    main()

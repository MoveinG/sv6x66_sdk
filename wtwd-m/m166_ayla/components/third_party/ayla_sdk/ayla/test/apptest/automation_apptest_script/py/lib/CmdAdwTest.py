#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

from CmdAdw import *

class CmdAdwTest(CmdAdw):
    """
    Provides methods to ADW Test CLI commands
    """

    def __init__(self, suite, port='', prompt='', localCmd=''):
        """
        Constructor configures serial port, change prompt for a platform
        """
        CmdAdw.__init__(self, suite, port, prompt, localCmd)

    def getcmd(self):
        """
        Dictionary of core ADW and system commands
        """
        cmds = {
            'show version': '',
            'test-show sys': '',
            'test-show prop': '',
            'test-show res': '',
            }
        return cmds

    def getcmdExt(self):
        """
        Get the list of additional ADA commands supported
        """
        cmds = {
            'conf show': '',
            }
        return cmds

    # Test reset

    def testreset(self, verb=False):
        """
        Test wrapper for reset
        """
        mlvl = [
            ['test','debug','ada_init 0'],
            ['test','debug','ada_sched_enable 0'],
            ['test','debug','ada_ota_register 1'],
            ['test','debug','adw_wifi_init 0'],
            ['test','debug','adw_wifi_page_init 1'],
            ['test','debug','adw_wifi_ap_ssid_set 0'],
            ['test','debug','adw_wifi_ios_setup_app_set 0'],
            ['test','debug','adw_wifi_cli 0'],
            ['test','debug','ada_prop_mgr_register 0'],
            ['test','debug','ada_sprop_mgr_register 0'],
            ['test','debug','ada_prop_mgr_ready 0'],
        ]
        rl = self.sendcmdlog('reset', mlvl, tout=None, pers=True, exc=False, verb=verb)
        res = [r['msg'].split()[1] for r in rl]
        return [0,res,res]

     # <test-show> commands

    def wtshowprop(self, verb=False, valid=True, cmd='test-show prop'):
        """
        Execute <test-show prop> command
        """
        kde = {
            'Blue_LED': '1',
            'Blue_button': '0',
            'Green_LED': '1',
            'cmd': 'ert',
            'decimal_in': '34500',
            'decimal_out': '34500',
            'input': '432',
            'log': 'ert',
            'oem_host_version': '1.3.5',
            'output': '432',
            'uinput': '1',
            'uoutput': '1',
            'version': '12:49:14',
            }
        rl = self.cmd2list(cmd,verb=verb)
        rd = {l.split()[0]:l.split()[-1] for l in rl}
        if valid:
            assert sorted(kde) == sorted(rd), 'Expected keys <{0}> got <{1}>'.format(sorted(kde),sorted(rd))
        else:
            rd = [int(sorted(kde)!=sorted(rd)),sorted(kde),sorted(rd)]
        return rd

    def testshowprop(self, verb=False):
        """
        Test wrapper for<test-show prop> command
        """
        rl = self.wtshowprop(verb=verb,valid=False)
        return rl

    def testpropshow(self, verb=False, cmd='test-prop show'):
        """
        Test wrapper for<test-prop show> command
        """
        el = []
        rl = []
        cd = {}
        rd = self.wtshowprop(verb=verb)
        for name in sorted(rd.keys()):
            cmd1= cmd + ' ' + name
            cl = self.cmd2list(cmd1,verb=verb)
            name,junk,val = cl[0].split()
            cd[name] = val
        for name in sorted(rd):
            if rd[name] != cd[name]:
                rl.append([name,rd[name],cd[name]])
        return [int(el != rl),el,rl]

    def wtshowsys(self, verb=False, valid=True, cmd='test-show sys'):
        """
        Execute <test-show sys> command
        """
        kde = {
            'model': 'AY008MVL1',
            'serial': 'AC000W000432405',
            'oem': '0dfc7900',
            'oem_model': 'ledevb',
            'version': 'ADA 1.3.5 wmsdk-3.4 2017-11-29 11:22:53 a137c69',
            'version_demo': '1.3.5',
            }
        rl = self.cmd2list(cmd,verb=verb)
        rd = {l.split()[0]:l.split()[-1] for l in rl}
        if valid:
            assert sorted(kde) == sorted(rd), 'Expected keys <{0}> got <{1}>'.format(sorted(kde),sorted(rd))     
        else:
            rd = [int(sorted(kde)!=sorted(rd)),sorted(kde),sorted(rd)]
        return rd

    def testshowsys(self, verb=False):
        """
        Test wrapper for<test-show sys> command
        """
        rl = self.wtshowsys(verb=verb,valid=False)
        return rl

    def testwtlog(self, verb=False, cmd='log'):
        """
        Test wrapper for <log> command
        """
        rl = super(CmdAdwTest, self).testlog()
        mlvl = [
            ['test','debug','ada_log_cli 0'],
        ]
        rl1 = self.sendcmdlog(cmd=cmd, mlvl=mlvl, verb=verb)
        res = [r['msg'].split()[1] for r in rl1]
        return rl

def main():
    cmd = CmdAdwTest('TestCli', port='/dev/ttyUSB1')
    pprint(cmd.testwtapcond())

if __name__ == '__main__':
    main()

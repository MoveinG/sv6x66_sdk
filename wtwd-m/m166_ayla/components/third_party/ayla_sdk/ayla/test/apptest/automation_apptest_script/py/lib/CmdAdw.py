#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

from CmdAda import *

class CmdAdw(CmdAda):
    """
    Provides methods to ADW commands
    """

    def __init__(self, suite, port='', prompt='', localCmd=''):
        """
        Constructor configures serial port, change prompt for a platform
        """
        CmdAda.__init__(self, suite, port, prompt, localCmd=localCmd)
        self.data['wa'] = {}
        self.data['wa']['rd'] = {}

    def getcmd(self):
        """
        Dictionary of core ADW and system commands
        """
        cmds = {
            'show version': '',
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


def main():
    cmd = CmdAdw('TestCli', port='/dev/ttyUSB2')
#    showdict(cmd.getwifi())
#    pprint(cmd.getprofs())
#    showdict(cmd.parsewifi()['prop'])
#    cmd.wifiwa(sec=0,refresh=1)
#    cmd.wifiwa(sec=1)


if __name__ == '__main__':
    main()

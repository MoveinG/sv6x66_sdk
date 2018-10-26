#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

from CmdAdwTest import *

class CmdAdwAmeba(CmdAdwTest):
    """
    Provides methods to AMEBA ADA serial commands
    """
    data = {}
    dcls = {}

    def __init__(self, suite, port='', localCmd=''):
        """
        Constructor configures serial port, change prompt for a platform
        """
        CmdAdw.__init__(self, suite, port, plat2prompt('ameba'), localCmd)

    def getcmdExt(self):
        cmds = {
            'show conf': '',
            'help': '',
            }
        return cmds

    def parseconf(self, refresh=0, bufidx=None, cname='conf show'):
        """
        Convert the response to <conf show> command into dict
        """
        sd = {}
        bd = 0
        t2s = {'f':'factory','s':'startup','r':'running'}
        if refresh:
            self.cmd2list(cname)
        for s in self.data['buf'][cname]:
            if s.split(' ')[0] in ['r','f','s']:
                ctype = s.split(' ')[0]
                key = s.split(' ')[1]
                val = s.split(' ')[3]
                d = {'val':val, 'type':t2s[ctype], 'str':s.strip()}
                sd[key] = d           
            else:
                self.logger.warn("Filtering out <{0}>".format(s))
        return sd

    def getconfold(self, key='', refresh=0, name='conf show'):
        """
        Get the value of psm-dump keys
        """
        sd = self.parsepsm(refresh)
        try:
            if key != '':
                rv = sd[key]
            else:
                rv = sd
        except KeyError:
            print("Unknown <{0}> name <{1}> in {2}".format(name, key, self.parsepsm(refresh).keys()))
            sys.exit(1)
        return rv

    def validres(self, cmd, res):
        """
        Validate a command's response
        """
        if res != cmd:
            if cmd != 'pm-reboot' or res != '':
                print("Expected: {0}, got: {1}, help: {2}".format(cmd, res, h))
                sys.exit(1)
        if cmd == 'pm-reboot':
            sec = 1
            print("Executing {0}, waiting for {1} seconds".format(self.data['cmd'], sec))
            time.sleep(sec)
            print("Resetting serial connection")
            self.reset()
            tr = self.gettrace('starting HTTP server')
            self.data['ip'] = self.util.findip(tr)[0]
            print("Assigned IP address {0}".format(self.data['ip']))
        return cmd


def main():
    port = '/dev/ttyACM1'
    cmd = CmdAdwWmsdk('TestCli', port=port)
    showdict(cmd.parseconf())
    cmd.showsys()
#    cmd.showsys()
#    showdict(cmd.getpsm(refresh=1))
#    print()
#    showdict(cmd.gethelp())
#    print()
#  print cmd.cfgwifi('Ayla-test',reboot=1)
#  print ""

if __name__ == '__main__':
    main()

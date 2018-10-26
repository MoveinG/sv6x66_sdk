#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

from CmdAdwTest import *

class CmdAdwWmsdk(CmdAdwTest):
    """
    Provides methods to WMSDK ADA serial commands
    """
    data = {}
    dcls = {}

    def __init__(self, suite, port='', localCmd=''):
        """
        Constructor configures serial port, change prompt for a platform
        """
        CmdAdw.__init__(self, suite, port, plat2prompt('wmsdk'), localCmd)

    def getcmdExt(self):
        cmds = {
            'psm-dump': '',
            'help': '',
            }
        return cmds

    def parseconf(self, refresh=0, bufidx=None, cname='show conf'):
        return self.parsepsm(refresh=refresh)

    def parsepsm(self, refresh=0, cname='psm-dump', verb=False):
        """
        Convert the response to psm-dump command into dict
        """
        sd = {}
        bd = 0
        t2s = {'f':'factory','s':'startup','r':'running'}
        if refresh:
            self.cmd2list(cname)
        for s in self.data['buf'][cname]:
            if s.split('.')[0] != 'ada':
                if verb:
                    self.logger.warn("Filtering out <{0}>".format(s))
            else:
                cnt = s.count('=')
                if cnt == 1:
                    if bd:
                        sd[key.strip()] = [i for i in sd[key.strip()].split()[2:]]
                    bd = s.count('<binary data>')
                    try:
                        key,val = s.split('=')
                    except Exception as e:
                        if verb:
                            estr = 'String <{0}> not valid: <{1}>'.format(s,e)
                            self.logger.warn(estr)
                elif cnt == 0:
                    val = val + ' ' + s.strip()
                else:
                    i = s.index('=')
                    key = s[:i]
                    val = s[i+1:]
                key = key.strip()
                try:
                    mod,ctype,key = key.split('.')
                    val = val.strip()
                    vl = val.split()
                    if vl[0] == '(hex)':
                        val = ' '.join(vl[1:])
                    if vl[-1] == 'bytes)':
                        val = ' '.join(vl[:-2])
                    d = {'val':val, 'type':t2s[ctype], 'str':s}
                    sd[key] = d
                except Exception as e:
                    if verb:
                        self.logger.warn("Skipping obsolete entry {0}".format(key))
        if bd:
            sd[key] = [i for i in sd[key].split()[2:]]
        return sd

    def getpsm(self, key='', refresh=0, name='psm-dump'):
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
            estr = 'Unknown <{0}> name <{1}> in {2}'.format(name, key, self.parsepsm(refresh).keys())
            self.adaexcept(estr)
        return rv

    def parsehelp(self, refresh=0, name='help'):
        """
        Convert the response to help command into dict
        """
        sd = {}
        cname = name
        if refresh:
            self.cmd2list(cname)
        for s in self.data['buf'][cname]:
            key = s.split()[0]
            val = s
            sd[key] = val
        return sd

    def gethelp(self, key='', refresh=0, name='help'):
        """
        Get the value of help keys
        """
        sd = self.parsehelp(refresh)
        try:
            if key != '':
                rv = sd[key]
            else:
                rv = sd
        except KeyError:
            estr = 'Unknown <{0}> name <{1}> in {2}'.format(name, key, self.parsepsm(refresh).keys())
            self.adaexcept(estr)
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
    port = '/dev/ttyUSB2'
    cmd = CmdAdwWmsdk('TestCli', port=port)
#    cmd.showsys()
    showdict(cmd.getpsm(refresh=1))
#    print()
#    showdict(cmd.gethelp())
#    print()
#  print cmd.cfgwifi('Ayla-test',reboot=1)
#  print ""

if __name__ == '__main__':
    main()

#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

from Serial import *
import pexpect
import sys
import time

class Pexpect(Serial):
    """
    Provides methods to handle low level serial communications
    """
    dpex = {}

    def __init__(self, port='', brate='115200', prompt='', eol='\r', localCmd=''):
        """
        Constructor configures serial port
        """
        self.logger = loggerinit(self.__class__.__name__)
        if localCmd == '':
            Serial.__init__(self, port, brate)
            self.port = port
            self.localcmd = ''
            self.child = self.spawnfd(self.getfd())
        else:
            self.localcmd = localCmd
            self.port = ''
            self.child = pexpect.spawn(localCmd, timeout=20, maxread=2000)
        self.dpex['eol'] = eol
        if prompt == '':
            self.setprompt(self.findprompt(prompt))
        else:
            self.findprompt('')
            self.setprompt(prompt)
        self.logger.info("Prompt: <{0}>".format(self.getprompt()))
        self.sendcmd('')

    def spawnfd(self, fd):
        child = pexpect.fdpexpect.fdspawn(fd, timeout=20, maxread=2000)
        return child

    def convb2str(self, data=b'', text=''):
        try:
            text = data.decode()
        except Exception as e:
            self.logger.warn("Invalid utf8 character, trying <latin1>: ({0})".format(e))
            text = data.decode('latin1')
#       self.logger.warn('Pexpect buffer {0}'.format(text))
        return text

    def findprompt(self, prompt='', step=3, ival=9):
        if prompt == '':
            for i in range(0, ival, step):
                self.child.send(self.dpex['eol'])
                try:
                    self.child.expect('@#%>', timeout=2)
                except Exception as e:
#                    self.logger.info("Expect buffer: <{0}>".format(self.child.before))
                    pass
                prompt = self.convb2str(self.child.before)
                self.logger.info(prompt)
                self.child.expect(r'.*')
                prompt = prompt.replace('\n', '')
                prompt = prompt.replace('\r', '')
##                prompt = prompt.rstrip()
                if (len(prompt) < 24) and (len(prompt) > 0):
                    break
                time.sleep(step)
        if (len(prompt) >= 24) or (len(prompt) == 0):
            self.logger.error("Prompt was not discovered in {0} secs".format(ival))
            sys.exit(0)
        self.logger.info("find prompt return {0}".format(prompt))
        return prompt

    def getprompt(self):
        return self.dpex['prompt']

    def setprompt(self, prompt):
        self.dpex['prompt'] = prompt
        return self.getprompt()

    def sendcmd(self, cmd, prompt='', verb=True, exc=True):
        if prompt == '':
            prompt = self.getprompt().replace('(','\(').replace(')','\)')
        if verb:
            if cmd != "":
                self.logger.info("Executing cmd <{0}>".format(cmd))
        if cmd in ['conf show']:
            sec = 3
            self.logger.info('Timeout {1} sec after <{0}>'.format(cmd,sec))
            time.sleep(sec)
        self.child.send(cmd + self.dpex['eol'])
        #time.sleep(3)
        try:
            if 'ACM' in self.port:
                self.child.expect(self.dpex['eol'] + prompt)
            else:
                self.child.expect('\r\n' + prompt)
        except Exception as e:
            if exc:
                if verb:
                    self.logger.error(estr)
                raise Exception(estr)
            else:
                self.logger.info("Expect exception: {0}>".format(e))
        buf = self.child.before
        resp = self.convb2str(buf)
        #self.logger.info("excuting cmd {0} return {1}".format(cmd,resp))
        return resp

    def gettrace(self, msg):
        self.child.expect(msg)
        return self.convb2str(self.child.before)

    def pclose(self):
        self.logger.info("Closing spawned process")
        try:
            self.child.close()
        except Exception as e:
            print('close excpetion')
            print(e)

    def __del__(self):
        try:
            self.pclose()
        except Exception as e:
            print('pclose excpetion')
            print(e)


def main():
    #pex = Pexpect(port='/dev/ttyUSB0')
    pex = Pexpect(localCmd='apptest')
    cmdl = ['', 'show version', 'conf show']
    cmdl = ['test-show sys', 'test-show res']
    for cmd in cmdl:
        print(pex.sendcmd(cmd))
    
if __name__ == '__main__':
    main()

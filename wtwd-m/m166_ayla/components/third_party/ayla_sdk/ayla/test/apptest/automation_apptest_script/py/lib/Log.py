#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

import time
from Util import *

class Log:
    """
    Provides methods logging the regression results
    """
    def __init__(self, ver, model, suite):
        """
        Constructor validates LOGSHOME
        """
        self.dext = {'log': '.log', 'tr': '.tr', 'txt': '.txt', 'rlog': '.txt', 'xml': '.xml'}
        self.denv = {'logs': 'LOGSHOME', 'grive': 'GRIVELOGS'}
        self.path = {}
        self.update(ver, model, suite)

    def update(self, ver, model, suite, refresh=1):
        self.model = model
        self.ver = self.convVer(ver)
        self.suite = suite
        self.validenv()
        self.settime(refresh)

    def getmstone(self):
        return self.ver + '_' + self.model

    def validenv(self):
        """
        Validate rlog and grive environment settings, rlog should be checked first
        """
        for d in reversed(sorted(self.denv)):
            for e in self.dext:
                if e != 'rlog' or d != 'grive':
                    try:
                        if e not in self.path:
                            self.path[e] = {}
                        self.path[e][d] = os.environ[self.denv[d]]
#                        print("Logs dir: {0}".format(self.home[e[1]]))
                    except KeyError:
                        print("Please set environment variable {0}".format(self.denv[d]))
                        sys.exit(1)
                    if e == 'rlog':
                        dir = 'Regression'
                    else:
                        dir = self.getmstone()
                    self.path[e][d] += '/' + dir
                    self.path[e]['fname'] = None
                else:
                    self.path[e][d] = None

    def updfnames(self):
        for d in self.denv:
            for e in self.dext:
                if e == 'rlog':
                    self.path[e]['fname'] = 'rlog' + '_' + self.suite + '_' + self.getstamp(e) + self.dext[e]
                else:
                    self.path[e]['fname'] = self.suite + '_' + self.model + '_' + self.ver.replace('.','-') + '_' + \
                                                self.getstamp(e) + self.dext[e]        

    def settime(self, refresh=1):
        if refresh:
            self.time = int(time.time())
        self.updfnames()
        return self.time

    def getstamp(self, ext=''):
        if ext == 'rlog':
            self.tstamp = str(self.time)
        else:
            self.tstamp = time.strftime("%Y%m%d%H%M%S", time.localtime(self.time))
        return self.tstamp

    def getdir(self, ext, dirs = '', make=0):
        dl = []
        if dirs == '':
            dirs = self.denv.keys()
        for d in dirs:
            dp = self.path[ext][d]
            dl.append(dp)
            if make:
                if dp is not None:
                    try:
                        os.makedirs(dp)
                        print("Created directory <{0}>".format(dp))
                    except OSError:
                        pass
        return dl

    def makedirs(self, ext='', dirs=''):
        dl = []
        if ext == '':
            ext = self.dext.keys()
        for e in ext:
            dl += self.getdir(e, dirs, make=1)
        return dl
        
    def getfname(self, ext, dirs):
        return self.path[ext][dirs] + '/' + self.path[ext]['fname']

    def gettr(self):
        return self.path['tr']['fname'].split('.')[0]
        
    def gettc(self, dres):
        v2r = {'PASSED': 1, 'FAILED': 5, 'ERROR': 6, 'SKIPPED': 7}
        trl = []
        for d in dres:
            try:
                tr = '{"case_id":%s, "status_id":%s, "elapsed":\"%s\", "comment":\"%s\"}' % (d['name'].strip('C'),
                    	v2r[d['tres']], d['elapsed'], d['comment'])
                trl.append(tr)
            except Exception as err:
                print(err)
        return trl

    def getres(self, dres, dp):
        rl = ['completed', 'passed', 'failed', 'error', 'skipped']
        l = []
        tc = dict()
        for i in range(0, 4):
            tc[rl[i]] = []
            for d in dres:
                if rl[i] == 'completed':
                    tc[rl[i]].append(d['name'])
                else:
                    if d['tres'] == rl[i].upper():
                        tc[rl[i]].append(d['name'])
        l.append("{0} Test Results for <{1}> <{2}>".format(dp['suite'], dp['model'], dp['version']))
        l.append("")
        l.append('*'*30)
        l.append('*          Summary           *')
        l.append('*'*30)
        for i, j in ['Model', 'model'], ['DSN', 'serial'], ['Version', 'version'], ['Build', 'version']:
            l.append("{0:10s}: {1}".format(i, dp[j]))
        l.append('')
        for i in range(4):
            l.append("{0:22s}: {1:d}".format('Test cases ' + rl[i], len(tc[rl[i]])))
        l.append('')
        for i in range(1, 4):
            srl = dict()
            srl[i] = ''
            for j in tc[rl[i]]:
                srl[i] += "{{{0}}} ".format(j)
            l.append('{0} cases:'.format(rl[i].capitalize()))
            l.append('')
            l.append('>>>>>{0}<<<<<'.format(srl[i].strip()))
            l.append('')
        for d in dres:
            try:
                l.append(' {0} {2} '.format(d['name'], d['descr'], d['tres'].lower()).center(45, '='))
                l.append(' {1} '.format(d['name'], d['descr'], d['tres'].lower()).center(45, '='))
            except AttributeError:
                l.append(' {0} {2} '.format(d['name'], d['descr'], d['tres']).center(45, '='))
                l.append(' {1} '.format(d['name'], d['descr'], d['tres']).center(45, '='))
            l.append('Expected: {0}'.format(d['exp']))
            l.append('Received: {0}'.format(d['got']))
            l.append('')
        return l

    def writelog(self, ext, dres, dp=None, dirs=''):
        if dirs == '':
            dirs = self.denv.keys()
        for d in dirs:
            if ext == 'tr':
                rl = self.gettc(dres)
            elif ext == 'txt':
                rl = self.getres(dres, dp)
            else:
                raise LogError('Unsupported extension {0}'.format(ext))
            fn = self.getfname(ext, d)
            print("Saving <{0}> {1}: {2}".format(ext, d, fn))
            f = open(fn, 'w')
            for t in rl:
                f.write("{0}\n".format(str(t)))
            f.close()

    def convVer(self, vers):
        '''
        Converts version according to the convension
        '''
        ver = vers.replace('.','-').split('-')
        if 'ADA' in ver:
            nl = [c for c in ver if c.isdigit()]
            cl = [c.lower() for c in ver if not c.isdigit()]
            ver = '.'.join(nl) + '-' + '-'.join(cl)
        else:
            vl = vers.split()
            h = hash2bld(vl[-1].split('/')[-1])
            if vl[0][0].isdigit():
                ver = vl[0]
            else:
                ver = vl[1] + '-' + vl[0]
            ver = ver + '-' +  h
        return ver


def main():
    log = Log('ADA-WMSDK-0.5-eng', 'XXX', 'TestCli')
    log = Log('bci 2.4.5-beta 04/22/16 09:35:36 ID 2123200', 'XXX', 'TestCli')
    log.settime()
    for e in 'log', 'txt', 'tr', 'rlog', 'xml':
        print("%s:" % (e))
        print(log.getfname(e,'logs'))
        if e != 'rlog':
            print(log.getfname(e,'grive'))
        print(log.getdir(e))
    print()
    dp = {'model': 'AY001','version': 'ADA-WMSDK-0.5-eng',\
         'serial': '123456','suite': 'TestAdaProp'}
    dp = {'model': 'AY001','version': 'bci 2.4.5-beta 04/22/16 09:35:36 ID 2123200',\
         'serial': '123456','suite': 'TestAdaProp'}
    lres = [
        ['C287917', 'ada_sprop_send', [0, -7], -7, '<in progress>',  'PASSED'],
        ['C287834', 'ada_sprop_set_bool', [0], 0, '<none>', 'PASSED'],
        ]
    l = log.getres(lres, dp)
    for i in l:
        print(i)

    l = log.gettc(lres)
    for i in l:
        print(i)

    print(log.gettr())

#    for e in 'tr','txt':
#        log.writelog(e, lres, dp)
     
if __name__ == '__main__':
    main()

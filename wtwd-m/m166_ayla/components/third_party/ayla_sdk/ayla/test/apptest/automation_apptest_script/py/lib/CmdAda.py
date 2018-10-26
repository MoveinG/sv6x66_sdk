#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

import dis
import time
from Pexpect import *

class CmdAda(Pexpect):
    """
    Base class for ADA CLI commands
    """
    data = {}
    data['cfgdef'] = {
        'server/security/enable':'0',
    }
    dcls = {}

    def __init__(self, suite, port='', prompt='', warn=False, localCmd=''):
        """
        Constructor: configure serial port, execute all commands
        """
        Pexpect.__init__(self, port, prompt=prompt, localCmd=localCmd)
        self.data['suite'] = suite
        self.data['buf'] = self.getcmdAll()
        self.data['bufall'] = {}
        self.data['log'] = {}
        for c in sorted(self.data['buf']):
            try:
                self.cmd2list(c)
            except:
                if warn:
                    self.logger.warn("<{0}> ADA command failed".format(c))
            #self.logger.info("wait 3 seconds after executing ADA command {0}".format(c))
            #time.sleep(3)

    # ADA commands

    def getcmd(self):
        """
        Dictionary of base class commands
        """
        cmds = {
            'show version': '',
            }
        return cmds

    def getcmdExt(self):
        """
        Dictionary of extended commands
        """
        cmds = {
            'conf show': '',
        }
        return cmds

    def getcmdAll(self):
        """
        Dictionary of base and extended commands
        """
        cd = self.getcmd()
        cd.update(self.getcmdExt())
        return cd
 
    def res2list(self, buf, fltr=None):
        """
        Convert read buffer to the list of lines
        """
        rl = []
        for r in buf.splitlines():
            if r.strip():
                if fltr and re.search(fltr,r):
                    pass
                else:
                    rl.append(r.strip())    
        return rl

    def cmd2list(self, cmd='test-show sys', prompt='', red={}, verb=True, verb1=True):
        """
        Execute the command, convert response to lines and create log names
        """
        if red is {}:
            red = {
                'log': '\[ada\]',
                'socket': 'SOCK_|ip_sock_|tcp_sock_',
                'demo': 'demo: event',
                'QCA': 'dns_query_do|gethostbyname|ads_demo_connect_sts|ni_flags|noise_floor|Disconnected from',
                }
        self.data['buf'][cmd] = []
        self.data['log'][cmd] = []
        c2l = [c.replace('"', '') for c in self.res2list(self.sendcmd(cmd,prompt)) if c != cmd]
        for c in c2l:
            for name in red:
                if re.compile(red[name]).search(c):
                    if verb:
                        self.logger.warn("Filtered out {0} data <{1}>".format(name,c))
                    c = None
            if c:
                self.data['buf'][cmd].append(c)
        try:
            self.data['bufall'][cmd].append(self.data['buf'][cmd])
        except:
            self.data['bufall'][cmd] = [self.data['buf'][cmd]]
        if verb1:
            pprint(self.data['buf'][cmd])

        res = self.cmdproc(cmd)

        return self.data['buf'][cmd]

    def cmdproc(self, cmd, verb=True):
        """
        Processing executed command
        """
        if cmd == 'test-show sys':
            self.logger.info("processing cmd test-show sys")
            try:
                self.dcls['log'].update(self.getsys('version', '-'), self.getsys('model'), suite=self.data['suite'], refresh=0)
            except KeyError:
                try:
                    self.dcls['log'] = Log(self.getsys('version', '-'), self.getsys('model'), suite=self.data['suite'])
                except Exception as e:
#                   self.logger.info("Exception at cmd2list <{0}>: {1}".format(cmd, e))
                    self.adaexcept(e)

    def adacmd(self, cmd):
        """
        Execute ADA command
        """
        cmds = self.getcmdAll().keys()
        if not (cmd in cmds):
            estr = "ADA command {0} not found in {1}".format(cmd, cmds)
            self.adaexcept(estr)
        self.logger("Executing ada cmd {0}".format(cmd))
        res = self.sendcmd(cmd).strip()
        self.data['cmd'] = self.validres(cmd, res)

    def validres(self, cmd, res):
        """
        Validate ADA command's response
        """
        if res != cmd:
            if res != '':
                estr = "Expected: {0}, got: {1}".format(cmd, res)
                self.adaexcept(estr)
        return cmd

    def adaexcept(self, estr, verb=True):
        """
        ADA exception handler
        """
        if verb:
            self.logger.error(estr)
        raise Exception(estr)

    def adatest(self, func, **kwargs):
        """
        ADA test wrapper
        """
        try:
            got = func(**kwargs)
            exp = got
        except AssertionError as e:
            pprint('>>>')
            pprint(e)
            pprint('>>>')
            l = e.args[0].split()
            d = {'Expected':None,'got':None}
            for n in d:
                d[n] = l.index(n)
            exp = ' '.join(l[1:d['got']])
            got = ' '.join(l[d['got']+1:])
        return [int(exp!=got),exp,got]

    # Show version

    def showver(self, refresh=0, valid=True, verb=False, cmd='show version'):
        """
        Parser <show version>
        """
        if refresh:
            self.cmd2list(cmd)
        cl = self.data['buf'][cmd]
        nvl = cl[0].split()
        cd = {'cmd':cmd,'name':nvl[0],'ver':nvl[1]}
        if valid:
            exp = 'ADA'
            assert exp == cd['name'].split('-')[0],'Expected {0} got {1}'.format(exp,cd['name'])
        return cd

    def testshowver(self, verb=False):
        """
        Test wrapper for <show version>
        """
        return self.adatest(self.showver,verb=verb,valid=True)

    # System properties

    def parsesys(self, refresh=0, name='sys', cmd='test-show'):
        """
        Convert the response to show sys, prop, res commands into dict
        """
        sd = {}
        cname = cmd + ' ' + name
        if refresh:
            self.cmd2list(cname)
        #self.logger.info('parse sys cname {0}, buffer {1}', cname, self.data['buf'][cname])
        for s in self.data['buf'][cname]:
            if name == 'res':
                l = s.split()
                if l[0] in ['[MEM]']:
                    pass
                else:
                    sd[l[0]] = {'name': l[1:][0], 'res': l[1:][2], 'cnt': l[1:][4].strip(')'), 'verb': " ".join(l[1:][5:])}
            else:
                try:
                    key, val = s.split('=')
                    sd[key.strip()] = val.strip()
                except:
                    pass
        return sd

    def getsys(self, key='', sep='', refresh=0, name='sys'):
        """
        Get the value of sys, prop, res keys
        """
        sd = self.parsesys(refresh, name)
        #self.logger.info('get sys refresh {0}, key {1}, parse return {2}'.format(refresh, key, sd))
        try:
            if key != '':
                rv = sd[key]
                if sep != '':
                    if key == 'version':
                        verlist  = rv.split()[0:2]
                        verlist.append(rv.split()[2].replace('-',':').replace('.',':').replace('_',':'))
                        vhash = rv.split()[-1]
                        vhash = vhash.split('/')[-1]
                        verlist.append(hash2bld(vhash, rv.split()[1]))
                        rv = sep.join(verlist)
                    else:
                        rv = sep.join(rv.split())
            else:
                rv = sd
        except KeyError:
            estr = "Unknown <{0}> name <{1}> in {2}".format(name, key, self.parsesys(refresh, name).keys())
            self.adaexcept(estr)
        return rv

    def getver(self, num=None, refresh=0, brief=None):
        """
        Get firmware version
        """
        versys = self.getsys('version',refresh=refresh)
        ver = versys
        if num is not None:
            match = re.search(r'[0-9]+(\.[0-9]+)+',ver)
            if match:
                ver = match.group()
            else:
                assert False,"Couldn't extract numeric version from {0}".format(ver)
            match = re.search(r'ADA',versys)
            if match:
                ver = 'ADA ' + ver
        return ver

    def ismodel(self, modlist=None):
        """
        Model belongs to the list
        """
        model = self.getsys('model')
        if modlist is None:
            pass
        else:
            if model in modlist:
                model = True
            else:
                model = False
        return model

    def ismvl(self, verb=True):
        """
        Model is Marvell
        """
        res = self.ismodel(['AY001MTM1','AY001MWA1','AY008MVL1'])
        if res:
            if verb:
                self.logger.info('Marvell model <{0}> discovered'.format(self.ismodel()))
        return res

    def showsys(self):
        """
        Printout sys, prop, res dictionaries
        """
        for s in 'sys', 'prop', 'res':
            print()
            pprint(self.getsys(name=s))
        print()

    # Configuration properties

    def parseconf(self, refresh=0, bufidx=None, cname='conf show'):
        """
        Convert the response to show conf
        """
        if refresh:
            self.cmd2list(cname)
        if bufidx is None:
            cd = self.data['buf'][cname]
        else:
            cd = self.data['bufall'][cname][bufidx]
        sd = {}
        s1 = 'foo'
        for s in cd:
            try:
                xkey,xval = s.split('|')
                self.logger.info('parse conf s {0} key {1} val {2}'.format(s, xkey, xval))
            except ValueError:
                self.logger.error('Skipping conf: {0}'.format(s))
                continue
            if s1 != s:
                key = xkey.strip()
                try:
                    val,hexval = xval.strip().split()
                except ValueError:
                    try:
                        val = xval.strip('\"')
                    except ValueError:
                        self.logger.error('Skipping conf: {0}'.format(s))
                        continue
                d = {'val':val,'str':s}
                try:
                    psd = sd[key]
                except:
                    sd[key] = {}                    
                sd[key] = d
        pprint({n:sd[n]['val'] for n in sd})
        return sd

    def confdiff(self,refresh=1,cname='conf show'):
        old = self.parseconf(refresh=0,bufidx=-1)
        new = self.parseconf(refresh=refresh)
        ul = list(set(new) | set(old))
        il = list(set(new) & set(old))
        md = {'new':[],'old':[],'mod':[],'all':{}, 'diff':{}}
        for k in ul:
            if k in il:
                if new[k] != old[k]:
                    md['mod'].append(k)
            elif k not in old:
                print('new')
                md['new'].append(k)
                old[k] = None
            elif k not in new:
                print('old')
                md['old'].append(k)
                new[k] = None
            else:
                assert False,'Unexpected key {0}'.format(k)
            if new[k] != old[k]:
                md['diff'][k] = [new[k],old[k]]
            else:
                md['all'][k] = [new[k],old[k]]
        for k in sorted(md['diff'].keys()):
            self.logger.info('{0:34} {1} -> {2}'.format(k, md['diff'][k][1], md['diff'][k][0]))
        return md['diff']

    def getconf(self, key=None, mode=None, refresh=0, verb=False):
        """
        Get the conf key value
        """
        cd = self.parseconf(refresh=refresh)
        if key is None:
            rv = cd
        else:
            try:
                rv = cd[key]
            except KeyError:
                try:
                    rv = self.data['cfgdef'][key]
                    if verb:
                        self.logger.info("Setting {0} to the default <{1}>".format(key,rv))
                except KeyError:
                    rv = None
                    if verb:
                        self.logger.warn("Unknown conf key <{0}>, possibly default".format(key))
        if verb:
            self.logger.info("conf show: {0} = {1}".format(key,rv))
        return rv

    def getssec(self, refresh=0, key='server/security/enable', verb=1):
        """
        Get conf server security
        """
        rv = self.getconf(refresh=refresh, key=key)
        if rv not in ['0','1']:
            assert False,'Uknown {0} value {1}'.format(key,rv)
        if verb:
            self.logger.info("Server security: {0}".format(rv))
        return 0 if rv == '0' else 1

    # OEM properties

    def parseoem(self, refresh=False, bufidx=None, valid=True, verb=True, cname='oem'):
        """
        Convert the response to <oem>
        """
        cd = {}
        cde = {'oem': '0dfc7900', 'oem_model': 'ledevb', 'oem_key': '(is set)'}
        if refresh:
            self.cmd2list(cname)
        if bufidx is None:
            cl = self.data['buf'][cname]
        else:
            cl = self.data['bufall'][cname][bufidx]
        cd = {l.split(':')[0].strip().strip('"'):l.split(':')[-1].strip().strip('"') for l in cl}
        if valid:
            assert sorted(cde) == sorted(cd), 'Expected {0} got {1}'.format(sorted(cde),sorted(cd))
        else:
            cd = [int(sorted(cde) != sorted(cd)),sorted(cde),sorted(cd)]
        return cd

    def testoem(self, verb=False, cname='oem'):
        """
        Test wrapper for <oem> command
        """
        return self.adatest(self.parseoem,verb=verb,valid=True,cname=cname)

    # Logging

    def levellog(self, nick, rev=False):
        """
        Convert nick to log levels
        """
        ld = {'e':'error','w':'warning','i':'info','d':'debug','D':'debug2','m':'metric'}
        if rev:
            pass
        else:
            name = ld[nick]
        return name

    def setlog(self, mld={}, reset=True, cmd='log', verb=False, pers=False):
        """
        Set the selected log levels for the modules
        """
        cmds = []
        if reset:
            cmds.append('log none')
        for mod in mld.keys():
            if mod == 'all':
                cmds.append(' '.join([cmd, mld[mod]]))
            else:
                cmds.append(' '.join([cmd,'--mod', mod, ' '.join(mld[mod])]))
        for cmd in cmds:
            if verb:
                self.logger.info('{0}'.format(cmd))
            self.sendcmd(cmd)
#        self.getlog(list(mld.keys()), refresh=1, verb=verb)
        if pers:
            self.sendcmd('save')
        return self.getlog(refresh=1, verb=verb)

    def getlog(self, mods=None, refresh=0, verb=True):
        """
        Get configured log levels for the module(s)
        """
        sd = self.parselog(refresh)
        mkeys = list(sd.keys())
        if (mods is None) or (mods == 'all'):
            rd = sd
        else:
            rd = {}
            ml = []
            if not isinstance(mods,list):
                ml.append(mods)
            else:
                ml.extend(mods)
            for mod in ml:
                assert mod in mkeys, 'Module <{0}> not found in <{1}>'.format(mod,mkeys)
                rd.update({mod:sd[mod]})
        return rd

    def parselog(self, refresh=0, name=None, verb=True, cmd='log'):
        """
        Parse <log> command
        """
        ld = {}
        if name is None:
            cname = cmd
        else:
            cname = ' '.join(cmd, '--mod', name)
        if refresh:
            self.cmd2list(cname)
        else:
            try:
                cstr = self.data['buf'][cname]
            except KeyError:
                if verb:
                    self.logger.warn('Excecuting <{0}> for the first time'.format(cname))
                    self.cmd2list(cname)
        for s in self.data['buf'][cname]:
            mod,val = s.split(':')
            ld[mod.split(' ')[-1]] = [v.strip() for v in val.strip('.').split('.')]
        return ld

    def catchlog(self, rl, mlvl=None, verb=False, exc=None):
        """
        Capture log for the selected module,level,value(description) list
        """
        logl = []
        for r in rl:
#            print(r)
            rd = {}
            rd['lstr'] = r
            try:
                ll = r.split()
                if ll[0] == '[ada]':
                    ll = ll[1:]
                rd['llst'] = ll
                rd['tstamp'] = ll[0]
                ls = ' '.join(ll[1:])
                ll1 = ls.split(':')
                rd['msg'] = ' '.join(ll1[1:]).strip()
                rd['mod'] = ll1[0].split()[-1]
                rd['levl'] = ll1[0].split()[0:-1]
                rd['lev'] = self.levellog(rd['levl'][0])
                if verb:
                    pprint(rd)
                if len(rd['levl']) > 1:
                    if verb:
                        self.logger.warn('Too many log levels {0}, using {1}'.format(rd['levl'],rd['lev']))
                if mlvl is not None:
                    for mlv in mlvl:
                        mod,lev,msg = mlv
                        if (mod == rd['mod']) and (lev == rd['lev']) and (msg in rd['msg']):
                            rd['msge'] = msg
                            logl.append(rd)
                            self.logger.info('Loggged: {0}'.format(rd['lstr']))
                        else:
                            if verb:
                                self.logger.warn('Not Logged: {0}'.format(rd['lstr']))
            except Exception as e:
                if exc:
                    estr = '<{0}> is not a log <{1}>'.format(r,e)
                    self.adaexcept(estr)
                else:
                    if r != "":
                        if verb:
                            self.logger.warn('Skipping not a log <{0}>: <{1}>'.format(r,e))
        if mlvl is not None:
            mlvlg = [[e['mod'],e['lev'],e['msge']] for e in logl]
            assert mlvl == mlvlg[:len(mlvl)], 'Expected {0} got {1}'.format(mlvl, mlvlg)
        return logl

    def sendcmdlog(self, cmd, mlvl=[], tout=3, valid=True, verb=False, pers=False, exc=True):
        """
        Send command and validate logs from mlvl list
        """
        if mlvl:
            self.setlog({m:{l} for m,l,v in mlvl},pers=pers)
        cl = self.sendcmd(cmd,exc=exc).splitlines()
        rl = [c.strip(' ') for c in cl if c != '']
        if cl != rl:
            if verb:
                self.logger.warn('<{0}> converted to <{1}>'.format(cl,rl))
        if valid:
            assert cmd == rl[0], 'Expected <{0}> got <{1}>'.format(cmd,rl[0])
            rl = rl[1:]
        if tout is not None:
            time.sleep(int(tout))
            rl += self.sendcmd('').splitlines()
        logl = self.catchlog(rl,mlvl,verb=verb)
        if verb:
            for ld in logl:
                 self.logger.info('{0} {1}: {2}'.format(ld['mod'],ld['lev'],ld['msg']))
        if mlvl:
            self.setlog()
        return logl

    def testlog(self, verb=False, cmd='log'):
        """
        Verify <log> command
        """
        lnd = {
            'mod': ['error','warning'],
            'client': ['error','warning'],
            'conf': ['error','warning'],
            'dnss': ['error','warning'],
            'netsim': ['error','warning'],
            'notify': ['error','warning'],
            'server': ['error','warning'],
            'wifi': ['error','warning'],
            'ssl': ['error','warning'],
            'log-client': ['error','warning'],
            'io': ['error','warning'],
            'sched': ['error','warning'],
            'eth': ['error','warning'],
            'test': ['error','warning'],
            }
        lmod = sorted(lnd)
        llev = ['error', 'warning', 'info', 'debug', 'debug2', 'metric', 'tests passing', 'tests failing']
        lad = dict(zip(lmod, [llev]*len(lmod)))
        for cmd in ['log all','log none','log --mod test debug']:
            rd = self.parselog(refresh=True,cmd=cmd)
            if cmd == 'log none':
                md = lnd
            elif cmd == 'log all':
                md = lad
            else:
                md = {}
                c = cmd.split()
                assert c[0]+' '+c[1] == 'log --mod', 'Malformed log cmd {0}'.format(cmd)
                mod = c[2]
                lev = c[3:]
                md[mod] = lnd[mod] + lev
            if md != rd:
                break
        return [int(md != rd),md,rd]

    def cmdappup(self, verb=True):
        self.logger.info('cmd app up')
        resp = self.sendcmd('up')
        return resp

    def cmdappdown(self, verb=True):
        self.logger.info('cmd app down')
        resp = self.sendcmd('down')
        return resp

    def cmdlogdisable(self, verb=True):
        self.logger.info('cmd log disable')
        resp = self.sendcmd('log debug -all')
        return resp

    def cmdpropsetblueled(self, verb=True):
        self.logger.info('cmd prop set blue led')
        resp = self.sendcmd('test-prop set Blue_LED 1')
        return resp

    def cmdpropgetblueled(self, verb=True):
        self.logger.info('cmd prop get bool')
        resp = self.sendcmd('test-prop get Blue_LED')
        return resp

    def cmdpropsetgreenled(self, verb=True):
        self.logger.info('cmd prop set green led')
        resp = self.sendcmd('test-prop set Green_LED 1')
        return resp

    def cmdpropsetint(self, verb=True):
        self.logger.info('cmd prop set int')
        resp = self.sendcmd('test-prop set input -101')
        return resp

    def cmdpropgetint(self, verb=True):
        self.logger.info('cmd prop get int')
        resp = self.sendcmd('test-prop get input')
        return resp

    def cmdpropsetuint(self, verb=True):
        self.logger.info('cmd prop set uint')
        resp = self.sendcmd('test-prop set uinput 212')
        return resp

    def cmdpropgetuint(self, verb=True):
        self.logger.info('cmd prop get uint')
        resp = self.sendcmd('test-prop get uinput')
        return resp

    def cmdpropsetstring(self, verb=True):
        self.logger.info('cmd prop set string')
        resp = self.sendcmd('test-prop set cmd abc')
        return resp

    def cmdpropgetstring(self, verb=True):
        self.logger.info('cmd prop get string')
        resp = self.sendcmd('test-prop get cmd')
        return resp

def main():
    port = '/dev/ttyUSB2'
    cmd = CmdAda('TestCli', port=port)
    pprint(cmd.testshowver())
#    cmd.showsys()
#    cmd.dcls['log'].settime()
#    cmd.parseconf(refresh=1)

    if 0:
        for e in 'log', 'tr', 'txt', 'rlog':
            print(cmd.dcls['log'].getdir(e))
            print(cmd.dcls['log'].getfname(e, 'logs'))
            if e != 'rlog':
                print(cmd.dcls['log'].getfname(e, 'grive'))

if __name__ == '__main__':
    main()

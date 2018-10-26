#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

from CmdAdw import *

class CmdAdwMod(CmdAdw):
    """
    Module's ADW CLI commands
    """

    def __init__(self, suite, port='', localCmd=''):
        """
        Constructor configures serial port, change prompt for a platform
        """
        self.data['prompt'] = {'prod':'-->','setup':'setup->','manuf':'manuf->'}
        CmdAdw.__init__(self, suite, port, plat2prompt('module'), localCmd)

    def getcmdExt(self):
        """
        Dictionary of extended platform's commands
        """
        cmds = {
            'show conf': '',
            'show oem': '',
            'show id': '',
            'show version-fw': '',
            'show version-boot': '',
            }
        return cmds

    def cmd2list(self, cmd='show id', prompt=''):
        """
        Execute the command, convert response to lines and create log names
        """
        red = {
            'log': '\d{2}:\d{2}:\d{2}\.\d{3}',
            }
        return super(CmdAdwMod, self).cmd2list(cmd,prompt,red)

    def cmdproc(self, cmd, verb=True):
        """
        Command's postprocessing, executed by cmd2list
        """
        try:
            try:
                self.getsys('model', verb=False)
            except Exception as e:
                self.logger.err('Command <model> failed {0}'.format(e))
                raise e
            try:
                self.getsys('version', ' ', verb=False)
                try:
                    self.dcls['log'].update(self.getsys('version', ' '), self.getsys('model'), suite=self.data['suite'],
                                            refresh=0)
                except:
                    self.dcls['log'] = Log(self.getsys('version', ' '), self.getsys('model'), suite=self.data['suite'])
            except:
                pass
        except:
            pass
        
    def parsesys(self, refresh=0, name='sys', cmd='show', verb=False):
        """
        Convert the response to show sys, prop, res commands into dict
        """
        if name == 'sys':
            nl = ['version', 'id', 'oem', 'version-fw', 'version-boot']
        else:
            nl = [name] 
        sd = {}
        for name in nl:
            if name == 'version-fw':
                cname = 'version'
            elif name == 'version-boot':
                cname = 'version boot'
            elif name == 'pwr':
                cname = 'power'
            else:
                cname = cmd + ' ' + name
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
                if name == 'id':
                    junk, key, val = s.split(' ')
                elif name == 'oem':
                    key, val = s.split(':')
                elif name == 'pwr':
                    kvl = [v.strip() for v in s.split(',')]
                    for kv in kvl:
                        try:
                            key, val = kv.split(':')
                            if key == 'power current':
                                key = 'current'
                            elif key == 'timer_names':
                                vl = kvl[1:]
                                vl.insert(0, val.strip())
                                val = vl
                        except:
                            key = kv.split()[0]
                            if key == 'modes':
                                vl = kvl[1:]
                                vl.insert(0, kv.split()[-1])
                                val = vl
                            else:
                                key = None
                                val = 'junk'
                        try:
                            val = val.strip()
                        except:
                            pass
                        if key is not None:
                            sd[key.strip()] = val
                elif name in ['version', 'version-fw', 'version-boot']:
                    key = name
                    val = s
                else:
                    err = 'Unknown command {0}'.format(cname)
                    self.adaexcept(err)
                if key is not None:
                    try:
                        val = val.strip()
                    except Exception as err:
                        err = 'Key:Val <{0}>:<{1}>, {2}'.format(key,val,err)
                        self.logger.warn('{0}'.format(err))
                    sd[key.strip()] = val
        return sd

    def getsys(self, key='', sep='', refresh=0, name='sys', valid=True, verb=True):
        """
        Get the values of sys, prop, res keys
        """
        sd = self.parsesys(refresh, name)
        if valid:
            if name == 'pwr':
                sd = self.validpwr()
        try:
            if key != '':
                rv = sd[key]
                if sep != '':
                    if key == 'version':
                        rv = sep.join(rv.split())
                        if 'ADA' in rv:
                            rv = rv[0:2]
                    else:
                        rv = sep.join(rv.split())
            else:
                rv = sd
        except KeyError:
            err = "Unknown <{0}> name <{1}> in {2}".format(name, key, self.parsesys(refresh, name).keys())
            self.adaexcept(err, verb)
        return rv

    # OEM properties

    def parseoem(self, refresh=False, bufidx=None, valid=True, verb=True, cname='show oem'):
        return super(CmdAdwMod, self).parseoem(refresh=refresh,bufidx=bufidx,valid=valid,verb=verb,cname=cname)

    # Power

    def validpwr(self, refresh=0):
        """
        Validate <power> dict
        """
        sd = self.parsesys(refresh, 'pwr')
        modes  = sd['modes']
        for m in ['current', 'saved']:
            assert sd[m] in modes, 'Mode <{0}> for {1} is not in {2}'.format(sd[m],m,modes)
        timers = sd['timer_names']
        for t in timers:
            assert sd[t].split()[-1] == 'sec', 'Timer <{0}> value error <{1}>'.format(t,sd[t])
            sd[t] = sd[t].split()[0]
        sd['usage'] = sd['usage'].split()[1].strip('[').strip(']').split('|')
        return sd

    def setpwr(self, cmd, val, valid=True, verb=True):
        """
        Set <power> mode and timers
        """
        logl = {}
        modes  = self.getsys('usage',name='pwr')
        timers = self.getsys('timer_names',name='pwr')
        if cmd in modes:
            mvals = self.getsys('modes',name='pwr')
            assert val in mvals, '{0} value {1} not in {2}'.format(cmd,val,mvals)
            md = dict(zip(modes,modes))
            md['mode'] = 'saved'
        elif cmd in timers:
            assert int(val) >= 0, '{0} value {1} is not a positive number of sec'.format(cmd,val)
            val = str(val)
            md = dict(zip(timers,timers))
        else:
            self.logger.error('power command <{0}> not found in <{1}> or <{2}>'.format(cmd,modes,timers))
        val0 = self.getsys(md[cmd],name='pwr')
        if verb:
            self.logger.info('power {0}: {1} -> {2}'.format(cmd, val0, val))
        pcmd = ' '.join(['power', cmd, val])
        nol = [
            'current default',
            'mode default',
            'mode max_perf',
            'mode min',
            'mode standby',
            'unconf_powered 29',
            'unconf_powered 28801',
            'awake_time 0',
            'awake_time 301',
            'standby_powered 3601',
            'standby_powered 4',
            ]
        if cmd+' '+val in nol:
            mlvl=[]
            logl = self.cmd2list(pcmd)             
        else:
            mlvl=[['wifi','debug','set listen itvl to 1']]
            logl = self.sendcmdlog(pcmd, mlvl)
        val1 = self.getsys(md[cmd],name='pwr',refresh=1)
        if verb:
            self.logger.info('power {0}: {1}'.format(cmd, val1))
        if valid:
            if logl != ['invalid value']:
                assert val == val1, 'Command <power {0}> expected {1}, got {2}'.format(cmd,val,val1)
            tok = '/'.join(['power', cmd])
            val2 = val1
            if cmd == 'mode':
                mode = None
                vd = {'default':'80','max_perf':'50','min':'81','standby':'82'}
                val2 = vd[val1]
            else:
                mode = None
            try:
                val3 = self.getconf(tok, refresh=1, mode=mode)['val']
            except TypeError:
                val3 = None
            if cmd == 'current':
                assert val3 is None, 'Conf token <{0}> expected {1}, got {2}'.format(tok,None,val3)
            else:
                assert val2 == val3, 'Conf token <{0}> expected {1}, got {2}'.format(tok,val2,val3)
        return [val0, val, val1, logl]

    def testpwr(self, cmd, val, verb=True):
        tvd =  {'unconf_powered':    {'min':'30','max':'28800','default':'3600'},
                'awake_time':     {'min':'1', 'max':'300',  'default':'10'},
                'standby_powered': {'min':'5','max':'3600', 'default':'10'},
                }
        try:
            val1 = tvd[cmd][val]
            if val == 'min':
                val2 = str(int(val1) - 1)
            elif val == 'max':
                val2 = str(int(val1) + 1)
            else:
                val2 = None
            if verb:
                self.logger.info('{0}: converted {1} to <{2},{3}>'.format(cmd,val,val2,val1))
            if val2 is not None:
                exp = val2
                try:
                    pl = self.setpwr(cmd,val2)
                    org,exp,got,logl = pl
                    assert val2 == exp, '{0} val {1} not equal to exp {2}'.format(cmd,val2,exp)
                    if exp == got:
                        if verb:
                            self.logger.error('{0} {1} = {2} is violated {3}'.format(cmd,val,val1,val2))
                        val1 = None
                        got = 'violated'
                except Exception as e:
                    err = str(e)
                    got = err
                    if verb:
                        self.logger.info('<{0} {1}>: <{2}>'.format(cmd,val2,err))
                    exp = 'invalid value'
                    assert err == exp, '<{0} {1}> expected <{2}> got <{3}>'.format(cmd,val2,exp,err)
            if val1 is not None:
                exp = val1
                try:
                    org,exp,got,logl = self.setpwr(cmd,val1)
                    assert val1 == exp, '{0} val {1} not equal to exp {2}'.format(cmd,val1,exp)
                    if cmd == 'unconf_powered' and val == 'min':
                        self.wifiprofall('erase')
                        wgot = self.getwifi('prop','wifi')['prop']['wifi']
                        wexp = 'AP mode'
                        assert wgot== wexp,'WiFi expected <{0}>, got <{1}>'.format(wexp,wgot)
                        if verb:
                            self.logger.info('WiFi {0}'.format(wgot))
                            self.logger.info('Testing timer {0} for {1} sec'.format(cmd,val1))
                        tout = int(val1) + 1
                        mlvl=[['mod','info','wifi core disabled'],['wifi','info','stopping AP mode']]
                        wrnl = [mlv[-1] for mlv in mlvl]
                        lgl = self.sendcmdlog('power',mlvl,tout=tout)
                        tstamp = {0:None,1:None}
                        for i in [0,1]:
                            try:
                                assert lgl[i]['msg'] == wrnl[i],'Timer {0} expected {1} got {2}'.format(cmd,wrnl[i],lgl[i]['msg'])
                                if verb:
                                    self.logger.info('{0} -> {1}'.format(cmd,lgl[i]['msg']))
                                tstamp[i] = lgl[i]['tstamp']
                            except Exception as e:
                                self.logger.error('{0}'.format(e))
                        tstamp[0] = logl[0]['tstamp']
                        tim = hmsdiff(tstamp[1],tstamp[0])
                        if verb:
                            self.logger.info('{0}: {2} - {1} = {3}'.format(cmd,tstamp[0],tstamp[1],tim))
                        wgot = self.getwifi('prop','wifi')['prop']['wifi']
                        wexp = 'disabled'
                        assert wgot== wexp,'WiFi expected <{0}>, got {<1>}'.format(wexp,wgot)
                        if verb:
                            self.logger.info('WiFi {0}'.format(wgot))
                        self.wifien()
                        toutmin = int(val1)
                        toutmax = toutmin + toutmin/3
                        assert tim >= toutmin and tim <= toutmax, 'Timer {0}: {1} out of {2} -:- {3}'.format(cmd,tim,toutmin,toutmax)
                except Exception as err:
                    got = err
        except KeyError:
            exp = val
            try:
                org,exp,got,logl = self.setpwr(cmd,val)
                assert val == exp, '{0} val {1} not equal to exp {2}'.format(cmd,val,exp)
            except Exception as err:
                exp = val
                got = err
        try:
            default = tvd[cmd]['default']
        except:
            default = 'default'
        defl = self.setpwr(cmd,default)
        if verb:
            self.logger.info('Restoring default for {0} -> {1}'.format(cmd,defl))
        if exp == got:
            res = 0
        else:
            res = 1
        return [res,exp,got]

    def setupmodecmd(self, cmd, delay=3):
        sm = {
            'on':{'exe':'setup_mode enable g8RRgz2eV1ebs2VfKuuIuMXRTsU=','desc':'Entering setup_mode','resp':'configuration saved'},
            'off':{'exe':'setup_mode disable','desc':'Leaving setup_mode','resp':'configuration saved'},
            'off1':{'exe':'setup_mode disable','desc':'Bogus setup_mode','resp':'not in mfg or setup mode'},
            'secon':{'exe':'server security enable','desc':'Enabling server http security','resp':'Secure HTTP access - enabled'},
            'secoff':{'exe':'server security disable','desc':'Disabling server http security','resp':'Secure HTTP access - disabled'},
        }
        for cn in ['on',cmd,'off']:
            if cn == 'off':
                pcmd = self.data['prompt']['prod']
            else:
                pcmd = self.data['prompt']['setup']
            if cn == cmd:
                pass
            self.logger.info('{0}'.format(sm[cn]['desc']))
            for i in range(3):
                try:
                    if cn in ['on3']:
                        pcmd = self.data['prompt']['setup']
                        resp = self.cmd2list(sm[cn]['exe'],prompt=pcmd)
                        resp = self.cmd2list('save',prompt=pcmd)
                    elif cn in ['off3','off13']:
                        pcmd = self.data['prompt']['setup']
                        resp = self.cmd2list(sm[cn]['exe'],prompt=pcmd)
                        pcmd = self.data['prompt']['prod']
                        resp = self.cmd2list('save',prompt=pcmd)
                    else:
                        resp = self.cmd2list(sm[cn]['exe'],prompt=pcmd)
                    break
                except Exception as err:
                    self.logger.error('Failed executing {0} with error {1}'.format(cn,err))
            if sm[cn]['resp'] not in resp:
                self.logger.error('Expected response <{0}>, got <{1}>'.format(sm[cn]['resp'],resp))
            time.sleep(delay)

    def getssec(self,sec,refresh=0):
        return sec

    def cmdssec(self,sec):
        if sec != self.getssec(sec):
            refresh = 0
        else:
            d = {0:'secoff',1:'secon'}
            self.setupmodecmd(d[sec])
            refresh = 1
        return self.getssec(sec,refresh=refresh)

def main():
    cmd = CmdAdwMod('TestCli', port='/dev/ttyUSB3')
    print(cmd.testpwr('unconf_powered','min'))
#    for c in [0,1]:    
#        cmd.cmdsser(c)

#    cmd.wifiwa(1,url='/client',refresh=1)
#    cmd.wagetres(ssec=1,url='/client')
#    if False:
#        ssid = 'AP'
#        cmd.wifijoin(ssid, sync=1)
#        ssid = 'AY-DEV'
#        cmd.wifijoin(ssid, sync=1)
#    cmd.nm.nmconn(ssid)
#    cmd.sw.cmdiftog(ssid,5,verb=1)
#    if False:
#        for ssid in ['AirPort Time Capsule 1']:
#            print(cmd.wifijoin(ssid, check=1))
#        wf = cmd.getwifi()
#        for x in wf:
#            print("\n--> {0}\n".format(x))
#            showdict(wf[x])
#        print()
#    showdict(cmd.parsesys(1, name='pwr'))
    if 0:
        pd = {'mode':['max_perf','min','standby','default'],
              'current':['max_perf','min','standby','default'],
              'unconf_powered':['3599','3600'],
              'standby_powered':['299','300'],
              'awake_time':['19','20']}
        for key in ['mode','unconf_powered','standby_powered','awake_time','current']:
            for val in pd[key]:
                cmd.setpwr(key,val)
        for d in [{'cmd':'unconf_powered','val':'min'},
                  {'cmd':'unconf_powered','val':'max'},
                  {'cmd':'unconf_powered','val':'default'},]:
            print(cmd.testpwr(d['cmd'],d['val']))
    elif 0:
        for ant in ['0','1','3']:
            print(cmd.testwshow())
    else:
        if 0:
            cmd.setupmodecmd('on')
#        print(cmd.getwifi('prop','ssid'))
#        print(cmd.testpwr('unconf_powered','min'))
#    cmd.parseconf(refresh=1)
#    cmd.setpwr('unconf_powered','3599')   
#    print(cmd.confdiff())
#    print(cmd.setpwr('standby_powered','299'))
#    print(cmd.setpwr('awake_time','15'))
#    print(cmd.setpwr('unconf_powered',3600))
#    showdict(cmd.parselog())
#    showdict(cmd.setlog({'wifi':['info','debug'],'mod':['info','debug']}))
if __name__ == '__main__':
    main()

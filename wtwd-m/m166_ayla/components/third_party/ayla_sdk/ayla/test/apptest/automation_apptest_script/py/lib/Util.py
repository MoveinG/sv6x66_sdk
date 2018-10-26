#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

import re
import sys
import os
import time
import subprocess
import logging
import datetime
from pprint import pprint

class Util:
    """
    Provides utilities
    """
    def __init__(self):
        """
        Constructor configures utils
        """
        pass


def showdict(d):
    """
    Prints out formatted dict
    """
    fmt = "%-Xs -> %s".replace('X', str(max(map(len, d))))
    klist = list(d.keys())
    klist.sort()
    for k in klist:
        print(fmt % (k, d[k]))


def findip(string):
    """
    Extracts IP from the string
    """
    ip = re.findall(r'[0-9]+(?:\.[0-9]+){3}', string)
    try:
        ip[0]
    except IndexError:
        print("IP address not found in {0}".format(string))
        sys.exit(1)
    return ip


def cmdexe(cmd):
    rv = subprocess.getstatusoutput(cmd)
    if rv[0] != 0:
        print("Command {0} failed, with status {1}".format(cmd, rv))
        sys.exit(1)
    return rv[1].split('\n')


def findusb(port = 1):
    usbl = cmdexe('find /dev -name "ttyUSB*"')
    if usbl == '':
        print("No USB ports found")
        sys.exit(1)
    usbl.sort()
    print("/dev/ttyUSB found: {0}".format(usbl))
    if port == 1:
        usbl = re.findall(r'[0-9]+', " ".join(usbl))
    return usbl


def port2usb(port):
    usb = '/dev/ttyUSB' + str(port)
    assert os.path.isfile(usb) == False, \
        "USB port <%s> doesn't exist" % usb
    return usb


def chkenv():
    evars = ['PYLIB']
    for evar in evars:
        try:
            os.environ[evar]
            sys.path.append(os.environ[evar])
            print("Added evar {0} to system path".format(evar))
        except KeyError:
            print("Please set environment variable %s in rc.sh".format(evar))
            sys.exit(1)


def prompt2plat(prompt, rev=0, verb=1, port=None):
    """
    Convert prompt to platform
    """
    pr2pl = {'#': 'ameba', '# ': 'wmsdk', 'shell> ': 'qca4010', '--> ': 'module', 'PWB apptest> ': 'linux'}
    if rev != 0:
        pl2pr = dict(zip(pr2pl.values(), pr2pl.keys()))
        pr2pl = pl2pr
    try:
        plat = pr2pl[prompt]
    except KeyError:
        plat = 'Unknown prompt <{0}>'.format(prompt)
    if port and plat == 'wmsdk':
        plat = 'ameba'
    return plat

def plat2prompt(plat):
    """
    Convert platform to prompt
    """
    return prompt2plat(plat, 1)


def aylassid():
    sl = {
        'AY-DEV': {'security': 'WPA2_Personal', 'key': '@ayla1234'},
        }
    return sl


def crlist(el, mult):
    if el is None:
        lel = []
        for i in range(0, mult):
            lel.append(el)
    elif len(el) != 1:
        print('Error: length of the element <{0}> is not 1'.format(el))
        sys.exit(0)
    else:
        lel = el*mult
    return lel


def hashdict():
    hd = {
		'e088142': '2.9.1 rc1', '483563f': '2.9.1 rc2',
    }
    return hd


def hash2bld(hsh, rel=''):
    try:
        rl = hashdict()[hsh].split()
        hrel = rl[0]
        hbld = rl[1]
        if rel != '':
            if rel != hrel:
                print("Error: for hash {0} got release {1}, expected {2}".
                        format(hsh, rel, hrel))
                sys.exit(0)
        bld = hbld
    except KeyError:
        bld = hsh
    return bld

def gettstamp():
    return '{:%Y-%m-%d %H:%M:%S}'.format(datetime.datetime.now())

def sec2hms(sec, valid=None, verb=False):
    sec = int(sec)
    if valid:
        if sec == 0:
            if verb:
                print("Warning: changing seconds from 0 to 1")
            sec += 1
    hrs = sec//3600
    sec -= 3600*hrs
    min = sec//60
    sec -= 60*min
    if hrs:
        hms = "{0}h {1}m {2}s".format(hrs, min, sec)
    elif min:
        hms = "{0}m {1}s".format(min, sec)
    else:
        hms = "{0}s".format(sec)
    return hms

def hms2sec(hms, msec=False, verb=False):
    try:
        hms,ms = hms.split('.')
    except:
        ms = '0'
    try:
        h,m,s = hms.split(':')
    except Exception as e:
        print('Error: {0} converting {1}'.format(e,hms))
    tim = int(h)*3600 + int(m)*60 + int(s)
    if msec:
        tim = tim*1000 + int(ms)
    return tim

def hmsdiff(hms2,hms1,msec=False,verb=True):
    sec2 = hms2sec(hms2,msec)
    sec1 = hms2sec(hms1,msec)
    if sec2 < sec1:
        dsec = 24*3600
        if msec:
            dsec*=1000
        if verb:
            print('Midnight rollover: converting {0} to {1}'.format(sec2,sec2+dsec))
        sec2 += dsec
    diff = sec2-sec1
    return diff

def loggerinit(name, root=None):
    logger = logging.getLogger(name)
    if not logger.handlers:
        tstamp = datetime.datetime.now().strftime("%Y%m%d%H%M%S")
        formatter = logging.Formatter(fmt='%(asctime)s %(name)-12s %(levelname)-8s %(message)s',
                                      datefmt='%H:%M:%S')
        handler = logging.FileHandler('/tmp/log_{0}_{1}.txt'.format(name,tstamp), mode='w')
        handler.setFormatter(formatter)
        screen_handler = logging.StreamHandler(stream=sys.stdout)
        screen_handler.setFormatter(formatter)
        logger.setLevel(logging.DEBUG)
        logger.addHandler(handler)
        logger.addHandler(screen_handler)
    else:
        logger.warn('Logger {0} already exists'.format(name))
    return logger

def balbrk(brkstr):
    bfl = ['{','[','(']
    bbl = ['}',']',')']
    bed = [[],[],[]]
    bfd = dict(zip(bfl,bbl))
    bbd = dict(zip(bbl,bfl))
    bod = dict(zip(bfl,bed))
    bl = []
    for b in brkstr:
        if b in bfd or b in bbd:
            for t in bbl:
                if b == t:
                    bod[t].append(b)
                elif b == bfd[t]:
                    bod[bbd[t]].append(b)
    return bod
            

def main():
    if 0:
        showdict({'First': '1', 'Second': 2, 'Two hundred ninth': 209})
        print(findip("[ada] 00:00:10.200 d m client: client_up: IP 172.16.17.102"))
        print(cmdexe('ls -l'))
        print(findusb()[-1])
        print(port2usb(2))
        showdict(hashdict())
        print(hash2bld('2123200'))
        print(hash2bld('2123201'))
        try:
            print(hash2bld('2123200', '2.4.0'))
        except:
            pass
    elif 0:
        hms1 = '22:11:38.013'
        hms2 = '00:12:10.253'
        print('{0} {1} {2}'.format(hms2sec(hms2),hmsdiff(hms2,hms1),hmsdiff(hms2,hms1,msec=True)))
    else:
        showdict(balbrk('{[)}(}['))

if __name__ == '__main__':
    main()

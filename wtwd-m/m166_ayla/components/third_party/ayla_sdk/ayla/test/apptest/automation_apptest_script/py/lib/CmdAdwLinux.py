#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

from CmdAdwTest import *

class CmdAdwLinux(CmdAdwTest):
    """
    Provides methods to qca4010 ADA serial commands
    """
    def __init__(self, suite, port='', localCmd=''):
        """
        Constructor configures serial port, change prompt for a platform
        """
        CmdAdw.__init__(self, suite, port, plat2prompt('linux'), localCmd)


def main():
    cmd = CmdAdwLinux('TestCli')
    print()

if __name__ == '__main__':
    main()

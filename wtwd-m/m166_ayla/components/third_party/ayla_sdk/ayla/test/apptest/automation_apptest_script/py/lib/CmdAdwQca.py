#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

from CmdAdwTest import *

class CmdAdwQca(CmdAdwTest):
    """
    Provides methods to qca4010 ADA serial commands
    """
    def __init__(self, suite, port='', localCmd=''):
        """
        Constructor configures serial port, change prompt for a platform
        """
        CmdAdw.__init__(self, suite, port, plat2prompt('qca4010'), localCmd)


def main():
    cmd = CmdAdwQca('TestCli')
    print()

if __name__ == '__main__':
    main()

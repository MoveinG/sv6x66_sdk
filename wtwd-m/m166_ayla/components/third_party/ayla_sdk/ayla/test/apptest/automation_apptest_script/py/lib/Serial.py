#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

import serial
import time
from Util import *
from Log import *

class Serial:
    """
    Provides methods to handle low level serial communications
    """
    dser = {}
    ser = ""

    def __init__(self, port='', brate='115200'):
        """
        Constructor configures serial port
        """
        self.logger = loggerinit(self.__class__.__name__)
        if port == '':
            port = findusb()[-1]
        self.dser = {'port': port, 'brate': brate}
        self.sopen(port, brate)

    def sopen(self, port, brate):
        """
        Configure serial port
        """
        if str(port).isdigit():
            port = port2usb(port)
        self.logger.info("Opening port {0} at {1}".format(port, brate))
        ser = serial.Serial(port, brate)
        ser.nonblocking()
        ser.flushInput()
        time.sleep(1)
        self.ser = ser
        return ser

    def getfd(self):
        return self.ser.fileno()

    def sclose(self):
        self.logger.info("Closing port {0}".format(self.dser['port']))
        self.ser.close()

    def sreset(self):
        self.logger.info("Resetting port {0}".format(self.dser['port']))
        self.ser.close()
        self.sopen(self.dser['port'], self.dser['brate'])

    def __del__(self):
        self.sclose()


def main():
    serObj = Serial()
    self.logger.info("Open port {0}, brate {1}, fd {2}".format(serObj.dser['port'], serObj.dser['brate'], serObj.getfd()))
    
if __name__ == '__main__':
    main()

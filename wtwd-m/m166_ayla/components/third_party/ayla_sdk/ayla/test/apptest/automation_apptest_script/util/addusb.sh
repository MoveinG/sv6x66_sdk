# !/bin/bash

#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

#
# Add missing USB ID for Broadcom MAB
#
#lsusb
#dmesg
ls /dev/*USB*
sudo modprobe ftdi_sio
sudo bash -c 'echo 0a5c 43fa > /sys/bus/usb-serial/drivers/ftdi_sio/new_id'
lsmod | grep ftdi
ls /dev/*USB*
#


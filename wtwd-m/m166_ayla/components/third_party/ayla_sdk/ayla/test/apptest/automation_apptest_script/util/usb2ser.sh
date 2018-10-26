# !/bin/bash

#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

#
# Find serial number for /dev/ttyUSB* device, all by default
#
if [ "$1" == "" ]; then
	DEVS=(`ls /dev/*USB*`)
else
	DEVS=("/dev/ttyUSB$1")
fi
for D in ${DEVS[@]}; do
	echo "$D"
	udevadm info -a -n $D | grep -E '{serial}' | head -n1
done
#


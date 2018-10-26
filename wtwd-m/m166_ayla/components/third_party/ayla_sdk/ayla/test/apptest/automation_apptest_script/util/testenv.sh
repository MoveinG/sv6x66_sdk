# !/bin/bash

#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

#
# Install test environment
# Use: ./testenv.sh
#

echo ""
echo "::: Installing test environment"
echo ""
#
DIR_GIT="${HOME}/automation_pda"
DIR_GRIVE="${HOME}/Grive/Automation/Device/logs"
DIR_GDRIVE="${HOME}/gdrive/Automation/Device/logs"
DIRS=($DIR_GIT $DIR_GRIVE $DIR_GDRIVE)
echo ">>> Checking directories ${DIRS[@]}"
echo ""
for DIR in ${DIRS[@]}; do
	if [ ! -d "$DIR" ]; then
		echo "+++ Creating directory $DIR"
		mkdir -p $DIR
	else
		echo "--- Directory $DIR elready exists"
	fi
done
#

UBUNTU_PLAT=$(uname -a | grep Ubuntu)
echo "$UBUNTU_PLAT"

if [ "$UBUNTU_PLAT" == "" ]; then
	UTILS=("git" "gitk" "tkcon" "gtkterm" "openocd" "dos2unix" "zlibc" "zlib1g" "zlib1g-dev" "libjansson-dev" "libxml2" "libxml2-dev" "libssl-dev" "idle" "idle3" "libmysqlclient-dev" "minicom" "python-xmlrunner" "python-mysqldb" "python3-pip" "python3-unittest2" "python3-serial" "python3-pexpect" "virtualenv") 
else
	UTILS=("git" "gitk" "tkcon" "gtkterm" "openocd" "dos2unix" "zlibc" "zlib1g" "zlib1g-dev" "libjansson-dev" "libxml2" "libxml2-dev" "libssl-dev" "idle" "idle3" "libmysqlclient-dev" "minicom")
fi

PIP_UTILS=("xmlrunner" "mysqlclient" "unittest2" "pexpect" "pyserial" "requests")
#
RPI=$(echo $HOSTNAME | grep lab-rpi)
if [ "$RPI" == "" ]; then
	echo "--- $HOSTNAME is NOT lab-rpi"
else
#	echo "+++ $HOSTNAME IS lab-rpi, adding Chromium"
#	UTILS=(${UTILS[@]} "chromium-browser")
	echo "+++ $HOSTNAME IS lab-rpi, adding Pycharm"
	echo "+++ Adding pycharm repository"
	$(sudo add-apt-repository -y ppa:mystic-mirage/pycharm)
	UTILS=(${UTILS[@]} "pycharm-community")
	echo "+++ Adding drive repository"
	$(sudo add-apt-repository -y ppa:twodopeshaggy/drive)
	UTILS=(${UTILS[@]} "drive")
	echo "+++ Adding gcc-arm repository"
	$(sudo add-apt-repository -y ppa:terry.guo/gcc-arm-embedded)
	UTILS=(${UTILS[@]} "gcc-arm-none-eabi")
fi
#
echo ""
echo ">>> Installing utilities <${UTILS[@]}>"
echo "> Updating apt-get"
GET_RESULT=$(sudo apt-get update)
echo "apt update result $GET_RESULT"
$(sudo dpkg --configure -a)
for UTIL in ${UTILS[@]}; do
	APT=$(sudo apt-get install -y $UTIL | grep "is already the newest version")
	if [ "$APT" == "" ]; then
		echo "+++ Installed utility $UTIL"
	else
		echo "--- $APT"
	fi
done

if [ "$UBUNTU_PLAT" != "" ]; then
	echo ""
	echo ">>> Installing pip3 utilities <${PIP_UTILS[@]}>"
	for PIP_UTIL in ${PIP_UTILS[@]}; do
		APT=$(pip3 install $PIP_UTIL)
		if [ "$APT" == "" ]; then
			echo "+++ Installed utility $PIP_UTIL"
		else
			echo "--- $APT"
		fi
	done
fi

#
REPOMODA="automation_apptest_script"

#

#
DIR_REG="$(find $DIR_GIT -name $REPOMODA)/logs/Regression"
echo ""
echo ">>> Checking regression logs directory ${DIR_REG}"
	if [ ! -d "$DIR_REG" ]; then
		echo "+++ Creating directory $DIR_REG"
		mkdir -p $DIR_REG
	else
		echo "--- Directory $DIR_REG already exists"
	fi
#
RCSRC="$(find $DIR_GIT -name $REPOMODA)/util/bashrc.sh"
RCTRG="${HOME}/.bashrc"
echo ""
echo ">>> Checking bashrc exists ${RCSRC}"
	if [ ! -f "$RCSRC" ]; then
		echo "ERROR: $RCSRC doesn't exist"
		exit 1
	else
		GH=$(cat ${RCTRG} | grep "GIT_HOME")
		if [ "$GH" == "" ]; then
#			echo "+++ Copy $RCSRC to $RCTRG"
#			cp -f $RCSRC $RCTRG
			echo "+++ Adding $RCSRC to $RCTRG"
			cat $RCSRC >> $RCTRG
		else
			echo "--- $RCTRG is already updated"
		fi
		GH=$(cat ${RCTRG} | grep "GIT_HOME")
		echo "::: Configured directories"
		echo $GH
	fi
#
echo ""
echo ">>> Allow $USER accsessing serial ports (dialout group)"
GRPS="$(groups | grep dialout)"
if [ "$GRPS" == "" ]; then
	echo "+++ Adding $USER to dialout group"
	sudo adduser $USER dialout
else
	echo "--- $USER is a member of groups <$GRPS>"
fi
#


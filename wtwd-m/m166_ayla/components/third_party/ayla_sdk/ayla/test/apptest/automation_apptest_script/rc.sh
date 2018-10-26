#
# Copyright 2018 Ayla Networks, Inc.  All rights reserved.
#

# Make sure to keep this file and $AUTOMATION/util/bin/set-mac-paths consistent
export OS="`uname -s`"
export MACH="`uname -m`"
export PLAT="$OS.$MACH"
#
export GRIVELOGS="${GRIVEDIR}/Automation/Device/logs"
export GDRIVE="$HOME/gdrive/Automation/Device/logs"
#
export MCUHOME="${MODHOME}/mcu"
export MCUCONS="${MCUHOME}/lib/ayla/internal/console"
export MCUDEMO="${MCUHOME}/arch/stm32/stm32f3/gcc"
#
if [ -z "$GRIVELOGS" ]; then
        echo "ERROR: GRIVELOGS not set"
else
        alias goglogs='cd $GRIVELOGS'
fi
#
#if [ -z "$AUTOMATIONLIBS" ]; then
#        echo "ERROR: AUTOMATIONLIBS not set"
#else
#        . ${AUTOMATIONLIBS}/rc
#fi
#
if [ -z "$AUTOMOD" ]; then
	echo "ERROR: AUTOMOD not set"
else
	export TCLHOME=$AUTOMOD/tcl
	export PYHOME=$AUTOMOD/py
	export PYLIB=$PYHOME/lib
	export VPY35=$PYHOME/Vpy35
	export LOGSHOME=$AUTOMOD/logs
	export UTILHOME=$AUTOMOD/util
	export UTILRPI=$UTILHOME/rpi
	export UTILHKIT=$UTILHOME/homekit
	export MISCHOME=$AUTOMOD/misc
	export RLABHOME=${GIT_HOME}/remote_lab
#
	alias gomod='cd $MODHOME'
	alias goauto='cd $AUTOMOD'
	alias gopy='cd $PYHOME'
	alias gotcl='cd $TCLHOME'
	alias gologs='cd $LOGSHOME'
	alias goutil='cd $UTILHOME'
	alias gomisc='cd $MISCHOME'
	alias gorpi='cd $UTILRPI'
#	alias golibs='cd $AUTOMATIONLIBS'
	alias gomcu='cd $MCUHOME'
	alias godemo='cd $MCUDEMO'
	alias gocons='cd $MCUCONS'
#
	alias pycharm='cd /tmp;$HOME/opt/pycharm/bin/pycharm.sh&'
	alias vpy35='source $VPY35/bin/activate'
	alias vpyinst='virtualenv --python=python3.5 $VPY35;pip install -upgrade -r ${RLABHOME}/requirements.txt'
fi
#
# ADA home for wmsdk, hap and qca
#
export ADAHOME="${MISCHOME}/ada"
#
# ADA Git directories
#
export ADAGIT="${MODHOME}/bc/lib/ada/test"
alias adahome='cd $ADAHOME'
alias adagit='cd $ADAGIT'
#
export AYLIB="${ADAGIT}/sdk/external/ayla"
export ADALIB="${AYLIB}/libada"
alias aylib='cd $AYLIB'
alias ayada='cd $ADALIB'
#
export ADAPKG="${MODHOME}/bc/pkg/ada"
alias adapkg='cd $ADAPKG'
#
# ADA for WMSDK
#
export ADAWM="${ADAHOME}/wmsdk"
export WMHOME="${ADAWM}/wmsdk"
export WMDEMO="${WMHOME}/sample_apps/cloud_demo/ayla_demo"
export OCDHOME="${WMHOME}/sdk/tools/OpenOCD/mw300"
export WMGIT="${ADAGIT}/wmsdk/ayla_demo"
export WMHOMEPKG="${ADAPKG}/wmsdk"
export WMDEMOPKG="${WMHOMEPKG}/files/sample_apps/cloud_demo/ayla_demo"
#
if [ -z "$ADAWM" ]; then
	echo "ERROR: ADAWM not set"
else
	alias gowm='cd $ADAWM'
	alias wmhome='cd $WMHOME'
	alias wmdemo='cd $WMDEMO/src'
	alias wmgit='cd $WMGIT/src'
	alias wmhomepkg='cd $WMHOMEPKG'
	alias wmdemopkg='cd $WMDEMOPKG/src'
#
	alias makewm='gowm;make APPS=cloud_demo/ayla_demo'
	alias gocd='cd $OCDHOME'
	alias ocdboot='gocd;sudo ./flashprog.sh --ftfs ${ADAWM}/sample_apps/cloud_demo/ayla_demo/bin/ayla_demo.ftfs'
	alias ocdmcu='gocd;sudo ./flashprog.sh --mcufw ${ADAWM}/sample_apps/cloud_demo/ayla_demo/bin/ayla_demo.bin'
fi
#
# ADA for HAP
#
export ADAHAP="${ADAHOME}/hapsdk"
export HAPHOME="${ADAHAP}/hap_sdk"
export HAPDEMO="${HAPHOME}/accessories/feature_examples/cloud_demo/ayla_demo"
export HAPGIT="${ADAGIT}/hapsdk/ayla_demo"
export HAPHOMEPKG="${ADAPKG}/hap_sdk"
export HAPDEMOPKG="${HAPHOMEPKG}/files/accessories/feature_examples/cloud_demo/ayla_demo"
#
if [ -z "$ADAHAP" ]; then
	echo "ERROR: ADAHAP not set"
else
	alias gohap='cd $ADAHAP'
	alias haphome='cd $HAPHOME'
	alias hapdemo='cd $HAPDEMO/src'
	alias hapgit='cd $HAPGIT/src'
	alias haphomepkg='cd $HAPHOMEPKG'
	alias hapdemopkg='cd $HAPDEMOPKG/src'
fi
#
# ADA for QCA
#
export ADAQCA="${ADAHOME}/qca"
export QCAHOME="${ADAQCA}/qca4010"
export QCADEMO="${QCAHOME}/target/demo/ayla_demo"
export QCAGIT="${ADAGIT}/qca4010/ayla_demo"
export QCAHOMEPKG="${ADAPKG}/qca4010"
export QCADEMOPKG="${QCAHOMEPKG}/files/target/demo/ayla_demo"
#
# XTENSA for ADA QCA
#
export XTENSA_INST=$HOME/xtensa
if [ -d "$XTENSA_INST" ]; then
	export XTENSA_CORE=KF
	export XTENSA_ROOT=$XTENSA_INST/XtDevTools/install/builds/RE-2013.3-linux/KF
	export XTENSA_SYSTEM=$XTENSA_ROOT/config
	export XTENSA_TOOLS_ROOT=$XTENSA_INST/XtDevTools/install/tools/RE-2013.3-linux/XtensaTools
	export PATH=$PATH:$XTENSA_TOOLS_ROOT/bin
	export XOCD=/opt/tensilica/xocd-10.0.3
fi
#
if [ -z "$ADAQCA" ]; then
	echo "ERROR: ADAQCA not set"
else
	alias goqca='cd $ADAQCA'
	alias qcahome='cd $QCAHOME'
	alias qcademo='cd $QCADEMO'
	alias qcagit='cd $QCAGIT'
	alias qcahomepkg='cd $QCAHOMEPKG'
	alias qcademopkg='cd $QCADEMOPKG'
#
	alias goxocd='cd $XOCD'
	alias xtocd='goxocd;./xt-ocd'
fi
#
# ADA for AMEBA
#
export AMEBAGIT="${ADAGIT}/ameba/ayla_demo"
export AMEBAHOMEPKG="${ADAPKG}/ameba"
export AMEBADEMOPKG="${AMEBAHOMEPKG}/files/component/common/application/ayla_demo"
if [ -z "$AMEBAGIT" ]; then
	echo "ERROR: AMEBAGIT not set"
else
	alias amebagit='cd $AMEBAGIT/src'
	alias amebahomepkg='cd $AMEBAHOMEPKG'
	alias amebademopkg='cd $AMEBADEMOPKG/src'
fi
#
	PATH=$PATH:$UTILHOME:$UTILRPI:$UTILHOMEKIT
#
MTS=`aylausb.sh | egrep 'mts|mab|mup' 2>/dev/null`
if [ -n "$MTS" ]; then
	echo ">>> Found <$MTS>, updating USB drivers"
	addusb.sh
fi
#
#goauto
#

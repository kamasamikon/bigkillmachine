#!/bin/sh

echo ">>>>>>>>>>>>>>>>>>>>> bkm.do <<<<<<<<<<<<<<<<<<<<<<<<<"
pwd
echo ">>>>>>>>>>>>>>>>>>>>> bkm.do <<<<<<<<<<<<<<<<<<<<<<<<<"

BLD_TYPE=`cat .bld_type 2> /dev/null`
CFG_TYPE=`cat .cfg_type 2> /dev/null`

BIN_DIR=../${BLD_TYPE}_${CFG_TYPE}/target

bkm_TARGET_DIR=$1

help_and_exit() {
    echo "bkm.do TargetDir"
    echo "  TargetDir : bld_all_cab/target etc"
    echo "  .bld_type : put the BUILD_TYPE to this file"
    echo "  .cfg_type : put the CONFIG_TYPE to this file"
    exit
}

inst_busybox() {
    for applet in `busybox --list`; do
        ln -s /bin/busybox $BIN_DIR/bin/$applet 2> /dev/null
    done
}

gen_banner() {
    echo "Generate banner"

    BANNER=${bkm_TARGET_DIR}/etc/banner.bkm

    figlet -w 5000 "BigKillMachine V0.2" > ${BANNER}
    echo "" >> ${BANNER}
    echo "" >> ${BANNER}

    echo "    PWD : `pwd`" >> ${BANNER}
    echo "   TIME : `date -R`" >> ${BANNER}
    echo "COMMAND : make DEBUG_INIT=1 CONFIG_TYPE=${CFG_TYPE} BUILD_TYPE=${BLD_TYPE}" >> ${BANNER}
    echo "" >> ${BANNER}
}

inst_ntvapps() {
    #1. Move ntvapp to ntvapp.otvorig
    #2. Copy muzei to ntvapp

    gen_banner

    echo "Replace init"
    if [ ! -e ${bkm_TARGET_DIR}/sbin/init.otvorig ]; then
        mv ${bkm_TARGET_DIR}/sbin/init ${bkm_TARGET_DIR}/sbin/init.otvorig
        cp init ${bkm_TARGET_DIR}/sbin/init
    fi

    echo "Replace normal applications"
    cp muzei ${bkm_TARGET_DIR}/bin/muzei
    for ntvapp in `cat ntvapp.list`; do
        if [ ! -e ${bkm_TARGET_DIR}/usr/local/bin/${ntvapp}.otvorig ]; then
            if [ -e ${bkm_TARGET_DIR}/usr/local/bin/${ntvapp} ]; then
                mv ${bkm_TARGET_DIR}/usr/local/bin/${ntvapp} ${bkm_TARGET_DIR}/usr/local/bin/${ntvapp}.otvorig
                ln -s /bin/muzei ${bkm_TARGET_DIR}/usr/local/bin/${ntvapp}
            fi
        fi
    done
}

inst_dagou() {
    cp -f libdagou.so ${bkm_TARGET_DIR}/lib
}

inst_logsewer() {
    cp -f dalogsewer ${bkm_TARGET_DIR}/bin
}

inst_envfile() {
    cp -f envfile.templ ${bkm_TARGET_DIR}/home/env.da
}

if [ ! -e .bld_type ]; then
    help_and_exit
fi
if [ ! -e .cfg_type ]; then
    help_and_exit
fi

inst_envfile
inst_logsewer
inst_dagou
inst_busybox
inst_ntvapps


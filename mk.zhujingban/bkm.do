#!/bin/sh

echo ">>>>>>>>>>>>>>>>>>>>> bkm.do <<<<<<<<<<<<<<<<<<<<<<<<<"
pwd
echo ">>>>>>>>>>>>>>>>>>>>> bkm.do <<<<<<<<<<<<<<<<<<<<<<<<<"

BLD_TYPE=`cat .bld_type 2> /dev/null`
CFG_TYPE=`cat .cfg_type 2> /dev/null`

BIN_DIR=../${BLD_TYPE}_${CFG_TYPE}/target

help_and_exit() {
    echo "bkm.do --help|help|all|bb|otv"
    echo ".bld_type : put the BUILD_TYPE to this file"
    echo ".cfg_type : put the CONFIG_TYPE to this file"
    exit
}

inst_busybox() {
    for applet in `busybox --list`; do
        ln -s /bin/busybox $BIN_DIR/bin/$applet
    done
}

inst_ntvapps() {
    #1. Move ntvapp to ntvapp.otvorig
    #2. Copy muzei to ntvapp

    echo "Replace init"
    cp banner.bkm ${TARGET_DIR}/sbin/
    if [ ! -e ${TARGET_DIR}/sbin/init.otvorig ]; then
        mv ${TARGET_DIR}/sbin/init ${TARGET_DIR}/sbin/init.otvorig
        cp init ${TARGET_DIR}/sbin/init
    fi

    echo "Replace normal applications"
    cp muzei ${TARGET_DIR}/bin/muzei
    for ntvapp in `cat ntvapp.list`; do
        if [ ! -e ${TARGET_DIR}/usr/local/bin/${ntvapp}.otvorig ]; then
            mv ${TARGET_DIR}/usr/local/bin/${ntvapp} ${TARGET_DIR}/usr/local/bin/${ntvapp}.otvorig
            cp -f muzei ${TARGET_DIR}/usr/local/bin/${ntvapp}
        fi
    done
}

inst_dagou() {
    cp -f libdagou.so ${TARGET_DIR}/lib
}

if [ ! -e .bld_type ]; then
    help_and_exit
fi
if [ ! -e .cfg_type ]; then
    help_and_exit
fi

if [ "$1" == "--help" ]; then
    help_and_exit
fi

inst_dagou
inst_busybox
inst_ntvapps


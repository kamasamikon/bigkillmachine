#!/bin/sh

BLD_TYPE=`cat .bld_type`
CFG_TYPE=`cat .cfg_type`

BIN_DIR=../${BLD_TYPE}_${CFG_TYPE}/target/bin

if [ ! -e $BIN_DIR ]; then
    echo "Set CONFIG_TYPE to .cfg_type"
    echo "Set BULID_TYPE to .bld_type"
    exit
fi

for applet in `busybox --list`; do
    ln -s /bin/busybox $BIN_DIR/$applet
done


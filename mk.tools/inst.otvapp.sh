#!/bin/sh

BLD_TYPE=`cat .bld_type`
CFG_TYPE=`cat .cfg_type`

TARGET_DIR=../${BLD_TYPE}_${CFG_TYPE}/target

if [ ! -e ../${BLD_TYPE}_${CFG_TYPE} ]; then
    echo "Set CONFIG_TYPE to .cfg_type"
    echo "Set BULID_TYPE to .bld_type"
    exit
fi

echo "Replace init"
if [ ! -e ${TARGET_DIR}/sbin/init.daorg ]; then
    mv ${TARGET_DIR}/sbin/init ${TARGET_DIR}/sbin/init.daorg
    cp init ${TARGET_DIR}/sbin/init
fi

echo "Replace normal applications"
cp muzei ${TARGET_DIR}/bin/muzei
for app in `cat ntvapp.list`; do
    if [ ! -e ${TARGET_DIR}/usr/local/bin/${app}.daorg ]; then
        mv ${TARGET_DIR}/usr/local/bin/${app} ${TARGET_DIR}/usr/local/bin/${app}.daorg
        cp -f muzei ${TARGET_DIR}/usr/local/bin/${app}
    fi
done


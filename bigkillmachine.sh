#!/bin/sh

echo "Usage: bigkillmachine.sh DIR CONFIG BUILD"
echo "eg.: bigkillmachine.sh /home/auv/Perforce/nemotv/NTV_OS/NemoTV5/MAIN/ntvtgt/sam7231_uclibc_bc sh_hdi bld"

DIR_PROJ=$1
TYPE_CONFIG=$2
TYPE_BUILD=$3

echo -e Hit return to continue.
read

echo
echo
echo "Apply the rcS patch"
RCS_PATH=${DIR_PROJ}/fs/skeleton/etc/init.d/rcS
if [ -f ${RCS_PATH}.nh.bak ]; then
    echo "rcS already backuped, skip"
fi

RCS_HASH=`md5sum ${RCS_PATH} | cut -d " " -f1 `
if [ "dcbfed08fb0852617beedb68561f37fd" != ${RCS_PATH} ]; then
    sudo meld rcS ${RCS_PATH}
fi
cp -v ${RCS_PATH} ${DIR_PROJ}/${TYPE_BUILD}_${TYPE_CONFIG}/target/etc/init.d/

echo
echo
echo "Copy libnemohook.so"
cp -v ./build/target/7231/libnemohook.so ${DIR_PROJ}/${TYPE_BUILD}_${TYPE_CONFIG}/target/lib
cp -v ./build/target/7231/libnemohook.so ${DIR_PROJ}/fs/skeleton/lib
cp -v ./build/target/7231/libhilda.so ${DIR_PROJ}/${TYPE_BUILD}_${TYPE_CONFIG}/target/lib
cp -v ./build/target/7231/libhilda.so ${DIR_PROJ}/fs/skeleton/lib

echo
echo
echo "Apply dbus buildroot patch"
BR_DIR=${DIR_PROJ}/../../buildroot
cp -v hook/dbus/*.patch ${BR_DIR}/package/dbus

echo
echo
echo "Process ntvlog.h"
if [ -f ${DIR_PROJ}/../../nemotv/src/utils/ntvlog.h.nh.bak ]; then
    echo "ntvlog.h already backuped, skip"
else
    mv -v ${DIR_PROJ}/../../nemotv/src/utils/ntvlog.h ${DIR_PROJ}/../../nemotv/src/utils/ntvlog.h.nh.bak
fi
cp -v ntvlog.h ${DIR_PROJ}/../../nemotv/src/utils/ntvlog.h

echo
echo
echo "DONE"

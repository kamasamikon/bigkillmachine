#!/bin/sh

echo "Usage: bigkillmachine.sh DIR CONFIG BUILD"
echo "eg.: bigkillmachine.sh /home/auv/Perforce/nemotv/NTV_OS/NemoTV5/MAIN/ntvtgt/sam7231_uclibc_bc sh_hdi bld"

echo $1

DIR_PROJ=$1
TYPE_CONFIG=$2
TYPE_BUILD=$3

BR_DIR=${DIR_PROJ}/../../buildroot

echo -e Hit return to continue.
read

echo "Apply the rcS patch"
RCS_PATH=${DIR_PROJ}/${TYPE_BUILD}_${TYPE_CONFIG}/target/etc/init.d/rcS
RCS_PATH=${DIR_PROJ}/fs/skeleton/etc/init.d/rcS
RCS_HASH=`md5sum ${RCS_PATH} | cut -d " " -f1 `
if [ "dcbfed08fb0852617beedb68561f37fd" != ${RCS_PATH} ]; then
    sudo meld rcS ${RCS_PATH}
else
    cp -v rcS ${DIR_PROJ}/${TYPE_BUILD}_${TYPE_CONFIG}/target/etc/init.d/
fi

echo "Copy libnemohook.so"
cp -v libnemohook.so ${DIR_PROJ}/${TYPE_BUILD}_${TYPE_CONFIG}/target/lib
cp -v libnemohook.so ${DIR_PROJ}/fs/skeleton/lib

cp -v libhilda.so ${DIR_PROJ}/${TYPE_BUILD}_${TYPE_CONFIG}/target/lib
cp -v libhilda.so ${DIR_PROJ}/fs/skeleton/lib

echo "Apply dbus buildroot patch"
cp -v hook/dbus/dbus_dbus_connection_dispatch_hook.patch ${BR_DIR}/package/dbus


echo "Process ntvlog.h"
if [ -f ${DIR_PROJ}/../../nemotv/src/utils/ntvlog.h.bak ]; then
    echo "ntvlog.h already backuped, skip"
else
    mv -v ${DIR_PROJ}/../../nemotv/src/utils/ntvlog.h ${DIR_PROJ}/../../nemotv/src/utils/ntvlog.h.bak
fi
cp -v ntvlog.h ${DIR_PROJ}/../../nemotv/src/utils/ntvlog.h

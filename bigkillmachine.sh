#!/bin/sh

echo "Usage: bigkillmachine.sh DIR CONFIG BUILD"
echo "eg.: bigkillmachine.sh /home/auv/Perforce/nemotv/NTV_OS/NemoTV5/MAIN/ntvtgt/sam7231_uclibc_bc sh_hdi bld 7231"

DIR_PROJ=$1
TYPE_CONFIG=$2
TYPE_BUILD=$3
TYPE_PLATFORM=$4
DIR_BR=${DIR_PROJ}/../../buildroot

echo -e "Hit return to continue."
read

echo
echo
echo "Apply the rcS patch"
echo
RCS_PATH=${DIR_PROJ}/fs/skeleton/etc/init.d/rcS
if [ -f ${RCS_PATH}.nhbak ]; then
    echo "rcS already backuped, skip"
fi

RCS_HASH=`md5sum ${RCS_PATH} | cut -d " " -f1 `
sudo meld rcS ${RCS_PATH}
cp -vf ${RCS_PATH} ${DIR_PROJ}/${TYPE_BUILD}_${TYPE_CONFIG}/target/etc/init.d/

echo
echo
echo "Copy libnemohook.so"
echo
cp -vf ./build/target/${TYPE_PLATFORM}/libnemohook.so ${DIR_PROJ}/${TYPE_BUILD}_${TYPE_CONFIG}/target/lib
cp -vf ./build/target/${TYPE_PLATFORM}/libnemohook.so ${DIR_PROJ}/fs/skeleton/lib
cp -vf ./build/target/${TYPE_PLATFORM}/libhilda.so ${DIR_PROJ}/${TYPE_BUILD}_${TYPE_CONFIG}/target/lib
cp -vf ./build/target/${TYPE_PLATFORM}/libhilda.so ${DIR_PROJ}/fs/skeleton/lib

echo
echo
echo "Apply dbus buildroot patch"
echo
cp -vf hook/dbus/*.patch ${DIR_BR}/package/dbus

echo
echo
echo "Process ntvlog.h"
echo
if [ -f ${DIR_PROJ}/../../nemotv/src/utils/ntvlog.h.nhbak ]; then
    echo "ntvlog.h already backuped, skip"
else
    mv -v ${DIR_PROJ}/../../nemotv/src/utils/ntvlog.h ${DIR_PROJ}/../../nemotv/src/utils/ntvlog.h.nhbak
fi
cp -vf ntvlog.h ${DIR_PROJ}/../../nemotv/src/utils/ntvlog.h

echo
echo
echo "Process buildroot package Makefile.in"
echo
if [ -f ${DIR_BR}/package/Makefile.in.nhbak ]; then
    echo "buildroot package Makefile.in already backuped, skip"
else
    mv -v ${DIR_BR}/package/Makefile.in ${DIR_BR}/package/Makefile.in.nhbak
fi
sudo meld br-package-Makefile.in ${DIR_BR}/package/Makefile.in

echo
echo
echo "DONE"
echo

#!/bin/sh

#
# /etc/init.d/rcS
#
# /bin/nemo.sh
# /bin/klagent
# /bin/klagent.sh
#
# /lib/libhilda.so
# /lib/libnemohook.so
#

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
DIR_RCS=${DIR_PROJ}/fs/skeleton/etc/init.d
if [ -f ${DIR_RCS}/rcS.nhbak ]; then
    echo "rcS already backuped, skip"
else
    cp -v ${DIR_RCS}/rcS ${DIR_RCS}/rcS.nhbak
fi

RCS_HASH=`md5sum ${DIR_RCS}/rcS | cut -d " " -f1 `
sudo meld templ/rcS ${DIR_RCS}/rcS

cp -vf ${DIR_RCS}/rcS ${DIR_PROJ}/${TYPE_BUILD}_${TYPE_CONFIG}/target/etc/init.d/

cp -vf ./build/target/${TYPE_PLATFORM}/libnemohook.so ${DIR_PROJ}/${TYPE_BUILD}_${TYPE_CONFIG}/target/lib
cp -vf ./build/target/${TYPE_PLATFORM}/libnemohook.so ${DIR_PROJ}/fs/skeleton/lib

cp -vf ./build/target/${TYPE_PLATFORM}/libhilda.so ${DIR_PROJ}/${TYPE_BUILD}_${TYPE_CONFIG}/target/lib
cp -vf ./build/target/${TYPE_PLATFORM}/libhilda.so ${DIR_PROJ}/fs/skeleton/lib

cp -vf ./build/target/${TYPE_PLATFORM}/klagent ${DIR_PROJ}/${TYPE_BUILD}_${TYPE_CONFIG}/target/bin
cp -vf ./build/target/${TYPE_PLATFORM}/klagent ${DIR_PROJ}/fs/skeleton/bin

cp -vf ./templ/nemo.sh ${DIR_PROJ}/${TYPE_BUILD}_${TYPE_CONFIG}/target/bin
cp -vf ./templ/nemo.sh ${DIR_PROJ}/fs/skeleton/bin

cp -vf ./templ/klagent.sh ${DIR_PROJ}/${TYPE_BUILD}_${TYPE_CONFIG}/target/bin
cp -vf ./templ/klagent.sh ${DIR_PROJ}/fs/skeleton/bin

# Use for make
cp -vf ./build/target/${TYPE_PLATFORM}/libhilda.so ${DIR_PROJ}/${TYPE_BUILD}_${TYPE_CONFIG}/staging/lib
ln -s ${HILDA_INC}/hilda ${DIR_PROJ}/${TYPE_BUILD}_${TYPE_CONFIG}/staging/usr/include

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
    cp -v ${DIR_PROJ}/../../nemotv/src/utils/ntvlog.h ${DIR_PROJ}/../../nemotv/src/utils/ntvlog.h.nhbak
fi
cp -vf templ/ntvlog.h ${DIR_PROJ}/../../nemotv/src/utils/ntvlog.h

echo
echo
echo "Process buildroot package nemotv_build.mak"
echo
if [ -f ${DIR_BR}/nemotv_build.mak ]; then
    echo "buildroot nemotv_build.mak already backuped, skip"
else
    cp -v ${DIR_BR}/nemotv_build.mak ${DIR_BR}/nemotv_build.mak.nhbak
fi
sudo meld templ/br-nemotv_build.mak ${DIR_BR}/nemotv_build.mak

echo
echo
echo "Fixing translater configure.ac, change 'CFLAGS = xxx' => 'CFLAGS += xxx'"
echo
vim ${DIR_PROJ}/../../nemotv/src/translator/configure.ac -c :66

echo
echo
echo "DONE"
echo

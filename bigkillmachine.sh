#!/bin/sh

# hilda_INC, hilda_LIB 
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

title ()  {
    echo
    echo
    echo $1
    echo
}

echo -e "Hit return to continue."
read

#########################################################################
title "Copy hilda stuff"

rm -fr ${DIR_PROJ}/../../toolchains/stbgcc-4.5.3-2.4/mipsel-linux-uclibc/sys-root/usr/include/hilda
ln -vs ${HILDA_INC}/hilda ${DIR_PROJ}/../../toolchains/stbgcc-4.5.3-2.4/mipsel-linux-uclibc/sys-root/usr/include

rm -fr ${DIR_PROJ}/../../toolchains/stbgcc-4.5.3-2.4/mipsel-linux-uclibc/sys-root/lib/libhilda.so
ln -vs ./build/target/${TYPE_PLATFORM}/libhilda.so ${DIR_PROJ}/../../toolchains/stbgcc-4.5.3-2.4/mipsel-linux-uclibc/sys-root/lib

#########################################################################
title "Apply the rcS patch"
DIR=${DIR_PROJ}/fs/skeleton/etc/init.d
if [ -f ${DIR}/rcS.nhbak ]; then
    echo "rcS already backuped, skip"
else
    cp -v ${DIR}/rcS ${DIR}/rcS.nhbak
fi

sudo meld templ/rcS ${DIR}/rcS

#########################################################################
cp -vf ${DIR}/rcS ${DIR_PROJ}/${TYPE_BUILD}_${TYPE_CONFIG}/target/etc/init.d/

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
ln -ns ${HILDA_INC}/hilda ${DIR_PROJ}/${TYPE_BUILD}_${TYPE_CONFIG}/staging/usr/include

#########################################################################
title "Apply dbus buildroot patch"
cp -vf hook/dbus/*.patch ${DIR_BR}/package/dbus

#########################################################################
title "Process ntvlog.h"
if [ -f ${DIR_PROJ}/../../nemotv/src/utils/ntvlog.h.nhbak ]; then
    echo "ntvlog.h already backuped, skip"
else
    cp -v ${DIR_PROJ}/../../nemotv/src/utils/ntvlog.h ${DIR_PROJ}/../../nemotv/src/utils/ntvlog.h.nhbak
fi
cp -vf templ/ntvlog.h ${DIR_PROJ}/../../nemotv/src/utils/ntvlog.h

#########################################################################
title "Process buildroot package nemotv_build.mak"
if [ -f ${DIR_BR}/nemotv_build.mak ]; then
    echo "buildroot nemotv_build.mak already backuped, skip"
else
    cp -v ${DIR_BR}/nemotv_build.mak ${DIR_BR}/nemotv_build.mak.nhbak
fi
sudo meld templ/br-nemotv_build.mak ${DIR_BR}/nemotv_build.mak

#########################################################################
# title "Fixing translater configure.ac, change 'CFLAGS = xxx' => 'CFLAGS += xxx'"
# vim ${DIR_PROJ}/../../nemotv/src/translator/configure.ac -c :66

echo "DONE"

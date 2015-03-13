#!/bin/sh

### #####################################################################
## NTV APPLICATION LIST
#
NTVAPPS="downloadmgr \
    measure \
    snmpmgr \
     timemgr \
    auth"

### #####################################################################
## Parse args
#
BLD=$1
CFG=$2
OUTPUTDIR=$3

if [ ! -d ${OUTPUTDIR}/${BLD}_${CFG} ]; then
    echo "  Usage : skmpost.sh BuildType ConfigType WorkDir"
    echo "Example : skmpost.sh bld all_cab /aaa/bbb/.../sdtv237_glibc_ct"
    echo
    echo "Not found: ${OUTPUTDIR}/${BLD}_${CFG}"
    exit
fi

if [ "x${OUTPUTDIR}" == "x" ]; then
    echo skmpost.sh BuildType ConfigType WorkDir
    exit
fi

### #####################################################################
## Show welcome
#
figlet -w 200 "Mou Kill Machine"

### #####################################################################
## Output BUILDINFO file
#
gen_buildinfo() {
    figlet -w 200 "Build Information" > ${OUTPUTDIR}/${BLD}_${CFG}/target/BUILDINFO
    echo "    DIR : ${OUTPUTDIR}" >> ${OUTPUTDIR}/${BLD}_${CFG}/target/BUILDINFO
    echo "   TIME : `date -R`" >> ${OUTPUTDIR}/${BLD}_${CFG}/target/BUILDINFO
    echo "COMMAND : make DEBUG_INIT=1 CONFIG_TYPE=${CFG} BUILD_TYPE=${BLD}" >> ${OUTPUTDIR}/${BLD}_${CFG}/target/BUILDINFO
    echo "" >> ${OUTPUTDIR}/${BLD}_${CFG}/target/BUILDINFO
}
gen_buildinfo

### #####################################################################
## Install ALL busybox applets
#
inst_busybox() {
    pushd .
    cd $1

    for applet in `busybox --list`; do
        ln -s /bin/busybox $applet 2> /dev/null
    done

    popd
}

inst_busybox ${OUTPUTDIR}/${BLD}_${CFG}/target/bin

### #####################################################################
## Copy MKM Helper
#

# Start telnet
gen_telnet_script__xxn() {
    OUTPUT=${OUTPUTDIR}/${BLD}_${CFG}/target/bin/xxn

    echo "#!/bin/sh" > ${OUTPUT}
    chmod a+rwx ${OUTPUT}

    echo "" >> ${OUTPUT}
    echo "# start udhcpc" >> ${OUTPUT}
    echo "cp /usr/share/udhcpc/default.script /tmp" >> ${OUTPUT}
    echo "sed -i 's/sbin/bin/g' /tmp/default.script" >> ${OUTPUT}
    echo "nohup udhcpc -s /tmp/default.script" >> ${OUTPUT}
    echo "" >> ${OUTPUT}
    echo "# start telnetd" >> ${OUTPUT}
    echo "nohup telnetd" >> ${OUTPUT}
    echo "" >> ${OUTPUT}
    echo "# Show me" >> ${OUTPUT}
    echo "ifconfig" >> ${OUTPUT}

    # XXX: the chmod in m.xxx.yyy cannot work
    chmod a+rwx /dev/console
}
gen_telnet_script__xxn

# Prepare MKM stuff
gen_mkm_stuff_xwood() {
    OUTPUT=${OUTPUTDIR}/${BLD}_${CFG}/target/bin/xwood

    echo "#!/bin/sh" > ${OUTPUT}
    chmod a+rwx ${OUTPUT}

    echo "" >> ${OUTPUT}
    echo "# dalog stuff" >> ${OUTPUT}
    echo "chmod a+rwx /dev/console" >> ${OUTPUT}
    echo "" >> ${OUTPUT}
    echo "cp /home/ntvroot/dalogrc /tmp/" >> ${OUTPUT}
    echo "cp /home/ntvroot/.dalog.cfg /tmp/" >> ${OUTPUT}
    echo "" >> ${OUTPUT}
    echo "# dabao stuff" >> ${OUTPUT}
    echo "cp /home/ntvroot/dabao /tmp/" >> ${OUTPUT}
    for ntvapp in ${NTVAPPS}; do
        echo "ln -s /tmp/dabao /tmp/dabao__${ntvapp}" >> ${OUTPUT}
    done
}
gen_mkm_stuff__xwood

### #####################################################################
## Create dalogrc
#
gen_dalogrc() {
    DALOGRC=${OUTPUTDIR}/${BLD}_${CFG}/target/home/ntvroot/dalogrc
    echo "DALOG_TO_LOCAL=/tmp/dalog.output" > ${DALOGRC}
    echo "_DALOG_TO_LOCAL=/dev/console" >> ${DALOGRC}
    echo "_DALOG_TO_LOCAL=/dev/stdout" >> ${DALOGRC}
    echo "_DALOG_TO_LOCAL=/dev/null" >> ${DALOGRC}
    echo "_DALOG_TO_NETWORK=`ifconfig eth0   | grep "inet addr" | tr ':' ' '  | awk '{ print $3 }'`:9999" >> ${DALOGRC}
}
gen_dalogrc

### #####################################################################
## Create .dalog.cfg
#
gen_dalog_cfg() {
    DALOGCFG=${OUTPUTDIR}/${BLD}_${CFG}/target/home/ntvroot/.dalog.cfg
    echo "mask=facewind-S-x-j" > ${DALOGCFG}
}
gen_dalog_cfg

### #####################################################################
## Help to log to /dev/console
#
set_lxc_console() {
    # chmod the /dev/console, so that DALOG_TO_LOCAL=/dev/console can work
    chmod a+rwx ${OUTPUTDIR}/${BLD}_${CFG}/target/dev/console

    # lxc.console = none => lxc.console = /dev/console
    find ${OUTPUTDIR}/${BLD}_${CFG}/target/usr/var/lib/lxc -name "*.conf" -exec sed -i "s/^\s*lxc.console.*$/lxc.console = \/dev\/console/g" \{\} \;
}

set_lxc_console


### #####################################################################
## Update the PCD file, /tmp/dabao__xxx -> /usr/local/bin/xxx
#
for ntvapp in ${NTVAPPS}; do
    sed "/^\s*COMMAND\s*=/ s@/usr/local/bin/${ntvapp}@/tmp/dabao__${ntvapp}@g" ${OUTPUTDIR}/${BLD}_${CFG}/target/etc/rules.pcd
    sed "/^\s*COMMAND\s*=/ s@/usr/local/bin/${ntvapp}@/tmp/dabao__${ntvapp}@g" ${OUTPUTDIR}/${BLD}_${CFG}/target/etc/rules_release.pcd
done

### #####################################################################
## Help to log to /dev/console
#
install_dabao() {
    echo "Create dabao for ntvapps"
    for ntvapp in ${NTVAPPS}; do
        ln -s /tmp/dabao /tmp/dabao__${ntvapp}
    done
}
install_dabao


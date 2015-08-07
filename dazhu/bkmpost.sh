#!/bin/sh

#
# THIS SHOULD BE PLACED AT THE END OF bld_all_cab/ntv_customize_fs.txt
#

### #####################################################################
## Parse args
#
BLD=$1                  # BUILD_TYPE
CFG=$2                  # CONFIG_TYPE
OUT=$3                  # sam7231_uclibc_bc, dazhu also in this dir

echo ">>>>>>>>>>>>>>>> BKMPOST <<<<<<<<<<<<<<<<"

### #####################################################################
## NTV APPLICATION LIST
#
NTVAPPS="downloadmgr \
    measure \
    snmpmgr \
     timemgr \
    auth"

BBAPPLETS="[ [[ addgroup adduser ar arping ash awk basename bash blkid bunzip2 \
    bzcat cat catv chattr chgrp chmod chown chroot chrt chvt cksum clear cmp \
    cp cpio crond crontab cut date dc dd deallocvt delgroup deluser devmem \
    df diff dirname dmesg dnsd dnsdomainname dos2unix du dumpkmap echo egrep \
    eject env ether-wake expr false fdflush fdformat fgrep find fold free \
    freeramdisk fsck ftpput fuser getopt getty grep groups gunzip gzip halt \
    hdparm head hexdump hostid hostname hwclock id ifconfig ifdown ifup \
    inetd init insmod install ip ipaddr ipcrm ipcs iplink iproute iprule \
    iptunnel kill killall killall5 klogd last less linux32 linux64 linuxrc \
    ln loadfont loadkmap logger login logname losetup ls lsattr lsmod lsof \
    lspci lsusb lzcat lzma makedevs md5sum mdev mesg microcom mkdir mkfifo \
    mknod mkswap mktemp modprobe more mount mountpoint mt mv nameif netstat \
    nice nohup nslookup od openvt passwd patch pidof ping ping6 \
    pipe_progress pivot_root pmap poweroff printenv printf ps pstree pwd \
    pwdx rdate readlink readprofile realpath reboot renice reset resize rm \
    rmdir rmmod route run-parts runlevel sed seq setarch setconsole \
    setkeycodes setlogcons setserial setsid sh sha1sum sha256sum sha3sum \
    sha512sum sleep sort start-stop-daemon stat strings stty su sulogin \
    swapoff swapon switch_root sync sysctl syslogd tail tar tee telnet \
    telnetd test tftp time top touch tr traceroute true tty ubimkvol \
    ubirmvol ubirsvol ubiupdatevol udhcpc umount uname uniq unix2dos unlzma \
    unxz unzip uptime users usleep uudecode uuencode vconfig vi vlock watch \
    watchdog wc wget which who whoami whois xargs xz xzcat yes zcat"

### #####################################################################
## Output BUILDINFO file
#
gen_buildinfo() {
    echo "gen_buildinfo"
    figlet -w 200 "Build Information" > ${OUT}/${BLD}_${CFG}/target/BUILDINFO
    echo "    DIR : ${OUT}" >> ${OUT}/${BLD}_${CFG}/target/BUILDINFO
    echo "   TIME : `date -R`" >> ${OUT}/${BLD}_${CFG}/target/BUILDINFO
    echo "COMMAND : make DEBUG_INIT=1 CONFIG_TYPE=${CFG} BUILD_TYPE=${BLD}" >> ${OUT}/${BLD}_${CFG}/target/BUILDINFO
    echo "" >> ${OUT}/${BLD}_${CFG}/target/BUILDINFO
}
gen_buildinfo

### #####################################################################
## Install ALL busybox applets
#
inst_busybox() {
    echo "inst_busybox"
    pushd . &> /dev/null
    cd $1

    for applet in ${BBAPPLETS}; do
        ln -s /bin/busybox $applet 2> /dev/null
    done

    popd &> /dev/null
}
inst_busybox ${OUT}/${BLD}_${CFG}/target/bin

### #####################################################################
## Install DA Files
#
inst_dagou() {
    echo "inst_dagou"
    # GouZi
    cp -f ${OUT}/dazhu/libdagou.so ${OUT}/${BLD}_${CFG}/target/lib
}
inst_dagou

inst_daxia() {
    echo "inst_daxia"
    # XiaShuiDao
    cp -f ${OUT}/dazhu/daxia ${OUT}/${BLD}_${CFG}/target/bin
}
inst_daxia

inst_dr() {
    echo "inst_dr"
    # DA Runtime Configure
    cp -f ${OUT}/dazhu/dr ${OUT}/${BLD}_${CFG}/target/bin
}
inst_dr

inst_inotdo() {
    echo "inst_inotdo"
    # DA Runtime Configure
    cp -f ${OUT}/dazhu/inotdo ${OUT}/${BLD}_${CFG}/target/bin
}
inst_inotdo

inst_dabao() {
    echo "inst_dabao"
    # BaoGuoChengXu
    cp -f ${OUT}/dazhu/dabao ${OUT}/${BLD}_${CFG}/target/home/ntvroot/dabao
}
inst_dabao

# Some additional script
inst_dabie() {
    echo "inst_dabie"
    # DieDeJiaoBen

    # Run those in dazhu/dabie/
    find ${OUT}/dazhu/dabie -perm /u+x -and -type f -exec {} ${BLD} ${CFG} ${OUT} \;

    # Run those in ${OUT}/dabie/
    find ${OUT}/dabie -perm /u+x -and -type f -exec {} ${BLD} ${CFG} ${OUT} \;
}
inst_dabie

inst_dapei() {
    echo "inst_dapei"
    # PeiZhiWenJian
    cp -f ${OUT}/dazhu/dapei ${OUT}/${BLD}_${CFG}/target/home/ntvroot/dapei
    sed -i "s@HOSTIP@`ifconfig eth0 | tr ':' ' ' | awk '/inet addr / { print $3 }'`@g" ${OUT}/${BLD}_${CFG}/target/home/ntvroot/dapei
}
inst_dapei

inst_cfgXet() {
    echo "inst_cfgXet"
    cp -f ${OUT}/${BLD}_${CFG}/build/dbus-1.4.16/tools/dbus-send ${OUT}/${BLD}_${CFG}/target/bin

    cp -f ${OUT}/dazhu/cfgset ${OUT}/${BLD}_${CFG}/target/bin
    cp -f ${OUT}/dazhu/cfgget ${OUT}/${BLD}_${CFG}/target/bin
}
inst_cfgXet


### #####################################################################
## Copy MKM Helper
#

# Start telnet
# xxn = cha cha men = wang
gen_telnet_script__xxn() {
    echo "gen_telnet_script__xxn"
    OUTPUT=${OUT}/${BLD}_${CFG}/target/bin/xxn

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
# xmu = x mutou = sha = da sha qi
gen_mkm_stuff__xmu() {
    echo "gen_mkm_stuff__xmu"
    OUTPUT=${OUT}/${BLD}_${CFG}/target/bin/xmu

    echo "#!/bin/sh" > ${OUTPUT}
    chmod a+rwx ${OUTPUT}

    echo "" >> ${OUTPUT}
    echo "# dalog stuff" >> ${OUTPUT}
    echo "chmod a+rwx /dev/console" >> ${OUTPUT}

    echo "" >> ${OUTPUT}
    echo "cp /home/ntvroot/dalog.dfcfg /tmp/" >> ${OUTPUT}
    echo "touch /tmp/dalog.rtcfg" >> ${OUTPUT}

    echo "" >> ${OUTPUT}
    echo "# dabao stuff" >> ${OUTPUT}
    echo "cp /home/ntvroot/dabao /tmp/" >> ${OUTPUT}
    for ntvapp in ${NTVAPPS}; do
        echo "cp /tmp/dabao /tmp/dabao__${ntvapp}" >> ${OUTPUT}
    done

    echo "" >> ${OUTPUT}
    echo "# dabao file" >> ${OUTPUT}
    echo "cp /home/ntvroot/dapei /tmp/" >> ${OUTPUT}

    echo "" >> ${OUTPUT}
    echo "# Original PCD" >> ${OUTPUT}
    echo "cp /etc/rules.pcd /tmp/" >> ${OUTPUT}
    echo "sed -i 's:/tmp/dabao__:/usr/local/bin/:g' /tmp/rules.pcd" >> ${OUTPUT}
}
gen_mkm_stuff__xmu

### #####################################################################
## Create dalog.dfcfg
#
gen_dalog_cfg() {
    echo "gen_dalog_cfg"
    DALOGCFG=${OUT}/${BLD}_${CFG}/target/home/ntvroot/dalog.dfcfg
    echo "mask=facew-i-n-d-S-x-j" > ${DALOGCFG}
}
gen_dalog_cfg

### #####################################################################
## Help to log to /dev/console
#
set_lxc_console() {
    echo "set_lxc_console"
    # chmod the /dev/console, so that DALOG_TO_LOCAL=/dev/console can work
    chmod a+rwx ${OUT}/${BLD}_${CFG}/target/dev/console

    # lxc.console = none => lxc.console = /dev/console
    find ${OUT}/${BLD}_${CFG}/target/usr/var/lib/lxc -name "*.conf" -exec sed -i "s/^\s*lxc.console.*$/lxc.console = \/dev\/console/g" \{\} \;
}

set_lxc_console


### #####################################################################
## Update the PCD file, /tmp/dabao__xxx -> /usr/local/bin/xxx
#
update_pcd() {
    echo "update_pcd"
    for ntvapp in ${NTVAPPS}; do
        if [ -e ${OUT}/${BLD}_${CFG}/target/etc/rules.pcd ]; then
            sed -i "/^\s*COMMAND\s*=/ s@/usr/local/bin/${ntvapp}@/tmp/dabao__${ntvapp}@g" ${OUT}/${BLD}_${CFG}/target/etc/rules.pcd
        fi

        if [ -e ${OUT}/${BLD}_${CFG}/target/etc/rules_release.pcd ]; then
            sed -i "/^\s*COMMAND\s*=/ s@/usr/local/bin/${ntvapp}@/tmp/dabao__${ntvapp}@g" ${OUT}/${BLD}_${CFG}/target/etc/rules_release.pcd
        fi
    done
}
update_pcd

### #####################################################################
## Helper for configman use dbus-send
#

echo "<<<<<<<<<<<<<<<< BKMPOST >>>>>>>>>>>>>>>>"

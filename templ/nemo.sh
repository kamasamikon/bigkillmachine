###
## ######################################################################
# Nemo: Load klog thing

# Dump boot command line
echo
echo "DUMP /proc/cmdline"
echo ">>>>>"
cat /proc/cmdline
echo "<<<<<"

grep "\bkl2syslog\b" -w -o /proc/cmdline
if [ "$?" == "0" ]; then
    export KLOG_TO_SYSLOG=YES
fi

# Remote Default klog configure
KLOG_RMCFG=`grep "\bklcfg=.*" -o /proc/cmdline | cut -d ' ' -f1 | awk -F= '{ print $2 }'`
if [ "$KLOG_RMCFG" != "" ]; then
	wget "${KLOG_RMCFG}" -O /tmp/klog.df.cfg
	cat /tmp/klog.df.cfg

    # KLOG_DFCFG will internal used by klog_init
    export KLOG_DFCFG=/tmp/klog.df.cfg
fi

# KLOG Save to remote
KLOG_SEWER_REMOTE=`grep "\bkl2remote=.*" -o /proc/cmdline | cut -d ' ' -f1 | awk -F= '{ print $2 }'`
if [ "$KLOG_SEWER_REMOTE" != "" ]; then
    export KLOG_SEWER_REMOTE
fi

# KLOG Save to file
KLOG_SEWER_LOCAL=`grep "\bkl2local=.*" -o /proc/cmdline | cut -d ' ' -f1 | awk -F= '{ print $2 }'`
if [ "$KLOG_SEWER_LOCAL" != "" ]; then
    export KLOG_SEWER_LOCAL
fi


# KLOG_RTCFG will used by nh_klog
export KLOG_RTCFG=/tmp/klog.rt.cfg

# KLOG Agent is a opt-rpc-server
/bin/klagent.sh ${KLOG_RTCFG} &

# Nemo: PCD output 
PCDOUT=`grep "\bpcdout=.*" -o /proc/cmdline | cut -d ' ' -f1 | awk -F= '{ print $2 }'`
if [ "$PCDOUT" != "" ]; then
    export PCDOUT
else
    export PCDOUT=/dev/null
fi


# Nemo: HOOK
# preload=/lib/libneomohook.so
PRELOAD=`grep "\bpreload=.*" -o /proc/cmdline | cut -d ' ' -f1 | awk -F= '{ print $2 }'`
if [ "$PRELOAD" != "" ]; then
    PRELOAD=LD_PRELOAD=${PRELOAD}
    export ${PRELOAD}
fi

echo
echo "KLOG_TO_SYSLOG = [[[ ${KLOG_TO_SYSLOG} ]]]"
echo
echo "KLOG_RMCFG = [[[ ${KLOG_RMCFG} ]]]"
echo "KLOG_SEWER_REMOTE = [[[ ${KLOG_SEWER_REMOTE} ]]]"
echo "KLOG_SEWER_LOCAL = [[[ ${KLOG_SEWER_LOCAL} ]]]"
echo
echo "PCDOUT = [[[ ${PCDOUT} ]]]"
echo
echo "PRELOAD = [[[ ${PRELOAD} ]]]"
echo "COMMAND = [[[ /usr/sbin/pcd -f /etc/rules.pcd -v &> ${PCDOUT} & ]]]"
echo

export NH_SYSLOG_NOREAL=YES


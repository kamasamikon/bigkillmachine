###
## ######################################################################
# Nemo: Load klog thing

# Dump boot command line
echo
echo "DUMP /proc/cmdline"
echo ">>>>>"
cat /proc/cmdline
echo "<<<<<"
echo

grep "kl2printf" -w -o
if [ "$?" == "0" ]; then
    export KLOG_TO_PRINTF=YES
fi

grep "kl2syslog" -w -o
if [ "$?" == "0" ]; then
    export KLOG_TO_SYSLOG=YES
fi

grep "kl2remote" -w -o
if [ "$?" == "0" ]; then
    export KLOG_TO_REMOTE=YES
fi

# Remote Default klog configure
KLOG_RMCFG=`grep "klcfg=.*" -o /proc/cmdline | cut -d ' ' -f1 | awk -F= '{ print $2 }'`
if [ "$KLOG_RMCFG" != "" ]; then
	wget "${KLOG_RMCFG}" -O /tmp/klog.df.cfg
	cat /tmp/klog.df.cfg

    # KLOG_DFCFG will internal used by klog_init
    export KLOG_DFCFG=/tmp/klog.df.cfg
fi

# klsewer
KLOG_SEWER_URL=`grep "klsewer=.*" -o /proc/cmdline | cut -d ' ' -f1 | awk -F= '{ print $2 }'`
if [ "$KLOG_SEWER_URL" != "" ]; then
    export KLOG_TO_REMOTE=YES
    export KLOG_SEWER_URL
else
    unset KLOG_TO_REMOTE
fi


# KLOG_RTCFG will used by nh_klog
export KLOG_RTCFG=/tmp/klog.rt.cfg

# KLOG Agent is a opt-rpc-server
/usr/local/bin/klagent ${KLOG_RTCFG}

# Nemo: PCD output 
PCDOUT=`grep "pcdout=.*" -o /proc/cmdline | cut -d ' ' -f1 | awk -F= '{ print $2 }'`
if [ "$PCDOUT" != "" ]; then
    export PCDOPT= -v &> ${PCDOUT}
else
    export PCDOPT=
fi


# Nemo: HOOK
# preload=/lib/libneomohook.so
PRELOAD=`grep "preload=.*" -o /proc/cmdline | cut -d ' ' -f1 | awk -F= '{ print $2 }'`
if [ "$PRELOAD" != "" ]; then
    PRELOAD=LD_PRELOAD=${PRELOAD}
    export ${PRELOAD}
fi

echo
echo "NEMO Part finished"
echo
echo "KLOG_TO_PRINTF = [[[ ${KLOG_TO_PRINTF} ]]]"
echo "KLOG_TO_SYSLOG = [[[ ${KLOG_TO_SYSLOG} ]]]"
echo "KLOG_TO_REMOTE = [[[ ${KLOG_TO_REMOTE} ]]]"
echo
echo "KLOG_RMCFG = [[[ ${KLOG_RMCFG} ]]]"
echo "KLOG_SEWER_URL = [[[ ${KLOG_SEWER_URL} ]]]"
echo
echo "PCDOPT = [[[ ${PCDOPT} ]]]"
echo
echo "PRELOAD = [[[ ${PRELOAD} ]]]"
echo "COMMAND = [[[ /usr/sbin/pcd -f /etc/rules.pcd ${PCDOPT} & ]]]"
echo


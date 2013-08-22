#!/bin/sh

KLOG_CFG_URL=`grep "klog_cfg_url=.*" -o /proc/cmdline | cut -d ' ' -f1 | awk -F= '{ print $2 }'`

if [ "$KLOG_CFG_URL" != "" ]; then
	echo "Nemo: should get klog configure file from ${KLOG_CFG_URL}"
	wget ${CFG_SRC_URL} -o /tmp/klog.cfg
else
	echo "Nemo: No klog configure from /proc/cmdline, create empty one"
	echo "" > /tmp/klog.cfg
fi


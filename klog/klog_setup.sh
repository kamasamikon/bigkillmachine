#!/bin/sh

KLOG_CFG_URL=`grep "klog_cfg_url=.*\b" -o /proc/cmdline | awk -F= '{ print $2 }'`

if [ "$KLOG_CFG_URL" != "" ]; then
	echo "Auv: should get klog configure file from ${KLOG_CFG_URL}"
	wget ${CFG_SRC_URL} -o /tmp/klog.cfg
else
	echo "No  klog configure from /proc/cmdline"
	echo "" > /tmp/klog.cfg
fi


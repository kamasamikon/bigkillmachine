#!/bin/sh

TARGETDIR=shit

for applet in `busybox --list`; do
    ln -s /bin/busybox $TARGETDIR/$applet
done


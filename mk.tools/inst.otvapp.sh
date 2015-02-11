#!/bin/sh

TARGETDIR=shit

for app in `cat ntvapp.list`; do
    ln -s /bin/muzei $TARGETDIR/${app}.org
done


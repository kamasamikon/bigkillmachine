#!/bin/sh

TARGETDIR=shit

for app in `cat ntvapp.list`; do
    if [ ! -e $TARGETDIR/${app}.org ]; then
        mv $TARGETDIR/${app} $TARGETDIR/${app}.org
        ln -s /bin/muzei $TARGETDIR/${app}
    fi
done


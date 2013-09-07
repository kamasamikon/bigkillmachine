#!/bin/sh

echo "Apply the rcS patch"
RCS_HASH=`md5sum ${RCS_PATH} | cut -d " " -f1 `
if [ "dcbfed08fb0852617beedb68561f37fd" != ${RCS_PATH} ]; then
    sudo meld rcS ${RCS_PATH}
fi

echo "Copy libnemohook.so"
cp libnemohook.so ${sdasdasd}

echo "Apply dbus buildroot patch"
cp *.patch ${BR_DBUS_PATH}

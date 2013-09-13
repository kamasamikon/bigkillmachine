#!/bin/sh

while [ 1 ]; do
    touch $1
    /bin/klagent $1
    echo "klagent error, auto relaunch"
done


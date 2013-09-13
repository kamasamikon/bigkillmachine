#!/bin/sh

while [ 1 ]; do
    touch $1
    /usr/local/bin/klagent $1
    echo "klagent error, auto relaunch"
done


#!/bin/sh

while [ 1 ]; do
    echo "klagent will monitor the ORCmd for KL"
    touch $1
    /bin/klagent $1
    echo "klagent error, auto relaunch"
done


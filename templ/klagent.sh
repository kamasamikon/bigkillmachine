#!/bin/sh

alias echo='busybox echo'
alias touch='busybox touch'

while [ 1 ]; do
    echo "klagent will monitor the ORCmd for KL"
    touch $1
    /usr/bin/klagent $1
    echo "klagent error, auto relaunch"
done


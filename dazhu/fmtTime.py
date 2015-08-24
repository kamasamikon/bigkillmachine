#!/usr/bin/env python

import sys

s_days = 60 * 60 * 24
s_hours = 60 * 60
s_minutes = 60

def fmtTime(t):
    s = ""

    if t >= s_days:
        s += "%d days " % (t / s_days)

    t = t % s_days
    if t >= s_hours:
        s += "%d hours " % (t / s_hours)

    t = t % s_hours
    if t >= s_minutes:
        s += "%d minutes " % (t / s_minutes)

    t = t % s_minutes
    s += "%d seconds" % t

    return s

if __name__ == "__main__":
    print fmtTime(int(sys.argv[1]))


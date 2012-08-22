#!/usr/bin/env python

import serial
import time
import sys

PORT = "/dev/ttyACM0"
max = 100

s = serial.Serial(PORT, baudrate=115200)

def write_command_generic(address,bytes)
    full_bytes = "".join([chr(x) for x in (0xff,address) + bytes])
    for jj in range(10):
        s.write(full_bytes)
        time.sleep(0.01)

def write_command_raw(m1, m2, m3, cam=128):
    write_command_generic(1,(0,m1,0,m2,0,m3,cam,3))

def write_command_xyr(x, y, r, cam=128):
    write_command_generic(0,(x,y,r,cam,3))

ramp = range(0,max) + range(max,0,-1) + range(255,255-max,-1) + range(255-max,255) + [0,]

if len(sys.argv) > 1 and sys.argv[1] == "xyr":
    write_command = write_command_xyr
    axis_names = ("X","Y","R")
else:
    write_command = write_command_raw
    axis_names = ("M1","M2","M3")

for axis in range(3):
    print "Testing axis: ", axis_names[a]
    for ii in ramp:
        sys.stdout.write("%d, " % ii)
        sys.stdout.flush()
        write_command(ii if a == 0 else 0,ii if a == 1 else 0,ii if a == 2 else 0)

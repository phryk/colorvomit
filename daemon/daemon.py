#!env python
# -*- coding: utf-8 -*-

import serial
from time import sleep

if __name__ == '__main__':

    com = serial.Serial('/dev/ttyU1', 9600, timeout=0.5)

    print "sleeping 3 secs"
    sleep(3);


    while(True):

        for v in range(0, 256, 10):
            print v

            line = "FRAME %d %d %d %d %d %d %d %d %d %d %d %d\r\n" % (v,v,v,v,v,v,v,v,v,v,v,v)

            com.write(line)

            response = com.readline()

            print response
            sleep(0.1)


#!env python
# -*- coding: utf-8 -*-

import serial
import colorsys
import numpy
from time import sleep

class LED(object):

    red = None
    green = None
    blue = None
    hue = None
    saturation = None
    value = None


    def __init__(self):

        self.red = 0
        self.green = 0
        self.blue = 0
        self.hue = 0
        self.saturation = 0
        self.value = 0

    
    def __setattr__(self, name, value):

        super(LED, self).__setattr__(name, value)
        if name in ('red', 'green', 'blue') and (self.red != None and self.green != None and self.blue != None):

            hsv = colorsys.rgb_to_hsv(self.red, self.green, self.blue)
            super(LED, self).__setattr__('hue', hsv[0])
            super(LED, self).__setattr__('saturation', hsv[1])
            super(LED, self).__setattr__('value', hsv[2])

        elif name in ('hue', 'saturation', 'value') and (self.hue != None and self.saturation != None and self.value != None):
            rgb = colorsys.hsv_to_rgb(self.hue, self.saturation, self.value)
            super(LED, self).__setattr__('red', rgb[0])
            super(LED, self).__setattr__('green', rgb[1])
            super(LED, self).__setattr__('blue', rgb[2])


    def __str__(self):

        return "%d %d %d" % (self.red, self.green, self.blue)


class Display(list):

    conn = None

    def __init__(self, conn, *args, **kw):

        self.conn = conn
        super(Display, self).__init__(*args, **kw)


    def render(self):

        line = 'FRAME '+' '.join([str(item) for item in self])
        self.conn.write(line)
        print "Response: "+self.conn.readline()


if __name__ == '__main__':


    com = serial.Serial('/dev/ttyU1', 115200, timeout=0.5)
    display = Display(com, [LED(), LED(), LED(), LED()])

    print "sleeping a bit"
    sleep(1.5);

    while(True):

        for hue in numpy.linspace(0,1, 100):

            for led in display:
                led.hue = hue
                led.saturation = 1
                led.value = 255

            display.render()
            sleep(0.01)

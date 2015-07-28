#!env python
# -*- coding: utf-8 -*-

import serial
import colorsys
import numpy
from time import sleep

# extra shit for audio analysis
from threading import Thread
from struct import unpack
from scipy.signal import gaussian, hann


FIFO_FILE = '/tmp/mpd.fifo'
FREQ = 44100 # Not currently used
SAMPLE_MIN = -32768
SAMPLE_MAX = 32767
WINDOW_SIZE = 2048 # The number of samples used to build a spectrum, also the size of the Collector
READ_SIZE = 2 # How many samples to read at once, make this WINDOW_SIZE if any problems occur


def bin_ranges(length, num_bins):

        length = numpy.float64(length)
        bin_range = []
        current_range = length / (2 ** num_bins -1)
        
        lower = 0
        upper = current_range -1
        if upper < lower:
            upper = lower
        bin_range.append([int(round(lower)), int(round(upper))])

        for i in range(1, num_bins):

            current_range *=2

            lower = upper + 1
            upper = lower + current_range - 1
            if upper < lower:
                upper = lower

            bin_range.append([int(round(lower)), int(round(upper))])

        return bin_range


class Analyzer(list):

    std = None
    filled = None
    amplitudes = None

    def __init__(self, *args, **kw):

        if kw.has_key('std'):
            self.std = kw.pop('std')
        else:
            self.std = False

        self.max = 0 #TODO: Killme
        self.filled = False

        self.amplitudes = numpy.zeros(int(WINDOW_SIZE/4))

        super(Analyzer, self).__init__(*args, **kw)

    def append(self, item):

        if len(self) == WINDOW_SIZE:
            self.pop(0)

        elif len(self) == (WINDOW_SIZE - 1):
            self.filled = True

        super(Analyzer, self).append(item)


    def get_snapshot(self):

        return list(self)

    
    def update(self):

        frame = self.get_snapshot()

        if self.std:
            frame = numpy.array(frame) * gaussian(len(frame), self.std)
        else:
            frame = numpy.array(frame) * hann(len(frame))

        fft = numpy.abs(numpy.fft.rfft(frame))
        n = fft.size
        
        try:
            #tm = numpy.max(fft[0:n/2])
            #if tm > self.max:
            #    self.max = tm
            #    print self.max
            #amplitudes = fft[0:n/2] / self.max
            amplitudes = fft[0:n/2] / 3200000

        except Exception as e:
            print fft
            exit("Whoopsie!")

        clean_amplitudes = []

        for amplitude in amplitudes:

            if not numpy.isnan(amplitude):
                clean_amplitudes.append(amplitude)

            else:
                print "NAN in fft!"
                clean_amplitudes.append(numpy.float64(0))

        self.amplitudes = numpy.array(clean_amplitudes)


    def get_amplitudes(self):


        return self.amplitudes


    def get_amplitudes_in_bins(self, num_bins):

        amplitudes = self.amplitudes
        
        bins = numpy.zeros(num_bins)
        i = 0
        ranges = bin_ranges(len(amplitudes), num_bins)
        for lower, upper in ranges: 

            if lower == upper:
                bin = amplitudes[lower]

            else: 
                bin = amplitudes[lower:upper].mean()

            if numpy.isnan(bin):
                bin = numpy.float64(0)

            bins[i] = bin
            i += 1

        return bins



class Collector(Thread):

    stop_signal = None
    analyzer = None

    def __init__(self, analyzer, interval=None):

        super(Collector, self).__init__()

        self.analyzer = analyzer
        self.daemon = True
        self.stop_signal = False


    def run(self):

        handle = open(FIFO_FILE, 'r')

        while not self.stop_signal:

            chunk = handle.read(READ_SIZE * 2)
            raw_samples = [chunk[i:i+2] for i in range(0, READ_SIZE)]

            for raw_sample in raw_samples:
                self.analyzer.append(unpack('>h', raw_sample)[0])

    def stop(self):

        self.stop_signal = True


class Visualizer(object):

    analyzer = None
    display = None
    smoothed_amplitudes = None
    smoothed_volume = None

    def __init__(self, analyzer, display):

        self.analyzer = analyzer
        self.display = display

        amps = []
        self.smoothed_amplitudes = numpy.zeros(int(WINDOW_SIZE/4))
        self.smoothed_volume = 0

    def update(self):
        if self.analyzer.filled:

            amplitudes = self.analyzer.get_amplitudes()
            self.smoothed_amplitudes = (self.smoothed_amplitudes * 0.95) + (amplitudes * 0.05)
            self.smoothed_volume = (self.smoothed_volume * 0.95) + (amplitudes[-1] * 0.05)
            frame = self.smoothed_amplitudes


class TestVisu(Visualizer):

    def update(self):

        if self.analyzer.filled:
            amplitudes = self.analyzer.get_amplitudes_in_bins(4)
            self.smoothed_volume = (self.smoothed_volume * 0.9) + (amplitudes[-1] * 0.1)

            i = 0
            for led in display:
                led.hue += amplitudes[i] * 0.2
                led.saturation = 1
                led.value = self.smoothed_volume
                i += 1


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

            # make sure values can't be > 1
            if value > 1:
                value = value - int(value)

            rgb = colorsys.hsv_to_rgb(self.hue, self.saturation, self.value)
            super(LED, self).__setattr__('red', rgb[0])
            super(LED, self).__setattr__('green', rgb[1])
            super(LED, self).__setattr__('blue', rgb[2])


    def __str__(self):

        return "%d %d %d" % (int(self.red * 255), int(self.green * 255), int(self.blue * 255))


class Display(list):

    conn = None

    def __init__(self, conn, *args, **kw):

        self.conn = conn
        self.successes = 0
        self.failures = 0
        super(Display, self).__init__(*args, **kw)


    def render(self):

        line = 'FRAME %s\n' % (' '.join([str(item) for item in self]),)
        self.conn.write(line)
        resp = self.conn.readline()

        if resp.startswith('OK'):
            self.successes += 1
        else:
            self.failures += 1
            print "Success count: %d" % (self.successes,)
            print "Failure count: %d" % (self.failures,)

            print (float(self.failures) / float(self.successes + self.failures)) * 100


if __name__ == '__main__':


    com = serial.Serial('/dev/cuaU1', 57600, timeout=0.5)
    display = Display(com, [LED(), LED(), LED(), LED()])

    analyzer = Analyzer(std=WINDOW_SIZE/16)
    collector = Collector(analyzer)
    collector.start()
    visu = TestVisu(analyzer, display)

    print "Sleeping a bit."
    sleep(3)
    print "Starting to do stuff now."


    while(True):

            analyzer.update()
            visu.update()
            display.render()
            #sleep(0.01)

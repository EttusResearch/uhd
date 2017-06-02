#!/usr/bin/env python

from usrptest.labview_control import lv_control
import time
import numpy as np
import argparse
import sys

def test00(freq, frange, steps):
    freqrange = np.arange(freq,freq+frange,steps)
    source = 'VST-Out'
    sink = 'X3x0-4-B-RX2'
    host = 'pollux'
    base_path = 'C:\Users\sdrtest\git\labview-test\labview\RTS_Control\Host\\'
    print('connecting to switch')
    switch = lv_control.executive_switch(host,base_path,'RTSwitch')
    print('connecting to siggen')
    siggen = lv_control.vst_siggen(host,base_path,'RIO0')
    # Configure RF-Switching
    switch.connect_ports(source, sink)

    # Sweep over freqrange
    for freq in freqrange:
        siggen.set_freq(float(freq))
    time.sleep(1)
    # Shutdown Siggen
    siggen.disconnect()
    switch.disconnect_all()

def test01(freq, frange, steps):
    freqrange = np.arange(freq,freq+frange,steps)
    source = 'VST-Out'
    sink0 = 'X3x0-4-B-RX2'
    sink1 = 'X3x0-2-B-RX2'
    host = 'pollux'
    base_path = 'C:\Users\sdrtest\git\labview-test\labview\RTS_Control\Host\\'
    print('connecting to switch')
    switch = lv_control.executive_switch(host,base_path,'RTSwitch')
    print('connecting to siggen')
    siggen = lv_control.vst_siggen(host,base_path,'RIO0')
    for freq in freqrange:
        siggen.set_freq(float(freq))
        print('retuning siggen to {freq} MHz'.format(freq=freq/1e6))
        switch.connect_ports(source,sink0)
        time.sleep(0.5)
        switch.connect_ports(source,sink1)
        time.sleep(0.5)
        switch.disconnect_all()
    time.sleep(0.2)
    siggen.disconnect()
    switch.disconnect_all()
        

if __name__ == '__main__':
    thismodule = sys.modules[__name__]
    parser = argparse.ArgumentParser()
    parser.add_argument(
            '-t',
            '--test',
            help='which testcase?'
            )
    parser.add_argument(
            '-f',
            '--freq',
            type=float,
            help='which center freq?'
            )
    parser.add_argument(
            '-r',
            '--range',
            type=float,
            help='which freq range?'
            )
    parser.add_argument(
            '--steps',
            type=float,
            default=1e6,
            help='which frequency step size?'
            )
    args = parser.parse_args()
    getattr(thismodule,args.test)(args.freq,args.range,args.steps)

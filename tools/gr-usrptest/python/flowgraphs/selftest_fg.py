#!/usr/bin/env python

from gnuradio import blocks
from gnuradio import gr
from gnuradio import uhd
from usrptest import phase_calc_ccf
from gnuradio.uhd.uhd_app import UHDApp
import numpy as np

class selftest_fg(gr.top_block):

    def __init__(self, freq, samp_rate, dphase, devices=list()):
        gr.top_block.__init__(self, "Generate Signal extract phase")

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate
        self.freq = 10e3
        self.devices = devices
        self.dphase = dphase
        self.tx_gain = 50
        self.rx_gain = 50
        self.center_freq = freq
        self.omega = 2*np.pi*self.freq
        self.steps = np.arange(
            self.samp_rate)*float(self.omega)/float(self.samp_rate)
        self.reference = self.complex_sine(self.steps)
        self.test_signal = self.complex_sine(self.steps+0.5*np.pi)
        self.device_test = False

        ##################################################
        # Block dicts
        ##################################################
        self.rx_devices = dict()
        self.tx_devices = dict()
        self.sink_dict = dict()
        self.phase_dict = dict()
        self.reference_source = blocks.vector_source_c(self.reference)

        if len(self.devices):
            ##################################################
            # Devices
            ##################################################
            self.device_test = True
            #To reuse existing setup_usrp() command
            for device in self.devices:
                # Create and configure all devices
                self.rx_devices[device] = uhd.usrp_source(
                    device, uhd.stream_args(
                        cpu_format="fc32", channel=range(1)))
                self.tx_devices[device] = uhd.usrp_sink(
                    device, uhd.stream_args(
                        cpu_format="fc32", channel=range(1)))
                self.rx_devices[device].set_samp_rate(self.samp_rate)
                self.rx_devices[device].set_center_freq(self.center_freq, 0)
                self.rx_devices[device].set_gain(self.rx_gain, 0)
                self.tx_devices[device].set_samp_rate(self.samp_rate)
                self.tx_devices[device].set_center_freq(self.center_freq, 0)
                self.tx_devices[device].set_gain(self.tx_gain, 0)
                self.sink_dict[device] = blocks.vector_sink_f()
                self.phase_dict[device] = phase_calc_ccf(
                    self.samp_rate, self.freq)
            for device in self.tx_devices.values():
                self.connect((self.reference_source, 0), (device, 0))

            for device_key in self.rx_devices.keys():
                self.connect(
                    (self.rx_devices[device_key], 0), (self.phase_dict[device_key], 0))
                self.connect((self.reference_source, 0),
                             (self.phase_dict[device_key], 1))
                self.connect(
                    (self.phase_dict[device_key], 0), (self.sink_dict[device_key], 0))
            # Debug options
            # self.sink_list.append(blocks.vector_sink_c())
            #self.connect((device, 0), (self.sink_list[-1], 0))
            # self.sink_list.append(blocks.vector_sink_c())
            #self.connect((self.reference_source, 0), (self.sink_list[-1], 0))
        else:
            ##################################################
            # Blocks
            ##################################################
            self.result = blocks.vector_sink_f(1)
            self.test_source = blocks.vector_source_c(self.test_signal)
            self.block_phase_calc = phase_calc_ccf(
                self.samp_rate, self.freq)

            ##################################################
            # Connections
            ##################################################
            self.connect((self.reference_source, 0), (self.block_phase_calc, 1))
            self.connect((self.test_source, 0), (self.block_phase_calc, 0))
            self.connect((self.block_phase_calc, 0), (self.result, 0))
    def complex_sine(self, steps):
        return np.exp(1j*steps)

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate

    def run(self):
        self.start()
        self.wait()
        if self.device_test:
            data = dict()
            for device_key in self.sink_dict.keys():
                curr_data = self.sink_dict[device_key].data()
                curr_data = curr_data[int(0.2*self.samp_rate):-int(0.2*self.samp_rate)]
                phase_avg = np.average(curr_data)
                if (np.max(curr_data) < phase_avg+self.dphase*0.5) and (np.min(curr_data) > phase_avg-self.dphase*0.5):
                    data[device_key] = phase_avg
                else:
                    print("Error phase not settled")

            #Debug
            # plt.ylim(-1, 1)
            # plt.xlim(self.samp_rate/2.), (self.samp_rate/2.)+1000)
            #for key in data:
            #    plt.plot(data[key])
        return data

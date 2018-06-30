#!/usr/bin/env python2

from gnuradio import gr
from gnuradio import uhd
from gnuradio import analog
from usrptest import phase_calc_ccf, measurement_sink_f
from usrptest.functions import log_level
from random import uniform
from ast import literal_eval
import copy
import logging
import sys


class phasealignment_fg(gr.top_block):
    def __init__(self, uhd_app):
        gr.top_block.__init__(self, "Calculate dphi for all USRPs (Rx only)")
        ##############################
        #   Block dicts
        ##############################
        self.log = logging.getLogger(__name__)
        [self.log.removeHandler(h) for h in self.log.handlers]
        self.log.addHandler(logging.StreamHandler(sys.stdout))
        self.log.setLevel(log_level(uhd_app.args.log_level))
        self.rx_streams = list()
        self.phase_diff_calc = list()
        self.measurement_sink = list()
        self.uhd_app = copy.copy(uhd_app)
        self.uhd_app.args = copy.copy(uhd_app.args)
        self.tx_app = copy.copy(uhd_app)
        self.tx_app.args = copy.copy(uhd_app.args)
        self.samp_rate = uhd_app.args.samp_rate
        # Create all devices specified in --receiver
        # Create all remaining blocks and connect devices to the first port and
        # sink
        self.uhd_app.args.num_chan = len(self.uhd_app.args.channels)
        self.log.info('setting up usrp....')
        ##############################
        #  Setup RX
        ##############################
        self.log.debug("RX-Setup with args: {}".format(self.uhd_app.args))
        self.uhd_app.setup_usrp(uhd.usrp_source, self.uhd_app.args)
        if self.uhd_app.args.measurement_setup is not None:
            self.measurement_channels = self.uhd_app.args.measurement_setup.strip(
            ).split(',')
            # make sure every channels is listed in measurement_channels at least once
            if len(set(
                    self.measurement_channels)) != self.uhd_apps.args.num_chan:
                self.uhd_app.vprint(
                    "[{prefix}] Number of measurement channels has to be the number of used channels."
                )
                self.uhd_app.exit(1)
            self.measurement_channels = [self.uhd_app.args.channels.index(m) for m in self.measurement_channels]
        else:
            self.measurement_channels = range(self.uhd_app.args.num_chan)

        self.measurement_channels_names = list()
        for chan in self.measurement_channels:
            usrp_info = self.uhd_app.usrp.get_usrp_info(chan)
            self.measurement_channels_names.append("_".join(
                [usrp_info['mboard_serial'], usrp_info['rx_serial']]))

        #Connect channels to first port of d_phi_calc_block and to measurement_sink
        for num, chan in enumerate(self.measurement_channels[:-1]):
            self.phase_diff_calc.append(phase_calc_ccf())
            self.measurement_sink.append(
                measurement_sink_f(
                    int(self.uhd_app.args.samp_rate *
                        self.uhd_app.args.duration), self.uhd_app.args.runs))
            self.connect((self.uhd_app.usrp, chan),
                         (self.phase_diff_calc[num], 0))
            self.connect((self.phase_diff_calc[num], 0),
                         (self.measurement_sink[num], 0))
        # Connect devices to second port of d_phi_block
        for num, chan in enumerate(self.measurement_channels[1:]):
            self.connect((self.uhd_app.usrp, chan),
                         (self.phase_diff_calc[num], 1))
        ##############################
        #  Setup TX
        ##############################
        if self.uhd_app.args.tx_channels is not None:
            self.tx_app.args.antenna = self.tx_app.args.tx_antenna
            self.tx_app.args.channels = [
                int(chan.strip())
                for chan in self.tx_app.args.tx_channels.split(',')
            ]
            self.tx_app.usrp = None
            self.log.debug("TX-Setup with args: {}".format(self.tx_app.args))
            self.tx_app.setup_usrp(uhd.usrp_sink, self.tx_app.args)
            self.siggen = analog.sig_source_c(self.samp_rate,
                                              analog.GR_COS_WAVE,
                                              self.tx_app.args.tx_offset, 1.0)
            for chan in range(len(self.tx_app.channels)):
                self.connect((self.siggen, 0), (self.tx_app.usrp, chan))


    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate

    def retune_frequency(self, band_num=1, bands=1):
        ref_chan = self.uhd_app.channels[0]
        freq_range = literal_eval(
            self.uhd_app.usrp.get_freq_range(ref_chan).__str__(
            ))  # returns tuple with (start_freq, end_freq, step)

        if bands > 1:
            bw = (freq_range[1] - freq_range[0])/bands
            freq_range = list(freq_range)
            freq_range[0] = freq_range[0] + ((band_num-1) % bands)*bw
            freq_range[1] = freq_range[0] + bw

        retune_freq = uniform(freq_range[0], freq_range[1])
        self.log.info('tune all channels to: {:f} MHz'.format(retune_freq /
                                                              1e6))
        self.uhd_app.set_freq(retune_freq)
        self.log.info('tune all channels to: {:f} MHz'.format(
            self.uhd_app.args.freq / 1e6))
        self.uhd_app.set_freq(self.uhd_app.args.freq)

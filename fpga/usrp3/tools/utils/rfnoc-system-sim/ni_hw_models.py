#!/usr/bin/env python3
#
# Copyright 2016 Ettus Research
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

import math
import rfnocsim

class UsrpX310(rfnocsim.SimComp):
    # Hardware specific constants
    RADIO_LATENCY = 1e-6
    IO_LATENCY    = 1e-6
    MAX_SAMP_RATE = 300e6   # Limited by 10GbE
    BPI = 4                 # Bytes per sample (item)

    """
    Simulation model for the USRP X310
    - Has two producers and consumers of FFT data
    - Computes bandwidth and latency using FFT size and overlap
    """
    def __init__(self, sim_core, index, app_settings):
        rfnocsim.SimComp.__init__(self, sim_core, name='USRP_%03d' % (index), ctype=rfnocsim.comptype.hardware)
        # USRP i carries data for radio 2i and 2i+1 interleaved into one stream
        self.index = index
        items = [rfnocsim.DataStream.submatrix_gen('rx', [2*index]),
                 rfnocsim.DataStream.submatrix_gen('rx', [2*index+1])]
        # Samples are 4 bytes I and Q
        latency = (self.RADIO_LATENCY + self.IO_LATENCY/2) * self.get_tick_rate()
        if app_settings['domain'] == 'frequency':
            # Max latency per direction depends on the FFT size and sample rate
            latency += self.__get_fft_latency(
                app_settings['fft_size'], app_settings['samp_rate'], self.get_tick_rate())
        # An X310 Radio has two producers (RX data) and consumers (TX data) (i.e. two ethernet ports)
        # Both ports can carry data from both radio frontends
        self.sources = ([
            rfnocsim.Producer(sim_core, self.name + '/TX0', self.BPI, items, self.MAX_SAMP_RATE, latency),
            rfnocsim.Producer(sim_core, self.name + '/TX1', self.BPI, items, self.MAX_SAMP_RATE, latency)])
        self.sinks = ([
            rfnocsim.Consumer(sim_core, self.name + '/RX0', self.BPI * self.MAX_SAMP_RATE, latency),
            rfnocsim.Consumer(sim_core, self.name + '/RX1', self.BPI * self.MAX_SAMP_RATE, latency)])
        # The actual sample rate depends over the wire depends on the radio sample rate,
        # the FFT size and FFT overlap
        for src in self.sources:
            if app_settings['domain'] == 'frequency':
                src.set_rate(app_settings['samp_rate'] *
                    (1.0 + (float(app_settings['fft_overlap'])/app_settings['fft_size'])))
            else:
                src.set_rate(app_settings['samp_rate'])

    def inputs(self, i, bind=False):
        return self.sinks[i].inputs(0, bind)

    def connect(self, i, dest):
        self.sources[i].connect(0, dest)

    def get_utilization(self, what):
        return 0.0

    def get_util_attrs(self):
        return []

    def validate(self, chan):
        recvd = self.sinks[chan].get_items()
        idxs = []
        for i in recvd:
            (str_id, idx) = rfnocsim.DataStream.submatrix_parse(i)
            if str_id != 'tx':
                raise RuntimeError(self.name + ' received incorrect TX data on channel ' + str(chan))
            idxs.append(idx[0][0])
        if sorted(idxs) != [self.index*2, self.index*2 + 1]:
            raise RuntimeError(self.name + ' received incorrect TX data. Got: ' + str(sorted(idxs)))

    def __get_fft_latency(self, fft_size, samp_rate, tick_rate):
        FFT_CLK_RATE = 200e6
        fft_cycles = {128:349, 256:611, 512:1133, 1024:2163, 2048:4221, 4096:8323}
        latency = max(
            fft_cycles[fft_size] / FFT_CLK_RATE,    #Min time to leave FFT
            fft_size / samp_rate)                   #Min time to enter FFT
        return latency * tick_rate


class Bee7Fpga(rfnocsim.SimComp):
    """
    Simulation model for a single Beecube BEE7 FPGA
    - Type = hardware
    - Contains 80 IO lanes per FPGA: 16 each to neighboring
      FPGAs and 32 lanes going outside
    """
    # IO lanes (How the various IO lanes in an FPGA are allocated)
    EW_IO_LANES  = list(range(0,16))
    NS_IO_LANES  = list(range(16,32))
    XX_IO_LANES  = list(range(32,48))
    EXT_IO_LANES = list(range(48,80))
    # External IO lane connections
    FP_BASE  = 0    # Front panel FMC
    FP_LANES = 16
    BP_BASE  = 16   # Backplane RTM
    BP_LANES = 16

    # Hardware specific constants
    IO_LN_LATENCY = 1.5e-6
    IO_LN_BW = 10e9/8
    ELASTIC_BUFF_FULLNESS = 0.5
    BRAM_BYTES = 18e3/8

    def __init__(self, sim_core, name):
        self.sim_core = sim_core
        rfnocsim.SimComp.__init__(self, sim_core, name, rfnocsim.comptype.hardware)
        # Max resources from Virtex7 datasheet
        self.max_resources = rfnocsim.HwRsrcs()
        self.max_resources.add('DSP', 3600)
        self.max_resources.add('BRAM_18kb', 2940)
        self.resources = rfnocsim.HwRsrcs()
        # Each FPGA has 80 SERDES lanes
        self.max_io = 80
        self.serdes_i = dict()
        self.serdes_o = dict()
        # Each lane can carry at most 10GB/s
        # Each SERDES needs to have some buffering. We assume elastic buffering (50% full on avg).
        io_buff_size = (self.IO_LN_BW * self.IO_LN_LATENCY) / self.ELASTIC_BUFF_FULLNESS
        # Worst case lane latency
        lane_latency = self.IO_LN_LATENCY * self.get_tick_rate()
        for i in range(self.max_io):
            self.serdes_i[i] = rfnocsim.Channel(sim_core, self.__ioln_name(i)+'/I', self.IO_LN_BW, lane_latency / 2)
            self.serdes_o[i] = rfnocsim.Channel(sim_core, self.__ioln_name(i)+'/O', self.IO_LN_BW, lane_latency / 2)
            self.resources.add('BRAM_18kb', 1 + math.ceil(io_buff_size / self.BRAM_BYTES))  #input buffering per lane
            self.resources.add('BRAM_18kb', 1)                                          #output buffering per lane
        # Other resources
        self.resources.add('BRAM_18kb', 72)     # BPS infrastructure + microblaze
        self.resources.add('BRAM_18kb', 128)    # 2 MIGs

        self.functions = dict()

    def inputs(self, i, bind=False):
        return self.serdes_i[i].inputs(0, bind)

    def connect(self, i, dest):
        self.serdes_o[i].connect(0, dest)

    def get_utilization(self, what):
        if self.max_resources.get(what) != 0:
            return self.resources.get(what) / self.max_resources.get(what)
        else:
            return 0.0

    def get_util_attrs(self):
        return ['DSP', 'BRAM_18kb']

    def rename(self, name):
        self.name = name

    def add_function(self, func):
        if func.name not in self.functions:
            self.functions[func.name] = func
        else:
            raise RuntimeError('Function ' + self.name + ' already defined in ' + self.name)
        self.resources.merge(func.get_rsrcs())

    def __ioln_name(self, i):
        if i in self.EW_IO_LANES:
            return '%s/SER_EW_%02d'%(self.name,i-self.EW_IO_LANES[0])
        elif i in self.NS_IO_LANES:
            return '%s/SER_NS_%02d'%(self.name,i-self.NS_IO_LANES[0])
        elif i in self.XX_IO_LANES:
            return '%s/SER_XX_%02d'%(self.name,i-self.XX_IO_LANES[0])
        else:
            return '%s/SER_EXT_%02d'%(self.name,i-self.EXT_IO_LANES[0])

class Bee7Blade(rfnocsim.SimComp):
    """
    Simulation model for a single Beecube BEE7
    - Contains 4 FPGAs (fully connected with 16 lanes)
    """
    NUM_FPGAS = 4
    # FPGA positions in the blade
    NW_FPGA = 0
    NE_FPGA = 1
    SW_FPGA = 2
    SE_FPGA = 3

    def __init__(self, sim_core, index):
        self.sim_core = sim_core
        self.name = name='BEE7_%03d' % (index)
        # Add FPGAs
        names = ['FPGA_NW', 'FPGA_NE', 'FPGA_SW', 'FPGA_SE']
        self.fpgas = []
        for i in range(self.NUM_FPGAS):
            self.fpgas.append(Bee7Fpga(sim_core, name + '/' + names[i]))
        # Build a fully connected network of FPGA
        # 4 FPGAs x 3 Links x 2 directions = 12 connections
        self.sim_core.connect_multi_bidir(
            self.fpgas[self.NW_FPGA], Bee7Fpga.EW_IO_LANES, self.fpgas[self.NE_FPGA], Bee7Fpga.EW_IO_LANES)
        self.sim_core.connect_multi_bidir(
            self.fpgas[self.NW_FPGA], Bee7Fpga.NS_IO_LANES, self.fpgas[self.SW_FPGA], Bee7Fpga.NS_IO_LANES)
        self.sim_core.connect_multi_bidir(
            self.fpgas[self.NW_FPGA], Bee7Fpga.XX_IO_LANES, self.fpgas[self.SE_FPGA], Bee7Fpga.XX_IO_LANES)
        self.sim_core.connect_multi_bidir(
            self.fpgas[self.NE_FPGA], Bee7Fpga.XX_IO_LANES, self.fpgas[self.SW_FPGA], Bee7Fpga.XX_IO_LANES)
        self.sim_core.connect_multi_bidir(
            self.fpgas[self.NE_FPGA], Bee7Fpga.NS_IO_LANES, self.fpgas[self.SE_FPGA], Bee7Fpga.NS_IO_LANES)
        self.sim_core.connect_multi_bidir(
            self.fpgas[self.SW_FPGA], Bee7Fpga.EW_IO_LANES, self.fpgas[self.SE_FPGA], Bee7Fpga.EW_IO_LANES)

    def inputs(self, i, bind=False):
        IO_PER_FPGA = len(Bee7Fpga.EXT_IO_LANES)
        return self.fpgas[int(i/IO_PER_FPGA)].inputs(Bee7Fpga.EXT_IO_LANES[i%IO_PER_FPGA], bind)

    def connect(self, i, dest):
        IO_PER_FPGA = len(Bee7Fpga.EXT_IO_LANES)
        self.fpgas[int(i/IO_PER_FPGA)].connect(Bee7Fpga.EXT_IO_LANES[i%IO_PER_FPGA], dest)

    @staticmethod
    def io_lane(fpga, fpga_lane):
        IO_PER_FPGA = len(Bee7Fpga.EXT_IO_LANES)
        return (fpga_lane - Bee7Fpga.EXT_IO_LANES[0]) + (fpga * IO_PER_FPGA)

class ManagementHostandSwitch(rfnocsim.SimComp):
    """
    Simulation model for a management host computer
    - Sources channel coefficients
    - Configures radio
    """
    def __init__(self, sim_core, index, num_coeffs, switch_ports, app_settings):
        rfnocsim.SimComp.__init__(self, sim_core, name='MGMT_HOST_%03d'%(index), ctype=rfnocsim.comptype.other)
        if app_settings['domain'] == 'frequency':
            k = app_settings['fft_size']
        else:
            k = app_settings['fir_taps']

        self.sources = dict()
        self.sinks = dict()
        for l in range(switch_ports):
            self.sources[l] = rfnocsim.Producer(
                sim_core, '%s/COEFF_%d'%(self.name,l), 4, ['coeff_%03d[%d]'%(index,l)], (10e9/8)/switch_ports, 0)
            self.sinks[l] = rfnocsim.Consumer(sim_core, self.name + '%s/ACK%d'%(self.name,l))
            self.sources[l].set_rate(k*num_coeffs*app_settings['coherence_rate'])

    def inputs(self, i, bind=False):
        return self.sinks[i].inputs(0, bind)

    def connect(self, i, dest):
        self.sources[i].connect(0, dest)

    def get_utilization(self, what):
        return 0.0

    def get_util_attrs(self):
        return []

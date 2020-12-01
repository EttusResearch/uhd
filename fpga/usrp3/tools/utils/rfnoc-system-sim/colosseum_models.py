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
import ni_hw_models as hw

class ColGlobals():
    BPI = 4             # Number of bytes per sample or coefficient
    BPP = 1024          # Bytes per packet
    MIN_SAMP_HOPS = 1   # Minimum number of hops an RX sample will take before it is used to compute a PP
    MAX_SAMP_HOPS = 3   # Maximum number of hops an RX sample will take before it is used to compute a PP
    MIN_PP_HOPS = 0     # Minimum number of hops a PP will take before it is used to compute a TX sample
    MAX_PP_HOPS = 1     # Maximum number of hops a PP will take before it is used to compute a TX sample
    ELASTIC_BUFF_FULLNESS = 0.5

class PartialContribComputer(rfnocsim.Function):
    """
    Simulation model for function that computes the contribution of radio chans on other radio chans.
    This function computes a NxM dot product of FFTs, one bin at a time.
    Features:
        - Supports computing the product in multiple cycles (for resource reuse)
        - Supports deinterleaving data in streams (i.e. is Radio 0+1 data comes in thru the same ethernet)

    Args:
        sim_core: Simulator core object
        name: Name of this function
        size: Number of chans (inputs) for which contribution partial products are computed
        fft_size: The length of the FFT in bins
        dst_chans: Computes the contribution of the input chans on these dst_chans
        items_per_stream: How many channels per stream can this function deinterleave?
        ticks_per_exec: How many ticks for the function to generate a full output set
    """
    def __init__(self, sim_core, name, size, dst_chans, items_per_stream, app_settings):
        ticks_per_exec = 1      # This function will run once every tick. No multi-cycle paths here.
        rfnocsim.Function.__init__(self, sim_core, name, size, int(len(dst_chans)/items_per_stream), ticks_per_exec)
        self.items_per_stream = items_per_stream  # Each stream contains data from n radio chans
        self.dst_chans = dst_chans              # Where should the individual products go?
        # This block has to buffer enough data to ensure
        # sample alignment. How deep should those buffers be?
        sync_buff_depth = (((ColGlobals.MAX_SAMP_HOPS - ColGlobals.MIN_SAMP_HOPS) *
            hw.Bee7Fpga.IO_LN_LATENCY * float(app_settings['samp_rate'])) / ColGlobals.ELASTIC_BUFF_FULLNESS)

        # Adder latency: log2(radix) adder stages + 2 pipeline flops
        latency = math.ceil(math.log(size/len(dst_chans), 2)) + 2
        # Synchronization latency based on buffer size
        latency += (sync_buff_depth * ColGlobals.ELASTIC_BUFF_FULLNESS) * (self.get_tick_rate() / float(app_settings['samp_rate']))
        # Packet alignment latency
        latency += ColGlobals.BPP * (self.get_tick_rate() / hw.Bee7Fpga.IO_LN_BW)
        self.estimate_resources(size*items_per_stream, len(dst_chans), app_settings, sync_buff_depth*size, latency)

    def estimate_resources(self, N, M, app_settings, sync_buff_total_samps, pre_filt_latency):
        rscrs = rfnocsim.HwRsrcs()

        DSP_BLOCKS_PER_MAC = 3      # DSP blocks for a scaled complex MAC
        MAX_DSP_RATE = 400e6        # Max clock rate for a DSP48E block
        MAX_UNROLL_DEPTH = 2        # How many taps (or FFT bins) to compute in parallel?
        COEFF_SETS = 1              # We need two copies of coefficients one live
                                    # and one buffered for dynamic reload. If both
                                    # live in BRAM, this should be 2. If the live
                                    # set lives in registers, this should be 1

        samp_rate = float(app_settings['samp_rate'])
        dsp_cyc_per_samp = MAX_DSP_RATE / samp_rate

        if app_settings['domain'] == 'time':
            fir_taps = app_settings['fir_taps']
            if (fir_taps <= dsp_cyc_per_samp):
                unroll_factor = 1
                dsp_rate = samp_rate * fir_taps
            else:
                unroll_factor = math.ceil((1.0 * fir_taps) / dsp_cyc_per_samp)
                dsp_rate = MAX_DSP_RATE
                if (unroll_factor > MAX_UNROLL_DEPTH):
                    raise self.SimCompError('Too many FIR coefficients! Reached loop unroll limit.')

            rscrs.add('DSP', DSP_BLOCKS_PER_MAC * unroll_factor * N * M)
            rscrs.add('BRAM_18kb', math.ceil(ColGlobals.BPI * app_settings['fir_dly_line'] / hw.Bee7Fpga.BRAM_BYTES) * N * M) # FIR delay line memory
            rscrs.add('BRAM_18kb', math.ceil(ColGlobals.BPI * COEFF_SETS * fir_taps * unroll_factor * N * M / hw.Bee7Fpga.BRAM_BYTES))   # Coefficient storage

            samp_per_tick = dsp_rate / self.get_tick_rate()
            self.update_latency(func=pre_filt_latency + (fir_taps / (samp_per_tick * unroll_factor)))
        else:
            fft_size = app_settings['fft_size']
            rscrs.add('DSP', DSP_BLOCKS_PER_MAC * N * M * MAX_UNROLL_DEPTH) # MACs
            rscrs.add('BRAM_18kb', math.ceil(ColGlobals.BPI * N * M * fft_size * COEFF_SETS / hw.Bee7Fpga.BRAM_BYTES)) # Coeff storage

            samp_per_tick = MAX_DSP_RATE / self.get_tick_rate()
            self.update_latency(func=pre_filt_latency + (fft_size / samp_per_tick))

        rscrs.add('BRAM_18kb', math.ceil(ColGlobals.BPI * sync_buff_total_samps / hw.Bee7Fpga.BRAM_BYTES))
        self.update_rsrcs(rscrs)

    def do_func(self, in_data):
        """
        Gather FFT data from "size" channels, compute a dot product with the coeffieicnt
        matrix and spit the partial products out. The dot product is computed for each
        FFT bin serially.
        """
        out_data = list()
        src_chans = []
        # Iterate over each input
        for di in in_data:
            if len(di.items) != self.items_per_stream:
                raise RuntimeError('Incorrect items per stream. Expecting ' + str(self.items_per_stream))
            # Deinterleave data
            for do in range(len(di.items)):
                (sid, coords) = rfnocsim.DataStream.submatrix_parse(di.items[do])
                if sid != 'rx':
                    raise RuntimeError('Incorrect items. Expecting radio data (rx) but got ' + sid)
                src_chans.extend(coords[0])
        bpi = in_data[0].bpi
        count = in_data[0].count
        # Iterate through deinterleaved channels
        for i in range(0, len(self.dst_chans), self.items_per_stream):
            items = []
            for j in range(self.items_per_stream):
                # Compute partial products:
                # pp = partial product of "src_chans" on "self.dst_chans[i+j]"
                items.append(rfnocsim.DataStream.submatrix_gen('pp', [src_chans, self.dst_chans[i+j]]))
            out_data.append(self.create_outdata_stream(bpi, items, count))
        return out_data

class PartialContribCombiner(rfnocsim.Function):
    """
    Simulation model for function that adds multiple partial contributions (products) into a larger
    partial product. The combiner can optionally reduce a very large product into a smaller one.
    Ex: pp[31:0,i] (contribution on chan 0..31 on i) can alias to tx[i] if there are 32 channels.

    Args:
        sim_core: Simulator core object
        name: Name of this function
        radix: Number of partial products that are combined (Number of inputs)
        reducer_filter: A tuple that represents what pp channels to alias to what
        items_per_stream: How many channels per stream can this function deinterleave?
    """

    def __init__(self, sim_core, name, radix, app_settings, reducer_filter = (None, None), items_per_stream = 2):
        rfnocsim.Function.__init__(self, sim_core, name, radix, 1)
        self.radix = radix
        self.reducer_filter = reducer_filter
        self.items_per_stream = items_per_stream

        # This block has to buffer enough data to ensure
        # sample alignment. How deep should those buffers be?
        sync_buff_depth = (((ColGlobals.MAX_PP_HOPS - ColGlobals.MIN_PP_HOPS) *
            hw.Bee7Fpga.IO_LN_LATENCY * float(app_settings['samp_rate'])) / ColGlobals.ELASTIC_BUFF_FULLNESS)
        # Figure out latency based on sync buffer and delay line
        latency = math.ceil(math.log(radix, 2)) + 2     # log2(radix) adder stages + 2 pipeline flops
        # Synchronization latency based on buffer size
        latency += (sync_buff_depth * ColGlobals.ELASTIC_BUFF_FULLNESS) * (self.get_tick_rate() / float(app_settings['samp_rate']))
        # Packet alignment latency
        latency += ColGlobals.BPP * (self.get_tick_rate() / hw.Bee7Fpga.IO_LN_BW)

        self.update_latency(func=latency)
        self.estimate_resources(radix, sync_buff_depth)

    def estimate_resources(self, radix, sync_buff_depth):
        rscrs = rfnocsim.HwRsrcs()
        # Assume that pipelined adders are inferred in logic (not DSP)
        # Assume that buffering uses BRAM
        rscrs.add('BRAM_18kb', math.ceil(ColGlobals.BPI * sync_buff_depth * radix / hw.Bee7Fpga.BRAM_BYTES))
        self.update_rsrcs(rscrs)

    def do_func(self, in_data):
        """
        Gather partial dot products from inputs, add them together and spit them out
        Perform sanity check to ensure that we are adding the correct things
        """
        out_chans = dict()
        # Iterate over each input
        for di in in_data:
            if len(di.items) != self.items_per_stream:
                raise self.SimCompError('Incorrect items per stream. Expecting ' + str(self.items_per_stream))
            # Deinterleave data
            for do in range(len(di.items)):
                (sid, coords) = rfnocsim.DataStream.submatrix_parse(di.items[do])
                if sid == 'null':
                    continue
                elif sid != 'pp':
                    raise self.SimCompError('Incorrect items. Expecting partial produts (pp) but got ' + sid)
                if len(coords[1]) != 1:
                    raise self.SimCompError('Incorrect partial product. Target must be a single channel')
                if coords[1][0] in out_chans:
                    out_chans[coords[1][0]].extend(coords[0])
                else:
                    out_chans[coords[1][0]] = coords[0]
        # Check if keys (targets) for partial products == items_per_stream
        if len(list(out_chans.keys())) != self.items_per_stream:
            raise self.SimCompError('Inconsistent partial products. Too many targets.')
        # Verify that all influencers for each target are consistent
        if not all(x == list(out_chans.values())[0] for x in list(out_chans.values())):
            raise self.SimCompError('Inconsistent partial products. Influencers dont match.')
        contrib_chans = list(out_chans.values())[0]
        # Combine partial products and return
        out_items = []
        for ch in list(out_chans.keys()):
            if sorted(self.reducer_filter[0]) == sorted(contrib_chans):
                out_items.append(rfnocsim.DataStream.submatrix_gen(self.reducer_filter[1], [ch]))
            else:
                out_items.append(rfnocsim.DataStream.submatrix_gen('pp', [list(out_chans.values())[0], ch]))
        return self.create_outdata_stream(in_data[0].bpi, out_items, in_data[0].count)

# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# NOTE: The Torus Topology has not been maintained. Use at your own risk
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
class Topology_2D_4x4_Torus:
    @classmethod
    def config_bitstream(cls, bee7fpga, app_settings, in_chans, out_chans, total_num_chans, is_radio_node):
        if len(in_chans) != 64:
            raise bee7fpga.SimCompError('in_chans must be 64 channels wide. Got ' + str(len(in_chans)))
        if len(out_chans) != 16:
            raise bee7fpga.SimCompError('out_chans must be 16 channels wide. Got ' + str(len(out_chans)))
        GRP_LEN = 16 / 2   # 2 radio channesl per USRP

        # Broadcast raw data streams to all internal and external FPGAs
        for i in range(GRP_LEN):
            in_ln = bee7fpga.EXT_IO_LANES[bee7fpga.BP_BASE+i]
            bee7fpga.sim_core.connect(bee7fpga.serdes_i[in_ln], 0, bee7fpga.serdes_o[bee7fpga.EW_IO_LANES[i]], 0)
            bee7fpga.sim_core.connect(bee7fpga.serdes_i[in_ln], 0, bee7fpga.serdes_o[bee7fpga.NS_IO_LANES[i]], 0)
            bee7fpga.sim_core.connect(bee7fpga.serdes_i[in_ln], 0, bee7fpga.serdes_o[bee7fpga.XX_IO_LANES[i]], 0)
            bee7fpga.sim_core.connect(bee7fpga.serdes_i[in_ln], 0, bee7fpga.serdes_o[bee7fpga.EXT_IO_LANES[bee7fpga.BP_BASE+8+i]], 0)
        # Create an internal bus to hold the generated partial products
        bee7fpga.pp_bus = dict()
        for i in range(GRP_LEN):
            bee7fpga.pp_bus[i] = rfnocsim.Channel(bee7fpga.sim_core, '%s/_INTERNAL_PP_%02d' % (bee7fpga.name,i))
        # We need to compute partial products of the data that is broadcast to us
        # pp_input_lanes represents the IO lanes that hold this data
        pp_input_lanes = bee7fpga.EXT_IO_LANES[bee7fpga.BP_BASE:bee7fpga.BP_BASE+GRP_LEN] + \
            bee7fpga.EW_IO_LANES[0:GRP_LEN] + bee7fpga.NS_IO_LANES[0:GRP_LEN] + bee7fpga.XX_IO_LANES[0:GRP_LEN]
        # The function that computes the partial products
        func = PartialContribComputer(
            sim_core=bee7fpga.sim_core, name=bee7fpga.name + '/pp_computer/', size=len(pp_input_lanes),
            dst_chans=out_chans,
            items_per_stream=2, app_settings=app_settings)
        for i in range(len(pp_input_lanes)):
            bee7fpga.sim_core.connect(bee7fpga.serdes_i[pp_input_lanes[i]], 0, func, i)
        for i in range(GRP_LEN): #Outputs of function
            bee7fpga.sim_core.connect(func, i, bee7fpga.pp_bus[i], 0)
        bee7fpga.add_function(func)
        # Add a function combine all partial products (one per IO lane)
        for i in range(GRP_LEN):
            func = PartialContribCombiner(
                sim_core=bee7fpga.sim_core, name=bee7fpga.name + '/pp_combiner_%d/' % (i),
                radix=2, app_settings=app_settings, reducer_filter=(list(range(total_num_chans)), 'tx'))
            # Partial products generated internally have to be added to a partial
            # sum coming from outside
            bee7fpga.sim_core.connect(bee7fpga.serdes_i[bee7fpga.EXT_IO_LANES[bee7fpga.FP_BASE+i]], 0, func, 0)
            bee7fpga.sim_core.connect(bee7fpga.pp_bus[i], 0, func, 1)
            # If this FPGA is hooked up to the radio then send partial products
            # back to when samples came from. Otherwise send it out to the PP output bus
            if is_radio_node:
                bee7fpga.sim_core.connect(func, 0, bee7fpga.serdes_o[bee7fpga.EXT_IO_LANES[bee7fpga.BP_BASE+i]], 0)
            else:
                bee7fpga.sim_core.connect(func, 0, bee7fpga.serdes_o[bee7fpga.EXT_IO_LANES[bee7fpga.FP_BASE+8+i]], 0)
            bee7fpga.add_function(func)

    @classmethod
    def connect(cls, sim_core, usrps, bee7blades, hosts, app_settings):
        USRPS_PER_BLADE = 32

        # Create NULL source of "zero" partial products
        null_items = ['null[(0);(0)]', 'null[(0);(0)]']
        null_src = rfnocsim.Producer(sim_core, 'NULL_SRC', 4, null_items)
        if app_settings['domain'] == 'frequency':
            null_src.set_rate(app_settings['samp_rate']*(1.0 +
                (float(app_settings['fft_overlap'])/app_settings['fft_size'])))
        else:
            null_src.set_rate(app_settings['samp_rate'])

        # Reshape BEE7s
        # The blades are arranged in 2D Torus network with 4 blades across
        # each dimension (4x4 = 16)
        bee7grid = []
        for r in range(4):
            bee7row = []
            for c in range(4):
                blade = bee7blades[4*r + c]
                pp_chans = list(range(64*c,64*(c+1)))
                for i in range(4):
                    Topology_2D_4x4_Torus.config_bitstream(
                        blade.fpgas[i], app_settings, pp_chans, pp_chans[i*16:(i+1)*16], 256, (r==c))
                bee7row.append(blade)
            bee7grid.append(bee7row)

        # USRP-Bee7 Connections
        # Blades across the diagonal are connected to USRPs
        for b in range(4):
            for u in range(USRPS_PER_BLADE):
                sim_core.connect_bidir(
                    usrps[USRPS_PER_BLADE*b + u], 0, bee7grid[b][b],
                    len(hw.Bee7Fpga.EXT_IO_LANES)*(u/8) + hw.Bee7Fpga.BP_BASE+(u%8), 'SAMP')
            sim_core.connect_bidir(
                hosts[b], 0, bee7grid[b][b], hw.Bee7Fpga.FP_BASE+8, 'CONFIG', ['blue','blue'])

        # Bee7-Bee7 Connections
        null_srcs = []
        for r in range(4):      # Traverse across row
            for c in range(4):  # Traverse across col
                for f in range(4):
                    samp_in_base = len(hw.Bee7Fpga.EXT_IO_LANES)*f + hw.Bee7Fpga.BP_BASE
                    samp_out_base = len(hw.Bee7Fpga.EXT_IO_LANES)*f + hw.Bee7Fpga.BP_BASE+8
                    pp_in_base = len(hw.Bee7Fpga.EXT_IO_LANES)*f + hw.Bee7Fpga.FP_BASE
                    pp_out_base = len(hw.Bee7Fpga.EXT_IO_LANES)*f + hw.Bee7Fpga.FP_BASE+8
                    if r != c:
                        sim_core.connect_multi_bidir(
                            bee7grid[r][(c+3)%4], list(range(samp_out_base,samp_out_base+8)),
                            bee7grid[r][c], list(range(samp_in_base,samp_in_base+8)),
                            'SAMP_O2I', ['black','blue'])
                        sim_core.connect_multi_bidir(
                            bee7grid[r][c], list(range(pp_out_base,pp_out_base+8)),
                            bee7grid[(r+1)%4][c], list(range(pp_in_base,pp_in_base+8)),
                            'PP_O2I', ['black','blue'])
                    else:
                        for i in range(8):
                            sim_core.connect(null_src, 0, bee7grid[(r+1)%4][c], pp_in_base + i)

class Topology_3D_4x4_FLB:
    @classmethod
    def get_radio_num(cls, router_addr, radio_idx, concentration):
        """
        Returns the global radio index given local radio info

        (global_radio_idx) = get_radio_num(router_addr, radio_idx, concentration) where:
        - router_addr: Address of the current FPGA (router) in 3-D space
        - radio_idx: The local index of the radio for the current router_addr
        - concentration: Number of USRPs connected to each router
        """
        DIM_SIZE = 4
        multiplier = concentration
        radio_num = 0
        for dim in ['Z','Y','X']:
            radio_num += router_addr[dim] * multiplier
            multiplier *= DIM_SIZE
        return radio_num + radio_idx

    @classmethod
    def get_portmap(cls, node_addr):
        """
        Returns the router and terminal connections for the current FPGA

        (router_map, terminal_map) = get_portmap(node_addr) where:
        - node_addr: Address of the current FPGA in 3-D space
        - router_map: A double map indexed by the dimension {X,Y,Z} and the
                      FPGA address in that dimension that returns the Aurora
                      lane index that connects the current node to the neighbor.
                      Example: if node_addr = [0,0,0] then router_map['X'][1] will
                      hold the IO lane index that connects the current node with
                      its X-axis neighbor with address 1
        - terminal_map: A single map that maps a dimension {X,Y,Z} to the starting
                        IO lane index for terminals (like USRPs) in that dimension.
                        A terminal is a leaf node in the network.
        """
        router_map = dict()
        terminal_map = dict()
        # If "node_addr" is the address of the current FPGA in the (X,Y,Z) space,
        # then build a list of other addresses (neighbors) in each dimension
        DIM_SIZE = 4
        for dim in ['X','Y','Z']:
            all_addrs = list(range(DIM_SIZE))
            all_addrs.remove(node_addr[dim])
            router_map[dim] = dict()
            for dst in all_addrs:
                router_map[dim][dst] = 0  # Assign lane index as 0 for now
        # Assign Aurora lanes for all external connections between BEE7s
        io_base = hw.Bee7Fpga.EXT_IO_LANES[0]

        # ---- X-axis ----
        # All BEE7s in the X dimension are connected via the RTM
        # The fist quad on the RTM is reserved for SFP+ peripherals like
        # the USRPs, Ethernet switch ports, etc
        # All others are used for inter BEE connections over QSFP+
        terminal_map['X'] = io_base + hw.Bee7Fpga.BP_BASE
        xdst = terminal_map['X'] + DIM_SIZE
        for dst in router_map['X']:
            router_map['X'][dst] = xdst
            xdst += DIM_SIZE

        # ---- Z-axis ----
        # All BEE7s in the Z dimension are connected via FMC IO cards (front panel)
        # To be symmetric with the X-axis the first quad on the FMC bus is also
        # reserved (regardless of all quads being symmetric)
        terminal_map['Z'] = io_base + hw.Bee7Fpga.FP_BASE
        zdst = terminal_map['Z'] + DIM_SIZE
        for dst in router_map['Z']:
            router_map['Z'][dst] = zdst
            zdst += DIM_SIZE

        # ---- Y-axis ----
        # Within a BEE7, FPGAs re connected in the Y-dimension:
        # 0 - 1
        # | X |
        # 2 - 3
        Y_LANE_MAP = {
            0:{1:hw.Bee7Fpga.EW_IO_LANES[0], 2:hw.Bee7Fpga.NS_IO_LANES[0], 3:hw.Bee7Fpga.XX_IO_LANES[0]},
            1:{0:hw.Bee7Fpga.EW_IO_LANES[0], 2:hw.Bee7Fpga.XX_IO_LANES[0], 3:hw.Bee7Fpga.NS_IO_LANES[0]},
            2:{0:hw.Bee7Fpga.NS_IO_LANES[0], 1:hw.Bee7Fpga.XX_IO_LANES[0], 3:hw.Bee7Fpga.EW_IO_LANES[0]},
            3:{0:hw.Bee7Fpga.XX_IO_LANES[0], 1:hw.Bee7Fpga.NS_IO_LANES[0], 2:hw.Bee7Fpga.EW_IO_LANES[0]}}
        for dst in router_map['Y']:
            router_map['Y'][dst] = Y_LANE_MAP[node_addr['Y']][dst]

        return (router_map, terminal_map)

    @classmethod
    def config_bitstream(cls, bee7fpga, app_settings, fpga_addr):
        """
        Defines the FPGA behavior for the current FPGA. This function will make
        create the necessary simulation functions, connect them to IO lanes and
        define the various utilization metrics for the image.

        config_bitstream(bee7fpga, app_settings, fpga_addr):
        - bee7fpga: The FPGA simulation object being configured
        - fpga_addr: Address of the FPGA in 3-D space
        - app_settings: Application information
        """
        if len(fpga_addr) != 3:
            raise bee7fpga.SimCompError('fpga_addr must be 3-dimensional. Got ' + str(len(fpga_addr)))

        # Map that stores lane indices for all neighbors of this node
        (router_map, terminal_map) = cls.get_portmap(fpga_addr)
        # USRPs are connected in the X dimension (RTM) because it has SFP+ ports
        base_usrp_lane = terminal_map['X']

        DIM_WIDTH = 4       # Dimension size for the 3-D network
        MAX_USRPS = 4       # Max USRPs that can possibly be connected to each FPGA
        NUM_USRPS = 2       # Number of USRPs actually connected to each FPGA
        CHANS_PER_USRP = 2  # How many radio channels does each USRP have
        ALL_CHANS = list(range(pow(DIM_WIDTH, 3) * NUM_USRPS * CHANS_PER_USRP))

        # Each FPGA will forward the sample stream from each USRP to all of its
        # X-axis neighbors
        for ri in router_map['X']:
            for li in range(MAX_USRPS):  # li = GT Lane index
                bee7fpga.sim_core.connect(bee7fpga.serdes_i[base_usrp_lane + li], 0, bee7fpga.serdes_o[router_map['X'][ri] + li], 0)

        # Consequently, this FPGA will receive the USRP sample streams from each of
        # its X-axis neighbors. Define an internal bus to aggregate all the neighbor
        # streams with the native ones. Order the streams such that each FPGA sees the
        # same data streams.
        bee7fpga.int_samp_bus = dict()
        for i in range(DIM_WIDTH):
            for li in range(MAX_USRPS):  # li = GT Lane index
                bee7fpga.int_samp_bus[(MAX_USRPS*i) + li] = rfnocsim.Channel(
                    bee7fpga.sim_core, '%s/_INT_SAMP_%02d' % (bee7fpga.name,(MAX_USRPS*i) + li))
                ln_base = base_usrp_lane if i == fpga_addr['X'] else router_map['X'][i]
                bee7fpga.sim_core.connect(bee7fpga.serdes_i[ln_base + li], 0, bee7fpga.int_samp_bus[(MAX_USRPS*i) + li], 0)

        # Forward the X-axis aggregated sample streams to all Y-axis neighbors
        for ri in router_map['Y']:
            for li in range(DIM_WIDTH*DIM_WIDTH):  # li = GT Lane index
                bee7fpga.sim_core.connect(bee7fpga.int_samp_bus[li], 0, bee7fpga.serdes_o[router_map['Y'][ri] + li], 0)

        # What partial products will this FPGA compute?
        # Generate channel list to compute partial products
        pp_chans = list()
        for cg in range(DIM_WIDTH):     # cg = Channel group
            for r in range(NUM_USRPS):
                radio_num = cls.get_radio_num({'X':fpga_addr['X'], 'Y':fpga_addr['Y'], 'Z':cg}, r, NUM_USRPS)
                for ch in range(CHANS_PER_USRP):
                    pp_chans.append(radio_num*CHANS_PER_USRP + ch)

        # Instantiate partial product computer
        bee7fpga.func_pp_comp = PartialContribComputer(
            sim_core=bee7fpga.sim_core, name=bee7fpga.name+'/pp_computer/', size=DIM_WIDTH*DIM_WIDTH*NUM_USRPS,
            dst_chans=pp_chans,
            items_per_stream=CHANS_PER_USRP, app_settings=app_settings)
        bee7fpga.add_function(bee7fpga.func_pp_comp)

        # Partial product computer takes inputs from all Y-axis links
        for sg in range(DIM_WIDTH):     # sg = Group of sexdectects
            for qi in range(DIM_WIDTH):  # qi = GT Quad index
                for li in range(NUM_USRPS):
                    func_inln = (sg * DIM_WIDTH * NUM_USRPS) + (qi * NUM_USRPS) + li
                    if sg == fpga_addr['Y']:
                        bee7fpga.sim_core.connect(bee7fpga.int_samp_bus[(qi * DIM_WIDTH) + li], 0,
                            bee7fpga.func_pp_comp, func_inln)
                    else:
                        bee7fpga.sim_core.connect(bee7fpga.serdes_i[router_map['Y'][sg] + (qi * DIM_WIDTH) + li], 0,
                            bee7fpga.func_pp_comp, func_inln)

        # Internal bus to hold aggregated partial products
        bee7fpga.pp_bus = dict()
        for i in range(DIM_WIDTH*NUM_USRPS):
            bee7fpga.pp_bus[i] = rfnocsim.Channel(bee7fpga.sim_core, '%s/_INT_PP_%02d' % (bee7fpga.name,i))
            bee7fpga.sim_core.connect(bee7fpga.func_pp_comp, i, bee7fpga.pp_bus[i], 0)

        # Forward partial products to Z-axis neighbors
        for ri in router_map['Z']:
            for li in range(NUM_USRPS):  # li = GT Lane index
                bee7fpga.sim_core.connect(bee7fpga.pp_bus[ri*NUM_USRPS + li], 0, bee7fpga.serdes_o[router_map['Z'][ri] + li], 0)

        # Instantiate partial product adder
        bee7fpga.func_pp_comb = dict()
        for i in range(NUM_USRPS):
            bee7fpga.func_pp_comb[i] = PartialContribCombiner(
                sim_core=bee7fpga.sim_core, name=bee7fpga.name + '/pp_combiner_%d/'%(i),
                radix=DIM_WIDTH, app_settings=app_settings, reducer_filter=(ALL_CHANS, 'tx'),
                items_per_stream=CHANS_PER_USRP)
            bee7fpga.add_function(bee7fpga.func_pp_comb[i])

        # Aggregate partial products from Z-axis neighbors
        for u in range(NUM_USRPS):
            for ri in range(DIM_WIDTH):
                if ri in router_map['Z']:
                    bee7fpga.sim_core.connect(bee7fpga.serdes_i[router_map['Z'][ri] + u], 0, bee7fpga.func_pp_comb[u], ri)
                else:
                    bee7fpga.sim_core.connect(bee7fpga.pp_bus[ri*NUM_USRPS + u], 0, bee7fpga.func_pp_comb[u], ri)

        # Instantiate partial product adder
        for u in range(NUM_USRPS):
            bee7fpga.sim_core.connect(bee7fpga.func_pp_comb[u], 0, bee7fpga.serdes_o[base_usrp_lane + u], 0)

        # Coefficient consumer
        bee7fpga.coeff_sink = rfnocsim.Consumer(bee7fpga.sim_core, bee7fpga.name + '/coeff_sink', 10e9/8, 0.0)
        bee7fpga.sim_core.connect(bee7fpga.serdes_i[terminal_map['X'] + NUM_USRPS], 0, bee7fpga.coeff_sink, 0)

    @classmethod
    def connect(cls, sim_core, usrps, bee7blades, hosts, app_settings):
        NUM_USRPS = 2

        # Reshape BEE7s
        # The blades are arranged in 3D Flattened Butterfly configuration
        # with a dimension width of 4. The X and Z dimension represent row, col
        # and the Y dimension represents the internal connections
        bee7grid = []
        for r in range(4):
            bee7row = []
            for c in range(4):
                blade = bee7blades[4*r + c]
                for f in range(blade.NUM_FPGAS):
                    cls.config_bitstream(blade.fpgas[f], app_settings, {'X':r, 'Y':f, 'Z':c})
                bee7row.append(blade)
            bee7grid.append(bee7row)

        # USRP-Bee7 Connections
        # Blades across the diagonal are connected to USRPs
        for x in range(4):
            for y in range(4):
                for z in range(4):
                    for u in range(NUM_USRPS):
                        usrp_num = cls.get_radio_num({'X':x,'Y':y,'Z':z}, u, NUM_USRPS)
                        (router_map, terminal_map) = cls.get_portmap({'X':x,'Y':y,'Z':z})
                        sim_core.connect_bidir(
                            usrps[usrp_num], 0,
                            bee7grid[x][z], hw.Bee7Blade.io_lane(y, terminal_map['X'] + u), 'SAMP')

        # Bee7-Bee7 Connections
        null_srcs = []
        for row in range(4):
            for col in range(4):
                for fpga in range(4):
                    (src_map, t) = cls.get_portmap({'X':row,'Y':fpga,'Z':col})
                    for dst in range(4):
                        if row != dst:
                            (dst_map, t) = cls.get_portmap({'X':dst,'Y':fpga,'Z':col})
                            sim_core.connect_multi(
                                bee7grid[row][col],
                                list(range(hw.Bee7Blade.io_lane(fpga, src_map['X'][dst]), hw.Bee7Blade.io_lane(fpga, src_map['X'][dst]+4))),
                                bee7grid[dst][col],
                                list(range(hw.Bee7Blade.io_lane(fpga, dst_map['X'][row]), hw.Bee7Blade.io_lane(fpga, dst_map['X'][row]+4))),
                                'SAMP')
                        if col != dst:
                            (dst_map, t) = cls.get_portmap({'X':row,'Y':fpga,'Z':dst})
                            sim_core.connect_multi(
                                bee7grid[row][col],
                                list(range(hw.Bee7Blade.io_lane(fpga, src_map['Z'][dst]), hw.Bee7Blade.io_lane(fpga, src_map['Z'][dst]+4))),
                                bee7grid[row][dst],
                                list(range(hw.Bee7Blade.io_lane(fpga, dst_map['Z'][col]), hw.Bee7Blade.io_lane(fpga, dst_map['Z'][col]+4))),
                                'PP', 'blue')

        # Host connection
        for row in range(4):
            for col in range(4):
                for fpga in range(4):
                    (router_map, terminal_map) = cls.get_portmap({'X':row,'Y':row,'Z':col})
                    sim_core.connect_bidir(
                        hosts[row], col*4 + fpga,
                        bee7grid[row][col], hw.Bee7Blade.io_lane(fpga, terminal_map['X'] + NUM_USRPS), 'COEFF', 'red')

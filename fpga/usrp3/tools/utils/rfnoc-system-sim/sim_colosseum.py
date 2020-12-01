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

import argparse
import re
import rfnocsim
import ni_hw_models as hw
import colosseum_models

def main():
    # Arguments
    parser = argparse.ArgumentParser(description='Simulate the Colosseum network')
    parser.add_argument('--topology', type=str, default='flb', choices=['torus','flb'], help='Topology')
    parser.add_argument('--domain', type=str, default='time', choices=['time','frequency'], help='Domain')
    parser.add_argument('--fir_taps', type=int, default=4, help='FIR Filter Taps (Time domain only)')
    parser.add_argument('--fir_dly_line', type=int, default=512, help='FIR Delay Line (Time domain only)')
    parser.add_argument('--fft_size', type=int, default=512, help='FFT Size (Frequency domain only)')
    parser.add_argument('--fft_overlap', type=int, default=256, help='FFT Overlap (Frequency domain only)')
    parser.add_argument('--samp_rate', type=float, default=100e6, help='Radio Channel Sample Rate')
    parser.add_argument('--coherence_rate', type=float, default=1000, help='Channel coefficient update rate')
    args = parser.parse_args()

    sim_core = rfnocsim.SimulatorCore(tick_rate=100e6)
    NUM_USRPS   = 128
    NUM_HOSTS   = 4
    NUM_BLADES  = 16
    NUM_CHANS   = NUM_USRPS * 2

    # Build an application settings structure
    app_settings = dict()
    app_settings['domain'] = args.domain
    app_settings['samp_rate'] = args.samp_rate
    app_settings['coherence_rate'] = args.coherence_rate
    if args.domain == 'frequency':
        app_settings['fft_size'] = args.fft_size
        app_settings['fft_overlap'] = args.fft_overlap
    else:
        app_settings['fir_taps'] = args.fir_taps
        app_settings['fir_dly_line'] = args.fir_dly_line

    print('[INFO] Instantiating hardware resources...')
    # Create USRPs
    usrps = []
    for i in range(NUM_USRPS):
        usrps.append(hw.UsrpX310(sim_core, index=i, app_settings=app_settings))
    # Create BEE7s
    bee7blades = []
    for i in range(NUM_BLADES):
        bee7blades.append(hw.Bee7Blade(sim_core, index=i))
    # Create Management Hosts
    hosts = []
    for i in range(NUM_HOSTS):
        hosts.append(hw.ManagementHostandSwitch(sim_core, index=i,
            num_coeffs=pow(NUM_CHANS,2)/NUM_HOSTS, switch_ports=16, app_settings=app_settings))

    # Build topology
    print('[INFO] Building topology...')
    if args.topology == 'torus':
        colosseum_models.Topology_2D_4x4_Torus.connect(sim_core, usrps, bee7blades, hosts, app_settings)
    elif args.topology == 'flb':
        colosseum_models.Topology_3D_4x4_FLB.connect(sim_core, usrps, bee7blades, hosts, app_settings)
    else:
        raise RuntimeError('Invalid topology: ' + args.topology)

    print('[INFO] Running simulation...')
    sim_core.run(16e-9)

    # Sanity checks
    print('[INFO] Validating correctness...')
    for u in sim_core.list_components(rfnocsim.comptype.hardware, 'USRP.*'):
        sim_core.lookup(u).validate(0)
    print('[INFO] Validating feasibility...')
    for u in sim_core.list_components('', '.*'):
        c = sim_core.lookup(u)
        for a in c.get_util_attrs():
            if c.get_utilization(a) > 1.0:
                print('[WARN] %s: %s overutilized by %.1f%%' % (u,a,(c.get_utilization(a)-1)*100))
    print('[INFO] Validating BEE7 FPGA image IO consistency...')
    master_fpga = 'BEE7_000/FPGA_NE'
    master_stats = dict()
    for u in sim_core.list_components('', master_fpga + '/.*SER_.*'):
        c = sim_core.lookup(u)
        m = re.match('(.+)/(SER_.*)', u)
        master_stats[m.group(2)] = c.get_utilization('bandwidth')
    for ln in master_stats:
        for u in sim_core.list_components('', '.*/' + ln):
            c = sim_core.lookup(u)
            m = re.match('(.+)/(SER_.*)', u)
            if c.get_utilization('bandwidth') != master_stats[ln]:
                print('[WARN] Data flowing over ' + ln + ' is probably different between ' + master_fpga + ' and ' + m.group(1))

    # Visualize various metrics
    vis = rfnocsim.Visualizer(sim_core)
    vis.show_network()
    vis.new_figure([1, 2])
    vis.plot_utilization(rfnocsim.comptype.hardware, 'BEE7.*', 1)
    vis.plot_utilization(rfnocsim.comptype.producer, 'USRP.*', 2)
    vis.show_figure()
    vis.new_figure([1, 2])
    vis.plot_utilization(rfnocsim.comptype.channel, 'BEE7_000.*FPGA_NW.*EXT.*', 1)
    vis.plot_utilization(rfnocsim.comptype.channel, 'BEE7_006.*FPGA_SE.*EXT.*', 2)
    vis.show_figure()
    vis.new_figure([1, 3])
    vis.plot_utilization(rfnocsim.comptype.channel, 'BEE7_010.*FPGA_NW.*SER_EW_.*', 1)
    vis.plot_utilization(rfnocsim.comptype.channel, 'BEE7_010.*FPGA_NW.*SER_NS_.*', 2)
    vis.plot_utilization(rfnocsim.comptype.channel, 'BEE7_010.*FPGA_NW.*SER_XX_.*', 3)
    vis.show_figure()
    vis.new_figure([1, 4])
    vis.plot_utilization(rfnocsim.comptype.channel, 'BEE7_000.*FPGA_NW.*EXT.*', 1)
    vis.plot_utilization(rfnocsim.comptype.channel, 'BEE7_001.*FPGA_NW.*EXT.*', 2)
    vis.plot_utilization(rfnocsim.comptype.channel, 'BEE7_002.*FPGA_NW.*EXT.*', 3)
    vis.plot_utilization(rfnocsim.comptype.channel, 'BEE7_003.*FPGA_NW.*EXT.*', 4)
    vis.show_figure()
    vis.new_figure([1, 4])
    vis.plot_utilization(rfnocsim.comptype.channel, 'BEE7_010.*FPGA_NW.*EXT.*', 1)
    vis.plot_utilization(rfnocsim.comptype.channel, 'BEE7_010.*FPGA_NE.*EXT.*', 2)
    vis.plot_utilization(rfnocsim.comptype.channel, 'BEE7_010.*FPGA_SW.*EXT.*', 3)
    vis.plot_utilization(rfnocsim.comptype.channel, 'BEE7_010.*FPGA_SE.*EXT.*', 4)
    vis.show_figure()
    vis.new_figure([1, 2])
    vis.plot_consumption_latency('.*', '.*USRP_.*', 1)
    vis.plot_path_latency('tx[(0)]', '.*', 2)
    vis.show_figure()
    vis.plot_utilization(rfnocsim.comptype.producer, '.*MGMT_HOST.*')

if __name__ == '__main__':
    main()

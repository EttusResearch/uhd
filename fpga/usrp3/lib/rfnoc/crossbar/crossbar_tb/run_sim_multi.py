#!/usr/bin/python3
#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description
#   Run the crossbar testbench (crossbar_tb) for varios parameter
#   configurations and generates load-latency graphs for each run.

import argparse
import math
import os, sys
import shutil
import glob
import subprocess

g_tb_top_template = """
`timescale 1ns/1ps
module crossbar_tb_auto();
  crossbar_tb #(
    .TEST_NAME          ("crossbar_tb_auto"),
    .ROUTER_IMPL        ("{rtr_impl}"),
    .ROUTER_PORTS       ({rtr_ports}),
    .ROUTER_DWIDTH      ({rtr_width}),
    .MTU_LOG2           ({rtr_mtu}),
    .NUM_MASTERS        ({rtr_sources}),
    .TEST_MAX_PACKETS   ({tst_maxpkts}),
    .TEST_LPP           ({tst_lpp}),
    .TEST_MIN_INJ_RATE  ({tst_injrate_min}),
    .TEST_MAX_INJ_RATE  ({tst_injrate_max}),
    .TEST_INJ_RATE_INCR (10),
    .TEST_GEN_LL_FILES  (1)
  ) impl (
    /* no IO */
  );
endmodule
"""

g_test_params = {
    'data': {'rtr_width':64, 'rtr_mtu':7, 'tst_maxpkts':100, 'tst_lpp':100, 'tst_injrate_min':30, 'tst_injrate_max':100},
    'ctrl': {'rtr_width':64, 'rtr_mtu':5, 'tst_maxpkts':100, 'tst_lpp':20,  'tst_injrate_min':10, 'tst_injrate_max':50},
}

g_xb_types = {
    'chdr_crossbar_nxn':'data', 'axi_crossbar':'data',
    'axis_ctrl_2d_torus':'ctrl', 'axis_ctrl_2d_mesh':'ctrl'
}

def get_options():
    parser = argparse.ArgumentParser(description='Run correctness sim and generate load-latency plots')
    parser.add_argument('--impl', type=str, default='chdr_crossbar_nxn', help='Implementation (CSV) [%s]'%(','.join(g_xb_types.keys())))
    parser.add_argument('--ports', type=str, default='16', help='Number of ports (CSV)')
    parser.add_argument('--sources', type=str, default='16', help='Number of active data sources (masters)')
    return parser.parse_args()

def launch_run(impl, ports, sources):
    run_name = '%s_ports%d_srcs%d'%(impl, ports, sources)
    # Prepare a transform map to autogenerate a TB file
    transform = {'rtr_impl':impl, 'rtr_ports':ports, 'rtr_sources':sources}
    for k,v in g_test_params[g_xb_types[impl]].items():
        transform[k] = v
    # Create crossbar_tb_auto.sv with specified parameters
    with open('crossbar_tb_auto.sv', 'w') as out_file:
        out_file.write(g_tb_top_template.format(**transform))
    # Create data directory for the simulation
    data_dir = os.path.join('data', impl)
    export_dir = os.path.join('data', run_name)
    try:
        os.makedirs('data')
    except FileExistsError:
        pass
    os.makedirs(data_dir)
    os.makedirs(export_dir)
    # Run "make xsim"
    exitcode = subprocess.Popen('make xsim TB_TOP_MODULE=crossbar_tb_auto', shell=True).wait()
    if exitcode != 0:
        raise RuntimeError('Error running "make xsim". Was setupenv.sh run?')
    # Generate load-latency graphs
    exitcode = subprocess.Popen('gen_load_latency_graph.py ' + data_dir, shell=True).wait()
    if exitcode != 0:
        raise RuntimeError('Error running "gen_load_latency_graph.py"')
    # Copy files
    os.rename('xsim.log', os.path.join(export_dir, 'xsim.log'))
    for file in glob.glob(os.path.join(data_dir, '*.png')):
        shutil.copy(file, export_dir)
    # Cleanup outputs
    subprocess.Popen('make cleanall', shell=True).wait()
    try:
        os.remove('crossbar_tb_auto.sv')
    except FileNotFoundError:
        pass
    try:
        shutil.rmtree(data_dir)
    except OSError:
        print('WARNING: Could not delete ' + data_dir)

def main():
    args = get_options();
    for impl in args.impl.strip().split(','):
        for ports in args.ports.strip().split(','):
            for sources in args.sources.strip().split(','):
                launch_run(impl, int(ports), min(int(ports), int(sources)))

if __name__ == '__main__':
    main()

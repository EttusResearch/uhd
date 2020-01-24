#! /usr/bin/python3
#!/usr/bin/python3
#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

import sys, os
import subprocess
import re

def synth_run(modname, keys, transform):
    prefix = modname + '_' + ('_'.join(['%s%s'%(k,transform[k]) for k in keys]))
    print('='*(len(prefix)+2))
    print(' %s '%(prefix))
    print('='*(len(prefix)+2))
    # Write Verilog top-level file
    with open(modname + '_top.v.in', 'r') as in_file:
        with open(modname + '_top.v', 'w') as out_file:
            out_file.write(in_file.read().format(**transform))
    # Run Vivado
    exitcode = subprocess.Popen(
        'vivado -mode tcl -source %s_top.tcl -nolog -nojou'%(modname), shell=True
    ).wait()
    if exitcode != 0:
        raise RuntimeError('Error running vivado. Was setupenv.sh run?')
    # Extract info
    lut = 100.0
    reg = 100.0
    bram = 100.0
    dsp = 100.0
    fmax = 0.0
    with open(modname + '.rpt', 'r') as rpt_file:
        rpt = rpt_file.readlines()
    for line in rpt:
        lm = re.match(r'.*Slice LUTs\*.*\|(.*)\|(.*)\|(.*)\|(.*)\|.*', line)
        if lm is not None:
            lut = float(lm.group(1).strip())
        rm = re.match(r'.*Slice Registers.*\|(.*)\|(.*)\|(.*)\|(.*)\|.*', line)
        if rm is not None:
            reg = float(rm.group(1).strip())
        bm = re.match(r'.*Block RAM Tile.*\|(.*)\|(.*)\|(.*)\|(.*)\|.*', line)
        if bm is not None:
            bram = float(bm.group(1).strip())
        dm = re.match(r'.*DSPs.*\|(.*)\|(.*)\|(.*)\|(.*)\|.*', line)
        if dm is not None:
            dsp = float(dm.group(1).strip())
        tm = re.match(r'.*clk.*\| clk\s*\|(.*)\|.*\|.*\|.*\|.*\|.*\|.*\|.*\|', line)
        if tm is not None:
            fmax = 1000.0/float(tm.group(1).strip())
    # Save report
    os.rename(modname + '.rpt', prefix + '.rpt')
    os.rename(modname + '.dcp', prefix + '.dcp')
    try:
        os.remove(modname + '_top.v')
        os.remove('fsm_encoding.os')
    except FileNotFoundError:
        pass
    # Write summary report line
    res_keys = ['lut','reg','bram','dsp','fmax']
    res = {'lut':lut, 'reg':reg, 'bram':bram, 'dsp':dsp, 'fmax':fmax, 'prefix':prefix}
    if not os.path.exists(modname + '_summary.csv'):
        with open(modname + '_summary.csv', 'w') as summaryf:
            summaryf.write((','.join(keys + res_keys)) + '\n')
    with open(modname + '_summary.csv', 'a') as summaryf:
        summaryf.write((','.join(['%s'%(transform[k]) for k in keys])) + ',' + (','.join(['%.1f'%(res[k]) for k in res_keys])) + '\n')

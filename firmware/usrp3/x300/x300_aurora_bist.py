#!/usr/bin/env python
#
# Copyright 2016 Ettus Research LLC
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

from __future__ import print_function
from builtins import str
from builtins import range

import x300_debug
import argparse
import time
import datetime
import math
import tqdm
import numpy

########################################################################
# constants
########################################################################
SFP0_MAC_REG_BASE       = 0xC000
SFP1_MAC_REG_BASE       = 0xD000
SFP0_TYPE_REG_OFFSET    = 0xA000 + 16
SFP1_TYPE_REG_OFFSET    = 0xA000 + 20
SFP0_STATUS_REG_OFFSET  = 0xA000 + 32
SFP1_STATUS_REG_OFFSET  = 0xA000 + 36

SFP_TYPE_AURORA         = 2

MAC_REG_CTRL            = 0
MAC_REG_STATUS          = 0
MAC_REG_OVERRUNS        = 4
MAC_REG_CHECKSUM_ERRS   = 8
MAC_REG_BIST_SAMPS      = 12
MAC_REG_BIST_ERRORS     = 16

MAC_STATUS_LINK_UP_MSK          = 0x00000001
MAC_STATUS_HARD_ERR_MSK         = 0x00000002
MAC_STATUS_SOFT_ERR_MSK         = 0x00000004
MAC_STATUS_BIST_LOCKED_MSK      = 0x00000008
MAC_STATUS_BIST_LATENCY_MSK     = 0x000FFFF0
MAC_STATUS_BIST_LATENCY_OFFSET  = 4

MAC_CTRL_BIST_CHECKER_EN    = 0x00000001
MAC_CTRL_BIST_GEN_EN        = 0x00000002
MAC_CTRL_BIST_LOOPBACK_EN   = 0x00000004
MAC_CTRL_PHY_RESET          = 0x00000200
MAC_CTRL_BIST_RATE_MSK      = 0x000001F8
MAC_CTRL_BIST_RATE_OFFSET   = 3

BUS_CLK_RATE            = 187.50e6
BIST_MAX_TIME_LIMIT     = math.floor(pow(2,48)/BUS_CLK_RATE)-1

########################################################################
# utils
########################################################################
def get_aurora_info(ctrl):
    if (ctrl.peek(SFP0_TYPE_REG_OFFSET) == SFP_TYPE_AURORA):
        aur_port = 0
    elif (ctrl.peek(SFP1_TYPE_REG_OFFSET) == SFP_TYPE_AURORA):
        aur_port = 1
    else:
        aur_port = -1
    link_up = False
    if aur_port != -1:
        mac_base = SFP0_MAC_REG_BASE if aur_port == 0 else SFP1_MAC_REG_BASE
        link_up = ((ctrl.peek(mac_base + MAC_REG_STATUS) & MAC_STATUS_LINK_UP_MSK) != 0)
    return (aur_port, link_up)

def get_rate_setting(rate):
    RATE_RES_BITS = 6
    rate_sett = int((1.0*rate/(8e-6*BUS_CLK_RATE))*pow(2, RATE_RES_BITS))-1   # Round rate down
    rate_sett = numpy.clip(rate_sett, 0, pow(2, RATE_RES_BITS)-1)
    coerced_rate = ((1.0+rate_sett)/pow(2, RATE_RES_BITS))*(8e-6*BUS_CLK_RATE)
    return (rate_sett, coerced_rate)

def run_ber_loopback_bist(ctrls, duration, rate_tuple):
    (rate_sett, rate) = rate_tuple
    print('[INFO] Running BER Loopback BIST at %.0fMB/s for %.0fs...'%(rate,duration))
    # Determine offsets
    (mst_port, link_up) = get_aurora_info(ctrls['master'])
    MST_MAC_REG_BASE = SFP0_MAC_REG_BASE if (mst_port == 0) else SFP1_MAC_REG_BASE
    if 'slave' in ctrls:
        (sla_port, link_up) = get_aurora_info(ctrls['slave'])
        SLA_MAC_REG_BASE = SFP0_MAC_REG_BASE if (sla_port == 0) else SFP1_MAC_REG_BASE
    # Reset both PHYS
    ctrls['master'].poke(MST_MAC_REG_BASE + MAC_REG_CTRL, MAC_CTRL_PHY_RESET)
    ctrls['master'].poke(MST_MAC_REG_BASE + MAC_REG_CTRL, 0)
    if 'slave' in ctrls:
        ctrls['slave'].poke(SLA_MAC_REG_BASE + MAC_REG_CTRL, MAC_CTRL_PHY_RESET)
        ctrls['slave'].poke(SLA_MAC_REG_BASE + MAC_REG_CTRL, 0)
    time.sleep(1.5)
    # Put the slave in loopback mode and the master in BIST mode
    if 'slave' in ctrls:
        ctrls['slave'].poke(SLA_MAC_REG_BASE + MAC_REG_CTRL, MAC_CTRL_BIST_LOOPBACK_EN)
    master_ctrl = (rate_sett<<MAC_CTRL_BIST_RATE_OFFSET)|MAC_CTRL_BIST_GEN_EN|MAC_CTRL_BIST_CHECKER_EN
    ctrls['master'].poke(MST_MAC_REG_BASE + MAC_REG_CTRL, master_ctrl)
    start_time = datetime.datetime.now()
    # Wait and check if BIST locked
    time.sleep(0.5)
    mst_status = ctrls['master'].peek(MST_MAC_REG_BASE + MAC_REG_STATUS)
    if (not (mst_status & MAC_STATUS_BIST_LOCKED_MSK)):
        print('[ERROR] BIST engine did not lock onto a PRBS word!')
    # Wait for requested time
    try:
        for i in tqdm.tqdm(list(range(duration)), desc='[INFO] Progress'):
            time.sleep(1.0)
    except KeyboardInterrupt:
        print('[WARNING] Operation cancelled by user.')
    # Turn off the BIST generator and loopback
    ctrls['master'].poke(MST_MAC_REG_BASE + MAC_REG_CTRL, MAC_CTRL_BIST_CHECKER_EN)
    stop_time = datetime.datetime.now()
    time.sleep(0.5)
    if 'slave' in ctrls:
        ctrls['slave'].poke(SLA_MAC_REG_BASE + MAC_REG_CTRL, 0)
    # Validate status and no overruns
    mst_status = ctrls['master'].peek(MST_MAC_REG_BASE + MAC_REG_STATUS)
    mst_overruns = ctrls['master'].peek(MST_MAC_REG_BASE + MAC_REG_OVERRUNS)
    if (mst_status & MAC_STATUS_HARD_ERR_MSK):
        print('[ERROR] Hard errors in master PHY')
    if (mst_overruns > 0):
        print('[ERROR] Buffer overruns in master PHY')
    if 'slave' in ctrls:
        sla_status = ctrls['slave'].peek(SLA_MAC_REG_BASE + MAC_REG_STATUS)
        sla_overruns = ctrls['slave'].peek(SLA_MAC_REG_BASE + MAC_REG_OVERRUNS)
        if (sla_status & MAC_STATUS_HARD_ERR_MSK):
            print('[ERROR] Hard errors in slave PHY')
        if (sla_overruns > 0):
            print('[ERROR] Buffer overruns in slave PHY')
    # Compure latency
    mst_samps = 65536.0*ctrls['master'].peek(MST_MAC_REG_BASE + MAC_REG_BIST_SAMPS)
    mst_errors = 1.0*ctrls['master'].peek(MST_MAC_REG_BASE + MAC_REG_BIST_ERRORS)
    if (mst_samps != 0):
        mst_latency_cyc = 16.0*((mst_status & MAC_STATUS_BIST_LATENCY_MSK) >> MAC_STATUS_BIST_LATENCY_OFFSET)
        time_diff = stop_time - start_time
        print('[INFO] BIST Complete!')
        print('- Elapsed Time              = ' + str(time_diff))
        print('- Max BER (Bit Error Ratio) = %.4g (%d errors out of %d)'%((mst_errors+1)/mst_samps,mst_errors,mst_samps))
        print('- Max Roundtrip Latency     = %.1fus'%(1e6*mst_latency_cyc/BUS_CLK_RATE))
        print('- Approx Throughput         = %.0fMB/s'%((8e-6*mst_samps)/time_diff.total_seconds()))
    else:
        print('[ERROR] BIST Failed!')
    # Drain and Cleanup
    print('[INFO] Cleaning up...')
    ctrls['master'].poke(MST_MAC_REG_BASE + MAC_REG_CTRL, MAC_CTRL_BIST_CHECKER_EN)
    if 'slave' in ctrls:
        ctrls['slave'].poke(SLA_MAC_REG_BASE + MAC_REG_CTRL, MAC_CTRL_BIST_CHECKER_EN)
    time.sleep(0.5)
    ctrls['master'].poke(MST_MAC_REG_BASE + MAC_REG_CTRL, 0)
    if 'slave' in ctrls:
        ctrls['slave'].poke(SLA_MAC_REG_BASE + MAC_REG_CTRL, 0)

def run_latency_loopback_bist(ctrls, duration, rate_tuple):
    (rate_sett, rate) = rate_tuple
    print('[INFO] Running Latency Loopback BIST at %.0fMB/s for %.0fs...'%(rate,duration))
    # Determine offsets
    (mst_port, link_up) = get_aurora_info(ctrls['master'])
    MST_MAC_REG_BASE = SFP0_MAC_REG_BASE if (mst_port == 0) else SFP1_MAC_REG_BASE
    if 'slave' in ctrls:
        (sla_port, link_up) = get_aurora_info(ctrls['slave'])
        SLA_MAC_REG_BASE = SFP0_MAC_REG_BASE if (sla_port == 0) else SFP1_MAC_REG_BASE
    # Reset both PHYS
    ctrls['master'].poke(MST_MAC_REG_BASE + MAC_REG_CTRL, MAC_CTRL_PHY_RESET)
    ctrls['master'].poke(MST_MAC_REG_BASE + MAC_REG_CTRL, 0)
    if 'slave' in ctrls:
        ctrls['slave'].poke(SLA_MAC_REG_BASE + MAC_REG_CTRL, MAC_CTRL_PHY_RESET)
        ctrls['slave'].poke(SLA_MAC_REG_BASE + MAC_REG_CTRL, 0)
    time.sleep(1.5)

    # Put the slave in loopback mode and the master in BIST mode
    if 'slave' in ctrls:
        ctrls['slave'].poke(SLA_MAC_REG_BASE + MAC_REG_CTRL, MAC_CTRL_BIST_LOOPBACK_EN)
    master_ctrl = (rate_sett<<MAC_CTRL_BIST_RATE_OFFSET)|MAC_CTRL_BIST_GEN_EN|MAC_CTRL_BIST_CHECKER_EN

    start_time = datetime.datetime.now()
    latencies = []
    mst_lock_errors = 0
    mst_hard_errors = 0
    mst_overruns = 0
    try:
        for i in tqdm.tqdm(list(range(duration*10)), desc='[INFO] Progress'):
            ctrls['master'].poke(MST_MAC_REG_BASE + MAC_REG_CTRL, master_ctrl)
            # Wait and check if BIST locked
            time.sleep(0.05)
            mst_status = ctrls['master'].peek(MST_MAC_REG_BASE + MAC_REG_STATUS)
            if (not (mst_status & MAC_STATUS_BIST_LOCKED_MSK)):
                mst_lock_errors += 1
            # Turn off the BIST generator
            ctrls['master'].poke(MST_MAC_REG_BASE + MAC_REG_CTRL, MAC_CTRL_BIST_CHECKER_EN)
            # Validate status and no overruns
            mst_status = ctrls['master'].peek(MST_MAC_REG_BASE + MAC_REG_STATUS)
            mst_overruns += ctrls['master'].peek(MST_MAC_REG_BASE + MAC_REG_OVERRUNS)
            if (mst_status & MAC_STATUS_HARD_ERR_MSK):
                mst_hard_errors += 1
            time.sleep(0.05)
            ctrls['master'].poke(MST_MAC_REG_BASE + MAC_REG_CTRL, 0)
            # Compure latency
            mst_latency_cyc = 16.0*((mst_status & MAC_STATUS_BIST_LATENCY_MSK) >> MAC_STATUS_BIST_LATENCY_OFFSET)
            mst_latency_us = 1e6*mst_latency_cyc/BUS_CLK_RATE
            latencies.append(mst_latency_us)
    except KeyboardInterrupt:
        print('[WARNING] Operation cancelled by user.')
    stop_time = datetime.datetime.now()
    # Report
    if (mst_lock_errors > 0):
        print('[ERROR] BIST engine did not lock onto a PRBS word %d times!'%(mst_lock_errors))
    if (mst_hard_errors > 0):
        print('[ERROR] There were %d hard errors in master PHY'%(mst_hard_errors))
    if (mst_overruns > 0):
        print('[ERROR] There were %d buffer overruns in master PHY'%(mst_overruns))

    print('[INFO] BIST Complete!')
    print('- Elapsed Time               = ' + str(stop_time - start_time))
    print('- Roundtrip Latency Mean     = %.2fus'%(numpy.mean(latencies)))
    print('- Roundtrip Latency Stdev    = %.2fus'%(numpy.std(latencies)))
    # Turn off BIST loopback
    time.sleep(0.5)
    if 'slave' in ctrls:
        ctrls['slave'].poke(SLA_MAC_REG_BASE + MAC_REG_CTRL, 0)
    if 'slave' in ctrls:
        sla_status = ctrls['slave'].peek(SLA_MAC_REG_BASE + MAC_REG_STATUS)
        sla_overruns = ctrls['slave'].peek(SLA_MAC_REG_BASE + MAC_REG_OVERRUNS)
        if (sla_status & MAC_STATUS_HARD_ERR_MSK):
            print('[ERROR] Hard errors in slave PHY')
        if (sla_overruns > 0):
            print('[ERROR] Buffer overruns in slave PHY')
    # Drain and Cleanup
    print('[INFO] Cleaning up...')
    ctrls['master'].poke(MST_MAC_REG_BASE + MAC_REG_CTRL, MAC_CTRL_BIST_CHECKER_EN)
    if 'slave' in ctrls:
        ctrls['slave'].poke(SLA_MAC_REG_BASE + MAC_REG_CTRL, MAC_CTRL_BIST_CHECKER_EN)
    time.sleep(0.5)
    ctrls['master'].poke(MST_MAC_REG_BASE + MAC_REG_CTRL, 0)
    if 'slave' in ctrls:
        ctrls['slave'].poke(SLA_MAC_REG_BASE + MAC_REG_CTRL, 0)

########################################################################
# command line options
########################################################################
def get_options():
    parser = argparse.ArgumentParser(description='Controller for the USRP X3X0 Aurora BIST Engine')
    parser.add_argument('--master', type=str, default=None, required=True, help='IP Address of master USRP-X3X0 device')
    parser.add_argument('--slave', type=str, default=None, help='IP Address of slave USRP-X3X0 device')
    parser.add_argument('--test', type=str, default='ber', choices=['ber', 'latency'], help='Type of test to run')
    parser.add_argument('--duration', type=int, default=10, help='Duration of test in seconds')
    parser.add_argument('--rate', type=int, default=1245, help='BIST throughput in MB/s')
    return parser.parse_args()

########################################################################
# main
########################################################################
if __name__=='__main__':
    options = get_options()

    if (options.duration < 0 or options.duration > BIST_MAX_TIME_LIMIT):
        raise Exception('Invalid duration. Min = 0s and Max = %ds'%(BIST_MAX_TIME_LIMIT))

    ctrls = dict()
    ctrls['master'] = x300_debug.ctrl_socket(addr=options.master)
    if options.slave:
        ctrls['slave'] = x300_debug.ctrl_socket(addr=options.slave)

    # Report device and core info
    links_up = True
    for node in ctrls:
        print('[INFO] ' + node.upper() + ':')
        ctrl = ctrls[node]
        (aur_port, link_up) = get_aurora_info(ctrl)
        if aur_port >= 0:
            status_str = str(aur_port) + (' UP' if link_up else ' DOWN')
        else:
            status_str = 'Not Detected!'
        print('- Mgmt IP Addr  : ' + (options.master if node == 'master' else options.slave))
        print('- Aurora Status : Port ' + status_str)
        links_up = links_up & link_up

    # Sanity check
    if not links_up:
        print('[ERROR] At least one of the links is down. Cannot proceed.')
        exit(1)

    # Run BIST
    if options.test == 'ber':
        run_ber_loopback_bist(ctrls, options.duration, get_rate_setting(options.rate))
    else:
        run_latency_loopback_bist(ctrls, options.duration, get_rate_setting(options.rate))

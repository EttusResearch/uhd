from __future__ import print_function
from builtins import str
from builtins import range
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

import x300_debug
import argparse
import time
import datetime
import math
import tqdm

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
MAC_REG_BIST_SAMPS      = 8
MAC_REG_BIST_ERRORS     = 12

MAC_STATUS_LINK_UP_MSK          = 0x00000001
MAC_STATUS_HARD_ERR_MSK         = 0x00000002
MAC_STATUS_SOFT_ERR_MSK         = 0x00000004
MAC_STATUS_BIST_LOCKED_MSK      = 0x00000008
MAC_STATUS_BIST_LATENCY_MSK     = 0x000FFFF0
MAC_STATUS_BIST_LATENCY_OFFSET  = 4
MAC_STATUS_CHECKSUM_ERRS_MSK    = 0xFFF00000
MAC_STATUS_CHECKSUM_ERRS_OFFSET = 20

MAC_CTRL_BIST_CHECKER_EN    = 0x00000001
MAC_CTRL_BIST_GEN_EN        = 0x00000002
MAC_CTRL_BIST_LOOPBACK_EN   = 0x00000004
MAC_CTRL_PHY_RESET          = 0x00000100
MAC_CTRL_BIST_RATE_MSK      = 0x000000F8
MAC_CTRL_BIST_RATE_OFFSET   = 3

AURORA_CLK_RATE         = 156.25e6
BUS_CLK_RATE            = 166.66e6
BIST_MAX_TIME_LIMIT     = math.floor(pow(2,48)/AURORA_CLK_RATE)-1

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
    for div in range(2,32):
        if (rate < 8e-6 * BUS_CLK_RATE * (1.0 - 1.0/div)):
            return (div-1, 8e-6 * BUS_CLK_RATE * (1.0 - 1.0/(div-1)))
    return (0, 8e-6 * BUS_CLK_RATE)

def run_loopback_bist(ctrls, duration, xxx_todo_changeme):
    (rate_sett, rate) = xxx_todo_changeme
    print('[INFO] Running Loopback BIST at %.0fMB/s for %.0fs...'%(rate,duration))
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
    if rate_sett == 0:
        master_ctrl = MAC_CTRL_BIST_GEN_EN|MAC_CTRL_BIST_CHECKER_EN
    else:
        master_ctrl = ((rate_sett - 1) << MAC_CTRL_BIST_RATE_OFFSET)|MAC_CTRL_BIST_GEN_EN|MAC_CTRL_BIST_CHECKER_EN
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
        print('- Max Roundtrip Latency     = %.1fus'%(1e6*mst_latency_cyc/AURORA_CLK_RATE))
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

########################################################################
# command line options
########################################################################
def get_options():
    parser = argparse.ArgumentParser(description='Controller for the USRP X3X0 Aurora BIST Engine')
    parser.add_argument('--master', type=str, default=None, required=True, help='IP Address of master USRP-X3X0 device')
    parser.add_argument('--slave', type=str, default=None, help='IP Address of slave USRP-X3X0 device')
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
    run_loopback_bist(ctrls, options.duration, get_rate_setting(options.rate))

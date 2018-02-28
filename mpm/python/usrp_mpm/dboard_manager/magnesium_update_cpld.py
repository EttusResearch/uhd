#!/usr/bin/env python3
#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Update the CPLD image for the N310
"""
import os
import argparse
import subprocess
import pyudev
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.periph_manager.n3xx import MboardRegsControl

OPENOCD_DIR = "/usr/share/openocd/scripts"
CONFIGS = {
    'sysfsgpio' : {
        'files'  : ["interface/sysfsgpio-ettus-magnesium-dba.cfg",
                    "interface/sysfsgpio-ettus-magnesium-dbb.cfg",
                    "cpld/altera-5m570z-cpld.cfg"],
        'cmd'  : "init; svf -tap 5m570z.tap %s -progress -quiet;exit"
    },
    'axi_bitq' : {
        'files' : ["cpld/altera-5m570z-cpld.cfg"],
        'cmd' : ["interface axi_bitq; axi_bitq_config %u %u; adapter_khz %u",
                 "init; svf -tap 5m570z.tap %s -progress -quiet;exit"]
    }
}

AXI_BITQ_ADAPTER_SPEED = 8000
AXI_BITQ_BUS_CLK = 40000000

def check_openocd_files(files, logger=None):
    """
    Check if all file required by OpenOCD exist
    :param logger: logger object
    """
    for ocd_file in files:
        if not os.path.exists(os.path.join(OPENOCD_DIR, ocd_file)):
            if logger is not None:
                logger.error("Missing file %s" % os.path.join(OPENOCD_DIR, ocd_file))
            return False
    return True


def check_fpga_state(which=0):
    """
    Check if the FPGA is operational
    :param which: the FPGA to check
    """
    logger = get_logger('update_cpld')
    try:
        context = pyudev.Context()
        fpga_mgrs = [dev for dev in context.list_devices(subsystem="fpga_manager")]
        if fpga_mgrs:
            state = fpga_mgrs[which].attributes.asstring('state')
            logger.trace("FPGA State: {}".format(state))
            return state == "operating"
    except OSError as ex:
        logger.error("Error while checking FPGA status: {}".format(ex))
        return False

def find_axi_bitq_uio(dboard):
    """
    Find the AXI Bitq UIO device
    :param dboard: the dboard, can be either 0 or 1
    """
    assert dboard < 2 and dboard >= 0
    label = 'dboard-jtag-%u' % dboard

    logger = get_logger('update_cpld')

    try:
        context = pyudev.Context()
        uios = [dev for dev in context.list_devices(subsystem="uio")]
        for uio in uios:
            uio_label = uio.attributes.asstring('maps/map0/name')
            logger.trace("UIO label: {}, match: {} number: {}".format(
                uio_label, uio_label == label, uio.sys_number))
            if uio_label == label:
                return int(uio.sys_number)
    except OSError as ex:
        logger.error("Error while looking for axi_bitq uio nodes: {}".format(ex))
        return -1

def do_update_cpld(filename, daughterboards):
    """
    Carry out update process for the CPLD
    :param filename: path (on device) to the new CPLD image
    :param daughterboards: iterable containing dboard numbers to update
    :return: True on success, False otherwise
    """
    logger = get_logger('update_cpld')
    logger.info("Programming CPLD of dboards {} with image {}".format(daughterboards, filename))

    if not daughterboards:
        logger.error("Invalid daughterboard selection.")
        return False

    if not os.path.exists(filename):
        logger.error("CPLD image file {} not found".format(filename))
        return False

    if not check_fpga_state():
        logger.error("CPLD lines are routed through fabric, FPGA is not programmed, giving up")
        return False

    regs = MboardRegsControl('mboard-regs', logger)
    compat_maj = regs.get_compat_number()[0]
    mode = 'axi_bitq' if compat_maj > 3 else 'sysfsgpio'

    logger.info("FPGA has compatibilty number {} using {}".format(compat_maj, mode))

    config = CONFIGS[mode]

    if check_openocd_files(config['files'], logger=logger):
        logger.trace("Found required OpenOCD files.")
    else:
        # check_openocd_files logs errors
        return False

    for dboard in daughterboards:
        logger.info("Updating daughterboard slot {}...".format(dboard))

        if mode == 'sysfsgpio':
            cmd = ["openocd",
                   "-f", (config['files'][int(dboard, 10)]).strip(),
                   "-f", (config['files'][2]).strip(),
                   "-c", config['cmd'] % filename]
        else:
            uio_id = find_axi_bitq_uio(int(dboard, 10))
            if uio_id < 0:
                logger.error("Failed to find axi_bitq uio devices, \
                             make sure overlays are up to date")
                continue
            cmd = ["openocd",
                   "-c", config['cmd'][0] % (uio_id, AXI_BITQ_BUS_CLK, AXI_BITQ_ADAPTER_SPEED),
                   "-f", (config['files'][0]).strip(),
                   "-c", config['cmd'][1] % filename]

        logger.trace("Update CPLD CMD: {}".format(" ".join(cmd)))
        subprocess.call(cmd)

    logger.trace("Done programming CPLD...")
    return True


def main():
    """
    Go, go, go!
    """
    # We need to make a logger if we're running stand-alone
    from usrp_mpm.mpmlog import get_main_logger
    log = get_main_logger()

    # Do some setup
    def parse_args():
        """Parse the command-line arguments"""
        parser = argparse.ArgumentParser(description='Update the CPLD image on NI Magnesium')
        parser.add_argument("--file", help="Filename of CPLD image",
                            default="/lib/firmware/ni/cpld-magnesium-revc.svf")
        parser.add_argument("--dboards", help="Slot name to program", default="0,1")
        return parser.parse_args()

    args = parse_args()
    dboards = args.dboards.split(",")
    if any([x not in ('0', '1') for x in dboards]):
        log.error("Unsupported dboards requested: %s", dboards)
        return False

    return do_update_cpld(args.file, dboards)


if __name__ == "__main__":
    exit(not main())

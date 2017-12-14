#!/usr/bin/env python3
#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#
"""
Update the CPLD image for the N310
"""
import os
import argparse
import subprocess
import pyudev
from usrp_mpm.mpmlog import get_logger


OPENOCD_DIR = "/usr/share/openocd/scripts"
REQUIRED_FILES = ["interface/sysfsgpio-ettus-magnesium-dba.cfg",
                  "interface/sysfsgpio-ettus-magnesium-dbb.cfg",
                  "cpld/altera-5m570z-cpld.cfg"]
OPENOCD_CMD = "init; svf -tap 5m570z.tap %s -progress -quiet;exit"


def check_openocd_files(logger=None):
    """
    Check if all file required by OpenOCD exist
    :param logger: logger object
    """
    for ocd_file in REQUIRED_FILES:
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


def do_update_cpld(filename, daughterboards, force=False):
    """
    Carry out update process for the CPLD
    :param filename: path (on device) to the new CPLD image
    :param daughterboards: iterable containing dboard numbers to update
    :param force: bool, ignore issues during update process
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

    # If we don't want to force this, check that the FPGA is operational
    if not force and not check_fpga_state():
        logger.error("CPLD lines are routed through fabric, FPGA is not programmed, giving up")
        return False

    if check_openocd_files(logger=logger):
        logger.trace("Found required OpenOCD files.")
    else:
        # check_openocd_files logs errors
        return False

    for dboard in daughterboards:
        logger.info("Updating daughterboard slot %d...", dboard)
        cmd = ["openocd",
               "-f", (REQUIRED_FILES[int(dboard, 10)]).strip(),
               "-f", (REQUIRED_FILES[2]).strip(),
               "-c", OPENOCD_CMD % filename]
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
        parser.add_argument("--force", help="Ignore issues and move on",
                            default=False, action="store_true")
        return parser.parse_args()

    args = parse_args()
    dboards = args.dboards.split(",")
    if any([x not in ('0', '1') for x in dboards]):
        log.error("Unsupported dboards requested: {}".format(dboards))
        return False
    else:
        return do_update_cpld(args.file, dboards, args.force)

if __name__ == "__main__":
    exit(not main())

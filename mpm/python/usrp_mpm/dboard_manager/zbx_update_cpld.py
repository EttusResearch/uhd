#!/usr/bin/env python3
#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Update the CPLD image for a ZBX daughterboard
"""

import sys
import os
import argparse
import subprocess
import pyudev
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.mpmutils import check_fpga_state
from usrp_mpm.sys_utils.sysfs_gpio import GPIOBank
from usrp_mpm.periph_manager.x4xx_periphs import CtrlportRegs
from usrp_mpm.periph_manager.x4xx_mb_cpld import MboardCPLD
from usrp_mpm.chips.max10_cpld_flash_ctrl import Max10CpldFlashCtrl
from usrp_mpm.sys_utils.udev import dt_symbol_get_spidev

OPENOCD_DIR = "/usr/share/openocd/scripts"
CONFIGS = {
    'axi_bitq' : {
        'files' : ["fpga/altera-10m50.cfg"],
        'cmd' : ["interface axi_bitq; axi_bitq_config %u %u %u; adapter_khz %u",
                 "init; svf -tap 10m50.tap %s -progress -quiet;exit"]
    }
}

AXI_BITQ_ADAPTER_SPEED = 5000
AXI_BITQ_BUS_CLK = 50000000

#The offsets are for JTAG_DB0 and JTAG_DB1 on the motherboard CPLD
DAUGHTERBOARD0_OFFSET = CtrlportRegs.MB_PL_CPLD + 0x60
DAUGHTERBOARD1_OFFSET = CtrlportRegs.MB_PL_CPLD + 0x80

# ZBX flash reconfiguration engine specific offsets
RECONFIG_ENGINE_OFFSET = 0x20
CPLD_MIN_REVISION      = 0x20052016

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

def find_offset(dboard):
    """
    Find the AXI Bitq UIO device
    :param dboard: the dboard, can be either 0 or 1
    """
    assert dboard in (0, 1)
    return DAUGHTERBOARD0_OFFSET if dboard == 0 else DAUGHTERBOARD1_OFFSET

def find_axi_bitq_uio():
    """
    Find the AXI Bitq UIO device
    """
    label = 'ctrlport-mboard-regs'

    logger = get_logger('update_cpld')

    try:
        context = pyudev.Context()
        for uio in context.list_devices(subsystem="uio"):
            uio_label = uio.attributes.asstring('maps/map0/name')
            logger.trace("UIO label: {}, match: {} number: {}".format(
                uio_label, uio_label == label, uio.sys_number))
            if uio_label == label:
                return int(uio.sys_number)
        return None
    except OSError as ex:
        logger.error("Error while looking for axi_bitq uio nodes: {}".format(ex))
        return None

def do_update_cpld(filename, daughterboards, updater_mode):
    """
    Carry out update process for the CPLD
    :param filename: path (on device) to the new CPLD image
    :param daughterboards: iterable containing dboard numbers to update
    :param updater_mode: the updater method to use- Either flash or legacy (JTAG)
    :return: True on success, False otherwise
    """
    assert updater_mode in ('flash', 'legacy'), \
        f"Invalid updater method {updater_mode} given"
    logger = get_logger('update_cpld')
    logger.info("Programming CPLD of dboards {} with image {} using {} mode"
                .format(daughterboards, filename, updater_mode))

    if not daughterboards:
        logger.error("Invalid daughterboard selection.")
        return False

    if not os.path.exists(filename):
        logger.error("CPLD image file {} not found".format(filename))
        return False

    if not check_fpga_state(logger=logger):
        logger.error("CPLD lines are routed through fabric, FPGA is not programmed, giving up")
        return False

    if updater_mode == 'legacy':
        return jtag_cpld_update(filename, daughterboards, logger)
    # updater_mode == flash:
    for dboard in daughterboards:
        dboard = int(dboard, 10)
        logger.info("Updating daughterboard slot {}...".format(dboard))
        # enable required daughterboard clock
        cpld_spi_node = dt_symbol_get_spidev('mb_cpld')
        cpld_control = MboardCPLD(cpld_spi_node, logger)
        cpld_control.enable_daughterboard_support_clock(dboard, enable=True)
        # setup flash configuration engine and required register access
        label = "ctrlport-mboard-regs"
        ctrlport_regs = CtrlportRegs(label, logger)
        regs = ctrlport_regs.get_db_cpld_iface(dboard)
        flash_control = Max10CpldFlashCtrl(
            logger, regs, RECONFIG_ENGINE_OFFSET, CPLD_MIN_REVISION)
        success = flash_control.update(filename)
        # disable clock
        cpld_control.enable_daughterboard_support_clock(dboard, enable=False)
        if not success:
            return success
    return True

def jtag_cpld_update(filename, daughterboards, logger=None):
    """
    Carry out update process for the CPLD
    :param filename: path (on device) to the new CPLD image
    :param daughterboards: iterable containing dboard numbers to update
    :return: True on success, False otherwise
    """
    mode = 'axi_bitq'
    config = CONFIGS[mode]

    if check_openocd_files(config['files'], logger=logger):
        logger.trace("Found required OpenOCD files.")
    else:
        # check_openocd_files logs errors
        return False

    for dboard in daughterboards:
        logger.info("Updating daughterboard slot {}...".format(dboard))

        uio_id = find_axi_bitq_uio()
        offset = find_offset(int(dboard, 10))
        if uio_id is None or uio_id < 0:
            logger.error('Failed to find axi_bitq uio devices. '\
                        'Make sure overlays are up to date')
            return False

        cmd = [
            "openocd",
            "-c", config['cmd'][0] % (uio_id, AXI_BITQ_BUS_CLK, offset, AXI_BITQ_ADAPTER_SPEED),
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
    # Do some setup
    def parse_args():
        """Parse the command-line arguments"""
        parser = argparse.ArgumentParser(description='Update the CPLD image on ZBX daughterboard')
        parser.add_argument("--file", help="Filename of CPLD image",
                            default="/lib/firmware/ni/cpld-zbx.rpd")
        parser.add_argument("--dboards", help="Slot name to program", default="0,1")
        parser.add_argument("--updater",
                            help="The image updater method to use, either "
                                 " 'legacy' (uses openocd) or 'flash'",
                            default="flash")
        parser.add_argument(
            '-v',
            '--verbose',
            help="Increase verbosity level",
            action="count",
            default=1
        )
        parser.add_argument(
            '-q',
            '--quiet',
            help="Decrease verbosity level",
            action="count",
            default=0
        )
        return parser.parse_args()

    args = parse_args()

    # We need to make a logger if we're running stand-alone
    from usrp_mpm.mpmlog import get_main_logger
    log = get_main_logger(log_default_delta=args.verbose-args.quiet)

    dboards = args.dboards.split(",")
    if any([x not in ('0', '1') for x in dboards]):
        log.error("Unsupported dboards requested: %s", dboards)
        return False

    return do_update_cpld(args.file, dboards, args.updater)


if __name__ == "__main__":
    sys.exit(not main())

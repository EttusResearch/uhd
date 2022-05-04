#!/usr/bin/env python3
#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Update the CPLD image for the X4xx
"""

import sys
import os
import argparse
import subprocess
import pyudev
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.sys_utils.sysfs_gpio import GPIOBank
from usrp_mpm.sys_utils.udev import dt_symbol_get_spidev
from usrp_mpm.periph_manager.x4xx_mb_cpld import MboardCPLD
from usrp_mpm.periph_manager.x4xx import x4xx
from usrp_mpm.chips.max10_cpld_flash_ctrl import Max10CpldFlashCtrl

OPENOCD_DIR = "/usr/share/openocd/scripts"
CONFIGS = {
    'axi_bitq' : {
        'files' : ["fpga/altera-10m50.cfg"],
        'cmd' : ["interface axi_bitq; axi_bitq_config %u %u; adapter_khz %u",
                 "init; svf -tap 10m50.tap %s -progress -quiet;exit"]
    }
}

AXI_BITQ_ADAPTER_SPEED = 5000
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
        fpga_mgrs = list(context.list_devices(subsystem="fpga_manager"))
        if fpga_mgrs:
            state = fpga_mgrs[which].attributes.asstring('state')
            logger.trace("FPGA State: {}".format(state))
            return state == "operating"
        return False
    except OSError as ex:
        logger.error("Error while checking FPGA status: {}".format(ex))
        return False

def find_axi_bitq_uio():
    """
    Find the AXI Bitq UIO device
    """
    label = 'jtag-0'

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

def get_gpio_controls():
    """
    Instantiates an object to control JTAG related GPIO pins
    Bank 3 - Pin 0: Allows toggle of JTAG Enable and additional signals
    Bank 3 - Pin 1: JTAG Enable signal to the CPLD
    """
    # Bank 3 starts at pin 78
    offset = 78
    mask = 0x03
    ddr = 0x03
    return GPIOBank({'label': 'zynqmp_gpio'}, offset, mask, ddr)

def enable_jtag_gpio(gpios, disable=False):
    """
    Toggle JTAG Enable line to the CPLD
    """
    CPLD_JTAG_OE_n_pin = 0
    PL_CPLD_JTAGEN_pin = 1
    if not disable:
        gpios.set(CPLD_JTAG_OE_n_pin, 0) # CPLD_JTAG_OE_n is active low
        gpios.set(PL_CPLD_JTAGEN_pin, 1) # PL_CPLD_JTAGEN is active high
    else:
        gpios.set(CPLD_JTAG_OE_n_pin, 0) # CPLD_JTAG_OE_n is active low
        gpios.set(PL_CPLD_JTAGEN_pin, 0) # PL_CPLD_JTAGEN is active high

def do_update_cpld(filename, updater_mode):
    """
    Carry out update process for the CPLD
    :param filename: path (on device) to the new CPLD image
    :param updater_mode: the updater method to use- Either flash or legacy
    :return: True on success, False otherwise
    """
    assert updater_mode in ('legacy', 'flash'), \
        f"Invalid updater method {updater_mode} given"
    logger = get_logger('update_cpld')
    logger.info("Programming CPLD of mboard with image {} using {} mode"
                .format(filename, updater_mode))

    if not os.path.exists(filename):
        logger.error("CPLD image file {} not found".format(filename))
        return False

    if updater_mode == 'legacy':
        return jtag_cpld_update(filename, logger)
    if updater_mode == 'flash':
        cpld_spi_node = dt_symbol_get_spidev('mb_cpld')
        regs = MboardCPLD(cpld_spi_node, logger)
        reconfig_engine_offset = 0x40
        cpld_min_revision = 0x19100108
        flash_control = Max10CpldFlashCtrl(logger, regs, reconfig_engine_offset, cpld_min_revision)
        return flash_control.update(filename)
    return False

def jtag_cpld_update(filename, logger):
    """
    Update the MB CPLD via dedicated JTAG lines in the FPGA
    Note: To use this update mechanism, a FPGA image with JTAG
    lines must be loaded.
    """
    if logger is None:
        logger = get_logger('update_cpld')

    if not check_fpga_state():
        logger.error("CPLD lines are routed through fabric, "
                     "FPGA is not programmed, giving up")
        return False

    mode = 'axi_bitq'
    config = CONFIGS[mode]

    if not filename.endswith('svf'):
        logger.warning('The legacy JTAG programming mechanism expects '
                       '.svf files. The CPLD file being used may be incorrect.')

    if check_openocd_files(config['files'], logger=logger):
        logger.trace("Found required OpenOCD files.")
    else:
        # check_openocd_files logs errors
        return False

    uio_id = find_axi_bitq_uio()
    if uio_id is None or uio_id < 0:
        logger.error('Failed to find axi_bitq uio devices. '\
                     'Make sure overlays are up to date')
        return False

    try:
        gpios = get_gpio_controls()
    except RuntimeError as ex:
        logger.error('Could not open GPIO required for JTAG programming!'\
                     ' {}'.format(ex))
        return False
    enable_jtag_gpio(gpios)

    cmd = ["openocd",
           "-c", config['cmd'][0] % (uio_id, AXI_BITQ_BUS_CLK, AXI_BITQ_ADAPTER_SPEED),
           "-f", (config['files'][0]).strip(),
           "-c", config['cmd'][1] % filename]

    logger.trace("Update CPLD CMD: {}".format(" ".join(cmd)))
    subprocess.call(cmd)

    # Disable JTAG dual-purpose pins to CPLD after reprogramming
    enable_jtag_gpio(gpios, disable=True)

    logger.trace("Done programming CPLD...")
    return True

def get_mb_compat_rev():
    import re
    cmd = ['eeprom-dump', 'mb']
    output = subprocess.check_output(
        cmd,
        stderr=subprocess.STDOUT,
        ).decode('utf-8')
    expression = re.compile("^usrp_eeprom_board_info.*compat_rev: 0x([0-9A-Fa-f]+)")
    for line in output.splitlines():
        match = expression.match(line)
        if match:
            compat_rev = int(match.group(1), 16)
            return compat_rev
    raise AssertionError("Cannot get compat_rev from MB eeprom.: `{}'".format(output))

def get_default_cpld_image_names(compat_rev):
    """Determine the default CPLD image name based on the compat_rev"""
    default_cpld_image_10m04 = ['cpld-x410-10m04.rpd', 'usrp_x410_cpld_10m04.rpd']
    default_cpld_image_10m08 = ['cpld-x410-10m08.rpd', 'usrp_x410_cpld_10m08.rpd']
    default_image_name_mapping = {
        1: default_cpld_image_10m04,
        2: default_cpld_image_10m04,
        3: default_cpld_image_10m04,
        4: default_cpld_image_10m04,
        5: default_cpld_image_10m04,
        6: default_cpld_image_10m04,
        7: default_cpld_image_10m08
    }
    if compat_rev not in default_image_name_mapping:
        raise NotImplementedError("The default CPLD image name for compat_rev {} is not available".format(compat_rev))
    return default_image_name_mapping[compat_rev]

def main():
    """
    Go, go, go!
    """
    def parse_args():
        """Parse the command-line arguments"""
        parser = argparse.ArgumentParser(description='Update the CPLD image on the X4xx')
        default_image_names = get_default_cpld_image_names(get_mb_compat_rev())
        default_image_path = "/lib/firmware/ni/" + default_image_names[0]
        parser.add_argument("--file", help="Filename of CPLD image",
                            default=default_image_path)
        # There are two --updater options supported by the script: "legacy" and "flash". However,
        # the "legacy" mode requires a custom FPGA bitfile where the JTAG engine is compiled into.
        # This is not the case for the standard UHD bitfile and hence the legacy mode cannot be
        # used with it. --> Hide the command line argument to avoid false expectations
        parser.add_argument("--updater",
                            help = argparse.SUPPRESS,
                            default="flash")
        parser.add_argument("--force", help="Force installing the CPLD image specified by the --file " \
                            "argument if it does not match the name of the default CPLD image. " \
                            "Using the wrong CPLD image may brick your device.",
                            action="store_true", default=False, required=False)
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
        args = parser.parse_args()
        if (os.path.basename(args.file) not in default_image_names) and not args.force:
            parser.epilog = "\nERROR: Valid CPLD image names for X410 compat_rev {} are {}, " \
                "but you selected {}. Using the wrong CPLD image may brick your device. " \
                "Please use the --force option if you are really sure.".format(
                    get_mb_compat_rev(),
                    ' and '.join(default_image_names),
                    args.file
                )
            parser.print_help()
            parser.epilog = None
            sys.exit(1)
        return args

    args = parse_args()

    # We need to make a logger if we're running stand-alone
    from usrp_mpm.mpmlog import get_main_logger
    log = get_main_logger(log_default_delta=args.verbose-args.quiet)

    return do_update_cpld(args.file, args.updater)

if __name__ == "__main__":
    sys.exit(not main())

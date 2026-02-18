#!/usr/bin/env python3
#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Update the CPLD image for a ZBX or HBX daughterboard."""

import argparse
import os
import re
import subprocess
import sys

import pyudev
from usrp_mpm.chips.max10_cpld_flash_ctrl import Max10CpldFlashCtrl
from usrp_mpm.mpmlog import get_logger, get_main_logger
from usrp_mpm.mpmutils import check_fpga_state
from usrp_mpm.periph_manager.x4xx_mb_cpld import make_mb_cpld_ctrl
from usrp_mpm.periph_manager.x4xx_periphs import CtrlportRegs
from usrp_mpm.sys_utils.udev import dt_symbol_get_spidev

OPENOCD_DIR = "/usr/share/openocd/scripts"

AXI_BITQ_ADAPTER_SPEED = 5000
AXI_BITQ_BUS_CLK = 50000000

# The offsets are for JTAG_DB0 and JTAG_DB1 on the motherboard CPLD
DAUGHTERBOARD0_OFFSET = CtrlportRegs.MB_PL_CPLD + 0x60
DAUGHTERBOARD1_OFFSET = CtrlportRegs.MB_PL_CPLD + 0x80

# *BX flash reconfiguration engine specific offsets
RECONFIG_ENGINE_OFFSET = 0x20
CPLD_MIN_REVISION = 0x20052016

# PID to DB name mapping
PID_TO_DB = {0x4002: "zbx", 0x4008: "hbx"}


def check_openocd_files(files, logger=None):
    """Check if all file required by OpenOCD exist.

    :param logger: logger object
    """
    for ocd_file in files:
        if not os.path.exists(os.path.join(OPENOCD_DIR, ocd_file)):
            if logger is not None:
                logger.error(f"Missing file {os.path.join(OPENOCD_DIR, ocd_file)}")
            return False
    return True


def find_offset(dboard):
    """Find the AXI Bitq UIO device.

    :param dboard: the dboard, can be either 0 or 1
    """
    assert dboard in (0, 1)
    return DAUGHTERBOARD0_OFFSET if dboard == 0 else DAUGHTERBOARD1_OFFSET


def find_axi_bitq_uio():
    """Find the AXI Bitq UIO device."""
    label = "ctrlport-mboard-regs"

    logger = get_logger("update_cpld")

    try:
        context = pyudev.Context()
        for uio in context.list_devices(subsystem="uio"):
            uio_label = uio.attributes.asstring("maps/map0/name")
            logger.trace(
                f"UIO label: {uio_label}, match: {uio_label == label} " f"number: {uio.sys_number}"
            )
            if uio_label == label:
                return int(uio.sys_number)
        return None
    except OSError as ex:
        logger.error(f"Error while looking for axi_bitq uio nodes: {ex}")
        return None


def do_update_cpld(dboard_update_settings):
    """Carry out update process for the CPLD.

    :param dboard_update_settings: list of db and corresponding path (on device) to the new
    CPLD image and updater mode to use
    :return: True on success, False otherwise
    """
    logger = get_logger("update_cpld")

    if not dboard_update_settings:
        logger.error("Invalid daughterboard selection.")
        return False

    if not check_fpga_state(logger=logger):
        logger.error("CPLD lines are routed through fabric, FPGA is not programmed, giving up")
        return False

    for dboard_update_setting in dboard_update_settings:
        dboard = dboard_update_setting[0]
        filename = dboard_update_setting[1]
        updater_mode = dboard_update_setting[2]
        cpld_update_strategies = dboard_update_setting[3]

        logger.info(
            f"Programming CPLD of dboard {dboard} with image {filename} "
            f"using {updater_mode} mode"
        )

        if not os.path.exists(filename):
            logger.error(f"CPLD image file {filename} not found")
            return False

        if updater_mode == "legacy":
            success = jtag_cpld_update(filename, dboard, cpld_update_strategies, logger)
        elif updater_mode == "flash":
            success = flash_cpld_update(filename, dboard, logger)
        else:
            raise NotImplementedError(f"Unknown updater mode {updater_mode}")

        if not success:
            return success

    return True


def get_db_pid_rev(slot):
    """Get the PID and revision of the daughterboard in the specified slot."""
    assert slot in [0, 1]
    cmd = ["eeprom-dump", f"db{slot}"]
    output = subprocess.check_output(
        cmd,
        stderr=subprocess.STDOUT,
    ).decode("utf-8")
    expression = re.compile(
        "^usrp_eeprom_board_info.*pid: 0x([0-9A-Fa-f]+).*rev: 0x([0-9A-Fa-f]+).*compat_rev:.*"
    )
    for line in output.splitlines():
        match = expression.match(line)
        if match:
            pid = int(match.group(1), 16)
            rev = int(match.group(2), 16)
            return pid, rev
    raise AssertionError(f"Cannot get pid and rev from DB{slot} eeprom.: `{output}'")


def get_cpld_update_strategies(pid, rev):
    """Determine the CPLD update strategies based on the pid and rev."""
    dboard = PID_TO_DB[pid]

    cpld_image_10m04_update_strategies = {
        "cpld_model": "10m04",
        "updaters": ["flash", "legacy"],
        "default_updater": "flash",
        "image_names": {
            "flash": [f"cpld-{dboard}-10m04.rpd", f"usrp_{dboard}_cpld_10m04.rpd"],
            "legacy": [f"cpld-{dboard}-10m04.svf", f"usrp_{dboard}_cpld_10m04.svf"],
        },
        "updater_config": {
            "legacy": {
                "files": ["fpga/altera-10m50.cfg"],
                "cmd": [
                    "interface axi_bitq; axi_bitq_config %u %u %u; adapter_khz %u",
                    "init; svf -tap 10m50.tap %s -progress -quiet;exit",
                ],
            }
        },
    }

    cpld_image_xo3lf_update_strategies = {
        "cpld_model": "xo3lf",
        "updaters": ["legacy"],
        "default_updater": "legacy",
        "image_names": {"legacy": [f"cpld-{dboard}-xo3lf.svf", f"usrp_{dboard}_cpld_xo3lf.svf"]},
        "updater_config": {
            "legacy": {
                "files": ["fpga/lattice-xo3lf.cfg"],
                "cmd": [
                    "interface axi_bitq; axi_bitq_config %u %u %u; adapter_khz %u",
                    "init; svf -tap xo3lf.tap %s -progress -quiet;exit",
                ],
            }
        },
    }

    cpld_update_strategies = {
        "zbx": {
            1: cpld_image_10m04_update_strategies,  # revA
            2: cpld_image_10m04_update_strategies,  # revB
            3: cpld_image_10m04_update_strategies,  # revC
            4: cpld_image_xo3lf_update_strategies,  # revD
            5: cpld_image_xo3lf_update_strategies,  # revE
            6: cpld_image_10m04_update_strategies,  # revF
            "10m04": cpld_image_10m04_update_strategies,  # 10m04
            "xo3lf": cpld_image_xo3lf_update_strategies,  # xo3lf
        },
        "hbx": {
            1: cpld_image_xo3lf_update_strategies,  # revA
            2: cpld_image_xo3lf_update_strategies,  # revB
            "xo3lf": cpld_image_xo3lf_update_strategies,  # xo3lf
        },
    }

    if rev not in cpld_update_strategies[dboard]:
        raise NotImplementedError(
            f"The CPLD update strategy for pid {pid} and "
            f"rev or CPLD model {rev} is not available"
        )
    return cpld_update_strategies[dboard][rev]


def flash_cpld_update(filename, dboard, logger=None):
    """Carry out update process for the CPLD using flash mode.

    :param filename: path (on device) to the new CPLD image
    :param dboard: dboard to update
    :return: True on success, False otherwise
    """
    dboard = int(dboard, 10)
    logger.info(f"Updating daughterboard slot {dboard}...")
    # enable required daughterboard clock
    cpld_spi_node = dt_symbol_get_spidev("mb_cpld")
    cpld_control = make_mb_cpld_ctrl(cpld_spi_node, logger)
    cpld_control.enable_daughterboard_support_clock(dboard, enable=True)
    # setup flash configuration engine and required register access
    label = "ctrlport-mboard-regs"
    ctrlport_regs = CtrlportRegs(label, logger)
    regs = ctrlport_regs.get_db_cpld_iface(dboard)
    flash_control = Max10CpldFlashCtrl(logger, regs, RECONFIG_ENGINE_OFFSET, CPLD_MIN_REVISION)
    success = flash_control.update(filename)
    # disable clock
    cpld_control.enable_daughterboard_support_clock(dboard, enable=False)
    if success:
        logger.trace("Done programming CPLD...")
    return success


def jtag_cpld_update(filename, dboard, cpld_update_strategies, logger=None):
    """Carry out update process for the CPLD.

    :param filename: path (on device) to the new CPLD image
    :param dboard: dboard to update
    :param cpld_update_strategies: a data struct containing updaters, image names, metadata.
    :return: True on success, False otherwise
    """
    config = cpld_update_strategies["updater_config"]["legacy"]

    if check_openocd_files(config["files"], logger=logger):
        logger.trace("Found required OpenOCD files.")
    else:
        # check_openocd_files logs errors
        return False

    logger.info(f"Updating daughterboard slot {dboard}...")

    uio_id = find_axi_bitq_uio()
    offset = find_offset(int(dboard, 10))
    if uio_id is None or uio_id < 0:
        logger.error("Failed to find axi_bitq uio devices. " "Make sure overlays are up to date")
        return False

    cmd = [
        "openocd",
        "-c",
        config["cmd"][0] % (uio_id, AXI_BITQ_BUS_CLK, offset, AXI_BITQ_ADAPTER_SPEED),
        "-f",
        (config["files"][0]).strip(),
        "-c",
        config["cmd"][1] % filename,
    ]

    logger.trace(f"Update CPLD CMD: {' '.join(cmd)}")
    subprocess.call(cmd)

    logger.trace("Done programming CPLD...")
    return True


def check_mpm_status(logger=None):
    logger.info("Checking MPM status...")
    cmd = ["check-filesystem", "--mpm-init-successful"]
    try:
        subprocess.check_call(cmd)
        logger.info("MPM is ready.")
    except subprocess.CalledProcessError:
        logger.error("MPM not started successfully, cannot proceed with CPLD update.")
        sys.exit(1)

def main():
    """Go, go, go!"""

    def parse_args():
        """Parse the command-line arguments."""
        parser = argparse.ArgumentParser(
            description="Update the CPLD image on ZBX or HBX daughterboard"
        )
        parser.add_argument(
            "--file",
            help="Filename of CPLD image. Also specify the updater using"
            " --updater arg when using this argument.",
            default="",
        )
        parser.add_argument("--dboards", help="Slot name to program", default="0,1")
        parser.add_argument(
            "--updater",
            help="The image updater method to use, either " " 'legacy' (uses openocd) or 'flash'",
            default="",
        )
        parser.add_argument(
            "-v", "--verbose", help="Increase verbosity level", action="count", default=1
        )
        parser.add_argument(
            "-q", "--quiet", help="Decrease verbosity level", action="count", default=0
        )
        parser.add_argument(
            "--force",
            help="Force installing the CPLD image specified by the "
            "--file argument if it does not match the name of the default CPLD image. "
            "Using the wrong CPLD image may brick your device.",
            action="store_true",
            default=False,
            required=False,
        )
        parser.add_argument(
            "--cpld_type",
            help="Specify the CPLD type. Currently supported types are '10m04' "
            "or 'xo3lf'. Use this argument to explicitly specify the CPLD "
            "hardware type and skip internal revision compatibilty checks. "
            "Use this only if you fully understand the hardware being used."
            " e.g. x4xx_db_update_cpld --cpld_type '10m04' ",
            default=None,
        )

        args = parser.parse_args()

        dboards = args.dboards.split(",")
        if any([x not in ("0", "1") for x in dboards]):
            log.error("Unsupported dboards requested: %s", dboards)
            return False

        if args.cpld_type:
            # Hardware Specified. Skip all checks.
            return args

        if args.file and not args.updater:
            parser.epilog = (
                "\nERROR: When setting --file, please also specify "
                "the updater to use using the --updater argument."
            )
            parser.print_help()
            parser.epilog = None
            sys.exit(1)

        for dboard in dboards:
            dboard_pid, dboard_rev = get_db_pid_rev(int(dboard, 10))
            if dboard_pid not in PID_TO_DB:
                parser.epilog = (
                    f"\nERROR: Daughterboard in slot {dboard} is not supported. "
                    f"(Supported PIDs: {', '.join(f'0x{pid:04x}' for pid in PID_TO_DB)}, "
                    f"but found {hex(dboard_pid)})"
                )
                parser.print_help()
                sys.exit(1)

            cpld_update_strategies = get_cpld_update_strategies(dboard_pid, dboard_rev)

            if args.updater and args.updater not in cpld_update_strategies["updaters"]:
                parser.epilog = (
                    f"\nERROR: Valid updaters for {PID_TO_DB[dboard_pid]} "
                    f"rev {dboard_rev} are {' and '.join(cpld_update_strategies['updaters'])}, "
                    f"but you selected {args.updater}."
                )
                parser.print_help()
                parser.epilog = None
                sys.exit(1)

            default_image_names = []
            for updater in cpld_update_strategies["updaters"]:
                default_image_names += cpld_update_strategies["image_names"][updater]
            if (
                args.file
                and (os.path.basename(args.file) not in default_image_names)
                and not args.force
            ):
                parser.epilog = (
                    f"\nERROR: Valid CPLD image names for {PID_TO_DB[dboard_pid]} "
                    f"rev {dboard_rev} are {' and '.join(default_image_names)}, "
                    f"but you selected {args.file}. Using the wrong CPLD image may brick your "
                    "device. Please use the --force option if you are really sure."
                )
                parser.print_help()
                parser.epilog = None
                sys.exit(1)

            if (
                args.file
                and args.updater
                and (
                    os.path.basename(args.file)
                    not in cpld_update_strategies["image_names"][args.updater]
                )
                and not args.force
            ):
                parser.epilog = (
                    f"\nERROR: Invalid CPLD image name and updater mode combination "
                    f" for {PID_TO_DB[dboard_pid]} rev {dboard_rev}. Using the wrong CPLD image "
                    "may brick your device. Please use the --force option if you are really sure."
                )
                parser.print_help()
                parser.epilog = None
                sys.exit(1)

        return args

    def get_dboard_update_settings(args, dboards):
        """Determine the dboard cpld image and updater mappings."""
        dboard_update_settings = []
        for dboard in dboards:
            dboard_pid, dboard_rev = get_db_pid_rev(int(dboard, 10))
            if args.cpld_type:
                cpld_update_strategies = get_cpld_update_strategies(dboard_pid, args.cpld_type)
            else:
                cpld_update_strategies = get_cpld_update_strategies(dboard_pid, dboard_rev)

            if args.updater:
                updater = args.updater
            else:
                updater = cpld_update_strategies["default_updater"]

            if args.file:
                image_path = args.file
            else:
                image_names = cpld_update_strategies["image_names"][updater]
                image_path = "/lib/firmware/ni/" + image_names[0]

            dboard_update_settings.append([dboard, image_path, updater, cpld_update_strategies])
        return dboard_update_settings

    args = parse_args()

    log = get_main_logger(log_default_delta=args.verbose - args.quiet)

    check_mpm_status(log)

    dboards = args.dboards.split(",")
    return do_update_cpld(get_dboard_update_settings(args, dboards))


if __name__ == "__main__":
    sys.exit(not main())

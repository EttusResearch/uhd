#!/usr/bin/env python3
"""
Copyright 2019 Ettus Research, A National Instrument Brand

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""

import sys
import argparse
import hashlib
import logging
import os
import re
import yaml

logging.basicConfig(format='[%(levelname).3s] %(message)s')

from uhd.imgbuilder import image_builder
from uhd.imgbuilder import yaml_utils

def setup_parser():
    """
    Create argument parser
    """
    parser = argparse.ArgumentParser(
        description="Build UHD image using RFNoC blocks",
    )

    config_group = parser.add_mutually_exclusive_group(required=True)
    config_group.add_argument(
        "-y", "--yaml-config",
        help="Path to yml configuration file")
    config_group.add_argument(
        "-r", "--grc-config",
        help="Path to grc file to generate config from")
    parser.add_argument(
        "-F", "--fpga-dir",
        help="Path to directory for the FPGA source tree. "
             "Defaults to the FPGA source tree of the current repo.",
        required=False,
        default=None)
    parser.add_argument(
        "-o", "--image-core-output",
        help="Path to where to save the image core Verilog source. "
             "Defaults to the directory of the YAML file, filename <DEVICE>_rfnoc_image_core.v",
        default=None)
    parser.add_argument(
        "-x", "--router-hex-output",
        help="Path to where to save the static router hex file. "
             "Defaults to the directory of the YAML file, filename <DEVICE>_static_router.hex",
        default=None)
    parser.add_argument(
        "-I", "--include-dir",
        help="Path directory of the RFNoC Out-of-Tree module",
        action='append', default=[]
        )
    parser.add_argument(
        "-b", "--grc-blocks",
        help="Path directory of GRC block descriptions (needed for --grc-config only)",
        default=None)
    parser.add_argument(
        "-l", "--log-level",
        help="Adjust log level",
        default='info')
    parser.add_argument(
        "-G", "--generate-only",
        help="Just generate files without building IP",
        action="store_true")
    parser.add_argument(
        "-d", "--device",
        help="Device to be programmed [x300, x310, e310, e320, n300, n310, n320, x410, x440]. "
             "Needs to be specified either here, or in the configuration file.",
        default=None)
    parser.add_argument(
        "-n", "--image_core_name",
        help="Name to use for the RFNoC image core. "
             "Defaults to the device name.",
        default=None)
    parser.add_argument(
        "-t", "--target",
        help="Build target (e.g. X310_HG, N320_XG, ...). Needs to be specified "
             "either here, on the configuration file.",
        default=None)
    parser.add_argument(
        "-g", "--GUI",
        help="Open Vivado GUI during the FPGA building process",
        action="store_true")
    parser.add_argument(
        "-c", "--clean-all",
        help="Cleans the IP before a new build",
        action="store_true")
    parser.add_argument(
        "-p", "--vivado-path",
        help="Path to the base install for Xilinx Vivado if not in default "
             "location (e.g., /tools/Xilinx/Vivado).",
        default=None)
    parser.add_argument(
        "-H", "--no-hash",
        help="Do not include source YAML hash in the generated source code.",
        action="store_true",
        default=False)
    parser.add_argument(
        "-D", "--no-date",
        help="Do not include date or time in the generated source code.",
        action="store_true",
        default=False)

    return parser


def image_config(args):
    """
    Load image configuration.

    The configuration can be either passed as RFNoC image configuration or as
    GNU Radio Companion grc. In latter case the grc files is converted into a
    RFNoC image configuration on the fly.
    :param args: arguments passed to the script.
    :return: image configuration as dictionary
    """
    if args.yaml_config:
        config = yaml_utils.load_config(args.yaml_config, get_config_path())
        device = config.get('device') if args.device is None else args.device
        target = config.get('default_target') if args.target is None else args.target
        image_core_name = config.get('image_core_name') if args.image_core_name is None else args.image_core_name
        if image_core_name is None:
            image_core_name = device
        return config, args.yaml_config, device, image_core_name, target
    with open(args.grc_config) as grc_file:
        config = yaml.load(grc_file)
        logging.info("Converting GNU Radio Companion file to image builder format")
        config = image_builder.convert_to_image_config(config, args.grc_blocks)
        image_core_name = args.device if args.image_core_name is None else args.image_core_name
        return config, args.grc_config, args.device, image_core_name, args.target


def resolve_path(path, local):
    """
    Replaced path by local if path is enclosed with "@" (placeholder markers)
    :param path: the path to check
    :param local: new path content if path is placeholder
    :return: path if path is not a placeholder else local
    """
    return re.sub("^@.*@$", local, path)


def get_fpga_path(args):
    """
    Returns FPGA path. This is the fpga_dir from the arguments. If fpga_dir
    does not exist, it is changed to the FPGA source path of the current repo.
    If current directory is not in a repo, an error is generated.
    :param args: arguments passed to the script
    :return: FPGA root path
    """
    # If a valid path is given, use that
    if args.fpga_dir:
        if os.path.isdir(args.fpga_dir):
            fpga_path = os.path.abspath(args.fpga_dir)
            logging.info("Using FPGA directory %s", fpga_path)
            return fpga_path
        else:
            logging.error("Bad fpga-dir argument. %s is not a valid directory.", args.fpga_dir)
            sys.exit(1)
    # Try to find an fpga directory at the base of the repo
    repo_base = os.getcwd()
    while True:
        repo_base = os.path.split(repo_base)[0]
        top_dir = os.path.split(repo_base)[1]
        if top_dir == 'uhd' or top_dir == 'uhddev':
            fpga_path = os.path.join(repo_base, 'fpga')
            break
        if top_dir == '':
            fpga_path = None
            break
    # If it's valid FPGA base path, use that
    if fpga_path and os.path.isdir(os.path.join(fpga_path, 'usrp3', 'top')):
        logging.info("Using FPGA directory %s", fpga_path)
        return fpga_path
    # No valid path found
    logging.error("FPGA path not found. Specify with --fpga-dir argument.")
    sys.exit(1)


def get_config_path():
    """
    Returns path that contains configurations files (yml descriptions for
    block, IO signatures and device bsp).
    :return: Configuration path
    """
    return os.path.normpath(resolve_path("@CONFIG_PATH@", os.path.join(
        os.path.dirname(__file__), '..', 'include', 'uhd')))


def main():
    """
    Wrapper for image_builder.build_image.
    :return: exit code
    """
    args = setup_parser().parse_args()
    if args.log_level is not None:
        logging.root.setLevel(args.log_level.upper())

    config, source, device, image_core_name, target = image_config(args)
    source_hash = hashlib.sha256()
    with open(source, "rb") as source_file:
        source_hash.update(source_file.read())

    image_builder.build_image(
        config=config,
        fpga_path=get_fpga_path(args),
        config_path=get_config_path(),
        device=device,
        image_core_name=image_core_name,
        target=target,
        generate_only=args.generate_only,
        clean_all=args.clean_all,
        GUI=args.GUI,
        source=source,
        source_hash=source_hash.hexdigest(),
        output_path=args.image_core_output,
        router_hex_path=args.router_hex_output,
        include_paths=args.include_dir,
        vivado_path=args.vivado_path,
        no_date=args.no_date,
        no_hash=args.no_hash,
        )

if __name__ == "__main__":
    sys.exit(main())

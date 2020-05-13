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
        "-y", "--yaml_config",
        help="Path to yml configuration file")
    config_group.add_argument(
        "-r", "--grc_config",
        help="Path to grc file to generate config from")
    parser.add_argument(
        "-F", "--fpga-dir",
        help="Path directory of the FPGA source tree",
        required=True,
        default=None)
    parser.add_argument(
        "-o", "--image-core-output",
        help="Path to where to save the image core Verilog source. "
             "Defaults to the location of the YAML file.")
    parser.add_argument(
        "-x", "--router-hex-output",
        help="Path to where to save the static router hex file. "
             "Defaults to the location of the YAML file, filename $device_static_router.hex",
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
        "--generate-only",
        help="Just generate files without building IP",
        action="store_true")
    parser.add_argument(
        "-d", "--device",
        help="Device to be programmed [x300, x310, e310, e320, n300, n310, n320]."
             "Needs to be specified either here, or in the configuration file.",
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
        return config, args.yaml_config, device, target
    with open(args.grc_config) as grc_file:
        config = yaml.load(grc_file)
        logging.info("Converting GNU Radio Companion file to image builder format")
        config = image_builder.convert_to_image_config(config, args.grc_blocks)
        return config, args.grc_config, args.device, args.target


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
    Returns FPGA path. This is the fpga_dir of arguments, If fpga_dir does
    not exists it is the predefined path of of the script.
    :param args: arguments passed to the script
    :return: FPGA root path
    """
    result = args.fpga_dir
    if not os.path.isdir(result):
        logging.info("%s is not a valid directory.", result)
        result = resolve_path("@FPGA_PATH@", os.path.join(
            os.path.dirname(__file__), '..', '..', '..', 'fpga'))
        logging.info("Fall back to %s", result)
    return result


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

    config, source, device, target = image_config(args)
    source_hash = hashlib.sha256()
    with open(source, "rb") as source_file:
        source_hash.update(source_file.read())

    image_builder.build_image(
        config=config,
        fpga_path=args.fpga_dir,
        config_path=get_config_path(),
        device=device,
        target=target,
        generate_only=args.generate_only,
        clean_all=args.clean_all,
        gui=args.GUI,
        source=source,
        source_hash=source_hash.hexdigest(),
        output_path=args.image_core_output,
        router_hex_path=args.router_hex_output,
        include_paths=args.include_dir,
        )

if __name__ == "__main__":
    sys.exit(main())

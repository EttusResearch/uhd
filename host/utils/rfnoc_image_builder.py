#!/usr/bin/env python3
"""RFNoC Image Builder: Create RFNoC FPGA bitfiles from YAML input.

SPDX-License-Identifier: GPL-3.0-or-later
"""

import argparse
import hashlib
import logging
import os
import sys

from uhd import get_pkg_data_path
from uhd.rfnoc_utils import image_builder, log as rfnoc_log


def get_valid_targets():
    """Get valid targets (--target parameter)."""
    target_fpga_map = {
        "E310_SG1": ["", "IDLE"],
        "E310_SG3": ["", "IDLE"],
        "E320": ["1G", "XG", "AA"],
        "N310": ["WX", "HG", "XG", "HA", "XA", "AA"],
        "N300": ["WX", "HG", "XG", "HA", "XA", "AA"],
        "N320": ["WX", "HG", "XG", "XQ", "AQ", "AA"],
        "X310": ["1G", "HG", "XG", "HA", "XA"],
        "X300": ["1G", "HG", "XG", "HA", "XA"],
        "X410": [""],
        "X440": [""],
    }
    targets = []
    for target, fpgas in target_fpga_map.items():
        for fpga in fpgas:
            if fpga:
                targets.append(f"{target}_{fpga}")
            else:
                targets.append(target)
    return targets


def setup_parser():
    """Create argument parser."""
    parser = argparse.ArgumentParser(
        description="Build UHD image using RFNoC blocks",
    )

    config_group = parser.add_mutually_exclusive_group(required=True)
    config_group.add_argument("-y", "--yaml-config", help="Path to yml configuration file")
    config_group.add_argument("-r", "--grc-config", help="Path to grc file to generate config from")
    parser.add_argument(
        "-C",
        "--base-dir",
        help="Path to the base directory. Defaults to the current directory.",
        required=False,
        default=os.getcwd(),
    )
    parser.add_argument(
        "-F",
        "--fpga-dir",
        help="Path to directory for the FPGA source tree. "
        "Defaults to the FPGA source tree of the current repo.",
        required=False,
        default=None,
    )
    parser.add_argument(
        "-B",
        "--build-dir",
        help="Path to directory where the image core and and build artifacts will be generated. "
        'Defaults to "build-<image-core-name>" in the base directory.',
        required=False,
        default=None,
    )
    parser.add_argument(
        "-O",
        "--build-output-dir",
        help="Path to directory for final FPGA build outputs. "
        'Defaults to "build" in the base directory.',
        required=False,
        default=None,
    )
    parser.add_argument(
        "-E",
        "--build-ip-dir",
        help="Path to directory for IP build artifacts. "
        'Defaults to "build-ip" in the base directory.',
        required=False,
        default=None,
    )
    parser.add_argument(
        "-o", "--image-core-output", help="DEPRECATED! This has been replaced by --build-dir. "
    )
    parser.add_argument(
        "-x", "--router-hex-output", help="DEPRECATED! This option will be ignored. ", default=None
    )
    parser.add_argument(
        "-I",
        "--include-dir",
        help="Path to directory of the RFNoC Out-of-Tree module",
        action="append",
        default=[],
    )
    parser.add_argument(
        "-b",
        "--grc-blocks",
        help="Path to directory of GRC block descriptions (needed for --grc-config only)",
        default=None,
    )
    parser.add_argument("-l", "--log-level", help="Adjust log level", default="info")
    parser.add_argument(
        "-R",
        "--reuse",
        help="Reuse existing files (do not regenerate image core).",
        action="store_true",
    )
    parser.add_argument(
        "-G",
        "--generate-only",
        help="Just generate files without building the FPGA",
        action="store_true",
    )
    parser.add_argument(
        "-W",
        "--ignore-warnings",
        help="Run build even when there are warnings in the build process",
        action="store_true",
    )
    parser.add_argument(
        "-S",
        "--secure-core",
        help="Build a secure image core instead of a bitfile. "
        "This argument provides the name of the generated YAML.",
    )
    parser.add_argument(
        "-K",
        "--secure-key",
        help="Path to encryption key file to use for secure core.",
        default="",
    )
    parser.add_argument(
        "-d",
        "--device",
        help="Device to be programmed [x300, x310, e310, e320, n300, n310, n320, x410, x440]. "
        "Needs to be specified either here, or in the configuration file.",
        default=None,
    )
    parser.add_argument(
        "-n",
        "--image-core-name",
        "--image_core_name",
        help="Name to use for the RFNoC image core. "
        "Defaults to name of the image core YML file, without the extension.",
        default=None,
    )
    parser.add_argument(
        "-t",
        "--target",
        help="Build target (e.g. X310_HG, N320_XG, ...). Needs to be specified "
        "either here, on the configuration file.",
        choices=get_valid_targets(),
        default=None,
    )
    parser.add_argument(
        "-g", "--GUI", help="Open Vivado GUI during the FPGA building process", action="store_true"
    )
    parser.add_argument(
        "-Y",
        "--SYNTH",
        help="Stop the FPGA build process after Synthesis",
        action="store_true",
    )
    parser.add_argument(
        "--CHECK",
        help="Run elaboration only to check HDL syntax",
        action="store_true",
    )
    parser.add_argument(
        "-s", "--save-project", help="Save Vivado project to disk", action="store_true"
    )
    parser.add_argument("-P", "--ip-only", help="Build only the required IPs", action="store_true")
    parser.add_argument(
        "-j",
        "--jobs",
        help="Number of parallel jobs to use with make",
        required=False,
        default=None,
    )
    parser.add_argument(
        "-c", "--clean-all", help="Cleans the IP before a new build", action="store_true"
    )
    parser.add_argument(
        "-p",
        "--vivado-path",
        help="Path to the base install for Xilinx Vivado if not in default "
        "location (e.g., /tools/Xilinx/Vivado).",
        default=None,
    )
    parser.add_argument(
        "-H",
        "--no-hash",
        help="Do not include source YAML hash in the generated source code.",
        action="store_true",
        default=False,
    )
    parser.add_argument(
        "-D",
        "--no-date",
        help="Do not include date or time in the generated source code.",
        action="store_true",
        default=False,
    )
    parser.add_argument(
        "--color",
        choices=("never", "auto", "always"),
        default="auto",
        help="Enable colorful output. When set to 'auto' will only show color "
        "output in TTY environments (e.g., interactive shells)",
    )

    return parser


def get_fpga_path(args):
    """Return FPGA path.

    This is the fpga_dir from the arguments. If fpga_dir
    does not exist, it is changed to the FPGA source path of the current repo.
    If current directory is not in a repo, an error is generated.
    :param args: arguments passed to the script
    :return: FPGA root path
    """
    # If a valid path is given, use that
    if args.fpga_dir:
        fpga_path = os.path.normpath(os.path.abspath(os.path.expanduser(args.fpga_dir)))
        if os.path.isdir(fpga_path):
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
        if top_dir == "uhd" or top_dir == "uhddev":
            fpga_path = os.path.join(repo_base, "fpga")
            break
        if top_dir == "":
            fpga_path = None
            break
    # Try to find an FPGA directory in the install prefix
    if fpga_path is None and os.path.isdir(
        os.path.join(get_pkg_data_path(), "rfnoc", "fpga", "usrp3")
    ):
        fpga_path = os.path.join(get_pkg_data_path(), "rfnoc", "fpga")
    # If fpga_path is a valid FPGA base path, use it
    if fpga_path and os.path.isdir(os.path.join(fpga_path, "usrp3", "top")):
        logging.info("Using FPGA directory %s", fpga_path)
        return fpga_path
    # No valid path found
    logging.error("FPGA path not found. Specify with --fpga-dir argument.")
    sys.exit(1)


def main():
    """Run image_builder.build_image.

    :return: exit code
    """
    args = setup_parser().parse_args()
    rfnoc_log.init_logging(args.color, args.log_level)

    source_arg = args.yaml_config if args.yaml_config else args.grc_config
    source = os.path.normpath(os.path.abspath(os.path.expanduser(source_arg)))
    source_type = "yaml" if args.yaml_config else "grc"
    if not os.path.isfile(source):
        logging.error("Source file %s does not exist", source)
        return 1

    source_hash = hashlib.sha256()
    with open(source, "rb") as source_file:
        source_hash.update(source_file.read())

    dump_config = args.grc_config and not args.yaml_config

    # Do some more sanitization of command line arguments
    if args.image_core_output:
        logging.warning(
            "Using deprecated command line option --image-core-output (-o)! "
            "Use --build-dir instead."
            "This option will be removed in future versions of UHD."
        )
        if args.build_dir:
            logging.warning(
                "Both --build-dir and --image-core-output are used. Ignoring the latter."
            )
        else:
            args.build_dir = args.image_core_output

    return image_builder.build_image(
        # Image core source file info
        source=source,
        source_type=source_type,
        source_hash=source_hash.hexdigest(),
        no_date=args.no_date,
        no_hash=args.no_hash,
        # Unsanitized device ID string from cmd line (e.g., x410, n320, ...)
        device=args.device,
        # Build target info (unsanitized, from command line)
        image_core_name=args.image_core_name,
        target=args.target,
        # Provide all the paths
        base_dir=args.base_dir,
        build_dir=args.build_dir,
        build_output_dir=args.build_output_dir,
        build_ip_dir=args.build_ip_dir,
        repo_fpga_path=get_fpga_path(args),
        yaml_path=args.yaml_config if args.yaml_config else args.grc_config,
        include_paths=args.include_dir,
        vivado_path=args.vivado_path,
        grc_blocks=args.grc_blocks,
        # Various build config parameters
        reuse=args.reuse,
        generate_only=args.generate_only,
        continue_on_warnings=args.ignore_warnings,
        clean_all=args.clean_all,
        GUI=args.GUI,
        synthesize_only=args.SYNTH,
        check_hdl=args.CHECK,
        save_project=args.save_project,
        ip_only=args.ip_only,
        num_jobs=args.jobs,
        secure_core=args.secure_core,
        secure_key_file=args.secure_key,
        dump_config=dump_config,
    )


if __name__ == "__main__":
    ret_val = main()
    # In some cases, we can get return codes out of the standard 0-254 range.
    # Coerce it the standard value of 255 to indicate it was out of range.
    if ret_val > 255:
        ret_val = 255
    sys.exit(ret_val)

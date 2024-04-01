#!/usr/bin/env python3
#
# Copyright 2023 Ettus Research, a National Instrument Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

"""
Repeatedly runs the requested build until it builds successfully and meets
timing, up to a maximum number of tries. Builds will be retried when they
fail with timing errors or other errors that might not reoccur. Builds will
stop if a an unrecognized error occurs.
"""

import sys
import argparse
import subprocess
import logging
import re
import random


def parse_args():
    """Parse the command line arguments.

    Returns:
        Populated namespace containing the arguments and their values.
    """
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        "--target",
        "-t",
        type=str,
        help="FPGA make target to build (e.g., X310_XG).",
    )
    parser.add_argument(
        "--image-core",
        "-y",
        type=str,
        help="For using the image builder instead of make, use this to specify "
        "the image core YAML.",
    )
    parser.add_argument(
        "--num",
        "-n",
        type=int,
        default=4,
        required=False,
        help="Number of times to attempt the build.",
    )
    parser.add_argument(
        "--persistent",
        "-p",
        action="store_true",
        default=False,
        help=(
            "Continue retrying builds regardless of which error occurs, "
            "up to the specified number of attempts."
        ),
    )
    parser.add_argument(
        "--seed",
        "-s",
        type=int,
        default=0,
        required=False,
        help="Initial seed value to use.",
    )
    return parser.parse_args()


def prepare_build(target, image_core):
    """
    Run tasks to prepare the build. In particular, execute the RFNoC image builder
    if desired.
    """
    if image_core:
        logging.info("Calling rfnoc_image_builder to prepare FPGA build.")
        cmd = [
            "rfnoc_image_builder",
            "--yaml-config",
            image_core,
            "--generate-only",
            "--no-hash",
            "--no-date",
        ]
        if target:
            cmd += ["--target", target]
        result = subprocess.run(
            cmd,
            check=False,
            encoding="utf-8",
            capture_output=True,
        )
        logging.info("rfnoc_image_builder output:")
        logging.info("stdout:\n%s", result.stdout)
        logging.info("stderr:\n%s", result.stderr)
        if result.returncode:
            logging.error("Image builder failed! Consult output for details.")
        result.check_returncode()
        # Parse the image builder output to get the make command we need to build the FPGA
        make_command = re.search(r"(?<=: )make.*$", result.stderr, flags=re.M).group(0)
        logging.info("Using make command: %s", make_command)
    else:
        assert target
        make_command = f"make {target}"
    return {
        "make_command": make_command,
    }


def run_fpga_build(build_seed, cfg):
    """Performs one iteration of an FPGA build.

    Args:
        build_seed: 32-bit signed integer to seed the FPGA build.
        cfg: Build configuration info

    Returns:
        0: The build succeeded.
        1: There was a timing or other error that might not reoccur.
        2: There was some other error that should cause us to stop trying.
    """
    output = ""
    make_cmd = cfg["make_command"]
    cmd = f'/bin/bash -c "{make_cmd} BUILD_SEED={build_seed}"'
    with subprocess.Popen(
        cmd,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        bufsize=1,
        universal_newlines=True,
    ) as proc:
        for line in proc.stdout:
            print(line, end="")
            output += line
    if proc.returncode != 0:
        # Regular expressions for error strings to search for that would tell
        # us we should try again.
        transient_errors = [
            # Standard timing error:
            "The design did not satisfy timing constraints",
            # Known issue fixed in Vivado 2021.2:
            (
                "Router encountered a fatal exception of type .*"
                "Trying to tool lock on already tool locked arc"
            ),
        ]
        for error_string in transient_errors:
            if re.search(error_string, output):
                return 1
        return 2
    return 0


def next_build_seed(previous_seed):
    """Determines the next seed to use based on the previous seed. This creates
    a reproducible sequence of values with a specific initial value.

    Args:
        previous_seed: The previous integer seed from which to determine the
            new seed.

    Returns:
        The next build seed value in the range of a 32-bit signed integer.
    """
    random.seed(previous_seed)
    return random.randint(-0x80000000, 0x7FFFFFFF)


def main():
    """Run the requested builds.

    Returns:
        The status of the last build (0 if successful, non-zero if the build
        failed).
    """
    logging.basicConfig(format="[REPEAT BUILD][%(levelname)s] %(message)s")
    logging.root.setLevel(logging.INFO)
    args = parse_args()
    if not args.target and not args.image_core:
        logging.error("Either --target or --image-core must be provided!")
        return 1
    build_seed = args.seed
    status = 128
    cfg = prepare_build(args.target, args.image_core)
    try:
        for build_num in range(1, args.num + 1):
            logging.info("Starting FPGA build %d with seed %s", build_num, build_seed)
            status = run_fpga_build(build_seed, cfg)
            logging.info("Finished FPGA build %d", build_num)
            if status == 0:
                logging.info("FPGA build succeeded on attempt number %s", build_num)
                break
            if build_num == args.num:
                logging.error("Reached maximum number of FPGA build attempts")
            elif status == 1:
                logging.info("FPGA build will be restarted due to unsuccessful attempt")
            elif status == 2 and not args.persistent:
                logging.error("Stopping due to unexpected FPGA build error")
                break
            build_seed = next_build_seed(build_seed)
    except KeyboardInterrupt:
        logging.info("Received SIGINT. Aborting . . .")
        # Return normal Bash value for SIGINT (128+2)
        return 130
    return status


if __name__ == "__main__":
    sys.exit(main())

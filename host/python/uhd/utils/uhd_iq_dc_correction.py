#!/usr/bin/env python3
#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
"""Measure IQ impairments and DC offsets and create correction files.

This script is used to measure IQ impairments and DC offsets in USRP devices both in TX and
RX directions. The measured impairments are then used to create correction files holding the
I and Q correction coefficients, a group delay and the DC offsets. When running a UHD session
with loaded correction data, the closest available correction data will be applied if exact
data points are not available for the requested frequency.
"""
import argparse
import sys

from uhd.usrp.cal.iq_dc_compensation import get_usrp_iq_dc_compensator

FREQUENCY_SWEEP_DEFAULTS = {
    "rf_start_frequency": 500e6,
    "rf_end_frequency": 20e9,
    "rf_frequency_step": 100e6,
}


def parse_args():
    """Argument parsing."""
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "-a", "--args", type=str, default="", help="UHD session arguments (default: '')"
    )
    parser.add_argument(
        "-r",
        "--sample_rate",
        type=float,
        default=1250e6,
        help="Sample rate in Hz (default: 1.25GS/s)",
    )
    parser.add_argument(
        "-n",
        "--num_samples",
        type=int,
        default=int(100e3),
        help="Number of samples (default: 100.000)",
    )
    parser.add_argument(
        "--num-coeffs",
        type=int,
        default=15,
        help="Number of coefficients for the fractional delay filter (default: 15).",
    )
    parser.add_argument(
        "-c",
        "--channels",
        type=int,
        nargs="+",
        default=[0],
        help="Array of channel numbers separated by space (default: [0])",
    )
    parser.add_argument(
        "--rj45",
        action="store_true",
        help=(
            "Enable this option if the provided USRP address is associated to the RJ45 "
            "(1G Ethernet) connection to avoid streaming errors. "
            "Leave disabled when using 10G or 100G Ethernet connections. (default: False)"
        ),
    )
    parser.add_argument(
        "-lc",
        "--lo-channel",
        type=int,
        default=-1,
        help=(
            "Specify the radio channel from which the LO is sourced (default: -1, meaning no "
            "external LO). This will set the LO source to 'external' for all channels to be "
            "measured."
        ),
    )
    parser.add_argument(
        "-l",
        "--log",
        action="store_true",
        help=(
            "Enable logging of measurement results into file `measurements_<db_serial>.json` "
            "in current directory (default: False). Current directory needs to be writable."
        ),
    )
    freq_group = parser.add_argument_group(
        "Frequency Parameters",
        "Parameters to define the frequency sweep range and steps. "
        "If center frequencies are provided, frequency sweep parameters should not be specified.",
    )
    freq_group.add_argument(
        "-cf",
        "--center_frequencies",
        nargs="+",
        type=float,
        default=None,
        help=(
            "Center frequencies in Hz. If this argument is provided, these frequencies"
            " will be used instead of a frequency sweep."
        ),
    )
    freq_group.add_argument(
        "-sf",
        "--rf_start_frequency",
        type=float,
        default=FREQUENCY_SWEEP_DEFAULTS["rf_start_frequency"],
        help="Start frequency of the frequency sweep in Hz (default: 500 MHz)",
    )
    freq_group.add_argument(
        "-ef",
        "--rf_end_frequency",
        type=float,
        default=FREQUENCY_SWEEP_DEFAULTS["rf_end_frequency"],
        help="End frequency of the frequency sweep in Hz (default: 20 GHz)",
    )
    freq_group.add_argument(
        "-rfs",
        "--rf_frequency_step",
        type=float,
        default=FREQUENCY_SWEEP_DEFAULTS["rf_frequency_step"],
        help="Frequency step of the frequency sweep in Hz (default: 100MHz)",
    )
    parser.add_argument(
        "-bfs",
        "--bb_frequency_step",
        type=float,
        default=10e6,
        help="Step size for frequency sweep in Hz (default: 10 MHz)",
    )
    parser.add_argument(
        "-t", "--tx", action="store_true", help="Perform TX measurements (default: False)"
    )
    # Show help if no arguments are given
    if len(sys.argv) == 1:
        parser.print_help(sys.stderr)
        sys.exit(1)
    # check for center frequencies vs. frequency sweep arguments
    args = parser.parse_args()
    if args.center_frequencies is not None:
        if (
            args.rf_start_frequency != FREQUENCY_SWEEP_DEFAULTS["rf_start_frequency"]
            or args.rf_end_frequency != FREQUENCY_SWEEP_DEFAULTS["rf_end_frequency"]
            or args.rf_frequency_step != FREQUENCY_SWEEP_DEFAULTS["rf_frequency_step"]
        ):
            parser.error(
                "Cannot specify both center frequencies and frequency sweep parameters "
                "(rf_start_frequency, rf_end_frequency, rf_frequency_step). "
                "Please provide only one set of frequency options."
            )
    return args


def main():
    """Run the IQ impairment correction capture and calculation."""
    args = parse_args()
    usrp_corr = get_usrp_iq_dc_compensator(args)
    usrp_corr.run_measurement()
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (RuntimeError, ValueError) as ex:
        print(f"Error: {ex}")
        sys.exit(1)

#!/usr/bin/env python3
#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Use a calibrated USRP as a power meter.

This example demonstrates how to use a calibrated USRP device as a power
meter. It measures the received signal power in dBm at a specified center
frequency (plus a local oscillator offset) and within the configured analog
filter bandwidth. Results are provided either as a one-time measurement or
continuously until interrupted. This script is useful for applications
requiring precise power measurements, such as signal strength analysis or
calibration.

Refer to the UHD Manual on how to calibrate a USRP device:
https://files.ettus.com/manual/page_calibration.html

Example Usage:
usrp_power_meter.py --args addr=192.168.10.2 --freq 2.1e9 --rate 10e6
                    --bandwidth 5e6 --antenna RX1 --ref-level -10
                    --samps-per-est 1e7 --mode continuous
"""

import argparse
import signal
import sys

import uhd


def parse_args():
    """Parse the command line arguments."""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-a",
        "--args",
        default="",
        help="""specifies the USRP device arguments, which holds
        multiple key value pairs separated by commas
        (e.g., addr=192.168.40.2,type=x300) [default = ""].""",
    )
    parser.add_argument(
        "-f",
        "--freq",
        type=float,
        required=True,
        help="specifies the center frequency in Hz [input is required].",
    )
    parser.add_argument(
        "-o",
        "--lo-offset",
        type=float,
        default=0.0,
        help="specifies the local oscillator (LO) offset with respect to the "
        "configured center frequency [default = 0.0].",
    )
    parser.add_argument(
        "-c",
        "--channel",
        type=int,
        default=0,
        help="specifies the RX Channel to use [default = 0].",
    )
    parser.add_argument("-t", "--antenna", help="specifies the RX Antenna to use [default = None].")
    parser.add_argument(
        "-r",
        "--rate",
        default=1e6,
        type=float,
        help="specifies the sample rate in samples/sec [default = 1e6].",
    )
    parser.add_argument(
        "-b",
        "--bandwidth",
        type=float,
        help="specifies the analog filter bandwidth in Hz (if supported) [default = None].",
    )
    parser.add_argument(
        "-l",
        "--ref-level",
        type=float,
        default=-15,
        help="specifies the RX reference power level in dBm. The receiver will "
        "automatically pick a gain such that an input CW tone at reference power "
        "level will fully excite the ADC. The Reference power level should be higher "
        "than the expected power level [default = -15].",
    )
    parser.add_argument(
        "-n",
        "--samps-per-est",
        type=float,
        default=1e6,
        help="specifies the number of samples per estimate [default = 1e6].",
    )
    parser.add_argument(
        "--mode",
        choices=["one-shot", "continuous"],
        default="one-shot",
        help="specifies the operation mode, that is either measure once, "
        "or keep measuring until Ctrl-C is pressed. [default = one-shot]",
    )
    return parser.parse_args()


def get_streamer(usrp, chan):
    """Return an RX streamer with fc32 output."""
    stream_args = uhd.usrp.StreamArgs("fc32", "sc16")
    stream_args.channels = [chan]
    return usrp.get_rx_stream(stream_args)


def setup_device(usrp, args):
    """Apply the settings from args to the device."""
    chan = args.channel
    if chan > usrp.get_rx_num_channels():
        print(
            "ERROR: Invalid channel selected: {} (only {} channels available!)".format(
                chan, usrp.get_rx_num_channels()
            )
        )
        raise RuntimeError("Invalid channel selected")
    print("Using channel: {}".format(chan))

    if args.antenna:
        print("Setting RX antenna to `{}'...".format(args.antenna), end="")
        usrp.set_rx_antenna(args.antenna, chan)
        print("OK")

    if not usrp.has_rx_power_reference(chan):
        antenna = usrp.get_rx_antenna()
        print("ERROR: This device is not calibrated for RX at RF%d-%s!" % (chan, antenna))
        raise RuntimeError("Device not calibrated for selected antenna")

    print("Requesting RX rate of {} Msps...".format(args.rate / 1e6), end="")
    usrp.set_rx_rate(args.rate, chan)
    if abs(usrp.get_rx_rate(chan) - args.rate) > 1.0:
        print("ALMOST. Actual rate: {} Msps".format(usrp.get_rx_rate(chan) / 1e6))
    else:
        print("OK")

    print("Requesting RX frequency of {} MHz...".format(args.freq / 1e6), end="")
    tr = uhd.types.TuneRequest(args.freq, args.lo_offset)
    usrp.set_rx_freq(tr, chan)
    if abs(usrp.get_rx_freq(chan) - args.freq) > 1.0:
        print("ALMOST. Actual frequency: {} MHz".format(usrp.get_rx_freq(chan) / 1e6))
    else:
        print("OK")

    print("Requesting RX power reference level of {:.2f} dBm...".format(args.ref_level), end="")
    usrp.set_rx_power_reference(args.ref_level, chan)
    ref_level = usrp.get_rx_power_reference(chan)
    if abs(ref_level - args.ref_level) > 1.0:
        print("ALMOST. Actual ref level: {:.2f} dBm".format(ref_level))
    else:
        print("OK")

    if args.bandwidth:
        print("Requesting analog RX bandwidth of {} Msps...".format(args.bandwidth), end="")
        usrp.set_rx_bandwidth(args.bandwidth, chan)
        if abs(usrp.get_rx_bandwidth(chan) - args.bandwidth) > 1.0:
            print("ALMOST. Actual bandwidth: {} MHz".format(usrp.get_rx_bandwidth(chan) / 1e6))
        else:
            print("OK")
    return (chan, ref_level)


RUN = True


def main():
    """Run USRP power meter."""
    args = parse_args()
    usrp = uhd.usrp.MultiUSRP(args.args)
    (chan, ref_level) = setup_device(usrp, args)
    streamer = get_streamer(usrp, chan)
    if args.mode == "continuous":

        def handle_sigint(_sig, _frame):
            print("Caught Ctrl-C, exiting...")
            global RUN
            RUN = False

        signal.signal(signal.SIGINT, handle_sigint)
    while RUN:
        try:
            power_dbfs = uhd.dsp.signals.get_usrp_power(streamer, num_samps=int(args.samps_per_est))
        except RuntimeError:
            # This is a hack b/c the signal handler is not gracefully handling
            # SIGINT
            break
        power_dbm = power_dbfs + ref_level
        print("Received power: {:+6.2f} dBm".format(power_dbm))
        if args.mode == "one-shot":
            break
    return True


if __name__ == "__main__":
    sys.exit(not main())

#!/usr/bin/env python
#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Generate and TX samples using a set of waveforms, and waveform characteristics
"""

import argparse
import numpy as np
import uhd

waveforms = {
    "sine": lambda n, tone_offset, rate: np.exp(n * 2j * np.pi * tone_offset / rate),
    "square": lambda n, tone_offset, rate: np.sign(waveforms["sine"](n, tone_offset, rate)),
    "const": lambda n, tone_offset, rate: 1 + 1j,
    "ramp": lambda n, tone_offset, rate:
            2*(n*(tone_offset/rate) - np.floor(float(0.5 + n*(tone_offset/rate))))
}


def parse_args():
    """Parse the command line arguments"""
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--args", default="", type=str)
    parser.add_argument(
        "-w", "--waveform", default="sine", choices=waveforms.keys(), type=str)
    parser.add_argument("-f", "--freq", type=float, required=True)
    parser.add_argument("-r", "--rate", default=1e6, type=float)
    parser.add_argument("-d", "--duration", default=5.0, type=float)
    parser.add_argument("-c", "--channels", default=0, nargs="+", type=int)
    parser.add_argument("-g", "--gain", type=int, default=10)
    parser.add_argument("--wave-freq", default=1e4, type=float)
    parser.add_argument("--wave-ampl", default=0.3, type=float)
    return parser.parse_args()


def main():
    """TX samples based on input arguments"""
    args = parse_args()
    usrp = uhd.usrp.MultiUSRP(args.args)
    if not isinstance(args.channels, list):
        args.channels = [args.channels]
    data = np.array(
        list(map(lambda n: args.wave_ampl * waveforms[args.waveform](n, args.wave_freq, args.rate),
            np.arange(
                int(10 * np.floor(args.rate / args.wave_freq)),
                dtype=np.complex64))),
        dtype=np.complex64)  # One period

    usrp.send_waveform(data, args.duration, args.freq, args.rate,
                       args.channels, args.gain)


if __name__ == "__main__":
    main()

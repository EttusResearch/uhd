#! /usr/bin/env python
#
# Copyright 2017 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

import numpy as np
import uhd
import argparse


waveforms = {
    "sine": lambda n, tone_offset, rate: np.exp(n * 2j * np.pi * tone_offset / rate),
    "square": lambda n, tone_offset, rate: np.sign(waveforms["sine"](n, tone_offset, rate)),
    "const": lambda n, tone_offset, rate: 1 + 1j,
    "ramp": lambda n, tone_offset, rate: 2*(n*(tone_offset/rate) - np.floor(float(0.5 + n*(tone_offset/rate))))
}


def parse_args():
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
    args = parse_args()
    usrp = uhd.multi_usrp(args.args)
    if not isinstance(args.channels, list):
        args.channels = [args.channels]
    data = np.array(
        map(lambda n: args.wave_ampl * waveforms[args.waveform](n, args.wave_freq, args.rate),
            np.arange(
                int(10 * np.floor(args.rate / args.wave_freq)),
                dtype=np.complex64)),
        dtype=np.complex64)  # One period

    usrp.send_waveform(data, args.duration, args.freq, args.rate,
                       args.channels, args.gain)


if __name__ == "__main__":
    main()

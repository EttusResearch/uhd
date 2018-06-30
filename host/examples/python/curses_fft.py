#!/usr/bin/env python
#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Curses FFT example using Python API
"""

import argparse
import curses as cs
import numpy as np
import uhd


def parse_args():
    """Parse the command line arguments"""
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--args", default="", type=str)
    parser.add_argument("-f", "--freq", type=float, required=True)
    parser.add_argument("-r", "--rate", default=1e6, type=float)
    parser.add_argument("-g", "--gain", type=int, default=10)
    parser.add_argument("-c", "--channel", type=int, default=0)
    parser.add_argument("-n", "--nsamps", type=int, default=100000)
    parser.add_argument("--dyn", type=int, default=60)
    parser.add_argument("--ref", type=int, default=0)
    return parser.parse_args()


def psd(nfft, samples):
    """Return the power spectral density of `samples`"""
    window = np.hamming(nfft)
    result = np.multiply(window, samples)
    result = np.fft.fftshift(np.fft.fft(result, nfft))
    result = np.square(np.abs(result))
    result = np.nan_to_num(10.0 * np.log10(result))
    result = np.abs(result)
    return result


def clip(minval, maxval, value):
    """Clip the value between a and b"""
    return min(minval, max(maxval, value))


def main():
    """Create Curses display of FFT"""
    args = parse_args()
    usrp = uhd.usrp.MultiUSRP(args.args)

    # Set the USRP rate, freq, and gain
    usrp.set_rx_rate(args.rate, args.channel)
    usrp.set_rx_freq(uhd.types.TuneRequest(args.freq), args.channel)
    usrp.set_rx_gain(args.gain, args.channel)

    # Initialize the curses screen
    screen = cs.initscr()
    cs.curs_set(0)
    cs.noecho()
    cs.cbreak()
    screen.keypad(1)
    height, width = screen.getmaxyx()

    # Create a pad for the y-axis
    y_axis_width = 10
    y_axis = cs.newwin(height, y_axis_width, 0, 0)

    # Create the buffer to recv samples
    num_samps = max(args.nsamps, width)
    samples = np.empty((1, num_samps), dtype=np.complex64)

    st_args = uhd.usrp.StreamArgs("fc32", "sc16")
    st_args.channels = [args.channel]

    metadata = uhd.types.RXMetadata()
    streamer = usrp.get_rx_stream(st_args)
    buffer_samps = streamer.get_max_num_samps()
    recv_buffer = np.zeros((1, buffer_samps), dtype=np.complex64)

    stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.start_cont)
    stream_cmd.stream_now = True
    streamer.issue_stream_cmd(stream_cmd)

    db_step = float(args.dyn) / (height - 1.0)
    db_start = db_step * int((args.ref - args.dyn) / db_step)
    db_stop = db_step * int(args.ref / db_step)

    try:
        while True:
            # Resize the frequency plot on screen resize
            screen.clear()
            if cs.is_term_resized(height, width):
                height, width = screen.getmaxyx()
                cs.resizeterm(height, width)

                db_step = float(args.dyn) / (height - 1.0)
                db_start = db_step * int((args.ref - args.dyn) / db_step)
                db_stop = db_step * int(args.ref / db_step)

                y_axis.clear()

            # Create the vertical (dBfs) axis
            y_axis.addstr(0, 1, "{:> 6.2f} |-".format(db_stop))
            for i in range(1, height - 1):
                label = db_stop - db_step * i
                y_axis.addstr(i, 1, "{:> 6.2f} |-".format(label))
            try:
                y_axis.addstr(height - 1, 1, "{:> 6.2f} |-".format(db_start))
            except cs.error:
                pass
            y_axis.refresh()

            # Receive the samples
            recv_samps = 0
            while recv_samps < num_samps:
                samps = streamer.recv(recv_buffer, metadata)

                if metadata.error_code != uhd.types.RXMetadataErrorCode.none:
                    print(metadata.strerror())
                if samps:
                    real_samps = min(num_samps - recv_samps, samps)
                    samples[:, recv_samps:recv_samps + real_samps] = recv_buffer[:, 0:real_samps]
                    recv_samps += real_samps

            # Get the power in each bin
            bins = psd(width, samples[args.channel][0:width])

            for i in range(y_axis_width, width):
                vertical_slot = clip(height, 0, np.int(bins[i] / db_step))
                try:
                    for j in range(vertical_slot, height):
                        screen.addch(j, i, '*')
                except cs.error:
                    pass
            screen.refresh()

    except KeyboardInterrupt:
        pass

    stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.stop_cont)
    streamer.issue_stream_cmd(stream_cmd)

    cs.curs_set(1)
    cs.nocbreak()
    screen.keypad(0)
    cs.echo()
    cs.endwin()


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
#
# Copyright 2018-2020 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
RX samples and apply settings as a timed command. Use this tool to analyze the
settling time of analog components as well as the accuracy of timed commands.
Typically, you will need to connect a tone or other signal generator to the
DUT's input.

Example: This would receive several seconds of data from an X3x0 device,
tune to 1 GHz, and then bump the gain by 30 dB after a set amount of
time:

$ rx_settling_time.py -a type=x300 -f 1e9 -g 0 --new-gain 30 --plot
"""

import sys
import argparse
import numpy as np
import uhd

def parse_args():
    """Parse the command line arguments"""
    parser = argparse.ArgumentParser(
        description=__doc__
    )
    parser.add_argument(
        "-a", "--args", default="",
        help="Device args (e.g., 'type=x300')")
    parser.add_argument(
        "--spec",
        help="Subdev spec (e.g. 'B:0')")
    parser.add_argument(
        "-d", "--duration", default=5.0, type=float,
        help="Total acquisition time")
    parser.add_argument(
        "--setup-delay", default=.5, type=float,
        help="Time before starting receive")
    parser.add_argument(
        "--set-delay", default=2.0, type=float,
        help="Time between starting to receive and changing settings")
    parser.add_argument(
        "--skip-time", default=0.0, type=float,
        help="Time to skip after starting to receive")
    parser.add_argument(
        "-o", "--output-file", type=str,
        help="Name of the output file (e.g., 'output.dat')")
    parser.add_argument(
        "-f", "--freq", type=float, required=True,
        help="Initial frequency")
    parser.add_argument(
        "-g", "--gain", type=float, default=20.0,
        help="Initial gain")
    parser.add_argument(
        "--new-freq", type=float,
        help="Frequency after set time")
    parser.add_argument(
        "--new-gain",
        help="Gain after set time")
    parser.add_argument(
        "--property-bool",
        help="Set a Boolean property tree node. Use the format path=True or path=False.")
    parser.add_argument(
        "-r", "--rate", default=1e6, type=float,
        help="Sampling rate (Hz)")
    parser.add_argument(
        "-c", "--channel", default=0, type=int, help="Channel on which to receive on")
    parser.add_argument(
        "-n", "--numpy", default=False, action="store_true",
        help="Save output file in NumPy format (default: No)")
    parser.add_argument(
        "--plot", default=False, action="store_true",
        help="Show nice pic")
    return parser.parse_args()


def get_rx_streamer(usrp, chan):
    """
    Return a streamer
    """
    st_args = uhd.usrp.StreamArgs("fc32", "sc16")
    st_args.channels = [chan,]
    return usrp.get_rx_stream(st_args)


def apply_initial_settings(usrp, chan, rate, freq, gain):
    """
    Apply initial settings for:
    - freq
    - gain
    - rate
    """
    usrp.set_rx_rate(rate)
    tune_req = uhd.types.TuneRequest(freq)
    usrp.set_rx_freq(tune_req, chan)
    usrp.set_rx_gain(gain, chan)


def start_rx_stream(streamer, start_time):
    """
    Kick off the RX streamer
    """
    stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.start_cont)
    stream_cmd.stream_now = False
    stream_cmd.time_spec = start_time
    streamer.issue_stream_cmd(stream_cmd)


def _cmd_set_property_bool(usrp, key_value):
    """
    Execute setting a Boolean property tree node. key_value is a string of the
    form "/path/to/node=True" (or "False").
    """
    path, value = key_value.split("=")
    value = value.lower()
    # Convert to bool from string, allowing all sorts of "true" values:
    value = value in ("1", "yes", "y", "true")
    usrp.get_tree().access_bool(path).set(value)


def load_commands(usrp, chan, cmd_time, **kwargs):
    """
    Load the switching commands.
    """
    usrp.set_command_time(cmd_time)
    kw_cb_map = {
        'freq': lambda freq: usrp.set_rx_freq(uhd.types.TuneRequest(float(freq)), chan),
        'gain': lambda gain: usrp.set_rx_gain(float(gain), chan),
        'prop_tree_bool': lambda key_value: _cmd_set_property_bool(usrp, key_value),
    }
    for key, callback in kw_cb_map.items():
        if kwargs.get(key) is not None:
            callback(kwargs[key])
    usrp.clear_command_time()


def recv_samples(rx_streamer, total_num_samps, skip_samples):
    """
    Run the receive loop and crop samples.
    """
    metadata = uhd.types.RXMetadata()
    result = np.empty((1, total_num_samps), dtype=np.complex64)
    total_samps_recvd = 0
    timeouts = 0 # This is a bit of a hack, until we can pass timeout values to
                 # Python
    max_timeouts = 20
    buffer_samps = rx_streamer.get_max_num_samps()
    recv_buffer = np.zeros(
        (1, buffer_samps), dtype=np.complex64)
    while total_samps_recvd < total_num_samps:
        samps_recvd = rx_streamer.recv(recv_buffer, metadata)
        if metadata.error_code == uhd.types.RXMetadataErrorCode.timeout:
            timeouts += 1
            if timeouts >= max_timeouts:
                print("[ERROR] Reached timeout threshold. Exiting.")
                return None
        elif metadata.error_code != uhd.types.RXMetadataErrorCode.none:
            print("[ERROR] " + metadata.strerror())
            return None
        if samps_recvd:
            samps_recvd = min(total_num_samps - total_samps_recvd, samps_recvd)
            result[:, total_samps_recvd:total_samps_recvd + samps_recvd] = \
                recv_buffer[:, 0:samps_recvd]
            total_samps_recvd += samps_recvd
    if skip_samples:
        print("Skipping {} samples.".format(skip_samples))
    return result[0][skip_samples:]


def save_to_file(samps, filename, save_as_numpy):
    """
    Save samples to binary file
    """
    with open(filename, 'wb') as out_file:
        if save_as_numpy:
            np.save(out_file, samps, allow_pickle=False, fix_imports=False)
        else:
            samps.tofile(out_file)

def plot_samps(samps, rate, set_offset):
    """
    Show a nice piccie
    """
    try:
        import pylab
    except ImportError:
        print("[ERROR] --plot requires pylab.")
        return
    ylim = max(
        max(np.abs(np.real(samps))),
        max(np.abs(np.imag(samps))),
    )
    time_axis = np.arange(len(samps)) / rate - set_offset
    pylab.plot(time_axis, np.real(samps))
    pylab.plot(time_axis, np.imag(samps))
    pylab.ylim((-ylim, ylim))
    pylab.grid(True)
    pylab.xlabel('Time offset [s]')
    pylab.ylabel('Amplitude')
    pylab.legend(('In-Phase', 'Quadrature'))
    pylab.title('Settling Time')
    pylab.show()


def main():
    """Execute"""
    args = parse_args()
    usrp = uhd.usrp.MultiUSRP(args.args)
    if args.spec is not None:
        usrp.set_rx_subdev_spec(uhd.usrp.SubdevSpec(args.spec))
    rx_streamer = get_rx_streamer(usrp, args.channel)
    total_num_samps = int(args.duration * args.rate)
    skip_samps = int(args.skip_time * args.rate)
    print("Total number of samples to acquire: {}".format(total_num_samps))
    apply_initial_settings(
        usrp,
        args.channel,
        args.rate,
        args.freq,
        args.gain
    )
    time_zero = usrp.get_time_now()
    print("Sending stream commands...")
    start_rx_stream(
        rx_streamer,
        time_zero+args.setup_delay,
    )
    print("Preloading set commands...")
    load_commands(
        usrp=usrp,
        chan=args.channel,
        cmd_time=time_zero+args.setup_delay+args.set_delay,
        freq=args.new_freq,
        gain=args.new_gain,
        prop_tree_bool=args.property_bool,
    )
    print("Starting receive...")
    samps = recv_samples(rx_streamer, total_num_samps, skip_samps)
    if samps is None:
        return False
    print("Received {} samples.".format(samps.size))
    print("New settings are applied at sample index {}."
          .format(int((args.set_delay - args.skip_time) * args.rate)))
    if args.plot:
        plot_samps(
            samps,
            args.rate,
            args.set_delay - args.skip_time,
        )
    if args.output_file:
        save_to_file(
            samps,
            args.output_file,
            args.numpy,
        )
    return True

if __name__ == "__main__":
    sys.exit(not main())

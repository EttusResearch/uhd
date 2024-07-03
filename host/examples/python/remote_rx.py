#!/usr/bin/env python3
#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Stream IQ data from a USRP to a remote destination.
"""

import sys
import argparse
import time
import uhd

INIT_DELAY = 0.05  # 50mS initial delay before receive

def parse_args():
    """Parse the command line arguments"""
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--args", default="",
                        help="Device args (e.g., addr=192.168.40.2)")
    parser.add_argument("-r", "--rate", type=float,
                        help="Output rate (samples/sec)")
    parser.add_argument("-f", "--freq", type=float, required=True,
                        help="Center frequency")
    parser.add_argument("-g", "--gain", type=float, default=0.0,
                        help="Gain (dB)")
    parser.add_argument("-d", "--duration", default=None, type=float,
                        help="Stream duration (seconds), leave out to stream until stopped")
    parser.add_argument("-c", "--channels", default=[0], nargs="+", type=int,
                        help="Which channel(s) to use (specify \"0\", \"1\", \"0 1\", etc)")
    parser.add_argument("-i", "--dest-addr", type=str, required=True,
                        help="Remote destination IP address")
    parser.add_argument("-p", "--dest-port", nargs="+", type=int, required=True,
                        help="Remote destination UDP port")
    parser.add_argument("--adapter", type=str,
                        help="Adapter to use for remote streaming (e.g. 'sfp0')")
    parser.add_argument("--dest-mac-addr",
                        help="Manually provide destination MAC address in the "
                             "format 01:a2:4f:6d:7e:5f. By default, the device will "
                             "use ARP to identify the MAC address.")
    parser.add_argument("--keep-hdr", action="store_true",
                        help="Specify this argument to keep CHDR headers on outgoing packets")
    return parser.parse_args()

def get_stream_cmd(usrp, rate, duration, start_time = None):
    """
    Generate a stream command based on rate and duration.
    """
    if duration:
        stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.num_done)
        stream_cmd.num_samps = int(rate * duration)
    else:
        stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.start_cont)
    if start_time is None:
        stream_cmd.stream_now = True
    else:
        stream_cmd.stream_now = False
        stream_cmd.time_spec = start_time
    return stream_cmd

def check_channels(usrp, args):
    """Check that the device has sufficient RX channels available"""
    # Check that each channel specified is less than the number of total number of rx channels
    # the device can support
    channels = args.channels
    dev_rx_channels = usrp.get_rx_num_channels()
    if not all(map((lambda chan: chan < dev_rx_channels), channels)):
        print("Invalid channel(s) specified.")
        return []
    return channels

def main():
    """
    Go, go, go!
    """
    args = parse_args()
    usrp = uhd.usrp.MultiUSRP(args.args)
    channels = check_channels(usrp, args)
    if not channels:
        return False
    if len(channels) != len(args.dest_port):
        print("Number of channels must match number of ports.")
        return False
    print("Selected RX channels: {}.".format(', '.join(str(ch) for ch in channels)))
    if args.rate:
        print(f"Requesting sampling rate {args.rate/1e6} Msps...")
        for chan in channels:
            usrp.set_rx_rate(args.rate, chan)
    actual_rate = usrp.get_rx_rate(channels[0])
    print(f"Using sampling rate: {actual_rate/1e6} Msps.")
    print(f"Requesting center frequency {args.freq/1e6} MHz...")
    for chan in channels:
        usrp.set_rx_freq(args.freq, chan)
    print(f"Actual center frequency: {usrp.get_rx_freq(channels[0])/1e6} MHz.")
    print(f"Requesting gain {args.gain} dB...")
    for chan in channels:
        usrp.set_rx_gain(args.gain, chan)
    print(f"Actual gain: {usrp.get_rx_gain(channels[0])} dB.")
    print("Generating RX streamer object...")
    rx_streamers = []
    for chx, chan in enumerate(channels):
        stream_args = uhd.usrp.StreamArgs("sc16", "sc16")
        stream_args.channels = [chan]
        stream_args.args = \
            f"dest_addr={args.dest_addr},dest_port={args.dest_port[chx]}," \
            f"stream_mode={'full_packet' if args.keep_hdr else 'raw_payload'}" + \
            (f",adapter={args.adapter}" if args.adapter else "") + \
            (f",dest_mac_addr={args.dest_mac_addr}" if args.dest_mac_addr else "")
        rx_streamers.append(usrp.get_rx_stream(stream_args))
    print("Starting stream(s)...")
    start_time = uhd.types.TimeSpec(usrp.get_time_now().get_real_secs() + INIT_DELAY) if len(channels) > 1 else None
    for rx_streamer in rx_streamers:
        stream_cmd = get_stream_cmd(usrp, actual_rate, args.duration, start_time)
        rx_streamer.issue_stream_cmd(stream_cmd)
    print("Stream started. Press Ctrl-C to stop.")
    timeout = time.monotonic() + args.duration if args.duration else None
    if timeout and len(channels) > 1:
        timeout += INIT_DELAY
    try:
        while timeout is None or time.monotonic() < timeout:
            time.sleep(1)
    except KeyboardInterrupt:
        pass
    print("Stopping stream...")
    for rx_streamer in rx_streamers:
        rx_streamer.issue_stream_cmd(uhd.types.StreamCMD(uhd.types.StreamMode.stop_cont))
    print("Streaming complete. Exiting.")
    return 0

if __name__ == "__main__":
    sys.exit(main())

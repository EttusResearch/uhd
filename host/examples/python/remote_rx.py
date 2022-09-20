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
    parser.add_argument("-p", "--dest-port", type=int, required=True,
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

def get_stream_cmd(usrp, rate, duration, num_channels):
    """
    Generate a stream command based on rate and duration.
    """
    if duration:
        stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.num_done)
        stream_cmd.num_samps = int(rate * duration)
    else:
        stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.start_cont)
    stream_cmd.stream_now = (num_channels == 1)
    if num_channels > 1:
        stream_cmd.time_spec = uhd.types.TimeSpec(usrp.get_time_now().get_real_secs() + INIT_DELAY)
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
    chan = 0
    if args.rate:
        print(f"Requesting sampling rate {args.rate/1e6} Msps...")
        usrp.set_rx_rate(args.rate, chan)
    actual_rate = usrp.get_rx_rate(chan)
    print(f"Using sampling rate: {usrp.get_rx_rate(chan)/1e6} Msps.")
    print(f"Requesting center frequency {args.freq/1e6} MHz...")
    usrp.set_rx_freq(args.freq, chan)
    print(f"Actual center frequency: {usrp.get_rx_freq(chan)/1e6} MHz.")
    print(f"Requesting gain {args.gain} dB...")
    usrp.set_rx_gain(args.gain, chan)
    print(f"Actual gain: {usrp.get_rx_gain(chan)} dB.")
    channels = check_channels(usrp, args)
    if not channels:
        return False
    print("Selected {} RX channels.".format(', '.join(str(ch) for ch in channels)))
    print("Generating RX streamer object...")
    stream_args = uhd.usrp.StreamArgs("sc16", "sc16")
    stream_args.channels = channels
    stream_args.args = \
        f"dest_addr={args.dest_addr},dest_port={args.dest_port}," \
        f"stream_mode={'full_packet' if args.keep_hdr else 'raw_payload'}" + \
        (f",adapter={args.adapter}" if args.adapter else "") + \
        (f",dest_mac_addr={args.dest_mac_addr}" if args.dest_mac_addr else "")
    rx_streamer = usrp.get_rx_stream(stream_args)
    print("Starting stream...")
    num_channels = rx_streamer.get_num_channels()
    stream_cmd = get_stream_cmd(usrp, actual_rate, args.duration, num_channels)
    rx_streamer.issue_stream_cmd(stream_cmd)
    print("Stream started. Press Ctrl-C to stop.")
    if num_channels > 1:
        args.duration += INIT_DELAY
    timeout = time.monotonic() + args.duration if args.duration else None
    try:
        while timeout is None or time.monotonic() < timeout:
            time.sleep(1)
    except KeyboardInterrupt:
        pass
    print("Stopping stream...")
    rx_streamer.issue_stream_cmd(uhd.types.StreamCMD(uhd.types.StreamMode.stop_cont))
    print("Streaming complete. Exiting.")
    return 0

if __name__ == "__main__":
    sys.exit(main())

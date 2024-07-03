#!/usr/bin/env python3
#
# Copyright 2024 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Test the gain block with the RFNoC Python API
"""


import argparse
import sys
import uhd
import rfnoc_example

def parse_args():
    """Parse the command line arguments"""
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--args", default="",
                        help="USRP Device Args")
    parser.add_argument("--gain-block", "-G", type=str, default="0/Gain#0",
                        help="Gain block to use. Defaults to \"0/Gain#0\".")
    parser.add_argument("--radio-block", "-R", type=str, default="0/Radio#0",
                        help="Radio block to use. Defaults to \"0/Radio#0\".")
    parser.add_argument("-f", "--freq", type=float, required=True,
                        help="Center Frequency")
    parser.add_argument("-g", "--gain", type=float, required=True,
                        help="Analog gain")
    parser.add_argument("-d", "--digital-gain", type=int, required=True,
                        help="Digital gain")
    parser.add_argument("-c", "--channel", type=int, default=0,
                        help="Radio block channel index")
    parser.add_argument("-t", "--antenna", default="",
                        help="USRP RX Antenna")
    parser.add_argument("-r", "--rate", default=1e6, type=float,
                        help="Sampling Rate")
    parser.add_argument("-b", "--bandwidth", type=float,
                        help="Analog filter bandwidth (if supported)")
    parser.add_argument("-n", "--samps-per-est", type=float, default=1e6,
                        help="Samples per estimate.")
    return parser.parse_args()


def main():
    """Go go go!"""
    args = parse_args()
    # Create graph and block references
    graph = uhd.rfnoc.RfnocGraph(args.args)
    gain_block_base = graph.get_block(args.gain_block)
    gain_block = rfnoc_example.GainBlockControl(gain_block_base)
    radio_block = uhd.rfnoc.RadioControl(graph.get_block(args.radio_block))
    radio_chan = args.channel
    assert radio_chan < radio_block.get_num_output_ports()
    rx_streamer = graph.create_rx_streamer(1, uhd.usrp.StreamArgs("fc32", "sc16"))
    # Set up graph
    blocks_in_graph = uhd.rfnoc.connect_through_blocks(
        graph,
        radio_block.get_unique_id(), radio_chan,
        gain_block_base.get_unique_id(), 0)
    ddc_block_id, ddc_port = next((
        (x.dst_blockid, x.dst_port)
        for x in blocks_in_graph
        if uhd.rfnoc.BlockID(x.dst_blockid).get_block_name() == 'DDC'
    ), (None, None))
    graph.connect(
        gain_block_base.get_unique_id(), 0,
        rx_streamer, 0)
    graph.commit()
    # Apply settings
    radio_block.set_rx_frequency(args.freq, radio_chan)
    print(
        f"Requested RX frequency: {args.freq/1e9:.3f} GHz, "
        f"actual RX frequency: {radio_block.get_rx_frequency(radio_chan)/1e9:.3f} GHz")
    if args.antenna:
        radio_block.set_rx_antenna(args.antenna, radio_chan)
    print(
        f"Requested RX antenna: '{args.antenna}', "
        f"actual RX antenna: {radio_block.get_rx_antenna(radio_chan)}")
    radio_block.set_rx_gain(args.gain, radio_chan)
    print(
        f"Requested analog RX gain: {args.gain:.1f} dB, "
        f"actual analog RX gain: {radio_block.get_rx_gain(radio_chan):.1f} dB")
    gain_block.set_gain_value(args.digital_gain)
    print(
        f"Requested digital RX gain factor: x{args.digital_gain}, "
        f"actual digital RX gain factor: x{gain_block.get_gain_value()}")
    if ddc_block_id:
        ddc_block = uhd.rfnoc.DdcBlockControl(graph.get_block(ddc_block_id))
        ddc_block.set_output_rate(args.rate, ddc_port)
        rate = ddc_block.get_output_rate(ddc_port)
    else:
        radio_block.set_rate(args.rate)
        rate = radio_block.get_rate()
    print(
        f"Requested RX rate: {args.rate/1e6} Msps, "
        f"actual RX rate: {rate} Msps")
    # Now do a power reading
    print(f"Estimating RX power from {args.samps_per_est/rate} s worth of signal...")
    power_dbfs = uhd.dsp.signals.get_usrp_power(
        rx_streamer, num_samps=int(args.samps_per_est))
    print(f"Received power: {power_dbfs:+6.2f} dBFS")
    return 0

if __name__ == "__main__":
    sys.exit(main())

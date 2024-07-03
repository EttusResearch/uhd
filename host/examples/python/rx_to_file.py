#!/usr/bin/env python3
#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
RX samples to file using Python API
"""

import argparse
import numpy as np
import uhd
from uhd.usrp import dram_utils
from uhd.types import StreamCMD, StreamMode


def parse_args():
    """Parse the command line arguments"""
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--args", default="", type=str)
    parser.add_argument("-o", "--output-file", type=str, required=True)
    parser.add_argument("-f", "--freq", type=float, required=True)
    parser.add_argument("-r", "--rate", default=1e6, type=float)
    parser.add_argument("-d", "--duration", default=5.0, type=float)
    parser.add_argument("-c", "--channels", default=0, nargs="+", type=int)
    parser.add_argument("-g", "--gain", type=int, default=10)
    parser.add_argument("-n", "--numpy", default=False, action="store_true",
                        help="Save output file in NumPy format (default: No)")
    parser.add_argument("--dram", action='store_true',
                        help="If given, will attempt to stream via DRAM")
    return parser.parse_args()

def multi_usrp_rx(args):
    """
    multi_usrp based RX example
    """
    usrp = uhd.usrp.MultiUSRP(args.args)
    num_samps = int(np.ceil(args.duration*args.rate))
    if not isinstance(args.channels, list):
        args.channels = [args.channels]
    samps = usrp.recv_num_samps(num_samps, args.freq, args.rate, args.channels, args.gain)
    with open(args.output_file, 'wb') as out_file:
        if args.numpy:
            np.save(out_file, samps, allow_pickle=False, fix_imports=False)
        else:
            samps.tofile(out_file)

def rfnoc_dram_rx(args):
    """
    rfnoc_graph + replay-block based RX example
    """
    # Init graph
    graph = uhd.rfnoc.RfnocGraph(args.args)
    num_samps = int(np.ceil(args.duration*args.rate))
    if graph.get_num_mboards() > 1:
        print(
            "ERROR: This example only supports DRAM streaming on a single "
            "motherboard.")
        return
    # Init radios and replay block
    available_radio_chans = [
        (radio_block_id, chan)
        for radio_block_id in graph.find_blocks("Radio")
        for chan in range(graph.get_block(radio_block_id).get_num_output_ports())
    ]
    radio_chans = [available_radio_chans[x] for x in args.channels]
    print("Receiving from radio channels:", end="")
    print("\n* ".join((f"{r}:{c}" for r, c in radio_chans)))
    dram = dram_utils.DramReceiver(graph, radio_chans, cpu_format='fc32')
    replay = dram.replay_blocks[0]
    print(f"Using replay block {replay.get_block_id()}")
    for (radio, radio_chan), ddc_info in zip(dram.radio_chan_pairs, dram.ddc_chan_pairs):
        radio.set_rx_frequency(args.freq, radio_chan)
        radio.set_rx_gain(args.gain, radio_chan)
        if ddc_info:
            ddc, ddc_chan = ddc_info
            ddc.set_output_rate(args.rate, ddc_chan)
        else:
            radio.set_rate(args.rate)
    # Overwrite default memory regions to maximize available memory
    mem_per_ch = int(replay.get_mem_size() / len(args.channels))
    mem_regions = [(idx * mem_per_ch, mem_per_ch) for idx, _ in enumerate(args.channels)]
    dram.mem_regions = mem_regions

    data = np.zeros((len(radio_chans), num_samps), dtype=np.complex64)
    stream_cmd = StreamCMD(StreamMode.num_done)
    stream_cmd.stream_now = True
    stream_cmd.num_samps = num_samps
    dram.issue_stream_cmd(stream_cmd)
    rx_md = uhd.types.RXMetadata()
    dram.recv(data, rx_md)
    with open(args.output_file, 'wb') as out_file:
        if args.numpy:
            np.save(out_file, data, allow_pickle=False, fix_imports=False)
        else:
            data.tofile(out_file)

def main():
    """RX samples and write to file"""
    args = parse_args()

    if args.dram:
        rfnoc_dram_rx(args)
    else:
        multi_usrp_rx(args)

if __name__ == "__main__":
    main()

#!/usr/bin/env python3
#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Stream received samples to the host and store in a file using Python API.

This example demonstrates how to use the UHD Python API to receive samples
from a USRP device and store them in a file on the host computer. It supports
two modes of operation:

1. Host based: This mode uses the MultiUSRP API
<https://files.ettus.com/manual/page_coding.html#coding_api_hilevel> to stream
received samples directly from the USRP to a host. This mode subjects the
host's ability (which depends on CPU speed, network interface speed, CPU thread
interruptions) to stream samples as fast as the USRP is receiving them.

2. RFNoC Replay: This mode uses advanced API functions to stream samples via
DRAM using capabilities of RFNoC (RF Network-on-Chip)
<https://www.ettus.com/sdr-software/rfnoc/>. The samples are first received and
buffered in DRAM and them streamed to host. This mode requires FPGA bitfiles
that contain the RFNoC Replay Block and is not supported by all USRP devices.

This example is useful for applications where users want to capture
samples from a USRP device for offline processing, analysis, or storage.

Example Usage:
rx_to_file.py --args addr=192.168.10.2 --output-file samples.dat --freq 2.45e9
              --rate 20e6 --duration 15 --channels 0 1 --gain 30

Note: argument --output-file needs to point to a location where thr user has
write access.
"""

import argparse

import numpy as np
import uhd
from uhd.types import StreamCMD, StreamMode
from uhd.usrp import dram_utils


def parse_args():
    """Parse the command line arguments."""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-a",
        "--args",
        default="",
        type=str,
        help="""specifies the USRP device arguments, which holds
        multiple key value pairs separated by commas
        (e.g., addr=192.168.40.2,type=x300) [default = ""].""",
    )
    parser.add_argument(
        "-o",
        "--output-file",
        type=str,
        required=True,
        help="specifies the name of the file(s) to write binary samples to, "
        "inserts channel number into the file name if more than one is given [default = None].",
    )
    parser.add_argument(
        "-f",
        "--freq",
        type=float,
        required=True,
        help="specifies the center frequency in Hz [input is required].",
    )
    parser.add_argument(
        "-r",
        "--rate",
        default=1e6,
        type=float,
        help="specifies the sample rate in samples/sec [default = 1e6].",
    )
    parser.add_argument(
        "-d",
        "--duration",
        default=5.0,
        type=float,
        help="specifies the RX stream duration in seconds [default = 5.0].",
    )
    parser.add_argument(
        "-c",
        "--channels",
        default=0,
        nargs="+",
        type=int,
        help='specifies the channel(s) to use (specify "0", "1", "0 1", etc) [default = 0].',
    )
    parser.add_argument(
        "-g",
        "--gain",
        type=int,
        default=10,
        help="specifies the receive gain in dB [default = 10].",
    )
    parser.add_argument(
        "-n",
        "--numpy",
        default=False,
        action="store_true",
        help="specifies whether to save the output file in NumPy format. If --numpy "
        "is not specified, the output file will be in binary format.",
    )
    parser.add_argument(
        "--dram",
        action="store_true",
        help="specifies the mode of operation (Host based or RFNoC Replay). "
        "If argument --dram is specified, the program will attempt to use the RFNoC "
        "Replay mode, else the Host Based is used.",
    )
    return parser.parse_args()


def multi_usrp_rx(args):
    """multi_usrp based RX example."""
    usrp = uhd.usrp.MultiUSRP(args.args)
    num_samps = int(np.ceil(args.duration * args.rate))
    if not isinstance(args.channels, list):
        args.channels = [args.channels]
    samps = usrp.recv_num_samps(num_samps, args.freq, args.rate, args.channels, args.gain)
    with open(args.output_file, "wb") as out_file:
        if args.numpy:
            np.save(out_file, samps, allow_pickle=False, fix_imports=False)
        else:
            samps.tofile(out_file)


def rfnoc_dram_rx(args):
    """rfnoc_graph + replay-block based RX example."""
    # Init graph
    graph = uhd.rfnoc.RfnocGraph(args.args)
    num_samps = int(np.ceil(args.duration * args.rate))
    if graph.get_num_mboards() > 1:
        print("ERROR: This example only supports DRAM streaming on a single " "motherboard.")
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
    dram = dram_utils.DramReceiver(graph, radio_chans, cpu_format="fc32")
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
    with open(args.output_file, "wb") as out_file:
        if args.numpy:
            np.save(out_file, data, allow_pickle=False, fix_imports=False)
        else:
            data.tofile(out_file)


def main():
    """RX samples and write to file."""
    args = parse_args()

    if args.dram:
        rfnoc_dram_rx(args)
    else:
        multi_usrp_rx(args)


if __name__ == "__main__":
    main()

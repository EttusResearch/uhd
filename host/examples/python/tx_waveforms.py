#!/usr/bin/env python3
#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Generate and TX samples using a set of waveforms, and waveform characteristics
"""

import time
import argparse
import numpy as np
import uhd
from uhd.usrp import dram_utils


def parse_args():
    """Parse the command line arguments"""
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--args", default="", type=str)
    parser.add_argument(
        "-w", "--waveform", default="sine",
        choices=['sine', 'square', 'const', 'ramp'], type=str)
    parser.add_argument("-f", "--freq", type=float, required=True)
    parser.add_argument("-r", "--rate", default=1e6, type=float)
    parser.add_argument("-d", "--duration", default=5.0, type=float)
    parser.add_argument("-c", "--channels", default=0, nargs="+", type=int)
    parser.add_argument("-g", "--gain", type=int, default=10)
    parser.add_argument("--wave-freq", default=1e4, type=float)
    parser.add_argument("--wave-ampl", default=0.3, type=float)
    parser.add_argument("--dram", action='store_true',
                        help="If given, will attempt to stream via DRAM")
    return parser.parse_args()



def multi_usrp_tx(args):
    """
    multi_usrp based TX example
    """
    usrp = uhd.usrp.MultiUSRP(args.args)
    if args.wave_freq == 0.0:
        desired_size = 1e6 # Just pick a value
    else:
        desired_size = 10 * np.floor(args.rate / args.wave_freq)
    data = uhd.dsp.signals.get_continuous_tone(
        args.rate,
        args.wave_freq,
        args.wave_ampl,
        desired_size=desired_size,
        max_size=(args.duration * args.rate),
        waveform=args.waveform)
    usrp.send_waveform(data, args.duration, args.freq, args.rate,
                       args.channels, args.gain)

def rfnoc_dram_tx(args):
    """
    rfnoc_graph + replay-block based TX example
    """
    # Init graph
    graph = uhd.rfnoc.RfnocGraph(args.args)
    if graph.get_num_mboards() > 1:
        print(
            "ERROR: This example only supports DRAM streaming on a single "
            "motherboard.")
        return
    # Init radios and replay block
    available_radio_chans = [
        (radio_block_id, chan)
        for radio_block_id in graph.find_blocks("Radio")
        for chan in range(graph.get_block(radio_block_id).get_num_input_ports())
    ]
    radio_chans = [available_radio_chans[x] for x in args.channels]
    print("Transmitting on radio channels:", end="")
    print("\n* ".join((f"{r}:{c}" for r, c in radio_chans)))
    dram = dram_utils.DramTransmitter(graph, radio_chans, cpu_format='fc32')
    replay = dram.replay_blocks[0]
    print(f"Using replay block: {replay.get_unique_id()}")
    for (radio, radio_chan), duc_info in zip(dram.radio_chan_pairs, dram.duc_chan_pairs):
        radio.set_tx_frequency(args.freq, radio_chan)
        radio.set_tx_gain(args.gain, radio_chan)
        if duc_info:
            duc, duc_chan = duc_info
            duc.set_output_rate(args.rate, duc_chan)
        else:
            radio.set_rate(args.rate)
    # Overwrite default memory regions to maximize available memory
    mem_regions = [(0, replay.get_mem_size()) for _ in args.channels]
    dram.mem_regions = mem_regions
    # Generate waveform
    data = uhd.dsp.signals.get_continuous_tone(
        args.rate,
        args.wave_freq,
        args.wave_ampl,
        desired_size=args.duration * args.rate,
        max_size=dram.replay_blocks[0].get_mem_size(),
        waveform=args.waveform)
    if args.duration > 0 and \
            len(radio_chans) == 1 and \
            dram.replay_blocks[0].get_mem_size() / 4 < args.duration * args.rate:
        if len(data) < args.duration * args.rate:
            # If we are using this API, we need to upload the entire waveform,
            # we can't make use of looping over the same memory region over and
            # over again.
            data = np.tile(data, int(args.duration * args.rate // len(data)) + 1)
            data = data[:args.duration * args.rate]
        # This if-branch is completely redundant, but we keep it here as this is
        # an example and we want to showcase different ways of using the
        # DramTransmitter class.
        print(f"Uploading waveform data ({data.nbytes/(1024**2):.2f} MiB) "
              f"and starting streaming...")
        tx_md = uhd.types.TXMetadata()
        tx_md.has_time_spec = False
        # These flags actually don't do anything; but if this was a regular TX
        # streamer object, that's what we would write here.
        tx_md.start_of_burst = True
        tx_md.end_of_burst = True
        dram.send(data, tx_md, 1.0)
    else:
        # Upload
        print(f"Uploading waveform data ({data.nbytes/(1024**2):.2f} MiB)...")
        dram.upload(data)
        # Start streaming
        stream_mode = uhd.types.StreamMode.start_cont \
                if args.duration <= 0 \
                else uhd.types.StreamMode.num_done
        stream_cmd = uhd.types.StreamCMD(stream_mode)
        if args.duration > 0:
            stream_cmd.num_samps = int(args.duration * args.rate)
        stream_cmd.stream_now = len(radio_chans) == 1
        stream_cmd.time_spec = dram.radio_chan_pairs[0][0].get_time_now() + 0.5
        print("Starting streaming...")
        dram.issue_stream_cmd(stream_cmd)
    if args.duration > 0:
        print("Waiting for transmission to complete...")
        time.sleep(args.duration + 1.0 + 0.5)
        async_timeout = time.monotonic() + 5.0
        async_md = None
        while time.monotonic() < async_timeout:
            async_md = dram.recv_async_msg(0.5)
            if async_md and async_md.event_code == uhd.types.TXMetadataEventCode.burst_ack:
                break
            if async_md:
                print(f"Caught TX event code: {async_md.event_code}")
                async_md = None
        if not async_md:
            print("ERROR: Unable to receive ACK after burst!")
    else:
        print("Transmitting (Hit Ctrl-C to stop)...")
        try:
            while True:
                time.sleep(1.0)
        except KeyboardInterrupt:
            print("Terminating streaming.")
        stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.stop_cont)
        stream_cmd.stream_now = True
        dram.issue_stream_cmd(stream_cmd)
        time.sleep(0.5)


def main():
    """TX samples based on input arguments"""
    args = parse_args()
    if not isinstance(args.channels, list):
        args.channels = [args.channels]
    if args.dram:
        rfnoc_dram_tx(args)
    else:
        multi_usrp_tx(args)

if __name__ == "__main__":
    main()

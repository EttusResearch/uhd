# !/usr/bin/env python3
#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
"""Example to transmit waveforms using UHD python API.

This example transmits samples of dynamically generated, common waveforms,
supporting sine, square, constant and ramp pattern. It's intended for use with
a single device and supports transmitting the same waveform on multiple
channels. The example implements two main operation modes that may not be
supported by all USRPs devices and FPGA bitfile flavors.

1. Host based: This mode uses the MultiUSRP API
<https://files.ettus.com/manual/page_coding.html#coding_api_hilevel> to stream
a continuous tone waveform to the USRP and transmit it on the fly. This mode
is supported by all USRPs and subjects the host's ability (which depends on CPU
speed, network interface speed, CPU thread interruptions) to stream samples as
fast as the USRP is transmitting them.

2. RFNoC Replay: This mode uses advanced API functions to buffer a waveform
in the USRP's onboard DRAM before transmitting it. It utilizes capabilities of
RFNoC (RF Network-on-Chip) <https://www.ettus.com/sdr-software/rfnoc/> together
to create a dynamic RFNoC graph connecting multiple RFNoC blocks stream a
waveform pattern from the host to DRAM and transmit the buffered waveform
pattern from DRAM. This mode requires FPGA bitfiles that contain the RFNoC
Replay Block and is not supported on all USRPs.

Example usage:
tx_waveforms.py --args addr=192.168.10.2 --freq 2.4e9 --rate 1e6 --duration 10
                --channels 0 --wave-freq 1e4 --wave-ampl 0.3 --dram
"""

import argparse
import time

import numpy as np
import uhd
from uhd.usrp import dram_utils


def parse_args():
    """Parses the command line arguments.

    Configuring the transmission, such as frequency, rate, duration,
    gain, waveform type, and whether to use DRAM for streaming.
    """
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=__doc__,
    )
    parser.add_argument(
        "-a",
        "--args",
        type=str,
        default="",
        help="""specifies the USRP device arguments, which holds
        multiple key value pairs separated by commas
        (e.g., addr=192.168.40.2,type=x300) [default = ""].""",
    )
    parser.add_argument(
        "-w",
        "--waveform",
        choices=["sine", "square", "const", "ramp"],
        type=str,
        default="sine",
        help="specifies the waveform type [Default = sine]",
    )
    parser.add_argument(
        "-f",
        "--freq",
        type=float,
        required=True,
        help="specifies the RF center frequency in Hz (input is required)",
    )
    parser.add_argument(
        "-r",
        "--rate",
        type=float,
        default=1e6,
        help="specifies the sample rate in samples/seconds of the waveform generated at the host [Default = 1e6]",
    )
    parser.add_argument(
        "-d",
        "--duration",
        type=float,
        default=5.0,
        help="""specifies the transmit duration in seconds.
        If duration = -1 and --dram argument is used, the waveform pattern is repeated continuously until stopped.
        If duration > 0, the transmission stops after the configured duration.
        [Default = 0.5]""",
    )
    parser.add_argument(
        "-c",
        "--channels",
        nargs="+",
        type=int,
        default=0,
        help='specifies the channels to use (e.g., "0", "1", "0 1", etc) [Default = 0]',
    )
    parser.add_argument(
        "-g",
        "--gain",
        type=int,
        default=10,
        help="specifies the gain for the RF chain in dB scale [Default = 10]",
    )
    parser.add_argument(
        "--wave-freq",
        type=float,
        default=1e4,
        help="specifies the waveform frequency in Hz [Default = 1e4]",
    )
    parser.add_argument(
        "--wave-ampl",
        type=float,
        default=0.3,
        help="specifies the waveform amplitude in the range [0 to 0.7] [Default = 0.3]",
    )
    parser.add_argument(
        "--tx-delay",
        type=float,
        default=0.5,
        help="specifies the delay in seconds (relative to when the command is executed) before simultaneously starting transmission on all channels [Default = 0.5]",
    )
    parser.add_argument(
        "--dram",
        action="store_true",
        help="specifies the operation mode (Host based or RFNoC Replay). If argument --dram is specified, the program will attempt to use RFNoC Replay mode, else Host Based",
    )
    return parser.parse_args()


def multi_usrp_tx(args):
    """multi_usrp based TX example.

    The function sets up and transmits a waveform using
    the uhd.usrp.MultiUSRP class. It configures the USRP device, generates a
    continuous tone waveform, streams it to the USRP and transmits it.
    """
    usrp = uhd.usrp.MultiUSRP(args.args)
    if args.wave_freq == 0.0:
        desired_size = 1e6  # Default size of the transmission buffer (or burst)
    else:
        desired_size = 10 * np.floor(args.rate / args.wave_freq)

    tx_start_time = None  # Start transmission immediately
    # Delay transmission start. This will allow SW to apply all
    # configurations and start transmission on all channels simultaneously.
    # If tx_delay is too small, tx_start_time may be in the past by the time
    # all configurations are applied and late command errors being reported.
    tx_start_time = usrp.get_time_now() + args.tx_delay
    print("Generating waveform...")
    data = uhd.dsp.signals.get_continuous_tone(
        args.rate,
        args.wave_freq,
        args.wave_ampl,
        desired_size=desired_size,
        max_size=(args.duration * args.rate),
        waveform=args.waveform,
    )
    print(
        f"Starting to stream waveform at the rate of {args.rate} samples/sec for the duration of {args.duration} seconds..."
    )
    usrp.send_waveform(
        data, args.duration, args.freq, args.rate, args.channels, args.gain, tx_start_time
    )
    print("Transmission complete.")


def rfnoc_dram_tx(args):
    """rfnoc_graph + replay-block based TX example.

    Refer Replay Block usage here <https://kb.ettus.com/Using_the_RFNoC_Replay_Block>.
    The function uses the RFNoC (RF Network-on-Chip) framework to transmit waveforms
    via DRAM. It initializes a graph, configures radio channels, sets rates and
    frequencies, uploads waveform data to DRAM, and starts streaming.
    """
    # Init graph
    graph = uhd.rfnoc.RfnocGraph(args.args)
    if graph.get_num_mboards() > 1:
        print("ERROR: This example only supports DRAM streaming on a single motherboard.")
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
    dram = dram_utils.DramTransmitter(graph, radio_chans, cpu_format="fc32")
    replay = dram.replay_blocks[0]
    print(f"Using replay block: {replay.get_unique_id()}")
    # Separate loops for setting rate and frequency to minimize timed cmd queue
    for (radio, radio_chan), duc_info in zip(dram.radio_chan_pairs, dram.duc_chan_pairs):
        radio.set_tx_gain(args.gain, radio_chan)
        if duc_info:
            duc, duc_chan = duc_info
            duc.set_output_rate(args.rate, duc_chan)
        else:
            radio.set_rate(args.rate)

    if len(dram.radio_chan_pairs) > 1:
        # Use timed tuning for more than one channel to produce consistent
        # phase offsets between channels.
        cmd_time_offset = 0.1
        cmd_time = dram.radio_chan_pairs[0][0].get_time_now() + cmd_time_offset
        for radio, radio_chan in dram.radio_chan_pairs:
            radio.set_command_time(cmd_time, radio_chan)
            radio.set_tx_frequency(args.freq, radio_chan)
        for radio, radio_chan in dram.radio_chan_pairs:
            radio.clear_command_time(radio_chan)
        # Enough time for tune time to expire
        time.sleep(cmd_time_offset)
    else:
        for radio, radio_chan in dram.radio_chan_pairs:
            radio.set_tx_frequency(args.freq, radio_chan)

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
        waveform=args.waveform,
    )
    if (
        args.duration > 0
        and len(radio_chans) == 1
        and dram.replay_blocks[0].get_mem_size() / 4 < args.duration * args.rate
    ):
        if len(data) < args.duration * args.rate:
            # If we are using this API, then we need to upload the entire waveform.
            # We can't make use of looping over the same memory region over and
            # over again.
            data = np.tile(data, int(args.duration * args.rate // len(data)) + 1)
            data = data[: args.duration * args.rate]
        # This if-branch is completely redundant, but we keep it here as this is
        # an example and we want to showcase different ways of using the
        # DramTransmitter class.
        print(
            f"Uploading waveform data ({data.nbytes/(1024**2):.2f} MiB) "
            f"Starting to stream waveform at the rate of {args.rate} samples/sec for the duration of {args.duration} seconds..."
        )
        tx_md = uhd.types.TXMetadata()
        # do not use time spec if tx_delay is 0
        if args.tx_delay == 0:
            tx_md.has_time_spec = False
        else:
            # In this case the tx_start_time needs to be sufficiently in the future
            # to also allow uploading of the data to the DRAM.
            tx_md.has_time_spec = True
            tx_md.time_spec = dram.radio_chan_pairs[0][0].get_time_now() + args.tx_delay
        # These flags actually don't do anything; but if this was a regular TX
        # streamer object, that's what we would write here.
        tx_md.start_of_burst = True
        tx_md.end_of_burst = True
        # Upload and send at time specified by tx_start_time
        dram.send(data, tx_md, 1.0)
    else:
        # Upload
        print(f"Uploading waveform data ({data.nbytes/(1024**2):.2f} MiB)...")
        dram.upload(data)
        # Start streaming
        stream_mode = (
            uhd.types.StreamMode.start_cont if args.duration <= 0 else uhd.types.StreamMode.num_done
        )
        stream_cmd = uhd.types.StreamCMD(stream_mode)
        if args.duration > 0:
            stream_cmd.num_samps = int(args.duration * args.rate)
        if args.tx_delay == 0:
            stream_cmd.stream_now = True
        else:
            stream_cmd.stream_now = False
            stream_cmd.time_spec = dram.radio_chan_pairs[0][0].get_time_now() + args.tx_delay
        print(
            f"Starting to stream waveform at the rate of {args.rate} samples/sec for the duration of {args.duration} seconds..."
        )
        dram.issue_stream_cmd(stream_cmd)
    if args.duration > 0:
        print("Waiting for transmission to complete...")
        # Sleep time allows for 1s of extra time for the transmission to complete
        time.sleep(args.duration + 1.0 + args.tx_delay)
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
        print("Transmission complete.")
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
        time.sleep(args.tx_delay)


def main():
    """Transmit samples based on the input arguments."""
    args = parse_args()
    if not isinstance(args.channels, list):
        args.channels = [args.channels]
    if args.dram:
        rfnoc_dram_tx(args)
    else:
        multi_usrp_tx(args)


if __name__ == "__main__":
    main()

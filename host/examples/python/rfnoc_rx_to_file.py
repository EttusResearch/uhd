#!/usr/bin/env python3
#
# Copyright 2025 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""RFNoC RX streaming example.

This example demonstrates the usage of the RFNoC API to receive data from one
or more USRP's and write it to disk. The example will create a
RX streamer for the requested channels and start streaming. The received samples
are written to a file per channel. The example will stop after a given duration
or after a given number of samples have been received.

Example usage:
rfnoc_rx_to_file.py --args addr=192.168.10.2 --rate 1e6 --freq 1e9 --output rx_samples.dat
"""

import argparse
import collections
import contextlib
import os
import signal
import time

import numpy as np
import uhd

# Named tuple to hold all relevant information for the RX configuration.
RxInfo = collections.namedtuple("RxInfo", ["rx_streamer", "radio_ports", "ddcs"])


class SignalHandler:
    """Signal handler to be used within a context.

    It will catch the given signal and set the signaled flag to True.
    The release method will restore the original signal handler.
    """

    def __init__(self, sig=signal.SIGINT):
        """The constructor."""
        self.signaled = False
        self.released = False
        self.sig = sig
        self._save_handler = None

    def __enter__(self):
        """Set the signal handler."""
        self.signaled = False
        self.released = False

        self._save_handler = signal.getsignal(self.sig)

        def handler(_sig, _frame):
            self.release()
            self.signaled = True

        signal.signal(self.sig, handler)
        return self

    def __exit__(self, _type, _value, _tb):
        """Release the signal handler."""
        self.release()

    def release(self):
        """Restore the original signal handler and set the current to released, if any."""
        if self.released:
            return False
        signal.signal(self.sig, self._save_handler)
        self.released = True
        return True


# Maps the cpu_format to the corresponding numpy data type.
CPU_NUMPY_MAPPING = {
    "sc8": np.dtype([("re", np.int8), ("im", np.int8)]),
    "sc16": np.dtype([("re", np.int16), ("im", np.int16)]),
    "fc32": np.complex64,
    "fc64": np.complex128,
}


# Maps the otw_format to the corresponding sample size in bytes.
OTW_SIZE_MAPPING = {"sc8": 2, "sc16": 4}


def parse_args():
    """Parse the command line arguments."""
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter, prog=__name__, description=__doc__
    )
    parser.add_argument(
        "-a",
        "--args",
        default="",
        required=True,
        help="device arguments which holds multiple key value pairs separated by commas (e.g., addr=192.168.40.2,type=x300)",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=str,
        default="usrp_samples.dat",
        help="name of the file(s) to write binary samples to, "
        "injects channel number into the file name if more than one is given",
    )
    parser.add_argument(
        "-d",
        "--duration",
        default=None,
        type=float,
        help="stream duration (seconds), leave out to stream until stopped",
    )
    parser.add_argument(
        "-n", "--num_samples", default=0, type=int, help="total number of samples to receive"
    )
    parser.add_argument(
        "-b", "--buffer-size", default=10000, type=int, help="buffer size for single receive call"
    )
    parser.add_argument(
        "-C", "--continue-on-bad-packet", default=True, type=bool, help="do not abort on bad packet"
    )

    parser.add_argument(
        "-u",
        "--cpu-format",
        default="fc32",
        type=str,
        choices=("sc8", "sc16", "fc32", "fc64"),
        help="file sample format",
    )
    parser.add_argument(
        "-w",
        "--otw-format",
        default="sc16",
        type=str,
        choices=("sc8", "sc16"),
        help="over-the-wire sample format",
    )
    parser.add_argument("-r", "--rate", type=float, required=True, help="sample rate (samples/sec)")
    parser.add_argument("-f", "--freq", type=float, required=True, help="center frequency (Hz)")
    parser.add_argument("-g", "--gain", type=float, default=0.0, help="gain (dB)")
    parser.add_argument(
        "-t", "--antenna", help="USRP RX antenna to be used (same for all channels)"
    )
    parser.add_argument(
        "-c",
        "--channels",
        default=None,
        nargs="+",
        type=int,
        help='which channel(s) to use (specify "0", "1", "0 1", etc.'
        "Leave empty for all channels)",
    )
    parser.add_argument(
        "--block_id",
        default=None,
        type=str,
        help="specifies the ID of custom RfNoC block when connecting it to the chain "
        "between the radio and the host.",
    )
    parser.add_argument(
        "--block_port",
        default=0,
        type=int,
        help="specifies which port of custom RfNoC block should be used when connecting it"
        "to the chain between radio and the host.",
    )
    parser.add_argument(
        "--block_props",
        default=None,
        type=str,
        help="specifies the additional properties to be passed to the block once connected "
        "to the RfNoC graph (see set_properties()).",
    )
    return parser.parse_args()


def map_channels_to_radio_port(radios, channels):
    """Returns a map of channels to radio ports.

    Create a mapping of radio ID and ports versus channels. The channels can
    be in any order and the radio ID ports will be selected accordingly. So
    to get a reverse mapping of the first four channels pass [3, 2, 1, 0] as
    channels. If channels is not given, a sequential list of all available
    channels is assumed.
    """
    mapping = [(radio, port) for radio in radios for port in range(radio.get_num_output_ports())]
    if not channels:
        channels = range(len(mapping))

    if min(channels) < 0 or max(channels) >= len(mapping):
        raise ValueError(
            "Invalid channel(s) specified. "
            "Current graph supports channels from 0 to {(len(mapping) - 1)}"
        )

    # filter all elements from mapping that are not in channels, this also
    # ensures the radio ID and port pairs are sorted in the give channel order
    return (list(map(lambda x: mapping[x], channels)), channels)


def get_radios(graph):
    """Returns a list of RadioControl objects.

    Generate a list of RadioControl objects for all radios in the given graph.
    The order of the radios here define the available channels. Channels are
    counted from zero, the number of channels is the sum of all channels in all
    radios and channels are ordered according to the order of the radios (first
    all channels of radio 0, then all channels of radio 1 and so on).
    """
    radio_ids = graph.find_blocks("Radio")
    # map the radio blocks to RadioControl objects, radios will be sorted by channels order
    return list(map(lambda radio_id: uhd.rfnoc.RadioControl(graph.get_block(radio_id)), radio_ids))


def connect_user_block(graph, edges, args, last_block_in_chain, last_port_in_chain):
    """Connect user block to singal processing chain.

    The user block is dynamically inserted after the last block in the chain.
    Also, it is ensured that there are no more static connections after the
    user block.
    """
    if args.block_id:
        # check if the user block is already in the chain
        user_block_found = False
        for edge in edges:
            if edge.dst_port == args.block_id:
                user_block_found = True
                print(f"User block {args.block_id} already in the chain, skipping connection.")

        # If the user block is not in the chain yet, see if we can connect that
        # separately
        if not user_block_found:
            user_block_id = uhd.types.lib.rfnoc.block_id(args.block_id)
            if not graph.has_block(user_block_id):
                raise RuntimeError(f"Block with ID {args.block_id} not found in the graph.")

            print(
                f"Attempting to connect {args.block_id} : {last_port_in_chain} to {last_block_in_chain} : {args.block_port} ..."
            )
            uhd.rfnoc.connect_through_blocks(
                graph,
                last_block_in_chain,
                last_port_in_chain,
                user_block_id,
                args.block_port,
            )
            print(
                f"Connected {args.block_id} : {last_port_in_chain} to {last_block_in_chain} : {args.block_port} ..."
            )
            last_block_in_chain = uhd.types.lib.rfnoc.block_id(args.block_id)
            last_port_in_chain = args.block_port
            #  Now we have to make sure that there are no more static connections
            # after the user-defined block
            edges_new = uhd.rfnoc.get_block_chain(
                graph, last_block_in_chain, last_port_in_chain, True
            )
            if len(edges_new) > 1:
                uhd.rfnoc.connect_through_blocks(
                    graph,
                    last_block_in_chain,
                    last_port_in_chain,
                    edges_new[-1].src_blockid,
                    edges_new[-1].src_port,
                )
                last_block_in_chain = edges_new[-1].src_blockid
                last_port_in_chain = edges_new[-1].src_port
    return (last_block_in_chain, last_port_in_chain)


def connected_ddcs(graph):
    """Returns a list of connected DDC blocks.

    Create a list of block ID, port pairs of connected DDC blocks
    in the active graph. The graph must be committed.
    """
    result = []
    for conn in graph.enumerate_active_connections():
        if "DDC" in conn.src_blockid:
            result.append((conn.src_blockid, conn.src_port))
    return result


def connect_radio(graph, args, radio_id, radio_chan, streamer, streamer_chan):
    """Connect radio to rx streamer.

    Connect the given radio/channel pair to streamer/channel pair. This
    utilizes connect_through_block which will either find a static connection,
    connect via stream end points or throw an error if none of that works.
    """
    edges = uhd.rfnoc.get_block_chain(graph, radio_id, radio_chan, True)
    if not edges:
        raise RuntimeError(f"No chain found for {radio_id} ({radio_chan})")

    last_block_in_chain = edges[-1].src_blockid
    last_port_in_chain = edges[-1].src_port

    if len(edges) > 1:
        uhd.rfnoc.connect_through_blocks(
            graph, radio_id, radio_chan, last_block_in_chain, last_port_in_chain
        )

    last_block_in_chain, last_port_in_chain = connect_user_block(
        graph, edges, args, last_block_in_chain, last_port_in_chain
    )

    graph.connect(last_block_in_chain, last_port_in_chain, streamer, streamer_chan)


def connect_radios(graph, args):
    """Connect all radios to matching rx streamer.

    Create an RX streamer and connect it to all radio channels requested by
    args.channels. It returns an named tuple with the generated streamer, all
    used radio ID/ports and the DDC blocks involved in the graph.
    """
    radios = get_radios(graph)
    (radio_ports, args.channels) = map_channels_to_radio_port(radios, args.channels)

    rx_streamer = graph.create_rx_streamer(
        len(radio_ports), uhd.usrp.StreamArgs(args.cpu_format, args.otw_format)
    )

    for idx, (radio, port) in enumerate(radio_ports):
        connect_radio(graph, args, radio.get_unique_id(), port, rx_streamer, idx)
    graph.commit()
    ddcs = connected_ddcs(graph)

    if len(ddcs) > 1:
        assert len(radio_ports) == len(ddcs)

    return RxInfo(rx_streamer, radio_ports, ddcs)


def apply_settings(graph, rx_info, args):
    """Apply the given settings to the radios and the DDCs.

    This includes setting the frequency, gain and output rate. Setting
    the output rate on the DDC is the easiest way to propagate rates to
    the graph. The DDC will adjust the sample rate of the radio automatically.
    """
    for i, (radio, port) in enumerate(rx_info.radio_ports):
        radio.set_rx_frequency(args.freq, port)
        radio.set_rx_gain(args.gain, port)
        if not rx_info.ddcs:
            radio.set_rate(args.rate)
        else:
            ddc_block = uhd.rfnoc.DdcBlockControl(graph.get_block(rx_info.ddcs[i][0]))
            ddc_block.set_output_rate(args.rate, rx_info.ddcs[i][1])

    # set block properties if given
    if args.block_id and args.block_props:
        block = uhd.rfnoc.BlockControl(graph.get_block(args.block_id))
        block.set_properties(args.block_props)


def start_stream(graph, rx_info, args):
    """Starts streaming.

    If number of samples is given use num_done otherwise continuous streaming.
    stream_now is off because it would only work for single device.
    """
    mode = (
        uhd.types.StreamMode.start_cont if args.num_samples == 0 else uhd.types.StreamMode.num_done
    )
    stream_cmd = uhd.types.StreamCMD(mode)
    stream_cmd.num_samps = args.num_samples

    stream_cmd.stream_now = False
    time_now = graph.get_mb_controller().get_timekeeper(0).get_time_now()
    stream_cmd.time_spec = time_now + uhd.types.TimeSpec(0.01)
    rx_info.rx_streamer.issue_stream_cmd(stream_cmd)

    return stream_cmd


def check_stop_conditions(sig_handler, start, duration, received_samples, num_samples):
    """Checks for stop conditions.

    Check whether any stop condition is met. This could be either:
    * a sig interrupt received
    * duration expired (if given)
    * enough samples received (if given)
    """
    if sig_handler.signaled:
        print("Received a signal interrupt.")
        return True
    if duration and (time.monotonic() - start) >= duration:
        print("Received samples for the configured duration.")
        return True
    if 0 < num_samples <= received_samples:
        print("Received all the samples configured by the num_samples.")
        return True
    return False


def check_errors(metadata, received, reported_overflow, data_rate, continue_on_bad_packet):
    """Check for error conditions."""
    if metadata.error_code == uhd.types.RXMetadataErrorCode.timeout:
        print("Timeout while streaming!")
        return (True, reported_overflow)

    if metadata.error_code == uhd.types.RXMetadataErrorCode.overflow:
        if not reported_overflow:
            error_name = "out-of-sequence" if metadata.out_of_sequence else metadata.error_code.name
            print(f"Got an {error_name} indication. Please consider the following:")
            print(f"  Your write medium must sustain a rate of {data_rate/1e6:0.3f}MB/s.")
            print("  Dropped samples will not be written to the file.")
            print("  Please modify this example for your purposes.")
            print("  This message will not appear again.")
        return (False, True)

    if metadata.error_code != uhd.types.RXMetadataErrorCode.none:
        print(f"Got an error indication: {metadata.strerror()}")
        return (not continue_on_bad_packet, reported_overflow)

    return (False, reported_overflow)


def main(sig_handler):
    """Main function to receive data from USRP using RFNoC API."""
    args = parse_args()

    graph = uhd.rfnoc.RfnocGraph(args.args)
    rx_info = connect_radios(graph, args)

    apply_settings(graph, rx_info, args)

    num_chans = len(rx_info.radio_ports)

    data_rate = args.rate * num_chans * OTW_SIZE_MAPPING[args.otw_format]
    reported_overflow = False

    samples = np.zeros((num_chans, args.buffer_size), dtype=CPU_NUMPY_MAPPING[args.cpu_format])

    def gen_filename(name, channel):
        """Generate filename.

        Generate a filename for the given channel based on name. A channel
        number is inserted only if there are multiple channels.
        """
        if len(rx_info.radio_ports) == 1:
            return name
        (fname, ext) = os.path.splitext(name)
        return f"{fname}_ch_{channel}{ext}"

    with contextlib.ExitStack() as stack:
        files = [
            stack.enter_context(open(gen_filename(args.output, ch), "wb")) for ch in args.channels
        ]

        stream_cmd = start_stream(graph, rx_info, args)

        start = time.monotonic()

        rx_md = uhd.types.RXMetadata()
        num_rx = 0
        print("Start streaming...")
        while True:
            received = rx_info.rx_streamer.recv(samples, rx_md, 2 * args.buffer_size / args.rate)
            num_rx += received

            for chan in range(num_chans):
                files[chan].write(samples[chan, :received])

            (error, reported_overflow) = check_errors(
                rx_md, received, reported_overflow, data_rate, args.continue_on_bad_packet
            )
            if error:
                break

            if check_stop_conditions(sig_handler, start, args.duration, num_rx, args.num_samples):
                break

        if stream_cmd.stream_mode == uhd.types.StreamMode.start_cont:
            stream_cmd.stream_mode = uhd.types.StreamMode.stop_cont
            rx_info.rx_streamer.issue_stream_cmd(stream_cmd)


if __name__ == "__main__":

    with SignalHandler() as signal_handler:
        main(signal_handler)

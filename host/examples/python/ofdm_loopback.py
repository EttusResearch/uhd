#!/usr/bin/env python3
#
# Copyright 2024 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Demonstrates how to send and receive OFDM frames using the radio's digital
loopback. All port of single daughter board are used in this example. The
transmitted and received data is plotted as a frequency spectrum.

A cyclic prefix (CP) schedule can provided, in which cast the OFDM uplink will
do CP insertion and the OFDM downlink block will do CP removal.

The data flow is as follows:

    Host Tx Streamer ─> OFDM uplink ─> Radio Tx ─>─┐
                                                   │
    ┌───────────< Digital Loopback <───────────────┘
    │
    └─> Radio Rx ─> OFDM down ─> Host Rx Streamer

It assumes the RFNoC topology like the one shown below. The configuration of the
SFPs may be 10 GbE (1 to 4 ports) or 100 GbE. The number of channels per radio
can be 1 or 2.

                     ┌────────────┐        Ch0   ┌─────────┐     ┌─────────┐
                     │            ├─────────────>│         ├────>│         │
                     │        sep0│        Ch1   │  OFDM#0 │     │Tx       │
                     │            │<───┐ ┌──────>│         ├────>│         │
                     │            │    │ │       └─────────┘     │         │
 ┌──────┐    sfp0    │            │  ┌─┼─┘                       │ Radio#0 │
 │      │<──────────>│            │  │ │   Ch0   ┌─────────┐     │         │
 │      │            │            ├──┘ └─────────┤         │<────┤         │
 │      │    sfp1    │        sep1│        Ch1   │  OFDM#1 │     │Rx       │
 │      │<──────────>│            │<─────────────┤         │<────┤         │
 │ Host │            │            │              └─────────┘     └─────────┘
 │      │    sfp2    │    RFNoC   │
 │      │<──────────>│  Crossbar  │
 │      │            │            │
 │      │    sfp3    │            │        Ch2   ┌─────────┐     ┌─────────┐
 │      │<──────────>│            ├─────────────>│         ├────>│         │
 └──────┘            │        sep2│        Ch3   │  OFDM#2 │     │Tx       │
                     │            │<───┐ ┌──────>│         ├────>│         │
                     │            │    │ │       └─────────┘     │         │
                     │            │  ┌─┼─┘                       │ Radio#1 │
                     │            │  │ │   Ch2   ┌─────────┐     │         │
                     │            ├──┘ └─────────┤         │<────┤         │
                     │        sep3│        Ch3   │  OFDM#3 │     │Rx       │
                     │            │<─────────────┤         │<────┤         │
                     └────────────┘              └─────────┘     └─────────┘

"""

import sys
import argparse
import math
import uhd

import numpy as np
import matplotlib.pyplot as plt


# Convert complex128 NumPy to sc16 stores as uint32
def complex128_to_sc16(data, scale=32767):
    real = np.int16(np.real(data) * scale)
    imag = np.int16(np.imag(data) * scale)
    return np.uint32((np.uint32(imag) << 16) | (np.uint32(real) & 0xFFFF))


# Convert sc16 stored as uint32 to complex128 NumPy
def sc16_to_complex128(data):
    real = np.array(np.int16(data & 0xFFFF), dtype=np.float64) / 32767
    imag = np.array(np.int16((data >> 16) & 0xFFFF), dtype=np.float64) / 32767
    return np.array(real + 1j * imag, dtype=np.complex128)


# Generate a complex128 NumPy sine wave
def complex_sinusoid(num_samples, norm_freq, amplitude=0.5, phase=0.0):
    n = np.arange(num_samples)
    return amplitude * np.exp(1j * (2 * np.pi * norm_freq * (n + phase)))


# Generate a complex128 NumPy sine wave in the frequency domain
def complex_sinusoid_freq(num_samples, norm_freq, amplitude=0.5, phase=0.0):
    # Generate num_samples zeros
    spectrum = np.zeros(num_samples, dtype=np.complex128)
    spectrum[round(num_samples * norm_freq)] = amplitude + 0j
    return spectrum


# Calculate the samples added by the cyclic prefix insertion
def total_cp_length(cp_list, num_ffts):
    if len(cp_list) == 0:
        return 0
    sum = 0
    for count in range(num_ffts):
        sum += cp_list[count % len(cp_list)]
    return sum


def parse_args():
    """Parse the command line arguments"""
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "-a",
        "--args",
        type=str,
        default="",
        help="Device args (e.g., addr=192.168.10.2)",
    )
    parser.add_argument(
        "-s", "--fft-size", type=int, default=4096, help="FFT size (default = 4096)"
    )
    parser.add_argument(
        "-n",
        "--num-ffts",
        type=int,
        default=14,
        help="Number of FFTs to transmit/receive (default=14)",
    )
    parser.add_argument(
        "-p",
        "--cp-list",
        type=int,
        nargs="+",
        default=[],
        help="List of cyclic prefix lengths to use (default=[])",
    )
    parser.add_argument(
        "-u",
        "--uplink",
        type=int,
        default=0,
        help="OFDM block number to use as OFDM uplink (default=0)",
    )
    parser.add_argument(
        "-d",
        "--downlink",
        type=int,
        default=1,
        help="OFDM block number to use as OFDM downlink (default=1)",
    )
    parser.add_argument(
        "-r",
        "--radio",
        type=int,
        default=0,
        help="Radio block number to use (default=0)",
    )
    return parser.parse_args()


def main():
    args = parse_args()

    # Fetch the arguments
    fft_size = args.fft_size
    num_ffts = args.num_ffts
    uplink_blk = f"0/OFDM#{args.uplink}"
    downlink_blk = f"0/OFDM#{args.downlink}"
    radio_blk = f"0/Radio#{args.radio}"
    cp_list = args.cp_list
    # Here's an example:
    # cp_list = [352, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288]

    # Choose a "samples per packet" (SPP) to use that evenly divides the FFT
    # size but won't exceed the USRPs 8 KB packet size limit (with header).
    spp = fft_size
    while spp > 1996:
        spp = spp // 2
    print(f"Using samples per packet of {spp}")

    # Get references for OFDM and radio blocks
    graph = uhd.rfnoc.RfnocGraph(args.args)
    ofdm_u = uhd.rfnoc.OfdmBlockControl(graph.get_block(uplink_blk))
    ofdm_d = uhd.rfnoc.OfdmBlockControl(graph.get_block(downlink_blk))
    radio = uhd.rfnoc.RadioControl(graph.get_block(radio_blk))

    print("Using OFDM uplink block:", uplink_blk)
    print("Using OFDM downlink block:", downlink_blk)
    print("Using radio block:", radio_blk)

    if len(cp_list):
        print("Using cyclic prefix list:", cp_list)

    # Figure out how many channels are on the OFDM block
    num_chan = ofdm_u.get_num_input_ports()
    print(f"Found {num_chan} channels on OFDM block")

    # Configure RFNoC graph
    sa = uhd.usrp.StreamArgs("sc16", "sc16")
    sa.args = "spp=" + str(spp)
    tx_streamer = graph.create_tx_streamer(num_chan, sa)
    rx_streamer = graph.create_rx_streamer(num_chan, sa)
    for chan in range(num_chan):
        graph.connect(tx_streamer, chan, ofdm_u.get_unique_id(), chan)
        graph.connect(ofdm_d.get_unique_id(), chan, rx_streamer, chan)
        graph.connect(ofdm_u.get_unique_id(), chan, radio.get_unique_id(), chan)
        graph.connect(radio.get_unique_id(), chan, ofdm_d.get_unique_id(), chan)
    graph.commit()

    # FFT/IFFT scaling schedule. See Xilinx Fast Fourier Transform v9.1 Product
    # Guide (PG901) for details.
    ifft_scale = 0b_00_00_00_00_00_00  # No scaling
    fft_scale = 0b_10_10_10_10_10_10  # 1/N scaling

    # Configure the OFDM uplink block
    ofdm_u.reset()
    ofdm_u.clear_cp_insertion_fifo()
    ofdm_u.clear_cp_removal_fifo()
    ofdm_u.set_fft_size(fft_size)
    ofdm_u.set_fft_direction(uhd.libpyuhd.rfnoc.ofdm_direction.REVERSE)
    ofdm_u.set_fft_scaling(ifft_scale)
    if len(cp_list):
        ofdm_u.load_cp_insertion_fifo(cp_list)

    # Configure the OFDM downlink block
    ofdm_d.reset()
    ofdm_d.clear_cp_insertion_fifo()
    ofdm_d.clear_cp_removal_fifo()
    ofdm_d.set_fft_size(fft_size)
    ofdm_d.set_fft_direction(uhd.libpyuhd.rfnoc.ofdm_direction.FORWARD)
    ofdm_d.set_fft_scaling(fft_scale)
    if len(cp_list):
        ofdm_d.load_cp_removal_fifo(cp_list)

    # Configure the radio block
    for chan in range(num_chan):
        # Enable digital loopback inside in the radio block. No external
        # connection is required on the USRP.
        radio.poke32(0x1000 + 128 * chan, 1)

        # Set the radio's RX packet size
        radio.set_properties("spp=" + str(spp), chan)

    # Create a set of symbols to send
    symbols_in = np.zeros((num_chan, num_ffts * fft_size), dtype=np.complex128)
    data_in = np.zeros((num_chan, num_ffts * fft_size), dtype=np.uint32)
    for chan in range(num_chan):
        for count in range(num_ffts):
            start = count * fft_size
            end = start + fft_size
            symbols_in[chan][start:end] = complex_sinusoid_freq(
                fft_size, (chan + 1) / (num_chan * 4), 0.99
            )
            data_in[chan][start:end] = complex128_to_sc16(symbols_in[chan][start:end])

    # Pick a time in the future to transmit and receive the data
    time_now = radio.get_time_now().get_real_secs()
    tx_time = time_now + 1.0
    # With internal digital loopback, there's a fixed delay between TX and RX
    # of two samples.
    rx_time = time_now + 1.0 + 2 / radio.get_tick_rate()

    # Issue receive command for the future time to capture the data when it is
    # transmitted. We must also account for the cyclic prefix length, which
    # means our acquisition length needs to be longer.
    stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.num_done)
    stream_cmd.stream_now = False
    stream_cmd.time_spec = uhd.types.TimeSpec(rx_time)
    stream_cmd.num_samps = fft_size * num_ffts + total_cp_length(cp_list, num_ffts)
    rx_streamer.issue_stream_cmd(stream_cmd)

    # Send symbols to the OFDM uplink block
    tx_md = uhd.types.TXMetadata()
    tx_md.time_spec = uhd.types.TimeSpec(tx_time)
    tx_md.has_time_spec = True
    tx_md.start_of_burst = True
    tx_md.end_of_burst = True
    num_tx = tx_streamer.send(data_in, tx_md, timeout=5.0)
    print(f"Sent {num_tx} samples")

    # Receive the result from the OFDM downlink block
    rx_md = uhd.types.RXMetadata()
    data_out = np.zeros((num_chan, num_ffts * fft_size), dtype=np.uint32)
    num_rx = 0
    num_rx = rx_streamer.recv(data_out, rx_md, 5.0)
    print(f"Received {num_rx} samples")

    # Convert the results back to complex floating point
    symbols_out = np.zeros((num_chan, num_ffts * fft_size), dtype=np.complex128)
    for chan in range(num_chan):
        symbols_out[chan] = sc16_to_complex128(data_out[chan])

    # Choose which FFT result to check and plot
    fft_index = 0  # Plot the first one
    start = fft_index * fft_size  # First sample of FFT
    end = start + fft_size  # Last sample of FFT (plus 1)

    # Print where the peaks occur for the FFT result
    threshold = 500
    for chan in range(num_chan):
        for i in range(start, end):
            real = np.int16(data_in[chan][i] & 0xFFFF)
            imag = np.int16((data_in[chan][i] >> 16) & 0xFFFF)
            mag = math.sqrt(real ** 2 + imag ** 2)
            if mag > threshold:
                print(
                    f"Chan {chan} input peak at {i} of {real}{imag:+d}j (FFT {fft_index} of {num_ffts})"
                )
            real = np.int16(data_out[chan][i] & 0xFFFF)
            imag = np.int16((data_out[chan][i] >> 16) & 0xFFFF)
            mag = math.sqrt(real ** 2 + imag ** 2)
            if mag > threshold:
                print(
                    f"Chan {chan} output peak at {i} of {real}{imag:+d}j (FFT {fft_index} of {num_ffts})"
                )

    # Plot the input sent and output received for the selected FFT
    freq_bins = np.arange(0, fft_size)
    plt.figure("Input Sent and Output Received")
    for chan in range(num_chan):
        plt.subplot(2, num_chan, chan + 1)
        plt.plot(freq_bins, np.real(symbols_in[chan][start:end]), label="I")
        plt.plot(freq_bins, np.imag(symbols_in[chan][start:end]), label="Q")
        plt.xlabel("Frequency Bin")
        plt.ylabel("Amplitude")
        plt.title(f"Ch {chan} Input (FFT {fft_index} of {num_ffts})")
        plt.legend()
        plt.grid(True)
        plt.subplot(2, num_chan, chan + 1 + num_chan)
        plt.plot(freq_bins, np.real(symbols_out[chan][start:end]), label="I")
        plt.plot(freq_bins, np.imag(symbols_out[chan][start:end]), label="Q")
        plt.xlabel("Frequency Bin")
        plt.ylabel("Amplitude")
        plt.title(f"Ch {chan} Output (FFT {fft_index} of {num_ffts})")
        plt.legend()
        plt.grid(True)
    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    sys.exit(main())

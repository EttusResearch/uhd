#!/usr/bin/env python3
#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Spectrum display example using Python API and Matplotlib.

This example demonstrates how to use the UHD Python API to create a simple
FFT (Fast Fourier Transform) display using the matplotlib library. The script
configures the USRP device to receive number of samples configured by the user.
The example then computes the instantaneous estimate of the power spectral
density of the received signal and is displayed on the terminal window. The
display is updated once new set of samples are received. The user can
adjust parameters such as frequency, gain, and sample rate through command-line
arguments. This example is useful for visualizing the frequency spectrum of
signals received by the USRP device, allowing users to monitor and analyze the
signal characteristics in a user-friendly way.

Example Usage:
python rx_spectrum_to_pychart.py --args addr=192.168.10.2 --freq 2.4e9 --rate 1e6
                                 --gain 10 --nsamps 100000 --ref 0 --dyn 60
"""

import argparse
import time

try:
    import matplotlib.pyplot as plt
    import matplotlib.style as mplstyle
    from matplotlib.ticker import EngFormatter
except ImportError as e:
    print("Error: matplotlib is required to run this example.")
    print("Install it with: pip install matplotlib")
    raise e
import numpy as np
import uhd


def parse_args():
    """Parse the command line arguments."""
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=__doc__,
    )
    parser.add_argument(
        "-a",
        "--args",
        default="",
        type=str,
        help="""specifies the USRP device arguments, which holds
        multiple key-value pairs separated by commas
        (e.g., addr=192.168.40.2,type=x300) [default = ""].""",
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
        "-g", "--gain", type=int, default=10, help="specifies the gain in dB [default = 10]."
    )
    parser.add_argument(
        "-c",
        "--channel",
        type=int,
        default=0,
        help='specifies the channel to use (e.g., "0", "1", etc) [Default = 0].',
    )
    parser.add_argument(
        "-n",
        "--nsamps",
        type=int,
        default=100000,
        help="specifies the total number of samples to be received for FFT calculation "
        "and display on the screen before refreshing [default = 100000].",
    )
    parser.add_argument(
        "--dyn",
        type=int,
        default=60,
        help="specifies the dynamic range in dB. This defines the range of power levels "
        "relative to the reference level argument (--ref). Adjusting dynamic range "
        "influences the resolution displayed on y-axis of the plot [default = 60].",
    )
    parser.add_argument(
        "--ref",
        type=int,
        default=0,
        help="specifies the reference level in dB. This defines the maximum power "
        "level displayed on the plot. All power levels are relative to reference level. "
        "Adjusting the reference level allows to focus on specific power ranges in the "
        "signal [default = 0].",
    )
    return parser.parse_args()


def psd(nfft: int, samples: np.ndarray) -> np.ndarray:
    """Return the power spectral density of `samples`."""
    window = np.hamming(nfft)
    fft = np.fft.fft(samples * window)
    window_power = sum(window * window) / nfft
    logfft = (
        20 * np.log10(np.abs(np.fft.fftshift(fft)))
        - 10 * np.log10(window_power)
        - 20 * np.log10(nfft)
        + 3
    )
    return logfft


def main():
    """Create matplotlib display of power spectral density.

    ######## Receive Data and Display ########

    # 1. Setup USRP rate, frequency, and gain.
    # 2. Create a matplotlib figure and axis for plotting.
    # 2. Configure radio to stream fixed number of samples.
    # 3. Receive user configured samples from the USRP device.
    # 4. Plot Spectrum using FFT
    # 5. Repeat Steps 3-4 until user interrupts the program.
    """
    args = parse_args()
    usrp = uhd.usrp.MultiUSRP(args.args)

    # Set the USRP rate, freq, and gain
    usrp.set_rx_rate(args.rate, args.channel)
    usrp.set_rx_freq(uhd.types.TuneRequest(args.freq), args.channel)
    usrp.set_rx_gain(args.gain, args.channel)
    rx_rate = usrp.get_rx_rate()
    rx_freq = usrp.get_rx_freq(0)

    # Plotting initialization.
    mplstyle.use("fast")  # Use a fast style for matplotlib
    plt.ion()  # Stop matplotlib windows from blocking execution

    fig, axes = plt.subplots(
        nrows=1, ncols=1, figsize=(8, 4), gridspec_kw={"wspace": 0, "hspace": 0}
    )
    ax_signal_plot = axes
    formatter = EngFormatter()
    # Setup figure, axis and initiate plot
    xdata = []
    (ln_signal_plot,) = ax_signal_plot.plot(
        [],
        [],
        label="Power Spectral Density",
    )
    ax_signal_plot.title.set_text(
        f"Channel:{args.channel} operating at "
        f"{round(rx_rate/1e6, 2)} MSps tuned to "
        f"{round(rx_freq/1e9, 2)} GHz."
    )
    ax_signal_plot.set_ylim(args.ref - args.dyn, args.ref)
    ax_signal_plot.grid()
    ax_signal_plot.margins(x=0)
    ax_signal_plot.legend()
    ax_signal_plot.set_xlabel("Frequency (Hz)")
    ax_signal_plot.set_ylabel("Power Spectral Density (dB)")
    ax_signal_plot.xaxis.set_major_formatter(formatter)

    fig.tight_layout()
    height, width = plt.get_current_fig_manager().canvas.get_width_height()

    # Update every update_interval seconds.
    update_interval = 0.2

    # Create the buffer to receive samples
    num_samps = max(args.nsamps, width)
    print(f"Receiving {num_samps} samples per channel.")
    samples = np.empty((1, num_samps), dtype=np.complex64)

    st_args = uhd.usrp.StreamArgs("fc32", "sc16")
    st_args.channels = [args.channel]

    # Create Rx streamer to receive samples
    metadata = uhd.types.RXMetadata()
    streamer = usrp.get_rx_stream(st_args)
    buffer_samps = streamer.get_max_num_samps()
    print(f"Recv Buffer size set to: {buffer_samps} samples.")
    recv_buffer = np.zeros((1, buffer_samps), dtype=np.complex64)

    # Configure the stream command to stream number of samples and done
    stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.num_done)
    stream_cmd.stream_now = True
    stream_cmd.num_samps = buffer_samps

    print(
        "Beginning Streaming...\n"
        "Using matplotlib for display. Press Ctrl+C or close the plot to exit."
    )

    try:
        first_run = True
        while True:

            # Receive the samples
            recv_samps = 0
            while recv_samps < num_samps:
                streamer.issue_stream_cmd(stream_cmd)
                samps = streamer.recv(recv_buffer, metadata)
                if metadata.error_code != uhd.types.RXMetadataErrorCode.none:
                    print(metadata.strerror())
                if samps:
                    real_samps = min(num_samps - recv_samps, samps)
                    samples[:, recv_samps : recv_samps + real_samps] = recv_buffer[:, 0:real_samps]
                    recv_samps += real_samps

            # Get power spectral density
            len_samples = len(samples[args.channel])
            logfft = psd(len_samples, samples[args.channel])

            # Create xdata and ydata for the plot
            xdata = (
                np.arange(-len_samples / 2, len_samples / 2, 1) / len_samples * rx_rate + rx_freq
            )
            ydata = logfft

            # Reset the data in the plot
            ln_signal_plot.set_xdata(xdata)
            ln_signal_plot.set_ydata(ydata)

            # Rescale axes.
            if first_run:
                ax_signal_plot.relim()
                ax_signal_plot.autoscale_view()
                # Update the window
                fig.canvas.draw()
                first_run = False

            # Update the window
            fig.canvas.flush_events()

            # check if plot window has been closed by user
            if not plt.fignum_exists(fig.number):
                break

            time.sleep(update_interval)
    except KeyboardInterrupt:
        pass

    plt.close(fig)


if __name__ == "__main__":
    main()

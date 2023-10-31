#!/usr/bin/env python3
"""
This example requires
 -a USRP X440
 -UHD 4.6 or higher
 -matplotlib in Python installation
 -the X4_1600 bitfile installed

Demonstrates dual rates on X440 at the example of the L-band by recording data into the replay
blocks at two given master clock rates and two different center frequencies and displays the
captured data in a pyplot. Since the X440 is a direct sampling device, Nyquist zones and their
boundaries are important to keep in mind. The converter rate determines the Nyquist zone 
boundaries:
Nyquist boundaries = n * (converter rate / 2)

The master clock rate can be derived from the converter rate by dividing it by the decimation
factor 2, 4 or 8. It is equivalent to the bandwidth that can be captured on one channel.
The maximum bandwidth that can be achieved by the X440 is 2 GHz when using the maximum converter
rate of 4 GSps. Although from a bandwidth perspective this would be sufficient to cover the L-band,
this does not work because these 2 GHz are roughly between 0 and 2 GHz while the L-band is located
between 1 GHz and 2.4 GHz. To work around this, two different rates can be used in the X440. The
captured spectra of both can be stitched together. This example does this in a visual way only to
demonstrate the principle. In that way we are crossing Nyquist gaps in the two spectra, but
you should apply external filtering to avoid the aliasing images that will be visible otherwise.

Because the X4_1600 bitfile is used, only channel 0 of each radio is available and will be used.

By default this example will use master clock rates, converter rates and center frequencies to
cover the L-band. The parameters are:
 -Master clock rate 0: 1024 MHz (converter rate: 4.096 GHz)
 -Master clock rate 1: 1280 MHz (converter rate: 2.560 GHz)
 -Center frequency 0: 0.9 GHz
 -Center frequency 1: 2 GHz

With these settings, radio 0 will be able to capture the L-band between 0.39 GHz and 1.4 GHz in its
first Nyquist zone, and radio 1 will cover the range between 1.36 GHz and 2.64 GHz with its second
Nyquist zone.
"""

import time
import argparse
import sys
import uhd
from uhd import rfnoc
import numpy as np
import matplotlib.pyplot as plt

# sc16 (32-bit) samples on the USRP
BYTES_PER_SAMP = 4

# pylint: disable=too-many-arguments
def parse_args():
    """
    Return parsed command line args
    """
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--args", "-a", type=str, default="",
                        help="Device args to use when connecting to the USRP.")
    parser.add_argument("--mcrs", "-m", type=float, required=False, nargs=2,
                        help="Master clock rates for both channels, default "
                        "1024e6 1280e6",
                        default=[1024e6,1280e6])
    parser.add_argument("-f", "--freq", type=float, required=False, nargs=2,
                        help="Center frequencies for 2 channels, default 900e6 2e9",
                        default=[900e6, 2e9])
    parser.add_argument("--antenna", help="Antennas for both channels, default "
                        "RX1 RX1", nargs=2,
                        default=['RX1', 'RX1'])
    parser.add_argument("-d", "--duration", type=float,
                        help="Duration in seconds to capture. "
                             "Default is 0.0001 seconds.", default=0.0001)
    parser.add_argument("--delay", "-l", type=float, default=0.01,
                        help="Capture delay in seconds, default 0.01")
    parser.add_argument("--throttle", type=float, default=0.25,
                        help="Throttle for streaming to host, range (0, 1] (default: 0.25)")
    return parser.parse_args()


def main():
    """
    Main function of the dual rate example.
    """
    args = parse_args()

    dev_args = f"{args.args},product=x440,master_clock_rate=({args.mcrs[0]};{args.mcrs[1]})"

    graph = uhd.rfnoc.RfnocGraph(dev_args)

    radio0 = uhd.rfnoc.RadioControl(graph.get_block("0/Radio#0"))
    radio1 = uhd.rfnoc.RadioControl(graph.get_block("0/Radio#1"))

    replay0 = uhd.rfnoc.ReplayBlockControl(graph.get_block("0/Replay#0"))
    replay1 = uhd.rfnoc.ReplayBlockControl(graph.get_block("0/Replay#1"))

    radio0_frequency = args.freq[0]
    radio1_frequency = args.freq[1]

    radio0.set_rx_frequency(radio0_frequency, 0)
    radio0.set_rate(args.mcrs[0])
    radio1.set_rx_frequency(radio1_frequency, 0)
    radio1.set_rate(args.mcrs[1])

    graph.connect(radio0.get_unique_id(), 0, replay0.get_unique_id(), 0)
    graph.connect(radio1.get_unique_id(), 0, replay1.get_unique_id(), 0)

    throttle = args.throttle
    num_bytes0 = 0
    num_bytes1 = 0
    num_samps0 = 0
    num_samps1 = 0
    num_ports = 1

    cap_dtype=np.complex64

    stream_args = uhd.usrp.StreamArgs("fc32", "sc16")
    stream_args.args['throttle'] = str(throttle)

    # Set up streamer0
    rx_streamer0 = graph.create_rx_streamer(num_ports, stream_args)
    graph.connect(replay0.get_unique_id(), 0, rx_streamer0, 0)

    # Set up streamer1
    rx_streamer1 = graph.create_rx_streamer(num_ports, stream_args)
    graph.connect(replay1.get_unique_id(), 0, rx_streamer1, 0)

    graph.commit()

    capture_duration = args.duration
    cap_delay = args.delay
    update_interval = 0.2 # Update every update_interval seconds.


    # Plotting initialization.
    plt.ion() # Stop matplotlib windows from blocking
    plt.rcParams['toolbar'] = 'None'

    fig, axes = plt.subplots(nrows=1, ncols=2, figsize=(15, 6), gridspec_kw = {'wspace':0, 'hspace':0})
    ax_replay0 = axes[0]
    ax_replay1 = axes[1]

    # Setup figure, axis and initiate plot
    xdata_replay0, ydata_replay0 = [], []
    ln_replay0, = ax_replay0.plot([], [],)
    ax_replay0.title.set_text(f"Radio0 operating at {round(radio0.get_rate()/1e6, 2)} MSps tuned to {round(radio0.get_rx_frequency(0)/1e9, 2)} GHz.")
    ax_replay0.set_ylim(-160,20)
    ax_replay0.grid()
    ax_replay0.margins(x=0)

    xdata_replay1, ydata_replay1 = [], []
    ln_replay1, = ax_replay1.plot([], [],)
    ax_replay1.title.set_text(f"Radio1 operating at {round(radio1.get_rate()/1e6, 2)} MSps tuned to {round(radio1.get_rx_frequency(0)/1e9, 2)} GHz.")
    ax_replay1.set_ylim(-160,20)
    ax_replay1.grid()
    ax_replay1.margins(x=0)

    # Make it tight.
    ax_replay1.set_yticklabels([])
    fig.tight_layout()

    ######## Data Capture and Display ########

    # 1. Setup replay blocks for capture (replay0 and replay1)
    # 2. Configure radios to stream synchronously in future
    # 3. Fetch data from replay blocks to host
    # 4. Plot Spectrum using FFT
    # 5. Repeat after fixed intervals.
    # 6. Flush replay block.

    print("Beginning Streaming... Press Ctrl+C to exit")

    try:
        while True:
            time_now = graph.get_mb_controller().get_timekeeper(0).get_time_now()
            # Record from radio0 into DRAM using replay0
            num_samps0 = int(args.mcrs[0] * capture_duration)
            num_bytes0 = num_samps0 * BYTES_PER_SAMP
            mem_size = replay0.get_mem_size()
            mem_stride = mem_size // num_ports

            ## Arm replay0 block for recording
            for idx in range(num_ports):
                replay0.record(idx * mem_stride, num_bytes0, idx)
            ## Send stream command to radio0
            stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.num_done)
            stream_cmd.num_samps = num_samps0
            stream_cmd.stream_now = False
            stream_cmd.time_spec = \
                    time_now + \
                    uhd.types.TimeSpec(cap_delay)
            radio0.issue_stream_cmd(stream_cmd, 0)


            # Record from radio1 into DRAM using replay1
            num_samps1 = int(args.mcrs[1] * capture_duration)
            num_bytes1 = num_samps1 * BYTES_PER_SAMP
            mem_size = replay1.get_mem_size()
            mem_stride = mem_size // num_ports

            ## Arm replay1 block for recording
            for idx in range(num_ports):
                replay1.record(idx * mem_stride, num_bytes1, idx)
            ## Send stream command to radio1
            stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.num_done)
            stream_cmd.num_samps = num_samps1
            stream_cmd.stream_now = False
            stream_cmd.time_spec = \
                    time_now + \
                    uhd.types.TimeSpec(cap_delay)
            radio1.issue_stream_cmd(stream_cmd, 0)

            # Wait for record buffers to fill up
            timeout = time.monotonic() + num_samps0 / args.mcrs[0] + cap_delay + 10
            while any((replay0.get_record_fullness(port) < num_bytes0
                        for port in range(num_ports))):
                time.sleep(0.100)
                if time.monotonic() > timeout:
                    raise RuntimeError("Timeout while loading replay0 buffer!")

            ## Wait for record buffers to fill up
            timeout = time.monotonic() + num_samps1 / args.mcrs[1] + cap_delay + 10
            while any((replay1.get_record_fullness(port) < num_bytes1
                        for port in range(num_ports))):
                time.sleep(0.100)
                if time.monotonic() > timeout:
                    raise RuntimeError("Timeout while loading replay1 buffer!")

            output_buf_replay0 = np.zeros(num_samps0, dtype=cap_dtype)

            rx_md = uhd.types.RXMetadata()
            num_bytes = num_samps0 * BYTES_PER_SAMP
            pkt_size_bytes = replay0.get_max_packet_size(0)
            max_samps_per_pkt = pkt_size_bytes // BYTES_PER_SAMP
            mem_stride = replay0.get_mem_size() // num_ports
            # Configure playback regions
            for idx in range(num_ports):
                replay0.config_play(idx * mem_stride, num_bytes, idx)
            stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.num_done)
            stream_cmd.num_samps = num_samps0
            # This is not strictly necessary, but the streamer will not allow a
            # multi-chan operation without a time spec.
            stream_cmd.stream_now = False
            stream_cmd.time_spec = uhd.types.TimeSpec(0.0)
            rx_streamer0.issue_stream_cmd(stream_cmd)


            num_rx = rx_streamer0.recv(output_buf_replay0, rx_md, 5.0)
            if rx_md.error_code != uhd.types.RXMetadataErrorCode.none:
                print("Error during download: " + rx_md.strerror())
            if num_rx != num_samps0:
                print("ERROR: Fewer samples received than expected!")

            # Get the new data
            N = len(output_buf_replay0)
            window = np.hamming(N)
            fft = np.fft.fft(output_buf_replay0 * window)
            window_power = sum(window * window) / N
            logfft = 20*np.log10(np.abs(np.fft.fftshift(fft))) - 10*np.log10(window_power) - 20*np.log10(N) + 3
            xdata_replay0 = np.arange(-N/2,N/2,1) / N * args.mcrs[0] + radio0.get_rx_frequency(0)
            ydata_replay0 = logfft


            output_buf_replay1 = np.zeros(num_samps1, dtype=cap_dtype)

            #print("Downloading data to host from Replay1...")
            rx_md = uhd.types.RXMetadata()
            num_bytes = num_samps1 * BYTES_PER_SAMP
            pkt_size_bytes = replay1.get_max_packet_size(0)
            max_samps_per_pkt = pkt_size_bytes // BYTES_PER_SAMP
            mem_stride = replay1.get_mem_size() // num_ports
            # Configure playback regions
            for idx in range(num_ports):
                replay1.config_play(idx * mem_stride, num_bytes, idx)
            stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.num_done)
            stream_cmd.num_samps = num_samps1
            # This is not strictly necessary, but the streamer will not allow a
            # multi-chan operation without a time spec.
            stream_cmd.stream_now = False
            stream_cmd.time_spec = uhd.types.TimeSpec(0.0)
            rx_streamer1.issue_stream_cmd(stream_cmd)


            num_rx = rx_streamer1.recv(output_buf_replay1, rx_md, 5.0)
            if rx_md.error_code != uhd.types.RXMetadataErrorCode.none:
                print("Error during download: " + rx_md.strerror())
            if num_rx != num_samps1:
                print("ERROR: Fewer samples received than expected!")

            # Get the new data
            N = len(output_buf_replay1)
            window = np.hamming(N)
            fft = np.fft.fft(output_buf_replay1 * window)
            window_power = sum(window * window) / N
            logfft = 20*np.log10(np.abs(np.fft.fftshift(fft))) - 10*np.log10(window_power) - 20*np.log10(N) + 3
            xdata_replay1 = np.arange(-N/2,N/2,1) / N * args.mcrs[1] + radio1.get_rx_frequency(0)
            ydata_replay1 = logfft

            # Reset the data in the plot
            ln_replay0.set_xdata(xdata_replay0)
            ln_replay0.set_ydata(ydata_replay0)
            ln_replay1.set_xdata(xdata_replay1)
            ln_replay1.set_ydata(ydata_replay1)

            # Rescale axis.
            ax_replay0.relim()
            ax_replay0.autoscale_view()
            ax_replay1.relim()
            ax_replay1.autoscale_view()

            # Update the window
            fig.canvas.draw()
            fig.canvas.flush_events()


            time.sleep(update_interval)

    except KeyboardInterrupt:
        print("--> Caught CTRL+C, exiting...")
        plt.close(fig)
        return None


if __name__ == "__main__":
    sys.exit(not main())

\page page_gsg_examples Example Programs

# UHD Example Programs

UHD (USRP Hardware Driver) ships with a comprehensive set of example programs in both C++ and Python. These examples are designed to help users understand how to interact with USRP devices, configure hardware, stream data, and implement common signal processing tasks using the UHD Multi-USRP and RFNoC APIs.

In addition to the UHD examples, it may also be helpful to explore the GNU Radio example programs that demonstrate how to use UHD with USRP devices. These can be found in the [gr-uhd directory of the GNU Radio repository](https://github.com/gnuradio/gnuradio/tree/main/gr-uhd).

## What are UHD Examples For?

The UHD examples serve several important purposes:
- **Getting Started:** They provide working code for basic device setup, streaming, and file I/O, making it easy for new users to get started with USRP hardware.
- **Reference Implementations:** The examples demonstrate recommended usage patterns for the UHD API, including device configuration, error handling, and timing control.
- **Testing and Validation:** Some examples can be used to verify hardware operation, test connectivity, and validate system performance.
- **Advanced Features:** Some examples showcase advanced UHD features such as multi-channel streaming, timed commands, RFNoC block usage, and synchronization across multiple devices.

You can use these examples as templates for your own applications, as diagnostic tools, or as a starting point for learning UHD programming.

## Location of Example Programs

The UHD example programs are installed in the `examples` directory within the UHD installation.

- **Linux:**
  - The default installation path is `/usr/lib/uhd/examples` (or `/usr/libexec/uhd/examples`).
- **Windows:**
  - The default installation path is `C:\Program Files\UHD\lib\uhd\examples`.

The `examples` directory contains:
- **Precompiled C++ example programs** (for sources see [https://github.com/EttusResearch/uhd/tree/master/host/examples](https://github.com/EttusResearch/uhd/tree/master/host/examples))
- **Python example programs** (located in the `python` subfolder)

## Running the Examples:
Most UHD examples can be run directly from the command line. Use the `--help` option with each example to see available command-line arguments and usage instructions. Most examples require device arguments (e.g., `--args "addr=192.168.10.2"` or `--args "type=b200,name=123456"`).

## Modifying and Compiling C++ Example Programs

Most C++ examples are built and installed automatically as part of the UHD build process.  
  For instructions on building UHD from source, see the [UHD Build Guide](\ref page_build_guide).
- The C++ example source files are located in the UHD source tree at:  
  `host/examples/`
- If you wish to develop your own C++ application and link it against UHD (either dynamically or statically), refer to the `init_usrp.cpp` example in the UHD source tree.  
  For a step-by-step guide, see [Getting Started with UHD and C++](https://kb.ettus.com/Getting_Started_with_UHD_and_C%2B%2B).

##Overview of UHD Example Programs

The tables below list the UHD C++ and Python example programs that are shipped with the UHD driver. These examples are provided to help users learn how to interact with USRP devices, explore UHD features, and perform common or advanced tasks.

**API Compatibility and Device Requirements:**  
The "UHD API" column indicates whether the example uses the high-level Multi-USRP API (compatible with most USRP devices) or the RFNoC API (requires RFNoC-capable USRP devices).

**Glossary for Table Columns:**  
- *Multiple Channel Support*: Indicates if the example supports multiple USRP channels from one ore more USRP devices simultaneously.  
- *TX/RX*: Shows whether the example transmits (TX), receives (RX), or does both (TX/RX).  
- *UHD API*: Specifies the UHD API used (multi_usrp or rfnoc).  
- *Description*: Brief summary of the example's purpose.

For clarity, the examples are classified into three categories, although the boundaries between them are not strict (The classification of examples into Application, Feature, and Utility is for convenience; some examples may demonstrate multiple aspects of UHD usage.):

- **Application Examples:**
  Ready-to-use programs for transmitting or receiving data with USRP device(s). These examples typically support a wide range of command-line options, allowing users to adapt the behavior to various hardware configurations, signal types, and operational scenarios.

- **Feature Examples:**
  Programs designed to specifically demonstrate the use of a particular UHD feature or API function. These are ideal for learning how to implement or integrate specific capabilities, such as timed commands, streaming modes, or device synchronization.

- **Utility Examples:**
  Tools for characterizing USRP performance, diagnosing hardware or software issues, or performing measurements such as latency, throughput, or clock stability.

Some examples use the **Multi-USRP API**, which provides a high-level, device-agnostic interface for configuring, controlling, and streaming data to and from USRP devices. The Multi-USRP API makes it easy to set up devices, manage channels, and perform standard SDR operations with minimal code. If an example listed in the tables below uses the 'Multi-USRP API' and is marked as having 'Multiple Channel Support', then it inherently supports operation with multiple USRP devices.

Other examples use the **RFNoC API**, which allows for direct interaction with RFNoC (RF Network-on-Chip) blocks on supported USRP devices. The RFNoC API enables advanced users to leverage FPGA-based signal processing, custom block development, and flexible data routing for high-performance and specialized applications.

Refer to the tables below for a summary of the available UHD C++ and Python examples, their classification, and a brief description of their purpose.

<table>
<caption>C++ Examples</caption>
  <tr>
    <th>Example Filename</th>
    <th>Multiple Channel Support</th>
    <th>TX/RX</th>
    <th>UHD API</th>
    <th>Description</th>
</tr>
  <tr style="background-color:#f5f5f5;">
    <td colspan="5" align="center"><b>Application Examples</b></td>
  </tr>
  <tr>
    <td>[rx_ascii_art_dft[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//rx_ascii_art_dft.cpp)</td>
    <td>No</td>
    <td>RX</td>
    <td>Multi-USRP</td>
    <td>Continuously receives complex baseband samples (IQ) from a USRP device and displays an ASCII-art DFT spectrum in the terminal.</td>
  </tr>
  <tr>
    <td>[rx_samples_to_file[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//rx_samples_to_file.cpp)</td>
    <td>Yes</td>
    <td>RX</td>
    <td>Multi-USRP</td>
    <td>Receive complex baseband samples (IQ) from USRP device(s) and save them to binary file(s).</td>
  </tr>
  <tr>
    <td>[rx_samples_to_udp[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//rx_samples_to_udp.cpp)</td>
    <td>No</td>
    <td>RX</td>
    <td>Multi-USRP</td>
    <td>Stream received complex baseband samples (IQ) from USRP device(s) and forward them to a UDP socket.</td>
  </tr>
  <tr>
    <td>[twinrx_freq_hopping[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//twinrx_freq_hopping.cpp)</td>
    <td>No</td>
    <td>RX</td>
    <td>Multi-USRP</td>
    <td>Demonstrates how to use the TwinRX daughterboard to receive complex baseband samples (IQ) with frequency hopping on USRP devices.</td>
  </tr>
  <tr>
    <td>[tx_samples_from_file[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//tx_samples_from_file.cpp)</td>
    <td>Yes</td>
    <td>TX</td>
    <td>Multi-USRP</td>
    <td>Transmit complex baseband samples (IQ) from a binary file via USRP device(s).</td>
  </tr>
  <tr>
    <td>[tx_waveforms[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//tx_waveforms.cpp)</td>
    <td>Yes</td>
    <td>TX</td>
    <td>Multi-USRP</td>
    <td>Transmit complex baseband samples (IQ) of generated waveforms (sine, square, ramp, constant) via USRP device(s).</td>
  </tr>
  <tr>
    <td>[txrx_loopback_to_file[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//txrx_loopback_to_file.cpp)</td>
    <td>Yes</td>
    <td>TX/RX</td>
    <td>Multi-USRP</td>
    <td>Transmit and receive complex baseband samples (IQ) in TX/RX loopback mode using one or two USRP devices, with support for waveform generation and multi-channel file recording.</td>
  </tr>
  <tr>
    <td>[rfnoc_radio_loopback[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//rfnoc_radio_loopback.cpp)</td>
    <td>No</td>
    <td>n/a</td>
    <td>RFNoC</td>
    <td>Configures an RFNoC graph to stream complex baseband samples (IQ) directly from RX to TX.</td>
  </tr>
  <tr>
    <td>[rfnoc_replay_samples_from_file[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//rfnoc_replay_samples_from_file.cpp)</td>
    <td>No</td>
    <td>TX</td>
    <td>RFNoC</td>
    <td>Demonstrates replaying complex baseband samples (IQ) from a file to a USRP radio using the RFNoC Replay block.</td>
  </tr>
  <tr>
    <td>[rfnoc_rx_to_file[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//rfnoc_rx_to_file.cpp)</td>
    <td>No</td>
    <td>RX</td>
    <td>RFNoC</td>
    <td>Configures an RFNoC graph to stream complex baseband samples (IQ) from Radio to host and saves received data to file. Allows to insert a user-defined RFNoC block.</td>
  </tr>
  <tr style="background-color:#f5f5f5;">
    <td colspan="5" align="center"><b>Feature Examples</b></td>
  </tr>
  <tr>
    <td>[gpio[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//gpio.cpp)</td>
    <td>Yes</td>
    <td>TX/RX</td>
    <td>Multi-USRP</td>
    <td>Demonstrates and tests USRP GPIO configuration, bit-banging, and ATR control.</td>
  </tr>
  <tr>
    <td>[rx_multi_samples[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//rx_multi_samples.cpp)</td>
    <td>Yes</td>
    <td>RX</td>
    <td>Multi-USRP</td>
    <td>Demonstrates time aligned reception of complex baseband samples (IQ) from multiple channels/USRP devices.</td>
  </tr>
  <tr>
    <td>[rx_timed_samples[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//rx_timed_samples.cpp)</td>
    <td>Yes</td>
    <td>RX</td>
    <td>Multi-USRP</td>
    <td>Demonstrates scheduling RX at a specific USRP time, stopping after N samples, and checking for errors.</td>
  </tr>
  <tr>
    <td>[spi[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//spi.cpp)</td>
    <td>No</td>
    <td>n/a</td>
    <td>Multi-USRP</td>
    <td>Demonstrates basic SPI communication via the USRP GPIO interface of X4xx USRP devices using the UHD API.</td>
  </tr>
  <tr>
    <td>[sync_to_gps[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//sync_to_gps.cpp)</td>
    <td>No</td>
    <td>n/a</td>
    <td>Multi-USRP</td>
    <td>Demonstrates configuring USRP to use a built-in GPSDO for clock and PPS reference.</td>
  </tr>
  <tr>
    <td>[test_messages[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//test_messages.cpp)</td>
    <td>No</td>
    <td>TX/RX</td>
    <td>Multi-USRP</td>
    <td>Tests USRP async error and event message handling (e.g., late command, burst ACK).</td>
  </tr>
  <tr>
    <td>[test_timed_commands[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//test_timed_commands.cpp)</td>
    <td>No</td>
    <td>RX</td>
    <td>Multi-USRP</td>
    <td>Demonstrates and tests timed USRP commands and timed RX streaming with UHD.</td>
  </tr>
  <tr>
    <td>[tx_bursts[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//tx_bursts.cpp)</td>
    <td>Yes</td>
    <td>TX</td>
    <td>Multi-USRP</td>
    <td>Transmit complex baseband samples (IQ) in timed bursts with user-defined timing and burst length via USRP.</td>
  </tr>
  <tr>
    <td>[tx_timed_samples[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//tx_timed_samples.cpp)</td>
    <td>No</td>
    <td>TX</td>
    <td>Multi-USRP</td>
    <td>Demonstrates scheduling TX at a specific USRP time, stopping after N samples, and checking for ACK.</td>
  </tr>
  <tr>
    <td>[txrx_complex_gain[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//txrx_complex_gain.cpp)</td>
    <td>Yes</td>
    <td>TX/RX</td>
    <td>Multi-USRP</td>
    <td>Demonstrates how to enable and use timed complex gain adjustment for TX and RX.</td>
  </tr>
  <tr>
    <td>[rfnoc_nullsource_ce_rx[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//rfnoc_nullsource_ce_rx.cpp)</td>
    <td>No</td>
    <td>n/a</td>
    <td>RFNoC</td>
    <td>Demonstrates use of the RFNoC Null Source/Sink block to generate and stream test data to the host.</td>
  </tr>
  <tr style="background-color:#f5f5f5;">
    <td colspan="5" align="center"><b>Utilities</b></td>
  </tr>
  <tr>
    <td>[benchmark_rate[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//benchmark_rate.cpp)</td>
    <td>Yes</td>
    <td>TX/RX</td>
    <td>Multi-USRP</td>
    <td>Benchmark streaming performance for complex baseband samples (IQ) on transmit and/or receive channels at a user-specified rate.</td>
  </tr>
  <tr>
    <td>[latency_test[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//latency_test.cpp)</td>
    <td>No</td>
    <td>TX/RX</td>
    <td>Multi-USRP</td>
    <td>Measures and verifies whether the system can reliably achieve a user-specified round-trip latency for transmitting and receiving complex baseband samples (IQ) with USRP devices.</td>
  </tr>
  <tr>
    <td>[test_clock_synch[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//test_clock_synch.cpp)</td>
    <td>No</td>
    <td>n/a</td>
    <td>Multi-USRP</td>
    <td>Tests synchronizing time across multiple USRPs using a clock device (e.g., OctoClock).</td>
  </tr>
  <tr>
    <td>[test_dboard_coercion[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//test_dboard_coercion.cpp)</td>
    <td>No</td>
    <td>n/a</td>
    <td>Multi-USRP</td>
    <td>Sweeps daughterboard frequency/gain ranges to verify tuning across all supported values.</td>
  </tr>
  <tr>
    <td>[test_pps_input[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//test_pps_input.cpp)</td>
    <td>No</td>
    <td>n/a</td>
    <td>Multi-USRP</td>
    <td>Tests if the USRP's PPS (pulse-per-second) input signal is detected and working.</td>
  </tr>
  <tr>
    <td>[usrp_list_sensors[.cpp]](https://github.com/EttusResearch/uhd/tree/master/host/examples//usrp_list_sensors.cpp)</td>
    <td>No</td>
    <td>n/a</td>
    <td>Multi-USRP</td>
    <td>Lists all available USRP device, motherboard, and channel sensors and prints their values.</td>
  </tr>
</table>

<br/>

<table>
<caption>Python Examples</caption>
  <tr>
    <th>Example Filename</th>
    <th>Multiple Channel Support</th>
    <th>TX/RX</th>
    <th>UHD API</th>
    <th>Description</th>
</tr>
  <tr style="background-color:#f5f5f5;">
    <td colspan="5" align="center"><b>Application Examples</b></td>
  </tr>
  <tr>
    <td>[rx_spectrum_to_asciiplot.py](https://github.com/EttusResearch/uhd/tree/master/host/examples//python/rx_spectrum_to_asciiplot.py)</td>
    <td>No</td>
    <td>RX</td>
    <td>Multi-USRP</td>
    <td>Continuously receives complex baseband samples (IQ) from a USRP device and displays an ASCII-art DFT spectrum in the terminal (using curses).</td>
  </tr>
  <tr>
    <td>[rx_spectrum_to_pyplot.py](https://github.com/EttusResearch/uhd/tree/master/host/examples//python/rx_spectrum_to_pyplot.py)</td>
    <td>No</td>
    <td>RX</td>
    <td>Multi-USRP</td>
    <td>Continuously receives complex baseband samples (IQ) from a USRP device and displays an ASCII-art DFT spectrum (using matplotlib).</td>
  </tr>
  <tr>
    <td>[rx_to_file.py](https://github.com/EttusResearch/uhd/tree/master/host/examples//python/rx_to_file.py)</td>
    <td>Yes</td>
    <td>RX</td>
    <td>Multi-USRP
RFNoC</td>
    <td>Receive complex baseband samples (IQ) from USRP device(s) and save them to binary file(s). Supports RX streaming from host using Multi-USRP API as well as recording using RFNoC Replay block using RFNoC API.</td>
  </tr>
  <tr>
    <td>[rx_to_remote_udp.py](https://github.com/EttusResearch/uhd/tree/master/host/examples//python/rx_to_remote_udp.py)</td>
    <td>Yes</td>
    <td>RX</td>
    <td>Multi-USRP</td>
    <td>Stream received complex baseband samples (IQ) from USRP channels/device(s) and forward them to remote UDP socket(s).</td>
  </tr>
  <tr>
    <td>[tx_waveforms.py](https://github.com/EttusResearch/uhd/tree/master/host/examples//python/tx_waveforms.py)</td>
    <td>Yes</td>
    <td>TX</td>
    <td>Multi-USRP
RFNoC</td>
    <td>Transmit complex baseband samples (IQ) of generated waveforms (sine, square, ramp, constant) via USRP device(s). Supports TX streaming from host using Multi-USRP API as well as using RFNoC Replay block using RFNoC API.</td>
  </tr>
  <tr>
    <td>[rfnoc_rx_replay_samples_to_file.py](https://github.com/EttusResearch/uhd/tree/master/host/examples//python/rfnoc_rx_replay_samples_to_file.py)</td>
    <td>Yes</td>
    <td>RX</td>
    <td>RFNoC</td>
    <td>Capture complex baseband samples (IQ) from one or more channels of a USRP device into a Replay block, then stream them to host and write them to binary file(s).</td>
  </tr>
  <tr>
    <td>[rfnoc_rx_to_file.py](https://github.com/EttusResearch/uhd/tree/master/host/examples//python/rfnoc_rx_to_file.py)</td>
    <td>Yes</td>
    <td>RX</td>
    <td>RFNoC</td>
    <td>Receive complex baseband samples (IQ) from USRP device(s) and write them to binary file(s).</td>
  </tr>
  <tr>
    <td>[rfnoc_txrx_fft_block_loopback.py](https://github.com/EttusResearch/uhd/tree/master/host/examples//python/rfnoc_txrx_fft_block_loopback.py)</td>
    <td>No</td>
    <td>TX/RX</td>
    <td>RFNoC</td>
    <td>Demonstrates how to send and receive FFT frames using the RFNoC FFT block.</td>
  </tr>
  <tr>
    <td>[x440_L_band_capture.py](https://github.com/EttusResearch/uhd/tree/master/host/examples//python/x440_L_band_capture.py)</td>
    <td>No</td>
    <td>RX</td>
    <td>RFNoC</td>
    <td>Demonstrates simultaneous capture of complex baseband samples (IQ) at different rates, frequencies and master clock rates to display a wideband PSD for the L-Band.</td>
  </tr>
  <tr style="background-color:#f5f5f5;">
    <td colspan="5" align="center"><b>Feature Examples</b></td>
  </tr>
  <tr>
    <td>[usrp_power_meter.py](https://github.com/EttusResearch/uhd/tree/master/host/examples//python/usrp_power_meter.py)</td>
    <td>No</td>
    <td>n/a</td>
    <td>Multi-USRP</td>
    <td>Measure the received signal power in dBm at a specified center frequency using a power calibrated USRP device.</td>
  </tr>
  <tr style="background-color:#f5f5f5;">
    <td colspan="5" align="center"><b>Utilities</b></td>
  </tr>
  <tr>
    <td>[benchmark_rate.py](https://github.com/EttusResearch/uhd/tree/master/host/examples//python/benchmark_rate.py)</td>
    <td>Yes</td>
    <td>TX/RX</td>
    <td>Multi-USRP</td>
    <td>Benchmark streaming performance for complex baseband samples (IQ) on transmit and/or receive channels at a user-specified rate.</td>
  </tr>
</table>

<br/>

<table>
<caption>C Examples</caption>
  <tr>
    <th>Example Filename</th>
    <th>Multiple Channel Support</th>
    <th>TX/RX</th>
    <th>Description</th>
</tr>
  <tr>
    <td>[rx_samples_c[.c]](https://github.com/EttusResearch/uhd/tree/master/host/examples//rx_samples_c.c)</td>
    <td>No</td>
    <td>RX</td>
    <td>Receive complex baseband samples (IQ) from a USRP device and save them to a binary file.</td>
  </tr>
  <tr>
    <td>[tx_samples_c[.c]](https://github.com/EttusResearch/uhd/tree/master/host/examples//tx_samples_c.c)</td>
    <td>No</td>
    <td>TX</td>
    <td>Transmit complex baseband samples (IQ) for transmission via a USRP device.</td>
  </tr>
</table>
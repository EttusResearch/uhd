#!/usr/bin/env python3
#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
"""Test the TX complex gain and RX complex gain features of the Radio block.

This script requires the Python module 'pytest' to be installed.

Execute as follows:

./test_radio_control_complex_gain.py --args <UHD address arguments>
"""

import random
import sys

import numpy as np
import pytest
import uhd.rfnoc

SPP = 128
TOLERANCE = 1e-4


@pytest.fixture(scope="session")
def radio(graph):
    """Get the Radio controller."""
    return uhd.rfnoc.RadioControl(graph.get_block("Radio"))


@pytest.fixture(scope="session")
def ddc(graph):
    """Get the DDC block controller, in case it is available."""
    try:
        return uhd.rfnoc.DdcBlockControl(graph.get_block("DDC"))
    except RuntimeError:
        return None


@pytest.fixture(scope="session")
def duc(graph):
    """Get the DUC block controller, in case it is available."""
    try:
        return uhd.rfnoc.DucBlockControl(graph.get_block("DUC"))
    except RuntimeError:
        return None


@pytest.fixture(scope="session")
def rx_streamer(graph, radio, ddc):
    """Create a RX streamer and connect it to the radio."""
    num_chan = radio.get_port_num()
    sa = uhd.usrp.StreamArgs("sc16", "sc16")
    sa.args = f"spp={SPP}"
    rx_streamer = graph.create_rx_streamer(num_chan, sa)
    for chan_idx in range(num_chan):
        if ddc is not None:
            graph.connect(
                radio.get_unique_id(),
                chan_idx,
                ddc.get_unique_id(),
                chan_idx,
            )
            graph.connect(
                ddc.get_unique_id(),
                chan_idx,
                rx_streamer,
                chan_idx,
            )
        else:
            graph.connect(
                radio.get_unique_id(),
                chan_idx,
                rx_streamer,
                chan_idx,
            )
    return rx_streamer


@pytest.fixture(scope="session")
def tx_streamer(graph, radio, duc):
    """Create a TX streamer and connect it to the radio."""
    num_chan = radio.get_port_num()
    sa = uhd.usrp.StreamArgs("sc16", "sc16")
    sa.args = f"spp={SPP}"
    tx_streamer = graph.create_tx_streamer(num_chan, sa)
    for chan_idx in range(num_chan):
        if duc is not None:
            graph.connect(
                tx_streamer,
                chan_idx,
                duc.get_unique_id(),
                chan_idx,
            )
            graph.connect(
                duc.get_unique_id(),
                chan_idx,
                radio.get_unique_id(),
                chan_idx,
            )
        else:
            graph.connect(
                tx_streamer,
                chan_idx,
                radio.get_unique_id(),
                chan_idx,
            )
    return tx_streamer


def configure_radio(radio, digital_loopback=False):
    """Configure the radio."""
    for chan_idx in range(radio.get_port_num()):
        # Set the radio's RX packet size
        radio.set_properties(f"spp={SPP}", chan_idx)
        # TODO: add a function to the radio block controller for enabling digital loopback
        radio.poke32(0x1000 + 1024 * chan_idx, int(digital_loopback))


def complex128_to_sc16(data, scale=32767):
    """Convert complex128 NumPy to sc16 stores as uint32."""
    real = np.int16(np.real(data) * scale)
    imag = np.int16(np.imag(data) * scale)
    return np.uint32((np.uint32(imag) << 16) | (np.uint32(real) & 0xFFFF))


def sc16_to_complex128(data):
    """Convert sc16 stored as uint32 to complex128 NumPy."""
    real = np.array(np.int16(data & 0xFFFF), dtype=np.float64) / 32767
    imag = np.array(np.int16((data >> 16) & 0xFFFF), dtype=np.float64) / 32767
    return np.array(real + 1j * imag, dtype=np.complex128)


def create_symbols(channels, num_samples, amplitude):
    """Create symbols with a contant value.

    The dimension is as follows:
    first dimenstion: num
    """
    num_chan = len(channels)
    symbols = np.ones((num_chan, num_samples), dtype=np.complex128) * amplitude
    return symbols


def transmit_and_receive(
    radio, tx_streamer, rx_streamer, data_in, data_out_size, loopback_delay, start_time
):
    """Transmit and receive simultaniuously.

    The start time is set to 'time_advance' seconds (default: 0.1) in the future.
    """
    # Check that the input data has data for all channels
    num_chan = radio.get_port_num()
    assert num_chan == data_in.shape[0]
    assert num_chan == data_out_size[0]

    # Start RX streaming
    stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.num_done)
    stream_cmd.stream_now = False
    stream_cmd.num_samps = data_in.shape[1]
    stream_cmd.time_spec = start_time + loopback_delay
    rx_streamer.issue_stream_cmd(stream_cmd)

    # Transmit the input data
    tx_md = uhd.types.TXMetadata()
    tx_md.has_time_spec = True
    tx_md.time_spec = start_time
    tx_md.start_of_burst = True
    tx_md.end_of_burst = True
    num_tx = tx_streamer.send(data_in, tx_md, timeout=5.0)

    # Check that all input data was transmitted
    if num_tx < data_in.shape[1]:
        raise RuntimeError(
            f"ERROR: number of transmitted samples is too low (transmitted {num_tx} samples per channel; input data has {data_in.shape[1]} samples per channel)"
        )

    # Receive the output data
    rx_md = uhd.types.RXMetadata()
    data_out = np.zeros(data_out_size, dtype=np.uint32)
    num_rx = rx_streamer.recv(data_out, rx_md, 5.0)

    # Check that enough data was received
    if num_rx < data_out_size[1]:
        raise RuntimeError(
            f"ERROR: number of received samples is too low (received {num_rx} but expected {data_out_size[1]})"
        )

    return data_out


def process_data(
    radio,
    rx_streamer,
    tx_streamer,
    samples_in,
    samples_reference,
    loopback_delay=uhd.types.TimeSpec(0.0),
    stream_start=uhd.types.TimeSpec(0.1),
):
    """Stream the input data and compare the received data with the reference.

    For streaming, the complex floating point numbers are converted to signed
    complex fixed-point (I16/I16).
    """
    # Convert RX data from complex floating point to I16/I16
    data_in = complex128_to_sc16(samples_in)

    # Transmit and receive the data
    data_out = transmit_and_receive(
        radio,
        tx_streamer,
        rx_streamer,
        data_in=data_in,
        data_out_size=samples_reference.shape,
        loopback_delay=loopback_delay,
        start_time=stream_start,
    )

    # Convert RX data from I16/I16 to complex floating point
    samples_out = sc16_to_complex128(data_out)

    # Check that the data is not all-zero
    assert samples_out.any(), "array contains only zeros"

    # Check that the received data matches the reference data
    for chan in range(samples_reference.shape[0]):
        np.testing.assert_allclose(
            samples_out[chan], samples_reference[chan], rtol=TOLERANCE, atol=TOLERANCE
        )


def random_numbers(min, max, num, seed=0):
    """Generate random numbers in a given range."""
    random.seed(seed)
    return [random.randint(min, max) for _ in range(num)]


@pytest.mark.parametrize(
    "rx_coeff", [1 + 0j, 1j, 1.9999 + 0j, -1.9999 + 0j, 0.0001 + 0j, 1.2 + -0.3j]
)
@pytest.mark.parametrize(
    "tx_coeff", [1 + 0j, 1j, 1.9999 + 0j, -1.9999 + 0j, 0.0001 + 0j, 1.2 + -0.3j]
)
def test_complex_gain(radio, rx_streamer, tx_streamer, graph, tx_coeff, rx_coeff):
    """Test the RX and TX complex gain feature concurrently.

    Use different RT gain (rx_coeff) and TX gain (rx_coeff) values and each
    possible combination.
    """
    if abs(tx_coeff * rx_coeff) > (2 - TOLERANCE):
        pytest.skip("coefficients combined are above max. threshold")
    if abs(tx_coeff * rx_coeff) < (TOLERANCE):
        pytest.skip("coefficients combined are below min. threshold")
    print(f"TX gain: {tx_coeff}, RX gain: {rx_coeff}")
    num_chan = radio.get_port_num()
    graph.commit()

    configure_radio(radio, digital_loopback=True)

    samples_in = create_symbols(range(num_chan), num_samples=SPP, amplitude=0.5)
    tx_cgain = radio.get_tx_complex_gain()
    rx_cgain = radio.get_rx_complex_gain()

    # Calculate when to start streaming
    stream_start = radio.get_time_now() + 0.1
    # Calculate the delay to start rx streaming when using digital loopback
    loopback_delay = uhd.types.TimeSpec.from_ticks(radio.get_spc() * 19, radio.get_tick_rate())

    for chan in range(num_chan):
        tx_cgain.set_gain_coeff(tx_coeff, chan)
        rx_cgain.set_gain_coeff(rx_coeff, chan)
        samples_reference = np.copy(samples_in) * rx_coeff * tx_coeff

    process_data(
        radio, rx_streamer, tx_streamer, samples_in, samples_reference, loopback_delay, stream_start
    )


@pytest.mark.parametrize("rx_coeff", [1 + 0.5j, 1.2 + -0.3j])
@pytest.mark.parametrize("tx_coeff", [1 + 0.5j, 1.2 + -0.3j])
@pytest.mark.parametrize("rx_cgain_time", random_numbers(0, SPP, 3, seed=2))
@pytest.mark.parametrize("tx_cgain_time", random_numbers(0, SPP, 3, seed=3))
def test_timed_complex_gain(
    radio, rx_streamer, tx_streamer, graph, tx_coeff, rx_coeff, tx_cgain_time, rx_cgain_time
):
    """Test the RX and TX complex gain feature concurrently.

    Use different RT gain (rx_coeff) and TX gain (rx_coeff) values and each
    possible combination.
    """
    if abs(tx_coeff * rx_coeff) > (2 - TOLERANCE):
        pytest.skip("coefficients combined are above max. threshold")
    if abs(tx_coeff * rx_coeff) < (TOLERANCE):
        pytest.skip("coefficients combined are below min. threshold")
    print(f"TX gain: {tx_coeff}, RX gain: {rx_coeff}")
    num_chan = radio.get_port_num()
    graph.commit()

    configure_radio(radio, digital_loopback=True)

    samples_in = create_symbols(range(num_chan), num_samples=SPP, amplitude=0.5)
    samples_reference = np.copy(samples_in)

    # Get complex gain objects
    tx_cgain = radio.get_tx_complex_gain()
    rx_cgain = radio.get_rx_complex_gain()

    # Calculate when to start streaming
    stream_start = radio.get_time_now() + 0.1
    # Calculate the rx streaming latency when using digital loopback
    loopback_delay = uhd.types.TimeSpec.from_ticks(radio.get_spc() * 19, radio.get_tick_rate())

    # Calculate timestamp for setting complex gain
    tx_cgain_timestamp = stream_start + uhd.types.TimeSpec.from_ticks(
        tx_cgain_time, radio.get_tick_rate()
    )
    rx_cgain_timestamp = (
        stream_start
        + loopback_delay
        + uhd.types.TimeSpec.from_ticks(rx_cgain_time, radio.get_tick_rate())
    )

    # Test initialization: Set initial gain to 1 + 0j
    for chan in range(num_chan):
        tx_cgain.set_gain_coeff(1 + 0j, chan)
        rx_cgain.set_gain_coeff(1 + 0j, chan)

    for chan in range(num_chan):
        tx_cgain.set_gain_coeff(tx_coeff, chan, tx_cgain_timestamp)
        rx_cgain.set_gain_coeff(rx_coeff, chan, rx_cgain_timestamp)
        samples_reference[chan, tx_cgain_time:] = samples_reference[chan, tx_cgain_time:] * tx_coeff
        samples_reference[chan, rx_cgain_time:] = samples_reference[chan, rx_cgain_time:] * rx_coeff

    process_data(
        radio, rx_streamer, tx_streamer, samples_in, samples_reference, loopback_delay, stream_start
    )


if __name__ == "__main__":
    """Execute the test_complex_gain test with enabled logging."""
    pytest.main([__file__, "-k", "test_complex_gain", "-s", "-vv"] + sys.argv[1:])
    """Execute the test_timed_complex_gain test with enabled logging."""
    pytest.main([__file__, "-k", "test_timed_complex_gain", "-s", "-vv"] + sys.argv[1:])

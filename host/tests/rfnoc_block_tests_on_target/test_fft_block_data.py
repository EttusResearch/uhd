import numpy as np
import pytest
import uhd.rfnoc

from uhd.libpyuhd.rfnoc import fft_direction, fft_magnitude, fft_shift

MIN_FFT_LENGTH = 8
DEFAULT_FFT_LENGTH = 256


@pytest.fixture(scope="session")
def fft_blocks(graph):
    ids = graph.find_blocks("FFT")
    return [uhd.rfnoc.FftBlockControl(graph.get_block(id)) for id in ids]


@pytest.fixture(scope="session")
def rx_streamer(graph, fft_blocks):
    num_chan = 2
    sa = uhd.usrp.StreamArgs("sc16", "sc16")
    fft_blocks = fft_blocks[0:1] # take only the first fft block
    rx_streamer = graph.create_rx_streamer(num_chan * len(fft_blocks), sa)
    for block_idx, block in enumerate(fft_blocks):
        for chan_idx in range(num_chan):
            graph.connect(
                        block.get_unique_id(),
                        chan_idx,
                        rx_streamer,
                        (block_idx * num_chan) + chan_idx,
                    )
    return rx_streamer


@pytest.fixture(scope="session")
def tx_streamer(graph, fft_blocks):
    num_chan = 2
    sa = uhd.usrp.StreamArgs("sc16", "sc16")
    fft_blocks = fft_blocks[0:1] # take only the first fft block
    tx_streamer = graph.create_tx_streamer(num_chan * len(fft_blocks), sa)
    for block_idx, block in enumerate(fft_blocks):
        for chan_idx in range(num_chan):
            graph.connect(
                        tx_streamer,
                        (block_idx * num_chan) + chan_idx,
                        block.get_unique_id(),
                        chan_idx,
                    )
    return tx_streamer


def configure_fft(block,
                  direction=fft_direction.FORWARD,
                  magnitude=fft_magnitude.COMPLEX,
                  length=DEFAULT_FFT_LENGTH,
                  scaling_factor=1.0,
                  cp_insertion_list=[],
                  cp_removal_list=[]):
    block.set_direction(direction)
    if direction == fft_direction.FORWARD:
        block.set_shift_config(fft_shift.NORMAL)
    else:
        block.set_shift_config(fft_shift.NATURAL)
    block.set_magnitude(magnitude)
    block.set_length(length)
    block.set_scaling_factor(scaling_factor)
    block.set_cp_insertion_list(cp_insertion_list)
    block.set_cp_removal_list(cp_removal_list)


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


def create_symbols(channels, num_symbols, fft_length, amplitude):
    """Create symbols with a single tone per symbol."""
    num_chan = len(channels)
    symbols = np.zeros((num_chan, num_symbols * fft_length), dtype=np.complex128)
    for chan_idx, chan in enumerate(channels):
        for symbol_index in range(num_symbols):
            start = symbol_index * fft_length
            end = start + fft_length
            # create single tone at 'subcarrier' with 'amplitude'
            symbol_data = np.zeros(fft_length, dtype=np.complex128)
            subcarrier = ((chan + 1) * fft_length) // (num_chan * 4)
            subcarrier += symbol_index
            symbol_data[subcarrier] = amplitude
            symbols[chan_idx][start:end] = symbol_data
    return symbols


def transmit_and_receive(
    tx_streamer, rx_streamer, data_in, data_out_size
):
    """Transmit and receive.

    Issue receive command for the future time to capture the data when it is
    transmitted. We must also account for the cyclic prefix length, which
    means our acquisition length needs to be longer.
    """
    # Send symbols to the FFT tx block
    tx_md = uhd.types.TXMetadata()
    tx_md.has_time_spec = False
    tx_md.start_of_burst = True
    tx_md.end_of_burst = True
    num_tx = tx_streamer.send(data_in, tx_md, timeout=5.0)

    # Receive the result from the FFT rx block
    rx_md = uhd.types.RXMetadata()
    data_out = np.zeros(data_out_size, dtype=np.uint32)
    num_rx = rx_streamer.recv(data_out, rx_md, 5.0)

    if num_rx < data_out_size[1]:
        raise RuntimeError("ERROR: number of received samples is too low")

    return data_out


def fft_process_data(rx_streamer, tx_streamer, symbols_in, symbols_reference):
    # convert TX data from complex floating point to I16/I16
    data_in = complex128_to_sc16(symbols_in)

    data_out = transmit_and_receive(
        tx_streamer,
        rx_streamer,
        data_in=data_in,
        data_out_size=symbols_reference.shape,
    )

    # Convert RX data from I16/I16 to complex floating point
    symbols_out = sc16_to_complex128(data_out)

    for chan in range(symbols_reference.shape[0]):
        np.testing.assert_allclose(symbols_out[chan], symbols_reference[chan], rtol=1e-4, atol=1e-4)


def test_fft_length(fft_blocks, rx_streamer, tx_streamer, graph):
    graph.commit()
    channels = [0, 1]
    num_chan = len(channels)
    num_symbols = 1
    amplitude = 0.5

    length = MIN_FFT_LENGTH
    max_length = fft_blocks[0].get_max_length()
    while length <= max_length:
        print("Testing FFT length:", length)
        symbols_in = create_symbols(channels, num_symbols, length, amplitude)
        for block in fft_blocks:
            configure_fft(block,
                        length=length,
                        direction=fft_direction.REVERSE,
                        )

        symbols_reference = np.zeros((num_chan, num_symbols * length), dtype=np.complex128)
        for chan in channels:
            symbols_reference[chan] = np.fft.ifft(symbols_in[chan], norm="forward")

        fft_process_data(rx_streamer, tx_streamer, symbols_in, symbols_reference)
        length *= 2


def test_fft_scaling_factor(fft_blocks, rx_streamer, tx_streamer, graph):
    graph.commit()
    channels = [0, 1]
    num_chan = len(channels)
    num_symbols = 1
    amplitude = 0.5

    length = DEFAULT_FFT_LENGTH
    for N_log2 in range(5):
        scaling_factor = 1.0 / (1<<N_log2)
        print("Testing scaling_factor:", scaling_factor)
        symbols_in = create_symbols(channels, num_symbols, length, amplitude)
        for block in fft_blocks:
            configure_fft(block,
                        length=length,
                        direction=fft_direction.REVERSE,
                        scaling_factor=scaling_factor,
                        )

        symbols_reference = np.zeros((num_chan, num_symbols * length), dtype=np.complex128)
        for chan in channels:
            symbols_reference[chan] = scaling_factor * np.fft.ifft(symbols_in[chan], norm="forward")

        fft_process_data(rx_streamer, tx_streamer, symbols_in, symbols_reference)


def test_fft_cp_removal(fft_blocks, rx_streamer, tx_streamer, graph):
    graph.commit()
    channels = [0, 1]
    num_chan = len(channels)
    num_symbols = 1
    amplitude = 0.5
    length = 1024
    cp_removal_list = [512, 100, 100, 100]

    scaling_factor = 1.0
    symbols_in = create_symbols(channels, num_symbols, length, amplitude)
    for block in fft_blocks:
        configure_fft(block,
                    direction=fft_direction.REVERSE,
                    length=length,
                    cp_removal_list=cp_removal_list,
                    )

    symbols_reference = np.zeros((num_chan, num_symbols * length), dtype=np.complex128)
    for chan in channels:
        symbols_reference[chan] = scaling_factor * np.fft.ifft(symbols_in[chan], norm="forward")

    for cp in cp_removal_list:
        print("Testing cp removal value:", cp)
        symbols_in_cp = np.take(symbols_in, range(length-cp, length), axis=1)
        symbols_in_with_cp = np.append(symbols_in_cp, symbols_in, axis=1)
        fft_process_data(rx_streamer, tx_streamer, symbols_in_with_cp, symbols_reference)

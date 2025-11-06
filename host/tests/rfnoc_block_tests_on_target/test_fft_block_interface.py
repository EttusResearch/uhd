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


def test_fft_block_ids(fft_blocks):
    ids = [block.get_unique_id() for block in fft_blocks]
    assert len(ids) > 0
    for i, id in enumerate(ids):
        assert id == f"0/FFT#{i}"


def test_fft_get_direction(fft_blocks):
    for block in fft_blocks:
        assert isinstance(block.get_direction(), fft_direction)


def test_fft_set_direction(fft_blocks):
    for block in fft_blocks:
        current_direction = block.get_direction()
        for direction in fft_direction.__members__.values():
            block.set_direction(direction)
            assert block.get_direction() == direction
        block.set_direction(current_direction)


def test_fft_get_magnitude(fft_blocks):
    for block in fft_blocks:
        assert isinstance(block.get_magnitude(), fft_magnitude)


def test_fft_set_magnitude(fft_blocks):
    for block in fft_blocks:
        current_magnitude = block.get_magnitude()
        for magnitude in fft_magnitude.__members__.values():
            block.set_magnitude(magnitude)
            assert block.get_magnitude() == magnitude
        block.set_magnitude(current_magnitude)


def test_fft_get_shift_config(fft_blocks):
    for block in fft_blocks:
        assert isinstance(block.get_shift_config(), fft_shift)


def test_fft_set_shift_config(fft_blocks):
    for block in fft_blocks:
        current_shift = block.get_shift_config()
        for shift in fft_shift.__members__.values():
            block.set_shift_config(shift)
            assert block.get_shift_config() == shift
        block.set_shift_config(current_shift)


def test_fft_get_length(fft_blocks):
    for block in fft_blocks:
        block.set_length(DEFAULT_FFT_LENGTH)
        assert block.get_length() == DEFAULT_FFT_LENGTH


def test_fft_get_max_length(fft_blocks):
    for block in fft_blocks:
        max_length = block.get_max_length()
        assert (max_length & (max_length - 1)) == 0


def test_fft_set_length(fft_blocks):
    for block in fft_blocks:
        current_length = block.get_length()
        with pytest.raises(RuntimeError):
            block.set_length(MIN_FFT_LENGTH>>1)
        length = MIN_FFT_LENGTH
        max_length = block.get_max_length()
        while length <= max_length:
            block.set_length(length)
            length *= 2
        with pytest.raises(RuntimeError):
            block.set_length(max_length<<1)
        block.set_length(current_length)

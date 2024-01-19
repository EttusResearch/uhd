#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Utilities for generating/analyzing signals
"""

import math
import numpy
import uhd

def get_continuous_tone(rate, freq, ampl, desired_size=None, max_size=None, waveform='sine'):
    """
    Return a buffer containing a complex tone at frequency freq. The tone is
    continuous, that is, repeating this signal will produce a continuous phase
    sinusoid.
    The buffer will try and approximate desired_size in length. If it is not
    possible to create a buffer smaller than max_size, an exception is thrown.

    Arguments:
    rate   -- Sampling rate in Hz.
    freq   -- Tone frequency in Hz
    ampl   -- Amplitude
    desired_size -- Number of samples ideally in returned buffer
    max_size -- Number of samples maximally in returned buffer
    waveform -- Waveform type: 'sine', 'square', 'ramp'
    """
    desired_size = desired_size or 1.0 * rate # About one second worth of data
    max_size = max_size or 100e6
    assert rate > freq
    rate_int = int(rate)
    freq_int = int(freq)
    gcd = math.gcd(rate_int, freq_int)
    rate_int = rate_int / gcd
    freq_int = freq_int / gcd
    length = int(max(freq_int * rate_int, 1)) # freq may be zero
    f_norm = freq/rate
    if waveform in ('sine', 'square'):
        tone = \
            numpy.exp(1j * 2 * numpy.pi * f_norm * numpy.arange(length),
                      dtype=numpy.complex64)
        if waveform == 'square':
            tone = numpy.sign(tone) * ampl
        else:
            tone = tone * ampl
    elif waveform == 'ramp':
        tone = numpy.array(
            [2 * (n * f_norm - numpy.floor(float(0.5 + n * f_norm)))
             for n in range(length)],
            dtype=numpy.complex64)
    elif waveform == 'const':
        tone = numpy.ones(length, dtype=numpy.complex64) * ampl
    else:
        raise KeyError(f"Invalid waveform type: `{waveform}'")

    if length < desired_size:
        tone = numpy.tile(tone, int(desired_size // length))
    if length > max_size:
        raise ValueError("Cannot create a TX buffer! Rate/Freq ratio is too odd.")
    return tone

def get_power_dbfs(signal):
    """
    Return the power in dBFS for a digital signal, where fullscale is considered
    1.0.

    A sinusoid with amplitude 1.0 will thus return 0.0 (dBFS).
    """
    return 10 * numpy.log10(numpy.var(signal))

def get_usrp_power(streamer, num_samps=1e6, chan=0):
    """
    Return the measured input power in dBFS

    The return value is a list of dBFS power values, one per channel.
    """
    recv_buffer = numpy.zeros(
        (streamer.get_num_channels(), num_samps), dtype=numpy.complex64)
    metadata = uhd.types.RXMetadata()
    stream_cmd = uhd.types.StreamCMD(uhd.types.StreamMode.num_done)
    stream_cmd.num_samps = num_samps
    stream_cmd.stream_now = True
    streamer.issue_stream_cmd(stream_cmd)
    # Pass in long timeout, so we can rx the entire buffer in one go
    samps_recvd = streamer.recv(recv_buffer, metadata, 5.0)
    if samps_recvd != num_samps:
        raise RuntimeError(
            "ERROR! get_usrp_power(): Did not receive the correct number of samples!")
    return get_power_dbfs(recv_buffer[chan])

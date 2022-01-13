#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Provide a tone generator class for USRPs.
"""

import threading
import numpy
import uhd

class WaveformGenerator:
    """
    Class that can output arbitrary waveform 
    from a different thread until told to stop
    """
    def __init__(self, iq_data, streamer=None):
        self._buffer = iq_data
        self._streamer = streamer
        self._run = False
        self._thread = None

    def set_streamer(self, streamer):
        """
        Update streamer object
        """
        if self._run:
            self.stop()
        self._streamer = streamer

    def start(self):
        """
        Spawn the thread in the background
        """
        if not self._streamer:
            raise RuntimeError("No streamer defined!")
        self._run = True
        self._thread = threading.Thread(target=self._worker)
        self._thread.start()
        self._thread.setName("cal_tx")

    def stop(self):
        """
        Stop the transmitter
        """
        self._run = False
        self._thread.join()
        self._thread = None

    def _worker(self):
        """ Here is where the action happens """
        metadata = uhd.types.TXMetadata()
        while self._run:
            # Give it a long-ish timeout so we don't have to throttle in here
            if self._streamer.send(self._buffer, metadata, 1.0) != len(self._buffer):
                print("WARNING: Failed to transmit entire buffer in ToneGenerator!")
        # Send an EOB packet with a single zero-valued sample to close out TX
        metadata.end_of_burst = True
        self._streamer.send(
            numpy.array([0], dtype=numpy.complex64), metadata, 0.1)


class ToneGenerator(WaveformGenerator):
    """
    Class that can output a tone based on WaveformGenerator
    """
    def __init__(self, rate, freq, ampl, streamer=None):
        super().__init__(
            uhd.dsp.signals.get_continuous_tone(rate, freq, ampl),
            streamer)

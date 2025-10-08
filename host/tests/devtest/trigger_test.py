#
# Copyright 2025 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Test TX trigger on RX data."""

from uhd_test_base import UHDPythonTestCase


class TxTriggerTest(UHDPythonTestCase):
    """Test class to run trigger test."""

    test_name = "TxTriggerTest"

    def run_test(self, test_name, test_args):
        """Run trigger test.

        See inline comments for test details.
        """
        import numpy as np
        import uhd
        from uhd.types import StreamCMD, StreamMode

        # Create a waveform: 4k samples ramp (uint32 treated as 16bit complex values).
        # This allows bit-exact comparison using digital loopback.
        num_samples = 4096
        step_size = 131
        tx_waveform = np.arange(0, num_samples * step_size, step_size, dtype=np.uint32)

        usrp = uhd.usrp.MultiUSRP(self.args_str)
        st_args = uhd.usrp.StreamArgs("sc16", "sc16")
        st_args.channels = [0]

        # enable digital loopback
        radio = usrp.get_radio_control(0)
        radio.poke32(0x1000, 1)

        # configure RX to start on TX
        stream_cmd = StreamCMD(StreamMode.num_done)
        stream_cmd.stream_now = False
        stream_cmd.trigger = StreamCMD.trigger_t.TX_RUNNING
        stream_cmd.num_samps = len(tx_waveform)

        rx_streamer = usrp.get_rx_stream(st_args)
        rx_streamer.issue_stream_cmd(stream_cmd)

        # start TX in future to allow samples to be transferrred
        seconds_in_future = 0.5
        tx_md = uhd.types.TXMetadata()
        tx_md.has_time_spec = True
        tx_md.time_spec = usrp.get_time_now() + seconds_in_future

        tx_streamer = usrp.get_tx_stream(st_args)
        tx_streamer.send(tx_waveform, tx_md, seconds_in_future)

        # receive the waveform from RX
        rx_waveform = np.zeros((1, len(tx_waveform)), dtype=np.uint32)
        rx_md = uhd.types.RXMetadata()
        rx_streamer.recv(rx_waveform, rx_md, seconds_in_future)

        # Compare the received waveform to the transmitted waveform
        # This constant is dictated by the radio_core trigger implementation
        offset_cycles = 3
        sample_offset = offset_cycles * radio.get_spc()
        expected = np.concatenate(
            [
                np.zeros(sample_offset, dtype=tx_waveform.dtype),
                tx_waveform[: len(tx_waveform) - sample_offset],
            ]
        )
        compare = all(rx_waveform[0] == expected)

        return {"passed": compare}

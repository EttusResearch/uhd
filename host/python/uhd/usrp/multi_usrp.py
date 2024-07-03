#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" @package usrp
Python UHD module containing the MultiUSRP and other objects
"""

import time

import numpy as np

from .. import libpyuhd as lib


def _get_mpm_client(token, mb_args):
    """
    Wrapper function to dynamically generate a Pythonic MPM client from
    MultiUSRP.
    """
    from uhd.utils import mpmtools

    rpc_addr = mb_args.get("mgmt_addr")
    rpc_port = mb_args.get("rpc_port", mpmtools.MPM_RPC_PORT)
    return mpmtools.MPMClient(mpmtools.InitMode.Hijack, rpc_addr, rpc_port, token)


class MultiUSRP(lib.usrp.multi_usrp):
    """
    MultiUSRP object for controlling devices
    """

    def __init__(self, args=""):
        """MultiUSRP constructor"""
        super(MultiUSRP, self).__init__(args)
        # If we're on an MPM device, dynamically add this method to access the
        # MPM client
        if self.get_tree().exists("/mboards/0/token"):
            token = self.get_tree().access_str("/mboards/0/token").get()
            mb_args = self.get_tree().access_device_addr("/mboards/0/args").get().to_dict()
            setattr(self, "get_mpm_client", lambda: _get_mpm_client(token, mb_args))

    def recv_num_samps(
        self, num_samps, freq, rate=1e6, channels=(0,), gain=10, start_time=None, streamer=None
    ):
        """
        RX a finite number of samples from the USRP

        This is a convenience function to minimize the amount of code required
        for just capturing samples. When calling this function more than once
        in a script, pass in a streamer object to avoid recreating streamers
        more than once.

        :param num_samps: number of samples to RX
        :param freq: RX frequency (Hz)
        :param rate: RX sample rate (Hz)
        :param channels: list of channels to RX on
        :param gain: RX gain (dB)
        :param start_time: A valid TimeSpec object with the starting time. If
                           None, then streaming starts immediately.
        :param streamer: An RX streamer object. If None, this function will create
                         one locally and attempt to destroy it afterwards.
        :return: numpy array of complex floating-point samples (fc32)
        """

        def _config_streamer(streamer):
            """
            Set up the correct streamer
            """
            if streamer is None:
                st_args = lib.usrp.stream_args("fc32", "sc16")
                st_args.channels = channels
                streamer = super(MultiUSRP, self).get_rx_stream(st_args)
            return streamer

        def _start_stream(streamer):
            """
            Issue the start-stream command.
            """
            stream_cmd = lib.types.stream_cmd(lib.types.stream_mode.start_cont)
            stream_cmd.stream_now = (len(channels) == 1) and start_time is None
            if not stream_cmd.stream_now:
                if start_time is not None:
                    stream_cmd.time_spec = start_time
                else:
                    stream_cmd.time_spec = lib.types.time_spec(
                        super(MultiUSRP, self).get_time_now().get_real_secs() + 0.05
                    )
            streamer.issue_stream_cmd(stream_cmd)

        def _stop_stream(streamer):
            """
            Issue the stop-stream command and flush the queue.
            """
            metadata = lib.types.rx_metadata()
            stream_cmd = lib.types.stream_cmd(lib.types.stream_mode.stop_cont)
            streamer.issue_stream_cmd(stream_cmd)
            while streamer.recv(recv_buffer, metadata):
                pass

        ## And go!
        # Configure USRP
        for chan in channels:
            super(MultiUSRP, self).set_rx_rate(rate, chan)
            super(MultiUSRP, self).set_rx_freq(lib.types.tune_request(freq), chan)
            super(MultiUSRP, self).set_rx_gain(gain, chan)
        # Configure streamer
        streamer = _config_streamer(streamer)
        metadata = lib.types.rx_metadata()
        # Set up buffers and counters
        result = np.empty((len(channels), num_samps), dtype=np.complex64)
        recv_buffer = np.zeros((len(channels), streamer.get_max_num_samps()), dtype=np.complex64)
        recv_samps = 0
        samps = 0
        # Now stream
        _start_stream(streamer)
        while recv_samps < num_samps:
            samps = streamer.recv(recv_buffer, metadata)
            if metadata.error_code != lib.types.rx_metadata_error_code.none:
                print(metadata.strerror())
            if samps:
                real_samps = min(num_samps - recv_samps, samps)
                result[:, recv_samps : recv_samps + real_samps] = recv_buffer[:, 0:real_samps]
                recv_samps += real_samps
        # Stop and clean up
        _stop_stream(streamer)
        # Help the garbage collection
        streamer = None
        return result

    def send_waveform(
        self,
        waveform_proto,
        duration,
        freq,
        rate=1e6,
        channels=(0,),
        gain=10,
        start_time=None,
        streamer=None,
    ):
        """
        TX a finite number of samples from the USRP
        :param waveform_proto: numpy array of samples to TX
        :param duration: time in seconds to transmit at the supplied rate
        :param freq: TX frequency (Hz)
        :param rate: TX sample rate (Hz)
        :param channels: list of channels to TX on
        :param gain: TX gain (dB)
        :param start_time: A valid TimeSpec object with the starting time. If
                           None, then streaming starts immediately.
        :param streamer: A TX streamer object. If None, this function will create
                         one locally and attempt to destroy it afterwards.
        :return: the number of transmitted samples
        """

        def _config_streamer(streamer):
            """
            Set up the correct streamer
            """
            if streamer is None:
                st_args = lib.usrp.stream_args("fc32", "sc16")
                st_args.channels = channels
                streamer = super(MultiUSRP, self).get_tx_stream(st_args)
            return streamer

        ## And go!
        for chan in channels:
            super(MultiUSRP, self).set_tx_rate(rate, chan)
            if start_time is None:
                super(MultiUSRP, self).set_tx_freq(lib.types.tune_request(freq), chan)
            super(MultiUSRP, self).set_tx_gain(gain, chan)

        if start_time is not None:
            # Use timed commands to set frequency to minimize phase differences
            # between channels. Picking a command time in the middle between
            # now and start time.
            cmd_time_offset = max(
                0, (start_time - super(MultiUSRP, self).get_time_now()).get_real_secs() / 2
            )
            cmd_time = super(MultiUSRP, self).get_time_now() + cmd_time_offset
            super(MultiUSRP, self).set_command_time(cmd_time)
            for chan in channels:
                super(MultiUSRP, self).set_tx_freq(lib.types.tune_request(freq), chan)
            super(MultiUSRP, self).clear_command_time()
            # Enough time for tune time to expire
            time.sleep(cmd_time_offset)

        # Configure streamer
        streamer = _config_streamer(streamer)
        # Set up buffers and counters
        buffer_samps = streamer.get_max_num_samps()
        proto_len = waveform_proto.shape[-1]
        if proto_len < buffer_samps:
            waveform_proto = np.tile(
                waveform_proto, (1, int(np.ceil(float(buffer_samps) / proto_len)))
            )
            proto_len = waveform_proto.shape[-1]
        send_samps = 0
        max_samps = int(np.floor(duration * rate))
        if len(waveform_proto.shape) == 1:
            waveform_proto = waveform_proto.reshape(1, waveform_proto.size)
        if waveform_proto.shape[0] < len(channels):
            waveform_proto = np.tile(waveform_proto[0], (len(channels), 1))
        # Now stream
        metadata = lib.types.tx_metadata()
        if start_time is not None:
            metadata.time_spec = start_time
            metadata.has_time_spec = True
        while send_samps < max_samps:
            real_samps = min(proto_len, max_samps - send_samps)
            if real_samps < proto_len:
                samples = streamer.send(waveform_proto[:, :real_samps], metadata)
            else:
                samples = streamer.send(waveform_proto, metadata)
            send_samps += samples
        # Send EOB to terminate Tx
        metadata.end_of_burst = True
        streamer.send(np.zeros((len(channels), 1), dtype=np.complex64), metadata)
        # Help the garbage collection
        streamer = None
        return send_samps

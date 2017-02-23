#
# Copyright 2017 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

import libpyuhd as lib
import numpy as np


class multi_usrp(object):
    def __init__(self, args=""):
        self.usrp = lib.multi_usrp.multi_usrp.make(args)

    def __del__(self):
        self.usrp = None

    def set_rx_rate(self, rate):
        for chan in xrange(self.usrp.get_rx_num_channels()):
            self.usrp.set_rx_rate(rate, chan)

    def set_tx_rate(self, rate):
        for chan in xrange(self.usrp.get_tx_num_channels()):
            self.usrp.set_tx_rate(rate, chan)

    def recv_num_samps(self, num_samps, freq, rate=1e6, channels=[0], gain=10):
        result = np.empty((len(channels), num_samps), dtype=np.complex64)
        for chan in channels:
            self.usrp.set_rx_rate(rate, chan)
            self.usrp.set_rx_freq(lib.types.tune_request(freq), chan)
            self.usrp.set_rx_gain(gain, chan)
        st_args = lib.types.stream_args("fc32", "sc16")
        st_args.channels = channels
        metadata = lib.types.rx_metadata()
        streamer = self.usrp.get_rx_stream(st_args)
        buffer_samps = streamer.get_max_num_samps()
        recv_buffer = np.zeros(
            (len(channels), buffer_samps), dtype=np.complex64)
        recv_samps = 0
        stream_cmd = lib.types.stream_cmd(lib.types.stream_mode.start_cont)
        stream_cmd.stream_now = True
        streamer.issue_stream_cmd(stream_cmd)
        while (recv_samps < num_samps):
            samps = streamer.recv(recv_buffer, metadata)
            if metadata.error_code != lib.types.rx_metadata_error_code.none:
                print(metadata.strerror())
            if samps:
                real_samps = min(num_samps - recv_samps, samps)
                result[:, recv_samps:recv_samps + real_samps -
                       1] = recv_buffer[:, 0:real_samps - 1]
                recv_samps += real_samps
        stream_cmd = lib.types.stream_cmd(lib.types.stream_mode.stop_cont)
        streamer.issue_stream_cmd(stream_cmd)
        while samps:
            samps = streamer.recv(recv_buffer, metadata)
        # Help the garbage collection
        streamer = None
        return result

    def send_waveform(self,
                      waveform_proto,
                      duration,
                      freq,
                      rate=1e6,
                      channels=[0],
                      gain=10):
        self.set_tx_rate(rate)
        for chan in channels:
            self.usrp.set_tx_rate(rate, chan)
            self.usrp.set_tx_freq(lib.types.tune_request(freq), chan)
            self.usrp.set_tx_gain(gain, chan)
        st_args = lib.types.stream_args("fc32", "sc16")
        st_args.channels = channels
        metadata = lib.types.rx_metadata()
        streamer = self.usrp.get_tx_stream(st_args)
        buffer_samps = streamer.get_max_num_samps()
        proto_len = waveform_proto.shape[-1]
        if proto_len < buffer_samps:
            waveform_proto = np.tile(waveform_proto, (1, int(np.ceil(float(buffer_samps)/proto_len))))
            proto_len = waveform_proto.shape[-1]
        metadata = lib.types.tx_metadata()
        send_samps = 0
        max_samps = int(np.floor(duration * rate))
        if waveform_proto.shape[0] < len(channels):
            waveform_proto = np.tile(waveform_proto[0], (len(channels), 1))
        while send_samps < max_samps:
            real_samps = min(proto_len, max_samps-send_samps)
            if real_samps < proto_len:
                samples = streamer.send(waveform_proto[:real_samps], metadata)
            else:
                samples = streamer.send(waveform_proto, metadata)
            send_samps += samples
        # Help the garbage collection
        streamer = None
        return send_samps

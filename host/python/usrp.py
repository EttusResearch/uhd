#
# Copyright 2017 Ettus Research
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

import numpy as np
from . import libpyuhd as lib


## @package usrp
#  Python UHD module containing the MultiUSRP and other objects

## MultiUSRP object for controlling devices
class MultiUSRP(lib.usrp.multi_usrp):

    ## MultiUSRP constructor
    def __init__(self, args=""):
        super(MultiUSRP, self).__init__(args)

    ## RX a finite number of samples from the USRP
    #  @param num_samps number of samples to RX
    #  @param freq RX frequency
    #  @param rate RX sample rate
    #  @param channels list of channels to RX on
    #  @param gain RX gain in dB
    #  @return numpy array of complex floating-point samples (fc32)
    def recv_num_samps(self, num_samps, freq, rate=1e6, channels=[0], gain=10):
        result = np.empty((len(channels), num_samps), dtype=np.complex64)

        for chan in channels:
            super(MultiUSRP, self).set_rx_rate(rate, chan)
            super(MultiUSRP, self).set_rx_freq(lib.types.tune_request(freq), chan)
            super(MultiUSRP, self).set_rx_gain(gain, chan)

        st_args = lib.usrp.stream_args("fc32", "sc16")
        st_args.channels = channels
        metadata = lib.types.rx_metadata()
        streamer = super(MultiUSRP, self).get_rx_stream(st_args)
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
                result[:, recv_samps:recv_samps + real_samps] = recv_buffer[:, 0:real_samps]
                recv_samps += real_samps

        stream_cmd = lib.types.stream_cmd(lib.types.stream_mode.stop_cont)
        streamer.issue_stream_cmd(stream_cmd)

        while samps:
            samps = streamer.recv(recv_buffer, metadata)

        # Help the garbage collection
        streamer = None
        return result

    ## TX a finite number of samples from the USRP
    #  @param duration time in seconds to transmit at the supplied rate
    #  @param freq TX frequency
    #  @param rate TX sample rate
    #  @param channels list of channels to TX on
    #  @param gain TX gain in dB
    #  @return the number of submitted samples
    def send_waveform(self,
                      waveform_proto,
                      duration,
                      freq,
                      rate=1e6,
                      channels=[0],
                      gain=10):

        super(MultiUSRP, self).set_tx_rate(rate)
        for chan in channels:
            super(MultiUSRP, self).set_tx_rate(rate, chan)
            super(MultiUSRP, self).set_tx_freq(lib.types.tune_request(freq), chan)
            super(MultiUSRP, self).set_tx_gain(gain, chan)

        st_args = lib.usrp.stream_args("fc32", "sc16")
        st_args.channels = channels

        streamer = super(MultiUSRP, self).get_tx_stream(st_args)
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


## See: uhd::usrp::subdev_spec_pair_t
class SubdevSpecPair(lib.usrp.subdev_spec_pair):
    pass


## See: uhd::usrp::subdev_spec_t
class SubdevSpec(lib.usrp.subdev_spec):
    pass


## See: uhd::usrp::gpio_atr::gpio_atr_reg_t
class GPIOAtrReg(lib.usrp.gpio_atr_reg):
    pass


## See: uhd::usrp::gpio_atr::gpio_atr_mode_t
class GPIOAtrMode(lib.usrp.gpio_atr_mode):
    pass


## See: uhd::usrp::dboard_iface::unit_t
class Unit(lib.usrp.unit):
    pass


## See: uhd::usrp::dboard_iface::aux_dac_t
class AuxDAC(lib.usrp.aux_dac):
    pass


## See: uhd::usrp::dboard_iface::aux_adc_t
class AuxADC(lib.usrp.aux_adc):
    pass


## See: uhd::usrp::dboard_iface_special_props_t
class SpecialProps(lib.usrp.special_props):
    pass


## See: uhd::usrp::fe_connection_t::sampling_t
class Sampling(lib.usrp.sampling):
    pass


## See: uhd::usrp::fe_connection_t
class FEConnection(lib.usrp.fe_connection):
    pass


## See: uhd::stream_args_t
class StreamArgs(lib.usrp.stream_args):
    pass


## See: uhd::rx_streamer
class RXStreamer(lib.usrp.rx_streamer):
    pass


## See: uhd::tx_streamer
class TXStreamer(lib.usrp.tx_streamer):
    pass

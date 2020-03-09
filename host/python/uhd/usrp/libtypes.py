#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" @package usrp
Python UHD module containing the MultiUSRP and other objects
"""

from .. import libpyuhd as lib

# Disable PyLint because we want to make it look like the following classes are
# defined in Python, but they're just renames of lib types. They therefore
# follow name conventions for Python classes, not for global constants.
# pylint: disable=invalid-name
SubdevSpecPair = lib.usrp.subdev_spec_pair
SubdevSpec = lib.usrp.subdev_spec
GPIOAtrReg = lib.usrp.gpio_atr_reg
GPIOAtrMode = lib.usrp.gpio_atr_mode
Unit = lib.usrp.unit
AuxDAC = lib.usrp.aux_dac
AuxADC = lib.usrp.aux_adc
SpecialProps = lib.usrp.special_props
Sampling = lib.usrp.sampling
FEConnection = lib.usrp.fe_connection
StreamArgs = lib.usrp.stream_args
RXStreamer = lib.usrp.rx_streamer
TXStreamer = lib.usrp.tx_streamer
# pylint: enable=invalid-name

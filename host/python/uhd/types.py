#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" @package types
Python UHD module containing types to be used with a MultiUSRP object
"""

from . import libpyuhd as lib


StreamMode = lib.types.stream_mode
StreamCMD = lib.types.stream_cmd
DeviceAddr = lib.types.device_addr
TimeSpec = lib.types.time_spec
SPIEdge = lib.types.spi_edge
SPIConfig = lib.types.spi_config
RXMetadataErrorCode = lib.types.rx_metadata_error_code
Range = lib.types.range
MetaRange = lib.types.meta_range_t
GainRange = lib.types.meta_range_t
FreqRange = lib.types.meta_range_t
RXMetadata = lib.types.rx_metadata
TXMetadata = lib.types.tx_metadata
TXAsyncMetadata = lib.types.async_metadata
TXMetadataEventCode = lib.types.tx_metadata_event_code
DataType = lib.types.data_type
SensorValue = lib.types.sensor_value
TuneRequestPolicy = lib.types.tune_request_policy
TuneRequest = lib.types.tune_request
TuneResult = lib.types.tune_result
separate_device_addr = lib.types.separate_device_addr
combine_device_addrs = lib.types.combine_device_addrs
Direction = lib.types.direction_t

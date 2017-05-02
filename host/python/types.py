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

from . import libpyuhd as lib

## @package types
#  Python UHD module containing types to be used with a MultiUSRP object

## See: uhd::stream_cmd_t::stream_mode_t
class StreamMode(lib.types.stream_mode):
    pass


## See: uhd::stream_cmd_t
class StreamCMD(lib.types.stream_cmd):
    pass


## See: uhd::time_spec_t
class TimeSpec(lib.types.time_spec):
    pass


## See: uhd::spi_config_t::spi_edge_t
class SPIEdge(lib.types.spi_edge):
    pass


## See: uhd::spi_config_t
class SPIConfig(lib.types.spi_config):
    pass


## See: uhd::rx_metadata_t::error_code_t
class RXMetadataErrorCode(lib.types.rx_metadata_error_code):
    pass


## See: uhd::range_t
class Range(lib.types.range):
    pass


class RangeVector(lib.types.range_vector):
    pass


## See: uhd::meta_range_t
class MetaRange(lib.types.meta_range):
    pass


## See: uhd::rx_metadata_t
class RXMetadata(lib.types.rx_metadata):
    pass


## See: uhd::tx_metadata_t
class TXMetadata(lib.types.tx_metadata):
    pass


## See: uhd::sensor_value_t::data_type_t
class DataType(lib.types.data_type):
    pass


## See: uhd::sensor_value_t
class SensorValue(lib.types.sensor_value):
    pass


## See: uhd::tune_request_t::policy_t
class TuneRequestPolicy(lib.types.tune_request_policy):
    pass


## See: uhd::tune_request_t
class TuneRequest(lib.types.tune_request):
    pass


## See: uhd::tune_result_t
class TuneResult(lib.types.tune_result):
    pass

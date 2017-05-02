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

## @package filters
#  Python UHD module containing the filter API

## See: uhd::filter_info_base::filter_info_type
class FilterType(lib.filters.filter_type):
    pass


## See: uhd::filter_info_base
class FilterInfoBase(lib.filters.filter_info_base):
    pass


## See: uhd::analog_filter_base
class AnalogFilterBase(lib.filters.analog_filter_base):
    pass


## See: uhd::analog_filter_lp
class AnalogFilterLP(lib.filters.analog_filter_lp):
    pass

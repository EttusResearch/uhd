#
# Copyright 2010 Ettus Research LLC
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

########################################################################
# unit test suite
########################################################################
ADD_EXECUTABLE(main_test
    main_test.cpp
    addr_test.cpp
    buffer_test.cpp
    byteswap_test.cpp
    convert_types_test.cpp
    dict_test.cpp
    error_test.cpp
    gain_group_test.cpp
    subdev_spec_test.cpp
    time_spec_test.cpp
    tune_helper_test.cpp
    vrt_test.cpp
    warning_test.cpp
    wax_test.cpp
)
TARGET_LINK_LIBRARIES(main_test uhd)
ADD_TEST(test main_test)

########################################################################
# demo of a loadable module
########################################################################
ADD_LIBRARY(module_test MODULE module_test.cpp)

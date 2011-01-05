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
SET(test_sources
    addr_test.cpp
    buffer_test.cpp
    byteswap_test.cpp
    convert_test.cpp
    dict_test.cpp
    error_test.cpp
    gain_group_test.cpp
    ranges_test.cpp
    subdev_spec_test.cpp
    time_spec_test.cpp
    tune_helper_test.cpp
    vrt_test.cpp
    warning_test.cpp
    wax_test.cpp
)

#turn each test cpp file into an executable with an int main() function
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK -DBOOST_TEST_MAIN)

#for each source: build an executable, register it as a test, and install
FOREACH(test_source ${test_sources})
    GET_FILENAME_COMPONENT(test_name ${test_source} NAME_WE)
    ADD_EXECUTABLE(${test_name} ${test_source})
    TARGET_LINK_LIBRARIES(${test_name} uhd)
    ADD_TEST(${test_name} ${test_name})
    INSTALL(TARGETS ${test_name} RUNTIME DESTINATION ${PKG_DATA_DIR}/tests)
ENDFOREACH(test_source)

########################################################################
# demo of a loadable module
########################################################################
ADD_LIBRARY(module_test MODULE module_test.cpp)

INSTALL(TARGETS
   RUNTIME DESTINATION ${PKG_DATA_DIR}/tests
)

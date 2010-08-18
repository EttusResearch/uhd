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


ADD_EXECUTABLE(tx_continuous_samples tx_continuous_samples.cpp)
TARGET_LINK_LIBRARIES(tx_continuous_samples uhd)

ADD_EXECUTABLE(benchmark_rx_rate benchmark_rx_rate.cpp)
TARGET_LINK_LIBRARIES(benchmark_rx_rate uhd)

ADD_EXECUTABLE(rx_timed_samples rx_timed_samples.cpp)
TARGET_LINK_LIBRARIES(rx_timed_samples uhd)

ADD_EXECUTABLE(rx_to_file rx_to_file.cpp)
TARGET_LINK_LIBRARIES(rx_to_file uhd)

ADD_EXECUTABLE(tx_timed_samples tx_timed_samples.cpp)
TARGET_LINK_LIBRARIES(tx_timed_samples uhd)

INSTALL(TARGETS
    tx_continuous_samples
    benchmark_rx_rate
    rx_timed_samples
    tx_timed_samples
    RUNTIME DESTINATION ${PKG_DATA_DIR}/examples
)

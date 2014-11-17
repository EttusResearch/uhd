#
# Copyright 2014 Ettus Research LLC
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

UHD_INSTALL(FILES
    # Infrastructure
    block_ctrl_base.hpp
    block_ctrl.hpp
    block_id.hpp
    blockdef.hpp
    constants.hpp
    node_ctrl_base.hpp
    rx_block_ctrl_base.hpp
    stream_sig.hpp
    sink_node_ctrl.hpp
    source_node_ctrl.hpp
    tx_block_ctrl_base.hpp
    # Block controllers
    fir_block_ctrl.hpp
    fft_block_ctrl.hpp
    null_block_ctrl.hpp
    window_block_ctrl.hpp
    addsub_block_ctrl.hpp
    split_stream_block_ctrl.hpp
    packet_resizer_block_ctrl.hpp
    DESTINATION ${INCLUDE_DIR}/uhd/usrp/rfnoc
    COMPONENT headers
)

ADD_SUBDIRECTORY(blocks)
#ADD_SUBDIRECTORY(components)


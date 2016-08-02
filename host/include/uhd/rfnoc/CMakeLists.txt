#
# Copyright 2014-2016 Ettus Research LLC
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

IF(ENABLE_RFNOC)
    UHD_INSTALL(FILES
        # Infrastructure
        block_ctrl_base.hpp
        block_ctrl.hpp
        blockdef.hpp
        block_id.hpp
        constants.hpp
        graph.hpp
        node_ctrl_base.hpp
        node_ctrl_base.ipp
        rate_node_ctrl.hpp
        scalar_node_ctrl.hpp
        sink_block_ctrl_base.hpp
        sink_node_ctrl.hpp
        source_block_ctrl_base.hpp
        source_node_ctrl.hpp
        stream_sig.hpp
        terminator_node_ctrl.hpp
        tick_node_ctrl.hpp
        # Block controllers
        ddc_block_ctrl.hpp
        duc_block_ctrl.hpp
        radio_ctrl.hpp
        DESTINATION ${INCLUDE_DIR}/uhd/rfnoc
        COMPONENT headers
    )
ENDIF(ENABLE_RFNOC)

ADD_SUBDIRECTORY(blocks)
#ADD_SUBDIRECTORY(components)


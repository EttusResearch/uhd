#
# Copyright 2017 Ettus Research (National Instruments)
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
"""
 Module
"""

import libpyusrp_periphs as lib
import logging
import periph_manager
import dboard_manager


log = logging.Logger("usrp_mpm.periphs")

def init_periph_manager(mb_type=None, db_types={}, fpga=None):
    pass
    # Moved to periph_manager/base.py __init__


    # Next steps implemented in periph_manager/derived class
    #
    # 1. Load FPGA image
    # 2. Use motherboard and daughterboard types to load the FPGA image
    # 3. Create periph_manager object wth given mb_type + db_types information

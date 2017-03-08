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
import re
import logging
log = logging.Logger("usrp_mpm.periphs")


def init_periph_manager(mb_type=None, db_types={}, fpga=None):
    # Detect motherboard type if not already specified
    if mb_type is None:
        mb_files = lib.helper.find_mb_file()
        with open(mb_files, "r") as f:
            info = "".join(f.readlines())
            device_type = re.match("^.*USRP;([-\w])+;.*", info)
        if device_type is None:
            log.fatal("Could not determine device type from {}".format(info))
            exit(1)
        mb_type = device_type.group(1)
    # Check if we have an implementation for this motherboard type
    try:
        device_class = getattr(lib, mb_type)
    except AttributeError:
        log.fatal("Motherboard class implementation for {} device not found!".format(mb_type))
        exit(1)

    # Detect daughterboard type if not already specified
    if not db_types:
        db_files = lib.helper.find_db_files()
        db_types = {}
        for db in db_files:
            with open(db, "r") as f:
                info = "".join(f.readlines())
                device_type = re.match("^.*SLOT;([\w]);DB;([-\w])+;.*", info)
                if device_type is None:
                    log.fatal("Could not determine device type from: {}".format(info))
                    exit(2)
                slot = device_type.group(1)
                db_type = device_type.group(2)
                db_types.update({slot: db_type})
    # Check if we have an implementation for the daughterboard types
    for db in db_types.values():
        try:
            getattr(lib.db, db)
        except AttributeError:
            log.fatal("Daughterboard class implementation for {} device not found!".format(db))
            exit(1)

    # Next steps
    #
    # 1. Load FPGA image
    # 2. Use motherboard and daughterboard types to load the FPGA image
    # 3. Create periph_manager object wth given mb_type + db_types information

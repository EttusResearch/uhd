#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Dummy daughterboard class for empty slots
"""
from usrp_mpm.dboard_manager import DboardManagerBase
from usrp_mpm.mpmlog import get_logger

class EmptySlot(DboardManagerBase):
    """
    DboardManager class for when a slot is empty
    """
    #########################################################################
    # Overridables
    #
    # See DboardManagerBase for documentation on these fields
    #########################################################################
    pids = [0x0]
    ### End of overridables #################################################

    def __init__(self, slot_idx, **kwargs):
        DboardManagerBase.__init__(self, slot_idx, **kwargs)
        self.log = get_logger("EmptyDB-{}".format(slot_idx))
        self.log.trace("Initializing Empty dboard, slot index %d",
                       self.slot_idx)

    def init(self, args):
        """
        Execute necessary init dance to bring up dboard
        """
        self.log.debug("init() called with args `{}'".format(
            ",".join(['{}={}'.format(x, args[x]) for x in args])
        ))
        return True

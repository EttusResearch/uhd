# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
FBX dboard implementation module
"""

from usrp_mpm.dboard_manager import DboardManagerBase
from usrp_mpm.dboard_manager.x4xx_db import X4xxDbMixin
from usrp_mpm.periph_manager.x4xx_periphs import get_temp_sensor

# pylint: disable=too-few-public-methods

###############################################################################
# Main dboard control class
###############################################################################
class FBX(X4xxDbMixin, DboardManagerBase):
    """
    Holds all dboard specific information and methods of the FBX dboard
    """
    #########################################################################
    # Overridables
    #
    # See DboardManagerBase for documentation on these fields
    #########################################################################
    pids = [0x4007]
    rx_sensor_callback_map = {
        'temperature': 'get_rf_temp_sensor',
        'rfdc_rate': 'get_rfdc_rate_sensor',
    }
    tx_sensor_callback_map = {
        'temperature': 'get_rf_temp_sensor',
        'rfdc_rate': 'get_rfdc_rate_sensor',
    }
    # FBX depends on several RF core implementations which each have
    # compat versions.
    updateable_components = {
        'fpga': {
            'compatibility': {
                'rf_core_full': {
                    'current': (1, 0),
                    'oldest': (1, 0),
                },
            }
        },
    }
    ### End of overridables #################################################

    ### Daughterboard driver/hardware compatibility value
    # The FBX has a field in its EEPROM which stores a rev_compat value. This
    # tells us which other revisions of the FBX this revision is compatible with.
    #
    # In theory, we could make the revision compatibility check a simple "less
    # or equal than comparison", i.e., we can support a certain revision and all
    # previous revisions. However, we deliberately don't support Revision A (0x1),
    # and we prefer to explicitly list the valid compat revision numbers we
    # know exist. No matter how, we need to change this line everytime we add a
    # new revision that is incompatible with the previous.
    #
    # In the EEPROM, we only change this number for hardware revisions that are
    # not compatible with this software version.
    DBOARD_SUPPORTED_COMPAT_REVS = (0x1,)

    #########################################################################
    # MPM Initialization
    #########################################################################
    def __init__(self, slot_idx, **kwargs):
        super().__init__("FBX", slot_idx, **kwargs)

    #########################################################################
    # UHD (De-)Initialization
    #########################################################################
    def init(self, args):
        """
        Execute necessary init dance to bring up dboard. This happens when a UHD
        session starts.
        """
        self.log.debug("init() called with args `{}'".format(
            ",".join(['{}={}'.format(x, args[x]) for x in args])
        ))
        return True

    def deinit(self):
        """
        De-initialize after UHD session completes
        """
        self.log.debug("Setting board back to safe defaults after UHD session.")

    def tear_down(self):
        self.db_iface.tear_down()

    ###########################################################################
    # LEDs
    ###########################################################################
    def set_leds(self, channel, rx, trx_rx, trx_tx):
        """ Set the frontpanel LEDs """
        assert channel in (0, 1, 2, 3)

    ###########################################################################
    # Sensors
    ###########################################################################
    def get_rf_temp_sensor(self, _):
        """
        Return the RF temperature sensor value
        """
        self.log.trace("Reading RF daughterboard temperature.")
        sensor_names = [
            f"TMP112 DB{self.slot_idx} Top",
            f"TMP112 DB{self.slot_idx} Bottom",
        ]
        return get_temp_sensor(sensor_names, log=self.log)

#
# Copyright 2018 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
E310 dboard (RF and control) implementation module
"""

from usrp_mpm import lib # Pulls in everything from C++-land
from usrp_mpm.dboard_manager import DboardManagerBase, AD936xDboard
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.periph_manager.e31x_periphs import MboardRegsControl

DEFAULT_MASTER_CLOCK_RATE = 16e6
###############################################################################
# Main dboard control class
###############################################################################
class E31x_db(DboardManagerBase, AD936xDboard):
    """
    Holds all dboard specific information and methods of the E31x_db dboard
    """
    #########################################################################
    # Overridables
    #
    # See DboardManagerBase for documentation on these fields
    #########################################################################
    pids = [0x0110]
    rx_sensor_callback_map = {
        'ad9361_temperature': 'get_catalina_temp_sensor',
        'rssi' : 'get_rssi_sensor',
        # For backward compatibility reasons we have the same sensor with two
        # different names
        'lo_lock' : 'get_rx_lo_lock_sensor',
        'lo_locked' : 'get_rx_lo_lock_sensor',
    }
    tx_sensor_callback_map = {
        'ad9361_temperature': 'get_catalina_temp_sensor',
        # For backward compatibility reasons we have the same sensor with two
        # different names
        'lo_lock' : 'get_tx_lo_lock_sensor',
        'lo_locked' : 'get_tx_lo_lock_sensor',
    }
    # Maps the chipselects to the corresponding devices:
    spi_chipselect = {"catalina": 0}
    ### End of overridables #################################################
    # MB regs label: Needed to access the lock bit
    mboard_regs_label = "mboard-regs"

    def __init__(self, slot_idx, **kwargs):
        DboardManagerBase.__init__(self, slot_idx, **kwargs)
        AD936xDboard.__init__(
            self, lambda: MboardRegsControl(self.mboard_regs_label, self.log))
        self.log = get_logger("E31x_db-{}".format(slot_idx))
        self.log.trace("Initializing e31x daughterboard, slot index %d",
                       self.slot_idx)
        self.swap_fe = True
        self.rev = int(self.device_info['rev'])
        self.log.trace("This is a rev: {}".format(chr(65 + self.rev)))
        # These will get updated during init()
        self.master_clock_rate = None
        # Now initialize all peripherals. If that doesn't work, put this class
        # into a non-functional state (but don't crash, or we can't talk to it
        # any more):
        try:
            self._init_periphs()
            self._periphs_initialized = True
        except BaseException as ex:
            self.log.error("Failed to initialize peripherals: %s", str(ex))
            self._periphs_initialized = False

    def _init_periphs(self):
        """
        Initialize power and peripherals that don't need user-settings
        """
        self.log.debug("Loading C++ drivers...")
        # Setup AD9361 / the E31x_db Manager
        self._device = lib.dboards.e31x_db_manager(self._spi_nodes['catalina'])
        ad936x_rfic = self._device.get_radio_ctrl()
        self.log.trace("Loaded C++ drivers.")
        self._init_cat_api(ad936x_rfic)

    def init(self, args):
        """
        Initialize RF. Remember that we might have to do this from scratch every
        time because of the FPGA reload.
        """
        if not self._periphs_initialized:
            error_msg = "Cannot run init(), peripherals are not initialized!"
            self.log.error(error_msg)
            raise RuntimeError(error_msg)
        master_clock_rate = \
            float(args.get('master_clock_rate', DEFAULT_MASTER_CLOCK_RATE))
        self.init_rfic(master_clock_rate)
        return True

    def tear_down(self):
        """
        De-init this object as much as possible.
        """
        self.tear_down_rfic()

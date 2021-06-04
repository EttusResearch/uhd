#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Debug dboard implementation module
"""

from usrp_mpm.dboard_manager import DboardManagerBase
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.sys_utils.gpio import Gpio

class DebugDboardSignalPath:
    def __init__(self, slot_idx, path, adc_indexes, dac_indexes, loopback):
        self.log = get_logger("X4xxDebugDboard-{}-path-{}".format(slot_idx, path))

        self.rxa2_led = Gpio("DB{}_RX{}2_LED".format(slot_idx, path), Gpio.OUTPUT, 0)
        self.rxa_led = Gpio("DB{}_RX{}_LED".format(slot_idx, path), Gpio.OUTPUT, 0)
        self.txa_led = Gpio("DB{}_TX{}_LED".format(slot_idx, path), Gpio.OUTPUT, 0)

        self.trx_ctrl = Gpio("DB{}_TRX{}_CTRL".format(slot_idx, path), Gpio.OUTPUT, 0)
        self.rx_mux_ctrl = Gpio("DB{}_RX{}_MUX_CTRL".format(slot_idx, path), Gpio.OUTPUT, 0)
        self.tx_mux_ctrl = Gpio("DB{}_TX{}_MUX_CTRL".format(slot_idx, path), Gpio.OUTPUT, 0)

        self._adc_indices = adc_indexes
        self._dac_indices = dac_indexes
        self._loopback = loopback
        self._path = path

    def configure(self, adc, dac, loopback):
        """
        Configure this path with the appropriate settings
        """
        if adc.lower() not in self._adc_indices:
            error_msg = "Could not find ADC {} on path {}. Possible ADCs: {}".format(
                adc, self._path, ", ".join(self._adc_indices.keys())
            )
            self.log.error(error_msg)
            raise RuntimeError(error_msg)

        if dac.lower() not in self._dac_indices:
            error_msg = "Could not find DAC {} on path {}. Possible DACs: {}".format(
                dac, self._path, ", ".join(self._dac_indices.keys())
            )
            self.log.error(error_msg)
            raise RuntimeError(error_msg)

        self.rx_mux_ctrl.set(self._adc_indices[adc.lower()])
        self.tx_mux_ctrl.set(self._dac_indices[dac.lower()])
        self.trx_ctrl.set(self._loopback if loopback else not self._loopback)


class X4xxDebugDboard(DboardManagerBase):
    """
    Holds all dboard specific information and methods of the X4xx debug dboard
    """
    #########################################################################
    # Overridables
    #
    # See DboardManagerBase for documentation on these fields
    #########################################################################
    pids = [0x4001]
    ### End of overridables #################################################

    def __init__(self, slot_idx, **kwargs):
        DboardManagerBase.__init__(self, slot_idx, **kwargs)
        self.log = get_logger("X4xxDebugDboard-{}".format(slot_idx))
        self.log.trace("Initializing X4xxDebug daughterboard, slot index %d",
                       self.slot_idx)

        # Interface with MB HW
        if 'db_iface' not in kwargs:
            self.log.error("Required DB Iface was not provided!")
            raise RuntimeError("Required DB Iface was not provided!")
        self.db_iface = kwargs['db_iface']

        # Power on the card
        self.db_iface.enable_daughterboard(enable=True)
        if not self.db_iface.check_enable_daughterboard():
            self.db_iface.enable_daughterboard(enable=False)
            self.log.error('Debug dboard {} power up failed'.format(self.slot_idx))
            raise RuntimeError('Debug dboard {} power up failed'.format(self.slot_idx))

        self._paths = {
            "a": DebugDboardSignalPath(
                slot_idx,
                "A",
                {
                    "adc0": 1,
                    "adc2": 0,
                },
                {
                    "dac0": 1,
                    "dac2": 0,
                },
                1  # TRXA_CTRL=1 enables loopback
            ),
            "b": DebugDboardSignalPath(
                slot_idx,
                "B",
                {
                    "adc3": 1,
                    "adc1": 0,
                },
                {
                    "dac3": 1,
                    "dac1": 0,
                },
                0  # TRXB_CTRL=0 enables loopback
            ),
        }


        # TODO: Configure the correct RFDC settings for this board
        #if not self.db_iface.disable_mixer():
        #    raise RuntimeError("Received an error disabling the mixer for slot_idx={}".format(slot_idx))

    def init(self, args):
        """
        Execute necessary init dance to bring up dboard
        """
        self.log.debug("init() called with args `{}'".format(
            ",".join(['{}={}'.format(x, args[x]) for x in args])
        ))
        self.config_path("a", "adc0", "dac0", 0)
        self.config_path("b", "adc1", "dac1", 0)
        return True

    def deinit(self):
        pass

    def tear_down(self):
        self.db_iface.tear_down()

    def config_path(self, path, adc, dac, loopback):
        """
        Configure the signal paths on the daughterboard.
        path - Select between front panel connectors A or B. The two paths are unconnected.
        adc - Select which ADC to connect to the path (adc0 or adc2 on A, adc1 or adc3 on B)
        dac - Select which DAC to connect to the path (dac0 or dac2 on A, dac1 or dac3 on B)
        loopback - Whether to enable loopback (1) or route the ADC/DACs to the front panel (0)

        Example MPM shell usage:
        > db_0_config_path a adc0 dac2 1
        """
        if path.lower() not in self._paths:
            self.log.error("Tried to configure path {} which does not exist!".format(path))
            raise RuntimeError("Tried to configure path {} which does not exist!".format(path))

        path = self._paths[path.lower()]
        path.configure(adc, dac, int(loopback))

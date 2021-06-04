#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
IF Test CCA implementation module
"""
from usrp_mpm.dboard_manager import DboardManagerBase
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.sys_utils.gpio import Gpio

class X4xxIfTestCCA(DboardManagerBase):
    """
    Holds all dboard specific information and methods of the X4xx IF Test CCA
    """
    #########################################################################
    # Overridables
    #
    # See DboardManagerBase for documentation on these fields
    #########################################################################
    pids = [0x4006]
    ### End of overridables #################################################

    def __init__(self, slot_idx, **kwargs):
        DboardManagerBase.__init__(self, slot_idx, **kwargs)
        self.log = get_logger("X4xxIfTestCCA-{}".format(slot_idx))
        self.log.trace("Initializing X4xxIfTestCCA, slot index %d",
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
            self.log.error('IF Test CCA {} power up failed'.format(self.slot_idx))
            raise RuntimeError('IF Test CCA {} power up failed'.format(self.slot_idx))

        # [boolean for stage 1 mux , boolean for stage 2 mux]
        self._adc_mux_settings = {
            "adc0" : [0, 0],
            "adc1" : [1, 1],
            "adc2" : [1, 0],
            "adc3" : [0, 1],
        }

        self._dac_mux_settings = {
            "dac0" : [1, 0],
            "dac1" : [1, 1],
            "dac2" : [0, 0],
            "dac3" : [0, 1],
        }

        # There are 4 possible Tx (DAC) streams that are available to choose
        # to export to the SMA TX port using a 2-stage hardware mux.

        # Choose between 0 and 2 OR 1 and 3
        self.tx_0_2_1_3_mux_ctrl = Gpio("DB{}_TX0_2p_1_3n".format(slot_idx), Gpio.OUTPUT, 0)
        # Choose between 0 OR 2
        self.tx_0_2_mux_ctrl = Gpio("DB{}_TX_MUX_0p_2n".format(slot_idx), Gpio.OUTPUT, 0)
        # Choose between 1 OR 3
        self.tx_1_3_mux_ctrl = Gpio("DB{}_TX_MUX_1p_3n".format(slot_idx), Gpio.OUTPUT, 0)

        # The signal from the SMA RX port can be directed to one of the 4
        # available Rx (ADC) streams using a 2-stage hardware mux.

        # Choose between 0 and 2 OR 1 and 3
        self.rx_0_2_1_3_mux_ctrl = Gpio("DB{}_RX0_2p_1_3n".format(slot_idx), Gpio.OUTPUT, 0)
        # Choose between 0 OR 2
        self.rx_0_2_mux_ctrl = Gpio("DB{}_RX_MUX_0p_2n".format(slot_idx), Gpio.OUTPUT, 0)
        # Choose between 1 OR 3
        self.rx_1_3_mux_ctrl = Gpio("DB{}_RX_MUX_1p_3n".format(slot_idx), Gpio.OUTPUT, 0)

        self._tx_path = ""
        self._rx_path = ""

        # Controls to load the power supplies on the daughterboard. Enabling
        # these will increase the power draw of the daughterboard.
        self.enable_1v8_load = Gpio("DB{}_1V8_LOAD".format(slot_idx), Gpio.OUTPUT, 0)
        self.enable_2v5_load = Gpio("DB{}_2V5_LOAD".format(slot_idx), Gpio.OUTPUT, 0)
        self.enable_3v3_load = Gpio("DB{}_3V3_LOAD".format(slot_idx), Gpio.OUTPUT, 0)
        self.enable_3v3_mcu_load = Gpio("DB{}_3V3_MCU_LOAD".format(slot_idx), Gpio.OUTPUT, 0)
        self.enable_3v7_load = Gpio("DB{}_3V7_LOAD".format(slot_idx), Gpio.OUTPUT, 0)
        self.enable_12v_load = Gpio("DB{}_12V_LOAD".format(slot_idx), Gpio.OUTPUT, 0)

        # Control to choose between DAC output or MB VCM signals as the VCM
        # signal to use on board.
        self.disable_vcm_dac = Gpio("DB{}_VCM_MB_nDAC".format(slot_idx), Gpio.OUTPUT, 0)

        # Control to choose which MB clock to output to the SMA Clock port.
        # Choices are BaseRefClk and PllRefClk
        self.disable_vcm_dac = Gpio("DB{}_REF_CLK_SEL_USR".format(slot_idx), Gpio.OUTPUT, 0)


    def init(self, args):
        """
        Execute necessary init dance to bring up dboard
        """
        self.log.debug("init() called with args `{}'".format(
            ",".join(['{}={}'.format(x, args[x]) for x in args])
        ))
        self.config_tx_path("dac0")
        self.config_rx_path("adc0")
        return True

    def deinit(self):
        pass

    def tear_down(self):
        self.db_iface.tear_down()

    def config_tx_path(self, dac):
        """
        Configure the tx signal path on the daughterboard.
        dac - Select which DAC to connect to the Tx path (dac0 through dac3)

        Example MPM shell usage:
        > db_0_config_tx_path dac2
        """

        if dac.lower() not in self._dac_mux_settings:
            error_msg = "Could not find DAC {}. Possible DACs: {}".format(
                dac, ", ".join(self._dac_mux_settings.keys())
            )
            self.log.error(error_msg)
            raise RuntimeError(error_msg)

        # Only one of the following setting really matters; simplify logic
        # by toggling both since the stage 2 decides what gets connected.
        self.tx_0_2_mux_ctrl.set(self._dac_mux_settings[dac.lower()][0])
        self.tx_1_3_mux_ctrl.set(self._dac_mux_settings[dac.lower()][0])
        self.tx_0_2_1_3_mux_ctrl.set(self._dac_mux_settings[dac.lower()][1])
        self._tx_path = dac.upper()

    def get_tx_path(self):
        return self._tx_path

    def config_rx_path(self, adc):
        """
        Configure the rx signal path on the daughterboard.
        adc - Select which ADC to connect to the Rx path (adc0 through adc3)

        Example MPM shell usage:
        > db_0_config_rx_path adc0
        """

        if adc.lower() not in self._adc_mux_settings:
            error_msg = "Could not find ADC {}. Possible ADCs: {}".format(
                adc, ", ".join(self._adc_mux_settings.keys())
            )
            self.log.error(error_msg)
            raise RuntimeError(error_msg)

        self.rx_0_2_1_3_mux_ctrl.set(self._adc_mux_settings[adc.lower()][1])
        # Only one of the following setting really matters; simplify logic
        # by toggling both
        self.rx_0_2_mux_ctrl.set(self._adc_mux_settings[adc.lower()][0])
        self.rx_1_3_mux_ctrl.set(self._adc_mux_settings[adc.lower()][0])
        self._rx_path = adc.upper()

    def get_rx_path(self):
        return self._rx_path

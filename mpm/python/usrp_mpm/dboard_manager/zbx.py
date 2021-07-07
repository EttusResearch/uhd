#
# Copyright 2019-2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
ZBX dboard implementation module
"""

import time
from usrp_mpm import tlv_eeprom
from usrp_mpm.dboard_manager import DboardManagerBase
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.chips.ic_reg_maps import zbx_cpld_regs_t
from usrp_mpm.periph_manager.x4xx_periphs import get_temp_sensor
from usrp_mpm.sys_utils.udev import get_eeprom_paths_by_symbol

###############################################################################
# Helpers
###############################################################################
def parse_encoded_git_hash(encoded):
    """
    Helper function: Unpacks the git hash encoded in the ZBX CPLD image into
    the git hash and a dirty flag.
    """
    git_hash = encoded & 0x0FFFFFFF
    tree_dirty = ((encoded & 0xF0000000) > 0)
    dirtiness_qualifier = 'dirty' if tree_dirty else 'clean'
    return (git_hash, dirtiness_qualifier)

# pylint: disable=too-few-public-methods
class EepromTagMap:
    """
    Defines the tagmap for EEPROMs matching this magic.
    The tagmap is a dictionary mapping an 8-bit tag to a NamedStruct instance.
    The canonical list of tags and the binary layout of the associated structs
    is defined in mpm/tools/tlv_eeprom/usrp_eeprom.h. Only the subset relevant
    to MPM are included below.
    """
    magic = 0x55535250
    tagmap = {
        # 0x10: usrp_eeprom_board_info
        0x10: tlv_eeprom.NamedStruct('< H H H 7s 1x',
                                     ['pid', 'rev', 'rev_compat', 'serial']),
    }


###############################################################################
# Main dboard control class
###############################################################################
class ZBX(DboardManagerBase):
    """
    Holds all dboard specific information and methods of the ZBX dboard
    """
    #########################################################################
    # Overridables
    #
    # See DboardManagerBase for documentation on these fields
    #########################################################################
    pids = [0x4002]
    rx_sensor_callback_map = {
        'temperature': 'get_rf_temp_sensor',
    }
    tx_sensor_callback_map = {
        'temperature': 'get_rf_temp_sensor',
    }
    ### End of overridables #################################################

    ### Daughterboard driver/hardware compatibility value
    # The ZBX has a field in its EEPROM which stores a rev_compat value. This
    # tells us which other revisions of the ZBX this revision is compatible with.
    #
    # In theory, we could make the revision compatibility check a simple "less
    # or equal than comparison", i.e., we can support a certain revision and all
    # previous revisions. However, we deliberately don't support Revision A (0x1),
    # and we prefer to explicitly list the valid compat revision numbers we
    # know exist. No matter how, we need to change this line everytime we add a
    # new revision that is incompatible with the previous.
    #
    # In the EEPROM, we only change this number for hardware revisions that are
    # not compatible with this software version. Note the CPLD image has its own
    # compat number (see below).
    #
    # RevB and all compatible revisions are supported (that includes RevC). RevA
    # is not supported.
    DBOARD_SUPPORTED_COMPAT_REVS = (0x2,)

    # CPLD compatibility revision
    # Change this revision only on breaking changes.
    REQ_OLDEST_COMPAT_REV = 0x20110611
    REQ_COMPAT_REV = 0x20110611

    #########################################################################
    # MPM Initialization
    #########################################################################
    def __init__(self, slot_idx, **kwargs):
        DboardManagerBase.__init__(self, slot_idx, **kwargs)
        self.log = get_logger("ZBX-{}".format(slot_idx))
        self.log.trace("Initializing ZBX daughterboard, slot index %d",
                       self.slot_idx)

        # local variable to track if PLL ref clock is enabled for the CPLD logic
        self._clock_enabled = False

        # Interface with MB HW
        if 'db_iface' not in kwargs:
            self.log.error("Required DB Iface was not provided!")
            raise RuntimeError("Required DB Iface was not provided!")
        self.db_iface = kwargs['db_iface']

        self.eeprom_symbol = f"db{slot_idx}_eeprom"
        eeprom = self._get_eeprom()
        self._assert_rev_compatibility(eeprom["rev_compat"])
        # Initialize daughterboard CPLD control
        self.poke_cpld = self.db_iface.poke_db_cpld
        self.peek_cpld = self.db_iface.peek_db_cpld
        self.regs = zbx_cpld_regs_t()
        self._spi_addr = self.regs.SPI_READY_addr
        self._enable_base_power()
        # Check register map compatibility
        self._check_compat_version()
        self.log.debug("ZBX CPLD build git hash: %s", self._get_cpld_git_hash())
        # Power up the DB
        self._enable_power()
        # enable PLL reference clock
        self.reset_clock(False)
        self._cpld_set_safe_defaults()

    def _get_eeprom(self):
        """
        Return the eeprom data.
        """
        path = get_eeprom_paths_by_symbol(self.eeprom_symbol)[self.eeprom_symbol]
        eeprom, _ = tlv_eeprom.read_eeprom(path, EepromTagMap.tagmap, EepromTagMap.magic, None)
        return eeprom

    def _assert_rev_compatibility(self, rev_compat):
        """
        Check the ZBX hardware revision compatibility with this driver.

        Throws a RuntimeError() if this version of MPM does not recognize the
        hardware.

        Note: The CPLD image version is checked separately.
        """
        if rev_compat not in self.DBOARD_SUPPORTED_COMPAT_REVS:
            err = "This MPM version is not compatible with this ZBX daughterboard. " \
                f"Found rev_compat value: 0x{rev_compat:02x}. " \
                "Please update your MPM version to support this daughterboard revision."
            self.log.error(err)
            raise RuntimeError(err)

    def _enable_base_power(self, enable=True):
        """
        Enables or disables power to the DB which enables communication to DB CPLD
        """
        if enable:
            self.db_iface.enable_daughterboard(enable=True)
            if not self.db_iface.check_enable_daughterboard():
                self.db_iface.enable_daughterboard(enable=False)
                self.log.error('ZBX {} power up failed'.format(self.slot_idx))
                raise RuntimeError('ZBX {} power up failed'.format(self.slot_idx))
        else: # disable
            # Removing power from the CPLD will set all the the output pins to open and the
            # supplies default to disabled on power up.
            self.db_iface.enable_daughterboard(enable=False)
            if self.db_iface.check_enable_daughterboard():
                self.log.error('ZBX {} power down failed'.format(self.slot_idx))

    def _enable_power(self, enable=True):
        """ Enables or disables power switches internal to the DB CPLD """
        self.regs.ENABLE_TX_POS_7V0 = self.regs.ENABLE_TX_POS_7V0_t(int(enable))
        self.regs.ENABLE_RX_POS_7V0 = self.regs.ENABLE_RX_POS_7V0_t(int(enable))
        self.regs.ENABLE_POS_3V3 = self.regs.ENABLE_POS_3V3_t(int(enable))
        self.poke_cpld(
            self.regs.ENABLE_POS_3V3_addr,
            self.regs.get_reg(self.regs.ENABLE_POS_3V3_addr))

    def _check_compat_version(self):
        """ Check compatibility of DB CPLD image and SW regmap """
        compat_revision_addr = self.regs.OLDEST_COMPAT_REVISION_addr
        cpld_oldest_compat_revision = self.peek_cpld(compat_revision_addr)
        if cpld_oldest_compat_revision < self.REQ_OLDEST_COMPAT_REV:
            err_msg = (
                f'DB CPLD oldest compatible revision 0x{cpld_oldest_compat_revision:x}'
                f' is out of date, the required revision is 0x{self.REQ_OLDEST_COMPAT_REV:x}. '
                f'Update your CPLD image.')
            self.log.error(err_msg)
            raise RuntimeError(err_msg)
        if cpld_oldest_compat_revision > self.REQ_OLDEST_COMPAT_REV:
            err_msg = (
                f'DB CPLD oldest compatible revision 0x{cpld_oldest_compat_revision:x}'
                f' is newer than the expected revision 0x{self.REQ_OLDEST_COMPAT_REV:x}.'
                ' Downgrade your CPLD image or update MPM.')
            self.log.error(err_msg)
            raise RuntimeError(err_msg)

        if not self.has_compat_version(self.REQ_COMPAT_REV):
            err_msg = (
                "ZBX DB CPLD revision is too old. Update your"
                f" CPLD image to at least 0x{self.REQ_COMPAT_REV:08x}.")
            self.log.error(err_msg)
            raise RuntimeError(err_msg)

    def has_compat_version(self, min_required_version):
        """
        Check for a minimum required version.
        """
        cpld_image_compat_revision = self.peek_cpld(self.regs.REVISION_addr)
        return cpld_image_compat_revision >= min_required_version

    # pylint: disable=too-many-statements
    def _cpld_set_safe_defaults(self):
        """
        Set the CPLD into a safe state.
        """
        cpld_regs = zbx_cpld_regs_t()
        # We un-configure some registers to force a change later. None of these
        # values get written to the CPLD!
        cpld_regs.RF0_OPTION = cpld_regs.RF0_OPTION.RF0_OPTION_FPGA_STATE
        cpld_regs.RF1_OPTION = cpld_regs.RF1_OPTION.RF1_OPTION_FPGA_STATE
        cpld_regs.SW_RF0_CONFIG = 255
        cpld_regs.SW_RF1_CONFIG = 255
        cpld_regs.TX0_DSA1[0] = 0
        cpld_regs.TX0_DSA2[0] = 0
        cpld_regs.RX0_DSA1[0] = 0
        cpld_regs.RX0_DSA2[0] = 0
        cpld_regs.RX0_DSA3_A[0] = 0
        cpld_regs.RX0_DSA3_B[0] = 0
        cpld_regs.save_state()
        # Now all the registers we touch will be enumerated by get_changed_addrs()
        # Everything below *will* get written to the CPLD:
        # ATR control
        cpld_regs.RF0_OPTION = cpld_regs.RF0_OPTION.RF0_OPTION_SW_DEFINED
        cpld_regs.RF1_OPTION = cpld_regs.RF1_OPTION.RF1_OPTION_SW_DEFINED
        # Back to state 0 and sw-defined. That means nothing will get configured
        # until UHD boots again.
        cpld_regs.SW_RF0_CONFIG = 0
        cpld_regs.SW_RF1_CONFIG = 0
        # TX0 path control
        cpld_regs.TX0_IF2_1_2[0] = cpld_regs.TX0_IF2_1_2[0].TX0_IF2_1_2_FILTER_2
        cpld_regs.TX0_IF1_3[0] = cpld_regs.TX0_IF1_3[0].TX0_IF1_3_FILTER_0_3
        cpld_regs.TX0_IF1_4[0] = cpld_regs.TX0_IF1_4[0].TX0_IF1_4_TERMINATION
        cpld_regs.TX0_IF1_5[0] = cpld_regs.TX0_IF1_5[0].TX0_IF1_5_TERMINATION
        cpld_regs.TX0_IF1_6[0] = cpld_regs.TX0_IF1_6[0].TX0_IF1_6_FILTER_0_3
        cpld_regs.TX0_7[0] = cpld_regs.TX0_7[0].TX0_7_TERMINATION
        cpld_regs.TX0_RF_8[0] = cpld_regs.TX0_RF_8[0].TX0_RF_8_RF_1
        cpld_regs.TX0_RF_9[0] = cpld_regs.TX0_RF_9[0].TX0_RF_9_RF_1
        cpld_regs.TX0_ANT_10[0] = cpld_regs.TX0_ANT_10[0].TX0_ANT_10_BYPASS_AMP
        cpld_regs.TX0_ANT_11[0] = cpld_regs.TX0_ANT_11[0].TX0_ANT_11_BYPASS_AMP
        cpld_regs.TX0_LO_13[0] = cpld_regs.TX0_LO_13[0].TX0_LO_13_INTERNAL
        cpld_regs.TX0_LO_14[0] = cpld_regs.TX0_LO_14[0].TX0_LO_14_INTERNAL
        # TX1 path control
        cpld_regs.TX1_IF2_1_2[0] = cpld_regs.TX1_IF2_1_2[0].TX1_IF2_1_2_FILTER_2
        cpld_regs.TX1_IF1_3[0] = cpld_regs.TX1_IF1_3[0].TX1_IF1_3_FILTER_0_3
        cpld_regs.TX1_IF1_4[0] = cpld_regs.TX1_IF1_4[0].TX1_IF1_4_TERMINATION
        cpld_regs.TX1_IF1_5[0] = cpld_regs.TX1_IF1_5[0].TX1_IF1_5_TERMINATION
        cpld_regs.TX1_IF1_6[0] = cpld_regs.TX1_IF1_6[0].TX1_IF1_6_FILTER_0_3
        cpld_regs.TX1_7[0] = cpld_regs.TX1_7[0].TX1_7_TERMINATION
        cpld_regs.TX1_RF_8[0] = cpld_regs.TX1_RF_8[0].TX1_RF_8_RF_1
        cpld_regs.TX1_RF_9[0] = cpld_regs.TX1_RF_9[0].TX1_RF_9_RF_1
        cpld_regs.TX1_ANT_10[0] = cpld_regs.TX1_ANT_10[0].TX1_ANT_10_BYPASS_AMP
        cpld_regs.TX1_ANT_11[0] = cpld_regs.TX1_ANT_11[0].TX1_ANT_11_BYPASS_AMP
        cpld_regs.TX1_LO_13[0] = cpld_regs.TX1_LO_13[0].TX1_LO_13_INTERNAL
        cpld_regs.TX1_LO_14[0] = cpld_regs.TX1_LO_14[0].TX1_LO_14_INTERNAL
        # RX0 path control
        cpld_regs.RX0_ANT_1[0] = cpld_regs.RX0_ANT_1[0].RX0_ANT_1_TERMINATION
        cpld_regs.RX0_2[0] = cpld_regs.RX0_2[0].RX0_2_LOWBAND
        cpld_regs.RX0_RF_3[0] = cpld_regs.RX0_RF_3[0].RX0_RF_3_RF_1
        cpld_regs.RX0_4[0] = cpld_regs.RX0_4[0].RX0_4_LOWBAND
        cpld_regs.RX0_IF1_5[0] = cpld_regs.RX0_IF1_5[0].RX0_IF1_5_FILTER_1
        cpld_regs.RX0_IF1_6[0] = cpld_regs.RX0_IF1_6[0].RX0_IF1_6_FILTER_1
        cpld_regs.RX0_LO_9[0] = cpld_regs.RX0_LO_9[0].RX0_LO_9_INTERNAL
        cpld_regs.RX0_LO_10[0] = cpld_regs.RX0_LO_10[0].RX0_LO_10_INTERNAL
        cpld_regs.RX0_RF_11[0] = cpld_regs.RX0_RF_11[0].RX0_RF_11_RF_3
        # RX1 path control
        cpld_regs.RX1_ANT_1[0] = cpld_regs.RX1_ANT_1[0].RX1_ANT_1_TERMINATION
        cpld_regs.RX1_2[0] = cpld_regs.RX1_2[0].RX1_2_LOWBAND
        cpld_regs.RX1_RF_3[0] = cpld_regs.RX1_RF_3[0].RX1_RF_3_RF_1
        cpld_regs.RX1_4[0] = cpld_regs.RX1_4[0].RX1_4_LOWBAND
        cpld_regs.RX1_IF1_5[0] = cpld_regs.RX1_IF1_5[0].RX1_IF1_5_FILTER_1
        cpld_regs.RX1_IF1_6[0] = cpld_regs.RX1_IF1_6[0].RX1_IF1_6_FILTER_1
        cpld_regs.RX1_LO_9[0] = cpld_regs.RX1_LO_9[0].RX1_LO_9_INTERNAL
        cpld_regs.RX1_LO_10[0] = cpld_regs.RX1_LO_10[0].RX1_LO_10_INTERNAL
        cpld_regs.RX1_RF_11[0] = cpld_regs.RX1_RF_11[0].RX1_RF_11_RF_3
        # TX DSA
        cpld_regs.TX0_DSA1[0] = 31
        cpld_regs.TX0_DSA2[0] = 31
        # RX DSA
        cpld_regs.RX0_DSA1[0] = 15
        cpld_regs.RX0_DSA2[0] = 15
        cpld_regs.RX0_DSA3_A[0] = 15
        cpld_regs.RX0_DSA3_B[0] = 15
        for addr in cpld_regs.get_changed_addrs():
            self.poke_cpld(addr, cpld_regs.get_reg(addr))
        # pylint: enable=too-many-statements

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
        self.log.debug("Setting CPLD back to safe defaults after UHD session.")
        self._cpld_set_safe_defaults()

    def tear_down(self):
        self.db_iface.tear_down()

    #########################################################################
    # API calls needed by the zbx_dboard driver
    #########################################################################
    def enable_iq_swap(self, enable, trx, channel):
        """
        Turn on IQ swapping in the RFDC
        """
        self.db_iface.enable_iq_swap(enable, trx, channel)

    def get_dboard_sample_rate(self):
        """
        Return the RFDC rate. This is usually a big number in the 3 GHz range.
        """
        return self.db_iface.get_sample_rate()

    def get_dboard_prc_rate(self):
        """
        Return the PRC rate. The CPLD and LOs are clocked with this.
        """
        return self.db_iface.get_prc_rate()

    def _has_compat_version(self, min_required_version):
        """
        Check for a minimum required version.
        """
        cpld_image_compat_revision = self.peek_cpld(self.regs.REVISION_addr)
        return cpld_image_compat_revision >= min_required_version

    def _get_cpld_git_hash(self):
        """
        Trace build of MB CPLD
        """
        git_hash_rb = self.peek_cpld(self.regs.GIT_HASH_addr)
        (git_hash, dirtiness_qualifier) = parse_encoded_git_hash(git_hash_rb)
        return "{:07x} ({})".format(git_hash, dirtiness_qualifier)

    def reset_clock(self, value):
        """
        Disable PLL reference clock to enable SPLL reconfiguration

        Puts the clock into reset if value is True, takes it out of reset
        otherwise.
        """
        if self._clock_enabled != bool(value):
            return
        addr = self.regs.get_addr("PLL_REF_CLOCK_ENABLE")
        enum = self.regs.PLL_REF_CLOCK_ENABLE_t
        if value:
            reg_value = enum.PLL_REF_CLOCK_ENABLE_DISABLE.value
        else:
            reg_value = enum.PLL_REF_CLOCK_ENABLE_ENABLE.value
        self.poke_cpld(addr, reg_value)
        self._clock_enabled = not bool(value)

    #########################################################################
    # LO SPI API
    #
    # We keep a LO peek/poke interface for debugging purposes.
    #########################################################################
    def _wait_for_spi_ready(self, timeout):
        """ Returns False if a timeout occurred. timeout is in ms """
        for _ in range(timeout):
            if (self.peek_cpld(self._spi_addr) >> self.regs.SPI_READY_shift) \
                    & self.regs.SPI_READY_mask:
                return True
            time.sleep(0.001)
        return False

    def _lo_spi_send_tx(self, lo_name, write, addr, data=None):
        """ Wait for SPI Ready and setup the TX data for a LO SPI transaction """
        if not self._wait_for_spi_ready(timeout=100):
            self.log.error('Timeout before LO SPI transaction waiting for SPI Ready')
            raise RuntimeError('Timeout before LO SPI transaction waiting for SPI Ready')
        lo_enum_name = 'LO_SELECT_' + lo_name.upper()
        assert hasattr(self.regs.LO_SELECT_t, lo_enum_name), \
            "Invalid LO name: {}".format(lo_name)
        self.regs.LO_SELECT = getattr(self.regs.LO_SELECT_t, lo_enum_name)
        if write:
            self.regs.READ_FLAG = self.regs.READ_FLAG_t.READ_FLAG_WRITE
        else:
            self.regs.READ_FLAG = self.regs.READ_FLAG_t.READ_FLAG_READ
        if data is not None:
            self.regs.DATA = data
        else:
            self.regs.DATA = 0
        self.regs.ADDRESS = addr
        self.regs.START_TRANSACTION = \
            self.regs.START_TRANSACTION_t.START_TRANSACTION_ENABLE
        self.poke_cpld(self._spi_addr, self.regs.get_reg(self._spi_addr))

    def _lo_spi_check_status(self, lo_name, addr, write=False):
        """ Wait for SPI Ready and check the success of the LO SPI transaction """
        # SPI Ready indicates that the previous transaction has completed
        # and the RX data is ready to be consumed
        if not write and not self._wait_for_spi_ready(timeout=100):
            self.log.error('Timeout after LO SPI transaction waiting for SPI Ready')
            raise RuntimeError('Timeout after LO SPI transaction waiting for SPI Ready')
        # If the address or CS are not the same as what we set, there
        # was interference during the SPI transaction
        lo_select = self.regs.LO_SELECT.name[len('LO_SELECT_'):]
        if self.regs.ADDRESS != addr or lo_select != lo_name.upper():
            self.log.error('SPI transaction to LO failed!')
            raise RuntimeError('SPI transaction to LO failed!')

    def _lo_spi_get_rx(self):
        """ Return RX data read from the LO SPI transaction """
        spi_reg = self.peek_cpld(self._spi_addr)
        return (spi_reg >> self.regs.DATA_shift) & self.regs.DATA_mask

    def peek_lo_spi(self, lo_name, addr):
        """ Perform a register read access to an LO via SPI """
        self._lo_spi_send_tx(lo_name=lo_name, write=False, addr=addr)
        self._lo_spi_check_status(lo_name, addr)
        return self._lo_spi_get_rx()

    def poke_lo_spi(self, lo_name, addr, val):
        """ Perform a register write access to an LO via SPI """
        self._lo_spi_send_tx(lo_name=lo_name, write=True, addr=addr, data=val)
        self._lo_spi_check_status(lo_name, addr, write=True)

    ###########################################################################
    # LEDs
    ###########################################################################
    def set_leds(self, channel, rx, trx_rx, trx_tx):
        """ Set the frontpanel LEDs """
        assert channel in (0, 1)

        self.regs.save_state()
        if channel == 0:
            # ensure to be in SW controlled mode
            self.regs.RF0_OPTION = self.regs.RF0_OPTION.RF0_OPTION_SW_DEFINED
            self.regs.SW_RF0_CONFIG = 0
            self.regs.RX0_RX_LED[0] = self.regs.RX0_RX_LED[0].RX0_RX_LED_ENABLE \
                if bool(rx) else self.regs.RX0_RX_LED[0].RX0_RX_LED_DISABLE
            self.regs.RX0_TRX_LED[0] = self.regs.RX0_TRX_LED[0].RX0_TRX_LED_ENABLE \
                if bool(trx_rx) else self.regs.RX0_TRX_LED[0].RX0_TRX_LED_DISABLE
            self.regs.TX0_TRX_LED[0] = self.regs.TX0_TRX_LED[0].TX0_TRX_LED_ENABLE \
                if bool(trx_tx) else self.regs.TX0_TRX_LED[0].TX0_TRX_LED_DISABLE
        else:
            # ensure to be in SW controlled mode
            self.regs.RF1_OPTION = self.regs.RF1_OPTION.RF1_OPTION_SW_DEFINED
            self.regs.SW_RF1_CONFIG = 0
            self.regs.RX1_RX_LED[0] = self.regs.RX1_RX_LED[0].RX1_RX_LED_ENABLE \
                if bool(rx) else self.regs.RX1_RX_LED[0].RX1_RX_LED_DISABLE
            self.regs.RX1_TRX_LED[0] = self.regs.RX1_TRX_LED[0].RX1_TRX_LED_ENABLE \
                if bool(trx_rx) else self.regs.RX1_TRX_LED[0].RX1_TRX_LED_DISABLE
            self.regs.TX1_TRX_LED[0] = self.regs.TX1_TRX_LED[0].TX1_TRX_LED_ENABLE \
                if bool(trx_tx) else self.regs.TX1_TRX_LED[0].TX1_TRX_LED_DISABLE

        for addr in self.regs.get_changed_addrs():
            self.poke_cpld(addr, self.regs.get_reg(addr))

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

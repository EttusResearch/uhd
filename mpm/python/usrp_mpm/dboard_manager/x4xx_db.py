#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Mixin class for daughterboard classes that live on a X4xx motherboard
"""

from usrp_mpm.mpmlog import get_logger
from usrp_mpm import tlv_eeprom
from usrp_mpm.sys_utils.udev import get_eeprom_paths_by_symbol
from usrp_mpm.rpc_utils import no_rpc

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

class X4xxDbMixin:
    """
    Mixin class for daughterboards that live in an X4xx motherboard.
    """
    # Make sure to populated this in the base class
    DBOARD_SUPPORTED_COMPAT_REVS = ()

    def __init__(self, product_name, slot_idx, **kwargs):
        self.log = get_logger(f"{product_name}-{slot_idx}")
        self.log.trace("Initializing %s daughterboard, slot index %d",
                       product_name, slot_idx)
        super().__init__(slot_idx, **kwargs)
        self.eeprom_symbol = f"db{slot_idx}_eeprom"
        self.product_name = product_name

        # Get MB interface
        if 'db_iface' not in kwargs:
            self.log.error("Required DB Iface was not provided!")
            raise RuntimeError("Required DB Iface was not provided!")
        self.db_iface = kwargs['db_iface']

        self.enable_base_power()

        assert self.DBOARD_SUPPORTED_COMPAT_REVS
        # This allows overriding the rev_compat from the EEPROM
        rev_compat = kwargs.get('rev_compat') if 'rev_compat' in kwargs else \
                self.get_eeprom()['rev_compat']
        self._assert_rev_compatibility(rev_compat)
        self._assert_mb_cpld_compatibility(self.db_iface.mboard.cpld_control)

    ###########################################################################
    # Init helpers
    ###########################################################################
    @no_rpc
    def get_eeprom(self):
        """
        Return the eeprom data.
        """
        path = get_eeprom_paths_by_symbol(self.eeprom_symbol)[self.eeprom_symbol]
        eeprom, _ = tlv_eeprom.read_eeprom(
            path, EepromTagMap.tagmap, EepromTagMap.magic, None)
        return eeprom

    def _assert_rev_compatibility(self, rev_compat):
        """
        Check the daughterboard hardware revision compatibility with this driver.

        Throws a RuntimeError() if this version of MPM does not recognize the
        hardware.

        Note: The CPLD image version is checked separately.
        """
        if rev_compat not in self.DBOARD_SUPPORTED_COMPAT_REVS:
            err = \
                f"This MPM version is not compatible with this {self.product_name}" \
                f"daughterboard. Found rev_compat value: 0x{rev_compat:02x}. " \
                "Please update your MPM version to support this daughterboard revision."
            self.log.error(err)
            raise RuntimeError(err)

    def _assert_mb_cpld_compatibility(self, mb_cpld_ctrl):
        """
        Verify that the MB CPLD is compatible with this daughterboard.

        Throws a RuntimeError() if that's not the case.
        """
        if not any(pid in self.pids for pid in mb_cpld_ctrl.COMPATIBLE_DB_PIDS):
            err = \
                f"This {self.product_name} daughterboard is not compatible with " \
                f"the motherboard CPLD image (CPLD signature: {mb_cpld_ctrl.SIGNATURE:X}). " \
                f"Please update the motherboard CPLD image."
            self.log.error(err)
            raise RuntimeError(err)

    @no_rpc
    def enable_base_power(self, enable=True):
        """
        Enables or disables power to the DB.
        """
        if enable:
            self.db_iface.enable_daughterboard(enable=True)
            if not self.db_iface.check_enable_daughterboard():
                self.db_iface.enable_daughterboard(enable=False)
                err_msg = f"{self.product_name} {self.slot_idx} power up failed"
                self.log.error(err_msg)
                raise RuntimeError(err_msg)
        else: # disable
            # Removing power from the CPLD will set all the output pins to open and the
            # supplies default to disabled on power up.
            self.db_iface.enable_daughterboard(enable=False)
            if self.db_iface.check_enable_daughterboard():
                err_msg = f"{self.product_name} {self.slot_idx} power down failed"
                self.log.error(err_msg)

    ###########################################################################
    # Clocking and RFDC interface
    ###########################################################################
    def get_rfdc_rate_sensor(self, _):
        """
        Return the RFDC rate (the ADC/DAC converter rate) of this daughterboard's
        converters as a sensor value.
        """
        return {
            'name': 'rfdc_rate',
            'type': 'REALNUM',
            'unit': 'Hz',
            'value': str(self.get_dboard_sample_rate()),
        }

    ###########################################################################
    # Clocking and RFDC interface
    ###########################################################################
    def enable_iq_swap(self, enable, trx, channel):
        """
        Turn on IQ swapping in the RFDC
        """
        self.db_iface.enable_iq_swap(enable, trx, channel)

    def get_dboard_sample_rate(self):
        """
        Return the RFDC rate. For ZBX, this is usually a big number in the
        3 GHz range.
        """
        return self.db_iface.get_sample_rate()

    def get_dboard_prc_rate(self):
        """
        Return the PRC rate. The CPLD and LOs are clocked with this.
        """
        return self.db_iface.get_prc_rate()

    def get_master_clock_rate(self):
        """
        Return the master clock rate. This is the rate that UHD cares about,
        in the 125-500 MHz range for X410/ZBX.
        """
        return self.db_iface.mboard.clk_mgr.get_master_clock_rate(self.slot_idx)

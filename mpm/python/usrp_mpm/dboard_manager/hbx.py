# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""HBX dboard implementation module."""
import time

from usrp_mpm.chips.ic_reg_maps import hbx_cpld_regs_t
from usrp_mpm.dboard_manager import DboardManagerBase
from usrp_mpm.dboard_manager.x4xx_db import X4xxDbMixin
from usrp_mpm.mpmutils import parse_encoded_git_hash, poll_with_timeout
from usrp_mpm.periph_manager.x4xx_periphs import get_temp_sensor


###############################################################################
# Main dboard control class
###############################################################################
class HBX(X4xxDbMixin, DboardManagerBase):
    """Holds all dboard specific information and methods of the HBX dboard."""

    #########################################################################
    # Overridables
    #
    # See DboardManagerBase for documentation on these fields
    #########################################################################
    pids = [0x4008]
    rx_sensor_callback_map = {
        "temperature": "get_rf_temp_sensor_average",
        "temperature_top": "get_rf_temp_sensor_top",
        "temperature_bottom": "get_rf_temp_sensor_bottom",
        "rfdc_rate": "get_rfdc_rate_sensor",
    }
    tx_sensor_callback_map = {
        "temperature": "get_rf_temp_sensor_average",
        "temperature_top": "get_rf_temp_sensor_top",
        "temperature_bottom": "get_rf_temp_sensor_bottom",
        "rfdc_rate": "get_rfdc_rate_sensor",
    }
    # HBX depends on several RF core implementations which each have
    # compat versions.
    updateable_components = {
        "fpga": {
            "compatibility": {
                "rf_core_1000m": {
                    "current": (2, 0),
                    "oldest": (2, 0),
                },
            }
        },
    }
    ### End of overridables #################################################

    ### Daughterboard driver/hardware compatibility value
    # The HBX has a field in its EEPROM which stores a rev_compat value. This
    # tells us which other revisions of the HBX this revision is compatible with.
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
    DBOARD_SUPPORTED_COMPAT_REVS = (0x2,)

    # CPLD compatibility revision
    # Change this revision only on breaking changes.
    REQ_OLDEST_COMPAT_REV = 0x25102716
    REQ_COMPAT_REV = 0x25102718

    WAIT_PERIOD_S = 0.1
    TIMEOUT_MS = 1000
    POLLING_TIME_MS = 1

    #########################################################################
    # MPM Initialization
    #########################################################################
    def __init__(self, slot_idx, **kwargs):
        """Initialize the HBX dboard."""
        super().__init__("HBX", slot_idx, **kwargs)
        # Initialize daughterboard CPLD control
        self.poke_cpld = self.db_iface.poke_db_cpld
        self.peek_cpld = self.db_iface.peek_db_cpld
        self.regs = hbx_cpld_regs_t()
        self._check_compat_version()
        self.log.debug("HBX CPLD build git hash: %s", self._get_cpld_git_hash())
        self.hbx_power_up()
        self._cpld_set_safe_defaults()
        self.commit()

    #########################################################################
    # DB-CPLD Interfacing
    #########################################################################
    def _check_compat_version(self):
        """Check compatibility of DB CPLD image and SW regmap."""
        self.update_reg("BOARD_ID")
        self.update_reg("REVISION_REG")
        self.update_reg("OLDEST_REVISION_REG")
        # Bring regmap state in sync with read-back values.
        self.regs.save_state()
        if self.regs.OLDEST_REVISION_REG < self.REQ_OLDEST_COMPAT_REV:
            err_msg = (
                f"DB CPLD oldest compatible revision 0x{self.regs.OLDEST_REVISION_REG:x}"
                f" is out of date, the required revision is 0x{self.REQ_OLDEST_COMPAT_REV:x}. "
                f"Update your CPLD image."
            )
            self.log.error(err_msg)
            raise RuntimeError(err_msg)
        if self.regs.OLDEST_REVISION_REG > self.REQ_OLDEST_COMPAT_REV:
            err_msg = (
                f"DB CPLD oldest compatible revision 0x{self.regs.OLDEST_REVISION_REG:x}"
                f" is newer than the expected revision 0x{self.REQ_OLDEST_COMPAT_REV:x}."
                " Downgrade your CPLD image or update MPM."
            )
            self.log.error(err_msg)
            raise RuntimeError(err_msg)

        if not self.has_compat_version(self.REQ_COMPAT_REV):
            err_msg = (
                f"HBX DB CPLD revision 0x{self.regs.REVISION_REG:x} is too old. "
                f"Update your CPLD image to at least 0x{self.REQ_COMPAT_REV:08x}."
            )
            self.log.error(err_msg)
            raise RuntimeError(err_msg)

    def has_compat_version(self, min_required_version):
        """Check for a minimum required version."""
        return self.regs.REVISION_REG >= min_required_version

    def _cpld_set_safe_defaults(self):
        """Set the CPLD into a safe state."""
        # Set the TRX antenna switch to termination to prevent connected
        # power to get into the RX path.
        self.regs.RX_SW_TRX[0] = self.regs.RX_SW_TRX_t.RX_SW_TRX_TERMINATION

    def _get_cpld_git_hash(self):
        """Trace build of MB CPLD."""
        self.update_reg("GIT_HASH")
        self.regs.save_state()
        git_hash_rb = self.regs.GIT_HASH
        (git_hash, dirtiness_qualifier) = parse_encoded_git_hash(git_hash_rb)
        return f"{git_hash:07x} ({dirtiness_qualifier})"

    #########################################################################
    # UHD (De-)Initialization
    #########################################################################
    def init(self, args):
        """Execute necessary init dance to bring up dboard.

        This happens when a UHD session starts.
        """
        self.log.debug(f"init() called with args `{' '.join([f'{x}={args[x]}' for x in args])}'")
        return True

    def deinit(self):
        """De-initialize after UHD session completes."""
        self.log.debug("Setting board back to safe defaults after UHD session.")

    def tear_down(self):
        """Tear down method."""
        self.db_iface.tear_down()

    #########################################################################
    # API calls needed by the clock manager
    #########################################################################
    def reset_clock(self, reset):
        """Disable PLL reference clock (PRC).

        Disables the PLL reference clock to enable SPLL reconfiguration. Puts
        the clock into reset if value is True, takes it out of reset otherwise.
        """
        self.regs.PLL_REF_CLOCK_ENABLE = 0 if reset else 1
        self.commit()

    ###########################################################################
    # Internal helpers
    ###########################################################################
    def commit(self, save_all=False):
        """Write all rw regs that have a changed state or all rw regs if save_all is True."""
        addrs = self.regs.get_all_addrs() if save_all else self.regs.get_changed_addrs()
        for addr in addrs:
            self.poke_cpld(addr, self.regs.get_reg(addr))
        self.regs.save_state()

    def update_reg(self, reg_name, idx=0):
        """Update the saved state of a register from the hardware."""
        addr = self.regs.get_addr(reg_name)
        reg_val = self.peek_cpld(addr + 4 * idx)
        self.regs.set_reg(addr + 4 * idx, reg_val)

    ###########################################################################
    # Power-up steps
    ###########################################################################

    def hbx_power_up(self):
        """Power up sequence for HBX dboard."""
        # Check if anything is already powered on by reading the enable register.
        # This way we avoid lowering bits that are already high.
        self.update_reg("N1d7V_EN")
        self.enable_rf_dsa_sw()
        self.enable_smps()
        self.enable_refclk_ldo()
        # Enable PLL Ref clock
        # At this point we assume it is correctly configured
        self.reset_clock(False)
        time.sleep(self.WAIT_PERIOD_S)
        self.enable_admv()
        self.enable_rf_and_bb()
        self.enable_lo()
        time.sleep(self.WAIT_PERIOD_S)
        # Enable all CPLD IOs when power is up
        self.regs.IO_ENABLE = 1
        self.commit()
        time.sleep(self.WAIT_PERIOD_S)

    def enable_rf_dsa_sw(self):
        """Enable the LDOs to supply ±3.3V for the RF DSAs and SWs (TX and RX).

        This needs to be the first step to avoid a latch-up situation on the ADRF5700 DSA
        """
        # Set the P4d0V_EN bit to high/1
        self.regs.P4d0V_EN = 1
        # write this into the CPLD
        self.commit()

        # function to be used for polling
        def check_p4d0v_pg_bit_high():
            self.update_reg("P4d0V_PG")
            return self.regs.P4d0V_PG

        # verify that the P4d0V_PG bit is high/1
        if not poll_with_timeout(
            check_p4d0v_pg_bit_high, timeout_ms=self.TIMEOUT_MS, interval_ms=self.POLLING_TIME_MS
        ):
            raise RuntimeError("P4d0V_PG bit is not high within the timeout period")

        # Set the P3d3V_RF_EN bit to high/1
        self.regs.P3d3V_RF_EN = 1
        self.regs.N3d3V_EN = 1
        # write this into the CPLD
        self.commit()

        def check_n3d3v_pg_bits_high():
            self.update_reg("N3d3V_PG_TX")
            self.update_reg("N3d3V_PG_RX")
            return self.regs.N3d3V_PG_TX and self.regs.N3d3V_PG_RX

        # verify that the N3d3V_PG_TX and N3d3V_PG_RX bits are high/1
        if not poll_with_timeout(
            check_n3d3v_pg_bits_high, timeout_ms=self.TIMEOUT_MS, interval_ms=self.POLLING_TIME_MS
        ):
            raise RuntimeError(
                "N3d3V_PG_TX and N3d3V_PG_RX bits are not high within the timeout period"
            )

    def enable_smps(self):
        """Turn on all SMPS +8.7V, +5.6V and -3.2V."""
        self.regs.P8d7V_SMPS_EN = 1
        self.commit()

        def check_p8d7v_smps_pg_bit_high():
            self.update_reg("P8d7V_SMPS_PG")
            return self.regs.P8d7V_SMPS_PG

        if not poll_with_timeout(
            check_p8d7v_smps_pg_bit_high,
            timeout_ms=self.TIMEOUT_MS,
            interval_ms=self.POLLING_TIME_MS,
        ):
            raise RuntimeError("P8d7V_SMPS_PG bit is not high within the timeout period")

        self.regs.P5d6V_EN = 1
        self.commit()

        def check_p5d6v_pg_bit_high():
            self.update_reg("P5d6V_PG")
            return self.regs.P5d6V_PG

        if not poll_with_timeout(
            check_p5d6v_pg_bit_high, timeout_ms=self.TIMEOUT_MS, interval_ms=self.POLLING_TIME_MS
        ):
            raise RuntimeError("P5d6V_PG bit is not high within the timeout period")

        self.regs.N3d2V_EN = 1
        self.commit()

        def check_n3d2v_pg_bit_high():
            self.update_reg("N3d2V_PG")
            return self.regs.N3d2V_PG

        if not poll_with_timeout(
            check_n3d2v_pg_bit_high, timeout_ms=self.TIMEOUT_MS, interval_ms=self.POLLING_TIME_MS
        ):
            raise RuntimeError("N3d2V_PG bit is not high within the timeout period")

    def enable_refclk_ldo(self):
        """Enable the reference clock LDO."""
        self.regs.P3d3V_REF_CLK_EN = 1
        self.commit()

    def enable_admv(self):
        """Enable LDOs which will turn on ADMV1320 and ADMV1420."""
        # Enable ADMV1420
        self.regs.P3d3V_ADMV1420_EN = 1
        self.commit()

        def check_p3d3v_admv1420_pg_bit_high():
            self.update_reg("P3d3V_ADMV1420_PG")
            return self.regs.P3d3V_ADMV1420_PG

        if not poll_with_timeout(
            check_p3d3v_admv1420_pg_bit_high,
            timeout_ms=self.TIMEOUT_MS,
            interval_ms=self.POLLING_TIME_MS,
        ):
            raise RuntimeError("P3d3V_ADMV1420_PG bit is not high within the timeout period")

        # Enable ADMV1320
        self.regs.P3d3V_ADMV1320_EN = 1
        self.commit()

        def check_p3d3v_admv1320_pg_bit_high():
            self.update_reg("P3d3V_ADMV1320_PG")
            return self.regs.P3d3V_ADMV1320_PG

        if not poll_with_timeout(
            check_p3d3v_admv1320_pg_bit_high,
            timeout_ms=self.TIMEOUT_MS,
            interval_ms=self.POLLING_TIME_MS,
        ):
            raise RuntimeError("P3d3V_ADMV1320_PG bit is not high within the timeout period")

    def enable_rf_and_bb(self):
        """Enable baseband and RF rails."""
        self.regs.P3d3V_RX_BB_AMP2_EN = 1
        self.regs.P3d3V_RX_BB_AMP1_EN = 1
        self.regs.P2d5V_TX_BB_AMP_EN = 1
        self.regs.N1d7V_EN = 1
        self.regs.P5V_RX_RF_EN = 1
        self.regs.P5V_TX_RF_EN = 1
        self.regs.P8d0V_EN = 1
        self.commit()

        # Making a seperate commit to avoid backdriving the P5V_TX_RF rail
        self.regs.N2d5V_EN = 1
        self.commit()

        def check_n1d7v_pg_bit_high():
            self.update_reg("N1d7V_PG")
            return self.regs.N1d7V_PG

        if not poll_with_timeout(
            check_n1d7v_pg_bit_high, timeout_ms=self.TIMEOUT_MS, interval_ms=self.POLLING_TIME_MS
        ):
            raise RuntimeError("N1d7V_PG bit is not high within the timeout period")

        def check_n2d5v_pg_bit_high():
            self.update_reg("N2d5V_PG")
            return self.regs.N2d5V_PG

        if not poll_with_timeout(
            check_n2d5v_pg_bit_high, timeout_ms=self.TIMEOUT_MS, interval_ms=self.POLLING_TIME_MS
        ):
            raise RuntimeError("N2d5V_PG bit is not high within the timeout period")

        def check_p8d0v_pg_bit_high():
            self.update_reg("P8d0V_PG")
            return self.regs.P8d0V_PG

        if not poll_with_timeout(
            check_p8d0v_pg_bit_high, timeout_ms=self.TIMEOUT_MS, interval_ms=self.POLLING_TIME_MS
        ):
            raise RuntimeError("P8d0V_PG bit is not high within the timeout period")
        time.sleep(self.WAIT_PERIOD_S)

    def enable_lo(self):
        """Enable LO voltage rails (RX and TX)."""
        self.regs.P3d3V_TX1_LO1_EN = 1
        self.regs.P3d3V_RX1_LO1_EN = 1
        self.regs.P5V_RX_LO_EN = 1
        self.regs.P5V_TX_LO_EN = 1
        self.commit()

    ###########################################################################
    # Sensors
    ###########################################################################
    def get_rf_temp_sensor_average(self, _):
        """Return the average RF temperature value of the top and bottom sensors."""
        self.log.trace("Reading average RF daughterboard temperature.")
        sensor_names = [
            f"TMP112 DB{self.slot_idx} Top",
            f"TMP112 DB{self.slot_idx} Bottom",
        ]
        return get_temp_sensor(sensor_names, log=self.log)

    def get_rf_temp_sensor_top(self, _):
        """Return the RF temperature value of the sensor on the top of the PCB."""
        self.log.trace("Reading RF daughterboard temperature value from the top sensor.")
        sensor_names = [
            f"TMP112 DB{self.slot_idx} Top",
        ]
        return get_temp_sensor(sensor_names, log=self.log)

    def get_rf_temp_sensor_bottom(self, _):
        """Return the RF temperature value of the sensor on the bottom of the PCB."""
        self.log.trace("Reading RF daughterboard temperature value from the bottom sensor.")
        sensor_names = [
            f"TMP112 DB{self.slot_idx} Bottom",
        ]
        return get_temp_sensor(sensor_names, log=self.log)

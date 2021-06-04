#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Drivers for the X410 clocking aux board.
"""

import subprocess
from usrp_mpm import lib  # Pulls in everything from C++-land
from usrp_mpm import tlv_eeprom
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.sys_utils.gpio import Gpio
from usrp_mpm.sys_utils.udev import dt_symbol_get_spidev
from usrp_mpm.sys_utils.udev import get_eeprom_paths_by_symbol
from usrp_mpm.sys_utils import i2c_dev
from usrp_mpm.chips import LMK05318

X400_CLKAUX_DEFAULT_TUNING_WORD = 0x200 # 1.65V which would be a DAC value of 512
X400_CLKAUX_DEFAULT_REVISION = 0x1
X400_CLKAUX_I2C_LABEL = 'clkaux_i2c'
X400_CLKAUX_SPI_LABEL = 'clkaux_spi'
X400_CLKAUX_GPSDO_PID = 0x4004
X400_CLKAUX_NOGPSDO_PID = 0x4005


def _check_i2c_bus():
    """
    Assert that the I2C connection to the clocking board is available in the
    device tree.
    """
    i2c_bus = i2c_dev.dt_symbol_get_i2c_bus(X400_CLKAUX_I2C_LABEL)
    if i2c_bus is None:
        raise RuntimeError("ClockingAuxBrdControl I2C bus not found")

def _check_spi_bus():
    """
    Assert that the SPI connection to the clocking board is available in the
    device tree.
    """
    spi_node = dt_symbol_get_spidev(X400_CLKAUX_SPI_LABEL)
    if spi_node is None:
        raise RuntimeError("ClockingAuxBrdControl SPI bus not found")


# pylint: disable=too-few-public-methods
class ClkAuxTagMap:
    """
    x4xx main tagmap is in the main class. Only the subset relevant to the
    clocking aux board is included below.
    """
    magic = 0x55535250
    tagmap = {
        # 0x23: usrp_eeprom_clkaux_tuning_word
        0x23: tlv_eeprom.NamedStruct('< H',
                                     ['tuning_word']),
        # 0x10: usrp_eeprom_board_info
        0x10: tlv_eeprom.NamedStruct('< H H H 7s 1x',
                                     ['pid', 'rev', 'rev_compat', 'serial']),
    }

# We are encapsulating a complicated piece of hardware here, so let's live with
# the fact that there will be many things hanging off of it.
# pylint: disable=too-many-public-methods
# pylint: disable=too-many-instance-attributes
class ClockingAuxBrdControl:
    """
    Control interface for the Clocking Aux Board over I2C and SPI
    """
    SOURCE_INTERNAL = "internal"
    SOURCE_EXTERNAL = "external"
    SOURCE_GPSDO = "gpsdo"
    SOURCE_NSYNC = "nsync"

    SOURCE_NSYNC_LMK_PRI_FABRIC_CLK = "fabric_clk"
    SOURCE_NSYNC_LMK_PRI_GTY_RCV_CLK = "gty_rcv_clk"

    NSYNC_PRI_REF = "pri_ref"
    NSYNC_SEC_REF = "sec_ref"

    DIRECTION_INPUT = "input"
    DIRECTION_OUTPUT = "output"

    VALID_CLK_EXPORTS = (SOURCE_INTERNAL, SOURCE_GPSDO, SOURCE_NSYNC)
    VALID_NSYNC_LMK_PRI_REF_SOURCES = (SOURCE_NSYNC_LMK_PRI_FABRIC_CLK,
                                       SOURCE_NSYNC_LMK_PRI_GTY_RCV_CLK)

    def __init__(self, default_source=None, parent_log=None):
        self.log = \
            parent_log.getChild(self.__class__.__name__) if parent_log is not None \
            else get_logger(self.__class__.__name__)
        _check_i2c_bus()
        self._revision = self._get_eeprom_field('rev', X400_CLKAUX_DEFAULT_REVISION)
        self._pid = self._get_eeprom_field('pid', X400_CLKAUX_NOGPSDO_PID)
        self._nsync_support = self._revision >= 2
        self._gps_support = self._pid == X400_CLKAUX_GPSDO_PID
        default_source = default_source or ClockingAuxBrdControl.SOURCE_INTERNAL

        # Some GPIO lines are named differently in the overlays for rev 1 and
        # rev 2 and some perform different functions in rev 1 and 2 even if
        # named similarly.

        # GPIO common to rev 1 and 2
        self._3v3_power_good = Gpio("CLKAUX_3V3_CLK_PG", Gpio.INPUT)
        self._pps_term = Gpio("CLKAUX_PPS_TERM", Gpio.OUTPUT, 1)
        self._trig_oe_n = Gpio("CLKAUX_TRIG_OEn", Gpio.OUTPUT, 1)
        self._trig_dir = Gpio("CLKAUX_TRIG_DIR", Gpio.OUTPUT, 0)
        self._ref_lck_led = Gpio("CLKAUX_REF_LCK", Gpio.OUTPUT, 0)

        if self._revision == 1:
            self._ref_clk_sel_usr = Gpio("CLKAUX_REF_CLK_SEL_USR", Gpio.OUTPUT, 1)
            self._mbrefclk_bias = Gpio("CLKAUX_MBRefCLK_En", Gpio.OUTPUT, 0)
            self._exportclk_bias = Gpio("CLKAUX_ExportClk_En", Gpio.OUTPUT, 0)
        elif self._revision >= 2:
            self._ref_clk_sel_usr = Gpio("CLKAUX_UsrRefSel", Gpio.OUTPUT, 0)
            self._mbrefclk_bias = Gpio("CLKAUX_MBRefCLK_Bias", Gpio.OUTPUT, 0)
            self._exportclk_bias = Gpio("CLKAUX_ExportClk_Bias", Gpio.OUTPUT, 0)
            self._tcxo_en = Gpio("CLKAUX_TCXO_EN", Gpio.OUTPUT, 0)
            self._exportclk_en = Gpio("CLKAUX_EXP_CLK_EN", Gpio.OUTPUT, 0)
            self._nsync_refsel = Gpio("CLKAUX_NSYNC_REFSEL", Gpio.OUTPUT, 0)
            self._nsync_power_ctrl = Gpio("CLKAUX_NSYNC_PDN", Gpio.OUTPUT, 0)
            self._ref_clk_select_net = Gpio("CLKAUX_REF_CLK_SEL_NET", Gpio.OUTPUT, 0)
            self._nsync_gpio0 = Gpio("CLKAUX_NSYNC_GPIO0", Gpio.INPUT)
            self._nsync_status1 = Gpio("CLKAUX_NSYNC_STATUS1", Gpio.INPUT)
            self._nsync_status0 = Gpio("CLKAUX_NSYNC_STATUS0", Gpio.INPUT)
            self._fpga_clk_gty_fabric_sel = Gpio("CLKAUX_FPGA_CLK_SEL", Gpio.OUTPUT, 0)
            self._lmk05318_bias = Gpio("CLKAUX_05318Ref_Bias", Gpio.OUTPUT, 0)

        if self._gps_support:
            self._gps_phase_lock = Gpio("CLKAUX_GPS_PHASELOCK", Gpio.INPUT)
            self._gps_warmup = Gpio("CLKAUX_GPS_WARMUP", Gpio.INPUT)
            self._gps_survey = Gpio("CLKAUX_GPS_SURVEY", Gpio.INPUT)
            self._gps_lock = Gpio("CLKAUX_GPS_LOCK", Gpio.INPUT)
            self._gps_alarm = Gpio("CLKAUX_GPS_ALARM", Gpio.INPUT)
            self._gps_rst_n = Gpio("CLKAUX_GPS_RSTn", Gpio.OUTPUT, 0)
            if self._revision == 1:
                self._gps_initsrv_n = Gpio("CLKAUX_GPS_INITSRVn", Gpio.OUTPUT, 1)
            elif self._revision >= 2:
                self._gps_initsrv_n = Gpio("CLKAUX_GPS_INITSURVn", Gpio.OUTPUT, 0)

        if self._revision >= 2:
            _check_spi_bus()
            # Create SPI interface to the LMK05318 registers
            nsync_spi_node = dt_symbol_get_spidev(X400_CLKAUX_SPI_LABEL)
            nsync_lmk_regs_iface = lib.spi.make_spidev_regs_iface(
                nsync_spi_node,
                1000000,    # Speed (Hz)
                0x0,        # SPI mode
                8,          # Addr shift
                0,          # Data shift
                1<<23,      # Read flag
                0,          # Write flag
            )
            self._nsync_pll = LMK05318(nsync_lmk_regs_iface, self.log)

        self.set_source(default_source)
        self.set_trig(False, self.DIRECTION_OUTPUT)
        self._init_dac()

    def _init_dac(self):
        """
        Initializes i2c bus to communicate with the DAC and configures the
        tuning word for both voltage outputs
        """
        dac_i2c_bus = i2c_dev.dt_symbol_get_i2c_bus(X400_CLKAUX_I2C_LABEL)
        self._dac_i2c_iface = lib.i2c.make_i2cdev(
            dac_i2c_bus,
            0xC,    # addr
            False,  # ten_bit_addr
            100
        )
        tuning_word = self._get_eeprom_field('tuning_word', X400_CLKAUX_DEFAULT_TUNING_WORD)
        self.config_dac(tuning_word, 0)
        self.config_dac(tuning_word, 1)

    def _get_eeprom_field(self, field_name, default_val):
        """
        Return the value of the requested eeprom field.
        """
        eeprom_paths = get_eeprom_paths_by_symbol("clkaux_eeprom")
        path = eeprom_paths['clkaux_eeprom']
        val = default_val
        try:
            eeprom, _ = tlv_eeprom.read_eeprom(
                path, ClkAuxTagMap.tagmap, ClkAuxTagMap.magic, None)
            val = eeprom.get(field_name, default_val)
        except TypeError as err:
            self.log.warning(f"Error reading eeprom; will use defaults. ({err})")
        return val

    def store_tuning_word(self, tuning_word):
        """ Store the dac tuning word in the ID EEPROM"""
        cmd = ["eeprom-update",
               "clkaux",
               "--clkaux_tuning_word",
               str(tuning_word)]
        try:
            subprocess.call(cmd)
        except subprocess.CalledProcessError as ex:
            self.log.warning("Failed to write to clkaux EEPROM: %s", str(ex))

    def config_dac(self, tuning_word, out_select):
        """Configure tuning word on the the selected DAC output through i2c"""
        high, low = divmod(tuning_word << 6, 0x100)
        command = 0x38 if out_select else 0x31
        tx_cmd = [command, high, low]
        # Send command to write tuning word and update output
        self._dac_i2c_iface.transfer(tx_cmd, 0, True)

    def read_dac(self, out_select):
        """Read the tuning word on the selected DAC output through i2c"""
        command = 0x8 if out_select else 0x1
        rx_buffer = self._dac_i2c_iface.transfer([command], 2, True)
        val = ((rx_buffer[0] << 8) | rx_buffer[1]) >> 6
        return val

    def is_nsync_supported(self):
        """Return True if nsync clock source is supported by hardware"""
        return self._nsync_support

    def _check_nsync_supported(self):
        """
        Assert nsync clock source is supported by hardware or throw otherwise
        """
        if not self.is_nsync_supported():
            raise RuntimeError("NSYNC related features are not supported!")

    def is_gps_supported(self):
        """Return True if GPS clock source is supported by hardware"""
        return self._gps_support

    def is_gps_enabled(self):
        """
        Return True if the GPS is currently enabled (i.e., not held in reset).
        """
        return bool(self._gps_rst_n.get())

    def _assert_gps_supported(self):
        """ Throw a RuntimeError if GPS is not supported on this board. """
        if not self.is_gps_supported():
            raise RuntimeError("GPS related features are not supported!")

    def _init_nsync_lmk(self):
        """Initialize the LMK05318 network sync IC"""
        self._check_nsync_supported()
        self.set_nsync_lmk_power_en(1)
        self._nsync_pll.soft_reset(True)
        self._nsync_pll.soft_reset(False)
        if not self._nsync_pll.is_chip_id_valid():
            raise RuntimeError("ClockingAuxBrdControl Unable to locate LMK05318!")

    def set_source(self, clock_source, time_source=None):
        """
        Select the clock and time source on the clock auxbrd.

        Notes:
        - The clocking aux board has a PPS termination which must be disabled
          when the PPS input is active. Note that we can only have an external
          time source when the clock source is also external. We enable it in
          all other cases.
        - The pin that selects the reference clock (UsrRefSel or ref_clk_sel_usr)
          selects *both* the clock reference and the time reference (external
          or GPSDO). So if clock source is set to external, but time source to
          internal, then we are still connecting the PPS In SMA to the FPGA.
        - This function will disable clock export if we switch to external clock.
        - The time source is irrelevant unless the clock source is EXTERNAL, so
          we allow not specifying it for the other cases.
        - We actually put the GPS chip into reset when it's unused so we don't
          collect spurs from there. However, this means the GPS will need to
          warm up when being selected. Selecting the GPS as a source at runtime
          is possible, it just won't be available until it's warmed up.
        """
        if clock_source not in self.VALID_CLK_EXPORTS:
            self.export_clock(False)
        if clock_source == self.SOURCE_INTERNAL:
            self._set_gps_rstn(0)
            self._set_ref_clk_sel_usr(0)
            # If we are using an internal PPS, then we terminate the connector
            use_pps_term = time_source == self.SOURCE_INTERNAL
            self._pps_term.set(use_pps_term)
            self._mbrefclk_bias.set(1)
            if self._revision >= 2:
                self.set_nsync_lmk_power_en(0)
                self._lmk05318_bias.set(0)
                self._ref_clk_select_net.set(0)
        elif clock_source == self.SOURCE_EXTERNAL:
            self._set_gps_rstn(0)
            self._set_ref_clk_sel_usr(1)
            self._exportclk_bias.set(0)
            self._pps_term.set(1)
            self._mbrefclk_bias.set(1)
            if self._revision >= 2:
                self.set_nsync_lmk_power_en(0)
                self._lmk05318_bias.set(0)
                self._ref_clk_select_net.set(0)
        elif clock_source == self.SOURCE_GPSDO:
            self._assert_gps_supported()
            self._set_gps_rstn(1)
            self._set_ref_clk_sel_usr(0)
            self._pps_term.set(1)
            self._mbrefclk_bias.set(1)
            if self._revision >= 2:
                self.set_nsync_lmk_power_en(0)
                self._lmk05318_bias.set(0)
                self._ref_clk_select_net.set(0)
        elif clock_source == self.SOURCE_NSYNC:
            self._check_nsync_supported()
            self._set_gps_rstn(0)
            self._set_ref_clk_sel_usr(0)
            self._tcxo_en.set(1)
            self._nsync_refsel.set(1)
            self._mbrefclk_bias.set(0)
            self._lmk05318_bias.set(1)
            self._pps_term.set(1)
            self._ref_clk_select_net.set(1)
            self._init_nsync_lmk()
        else:
            raise RuntimeError('Invalid clock source {}'.format(clock_source))
        self._source = clock_source
        self.log.trace("set clock source to: {}".format(self._source))


    def export_clock(self, enable=True):
        """Export clock source to RefOut"""
        clock_source = self.get_clock_source()
        if not enable:
            self._exportclk_bias.set(0)
            self._pps_term.set(1)
            if self._revision >= 2:
                self._exportclk_en.set(0)
        elif clock_source in self.VALID_CLK_EXPORTS:
            self._exportclk_bias.set(1)
            self._pps_term.set(0)
            if self._revision >= 2:
                self._exportclk_en.set(1)
        else:
            raise RuntimeError('Invalid source to export: {}'.format(clock_source))

    def set_trig(self, enable, direction=None):
        """Enable/disable the Trig IO out"""
        if direction is None:
            direction = ClockingAuxBrdControl.DIRECTION_OUTPUT

        if enable:
            self._trig_oe_n.set(0)
        else:
            self._trig_oe_n.set(1)

        if direction == self.DIRECTION_INPUT:
            self._trig_dir.set(0)
        elif direction == self.DIRECTION_OUTPUT:
            self._trig_dir.set(1)
        else:
            raise RuntimeError(
                'Invalid direction {}, valid options are {} and {}'
                .format(direction, self.DIRECTION_INPUT, self.DIRECTION_OUTPUT))

    def get_clock_source(self):
        """Returns the clock source"""
        return self._source

    def get_gps_phase_lock(self):
        """Returns true if the GPS Phase is locked, and false if it is not"""
        return self._gps_phase_lock.get()

    def get_gps_warmup(self):
        """Returns true if the GPS is warming up"""
        return self._gps_warmup.get()

    def get_gps_survey(self):
        """Returns whether or not an auto survey is in progress"""
        return self._gps_survey.get()

    def get_gps_lock(self):
        """Returns whether or not the GPS has a lock"""
        return self._gps_lock.get()

    def get_gps_alarm(self):
        """Returns true if the GPS detects a hardware fault or software alarm"""
        return self._gps_alarm.get()

    def get_3v3_pg(self):
        """Returns true if the 3.3V rail is good, false otherwise"""
        return self._3v3_power_good.get()

    def _set_ref_clk_sel_usr(self, value):
        """Sets REF_CLK_SEL_USR to value"""
        value = int(value)
        assert value in (0, 1)
        if value == 1:
            #Never set REF_CLK_SEL_USR and GPS_RSTn high at the same time, hardware can be damaged
            self._set_gps_rstn(0)
        self._ref_clk_sel_usr.set(value)

    def _set_gps_rstn(self, value):
        """
        Sets GPS_RSTn to value

        If value == 0, then the GPS is held in reset and is not usable.
        """
        value = int(value)
        assert value in (0, 1)
        if value == 1:
            # Never set REF_CLK_SEL_USR and GPS_RSTn high at the same time,
            # hardware can be damaged
            self._set_ref_clk_sel_usr(0)
        if self._gps_support:
            self._gps_rst_n.set(value)
        elif value == 1:
            raise RuntimeError("No GPS, so can't bring it out of reset")

    def get_nsync_chip_id_valid(self):
        """Returns whether the chip ID of the LMK is valid"""
        return self._nsync_pll.is_chip_id_valid()

    def set_nsync_soft_reset(self, value=True):
        """Soft reset LMK chip"""
        return self._nsync_pll.soft_reset(value)

    def get_nsync_status0(self):
        """Returns value of STATUS0 pin on LMK05318 NSYNC IC"""
        self._check_nsync_supported()
        return self._nsync_status0.get()

    def get_nsync_status1(self):
        """Returns value of STATUS1 pin on LMK05318 NSYNC IC"""
        self._check_nsync_supported()
        return self._nsync_status1.get()

    def set_nsync_pri_ref_source(self, source):
        """Sets LMK05318 PRIMREF (primary reference) to specified source"""
        self._check_nsync_supported()

        if source not in self.VALID_NSYNC_LMK_PRI_REF_SOURCES:
            raise RuntimeError(
                "Invalid primary reference clock source for LMK05318 NSYNC IC")

        self.config_dpll(source)
        if source == self.SOURCE_NSYNC_LMK_PRI_FABRIC_CLK:
            self._fpga_clk_gty_fabric_sel.set(1)
        else:
            self._fpga_clk_gty_fabric_sel.set(0)

    def set_nsync_ref_select(self, source):
        """Sets LMK05318 REFSEL to PRIREF or SECREF"""
        self._check_nsync_supported()
        if source == self.NSYNC_PRI_REF:
            self._nsync_refsel.set(0)
        elif source == self.NSYNC_SEC_REF:
            self._nsync_refsel.set(1)
        else:
            raise RuntimeError(
                "Invalid setting for LMK05318 NSYNC REFSEL")

    def set_nsync_tcxo_en(self, enable):
        """
        Enables/Disables the 10 MHz TCXO chip output; this signal serves as the
        oscillator input to the LMK05318 NSYNC IC.
        """
        self._check_nsync_supported()
        if enable:
            self._tcxo_en.set(1)
        else:
            self._tcxo_en.set(0)

    def set_nsync_lmk_power_en(self, enable):
        """Turn on/off the LMK05318 IC using the PDN pin"""
        self._check_nsync_supported()
        if enable:
            self.log.trace("enable LMK05318 power")
            self._nsync_power_ctrl.set(1)
        else:
            self.log.trace("disable LMK05318 power")
            self._nsync_power_ctrl.set(0)

    def write_nsync_lmk_cfg_regs_to_eeprom(self, method):
        """program the current LMK config to LMK eeprom"""
        self._check_nsync_supported()
        self.log.trace("LMK05318: store cfg in eeprom")
        self._nsync_pll.write_cfg_regs_to_eeprom(method)

    def write_nsync_lmk_eeprom_to_cfg_regs(self):
        """read register cfg from eeprom and store it into registers"""
        self._check_nsync_supported()
        self.log.trace("LMK05318: read cfg from eeprom")
        self._nsync_pll.write_eeprom_to_cfg_regs()

    def get_nsync_lmk_eeprom_prog_cycles(self):
        """
        returns the number of eeprom programming cycles
        note:
        the actual counter only increases after programming AND power-cycle/hard-reset
        so multiple programming cycles without power cycle will lead to wrong
        counter values
        """
        self._check_nsync_supported()
        return self._nsync_pll.get_eeprom_prog_cycles()

    def get_nsync_lmk_status_dpll(self):
        """
        returns the DPLL status register as human readable string
        """
        self._check_nsync_supported()
        return self._nsync_pll.get_status_dpll()

    def get_nsync_lmk_status_pll_xo(self):
        """
        returns the PLL and XO status register as human readable string
        """
        self._check_nsync_supported()
        return self._nsync_pll.get_status_pll_xo()

    def peek8(self, addr):
        """Read from addr over SPI"""
        self._check_nsync_supported()
        val = self._nsync_pll.peek8(addr)
        return val

    def poke8(self, addr, val, overwrite_mask=False):
        """
        Write val to addr over SPI
        Some register of the LMK IC are supposed not to be written and therefore
        the whole register or just some bits. are protected by masking.
        If you are really sure what you are doing you can overwrite the masking
        by setting overwrite_mask=True
        """
        self._check_nsync_supported()
        self._nsync_pll.poke8(addr, val, overwrite_mask)

    def config_dpll(self, source):
        """
        configures the dpll registers needed to lock to the expected signal

        Initial config files were created with TICSpro, then files were compared
        against each other to determine which registers needed to be changed
        """
        if source == self.SOURCE_NSYNC_LMK_PRI_GTY_RCV_CLK:
            self._nsync_pll.pokes8((
                (0xC5,0x0B),
                (0xCC,0x05),
                (0xD1,0x08),
                (0xD3,0x0A),
                (0xD5,0x08),
                (0xD7,0x0A),
                (0xDA,0x02),
                (0xDB,0xFA),
                (0xDC,0xF1),
                (0xDE,0x06),
                (0xDF,0x1A),
                (0xE0,0x81),
                (0xE3,0x30),
                (0xE4,0xD4),
                (0xE6,0x06),
                (0xE7,0x1A),
                (0xE8,0x80),
                (0x100,0x00),
                (0x101,0x7D),
                (0x103,0x08),
                (0x109,0x0F),
                (0x10A,0xA0),
                (0x10F,0x78),
                (0x110,0x00),
                (0x111,0x00),
                (0x112,0x00),
                (0x113,0x0F),
                (0x114,0x0E),
                (0x115,0x0F),
                (0x116,0x08),
                (0x118,0x08),
                (0x119,0x06),
                (0x11A,0x08),
                (0x11B,0x06),
                (0x11E,0x00),
                (0x11F,0x71),
                (0x121,0xEB),
                (0x123,0x09),
                (0x128,0x03),
                (0x129,0x05),
                (0x12A,0x03),
                (0x12D,0x3E),
                (0x12E,0x3F),
                (0x130,0x01),
                (0x133,0x01),
                (0x134,0x4D),
                (0x135,0x55),
                (0x136,0x55),
                (0x137,0x55),
                (0x138,0x55),
                (0x139,0x55),
                (0x13A,0xFF),
                (0x13B,0xFF),
                (0x13C,0xFF),
                (0x13D,0xFF),
                (0x13E,0xFF),
                (0x141,0x19),
                (0x145,0x78),
                (0x147,0x00),
                (0x148,0x27),
                (0x149,0x10),
                (0x14B,0x32),
                (0x14F,0x78),
                (0x151,0x00),
                (0x152,0x27),
                (0x153,0x10)))
        elif source == self.SOURCE_NSYNC_LMK_PRI_FABRIC_CLK:
            self._nsync_pll.pokes8((
                (0xC5,0x0D),
                (0xCC,0x07),
                (0xD1,0x04),
                (0xD3,0x05),
                (0xD5,0x04),
                (0xD7,0x05),
                (0xDA,0x01),
                (0xDB,0x2C),
                (0xDC,0x00),
                (0xDE,0x03),
                (0xDF,0x0D),
                (0xE0,0x40),
                (0xE3,0x18),
                (0xE4,0x6A),
                (0xE6,0x03),
                (0xE7,0x0D),
                (0xE8,0x40),
                (0x100,0x06),
                (0x101,0x00),
                (0x103,0x7D),
                (0x109,0xF4),
                (0x10A,0x24),
                (0x10F,0x7A),
                (0x110,0x1F),
                (0x111,0x1F),
                (0x112,0x1F),
                (0x113,0x13),
                (0x114,0x10),
                (0x115,0x13),
                (0x116,0x04),
                (0x118,0x04),
                (0x119,0x02),
                (0x11A,0x07),
                (0x11B,0x02),
                (0x11E,0x02),
                (0x11F,0x6C),
                (0x121,0xE7),
                (0x123,0x25),
                (0x128,0x00),
                (0x129,0x06),
                (0x12A,0x00),
                (0x12D,0x17),
                (0x12E,0x1B),
                (0x130,0x00),
                (0x133,0x1E),
                (0x134,0x84),
                (0x135,0x80),
                (0x136,0x00),
                (0x137,0x00),
                (0x138,0x00),
                (0x139,0x00),
                (0x13A,0x00),
                (0x13B,0x00),
                (0x13C,0x00),
                (0x13D,0x00),
                (0x13E,0x00),
                (0x141,0x0A),
                (0x145,0x0A),
                (0x147,0x03),
                (0x148,0x0F),
                (0x149,0x49),
                (0x14B,0x14),
                (0x14F,0x9A),
                (0x151,0x03),
                (0x152,0x0F),
                (0x153,0x49)))
        else:
            raise RuntimeError(
                "Invalid source for dpll programming")

    def set_ref_lock_led(self, val):
        """
        Set the reference-locked LED on the back panel
        """
        self._ref_lck_led.set(int(val))

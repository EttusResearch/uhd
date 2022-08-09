#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
X4xx peripherals
"""

import time
import struct
from statistics import mean
from usrp_mpm import lib  # Pulls in everything from C++-land
from usrp_mpm.sys_utils import i2c_dev
from usrp_mpm.sys_utils.gpio import Gpio
from usrp_mpm.sys_utils.uio import UIO
from usrp_mpm.mpmutils import poll_with_timeout
from usrp_mpm.sys_utils.sysfs_thermal import read_thermal_sensor_value
from usrp_mpm.periph_manager.common import MboardRegsCommon


class MboardRegsControl(MboardRegsCommon):
    """
    Control the FPGA Motherboard registers

    A note on using this object: All HDL-specifics should be encoded in this
    class. That means that calling peek32 or poke32 on this class should only be
    used in rare exceptions. The call site shouldn't have to know about
    implementation details behind those registers.

    To do multiple peeks/pokes without opening and closing UIO objects, it is
    possible to do that from outside the object:
    >>> with mb_regs_control.regs:
    ...     mb_regs_control.poke32(addr0, data0)
    ...     mb_regs_control.poke32(addr1, data1)
    """
    # Motherboard registers
    # pylint: disable=bad-whitespace
    # 0 through 0x14 are common regs (see MboardRegsCommon)
    MB_CLOCK_CTRL        = 0x0018
    MB_PPS_CTRL          = 0x001C
    MB_BUS_CLK_RATE      = 0x0020
    MB_BUS_COUNTER       = 0x0024
    MB_GPIO_CTRL         = 0x002C
    MB_GPIO_MASTER       = 0x0030
    MB_GPIO_RADIO_SRC    = 0x0034
    # MB_NUM_TIMEKEEPERS   = 0x0048 Shared with MboardRegsCommon
    MB_SERIAL_NO_LO      = 0x004C
    MB_SERIAL_NO_HI      = 0x0050
    MB_MFG_TEST_CTRL     = 0x0054
    MB_MFG_TEST_STATUS   = 0x0058
    # QSFP port info consists of 2 ports of 4 lanes each,
    # both separated by their corresponding stride value
    MB_QSFP_PORT_INFO    = 0x0060
    MB_QSFP_LANE_STRIDE  = 0x4
    MB_QSFP_PORT_STRIDE  = 0x10
    # Versioning registers
    MB_VER_FPGA         = 0x0C00
    MB_VER_CPLD_IFC     = 0x0C10
    MB_VER_RF_CORE_DB0  = 0x0C20
    MB_VER_RF_CORE_DB1  = 0x0C30
    MB_VER_GPIO_IFC_DB0 = 0x0C40
    MB_VER_GPIO_IFC_DB1 = 0x0C50
    CURRENT_VERSION_OFFSET           = 0x0
    OLDEST_COMPATIBLE_VERSION_OFFSET = 0x4
    VERSION_LAST_MODIFIED_OFFSET     = 0x8
    # Timekeeper registers start at 0x1000 (see MboardRegsCommon)

    # Clock control register bit masks
    CLOCK_CTRL_PLL_SYNC_DELAY   = 0x00FF0000
    CLOCK_CTRL_PLL_SYNC_DONE    = 0x00000200
    CLOCK_CTRL_PLL_SYNC_TRIGGER = 0x00000100
    CLOCK_CTRL_TRIGGER_IO_SEL   = 0x00000030
    CLOCK_CTRL_TRIGGER_PPS_SEL  = 0x00000003

    MFG_TEST_CTRL_GTY_RCV_CLK_EN = 0x00000001
    MFG_TEST_CTRL_FABRIC_CLK_EN  = 0x00000002
    MFG_TEST_AUX_REF_FREQ        = 0x03FFFFFF
    # Clock control register values
    CLOCK_CTRL_TRIG_IO_INPUT       = 0
    CLOCK_CTRL_PPS_INT_25MHz       = 0
    CLOCK_CTRL_TRIG_IO_PPS_OUTPUT  = 0x10
    CLOCK_CTRL_PPS_INT_10MHz       = 0x1
    CLOCK_CTRL_PPS_EXT             = 0x2
    # pylint: enable=bad-whitespace

    def __init__(self, label, log):
        MboardRegsCommon.__init__(self, label, log)
        def peek32(address):
            """
            Safe peek (opens and closes UIO).
            """
            with self.regs:
                return self.regs.peek32(address)
        def poke32(address, value):
            """
            Safe poke (opens and closes UIO).
            """
            with self.regs:
                self.regs.poke32(address, value)
        # MboardRegsCommon.poke32() and ...peek32() don't open the UIO, so we
        # overwrite them with "safe" versions that do open the UIO.
        self.peek32 = peek32
        self.poke32 = poke32

    def set_serial_number(self, serial_number):
        """
        Set serial number register
        """
        assert len(serial_number) > 0
        assert len(serial_number) <= 8
        serial_number = serial_number + b'\x00' * (8 - len(serial_number))
        (sn_lo, sn_hi) = struct.unpack("II", serial_number)
        with self.regs:
            self.poke32(self.MB_SERIAL_NO_LO, sn_lo)
            self.poke32(self.MB_SERIAL_NO_HI, sn_hi)

    def get_compat_number(self):
        """get FPGA compat number

        This function reads back FPGA compat number.
        The return is a tuple of
        2 numbers: (major compat number, minor compat number)
        X4xx stores this tuple in MB_VER_FPGA instead of MB_COMPAT_NUM
        """
        version = self.peek32(self.MB_VER_FPGA + self.CURRENT_VERSION_OFFSET)
        major = (version >> 23) & 0x1FF
        minor = (version >> 12) & 0x7FF
        return (major, minor)

    def get_db_gpio_ifc_version(self, slot_id):
        """
        Get the version of the DB GPIO interface for the corresponding slot
        """
        if slot_id == 0:
            current_version = self.peek32(self.MB_VER_GPIO_IFC_DB0)
        elif slot_id == 1:
            current_version = self.peek32(self.MB_VER_GPIO_IFC_DB1)
        else:
            raise RuntimeError("Invalid daughterboard slot id: {}".format(slot_id))

        major = (current_version>>23) & 0x1ff
        minor = (current_version>>12) & 0x7ff
        build = current_version & 0xfff
        return (major, minor, build)

    def _get_qsfp_lane_value(self, port, lane):
        addr = self.MB_QSFP_PORT_INFO + (port * self.MB_QSFP_PORT_STRIDE) \
                                      + (lane * self.MB_QSFP_LANE_STRIDE)
        return (self.peek32(addr) >> 8) & 0xFF

    def _get_qsfp_type(self, port=0):
        """
        Read the type of qsfp port is in the specified port
        """
        x4xx_qsfp_types = {
            0: "",    # Port not connected
            1: "1G",
            2: "10G",
            3: "A",   # Aurora
            4: "W",   # White Rabbit
            5: "100G"
        }

        lane_0_val = self._get_qsfp_lane_value(port, 0)
        num_lanes = 1

        # Because we have qsfp, we could have up to 4x connection at the port
        if lane_0_val > 0:
            for lane in range(1, 4):
                lane_val = self._get_qsfp_lane_value(port, lane)
                if lane_val == lane_0_val:
                    num_lanes += 1

        if num_lanes > 1:
            return str(num_lanes) + "x" + x4xx_qsfp_types.get(lane_0_val, "")

        return x4xx_qsfp_types.get(lane_0_val, "")

    def get_fpga_type(self):
        """
        Reads the type of the FPGA image currently loaded
        Returns a string with the type (ie CG, XG, C2, etc.)
        """
        x4xx_fpga_types_by_qsfp = {
            ("", ""):          "",
            ("10G", "10G"):    "XG",
            ("10G", ""):       "X1",
            ("2x10G", ""):     "X2",
            ("4x10G", ""):     "X4",
            ("4x10G", "100G"): "X4C",
            ("100G", "100G"):  "CG",
            ("100G", ""):      "C1"
        }

        qsfp0_type = self._get_qsfp_type(0)
        qsfp1_type = self._get_qsfp_type(1)
        self.log.trace("QSFP types: ({}, {})".format(qsfp0_type, qsfp1_type))
        try:
            fpga_type = x4xx_fpga_types_by_qsfp[(qsfp0_type, qsfp1_type)]
        except KeyError:
            self.log.warning("Unrecognized QSFP type combination: ({}, {})"
                             .format(qsfp0_type, qsfp1_type))
            fpga_type = ""

        if not fpga_type and self.is_pcie_present():
            fpga_type = "LV"

        return fpga_type

    def is_pcie_present(self):
        """
        Return True in case the PCI_EXPRESS_BIT is set in the FPGA image, which
        means there is a PCI-Express core. False otherwise.
        """
        regs_val = self.peek32(self.MB_DEVICE_ID)
        return (regs_val & 0x80000000) != 0

    def is_pll_sync_done(self):
        """
        Return state of PLL sync bit from motherboard clock control register.
        """
        return bool(self.peek32(MboardRegsControl.MB_CLOCK_CTRL) & \
                    self.CLOCK_CTRL_PLL_SYNC_DONE)

    def pll_sync_trigger(self, clock_ctrl_pps_src):
        """
        Callback for LMK04832 driver to actually trigger the sync. Set PPS
        source accordingly.
        """
        with self.regs:
            # Update clock control config register to use the currently relevant
            # PPS source
            config = self.peek32(self.MB_CLOCK_CTRL)
            trigger_config = \
                (config & ~MboardRegsControl.CLOCK_CTRL_TRIGGER_PPS_SEL) \
                | clock_ctrl_pps_src
            # trigger sync with appropriate configuration
            self.poke32(
                self.MB_CLOCK_CTRL,
                trigger_config | self.CLOCK_CTRL_PLL_SYNC_TRIGGER)
            # wait for sync done indication from FPGA
            # The following value is in ms, it was experimentally picked.
            pll_sync_timeout = 1500 # ms
            result = poll_with_timeout(self.is_pll_sync_done, pll_sync_timeout, 10)
            # de-assert sync trigger signal
            self.poke32(self.MB_CLOCK_CTRL, trigger_config)
            if not result:
                self.log.error("PLL_SYNC_DONE not received within timeout")
            return result

    def set_trig_io_output(self, enable_output):
        """
        Enable TRIG I/O output. If enable_output is "True", then we set TRIG I/O
        to be an output. If enable_output is "False", then we make it an input.

        Note that the clocking board also is involved in configuring the TRIG I/O.
        This is only the part that configures the FPGA registers.
        """
        with self.regs:
            # prepare clock control FPGA register content
            clock_ctrl_reg = self.peek32(MboardRegsControl.MB_CLOCK_CTRL)
            clock_ctrl_reg &= ~self.CLOCK_CTRL_TRIGGER_IO_SEL
            if enable_output:
                clock_ctrl_reg |= self.CLOCK_CTRL_TRIG_IO_PPS_OUTPUT
            else:
                # for both input and off ensure FPGA does not drive trigger IO line
                clock_ctrl_reg |= self.CLOCK_CTRL_TRIG_IO_INPUT
            self.poke32(MboardRegsControl.MB_CLOCK_CTRL, clock_ctrl_reg)

    def configure_pps_forwarding(self, enable, master_clock_rate, prc_rate, delay):
        """
        Configures the PPS forwarding to the sample clock domain (master
        clock rate). This function assumes _sync_spll_clocks function has
        already been executed.

        :param enable: Boolean to choose whether PPS is forwarded to the
                       sample clock domain.

        :param master_clock_rate: Master clock rate in MHz

        :param prc_rate: PRC rate in MHz

        :param delay:  Delay in seconds from the PPS rising edge to the edge
                       occurence in the application. This value has to be in
                       range 0 < x <= 1. In order to forward the PPS signal
                       from base reference clock to sample clock an aligned
                       rising edge of the clock is required. This can be
                       created by the _sync_spll_clocks function. Based on the
                       greatest common divisor of the two clock rates there
                       are multiple occurences of an aligned edge each second.
                       One of these aligned edges has to be chosen for the
                       PPS forwarding by setting this parameter.

        :return:       None, Exception on error
        """
        # delay range check 0 < x <= 1
        if (delay <= 0 or delay > 1):
            raise RuntimeError("The delay has to be in range 0 < x <= 1")

        with self.regs:
            # configure delay in BRC clock domain
            value = self.peek32(self.MB_CLOCK_CTRL)
            pll_sync_delay = (value >> 16) & 0xFF
            # pps_brc_delay constants required by HDL implementation
            pps_brc_delay = pll_sync_delay + 2 - 1
            value = (value & 0x00FFFFFF) | (pps_brc_delay << 24)
            self.poke32(self.MB_CLOCK_CTRL, value)

            # configure delay in PRC clock domain
            # reduction by 4 required by HDL implementation
            pps_prc_delay = (int(delay * prc_rate) - 4) & 0x3FFFFFF
            if pps_prc_delay == 0:
                # limitation from HDL implementation
                raise RuntimeError("The calculated delay has to be greater than 0")
            value = pps_prc_delay

            # configure clock divider
            # reduction by 2 required by HDL implementation
            prc_rc_divider = (int(master_clock_rate/prc_rate) - 2) & 0x3
            value = value | (prc_rc_divider << 28)

            # write configuration to PPS control register (with PPS disabled)
            self.poke32(self.MB_PPS_CTRL, value)

            # enables PPS depending on parameter
            if enable:
                # wait for 1 second to let configuration settle for any old PPS pulse
                time.sleep(1)
                # update value with enabled PPS
                value = value | (1 << 31)
                # write final configuration to PPS control register
                self.poke32(self.MB_PPS_CTRL, value)
        return True

    def enable_ecpri_clocks(self, enable, clock):
        """
        Enable or disable the export of FABRIC and GTY_RCV eCPRI
        clocks. Main use case until we support eCPRI is manufacturing
        testing.
        """
        valid_clocks_list = ['gty_rcv', 'fabric', 'both']
        assert clock in valid_clocks_list
        clock_enable_sig = 0
        if clock == 'gty_rcv':
            clock_enable_sig = self.MFG_TEST_CTRL_GTY_RCV_CLK_EN
        elif clock == 'fabric':
            clock_enable_sig = self.MFG_TEST_CTRL_FABRIC_CLK_EN
        else:# 'both' case
            clock_enable_sig = (self.MFG_TEST_CTRL_GTY_RCV_CLK_EN |
                                self.MFG_TEST_CTRL_FABRIC_CLK_EN)
        with self.regs:
            clock_ctrl_reg = self.peek32(MboardRegsControl.MB_MFG_TEST_CTRL)
            if enable:
                clock_ctrl_reg |= clock_enable_sig
            else:
                clock_ctrl_reg &= ~clock_enable_sig
            self.poke32(MboardRegsControl.MB_MFG_TEST_CTRL, clock_ctrl_reg)

    def get_fpga_aux_ref_freq(self):
        """
        Return the tick count of an FPGA counter which measures the width of
        the PPS signal on the FPGA_AUX_REF FPGA input using a 40 MHz clock.
        Main use case until we support eCPRI is manufacturing testing.
        A return value of 0 indicates absence of a valid PPS signal on the
        FPGA_AUX_REF line.
        """
        status_reg = self.peek32(self.MB_MFG_TEST_STATUS)
        return status_reg & self.MFG_TEST_AUX_REF_FREQ


class CtrlportRegs:
    """
    Control the FPGA Ctrlport registers
    """
    # pylint: disable=bad-whitespace
    IPASS_OFFSET        = 0x000010
    MB_PL_SPI_CONFIG    = 0x000020
    DB_SPI_CONFIG       = 0x000024
    MB_PL_CPLD          = 0x008000
    DB_0_CPLD           = 0x010000
    DB_1_CPLD           = 0x018000
    # pylint: enable=bad-whitespace

    min_mb_cpld_spi_divider = 2
    min_db_cpld_spi_divider = 5
    class MbPlCpldIface:
        """ Exposes access to register mapped MB PL CPLD register space """
        SIGNATURE_OFFSET = 0x0000
        REVISION_OFFSET  = 0x0004

        SIGNATURE        = 0x3FDC5C47
        MIN_REQ_REVISION = 0x20082009

        def __init__(self, regs_iface, offset, log):
            self.log = log
            self.offset = offset
            self.regs = regs_iface

        def peek32(self, addr):
            return self.regs.peek32(addr + self.offset)

        def poke32(self, addr, val):
            self.regs.poke32(addr + self.offset, val)

        def check_signature(self):
            read_signature = self.peek32(self.SIGNATURE_OFFSET)
            if self.SIGNATURE != read_signature:
                self.log.error('MB PL CPLD signature {:X} does not match '
                               'expected value {:X}'.format(read_signature, self.SIGNATURE))
                raise RuntimeError('MB PL CPLD signature {:X} does not match '
                                   'expected value {:X}'.format(read_signature, self.SIGNATURE))

        def check_revision(self):
            read_revision = self.peek32(self.REVISION_OFFSET)
            if read_revision < self.MIN_REQ_REVISION:
                error_message = ('MB PL CPLD revision {:X} is out of date. '
                                'Expected value {:X}. Update your CPLD image.'
                                .format(read_revision, self.MIN_REQ_REVISION))
                self.log.error(error_message)
                raise RuntimeError(error_message)

    class DbCpldIface:
        """ Exposes access to register mapped DB CPLD register spaces """
        def __init__(self, regs_iface, offset):
            self.offset = offset
            self.regs = regs_iface

        def peek32(self, addr):
            return self.regs.peek32(addr + self.offset)

        def poke32(self, addr, val):
            self.regs.poke32(addr + self.offset, val)

    def __init__(self, label, log):
        self.log = log.getChild("CtrlportRegs")
        self._regs_uio_opened = False
        try:
            self.regs = UIO(
                label=label,
                read_only=False
            )
        except RuntimeError:
            self.log.warning('Ctrlport regs could not be found. ' \
                             'MPM Endpoint to the FPGA is not part of this image.')
            self.regs = None
        # Initialize SPI interface to MB PL CPLD and DB CPLDs
        self.set_mb_pl_cpld_divider(self.min_mb_cpld_spi_divider)
        self.set_db_divider_value(self.min_db_cpld_spi_divider)
        self.mb_pl_cpld_regs = self.MbPlCpldIface(self, self.MB_PL_CPLD, self.log)
        self.mb_pl_cpld_regs.check_signature()
        self.mb_pl_cpld_regs.check_revision()
        self.db_0_regs = self.DbCpldIface(self, self.DB_0_CPLD)
        self.db_1_regs = self.DbCpldIface(self, self.DB_1_CPLD)

    def init(self):
        if not self._regs_uio_opened:
            self.regs._open()
            self._regs_uio_opened = True

    def deinit(self):
        if self._regs_uio_opened:
            self.regs._close()
            self._regs_uio_opened = False

    def peek32(self, addr):
        if self.regs is None:
            raise RuntimeError('The ctrlport registers were never configured!')
        if self._regs_uio_opened:
            return self.regs.peek32(addr)
        else:
            with self.regs:
                return self.regs.peek32(addr)

    def poke32(self, addr, val):
        if self.regs is None:
            raise RuntimeError('The ctrlport registers were never configured!')
        if self._regs_uio_opened:
            return self.regs.poke32(addr, val)
        else:
            with self.regs:
                return self.regs.poke32(addr, val)

    def set_mb_pl_cpld_divider(self, divider_value):
        if not self.min_mb_cpld_spi_divider <= divider_value <= 0xFFFF:
            self.log.error('Cannot set MB CPLD SPI divider to invalid value {}'
                           .format(divider_value))
            raise RuntimeError('Cannot set MB CPLD SPI divider to invalid value {}'
                               .format(divider_value))
        self.poke32(self.MB_PL_SPI_CONFIG, divider_value)

    def set_db_divider_value(self, divider_value):
        if not self.min_db_cpld_spi_divider <= divider_value <= 0xFFFF:
            self.log.error('Cannot set DB SPI divider to invalid value {}'
                           .format(divider_value))
            raise RuntimeError('Cannot set DB SPI divider to invalid value {}'
                               .format(divider_value))
        self.poke32(self.DB_SPI_CONFIG, divider_value)

    def get_db_cpld_iface(self, db_id):
        return self.db_0_regs if db_id == 0 else self.db_1_regs

    def get_mb_pl_cpld_iface(self):
        return self.mb_pl_cpld_regs

    def enable_cable_present_forwarding(self, enable=True):
        value = 1 if enable else 0
        self.poke32(self.IPASS_OFFSET, value)


# QSFP Adapter IDs according to SFF-8436 rev 4.9 table 30
QSFP_IDENTIFIERS = {
    0x00: "Unknown or unspecified",
    0x01: "GBIC",
    0x02: "Module/connector soldered to motherboard (using SFF-8472)",
    0x03: "SFP/SFP+/SFP28",
    0x04: "300 pin XBI",
    0x05: "XENPAK",
    0x06: "XFP",
    0x07: "XFF",
    0x08: "XFP-E",
    0x09: "XPAK",
    0x0A: "X2",
    0x0B: "DWDM-SFP/SFP+ (not using SFF-8472)",
    0x0C: "QSFP (INF-8438)",
    0x0D: "QSFP+ or later (SFF-8436, SFF-8635, SFF-8665, SFF-8685 et al)",
    0x0E: "CXP or later",
    0x0F: "Shielded Mini Multilane HD0x4X",
    0x10: "Shielded Mini Multilane HD0x8X",
    0x11: "QSFP28 or later (SFF-8665 et al)",
    0x12: "CXP2 (aka CXP28) or later",
    0x13: "CDFP (Style0x1/Style2)",
    0x14: "Shielded Mini Multilane HD0x4X Fanout Cable",
    0x15: "Shielded Mini Multilane HD0x8X Fanout Cable",
    0x16: "CDFP (Style0x3)"
}

# QSFP revison compliance according to SFF-8636 rev 2.9 table 6-3
QSFP_REVISION_COMPLIANCE = {
    0x00: "Not specified.",
    0x01: "SFF-8436 Rev 4.8 or earlier",
    0x02: "SFF-8436 Rev 4.8 or earlier (except 0x186-0x189)",
    0x03: "SFF-8636 Rev 1.3 or earlier",
    0x04: "SFF-8636 Rev 1.4",
    0x05: "SFF-8636 Rev 1.5",
    0x06: "SFF-8636 Rev 2.0",
    0x07: "SFF-8636 Rev 2.5, 2.6 and 2.7",
    0x08: "SFF-8636 Rev 2.8 or later"
}

# QSFP connector types according to SFF-8029 rev 3.2 table 4-3
QSFP_CONNECTOR_TYPE = {
    0x00: "Unknown or unspecified",
    0x01: "SC (Subscriber Connector)",
    0x02: "Fibre Channel Style 1 copper connector",
    0x03: "Fibre Channel Style 2 copper connector",
    0x04: "BNC/TNC (Bayonet/Threaded Neill-Concelman)",
    0x05: "Fibre Channel coax headers",
    0x06: "Fiber Jack",
    0x07: "LC (Lucent Connector)",
    0x08: "MT-RJ (Mechanical Transfer - Registered Jack)",
    0x09: "MU (Multiple Optical)",
    0x0A: "SG",
    0x0B: "Optical Pigtail",
    0x0C: "MPO 1x12 (Multifiber Parallel Optic)",
    0x0D: "MPO 2x16",
    0x20: "HSSDC II (High S peed Serial Data Connector)",
    0x21: "Copper pigtail",
    0x22: "RJ45 (Registered Jack)",
    0x23: "No separable connector",
    0x24: "MXC 2x16"
}

class QSFPModule:
    """
    QSFPModule enables access to the I2C register interface of an QSFP module.

    The class queries the module register using I2C commands according to
    SFF-8486 rev 4.9 specification.
    """

    def __init__(self, gpio_modprs, gpio_modsel, devsymbol, log):
        """
        modprs: Name of the GPIO pin that reports module presence
        modsel: Name of the GPIO pin that controls ModSel of QSFP module
        devsymbol: Symbol name of the device used for I2C communication
        """

        self.log = log.getChild('QSFP')

        # Hold the ModSelL GPIO low for communication over I2C. Because X4xx
        # uses a I2C switch to communicate with the QSFP modules we can keep
        # ModSelL low all the way long, because each QSFP module has
        # its own I2C address (see SFF-8486 rev 4.9, chapter 4.1.1.1).
        self.modsel = Gpio(gpio_modsel, Gpio.OUTPUT, 0)

        # ModPrs pin read pin MODPRESL from QSFP connector
        self.modprs = Gpio(gpio_modprs, Gpio.INPUT, 0)

        # resolve device node name for I2C communication
        devname = i2c_dev.dt_symbol_get_i2c_bus(devsymbol)

        # create an object to access I2C register interface
        self.qsfp_regs = lib.i2c.make_i2cdev_regs_iface(
            devname, # dev node name
            0x50,    # start address according to SFF-8486 rev 4.9 chapter 7.6
            False,   # use 7 bit address schema
            100,     # timeout_ms
            1        # reg_addr_size
        )

    def _peek8(self, address):
        """
        Helper method to read bytes from the I2C register interface.

        This helper returns None in case of failed communication
        (e.g. missing or broken adapter).
        """
        try:
            return self.qsfp_regs.peek8(address)
        except RuntimeError as err:
            self.log.debug("Could not read QSFP register ({})".format(err))
            return None

    def _revision_compliance(self, status):
        """
        Map the revison compliance status byte to a human readable string
        according to SFF-8636 rev 2.9 table 6-3
        """
        assert isinstance(status, int)
        assert 0 <= status <= 255
        if status > 0x08:
            return "Reserved"
        return QSFP_REVISION_COMPLIANCE[status]

    def is_available(self):
        """
        Checks whether QSFP adapter is available by checking modprs pin
        """
        return self.modprs.get() == 0 #modprs is active low

    def enable_i2c(self, enable):
        """
        Enable or Disable I2C communication with QSFP module. Use with
        care. Because X4xx uses an I2C switch to address the QSFP ports
        there is no need to drive the modsel high (inactive). Disabled
        I2C communication leads to unwanted result when query module
        state even if the module reports availability.
        """
        self.modsel.set("0" if enable else "1") #modsel is active low

    def adapter_id(self):
        """
        Returns QSFP adapter ID as a byte (None if not present)
        """
        return self._peek8(0)

    def adapter_id_name(self):
        """
        Maps QSFP adapter ID to a human readable string according
        to SFF-8436 rev 4.9 table 30
        """
        adapter_id = self.adapter_id()
        if adapter_id is None:
            return adapter_id
        assert isinstance(adapter_id, int)
        assert 0 <= adapter_id <= 255
        if adapter_id > 0x7F:
            return "Vendor Specific"
        if adapter_id > 0x16:
            return "Reserved"
        return QSFP_IDENTIFIERS[adapter_id]

    def status(self):
        """
        Return the 2 byte QSFP adapter status according to SFF-8636
        rev 2.9 table 6-2
        """
        compliance = self._peek8(1)
        status = self._peek8(2)
        if compliance is None or status is None:
            return None
        assert isinstance(compliance, int)
        assert isinstance(status, int)
        return (compliance, status)

    def decoded_status(self):
        """
        Decode the 2 status bytes of the QSFP adapter into a tuple
        of human readable strings. See SFF-8436 rev 4.9 table 17
        """
        status = self.status()
        if not status:
            return None
        return (
            self._revision_compliance(status[0]),
            "Flat mem" if status[1] & 0b100 else "Paged mem",
            "IntL asserted" if status[1] & 0b010 else "IntL not asserted",
            "Data not ready" if status[1] & 0b001 else "Data ready"
        )

    def vendor_name(self):
        """
        Return vendor name according to SFF-8436 rev 4.9 chapter 7.6.2.14
        """
        content = [self._peek8(i) for i in range(148, 163)]

        if all(content): # list must not contain any None values
            # convert ASCII codes to string and strip whitespaces at the end
            return "".join([chr(i) for i in content]).rstrip()

        return None

    def connector_type(self):
        """
        Return connector type according to SFF-8029 rev 3.2 table 4-3
        """
        ctype = self._peek8(130)
        if ctype is None:
            return None
        assert isinstance(ctype, int)
        assert 0 <= ctype <= 255

        if (0x0D < ctype < 0x20) or (0x24 < ctype < 0x80):
            return "Reserved"
        if ctype > 0x7F:
            return "Vendor Specific"
        return QSFP_CONNECTOR_TYPE[ctype]

    def info(self):
        """
        Human readable string of important QSFP module information
        """
        if self.is_available():
            status = self.decoded_status()
            return "Vendor name:    {}\n" \
                   "id:             {}\n" \
                   "Connector type: {}\n" \
                   "Compliance:     {}\n" \
                   "Status:         {}".format(
                       self.vendor_name(), self.adapter_id_name(),
                       self.connector_type(), status[0], status[1:])

        return "No module detected"

def get_temp_sensor(sensor_names, reduce_fn=mean, log=None):
    """ Get temperature sensor reading from X4xx. """
    temps = []
    try:
        for sensor_name in sensor_names:
            temp_raw = read_thermal_sensor_value(
                sensor_name, 'in_temp_raw', 'iio', 'name')
            temp_offset = read_thermal_sensor_value(
                sensor_name, 'in_temp_offset', 'iio', 'name')
            temp_scale = read_thermal_sensor_value(
                sensor_name, 'in_temp_scale', 'iio', 'name')
            # sysfs-bus-iio linux kernel API reports temp in milli deg C
            # https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-bus-iio
            temp_in_deg_c = (temp_raw + temp_offset) * temp_scale / 1000
            temps.append(temp_in_deg_c)
    except ValueError:
        if log:
            log.warning("Error when converting temperature value.")
        temps = [-1]
    except KeyError:
        if log:
            log.warning("Can't read %s temp sensor fron iio sub-system.",
                        str(sensor_name))
        temps = [-1]
    return {
        'name': 'temperature',
        'type': 'REALNUM',
        'unit': 'C',
        'value': str(reduce_fn(temps))
    }

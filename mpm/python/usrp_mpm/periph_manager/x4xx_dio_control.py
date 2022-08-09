#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
X4xx DIO Control
"""

import re
import signal
from multiprocessing import Process, Event, Value
from usrp_mpm.sys_utils.gpio import Gpio
from usrp_mpm.mpmutils import poll_with_timeout

class DioControl:
    """
    DioControl acts as front end for DIO AUX BOARD

    The DioControl class uses three hardware resources to control the behavior
    of the board, which are
     * I2C extender
     * MB registers
     * MB cpld registers

    DioControl supports arbitrary methods of addressing the pins on the
    frontend. The current implementation supports two ways of pin addressing:
    HDMI and DIO. Use set_port_mapping to switch between both of them.

    When using HDMI as pin addressing scheme you have to give the real pin
    number of the HDMI adapter like this::
        ┌───────────────────────────────┐
        └┐19 17 15 13 11 09 07 05 03 01┌┘
         └┐ 18 16 14 12 10 08 06 04 02┌┘
          └───────────────────────────┘
    Be aware that not all pins are accessible. The DioControl class will warn
    about the usage of unassigned pins.

    The second option is the DIO addressing scheme. Here all user accessible
    pins are numbered along the HDMI pin numbers which gives a pin table like
    this::
        ┌───────────────────────────────┐
        └┐11 -- 09 08 -- 05 04 -- 01 00┌┘
         └┐ -- 10 -- 07 06 -- 03 02 --┌┘
          └───────────────────────────┘

    Within the MPM shell one can query the state of the DIO board using
    dio_status. This gives an output like this::
         HDMI mapping | PORT A                  | PORT B
        --------------+-------------------------+-------------------------
         voltage      | OFF - PG:NO - EXT:OFF   | 1V8 - PG:YES - EXT:OFF
        --------------+-------------------------+-------------------------
         master       | 0.. 00.0 0.00 .00. 00.1 | 0.. 00.0 0.00 .00. 00.1
         direction    | 0.. 00.0 0.00 .00. 00.1 | 0.. 00.0 0.00 .00. 00.0
        --------------+-------------------------+-------------------------
         output       | 0.. 00.0 0.00 .00. 00.1 | 0.. 00.0 0.00 .00. 00.0
         input        | 0.. 00.0 0.00 .00. 00.1 | 0.. 00.0 0.00 .00. 00.0
    The table displays the current state of HDMI port A and B provided by the
    DIO board as well as the state of the corresponding register maps and GPIO
    pins in a user readable form.

    The header shows the active mapping and the port names. Change the
    mapping with set_port_mapping.

    The first row shows the voltage state of each port. Voltage can be one of
    the states in (OFF, 1V8, 2V5, 3V3). Change the power state by using
    set_voltage_level. When the voltage level is set to OFF, the corresponding
    GPIO pin EN_PORT is set low, high otherwise.
    When voltage is set to one of the other states EN_PORT<x> is set to high and
    EN_PORT<x>_<?V?> is set accordingly where 1V8 corresponds to 2V5 and 3V3
    being both low. PG shows whether the PG (power good) pin corresponding to
    the port (PORT<x>_PG) is high. This is NO if power is OFF and YES otherwise.
    EXT shows whether EN_EXT_PWR_<x> is enabled for the port. Change the
    external power using set_external_power.
    Note: A port must have a reasonable voltage level assigned to it before
          changes to the output register takes effect on the HDMI port pins or
          pin states of the HDMI port can be read in input register.

    The master row shows the pin assignments for the master register which
    decides whether PS (1) or FPGA (0) drives output register pins. Change
    values using set_master_pin(s).

    The direction row shows the pin assignments for the direction register which
    decides whether the pin is written (1) or read (0) by the FPGA. Change
    values using set_direction_pin(s).

    The output and input rows shows the pin assignments for the FPGAs input and
    output registers. Only the output register pins can be changed. Change
    values using set_output_pin(s).

    """
    # For this, we want our own formatting rules
    # pylint: disable=bad-whitespace

    # Available DIO ports
    DIO_PORTS = ("PORTA", "PORTB")
    # Available voltage levels
    DIO_VOLTAGE_LEVELS = ("OFF", "1V8", "2V5", "3V3")

    # For each mapping supported by DioControl the class needs the following
    # information
    # * map_name: name of the mapping (in uppercase)
    # * pin_names: names of the pins starting with smallest. Unassignable PINS
    #              must be named as well
    # * port_map: mapping table from FPGA register indices to pin indices. A
    #             mapping of (4, 2) means pin 4 is mapped to register bit 0 and
    #             pin2 is mapped to register bit 1. Only assignable pins
    #             may appear in the mapping.
    # * first_pin: index of the first pin for the mapping

    # HDMI mapping constants
    HDMI_MAP_NAME = "HDMI"
    HDMI_PIN_NAMES = ("Data2+", "Data2_SHD", "Data2-", "Data1+", "Data1_SHD",
                      "Data1-", "Data0+", "Data0_SHD", "Data0-", "CLK+",
                      "CLK_SHD", "CLK-", "RESERVED", "HEC_Data-", "SCL",
                      "SDA", "HEC_GND", "V+", "HEC_Data+")
    HDMI_PORT_MAP = {
        DIO_PORTS[0]: ( 3,  1,  4,  6,  9,  7, 10, 12, 15, 13, 16, 19),
        DIO_PORTS[1]: (16, 19, 15, 13, 10, 12,  9,  7,  4,  6,  3,  1)
    }
    HDMI_FIRST_PIN = 1

    # DIO mapping constants
    DIO_MAP_NAME = "DIO"
    DIO_PIN_NAMES = ("DIO0", "DIO1", "DIO2", "DIO3", "DIO4", "DIO5",
                     "DIO6", "DIO7", "DIO8", "DIO9", "DIO10", "DIO11")
    DIO_PORT_MAP = {
        DIO_PORTS[0]: (1, 0, 2, 3, 5, 4, 6, 7, 9, 8, 10, 11),
        DIO_PORTS[1]: (10, 11, 9, 8, 6, 7, 5, 4, 2, 3, 1, 0)
    }
    DIO_FIRST_PIN = 0

    # Register layout/size constants
    PORT_BIT_SIZE       = 16    # number of bits used in register per port
    PORT_USED_BITS_MASK = 0xFFF # masks out lower 12 of 16 bits used per port

    # DIO registers addresses in FPGA
    FPGA_DIO_REGISTER_BASE                 = 0x2000
    FPGA_DIO_MASTER_REGISTER               = FPGA_DIO_REGISTER_BASE
    FPGA_DIO_DIRECTION_REGISTER            = FPGA_DIO_REGISTER_BASE + 0x4
    FPGA_DIO_INPUT_REGISTER                = FPGA_DIO_REGISTER_BASE + 0x8
    FPGA_DIO_OUTPUT_REGISTER               = FPGA_DIO_REGISTER_BASE + 0xC
    FPGA_DIO_SOURCE_REGISTER               = FPGA_DIO_REGISTER_BASE + 0x10
    FPGA_DIO_RADIO_SOURCE_REGISTER         = FPGA_DIO_REGISTER_BASE + 0x14
    FPGA_DIO_INTERFACE_DIO_SELECT_REGISTER = FPGA_DIO_REGISTER_BASE + 0x18
    FPGA_DIO_OVERRIDE_REGISTER             = FPGA_DIO_REGISTER_BASE + 0x1C
    FPGA_DIO_SW_DIO_CONTROL_REGISTER       = FPGA_DIO_REGISTER_BASE + 0x20

    FULL_DIO_FPGA_COMPAT = (7, 5)
    FULL_SPI_FPGA_COMPAT = (7, 7)

    # DIO registers addresses in CPLD
    CPLD_DIO_DIRECTION_REGISTER = 0x30

    # GPIO attributes
    X4XX_GPIO_BANKS        = ["GPIO0", "GPIO1"]
    X4XX_GPIO_SRC_PS       = "PS"
    X4XX_GPIO_SRC_MPM      = "MPM"
    X4XX_GPIO_SRC_USER_APP = "USER_APP"
    X4XX_GPIO_SRC_RADIO = [
        ["DB0_RF0", "DB0_RF1"],
        ["DB1_RF0", "DB1_RF1"]
    ]
    X4XX_GPIO_SPI_SRC_RADIO = [["DB0_SPI"], ["DB1_SPI"]]
    X4XX_GPIO_WIDTH = 12
    # pylint: enable=bad-whitespace

    class _PortMapDescriptor:
        """
        Helper class to hold port mapping relevant information
        """
        def __init__(self, name, pin_names, pin_map, first_pin):
            self.name = name
            self.pin_names = pin_names
            self.map = pin_map
            self.first_pin = first_pin


    class _PortControl:
        """
        Helper class for controlling ports on the I2C expander
        """
        def __init__(self, port):
            assert port in DioControl.DIO_PORTS
            prefix = "DIOAUX_%s" % port

            self.enable = Gpio('%s_ENABLE' % prefix, Gpio.OUTPUT)
            self.en_3v3 = Gpio('%s_3V3' % prefix, Gpio.OUTPUT)
            self.en_2v5 = Gpio('%s_2V5' % prefix, Gpio.OUTPUT)
            self.ext_pwr = Gpio('%s_ENABLE_EXT_PWR' % prefix, Gpio.OUTPUT)
            self.power_good = Gpio('%s_PWR_GOOD' % prefix, Gpio.INPUT)


    class _GpioReg:
        """
        Helper class for manipulating GPIO source configuration registers of this form:
        [31..28]: Reserved
        [27..16]: Port B
        [15..12]: Reserved
        [11..0]:  Port A
        """

        def __init__(self, dio_control, bank, offset):
            self.offset = offset
            self.value = dio_control.mboard_regs.peek32(offset)
            self.mboard_regs = dio_control.mboard_regs
            self.bank_offset = 0 if bank == dio_control.X4XX_GPIO_BANKS[0] else 16

        def set_pin(self, pin_index, value):
            self.value &= ~(1 << (pin_index + self.bank_offset))
            self.value |= (value << (pin_index + self.bank_offset))

        def get_pin(self, pin_index):
            return bool((self.value >> (pin_index + self.bank_offset)) & 0x1)

        def save(self):
            self.mboard_regs.poke32(self.offset, self.value)


    def __init__(self, mboard_regs, mboard_cpld, log, dboards):
        """
        Initializes access to hardware components as well as creating known
        port mappings
        :param log: logger to be used for output
        """
        self.log = log.getChild(self.__class__.__name__)
        self.port_control = {port: self._PortControl(port) for port in self.DIO_PORTS}
        self.mboard_regs = mboard_regs
        self.mboard_cpld = mboard_cpld
        if self.mboard_regs.get_compat_number() < self.FULL_DIO_FPGA_COMPAT:
            self.log.warning("DIO board does not support the full feature set.")
        if self.mboard_regs.get_compat_number() < self.FULL_SPI_FPGA_COMPAT:
            self.log.warning("DIO board does not support SPI.")
        # initialize port mapping for HDMI and DIO
        self.port_mappings = {}
        self.mapping = None
        self.port_mappings[self.HDMI_MAP_NAME] = self._PortMapDescriptor(
            self.HDMI_MAP_NAME, self.HDMI_PIN_NAMES,
            self.HDMI_PORT_MAP, self.HDMI_FIRST_PIN)
        self.port_mappings[self.DIO_MAP_NAME] = self._PortMapDescriptor(
            self.DIO_MAP_NAME, self.DIO_PIN_NAMES,
            self.DIO_PORT_MAP, self.DIO_FIRST_PIN)
        self.set_port_mapping(self.HDMI_MAP_NAME)
        self.log.trace("Spawning DIO fault monitors...")
        self._tear_down_monitor = Event()
        self._dio_fault = {
            "PORTA": Value('b', 0),
            "PORTB": Value('b', 0),
        }
        self._dio0_fault_monitor = Process(
            target=self._monitor_dio_fault,
            args=('A', "DIO_INT0", self._tear_down_monitor, self._dio_fault["PORTA"])
        )
        self._dio1_fault_monitor = Process(
            target=self._monitor_dio_fault,
            args=('B', "DIO_INT1", self._tear_down_monitor, self._dio_fault["PORTB"])
        )
        signal.signal(signal.SIGINT, self._monitor_int_handler)
        self._dio0_fault_monitor.start()
        self._dio1_fault_monitor.start()

        # Init GPIO sources
        gpio_srcs = [
            self.X4XX_GPIO_SRC_PS,
            self.X4XX_GPIO_SRC_MPM,
            self.X4XX_GPIO_SRC_USER_APP
        ]
        for dboard in dboards:
            gpio_srcs.extend(self.X4XX_GPIO_SRC_RADIO[dboard.slot_idx])
            # Only add SPI if FPGA version is high enough
            if self.mboard_regs.get_compat_number() >= self.FULL_SPI_FPGA_COMPAT:
                gpio_srcs.extend(self.X4XX_GPIO_SPI_SRC_RADIO[dboard.slot_idx])

        self._gpio_srcs = {
            gpio_bank : gpio_srcs for gpio_bank in self.X4XX_GPIO_BANKS
        }

        self.log.debug(f"Found the following GPIO sources: {', '.join(gpio_srcs)}")

        self._current_voltage_level = {
            "PORTA": "3V3",
            "PORTB": "3V3",
        }
        self.set_voltage_level("PORTA", "3V3")
        self.set_voltage_level("PORTB", "3V3")

    def _monitor_dio_fault(self, dio_port, fault, tear_down, fault_state):
        """
        Monitor the DIO_INT lines to detect an external power fault.
        If there is a fault, turn off external power.
        """
        self.log.trace("Launching monitor loop...")
        fault_line = Gpio(fault, Gpio.FALLING_EDGE)
        while True:
            try:
                if fault_line.event_wait():
                    # If we saw a fault, disable the external power
                    self.log.warning(
                        f"DIO fault occurred on port {dio_port} - "
                        "turning off external power")
                    self.set_external_power(dio_port, 0)
                    fault_state.value = 1
            # If the event wait gets interrupted because we are trying to tear
            # down then stop the monitoring process. If not, keep monitoring
            except InterruptedError:
                pass
            if tear_down.is_set():
                break

    def _monitor_int_handler(self, _signum, _frame):
        """
        If we see an expected interrupt signal, mark the DIO fault monitors
        for tear down.
        """
        self._tear_down_monitor.set()

    # --------------------------------------------------------------------------
    # Helper methods
    # --------------------------------------------------------------------------
    def _delinearize_pin(self, port, pin):
        """
        Converts a pin from the compacted range [0-11] to the expanded range
        given by the current mapping (0-19 for HDMI, 0-11 for DIO). Note that
        this does not perform the mapping to the register bit, that is
        handled by _map_to_register_bit.
        :param port: port to delinearize
        :param pin: pin (in compacted [0-11] form)
        :return: pin in appropriate range for the current mapping
        """
        port = self._normalize_port_name(port)
        pins = sorted(self.mapping.map[port])
        return pins[pin]

    def _map_to_register_bit(self, port, pin, lift_portb = True):
        """
        Maps a pin denoted in current mapping scheme to a corresponding bit in
        the register map.
        :param port: port to do the mapping on
        :param pin: pin (in current mapping scheme)
        :return: bit position in register map
        :raises RuntimeError: pin is not in range of current mapping scheme
                              or not user assignable.
        """
        assert isinstance(pin, int)
        port = self._normalize_port_name(port)
        first_pin = self.mapping.first_pin
        last_pin = first_pin + len(self.mapping.pin_names) - 1
        port_map = self.mapping.map[port]

        if not first_pin <= pin <= last_pin:
            raise RuntimeError("Pin must be in range [%d,%d]. Given pin: %d" %
                               (first_pin, last_pin, pin))
        if pin not in port_map:
            raise RuntimeError("Pin %d (%s) is not a user assignable pin." %
                               (pin,
                                self.mapping.pin_names[pin - first_pin]))

        # map pin back to register bit
        bit = port_map.index(pin)
        if lift_portb:
            # lift register bit up by PORT_BIT_SIZE for port b
            bit = bit if port == self.DIO_PORTS[0] else bit + self.PORT_BIT_SIZE
        return bit

    def _calc_register_value(self, register, port, pin, value):
        """
        Recalculates register value.

        Current register state is read and the bit that corresponds to the
        values given by port and pin is determined. The register content is
        changed at position of bit to what is given by value.

        Note: This routine only reads the current and calculates the new
              register value. It is up to the callee to set the register value.
        :param register: Address of the register value to recalculate
        :param port:     port associated with pin
        :param pin:      pin to change (will be mapped to bit according to
                         current mapping scheme an given port)
        :param value:    new bit value to set
        :return:         new register value.
        """
        assert value in [0, 1]

        content = self.mboard_regs.peek32(register)
        bit = self._map_to_register_bit(port, pin)
        content = (content | 1 << bit) if value == 1 else (content & ~(1 << bit))
        return content

    def _set_pin_values(self, port, values, set_method):
        """
        Helper method to assign multiple pins in one call.
        :param port: Port to set pins on
        :param values: New pin assignment represented by an integer. Each bit of
                       values corresponds to a pin on board according to current
                       mapping scheme. Bits that do not correspond to a pin in
                       the current mapping scheme are skipped.
        :param set_method: method to be used to set/unset a pin. Signature of
                           set_method is (port, pin).
        """
        first_pin = self.mapping.first_pin
        port = self._normalize_port_name(port)
        for i, _ in enumerate(self.mapping.pin_names):
            if i + first_pin in self.mapping.map[port]:
                set_method(port, i + first_pin, int(values & 1 << i != 0))

    # --------------------------------------------------------------------------
    # Helper to convert abbreviations to constants defined in DioControl
    # --------------------------------------------------------------------------

    def _normalize_mapping(self, mapping):
        """
        Map name to one of the key in self.port_mappings.
        :param mapping: mapping name or any abbreviation by removing letters
                        from the end of the name
        :return: Key found for mapping name
        :raises RuntimeError: no matching mapping could be found
        """
        assert isinstance(mapping, str)
        mapping = mapping.upper()
        mapping_names = self.port_mappings.keys()
        try:
            # search for abbr of mapping in mapping names
            index = [
                re.match("^%s" % mapping, name) is not None for name in mapping_names
            ].index(True)
            return list(self.port_mappings.keys())[index]
        except ValueError:
            raise RuntimeError("Mapping %s not found in %s" % (mapping, mapping_names))

    def _normalize_port_name(self, name):
        """
        Map port name to the normalized form of self.DIO_PORTS
        :param name: port name or abbreviation with A or B, case insensitive
        :return: normalized port name
        :raises RuntimeError: name could not be normalized
        """
        assert isinstance(name, str)
        gpio0_names = (self.DIO_PORTS[0], self.X4XX_GPIO_BANKS[0], "A")
        gpio1_names = (self.DIO_PORTS[1], self.X4XX_GPIO_BANKS[1], "B")
        if name.upper() not in gpio0_names + gpio1_names:
            raise RuntimeError("Could not map %s to port name" % name)
        return self.DIO_PORTS[0] if name.upper() in gpio0_names \
            else self.DIO_PORTS[1]

    # --------------------------------------------------------------------------
    # Helper to format status output
    # --------------------------------------------------------------------------

    def _get_port_voltage(self, port):
        """
        Format voltage table cell value.
        """
        port_control = self.port_control[port]
        result = ""
        if port_control.enable.get() == 0:
            result += self.DIO_VOLTAGE_LEVELS[0]
        elif port_control.en_2v5.get() == 1:
            result += self.DIO_VOLTAGE_LEVELS[2]
        elif port_control.en_3v3.get() == 1:
            result += self.DIO_VOLTAGE_LEVELS[3]
        else:
            result += self.DIO_VOLTAGE_LEVELS[1]
        result += " - PG:"
        result += "YES" if port_control.power_good.get() else "NO"
        result += " - EXT:"
        result += "ON" if port_control.ext_pwr.get() else "OFF"
        return result

    def _get_voltage(self):
        """
        Format voltage table cells
        """
        return [self._get_port_voltage(port) for port in self.DIO_PORTS]

    def _format_register(self, port, content):
        """
        Format a port value according to current mapping scheme. Pins are
        grouped by 4. Pins which are not user assignable are marked with a dot.
        :param content: register content
        :return: register content as pin assignment according to current
                 mapping scheme
        """
        result = ""
        first_pin = self.mapping.first_pin
        pin_names = self.mapping.pin_names
        mapping = self.mapping.map[port]
        for i, _ in enumerate(pin_names):
            if i % 4 == 0 and i > 0:
                result = " " + result
            if i + first_pin in mapping:
                result = str(int(content & (1 << mapping.index(i + first_pin)) != 0)) + result
            else:
                result = "." + result
        return result

    def _format_registers(self, content):
        """
        Formats register content for port A and B
        :param content:
        :return:
        """
        port_a = content & self.PORT_USED_BITS_MASK
        port_b = (content >> self.PORT_BIT_SIZE) & self.PORT_USED_BITS_MASK
        return [self._format_register(self.DIO_PORTS[0], port_a),
                self._format_register(self.DIO_PORTS[1], port_b)]

    def _format_row(self, values, fill=" ", delim="|"):
        """
        Format a table row with fix colums widths. Generates row spaces using
        value list with empty strings and "-" as fill and "+" as delim.
        :param values: cell values (list of three elements)
        :param fill: fill character to use (space by default)
        :param delim: delimiter character between columns
        :return: formated row
        """
        col_widths = [14, 25, 25]
        return delim.join([
            fill + values[i].ljust(width - len(fill), fill)
            for i, width in enumerate(col_widths)
        ]) + "\n"

    def get_gpio_banks(self):
        """
        Returns a list of GPIO banks over which MPM has any control
        """
        if self.mboard_regs.get_compat_number() < self.FULL_DIO_FPGA_COMPAT:
            return []

        return self.X4XX_GPIO_BANKS

    def get_gpio_srcs(self, bank: str):
        """
        Return a list of valid GPIO sources for a given bank
        """
        if self.mboard_regs.get_compat_number() < self.FULL_DIO_FPGA_COMPAT:
            return ["MPM"]

        assert bank in self.get_gpio_banks(), f"Invalid GPIO bank: {bank}"
        return self._gpio_srcs[bank]

    def get_gpio_src(self, bank: str):
        """
        Return the currently selected GPIO source for a given bank. The return
        value is a list of strings. The length of the vector is identical to
        the number of controllable GPIO pins on this bank. USER_APP is a GPIO
        source that can be used in custom FPGA designs (e.g. LabView binary uses
        this pin source).
        """
        if self.mboard_regs.get_compat_number() < self.FULL_DIO_FPGA_COMPAT:
            return []

        assert bank in self.get_gpio_banks(), f"Invalid GPIO bank: {bank}"

        master_reg = self._GpioReg(self, bank, self.FPGA_DIO_MASTER_REGISTER)
        source_reg = self._GpioReg(self, bank, self.FPGA_DIO_SOURCE_REGISTER)
        radio_source_reg = self._GpioReg(self, bank, self.FPGA_DIO_RADIO_SOURCE_REGISTER)
        interface_select_reg = self._GpioReg(
            self, bank, self.FPGA_DIO_INTERFACE_DIO_SELECT_REGISTER)
        override_reg = self._GpioReg(self, bank, self.FPGA_DIO_OVERRIDE_REGISTER)
        sw_control_reg = self._GpioReg(self, bank, self.FPGA_DIO_SW_DIO_CONTROL_REGISTER)

        def get_gpio_src_i(gpio_pin_index):
            """
            Return the current source given a pin index.
            """
            if source_reg.get_pin(gpio_pin_index):
                if override_reg.get_pin(gpio_pin_index):
                    db = int(interface_select_reg.get_pin(gpio_pin_index))
                    return f"DB{db}_SPI"
                else:
                    db = int(radio_source_reg.get_pin(gpio_pin_index))
                    # Note that we can't distinguish between RF0 and RF1
                    return f"DB{db}_RF0"
            else:
                if master_reg.get_pin(gpio_pin_index):
                    if sw_control_reg.get_pin(gpio_pin_index):
                        return self.X4XX_GPIO_SRC_PS
                    else:
                        return self.X4XX_GPIO_SRC_MPM
                else:
                    return self.X4XX_GPIO_SRC_USER_APP

        return [
            get_gpio_src_i(
                self._map_to_register_bit(
                    bank,
                    self._delinearize_pin(bank, i),
                    False
                )
            )
            for i in range(self.X4XX_GPIO_WIDTH)
        ]

    def set_gpio_src(self, bank: str, src):
        """
        Set the GPIO source for a given bank.
        src input is big-endian
        Usage:
        > set_gpio_src <bank> <srcs>
        > set_gpio_src GPIO0 PS DB1_RF0 PS PS MPM PS PS PS MPM USER_APP PS
        """
        if self.mboard_regs.get_compat_number() < self.FULL_DIO_FPGA_COMPAT:
            # Older FPGAs only have a single GPIO source selector
            master_reg = self._GpioReg(self, bank, self.FPGA_DIO_MASTER_REGISTER)
            for pin_index, src_name in enumerate(src):
                if src_name == "MPM":
                    master_reg.set_pin(pin_index, 1)
                else:
                    master_reg.set_pin(pin_index, 0)
            master_reg.save()
            return

        assert bank in self.get_gpio_banks(), f"Invalid GPIO bank: {bank}"
        assert len(src) == self.X4XX_GPIO_WIDTH, \
            f"Invalid number of GPIO sources! Expecting {self.X4XX_GPIO_WIDTH}, but got {len(src)}."

        for pin_index, src_name in enumerate(src):
            if src_name not in self.get_gpio_srcs(bank):
                raise RuntimeError(
                    f"Invalid GPIO source name `{src_name}' at bit position {pin_index}!")

        master_reg = self._GpioReg(self, bank, self.FPGA_DIO_MASTER_REGISTER)
        source_reg = self._GpioReg(self, bank, self.FPGA_DIO_SOURCE_REGISTER)
        radio_source_reg = self._GpioReg(self, bank, self.FPGA_DIO_RADIO_SOURCE_REGISTER)
        interface_select_reg = self._GpioReg(
            self, bank, self.FPGA_DIO_INTERFACE_DIO_SELECT_REGISTER)
        override_reg = self._GpioReg(self, bank, self.FPGA_DIO_OVERRIDE_REGISTER)
        sw_control_reg = self._GpioReg(self, bank, self.FPGA_DIO_SW_DIO_CONTROL_REGISTER)

        for pin_index, src_name in enumerate(src):
            pin_index = self._map_to_register_bit(
                bank,
                self._delinearize_pin(bank, pin_index),
                False
            )
            radio_srcs = [
                item for sublist in (self.X4XX_GPIO_SRC_RADIO + self.X4XX_GPIO_SPI_SRC_RADIO) for item in sublist]
            if src_name in radio_srcs:
                source_reg.set_pin(pin_index, 1)
                slot = int(src_name[2])
                if src_name.endswith("_SPI"):
                    override_reg.set_pin(pin_index, 1)
                    interface_select_reg.set_pin(pin_index, slot)
                else:
                    channel = int(src_name[6])
                    override_reg.set_pin(pin_index, 0)
                    radio_source_reg.set_pin(pin_index, slot)
            else:
                source_reg.set_pin(pin_index, 0)
                if src_name in (self.X4XX_GPIO_SRC_PS, self.X4XX_GPIO_SRC_MPM):
                    master_reg.set_pin(pin_index, 1)
                    sw_control_reg.set_pin(
                        pin_index, int(src_name == self.X4XX_GPIO_SRC_PS))
                else:
                    master_reg.set_pin(pin_index, 0)

        master_reg.save()
        source_reg.save()
        radio_source_reg.save()
        interface_select_reg.save()
        override_reg.save()
        sw_control_reg.save()

    # --------------------------------------------------------------------------
    # Public API
    # --------------------------------------------------------------------------

    def tear_down(self):
        """
        Mark the DIO monitoring processes for tear down and terminate the processes
        """
        self._tear_down_monitor.set()
        self._dio0_fault_monitor.terminate()
        self._dio1_fault_monitor.terminate()
        self._dio0_fault_monitor.join(3)
        self._dio1_fault_monitor.join(3)
        if self._dio0_fault_monitor.is_alive() or \
           self._dio1_fault_monitor.is_alive():
            self.log.warning("DIO monitor didn't exit properly")

    def set_port_mapping(self, mapping):
        """
        Change the port mapping to mapping. Mapping must denote a mapping found
        in this.port_mappings.keys() or any abbreviation allowed by
        _normalize_port_mapping. The mapping does not change the status of the
        FPGA registers. It only changes the status display and the way calls
        to set_pin_<register_name>(s) are interpreted.
        :param mapping: new mapping to be used
        :raises RuntimeError: mapping could not be found
        """
        assert isinstance(mapping, str)
        map_name = self._normalize_mapping(mapping)
        if not map_name in self.port_mappings.keys():
            raise RuntimeError("Could not map %s to port mapping" % mapping)
        self.mapping = self.port_mappings[map_name]

    def set_pin_direction(self, port, pin, value=1):
        """
        Set direction pin of a port. The direction pin decides whether the DIO
        external pin is used as an output (write - value is 1) or input (read -
        value is 0). To change the pin value the current register content is
        read first and modified before it is written back, so the register must
        be readable.
        Besides the FPGA register map, the CPLD register map is also written. To
        prevent the internal line to be driven by FGPA and DIO board at the same
        time the CPLD register is written first if the direction will become an
        output. If direction will become an input the FPGA register is written
        first.
        :param port: port to change direction assignment on
        :param pin: pin to change
        :param value: desired pin value
        """
        content = self._calc_register_value(self.FPGA_DIO_DIRECTION_REGISTER,
                                            port, pin, value)
        # When setting direction pin, order matters. Always switch the component
        # first that will get the driver disabled.
        # This ensures that there wont be two drivers active at a time.
        if value == 1:  # FPGA is driver => write DIO register first
            self.mboard_cpld.poke32(self.CPLD_DIO_DIRECTION_REGISTER, content)
            self.mboard_regs.poke32(self.FPGA_DIO_DIRECTION_REGISTER, content)
        else:  # DIO is driver => write FPGA register first
            self.mboard_regs.poke32(self.FPGA_DIO_DIRECTION_REGISTER, content)
            self.mboard_cpld.poke32(self.CPLD_DIO_DIRECTION_REGISTER, content)
        # Read back values to ensure registers are in sync
        cpld_content = self.mboard_cpld.peek32(self.CPLD_DIO_DIRECTION_REGISTER)
        mbrd_content = self.mboard_regs.peek32(self.FPGA_DIO_DIRECTION_REGISTER)
        if not ((cpld_content == content) and (mbrd_content == content)):
            raise RuntimeError("Direction register content mismatch. Expected:"
                               "0x%0.8X, CPLD: 0x%0.8X, FPGA: 0x%0.8X." %
                               (content, cpld_content, mbrd_content))

    def set_pin_directions(self, port, values):
        """
        Set all direction pins of a port at once using a bit mask.
        :param port: port to change direction pin assignment
        :param values: New pin assignment represented by an integer. Each bit of
                       values corresponds to a pin on board according to current
                       mapping scheme. Bits that do not correspond to a pin in
                       the current mapping scheme are skipped.
        """
        self._set_pin_values(port, values, self.set_pin_direction)

    def set_pin_output(self, port, pin, value=1):
        """
        Set output value of a pin on a port. Setting this value only takes
        effect if the direction of the corresponding pin of this port is set
        accordingly. To change the pin value the current register content is
        read first and modified before it is written back, so the register must
        be readable.
        :param port: port to change output assignment on
        :param pin: pin to change
        :param value: desired pin value
        """
        content = self._calc_register_value(self.FPGA_DIO_OUTPUT_REGISTER,
                                            port, pin, value)
        self.mboard_regs.poke32(self.FPGA_DIO_OUTPUT_REGISTER, content)

    def set_pin_outputs(self, port, values):
        """
        Set all output pins of a port at once using a bit mask.
        :param port: port to change direction pin assignment
        :param values: New pin assignment represented by an integer. Each bit of
                       values corresponds to a pin on board according to current
                       mapping scheme. Bits that do not correspond to a pin in
                       the current mapping scheme are skipped.
        """
        self._set_pin_values(port, values, self.set_pin_output)

    def get_pin_input(self, port, pin):
        """
        Returns the input pin value of a port.
        If the pin is not assignable in the current mapping None is returned.

        :param port: port to read pin value from
        :param pin: pin value to read
        :returns: actual pin value or None if pin is not assignable
        """
        port = self._normalize_port_name(port)

        register = self.mboard_regs.peek32(self.FPGA_DIO_INPUT_REGISTER)
        if port == self.DIO_PORTS[1]:
            register = register >> self.PORT_BIT_SIZE
        register &= self.PORT_USED_BITS_MASK

        mapping = self.mapping.map[port]
        if not pin in mapping:
            raise RuntimeError("Pin %d (%s) is not a user readable pin." %
                               (pin,
                                self.mapping.pin_names[pin - self.mapping.first_pin]))
        return 0 if (register & (1 << mapping.index(pin)) == 0) else 1

    def get_pin_inputs(self, port):
        """
        Returns a bit mask of all pins for the given port.

        :param port: port to read input pins from
        :returns: Bit map of input pins, each bit of pins corresponds to a pin
                  on board according to current mapping scheme. Unused pins
                  stay zero
        """
        result = 0
        first_pin = self.mapping.first_pin
        pin_names = self.mapping.pin_names
        port = self._normalize_port_name(port)
        mapping = self.mapping.map[port]
        for i, _name in enumerate(pin_names):
            if i + first_pin in mapping:
                if self.get_pin_input(port, i + first_pin):
                    result |= 1 << i
        return result

    def set_voltage_level(self, port, level):
        """
        Change voltage level of a port. This is how EN_<port>, EN_<port>_2V5 and
        EN_<port>_3V3 are set according to level::
            level EN_<port>   EN_<port>_2V5   EN_<port>_3V3
            off       0            0               0
            1V8       1            0               0
            2V5       1            1               0
            3V3       1            0               1
        If level is set to anything other than off this method waits for
        <port>_PG to go high. Waiting stops as soon as <port>_PG goes high or
        a timeout of 1s occurs.
        Note: All pins are set to zero first before the new level is applied.
        :param port: port to change power level for
        :param level: new power level
        :raises RuntimeError: power good pin did not go high
        """
        port = self._normalize_port_name(port)
        level = level.upper()
        assert port in self.DIO_PORTS
        assert level in self.DIO_VOLTAGE_LEVELS
        port_control = self.port_control[port]

        self._current_voltage_level[port] = level

        port_control.enable.set(0)
        port_control.en_2v5.set(0)
        port_control.en_3v3.set(0)
        if level == self.DIO_VOLTAGE_LEVELS[2]:
            port_control.en_2v5.set(1)
        elif level == self.DIO_VOLTAGE_LEVELS[3]:
            port_control.en_3v3.set(1)

        # wait for <port>_PG to go high
        if not level == self.DIO_VOLTAGE_LEVELS[0]: # off
            port_control.enable.set(1)
            if not poll_with_timeout(
                    lambda: port_control.power_good.get() == 1, 1000, 10):
                raise RuntimeError(
                    "Power good pin did not go high after power up")

    def get_voltage_level(self, port):
        """
        Returns the current GPIO voltage level as set by set_voltage_level
        """
        port = self._normalize_port_name(port)
        assert port in self.DIO_PORTS
        return self._current_voltage_level[port]

    def set_external_power(self, port, value):
        """
        Change EN_EXT_PWR_<port> to value.
        :param port: port to change external power level for
        :param value: 1 to enable external power, 0 to disable
        :raise RuntimeError: port or pin value could not be mapped
        """
        port = self._normalize_port_name(port)
        value = int(value)
        assert value in (0, 1)
        assert port in self.DIO_PORTS
        self.port_control[port].ext_pwr.set(value)
        self._dio_fault[port].value = 0

    def get_external_power_state(self, port):
        """
        Returns the current state of the external power supply.

        Usage:
        > get_external_power_state PORTA
        """
        port = self._normalize_port_name(port)
        if self._dio_fault[port].value == 1:
            return "FAULT"
        if self.port_control[port].ext_pwr.get() == 1:
            return "ON"
        return "OFF"

    def get_supported_voltage_levels(self, port):
        """
        Returns the list of all supported voltage levels for the given port.
        Note that, currently, all ports support all voltage levels, so we
        simply validate that the given port name is valid.
        """
        _ = self._normalize_port_name(port)
        return ["OFF", "1V8", "2V5", "3V3"]

    def status(self):
        """
        Build a full status string for the DIO AUX board, including
        I2C pin states and register content in a human readable form.
        :return: board status
        """
        result = "\n" \
            + self._format_row(["%s mapping" % self.mapping.name, self.DIO_PORTS[0], self.DIO_PORTS[1]]) \
            + self._format_row(["", "", ""], "-", "+") \
            + self._format_row(["voltage"] + self._get_voltage()) \
            + self._format_row(["", "", ""], "-", "+")

        register = self.mboard_regs.peek32(self.FPGA_DIO_MASTER_REGISTER)
        result += self._format_row(["master"] + self._format_registers(register))

        register = self.mboard_regs.peek32(self.FPGA_DIO_DIRECTION_REGISTER)
        result += self._format_row(["direction"] + self._format_registers(register))

        result += self._format_row(["", "", ""], "-", "+")

        register = self.mboard_regs.peek32(self.FPGA_DIO_OUTPUT_REGISTER)
        result += self._format_row(["output"] + self._format_registers(register))

        register = self.mboard_regs.peek32(self.FPGA_DIO_INPUT_REGISTER)
        result += self._format_row(["input"] + self._format_registers(register))
        return result

    def debug(self):
        """
        Create a debug string containing the FPGA register maps. The CPLD
        direction register is not part of the string as the DioControl maintains
        it in sync with the FPGA direction register.
        :return: register states for debug purpose in human readable form.
        """
        master = format(self.mboard_regs.peek32(self.FPGA_DIO_MASTER_REGISTER), "032b")
        direction = format(self.mboard_regs.peek32(self.FPGA_DIO_DIRECTION_REGISTER), "032b")
        output = format(self.mboard_regs.peek32(self.FPGA_DIO_OUTPUT_REGISTER), "032b")
        input_reg = format(self.mboard_regs.peek32(self.FPGA_DIO_INPUT_REGISTER), "032b")
        return "\nmaster:    " + " ".join(re.findall('....', master)) + "\n" + \
            "direction: " + " ".join(re.findall('....', direction)) + "\n" + \
            "output:    " + " ".join(re.findall('....', output)) + "\n" + \
            "input:     " + " ".join(re.findall('....', input_reg))

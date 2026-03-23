"""Copyright 2026 Ettus Research, a National Instruments Brand.

SPDX-License-Identifier: GPL-3.0-or-later

Access to GPIOs via libgpiod.
"""

import contextlib
import os

import gpiod


# From bindings/python/examples/find_line_by_name.py in libgpiod repo
def generate_gpio_chips():
    """Generator function which returns the gpiochip paths."""
    for entry in os.scandir("/dev/"):
        if gpiod.is_gpiochip_device(entry.path):
            yield entry.path


# From bindings/python/examples/find_line_by_name.py in libgpiod repo; slightly adopted
def find_line_by_name(line_name):
    """Find the chip paths and the offset of a given GPIO line name.

    Names are not guaranteed unique, so this finds the first line with
    the given name.
    """
    for path in generate_gpio_chips():
        with gpiod.Chip(path) as chip:
            try:
                offset = chip.line_offset_from_id(line_name)
                return path, offset
            except OSError:
                # An OSError is raised if the name is not found.
                continue
    raise RuntimeError("line '{}' not found".format(line_name))


class Gpio:
    """Class for accessing a named GPIO line via libgpiod.

    The GPIO line is requested when the object is created.
    """

    INPUT = gpiod.line.Direction.INPUT
    OUTPUT = gpiod.line.Direction.OUTPUT
    FALLING_EDGE = gpiod.line.Edge.FALLING

    def __init__(self, name, direction=INPUT, default_val=None):
        """Create new Gpio object.

        :param name: The name of the GPIO line to access
        :param direction: The direction to request for the GPIO line
        :param default_val: The default value to assume for an output GPIO
        """
        self._direction = direction
        self._chip_path, self._offset = find_line_by_name(name)
        self._out_value = False

        if default_val is not None and direction == Gpio.OUTPUT:
            self.set(default_val)

    @contextlib.contextmanager
    def request_gpio(self):
        """Context manager for a GPIO line.

        :param line: The GPIO line to request
        :param direction: The direction to requested for line
        """
        request = gpiod.request_lines(
            self._chip_path,
            consumer="mpm",
            config={self._offset: gpiod.LineSettings(direction=self._direction)},
        )
        try:
            yield request
        finally:
            request.release()

    def get(self):
        """Read the value of this GPIO.

        :return: The value of the GPIO line.
        """
        if self._direction == self.OUTPUT:
            return self._out_value

        with self.request_gpio() as request:
            return bool(request.get_value(self._offset).value)

    def set(self, value):
        """Set the value of this GPIO.

        :param value: The value to set the GPIO line to.
        """
        with self.request_gpio() as request:
            request.set_value(self._offset, gpiod.line.Value(value))
            self._out_value = bool(value)

    def event_wait(self, timeout=1):
        """Wait for an event to happen on this line.

        :param timeout: The time to wait for an event in seconds.
                The timeout has to be passed in full seconds as an int.
        :return: True if an event happened, False if the wait timed out.
        """
        with self.request_gpio() as request:
            try:
                retval = request.wait_edge_events(timeout=timeout)
            except KeyboardInterrupt:
                retval = False
        return retval

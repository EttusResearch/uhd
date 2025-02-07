"""Copyright 2020 Ettus Research, a National Instruments Brand.

SPDX-License-Identifier: GPL-3.0-or-later

Access to GPIOs via libgpiod.
"""

import contextlib

import gpiod


@contextlib.contextmanager
def request_gpio(line, direction):
    """Context manager for a GPIO line.

    :param line: The GPIO line to request
    :param direction: The direction to requested for line
    """
    line.request(consumer="mpm", type=direction)
    try:
        yield line
    finally:
        line.release()


class Gpio:
    """Class for accessing a named GPIO line via libgpiod.

    The GPIO line is requested when the object is created.
    """

    INPUT = gpiod.LINE_REQ_DIR_IN
    OUTPUT = gpiod.LINE_REQ_DIR_OUT
    FALLING_EDGE = gpiod.LINE_REQ_EV_FALLING_EDGE

    def __init__(self, name, direction=INPUT, default_val=None):
        """Create new Gpio object.

        :param name: The name of the GPIO line to access
        :param direction: The direction to request for the GPIO line
        :param default_val: The default value to assume for an output GPIO
        """
        self._direction = direction
        self._line = gpiod.find_line(name)
        self._out_value = False
        if self._line is None:
            raise RuntimeError("failed to find gpio with name %s" % name)

        if default_val is not None and direction == Gpio.OUTPUT:
            self.set(default_val)

    def get(self):
        """Read the value of this GPIO.

        :return: The value of the GPIO line.
        """
        if self._direction == self.OUTPUT:
            return self._out_value

        with request_gpio(self._line, self._direction) as gpio:
            return bool(gpio.get_value())

    def set(self, value):
        """Set the value of this GPIO.

        :param value: The value to set the GPIO line to.
        """
        with request_gpio(self._line, self._direction) as gpio:
            gpio.set_value(int(value))
            self._out_value = bool(value)

    def event_wait(self, timeout=1):
        """Wait for an event to happen on this line.

        :param timeout: The time to wait for an event in seconds.
                The timeout has to be passed in full seconds as an int.
        :return: True if an event happened, False if the wait timed out.
        """
        with request_gpio(self._line, self._direction) as gpio:
            return gpio.event_wait(sec=timeout)

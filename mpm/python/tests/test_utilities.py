#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" Utility classes to facilitate unit testing """

import queue

class MockRegsIface(object):
    """
    Mocks the interaction with a register interface by returning
    values from an ic_reg_map
    """
    def __init__(self, register_map):
        self.map = register_map
        self.recent_vals = {}
        self.next_vals = {}

    def peek32(self, addr):
        """
        Reads either
        a. the default value (if no values were queued)
        or
        b. the next value queued for this register
        """
        if (addr in self.next_vals) and (not self.next_vals[addr].empty()):
            return self.next_vals[addr].get_nowait()
        else:
            return self.map.get_reg(addr)

    def poke32(self, addr, value):
        """
        Saves a new value to the given register and stores a history
        of all values previously set for that register.
        """
        self.map.set_reg(addr, value)

        # Store written value in a list
        if addr in self.recent_vals:
            self.recent_vals[addr].append(value)
        else:
            self.recent_vals[addr] = [value]

    def get_recent_vals(self, addr):
        """
        Returns the past values written to a given address.
        Useful for validating HW interaction
        """
        return self.recent_vals.get(addr, [])

    def set_next_vals(self, addr, vals):
        """
        Sets a list of the next values to be read from the
        corresponding register address.
        Useful for mocking HW interaction.
        """
        if addr not in self.next_vals:
            self.next_vals[addr] = queue.Queue()
        for val in vals:
            self.next_vals[addr].put_nowait(val)

class MockLog(object):
    """
    Mocks logging functionality for testing purposes by putting log
    messages in a queue.
    """
    # The MockLog class is not currently implemented to be thread safe
    def __init__(self):
        self.error_log = queue.Queue()
        self.warning_log = queue.Queue()
        self.info_log = queue.Queue()
        self.trace_log = queue.Queue()
        self.debug_log = queue.Queue()

    def error(self, msg):
        self.error_log.put_nowait(msg)

    def warning(self, msg):
        self.warning_log.put_nowait(msg)

    def info(self, msg):
        self.info_log.put_nowait(msg)

    def trace(self, msg):
        self.trace_log.put_nowait(msg)

    def debug(self, msg):
        self.debug_log.put_nowait(msg)

    def clear_all(self):
        """ Clears all log queues """
        self.error_log.queue.clear()
        self.warning_log.queue.clear()
        self.info_log.queue.clear()
        self.trace_log.queue.clear()
        self.debug_log.queue.clear()

    def get_last_msg(self, log_level):
        """
        Gets the last message logged to a given queue. Will return an
        empty string if the queue is empty or throw an error if a queue
        of that log_level does not exist.
        """
        queue_name = log_level + '_log'
        if not hasattr(self, queue_name):
            raise RuntimeError("Log level {} does not exist in " \
                               "mock log".format(log_level))
        log_messages = getattr(self, queue_name)
        if log_messages.empty():
            return ''
        else:
            return log_messages.get_nowait()

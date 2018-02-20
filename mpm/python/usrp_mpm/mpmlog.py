#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
MPM Logging
"""

from __future__ import print_function
import copy
import logging
from logging import CRITICAL, ERROR, WARNING, INFO, DEBUG
from logging import handlers
import collections
from builtins import str

# Colors
BOLD = str('\033[1m')
RED = str('\x1b[31m')
YELLOW = str('\x1b[33m')
GREEN = str('\x1b[32m')
PINK = str('\x1b[35m')
GREY = str('\x1b[90m')
RESET = str('\x1b[0m')

# Additional log level
TRACE = 1

class ColorStreamHandler(logging.StreamHandler):
    """
    StreamHandler that prints colored output
    """
    def emit(self, record):
        """
        Prints record with colors.
        record is not modified.
        """
        record_ = copy.copy(record)
        levelno = record_.levelno
        if levelno >= CRITICAL:
            color = RED
        elif levelno >= ERROR:
            color = RED
        elif levelno >= WARNING:
            color = YELLOW
        elif levelno >= INFO:
            color = GREEN
        elif levelno >= DEBUG:
            color = PINK
        elif levelno >= TRACE:
            color = ''
        else: # NOTSET and anything else
            color = RESET
        record_.msg = BOLD + color + str(record_.msg) + RESET
        logging.StreamHandler.emit(self, record_)

class LossyQueueHandler(handlers.QueueHandler):
    """
    Like QueueHandler, except it'll try and keep the youngest, not oldest,
    entries.
    """
    def enqueue(self, record):
        """
        Replaces logging.handlers.QueueHandler.enqueue()
        """
        self.queue.appendleft(record)

class MPMLogger(logging.getLoggerClass()):
    """
    Extends the regular Python logging with level 'trace' (like UHD)
    """
    def __init__(self, *args, **kwargs):
        logging.Logger.__init__(self, *args, **kwargs)
        self.cpp_log_buf = None
        try:
            import usrp_mpm.libpyusrp_periphs as lib
            self.cpp_log_buf = lib.types.log_buf.make_singleton()
        except ImportError:
            pass
        from usrp_mpm import prefs
        self.py_log_buf = collections.deque(
            maxlen=prefs.get_prefs().getint('mpm', 'log_buf_size')
        )

    def trace(self, *args, **kwargs):
        """ Extends logging for super-high verbosity """
        self.log(TRACE, *args, **kwargs)

    def get_log_buf(self):
        """
        Return the contents of the logging queue, formatted as a list of
        dictionaries.
        """
        records = []
        # Note: This loop does not guarantee that all log items will be
        # returned. The while loop is set up to be bounded, and to return as
        # soon as is sensible.
        while len(records) < self.py_log_buf.maxlen:
            try:
                records.append(self.py_log_buf.pop())
            except IndexError:
                break
        return [{
            'name': record.name,
            'message': record.message,
            'levelname': record.levelname,
            'msecs': int(record.msecs),
        } for record in records]


LOGGER = None # Logger singleton
def get_main_logger(
        use_console=True,
        use_journal=False,
        use_logbuf=True,
        console_color=True,
        log_default_delta=0
    ):
    """
    Returns the top-level logger object. This is the only API call from this
    file that should be used outside.
    """
    global LOGGER
    if LOGGER is not None:
        return LOGGER
    logging.addLevelName(TRACE, 'TRACE')
    logging.setLoggerClass(MPMLogger)
    LOGGER = logging.getLogger('MPM')
    if use_console:
        console_handler = ColorStreamHandler() if console_color else logging.StreamHandler()
        console_formatter = logging.Formatter("[%(name)s] [%(levelname)s] %(message)s")
        console_handler.setFormatter(console_formatter)
        LOGGER.addHandler(console_handler)
    if use_journal:
        from systemd.journal import JournalHandler
        journal_handler = JournalHandler(SYSLOG_IDENTIFIER='usrp_hwd')
        journal_formatter = logging.Formatter('[%(levelname)s] [%(module)s] %(message)s')
        journal_handler.setFormatter(journal_formatter)
        LOGGER.addHandler(journal_handler)
    if use_logbuf:
        queue_handler = LossyQueueHandler(LOGGER.py_log_buf)
        LOGGER.addHandler(queue_handler)
    # Set default level:
    from usrp_mpm import prefs
    mpm_prefs = prefs.get_prefs()
    default_log_level = int(min(
        mpm_prefs.get_log_level() - log_default_delta * 10,
        CRITICAL
    ))
    default_log_level = max(TRACE, default_log_level - (default_log_level % 10))
    LOGGER.setLevel(default_log_level)
    # Connect to C++ logging:
    if LOGGER.cpp_log_buf is not None:
        lib_logger = LOGGER.getChild('lib')
        def log_from_cpp():
            " Callback for logging from C++ "
            log_level, component, message = LOGGER.cpp_log_buf.pop()
            if log_level:
                lib_logger.log(log_level, "[%s] %s",
                               component, message.strip())
        LOGGER.cpp_log_buf.set_notify_callback(log_from_cpp)
    # Flush errors stuck in the prefs module:
    log = LOGGER.getChild('prefs')
    for err_key, err_msg in mpm_prefs.get_log_errors():
        log.error('%s: %s', err_key, err_msg)
    return LOGGER

def get_logger(child_name):
    """
    Returns a child logger. Prior to calling this, get_main_logger() needs to
    have been called.
    """
    assert LOGGER is not None
    return get_main_logger().getChild(child_name)


if __name__ == "__main__":
    print("Testing logger: ")
    LOG = get_main_logger().getChild('test')
    LOG.setLevel(TRACE)
    LOG.trace("trace message")
    LOG.debug("debug message")
    LOG.info("info message")
    LOG.warning("warning message")
    LOG.error("error message")
    LOG.critical("critical message")

    LOG2 = get_main_logger()
    LOG3 = get_main_logger()
    assert LOG2 is LOG3


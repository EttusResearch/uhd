#
# Copyright 2017 Ettus Research (National Instruments)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
"""
MPM Logging
"""

from __future__ import print_function
import copy
import logging
from logging import CRITICAL, ERROR, WARNING, INFO, DEBUG
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

DEFAULT_LOG_LEVEL = TRACE

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

class MPMLogger(logging.getLoggerClass()):
    """
    Extends the regular Python logging with level 'trace' (like UHD)
    """
    def __init__(self, *args, **kwargs):
        logging.Logger.__init__(self, *args, **kwargs)

    def trace(self, *args, **kwargs):
        """ Extends logging for super-high verbosity """
        self.log(TRACE, *args, **kwargs)


LOGGER = None # Logger singleton
def get_main_logger(
        use_console=True,
        use_journal=False,
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
    # Set default level:
    default_log_level = int(min(
        DEFAULT_LOG_LEVEL - log_default_delta * 10,
        CRITICAL
    ))
    default_log_level = max(1, default_log_level - (default_log_level % 10))
    LOGGER.setLevel(default_log_level)
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


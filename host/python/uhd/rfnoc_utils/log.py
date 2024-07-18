"""RFNoC Utilities: Logging.

SPDX-License-Identifier: GPL-3.0-or-later
"""

import logging
import sys


class ColorFormatter(logging.Formatter):
    """Logging Formatter to add colors and icons."""

    RESET = 0
    BOLD = 1
    DIM = 2
    GREY = "38;20"
    BLACK = "30;20"
    YELLOW = "33;20"
    RED = "31;20"
    BRIGHTRED = 91
    BRIGHT = 99

    def c(colors):  # noqa -- should be staticmethod but that requires Python 3.10
        """Format escape sequence from list of colors."""
        return f"\x1b[{';'.join(str(c) for c in colors)}m"

    debug_color = c([DIM])
    info_color = c([BRIGHT, BOLD])
    warning_color = c([YELLOW])
    error_color = c([BRIGHTRED])
    crit_color = c([RED, BOLD])
    reset = c([RESET])

    FORMATS = {
        logging.DEBUG: debug_color + "[debug] %(message)s" + reset,
        logging.INFO: info_color + "%(message)s" + reset,
        logging.WARNING: warning_color + "⚠   %(message)s" + reset,
        logging.ERROR: error_color + "⛔   %(message)s" + reset,
        logging.CRITICAL: crit_color + "⛔   %(message)s" + reset,
    }

    def format(self, record):
        """Format a record the way we like it."""
        log_fmt = self.FORMATS.get(record.levelno)
        return logging.Formatter(log_fmt).format(record)


class SimpleFormatter(logging.Formatter):
    """Logging Formatter for non-interactive shells."""

    FORMATS = {
        logging.DEBUG: "[debug] %(message)s",
        logging.INFO: "%(message)s",
        logging.WARNING: "[warning] %(message)s",
        logging.ERROR: "[error] %(message)s",
        logging.CRITICAL: "[critical] %(message)s",
    }

    def format(self, record):
        """Format a record the way we like it."""
        log_fmt = self.FORMATS.get(record.levelno)
        return logging.Formatter(log_fmt).format(record)


def init_logging(color_mode="auto", log_level="info"):
    """Initialize the logging interface for RFNoC utilities."""
    use_color = (color_mode == "always") or (
        color_mode == "auto" and sys.__stdout__.isatty() and sys.__stderr__.isatty()
    )
    handler = logging.StreamHandler()
    handler.setFormatter(ColorFormatter() if use_color else SimpleFormatter())
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    logger.addHandler(handler)

    if log_level:
        logging.root.setLevel(log_level.upper())

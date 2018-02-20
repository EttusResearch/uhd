#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
MPM preferences management
"""

import configparser
from builtins import object
from usrp_mpm.mpmlog import TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL

# Store the global preferences object
_PREFS = None

### MPM defaults ##############################################################
MPM_DEFAULT_CONFFILE_PATH = '/etc/uhd/mpm.conf'
MPM_DEFAULT_LOG_LEVEL = 'info'
MPM_DEFAULT_LOG_BUF_SIZE = 100 # Number of log records to buf

class _MPMPrefs(configparser.ConfigParser):
    """
    Container for MPM preferences.

    On initialization, it will read preferences from the system-wide config
    file, if available, and from environment variables.
    Settings can be updated at any time (e.g., because want to factor in
    command line arguments).

    This class also encodes default prefs in the self.default_prefs dictionary.
    Said dictionary needs to be compatible with the ConfigParser.read_dict()
    API call (refer to the Python documentation).
    """
    default_prefs = {
        'mpm': {
            'log_level': MPM_DEFAULT_LOG_LEVEL,
            'log_buf_size': MPM_DEFAULT_LOG_BUF_SIZE,
        },
        'overrides': {
            'override_db_pids': '',
        },
    }

    def __init__(self):
        configparser.ConfigParser.__init__(self)
        self.read_dict(self.default_prefs)
        self._errors = []
        try:
            self.read(MPM_DEFAULT_CONFFILE_PATH)
        except configparser.Error as ex:
            self._errors.append('Config file parsing error: {}'.format(str(ex)))

    def get_log_level(self):
        """
        Return the selected default log level as an integer, usable by the
        logging API. This is a safe call, it will always return a useful value.
        Because this gets called as part of the logging API, it can't log any
        error messages. It will store them in the object itself under the
        __ERRORS__ section, key 'log_level'.
        """
        log_level = self.get('mpm', 'log_level').lower()
        log_level_map = {
            'trace': TRACE,
            'debug': DEBUG,
            'info': INFO,
            'warning': WARNING,
            'error': ERROR,
            'critical': CRITICAL,
        }
        if log_level not in log_level_map:
            self._errors.append('Invalid log level: {}'.format(log_level))
            # Note: After this function returns, we can use the logger so we
            # don't need to use this awkward side channel anymore!
        return log_level_map.get(
            log_level,
            log_level_map[MPM_DEFAULT_LOG_LEVEL]
        )

    def get_log_errors(self):
        """
        Returns errors that were generated during init but couldn't be logged
        because the logger isn't ready yet.
        """
        return self._errors

def get_prefs():
    """
    Return singleton preferences object. It's an object of type ConfigParser.
    """
    global _PREFS
    if _PREFS is None:
        _PREFS = _MPMPrefs()
    return _PREFS


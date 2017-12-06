#
# Copyright 2017 Ettus Research, National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#
"""
Miscellaneous utilities for MPM
"""

import time
import sys

def poll_with_timeout(state_check, timeout_ms, interval_ms):
    """
    Calls state_check() every interval_ms until it returns a positive value, or
    until a timeout is exceeded.

    Returns True if state_check() returned True within the timeout.

    Arguments:
    state_check -- Functor that returns a Boolean success value, and takes no
                   arguments.
    timeout_ms -- The total timeout in milliseconds. state_check() has to
                  return True within this time.
    interval_ms -- Sleep time between calls to state_check(). Note that if
                   interval_ms is larger than timeout_ms, state_check() will be
                   called exactly once, and then poll_with_timeout() will still
                   sleep for interval_ms milliseconds. Typically, interval_ms
                   should be chosen much smaller than timeout_ms, but not too
                   small for this to become a busy loop.
    """
    max_time = time.time() + (float(timeout_ms) / 1000)
    interval_s = float(interval_ms) / 1000
    while time.time() < max_time:
        if state_check():
            return True
        time.sleep(interval_s)
    return False

def to_native_str(str_or_bstr):
    """
    Returns a native string, regardless of the input string type (binary or
    UTF-8), and the Python version (2 or 3).
    Note that the native string type is actually not the same in Python 2 and
    3: In the former, it's a binary string, in the latter, it's Unicode.
    >>> to_native_str(b'foo')
    'foo'
    >>> to_native_str(u'foo')
    'foo'
    """
    if isinstance(str_or_bstr, str):
        return str_or_bstr
    if sys.version_info.major >= 3:
        return str(str_or_bstr, encoding='ascii')
    else:
        return str(str_or_bstr)

def to_binary_str(str_or_bstr):
    """
    Returns a binary string, regardless of the input string type (binary or
    UTF-8), and the Python version (2 or 3).
    Note that in Python 2, a binary string is the native string type.
    """
    try:
        return bytes(str_or_bstr.encode('utf-8'))
    except AttributeError:
        return bytes(str_or_bstr)


def to_utf8_str(str_or_bstr):
    """
    Returns a unicode string, regardless of the input string type (binary or
    UTF-8), and the Python version (2 or 3).
    Note that in Python 2, a unicode string is not the native string type.
    """
    try:
        return str_or_bstr.decode('utf-8')
    except AttributeError:
        return str_or_bstr


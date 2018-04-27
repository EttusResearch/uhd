#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Miscellaneous utilities for MPM
"""

import time

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
    try:
        # This will either fail because we're running Python 2 (which doesn't)
        # have the encoding argument) or because we're not passing in a bytes-
        # like object (e.g., an integer)
        return str(str_or_bstr, encoding='ascii')
    except TypeError:
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


def assert_compat_number(
        expected_compat,
        actual_compat,
        component=None,
        fail_on_old_minor=False,
        log=None,
    ):
    """
    Check if a compat number pair is acceptable. A compat number is a pair of
    integers (MAJOR, MINOR). A compat number is not acceptable if the major
    part differs from the expected value (regardless of how it's different) or
    if the minor part is behind the expected value and fail_on_old_minor was
    given.
    On failure, will throw a RuntimeError.

    Arguments:
    expected_compat -- A tuple (major, minor) which represents the compat
                       number we are expecting.
    actual_compat -- A tuple (major, minor) which represents the compat number
                     that is actually available.
    component -- A name of the component for which we are checking the compat
                 number, e.g. "FPGA".
    fail_on_old_minor -- Will also fail if the actual minor compat number is
                         behind the expected minor compat number, assuming the
                         major compat number matches.
    log -- Logger object. If given, will use this to report on intermediate
           steps and non-fatal minor compat mismatches.
    """
    assert len(expected_compat) == 2
    assert len(actual_compat) == 2
    log_err = lambda msg: log.error(msg) if log is not None else None
    log_warn = lambda msg: log.warning(msg) if log is not None else None
    expected_actual_str = "Expected: {:d}.{:d} Actual: {:d}.{:d}".format(
        expected_compat[0], expected_compat[1],
        actual_compat[0], actual_compat[1],
    )
    component_str = "" if component is None else " for component `{}'".format(
        component
    )
    if actual_compat[0] != expected_compat[0]:
        err_msg = "Major compat number mismatch{}: {}".format(
            component_str, expected_actual_str
        )
        log_err(err_msg)
        raise RuntimeError(err_msg)
    if actual_compat[1] > expected_compat[1]:
        log_warn("Actual minor compat ahead of expected compat{}. {}".format(
            component_str, expected_actual_str
        ))
    if actual_compat[1] < expected_compat[1]:
        err_msg = "Minor compat number mismatch{}: {}".format(
            component_str, expected_actual_str
        )
        log_err(err_msg)
        if fail_on_old_minor:
            raise RuntimeError(err_msg)
    return

def str2bool(value):
    """Return a Boolean value from a string, even if the string is not simply
    'True' or 'False'. For non-string values, this will do a simple default
    coercion to bool.
    """
    try:
        return value.lower() in ("yes", "true", "t", "1")
    except AttributeError:
        return bool(value)


def async_exec(parent, method_name, *args):
    """Execute method_name asynchronously.
    Requires the parent class to have this feature enabled.
    """
    async_name = 'async__' + method_name
    await_name = 'await__' + method_name
    # Spawn async
    getattr(parent, async_name)(*args)
    awaitable_method = getattr(parent, await_name)
    # await
    while not awaitable_method():
        time.sleep(0.1)


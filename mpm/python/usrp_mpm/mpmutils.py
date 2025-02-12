"""Copyright 2017 Ettus Research, a National Instruments Company.

SPDX-License-Identifier: GPL-3.0-or-later

Miscellaneous utilities for MPM.
"""

import time
from contextlib import contextmanager
from functools import partial

import pyudev


def poll_with_timeout(state_check, timeout_ms, interval_ms):
    """Runs blocking poll on state_check with timeout.

    Calls state_check() every interval_ms until it returns a positive value, or
    until a timeout is exceeded.

    Returns True if state_check() returned True within the timeout.

    :params state_check: Functor that returns a Boolean success value,
            and takes no arguments.
    :params timeout_ms: The total timeout in milliseconds. state_check() has to
            return True within this time.
    :params interval_ms: Sleep time between calls to state_check(). Note that if
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
    """Returns a native string.

    Returns a native string regardless of the input string type (binary or
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
        return str(str_or_bstr, encoding="ascii")
    except TypeError:
        return str(str_or_bstr)


def to_binary_str(str_or_bstr):
    """Returns a binary string.

    Returns a binary string, regardless of the input string type (binary or
    UTF-8), and the Python version (2 or 3).
    Note that in Python 2, a binary string is the native string type.
    """
    try:
        return bytes(str_or_bstr.encode("utf-8"))
    except AttributeError:
        return bytes(str_or_bstr)


def to_utf8_str(str_or_bstr):
    """Returns a unicode string.

    Returns a unicode string, regardless of the input string type (binary or
    UTF-8), and the Python version (2 or 3).
    Note that in Python 2, a unicode string is not the native string type.
    """
    try:
        return str_or_bstr.decode("utf-8")
    except AttributeError:
        return str_or_bstr


def assert_compat_number(
    expected_compat,
    actual_compat,
    component=None,
    fail_on_old_minor=False,
    log=None,
):
    """Check if a compat number tuple is acceptable.

    A compat number is a tuple of
    integers (MAJOR, MINOR, BUILD). A compat number is not acceptable if the major
    part differs from the expected value (regardless of how it's different) or
    if the minor part is behind the expected value and fail_on_old_minor was
    given. Build number is not checked here.
    On failure, will throw a RuntimeError.

    :params expected_compat: A tuple (major, minor) or (major, minor, build) which
            represents the compat number we are expecting.
    :params actual_compat: A tuple (major, minor) or (major, minor, build) which
            represents the compat number that is actually available.
    :params component: A name of the component for which we are checking the compat
            number, e.g. "FPGA".
    :params fail_on_old_minor: Will also fail if the actual minor compat number is
            behind the expected minor compat number, assuming the
            major compat number matches.
    :params log: Logger object. If given, will use this to report on intermediate
            steps and non-fatal minor compat mismatches.
    """
    valid_tuple_lengths = (2, 3)
    assert len(expected_compat) in valid_tuple_lengths, (
        f"Version {expected_compat} has invalid format. Valid formats are"
        "(major, minor) or (major, minor, build)"
    )
    assert len(actual_compat) in valid_tuple_lengths, (
        f"Version {expected_compat} has invalid format. Valid formats are"
        "(major, minor) or (major, minor, build)"
    )

    def log_err(msg):
        log.error(msg) if log is not None else None

    def log_warn(msg):
        log.warning(msg) if log is not None else None

    def log_debug(msg):
        log.debug(msg) if log is not None else None

    expected_actual_str = "Expected: {:d}.{:d} Actual: {:d}.{:d}".format(
        expected_compat[0],
        expected_compat[1],
        actual_compat[0],
        actual_compat[1],
    )
    component_str = "" if component is None else " for component '{}'".format(component)
    if actual_compat[0] != expected_compat[0]:
        err_msg = "Major compat number mismatch{}: {}".format(component_str, expected_actual_str)
        log_err(err_msg)
        raise RuntimeError(err_msg)
    if actual_compat[1] > expected_compat[1]:
        log_debug(
            "Minor compat ahead of expected compat{}. {}".format(component_str, expected_actual_str)
        )
    if actual_compat[1] < expected_compat[1]:
        err_msg = "Minor compat number mismatch{}: {}".format(component_str, expected_actual_str)
        if fail_on_old_minor:
            log_err(err_msg)
            raise RuntimeError(err_msg)
        log_warn(err_msg)


def str2bool(value):
    """Return a Boolean value from a string.

    Return a Boolean value from a string, even if the string is not simply
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
    async_name = "async__" + method_name
    await_name = "await__" + method_name
    # Spawn async
    getattr(parent, async_name)(*args)
    awaitable_method = getattr(parent, await_name)
    # await
    while not awaitable_method():
        time.sleep(0.1)


@contextmanager
def lock_guard(lockable):
    """Context-based lock guard.

    Use this in a with statement to lock out the following scope. Example:
    >>> with lock_guard(some_mutex):
    >>>    thread_sensitive_function()

    In this snippet, we assume that some_mutex is a lockable object, and
    implements lock() and unlock() member functions. Everything within the
    with context will then be serialized.

    This is a useful mechanic for sharing mutexes between Python and C++.

    :params lockable: Must have a .lock() and .unlock() method
    """
    lockable.lock()
    try:
        yield
    finally:
        lockable.unlock()


def check_fpga_state(which=0, logger=None):
    """Check if the FPGA is operational.

    :param which: the FPGA to check
    """
    try:
        context = pyudev.Context()
        fpga_mgrs = list(context.list_devices(subsystem="fpga_manager"))
        if fpga_mgrs:
            state = fpga_mgrs[which].attributes.asstring("state")
            if logger is not None:
                logger.trace("FPGA State: {}".format(state))
            return state == "operating"
        return False
    except OSError as ex:
        if logger is not None:
            logger.error("Error while checking FPGA status: {}".format(ex))
        return False


def parse_encoded_git_hash(encoded):
    """Turn a register-encoded git hash into a readable git hash.

    32-bit registers are used to store 7 characters of the hex git hash.
    The top nibble is used to store the dirtiness flag.
    """
    git_hash = encoded & 0x0FFFFFFF
    tree_dirty = (encoded & 0xF0000000) > 0
    dirtiness_qualifier = "dirty" if tree_dirty else "clean"
    return (git_hash, dirtiness_qualifier)


def parse_multi_device_arg(arg, conv=None, delim=";"):
    """Converts arg into tuple.

    In device args, there may be values that can either be scalar or a vector.
    They are listed as one of these:
    - key=val
    - key=[val0;val1]
    - key=[val0]

    This function takes the string representation of val and returns a tuple
    of values.

    :param arg: The argument value as a string
    :param conv: An optional converter function. This will be applied to all
                 individual elements.
    :param delim: The delimiter

    Example:
    >>> parse_multi_device_arg('[1;2;3]', float)
    (1.0, 2.0, 3.0)
    >>> parse_multi_device_arg('1', float)
    (1.0,)
    >>> parse_multi_device_arg('[1;2;3]')
    ('1', '2', '3')
    >>> parse_multi_device_arg('')
    ()
    """

    def identity(x):
        return x

    if conv is None:
        conv = identity
    arg = str(arg).strip()
    if not arg:  # Handle empty string
        return tuple()

    if (
        (arg[0], arg[-1]) == ("{", "}")
        or (arg[0], arg[-1]) == ("[", "]")
        or (arg[0], arg[-1]) == ("(", ")")
    ):
        arg = arg[1:-1]
    arg = arg.split(delim)
    return tuple(conv(x) for x in arg)


def get_dboard_class_from_pid(pid):
    """Given a PID, return a dboard class initializer callable.

    :param pid: The PID of the device
    """
    from usrp_mpm import dboard_manager

    for member in dboard_manager.__dict__.values():
        try:
            if (
                issubclass(member, dboard_manager.DboardManagerBase)
                and hasattr(member, "pids")
                and pid in member.pids
            ):
                return member
        except (TypeError, AttributeError):
            continue
    return None


def register_chained_signal_handler(signal, handler):
    """Register a signal handler that chains to the previous handler.

    :param signal: The signal to handle
    :param handler: Handler to be added to the chain
    """
    import signal as sig

    registered_handler = sig.getsignal(signal)

    def chained_handler(signum, frame):
        handler(signum, frame)
        if callable(registered_handler):
            registered_handler(signum, frame)

    sig.signal(signal, chained_handler)


def set_proc_title(title, logger=None):
    """Set the process title but only if the module is available.

    setproctitle module is not avalible by default on our embedded systems.
    Therefore, we set the process title only if the module is available.

    :param title: The new process title
    """
    try:
        import setproctitle

        setproctitle.setproctitle(title)
    except ImportError:
        if logger is not None:
            logger.debug(
                f"setproctitle module not available. Skip setting process title to {title}."
            )


# pylint: disable=too-few-public-methods
class LogWrapper:
    """This is a class that can be wrapped around any other class.

    It will log any calls to this class, including call arguments,
    return value, and execution time.
    """

    def __init__(self, logger, level, wrap_class):
        """Initialize the LogWrapper.

        Instruments public methods with logger calls.
        """
        self.logger = logger
        self.log = getattr(self.logger, level)
        self._wc = wrap_class
        for attr_name in dir(self._wc):
            attr = getattr(self._wc, attr_name)
            if not callable(attr):
                continue

            def new_method(name, *args, **kwargs):
                args_str = ", ".join(str(x) for x in args)
                if kwargs:
                    if args_str:
                        args_str += ","
                    args_str += ",".join([f" {k}={v}" for k, v in kwargs.items()])
                self.log(f"{name}({args_str.strip()})")
                start_time = time.monotonic()
                ret_val = getattr(self._wc, name)(*args, **kwargs)
                stop_time = time.monotonic()
                self.log(f"--> {ret_val} [Execution time: {(stop_time-start_time)*1e3:.3f} ms]")
                return ret_val

            setattr(self, attr_name, partial(new_method, attr_name))

    def __getattr__(self, k):
        """Catch-all getattr.

        We forward that to the wrapped class.
        """
        return getattr(self._wc, k)


# pylint: enable=too-few-public-methods


class LogRuntimeError(RuntimeError):
    """Custom version of RuntimeError.

    This class also prints the exception message to a logger.
    """

    def __init__(self, log, message):
        """Initialize LogRuntimeError.

        Print the message to the logger and then calls parent
        implementation.
        """
        log.error(message)
        super().__init__(message)

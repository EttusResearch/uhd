#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
systemd watchdog control module
"""

import os
import time
import threading
from systemd import daemon

MPM_WATCHDOG_DEFAULT_TIMEOUT = 30
# How often per watchdog interval we send a ping
MPM_WATCHDOG_TIMEOUT_FRAC = 3.0

def has_watchdog():
    """Check if the system has a watchdog checking on us.

    We do this by checking on a set value for WATCHDOG_USEC.
    """
    return bool(os.environ.get('WATCHDOG_USEC', False))

def transfer_control(pid):
    """
    Transfer control of watchdog notifications to new PID.
    """
    daemon.notify("MAINPID={:d}".format(int(pid)))

def _watchdog_task(shared_state, log):
    """
    Continuously ping the watchdog to tell him that we're still alive.

    This will keep running until the parent thread dies, or
    shared_state.system_ready gets set to False by someone.
    """
    watchdog_timeout = \
            float(os.environ.get(
                'WATCHDOG_USEC',
                MPM_WATCHDOG_DEFAULT_TIMEOUT
            )) / 1e6
    watchdog_interval = watchdog_timeout / MPM_WATCHDOG_TIMEOUT_FRAC
    daemon.notify("READY=1")
    log.debug("Watchdog primed, going into watchdog loop (Interval: %s s)",
              watchdog_interval)
    while shared_state.system_ready.value:
        # Sleep first, then ping, that avoids the case where transfer_control()
        # is not yet complete before we call this for the first time, which
        # would lead in error messages popping up in the systemd journal.
        time.sleep(watchdog_interval)
        log.trace("Pinging watchdog....")
        daemon.notify("WATCHDOG=1")
    log.error("Terminating watchdog thread!")
    return

def spawn_watchdog_task(shared_state, log):
    """Spawn and return watchdog thread.

    Creates a daemonic thread, because we don't want the watchdog task to
    outlive the main thread.
    """
    task = threading.Thread(
        target=_watchdog_task,
        args=[shared_state, log],
        name="MPMWatchdogTask",
        daemon=True,
    )
    task.start()
    return task


#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
This module is used as an interface between the sim_find.cpp discovery
and mboard_iface in uhd and the usrp_hwd python file. It manages
starting and stopping the simulator subprocess and configuring logging
"""
from multiprocessing import Process, Event
import sys
try:
    # Location if installed from using make install
    import usrp_hwd
except ImportError:
    # Location if installed from libpyuhd using setuptools
    from usrp_mpm import usrp_hwd

class ProcessManager:
    """This object is used to manage a simulator process which is launched
    from a python interpreter rather than from an os shell or using systemd
    """
    def __init__(self, args):
        """args are the command line arguments received by the simulator"""
        self.stop_event = Event()
        self.process = Process(target=_bootstrap, args=[args, self.stop_event])

    def start(self):
        """Launch the simulator's process"""
        self.process.start()

    def stop(self, timeout):
        """Attempt to stop the simulator cleanly. Returns True if successful"""
        self.stop_event.set()
        self.process.join(timeout)
        return self.process.exitcode is not None

    def terminate(self):
        """Forcefully terminates the simulator"""
        self.process.terminate()

    def pid(self):
        """Returns the PID of the simulator subprocess"""
        return int(self.process.pid)

def _bootstrap(args, stop_event):
    # Set args for new process
    #
    # Disable UHD log forwarding to avoid
    # duplicate messages
    sys.argv = ["usrp_hwd.py"] + args + ["--no-logbuf"]
    # tell main() not to block
    usrp_hwd.JOIN_PROCESSES = False
    # Start the discovery and RPC processes
    usrp_hwd.main()
    # Wait for signal from other process
    stop_event.wait()
    # Stop the discovery and RPC processes
    usrp_hwd.kill_time(None, None)

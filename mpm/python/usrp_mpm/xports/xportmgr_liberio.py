#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Liberio Transport manager
"""

from builtins import object
import pyudev

class XportMgrLiberio(object):
    """
    Transport manager for Liberio connections
    """
    # udev label for the UIO device that controls the DMA engine
    liberio_label = 'liberio'
    # Number of available DMA channels
    max_chan = 4

    def __init__(self, log, max_chan=-1):
        self.log = log.getChild('Liberio')
        if max_chan < 0:
            context = pyudev.Context()
            rx_of_nodes = list(context.list_devices(subsystem="platform",
                OF_COMPATIBLE_0="ettus,usrp-rx-dma"))
            tx_of_nodes = list(context.list_devices(subsystem="platform",
                OF_COMPATIBLE_0="ettus,usrp-tx-dma"))
            self.max_chan = min(len(rx_of_nodes), len(tx_of_nodes))
            self.log.debug("Found {} channels".format(self.max_chan))
        else:
            self.max_chan = max_chan
            self.log.debug("Reporting {} channels".format(self.max_chan))

    def init(self, args):
        """
        Call this when the user calls 'init' on the periph manager
        """
        pass

    def deinit(self):
        " Clean up after a session terminates "
        pass

    def get_xport_info(self):
        """
        Returns a dictionary of useful information, e.g. for appending into the
        device info.

        Note: This can be run by callers not owning a claim, even when the
        device has been claimed by someone else.

        In this case, returns an empty dict.
        """
        assert hasattr(self, 'log')
        return {}

    def get_chdr_link_options(self):
        """
        Returns a list of dictionaries for returning by
        PeriphManagerBase.get_chdr_link_options().

        Note: This requires a claim, which means that init() was called, and
        deinit() was not yet called.
        """
        return [
            {
                'tx_dev': "/dev/tx-dma{}".format(chan),
                'rx_dev': "/dev/rx-dma{}".format(chan),
                'dma_chan': str(chan),
            }
            for chan in range(self.max_chan)
        ]

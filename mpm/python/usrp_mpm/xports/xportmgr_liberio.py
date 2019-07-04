#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Liberio Transport manager
"""

from builtins import object

class XportMgrLiberio(object):
    """
    Transport manager for Liberio connections
    """
    # udev label for the UIO device that controls the DMA engine
    liberio_label = 'liberio'
    # Number of available DMA channels
    max_chan = 4

    def __init__(self, log):
        self.log = log.getChild('Liberio')

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

#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Liberio Transport manager
"""

from builtins import object
from usrp_mpm.liberiotable import LiberioDispatcherTable
from usrp_mpm import lib

class XportMgrLiberio(object):
    """
    Transport manager for Liberio connections
    """
    # udev label for the UIO device that controls the DMA engine
    liberio_label = 'liberio'
    # Number of available DMA channels
    max_chan = 4
    # Crossbar to which the Liberio DMA engine is connected
    xbar_dev = "/dev/crossbar0"
    xbar_port = 2

    def __init__(self, log):
        self.log = log
        self._dma_dispatcher = LiberioDispatcherTable(self.liberio_label)
        self._data_chan_ctr = 0

    def init(self, args):
        """
        Call this when the user calls 'init' on the periph manager
        """
        pass

    def deinit(self):
        " Clean up after a session terminates "
        self._data_chan_ctr = 0

    def get_xport_info(self):
        """
        Returns a dictionary of useful information, e.g. for appending into the
        device info.

        Note: This can be run by callers not owning a claim, even when the
        device has been claimed by someone else.

        In this case, returns an empty dict.
        """
        return {}

    def request_xport(
            self,
            sid,
            xport_type,
        ):
        """
        Return liberio xport info
        """
        assert xport_type in ('CTRL', 'ASYNC_MSG', 'TX_DATA', 'RX_DATA')
        if xport_type == 'CTRL':
            chan = 0
        elif xport_type == 'ASYNC_MSG':
            chan = 1
        else:
            chan = 2 + self._data_chan_ctr
            self._data_chan_ctr += 1
        xport_info = {
            'type': 'liberio',
            'send_sid': str(sid),
            'muxed': str(xport_type in ('CTRL', 'ASYNC_MSG')),
            'dma_chan': str(chan),
            'tx_dev': "/dev/tx-dma{}".format(chan),
            'rx_dev': "/dev/rx-dma{}".format(chan),
        }
        self.log.trace("Liberio: Chan: {} TX Device: {} RX Device: {}".format(
            chan, xport_info['tx_dev'], xport_info['rx_dev']))
        self.log.trace("Liberio channel is muxed: %s",
                       "Yes" if xport_info['muxed'] else "No")
        return [xport_info]

    def commit_xport(self, sid, xport_info):
        " Commit liberio transport "
        chan = int(xport_info['dma_chan'])
        xbar_iface = lib.xbar.xbar.make(self.xbar_dev)
        xbar_iface.set_route(sid.src_addr, self.xbar_port)
        self._dma_dispatcher.set_route(sid.reversed(), chan)
        self.log.trace("Liberio transport successfully committed!")
        return True


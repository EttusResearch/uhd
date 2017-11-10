#
# Copyright 2017 Ettus Research, National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#
"""
Liberio DMA dispatcher table control
"""

from builtins import str
from builtins import object
from .mpmlog import get_logger
from .uio import UIO

class LiberioDispatcherTable(object):
    """
    Controls a Liberio DMA dispatcher table.

    label -- A label that can be used by udev to find a UIO device
    """

    MTU_OFFSET = 0x80000

    def __init__(self, label):
        self.log = get_logger(label)
        self._regs = UIO(label=label, read_only=False)
        self.poke32 = self._regs.poke32
        self.peek32 = self._regs.peek32

    def set_route(self, sid, dma_channel, mtu):
        """
        Sets up routing in the Liberio dispatcher. From sid, only the
        destination part is important. After this call, any CHDR packet with the
        appropriate destination address will get routed to `dma_channel`.

        sid -- Full SID, but only destination part matters.
        dma_channel -- The DMA channel to which these packets should get routed.
        mtu -- Max size of bytes per packet. This is important to get right. The
               DMA implementation will pad packets smaller than MTU up to the
               mtu value, so making MTU extra large is inefficient. Packets
               larger than MTU will get chopped up. Even worse.
        """
        self.log.debug(
            "Routing SID `{sid}' to DMA channel `{chan}', MTU {mtu} bytes.".format(
                sid=str(sid), chan=dma_channel, mtu=mtu
            )
        )
        def poke_and_trace(addr, data):
            " Do a poke32() and log.trace() "
            self.log.trace("Writing to address 0x{:04X}: 0x{:04X}".format(
                addr, data
            ))
            self.poke32(addr, data)

        # Poke reg for destination channel
        # Poke reg for MTU
        try:
            poke_and_trace(
                0 + 4 * sid.dst_ep,
                dma_channel,
            )
            poke_and_trace(
                self.MTU_OFFSET + 4 * sid.dst_ep,
                int(mtu / 8),
            )
        except Exception as ex:
            self.log.error(
                "Unexpected exception while setting route: %s",
                str(ex),
            )
            raise


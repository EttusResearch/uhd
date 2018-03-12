#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Liberio DMA dispatcher table control
"""

from builtins import str
from builtins import object
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.sys_utils.uio import UIO

class LiberioDispatcherTable(object):
    """
    Controls a Liberio DMA dispatcher table.

    label -- A label that can be used by udev to find a UIO device
    """

    def __init__(self, label):
        self.log = get_logger(label)
        self._regs = UIO(label=label, read_only=False)
        self.poke32 = self._regs.poke32
        self.peek32 = self._regs.peek32

    def set_route(self, sid, dma_channel):
        """
        Sets up routing in the Liberio dispatcher. From sid, only the
        destination part is important. After this call, any CHDR packet with the
        appropriate destination address will get routed to `dma_channel`.

        sid -- Full SID, but only destination part matters.
        dma_channel -- The DMA channel to which these packets should get routed.
        """
        self.log.debug(
            "Routing SID `{sid}' to DMA channel `{chan}'.".format(
                sid=str(sid), chan=dma_channel
            )
        )
        def poke_and_trace(addr, data):
            " Do a poke32() and log.trace() "
            self.log.trace("Writing to address 0x{:04X}: 0x{:04X}".format(
                addr, data
            ))
            self.poke32(addr, data)
        # Poke reg for destination channel
        try:
            with self._regs.open():
                poke_and_trace(
                    0 + 4 * sid.dst_ep,
                    dma_channel,
                )
        except Exception as ex:
            self.log.error(
                "Unexpected exception while setting route: %s",
                str(ex),
            )
            raise

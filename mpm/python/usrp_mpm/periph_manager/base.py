#
# Copyright 2017 Ettus Research (National Instruments)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
"""
Mboard implementation base class
"""

from __future__ import print_function
import os
from builtins import str
from builtins import range
from builtins import object
from six import iteritems, itervalues
from ..mpmlog import get_logger
from .udev import get_eeprom_paths
from .udev import get_spidev_nodes
from usrp_mpm import net
from usrp_mpm import dtoverlay
from usrp_mpm import eeprom
from usrp_mpm.rpc_server import no_claim, no_rpc

def get_dboard_class_from_pid(pid):
    """
    Given a PID, return a dboard class initializer callable.
    """
    from .. import dboard_manager
    for member in itervalues(dboard_manager.__dict__):
        try:
            if issubclass(member, dboard_manager.DboardManagerBase) and \
                    hasattr(member, 'pids') and \
                    pid in member.pids:
                return member
        except (TypeError, AttributeError):
            continue
    return None


class PeriphManagerBase(object):
    """"
    Base class for all motherboards. Common function and API calls should
    be implemented here. Motherboard specific information can be stored in
    separate motherboard classes derived from this class
    """
    # stores discovered device information in dicts
    mboard_if_addrs = {}
    # this information has to be provided by
    # the specific periph_manager implementation
    updateable_components = []
    sid_endpoints = {}

    #########################################################################
    # Overridables
    #
    # These values are meant to be overridden by the according subclasses
    #########################################################################
    # Very important: A list of PIDs that apply to the current device. Must be
    # list, even if there's only one entry.
    pids = []
    # Address of the motherboard EEPROM. This could be something like
    # "e0005000.i2c". This value will be passed to get_eeprom_paths() tos
    # determine a full path to an EEPROM device.
    # If empty, this will be ignored and no EEPROM info for the device is read
    # out.
    mboard_eeprom_addr = ""
    # The EEPROM code checks for this word to see if the readout was valid.
    # Typically, devices should not override this unless their EEPROM follows a
    # different standard.
    mboard_eeprom_magic = 0xF008AD10
    # If this value is not set, the code will try and read out the entire EEPROM
    # content as a binary blob. Use this to limit the number of bytes actually
    # read. It's usually safe to not override this, as EEPROMs typically aren't
    # that big.
    mboard_eeprom_max_len = None
    # This is the *default* mboard info. The keys from this dict will be copied
    # into the current device info before it actually gets initialized. This
    # means that keys from this dict could be overwritten during the
    # initialization process.
    mboard_info = {"type": "unknown"}
    # For checking revision numbers, this is the highest revision that this
    # particular version of MPM supports. Leave at None to skip a max rev
    # check.
    mboard_max_rev = None
    # This is a sanity check value to see if the correct number of
    # daughterboards are detected. If somewhere along the line more than
    # max_num_dboards dboards are found, an error or warning is raised,
    # depending on the severity of the issue. If fewer dboards are found,
    # that's generally considered OK.
    max_num_dboards = 2
    # Address of the daughterboard EEPROMs. This could be something like
    # "e0004000.i2c". This value will be passed to get_eeprom_paths() to
    # determine a full path to an EEPROM device.
    # If empty, this will be ignored and no EEPROM info for the device is read
    # out.
    # If this is a list of EEPROMs, paths will be concatenated.
    dboard_eeprom_addr = None
    # The EEPROM code checks for this word to see if the readout was valid.
    # Typically, devices should not override this unless their EEPROM follows a
    # different standard.
    dboard_eeprom_magic = 0xF008AD11
    # If this value is not set, the code will try and read out the entire EEPROM
    # content as a binary blob. Use this to limit the number of bytes actually
    # read. It's usually safe to not override this, as EEPROMs typically aren't
    # that big.
    dboard_eeprom_max_len = None
    # If the dboard requires spidev access, the following attribute is a list
    # of SPI master addrs (typically something like 'e0006000.spi'). You
    # usually want the length of this list to be as long as the number of
    # dboards, but if it's shorter, it simply won't instantiate list SPI nodes
    # for those dboards.
    dboard_spimaster_addrs = []
    # Lists the network interfaces which can theoretically support CHDR. These
    # do not have to exist, but these interfaces will be probed for
    # availability. If the list is empty, no CHDR traffic will be possible over
    # the network. Example: ['eth1', 'eth2']
    chdr_interfaces = []
    @staticmethod
    # Yes, this is overridable too: List the required device tree overlays
    def list_required_dt_overlays(eeprom_md, device_args):
        """
        Lists device tree overlays that need to be applied before this class can
        be used. List of strings.
        Are applied in order.

        eeprom_md -- Dictionary of info read out from the mboard EEPROM
        device_args -- Arbitrary dictionary of info, typically user-defined
        """
        return []
    ### End of overridables ###################################################

    def __init__(self, args):
        # First, make some checks to see if the child class is correctly set up:
        assert len(self.pids) > 0
        assert self.mboard_eeprom_magic is not None
        # Set up logging
        self.log = get_logger('PeriphManager')
        self.claimed = False
        self._init_mboard_with_eeprom()
        self._init_mboard_overlays(self._eeprom_head, args)
        self._init_dboards(args.override_db_pids)
        self._available_endpoints = list(range(256))
        self._init_args = {}
        self.log.info("Identifying available network interfaces...")
        self._chdr_interfaces = self._init_interfaces(self.chdr_interfaces)

    def _init_mboard_with_eeprom(self):
        """
        Starts the device initialization. Typically requires reading from an
        EEPROM.
        """
        if len(self.mboard_eeprom_addr):
            self.log.trace("Reading EEPROM from address `{}'...".format(self.mboard_eeprom_addr))
            (self._eeprom_head, self._eeprom_rawdata) = eeprom.read_eeprom(
                get_eeprom_paths(self.mboard_eeprom_addr)[0],
                eeprom.MboardEEPROM.eeprom_header_format,
                eeprom.MboardEEPROM.eeprom_header_keys,
                self.mboard_eeprom_magic,
                self.mboard_eeprom_max_len,
            )
            self.log.trace("Found EEPROM metadata: `{}'".format(str(self._eeprom_head)))
            self.log.trace("Read {} bytes of EEPROM data.".format(len(self._eeprom_rawdata)))
            for key in ('pid', 'serial', 'rev'):
                # In C++, we can only handle dicts if all the values are of the
                # same type. So we must convert them all to strings here:
                try:
                    self.mboard_info[key] = str(
                        self._eeprom_head.get(key, ''),
                        'ascii'
                    )
                except TypeError:
                    self.mboard_info[key] = str(self._eeprom_head.get(key, ''))
            if 'pid' in self._eeprom_head \
                    and self._eeprom_head['pid'] not in self.pids:
                self.log.error(
                    "Found invalid PID in EEPROM: 0x{:04X}. " \
                    "Valid PIDs are: {}".format(
                        self._eeprom_head['pid'],
                        ", ".join(["0x{:04X}".format(x) for x in self.pids]),
                    )
                )
                raise RuntimeError("Invalid PID found in EEPROM.")
            if 'rev' in self._eeprom_head:
                try:
                    rev_numeric = int(self._eeprom_head.get('rev'))
                except (ValueError, TypeError):
                    raise RuntimeError(
                        "Invalid revision info read from EEPROM!"
                    )
                if self.mboard_max_rev is not None \
                        and rev_numeric > self.mboard_max_rev:
                    raise RuntimeError(
                        "Device has revision `{}', but max supported " \
                        "revision is `{}'".format(
                            rev_numeric, self.mboard_max_rev
                        ))
            else:
                raise RuntimeError("No revision found in EEPROM.")
        else:
            self.log.trace("No EEPROM address to read from.")
            self._eeprom_head = {}
            self._eeprom_rawdata = ''
        self.log.info("Device serial number: {}".format(self.mboard_info.get('serial', 'n/a')))

    def _init_mboard_overlays(self, eeprom_md, device_args):
        """
        Load all required overlays for this motherboard
        """
        requested_overlays = self.list_required_dt_overlays(
            eeprom_md,
            device_args,
        )
        self.log.trace("Motherboard requires device tree overlays: {}".format(
            requested_overlays
        ))
        for overlay in requested_overlays:
            dtoverlay.apply_overlay_safe(overlay)


    def _init_dboards(self, override_dboard_pids=None):
        """
        Initialize all the daughterboards
        """
        def _init_dboards_overlay(db_class):
            """
            Load the required overlays for this dboard.
            """
            requested_overlays = db_class.list_required_dt_overlays(
                dboard_eeprom_md,
                'XG', # FIXME don't hardcode
                {}, # FIXME don't hardcode
            )
            self.log.trace("Dboard requires device tree overlays: {}".format(
                requested_overlays
            ))
            for overlay in requested_overlays:
                dtoverlay.apply_overlay_safe(overlay)
        # Go, go, go!
        override_dboard_pids = override_dboard_pids or []
        dboard_eeprom_addrs = self.dboard_eeprom_addr \
                              if isinstance(self.dboard_eeprom_addr, list) \
                              else [self.dboard_eeprom_addr]
        dboard_eeprom_paths = []
        self.log.trace("Identifying dboard EEPROM paths from addrs `{}'...".format(",".join(dboard_eeprom_addrs)))
        for dboard_eeprom_addr in dboard_eeprom_addrs:
            self.log.trace("Resolving {}...".format(dboard_eeprom_addr))
            dboard_eeprom_paths += get_eeprom_paths(dboard_eeprom_addr)
        self.log.trace("Found dboard EEPROM paths: {}".format(",".join(dboard_eeprom_paths)))
        if len(dboard_eeprom_paths) > self.max_num_dboards:
            self.log.warning("Found more EEPROM paths than daughterboards. Ignoring some of them.")
            dboard_eeprom_paths = dboard_eeprom_paths[:self.max_num_dboards]
        if len(override_dboard_pids) and \
                len(override_dboard_pids) < len(dboard_eeprom_paths):
            self.log.warning("--override-db-pids is going to skip dboards.")
            dboard_eeprom_paths = \
                    dboard_eeprom_paths[:len(override_dboard_pids)]
        self.dboards = []
        for dboard_idx, dboard_eeprom_path in enumerate(dboard_eeprom_paths):
            self.log.debug("Initializing dboard {}...".format(dboard_idx))
            dboard_eeprom_md, dboard_eeprom_rawdata = eeprom.read_eeprom(
                dboard_eeprom_path,
                eeprom.DboardEEPROM.eeprom_header_format,
                eeprom.DboardEEPROM.eeprom_header_keys,
                self.dboard_eeprom_magic,
                self.dboard_eeprom_max_len,
            )
            self.log.trace("Found dboard EEPROM metadata: `{}'".format(str(dboard_eeprom_md)))
            self.log.trace("Read {} bytes of dboard EEPROM data.".format(len(dboard_eeprom_rawdata)))
            if len(override_dboard_pids) > dboard_idx:
                db_pid = override_dboard_pids[dboard_idx]
                self.log.warning("Overriding dboard PID for dboard {} with 0x{:04X}.".format(dboard_idx, db_pid))
            else:
                db_pid = dboard_eeprom_md.get('pid')
                if db_pid is None:
                    self.log.warning("No dboard PID found!")
                else:
                    self.log.debug("Found dboard PID in EEPROM: 0x{:04X}".format(db_pid))
            db_class = get_dboard_class_from_pid(db_pid)
            if db_class is None:
                self.log.warning("Could not identify daughterboard class for PID {:04X}!".format(db_pid))
                continue
            _init_dboards_overlay(db_class)
            if len(self.dboard_spimaster_addrs) > dboard_idx:
                spi_nodes = sorted(get_spidev_nodes(self.dboard_spimaster_addrs[dboard_idx]))
                self.log.debug("Found spidev nodes: {0}".format(spi_nodes))
            else:
                spi_nodes = []
                self.log.warning("No SPI nodes for dboard {}.".format(dboard_idx))
            dboard_info = {
                'eeprom_md': dboard_eeprom_md,
                'eeprom_rawdata': dboard_eeprom_rawdata,
                'pid': db_pid,
                'spi_nodes': spi_nodes,
            }
            # This will actually instantiate the dboard class:
            self.dboards.append(db_class(dboard_idx, **dboard_info))
        self.log.info("Found {} daughterboard(s).".format(len(self.dboards)))

    def _init_interfaces(self, possible_ifaces):
        """
        Initialize the list of network interfaces
        """
        self.log.trace("Testing available interfaces out of `{}'".format(
            possible_ifaces
        ))
        valid_ifaces = net.get_valid_interfaces(possible_ifaces)
        if len(valid_ifaces):
            self.log.debug("Found CHDR interfaces: `{}'".format(valid_ifaces))
        else:
            self.log.warning("No CHDR interfaces found!")
        return {
            x: net.get_iface_info(x)
            for x in valid_ifaces
        }

    def init(self, args):
        """
        Run the mboard initialization. This is typically done at the beginning
        of a UHD session.
        Default behaviour is to call init() on all the daughterboards.`args' is
        passed to the daughterboard's init calls.  For additional features,
        this needs to be overridden.

        The main requirement of this function is, after calling it successfully,
        all RFNoC blocks must be reachable via CHDR interfaces (i.e., clocks
        need to be on).

        Return False on failure, True on success.

        args -- A dictionary of args for initialization. Similar to device args
                in UHD.
        """
        self.log.info("Mboard init() called with device args `{}'.".format(
            ",".join(['{}={}'.format(x, args[x]) for x in args])
        ))
        self._init_args = args
        self.log.debug("Initializing dboards...")
        return all((dboard.init(args) for dboard in self.dboards))

    def deinit(self):
        """
        Power down a device after a UHD session.
        This must be safe to call multiple times. The default behaviour is to
        call deinit() on all the daughterboards.
        """
        self.log.info("Mboard deinit() called.")
        for dboard in self.dboards:
            dboard.deinit()
        self.log.trace("Resetting SID pool...")
        self._available_endpoints = list(range(256))

    @no_claim
    def list_updateable_components(self):
        """
        return list of updateable components
        This method does not require a claim_token in the RPC
        """
        return self.updateable_components

    @no_claim
    def list_available_overlays(self):
        """
        Returns a list of available device tree overlays
        """
        return dtoverlay.list_available_overlays()

    @no_claim
    def list_active_overlays(self):
        """
        Returns a list of currently loaded device tree overlays
        check which dt overlay is loaded currently
        """
        return dtoverlay.list_overlays()

    def _get_device_info(self):
        """
        return the mboard_info dict and add a claimed field
        """
        result = {"claimed": str(self.claimed)}
        result.update(self.mboard_info)
        return result

    @no_claim
    def get_dboard_info(self):
        """
        Returns a list of dicts. One dict per dboard.
        """
        return [dboard.device_info for dboard in self.dboards]

    def load_fpga_image(self, target=None):
        """
        load a new fpga image
        """
        pass

    def _allocate_sid(self, sender_addr, sid, xbar_src_addr, xbar_src_port):
        """
        Overload this method in actual device implementation
        """
        raise NotImplementedError("_allocate_sid() not implented")

    @no_claim
    def get_num_xbars(self):
        """
        Returns the number of crossbars instantiated in the current design
        """
        return 1 # FIXME

    @no_claim
    def get_num_blocks(self, xbar_index):
        """
        Returns the number of blocks connected to crossbar with index
        xbar_index.

        xbar_index -- The index of the crossbar that's being queried.
        docstring for get_num_blocks"""
        # FIXME udev lookup
        xbar_sysfs_path = '/sys/class/rfnoc_crossbar/crossbar{}/nports'.format(
            xbar_index
        )
        return int(open(xbar_sysfs_path).read().strip()) - \
                self.get_base_port(xbar_index)

    @no_claim
    def get_base_port(self, xbar_index):
        """
        Returns the index of the first port which is connected to an RFNoC
        block. Example: Assume there are two SFPs connected to the crossbar, and
        one DMA engine for CHDR traffic. The convention would be to connect
        those to ports 0, 1, and 2, respectively. This makes port 3 the first
        block to be connected to an RFNoC block.

        xbar_index -- The index of the crossbar that's being queried
        """
        return 3 # FIXME It's 3 because 0,1,2 are SFP,SFP,DMA

    def set_xbar_local_addr(self, xbar_index, local_addr):
        """
        Program crossbar xbar_index to have the local address local_addr.
        """
        # FIXME udev lookup
        xbar_sysfs_path = '/sys/class/rfnoc_crossbar/crossbar{}/local_addr'.format(
            xbar_index
        )
        laddr_value = "0x{:X}".format(local_addr)
        self.log.trace("Setting local address for xbar {} to {}.".format(
            xbar_sysfs_path, laddr_value
        ))
        with open(xbar_sysfs_path, "w") as xbar_file:
            xbar_file.write(laddr_value)
        return True


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

import os
import struct
from six import iteritems, itervalues
from ..mpmlog import get_logger
from .udev import get_eeprom_paths
from .udev import get_spidev_nodes
from usrp_mpm import net

EEPROM_DEFAULT_HEADER = struct.Struct("!I I")

class MboardEEPROM(object):
    """
    Given a nvmem path, read out EEPROM values from the motherboard's EEPROM.
    The format of data in the EEPROM must follow the following standard:

    - 4 bytes magic. This will always be the same value; checking this value is
                     a sanity check for if the read was successful.
    - 4 bytes version. This is the version of the EEPROM format.

    The following bytes are version-dependent:

    Version 1:

    - 4x4 bytes mcu_flags -> throw them away
    - 2 bytes hw_pid
    - 2 bytes hw_rev (starting at 0)
    - 8 bytes serial number (zero-terminated string of 7 characters)
    - 6 bytes MAC address for eth0
    - 2 bytes padding
    - 6 bytes MAC address for eth1
    - 2 bytes padding
    - 6 bytes MAC address for eth2
    - 2 bytes padding
    - 4 bytes CRC

    MAC addresses are ignored here; they are read elsewhere. If we really need
    to know the MAC address of an interface, we can fish it out the raw data,
    or ask the system.
    """
    # Create one of these for every version of the EEPROM format:
    eeprom_header_format = (
        None, # For laziness, we start at version 1 and thus index 0 stays empty
        "!I I 16x H H 7s 25x I", # Version 1
    )
    eeprom_header_keys = (
        None, # For laziness, we start at version 1 and thus index 0 stays empty
        ('magic', 'eeprom_version', 'pid', 'rev', 'serial', 'CRC'), # Version 1
    )

class DboardEEPROM(object):
    """
    Given a nvmem path, read out EEPROM values from the daughterboard's EEPROM.
    The format of data in the EEPROM must follow the following standard:

    - 4 bytes magic. This will always be the same value; checking this value is
                     a sanity check for if the read was successful.
    - 4 bytes version. This is the version of the EEPROM format.

    The following bytes are version-dependent:

    Version 1:

    - 2 bytes hw_pid
    - 2 bytes hw_rev (starting at 0)
    - 8 bytes serial number (zero-terminated string of 7 characters)
    - 4 bytes CRC

    MAC addresses are ignored here; they are read elsewhere. If we really need
    to know the MAC address of an interface, we can fish it out the raw data,
    or ask the system.
    """
    # Create one of these for every version of the EEPROM format:
    eeprom_header_format = (
        None, # For laziness, we start at version 1 and thus index 0 stays empty
        "!I I H H 7s 1x I", # Version 1
    )
    eeprom_header_keys = (
        None, # For laziness, we start at version 1 and thus index 0 stays empty
        ('magic', 'eeprom_version', 'pid', 'rev', 'serial', 'CRC'), # Version 1
    )


def read_eeprom(
        nvmem_path,
        eeprom_header_format,
        eeprom_header_keys,
        expected_magic,
        max_size=None
):
    """
    Read the EEPROM located at nvmem_path and return a tuple (header, data)
    Header is already parsed in the common header fields
    Data contains the full eeprom data structure

    nvmem_path -- Path to readable file (typically something in sysfs)
    eeprom_header_format -- List of header formats, by version
    eeprom_header_keys -- List of keys for the entries in the EEPROM
    expected_magic -- The magic value that is expected
    max_size -- Max number of bytes to be read. If omitted, will read the full file.
    """
    assert len(eeprom_header_format) == len(eeprom_header_keys)
    def _parse_eeprom_data(
            data,
            version,
        ):
        """
        Parses the raw 'data' according to the version.
        Returns a dictionary.
        """
        eeprom_parser = struct.Struct(eeprom_header_format[version])
        eeprom_keys = eeprom_header_keys[version]
        parsed_data = eeprom_parser.unpack_from(data)
        return dict(zip(eeprom_keys, parsed_data))
    # Dawaj, dawaj
    max_size = max_size or -1
    with open(nvmem_path, "rb") as nvmem_file:
        data = nvmem_file.read(max_size)
    eeprom_magic, eeprom_version = EEPROM_DEFAULT_HEADER.unpack_from(data)
    if eeprom_magic != expected_magic:
        raise RuntimeError("Received incorrect EEPROM magic. " \
                           "Read: {:08X} Expected: {:08X}".format(
                               eeprom_magic, expected_magic
       ))
    if eeprom_version >= len(eeprom_header_format):
        raise RuntimeError("Unexpected EEPROM version: `{}'".format(eeprom_version))
    return (_parse_eeprom_data(data, eeprom_version), data)


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
    mboard_overlays = {}
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


    def __init__(self, args):
        # First, make some checks to see if the child class is correctly set up:
        assert len(self.pids) > 0
        assert self.mboard_eeprom_magic is not None
        # Set up logging
        self.log = get_logger('PeriphManager')
        self.claimed = False
        self._init_mboard_with_eeprom()
        self._init_dboards(args.override_db_pids)
        self._available_endpoints = range(256)
        self._init_args = {}
        self._chdr_interfaces = []

    def _init_mboard_with_eeprom(self):
        """
        Starts the device initialization. Typically requires reading from an
        EEPROM.
        """
        if len(self.mboard_eeprom_addr):
            self.log.trace("Reading EEPROM from address `{}'...".format(self.mboard_eeprom_addr))
            (self._eeprom_head, self._eeprom_rawdata) = read_eeprom(
                get_eeprom_paths(self.mboard_eeprom_addr)[0],
                MboardEEPROM.eeprom_header_format,
                MboardEEPROM.eeprom_header_keys,
                self.mboard_eeprom_magic,
                self.mboard_eeprom_max_len,
            )
            self.log.trace("Found EEPROM metadata: `{}'".format(str(self._eeprom_head)))
            self.log.trace("Read {} bytes of EEPROM data.".format(len(self._eeprom_rawdata)))
            for key in ('pid', 'serial', 'rev'):
                # In C++, we can only handle dicts if all the values are of the
                # same type. So we must convert them all to strings here:
                self.mboard_info[key] = str(self._eeprom_head.get(key, ''))
            if self._eeprom_head.has_key('pid') and self._eeprom_head['pid'] not in self.pids:
                self.log.error("Found invalid PID in EEPROM: 0x{:04X}. Valid PIDs are: {}".format(
                    self._eeprom_head['pid'],
                    ", ".join(["0x{:04X}".format(x) for x in self.pids]),
                ))
                raise RuntimeError("Invalid PID found in EEPROM.")
        else:
            self.log.trace("No EEPROM address to read from.")
            self._eeprom_head = {}
            self._eeprom_rawdata = ''
        self.log.info("Device serial number: {}".format(self.mboard_info.get('serial', 'n/a')))


    def _init_dboards(self, override_dboard_pids=None):
        """
        Initialize all the daughterboards
        """
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
        self.dboards = []
        for dboard_idx, dboard_eeprom_path in enumerate(dboard_eeprom_paths):
            self.log.debug("Initializing dboard {}...".format(dboard_idx))
            dboard_eeprom_md, dboard_eeprom_rawdata = read_eeprom(
                dboard_eeprom_path,
                DboardEEPROM.eeprom_header_format,
                DboardEEPROM.eeprom_header_keys,
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
            db_class = get_dboard_class_from_pid(db_pid)
            if db_class is None:
                self.log.warning("Could not identify daughterboard class for PID {:04X}!".format(db_pid))
                continue
            self.dboards.append(db_class(dboard_idx, **dboard_info))
        self.log.info("Found {} daughterboard(s).".format(len(self.dboards)))

        # self.overlays = ""

    def _init_interfaces(self):
        """
        Initialize the list of network interfaces
        """
        self.log.trace("Testing available interfaces out of `{}'".format(
            self.chdr_interfaces
        ))
        valid_ifaces = net.get_valid_interfaces(self.chdr_interfaces)
        self.log.debug("Found CHDR interfaces: `{}'".format(valid_ifaces))
        self._chdr_interfaces = {
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

        args -- A dictionary of args for initialization. Similar to device args
                in UHD.
        """
        self.log.info("Mboard init() called with device args `{}'.".format(
            ",".join(['{}={}'.format(x, args[x]) for x in args])
        ))
        self._init_args = args
        self.log.info("Identifying available network interfaces...")
        self._init_interfaces()
        self.log.debug("Initializing dboards...")
        for dboard in self.dboards:
            dboard.init(args)

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
        self._available_endpoints = range(256)

    def safe_list_updateable_components(self):
        """
        return list of updateable components
        This method does not require a claim_token in the RPC
        """
        return self.updateable_components

    def get_overlays(self):
        """
        get and store the list of available dt overlays
        """
        self.mboard_overlays = []
        for fw_files in os.listdir("/lib/firmware/"):
            if fw_files.endswith(".dtbo"):
                self.mboard_overlays.append(fw_files.strip(".dtbo"))

    def check_overlay(self):
        """
        check which dt overlay is loaded currently
        """
        for overlay_file in os.listdir("/sys/kernel/device-tree/overlays/"):
            self.overlays = overlay_file

    def _get_device_info(self):
        """
        return the mboard_info dict and add a claimed field
        """
        result = {"claimed": str(self.claimed)}
        result.update(self.mboard_info)
        return result

    def get_dboards(self):
        """
        get a dict with slot: hw_pid for each dboard
        """
        result = {}
        for slot, dboard in iteritems(self.dboards):
            result.update({slot:dboard.hw_pid})
        return result

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


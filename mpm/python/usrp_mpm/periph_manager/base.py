#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Mboard implementation base class
"""

from __future__ import print_function
import os
from enum import Enum
from hashlib import md5
from time import sleep
from concurrent import futures
from builtins import str
from builtins import object
from six import iteritems, itervalues
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.sys_utils.filesystem_status import get_fs_version
from usrp_mpm.sys_utils.filesystem_status import get_mender_artifact
from usrp_mpm.sys_utils.udev import get_eeprom_paths_by_symbol
from usrp_mpm.sys_utils.udev import get_eeprom_paths
from usrp_mpm.sys_utils.udev import get_spidev_nodes
from usrp_mpm.sys_utils import dtoverlay
from usrp_mpm.sys_utils import net
from usrp_mpm import eeprom
from usrp_mpm.rpc_server import no_claim, no_rpc
from usrp_mpm import prefs

def get_dboard_class_from_pid(pid):
    """
    Given a PID, return a dboard class initializer callable.
    """
    from usrp_mpm import dboard_manager
    for member in itervalues(dboard_manager.__dict__):
        try:
            if issubclass(member, dboard_manager.DboardManagerBase) and \
                    hasattr(member, 'pids') and \
                    pid in member.pids:
                return member
        except (TypeError, AttributeError):
            continue
    return None


# We need to disable the no-self-use check, because we might require self to
# become an RPC method, but PyLint doesnt' know that. We'll also disable
# warnings about this being a god class.
# pylint: disable=no-self-use
# pylint: disable=too-many-public-methods
# pylint: disable=too-many-instance-attributes
class PeriphManagerBase(object):
    """"
    Base class for all motherboards. Common function and API calls should
    be implemented here. Motherboard specific information can be stored in
    separate motherboard classes derived from this class
    """
    class _EepromSearch(Enum):
        """
        List supported ways of searching EEPROM files.
        """
        LEGACY = 1 # Using EEPROM address
        SYMBOL = 2 # Using symbol names
    #########################################################################
    # Overridables
    #
    # These values are meant to be overridden by the according subclasses
    #########################################################################
    # Very important: A map of PIDs that apply to the current device. Format is
    # pid -> product name. If there are multiple products with the same
    # motherboard PID, use generate_device_info() to update the product key.
    pids = {}
    # A textual description of this device type
    description = "MPM Device"
    # EEPROM layout used by this class. Defaults to legacy which uses eeprom.py
    # to read EEPROM data
    eeprom_search = _EepromSearch.LEGACY
    # Address of the motherboard EEPROM. This could be something like
    # "e0005000.i2c". This value will be passed to get_eeprom_paths() tos
    # determine a full path to an EEPROM device.
    # If empty, this will be ignored and no EEPROM info for the device is read
    # out.
    mboard_eeprom_addr = ""
    # Offset of the motherboard EEPROM. All accesses to this EEPROM will be
    # offset by this amount. In many cases, this value will be 0. But in some
    # situations, we may want to use the offset as a way of partitioning
    # access to an EEPROM.
    mboard_eeprom_offset = 0
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
    # A list of available sensors on the motherboard. This dictionary is a map
    # of the form sensor_name -> method name
    mboard_sensor_callback_map = {}
    # This is a sanity check value to see if the correct number of
    # daughterboards are detected. If somewhere along the line more than
    # max_num_dboards dboards are found, an error or warning is raised,
    # depending on the severity of the issue. If fewer dboards are found,
    # that's generally considered OK.
    max_num_dboards = 2
    # The index of the first port of the RFNoC crossbar which is connected to
    # an RFNoC block
    crossbar_base_port = 0
    # A DboardIface class which will be passed to the discovered DB
    # constructors.
    # If None, the MB does not support the DB Iface architecture.
    db_iface = None
    # Address of the daughterboard EEPROMs. This could be something like
    # "e0004000.i2c". This value will be passed to get_eeprom_paths() to
    # determine a full path to an EEPROM device.
    # If empty, this will be ignored and no EEPROM info for the device is read
    # out.
    # If this is a list of EEPROMs, paths will be concatenated.
    dboard_eeprom_addr = None
    # Offset of the daughterboard EEPROM. All accesses to this EEPROM will be
    # offset by this amount. In many cases, this value will be 0. But in some
    # situations, we may want to use the offset as a way of partitioning
    # access to an EEPROM.
    # Assume that all dboard offsets are the same for a given device. That is,
    # the offset of DBoard 0 == offset of DBoard 1
    dboard_eeprom_offset = 0
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
    # Dictionary containing valid IDs for the update_component function for a
    # specific implementation. Each PeriphManagerBase-derived class should list
    # information required to update the component, like a callback function
    updateable_components = {}
    # The RPC server checks this value to determine if it needs to clear
    # the RPC method registry. This is typically to remove stale references
    # to RPC methods caused by removal of overlay on unclaim() by peripheral
    # manager. Additionally the RPC server will re-register all methods on
    # a claim(). Override and set to True in the derived class if desired.
    clear_rpc_registry_on_unclaim = False

    # Symbols are use to find board EEPROM by a symbolic name. Can only be used
    # on systems that support symbol name under /proc/device-tree/__symbols__.
    # symbol name for motherboard EEPROM file
    mboard_eeprom_symbol = "mb_eeprom"
    # symbol glob for daugtherboard EEPROM files
    dboard_eeprom_symbols = "db[0,1]_eeprom"
    # symbol glob fox auxiliary boards
    auxboard_eeprom_symbols = "*aux_eeprom"
    # List of discoverable features supported by a device.
    discoverable_features = []


    # Disable checks for unused args in the overridables, because the default
    # implementations don't need to use them.
    # pylint: disable=unused-argument
    @staticmethod
    def generate_device_info(eeprom_md, mboard_info, dboard_infos):
        """
        Returns a dictionary which describes the device.

        mboard_info -- Dictionary; motherboard info
        device_args -- List of dictionaries; daughterboard info
        """
        # Try to add the MPM Git hash and version
        try:
            from usrp_mpm import __version__, __githash__
            version_string = __version__
            if __githash__:
                version_string += "-g" + str(__githash__)
        except ImportError:
            version_string = ""
        mboard_info["mpm_sw_version"] = version_string

        fs_version = get_fs_version()
        if fs_version is not None:
            mboard_info["fs_version"] = fs_version
        # Mender artifacts are generally not present on a machine hosting
        # a simulated device--let it slide if not found on sim devices
        try:
            mboard_info['mender_artifact'] = get_mender_artifact()
        except FileNotFoundError:
            # Note that the simulated key will not be present for
            # non-simulated devices, hence the use of get()
            if mboard_info.get('simulated', '') == 'True':
                pass
            else:
                raise

        for i,dboard_info in enumerate(dboard_infos):
            mboard_info["dboard_{}_pid".format(i)] = str(dboard_info["pid"])
            mboard_info["dboard_{}_serial".format(i)] = dboard_info["eeprom_md"]["serial"]

        return mboard_info

    @staticmethod
    # Yes, this is overridable too: List the required device tree overlays
    def list_required_dt_overlays(device_info):
        """
        Lists device tree overlays that need to be applied before this class can
        be used. List of strings.
        Are applied in order.

        eeprom_md -- Dictionary of info read out from the mboard EEPROM
        device_args -- Arbitrary dictionary of info, typically user-defined
        """
        return []
    # pylint: enable=unused-argument
    ### End of overridables ###################################################


    ###########################################################################
    # Device initialization (at MPM startup)
    ###########################################################################
    def __init__(self):
        # This gets set in the child class
        self.mboard_regs_control = None
        # Note: args is a dictionary.
        assert self.pids
        assert self.mboard_eeprom_magic is not None
        self.dboards = []
        self._default_args = ""
        # Set up logging
        self.log = get_logger('PeriphManager')
        self.claimed = False
        try:
            self.mboard_info = self._get_mboard_info()
            self.log.info("Device serial number: {}"
                          .format(self.mboard_info.get('serial', 'n/a')))
            self.dboard_infos = self._get_dboard_info()
            self.device_info = \
                    self.generate_device_info(
                        self._eeprom_head,
                        self.mboard_info,
                        self.dboard_infos
                    )
            self._aux_board_infos = self._get_aux_board_info()
        except BaseException as ex:
            self.log.error("Failed to initialize device: %s", str(ex))
            self._device_initialized = False
            self._initialization_status = str(ex)
        super(PeriphManagerBase, self).__init__()

    def overlay_apply(self):
        """
        Apply FPGA overlay
        """
        self._init_mboard_overlays()

    def init_dboards(self, args):
        """
        Run full initialization of daughter boards if they exist.
        Use 'override_db_pids' args to overwrite number of dboards that get
        initialized.
        """
        self._default_args = self._update_default_args(args)
        self.log.debug("Using default args: {}".format(self._default_args))
        override_db_pids_str = self._default_args.get('override_db_pids')
        if override_db_pids_str:
            override_db_pids = [
                int(x, 0) for x in override_db_pids_str.split(",")
            ]
        else:
            override_db_pids = []
        self._init_dboards(
            self.dboard_infos,
            override_db_pids,
            self._default_args
        )
        self._device_initialized = True
        self._initialization_status = "No errors."

    def _read_mboard_eeprom_data(self, path):
        return eeprom.read_eeprom(
                path,
                self.mboard_eeprom_offset,
                eeprom.MboardEEPROM.eeprom_header_format,
                eeprom.MboardEEPROM.eeprom_header_keys,
                self.mboard_eeprom_magic,
                self.mboard_eeprom_max_len)

    def _read_mboard_eeprom_legacy(self):
        """
        Read out mboard EEPROM.
        Saves _eeprom_head, _eeprom_rawdata as class members where the the
        former is a de-serialized dictionary representation of the data, and the
        latter is a binary string with the raw data.

        If no EEPROM both members are defined and empty.
        """
        if not self.mboard_eeprom_addr:
            self.log.trace("No mboard EEPROM path defined. "
                           "Skipping mboard EEPROM readout.")
            return

        self.log.trace("Reading EEPROM from address `{}'..."
                       .format(self.mboard_eeprom_addr))
        eeprom_paths = get_eeprom_paths(self.mboard_eeprom_addr)
        if not eeprom_paths:
            self.log.error("Could not identify EEPROM paths for %s!",
                           self.mboard_eeprom_addr)
            return

        self.log.trace("Found mboard EEPROM path: %s", eeprom_paths[0])
        (self._eeprom_head, self._eeprom_rawdata) = \
            self._read_mboard_eeprom_data(eeprom_paths[0])

    def _read_mboard_eeprom_by_symbol(self):
        """
        Read out mboard EEPROM.
        Saves _eeprom_head, _eeprom_rawdata as class members where the the
        former is a de-serialized dictionary representation of the data, and the
        latter is a binary string with the raw data.

        If no EEPROM both members are defined and empty.
        """
        if not self.mboard_eeprom_symbol:
            self.log.trace("No mboard EEPROM path defined. "
                           "Skipping mboard EEPROM readout.")
            return

        self.log.trace("Reading EEPROM from address `{}'..."
                       .format(self.mboard_eeprom_symbol))
        eeprom_paths = get_eeprom_paths_by_symbol(self.mboard_eeprom_symbol)
        if not eeprom_paths:
            self.log.error("Could not identify EEPROM paths for %s!",
                           self.mboard_eeprom_addr)
            return
        # There should be exact one item in the dictionary returned by
        # find_eeprom_paths. If so take the value of it as path for further
        # processing
        if not (len(eeprom_paths) == 1):
            raise RuntimeError("System should contain exact one EEPROM file"
                               "for motherboard but found %d." % len(eeprom_paths))
        eeprom_path = str(eeprom_paths.popitem()[1])

        self.log.trace("Found mboard EEPROM path: %s", eeprom_path)
        (self._eeprom_head, self._eeprom_rawdata) = \
            self._read_mboard_eeprom_data(eeprom_path)

    def _read_mboard_eeprom(self):
        """
        Read out mboard EEPROM.

        This is a wrapper call to switch between the support EEPROM layouts.
        """
        if not self.eeprom_search in self._EepromSearch:
            self.log.warning("%s is not a valid EEPROM layout type. "
                             "Skipping readout.")
            return

        self._eeprom_head, self._eeprom_rawdata = {}, b""
        if self.eeprom_search == self._EepromSearch.LEGACY:
            self._read_mboard_eeprom_legacy()
        elif self.eeprom_search == self._EepromSearch.SYMBOL:
            self._read_mboard_eeprom_by_symbol()

        self.log.trace("Found EEPROM metadata: `{}'"
                       .format(str(self._eeprom_head)))
        self.log.trace("Read {} bytes of EEPROM data."
                       .format(len(self._eeprom_rawdata)))

    def _get_mboard_info(self):
        """
        Creates the mboard info dictionary from the EEPROM data.
        """
        if not hasattr(self, "_eeprom_head"):
            # read eeprom if not done already
            self._read_mboard_eeprom()
        mboard_info = self.mboard_info
        if not self._eeprom_head:
            self.log.debug("No EEPROM info: Can't generate mboard_info")
            return mboard_info
        for key in ('pid', 'serial', 'rev', 'eeprom_version'):
            # In C++, we can only handle dicts if all the values are of the
            # same type. So we must convert them all to strings here:
            try:
                mboard_info[key] = str(self._eeprom_head.get(key, ''), 'ascii')
            except TypeError:
                mboard_info[key] = str(self._eeprom_head.get(key, ''))
        if 'pid' in self._eeprom_head:
            if self._eeprom_head['pid'] not in self.pids.keys():
                self.log.error(
                    "Found invalid PID in EEPROM: 0x{:04X}. " \
                    "Valid PIDs are: {}".format(
                        self._eeprom_head['pid'],
                        ", ".join(["0x{:04X}".format(x) for x in self.pids]),
                    )
                )
                raise RuntimeError("Invalid PID found in EEPROM.")
        # The rev_compat is either directly stored in the EEPROM, or we fall
        # back to the the rev itself (because every rev is compatible with
        # itself).
        rev_compat = \
            self._eeprom_head.get('rev_compat', self._eeprom_head.get('rev'))
        try:
            rev_compat = int(rev_compat)
        except (ValueError, TypeError):
            raise RuntimeError(
                "Invalid revision compat info read from EEPROM!"
            )
        # We check if this software is actually compatible with the hardware.
        # In order for the software to be able to understand the hardware, the
        # rev_compat value (stored on the EEPROM) must be smaller or equal to
        # the value stored in the software itself.
        if self.mboard_max_rev is None:
            self.log.warning("Skipping HW/SW compatibility check!")
        else:
            if rev_compat > self.mboard_max_rev:
                raise RuntimeError(
                    "Software is maximally compatible with revision `{}', but "
                    "the hardware has revision `{}' and is minimally compatible "
                    "with hardware revision `{}'. Please upgrade your version of"
                    "MPM in order to use this device."
                    .format(self.mboard_max_rev, mboard_info['rev'], rev_compat)
                )
        return mboard_info

    def _read_dboard_eeprom_data(self, path):
        return eeprom.read_eeprom(
            path,
            self.dboard_eeprom_offset,
            eeprom.DboardEEPROM.eeprom_header_format,
            eeprom.DboardEEPROM.eeprom_header_keys,
            self.dboard_eeprom_magic,
            self.dboard_eeprom_max_len)

    def _get_dboard_info_legacy(self):
        """
        Read back EEPROM info from the daughterboards
        """
        if self.dboard_eeprom_addr is None:
            self.log.debug("No dboard EEPROM addresses given.")
            return []
        dboard_eeprom_addrs = self.dboard_eeprom_addr \
                              if isinstance(self.dboard_eeprom_addr, list) \
                              else [self.dboard_eeprom_addr]
        dboard_eeprom_paths = []
        self.log.trace("Identifying dboard EEPROM paths from addrs `{}'..."
                       .format(",".join(dboard_eeprom_addrs)))
        for dboard_eeprom_addr in dboard_eeprom_addrs:
            self.log.trace("Resolving %s...", dboard_eeprom_addr)
            dboard_eeprom_paths += get_eeprom_paths(dboard_eeprom_addr)
        self.log.trace("Found dboard EEPROM paths: {}"
                       .format(",".join(dboard_eeprom_paths)))
        if len(dboard_eeprom_paths) > self.max_num_dboards:
            self.log.warning("Found more EEPROM paths than daughterboards. "
                             "Ignoring some of them.")
            dboard_eeprom_paths = dboard_eeprom_paths[:self.max_num_dboards]
        dboard_info = []
        for dboard_idx, dboard_eeprom_path in enumerate(dboard_eeprom_paths):
            self.log.debug("Reading EEPROM info for dboard %d...", dboard_idx)
            dboard_eeprom_md, dboard_eeprom_rawdata = \
                self._read_dboard_eeprom_data(dboard_eeprom_path)
            self.log.trace("Found dboard EEPROM metadata: `{}'"
                           .format(str(dboard_eeprom_md)))
            self.log.trace("Read %d bytes of dboard EEPROM data.",
                           len(dboard_eeprom_rawdata))
            db_pid = dboard_eeprom_md.get('pid')
            if db_pid is None:
                self.log.warning("No dboard PID found in dboard EEPROM!")
            else:
                self.log.debug("Found dboard PID in EEPROM: 0x{:04X}"
                               .format(db_pid))
            dboard_info.append({
                'eeprom_md': dboard_eeprom_md,
                'eeprom_rawdata': dboard_eeprom_rawdata,
                'pid': db_pid,
            })
        return dboard_info

    def _get_board_info_by_symbol(self, symbols):
        """
        Collect board info for given symbols.
        symbols: a (glob) expression identifying EEPROMs to search for
        returns: dictionary of EEPROM content found with symbol name as key
                 and an dictonary wih metadeta, rawdata and pid as value
        """
        result = {}
        self.log.trace("Identifying EEPROM paths from %s...", symbols)
        eeprom_paths = get_eeprom_paths_by_symbol(symbols)
        self.log.trace("Found EEPROM paths: %s", eeprom_paths)
        for name, path in eeprom_paths.items():
            self.log.debug("Reading EEPROM info for %s...", name)
            if not path:
                if "db" in name:
                    # In order to support having a single dboard in slot 1
                    # with slot 0 empty on a x4xx, we pretend that there is
                    # a dummy "EmptyDaughterboard" here.
                    self.log.debug("Not present. Inserting dummy DB info")
                    result[name] = {
                        'eeprom_md': {'serial': 'deadbee', 'pid': 0x0},
                        'eeprom_raw': [],
                        'pid': 0x0
                    }
                else:
                    self.log.debug("Not present. Skipping board")
                continue
            try:
                eeprom_md, eeprom_rawdata = self._read_dboard_eeprom_data(path)
                self.log.trace("Found EEPROM metadata: `{}'"
                               .format(str(eeprom_md)))
                self.log.trace("Read %d bytes of dboard EEPROM data.",
                               len(eeprom_rawdata))
                pid = eeprom_md.get('pid')
                if pid is None:
                    self.log.warning("No PID found in EEPROM!")
                else:
                    self.log.debug("Found PID in EEPROM: 0x{:04X}".format(pid))
                result[name] = {'eeprom_md': eeprom_md,
                                'eeprom_rawdata': eeprom_rawdata,
                                'pid': pid}
            except RuntimeError as e:
                self.log.warning("Could not read EEPROM for %s (%s)", name, e)

        return result

    def _get_dboard_info_by_symbol(self):
        """
        Read back EEPROM info from the daughterboards
        """
        dboard_info = self._get_board_info_by_symbol(self.dboard_eeprom_symbols)
        if len(dboard_info) > self.max_num_dboards:
            self.log.warning("Found more EEPROM paths than daughterboards. "
                             "Ignoring some of them.")
            # dboard_infos keys are sorted so it is safe to remove all items
            # but the first few. Assumption is that the board names are given
            # sorted such as db0, db1, db2, …, dbn.
            dboard_info = {key: val for key, val in dboard_info.items()
                    if key in list(dboard_info.keys())[:self.max_num_dboards]}

        # convert dboard dict back to list for backward compatibility
        return [value for key, value in dboard_info.items() if value]

    def _get_dboard_info(self):
        """
        Read back EEPROM info from the daughterboards
        """
        if not self.eeprom_search in self._EepromSearch:
            self.log.warning("%s is not a valid EEPROM search type. "
                             "Skipping readout.")
            return []

        if self.eeprom_search == self._EepromSearch.LEGACY:
            return self._get_dboard_info_legacy()
        if self.eeprom_search == self._EepromSearch.SYMBOL:
            return self._get_dboard_info_by_symbol()

    def _get_aux_board_info(self):
        """
        Read back EEPROM info from all auxiliary boards
        """
        if self.eeprom_search == self._EepromSearch.LEGACY:
            #legacy has no support for aux board EEPROM read
            return {}
        self.log.debug("Read aux boards EEPROMs")
        result = self._get_board_info_by_symbol(self.auxboard_eeprom_symbols)
        self.log.trace("Found aux board info for: %s.", ", ".join(result.keys()))
        return result

    def _update_default_args(self, default_args):
        """
        Pipe the default_args (that get passed into us from the RPC server)
        through the prefs API. This way, we respect both the config file and
        command line arguments.
        """
        prefs_cache = prefs.get_prefs()
        periph_section_name = None
        if prefs_cache.has_section(self.device_info.get('product')):
            periph_section_name = self.device_info.get('product')
        elif prefs_cache.has_section(self.device_info.get('type')):
            periph_section_name = self.device_info.get('type')
        if periph_section_name is not None:
            prefs_cache.read_dict({periph_section_name: default_args})
            return dict(prefs_cache[periph_section_name])
        # else:
        return default_args

    def _init_mboard_overlays(self):
        """
        Load all required overlays for this motherboard
        """
        requested_overlays = self.list_required_dt_overlays(
            self.device_info,
        )
        self.log.debug("Motherboard requests device tree overlays: {}".format(
            requested_overlays
        ))
        for overlay in requested_overlays:
            dtoverlay.apply_overlay_safe(overlay)
        # Need to wait here a second to make sure the ethernet interfaces are up
        # TODO: Fine-tune this number, or wait for some smarter signal.
        sleep(1)

    def _init_dboards(self, dboard_infos, override_dboard_pids, default_args):
        """
        Initialize all the daughterboards

        dboard_infos -- List of dictionaries as returned from
                       _get_dboard_info()
        override_dboard_pids -- List of dboard PIDs to force
        default_args -- Default args
        """
        if override_dboard_pids:
            self.log.warning("Overriding daughterboard PIDs with: {}"
                             .format(",".join(override_dboard_pids)))
        assert len(dboard_infos) <= self.max_num_dboards
        if override_dboard_pids and \
                len(override_dboard_pids) < len(dboard_infos):
            self.log.warning("--override-db-pids is going to skip dboards.")
            dboard_infos = dboard_infos[:len(override_dboard_pids)]
        for dboard_idx, dboard_info in enumerate(dboard_infos):
            self.log.debug("Initializing dboard %d...", dboard_idx)
            db_pid = dboard_info.get('pid')
            db_class = get_dboard_class_from_pid(db_pid)
            if db_class is None:
                self.log.warning("Could not identify daughterboard class "
                                 "for PID {:04X}! Skipping.".format(db_pid))
                continue
            if len(self.dboard_spimaster_addrs) > dboard_idx:
                spi_nodes = sorted(get_spidev_nodes(
                    self.dboard_spimaster_addrs[dboard_idx]))
                self.log.trace("Found spidev nodes: {0}".format(spi_nodes))
            else:
                spi_nodes = []
                self.log.trace("No SPI nodes for dboard %d.", dboard_idx)
            dboard_info.update({
                'spi_nodes': spi_nodes,
                'default_args': default_args,
            })
            # If the MB supports the DB Iface architecture, pass
            # the corresponding DB Iface to the dboard class
            if self.db_iface is not None:
                dboard_info['db_iface'] = self.db_iface(dboard_idx, self)
            # This will actually instantiate the dboard class:
            self.dboards.append(db_class(dboard_idx, **dboard_info))
        self.log.info("Initialized %d daughterboard(s).", len(self.dboards))

    def _add_public_methods(self, src, prefix="", filter_cb=None, allow_overwrite=False):
        """
        Add public methods (=API) of src to self. To avoid naming conflicts and
        make relations clear, all added method names are prefixed with 'prefix'.

        Example:
        >>> class Foo:
        ...     def print_x(self, x):
        ...         print(x)
        ...
        >>> foo = Foo()
        >>> self._add_public_methods(foo, prefix="ext")
        >>> self.ext_print_x(5) # Prints 5

        :param source: The object to import the API from
        :param prefix: method names in dest will be prefixed with prefix
        :param filter_cb: A callback that returns true if the method should be
                          added. Defaults to always returning True
        :param allow_overwrite: If True, then methods from src will overwrite
                                existing methods on self. Use with care.
        """
        filter_cb = filter_cb or (lambda *args: True)
        assert callable(filter_cb)
        self.log.trace("Adding API functions from %s to %s" % (
            src.__class__.__name__, self.__class__.__name__))
        # append _ to prefix if it is not an empty string
        if prefix:
            prefix = prefix + "_"
        for name in [name for name in dir(src)
                     if not name.startswith("_")
                     and callable(getattr(src, name))
                     and filter_cb(name, getattr(src, name))
                     ]:
            destname = prefix + name
            if hasattr(self, destname) and not allow_overwrite:
                self.log.warn("Cannot add method {} because it would "
                              "overwrite existing method.".format(destname))
            else:
                method = getattr(src, name)
                self.log.trace("Add function %s as %s", name, destname)
                setattr(self, destname, method)


    ###########################################################################
    # Session (de-)initialization (at UHD startup)
    ###########################################################################
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

        Return False on failure, True on success. If daughterboard inits return
        False (any of them), this will also return False.

        args -- A dictionary of args for initialization. Similar to device args
                in UHD.
        """
        self.log.info("init() called with device args `{}'.".format(
            ",".join(['{}={}'.format(x, args[x]) for x in args])
        ))
        if not self._device_initialized:
            self.log.error(
                "Cannot run init(), device was never fully initialized!")
            return False
        if not self.dboards:
            return True
        if args.get("serialize_init", False):
            self.log.debug("Initializing dboards serially...")
            return all((dboard.init(args) for dboard in self.dboards))
        self.log.debug("Initializing dboards in parallel...")
        num_workers = len(self.dboards)
        with futures.ThreadPoolExecutor(max_workers=num_workers) as executor:
            init_futures = [
                executor.submit(dboard.init, args)
                for dboard in self.dboards
            ]
            return all([
                x.result()
                for x in futures.as_completed(init_futures)
            ])

    def deinit(self):
        """
        Clean up after a UHD session terminates.
        This must be safe to call multiple times. The default behaviour is to
        call deinit() on all the daughterboards.
        """
        if not self._device_initialized:
            self.log.error(
                "Cannot run deinit(), device was never fully initialized!")
            return
        self.log.trace("Mboard deinit() called.")
        for slot, dboard in enumerate(self.dboards):
            self.log.trace("call deinit() on dBoard in slot {}".format(slot))
            dboard.deinit()

    def tear_down(self):
        """
        Tear down all members that need to be specially handled before
        deconstruction.
        """
        self.log.trace("Teardown called for Peripheral Manager base.")
        for each in self.dboards:
            each.tear_down()

    ###########################################################################
    # RFNoC & Device Info
    ###########################################################################
    def set_device_id(self, device_id):
        """
        Sets the device ID for this motherboard.
        The device ID is used to identify the RFNoC components associated with
        this motherboard.
        """
        self.log.debug("Setting device ID to `{}'".format(device_id))
        self.mboard_regs_control.set_device_id(device_id)

    def get_device_id(self):
        """
        Gets the device ID for this motherboard.
        The device ID is used to identify the RFNoC components associated with
        this motherboard.
        """
        return self.mboard_regs_control.get_device_id()

    @no_claim
    def get_proto_ver(self):
        """
        Return RFNoC protocol version
        """
        proto_ver = self.mboard_regs_control.get_proto_ver()
        self.log.debug("RFNoC protocol version supported by this device is {}".format(proto_ver))
        return proto_ver

    @no_claim
    def get_chdr_width(self):
        """
        Return RFNoC CHDR width
        """
        chdr_width = self.mboard_regs_control.get_chdr_width()
        self.log.debug("CHDR width supported by the device is {}".format(chdr_width))
        return chdr_width

    ###########################################################################
    # Misc device status controls and indicators
    ###########################################################################
    def get_init_status(self):
        """
        Returns the status of the device after its initialization (that happens
        at startup, not that happens when init() is called).
        The status is a tuple of 2 strings, the first is either "true" or
        "false", depending on whether or not the device initialization was
        successful, and the second is an arbitrary error string.

        Use this function to figure out if something went wrong at bootup, and
        what.
        """
        return [
            "true" if self._device_initialized else "false",
            self._initialization_status
        ]

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

    @no_rpc
    def get_device_info(self):
        """
        Return the device_info dict and add a claimed field.

        Will also call into get_device_info_dyn() for additional information.
        Don't override this function.
        """
        result = {"claimed": str(self.claimed)}
        result.update(self.device_info)
        result.update({
            'name': net.get_hostname(),
            'description': self.description,
        })
        result.update(self.get_device_info_dyn())
        return result

    @no_rpc
    def get_device_info_dyn(self):
        """
        "Dynamic" device info getter. When get_device_info() is called, it
        will also call into this function to see if there is 'dynamic' info
        that needs to be returned. The reason to split up these functions is
        because we don't want anyone to override get_device_info(), but we do
        want periph managers to be able to inject custom device info data.
        """
        self.log.trace("Called get_device_info_dyn(), but not implemented.")
        return {}

    @no_rpc
    def set_connection_type(self, conn_type):
        """
        Specify how the RPC client has connected to this MPM instance. Valid
        values are "remote", "local", or None. When None is given, the value
        is reset.
        """
        assert conn_type in ('remote', 'local', None)
        if conn_type is None:
            self.device_info.pop('rpc_connection', None)
        else:
            self.device_info['rpc_connection'] = conn_type

    @no_claim
    def get_dboard_info(self):
        """
        Returns a list of dicts. One dict per dboard.
        """
        return [dboard.device_info for dboard in self.dboards]

    ###########################################################################
    # Component updating
    ###########################################################################
    @no_claim
    def list_updateable_components(self):
        """
        return list of updateable components
        This method does not require a claim_token in the RPC
        """
        return list(self.updateable_components.keys())

    def update_component(self, metadata_l, data_l):
        """
        Updates the device component specified by comp_dict
        :param metadata_l: List of dictionary of strings containing metadata
        :param data_l: List of binary string with the file contents to be written
        """
        # We need a 'metadata' and a 'data' for each file we want to update
        assert (len(metadata_l) == len(data_l)),\
            "update_component arguments must be the same length"
        # Iterate through the components, updating each in turn
        basepath = os.path.join(os.sep, "tmp", "uploads")
        for metadata, data in zip(metadata_l, data_l):
            id_str = metadata['id']
            filename = os.path.basename(metadata['filename'])
            if id_str not in self.updateable_components:
                self.log.error("{0} not an updateable component ({1})".format(
                    id_str, self.updateable_components.keys()
                ))
                raise KeyError("Update component not implemented for {}".format(id_str))
            self.log.trace("Downloading component: {}".format(id_str))
            if 'md5' in metadata:
                given_hash = metadata['md5']
                comp_hash = md5()
                comp_hash.update(data)
                comp_hash = comp_hash.hexdigest()
                if comp_hash == given_hash:
                    self.log.trace("Component file hash matched: {}".format(
                        comp_hash
                    ))
                else:
                    self.log.error("Component file hash mismatched:\n"
                                   "Calculated {}\n"
                                   "Given      {}\n".format(
                                       comp_hash, given_hash))
                    raise RuntimeError("Component file hash mismatch")
            else:
                self.log.trace("Downloading unhashed {} image.".format(
                    id_str
                ))
            filepath = os.path.join(basepath, filename)
            if not os.path.isdir(basepath):
                self.log.trace("Creating directory {}".format(basepath))
                os.makedirs(basepath)
            self.log.trace("Writing data to {}".format(filepath))
            with open(filepath, 'wb') as comp_file:
                comp_file.write(data)

        # do the actual installation on the device
        for metadata in metadata_l:
            id_str = metadata['id']
            filename = os.path.basename(metadata['filename'])
            filepath = os.path.join(basepath, filename)
            update_func = \
                getattr(self, self.updateable_components[id_str]['callback'])
            self.log.info("Installing component `%s'", id_str)
            update_func(filepath, metadata)
        return True

    @no_claim
    def get_component_info(self, component_name):
        """
        Returns the metadata for the requested component
        :param component_name: string name of the component
        :return: Dictionary of strings containg metadata
        """
        if component_name in self.updateable_components:
            metadata = self.updateable_components.get(component_name)
            metadata['id'] = component_name
            self.log.trace("Component info: {}".format(metadata))
            # Convert all values to str
            return dict([a, str(x)] for a, x in metadata.items())

        self.log.trace("Component not found in updateable components: {}"
                       .format(component_name))
        return {}

    ##########################################################################
    # Mboard Sensors
    ##########################################################################
    def get_mb_sensors(self):
        """
        Return a list of sensor names.
        """
        return list(self.mboard_sensor_callback_map.keys())

    def get_mb_sensor(self, sensor_name):
        """
        Return a dictionary that represents the sensor values for a given
        sensor. If the requested sensor sensor_name does not exist, throw an
        exception.

        The returned dictionary has the following keys (all values are
        strings):
        - name: This is typically the same as sensor_name
        - type: One of the following strings: BOOLEAN, INTEGER, REALNUM, STRING
                Note that this matches uhd::sensor_value_t::data_type_t
        - value: The value. If type is STRING, it is interpreted as-is. If it's
                 REALNUM or INTEGER, it needs to be convertable to float or
                 int, respectively. If it's BOOLEAN, it needs to be either
                 'true' or 'false', although any string that is not 'true' will
                 be interpreted as false.
        - unit: This depends on the type. It is generally only relevant for
                pretty-printing the sensor value.
        """
        if sensor_name not in self.get_mb_sensors():
            error_msg = "Was asked for non-existent sensor `{}'.".format(
                sensor_name
            )
            self.log.error(error_msg)
            raise RuntimeError(error_msg)
        return getattr(
            self, self.mboard_sensor_callback_map.get(sensor_name)
        )()

    ##########################################################################
    # EEPROMS
    ##########################################################################
    def get_mb_eeprom(self):
        """
        Return a dictionary with EEPROM contents

        All key/value pairs are string -> string
        """
        return {
            k: v.decode() if isinstance(v, bytes) else str(v)
            for k, v in iteritems(self._eeprom_head)
        }

    def set_mb_eeprom(self, eeprom_vals):
        """
        eeprom_vals is a dictionary (string -> string)

        By default, we do nothing. Writing EEPROMs is highly device specific
        and is thus defined in the individual device classes.
        """
        self.log.warn("Called set_mb_eeprom(), but not implemented!")
        self.log.debug("Skipping writing EEPROM keys: {}"
                       .format(list(eeprom_vals.keys())))

    def get_db_eeprom(self, dboard_idx):
        """
        Return a dictionary representing the content of the daughterboard
        EEPROM.

        By default, will simply return the device info of the dboard.
        Typically, this gets overloaded by the device specific class.

        Arguments:
        dboard_idx -- Slot index of dboard
        """
        self.log.debug("Calling base-class get_db_eeprom(). This may not be " \
                       "what you want.")
        return self.dboards[dboard_idx].device_info

    def set_db_eeprom(self, dboard_idx, eeprom_data):
        """
        Write new EEPROM contents with eeprom_map.

        Arguments:
        dboard_idx -- Slot index of dboard
        eeprom_data -- Dictionary of EEPROM data to be written. It's up to the
                       specific device implementation on how to handle it.
        """
        self.log.warn("Attempted to write dboard `%d' EEPROM, but function " \
                      "is not implemented.", dboard_idx)
        self.log.debug("Skipping writing EEPROM keys: {}"
                       .format(list(eeprom_data.keys())))

    #######################################################################
    # Transport API
    #######################################################################
    def get_chdr_link_types(self):
        """
        Return a list of ways how the UHD session can connect to this device to
        initiate CHDR traffic.

        The return value is a list of strings. Every string is a key for a
        transport type. Values include:
        - "udp": Means this device can be reached via UDP

        The list is filtered based on what the device knows about where the UHD
        session is. For example, on an N310, it will only return "UDP".

        In order to get further information about how to connect to the device,
        the keys returned from this function can be used with
        get_chdr_link_options().
        """
        raise NotImplementedError("get_chdr_link_types() not implemented.")

    def get_chdr_link_options(self, xport_type):
        """
        Returns a list of dictionaries. Every dictionary contains information
        about one way to connect to this device in order to initiate CHDR
        traffic.

        The interpretation of the return value is very highly dependant on the
        transport type (xport_type).
        For UDP, the every entry of the list has the following keys:
        - ipv4 (IP Address)
        - port (UDP port)
        - link_rate (bps of the link, e.g. 10e9 for 10GigE)

        """
        raise NotImplementedError("get_chdr_link_options() not implemented.")

    #######################################################################
    # Claimer API
    #######################################################################
    def claim(self):
        """
        This is called when the device is claimed, in case the device needs to
        run any actions on claiming (e.g., light up an LED).

        Consider this a "post claim hook", not a function to actually claim
        this device (which happens outside of this class).
        """
        self.log.trace("Device was claimed. No actions defined.")

    def unclaim(self):
        """
        This is called when the device is unclaimed, in case the device needs
        to run any actions on claiming (e.g., turn off an LED).

        Consider this a "post unclaim hook", not a function to actually
        unclaim this device (which happens outside of this class).
        """
        self.log.debug("Device was unclaimed. No actions defined.")

    #######################################################################
    # Timekeeper API
    #######################################################################
    def get_num_timekeepers(self):
        """
        Return the number of timekeepers
        """
        return self.mboard_regs_control.get_num_timekeepers()

    def get_timekeeper_time(self, tk_idx, last_pps):
        """
        Get the time in ticks

        Arguments:
        tk_idx: Index of timekeeper
        next_pps: If True, get time at last PPS. Otherwise, get time now.
        """
        return self.mboard_regs_control.get_timekeeper_time(tk_idx, last_pps)

    def set_timekeeper_time(self, tk_idx, ticks, next_pps):
        """
        Set the time in ticks

        Arguments:
        tk_idx: Index of timekeeper
        ticks: Time in ticks
        next_pps: If True, set time at next PPS. Otherwise, set time now.
        """
        self.mboard_regs_control.set_timekeeper_time(tk_idx, ticks, next_pps)

    def set_tick_period(self, tk_idx, period_ns):
        """
        Set the time per tick in nanoseconds (tick period)

        Arguments:
        tk_idx: Index of timekeeper
        period_ns: Period in nanoseconds
        """
        self.mboard_regs_control.set_tick_period(tk_idx, period_ns)

    def get_clocks(self):
        """
        Gets the RFNoC-related clocks present in the FPGA design
        """
        raise NotImplementedError("get_clocks() not implemented.")

    #######################################################################
    # GPIO API
    #######################################################################
    def get_gpio_banks(self):
        """
        Returns a list of GPIO banks over which MPM has any control
        """
        self.log.debug("get_gpio_banks(): No banks defined on this device.")
        return []

    def get_gpio_srcs(self, bank):
        """
        Return a list of valid GPIO sources for a given bank
        """
        assert bank in self.get_gpio_banks(), \
            "Invalid GPIO bank: {}".format(bank)
        return []

    def get_gpio_src(self, bank):
        """
        Return the currently selected GPIO source for a given bank. The return
        value is a list of strings. The length of the vector is identical to
        the number of controllable GPIO pins on this bank.
        """
        assert bank in self.get_gpio_banks(), \
            "Invalid GPIO bank: {}".format(bank)
        raise NotImplementedError("get_gpio_src() not available on this device!")

    def set_gpio_src(self, bank, src):
        """
        Set the GPIO source for a given bank.
        """
        assert bank in self.get_gpio_banks(), \
            "Invalid GPIO bank: {}".format(bank)
        assert src in self.get_gpio_srcs(bank), \
            "Invalid GPIO source: {}".format(src)
        raise NotImplementedError("set_gpio_src() not available on this device!")

    #######################################################################
    # Sync API
    #######################################################################
    def get_clock_source(self):
        " Returns the currently selected clock source "
        raise NotImplementedError("get_clock_source() not available on this device!")

    def get_time_source(self):
        " Returns the currently selected time source "
        raise NotImplementedError("get_time_source() not available on this device!")

    def get_sync_source(self):
        """
        Gets the current time and clock source
        """
        return {
            "time_source": self.get_time_source(),
            "clock_source": self.get_clock_source(),
        }

    def get_clock_sources(self):
        """
        Returns a list of valid clock sources. This is a list of strings.
        """
        self.log.warning("get_clock_sources() was not specified for this device!")
        return []

    def get_time_sources(self):
        """
        Returns a list of valid time sources. This is a list of strings.
        """
        self.log.warning("get_time_sources() was not specified for this device!")
        return []

    def get_sync_sources(self):
        """
        Returns a list of valid sync sources. This is a list of dictionaries.
        """
        self.log.warning("get_sync_sources() was not specified for this device!")
        return []

    def set_clock_source(self, *args):
        """
        Set a clock source.

        The choice to allow arbitrary arguments is based on historical decisions
        and backward compatibility. UHD/mpmd will call this with a single argument,
        so args[0] is the clock source (as a string).
        """
        raise NotImplementedError("set_clock_source() not available on this device!")

    def set_time_source(self, time_source):
        " Set a time source "
        raise NotImplementedError("set_time_source() not available on this device!")

    def set_sync_source(self, sync_args):
        """
        If a device has no special code for setting the sync-source atomically,
        we simply forward these settings to set_clock_source() and set_time_source()
        (in that order).
        """
        if sync_args not in self.get_sync_sources():
            sync_args_str = \
                ','.join([str(k) + '=' + str(v) for k, v in sync_args.items()])
            self.log.warn(
                f"Attempting to set unrecognized Sync source `{sync_args_str}'!")
        clock_source = sync_args.get('clock_source', self.get_clock_source())
        time_source = sync_args.get('time_source', self.get_time_source())
        self.set_clock_source(clock_source)
        self.set_time_source(time_source)

    ###########################################################################
    # Clock/Time API
    ###########################################################################
    def set_clock_source_out(self, enable=True):
        """
        Allows routing the clock configured as source to the RefOut terminal.
        """
        raise NotImplementedError("set_clock_source_out() not implemented.")

    #######################################################################
    # Discoverable Features
    #######################################################################
    def supports_feature(self, query):
        """
        Returns true if the queried feature is supported by a device.
        """
        return query in self.discoverable_features

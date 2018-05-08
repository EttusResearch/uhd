#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Mboard implementation base class
"""

from __future__ import print_function
import os
from hashlib import md5
from time import sleep
from concurrent import futures
from builtins import str
from builtins import object
from six import iteritems, itervalues
from usrp_mpm.mpmlog import get_logger
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


class PeriphManagerBase(object):
    """"
    Base class for all motherboards. Common function and API calls should
    be implemented here. Motherboard specific information can be stored in
    separate motherboard classes derived from this class
    """
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

    @staticmethod
    def generate_device_info(eeprom_md, mboard_info, dboard_infos):
        """
        Returns a dictionary which describes the device.

        mboard_info -- Dictionary; motherboard info
        device_args -- List of dictionaries; daughterboard info
        """
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
    ### End of overridables ###################################################


    ###########################################################################
    # Device initialization (at MPM startup)
    ###########################################################################
    def __init__(self, args):
        # Note: args is a dictionary.
        assert len(self.pids) > 0
        assert self.mboard_eeprom_magic is not None
        self.dboards = []
        # Set up logging
        self.log = get_logger('PeriphManager')
        self.claimed = False
        # The _init_args are a check for the args that passed into init(). This
        # should always be a dictionary (or dictionary-like object).
        self._init_args = {}
        try:
            self._eeprom_head, self._eeprom_rawdata = \
                self._read_mboard_eeprom()
            self.mboard_info = self._get_mboard_info(self._eeprom_head)
            self.log.info("Device serial number: {}"
                          .format(self.mboard_info.get('serial', 'n/a')))
            dboard_infos = self._get_dboard_eeprom_info()
            self.device_info = \
                    self.generate_device_info(
                        self._eeprom_head,
                        self.mboard_info,
                        dboard_infos
                    )
            self._default_args = self._update_default_args(args)
            self.log.debug("Using default args: {}".format(self._default_args))
            self._init_mboard_overlays()
            override_db_pids_str = self._default_args.get('override_db_pids')
            if override_db_pids_str:
                override_db_pids = [
                    int(x, 0) for x in override_db_pids_str.split(",")
                ]
            else:
                override_db_pids = []
            self._init_dboards(
                dboard_infos,
                override_db_pids,
                self._default_args
            )
            self._device_initialized = True
            self._initialization_status = "No errors."
        except Exception as ex:
            self.log.error("Failed to initialize device: %s", str(ex))
            self._device_initialized = False
            self._initialization_status = str(ex)

    def _read_mboard_eeprom(self):
        """
        Read out mboard EEPROM.
        Returns a tuple: (eeprom_dict, eeprom_rawdata), where the the former is
        a de-serialized dictionary representation of the data, and the latter
        is a binary string with the raw data.

        If no EEPROM is defined, returns empty values.
        """
        if len(self.mboard_eeprom_addr):
            self.log.trace("Reading EEPROM from address `{}'..."
                           .format(self.mboard_eeprom_addr))
            (eeprom_head, eeprom_rawdata) = eeprom.read_eeprom(
                get_eeprom_paths(self.mboard_eeprom_addr)[0],
                self.mboard_eeprom_offset,
                eeprom.MboardEEPROM.eeprom_header_format,
                eeprom.MboardEEPROM.eeprom_header_keys,
                self.mboard_eeprom_magic,
                self.mboard_eeprom_max_len,
            )
            self.log.trace("Found EEPROM metadata: `{}'"
                           .format(str(eeprom_head)))
            self.log.trace("Read {} bytes of EEPROM data."
                           .format(len(eeprom_rawdata)))
            return eeprom_head, eeprom_rawdata
        # Nothing defined? Return defaults.
        self.log.trace("No mboard EEPROM path defined. "
                       "Skipping mboard EEPROM readout.")
        return {}, b''

    def _get_mboard_info(self, eeprom_head):
        """
        Creates the mboard info dictionary from the EEPROM data.
        """
        mboard_info = self.mboard_info
        if not eeprom_head:
            self.log.debug("No EEPROM info: Can't generate mboard_info")
            return mboard_info
        for key in ('pid', 'serial', 'rev', 'eeprom_version'):
            # In C++, we can only handle dicts if all the values are of the
            # same type. So we must convert them all to strings here:
            try:
                mboard_info[key] = str(eeprom_head.get(key, ''), 'ascii')
            except TypeError:
                mboard_info[key] = str(eeprom_head.get(key, ''))
        if 'pid' in eeprom_head:
            if eeprom_head['pid'] not in self.pids.keys():
                self.log.error(
                    "Found invalid PID in EEPROM: 0x{:04X}. " \
                    "Valid PIDs are: {}".format(
                        eeprom_head['pid'],
                        ", ".join(["0x{:04X}".format(x)
                                   for x in self.pids.keys()]),
                    )
                )
                raise RuntimeError("Invalid PID found in EEPROM.")
        if 'rev' in eeprom_head:
            try:
                rev_numeric = int(eeprom_head.get('rev'))
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
        return mboard_info

    def _get_dboard_eeprom_info(self):
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
            dboard_eeprom_md, dboard_eeprom_rawdata = eeprom.read_eeprom(
                dboard_eeprom_path,
                self.dboard_eeprom_offset,
                eeprom.DboardEEPROM.eeprom_header_format,
                eeprom.DboardEEPROM.eeprom_header_keys,
                self.dboard_eeprom_magic,
                self.dboard_eeprom_max_len,
            )
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
        else:
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
                       _get_dboard_eeprom_info()
        override_dboard_pids -- List of dboard PIDs to force
        default_args -- Default args
        """
        if override_dboard_pids:
            self.log.warning("Overriding daughterboard PIDs with: {}"
                             .format(",".join(override_dboard_pids)))
        assert len(dboard_infos) <= self.max_num_dboards
        if len(override_dboard_pids) and \
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
                self.log.warning("No SPI nodes for dboard %d.", dboard_idx)
            dboard_info.update({
                'spi_nodes': spi_nodes,
                'default_args': default_args,
            })
            # This will actually instantiate the dboard class:
            self.dboards.append(db_class(dboard_idx, **dboard_info))
        self.log.info("Initialized %d daughterboard(s).", len(self.dboards))

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
        self._init_args = args
        if len(self.dboards) == 0:
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
        for dboard in self.dboards:
            dboard.deinit()

    def tear_down(self):
        """
        Tear down all members that need to be specially handled before
        deconstruction.
        """
        self.log.trace("Teardown called for Peripheral Manager base.")

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
        # TODO: Update the manifest file

        # Iterate through the components, updating each in turn
        for metadata, data in zip(metadata_l, data_l):
            id_str = metadata['id']
            filename = os.path.basename(metadata['filename'])
            if id_str not in self.updateable_components:
                self.log.error("{0} not an updateable component ({1})".format(
                    id_str, self.updateable_components.keys()
                ))
                raise KeyError("Update component not implemented for {}".format(id_str))
            self.log.trace("Updating component: {}".format(id_str))
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
                self.log.trace("Loading unverified {} image.".format(
                    id_str
                ))
            basepath = os.path.join(os.sep, "tmp", "uploads")
            filepath = os.path.join(basepath, filename)
            if not os.path.isdir(basepath):
                self.log.trace("Creating directory {}".format(basepath))
                os.makedirs(basepath)
            self.log.trace("Writing data to {}".format(filepath))
            with open(filepath, 'wb') as f:
                f.write(data)
            update_func = \
                getattr(self, self.updateable_components[id_str]['callback'])
            self.log.info("Updating component `%s'", id_str)
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
        else:
            self.log.trace("Component not found in updateable components: {}"
                           .format(component_name))
            return {}

    ###########################################################################
    # Crossbar control
    ###########################################################################
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
        return {k: str(v) for k, v in iteritems(self._eeprom_head)}

    def set_mb_eeprom(self, eeprom_vals):
        """
        eeprom_vals is a dictionary (string -> string)

        By default, we do nothing. Writing EEPROMs is highly device specific
        and is thus defined in the individual device classes.
        """
        self.log.warn("Called set_mb_eeprom(), but not implemented!")
        self.log.debug("Skipping writing EEPROM keys: {}"
                       .format(list(eeprom_vals.keys())))
        raise NotImplementedError

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
        raise NotImplementedError

    #######################################################################
    # Transport API
    #######################################################################
    def request_xport(
            self,
            dst_address,
            suggested_src_address,
            xport_type,
        ):
        """
        When setting up a CHDR connection, this is the first call to be
        made. This function will return a list of dictionaries, each
        describing a way to open an CHDR connection. The list of dictionaries
        is sorted by preference, meaning that the caller should use the first
        option, if possible.
        All transports requested are bidirectional.

        The callee must maintain a lock on the available CHDR xports. After
        calling request_xport(), the caller needs to pick one of the
        dictionaries, possibly amend data (e.g., if the connection is an
        Ethernet connection, then we need to know the source port, but more
        details on that in commit_xport()'s documentation).
        One way to implement a lock is to simply lock a mutex here and
        unlock it in commit_xport(), even though there are probably more
        nuanced solutions.

        Arguments:
        dst_sid -- The destination part of the connection, i.e., which
                   RFNoC block are we connecting to. Example: 0x0230
        suggested_src_sid -- The source part of the connection, i.e.,
                             what's the source address of packets going to
                             the destination at dst_sid. This is a
                             suggestion, MPM can override this. Example:
                             0x0001.
        xport_type -- One of the following strings: CTRL, ASYNC_MSG,
                      TX_DATA, RX_DATA. See also xports_type_t in UHD.

        The return value is a list of dictionaries. Every dictionary has
        the following key/value pairs:
        - type: Type of transport, e.g., "UDP", "liberio".
        - ipv4 (UDP only): IPv4 address to connect to.
        - port (UDP only): IP port to connect to.
        - send_sid: String version of the SID used for this transport. This is
                    the definitive version of the SID, the suggested_src_address
                    can be ignored at this point.
        - allocation: This is an integer value which represents a score of
                      how much bandwidth is used. Note: Currently does not
                      have any unit, is just a counter. Higher numbers mean
                      higher utilization. RX means device to UHD, for
                      example, committing an RX streamer would increase this
                      value.
                      This key is optional, MPM does not have to provide it.
                      If the allocation affects the preference, it will be
                      factored into the order of the results, meaning the
                      caller does not strictly have to check its value even if
                      the transport option with the smallest allocation is
                      preferred.

        Note: The dictionary may include other keys which should be ignored,
        or at the very least, kept intact. commit_xport() might be requiring
        them.
        """
        raise NotImplementedError("request_xport() not implemented.")

    def commit_xport(self, xport_info):
        """
        When setting up a CHDR connection, this is the second call to be
        made.

        Arguments:
        xport_info -- A dictionary (string -> string). The dictionary must
                      have been originally created by request_xport(), but
                      additional key/value pairs need to be added.

        All transports need to also provide:
        - rx_mtu: In bytes, the max number of bytes going from device to UHD
        - tx_mtu: In bytes, the max number of bytes going from UHD to device

        UDP transports need to also provide:
        - src_ipv4: IPv4 address the connection is coming from.
        - src_port: IP port the connection is coming from.
        """
        raise NotImplementedError("commit_xport() not implemented.")

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


"""
Copyright 2019 Ettus Research, A National Instrument Brand

SPDX-License-Identifier: GPL-3.0-or-later

This contains the big ImageBuilderConfig class, which is the data structure
that is passed to the templates.
"""

import re
import sys
import copy
import logging

import numpy as np

from .utils import generate_edge_table
from .utils import merge_dicts
from .utils import resolve
from .utils import find_include_file
from . import yaml_utils

# Supported protocol version
RFNOC_PROTO_VERSION = "1.0"

DEVICE_NAME = "_device_"
NONE_PORT = "_none_"


class ImageBuilderConfig:
    """
    Holds the complete image configuration settings. This includes
    * the image configuration itself as it is passed to the script
    * all noc block configurations found by the script
    * device configuration information as found in the bsp.yml of the device
      information passed to the script.
    """

    DEFAULT_CLK_NAMES = ('rfnoc_ctrl', 'rfnoc_chdr')
    DEFAULT_RST_NAMES = ('rfnoc_ctrl', 'rfnoc_chdr')

    # pylint: disable=too-many-instance-attributes
    def __init__(self,
                 config,
                 known_modules,
                 device,
                 include_paths):
        self.rfnoc_version = config.get('rfnoc_version', RFNOC_PROTO_VERSION)
        self.chdr_width = config['chdr_width']
        self.parameters = {}
        self.crossbar_routes = config.get('crossbar_routes', [])
        self.noc_blocks = {}
        self.modules = {}
        self.stream_endpoints = {}
        self.transport_adapters = {}
        self.connections = []
        self.clk_domains = []
        self.resets = []
        self.block_ports = {}
        self.clocks = {}
        self.log = logging.getLogger(__name__)
        self.warnings = []
        self.errors = []

        def _make_store_and_log(store_list, orig_logger):
            """Create a logger that stores messages in a list"""

            def store_and_log(*msg):
                store_list.append(msg[0] % msg[1:])
                orig_logger(*msg)

            return store_and_log

        self.log.warning = _make_store_and_log(self.warnings, self.log.warning)
        self.log.error = _make_store_and_log(self.errors, self.log.error)
        self.log.critical = _make_store_and_log(self.errors, self.log.critical)
        # read configuration from config dictionary
        self.__dict__.update(**config)
        self.device = device
        self._merge_secure_core(
            config.get('secure_image_core', {}),
            getattr(self.device, 'default_secure_image_core', None),
            known_modules['noc_blocks'], known_modules['includes'])
        self._sort_modules()
        self._attach_defs(known_modules)
        self._update_sep_defaults()
        self._check_deprecated_signatures()
        self._set_indices()
        self._resolve_parameters()
        self._collect_noc_ports()
        self._collect_io_ports()
        self._collect_clocks()
        self._check_connections()
        self._check_resets()
        self._check_clk_domains()
        self._annotate_modules()
        self.make_defs = []
        self.constraints = []
        self.dts_includes = []
        self.fpga_includes = []
        self._collect_make_args(include_paths)
        self._collect_fpga_includes(include_paths)
        self.edge_table = generate_edge_table(self)
        self.secure_config = None
        self._check_configuration()
        if config.get('secure_image_core'):
            self.secure_config = self._split_secure_core()

    def _merge_secure_core(self, secure_config, default_secure_config, block_defs, includes):
        """
        Merge the secure core configuration with the rest
        """
        failure = ''
        # Check blocks outside of secure image core if they are allowed to be there
        for name, block in self.noc_blocks.items():
            desc = block_defs[block["block_desc"]]
            if getattr(desc, 'secure_core', None):
                error_msg = f"Invalid configuration: The block {name} must be " \
                            f"placed inside a secure image core."
                self.log.error(error_msg)
                failure += error_msg + "\n"
            if name in secure_config.get('noc_blocks', {}):
                error_msg = f"Invalid configuration: The block {name} is declared " \
                            f"both inside and outside the secure image core."
                self.log.error(error_msg)
                failure += error_msg + "\n"
        def merge_include(target, inc):
            """Helper to merge an include into a dict"""
            self.log.debug("Loading secure image core configuration %s", inc)
            key = inc + '.yml'
            if key not in includes:
                self.log.error("Cannot find include: %s", inc)
                sys.exit(1)
            inc_data = includes[key]
            if not hasattr(inc_data, 'secure_image_core'):
                self.log.error("Include %s does not contain a valid secure image core!",
                              key)
                sys.exit(1)
            merge_dicts(target, inc_data.secure_image_core)
            return target
        if secure_config:
            # The secure image core may use an include statement
            req_includes = secure_config.pop('include', [])
            for inc in req_includes:
                secure_config = merge_include(secure_config, inc)
            # Now fold the secure config onto the main config
            merge_dicts(self.__dict__, secure_config)
        elif default_secure_config:
            self.log.debug("Populating config with default secure core.")
            merge_dicts(self.__dict__, default_secure_config)
        for name in secure_config.get('noc_blocks', {}).keys():
            self.noc_blocks[name]['domain'] = 'secure_core'
        for name in secure_config.get('modules', {}).keys():
            self.modules[name]['domain'] = 'secure_core'
        for name in secure_config.get('transport_adapters', {}).keys():
            self.transport_adapters[name]['domain'] = 'secure_core'
        if failure:
            raise ValueError(failure)

    def _sort_modules(self):
        """ Sort blocks in the desired order.  """
        for sort_modules in ('noc_blocks', 'modules', 'transport_adapters'):
            module_dict = getattr(self, sort_modules)
            setattr(self, sort_modules,
                    dict(sorted(module_dict.items(), key=lambda i: i[1].get('priority', 1))))


    def _attach_defs(self, known_modules):
        """Attach the block/module definitions to the individual block/module instances.
        """
        for module_type, defs in known_modules.items():
            if module_type == "includes":
                continue
            for name, module in getattr(self, module_type).items():
                desc = defs[module['block_desc']]
                setattr(module, 'desc', desc)
                if not hasattr(module.desc, "parameters"):
                    setattr(module.desc, "parameters", {})
                # Populate parameters with defaults from the block/module
                # definition if not set by the user
                default_params = getattr(desc, 'parameters', {})
                user_params = module.get('parameters', {})
                module['parameters'] = {**default_params,
                        **{k: v for k, v in user_params.items() if k in default_params}}
                invalid_keys = [k for k in user_params if k not in default_params]
                if invalid_keys:
                    self.log.warning(
                        "%s contains unknown parameter(s): %s",
                        name, ", ".join(invalid_keys))
        # Also set parameters for the device
        default_params = getattr(self.device, 'parameters', {})
        self.parameters = {
            **default_params,
            **{k: v for k, v in self.parameters.items() if k in default_params}
        }
        invalid_keys = [k for k in self.parameters if k not in default_params]
        if invalid_keys:
            self.log.warning(
                "Device contains unknown parameter(s): %s",
                ", ".join(invalid_keys))
        setattr(self.device, 'parameters', self.parameters)
        # For the sake of symmetry, create a desc attribute on self.device as a copy of itself
        setattr(self.device, 'desc', self.device)

    def _annotate_modules(self):
        """Add information to all the module types.

        This adds the following information:

        - noc_blocks and stream_endpoints get a 'port_index' value, which
          indicates their index on the control crossbar.
        - noc_blocks get a 'blk_index' value, which is the index of the block
          after sorting by priority.
        - transport_adapters and stream_endpoints get a 'xbar_port' value, which
          tells us which crossbar port they are connected to (note that
          transport adapters can have multiple connections to the crossbar, in
          which case it's the first port).
        """
        base_ctrl_port = 1 + len([sep for sep in self.stream_endpoints.values() if sep.get('ctrl')])
        # First, cycle through NoC blocks
        for idx, block_name in enumerate(self.noc_blocks):
            # Add the block index/number. The first block has index 0 and so on.
            # Note the block port number (port on the control crossbar) is
            # added in _set_indices(), because we need to do that for the
            # combined set of SEPs and blocks.
            self.noc_blocks[block_name]['blk_index'] = idx
            self.noc_blocks[block_name]['port_index'] = base_ctrl_port + idx

        if self.get_ta_gen_mode() == "user_defined":
            idx = 0
            for t_a in self.transport_adapters.values():
                t_a['xbar_port'] = idx
                idx += sum(p.get('num_ports', 1) for p in t_a.data.get('inputs', {}).values())
            base_sep_xbar_port = idx
        else:
            base_sep_xbar_port = len(self.device.transports)
        for idx, sep in enumerate(self.stream_endpoints.values()):
            sep['xbar_port'] = idx + base_sep_xbar_port
            sep['port_index'] = idx + 1
        # Connect IO ports from modules
        for module_name in self.modules:
            if module_name in self.noc_blocks:
                raise ValueError(
                    f"Module name {module_name} is already assigned to a NoC block!")
        # Connect IO ports from transport adapters
        for ta_name in self.transport_adapters:
            if ta_name in self.noc_blocks or ta_name in self.modules:
                raise ValueError(
                    f"Transport adapter name {ta_name} is already assigned to "
                    f"a NoC block or module!")

    def _resolve_parameters(self):
        """
        Expand block/module parameters.
        """
        module_types = ['noc_blocks', 'modules', 'transport_adapters']
        for module_type in module_types:
            for _, module in getattr(self, module_type).items():
                module['parameters'] = {
                    k: resolve(v, **module)
                    for k, v in module.get('parameters', {}).items()
                }


    def _check_deprecated_signatures(self):
        """
        Check if the configuration uses deprecated IO signatures or block
        descriptions.
        """
        # Go through blocks and look for any deprecated descriptions
        for _name, block in self.noc_blocks.items():
            desc = block['block_desc']
            if desc in yaml_utils.deprecated_block_yml_map:
                self.log.warning(
                    "The block description '%s' has been deprecated. "
                    "Please update your image to use '%s'.",
                    desc, yaml_utils.deprecated_block_yml_map[desc])
                # Override the block with the new version
                block['block_desc'] = yaml_utils.deprecated_block_yml_map[desc]
        # Go through port connections and look for deprecated names
        for con in self.connections:
            for port in ('srcport', 'dstport'):
                if con[port] in yaml_utils.deprecated_port_name_map:
                    self.log.warning(
                        "The port name '%s' has been deprecated. "
                        "Please update your image to use '%s'.",
                        con[port], yaml_utils.deprecated_port_name_map[con[port]])
                    # Override the name with the new version
                    con[port] = yaml_utils.deprecated_port_name_map[con[port]]

    def _check_configuration(self):
        """Check current configuration."""
        self.log.debug("Running checks on the current configuration...")
        failures = []
        if not any(bool(sep["ctrl"]) for sep in self.stream_endpoints.values()):
            failures = "At least one streaming endpoint needs to have ctrl enabled"
        # Check RFNoC protocol version. Use latest if it was not specified.
        requested_version = self.rfnoc_version
        [requested_major, requested_minor, *_] = requested_version.split('.')
        [supported_major, supported_minor, *_] = RFNOC_PROTO_VERSION.split('.')
        if requested_major != supported_major or requested_minor > supported_minor:
            failures += [
                "Requested RFNoC protocol version (rfnoc_version) " +
                requested_version +
                " is ahead of latest version "
                + RFNOC_PROTO_VERSION
            ]
        elif requested_version != RFNOC_PROTO_VERSION:
            self.log.warning(
                "Generating code for latest RFNoC protocol version %s instead " "of requested %s",
                RFNOC_PROTO_VERSION,
                requested_version,
            )
        self.rfnoc_version = RFNOC_PROTO_VERSION
        # Give block_chdr_width a default value
        if not hasattr(self, "block_chdr_width"):
            self.block_chdr_width = self.chdr_width
        # Check crossbar_routes
        if hasattr(self.device, 'transports') and len(self.transport_adapters) > 0:
            failures += [
                "This device has fixed transports defined, but the image " +
                "configuration contains transport adapters. "
            ]
        num_tas = \
            len(self.device.transports) if self.get_ta_gen_mode() == 'fixed' \
            else sum(ta_info['num_ports'] for ta_info in self.get_ta_info())
        if num_tas == 0:
            failures += [
                "This device has no transports defined. You need to define " +
                "at least one transport adapter in the image configuration."
            ]
        num_seps = len(self.stream_endpoints)
        num_ports = num_tas + num_seps
        if self.crossbar_routes:
            # Check that crossbar_routes is an NxN array of ones and zeros
            routes = np.array(self.crossbar_routes)
            num_digits = np.count_nonzero(routes == 0) + np.count_nonzero(routes == 1)
            if routes.shape != (num_ports, num_ports) or num_digits != num_ports**2:
                failures += [
                    f"crossbar_routes must be a {num_ports} by {num_ports} binary array"
                ]
        else:
            self.log.debug("Generating default crossbar routes...")
            # Give crossbar_routes a default value
            routes = np.ones([num_ports, num_ports])
            # Disable all TA to TA paths, except loopback (required for discovery)
            for i in range(0, num_tas):
                routes[i, 0:num_tas] = 0
                routes[i, i] = 1
            self.crossbar_routes = routes.tolist()
        # Check SEP has buff_size and buff_size_bytes parameters
        for sep_name, sep in self.stream_endpoints.items():
            if "buff_size" not in sep and "buff_size_bytes" not in sep:
                failures += [
                    f"You must specify buff_size or buff_size_bytes for {sep_name}"
                ]
            # Initialize the one not set by user (schema doesn't allow both)
            if "buff_size_bytes" in sep:
                sep["buff_size"] = sep["buff_size_bytes"] // (sep["chdr_width"] // 8)
            elif "buff_size" in sep:
                sep["buff_size_bytes"] = sep["buff_size"] * (sep["chdr_width"] // 8)
        # Run checks on noc blocks/modules/transport adapters/device
        for module_name, module in self.get_module_list("all").items():
            checks = getattr(module.desc, "checks", [])
            for check in checks:
                if "condition" in check:
                    if not bool(resolve(check["condition"], config=self, **module)):
                        if "message" in check:
                            msg = f"Condition failed for {module_name}: " \
                                + resolve(check["message"], config=self, **module)
                        else:
                            msg = f"Condition failed for {module_name}: {check['condition']}"
                        if check.get("severity", "error") == "warning":
                            self.log.warning(msg)
                        else:
                            failures += [msg]
                if "run" in check:
                    try:
                        resolve(check["run"], config=self, **module)
                    except Exception as ex:
                        if "message" in check:
                            failures += [
                                f"Condition failed for {module_name}: "
                                + resolve(check["message"], config=self, **module, error=ex)
                            ]
                        else:
                            failures += [f"Condition failed for {module_name}: {str(ex)}"]
        # Handle any errors
        if failures:
            self.log.error("The image core configuration is invalid:")
            for failure in failures:
                self.log.error("* %s", failure)
            raise ValueError("The image core configuration is invalid.")

    def _split_secure_core(self):
        """
        Split the secure core from the top core.

        Rules:
        - All clocks provided by the device BSP are always routed to the secure core
        - IO ports are routed to the secure core if they are connected to a block
          inside the secure core
        - Blocks/Modules/TAs are only present in the domain they are instantiated
          in
        - Cross-domain connections become IO ports of _device_. For example, if
          block A port X in top domain is connected to block B port Y in secure
          domain, then we create a faux IO port in _device_ called B_Y for the
          top domain, and in the secure domain we create another faux IO port
          for _device_ called A_X. The cross-domain connection is thus split
          into two connections: A.X -> _device_.B_Y in the top domain, and
          _device_.A_X -> B.Y in the secure domain.
        """
        sconfig = copy.deepcopy(self)
        # Sort modules into sconfig and self
        secure_modules = []
        for mod_type in ('noc_blocks', 'modules', 'transport_adapters'):
            setattr(sconfig, mod_type, {})
            module_names = copy.deepcopy(list(getattr(self, mod_type).keys()))
            for module_name in module_names:
                if getattr(self, mod_type)[module_name].get('domain') == "secure_core":
                    secure_modules.append(module_name)
                    getattr(sconfig, mod_type)[module_name] = \
                            getattr(self, mod_type).pop(module_name)
        # Sort connections into sconfig and self
        def is_con_in_secure_core(con, src_dst=None):
            """
            Return true if the source, destination, or any of a connection are in
            the secure core.
            """
            if src_dst:
                return con[f'{src_dst}blk'] in secure_modules
            return is_con_in_secure_core(con, 'src') or \
                    is_con_in_secure_core(con, 'dst')
        for con_type in ('connections', 'clk_domains', 'resets'):
            con_list = getattr(self, con_type)
            setattr(sconfig, con_type, [])
            setattr(self, con_type, [])
            for con in con_list:
                if (con_type == 'clk_domains' and con['srcport'] in self.DEFAULT_CLK_NAMES) or \
                        (con_type == 'resets' and con['srcport'] in self.DEFAULT_RST_NAMES) or \
                        is_con_in_secure_core(con):
                    getattr(sconfig, con_type).append(con.copy())
                if (con_type == 'clk_domains' and con['srcport'] in self.DEFAULT_CLK_NAMES) or \
                        (con_type == 'resets' and con['srcport'] in self.DEFAULT_RST_NAMES) or \
                        not is_con_in_secure_core(con):
                    getattr(self, con_type).append(con.copy())
        # Remove all device IO ports that are not used inside the secure core
        sconfig.device.io_ports = {
            name: port for name, port in sconfig.device.io_ports.items()
            if any(
                (is_con_in_secure_core(c) and \
                        c['dstblk'] == DEVICE_NAME and c['dstport'] == name) or \
                (is_con_in_secure_core(c) and \
                        c['srcblk'] == DEVICE_NAME and c['srcport'] == name)
                for c in sconfig.connections)}

        # Split all clock domain connections and resets
        def get_nonsec_side(con):
            """Helper to determine which side of the connection is not secure"""
            for s_d in ('src', 'dst'):
                if not is_con_in_secure_core(con, s_d):
                    return s_d
            return None
        def get_sec_side(con):
            """Helper to determine which side of the connection is secure"""
            for s_d in ('src', 'dst'):
                if is_con_in_secure_core(con, s_d):
                    return s_d
            return None
        for clk_rst_t in ('clk', 'rst'):
            # Make all clocks that are either output clocks of blocks inside the
            # secure core or are consumed by blocks inside the secure core part of
            # the device clocks
            def is_clk_rst_required(block_name, clock, clk_rst):
                """
                Helper to determine if a clock or reset needs to go into the ports list
                """
                default_names = self.DEFAULT_CLK_NAMES \
                        if clk_rst == 'clk' \
                        else self.DEFAULT_RST_NAMES
                if clock['name'] in default_names:
                    return None
                if clock.get('direction') == 'out':
                    return {
                        'name': f"{block_name}_{clock['name']}",
                        'direction': 'in',
                    }
                # If it's an input clock or reset, we need to make it an input
                # signal to the secure image core if it's being driven from the
                # top domain, and if it's not already being imported.
                con_list = sconfig.clk_domains if clk_rst == 'clk' else sconfig.resets
                clk_con = [
                    c for c in con_list
                    if c['dstblk'] == block_name and c['dstport'] == clock['name']][0]
                if clk_con['srcblk'] == DEVICE_NAME:
                    return None
                clock_name = f"{clk_con['srcblk']}_{clock['name']}"
                ref_list = sconfig.device.clocks if clk_rst == 'clk' else sconfig.device.resets
                if all(clk.get('name') != clock_name for clk in ref_list):
                    return {
                        'name': clock_name,
                        'direction': 'out',
                    }
                return None
            for module_name, module in {
                    **sconfig.noc_blocks,
                    **sconfig.modules,
                    **sconfig.transport_adapters}.items():
                attr_name = 'clocks' if clk_rst_t == 'clk' else 'resets'
                for clock in getattr(module.desc, attr_name, []):
                    clock_info = is_clk_rst_required(module_name, clock, clk_rst_t)
                    if clock_info:
                        getattr(sconfig.device, attr_name).append(clock_info)
            # Make all clock connections to and from the secure core into
            # device clocks on the secure core
            for con_i, con in enumerate(
                    getattr(sconfig, 'clk_domains' if clk_rst_t == 'clk' else 'resets')):
                # Only modify cross-domain connections
                nonsec_side = get_nonsec_side(con)
                if nonsec_side is None:
                    continue
                assert is_con_in_secure_core(con, 'src') != \
                        is_con_in_secure_core(con, 'dst')
                # If the connection goes to the device IO ports, then we've covered
                # this
                if con[f'{nonsec_side}blk'] == DEVICE_NAME:
                    continue
                self.log.info("updating %s", con)
                # Otherwise, we sever the connection and make it a device IO port
                con[f'{nonsec_side}port'] = con['srcblk'] + '_' + con['srcport']
                con[f'{nonsec_side}blk'] = DEVICE_NAME
                self.log.info("-> %s", con)
                # Update the connection info
                getattr(sconfig, 'clk_domains' if clk_rst_t == 'clk' else 'resets')[con_i] = con
        # On the top side, this is a bit simpler. We simply reroute clocks and
        # resets to faux device IO ports.
        for clk_rst_t in ('clk', 'rst'):
            con_list = self.clk_domains if clk_rst_t == 'clk' else self.resets
            new_con_list = []
            for con_i, con in enumerate(con_list):
                sec_side = get_sec_side(con)
                if not sec_side:
                    new_con_list.append(con)
                    continue
                if con['srcblk'] == DEVICE_NAME:
                    continue
                con[f'{sec_side}port'] = con['srcblk'] + '_' + con['srcport']
                con[f'{sec_side}blk'] = DEVICE_NAME
                new_con_list.append(con)
            setattr(self, 'clk_domains' if clk_rst_t == 'clk' else 'resets', new_con_list)
        # Make all IO port connections to and from the secure core into
        # device IO ports on the secure core
        for con_i, con in enumerate(sconfig.connections):
            # Only care about IO ports, not data ports
            if con['srctype'] not in ('master', 'broadcaster'):
                continue
            # Only modify cross-domain connections
            nonsec_side = get_nonsec_side(con)
            if nonsec_side is None:
                continue
            assert is_con_in_secure_core(con, 'src') != \
                    is_con_in_secure_core(con, 'dst')
            # If the connection goes to the device IO ports, then we've covered
            # this
            if con[f'{nonsec_side}blk'] == DEVICE_NAME:
                continue
            # Otherwise, we sever the connection and make it a device IO port
            old_con_blk = con[f'{nonsec_side}blk']
            old_con_port = con[f'{nonsec_side}port']
            old_src_blk = con['srcblk']
            con[f'{nonsec_side}port'] = con['srcblk'] + '_' + con['srcport']
            con[f'{nonsec_side}blk'] = DEVICE_NAME
            sconfig.device.io_ports[con[f'{nonsec_side}port']] = \
                sconfig.get_module(old_con_blk).io_ports[old_con_port]
            for wire in sconfig.device.io_ports[con[f'{nonsec_side}port']]['wires']:
                wire['name'] = old_src_blk + '_' + wire['name']
            for wire in con[f'{nonsec_side}_iosig']['wires']:
                wire['name'] = old_src_blk + '_' + wire['name']
            # Update the connection info
            sconfig.connections[con_i] = con
        # Remove dupes
        unique_connections = []
        for con in sconfig.connections:
            if con not in unique_connections:
                unique_connections.append(con)
        sconfig.connections = unique_connections
        # Same for top domain
        for con_i, con in enumerate(self.connections):
            # Only care about IO ports, not data ports
            if con['srctype'] not in ('master', 'broadcaster'):
                continue
            # Only modify cross-domain connections
            if is_con_in_secure_core(con, 'src'):
                sec_side = 'src'
            elif is_con_in_secure_core(con, 'dst'):
                sec_side = 'dst'
            else:
                continue
            assert is_con_in_secure_core(con, 'src') != \
                    is_con_in_secure_core(con, 'dst')
            # If the connection goes to the device IO ports, then we've covered
            # this
            if con[f'{sec_side}blk'] == DEVICE_NAME:
                continue
            # Otherwise, we sever the connection and make it a device IO port
            old_con_blk, old_con_port = con[f'{sec_side}blk'], con[f'{sec_side}port']
            old_src_blk = con['srcblk']
            con[f'{sec_side}port'] = con['srcblk'] + '_' + con['srcport']
            con[f'{sec_side}blk'] = DEVICE_NAME
            for wire in con[f'{sec_side}_iosig']['wires']:
                wire['name'] = old_src_blk + '_' + wire['name']
            # Update the connection info
            self.connections[con_i] = con
        return sconfig

    def _update_sep_defaults(self):
        """
        Update any missing stream endpoint attributes with default values
        """
        for sep in self.stream_endpoints:
            if "num_data_i" not in self.stream_endpoints[sep]:
                self.stream_endpoints[sep]["num_data_i"] = 1
            if "num_data_o" not in self.stream_endpoints[sep]:
                self.stream_endpoints[sep]["num_data_o"] = 1
            if "chdr_width" not in self.stream_endpoints[sep]:
                self.stream_endpoints[sep]["chdr_width"] = self.chdr_width

    def _set_indices(self):
        """
        Add an index for each port of each stream endpoint and noc block.
        These indices are used to generate static_router.hex
        """
        start = 1
        i = 0
        for i, sep in enumerate(self.stream_endpoints.values()):
            sep["index"] = i + start
        start = start + i + 1
        for i, block in enumerate(self.noc_blocks.values()):
            block["index"] = start + i

    def _collect_noc_ports(self):
        """
        Create lookup table for noc blocks. The key is a tuple of block
        name, port name and flow direction. If any block port has num_ports > 1
        then unroll that port into multiple ports of the same name plus a
        number to make its name unique.
        """
        for name, block in self.noc_blocks.items():
            block['data'] = {}
            # Generate list of block ports, adding 'index' to each port's dict
            for direction in ("inputs", "outputs"):
                block['data'][direction] = {}
                index = 0
                data_ports = getattr(block.desc, "data", {}).get(direction, {}).items()
                if not data_ports:
                    self.log.debug("Block %s has no data %s.", block.desc.module_name, direction)
                for port_name, port_info in data_ports:
                    num_ports = 1
                    if "num_ports" in port_info:
                        parameter = port_info["num_ports"]
                        num_ports = parameter

                    # If num_ports isn't an integer, it could be an expression
                    # using values from the parameters section (e.g.,
                    # NUM_PORTS*NUM_BRANCHES for a stream-splitting block).
                    # If the parameter doesn't resolve to an integer, treat it
                    # as an expression that needs to be evaluated, hopefully to
                    # an integer.
                    if not isinstance(num_ports, int):
                        # Create a regex to find identifiers.
                        regex_ident = re.compile(r'[A-Za-z_][A-Za-z0-9_]*')

                        # Get a list of all identifiers in the num_ports
                        # expression and iterate over them all
                        idents = re.finditer(regex_ident, num_ports)
                        for ident in idents:
                            # If the identifier represents a valid parameter
                            # in the block, replace the identifier text with
                            # the value of the parameter. If no matching
                            # parameter is found, just leave the text in
                            # place. That may result in an exception being
                            # thrown from eval(), but we'll catch it and
                            # report an error a bit later on.
                            if ident[0] in block["parameters"]:
                                val = str(block["parameters"][ident[0]])
                                num_ports = re.sub(ident[0], val, num_ports)

                        # Now, with identifiers resolved to parameter values,
                        # attempt to evaluate the expression. If eval() fails,
                        # we'll catch the exception, num_ports will remain non-
                        # integral, and the if statement after the exception
                        # is caught will inform the user.
                        try:
                            num_ports = eval(num_ports)
                        except:
                            pass

                    # Make sure the parameter resolved to a number
                    if not isinstance(num_ports, int):
                        self.log.error(
                            "'num_ports' of port '%s' on block '%s' "
                            "resolved to invalid value of '%s'",
                            port_name, name, str(num_ports))
                        sys.exit(1)
                    if num_ports < 1 or num_ports > 64:
                        self.log.error(
                            "'num_ports' of port '%s' on block '%s' "
                            "has invalid value '%s', must be in [1, 64]",
                            port_name, name, str(num_ports))
                        sys.exit(1)
                    if "num_ports" in port_info:
                        # If num_ports was a variable in the YAML, unroll into
                        # multiple ports
                        for i in range(num_ports):
                            new_port_info = port_info.copy()
                            new_port_info['index'] = index
                            index = index + 1
                            self.block_ports.update({(name, port_name + "_" \
                                + str(i), direction[:-1]) : new_port_info})
                            block['data'][direction][f'{port_name}_{i}'] = new_port_info
                    else:
                        port_info['index'] = index
                        self.block_ports.update(
                            {(name, port_name, direction[:-1]) : port_info})
                        index = index + 1
                        block['data'][direction][f'{port_name}'] = port_info
        ports = self.stream_endpoints
        for sep in self.stream_endpoints:
            inputs = {(sep, f"in{port}", "input") :
                      ports[sep] for port in range(ports[sep]["num_data_i"])}
            self.block_ports.update(inputs)
            outputs = {(sep, f"out{port}", "output") :
                       ports[sep] for port in range(ports[sep]["num_data_o"])}
            self.block_ports.update(outputs)

    def _collect_io_ports(self):
        """
        Expand IO port information of blocks, module, and the device itself.

        This will create a `io_ports` attribute for every noc block and module.
        On these attributes, parameters are also resolved.
        """
        def expand_io_port(io_port_dict, block):
            """Helper to copy an IO port description onto the block itself"""
            new_io_port = dict(io_port_dict)
            for io_name, io_port in new_io_port.items():
                try:
                    for param_name, param_val in io_port.get('parameters', {}).items():
                        io_port['parameters'][param_name] = \
                            resolve(param_val, config=self, **block)
                    for wire in io_port.get('wires', {}):
                        for wire_k, wire_v in wire.items():
                            wire[wire_k] = resolve(wire_v, config=self, **io_port)
                except (KeyError,) as ex:
                    raise ValueError(f"Error parsing IO port {io_name}") from ex
            return copy.deepcopy(new_io_port)
        for name, block in self.noc_blocks.items():
            try:
                setattr(block, 'io_ports',
                        expand_io_port(getattr(block.desc, 'io_ports', {}), block))
            except ValueError as ex:
                self.log.error("Error parsing IO ports for %s: %s", name, str(ex))
                sys.exit(1)
        for name, module in self.modules.items():
            try:
                setattr(module, 'io_ports',
                        expand_io_port(getattr(module.desc, 'io_ports', {}), module))
            except ValueError as ex:
                self.log.error("Error parsing IO ports for %s: %s", name, str(ex))
                sys.exit(1)
        for name, transport_adapter in self.transport_adapters.items():
            try:
                setattr(transport_adapter, 'io_ports',
                        expand_io_port(
                            getattr(transport_adapter.desc, 'io_ports', {}),
                            transport_adapter))
                # Transport adapters also have data ports
                data_d = transport_adapter.desc.data
                for direction in ('inputs', 'outputs'):
                    for io_k, io_v in data_d.get(direction, {}).items():
                        for k, val in io_v.items():
                            data_d[direction][io_k][k] = resolve(val, **transport_adapter)
                setattr(transport_adapter, 'data', data_d)
            except (ValueError, NameError) as ex:
                self.log.error("Error parsing IO/data ports for %s: %s", name, str(ex))
                sys.exit(1)
        try:
            self.device.io_ports = expand_io_port(self.device.io_ports, {})
        except ValueError as ex:
            self.log.error("Error parsing IO ports for _device_: %s", str(ex))
            sys.exit(1)


    def _collect_clocks(self):
        """
        Create lookup table for clocks. The key is a combination of block name
        (_device_ for clocks of the bsp) and the clock name (e.g.,
        _device_.rfnoc_chdr, or ddc0.ce)
        """
        min_user_clock_index = 10
        clk_indices = {
            1: '_device_.rfnoc_ctrl',
            2: '_device_.rfnoc_chdr',
        }
        def register_clk_index(clock_id, clock_info):
            if 'index' in clock_info:
                clk_index = int(clock_info['index'])
                if clk_index in clk_indices and clk_indices[clk_index] != clock_id:
                    self.log.error(
                        "Conflicting clock indices found. Index %d is defined by " "%s and %s.",
                        clk_index,
                        clk_indices[clk_index],
                        clock_id,
                    )
                    sys.exit(1)
            else:
                # Automatically assign an index
                min_clk_index = max(min_user_clock_index, *list(clk_indices.keys()))
                clk_index = min_clk_index + 1
                self.log.debug("Assigning clock index %d to clock %s.", clk_index, clock_id)
            clk_indices[clk_index] = clock_id
        # Collect clocks from block definitions
        for name, block in self.noc_blocks.items():
            block_clocks = getattr(block.desc, "clocks", {})
            for clock in block_clocks:
                clock_id = name + '.' + clock["name"]
                # Sanitize the direction field: Block clocks are by default inputs
                if 'direction' not in clock:
                    clock['direction'] = 'in'
                self.clocks[clock_id] = clock
        # Collect clocks from device BSP
        for bsp_clk in getattr(self.device, 'clocks', {}):
            clock_id = "_device_." + bsp_clk["name"]
            # Sanitize the direction field: BSP clocks are by default outputs
            if 'direction' not in bsp_clk:
                bsp_clk['direction'] = 'out'
            self.clocks[clock_id] = bsp_clk
            register_clk_index(clock_id, bsp_clk)
        # Collect clocks from generic modules
        for name, module in self.modules.items():
            mod_clocks = getattr(module.desc, "clocks", {})
            for clock in mod_clocks:
                clock_id = name + '.' + clock["name"]
                # Sanitize the direction field: Block clocks are by default inputs
                if 'direction' not in clock:
                    clock['direction'] = 'in'
                self.clocks[clock_id] = clock
        # Some clocks always have to be present. If the BSP does not define them,
        # then we do it for them.
        for required_bsp_clock in "rfnoc_ctrl", "rfnoc_chdr":
            clock_id = "_device_." + required_bsp_clock
            if clock_id not in self.clocks:
                self.log.debug("Adding required clock not present in BSP: %s", required_bsp_clock)
                self.clocks[clock_id] = {
                    "name": required_bsp_clock,
                    # For the index, we do a reverse lookup from clk_indices
                    "index": {clk_id: idx
                              for idx, clk_id in clk_indices.items()
                              if clk_id.startswith('_device_.')
                              }[clock_id]
                }

    def _check_clk_domains(self):
        """
        - Check every clock domain connection for validity
        - Add unconnected clocks if they provide a default clock domain
        """
        failure = ''
        connected_clk_inputs = []
        # Check the given clock domains are valid
        for clk_domain in self.clk_domains:
            clk_src = clk_domain["srcblk"] + '.' + clk_domain["srcport"]
            clk_dst = clk_domain["dstblk"] + '.' + clk_domain["dstport"]
            if not (clk_src in self.clocks and clk_dst in self.clocks):
                failure += f"Invalid clock domain connection: " \
                           f"{clk_src} → {clk_dst}\n"
                continue
            connected_clk_inputs.append(clk_dst)
        # Check if there are unconnected clocks
        for clk, clk_info in self.clocks.items():
            clk_blk, clk_port = clk.split('.', 2)
            if clk_blk != DEVICE_NAME and \
                    clk_port not in self.DEFAULT_CLK_NAMES and \
                    clk not in connected_clk_inputs and \
                    clk_info['direction'] == 'in':
                if 'default' in clk_info:
                    srcclk_blk, srcclk_port = clk_info['default'].split('.', 2)
                    self.clk_domains.append(
                            {"srcblk": srcclk_blk, "srcport": srcclk_port,
                             "dstblk": clk_blk, "dstport": clk_port})
                else:
                    failure += f"Unconnected clock domain: {clk}"
        if failure:
            self.log.error("Clock domains have invalid configuration:\n%s", failure)
            sys.exit(1)
        # Annotate the list of clock connections
        for clk_i, clk_domain in enumerate(self.clk_domains):
            clk_src = clk_domain["srcblk"] + '.' + clk_domain["srcport"]
            clk_dst = clk_domain["dstblk"] + '.' + clk_domain["dstport"]
            self.clk_domains[clk_i]['srcport'] = self.clocks[clk_src]['name']
            self.clk_domains[clk_i]['dstport'] = self.clocks[clk_dst]['name']


    def _get_available_ports(self, io_port):
        """Return a list of available ports for an unconnected IO port.

        - If io_port is a master, then it can only connect to slave ports, and
          vice versa. The other port must be disconnected.
        - If io_port is a broadcaster, then it can only connect to listener ports,
          and vice versa. If we are looking for a broadcaster port, then it may be
          connected.
        - The port types must match.
        """
        desired_drive = {
            "master": "slave",
            "slave": "master",
            "broadcaster": "listener",
            "listener": "broadcaster",
        }[io_port["drive"]]
        desired_srcdst = "src" if desired_drive in ("master", "broadcaster") else "dst"
        desired_type = io_port.get("type")
        desired_disconnected = desired_drive != "broadcaster"
        candidates = []
        for module_name, module in self.get_module_list("all").items():
            for io_port_name, des_io_port in module.io_ports.items():
                if des_io_port.get("drive") == desired_drive and \
                        des_io_port.get("type") == desired_type:
                    candidates.append((module_name, io_port_name, desired_srcdst))
        if desired_disconnected:
            candidates = [
                (module_name, io_port_name, src_or_dst)
                for module_name, io_port_name, src_or_dst in candidates
                if not any(
                    con[f"{src_or_dst}blk"] == module_name and
                    con[f"{src_or_dst}port"] == io_port_name
                    for con in self.connections
                )
            ]
        return candidates


    def _check_connections(self):
        """
        - Validate the list of connections
        - Add annotations to the connections about domain, type, and IO signature
        - Provide error messages is not valid
        """
        failure = ''
        def sanitize_port(con, s_d, failure):
            """Helper to unpack port names (separate slice from port name)"""
            port_match = re.match(r"^([a-z0-9_]+)(?:\[([^]])\])?$", con[f'{s_d}port'])
            if not port_match:
                failure += f"Invalid port name: {con[f'{s_d}port']}\n"
                return con
            con[f'{s_d}port'], con[f'{s_d}slice'] = port_match.groups()
            return con
        ## Phase 1: We go through the list of connections and provide annotations
        for conn_idx, con in enumerate(self.connections):
            for s_d in ('src', 'dst'):
                con = sanitize_port(con, s_d, failure)
            src_blk = self.get_module(con['srcblk'])
            dst_blk = self.get_module(con['dstblk'])
            if (con["srcblk"], con["srcport"], "output") in self.block_ports and \
                    (con["dstblk"], con["dstport"], "input") in self.block_ports:
                con['srctype'] = 'output'
                con['dsttype'] = 'input'
            elif (src_blk.io_ports.get(con['srcport'], {}).get('drive') == 'master' and \
                    dst_blk.io_ports.get(con['dstport'], {}).get('drive') == 'slave') or \
                     (src_blk.io_ports.get(con['srcport'], {}).get('drive') == 'broadcaster' and \
                     dst_blk.io_ports.get(con['dstport'], {}).get('drive') == 'listener'):
                # TODO: Check IO port compatibility (e.g. wire widths)
                con['srctype'] = src_blk.io_ports[con['srcport']]['drive']
                con['dsttype'] = dst_blk.io_ports[con['dstport']]['drive']
                con['src_iosig'] = copy.deepcopy(src_blk.io_ports[con['srcport']])
                con['dst_iosig'] = copy.deepcopy(dst_blk.io_ports[con['dstport']])
                if con['src_iosig'].get('type') != con['dst_iosig'].get('type'):
                    failure += f"IO port type mismatch: {con['srcblk']}.{con['srcport']} " \
                               f"(type: {con['src_iosig'].get('type')}) → " \
                               f"{con['dstblk']}.{con['dstport']} " \
                               f"(type: {con['dst_iosig'].get('type')})\n"
            elif con["srcport"] == NONE_PORT or con["dstport"] == NONE_PORT:
                pass
            else:
                failure += f"Unresolved connection: " \
                           f"{con['srcblk']}:{con['srcport']} → " \
                           f"{con['dstblk']}:{con['dstport']}\n"
            self.connections[conn_idx] = con

        # Go through all the modules and check IO ports are connected
        for module_name, module in self.get_module_list("all").items():
            for io_port_name, io_port in module.io_ports.items():
                required = io_port.get("required")
                if required:
                    is_connected = any(
                        con["srcblk"] == module_name
                        and con["srcport"] == io_port_name
                        or con["dstblk"] == module_name
                        and con["dstport"] == io_port_name
                        for con in self.connections
                    )
                    if not is_connected:
                        available_ports = self._get_available_ports(io_port)
                        msg = f"IO port {module_name}.{io_port_name} is not connected. "
                        if len(available_ports) == 1:
                            msg += f"Suggested connection: " \
                                    f"{available_ports[0][0]}.{available_ports[0][1]}\n"
                            msg += f"Add the following connection to the image core file to " \
                                    f"use this connection:\n"
                            if available_ports[0][2] == "src":
                                msg += f"{{ srcblk: {available_ports[0][0]}, " \
                                        f"srcport: {available_ports[0][1]}, dstblk: " \
                                        f"{module_name}, dstport: {io_port_name} }}"
                            else:
                                msg += f"{{ srcblk: {module_name}, srcport: {io_port_name}, " \
                                        f"dstblk: {available_ports[0][0]}, dstport: " \
                                        f"{available_ports[0][1]} }}"
                        elif len(available_ports) > 1:
                            msg += f"Available connections:\n"
                            for port in available_ports:
                                msg += f"    {port[0]}.{port[1]}\n"
                            msg += "Add a connection line to the image core file to enable " \
                                    "this connection:\n"
                            if available_ports[0][2] == "src":
                                msg += f"{{ srcblk: ..., srcport: ..., dstblk: " \
                                        f"{module_name}, dstport: {io_port_name} }}"
                            else:
                                msg += f"{{ srcblk: {module_name}, srcport: {io_port_name}, " \
                                        f"dstblk: ..., dstport: ... }}"
                        else:
                            msg += f"No available IO ports found!"
                        if required == "recommended":
                            self.log.warning(msg)
                        else:
                            failure += msg
        # Go through blocks and check all ports are connected
        for block_name, block in self.noc_blocks.items():
            for direction in ("inputs", "outputs"):
                for port_name in block["data"][direction]:
                    if not any(
                        con["srcblk"] == block_name
                        and con["srcport"] == port_name
                        or con["dstblk"] == block_name
                        and con["dstport"] == port_name
                        for con in self.connections
                    ):
                        self.log.warning("Block port %s.%s is not connected", block_name, port_name)
        # Drop empty connections
        self.connections = [
            c for c in self.connections if c["srcport"] != NONE_PORT and c["dstport"] != NONE_PORT
        ]

        if failure:
            self.log.error(
                "There are errors in the image core file's connection settings:\n" + failure
            )
            self.log.info("    Make sure block ports are connected output " "(src) to input (dst)")
            self.log.info("    Available block ports for connections:")
            for block in self.block_ports:
                self.log.info("        %s", (block,))
            self.log.info(
                "    Make sure io ports are connected master      " "(src) to slave    (dst)"
            )
            self.log.info(
                "                                  or broadcaster " "(src) to listener (dst)"
            )
            self.log.info("    Available IO ports for connections:")
            for mod_list in (self.noc_blocks, self.modules):
                for module_name, module in mod_list.items():
                    for io_name, io_port in module.io_ports.items():
                        self.log.info("        %s.%s (%s)", module_name, io_name, io_port["type"])
            for io_name, io_port in self.device.io_ports.items():
                self.log.info("        %s.%s (%s)", DEVICE_NAME, io_name, io_port["type"])
            sys.exit(1)

    def _check_resets(self):
        """
        Checks the reset connections. The following checks and modifications are
        performed:
        - Go through the list of resets directly defined in the YAML and make
          sure that they connect valid/existing reset ports, and in the correct
          direction.
        - Identify reset inputs that are unconnected, but define a default reset.
          Create connections for those.
        - Annotate connections about their security domain.
        """
        # Go through resets defined directly in the YAML
        failure = ''
        if not hasattr(self.device, 'resets'):
            setattr(self.device, 'resets', [])
        for reset in self.resets:
            src, dst = reset["srcblk"], reset["dstblk"]
            srcport, dstport = reset["srcport"], reset["dstport"]
            if src != DEVICE_NAME or src not in self.modules:
                failure += f"Cannot connect reset! Unknown source: {src}\n"
                continue
            if dst != DEVICE_NAME or dst not in self.modules:
                failure += f"Cannot connect reset! Unknown destination: {dst}\n"
                continue
            src_module = self.device if src == DEVICE_NAME else self.modules[src]
            dst_module = self.device if dst == DEVICE_NAME else self.modules[dst]
            if srcport not in src_module.resets or \
                    src_module.resets[srcport].get('direction', 'in') != 'out':
                failure += f"Invalid reset source: {src}.{srcport}\n"
            if dstport not in dst_module.resets or \
                    dst_module.resets[srcport].get('direction', 'in') != 'in':
                failure += f"Invalid reset destination: {dst}.{dstport}\n"
        if failure:
            self.log.error("Invalid reset connections:")
            self.log.error(failure)
            sys.exit(1)
        # Now see if there are resets that need auto-connecting
        for module_name, module in self.modules.items():
            for reset in (r for r in module.desc.resets
                    if r.get('direction', 'in') == 'in'):
                if all(r['dstblk'] != module_name and r['dstport'] != reset['name']
                       for r in self.resets) and 'default' in reset:
                    rst_src, rst_port = reset['default'].split('.', 2)
                    self.resets.append({
                        'srcblk': rst_src, 'srcport': rst_port,
                        'dstblk': module_name, 'dstport': reset['name']})

    def _collect_make_args(self, include_paths):
        """
        Expand arguments to the make process
        """
        # Collect make arguments from the device
        for make_arg_type in ('make_defs', 'constraints', 'dts_includes'):
            for arg in getattr(self.device, make_arg_type, []):
                arg = resolve(arg, parameters=self.parameters).strip()
                if arg:
                    getattr(self, make_arg_type).append(arg)

        for ta in self.transport_adapters.values():
            for make_arg_type in ('make_defs', 'constraints', 'dts_includes'):
                for arg in getattr(ta.desc, make_arg_type, []):
                    arg = resolve(arg, **{**ta, 'device': self.device}).strip()
                    if arg:
                        if make_arg_type == "dts_includes":
                            arg = find_include_file(arg, include_paths)
                        getattr(self, make_arg_type).append(arg)

        def remove_dupes(lst):
            return list(dict.fromkeys(lst))
        self.constraints = remove_dupes(self.constraints)
        self.make_defs = remove_dupes(self.make_defs)
        self.dts_includes = remove_dupes(self.dts_includes)

    def _collect_fpga_includes(self, include_paths):
        """
        Find the include files for the FPGA build process
        """
        def resolve_include_path(include_dict, paths):
            if "include" in include_dict:
                include_dict["include"] = \
                        find_include_file(include_dict["include"], paths)
            return include_dict

        mod_list = {**self.noc_blocks, **self.modules, **self.transport_adapters}
        for block in list(mod_list.values()):
            for inc in getattr(block.desc, 'fpga_includes', []):
                self.fpga_includes.append(resolve(inc, **block))
            if hasattr(block.desc, 'makefile_srcs'):
                self.fpga_includes.append({
                    'include': resolve(block.desc.makefile_srcs, fpga_lib_dir="$(LIB_DIR)/rfnoc")
                })
        self.fpga_includes = list(
            map(lambda x: resolve_include_path(x, include_paths), self.fpga_includes))

    def get_module(self, block_or_module):
        """Helper to reference either a block, module, or _device_"""
        if block_or_module in self.noc_blocks:
            return self.noc_blocks[block_or_module]
        if block_or_module in self.transport_adapters:
            return self.transport_adapters[block_or_module]
        if block_or_module in self.modules:
            return self.modules[block_or_module]
        if block_or_module == DEVICE_NAME:
            return self.device
        if block_or_module in self.stream_endpoints:
            return self.stream_endpoints[block_or_module]
        assert f"Unknown module: {block_or_module}"
        return None

    def get_module_list(self, mod_type, domain='all'):
        """
        Return the module dictionary.

        Arguments:
        mod_type: One of 'noc_blocks', 'transport_adapters', 'modules'.
        domain: One of 'all', 'secure_core', 'top'.
        """
        assert domain in ('top', 'all', 'secure_core')
        assert mod_type in ('noc_blocks', 'transport_adapters', 'modules', 'device', 'all')
        if mod_type == 'all':
            return {
                **self.get_module_list('noc_blocks', domain),
                **self.get_module_list('transport_adapters', domain),
                **self.get_module_list('modules', domain),
                **self.get_module_list('device', domain),
            }
        if mod_type == 'device':
            return {DEVICE_NAME: self.device}
        if domain == 'top':
            return getattr(self, mod_type)
        secure_mods = {}
        if getattr(self, 'secure_config', None):
            secure_mods = getattr(self.secure_config, mod_type)
        if domain == 'secure_core':
            return secure_mods
        return {**getattr(self, mod_type), **secure_mods}

    def render_wire_width(self, wire, pad=8):
        """
        Helper to render a wire's width ([7:0]).
        """
        width = wire.get('width', 1)
        if isinstance(width, int):
            start_idx = width - 1
        else:
            start_idx = f"{width}-1"
        if start_idx == 0:
            return ''.rjust(pad)
        range_str = f"{start_idx}:0".rjust(pad-2)
        return f"[{range_str}]"

    def get_secure_core_def(self):
        """
        Returns the secure image core dictionary.
        """
        secure_core = copy.deepcopy(self.secure_image_core)
        for con in secure_core.get('connections', []):
            for s_d in ('src', 'dst'):
                con.pop(f'{s_d}domain', None)
                con.pop(f'{s_d}_iosig', None)
                con.pop(f'{s_d}type', None)
                w_slice = con.pop(f'{s_d}slice', None)
                if w_slice:
                    con[f'{s_d}port'] += f"[{w_slice}]"
        return {'secure_image_core': secure_core}

    def get_ta_gen_mode(self):
        """
        Returns 'fixed' or 'user_defined'.

        If a device BSP file defines fixed transports, then it returns 'fixed' and the
        assumption is that the image core won't contain any transport adapter modules.

        If the image core YAML contains transport adapters, then it returns 'user_defined'.
        In this case, the BSP file must not define any fixed transports. The transport
        adapter modules will be then generated in the image core.
        """
        return 'fixed' if hasattr(self.device, 'transports') else 'user_defined'

    def get_hdl_parameters(self, block_or_module):
        """
        Helper to collect the HDL parameters of a block or module.

        This is also where hdl_parameters get resolved. Regular parameters are
        resolved in _resolve_parameters(), towards the end of construction.
        """
        mod = self.get_module(block_or_module)
        params = getattr(mod.desc, 'hdl_parameters', mod['parameters'])
        return {k: resolve(v, **mod) for k, v in params.items()}

    def get_ta_info(self):
        """
        Returns a list of dictionaries with compact infos about the transport
        adapters.
        """
        result = []
        for ta_name, t_a in self.get_module_list('transport_adapters').items():
            num_ports = sum(p.get('num_ports', 1)
                            for p in t_a.data.get('inputs', {}).values())
            result.append({
                'name': ta_name,
                'num_ports': num_ports,
                'width': 'CHDR_W',
            })
        return result

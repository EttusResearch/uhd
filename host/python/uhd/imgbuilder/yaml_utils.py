"""
Copyright 2019 Ettus Research, A National Instrument Brand

SPDX-License-Identifier: GPL-3.0-or-later

This module contains methods, helpers, utilities to deal with handling YAML
files for RFNoC. This includes knowledge about paths, specific contents, and
RFNoC-specific extensions.
"""

import re
import json
import logging
import os
import sys
from collections.abc import Mapping
from collections import OrderedDict
from ruamel import yaml

from .utils import merge_dicts

# Allow jsonschema import to fail. If not available no schema validation will
# be done (but warning will be printed for each skipped validation).
try:
    import jsonschema
except ImportError:
    logging.warning("Module jsonschema is not installed. Configuration files "
                    "will not be validated against their schema.")


# List of blocks that have been replaced (old name : new name)
deprecated_block_yml_map = {
    "radio_1x64.yml"        : "radio.yml",
    "radio_2x64.yml"        : "radio.yml",
    "axi_ram_fifo_2x64.yml" : "axi_ram_fifo.yml",
    "axi_ram_fifo_4x64.yml" : "axi_ram_fifo.yml",
}

# List of IO signature types that have been replaced (old name : new name)
deprecated_port_type_map = {
    "ctrl_port"     : "ctrlport",
    "time_keeper"   : "timekeeper",
    "radio_1x32"    : "radio",
    "radio_2x32"    : "radio",
    "radio_8x32"    : "radio",
    "x300_radio"    : "radio",
}

# List of port names that have been replaced (old name : new name)
deprecated_port_name_map = {
    "x300_radio"     : "radio",
    "radio_iface"    : "radio",
    "x300_radio0"    : "radio0",
    "x300_radio1"    : "radio1",
    "radio_ch0"      : "radio0",
    "radio_ch1"      : "radio1",
    "ctrl_port"      : "ctrlport",
    "ctrlport_radio" : "ctrlport",
    "timekeeper"     : "time",
    "time_keeper"    : "time",
}


# pylint: disable=too-few-public-methods
class IOConfig(Mapping):
    """
    Class containing configuration from a yml file.

    Each top level entry is translated into a class member variable. If the
    configuration contains an io_ports section the ports get a wire list which
    is derived from the signature file. This allows easier processing of IO
    ports in the mako templates and failures from yml configuration files fail
    in this script rather than during template processing which is easier to
    track and debug.
    """
    def __init__(self, config, signatures):
        # read configuration from config dictionary
        # TODO: Is this guaranteed ordered?
        self.__dict__.update(**config)
        if hasattr(self, "io_ports"):
            expand_io_port_desc(getattr(self, "io_ports"), signatures)

    def __iter__(self):
        return iter(self.__dict__)

    def __getitem__(self, key):
        return self.__dict__[key]

    def __len__(self) -> int:
        return len(self.__dict__)
# pylint: enable=too-few-public-methods

USRP3_TOP_DIR = os.path.join('usrp3', 'top')

# Subdirectory for the core YAML files
RFNOC_CORE_DIR = os.path.join('rfnoc', 'core')


def get_top_path(fpga_root):
    """
    returns the path where FPGA top level sources reside
    """
    return os.path.join(fpga_root, USRP3_TOP_DIR)


def get_core_config_path(config_path):
    """
    returns the path where core configuration files are stored
    """
    return os.path.join(config_path, RFNOC_CORE_DIR)


def find_file(file_name, base_path, recurse=True):
    """
    Recursive search for a file. Only looks for a file with appropriate
    name without checking for content or accessibility.

    :param file_name: name of file to search for
    :param base_path: root path to start search in
    :return: full path to schema file if found, None else
    """
    if not recurse:
        if os.path.isfile(os.path.join(base_path, file_name)):
            return os.path.join(base_path, file_name)
    for root, _, _ in os.walk(base_path):
        filename = os.path.join(root, file_name)
        if os.path.isfile(filename):
            return filename
    return None


def validate_config(config, config_path):
    """
    Try to validate config.

    config contains a configuration loaded from a yaml file. config is assumed
    to be a dictionary which contains a key 'schema' which determines
    which schema to validate against. The schema (json formatted) needs to be
    located in config_path or any sub folder of it.
    If "jsonschema" module cannot be loaded validation is skipped and config
    is assumed to be valid. This way a configuration can also be loaded if
    "jsonschema" is not available.
    The method raises ValueError if no schema is defined in config or the
    schema defined in config cannot be found. The validation itself may throw
    a jsonschema.exceptions.ValidationError if config does not confirm to its
    schema.
    :param config: a dictionary to validate (loaded from yaml file).
    :param config_path: a path holding schema definitions
    """
    if "jsonschema" not in sys.modules:
        logging.warning("Skip schema validation (missing module jsonschema).")
        return

    if not "schema" in config:
        raise ValueError("Missing schema in configuration.")

    schema_name = config["schema"]
    logging.debug("Validating against schema %s...", schema_name)

    schema_file = find_file(f'{schema_name}.json', config_path)
    if not schema_file:
        raise ValueError(f"Unknown schema: '{schema_name}'.")

    logging.debug("Using schema file %s.", schema_file)

    with open(schema_file, encoding='utf-8') as stream:
        jsonschema.validate(instance=config, schema=json.load(stream))
        logging.debug("Configuration successful validated.")


def load_config_validate(config_file, config_path, allow_inherit=True):
    """
    Wrapper method to unify loading of configuration files.
    Beside loading the configuration (yaml file format) itself from config_file
    this method also validates the configuration against a schema. The root
    element of the configuration must load to a dictionary which contains a
    "schema" key. The value of schema points to a file named {value}.json which
    must be located in config_path or one of its sub folders.
    .. seealso:: validate_config.
    :param config_file: configuration to load
    :param config_path: root path of schema definition files
    :return:
    """
    logging.debug("Loading configuration %s...", config_file)
    with open(config_file, encoding='utf-8') as stream:
        rt_yaml = yaml.YAML(typ='rt')
        config = rt_yaml.load(stream)
        if allow_inherit and 'inherit' in config:
            inherits = config['inherit']
            if not isinstance(inherits, list):
                inherits = [inherits]
            for inherit in inherits:
                logging.debug("Image core file inherits from %s...", inherit)
                parent_file = find_file(inherit, '', False) or \
                        find_file(inherit, os.path.dirname(config_file), False) or \
                        find_file(inherit, get_core_config_path(config_path))
                logging.debug("Found parent file: %s", parent_file)
                if not parent_file:
                    logging.error("Cannot find parent file %s requested by %s!",
                                  inherit, config_file)
                    sys.exit(1)
                parent_config = load_config_validate(
                    parent_file,
                    config_path,
                    allow_inherit=True # Parent files can also inherit, why not
                )
                config = merge_dicts(parent_config, config)
            config.pop('inherit')
        logging.debug("Configuration successful loaded.")
        validate_config(config, config_path)
        return config

# Adapted from code found at
# https://stackoverflow.com/questions/5121931/
#     in-python-how-can-you-load-yaml-mappings-as-ordereddicts
# (Accessed 17 October 2019)
def ordered_load(stream, Loader=yaml.SafeLoader, object_pairs_hook=OrderedDict):
    """
    In Python 3.5, element insertion order into dictionaries is not preserved.
    This function uses an OrderedDict to read a YAML file, which does preserve order.
    """
    class OrderedLoader(Loader):
        pass
    def construct_mapping(loader, node):
        loader.flatten_mapping(node)
        return object_pairs_hook(loader.construct_pairs(node))
    OrderedLoader.add_constructor(
        yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG,
        construct_mapping)
    return yaml.load(stream, OrderedLoader)

def load_config(filename):
    """
    Loads yml configuration from filename.

    Configuration files are searched in folder returned by get_get_config_path.
    This method logs error and exits on IO failure

    :param filename: yml configuration to load
    :return: IO signatures as dictionary
    """
    dirname, basename = os.path.split(filename)
    try:
        with open(filename, encoding='utf-8') as stream:
            logging.debug(
                "Using %s from %s.", basename, os.path.normpath(dirname))
            config = ordered_load(stream)
        return config
    except IOError:
        logging.error("%s misses %s", os.path.normpath(dirname), basename)
        sys.exit(1)

def device_config(config_path, device):
    """
    Load device config from bsp.yml

    Location of bsp.yml is derived from the device chosen in the arguments

    :param config_path: location of core configuration files
    :param device: device to build for
    :return: device configuration as dictionary
    """
    return load_config(os.path.join(config_path, f"{device.lower()}_bsp.yml"))


def io_signatures(config_path, *modules):
    """
    Load IO signatures from io_signatures.yml and modules

    First IO signatures from config_path are loaded. All signatures from
    the config_path are marked as `core` in the `origin` key. Next all
    modules are scanned for a `io_signatures` key on the top level definition
    of the module. If one is found the final signature definitions is extended
    by these signatures and their origin is marked by the name of the module.

    :param config_path: location of core configuration files
    :param modules: a list of modules that might contain additional IO signatures
    :return: IO signatures as dictionary
    """
    result = load_config(os.path.join(config_path, "io_signatures.yml"))
    for key in result:
        result[key]['origin'] = 'core'

    for module in modules:
        for module_file, module_data in module.items():
            for signature, data in module_data.get("io_signatures", {}).items():
                if signature in result:
                    #temporary take the same origin for compare in next line to succeed
                    data['origin'] = result[signature]['origin']
                    if result[signature]==data:
                        logging.debug("Identical signature '%s' from '%s' " \
                                      "already defined in '%s'. Skip",
                                      signature, module_file, data['origin'])
                        continue

                    error = f"Redefinition of signature: '{signature}' in " \
                            f"'{module_file}'. Different definition already " \
                            f"in '{data['origin']}'"
                    logging.error(error)
                    logging.error("Existing definition from '%s': %s",
                                  data['origin'], result[signature])
                    logging.error("New definition from '%s': %s", module_file, data)
                    raise ValueError(error)

                data['origin'] = module_file
                result[signature] = data

    logging.debug("Loaded %d IO signatures", len(result))
    for signature in result:
        logging.debug("\t%s [%s]", signature, result[signature]['origin'])

    return result


def collect_module_paths(config_path, include_paths, module_type):
    """
    Create a list of directories that contain noc block configuration files.
    :param config_path: root path holding configuration files
    :return: list of noc block directories
    """
    # rfnoc blocks
    result = [os.path.join(config_path, 'rfnoc', module_type)] + \
            [os.path.join(x, module_type) for x in include_paths]
    return result


def read_yaml_definitions(*paths):
    """
    Non-recursively search all paths for YAML definitions.
    :param paths: paths to be searched
    :return: dictionary of noc blocks. Key is filename of the block, value
             is an IOConfig object
    """
    blocks = OrderedDict()
    for path in paths:
        for root, _, files, in os.walk(path):
            for filename in files:
                if re.match(r".*\.ya?ml$", filename):
                    with open(os.path.join(root, filename), encoding='utf-8') as stream:
                        data = ordered_load(stream)
                        if filename in deprecated_block_yml_map:
                            logging.warning("Skipping deprecated block description "
                                "%s (%s).", filename, os.path.normpath(root))
                        else:
                            logging.debug("Adding file %s (%s).",
                                         filename, os.path.normpath(root))
                            blocks[filename] = data
    return blocks

def resolve_io_signatures(blocks, signatures, require_schema):
    """
    Resolves the IO signatures in blocks

    Resolving the signatures is postponed from `read_yaml_definitions`
    because each yaml file read could potentially contain additional
    IO signatures that needs to be loaded first.

    :param blocks: OrderedDict of definitions
    :param signatures: signature passed to IOConfig initialization
    :param reqire_schema: only blocks with this schema will be resolved
    """
    for name, data in blocks.items():
        if data.get('schema') == require_schema:
            blocks[name] = IOConfig(data, signatures)

def expand_io_port_desc(io_ports, signatures):
    """
    Add a wires entry to each io port dictionary entry which contains a
    complete list of wires for the specific port according to the information
    in signature file. Each wire entry contains:
    * fully resolved wire name (wire name as in signature or replaced name
      with respect to regular expression if some is given, regular expression
      should use back references to retain original wire name).
    * width in bits
    * direction as input/output depending on whether the port is a
      master/broadcaster or slave/listener and the wire is described as from
      or to master
    :param io_ports: io port dictionary from yml configuration
    :param signatures: signature description from yml configuration
    :return: None
    """
    for io_port in io_ports.values():
        wires = []
        if io_port["type"] in deprecated_port_type_map:
            logging.warning(
                "The IO signature type '%s' has been deprecated. "
                "Please update your block YAML to use '%s'.",
                io_port["type"], deprecated_port_type_map[io_port["type"]])
            io_port["type"] = deprecated_port_type_map[io_port["type"]]
        if io_port["type"] not in signatures:
            logging.error("Unknown IO signature type '%s'.", io_port["type"])
            sys.exit(1)
        for signature in signatures[io_port["type"]]["ports"]:
            width = signature.get("width", 1)
            wire_type = signature.get("type", None)
            drive = io_port["drive"]
            direction = {"master": {"from-master": "input ", "to-master": "output"},
                         "slave":  {"from-master": "output", "to-master": "input "},
                         "broadcaster":  {None: "input "},
                         "listener":  {None: "output"}}[drive][wire_type]

            signature_name = signature["name"]
            if "rename" in io_port:
                signature_name = re.sub(io_port["rename"]["pattern"],
                                        io_port["rename"]["repl"],
                                        signature_name, 1)

            wires.append({"direction": direction,
                          "width": width,
                          "name": signature_name})
            if 'safe' in signature:
                wires[-1]['safe'] = signature['safe']
        io_port["wires"] = wires
        io_port["parameters"] = {**signatures[io_port['type']].get('parameters', {}),
                                 **io_port.get('parameters', {})}

def write_yaml(config, destination):
    """
    Write a config object as a YAML file.
    """
    with open(destination, 'w', encoding='utf-8') as out_file:
        # TODO remove superfluous comments
        yaml.round_trip_dump(config, out_file, default_flow_style=False, indent=4)

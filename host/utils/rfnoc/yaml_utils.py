"""
Copyright 2019 Ettus Research, A National Instrument Brand

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""

import json
import logging
import os
import sys
from ruamel import yaml

# Allow jsonschema import to fail. If not available no schema validation will
# be done (but warning will be printed for each skipped validation).
try:
    import jsonschema
except ImportError:
    logging.warning("Module jsonschema is not installed. Configuration files "
                    "will not be validated against their schema.")


def find_schema(schema_name, config_path):
    """
    Recursive search for schema file. Only looks for a file with appropriate
    name without checking for content or accessibility.
    This check will be performed later when trying to load the schema.
    :param schema_name: name of schema file to search for
    :param config_path: root path to start search in
    :return: full path to schema file if found, None else
    """
    for root, _, _ in os.walk(config_path):
        filename = os.path.join(root, schema_name)
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
    logging.debug("Validate against schema %s.", schema_name)

    schema_file = find_schema('%s.json' % schema_name, config_path)
    if not schema_file:
        raise ValueError("Unknown schema %s." % schema_name)

    logging.debug("Using schema file %s.", schema_file)

    with open(schema_file) as stream:
        jsonschema.validate(instance=config, schema=json.load(stream))
        logging.debug("Configuration successful validated.")


def load_config(config_file, config_path):
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
    logging.debug("Load configuration %s.", config_file)
    with open(config_file) as stream:
        rt_yaml = yaml.YAML(typ='rt')
        config = rt_yaml.load(stream)
        logging.debug("Configuration successful loaded.")
        validate_config(config, config_path)
        return config

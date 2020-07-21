#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
This module contains the logic to read configuration files from the
filesystem and parse configuration data from them. This data includes
anything from the serial number of the radio to the type of hardware
it identifies itself as.
"""

import configparser
from .sample_source import sinks, sources, NullSamples

class Config:
    """This class represents a configuration file for the usrp simulator.
    This file should conform to the .ini format defined by the
    configparser module

    It should have a [sample.source] section and a [sample.sink] section.
    Each section should have a 'class' key, which gives the name of the
    Source/Sink class to instanitate (see the decorators in
    sample_source.py). The other key value pairs in the section are
    passed to the source/sink constructor as strings through **kwargs
    """
    def __init__(self, source_gen, sink_gen):
        self.source_gen = source_gen
        self.sink_gen = sink_gen

    @classmethod
    def from_path(cls, path):
        """Parse a config .ini file from a path"""
        parser = configparser.ConfigParser()
        parser.read(path)
        source_gen = Config._read_sample_section(parser['sample.source'], sources)
        sink_gen = Config._read_sample_section(parser['sample.sink'], sinks)
        return cls(source_gen, sink_gen)

    @staticmethod
    def _read_sample_section(section, lookup):
        args = dict(section)
        class_name = args.pop("class")
        constructor = lookup[class_name]
        def section_gen():
            return constructor(**args)
        return section_gen

    @classmethod
    def default(cls):
        """Return a default config"""
        return cls(NullSamples, NullSamples)

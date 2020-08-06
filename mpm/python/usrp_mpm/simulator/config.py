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
from .sample_source import sinks, sources, NullSamples, from_import_path
from .hardware_presets import presets
import numbers

class HardwareDescriptor:
    """This class contains the various magic numbers that are needed to
    identify specific hardware to UHD
    """
    def __init__(self, product, uhd_device_type, description, pid, serial_num, dboard_class, rfnoc_device_type):
        """
        product -> MPM Product, stored in PeriphManager.mboard_info['product']
            e.g. "e320", "b200"
        uhd_device_type -> A device_type string recognized by UHD's device discovery.
            This is the same identifier that goes into --args=type="..."
            e.g. "e3xx", "n3xx"
        description -> MPM description, user visible. Stored in PeriphManager.description
        pid -> motherboard pid, stored as a key in PeriphManager.pids
            e.g. 0xE320
        serial_num -> Device Specific serial number.
            e.g. "3196D2A"
        dboard_class -> Python class which should be instantiated as a daughterboard
        rfnoc_device_type -> Device Type read from NoC Core Registers
            see defaults.hpp:device_type_t
            e.g. 0xE320, 0xA300
        """
        self.product = product
        self.uhd_device_type = uhd_device_type
        self.description = description
        self.pid = pid
        self.serial_num = serial_num
        self.dboard_class = dboard_class
        self.rfnoc_device_type = rfnoc_device_type

    @classmethod
    def from_dict(cls, dict):
        return cls(dict['product'],
            dict['uhd_device_type'],
            dict['description'],
            dict['pid'],
            dict['serial_num'],
            dict['dboard_class'],
            dict['rfnoc_device_type'])

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
    def __init__(self, source_gen, sink_gen, hardware):
        self.source_gen = source_gen
        self.sink_gen = sink_gen
        self.hardware = hardware

    @classmethod
    def from_path(cls, log, path):
        """Parse a config .ini file from a path"""
        parser = configparser.ConfigParser()
        parser.read(path)
        source_gen = NullSamples
        # Here we read data from a section and then pop it.
        # For some reason, you can't iterate over a section (needed to make a dict),
        # after its been popped.
        if 'sample.source' in parser:
            source_gen = Config._read_sample_section(parser['sample.source'], sources)
            parser.pop('sample.source')
        sink_gen = NullSamples
        if 'sample.sink' in parser:
            sink_gen = Config._read_sample_section(parser['sample.sink'], sinks)
            parser.pop('sample.sink')
        hardware_section = dict(parser['hardware'])
        preset_name = hardware_section.get('preset', None)
        hardware_preset = presets[preset_name].copy() if preset_name is not None else {}
        hardware_preset.update(hardware_section)
        hardware = HardwareDescriptor.from_dict(hardware_preset)
        parser.pop('hardware')
        for unused_section in parser:
            # Python sticks this into all config files
            if unused_section == 'DEFAULT':
                continue
            # This helps stop you from shooting yourself in the foot when you add
            # the [sampel.sink] section
            log.warning("Unrecognized section in config file: {}".format(unused_section))
        return cls(source_gen, sink_gen, hardware)

    @staticmethod
    def _read_sample_section(section, lookup):
        args = dict(section)
        class_name = args.pop("class")
        constructor = None
        if "import_path" in args:
            import_path = args.pop("import_path")
            constructor = from_import_path(class_name, import_path)
        else:
            constructor = lookup[class_name]
        def section_gen():
            return constructor(**args)
        return section_gen

    @classmethod
    def default(cls):
        """Return a default config"""
        hardware = dict(presets['E320'])
        # For the uninitiated, this is how you spell Fake Device in hex
        hardware['serial_num'] = "FA4EDE7"
        hardware = HardwareDescriptor.from_dict(hardware)
        return cls(NullSamples, NullSamples, hardware)

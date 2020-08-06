#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
This module contains the interface for providing data to a simulator
stream and receiving data from a simulator stream.
"""
import importlib.util

sources = {}
sinks = {}

def cli_source(cls):
    """This decorator adds a class to the global list of SampleSources"""
    sources[cls.__name__] = cls
    return cls

def cli_sink(cls):
    """This decorator adds a class to the global list of SampleSinks"""
    sinks[cls.__name__] = cls
    return cls

name_index = 0
module_lookup = {}
def from_import_path(class_name, import_path):
    global name_index
    global module_lookup
    module = None
    if import_path in module_lookup:
        module = module_lookup[import_path]
    else:
        spec = importlib.util.spec_from_file_location("simsample." + str(name_index), import_path)
        name_index =+ 1
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        module_lookup[import_path] = module
    return getattr(module, class_name)

class SampleSource:
    """This class defines the interface of a SampleSource. It
    provides samples to the simulator which are then sent over the
    network to a UHD client.
    """
    def fill_packet(self, packet, payload_size):
        """This method should fill the packet with enough samples to
        make its payload payload_size bytes long.
        Returning None signals that this source is exhausted.
        """
        raise NotImplementedError()

    def close(self):
        """Use this to clean up any resources held by the object"""
        raise NotImplementedError()

class SampleSink:
    """This class provides the interface of a SampleSink. It serves
    as a destination for smaples received over the network from a
    UHD client.
    """
    def accept_packet(self, packet):
        """Called whenever a new packet is received"""
        raise NotImplementedError()

    def close(self):
        """Use this to clean up any resources held by the object"""
        raise NotImplementedError()

@cli_source
@cli_sink
class NullSamples(SampleSource, SampleSink):
    """This combination source/sink simply provides an infinite
    number of samples with a value of zero. You may optionally provide
    a log object which will enable debug output.
    """
    def __init__(self, log=None):
        self.log = log

    def fill_packet(self, packet, payload_size):
        if self.log is not None:
            self.log.debug("Null Source called, providing {} bytes of zeroes".format(payload_size))
        payload = bytes(payload_size)
        packet.set_payload_bytes(payload)
        return packet

    def accept_packet(self, packet):
        if self.log is not None:
            self.log.debug("Null Source called, accepting {} bytes of payload"
                           .format(len(packet.get_payload_bytes())))

    def close(self):
        pass

class IOSource(SampleSource):
    """This adaptor class creates a sample source using a read object
    that provides a read(# of bytes) function.
    (e.g. the result of an open("<filename>", "rb") call)
    """
    def __init__(self, read):
        self.read_obj = read

    def fill_packet(self, packet, payload_size):
        payload = self.read_obj.read(payload_size)
        if len(payload) == 0:
            return None
        packet.set_payload_bytes(payload)
        return packet

    def close(self):
        self.read_obj.close()

class IOSink(SampleSink):
    """This adaptor class creates a sample sink using a write object
    that provides a write(bytes) function.
    (e.g. the result of an open("<filename>", "wb") call)
    """
    def __init__(self, write):
        self.write_obj = write

    def accept_packet(self, packet):
        payload = packet.get_payload_bytes()
        written = self.write_obj.write(bytes(payload))
        assert written == len(payload)

    def close(self):
        self.write_obj.close()

@cli_source
class FileSource(IOSource):
    """This class creates a SampleSource using a file path"""
    def __init__(self, read_file, repeat=False):
        self.open = lambda: open(read_file, "rb")
        if isinstance(repeat, bool):
            self.repeat = repeat
        else:
            self.repeat = repeat == "True"
        read = self.open()
        super().__init__(read)

    def fill_packet(self, packet, payload_size):
        payload = self.read_obj.read(payload_size)
        if len(payload) == 0:
            if self.repeat:
                self.read_obj.close()
                self.read_obj = self.open()
                payload = self.read_obj.read(payload_size)
            else:
                return None
        packet.set_payload_bytes(payload)
        return packet

@cli_sink
class FileSink(IOSink):
    """This class creates a SampleSink using a file path"""
    def __init__(self, write_file):
        write = open(write_file, "wb")
        super().__init__(write)

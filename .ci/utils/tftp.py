#!/usr/bin/env python3

import asyncio
import py3tftp.protocols
import pyroute2
import socket
import threading
from pathlib import Path


class FileReaderSingle:
    def __init__(self, path, fname_req, chunk_size=0):
        self.path = path
        # TODO: Should check fname_req against actual name
        self.chunk_size = chunk_size
        self._f = None
        self._f = open(self.path, 'rb')
        self.finished = False

    def file_size(self):
        return self.path.stat().st_size

    def read_chunk(self, size=None):
        size = size or self.chunk_size
        if self.finished:
            return b''

        data = self._f.read(size)
        if not data or (size > 0 and len(data) < size):
            self._f.close()
            self.finished = True

        return data

    def __del__(self):
        if self._f and not self._f.closed:
            self._f.close()


class TFTPServerSingle(py3tftp.protocols.BaseTFTPServerProtocol):
    def __init__(self, path, host_interface, loop, extra_opts):
        super().__init__(host_interface, loop, extra_opts)
        self.path = path

    def select_protocol(self, packet):
        if packet.is_rrq():
            return py3tftp.protocols.RRQProtocol
        raise py3tftp.protocols.ProtocolException("Unhandled protocol")

    def select_file_handler(self, packet):
        if packet.is_rrq():
            return lambda filename, opts: FileReaderSingle(self.path, filename, opts)


class TFTPServer:
    """
    Simple TFTP server, meant to be short-lived and capable of serving a single
    file only
    """
    def __init__(self, filename, remote_ip, port=None):
        self.path = Path(filename).absolute()
        assert self.path.exists()
        assert self.path.is_file()

        self.filename = self.path.name

        if port == None:
            with socket.socket() as s:
                s.bind(('', 0))
                self.port = s.getsockname()[1]
        else:
            self.port = port

        with pyroute2.IPRoute() as ipr:
            r = ipr.route('get', dst=remote_ip)
            for attr in r[0]['attrs']:
                if attr[0] == 'RTA_PREFSRC':
                    self.ip = attr[1]

    def __enter__(self):
        self.loop = asyncio.new_event_loop()
        listen = self.loop.create_datagram_endpoint(
                lambda: TFTPServerSingle(self.path, self.ip, self.loop, {}),
                local_addr=(self.ip, self.port))

        def start_loop(loop):
            asyncio.set_event_loop(loop)
            loop.run_forever()

        self.transport, protocol = self.loop.run_until_complete(listen)
        self.thread = threading.Thread(target=start_loop, args=(self.loop,))
        self.thread.start()
        return self

    def __exit__(self, type, value, exc):
        self.transport.close()
        self.loop.call_soon_threadsafe(self.loop.stop)
        self.thread.join()

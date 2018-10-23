#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
User EEPROM via Bfrfs mixin class
"""

import threading
from six import iterkeys, iteritems
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.sys_utils.udev import get_eeprom_paths
from usrp_mpm.bfrfs import BufferFS

DEFAULT_EEPROM_BLOCK_SIZE = 1024 # bytes

def _get_user_eeprom_info(rev, user_eeprom_map):
    """
    Return an EEPROM access map based on the rev. It picks an entry from
    user_eeprom_map that matches the rev.
    """
    rev_for_lookup = rev
    while rev_for_lookup not in user_eeprom_map:
        if rev_for_lookup < 0:
            raise RuntimeError("Could not find a user EEPROM map for "
                               "revision %d!", rev)
        rev_for_lookup -= 1
    assert rev_for_lookup in user_eeprom_map, \
            "Invalid EEPROM lookup rev!"
    return user_eeprom_map[rev_for_lookup]


class BfrfsEEPROM(object):
    """
    Mixin class to give classes user-EEPROM capabilities.
    """
    # This map describes how the user data is stored in EEPROM. If a dboard rev
    # changes the way the EEPROM is used, we add a new entry. If a dboard rev
    # is not found in the map, then we go backward until we find a suitable rev
    user_eeprom = {}
    # Note: the attributes are created by derived class (the class we are mixing
    #       into), so all logs in the BfrfsEEPROM class will be under the
    #       derived class's category, etc.
    log = None
    rev = None
    slot_idx = None


    def __init__(self):
        # Sanity check on the attributes. These need to be set properly by the
        # parent class.
        assert self.user_eeprom
        assert self.log is not None
        assert self.rev is not None
        assert self.slot_idx is not None
        self.eeprom_fs, self.eeprom_path = self._init_user_eeprom(
            _get_user_eeprom_info(self.rev, self.user_eeprom)
        )


    def _init_user_eeprom(self, eeprom_info):
        """
        Reads out user-data EEPROM, and intializes a BufferFS object from that.
        """
        self.log.trace("Initializing EEPROM user data...")
        eeprom_paths = get_eeprom_paths(eeprom_info.get('label'))
        self.log.trace("Found the following EEPROM paths: `{}'".format(
            eeprom_paths))
        eeprom_path = eeprom_paths[self.slot_idx]
        self.log.trace("Selected EEPROM path: `{}'".format(eeprom_path))
        user_eeprom_offset = eeprom_info.get('offset', 0)
        self.log.trace("Selected EEPROM offset: %d", user_eeprom_offset)
        user_eeprom_data = open(eeprom_path, 'rb').read()[user_eeprom_offset:]
        self.log.trace("Total EEPROM size is: %d bytes", len(user_eeprom_data))
        return BufferFS(
            user_eeprom_data,
            max_size=eeprom_info.get('max_size'),
            alignment=eeprom_info.get('alignment', DEFAULT_EEPROM_BLOCK_SIZE),
            log=self.log
        ), eeprom_path

    def get_user_eeprom_data(self):
        """
        Return a dict of blobs stored in the user data section of the EEPROM.
        """
        return {
            blob_id: self.eeprom_fs.get_blob(blob_id)
            for blob_id in iterkeys(self.eeprom_fs.entries)
        }

    def set_user_eeprom_data(self, eeprom_data):
        """
        Update the local EEPROM with the data from eeprom_data.

        The actual writing to EEPROM can take some time, and is thus kicked
        into a background task. Don't call set_user_eeprom_data() quickly in
        succession. Also, while the background task is running, reading the
        EEPROM is unavailable and MPM won't be able to reboot until it's
        completed.
        However, get_user_eeprom_data() will immediately return the correct
        data after this method returns.
        """
        for blob_id, blob in iteritems(eeprom_data):
            self.eeprom_fs.set_blob(blob_id, blob)
        self.log.trace("Writing EEPROM info to `{}'".format(self.eeprom_path))
        eeprom_offset = _get_user_eeprom_info(self.rev, self.user_eeprom)['offset']
        def _write_to_eeprom_task(path, offset, data, log):
            " Writer task: Actually write to file "
            # Note: This can be sped up by only writing sectors that actually
            # changed. To do so, this function would need to read out the
            # current state of the file, do some kind of diff, and then seek()
            # to the different sectors. When very large blobs are being
            # written, it doesn't actually help all that much, of course,
            # because in that case, we'd anyway be changing most of the EEPROM.
            with open(path, 'r+b') as eeprom_file:
                log.trace("Seeking forward to `{}'".format(offset))
                eeprom_file.seek(eeprom_offset)
                log.trace("Writing a total of {} bytes.".format(
                    len(self.eeprom_fs.buffer)))
                eeprom_file.write(data)
                log.trace("EEPROM write complete.")
        thread_id = "eeprom_writer_task_{}".format(self.slot_idx)
        if any([x.name == thread_id for x in threading.enumerate()]):
            # Should this be fatal?
            self.log.warn("Another EEPROM writer thread is already active!")
        writer_task = threading.Thread(
            target=_write_to_eeprom_task,
            args=(
                self.eeprom_path,
                eeprom_offset,
                self.eeprom_fs.buffer,
                self.log
            ),
            name=thread_id,
        )
        writer_task.start()
        # Now return and let the copy finish on its own. The thread will detach
        # and MPM won't terminate this process until the thread is complete.
        # This does not stop anyone from killing this process (and the thread)
        # while the EEPROM write is happening, though.
        #

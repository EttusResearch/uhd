#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
E320 dboard (RF and control) implementation module
"""

import threading
from usrp_mpm import lib # Pulls in everything from C++-land
from usrp_mpm.bfrfs import BufferFS
from usrp_mpm.chips import ADF400x
from usrp_mpm.dboard_manager import DboardManagerBase, AD936xDboard
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.sys_utils.udev import get_eeprom_paths
from usrp_mpm.periph_manager.e320_periphs import MboardRegsControl

DEFAULT_MASTER_CLOCK_RATE = 16e6
###############################################################################
# Main dboard control class
###############################################################################
class Neon(DboardManagerBase, AD936xDboard):
    """
    Holds all dboard specific information and methods of the neon dboard
    """
    #########################################################################
    # Overridables
    #
    # See DboardManagerBase for documentation on these fields
    #########################################################################
    pids = [0xe320]
    rx_sensor_callback_map = {
        'ad9361_temperature': 'get_catalina_temp_sensor',
        'rssi' : 'get_rssi_sensor',
        # For backward compatibility reasons we have the same sensor with two
        # different names
        'lo_lock' : 'get_rx_lo_lock_sensor',
        'lo_locked' : 'get_rx_lo_lock_sensor',
    }
    tx_sensor_callback_map = {
        'ad9361_temperature': 'get_catalina_temp_sensor',
        # For backward compatibility reasons we have the same sensor with two
        # different names
        'lo_lock' : 'get_tx_lo_lock_sensor',
        'lo_locked' : 'get_tx_lo_lock_sensor',
    }
    # Maps the chipselects to the corresponding devices:
    spi_chipselect = {"catalina": 0,
                      "adf4002": 1}
    ### End of overridables #################################################
    # MB regs label: Needed to access the lock bit
    mboard_regs_label = "mboard-regs"
    # This map describes how the user data is stored in EEPROM. If a dboard rev
    # changes the way the EEPROM is used, we add a new entry. If a dboard rev
    # is not found in the map, then we go backward until we find a suitable rev
    user_eeprom = {
        0: {
            'label': "e0004000.i2c",
            'offset': 1024,
            'max_size': 32786 - 1024,
            'alignment': 1024,
        },
    }

    def __init__(self, slot_idx, **kwargs):
        DboardManagerBase.__init__(self, slot_idx, **kwargs)
        AD936xDboard.__init__(
            self, lambda: MboardRegsControl(self.mboard_regs_label, self.log))
        self.log = get_logger("Neon-{}".format(slot_idx))
        self.log.trace("Initializing Neon daughterboard, slot index %d",
                       self.slot_idx)
        self.rev = int(self.device_info['rev'])
        self.log.trace("This is a rev: {}".format(chr(65 + self.rev)))
        # These will get updated during init()
        self.master_clock_rate = None
        # Predeclare some attributes to make linter happy:
        self.eeprom_fs = None
        self.eeprom_path = None
        # Now initialize all peripherals. If that doesn't work, put this class
        # into a non-functional state (but don't crash, or we can't talk to it
        # any more):
        try:
            self._init_periphs()
            self._periphs_initialized = True
        except Exception as ex:
            self.log.error("Failed to initialize peripherals: %s", str(ex))
            self._periphs_initialized = False

    def _init_periphs(self):
        """
        Initialize power and peripherals that don't need user-settings
        """
        self.log.debug("Loading C++ drivers...")
        # Setup the ADF4002
        adf4002_spi = lib.spi.make_spidev(
            str(self._spi_nodes['adf4002']),
            1000000,  # Speed (Hz)
            0  # SPI mode
            )
        self.log.trace("Initializing ADF4002.")
        from usrp_mpm.periph_manager.e320 import E320_DEFAULT_INT_CLOCK_FREQ
        self.adf4002 = ADF400x(adf4002_spi,
                               freq=E320_DEFAULT_INT_CLOCK_FREQ,
                               parent_log=self.log)
        # Set up AD9361 / the Neon Manager
        self._device = lib.dboards.neon_manager(self._spi_nodes['catalina'])
        ad936x_rfic = self._device.get_radio_ctrl()
        self.log.trace("Loaded C++ drivers.")
        self._init_cat_api(ad936x_rfic)
        self.eeprom_fs, self.eeprom_path = self._init_user_eeprom(
            self._get_user_eeprom_info(self.rev)
        )

    def init(self, args):
        """
        Initialize the RFIC portion of the E320 (rest happens in e320.init())
        """
        if not self._periphs_initialized:
            error_msg = "Cannot run init(), peripherals are not initialized!"
            self.log.error(error_msg)
            raise RuntimeError(error_msg)
        master_clock_rate = \
            float(args.get('master_clock_rate', DEFAULT_MASTER_CLOCK_RATE))
        self.init_rfic(master_clock_rate)
        return True

    ###########################################################################
    # Clocking
    ###########################################################################
    def update_ref_clock_freq(self, freq):
        """Update the reference clock frequency"""
        self.adf4002.set_ref_freq(freq)

    ###########################################################################
    # EEPROM Control
    ###########################################################################
    def _get_user_eeprom_info(self, rev):
        """
        Return an EEPROM access map (from self.user_eeprom) based on the rev.
        """
        rev_for_lookup = rev
        while rev_for_lookup not in self.user_eeprom:
            if rev_for_lookup < 0:
                raise RuntimeError(
                    f"Could not find a user EEPROM map for revision {rev}!")
            rev_for_lookup -= 1
        assert rev_for_lookup in self.user_eeprom, \
                "Invalid EEPROM lookup rev!"
        return self.user_eeprom[rev_for_lookup]

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
            alignment=eeprom_info.get('alignment', 1024),
            log=self.log
        ), eeprom_path

    def get_user_eeprom_data(self):
        """
        Return a dict of blobs stored in the user data section of the EEPROM.
        """
        return {
            blob_id: self.eeprom_fs.get_blob(blob_id)
            for blob_id in self.eeprom_fs.entries.keys()
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
        for blob_id, blob in eeprom_data.items():
            self.eeprom_fs.set_blob(blob_id, blob)
        self.log.trace("Writing EEPROM info to `{}'".format(self.eeprom_path))
        eeprom_offset = self.user_eeprom[self.rev]['offset']
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

#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
EISCAT rx board implementation module
"""

import time
from builtins import range
from builtins import object
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.sys_utils.uio import UIO
from usrp_mpm import lib
from usrp_mpm.dboard_manager import DboardManagerBase
from usrp_mpm.dboard_manager.lmk_eiscat import LMK04828EISCAT
from usrp_mpm.cores import ClockSynchronizer

def create_spidev_iface_sane(dev_node):
    """
    Create a regs iface from a spidev node (sane values)
    """
    return lib.spi.make_spidev_regs_iface(
        str(dev_node),
        1000000, # Speed (Hz)
        3, # SPI mode
        8, # Addr shift
        0, # Data shift
        1<<23, # Read flag
        0, # Write flag
    )

def create_spidev_iface_phasedac(dev_node):
    """
    Create a regs iface from a spidev node (ADS5681)
    """
    return lib.spi.make_spidev_regs_iface(
        str(dev_node),
        1000000, # Speed (Hz)
        1, # SPI mode
        20, # Addr shift
        8, # Data shift
        0, # Read flag
        0, # Write flag
    )


class ADS54J56(object):
    """
    Controls for ADS54J56 ADC

    These commands are very specific to the EISCAT daughterboard, so they stay
    here.
    """
    def __init__(self, regs, log):
        self.log = log
        self.regs = regs
        self.sync_line = "AB"

    def swap_sync_line(self, new_value=None):
        """
        Select sync pin value, or switch sync pin over. If new_value is given,
        use that (it has to be either AB or CD), otherwise, pick whatever is
        currently not selected.
        """
        if new_value is not None:
            self.sync_line = new_value
        elif self.sync_line == "AB":
            self.sync_line = "CD"
        else:
            self.sync_line = "AB"
        assert self.sync_line in ('AB', 'CD')
        self.log.trace(
            "The next setup() sequence will use sync pin: {}".format(
                self.sync_line
            )
        )

    def reset(self):
        """
        Perform reset sequence
        """
        self.log.trace("Resetting ADS54J56...")
        self.regs.poke8(0x000000, 0x81) # Analog reset
        self.regs.poke8(0x004004, 0x68) # Page = Main Digital
        self.regs.poke8(0x004003, 0x00) # Page = Main Digital
        self.regs.poke8(0x004002, 0x00) # Page = Main Digital
        self.regs.poke8(0x004001, 0x00) # Page = Main Digital
        self.regs.poke8(0x0060F7, 0x01) # Digital top reset
        self.regs.poke8(0x0070F7, 0x01) # Digital top reset
        self.regs.poke8(0x006000, 0x01) # Reset Digital (IL RESET)
        self.regs.poke8(0x007000, 0x01) # Reset Digital (IL RESET)
        self.regs.poke8(0x006000, 0x00) # Clear Reset
        self.regs.poke8(0x007000, 0x00) # Clear Reset
        self.regs.poke8(0x000011, 0x80) # Select Master page in Analog Bank
        self.regs.poke8(0x000053, 0x80) # Set clk divider to div-2
        self.regs.poke8(0x000039, 0xC0) # ALWAYS WRITE 1 to this bit
        self.regs.poke8(0x000059, 0x20) # ALWAYS WRITE 1 to this bit
        readback_test_addr = 0x11
        readback_test_val = self.regs.peek8(readback_test_addr)
        self.log.trace("ADC readback reg 0x{:x} post-reset: 0x{:x}".format(
            readback_test_addr,
            readback_test_val,
        ))


    def setup(self):
        """
        Enable the ADC for streaming
        """
        self.log.trace("Setting up ADS54J56 for EISCAT operation...")
        self.regs.poke8(0x0011, 0x80) # Select Master page in Analog Bank
        self.regs.poke8(0x0053, 0x80) # Set clk divider to div-2
        self.regs.poke8(0x0039, 0xC0) # ALWAYS WRITE 1 to this bit
        self.regs.poke8(0x0059, 0x20) # ALWAYS WRITE 1 to this bit
        self.regs.poke8(0x4004, 0x68) # Select Main Digital page
        self.regs.poke8(0x4003, 0x00) #
        self.regs.poke8(0x6042, 0x02) # Set interleaving correction to 3rd nyquist zone
        self.regs.poke8(0x604E, 0x80) # Enable correction
        self.regs.poke8(0x7042, 0x02) # Set interleaving correction to 3rd nyquist zone
        self.regs.poke8(0x704E, 0x80) # Enable correction
        self.regs.poke8(0x6000, 0x00) # Reset interleaving engine for Ch A-B (set to 0-1-0)
        self.regs.poke8(0x6000, 0x01) #
        self.regs.poke8(0x6000, 0x00) #
        self.regs.poke8(0x7000, 0x00) # Reset interleaving engine for Ch C-D (set to 0-1-0)
        self.regs.poke8(0x7000, 0x01) #
        self.regs.poke8(0x7000, 0x00) #
        self.regs.poke8(0x4004, 0x61) # Select decimation filter page of JESD bank.
        self.regs.poke8(0x4003, 0x41) #
        self.regs.poke8(0x6000, 0xE4) # DDC Mode 4 for A-B and E = CH A/B N value
        self.regs.poke8(0x7000, 0xE4) # DDC Mode 4 for A-B and E = CH A/B N value
        self.regs.poke8(0x6001, 0x04) # ALWAYS WRITE 1 to this bit
        self.regs.poke8(0x7001, 0x04) # ALWAYS WRITE 1 to this bit
        self.regs.poke8(0x6002, 0x0E) # Ch A/D N value
        self.regs.poke8(0x7002, 0x0E) # Ch A/D N value
        self.regs.poke8(0x4003, 0x00) # Select analog page in JESD Bank
        self.regs.poke8(0x4004, 0x6A) #
        self.regs.poke8(0x6016, 0x02) # PLL mode 40x for A-B
        self.regs.poke8(0x7016, 0x02) # PLL mode 40x for C-D
        self.regs.poke8(0x4003, 0x00) # Select digital page in JESD Bank
        self.regs.poke8(0x4004, 0x69) #
        self.regs.poke8(0x6000, 0x40) # Enable JESD Mode control for A-B
        self.regs.poke8(0x6001, 0x02) # Set JESD Mode to 40x for LMFS=2441
        self.regs.poke8(0x7000, 0x40) # Enable JESD Mode control for C-D
        self.regs.poke8(0x7001, 0x02) # Set JESD Mode to 40x for LMFS=2441
        self.regs.poke8(0x6000, 0x80) # Set CTRL K for A-B
        self.regs.poke8(0x6006, 0x0F) # Set K to 16
        self.regs.poke8(0x7000, 0x80) # Set CTRL K for C-D
        self.regs.poke8(0x7006, 0x0F) # Set K to 16
        # Choose the sync pin. We have both connected up to the FPGA, but we
        # can only use one at a time. Sync pins can become non-functional (e.g.
        # after ESD events) and thus we need the ability to choose between them.
        # In any case, we'll set the pin to issue a SYNC request for all 4
        # channels.
        assert self.sync_line in ("AB", "CD")
        if self.sync_line == "AB":
            self.log.trace("Using SyncAB")
            self.regs.poke8(0x7001, 0x22)
        else:
            self.log.trace("Using SyncCD")
            self.regs.poke8(0x6001, 0x22)
        # This readback is pretty useless, but we use it as a debug mechanic to
        # see if anything is coming back from the chip:
        readback_test_addr = 0x11
        readback_test_val = self.regs.peek8(readback_test_addr)
        self.log.trace("ADC readback reg 0x{:x} post-setup: 0x{:x}".format(
            readback_test_addr,
            readback_test_val,
        ))


class DboardClockControl(object):
    """
    Control the FPGA MMCM for Radio Clock control.
    """
    # Clocking Register address constants
    RADIO_CLK_MMCM      = 0x0020
    PHASE_SHIFT_CONTROL = 0x0024
    RADIO_CLK_ENABLES   = 0x0028
    MGT_REF_CLK_STATUS  = 0x0030

    def __init__(self, regs, log):
        self.log = log
        self.regs = regs
        self.poke32 = self.regs.poke32
        self.peek32 = self.regs.peek32

    def enable_outputs(self, enable=True):
        """
        Enables or disables the MMCM outputs.
        """
        with self.regs.open():
            if enable:
                self.poke32(self.RADIO_CLK_ENABLES, 0x011)
            else:
                self.poke32(self.RADIO_CLK_ENABLES, 0x000)

    def reset_mmcm(self):
        """
        Uninitialize and reset the MMCM
        """
        self.log.trace("Disabling all Radio Clocks, then resetting MMCM...")
        self.enable_outputs(False)
        with self.regs.open():
            self.poke32(self.RADIO_CLK_MMCM, 0x1)

    def enable_mmcm(self):
        """
        Unreset MMCM and poll lock indicators

        If MMCM is not locked after unreset, an exception is thrown.
        """
        self.log.trace("Un-resetting MMCM...")
        with self.regs.open():
            self.poke32(self.RADIO_CLK_MMCM, 0x2)
            time.sleep(0.5) # Replace with poll and timeout TODO
            mmcm_locked = bool(self.peek32(self.RADIO_CLK_MMCM) & 0x10)
            if not mmcm_locked:
                self.log.error("MMCM not locked!")
                raise RuntimeError("MMCM not locked!")
            self.log.trace("Enabling output MMCM clocks...")
            self.enable_outputs(True)

    def check_refclk(self):
        """
        Not technically a clocking reg, but related.
        """
        with self.regs.open():
            return bool(self.peek32(self.MGT_REF_CLK_STATUS) & 0x1)



class JesdCoreEiscat(object):
    """
    Wrapper for the JESD core. Note this core is specifically adapted for
    EISCAT, it is not general-purpose.
    """
    # JESD Core Register Address Space Setup
    ADDR_BASE     = 0x2000
    CORE_B_OFFSET = 0x1000

    # JESD Core Register Offsets
    JESD_SIGNATURE_REG = 0x100
    JESD_REVISION_REG  = 0x104

    # Expected value for the JESD Core Signature
    CORE_ID_BASE = 0x4A455344

    def __init__(self, regs, slot_idx, core_idx, log):
        self.log = log
        self.regs = regs
        self.slot = "A" if slot_idx == 0 else "B"
        assert core_idx in (0, 1)
        self.core_idx = core_idx
        self.base_addr = self.ADDR_BASE + self.CORE_B_OFFSET * self.core_idx
        self.log.trace("Slot: {} JESD Core {}: Base address {:x}".format(
            self.slot, self.core_idx, self.base_addr
        ))
        self.peek32 = lambda addr: self.regs.peek32(self.base_addr + addr)
        self.poke32 = lambda addr, data: self.regs.poke32(self.base_addr + addr, data)
        if not self.check_core_id():
            raise RuntimeError("Could not identify JESD core!")

    def check_core_id(self):
        """
        Verify that the JESD core ID is correct.
        """
        expected_id = self.CORE_ID_BASE + self.core_idx
        with self.regs.open():
            core_id = self.peek32(self.JESD_SIGNATURE_REG)
            self.log.trace("Reading JESD core ID: {:x}".format(core_id))
            if core_id != expected_id:
                self.log.error(
                    "Cannot identify JESD core! Read ID: {:x} Expected: {:x}".format(
                        core_id, expected_id
                    )
                )
                return False
            date_info = core_id = self.peek32(self.JESD_REVISION_REG)
            self.log.trace("Reading JESD date info: {:x}".format(date_info))
        return True

    def init(self):
        """
        Run initialization sequence on JESD core.

        Returns None, but will throw if there's a problem.
        """
        self.log.trace("Init JESD Core...")
        self._gt_pll_power_control()
        self._gt_rx_reset(True)
        if not self._gt_pll_lock_control():
            raise RuntimeError("JESD CORE {} PLLs not locked!".format(self.core_idx))
        self._gt_polarity_control()

    def init_deframer(self):
        """
        Init FPGA JESD204B Deframer (RX)

        Returns nothing, but throws on error.
        """
        self.log.trace("Init JESD Deframer...")
        with self.regs.open():
            self.poke32(0x40, 0x02) # Force assertion of ADC SYNC
            self.poke32(0x50, 0x01) # Data = 0 = Scrambler enabled. Data = 1 = disabled. Must match ADC settings.
            if not self._gt_rx_reset(reset_only=False):
                raise RuntimeError("JESD Core did not come out of reset properly!")
            self.poke32(0x40, 0x00) # Stop forcing assertion of ADC SYNC

    def check_deframer_status(self):
        """
        Check deframer status (who would have thought)

        Returns True if deframer status is good.
        """
        deframer_status = self.peek32(0x40) & 0xFFFFFFFF
        if deframer_status != 0x3000001C:
            self.log.error("Unexpected JESD Core Deframer Status: {:x}".format(deframer_status))
            return False
        return True

    def _gt_pll_power_control(self):
        """
        Power down unused CPLLs and QPLLs
        """
        with self.regs.open():
            self.poke32(0x00C, 0xFFFC0000)
            self.log.trace("MGT power enabled readback: {:x}".format(self.peek32(0x00C)))

    def _gt_rx_reset(self, reset_only=True):
        """
        RX Reset. Either only puts it into reset, or also pulls it out of reset
        and makes sure lock status is correct.

        Returns True on success.
        """
        with self.regs.open():
            self.poke32(0x024, 0x10) # Place the RX MGTs in reset
            if not reset_only:
                time.sleep(.001) # Probably not necessary
                self.poke32(0x024, 0x20) # Unreset and Enable
                time.sleep(0.1) # TODO replace with poll and timeout 20 ms
                self.log.trace("MGT power enabled readback (rst seq): {:x}".format(self.peek32(0x00C)))
                self.log.trace("MGT CPLL lock readback (rst seq): {:x}".format(self.peek32(0x004)))
                lock_status = self.peek32(0x024)
                if lock_status & 0xFFFF0000 != 0x30000:
                    self.log.error(
                        "JESD Core {}: RX MGTs failed to reset! Status: 0x{:x}".format(self.core_idx, lock_status)
                    )
                    return False
        return True

    def _gt_pll_lock_control(self):
        """
        Make sure PLLs are locked
        """
        with self.regs.open():
            self.poke32(0x004, 0x11111111) # Reset CPLLs
            self.poke32(0x004, 0x11111100) # Unreset the ones we're using
            time.sleep(0.02) # TODO replace with poll and timeout
            self.poke32(0x010, 0x10000) # Clear all CPLL sticky bits
            self.log.trace("MGT CPLL lock readback (lock seq): {:x}".format(self.peek32(0x004)))
            lock_status = self.peek32(0x004) & 0xFF
            lock_good = bool(lock_status == 0x22)
            if not lock_good:
                self.log.error("GT PLL failed to lock! Status: 0x{:x}".format(lock_status))
        return lock_good

    def _gt_polarity_control(self):
        """
        foo
        """
        reg_val = {
            'A': {0: 0x00, 1: 0x11},
            'B': {0: 0x01, 1: 0x10},
        }[self.slot][self.core_idx]
        self.log.trace(
            "JESD Core: Slot {}, ADC {}: Setting polarity control to 0x{:2x}".format(
                self.slot, self.core_idx, reg_val
            ))
        with self.regs.open():
            self.poke32(0x80, reg_val)


class EISCAT(DboardManagerBase):
    """
    EISCAT Daughterboard
    """

    #########################################################################
    # Overridables
    #
    # See DboardManagerBase for documentation on these fields
    #########################################################################
    pids = [0x180]
    spi_chipselect = {
        "lmk": 0,
        "adc0": 1,
        "adc1": 2,
        "phase_dac": 3,
    }
    spi_factories = {
        "lmk": create_spidev_iface_sane,
        "adc0": create_spidev_iface_sane,
        "adc1": create_spidev_iface_sane,
        "phase_dac": create_spidev_iface_phasedac,
    }


    # Daughterboard Control Register address constants
    ADC_CONTROL    = 0x0600
    LMK_STATUS     = 0x0604
    DB_ENABLES     = 0x0608
    DB_CH_ENABLES  = 0x060C
    SYSREF_CONTROL = 0x0620

    INIT_PHASE_DAC_WORD = 500 # Intentionally decimal

    def __init__(self, slot_idx, **kwargs):
        super(EISCAT, self).__init__(slot_idx, **kwargs)
        self.log = get_logger("EISCAT-{}".format(slot_idx))
        self.log.trace("Initializing EISCAT daughterboard, slot index {}".format(self.slot_idx))
        self.initialized = False
        self.ref_clock_freq = 10e6 # This is the only supported clock rate
        # Define some attributes so that PyLint stays quiet:
        self.radio_regs = None
        self.jesd_cores = None
        self.lmk = None
        self.adcs = []
        self.dboard_clk_control = None
        self.clock_synchronizer = None
        self._spi_ifaces = None

    def is_initialized(self):
        """
        Returns True if the daughterboard is a usable state and ready to stream
        """
        return self.initialized

    def init(self, args):
        """
        Execute necessary actions to bring up the daughterboard:
        - Initializes all the software controls for all the chips and registers
        - Turns on the power
        - Initializes clocking
        - Synchronizes clocks to reference
        - Forwards PPS to the radio block

        This assumes that an appropriate overlay was loaded. If not, this will
        fail loudly complaining about missing devices.

        For operation (streaming), the ADCs and deframers still need to be
        initialized. See init_jesd_core_reset_adcs(), init_adcs_and_deframers(),
        and check_deframer_status().

        Note that this function will do nothing if the device was previously
        initialized, unless force_init was specified in the init args.
        """
        def _init_dboard_regs():
            " Create a UIO object to talk to dboard regs "
            self.log.trace("Getting uio...")
            return UIO(
                label="dboard-regs-{}".format(self.slot_idx),
                read_only=False
            )
        def _init_jesd_cores(dboard_regs, slot_idx):
            " Init abstraction layer for JESD cores. Will also test registers. "
            return [
                JesdCoreEiscat(
                    dboard_regs,
                    slot_idx,
                    core_idx,
                    self.log
                ) for core_idx in range(2)
            ]
        def _init_spi_devices():
            " Returns abstraction layers to all the SPI devices "
            self.log.trace("Loading SPI interfaces...")
            return {
                key: self.spi_factories[key](self._spi_nodes[key])
                for key in self._spi_nodes
            }
        def _init_clock_control(dboard_regs):
            " Create a dboard clock control object and reset it "
            dboard_clk_control = DboardClockControl(dboard_regs, self.log)
            dboard_clk_control.reset_mmcm()
            return dboard_clk_control
        def _init_lmk(slot_idx, lmk_spi, ref_clk_freq,
                      pdac_spi, init_phase_dac_word):
            """
            Sets the phase DAC to initial value, and then brings up the LMK
            according to the selected ref clock frequency.
            Will throw if something fails.
            """
            self.log.trace("Initializing Phase DAC to d{}.".format(
                init_phase_dac_word
            ))
            pdac_spi.poke16(0x3, init_phase_dac_word)
            return LMK04828EISCAT(lmk_spi, ref_clk_freq, slot_idx)
        def _sync_db_clock():
            " Synchronizes the DB clock to the common reference "
            synchronizer = ClockSynchronizer(
                self.dboard_clk_control,
                self.lmk,
                self._spi_ifaces['phase_dac'],
                0, # register offset value.
                104e6, # TODO don't hardcode
                self.ref_clock_freq,
                1.9E-12, # fine phase shift. TODO don't hardcode. This should live in the EEPROM
                self.INIT_PHASE_DAC_WORD,
                0x3,
                3, # External PPS pipeline delay from the PPS captured at the FPGA to TDC input
                self.slot_idx)
            # The radio clock traces on the motherboard are 69 ps longer for Daughterboard B
            # than Daughterboard A. We want both of these clocks to align at the converters
            # on each board, so adjust the target value for DB B. This is an N3xx series
            # peculiarity and will not apply to other motherboards.
            trace_delay_offset = {0:  0.0e-0,
                                  1: 69.0e-12}[self.slot_idx]
            offset = synchronizer.run(
                num_meas=[512, 128],
                target_offset = trace_delay_offset)
            offset_error = abs(offset)
            if offset_error > 100e-12:
                self.log.error("Clock synchronizer measured an offset of {:.1f} ps!".format(
                    offset_error*1e12
                ))
                raise RuntimeError("Clock synchronizer measured an offset of {:.1f} ps!".format(
                    offset_error*1e12
                ))
            else:
                self.log.debug("Residual synchronization error: {:.1f} ps.".format(
                    offset_error*1e12
                ))
            synchronizer = None
            self.log.debug("Clock Synchronization Complete!")
        # Go, go, go!
        if args.get("force_init", False):
            self.log.info("Forcing re-initialization of dboard.")
        self.initialized = args.get("force_init", self.initialized)
        if self.initialized:
            self.log.debug(
                "Dboard was previously initialized; skipping init. " \
                "Specify force_init=1 to force initialization."
            )
            return True
        self.log.debug("init() called with args `{}'".format(
            ",".join(['{}={}'.format(x, args[x]) for x in args])
        ))
        self.radio_regs = _init_dboard_regs()
        self.jesd_cores = _init_jesd_cores(self.radio_regs, self.slot_idx)
        self.log.debug("Radio-register UIO object successfully generated!")
        self._spi_ifaces = _init_spi_devices() # Chips don't have power yet!
        self.log.debug("Loaded SPI interfaces!")
        self._init_power(self.radio_regs) # Now, we can talk to chips via SPI
        self.dboard_clk_control = _init_clock_control(self.radio_regs)
        self.lmk = _init_lmk(
            self.slot_idx,
            self._spi_ifaces['lmk'],
            self.ref_clock_freq,
            self._spi_ifaces['phase_dac'],
            self.INIT_PHASE_DAC_WORD,
        )
        self.adcs = [
            ADS54J56(self._spi_ifaces[spi_iface], self.log)
            for spi_iface in ('adc0', 'adc1')
        ]
        self.dboard_clk_control.enable_mmcm()
        self.log.debug("Clocking Configured Successfully!")
        # Synchronize DB Clocks
        self.clock_synchronizer = ClockSynchronizer(
            self.radio_regs,
            self.dboard_clk_control,
            self.lmk,
            self._spi_ifaces['phase_dac'],
            0, # TODO this might not actually be zero
            104e6, # TODO don't hardcode
            self.ref_clock_freq,
            1.9E-12, # TODO don't hardcode. This should live in the EEPROM
            self.INIT_PHASE_DAC_WORD,
            2.496e9,     # lmk_vco_freq
            0x3,         # spi_addr
            self.slot_idx
        )
        _sync_db_clock(self.clock_synchronizer)
        # Clocks and PPS are now fully active!
        return True

    def send_sysref(self):
        """
        Send a SYSREF from MPM. This is not possible to do in a timed
        fashion though.
        """
        self.log.trace("Sending SYSREF via MPM...")
        with self.radio_regs.open():
            self.radio_regs.poke32(self.SYSREF_CONTROL, 0x0)
            self.radio_regs.poke32(self.SYSREF_CONTROL, 0x1)
            self.radio_regs.poke32(self.SYSREF_CONTROL, 0x0)

    def init_jesd_core_reset_adcs(self):
        """
        - Initializes JESD cores
        - Initializes and resets ADCs
        """
        def _check_jesd_cores(db_clk_control, jesd_cores):
            " Checks clocks are enabled; init the JESD core; throw on failure. "
            if not db_clk_control.check_refclk():
                self.log.error("JESD Cores not getting a MGT RefClk!")
                raise RuntimeError("JESD Cores not getting a MGT RefClk")
            for jesd_core in jesd_cores:
                jesd_core.init()
        if self.initialized:
            self.log.debug(
                "Dboard already initialized; skipping initialization " \
                "of ADCs and JESD cores."
            )
            return True
        _check_jesd_cores(
            self.dboard_clk_control,
            self.jesd_cores
        )
        for adc in self.adcs:
            adc.reset()
        self.log.trace("ADC Reset Sequence Complete!")
        return True

    def init_adcs_and_deframers(self):
        """
        Initialize the ADCs and the JESD deframers. Assumption is that they were
        SYSREF'd before.
        """
        if self.initialized:
            self.log.debug(
                "Dboard already initialized; skipping initialization " \
                "of ADCs and JESD cores."
            )
            return True
        for adc in self.adcs:
            adc.setup()
        self.log.debug("ADC Initialization Complete!")
        for jesd_core in self.jesd_cores:
            jesd_core.init_deframer()
        return True

    def check_deframer_status(self):
        """
        Checks the JESD deframer status. This is done after initialization and
        sending a second SYSREF pulse.

        Calling this function is required to signal a completion of the
        initialization sequence.

        Will throw on failure.
        """
        if self.initialized:
            self.log.debug(
                "Dboard already initialized; assuming JESD deframer status " \
                "is fine."
            )
            return True
        for jesd_idx, jesd_core in enumerate(self.jesd_cores):
            if not jesd_core.check_deframer_status():
                self.log.error("JESD204B Core {} Error: Failed to Link. " \
                    "Don't ignore this, please tell someone!".format(jesd_idx)
                )
                error = True
                self.adcs[jesd_idx].swap_sync_line()
        if error:
            return False

        self.log.debug("JESD Core Initialized, link up! (woohoo!)")
        self.initialized = True
        return self.initialized

    def shutdown(self):
        """
        Safely turn off the daughterboard. This will take away power to the
        components; a re-initialization will be necessary after calling this.
        """
        self.log.info("Shutting down daughterboard")
        self.initialized = False
        self._deinit_power(self.radio_regs)

    def _init_power(self, regs):
        """
        Turn on power to the dboard.

        After this function, we should never touch this group again (other
        than turning it off, maybe).
        """
        # Enable all channels first due to a signal integrity issue when enabling them
        # after the LNA enable is asserted.
        self.log.trace("Enabling power to the daughterboard...")
        with regs.open():
            regs.poke32(self.DB_CH_ENABLES, 0x000000FF)
            regs.poke32(self.DB_ENABLES,    0x01000000)
            regs.poke32(self.DB_ENABLES,    0x00010101)
            regs.poke32(self.ADC_CONTROL,   0x00010000)
        time.sleep(0.100)

    def _deinit_power(self, regs):
        """
        Turn off power to the dboard. Sequence is reverse of init_power.
        """
        self.log.trace("Disabling power to the daughterboard...")
        with regs.open():
            regs.poke32(self.ADC_CONTROL,   0x00100000)
            regs.poke32(self.DB_ENABLES,    0x10101010)
            regs.poke32(self.DB_CH_ENABLES, 0x00000000) # Disable all channels (last)

    def update_ref_clock_freq(self, freq):
        """
        Call this to notify the daughterboard about a change in reference clock
        """
        if freq != self.ref_clock_freq:
            self.log.error(
                "EISCAT daughterboard only supports a reference clock " \
                "frequency of {} MHz".format(self.ref_clock_freq/1e6)
            )
            raise RuntimeError("Invalid reference clock frequency: {}".format(
                freq
            ))



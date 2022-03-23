#!/usr/bin/env python3
#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Update the CPLD image using the on-chip flash on Intel MAX10 devices
"""
import os
import time

class Max10CpldFlashCtrl():
    """
    Context manager class used to handle CPLD Flash reconfiguration.
    Calling "with" using an instance of this class will disable write
    protection for the duration of that context.
    """
    REVISION_REG                = 0x0004
    # Addresses relative to reconfiguration register offset
    FLASH_STATUS_REG            = 0x0000
    FLASH_CONTROL_REG           = 0x0004
    FLASH_ADDR_REG              = 0x0008
    FLASH_WRITE_DATA_REG        = 0x000C
    FLASH_READ_DATA_REG         = 0x0010
    FLASH_CFM0_START_ADDR_REG   = 0x0014
    FLASH_CFM0_END_ADDR_REG     = 0x0018

    # Masks for FLASH_STATUS_REG
    FLASH_MEM_INIT_ENABLED_MASK = 0x10000

    def __init__(self, logger, regs, reconfig_regs_offset, cpld_min_revision):
        if logger == None:
            logger = get_logger('update_cpld')
        self.log = logger
        self.regs = regs
        self.reconfig_regs_offset = reconfig_regs_offset
        self.cpld_min_revision = cpld_min_revision

    def peek32(self, addr):
        return self.regs.peek32(addr + self.reconfig_regs_offset)

    def poke32(self, addr, val):
        self.regs.poke32(addr + self.reconfig_regs_offset, val)

    def __enter__(self):
        self.enabled_write_protection(enable=False)

    def __exit__(self, exc_type, exc_value, traceback):
        self.enabled_write_protection(enable=True)

    def enabled_write_protection(self, enable=True):
        if enable:
            self.poke32(self.FLASH_CONTROL_REG, 1 << 0) # FLASH_ENABLE_WP_STB
        else:
            self.poke32(self.FLASH_CONTROL_REG, 1 << 1) # FLASH_DISABLE_WP_STB

    def check_revision(self):
        self.log.debug('Checking for compatible CPLD revision')
        cpld_revision = self.regs.peek32(self.REVISION_REG)
        if cpld_revision < self.cpld_min_revision:
            self.log.error("Unexpected CPLD revision 0x{:X}".format(cpld_revision))
            return False
        return True

    def get_start_addr(self):
        return self.peek32(self.FLASH_CFM0_START_ADDR_REG)

    def get_end_addr(self):
        return self.peek32(self.FLASH_CFM0_END_ADDR_REG)

    def is_memory_initialization_enabled(self):
        return self.peek32(self.FLASH_STATUS_REG) & self.FLASH_MEM_INIT_ENABLED_MASK

    # expected value of 0x1110 indicates idle state of read, write, and erase
    # routines (see function wait_for_idle for details), 0x0001 indicates the
    # write protection is enabled
    def check_reconfig_engine_status(self, expected_value=0x1111):
        status = self.peek32(self.FLASH_STATUS_REG)
        status = status & ~self.FLASH_MEM_INIT_ENABLED_MASK
        if (status != expected_value):
            self.log.error("Unexpected reconfig engine status 0x%08X" % status)
            return False
        return True

    def wait_for_idle(self, operation, timeout=350):
        """
        Wait for the idle bit to assert for the given operation.
        If the idle bit is not True before the timeout (given in ms),
        return False.
        """
        if operation == 'write':
            status_bit = 1 << 12   # FLASH_WRITE_IDLE
        elif operation == 'erase':
            status_bit = 1 << 8    # FLASH_ERASE_IDLE
        elif operation == 'read':
            status_bit = 1 << 4    # FLASH_READ_IDLE
        else:
            self.log.error('Cannot wait for unknown operation {}'.format(operation))
            raise RuntimeError('Cannot wait for unknown operation {}'.format(operation))
        for _ in range(0, timeout):
            status = self.peek32(self.FLASH_STATUS_REG)
            if (status & status_bit):
                return True
            time.sleep(0.001) # 1 ms
        return False

    def erase_flash_memory(self):
        with self:
            # determine M04 or M08 variant based on
            # value encoded in the FLASH_CFM0_START_ADDR_REG
            # register
            start_addr = self.get_start_addr()
            # 0x9C00 -> X410 MB, 0x1000  -> ZBX DB
            if start_addr in [0x9C00, 0x1000]:
                self.max10_variant = "m04"
            # 0xAC00 -> X410 MB, 0x2000  -> ZBX DB
            elif start_addr in [0xAC00, 0x2000]:
                self.max10_variant = "m08"
            else:
                raise RuntimeError('Unknown MAX10 variant (FLASH_CFM0_START_ADDR_REG=0x{:04X})'.format(start_addr))
            # check for sectors to be erased:
            if self.is_memory_initialization_enabled():
                if self.max10_variant == "m04":
                    sectors = [2, 3, 4]
                else:
                    sectors = [3, 4, 5]
            else:
                if self.max10_variant == "m04":
                    sectors = [4]
                else:
                    sectors = [5]
            # erase each sector individually
            for sector in sectors:
                # start erase
                self.poke32(self.FLASH_CONTROL_REG, (1 << 4) | ((sector & 0x7) << 5))
                # wait for erase to finish
                if not self.wait_for_idle('erase', timeout=350):
                    self.log.error('There was a timeout waiting for '
                                   'Flash erase to complete!')
                    return False
            return True

    def program_flash_memory(self, raw_data):
        with self:
            # write words one at a time
            for i, data in enumerate(raw_data):
                # status display
                if (i%1000 == 0):
                    self.log.debug('%d%% written', i*4/self.file_size*100)
                # write address and data
                self.poke32(self.FLASH_ADDR_REG, self.cpld_start_address+i)
                self.poke32(self.FLASH_WRITE_DATA_REG, data)
                # start write operation
                self.poke32(self.FLASH_CONTROL_REG, 1 << 3)
                # wait for write to finish
                if not self.wait_for_idle('write', timeout=2):
                    self.log.error('There was a timeout waiting for '
                                   'Flash write to complete!')
                    return False
                if not self.check_reconfig_engine_status(expected_value=0x1110):
                    return False
        return True

    def verify_flash_memory(self, raw_data):
        # read words one at a time
        for i, data in enumerate(raw_data):
            # write address
            self.poke32(self.FLASH_ADDR_REG, self.cpld_start_address+i)
            # start read operation
            self.poke32(self.FLASH_CONTROL_REG, 1 << 2)
            # wait for read to finish
            if not self.wait_for_idle('read', timeout=1):
                self.log.error('There was a timeout waiting for '
                               'Flash read to complete!')
                return False
            # read data from device
            device_data = self.peek32(self.FLASH_READ_DATA_REG)
            if (data != device_data):
                self.log.debug("CPLD image mismatch! address %d, expected value 0x%08X,"
                               " read value 0x%08X" %
                               (i+self.cpld_start_address, data, device_data))
                return False
            # status display
            if (i%1000 == 0):
                self.log.debug('%d%% verified', i*4/self.file_size*100)
        return True

    def reverse_bits_in_byte(self, n):
        result = 0
        for _ in range(8):
            result <<= 1
            result |= n & 1
            n >>= 1
        return result

    def update(self, filename):
        if not self.check_revision():
            return False

        self.log.debug('Checking CPLD image file')
        self.file_size = os.path.getsize(filename)
        self.cpld_start_address = self.get_start_addr()
        cpld_end_address = self.get_end_addr()
        expected_size = (cpld_end_address+1-self.cpld_start_address)*4
        if (self.file_size != expected_size):
            self.log.error("Unexpected file size! Required size: %d bytes" % expected_size)
            return False

        # Convert data from bytes to 32-bit words and reverse bit order
        # to be compatible with Altera's on-chip flash IP
        raw_data = []
        with open(filename, 'rb') as binary_file:
            for _ in range(self.file_size//4):
                number = 0
                for _ in range(4):
                    number <<= 8
                    number |= self.reverse_bits_in_byte(int.from_bytes(binary_file.read(1), 'big'))
                raw_data.append(number)

        if not self.check_reconfig_engine_status():
            return False

        self.log.debug('Checking if update is necessary...')
        if self.verify_flash_memory(raw_data):
            self.log.info('CPLD already programmed with specified image, not reprogramming.')
            return True

        if not self.check_reconfig_engine_status():
            return False

        self.log.debug('Erasing CPLD flash memory...')
        if not (self.erase_flash_memory()
                and self.check_reconfig_engine_status()):
            self.log.error('There was an error while reprogramming the CPLD image. '
                           'Please program the CPLD again with a valid image before power '
                           'cycling the board to ensure it is in a valid state.')
            return False
        self.log.debug('CPLD flash memory erased.')

        self.log.debug('Programming flash memory...')
        if not (self.program_flash_memory(raw_data)
                and self.check_reconfig_engine_status()):
            self.log.error('There was an error while reprogramming the CPLD image. '
                           'Please program the CPLD again with a valid image before power '
                           'cycling the board to ensure it is in a valid state.')
            return False
        self.log.debug('Flash memory programming complete.')

        self.log.debug('Verifying image in flash...')
        if not (self.verify_flash_memory(raw_data)
                and self.check_reconfig_engine_status()):
            self.log.error('There was an error while reprogramming the CPLD image. '
                           'Please program the CPLD again with a valid image before power '
                           'cycling the board to ensure it is in a valid state.')
            return False
        self.log.debug('Flash image verification complete.')

        self.log.info('CPLD reprogrammed! Please power-cycle the device.')

        return True

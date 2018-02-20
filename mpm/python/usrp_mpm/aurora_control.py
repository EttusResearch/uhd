#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Aurora SFP control
"""

import math
import time
from builtins import str
from builtins import object
from usrp_mpm.mpmlog import get_logger

def mean(vals):
    " Calculate arithmetic mean of vals "
    return float(sum(vals)) / max(len(vals), 1)

def stddev(vals, mu=None):
    " Calculate std deviation of vals "
    mu = mu or mean(vals)
    return float(sum(((x - mu)**2 for x in vals))) / max(len(vals), 1)

class AuroraControl(object):
    """
    Controls an Aurora core.
    """
    # These are relative addresses
    REG_AURORA_PORT_INFO       = 0x00
    REG_AURORA_MAC_CTRL_STATUS = 0x04
    REG_AURORA_PHY_CTRL_STATUS = 0x08
    REG_AURORA_OVERRUNS        = 0x20
    REG_CHECKSUM_ERRORS        = 0x24
    REG_BIST_CHECKER_SAMPS     = 0x28
    REG_BIST_CHECKER_ERRORS    = 0x2C

    MAC_STATUS_LINK_UP_MSK         = 0x00000001
    MAC_STATUS_HARD_ERR_MSK        = 0x00000002
    MAC_STATUS_SOFT_ERR_MSK        = 0x00000004
    MAC_STATUS_BIST_LOCKED_MSK     = 0x00000008
    MAC_STATUS_BIST_LATENCY_MSK    = 0x000FFFF0
    MAC_STATUS_BIST_LATENCY_OFFSET = 4

    RATE_RES_BITS = 6

    DEFAULT_BUS_CLK_RATE = 200e6

    def __init__(self, peeker_poker32, base_addr=None, bus_clk_rate=None):
        assert hasattr(peeker_poker32, 'peek32') \
                and callable(peeker_poker32.peek32)
        assert hasattr(peeker_poker32, 'poke32') \
                and callable(peeker_poker32.poke32)
        self.log = get_logger("AuroraCore")
        self._regs = peeker_poker32
        base_addr = base_addr or 0
        self.log.debug("Base address in register space is: 0x{:04X}".format(
            base_addr
        ))
        self.poke32 = lambda addr, data: self._regs.poke32(
            addr + base_addr, data
        )
        self.peek32 = lambda addr: self._regs.peek32(addr + base_addr)
        self.mac_ctrl = 0x000
        self.set_mac_ctrl(self.mac_ctrl)
        time.sleep(.5)
        self.bus_clk_rate = bus_clk_rate
        if self.bus_clk_rate is None:
            self.bus_clk_rate = self.DEFAULT_BUS_CLK_RATE
            self.log.warning("Unspecified bus clock rate. Assuming default "
                             "rate of {} MHz.".format(self.bus_clk_rate/1e6))
        else:
            self.log.debug("Bus clock rate: {} MHz.".format(
                self.bus_clk_rate/1e6
            ))
        self.bist_max_time_limit = math.floor(2**48/self.bus_clk_rate)-1
        self.log.debug("BIST max time limit: {} s".format(
            self.bist_max_time_limit
        ))
        self.log.debug("Status of PHY link: 0x{:08X}".format(
            self.read_phy_ctrl_status()
        ))
        if not self.is_phy_link_up():
            raise RuntimeError("PHY link not up. Check connectors.")

    def read_mac_ctrl_status(self):
        " Return MAC ctrl status word from core "
        return self.peek32(self.REG_AURORA_MAC_CTRL_STATUS)

    def read_phy_ctrl_status(self):
        " Return PHY ctrl status word from core "
        return self.peek32(self.REG_AURORA_PHY_CTRL_STATUS)

    def read_overruns(self):
        " Return overrun count from core "
        return self.peek32(self.REG_AURORA_OVERRUNS)

    def read_checksum_errors(self):
        " Return checksum error count from core "
        return self.peek32(self.REG_CHECKSUM_ERRORS)

    def read_bist_checker_samps(self):
        " Return number of samps processed from core "
        return self.peek32(self.REG_BIST_CHECKER_SAMPS)

    def read_bist_checker_errors(self):
        " Return number of errors from core "
        return self.peek32(self.REG_BIST_CHECKER_ERRORS)

    def set_mac_ctrl(self, mac_ctrl_word):
        " Write to the MAC ctrl register "
        self.log.debug("Setting MAC ctrl word to: 0x{:08X}".format(mac_ctrl_word))
        self.poke32(self.REG_AURORA_MAC_CTRL_STATUS, mac_ctrl_word)

    def set_bist_checker_and_gen(self, enable):
        " Enable or disable Aurora BIST: Checker + Generator "
        if enable:
            self.log.info("Enable Aurora BIST Checker and Gen")
            self.mac_ctrl = self.mac_ctrl | 0b11
        else:
            self.log.info("Disable Aurora BIST Checker and Gen")
            self.mac_ctrl = self.mac_ctrl & 0xFFFFFFFC
        self.set_mac_ctrl(self.mac_ctrl)

    def set_bist_checker(self, enable):
        " Enable or disable Aurora BIST: Checker only "
        if enable:
            self.log.info("Enable Aurora BIST Checker")
            self.mac_ctrl = self.mac_ctrl | 0b01
        else:
            self.log.info("Disable Aurora BIST Checker")
            self.mac_ctrl = self.mac_ctrl & 0xFFFFFFFE
        self.set_mac_ctrl(self.mac_ctrl)

    def set_bist_gen(self, enable):
        " Enable or disable Aurora BIST: Generator only "
        if enable:
            self.log.info("Enable Aurora BIST Gen")
            self.mac_ctrl = self.mac_ctrl | 0b10
        else:
            self.log.info("Disable Aurora BIST Gen")
            self.mac_ctrl = self.mac_ctrl & 0xFFFFFFFD
        self.set_mac_ctrl(self.mac_ctrl)

    def set_loopback(self, enable):
        " Enable or disable Aurora loopback mode "
        if enable:
            self.log.info("Enable Aurora loopback")
            self.mac_ctrl = self.mac_ctrl | 0b100
        else:
            self.log.info("Disable Aurora loopback")
            self.mac_ctrl = self.mac_ctrl & 0xFFFFFFFB
        self.set_mac_ctrl(self.mac_ctrl)

    def set_bist_rate(self, rate_word):
        " Set BIST rate. It's a 6-bit value in the MAC ctrl register. "
        self.log.debug("Setting Aurora BIST rate word to 0x{:02X}".format(
            rate_word
        ))
        self.mac_ctrl = self.mac_ctrl | ((rate_word & 0x3F) << 3)
        self.set_mac_ctrl(self.mac_ctrl)

    def reset_phy(self):
        " Reset Aurora PHY "
        self.log.debug("Reset Aurora PHY")
        self.mac_ctrl = self.mac_ctrl | (1<<9)
        self.set_mac_ctrl(self.mac_ctrl)
        self.clear_control_reg()

    def clear_mac(self):
        " Clear Aurora MAC "
        self.log.debug("Clear Aurora MAC")
        self.mac_ctrl = self.mac_ctrl | (1<<10)
        self.set_mac_ctrl(self.mac_ctrl)
        self.clear_control_reg()

    def clear_control_reg(self):
        " Zero out Aurora control register "
        self.mac_ctrl = 0
        self.set_mac_ctrl(self.mac_ctrl)

    def get_rate_setting(self, requested_rate, bus_clk_rate):
        """
        From a requested bit rate, return the value for the rate register, and
        the coerced value.
        """
        max_rate_word = 2**self.RATE_RES_BITS - 1
        lines_per_clock = float(requested_rate) / 64 / bus_clk_rate
        rate_word = int(lines_per_clock * 2**self.RATE_RES_BITS) - 1
        rate_word = min(max(rate_word, 0), max_rate_word)
        coerced_rate = ((1+rate_word) / 2**self.RATE_RES_BITS) \
                       * 64 * bus_clk_rate
        return rate_word, coerced_rate

    def is_phy_link_up(self):
        """
        Return True if the PHY link was successfully negotiated.
        """
        return bool(self.read_phy_ctrl_status() & 0x1)

    def reset_core(self):
        " Reset MAC. PHY reset not necessary"
        self.clear_control_reg()
        self.clear_mac()

    def run_latency_loopback_bist(
            self,
            duration,
            requested_rate,
            slave=None,
        ):
        """
        Run latency loopback BIST

        slave -- the other sfp core gets set to loopback mode
        ctrl -- sorta the master sfp core
        duration -- time we want to run the bist
        requested_rate -- Requested BIST rate in bits/s
        """
        rate_word, coerced_rate = \
                self.get_rate_setting(requested_rate, self.bus_clk_rate)
        self.log.info(
            'Running Latency Loopback BIST at %.0fMB/s for %.0fs...',
            coerced_rate/8e6, duration
        )
        self._pre_test_init(slave)
        start_time = time.time()
        results = {
            'latencies': [],
            'mst_lock_errors': 0,
            'mst_hard_errors': 0,
            'mst_overruns': 0,
        }
        try:
            for _ in range(duration*10):
                self.set_bist_rate(rate_word)
                self.set_bist_checker_and_gen(enable=True)
                # Wait and check if BIST locked
                time.sleep(0.05)
                mst_status = self.read_mac_ctrl_status()
                if not mst_status & self.MAC_STATUS_BIST_LOCKED_MSK:
                    results['mst_lock_errors'] += 1
                self.log.info('lock errors: %d', results['mst_lock_errors'])
                # Turn off the BIST generator
                self.set_bist_gen(0)
                # Validate status and no overruns
                mst_status = self.read_mac_ctrl_status()
                results['mst_overruns'] = self.read_overruns()
                if mst_status & self.MAC_STATUS_HARD_ERR_MSK:
                    results['mst_hard_errors'] += 1
                time.sleep(0.05)
                self.clear_control_reg()
                # Compute latency
                results['latencies'].append(
                    self._mst_status_to_latency_us(mst_status)
                )
        except KeyboardInterrupt:
            self.log.warning('Operation cancelled by user.')
        stop_time = time.time()
        # Report
        if results['mst_lock_errors'] > 0:
            self.log.error(
                'BIST engine did not lock onto a PRBS word %d times!',
                results['mst_lock_errors']
            )
        if results['mst_hard_errors'] > 0:
            self.log.error(
                'There were %d hard errors in master PHY',
                results['mst_hard_errors']
            )
        if results['mst_overruns'] > 0:
            self.log.error(
                'There were %d buffer overruns in master PHY',
                results['mst_overruns']
            )
        mu_lat = mean(results['latencies'])
        results['elapsed_time'] = stop_time - start_time
        self.log.info('BIST Complete!')
        self.log.info('- Elapsed Time               = ' + str(results['elapsed_time']))
        self.log.info('- Roundtrip Latency Mean     = %.2fus', mu_lat)
        self.log.info('- Roundtrip Latency Stdev    = %.6fus',
                      stddev(results['latencies'], mu=mu_lat))
        # Turn off BIST loopback
        time.sleep(0.5)
        if slave is not None:
            results['sla_overruns'], results['sla_hard_errors'] = \
                    self._get_slave_status(slave)
        self._post_test_cleanup(slave)
        return results

    def run_ber_loopback_bist(self, duration, requested_rate, slave=None):
        """
        Run BER Bist. Pump lots of bits through, and see how many come back
        correctly.

        duration -- Time to run the test in seconds
        """
        rate_word, coerced_rate = \
                self.get_rate_setting(requested_rate, self.bus_clk_rate)
        self.log.info('Running BER Loopback BIST at {}MB/s for {}s...'.format(
            coerced_rate/8e6, duration
        ))
        self._pre_test_init(slave)
        mst_overruns = 0
        self.log.info("Starting BER test...")
        start_time = time.time()
        self.set_bist_rate(rate_word)
        self.set_bist_checker_and_gen(enable=True)
        # Wait and check if BIST locked
        time.sleep(0.5)
        mst_status = self.read_mac_ctrl_status()
        if not mst_status & self.MAC_STATUS_BIST_LOCKED_MSK:
            error_msg = 'BIST engine did not lock onto a PRBS word! ' \
                        'MAC status word: 0x{:08X}'.format(mst_status)
            self.log.error(error_msg)
            raise RuntimeError(error_msg)
        # Wait for requested time
        try:
            time.sleep(duration)
        except KeyboardInterrupt:
            self.log.warning('Operation cancelled by user.')
        # Turn off the BIST generator and loopback
        self.set_bist_gen(enable=False)
        results = {}
        results['time_elapsed'] = time.time() - start_time
        time.sleep(0.5)
        # Validate status and no overruns
        mst_status = self.read_mac_ctrl_status()
        results['mst_overruns'] = self.read_overruns()
        results['mst_samps'] = 65536 * self.read_bist_checker_samps()
        results['mst_errors'] = self.read_bist_checker_errors()
        if mst_status & self.MAC_STATUS_HARD_ERR_MSK:
            self.log.error('Hard errors in master PHY')
            results['mst_hard_errors'] = True
        if mst_overruns > 0:
            self.log.error('Buffer overruns in master PHY')
        if slave is not None:
            results['sla_overruns'], results['sla_hard_errors'] = \
                    self._get_slave_status(slave)
        if results['mst_samps'] != 0:
            results['mst_latency_us'] = \
                    self._mst_status_to_latency_us(mst_status)
            self.log.info('BIST Complete!')
            self.log.info('- Elapsed Time              = {:.2} s'.format(
                results['time_elapsed']
            ))
            results['max_ber'] = \
                    float(results['mst_errors']+1) / results['mst_samps']
            results['approx_throughput'] = \
                (8 * results['mst_samps']) / results['time_elapsed']
            self.log.info('- Max BER (Bit Error Ratio) = %.4g ' \
                          '(%d errors out of %d)',
                          results['max_ber'],
                          results['mst_errors'],
                          results['mst_samps'])
            self.log.info('- Max Roundtrip Latency     = %.1fus',
                          results['mst_latency_us'])
            self.log.info('- Approx Throughput         = %.0fMB/s',
                          results['approx_throughput'] / 1e6)
        else:
            self.log.error('No samples received -- BIST Failed!')
        self._post_test_cleanup(slave)
        return results

    def _get_slave_status(self, slave):
        """
        Read back status from the slave
        """
        slave.clear_control_reg()
        sla_status = slave.read_mac_ctrl_status()
        sla_overruns = slave.read_overruns()
        sla_hard_errors = 0
        if sla_status & self.MAC_STATUS_HARD_ERR_MSK:
            self.log.error('Hard errors in slave PHY')
            sla_hard_errors = slave.read_overruns()
        if sla_overruns > 0:
            self.log.error('Buffer overruns in slave PHY')
        return sla_overruns, sla_hard_errors

    def _mst_status_to_latency_us(self, mst_status):
        " Convert a MAC status word into latency in microseconds "
        latency_cyc = 16.0 * \
                ((mst_status & self.MAC_STATUS_BIST_LATENCY_MSK) \
                    >> self.MAC_STATUS_BIST_LATENCY_OFFSET)
        return 1e6 * latency_cyc / self.bus_clk_rate

    def _pre_test_init(self, slave=None):
        " Set up core(s) for BISTing "
        self.reset_core()
        if slave is not None:
            slave.reset_core()
        time.sleep(1.5)
        if slave is not None:
            self.set_loopback(enable=False)
            slave.set_loopback(enable=True)
        self.log.debug("Status of PHY link: 0x{:08X}".format(
            self.read_phy_ctrl_status()
        ))
        if not self.is_phy_link_up():
            raise RuntimeError("PHY link not up. Check connectors.")

    def _post_test_cleanup(self, slave=None):
        " Drain and Cleanup "
        self.log.info('Cleaning up...')
        self.set_bist_checker(enable=True)
        if slave is not None:
            slave.set_bist_checker(enable=True)
        time.sleep(0.5)
        self.clear_control_reg()
        if slave is not None:
            slave.clear_control_reg()


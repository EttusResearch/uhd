//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/device_addr.hpp>
#include <uhdlib/utils/config_parser.hpp>
#include <string>

namespace uhd { namespace prefs {

/*! Return a reference to an object representing the UHD config file
 *  state.
 *
 * Note: Don't call this in static initializers.
 */
config_parser& get_uhd_config();

/*! Convenience function to update device args with settings from
 *  config files.
 *
 * Assume the user has a configuration file as such:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.ini}
 * [type=b200]
 * master_clock_rate=20e6
 *
 * [serial=f42f9b] ; Let's assume this is another B200
 * master_clock_rate=10e6
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * If get_usrp_args() gets called with "type" key equal to "b200", it will
 * first apply the `master_clock_rate=20e6` settings, as if they had been
 * passed in as device args into the initialization sequence. If the device
 * happens to have the serial number listed above, i.e., "serial" equals
 * "f42f9b", then the new value `master_clock_rate=10e6` will get applied.
 *
 * If the user actually specified their own value of `master_clock_rate`,
 * that value would get applied.
 *
 *
 * \param user_args After getting the device args from the config
 *                  files, all of these key/value pairs will be applied
 *                  and will overwrite the settings from config files
 *                  if they exist.
 */
uhd::device_addr_t get_usrp_args(const uhd::device_addr_t& user_args);

/*! Convenience function to update global DPDK args with settings from
 * config files.
 *
 * Searches for a profile attached to the dpdk-conf key, like this:
 * [dpdk-conf=myconfig]
 * num_mbufs=4095
 * mbuf_cache_size=315
 * mtu=8000
 *
 * \param user_args After getting the device args from the config
 *                  files, all of these key/value pairs will be applied
 *                  and will overwrite the settings from config files
 *                  if they exist.
 */
uhd::device_addr_t get_dpdk_args(const uhd::device_addr_t& user_args);

/*! Convenience function to update per-NIC DPDK args with settings from
 * config files.
 *
 * Grabs settings based on provided MAC address. Sections created like so:
 * [dpdk-mac=00:01:02:03:04:05]
 * dpdk-ipv4 = 192.168.20.1/24
 * dpdk-io-cpu = 1
 *
 * [dpdk-mac=00:01:02:03:04:06]
 * dpdk-ipv4 = 192.168.40.1/24
 * dpdk-io-cpu = 1
 *
 * \param user_args After getting the device args from the config
 *                  files, all of these key/value pairs will be applied
 *                  and will overwrite the settings from config files
 *                  if they exist.
 */
uhd::device_addr_t get_dpdk_nic_args(const uhd::device_addr_t& user_args);
}} /* namespace uhd::prefs */

//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/device_addr.hpp>
#include <map>

namespace uhd { namespace usrp {

/*! Struct containing user options for I/O services
 *
 * The I/O service manager supports the following args:
 *
 * recv_offload: set to "true" to use an offload thread for RX_DATA links, "false"
 *               to use an inline I/O service.
 * send_offload: set to "true" to use an offload thread for TX_DATA links, "false"
 *               to use an inline I/O service.
 * recv_offload_wait_mode: set to "poll" to use a polling strategy in the offload
 *                         thread, set to "block" to use a blocking strategy.
 * send_offload_wait_mode: set to "poll" to use a polling strategy in the offload
 *                         thread, set to "block" to use a blocking strategy.
 * num_poll_offload_threads: set to the total number of offload threads to use for
 *                           RX_DATA and TX_DATA in this rfnoc_graph. New connections
 *                           always go to the offload thread containing the fewest
 *                           connections, with lowest numbered thread as a second
 *                           criterion. The default is 1.
 * recv_offload_thread_<N>_cpu: an integer to specify cpu affinity of the offload
 *                              thread. N indicates the thread instance, starting
 *                              with 0 for each streamer and ending with the number
 *                              of transport adapters minus one. Only used if the
 *                              I/O service is configured to block.
 * send_offload_thread_<N>_cpu: an integer to specify cpu affinity of the offload
 *                              thread. N indicates the thread instance, starting
 *                              with 0 for each streamer and ending with the number
 *                              of transport adapters minus one. Only used if the
 *                              I/O service is configured to block.
 * poll_offload_thread_<N>_cpu: an integer to specify cpu affinity of the offload
 *                              thread. N indicates the thread instance, starting
 *                              with 0 and up to num_poll_offload_threads minus 1.
 *                              Only used if the I/O service is configured to poll.
 */
struct io_service_args_t
{
    enum wait_mode_t { POLL, BLOCK };

    //! Whether to offload streaming I/O to a worker thread
    bool recv_offload = false;

    //! Whether to offload streaming I/O to a worker thread
    bool send_offload = false;

    //! Whether the offload thread should poll or block
    wait_mode_t recv_offload_wait_mode = BLOCK;

    //! Whether the offload thread should poll or block
    wait_mode_t send_offload_wait_mode = BLOCK;

    //! Number of polling threads to use, if wait_mode is set to POLL
    size_t num_poll_offload_threads = 1;

    //! CPU affinity of offload threads, if wait_mode is set to BLOCK
    std::map<size_t, size_t> recv_offload_thread_cpu;

    //! CPU affinity of offload threads, if wait_mode is set to BLOCK
    std::map<size_t, size_t> send_offload_thread_cpu;

    //! CPU affinity of offload threads, if wait_mode is set to POLL
    std::map<size_t, size_t> poll_offload_thread_cpu;
};

/*! Reads I/O service args from provided dictionary
 *
 * If an option is not specified in the dictionary, the default value of the
 * struct above is returned.
 *
 * \param args The dictionary from which to read the I/O service args
 * \param defaults Default values (not including boost::optional values)
 * \return The I/O service args read
 */
io_service_args_t read_io_service_args(
    const device_addr_t& args, const io_service_args_t& defaults);

/*! Merges device_args with stream_args
 *
 * Copies args related to I/O services from device args to stream args, and
 * returns the merged result. If the same arg is specified in device_args and
 * stream args, the value in stream_args is returned.
 *
 * \param args The device args provided when the graph is created
 * \param args The stream args provided when a streamer is created
 * \return The merged device args
 */
device_addr_t merge_io_service_dev_args(
    const device_addr_t& dev_args, const device_addr_t& stream_args);

}} // namespace uhd::usrp

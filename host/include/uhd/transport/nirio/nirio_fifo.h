//
// Copyright 2013-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


#ifndef INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_FIFO_H
#define INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_FIFO_H

#include <uhd/transport/nirio/nirio_driver_iface.h>
#include <uhd/transport/nirio/niriok_proxy.h>
#include <uhd/transport/nirio/status.h>
#include <boost/noncopyable.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/atomic/atomic.hpp>
#include <string>
#include <chrono>
#include <thread>
#include <stdint.h>

namespace uhd { namespace niusrprio {

struct datatype_info_t {
    datatype_info_t(nirio_scalar_type_t t, uint32_t w):scalar_type(t),width(w) {}
    nirio_scalar_type_t     scalar_type;
    uint32_t        width;
};

template <typename data_t>
class nirio_fifo : private boost::noncopyable
{
public:
    typedef boost::shared_ptr< nirio_fifo<data_t> > sptr;

    typedef enum {
        MINIMIZE_LATENCY,
        MAXIMIZE_THROUGHPUT
    } fifo_optimization_option_t;

    /*!
    * Initializing constructor
    * \param riok_proxy Proxy to the kernel driver
    * \param direction Direction of the fifo (INPUT_FIFO [from device] or OUTPUT_FIFO [to device])
    * \param name Name of the fifo
    * \param fifo_instance Instance of the fifo as defined by RIO FPGA target
    */
    nirio_fifo(
        niriok_proxy::sptr riok_proxy,
        const fifo_direction_t direction,
        const std::string& name,
        const uint32_t fifo_instance);
    virtual ~nirio_fifo();

    /*!
    * Configures the characterists of this DMA FIFO, allocates DMA buffer and maps it to user mode
    * \param requested_depth Desired size of DMA FIFO buffer on host (in elements)
    * \param frame_size_in_elements Size of a single frame (smallest transfer block) for this FIFO
    * \param actual_depth Receives the size (in elements) of the allocated DMA buffer
    * \param actual_size Receives the size (in bytes) of the allocated DMA buffer
    * \param fifo_optimization_option FIFO acquire policy (MINIMIZE_LATENCY [fetch what was there last time] or MAXIMIZE_THROUGHPUT [fetch all available])
    * \return status
    */
    nirio_status initialize(
        const size_t requested_depth,
        const size_t frame_size_in_elements,
        size_t& actual_depth,
        size_t& actual_size, 
        const fifo_optimization_option_t fifo_optimization_option = MINIMIZE_LATENCY);

    /*!
    * Stops FIFO if started, releases any acquired blocks, and unmaps the DMA buffer
    */
    void finalize();

    /*!
    * Name accessor
    * \return FIFO name
    */
    inline const std::string& get_name() const { return _name; }

    /*!
    * Channel/Instance accessor
    * \return FIFO channel (instance)
    */
    inline uint32_t get_channel() const { return _fifo_channel; }

    /*!
    * Direction accessor
    * \return FIFO direction
    */
    inline fifo_direction_t get_direction() const { return _fifo_direction; }

    /*!
    * Type accessor
    * \return FIFO element type
    */
    inline nirio_scalar_type_t get_scalar_type() const { return _datatype_info.scalar_type; }

    /*!
    * Starts the DMA transfer between host and device, pre-acquires any available blocks
    * \return status
    */
    nirio_status start();

    /*!
    * Stops the DMA transfer between host and device, releases any acquired blocks
    * \return status
    */
    nirio_status stop();

    /*!
    * Acquires space in the DMA buffer so it can be written by the host (output) or read by the host (input)
    * \param elements Receives the address of the acquired block (pointer to mapped zero-copy DMA buffer)
    * \param elements_requested Size (in elements) of the block to acquire
    * \param timeout The amount of time (in ms) to wait for the elements to become available in the DMA buffer
    * \param elements_acquired Receives the number of DMA buffer elements actually acquired
    * \param elements_remaining Receives the number of DMA buffer elements available to be acquired by the host
    * \return status
    */
    nirio_status acquire(
        data_t*& elements,
        const size_t elements_requested,
        const uint32_t timeout,
        size_t& elements_acquired,
        size_t& elements_remaining);

    /*!
    * Releases space in the DMA buffer so it can be read by the device (output) or written by the device (input)
    * \param elements Size (in elements) of the block to release.
    * \return status
    */
    nirio_status release(const size_t elements);

    /*!
    * Reads data from the DMA FIFO into the provided buffer
    * \param buf The buffer into which to read data from the DMA FIFO
    * \param num_elements Size (in elements) of the data to read
    * \param timeout The amount of time (in ms) to wait for the elements to become available in the DMA buffer
    * \param num_read Receives the number of DMA buffer elements actually read
    * \param num_remaining Receives the number of DMA buffer elements available be read by the host
    * \return status
    */
    nirio_status read(
        data_t* buf,
        const uint32_t num_elements,
        const uint32_t timeout,
        uint32_t& num_read,
        uint32_t& num_remaining);

    /*!
    * Writes data from the DMA FIFO
    * \param buf The buffer containing data to be written to the DMA FIFO
    * \param num_elements Size (in elements) of the data to write
    * \param timeout The amount of time (in ms) to wait for the elements to become available in the DMA buffer
    * \param num_remaining Receives the number of DMA buffer elements available be written by the host
    * \return status
    */
    nirio_status write(
        const data_t* buf,
        const uint32_t num_elements,
        const uint32_t timeout,
        uint32_t& num_remaining);

private:    //Methods

    /*!
    * datatype info accessor
    * \return datatype info
    */
    datatype_info_t _get_datatype_info();

    /*!
    * Queries the total transfer count so far between host and device
    * \param transfer_count Receives the value from total transfer count register
    * \return status
    */
    nirio_status _get_transfer_count(uint64_t& transfer_count);

    /*!
    * Sleeps until expected transfer time has elapsed and checks total transfer count to see if transfer completed
    * \param timeout_ms The amount of time (in ms) to wait for the elements to become available in the DMA buffer
    * \return status
    */
    nirio_status _ensure_transfer_completed(uint32_t timeout_ms);


    /*!
    * Conducts the low level operations to reserve DMA buffer space from RIO kernel driver
    * \param elements_requested The minimum number of elements to acquire
    * \param timeout_in_ms The amount of time (in ms) to wait for the elements to become available in the DMA buffer
    * \param fifo_optimization_option FIFO acquire policy (MINIMIZE_LATENCY [fetch what was there last time] or MAXIMIZE_THROUGHPUT [fetch all available])
    * \param status status chaining variable
    * \return Whether acquisition of requested elements (or more) was successful.
    */
    bool _acquire_block_from_rio_buffer(
        size_t elements_requested,
        uint64_t timeout_in_ms,
        const fifo_optimization_option_t fifo_optimization_option,
        nirio_status& status);

private:    //Members
    enum fifo_state_t {
        UNMAPPED, MAPPED, STARTED
    };

    std::string                    _name;
    fifo_direction_t               _fifo_direction;
    uint32_t                       _fifo_channel;
    datatype_info_t                _datatype_info;
    fifo_state_t                   _state;
    size_t                         _remaining_in_claimed_block;
    size_t                         _remaining_acquirable_elements;
    nirio_driver_iface::rio_mmap_t _mem_map;
    boost::recursive_mutex         _mutex;
    niriok_proxy::sptr             _riok_proxy_ptr;

    uint64_t                       _expected_xfer_count;
    uint32_t                       _dma_base_addr;

    data_t*                        _elements_buffer;
    size_t                         _actual_depth_in_elements;
    boost::atomic<size_t>          _total_elements_acquired;
    size_t                         _frame_size_in_elements;
    fifo_optimization_option_t     _fifo_optimization_option;

    static const uint32_t FIFO_LOCK_TIMEOUT_IN_MS = 5000;
};

#include <uhd/transport/nirio/nirio_fifo.ipp>

}}

#endif /* INCLUDED_UHD_TRANSPORT_NIRIO_NIRIO_FIFO_H */

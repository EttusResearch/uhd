//
// Copyright 2013-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// "push" and "pop" introduced in GCC 4.6; works with all clang
#if defined(__clang__) || defined(__GNUC__) && (__GNUC__ > 3) && (__GNUC_MINOR__ > 5)
    #pragma GCC diagnostic push
#endif
#if defined(__clang__) || defined(__GNUC__)
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

template <typename data_t>
nirio_fifo<data_t>::nirio_fifo(
    niriok_proxy::sptr riok_proxy,
    const fifo_direction_t direction,
    const std::string& name,
    const uint32_t fifo_instance) :
    _name(name),
    _fifo_direction(direction),
    _fifo_channel(fifo_instance),
    _datatype_info(_get_datatype_info()),
    _state(UNMAPPED),
    _remaining_in_claimed_block(0),
    _remaining_acquirable_elements(0),
    _mem_map(),
    _riok_proxy_ptr(riok_proxy),
    _expected_xfer_count(0),
    _dma_base_addr(0),
    _elements_buffer(NULL),
    _actual_depth_in_elements(0),
    _total_elements_acquired(0),
    _frame_size_in_elements(0),
    _fifo_optimization_option(MINIMIZE_LATENCY)
{
    nirio_status status = 0;
    nirio_status_chain(_riok_proxy_ptr->set_attribute(RIO_ADDRESS_SPACE, BUS_INTERFACE), status);
    uint32_t base_addr = 0;
    uint32_t addr_space_word = 0;
    nirio_status_chain(_riok_proxy_ptr->peek(0x1C, base_addr), status);
    nirio_status_chain(_riok_proxy_ptr->peek(0xC, addr_space_word), status);
    _dma_base_addr = base_addr + (_fifo_channel * (1<<((addr_space_word>>16)&0xF)));
}

template <typename data_t>
nirio_fifo<data_t>::~nirio_fifo()
{
    finalize();
}

template <typename data_t>
nirio_status nirio_fifo<data_t>::initialize(
    const size_t requested_depth,
    const size_t frame_size_in_elements,
    size_t& actual_depth,
    size_t& actual_size,
    const fifo_optimization_option_t fifo_optimization_option)
{
    nirio_status status = NiRio_Status_Success;
    if (!_riok_proxy_ptr) return NiRio_Status_ResourceNotInitialized;
    boost::unique_lock<boost::recursive_mutex> lock(_mutex);

    if (_state == UNMAPPED) {

        _frame_size_in_elements = frame_size_in_elements;
        _fifo_optimization_option = fifo_optimization_option;

        uint32_t actual_depth_u32 = 0;
        uint32_t actual_size_u32 = 0;

        //Forcefully stop the fifo if it is running
        _riok_proxy_ptr->stop_fifo(_fifo_channel);    //Cleanup operation. Ignore status.

        //Configure the FIFO now that we know it is stopped
        status = _riok_proxy_ptr->configure_fifo(
            _fifo_channel, 
            static_cast<uint32_t>(requested_depth),
            1,
            actual_depth_u32,
            actual_size_u32);
        if (nirio_status_fatal(status)) return status;

        actual_depth = static_cast<size_t>(actual_depth_u32);
        _actual_depth_in_elements = actual_depth;
        actual_size = static_cast<size_t>(actual_size_u32);

        status = _riok_proxy_ptr->map_fifo_memory(_fifo_channel, actual_size, _mem_map);

        if (nirio_status_not_fatal(status)) {
            _state = MAPPED;
        }
    } else {
        status = NiRio_Status_SoftwareFault;
    }
    return status;
}

template <typename data_t>
void nirio_fifo<data_t>::finalize()
{
    boost::unique_lock<boost::recursive_mutex> lock(_mutex);

    //If the FIFO is started, the stop will change the state to MAPPED.
    stop();

    if (_state == MAPPED) {
        _riok_proxy_ptr->unmap_fifo_memory(_mem_map);
        _state = UNMAPPED;    //Assume teardown succeeded
    }
}


template <typename data_t>
bool nirio_fifo<data_t>::_acquire_block_from_rio_buffer(
    size_t elements_requested,
    uint64_t timeout_in_ms,
    const fifo_optimization_option_t fifo_optimization_option,
    nirio_status& status)
{
    uint32_t elements_acquired_u32 = 0;
    uint32_t elements_remaining_u32 = 0;
    size_t elements_to_request = 0;
    void* elements_buffer = NULL;

    if (fifo_optimization_option == MAXIMIZE_THROUGHPUT)
    {
        // We'll maximize throughput by acquiring all the data that is available
        // But this comes at the cost of an extra wait_on_fifo in which we query
        // the total available by requesting 0.
          
        // first, see how many are available to acquire
        // by trying to acquire 0
        nirio_status_chain(_riok_proxy_ptr->wait_on_fifo(
            _fifo_channel,
            0, // elements requested
            static_cast<uint32_t>(_datatype_info.scalar_type),
            _datatype_info.width * 8,
            0, // timeout
            _fifo_direction == OUTPUT_FIFO,
            elements_buffer,
            elements_acquired_u32,
            elements_remaining_u32),
            status);
    
        // acquire the maximum possible elements- all remaining
        // yet limit to a multiple of the frame size
        // (don't want to acquire partial frames)
        elements_to_request = elements_remaining_u32 - (elements_remaining_u32 % _frame_size_in_elements);

        // the next call to wait_on_fifo can have a 0 timeout since we
        // know there is at least as much data as we will request available
        timeout_in_ms = 0;
    }
    else
    {
        // fifo_optimization_option == MINIMIZE_LATENCY
        // acquire either the minimum amount (frame size) or the amount remaining from the last call 
        // (coerced to a multiple of frames)
        elements_to_request = std::max(
            elements_requested,
            (_remaining_acquirable_elements - (_remaining_acquirable_elements % _frame_size_in_elements)));
    }
    
    nirio_status_chain(_riok_proxy_ptr->wait_on_fifo(
        _fifo_channel,
        elements_to_request,
        static_cast<uint32_t>(_datatype_info.scalar_type),
        _datatype_info.width * 8,
        timeout_in_ms,
        _fifo_direction == OUTPUT_FIFO,
        elements_buffer,
        elements_acquired_u32,
        elements_remaining_u32),
        status);

    if (nirio_status_not_fatal(status))
    {
        _remaining_acquirable_elements = static_cast<size_t>(elements_remaining_u32);

        if (elements_acquired_u32 > 0)
        {
            _total_elements_acquired += static_cast<size_t>(elements_acquired_u32);
            _remaining_in_claimed_block = static_cast<size_t>(elements_acquired_u32);
            _elements_buffer = static_cast<data_t*>(elements_buffer);
        }

        return true;
    }

    return false;
}


template <typename data_t>
nirio_status nirio_fifo<data_t>::start()
{
    nirio_status status = NiRio_Status_Success;
    if (!_riok_proxy_ptr) return NiRio_Status_ResourceNotInitialized;

    boost::unique_lock<boost::recursive_mutex> lock(_mutex);

    if (_state == STARTED) {
        //Do nothing. Already started.
    } else if (_state == MAPPED) {

        _total_elements_acquired = 0;
        _remaining_in_claimed_block = 0;
        _remaining_acquirable_elements = 0;

        status = _riok_proxy_ptr->start_fifo(_fifo_channel);

        if (nirio_status_not_fatal(status)) {
            _state = STARTED;
            _expected_xfer_count = 0;
            
            if (_fifo_direction == OUTPUT_FIFO)
            {
                // pre-acquire a block of data 
                // (should be entire DMA buffer at this point)

                // requesting 0 elements, but it will acquire all since MAXIMUM_THROUGHPUT
                _acquire_block_from_rio_buffer(0, 1000, MAXIMIZE_THROUGHPUT, status);
            }
        }
    } else {
        status = NiRio_Status_ResourceNotInitialized;
    }
    return status;
}

template <typename data_t>
nirio_status nirio_fifo<data_t>::stop()
{
    nirio_status status = NiRio_Status_Success;
    if (!_riok_proxy_ptr) return NiRio_Status_ResourceNotInitialized;

    boost::unique_lock<boost::recursive_mutex> lock(_mutex);

    if (_state == STARTED) {

        // release any remaining acquired elements
        if (_total_elements_acquired > 0) release(_total_elements_acquired);
        _total_elements_acquired = 0;
        _remaining_in_claimed_block = 0;
        _remaining_acquirable_elements = 0;

        status = _riok_proxy_ptr->stop_fifo(_fifo_channel);

        _state = MAPPED;    //Assume teardown succeeded
    }

    return status;
}

template <typename data_t>
nirio_status nirio_fifo<data_t>::acquire(
    data_t*& elements,
    const size_t elements_requested,
    const uint32_t timeout,
    size_t& elements_acquired,
    size_t& elements_remaining)
{
    nirio_status status = NiRio_Status_Success;
    if (!_riok_proxy_ptr || _mem_map.is_null()) return NiRio_Status_ResourceNotInitialized;

    boost::unique_lock<boost::recursive_mutex> lock(_mutex);

    if (_state == STARTED) {

        if (_remaining_in_claimed_block == 0)
        {
   
            // so acquire some now
            if (!_acquire_block_from_rio_buffer(
                     elements_requested,
                     timeout,
                     _fifo_optimization_option,
                     status))
            {
                elements_acquired = 0;
                elements_remaining = _remaining_acquirable_elements;
                return status;
            }


            if (get_direction() == INPUT_FIFO &&
                UHD_NIRIO_RX_FIFO_XFER_CHECK_EN &&
                _riok_proxy_ptr->get_rio_quirks().rx_fifo_xfer_check_en())
            {
                _expected_xfer_count += static_cast<uint64_t>(elements_requested * sizeof(data_t));
                status = _ensure_transfer_completed(timeout);
            }
        }

        if (nirio_status_not_fatal(status))
        {
            // Assign the request the proper area of the DMA FIFO buffer
            elements = _elements_buffer;
            elements_acquired = std::min(_remaining_in_claimed_block, elements_requested);
            _remaining_in_claimed_block -= elements_acquired;
            elements_remaining = _remaining_in_claimed_block + _remaining_acquirable_elements;

            // Advance the write pointer forward in the acquired elements buffer
            _elements_buffer += elements_acquired;
        }
            
    } else {
        status = NiRio_Status_ResourceNotInitialized;
    }

    return status;
}

template <typename data_t>
nirio_status nirio_fifo<data_t>::release(const size_t elements)
{
    nirio_status status = NiRio_Status_Success;
    if (!_riok_proxy_ptr) return NiRio_Status_ResourceNotInitialized;

    boost::unique_lock<boost::recursive_mutex> lock(_mutex);

    if (_state == STARTED) {
        status = _riok_proxy_ptr->grant_fifo(
            _fifo_channel,
            static_cast<uint32_t>(elements));
        _total_elements_acquired -= elements;
    } else {
        status = NiRio_Status_ResourceNotInitialized;
    }

    return status;
}

template <typename data_t>
nirio_status nirio_fifo<data_t>::read(
    data_t* buf,
    const uint32_t num_elements,
    const uint32_t timeout,
    uint32_t& num_read,
    uint32_t& num_remaining)
{
    nirio_status status = NiRio_Status_Success;
    if (!_riok_proxy_ptr) return NiRio_Status_ResourceNotInitialized;

    boost::unique_lock<boost::recursive_mutex> lock(_mutex);

    if (_state == STARTED) {
        status = _riok_proxy_ptr->read_fifo(
            _fifo_channel,
            num_elements,
            static_cast<void*>(buf),
            _datatype_info.width,
            static_cast<uint32_t>(_datatype_info.scalar_type),
            _datatype_info.width * 8,
            timeout,
            num_read,
            num_remaining);
    } else {
        status = NiRio_Status_ResourceNotInitialized;
    }

    return status;
}

template <typename data_t>
nirio_status nirio_fifo<data_t>::write(
    const data_t* buf,
    const uint32_t num_elements,
    const uint32_t timeout,
    uint32_t& num_remaining)
{
    nirio_status status = NiRio_Status_Success;
    if (!_riok_proxy_ptr) return NiRio_Status_ResourceNotInitialized;

    boost::unique_lock<boost::recursive_mutex> lock(_mutex);

    if (_state == STARTED) {
        status = _riok_proxy_ptr->write_fifo(
            _fifo_channel,
            num_elements,
            buf,
            _datatype_info.width,
            static_cast<uint32_t>(_datatype_info.scalar_type),
            _datatype_info.width * 8,
            timeout,
            num_remaining);
    } else {
        status = NiRio_Status_ResourceNotInitialized;
    }

    return status;
}

template <typename data_t>
nirio_status nirio_fifo<data_t>::_get_transfer_count(uint64_t& transfer_count)
{
    //_riok_proxy_ptr must be valid and _mutex must be locked

    nirio_status status = NiRio_Status_Success;
    uint32_t lower_half = 0, upper_half = 0;
    nirio_status_chain(_riok_proxy_ptr->peek(_dma_base_addr + 0xA8, lower_half), status);  //Latches both halves
    nirio_status_chain(_riok_proxy_ptr->peek(_dma_base_addr + 0xAC, upper_half), status);

    if (nirio_status_not_fatal(status)) {
        transfer_count = lower_half | (((uint64_t)upper_half) << 32);
    }
    return status;
}

template <typename data_t>
nirio_status nirio_fifo<data_t>::_ensure_transfer_completed(uint32_t timeout_ms)
{
    //_riok_proxy_ptr must be valid and _mutex must be locked

    static const size_t MIN_TIMEOUT_IN_US = 2000;

    nirio_status status = NiRio_Status_Success;
    uint64_t actual_xfer_count = 0;
    nirio_status_chain(_get_transfer_count(actual_xfer_count), status);

    //We count the elapsed time using a simple counter instead of the high
    //resolution timebase for efficiency reasons. The call to fetch the time
    //requires a user-kernel transition which has a large overhead compared
    //to a simple mem read. As a tradeoff, we deal with a less precise timeout.
    size_t approx_us_elapsed = 0;
    while (
        nirio_status_not_fatal(status) &&
        (_expected_xfer_count > actual_xfer_count) &&
        approx_us_elapsed++ < std::max<size_t>(MIN_TIMEOUT_IN_US, timeout_ms * 1000)
    ) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        nirio_status_chain(_get_transfer_count(actual_xfer_count), status);
    }

    if (_expected_xfer_count > actual_xfer_count) {
        nirio_status_chain(NiRio_Status_CommunicationTimeout, status);
    }

    return status;
}

template <>
inline datatype_info_t nirio_fifo<int8_t>::_get_datatype_info()
{
    return datatype_info_t(RIO_SCALAR_TYPE_IB, 1);
}

template <>
inline datatype_info_t nirio_fifo<int16_t>::_get_datatype_info()
{
    return datatype_info_t(RIO_SCALAR_TYPE_IW, 2);
}

template <>
inline datatype_info_t nirio_fifo<int32_t>::_get_datatype_info()
{
    return datatype_info_t(RIO_SCALAR_TYPE_IL, 4);
}

template <>
inline datatype_info_t nirio_fifo<int64_t>::_get_datatype_info()
{
    return datatype_info_t(RIO_SCALAR_TYPE_IQ, 8);
}

template <>
inline datatype_info_t nirio_fifo<uint8_t>::_get_datatype_info()
{
    return datatype_info_t(RIO_SCALAR_TYPE_UB, 1);
}

template <>
inline datatype_info_t nirio_fifo<uint16_t>::_get_datatype_info()
{
    return datatype_info_t(RIO_SCALAR_TYPE_UW, 2);
}

template <>
inline datatype_info_t nirio_fifo<uint32_t>::_get_datatype_info()
{
    return datatype_info_t(RIO_SCALAR_TYPE_UL, 4);
}

template <>
inline datatype_info_t nirio_fifo<uint64_t>::_get_datatype_info()
{
    return datatype_info_t(RIO_SCALAR_TYPE_UQ, 8);
}

#if defined(__clang__) || defined(__GNUC__) && (__GNUC__ > 3) && (__GNUC_MINOR__ > 5)
    #pragma GCC diagnostic pop
#endif

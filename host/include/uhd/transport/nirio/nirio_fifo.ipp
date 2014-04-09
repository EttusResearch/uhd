//
// Copyright 2013-2014 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifdef __clang__
    #pragma GCC diagnostic push ignored "-Wmissing-field-initializers"
#elif defined(__GNUC__)
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

template <typename data_t>
nirio_fifo<data_t>::nirio_fifo(
    niriok_proxy& riok_proxy,
    fifo_direction_t direction,
    const std::string& name,
    uint32_t fifo_instance) :
    _name(name),
    _fifo_direction(direction),
    _fifo_channel(fifo_instance),
    _datatype_info(_get_datatype_info()),
    _state(UNMAPPED),
    _acquired_pending(0),
    _mem_map(),
    _riok_proxy_ptr(&riok_proxy),
    _expected_xfer_count(0),
    _dma_base_addr(0)
{
    nirio_status status = 0;
    nirio_status_chain(_riok_proxy_ptr->set_attribute(ADDRESS_SPACE, BUS_INTERFACE), status);
    uint32_t base_addr, addr_space_word;
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
    size_t& actual_depth,
    size_t& actual_size)
{
    nirio_status status = NiRio_Status_Success;
    if (!_riok_proxy_ptr) return NiRio_Status_ResourceNotInitialized;
    boost::unique_lock<boost::recursive_mutex> lock(_mutex);

    if (_state == UNMAPPED) {
        nirio_driver_iface::nirio_syncop_in_params_t in = {};
        nirio_driver_iface::nirio_syncop_out_params_t out = {};

        //Forcefully stop the fifo if it is running
        in.function    = nirio_driver_iface::NIRIO_FUNC::FIFO;
        in.subfunction = nirio_driver_iface::NIRIO_FIFO::STOP;
        in.params.fifo.channel = _fifo_channel;
        _riok_proxy_ptr->sync_operation(&in, sizeof(in), &out, sizeof(out));    //Cleanup operation. Ignore status.

        //Configure the FIFO now that we know it is stopped
        in.function = nirio_driver_iface::NIRIO_FUNC::FIFO;
        in.subfunction = nirio_driver_iface::NIRIO_FIFO::CONFIGURE;
        in.params.fifo.channel = _fifo_channel;
        in.params.fifo.op.config.requestedDepth = static_cast<uint32_t>(requested_depth);
        in.params.fifo.op.config.requiresActuals = 1;
        status = _riok_proxy_ptr->sync_operation(&in, sizeof(in), &out, sizeof(out));

        if (nirio_status_fatal(status)) return status;

        actual_depth = out.params.fifo.op.config.actualDepth;
        actual_size = out.params.fifo.op.config.actualSize;

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
nirio_status nirio_fifo<data_t>::start()
{
    nirio_status status = NiRio_Status_Success;
    if (!_riok_proxy_ptr) return NiRio_Status_ResourceNotInitialized;

    boost::unique_lock<boost::recursive_mutex> lock(_mutex);

    if (_state == STARTED) {
        //Do nothing. Already started.
    } else if (_state == MAPPED) {
        nirio_driver_iface::nirio_syncop_in_params_t in = {};
        nirio_driver_iface::nirio_syncop_out_params_t out = {};

        in.function    = nirio_driver_iface::NIRIO_FUNC::FIFO;
        in.subfunction = nirio_driver_iface::NIRIO_FIFO::START;

        in.params.fifo.channel = _fifo_channel;

        status = _riok_proxy_ptr->sync_operation(&in, sizeof(in), &out, sizeof(out));
        if (nirio_status_not_fatal(status)) {
            _state = STARTED;
            _acquired_pending = 0;
            _expected_xfer_count = 0;
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
        if (_acquired_pending > 0) release(_acquired_pending);

        nirio_driver_iface::nirio_syncop_in_params_t in = {};
        nirio_driver_iface::nirio_syncop_out_params_t out = {};

        in.function    = nirio_driver_iface::NIRIO_FUNC::FIFO;
        in.subfunction = nirio_driver_iface::NIRIO_FIFO::STOP;

        in.params.fifo.channel = _fifo_channel;

        status = _riok_proxy_ptr->sync_operation(&in, sizeof(in), &out, sizeof(out));

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
        nirio_driver_iface::nirio_syncop_in_params_t in = {};
        uint32_t stuffed[2];
        nirio_driver_iface::nirio_syncop_out_params_t out = {};
        init_syncop_out_params(out, stuffed, sizeof(stuffed));

        in.function    = nirio_driver_iface::NIRIO_FUNC::FIFO;
        in.subfunction = nirio_driver_iface::NIRIO_FIFO::WAIT;

        in.params.fifo.channel                   = _fifo_channel;
        in.params.fifo.op.wait.elementsRequested = static_cast<uint32_t>(elements_requested);
        in.params.fifo.op.wait.scalarType        = static_cast<uint32_t>(_datatype_info.scalar_type);
        in.params.fifo.op.wait.bitWidth          = _datatype_info.width * 8;
        in.params.fifo.op.wait.output            = _fifo_direction == OUTPUT_FIFO;
        in.params.fifo.op.wait.timeout           = timeout;

        status = _riok_proxy_ptr->sync_operation(&in, sizeof(in), &out, sizeof(out));

        if (nirio_status_not_fatal(status)) {
            elements = static_cast<data_t*>(out.params.fifo.op.wait.elements.pointer);
            elements_acquired = stuffed[0];
            elements_remaining = stuffed[1];
            _acquired_pending = elements_acquired;

            if (UHD_NIRIO_RX_FIFO_XFER_CHECK_EN &&
                _riok_proxy_ptr->get_rio_quirks().rx_fifo_xfer_check_en() &&
                get_direction() == INPUT_FIFO
            ) {
                _expected_xfer_count += static_cast<uint64_t>(elements_requested * sizeof(data_t));
                status = _ensure_transfer_completed(timeout);
            }
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
        nirio_driver_iface::nirio_syncop_in_params_t in = {};
        nirio_driver_iface::nirio_syncop_out_params_t out = {};

        in.function    = nirio_driver_iface::NIRIO_FUNC::FIFO;
        in.subfunction = nirio_driver_iface::NIRIO_FIFO::GRANT;

        in.params.fifo.channel           = _fifo_channel;
        in.params.fifo.op.grant.elements = static_cast<uint32_t>(elements);

        status = _riok_proxy_ptr->sync_operation(&in, sizeof(in), &out, sizeof(out));
        _acquired_pending = 0;
    } else {
        status = NiRio_Status_ResourceNotInitialized;
    }

    return status;
}

template <typename data_t>
nirio_status nirio_fifo<data_t>::read(
    data_t* buf,
    const uint32_t num_elements,
    uint32_t timeout,
    uint32_t& num_read,
    uint32_t& num_remaining)
{
    nirio_status status = NiRio_Status_Success;
    if (!_riok_proxy_ptr) return NiRio_Status_ResourceNotInitialized;

    boost::unique_lock<boost::recursive_mutex> lock(_mutex);

    if (_state == STARTED) {
        nirio_driver_iface::nirio_syncop_in_params_t in = {};
        nirio_driver_iface::nirio_syncop_out_params_t out = {};
        init_syncop_out_params(out, buf, num_elements * _datatype_info.width);

        in.function = nirio_driver_iface::NIRIO_FUNC::FIFO;
        in.subfunction = nirio_driver_iface::NIRIO_FIFO::READ;

        in.params.fifo.channel = _fifo_channel;
        in.params.fifo.op.readWithDataType.timeout = timeout;
        in.params.fifo.op.readWithDataType.scalarType = static_cast<uint32_t>(_datatype_info.scalar_type);
        in.params.fifo.op.readWithDataType.bitWidth = _datatype_info.width * 8;

        status = _riok_proxy_ptr->sync_operation(&in, sizeof(in), &out, sizeof(out));

        if (nirio_status_not_fatal(status) || status == NiRio_Status_FifoTimeout) {
            num_read = out.params.fifo.op.read.numberRead;
            num_remaining = out.params.fifo.op.read.numberRemaining;
        }
    } else {
        status = NiRio_Status_ResourceNotInitialized;
    }

    return status;
}

template <typename data_t>
nirio_status nirio_fifo<data_t>::write(
    const data_t* buf,
    const uint32_t num_elements,
    uint32_t timeout,
    uint32_t& num_remaining)
{
    nirio_status status = NiRio_Status_Success;
    if (!_riok_proxy_ptr) return NiRio_Status_ResourceNotInitialized;

    boost::unique_lock<boost::recursive_mutex> lock(_mutex);

    if (_state == STARTED) {
        nirio_driver_iface::nirio_syncop_in_params_t in = {};
        init_syncop_in_params(in, buf, num_elements * _datatype_info.width);
        nirio_driver_iface::nirio_syncop_out_params_t out = {};

        in.function = nirio_driver_iface::NIRIO_FUNC::FIFO;
        in.subfunction = nirio_driver_iface::NIRIO_FIFO::WRITE;

        in.params.fifo.channel = _fifo_channel;
        in.params.fifo.op.writeWithDataType.timeout = timeout;
        in.params.fifo.op.readWithDataType.scalarType = static_cast<uint32_t>(_datatype_info.scalar_type);
        in.params.fifo.op.readWithDataType.bitWidth = _datatype_info.width * 8;

        status = _riok_proxy_ptr->sync_operation(&in, sizeof(in), &out, sizeof(out));

        if (nirio_status_not_fatal(status) || status == NiRio_Status_FifoTimeout) {
            num_remaining = out.params.fifo.op.write.numberRemaining;
        }
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
    //resolution timebase for efficieny reasons. The call to fetch the time
    //requires a user-kernel transition which has a large overhead compared
    //to a simple mem read. As a tradeoff, we deal with a less precise timeout.
    size_t approx_us_elapsed = 0;
    while (
        nirio_status_not_fatal(status) &&
        (_expected_xfer_count > actual_xfer_count) &&
        approx_us_elapsed++ < std::max<size_t>(MIN_TIMEOUT_IN_US, timeout_ms * 1000)
    ) {
        boost::this_thread::sleep(boost::posix_time::microseconds(1));
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
    return datatype_info_t(SCALAR_I8, 1);
}

template <>
inline datatype_info_t nirio_fifo<int16_t>::_get_datatype_info()
{
    return datatype_info_t(SCALAR_I16, 2);
}

template <>
inline datatype_info_t nirio_fifo<int32_t>::_get_datatype_info()
{
    return datatype_info_t(SCALAR_I32, 4);
}

template <>
inline datatype_info_t nirio_fifo<int64_t>::_get_datatype_info()
{
    return datatype_info_t(SCALAR_I64, 8);
}

template <>
inline datatype_info_t nirio_fifo<uint8_t>::_get_datatype_info()
{
    return datatype_info_t(SCALAR_U8, 1);
}

template <>
inline datatype_info_t nirio_fifo<uint16_t>::_get_datatype_info()
{
    return datatype_info_t(SCALAR_U16, 2);
}

template <>
inline datatype_info_t nirio_fifo<uint32_t>::_get_datatype_info()
{
    return datatype_info_t(SCALAR_U32, 4);
}

template <>
inline datatype_info_t nirio_fifo<uint64_t>::_get_datatype_info()
{
    return datatype_info_t(SCALAR_U64, 8);
}

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

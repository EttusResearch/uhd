//
// Copyright 2014 Ettus Research LLC
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

#include "e300_async_serial.hpp"

namespace uhd { namespace usrp { namespace gps {

async_serial::async_serial()
    : _io(),
      _port(_io),
      _background_thread(),
      _open(false),
      _error(false)
{
}

async_serial::async_serial(
        const std::string &node,
        const size_t baud_rate,
        boost::asio::serial_port_base::parity opt_parity,
        boost::asio::serial_port_base::character_size opt_csize,
        boost::asio::serial_port_base::flow_control opt_flow,
        boost::asio::serial_port_base::stop_bits opt_stop)
    : _io(),
      _port(_io),
      _background_thread(),
      _open(false),
      _error(false)
{
    open(node, baud_rate, opt_parity, opt_csize, opt_flow, opt_stop);
}

void async_serial::open(
        const std::string &node,
        const size_t baud_rate,
        boost::asio::serial_port_base::parity opt_parity,
        boost::asio::serial_port_base::character_size opt_csize,
        boost::asio::serial_port_base::flow_control opt_flow,
        boost::asio::serial_port_base::stop_bits opt_stop)
{
    if(is_open())
        close();

    _set_error_status(true);
    _port.open(node);
    _port.set_option(
        boost::asio::serial_port_base::baud_rate(baud_rate));
    _port.set_option(opt_parity);
    _port.set_option(opt_csize);
    _port.set_option(opt_flow);
    _port.set_option(opt_stop);

    _io.post(boost::bind(&async_serial::_do_read, this));

    boost::thread t(boost::bind(&boost::asio::io_service::run, &_io));
    _background_thread.swap(t);
    _set_error_status(false);
    _open=true;
}

bool async_serial::is_open() const
{
    return _open;
}

bool async_serial::error_status() const
{
    boost::lock_guard<boost::mutex> l(_error_mutex);
    return _error;
}

void async_serial::close()
{
    if(!is_open())
        return;

    _open=false;
    _io.post(boost::bind(&async_serial::_do_close, this));
    _background_thread.join();
    _io.reset();
    if(error_status())
        throw(boost::system::system_error(boost::system::error_code(),
                "Error while closing the device"));
}

void async_serial::write(const char *data, size_t size)
{
    {
        boost::lock_guard<boost::mutex> l(_write_queue_mutex);
        _write_queue.insert(_write_queue.end(), data, data+size);
    }
    _io.post(boost::bind(&async_serial::_do_write, this));
}

void async_serial::write(const std::vector<char> &data)
{
    {
        boost::lock_guard<boost::mutex> l(_write_queue_mutex);
        _write_queue.insert(
            _write_queue.end(),
            data.begin(),
            data.end());
    }
    _io.post(boost::bind(&async_serial::_do_write, this));
}

void async_serial::write_string(const std::string &s)
{
    {
        boost::lock_guard<boost::mutex> l(_write_queue_mutex);
        _write_queue.insert(
            _write_queue.end(),
            s.begin(),
            s.end());
    }
    _io.post(boost::bind(&async_serial::_do_write, this));
}

async_serial::~async_serial()
{
    if(is_open()) {
        try {
            close();
        } catch(...) {
            //Don't throw from a destructor
        }
    }
}

void async_serial::_do_read()
{
    _port.async_read_some(boost::asio::buffer(
        _read_buffer,READ_BUFFER_SIZE),
        boost::bind(&async_serial::_read_end,
        this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
}

void async_serial::_read_end(
    const boost::system::error_code& error,
    size_t bytes_transferred)
{
    if(error) {
        if(is_open()) {
            _do_close();
            _set_error_status(true);
        }
    } else {
        if(_callback)
            _callback(
                _read_buffer,
                bytes_transferred);
        _do_read();
    }
}

void async_serial::_do_write()
{
    // if a write operation is already in progress, do nothing
    if(_write_buffer == 0) {
        boost::lock_guard<boost::mutex> l(_write_queue_mutex);
        _write_buffer_size=_write_queue.size();
        _write_buffer.reset(new char[_write_queue.size()]);
        std::copy(_write_queue.begin(),_write_queue.end(),
                _write_buffer.get());
        _write_queue.clear();
        async_write(
            _port, boost::asio::buffer(_write_buffer.get(),
               _write_buffer_size),
            boost::bind(
                &async_serial::_write_end,
                this,
                boost::asio::placeholders::error));
    }
}

void async_serial::_write_end(const boost::system::error_code& error)
{
    if(!error) {
        boost::lock_guard<boost::mutex> l(_write_queue_mutex);
        if(_write_queue.empty()) {
            _write_buffer.reset();
            _write_buffer_size=0;
            return;
        }
        _write_buffer_size = _write_queue.size();
        _write_buffer.reset(new char[_write_queue.size()]);
        std::copy(_write_queue.begin(),_write_queue.end(),
             _write_buffer.get());
        _write_queue.clear();
        async_write(
            _port,
            boost::asio::buffer(_write_buffer.get(),
            _write_buffer_size),
            boost::bind(
               &async_serial::_write_end,
               this,
               boost::asio::placeholders::error));
    } else {
        _set_error_status(true);
        _do_close();
    }
}

void async_serial::_do_close()
{
    boost::system::error_code ec;
    _port.cancel(ec);
    if(ec)
        _set_error_status(true);
    _port.close(ec);
    if(ec)
        _set_error_status(true);
}

void async_serial::_set_error_status(const bool e)
{
    boost::lock_guard<boost::mutex> l(_error_mutex);
    _error = e;
}


void async_serial::set_read_callback(
    const boost::function<void (const char*, size_t)> &callback)
{
    _callback = callback;
}


}}} // namespace

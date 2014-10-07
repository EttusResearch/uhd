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

#ifndef INCLUDED_ASYNC_SERIAL_HPP
#define INCLUDED_ASYNC_SERIAL_HPP

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/shared_array.hpp>

namespace uhd { namespace usrp { namespace gps {

class async_serial : private boost::noncopyable
{
public:
    async_serial();
    ~async_serial();

    async_serial(const std::string &node, const size_t baud_rate,
        boost::asio::serial_port_base::parity opt_parity=
            boost::asio::serial_port_base::parity(
                boost::asio::serial_port_base::parity::none),
        boost::asio::serial_port_base::character_size opt_csize=
            boost::asio::serial_port_base::character_size(8),
        boost::asio::serial_port_base::flow_control opt_flow=
            boost::asio::serial_port_base::flow_control(
                boost::asio::serial_port_base::flow_control::none),
        boost::asio::serial_port_base::stop_bits opt_stop=
            boost::asio::serial_port_base::stop_bits(
                boost::asio::serial_port_base::stop_bits::one));

   void open(const std::string& node, const size_t baud_rate,
        boost::asio::serial_port_base::parity opt_parity=
            boost::asio::serial_port_base::parity(
                boost::asio::serial_port_base::parity::none),
        boost::asio::serial_port_base::character_size opt_csize=
            boost::asio::serial_port_base::character_size(8),
        boost::asio::serial_port_base::flow_control opt_flow=
            boost::asio::serial_port_base::flow_control(
                boost::asio::serial_port_base::flow_control::none),
        boost::asio::serial_port_base::stop_bits opt_stop=
            boost::asio::serial_port_base::stop_bits(
                boost::asio::serial_port_base::stop_bits::one));

   bool is_open(void) const;

   bool error_status(void) const;

   void close(void);

   void write(const char *data, const size_t size);
   void write(const std::vector<char> &data);

   void write_string(const std::string &s);

   static const size_t READ_BUFFER_SIZE=512;

   void set_read_callback(
       const boost::function<void (const char*, size_t)>& callback);

   void clear_callback();

private: // methods
   void _do_read();

   void _read_end(
       const boost::system::error_code &error,
       size_t bytes_transferred);

   void _do_write();

   void _write_end(const boost::system::error_code &error);

   void _do_close();

   void _set_error_status(const bool e);
private: // members
    boost::asio::io_service         _io;
    boost::asio::serial_port        _port;
    boost::thread                   _background_thread;
    bool                            _open;
    bool                            _error;
    mutable boost::mutex            _error_mutex;

    std::vector<char>              _write_queue;
    boost::shared_array<char>      _write_buffer;
    size_t                         _write_buffer_size;
    boost::mutex                   _write_queue_mutex;
    char                           _read_buffer[READ_BUFFER_SIZE];

    boost::function<void (const char*, size_t)> _callback;
};

}}} // namespace

#endif //INCLUDED_ASYNC_SERIAL_HPP

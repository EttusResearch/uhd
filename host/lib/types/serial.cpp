//
// Copyright 2011-2013 Ettus Research LLC
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

#include <uhd/types/serial.hpp>
#include <boost/thread.hpp> //for sleeping
#include <boost/assign/list_of.hpp>

using namespace uhd;

i2c_iface::~i2c_iface(void)
{
    //empty
}

spi_iface::~spi_iface(void)
{
    //empty
}

uart_iface::~uart_iface(void)
{
    //empty
}

spi_config_t::spi_config_t(edge_t edge):
    mosi_edge(edge),
    miso_edge(edge)
{
    /* NOP */
}

void i2c_iface::write_eeprom(
    boost::uint16_t addr,
    boost::uint16_t offset,
    const byte_vector_t &bytes
){
    for (size_t i = 0; i < bytes.size(); i++){
        //write a byte at a time, its easy that way
        byte_vector_t cmd = boost::assign::list_of(offset+i)(bytes[i]);
        this->write_i2c(addr, cmd);
        boost::this_thread::sleep(boost::posix_time::milliseconds(10)); //worst case write
    }
}

byte_vector_t i2c_iface::read_eeprom(
    boost::uint16_t addr,
    boost::uint16_t offset,
    size_t num_bytes
){
    byte_vector_t bytes;
    for (size_t i = 0; i < num_bytes; i++){
        //do a zero byte write to start read cycle
        this->write_i2c(addr, byte_vector_t(1, offset+i));
        bytes.push_back(this->read_i2c(addr, 1).at(0));
    }
    return bytes;
}

struct eeprom16_impl : i2c_iface
{
    eeprom16_impl(i2c_iface* internal)
    {
        _internal = internal;
    }
    i2c_iface* _internal;

    byte_vector_t read_i2c(
        boost::uint16_t addr,
        size_t num_bytes
    ){
        return _internal->read_i2c(addr, num_bytes);
    }

    void write_i2c(
        boost::uint16_t addr,
        const byte_vector_t &bytes
    ){
        return _internal->write_i2c(addr, bytes);
    }

    byte_vector_t read_eeprom(
        boost::uint16_t addr,
        boost::uint16_t offset,
        size_t num_bytes
    ){
        byte_vector_t cmd = boost::assign::list_of(offset >> 8)(offset & 0xff);
        this->write_i2c(addr, cmd);
        return this->read_i2c(addr, num_bytes);
    }

    void write_eeprom(
        boost::uint16_t addr,
        boost::uint16_t offset,
        const byte_vector_t &bytes
    ){
        for (size_t i = 0; i < bytes.size(); i++)
        {
            //write a byte at a time, its easy that way
            boost::uint16_t offset_i = offset+i;
            byte_vector_t cmd = boost::assign::list_of(offset_i >> 8)(offset_i & 0xff)(bytes[i]);
            this->write_i2c(addr, cmd);
            boost::this_thread::sleep(boost::posix_time::milliseconds(10)); //worst case write
        }
    }
};

i2c_iface::sptr i2c_iface::eeprom16(void)
{
    return i2c_iface::sptr(new eeprom16_impl(this));
}

boost::uint32_t spi_iface::read_spi(
    int which_slave,
    const spi_config_t &config,
    boost::uint32_t data,
    size_t num_bits
){
    return transact_spi(
        which_slave, config, data, num_bits, true
    );
}

void spi_iface::write_spi(
    int which_slave,
    const spi_config_t &config,
    boost::uint32_t data,
    size_t num_bits
){
    transact_spi(
        which_slave, config, data, num_bits, false
    );
}

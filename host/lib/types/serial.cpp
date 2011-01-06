//
// Copyright 2010-2011 Ettus Research LLC
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

spi_config_t::spi_config_t(edge_t edge):
    mosi_edge(edge),
    miso_edge(edge)
{
    /* NOP */
}

void i2c_iface::write_eeprom(
    boost::uint8_t addr,
    boost::uint8_t offset,
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
    boost::uint8_t addr,
    boost::uint8_t offset,
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

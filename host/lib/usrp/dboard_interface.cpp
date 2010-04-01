//
// Copyright 2010 Ettus Research LLC
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

#include <uhd/usrp/dboard_interface.hpp>

using namespace uhd::usrp;

dboard_interface::dboard_interface(void){
    /* NOP */
}

dboard_interface::~dboard_interface(void){
    /* NOP */
}

void dboard_interface::write_spi(
    spi_dev_t dev,
    spi_edge_t edge,
    const byte_vector_t &buf
){
    transact_spi(dev, edge, buf, false); //dont readback
}

dboard_interface::byte_vector_t dboard_interface::read_spi(
    spi_dev_t dev,
    spi_edge_t edge,
    size_t num_bytes
){
    byte_vector_t buf(num_bytes, 0x00); //dummy data
    return transact_spi(dev, edge, buf, true); //readback
}

dboard_interface::byte_vector_t dboard_interface::read_write_spi(
    spi_dev_t dev,
    spi_edge_t edge,
    const byte_vector_t &buf
){
    return transact_spi(dev, edge, buf, true); //readback
}

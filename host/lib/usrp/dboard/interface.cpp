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

#include <uhd/usrp/dboard/interface.hpp>

using namespace uhd::usrp::dboard;

interface::interface(void){
    /* NOP */
}

interface::~interface(void){
    /* NOP */
}

void interface::write_spi(
    spi_dev_t dev,
    spi_push_t push,
    const byte_vector_t &buf
){
    transact_spi(dev, SPI_LATCH_RISE, push, buf, false); //dont readback
}

interface::byte_vector_t interface::read_spi(
    spi_dev_t dev,
    spi_latch_t latch,
    size_t num_bytes
){
    byte_vector_t buf(num_bytes, 0x00); //dummy data
    return transact_spi(dev, latch, SPI_PUSH_RISE, buf, true); //readback
}

interface::byte_vector_t interface::read_write_spi(
    spi_dev_t dev,
    spi_latch_t latch,
    spi_push_t push,
    const byte_vector_t &buf
){
    return transact_spi(dev, latch, push, buf, true); //readback
}

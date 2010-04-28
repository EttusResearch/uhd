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

#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/utils/assert.hpp>
#include <boost/format.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;

static const bool _dboard_eeprom_debug = false;

////////////////////////////////////////////////////////////////////////
// format of daughterboard EEPROM
// 00: 0xDB code for ``I'm a daughterboard''
// 01:   .. Daughterboard ID (LSB)
// 02:   .. Daughterboard ID (MSB)
// 03:   .. io bits  7-0 direction (bit set if it's an output from m'board)
// 04:   .. io bits 15-8 direction (bit set if it's an output from m'board)
// 05:   .. ADC0 DC offset correction (LSB)
// 06:   .. ADC0 DC offset correction (MSB)
// 07:   .. ADC1 DC offset correction (LSB)
// 08:   .. ADC1 DC offset correction (MSB)
//  ...
// 1f:   .. negative of the sum of bytes [0x00, 0x1e]

#define DB_EEPROM_MAGIC         0x00
#define DB_EEPROM_MAGIC_VALUE   0xDB
#define DB_EEPROM_ID_LSB        0x01
#define DB_EEPROM_ID_MSB        0x02
#define DB_EEPROM_OE_LSB        0x03
#define DB_EEPROM_OE_MSB        0x04
#define DB_EEPROM_OFFSET_0_LSB  0x05 // offset correction for ADC or DAC 0
#define DB_EEPROM_OFFSET_0_MSB  0x06
#define DB_EEPROM_OFFSET_1_LSB  0x07 // offset correction for ADC or DAC 1
#define DB_EEPROM_OFFSET_1_MSB  0x08
#define DB_EEPROM_CHKSUM        0x1f

#define DB_EEPROM_CLEN          0x20 // length of common portion of eeprom

#define DB_EEPROM_CUSTOM_BASE   DB_EEPROM_CLEN // first avail offset for
                                               //   daughterboard specific use
////////////////////////////////////////////////////////////////////////

//negative sum of bytes excluding checksum byte
static boost::uint8_t checksum(const byte_vector_t &bytes){
    int sum = 0;
    for (size_t i = 0; i < std::min(bytes.size(), size_t(DB_EEPROM_CHKSUM)); i++){
        sum -= int(bytes.at(i));
    }
    if (_dboard_eeprom_debug)
        std::cout << boost::format("sum: 0x%02x") % sum << std::endl;
    return boost::uint8_t(sum);
}

dboard_eeprom_t::dboard_eeprom_t(const byte_vector_t &bytes){
    if (_dboard_eeprom_debug){
        for (size_t i = 0; i < bytes.size(); i++){
            std::cout << boost::format(
                "eeprom byte[0x%02x] = 0x%02x") % i % int(bytes.at(i)
            ) << std::endl;
        }
    }
    try{
        UHD_ASSERT_THROW(bytes.size() >= DB_EEPROM_CLEN);
        UHD_ASSERT_THROW(bytes[DB_EEPROM_MAGIC] == DB_EEPROM_MAGIC_VALUE);
        UHD_ASSERT_THROW(bytes[DB_EEPROM_CHKSUM] == checksum(bytes));
        id = \
            (boost::uint16_t(bytes[DB_EEPROM_ID_LSB]) << 0) |
            (boost::uint16_t(bytes[DB_EEPROM_ID_MSB]) << 8) ;
    }catch(const uhd::assert_error &){
        id = dboard_id::NONE;
    }
}

byte_vector_t dboard_eeprom_t::get_eeprom_bytes(void){
    byte_vector_t bytes(DB_EEPROM_CLEN, 0); //defaults to all zeros
    bytes[DB_EEPROM_MAGIC] = DB_EEPROM_MAGIC_VALUE;
    bytes[DB_EEPROM_ID_LSB] = boost::uint8_t(id >> 0);
    bytes[DB_EEPROM_ID_MSB] = boost::uint8_t(id >> 8);
    bytes[DB_EEPROM_CHKSUM] = checksum(bytes);
    return bytes;
}

size_t dboard_eeprom_t::num_bytes(void){
    return DB_EEPROM_CLEN;
}

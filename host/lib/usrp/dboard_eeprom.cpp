//
// Copyright 2010-2011,2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/byte_vector.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <sstream>

using namespace uhd;
using namespace uhd::usrp;

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
#define DB_EEPROM_REV_LSB       0x03
#define DB_EEPROM_REV_MSB       0x04
#define DB_EEPROM_OFFSET_0_LSB  0x05 // offset correction for ADC or DAC 0
#define DB_EEPROM_OFFSET_0_MSB  0x06
#define DB_EEPROM_OFFSET_1_LSB  0x07 // offset correction for ADC or DAC 1
#define DB_EEPROM_OFFSET_1_MSB  0x08
#define DB_EEPROM_SERIAL        0x09
#define DB_EEPROM_SERIAL_LEN    0x09 //9 ASCII characters
#define DB_EEPROM_CHKSUM        0x1f

#define DB_EEPROM_CLEN          0x20 // length of common portion of eeprom

#define DB_EEPROM_CUSTOM_BASE   DB_EEPROM_CLEN // first avail offset for
                                               //   daughterboard specific use
////////////////////////////////////////////////////////////////////////

//negative sum of bytes excluding checksum byte
static uint8_t checksum(const byte_vector_t &bytes){
    int sum = 0;
    for (size_t i = 0; i < std::min(bytes.size(), size_t(DB_EEPROM_CHKSUM)); i++){
        sum -= int(bytes.at(i));
    }
    UHD_LOG_TRACE("DB_EEPROM", boost::format("byte sum: 0x%02x") % sum)
    return uint8_t(sum);
}

dboard_eeprom_t::dboard_eeprom_t(void){
    id = dboard_id_t::none();
    serial = "";
}

void dboard_eeprom_t::load(i2c_iface &iface, uint8_t addr){
    byte_vector_t bytes = iface.read_eeprom(addr, 0, DB_EEPROM_CLEN);

    std::ostringstream ss;
    for (size_t i = 0; i < bytes.size(); i++){
        UHD_LOG_TRACE("DB_EEPROM",
            boost::format("eeprom byte[0x%02x] = 0x%02x")
            % i
            % int(bytes.at(i))
        );
    }

    try{
        UHD_ASSERT_THROW(bytes.size() >= DB_EEPROM_CLEN);
        UHD_ASSERT_THROW(bytes[DB_EEPROM_MAGIC] == DB_EEPROM_MAGIC_VALUE);
        UHD_ASSERT_THROW(bytes[DB_EEPROM_CHKSUM] == checksum(bytes));

        //parse the ids
        id = dboard_id_t::from_uint16(0
            | (uint16_t(bytes[DB_EEPROM_ID_LSB]) << 0)
            | (uint16_t(bytes[DB_EEPROM_ID_MSB]) << 8)
        );

        //parse the serial
        serial = bytes_to_string(
            byte_vector_t(&bytes.at(DB_EEPROM_SERIAL),
            &bytes.at(DB_EEPROM_SERIAL+DB_EEPROM_SERIAL_LEN))
        );

        //parse the revision
        const uint16_t rev_num = 0
            | (uint16_t(bytes[DB_EEPROM_REV_LSB]) << 0)
            | (uint16_t(bytes[DB_EEPROM_REV_MSB]) << 8)
        ;
        if (rev_num != 0 and rev_num != 0xffff){
            revision = std::to_string(rev_num);
        }

    }catch(const uhd::assertion_error &){
        id = dboard_id_t::none();
        serial = "";
    }
}

void dboard_eeprom_t::store(i2c_iface &iface, uint8_t addr) const{
    byte_vector_t bytes(DB_EEPROM_CLEN, 0); //defaults to all zeros
    bytes[DB_EEPROM_MAGIC] = DB_EEPROM_MAGIC_VALUE;

    //load the id bytes
    bytes[DB_EEPROM_ID_LSB] = uint8_t(id.to_uint16() >> 0);
    bytes[DB_EEPROM_ID_MSB] = uint8_t(id.to_uint16() >> 8);

    //load the serial bytes
    byte_vector_t ser_bytes = string_to_bytes(serial, DB_EEPROM_SERIAL_LEN);
    std::copy(ser_bytes.begin(), ser_bytes.end(), &bytes.at(DB_EEPROM_SERIAL));

    //load the revision bytes
    if (not revision.empty()){
        const uint16_t rev_num = boost::lexical_cast<uint16_t>(revision);
        bytes[DB_EEPROM_REV_LSB] = uint8_t(rev_num >> 0);
        bytes[DB_EEPROM_REV_MSB] = uint8_t(rev_num >> 8);
    }

    //load the checksum
    bytes[DB_EEPROM_CHKSUM] = checksum(bytes);

    iface.write_eeprom(addr, 0, bytes);
}

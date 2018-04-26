//
// Copyright 2011-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/serial.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <chrono>
#include <thread>

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
    // By default don't use a custom clock speed for the transaction
    use_custom_divider = false;
}

void i2c_iface::write_eeprom(
    uint16_t addr,
    uint16_t offset,
    const byte_vector_t &bytes
){
    for (size_t i = 0; i < bytes.size(); i++){
        //write a byte at a time, its easy that way
        byte_vector_t cmd = {
            narrow_cast<uint8_t>(offset+i),
            narrow_cast<uint8_t>(bytes[i])
        };
        this->write_i2c(addr, cmd);
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); //worst case write
    }
}

byte_vector_t i2c_iface::read_eeprom(
    uint16_t addr,
    uint16_t offset,
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
        uint16_t addr,
        size_t num_bytes
    ){
        return _internal->read_i2c(addr, num_bytes);
    }

    void write_i2c(
        uint16_t addr,
        const byte_vector_t &bytes
    ){
        return _internal->write_i2c(addr, bytes);
    }

    byte_vector_t read_eeprom(
        uint16_t addr,
        uint16_t offset,
        size_t num_bytes
    ){
        byte_vector_t cmd = {
            narrow_cast<uint8_t>(offset >> 8),
            narrow_cast<uint8_t>(offset & 0xff)
        };
        this->write_i2c(addr, cmd);
        return this->read_i2c(addr, num_bytes);
    }

    void write_eeprom(
        uint16_t addr,
        uint16_t offset,
        const byte_vector_t &bytes
    ){
        for (size_t i = 0; i < bytes.size(); i++)
        {
            //write a byte at a time, its easy that way
            uint16_t offset_i = offset+i;
            byte_vector_t cmd{
                narrow_cast<uint8_t>(offset_i >> 8),
                narrow_cast<uint8_t>(offset_i & 0xff),
                bytes[i]
            };
            this->write_i2c(addr, cmd);
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); //worst case write
        }
    }
};

i2c_iface::sptr i2c_iface::eeprom16(void)
{
    return i2c_iface::sptr(new eeprom16_impl(this));
}

uint32_t spi_iface::read_spi(
    int which_slave,
    const spi_config_t &config,
    uint32_t data,
    size_t num_bits
){
    return transact_spi(
        which_slave, config, data, num_bits, true
    );
}

void spi_iface::write_spi(
    int which_slave,
    const spi_config_t &config,
    uint32_t data,
    size_t num_bits
){
    transact_spi(
        which_slave, config, data, num_bits, false
    );
}

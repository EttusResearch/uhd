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

#ifndef INCLUDED_UHD_TYPES_SERIAL_HPP
#define INCLUDED_UHD_TYPES_SERIAL_HPP

#include <uhd/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <vector>

namespace uhd{

    /*!
     * Byte vector typedef for passing data in and out of I2C interfaces.
     */
    typedef std::vector<boost::uint8_t> byte_vector_t;

    /*!
     * The i2c interface class:
     * Provides i2c and eeprom functionality.
     * A subclass should only have to implement the i2c routines.
     * An eeprom implementation comes for free with the interface.
     *
     * The eeprom routines are implemented on top of i2c.
     * The built in eeprom implementation only does single
     * byte reads and byte writes over the i2c interface,
     * so it should be portable across multiple eeproms.
     * Override the eeprom routines if this is not acceptable.
     */
    class UHD_API i2c_iface{
    public:
        typedef boost::shared_ptr<i2c_iface> sptr;

        /*!
         * Write bytes over the i2c.
         * \param addr the address
         * \param buf the vector of bytes
         */
        virtual void write_i2c(
            boost::uint8_t addr,
            const byte_vector_t &buf
        ) = 0;

        /*!
         * Read bytes over the i2c.
         * \param addr the address
         * \param num_bytes number of bytes to read
         * \return a vector of bytes
         */
        virtual byte_vector_t read_i2c(
            boost::uint8_t addr,
            size_t num_bytes
        ) = 0;

        /*!
         * Write bytes to an eeprom.
         * \param addr the address
         * \param offset byte offset
         * \param buf the vector of bytes
         */
        virtual void write_eeprom(
            boost::uint8_t addr,
            boost::uint8_t offset,
            const byte_vector_t &buf
        );

        /*!
         * Read bytes from an eeprom.
         * \param addr the address
         * \param offset byte offset
         * \param num_bytes number of bytes to read
         * \return a vector of bytes
         */
        virtual byte_vector_t read_eeprom(
            boost::uint8_t addr,
            boost::uint8_t offset,
            size_t num_bytes
        );
    };

    /*!
     * The SPI configuration struct:
     * Used to configure a SPI transaction interface.
     */
    struct UHD_API spi_config_t{
        /*!
         * The edge type specifies when data is valid
         * relative to the edge of the serial clock.
         */
        enum edge_t{
            EDGE_RISE = 'r',
            EDGE_FALL = 'f'
        };

        //! on what edge is the mosi data valid?
        edge_t mosi_edge;

        //! on what edge is the miso data valid?
        edge_t miso_edge;

        /*!
         * Create a new spi config.
         * \param edge the default edge for mosi and miso
         */
        spi_config_t(edge_t edge = EDGE_RISE);
    };
    
    /*!
     * The SPI interface class.
     * Provides routines to transact SPI and do other useful things which haven't been defined yet.
     */
    class UHD_API spi_iface{
    public:
        typedef boost::shared_ptr<spi_iface> sptr;

        /*!
        * Perform a spi transaction.
        * \param which_slave the slave device number
        * \param config spi config args
        * \param data the bits to write
        * \param num_bits how many bits in data
        * \param readback true to readback a value
        * \return spi data if readback set
        */
        virtual boost::uint32_t transact_spi(
            int which_slave,
            const spi_config_t &config,
            boost::uint32_t data,
            size_t num_bits,
            bool readback
        ) = 0;
        
        /*!
        * Read from the SPI bus.
        * \param which_slave the slave device number
        * \param config spi config args
        * \param data the bits to write out (be sure to set write bit) 
        * \param num_bits how many bits in data
        * \return spi data
        */
        virtual boost::uint32_t read_spi(
            int which_slave,
            const spi_config_t &config,
            boost::uint32_t data,
            size_t num_bits
        );
        
        /*!
        * Write to the SPI bus.
        * \param which_slave the slave device number
        * \param config spi config args
        * \param data the bits to write
        * \param num_bits how many bits in data
        */
        virtual void write_spi(
            int which_slave,
            const spi_config_t &config,
            boost::uint32_t data,
            size_t num_bits
        );
    };

    /*!
     * UART interface to write and read bytes.
     */
    class UHD_API uart_iface{
    public:
        typedef boost::shared_ptr<uart_iface> sptr;

        /*!
         * Write to a serial port.
         * \param buf the data to write
         */
        virtual void write_uart(const std::string &buf) = 0;

        /*!
         * Read from a serial port.
         * Reads until complete line or timeout.
         * \param timeout the timeout in seconds
         * \return the data read from the serial port
         */
        virtual std::string read_uart(double timeout) = 0;
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_SERIAL_HPP */

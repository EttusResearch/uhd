//
// Copyright 2010-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TYPES_SERIAL_HPP
#define INCLUDED_UHD_TYPES_SERIAL_HPP

#include <uhd/config.hpp>
#include <boost/shared_ptr.hpp>
#include <stdint.h>
#include <vector>

namespace uhd{

    /*!
     * Byte vector typedef for passing data in and out of I2C interfaces.
     */
    typedef std::vector<uint8_t> byte_vector_t;

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

        virtual ~i2c_iface(void);

        //! Create an i2c_iface than can talk to 16 bit addressable EEPROMS
        i2c_iface::sptr eeprom16(void);

        /*!
         * Write bytes over the i2c.
         * \param addr the address
         * \param buf the vector of bytes
         */
        virtual void write_i2c(
            uint16_t addr,
            const byte_vector_t &buf
        ) = 0;

        /*!
         * Read bytes over the i2c.
         * \param addr the address
         * \param num_bytes number of bytes to read
         * \return a vector of bytes
         */
        virtual byte_vector_t read_i2c(
            uint16_t addr,
            size_t num_bytes
        ) = 0;

        /*!
         * Write bytes to an eeprom.
         * \param addr the address
         * \param offset byte offset
         * \param buf the vector of bytes
         */
        virtual void write_eeprom(
            uint16_t addr,
            uint16_t offset,
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
            uint16_t addr,
            uint16_t offset,
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

        //! Set the clock speed for this transaction
        bool use_custom_divider;

        //! Optionally set the SPI clock divider for this transaction
        size_t divider;

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

        virtual ~spi_iface(void);

        /*!
        * Perform a spi transaction.
        * \param which_slave the slave device number
        * \param config spi config args
        * \param data the bits to write
        * \param num_bits how many bits in data
        * \param readback true to readback a value
        * \return spi data if readback set
        */
        virtual uint32_t transact_spi(
            int which_slave,
            const spi_config_t &config,
            uint32_t data,
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
        virtual uint32_t read_spi(
            int which_slave,
            const spi_config_t &config,
            uint32_t data,
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
            uint32_t data,
            size_t num_bits
        );
    };

    /*!
     * UART interface to write and read strings.
     */
    class UHD_API uart_iface{
    public:
        typedef boost::shared_ptr<uart_iface> sptr;

        virtual ~uart_iface(void);

        /*!
         * Write to a serial port.
         * \param buf the data to write
         */
        virtual void write_uart(const std::string &buf) = 0;

        /*!
         * Read a line from a serial port.
         * \param timeout the timeout in seconds
         * \return the line or empty string upon timeout
         */
        virtual std::string read_uart(double timeout) = 0;
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_SERIAL_HPP */

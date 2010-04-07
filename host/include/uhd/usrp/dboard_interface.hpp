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

#ifndef INCLUDED_UHD_USRP_DBOARD_INTERFACE_HPP
#define INCLUDED_UHD_USRP_DBOARD_INTERFACE_HPP

#include <uhd/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <vector>

namespace uhd{ namespace usrp{

/*!
 * The daughter board dboard_interface to be subclassed.
 * A dboard instance dboard_interfaces with the mboard though this api. 
 * This dboard_interface provides i2c, spi, gpio, atr, aux dac/adc access.
 * Each mboard should have a specially tailored dboard dboard_interface.
 */
class UHD_API dboard_interface{
public:
    typedef boost::shared_ptr<dboard_interface> sptr;
    typedef std::vector<boost::uint8_t> byte_vector_t;

    //tells the host which unit to use
    enum unit_type_t{
        UNIT_TYPE_RX = 'r',
        UNIT_TYPE_TX = 't'
    };

    //spi configuration struct
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
        spi_config_t(edge_t edge = EDGE_RISE){
            mosi_edge = edge;
            miso_edge = edge;
        }
    };

    //tell the host which gpio bank
    enum gpio_bank_t{
        GPIO_BANK_RX = 'r',
        GPIO_BANK_TX = 't'
    };

    //possible atr registers
    enum atr_reg_t{
        ATR_REG_IDLE        = 'i',
        ATR_REG_TX_ONLY     = 't',
        ATR_REG_RX_ONLY     = 'r',
        ATR_REG_FULL_DUPLEX = 'f'
    };

    /*!
     * Write to an aux dac.
     * \param unit which unit rx or tx
     * \param which_dac the dac index 0, 1, 2, 3...
     * \param value the value to write
     */
    virtual void write_aux_dac(unit_type_t unit, int which_dac, int value) = 0;

    /*!
     * Read from an aux adc.
     * \param unit which unit rx or tx
     * \param which_adc the adc index 0, 1, 2, 3...
     * \return the value that was read
     */
    virtual int read_aux_adc(unit_type_t unit, int which_adc) = 0;

    /*!
     * Set a daughterboard ATR register.
     *
     * \param bank   GPIO_TX_BANK or GPIO_RX_BANK
     * \param reg    which ATR register to set
     * \param value  16-bits, 0=FPGA output low, 1=FPGA output high
     */
    virtual void set_atr_reg(gpio_bank_t bank, atr_reg_t reg, boost::uint16_t value) = 0;

    /*!
     * Set daughterboard GPIO data direction register.
     *
     * \param bank      GPIO_TX_BANK or GPIO_RX_BANK
     * \param value     16-bits, 0=FPGA input, 1=FPGA output
     */
    virtual void set_gpio_ddr(gpio_bank_t bank, boost::uint16_t value) = 0;

    /*!
     * Read daughterboard GPIO pin values
     *
     * \param bank GPIO_TX_BANK or GPIO_RX_BANK
     * \return the value of the gpio bank
     */
    virtual boost::uint16_t read_gpio(gpio_bank_t bank) = 0;

    /*!
     * \brief Write to I2C peripheral
     * \param i2c_addr I2C bus address (7-bits)
     * \param buf the data to write
     */
    virtual void write_i2c(int i2c_addr, const byte_vector_t &buf) = 0;

    /*!
     * \brief Read from I2C peripheral
     * \param i2c_addr I2C bus address (7-bits)
     * \param num_bytes number of bytes to read
     * \return the data read if successful, else a zero length string.
     */
    virtual byte_vector_t read_i2c(int i2c_addr, size_t num_bytes) = 0;

    /*!
     * \brief Write data to SPI bus peripheral.
     *
     * \param unit which unit, rx or tx
     * \param config configuration settings
     * \param data the bits to write LSB first
     * \param num_bits the number of bits in data
     */
    virtual void write_spi(
        unit_type_t unit,
        const spi_config_t &config,
        boost::uint32_t data,
        size_t num_bits
    ) = 0;

    /*!
     * \brief Read data to SPI bus peripheral.
     *
     * \param unit which unit, rx or tx
     * \param config configuration settings
     * \param num_bits the number of bits
     * \return the data that was read
     */
    virtual boost::uint32_t read_spi(
        unit_type_t unit,
        const spi_config_t &config,
        size_t num_bits
    ) = 0;

    /*!
     * \brief Read and write data to SPI bus peripheral.
     * The data read back will be the same length as the input buffer.
     *
     * \param unit which unit, rx or tx
     * \param config configuration settings
     * \param data the bits to write LSB first
     * \param num_bits the number of bits in data
     * \return the data that was read
     */
    virtual boost::uint32_t read_write_spi(
        unit_type_t unit,
        const spi_config_t &config,
        boost::uint32_t data,
        size_t num_bits
    ) = 0;

    /*!
     * \brief Get the rate of the rx dboard clock.
     * \return the clock rate in Hz
     */
    virtual double get_rx_clock_rate(void) = 0;

    /*!
     * \brief Get the rate of the tx dboard clock.
     * \return the clock rate in Hz
     */
    virtual double get_tx_clock_rate(void) = 0;
};

}} //namespace

#endif /* INCLUDED_UHD_USRP_DBOARD_INTERFACE_HPP */

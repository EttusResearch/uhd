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

#ifndef INCLUDED_UHD_USRP_DBOARD_IFACE_HPP
#define INCLUDED_UHD_USRP_DBOARD_IFACE_HPP

#include <uhd/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <vector>

namespace uhd{ namespace usrp{

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

/*!
 * The daughter board dboard interface to be subclassed.
 * A dboard instance interfaces with the mboard though this api.
 * This interface provides i2c, spi, gpio, atr, aux dac/adc access.
 * Each mboard should have a specially tailored iface for its dboard.
 */
class UHD_API dboard_iface{
public:
    typedef boost::shared_ptr<dboard_iface> sptr;
    typedef std::vector<boost::uint8_t> byte_vector_t;

    //tells the host which unit to use
    enum unit_t{
        UNIT_RX = 'r',
        UNIT_TX = 't'
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
     *
     * \param unit which unit rx or tx
     * \param which_dac the dac index 0, 1, 2, 3...
     * \param value the value to write
     */
    virtual void write_aux_dac(unit_t unit, int which_dac, int value) = 0;

    /*!
     * Read from an aux adc.
     *
     * \param unit which unit rx or tx
     * \param which_adc the adc index 0, 1, 2, 3...
     * \return the value that was read
     */
    virtual int read_aux_adc(unit_t unit, int which_adc) = 0;

    /*!
     * Set a daughterboard ATR register.
     *
     * \param unit which unit rx or tx
     * \param reg which ATR register to set
     * \param value 16-bits, 0=FPGA output low, 1=FPGA output high
     */
    virtual void set_atr_reg(unit_t unit, atr_reg_t reg, boost::uint16_t value) = 0;

    /*!
     * Set daughterboard GPIO data direction register.
     *
     * \param unit which unit rx or tx
     * \param value 16-bits, 0=FPGA input, 1=FPGA output
     */
    virtual void set_gpio_ddr(unit_t unit, boost::uint16_t value) = 0;

    /*!
     * Read daughterboard GPIO pin values.
     *
     * \param unit which unit rx or tx
     * \return the value of the gpio unit
     */
    virtual boost::uint16_t read_gpio(unit_t unit) = 0;

    /*!
     * Write to an I2C peripheral.
     *
     * \param i2c_addr I2C bus address (7-bits)
     * \param buf the data to write
     */
    virtual void write_i2c(int i2c_addr, const byte_vector_t &buf) = 0;

    /*!
     * Read from an I2C peripheral.
     *
     * \param i2c_addr I2C bus address (7-bits)
     * \param num_bytes number of bytes to read
     * \return the data read if successful, else a zero length string.
     */
    virtual byte_vector_t read_i2c(int i2c_addr, size_t num_bytes) = 0;

    /*!
     * Write data to SPI bus peripheral.
     *
     * \param unit which unit, rx or tx
     * \param config configuration settings
     * \param data the bits to write LSB first
     * \param num_bits the number of bits in data
     */
    virtual void write_spi(
        unit_t unit,
        const spi_config_t &config,
        boost::uint32_t data,
        size_t num_bits
    ) = 0;

    /*!
     * Read and write data to SPI bus peripheral.
     *
     * \param unit which unit, rx or tx
     * \param config configuration settings
     * \param data the bits to write LSB first
     * \param num_bits the number of bits in data
     * \return the data that was read
     */
    virtual boost::uint32_t read_write_spi(
        unit_t unit,
        const spi_config_t &config,
        boost::uint32_t data,
        size_t num_bits
    ) = 0;

    /*!
     * Get the rate of a dboard clock.
     *
     * \param unit which unit rx or tx
     * \return the clock rate in Hz
     */
    virtual double get_clock_rate(unit_t unit) = 0;

    /*!
     * Enable or disable a dboard clock.
     *
     * \param unit which unit rx or tx
     * \param enb true for enabled
     */
    virtual void set_clock_enabled(unit_t unit, bool enb) = 0;
};

}} //namespace

#endif /* INCLUDED_UHD_USRP_DBOARD_IFACE_HPP */

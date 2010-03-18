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
class dboard_interface{
public:
    typedef boost::shared_ptr<dboard_interface> sptr;
    typedef std::vector<boost::uint8_t> byte_vector_t;

    //tells the host which unit to use
    enum unit_type_t{
        UNIT_TYPE_RX,
        UNIT_TYPE_TX
    };

    //tells the host which device to use
    enum spi_dev_t{
        SPI_TX_DEV,
        SPI_RX_DEV
    };

    //args for writing spi data
    enum spi_push_t{
        SPI_PUSH_RISE,
        SPI_PUSH_FALL
    };

    //args for reading spi data
    enum spi_latch_t{
        SPI_LATCH_RISE,
        SPI_LATCH_FALL
    };

    //tell the host which gpio bank
    enum gpio_bank_t{
        GPIO_TX_BANK,
        GPIO_RX_BANK
    };

    //structors
    dboard_interface(void);
    virtual ~dboard_interface(void);

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
     * Set daughterboard ATR register.
     * The ATR register for a particular bank has 2 values:
     * one value when transmitting, one when receiving.
     * The mask controls which pins are controlled by ATR.
     *
     * \param bank      GPIO_TX_BANK or GPIO_RX_BANK
     * \param tx_value  16-bits, 0=FPGA output low, 1=FPGA output high
     * \param rx_value  16-bits, 0=FPGA output low, 1=FPGA output high
     * \param mask      16-bits, 0=software, 1=atr
     */
    virtual void set_atr_reg(gpio_bank_t bank, boost::uint16_t tx_value, boost::uint16_t rx_value, boost::uint16_t mask) = 0;

    /*!
     * Set daughterboard GPIO data direction register.
     *
     * \param bank      GPIO_TX_BANK or GPIO_RX_BANK
     * \param value     16-bits, 0=FPGA input, 1=FPGA output
     * \param mask      16-bits, 0=ignore, 1=set
     */
    virtual void set_gpio_ddr(gpio_bank_t bank, boost::uint16_t value, boost::uint16_t mask) = 0;

    /*!
     * Set daughterboard GPIO pin values.
     *
     * \param bank     GPIO_TX_BANK or GPIO_RX_BANK
     * \param value    16 bits, 0=low, 1=high
     * \param mask     16 bits, 0=ignore, 1=set
     */
    virtual void write_gpio(gpio_bank_t bank, boost::uint16_t value, boost::uint16_t mask) = 0;

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
     * \param dev which spi device
     * \param push args for writing
     * \param buf the data to write
     */
    void write_spi(spi_dev_t dev, spi_push_t push, const byte_vector_t &buf);

    /*!
     * \brief Read data to SPI bus peripheral.
     *
     * \param dev which spi device
     * \param latch args for reading
     * \param num_bytes number of bytes to read
     * \return the data that was read
     */
    byte_vector_t read_spi(spi_dev_t dev, spi_latch_t latch, size_t num_bytes);

    /*!
     * \brief Read and write data to SPI bus peripheral.
     * The data read back will be the same length as the input buffer.
     *
     * \param dev which spi device
     * \param latch args for reading
     * \param push args for clock
     * \param buf the data to write
     * \return the data that was read
     */
    byte_vector_t read_write_spi(spi_dev_t dev, spi_latch_t latch, spi_push_t push, const byte_vector_t &buf);

    /*!
     * \brief Get the rate of the rx dboard clock.
     * \return the clock rate
     */
    virtual double get_rx_clock_rate(void) = 0;

    /*!
     * \brief Get the rate of the tx dboard clock.
     * \return the clock rate
     */
    virtual double get_tx_clock_rate(void) = 0;

private:
    /*!
     * \brief Read and write data to SPI bus peripheral.
     *
     * \param dev which spi device
     * \param latch args for reading
     * \param push args for clock
     * \param buf the data to write
     * \param readback false for write only
     * \return the data that was read
     */
    virtual byte_vector_t transact_spi(
        spi_dev_t dev,
        spi_latch_t latch,
        spi_push_t push,
        const byte_vector_t &buf,
        bool readback
    ) = 0;
};

}} //namespace

#endif /* INCLUDED_UHD_USRP_DBOARD_INTERFACE_HPP */

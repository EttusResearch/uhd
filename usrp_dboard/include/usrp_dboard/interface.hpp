//
// Copyright 2010 Ettus Research LLC
//

#ifndef INCLUDED_USRP_DBOARD_INTERFACE_HPP
#define INCLUDED_USRP_DBOARD_INTERFACE_HPP

#include <boost/shared_ptr.hpp>
#include <stdint.h>

namespace usrp_dboard{

/*!
 * The daughter board interface to be subclassed.
 * Each mboard should have a specially taylored dboard interface.
 * This interface provides i2c, spi, gpio access for dboard instances.
 */
class interface{
public:
    typedef boost::shared_ptr<interface> sptr;

    //tells the host which device to use
    enum spi_dev_t{
        SPI_TX_DEV,
        SPI_RX_DEV,
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
    interface(void);
    virtual ~interface(void);

    /*!
     * Set daughterboard GPIO data direction register.
     *
     * \param bank      GPIO_TX_BANK or GPIO_RX_BANK
     * \param value     16-bits, 0=FPGA input, 1=FPGA output
     * \param mask      16-bits, 0=ignore, 1=set
     */
    virtual void set_gpio_ddr(gpio_bank_t bank, uint16_t value, uint16_t mask) = 0;

    /*!
     * Set daughterboard GPIO pin values.
     *
     * \param bank     GPIO_TX_BANK or GPIO_RX_BANK
     * \param value    16 bits, 0=low, 1=high
     * \param mask     16 bits, 0=ignore, 1=set
     */
    virtual void write_gpio(gpio_bank_t bank, uint16_t value, uint16_t mask) = 0;

    /*!
     * Read daughterboard GPIO pin values
     *
     * \param bank     GPIO_TX_BANK or GPIO_RX_BANK
     * \return the value of the gpio bank
     */
    virtual uint16_t read_gpio(gpio_bank_t bank) = 0;

    /*!
     * \brief Write to I2C peripheral
     * \param i2c_addr		I2C bus address (7-bits)
     * \param buf		the data to write
     * Writes are limited to a maximum of of 64 bytes.
     */
    virtual void write_i2c (int i2c_addr, const std::string &buf) = 0;

    /*!
     * \brief Read from I2C peripheral
     * \param i2c_addr		I2C bus address (7-bits)
     * \param len		number of bytes to read
     * \return the data read if successful, else a zero length string.
     * Reads are limited to a maximum of 64 bytes.
     */
    virtual std::string read_i2c (int i2c_addr, size_t len) = 0;

    /*!
     * \brief Write data to SPI bus peripheral.
     *
     * \param dev which spi device
     * \param push args for writing
     * \param buf		the data to write
     *
     * Writes are limited to a maximum of 64 bytes.
     */
    virtual void write_spi (spi_dev_t dev, spi_push_t push, const std::string &buf) = 0;

    /*!
     * \brief Read data from SPI bus peripheral.
     *
     * \param dev which spi device
     * \param push args for reading
     * \param len		number of bytes to read.  Must be in [0,64].
     * \return the data read if sucessful, else a zero length string.
     *
     * Reads are limited to a maximum of 64 bytes.
     */
    virtual std::string read_spi (spi_dev_t dev, spi_latch_t latch, size_t len) = 0;
};

} //namespace usrp_dboard

#endif /* INCLUDED_USRP_DBOARD_INTERFACE_HPP */

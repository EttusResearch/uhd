//
// Copyright 2010-2013,2015-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_USRP_DBOARD_IFACE_HPP
#define INCLUDED_UHD_USRP_DBOARD_IFACE_HPP

#include <uhd/config.hpp>
#include <uhd/utils/pimpl.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/usrp/fe_connection.hpp>
#include <uhd/usrp/gpio_defs.hpp>
#include <boost/shared_ptr.hpp>
#include <stdint.h>
#include <boost/thread/thread.hpp>
#include <string>
#include <vector>

namespace uhd{ namespace usrp{

//! Special properties that differentiate this daughterboard slot
struct UHD_API dboard_iface_special_props_t{
    /*!
     * Soft clock divider:
     * When a motherboard cannot provided a divided dboard clock,
     * it may provided a "soft" divided clock over an FPGA GPIO.
     * The implementation must know the type of clock provided.
     */
    bool soft_clock_divider;

    /*!
     * Mangle i2c addresses:
     * When i2c is shared across multiple daughterboard slots,
     * the i2c addresses will be mangled on the secondary slot
     * to avoid conflicts between slots in the i2c address space.
     * The mangling is daughterboard specific so the implementation
     * needs to know whether it should use mangled addresses or not.
     */
    bool mangle_i2c_addrs;
};

/*!
 * The daughter board dboard interface to be subclassed.
 * A dboard instance interfaces with the mboard though this api.
 * This interface provides i2c, spi, gpio, atr, aux dac/adc access.
 * Each mboard should have a specially tailored iface for its dboard.
 */
class UHD_API dboard_iface : public uhd::i2c_iface{
public:
    typedef boost::shared_ptr<dboard_iface> sptr;
    typedef dboard_iface_special_props_t special_props_t;

    //! tells the host which unit to use
    enum unit_t{
        UNIT_RX   = int('r'),
        UNIT_TX   = int('t'),
        UNIT_BOTH = int('b'),
    };

    //! aux dac selection enums (per unit)
    enum aux_dac_t{
        AUX_DAC_A = int('a'),
        AUX_DAC_B = int('b'),
        AUX_DAC_C = int('c'),
        AUX_DAC_D = int('d')
    };

    //! aux adc selection enums (per unit)
    enum aux_adc_t{
        AUX_ADC_A = int('a'),
        AUX_ADC_B = int('b')
    };

    typedef uhd::usrp::gpio_atr::gpio_atr_reg_t atr_reg_t;

    virtual ~dboard_iface(void) {};

    /*!
     * Get special properties information for this dboard slot.
     * This call helps the dboard code to handle implementation
     * differences between different motherboards and dboard slots.
     * \return the special properties struct
     */
    virtual special_props_t get_special_props(void) = 0;

    /*!
     * Write to an aux dac.
     *
     * \param unit which unit rx or tx
     * \param which_dac the dac index 0, 1, 2, 3...
     * \param value the value in volts
     */
    virtual void write_aux_dac(unit_t unit, aux_dac_t which_dac, double value) = 0;

    /*!
     * Read from an aux adc.
     *
     * \param unit which unit rx or tx
     * \param which_adc the adc index 0, 1, 2, 3...
     * \return the value in volts
     */
    virtual double read_aux_adc(unit_t unit, aux_adc_t which_adc) = 0;

    /*!
     * Set a daughterboard output pin control source.
     *
     * \param unit which unit rx or tx
     * \param value 16-bits, 0=GPIO controlled, 1=ATR controlled
     * \param mask 16-bits, 0=do not change, 1=change value
     */
    virtual void set_pin_ctrl(
        unit_t unit, uint32_t value, uint32_t mask = 0xffff
    ) = 0;

    /*!
     * Read back the pin control setting.
     *
     * \param unit which unit rx or tx
     * \return the 16-bit settings value
     */
    virtual uint32_t get_pin_ctrl(unit_t unit) = 0;

    /*!
     * Set a daughterboard ATR register.
     *
     * \param unit which unit rx or tx
     * \param reg which ATR register
     * \param value 16-bits, 0=ATR output low, 1=ATR output high
     * \param mask 16-bits, 0=do not change, 1=change value
     */
    virtual void set_atr_reg(
        unit_t unit, atr_reg_t reg, uint32_t value, uint32_t mask = 0xffff
    ) = 0;

    /*!
     * Read back an ATR register setting.
     *
     * \param unit which unit rx or tx
     * \param reg which ATR register
     * \return the 16-bit settings value
     */
    virtual uint32_t get_atr_reg(unit_t unit, atr_reg_t reg) = 0;

    /*!
     * Set daughterboard GPIO data direction setting.
     *
     * \param unit which unit rx or tx
     * \param value 16-bits, 0=GPIO input, 1=GPIO output
     * \param mask 16-bits, 0=do not change, 1=change value
     */
    virtual void set_gpio_ddr(
        unit_t unit, uint32_t value, uint32_t mask = 0xffff
    ) = 0;

    /*!
     * Read back the GPIO data direction setting.
     *
     * \param unit which unit rx or tx
     * \return the 16-bit settings value
     */
    virtual uint32_t get_gpio_ddr(unit_t unit) = 0;

    /*!
     * Set daughterboard GPIO pin output setting.
     *
     * \param unit which unit rx or tx
     * \param value 16-bits, 0=GPIO output low, 1=GPIO output high
     * \param mask 16-bits, 0=do not change, 1=change value
     */
    virtual void set_gpio_out(
        unit_t unit, uint32_t value, uint32_t mask = 0xffff
    ) = 0;

    /*!
     * Read back the GPIO pin output setting.
     *
     * \param unit which unit rx or tx
     * \return the 16-bit settings value
     */
    virtual uint32_t get_gpio_out(unit_t unit) = 0;

    /*!
     * Read daughterboard GPIO pin values.
     *
     * \param unit which unit rx or tx
     * \return the value of the gpio unit
     */
    virtual uint32_t read_gpio(unit_t unit) = 0;

    /*!
     * Write data to SPI bus peripheral.
     *
     * \param unit which unit, rx or tx
     * \param config configuration settings
     * \param data the bits to write MSB first
     * \param num_bits the number of bits in data
     */
    virtual void write_spi(
        unit_t unit,
        const spi_config_t &config,
        uint32_t data,
        size_t num_bits
    ) = 0;

    /*!
     * Read and write data to SPI bus peripheral.
     *
     * \param unit which unit, rx or tx
     * \param config configuration settings
     * \param data the bits to write MSB first
     * \param num_bits the number of bits in data
     * \return the data that was read
     */
    virtual uint32_t read_write_spi(
        unit_t unit,
        const spi_config_t &config,
        uint32_t data,
        size_t num_bits
    ) = 0;

    /*!
     * Set the rate of a dboard clock.
     *
     * \param unit which unit rx or tx
     * \param rate the clock rate in Hz
     */
    virtual void set_clock_rate(unit_t unit, double rate) = 0;

    /*!
     * Get the rate of a dboard clock.
     *
     * \param unit which unit rx or tx
     * \return the clock rate in Hz
     */
    virtual double get_clock_rate(unit_t unit) = 0;

    /*!
     * Get a list of possible rates for the dboard clock.
     *
     * \param unit which unit rx or tx
     * \return a list of clock rates in Hz
     */
    virtual std::vector<double> get_clock_rates(unit_t unit) = 0;

    /*!
     * Enable or disable a dboard clock.
     *
     * \param unit which unit rx or tx
     * \param enb true for enabled
     */
    virtual void set_clock_enabled(unit_t unit, bool enb) = 0;

    /*!
     * Get the rate of the codec.
     * For rx, this is the rate the ADC feeds the DSP.
     * For tx, this is the rate the DSP feeds the DAC.
     * \param unit which unit rx or tx
     * \return the codec rate in Hz
     */
    virtual double get_codec_rate(unit_t unit) = 0;

    /*!
     * Configure the front-end connection parameters.
     *
     * \param unit which unit rx or tx
     * \param fe_name name of the front-end to update
     * \param fe_conn connection parameters class
     */
    virtual void set_fe_connection(
        unit_t unit,
        const std::string& fe_name,
        const uhd::usrp::fe_connection_t& fe_conn
    ) = 0;

    /*! Returns the true if set_fe_connection() is implemented on this dboard_iface
     */
    virtual bool has_set_fe_connection(const unit_t) { return false; }

    /*!
     * Get the command time.
     * \return the command time
     */
    virtual uhd::time_spec_t get_command_time(void) = 0;

    /*!
     * Set the command time.
     * \param t the time
     */
    virtual void set_command_time(const uhd::time_spec_t& t) = 0;

    /*!
     * Sleep for a set time
     * \param time time to sleep in nanoseconds
     */
    virtual void sleep(const boost::chrono::nanoseconds& time);
};

}} //namespace

#endif /* INCLUDED_UHD_USRP_DBOARD_IFACE_HPP */

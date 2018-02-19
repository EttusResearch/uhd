//
// Copyright 2010-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_X300_ADC_CTRL_HPP
#define INCLUDED_X300_ADC_CTRL_HPP

#include <uhd/types/serial.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

class x300_adc_ctrl : boost::noncopyable
{
public:
    typedef boost::shared_ptr<x300_adc_ctrl> sptr;

    virtual ~x300_adc_ctrl(void) = 0;

    /*!
     * Make a codec control for the ADC.
     * \param iface a pointer to the interface object
     * \param spiface the interface to spi
     * \return a new codec control object
     */
    static sptr make(uhd::spi_iface::sptr iface, const size_t slaveno);

    virtual double set_gain(const double &) = 0;

    virtual void set_test_word(const std::string &patterna, const std::string &patternb, const uint32_t = 0) = 0;

    virtual void reset(void) = 0;
};

#endif /* INCLUDED_X300_ADC_CTRL_HPP */

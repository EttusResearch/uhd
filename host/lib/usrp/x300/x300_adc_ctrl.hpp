//
// Copyright 2010-2014 Ettus Research LLC
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

    virtual void set_test_word(const std::string &patterna, const std::string &patternb, const boost::uint32_t = 0) = 0;

    virtual void reset(void) = 0;
};

#endif /* INCLUDED_X300_ADC_CTRL_HPP */

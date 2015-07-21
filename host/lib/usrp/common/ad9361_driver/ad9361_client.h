//
// Copyright 2014 Ettus Research
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

#ifndef INCLUDED_AD9361_CLIENT_H
#define INCLUDED_AD9361_CLIENT_H

#include <boost/shared_ptr.hpp>

namespace uhd { namespace usrp {

/*!
 * Frequency band settings
 */
typedef enum {
    AD9361_RX_BAND0,
    AD9361_RX_BAND1,
    AD9361_TX_BAND0
} frequency_band_t;

/*!
 * Clocking mode
 */
typedef enum {
    AD9361_XTAL_P_CLK_PATH,
    AD9361_XTAL_N_CLK_PATH
} clocking_mode_t;

/*!
 * Digital interface specific
 */
typedef enum {
    AD9361_DDR_FDD_LVCMOS,
    AD9361_DDR_FDD_LVDS
} digital_interface_mode_t;

/*!
 * Interface timing
 */
typedef struct {
    boost::uint8_t rx_clk_delay;
    boost::uint8_t rx_data_delay;
    boost::uint8_t tx_clk_delay;
    boost::uint8_t tx_data_delay;
} digital_interface_delays_t;

class ad9361_params {
public:
    typedef boost::shared_ptr<ad9361_params> sptr;

    virtual ~ad9361_params() {}

    virtual digital_interface_delays_t get_digital_interface_timing() = 0;
    virtual digital_interface_mode_t get_digital_interface_mode() = 0;
    virtual clocking_mode_t get_clocking_mode() = 0;
    virtual double get_band_edge(frequency_band_t band) = 0;
};

class ad9361_io
{
public:
    typedef boost::shared_ptr<ad9361_io> sptr;

    virtual ~ad9361_io() {}

    virtual boost::uint8_t peek8(boost::uint32_t reg) = 0;
    virtual void poke8(boost::uint32_t reg, boost::uint8_t val) = 0;
};


}}

#endif /* INCLUDED_AD9361_CLIENT_H */

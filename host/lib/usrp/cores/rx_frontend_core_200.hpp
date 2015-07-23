//
// Copyright 2011,2014 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_USRP_TX_FRONTEND_CORE_200_HPP
#define INCLUDED_LIBUHD_USRP_TX_FRONTEND_CORE_200_HPP

#include <uhd/config.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhd/property_tree.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <complex>
#include <string>

class rx_frontend_core_200 : boost::noncopyable{
public:
    static const std::complex<double> DEFAULT_DC_OFFSET_VALUE;
    static const bool DEFAULT_DC_OFFSET_ENABLE;
    static const std::complex<double> DEFAULT_IQ_BALANCE_VALUE;

    typedef boost::shared_ptr<rx_frontend_core_200> sptr;

    virtual ~rx_frontend_core_200(void) = 0;

    static sptr make(uhd::wb_iface::sptr iface, const size_t base);

    virtual void set_mux(const bool swap) = 0;

    virtual void set_dc_offset_auto(const bool enb) = 0;

    virtual std::complex<double> set_dc_offset(const std::complex<double> &off) = 0;

    virtual void set_iq_balance(const std::complex<double> &cor) = 0;

    virtual void populate_subtree(uhd::property_tree::sptr subtree) = 0;

};

#endif /* INCLUDED_LIBUHD_USRP_TX_FRONTEND_CORE_200_HPP */

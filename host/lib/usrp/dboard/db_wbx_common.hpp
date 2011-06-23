//
// Copyright 2011 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_USRP_DBOARD_DB_WBX_COMMON_HPP
#define INCLUDED_LIBUHD_USRP_DBOARD_DB_WBX_COMMON_HPP

#include "adf4350_regs.hpp"
#include <uhd/types/dict.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/utils/props.hpp>
#include <uhd/usrp/dboard_base.hpp>

namespace uhd{ namespace usrp{

/***********************************************************************
 * The WBX dboard base class
 **********************************************************************/
class wbx_base : public xcvr_dboard_base{
public:
    wbx_base(ctor_args_t args);
    virtual ~wbx_base(void);

protected:
    virtual void set_rx_gain(double gain, const std::string &name);
    virtual void set_tx_gain(double gain, const std::string &name);

    virtual void set_rx_enabled(bool enb);
    virtual void set_tx_enabled(bool enb);

    void rx_get(const wax::obj &key, wax::obj &val);
    void rx_set(const wax::obj &key, const wax::obj &val);

    void tx_get(const wax::obj &key, wax::obj &val);
    void tx_set(const wax::obj &key, const wax::obj &val);

    /*!
     * Set the LO frequency for the particular dboard unit.
     * \param unit which unit rx or tx
     * \param target_freq the desired frequency in Hz
     * \return the actual frequency in Hz
     */
    virtual double set_lo_freq(dboard_iface::unit_t unit, double target_freq);

    /*!
     * Get the lock detect status of the LO.
     * \param unit which unit rx or tx
     * \return true for locked
     */
    virtual bool get_locked(dboard_iface::unit_t unit);

    /*!
     * Detect if this a v3 WBX
     * \return true for locked
     */
    virtual bool is_v3(void);

private:
    uhd::dict<std::string, double> _tx_gains, _rx_gains;
    bool _rx_enabled, _tx_enabled;
};

}} //namespace uhd::usrp

#endif /* INCLUDED_LIBUHD_USRP_DBOARD_DB_WBX_COMMON_HPP */

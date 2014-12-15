//
// Copyright 2013-2014 Ettus Research LLC
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

#ifndef INCLUDED_B200_CORES_HPP
#define INCLUDED_B200_CORES_HPP

#include "spi_core_3000.hpp"
#include "adf4001_ctrl.hpp"
#include <boost/thread/mutex.hpp>

class b200_local_spi_core : boost::noncopyable, public uhd::spi_iface {

public:
    typedef boost::shared_ptr<b200_local_spi_core> sptr;

    enum perif_t {
        CODEC, PLL
    };

    b200_local_spi_core(uhd::wb_iface::sptr iface, perif_t default_perif);

    virtual boost::uint32_t transact_spi(
        int which_slave,
        const uhd::spi_config_t &config,
        boost::uint32_t data,
        size_t num_bits,
        bool readback);

    void change_perif(perif_t perif);
    void restore_perif();

    static sptr make(uhd::wb_iface::sptr iface, perif_t default_perif = CODEC);

private:
    spi_core_3000::sptr     _spi_core;
    perif_t                 _current_perif;
    perif_t                 _last_perif;
    boost::mutex            _mutex;
};

class b200_ref_pll_ctrl : public uhd::usrp::adf4001_ctrl {
public:
    typedef boost::shared_ptr<b200_ref_pll_ctrl> sptr;

    b200_ref_pll_ctrl(b200_local_spi_core::sptr spi);
    virtual void set_lock_to_ext_ref(bool external);

private:
    b200_local_spi_core::sptr _spi;
};

#endif /* INCLUDED_B200_CORES_HPP */

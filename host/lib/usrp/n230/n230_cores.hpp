//
// Copyright 2013-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_N230_CORES_HPP
#define INCLUDED_N230_CORES_HPP

#include <uhdlib/usrp/cores/spi_core_3000.hpp>
#include <uhdlib/usrp/common/adf4001_ctrl.hpp>
#include <boost/thread/mutex.hpp>

namespace uhd { namespace usrp { namespace n230 {

class n230_core_spi_core : boost::noncopyable, public uhd::spi_iface {

public:
    typedef boost::shared_ptr<n230_core_spi_core> sptr;

    enum perif_t {
        CODEC, PLL
    };

    n230_core_spi_core(uhd::wb_iface::sptr iface, perif_t default_perif);

    virtual uint32_t transact_spi(
        int which_slave,
        const spi_config_t &config,
        uint32_t data,
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

class n230_ref_pll_ctrl : public adf4001_ctrl {
public:
    typedef boost::shared_ptr<n230_ref_pll_ctrl> sptr;

    n230_ref_pll_ctrl(n230_core_spi_core::sptr spi);
    void set_lock_to_ext_ref(bool external);

private:
    n230_core_spi_core::sptr _spi;
};


}}} //namespace

#endif /* INCLUDED_N230_CORES_HPP */

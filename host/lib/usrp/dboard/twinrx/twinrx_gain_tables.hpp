//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_DBOARD_TWINRX_GAIN_TABLES_HPP
#define INCLUDED_DBOARD_TWINRX_GAIN_TABLES_HPP

#include <uhd/config.hpp>
#include <stdint.h>
#include <uhd/types/ranges.hpp>
#include "twinrx_ctrl.hpp"

namespace uhd { namespace usrp { namespace dboard { namespace twinrx {

class twinrx_gain_config_t {
public:
    twinrx_gain_config_t(
        size_t index_, double sys_gain_,
        uint8_t atten1_, uint8_t atten2_,
        bool amp1_, bool amp2_
    ): index(index_), sys_gain(sys_gain_), atten1(atten1_), atten2(atten2_),
       amp1(amp1_), amp2(amp2_)
    {}

    twinrx_gain_config_t& operator=(const twinrx_gain_config_t& src) {
        if (this != &src) {
            this->index = src.index;
            this->sys_gain = src.sys_gain;
            this->atten1 = src.atten1;
            this->atten2 = src.atten2;
            this->amp1 = src.amp1;
            this->amp2 = src.amp2;
        }
        return *this;
    }

    size_t         index;
    double         sys_gain;
    uint8_t atten1;
    uint8_t atten2;
    bool           amp1;
    bool           amp2;
};

class twinrx_gain_table {
public:
    static const twinrx_gain_table lookup_table(
        twinrx_ctrl::signal_path_t signal_path,
        twinrx_ctrl::preselector_path_t presel_path,
        std::string profile);

    twinrx_gain_table(const std::vector<twinrx_gain_config_t>& tbl)
    : _tbl(tbl) {}

    const twinrx_gain_config_t& find_by_index(size_t index) const;

    inline size_t get_num_entries() const {
        return _tbl.size();
    }

    uhd::gain_range_t get_gain_range() const;

private:
    const std::vector<twinrx_gain_config_t>& _tbl;
};


}}}} //namespaces

#endif /* INCLUDED_DBOARD_TWINRX_GAIN_TABLES_HPP */

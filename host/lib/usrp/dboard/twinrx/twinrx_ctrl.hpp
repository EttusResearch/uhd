//
// Copyright 2015 Ettus Research LLC
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

#ifndef INCLUDED_DBOARD_TWINRX_CTRL_HPP
#define INCLUDED_DBOARD_TWINRX_CTRL_HPP

#include "twinrx_io.hpp"
#include <uhd/types/wb_iface.hpp>
#include <boost/noncopyable.hpp>

namespace uhd { namespace usrp { namespace dboard { namespace twinrx {

class twinrx_ctrl : public boost::noncopyable {
public:
    typedef boost::shared_ptr<twinrx_ctrl> sptr;

    static sptr make(
        dboard_iface::sptr db_iface,
        twinrx_gpio::sptr gpio_iface,
        twinrx_cpld_regmap::sptr cpld_regmap,
        dboard_id_t rx_id
    );

    virtual ~twinrx_ctrl() {}

    enum channel_t { CH1 = 0, CH2 = 1, BOTH = 2 };

    enum preamp_state_t { PREAMP_LOWBAND, PREAMP_HIGHBAND, PREAMP_BYPASS };

    enum signal_path_t { PATH_LOWBAND, PATH_HIGHBAND };

    enum preselector_path_t { PRESEL_PATH1, PRESEL_PATH2, PRESEL_PATH3, PRESEL_PATH4 };

    enum lo_source_t { LO_INTERNAL, LO_EXTERNAL, LO_COMPANION, LO_DISABLED, LO_REIMPORT };

    enum lo_export_source_t { LO_CH1_SYNTH, LO_CH2_SYNTH, LO_EXPORT_DISABLED };

    enum antenna_mapping_t { ANTX_NATIVE, ANT1_SHARED, ANT2_SHARED, ANTX_SWAPPED, ANTX_DISABLED };

    enum lo_config_route_t { LO_CONFIG_CH1, LO_CONFIG_CH2, LO_CONFIG_BOTH, LO_CONFIG_NONE };

    enum cal_mode_t { CAL_DISABLED, CAL_CH1, CAL_CH2 };

    virtual void commit() = 0;

    virtual void set_chan_enabled(channel_t ch, bool enabled, bool commit = true) = 0;

    virtual void set_preamp1(channel_t ch, preamp_state_t value, bool commit = true) = 0;

    virtual void set_preamp2(channel_t ch, bool enabled, bool commit = true) = 0;

    virtual void set_lb_preamp_preselector(channel_t ch, bool enabled, bool commit = true) = 0;

    virtual void set_signal_path(channel_t ch, signal_path_t path, bool commit = true) = 0;

    virtual void set_lb_preselector(channel_t ch, preselector_path_t path, bool commit = true) = 0;

    virtual void set_hb_preselector(channel_t ch, preselector_path_t path, bool commit = true) = 0;

    virtual void set_input_atten(channel_t ch, uint8_t atten, bool commit = true) = 0;

    virtual void set_lb_atten(channel_t ch, uint8_t atten, bool commit = true) = 0;

    virtual void set_hb_atten(channel_t ch, uint8_t atten, bool commit = true) = 0;

    virtual void set_lo1_source(channel_t ch, lo_source_t source, bool commit = true) = 0;

    virtual void set_lo2_source(channel_t ch, lo_source_t source, bool commit = true) = 0;

    virtual void set_lo1_export_source(lo_export_source_t source, bool commit = true) = 0;

    virtual void set_lo2_export_source(lo_export_source_t source, bool commit = true) = 0;

    virtual void set_antenna_mapping(antenna_mapping_t mapping, bool commit = true) = 0;

    virtual void set_crossover_cal_mode(cal_mode_t cal_mode, bool commit = true) = 0;

    virtual double set_lo1_synth_freq(channel_t ch, double freq, bool commit = true) = 0;

    virtual double set_lo2_synth_freq(channel_t ch, double freq, bool commit = true) = 0;

    virtual bool read_lo1_locked(channel_t ch) = 0;

    virtual bool read_lo2_locked(channel_t ch) = 0;
};

}}}} //namespaces

#endif /* INCLUDED_DBOARD_TWINRX_CTRL_HPP */

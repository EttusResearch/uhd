//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "ltc5594_regs.hpp"
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/common/ltc5594.hpp>
#include <uhdlib/utils/narrow.hpp>


namespace {
// LOG ID
constexpr char LOG_ID[] = "LTC5594";

constexpr double MIN_DC_OFFSET_ADJ   = -80.0; // mV
constexpr double MAX_DC_OFFSET_ADJ   = 79.375; // mV
constexpr double MIN_PHASE_ADJ       = -2.56; // deg
constexpr double MAX_PHASE_ADJ       = 2.55; // deg
constexpr double MIN_IQ_GAIN_ERR_ADJ = -0.49; // dB
constexpr double MAX_IQ_GAIN_ERR_ADJ = 0.475; // dB

} // namespace

class ltc5594_impl : public ltc5594_iface
{
public:
    explicit ltc5594_impl(write_fn_t&& poke_fn, read_fn_t&& peek_fn)
        : _poke16(std::move(poke_fn)), _peek16(std::move(peek_fn)), _regs()
    {
        _regs = ltc5594_regs_t();
        _regs.save_state();
        // Use HD2IX register as a simple communication test
        const uint8_t random_value = static_cast<uint8_t>(time(NULL));
        _regs.HD2IX                = random_value;
        commit();
        update_field("HD2IX");
        UHD_ASSERT_THROW(_regs.HD2IX == random_value);
        // We know that we can talk, now create a clean state
        reset();
        UHD_LOG_DEBUG(LOG_ID, "Initialized IQ demodulator LTC5594.");
    }

    void commit() override
    {
        UHD_LOG_TRACE(LOG_ID, "Storing register cache to LTC5594...");
        const auto changed_addrs = _regs.get_changed_addrs<uint8_t>();
        for (const auto addr : changed_addrs) {
            _poke16(addr, _regs.get_reg(addr));
        }
        _regs.save_state();
        UHD_LOG_TRACE(LOG_ID,
            "Storing cache complete: Updated " << changed_addrs.size() << " registers.");
    }

    void update_field(const std::string& field) override
    {
        const uint8_t addr     = _regs.get_addr(field);
        const uint8_t chip_val = _peek16(addr);
        _regs.set_reg(addr, chip_val);
    }

    void reset() override
    {
        UHD_LOG_TRACE(LOG_ID, "Resetting LTC5594...");
        _regs = ltc5594_regs_t();
        _regs.save_state();
        _regs.SRST = 1;
        commit();
        // Resets the SRST bit in the SW regmap (the actual register should reset
        // automatically during commit).
        _regs.SRST = 0;
        _regs.save_state();
    }

    void set_lo_matching(const double lo_freq) override
    {
        auto settings = _get_lo_matching_settings(lo_freq);
        _regs.BAND    = settings.band;
        _regs.CF1     = settings.cf1;
        _regs.LF1     = settings.lf1;
        _regs.CF2     = settings.cf2;
        UHD_LOG_TRACE(LOG_ID,
            "Setting LO matching for frequency "
                << lo_freq / 1e6 << " MHz: "
                << "BAND=" << settings.band << ", CF1=" << settings.cf1
                << ", LF1=" << settings.lf1 << ", CF2=" << settings.cf2);
        commit();
    }

    void enable_sdo_readback(const bool enable) override
    {
        UHD_LOG_TRACE(LOG_ID, (enable ? "Enabling" : "Disabling") << " SDO readback...");
        _regs.SDO_MODE = enable ? 1 : 0;
        commit();
    }

    void set_phase_error_adj(const double phase_error_adj) override
    {
        // The datasheet is not completely clear on how to set the phase error adjustment
        // value, so we are making some assumptions here based on the default value and
        // the resolution of the register.
        UHD_LOG_TRACE(LOG_ID,
            "Setting phase error adjustment to " << phase_error_adj << " degrees...");
        UHD_ASSERT_THROW(phase_error_adj >= MIN_PHASE_ADJ);
        UHD_ASSERT_THROW(phase_error_adj <= MAX_PHASE_ADJ);
        // Calculate step width based on default value 0 at 0x100
        const double step_width = -1 * MIN_PHASE_ADJ / 0x100;
        const uint16_t set_val  = (phase_error_adj - MIN_PHASE_ADJ) * step_width;
        _regs.PHA_L             = set_val & 0x1;
        _regs.PHA_H             = (set_val >> 1) & 0xff;
        commit();
    }

    void set_dc_offset_adj(const double dc_off_i_adj, const double dc_off_q_adj) override
    {
        UHD_LOG_TRACE(LOG_ID,
            "Setting the dc offset adjustment values to " << dc_off_i_adj << " and "
                                                          << dc_off_q_adj << " mV.");
        UHD_ASSERT_THROW(dc_off_i_adj >= MIN_DC_OFFSET_ADJ);
        UHD_ASSERT_THROW(dc_off_i_adj <= MAX_DC_OFFSET_ADJ);
        UHD_ASSERT_THROW(dc_off_q_adj >= MIN_DC_OFFSET_ADJ);
        UHD_ASSERT_THROW(dc_off_q_adj <= MAX_DC_OFFSET_ADJ);
        // Calculate step width based on default value 0 at 0x80
        const double step_width = -1 * MIN_DC_OFFSET_ADJ / 0x80;
        const uint8_t set_val_i = (dc_off_i_adj - MIN_DC_OFFSET_ADJ) / step_width;
        const uint8_t set_val_q = (dc_off_q_adj - MIN_DC_OFFSET_ADJ) / step_width;
        _regs.DCOI              = set_val_i;
        _regs.DCOQ              = set_val_q;
        commit();
    }

    void set_gain_err_adj(const double gain_err_adj) override
    {
        UHD_LOG_TRACE(LOG_ID,
            "Setting the gain error adjustment value to " << gain_err_adj << " dB.");
        UHD_ASSERT_THROW(gain_err_adj >= MIN_IQ_GAIN_ERR_ADJ);
        UHD_ASSERT_THROW(gain_err_adj <= MAX_IQ_GAIN_ERR_ADJ);
        // Calculate step width based on default value 0 at 0x20
        const double step_width = -1 * MIN_IQ_GAIN_ERR_ADJ / 0x20;
        const uint8_t set_val   = (gain_err_adj - MIN_IQ_GAIN_ERR_ADJ) / step_width;
        _regs.GERR              = set_val & 0x3f;
        commit();
    }

private:
    /*********************************************
     * Attributes
     *********************************************/
    write_fn_t _poke16;
    read_fn_t _peek16;
    ltc5594_regs_t _regs = ltc5594_regs_t();

    /*********************************************
     * Private Methods
     *********************************************/
    // Picks the LO matching settings for a given LO frequency
    ltc5594_lo_matching_t _get_lo_matching_settings(double lo_freq)
    {
        auto settings     = lo_matching_table.begin();
        auto settings_end = lo_matching_table.end();
        // Check if the frequency is within the range of the settings, else return the
        // default settings saved at min=0, max=0:
        if ((*settings).min_band_freq <= lo_freq
            && (*settings_end).max_band_freq >= lo_freq) {
            lo_freq = 0.0;
        }
        for (; settings != settings_end; ++settings) {
            if (settings->max_band_freq >= lo_freq) {
                return *settings;
            }
        }
        // If no matching setting is found, throw an exception
        throw std::runtime_error(
            "No matching LO settings found for the given frequency.");
    }
};

ltc5594_iface::sptr ltc5594_iface::make(
    ltc5594_iface::write_fn_t&& poke_fn, ltc5594_iface::read_fn_t&& peek_fn)
{
    return std::make_shared<ltc5594_impl>(std::move(poke_fn), std::move(peek_fn));
}

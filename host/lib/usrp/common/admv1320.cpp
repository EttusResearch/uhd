//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/ranges.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/common/admv1320.hpp>

namespace {
// LOG ID
constexpr char LOG_ID[] = "ADMV1320";
} // namespace
class admv1320_impl : public admv1320_iface
{
public:
    explicit admv1320_impl(
        write_fn_t&& poke_fn, read_fn_t&& peek_fn, const std::string& unique_id)
        : _poke16(std::move(poke_fn))
        , _peek16(std::move(peek_fn))
        , _regs()
        , _log_id(unique_id + "::" + LOG_ID)
    {
        _regs.save_state();
        // Create a clean state
        _regs.SOFT_RESET   = 1;
        _regs.SOFT_RESET_R = 1;
        commit();
        // Reset bits should be latching, so clear them in SW after commit
        _regs.SOFT_RESET   = 0;
        _regs.SOFT_RESET_R = 0;
        // Configure chip to
        _regs.SDO_ACTIVE   = 1;
        _regs.SDO_ACTIVE_R = 1;
        _regs.SDO_LEVEL    = 1; // 1.8V
        commit();
        const uint8_t random_value = static_cast<uint8_t>(time(NULL));
        _poke16(_regs.get_addr("SCRATCH_PAD"), random_value);
        UHD_ASSERT_THROW(_peek16(_regs.get_addr("SCRATCH_PAD")) == random_value);
        UHD_LOG_DEBUG(_log_id, "Initialized TX mixer ADMV1320.");
    }

    void commit() override
    {
        UHD_LOG_TRACE(_log_id, "Storing register cache to ADMV1320...");
        const auto changed_addrs = _regs.get_changed_addrs<size_t>();
        for (const auto addr : changed_addrs) {
            _poke16(addr, _regs.get_reg(addr));
        }
        _regs.save_state();
        UHD_LOG_TRACE(_log_id,
            "Storing cache complete: Updated " << changed_addrs.size() << " registers.");
    }

    void set_rf_band(const rf_band_t band) override
    {
        _regs.DIRECT_RF_BAND_SELECT = static_cast<uint8_t>(band);
    }

    void enable_gain_table_bypass(const bool enable) override
    {
        _regs.GAIN_TABLE_BYPASS_EN = enable ? 1 : 0;
    }

    uint8_t set_dsa(const dsa_t dsa, uint8_t value) override
    {
        // DSAs are in a range between 0 and 15:
        const uint8_t min  = 0;
        const uint8_t max  = 15;
        const uint8_t step = 1;
        auto range         = uhd::gain_range_t(min, max, step);
        value              = range.clip(value);
        switch (dsa) {
            case dsa_t::DSA1:
                _regs.DSA1_BYPASS_VALUE = value;
                break;
            case dsa_t::DSA2:
                _regs.DSA2_BYPASS_VALUE = value;
                break;
            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
        return value;
    }

    uint8_t get_dsa(const dsa_t dsa) override
    {
        switch (dsa) {
            case dsa_t::DSA1:
                return _regs.DSA1_BYPASS_VALUE;
            case dsa_t::DSA2:
                return _regs.DSA2_BYPASS_VALUE;
            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
    }

    void set_input_mode(const input_mode_t mode) override
    {
        switch (mode) {
            case input_mode_t::BYPASS:
                _regs.IF_EN        = 1;
                _regs.BB_EN        = 0;
                _regs.MIXER_BYPASS = 1;
                break;
            case input_mode_t::IF:
                _regs.IF_EN        = 1;
                _regs.BB_EN        = 0;
                _regs.MIXER_BYPASS = 0;
                break;
            case input_mode_t::BASEBAND:
                _regs.IF_EN        = 0;
                _regs.BB_EN        = 1;
                _regs.MIXER_BYPASS = 0;
                break;
            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
    }

    void set_rf_hs_powerdown(const bool enable) override
    {
        _regs.RF_HS_PD = enable;
    }

    void set_chip_powerdown(const bool enable) override
    {
        _regs.CHIP_PD = enable;
    }

    void set_lo_sideband(const lo_sideband_t sideband) override
    {
        _regs.DIRECT_LO_SIDEBAND = static_cast<uint8_t>(sideband);
    }

    void set_lo_x3_filter(const double freq) override
    {
        if (freq < 8e9) {
            return;
        } else if (freq < 9e9) {
            _regs.DIRECT_LO_X3_FILTER = 0x0;
        } else if (freq < 11e9) {
            _regs.DIRECT_LO_X3_FILTER = 0x1;
        } else if (freq < 13e9) {
            _regs.DIRECT_LO_X3_FILTER = 0x2;
        } else if (freq < 17e9) {
            _regs.DIRECT_LO_X3_FILTER = 0x3;
        } else if (freq < 23e9) {
            _regs.DIRECT_LO_X3_FILTER = 0x4;
        } else if (freq < 28e9) {
            _regs.DIRECT_LO_X3_FILTER = 0x5;
        }
    }

    void select_lpf_source(const filter_sel_t select) override
    {
        _regs.LPF_SELECT = static_cast<uint8_t>(select);
    }

    void set_lpf_cutoff_freq(const uint8_t value) override
    {
        UHD_ASSERT_THROW(value <= 15);
        _regs.LPF_VALUE = value;
    }

    void select_hpf_source(const filter_sel_t select) override
    {
        _regs.HPF_SELECT = static_cast<uint8_t>(select);
    }

    void set_hpf_cutoff_freq(const uint8_t value) override
    {
        UHD_ASSERT_THROW(value <= 63);
        _regs.HPF_VALUE = value;
    }

private:
    /*********************************************
     * Attributes
     *********************************************/
    write_fn_t _poke16;
    read_fn_t _peek16;
    admv1320_regs_t _regs = admv1320_regs_t();
    const std::string _log_id;
};

admv1320_iface::sptr admv1320_iface::make(admv1320_iface::write_fn_t&& poke_fn,
    admv1320_iface::read_fn_t&& peek_fn,
    const std::string& unique_id)
{
    return std::make_unique<admv1320_impl>(
        std::move(poke_fn), std::move(peek_fn), unique_id);
}

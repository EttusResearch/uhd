//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/common/admv1420.hpp>

namespace {
// LOG ID
constexpr char LOG_ID[] = "ADMV1420";
} // namespace
class admv1420_impl : public admv1420_iface
{
public:
    explicit admv1420_impl(write_fn_t&& poke_fn, read_fn_t&& peek_fn)
        : _poke16(std::move(poke_fn)), _peek16(std::move(peek_fn)), _regs()
    {
        _regs.save_state();
        // Create a clean state
        _regs.SOFTRESET   = 1;
        _regs.SOFTRESET_R = 1;
        commit();
        _regs.SOFTRESET   = 0;
        _regs.SOFTRESET_R = 0;
        _regs.save_state();
        const uint8_t random_value = static_cast<uint8_t>(time(NULL));
        _poke16(_regs.get_addr("SCRATCH_PAD"), random_value);
        UHD_ASSERT_THROW(_peek16(_regs.get_addr("SCRATCH_PAD")) == random_value);
        UHD_LOG_DEBUG(LOG_ID, "Initialized RX mixer ADMV1420.");
    }

    void commit() override
    {
        UHD_LOG_TRACE(LOG_ID, "Storing register cache to ADMV1420...");
        const auto changed_addrs = _regs.get_changed_addrs<size_t>();
        for (const auto addr : changed_addrs) {
            _poke16(addr, _regs.get_reg(addr));
        }
        _regs.save_state();
        UHD_LOG_TRACE(LOG_ID,
            "Storing cache complete: Updated " << changed_addrs.size() << " registers.");
    }

    void set_vcm(internal_voltage_t voltage) override
    {
        // Currently, for HBX we want to set the VCM to 1.25 V. From measurements we know
        // that this maps to the register value 0x11, so this is the only option for now.
        // FIXME: If the register should be listed in the official regmap later on, we
        // should use that.
        if (voltage == internal_voltage_t::V1_25) {
            _poke16(0x14F, 0x11);
        } else {
            // If someone adds a new value to the enum and uses it, complain that we
            // do not use it.
            UHD_THROW_INVALID_CODE_PATH();
        }
    }

    void finalize_init() override
    {
        // FIXME: These are unofficial registers that eventually may be set statically by
        // ADI but are necessary for now. Remove this if not required anymore.
        _poke16(0x1A0, 0x02);
        _poke16(0x1A3, 0x01);
        _poke16(0x1A5, 0x17);
    }

    void set_rf_band(const rf_band_t band) override
    {
        // No need to check for band < 0 as uint8_t doesn't allow for that.
        _regs.RF_BAND_SELECT = static_cast<uint8_t>(band);
    }

    void set_lo_sideband(const lo_sideband_t sideband) override
    {
        _regs.LO_SIDEBAND = static_cast<uint8_t>(sideband);
    }

    void set_if_band(const if_band_t if_band) override
    {
        _regs.IF_BAND_SELECT = static_cast<uint8_t>(if_band);
    }

    void set_lo_x3_filter(const double freq) override
    {
        if (freq >= 8e9 && freq <= 10e9) {
            _regs.DIRECT_LO_X3_FILTER = 13;
        } else if (freq <= 12e9) {
            _regs.DIRECT_LO_X3_FILTER = 12;
        } else if (freq <= 14e9) {
            _regs.DIRECT_LO_X3_FILTER = 3;
        } else if (freq <= 18e9) {
            _regs.DIRECT_LO_X3_FILTER = 9;
        } else if (freq <= 24e9) {
            _regs.DIRECT_LO_X3_FILTER = 8;
        } else if (freq <= 28e9) {
            _regs.DIRECT_LO_X3_FILTER = 0;
        }
    }

    void enable_filter_table(const bool enable) override
    {
        _regs.FILTER_TABLE_EN = enable;
    }

    void enable_gain_table(const bool enable) override
    {
        _regs.GAIN_TABLE_EN        = enable;
        _regs.GAIN_TABLE_BYPASS_EN = !enable;
    }

    uint8_t set_dsa(const dsa_t dsa, const uint8_t value) override
    {
        UHD_ASSERT_THROW(value <= 15);
        switch (dsa) {
            case dsa_t::DSA1:
                _regs.DSA1_BYPASS_VALUE = value;
                break;
            case dsa_t::DSA2:
                // DSA 2 can only toggle between 0 and 6 dB, so we'll round to what fits
                // better
                UHD_ASSERT_THROW(value <= 6);
                _regs.DSA2_BYPASS_VALUE = static_cast<uint8_t>(std::round(value / 6.0));
                return _regs.DSA2_BYPASS_VALUE * 6;
            case dsa_t::DSA3:
                _regs.DSA3_BYPASS_VALUE = value;
                break;
            case dsa_t::DSA4:
                _regs.DSA4_BYPASS_VALUE = value;
                break;
            case dsa_t::DSA5:
                _regs.DSA5_BYPASS_VALUE = value;
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
                return _regs.DSA2_BYPASS_VALUE * 6;
            case dsa_t::DSA3:
                return _regs.DSA3_BYPASS_VALUE;
            case dsa_t::DSA4:
                return _regs.DSA4_BYPASS_VALUE;
            case dsa_t::DSA5:
                return _regs.DSA5_BYPASS_VALUE;
            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
    }

    void set_if_bb_switch_ctrl(const bb_switch_t if_bb_switch_ctrl) override
    {
        _regs.IF_BB_SWITCH_CTRL = static_cast<uint16_t>(if_bb_switch_ctrl);
    }

    void set_powerdown(
        const std::vector<pd_comp_t> components, const bool enable) override
    {
        for (auto comp : components) {
            switch (comp) {
                case pd_comp_t::DSA1:
                    _regs.RF_DSA1_PD = enable;
                    break;
                case pd_comp_t::RF_LNA_B0:
                    _regs.RF_LNA_B0_PD = enable;
                    break;
                case pd_comp_t::RF_LNA_B1:
                    _regs.RF_LNA_B1_PD = enable;
                    break;
                case pd_comp_t::RF_LNA_B2:
                    _regs.RF_LNA_B2_PD = enable;
                    break;
                case pd_comp_t::BB_AMP_I:
                    _regs.BBI_PD = enable;
                    break;
                case pd_comp_t::BB_AMP_Q:
                    _regs.BBQ_PD = enable;
                    break;
                default:
                    UHD_THROW_INVALID_CODE_PATH();
            }
        }
    }

    void set_bb_amp_output_common_mode_int(const bool enabled) override
    {
        _regs.BB_VOCMSEL = enabled;
    }

    void set_mix_gate_bias_adj_mode(const mix_gate_bias_adj_mode_t mode) override
    {
        _regs.MIXER_GATE_SELECT = static_cast<uint16_t>(mode);
    }

    double set_mix_gate_bias_voltage(const double voltage) override
    {
        constexpr double step_width = 50; // mV
        const uint8_t reg_val = static_cast<uint16_t>(std::round(voltage / step_width));
        _regs.MIXER_GATE_DAC_MANUAL_CTRL = reg_val;
        return reg_val * step_width;
    }

    void set_sdo_level(const sdo_level_t setting) override
    {
        _regs.SDO_LEVEL = static_cast<uint8_t>(setting);
    }

    void select_lpf_source(const filter_sel_t select) override
    {
        _regs.RF_LPF_SELECT = static_cast<uint8_t>(select);
    }

    void set_lpf_cutoff_freq(const uint8_t value) override
    {
        UHD_ASSERT_THROW(value <= 15);
        _regs.RF_LPF_VALUE = value;
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
    admv1420_regs_t _regs = admv1420_regs_t();
};

admv1420_iface::sptr admv1420_iface::make(
    admv1420_iface::write_fn_t&& poke_fn, admv1420_iface::read_fn_t&& peek_fn)
{
    return std::make_unique<admv1420_impl>(std::move(poke_fn), std::move(peek_fn));
}

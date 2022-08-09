//
// Copyright 2015-2017 Ettus Research, A National Instruments Company
// Copyright 2019 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "twinrx_ctrl.hpp"
#include "twinrx_ids.hpp"
#include <uhd/utils/math.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhdlib/usrp/common/adf435x.hpp>
#include <uhdlib/usrp/common/adf535x.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <boost/format.hpp>
#include <chrono>
#include <cmath>
#include <thread>

using namespace uhd;
using namespace usrp;
using namespace dboard::twinrx;

namespace {
typedef twinrx_cpld_regmap rm;

typedef enum { LO1, LO2 } lo_t;

inline uint32_t bool2bin(bool x)
{
    return x ? 1 : 0;
}

const double TWINRX_REV_AB_PFD_FREQ = 6.25e6;
const double TWINRX_REV_C_PFD_FREQ  = 12.5e6;
const double TWINRX_SPI_CLOCK_FREQ  = 3e6;
const uint32_t TWINRX_LO1_MOD2      = 2;
} // namespace

class twinrx_ctrl_impl : public twinrx_ctrl
{
public:
    twinrx_ctrl_impl(dboard_iface::sptr db_iface,
        twinrx_gpio::sptr gpio_iface,
        twinrx_cpld_regmap::sptr cpld_regmap,
        const dboard_id_t rx_id)
        : _db_iface(db_iface), _gpio_iface(gpio_iface), _cpld_regs(cpld_regmap)
    {
        // SPI configuration
        _spi_config.use_custom_divider = true;
        _spi_config.divider            = uhd::narrow_cast<size_t>(std::ceil(
            _db_iface->get_codec_rate(dboard_iface::UNIT_TX) / TWINRX_SPI_CLOCK_FREQ));

        // Daughterboard clock rates must be a multiple of the pfd frequency
        if (rx_id == twinrx::TWINRX_REV_C_ID) {
            if (fmod(_db_iface->get_clock_rate(dboard_iface::UNIT_RX),
                    TWINRX_REV_C_PFD_FREQ)
                != 0) {
                throw uhd::value_error(
                    str(boost::format(
                            "TwinRX clock rate %f is not a multiple of the pfd freq %f.")
                        % _db_iface->get_clock_rate(dboard_iface::UNIT_RX)
                        % TWINRX_REV_C_PFD_FREQ));
            }
        } else {
            if (fmod(_db_iface->get_clock_rate(dboard_iface::UNIT_RX),
                    TWINRX_REV_AB_PFD_FREQ)
                != 0) {
                throw uhd::value_error(
                    str(boost::format(
                            "TwinRX clock rate %f is not a multiple of the pfd freq %f.")
                        % _db_iface->get_clock_rate(dboard_iface::UNIT_RX)
                        % TWINRX_REV_AB_PFD_FREQ));
            }
        }
        // Initialize dboard clocks
        _db_iface->set_clock_enabled(dboard_iface::UNIT_TX, true);
        _db_iface->set_clock_enabled(dboard_iface::UNIT_RX, true);

        // Initialize default switch and attenuator states
        set_chan_enabled(BOTH, false, false);
        set_preamp1(BOTH, PREAMP_BYPASS, false);
        set_preamp2(BOTH, false, false);
        set_lb_preamp_preselector(BOTH, false, false);
        set_signal_path(BOTH, PATH_LOWBAND, false);
        set_lb_preselector(BOTH, PRESEL_PATH3, false);
        set_hb_preselector(BOTH, PRESEL_PATH1, false);
        set_input_atten(BOTH, 31, false);
        set_lb_atten(BOTH, 31, false);
        set_hb_atten(BOTH, 31, false);
        set_lo1_source(BOTH, LO_INTERNAL, false);
        set_lo2_source(BOTH, LO_INTERNAL, false);
        set_lo1_export_source(LO_EXPORT_DISABLED, false);
        set_lo2_export_source(LO_EXPORT_DISABLED, false);
        set_antenna_mapping(ANTX_NATIVE, false);
        set_crossover_cal_mode(CAL_DISABLED, false);
        _cpld_regs->flush();

        // Turn on power and wait for power good
        _gpio_iface->set_field(twinrx_gpio::FIELD_SWPS_EN, 1);
        size_t timeout_ms = 100;
        while (_gpio_iface->get_field(twinrx_gpio::FIELD_SWPS_PWR_GOOD) == 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
            if (--timeout_ms == 0) {
                throw uhd::runtime_error("power supply failure");
            }
        }

        // Assert synthesizer chip enables
        _gpio_iface->set_field(twinrx_gpio::FIELD_LO1_CE_CH1, 1);
        _gpio_iface->set_field(twinrx_gpio::FIELD_LO1_CE_CH2, 1);
        _gpio_iface->set_field(twinrx_gpio::FIELD_LO2_CE_CH1, 1);
        _gpio_iface->set_field(twinrx_gpio::FIELD_LO2_CE_CH2, 1);

        // Initialize synthesizers
        for (size_t i = 0; i < NUM_CHANS; i++) {
            // LO1
            if (rx_id == twinrx::TWINRX_REV_C_ID) {
                _lo1_iface[i] = adf535x_iface::make_adf5356(
                    [this](const std::vector<uint32_t>& regs) {
                        _write_lo_spi(dboard_iface::UNIT_TX, regs);
                    },
                    [this](uint32_t microseconds) {
                        _db_iface->sleep(std::chrono::microseconds(microseconds));
                    });
                _lo1_pfd_freq = TWINRX_REV_C_PFD_FREQ;
            } else {
                _lo1_iface[i] = adf535x_iface::make_adf5355(
                    [this](const std::vector<uint32_t>& regs) {
                        _write_lo_spi(dboard_iface::UNIT_TX, regs);
                    },
                    [this](uint32_t microseconds) {
                        _db_iface->sleep(std::chrono::microseconds(microseconds));
                    });
                _lo1_pfd_freq = TWINRX_REV_AB_PFD_FREQ;
            }
            _lo1_iface[i]->set_pfd_freq(_lo1_pfd_freq);
            _lo1_iface[i]->set_output_power(adf535x_iface::OUTPUT_POWER_5DBM);
            _lo1_iface[i]->set_reference_freq(
                _db_iface->get_clock_rate(dboard_iface::UNIT_TX));
            _lo1_iface[i]->set_muxout_mode(adf535x_iface::MUXOUT_DLD);
            _lo1_iface[i]->set_frequency(3e9, TWINRX_LO1_MOD2);

            // LO2
            _lo2_iface[i] =
                adf435x_iface::make_adf4351([this](const std::vector<uint32_t>& regs) {
                    _write_lo_spi(dboard_iface::UNIT_RX, regs);
                });
            _lo2_iface[i]->set_feedback_select(adf435x_iface::FB_SEL_DIVIDED);
            _lo2_iface[i]->set_output_power(adf435x_iface::OUTPUT_POWER_5DBM);
            _lo2_iface[i]->set_reference_freq(
                _db_iface->get_clock_rate(dboard_iface::UNIT_RX));
            _lo2_iface[i]->set_muxout_mode(adf435x_iface::MUXOUT_DLD);
            _lo2_iface[i]->set_tuning_mode(adf435x_iface::TUNING_MODE_LOW_SPUR);
            _lo2_iface[i]->set_prescaler(adf435x_iface::PRESCALER_8_9);
        }
        commit();
    }

    ~twinrx_ctrl_impl() override
    {
        UHD_SAFE_CALL(std::lock_guard<std::mutex> lock(_mutex);
                      _gpio_iface->set_field(twinrx_gpio::FIELD_SWPS_EN, 0);)
    }

    void commit() override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _commit();
    }

    void set_chan_enabled(channel_t ch, bool enabled, bool commit = true) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (ch == CH1 or ch == BOTH) {
            _cpld_regs->if0_reg3.set(rm::if0_reg3_t::IF1_IF2_EN_CH1, bool2bin(enabled));
            _cpld_regs->if0_reg0.set(rm::if0_reg0_t::AMP_LO2_EN_CH1, bool2bin(enabled));
            _chan_enabled[size_t(CH1)] = enabled;
        }
        if (ch == CH2 or ch == BOTH) {
            _cpld_regs->rf1_reg5.set(rm::rf1_reg5_t::AMP_LO1_EN_CH2, bool2bin(enabled));
            _cpld_regs->if0_reg4.set(rm::if0_reg4_t::IF1_IF2_EN_CH2, bool2bin(enabled));
            _cpld_regs->if0_reg0.set(rm::if0_reg0_t::AMP_LO2_EN_CH2, bool2bin(enabled));
            _chan_enabled[size_t(CH2)] = enabled;
        }
        _set_lo1_amp(_chan_enabled[size_t(CH1)],
            _chan_enabled[size_t(CH2)],
            _lo1_src[size_t(CH2)]);
        if (commit)
            _commit();
    }

    void set_preamp1(channel_t ch, preamp_state_t value, bool commit = true) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (ch == CH1 or ch == BOTH) {
            _cpld_regs->rf0_reg1.set(
                rm::rf0_reg1_t::SWPA1_CTL_CH1, bool2bin(value == PREAMP_HIGHBAND));
            _cpld_regs->rf2_reg2.set(
                rm::rf2_reg2_t::SWPA2_CTRL_CH1, bool2bin(value == PREAMP_BYPASS));
            _cpld_regs->rf0_reg1.set(
                rm::rf0_reg1_t::HB_PREAMP_EN_CH1, bool2bin(value == PREAMP_HIGHBAND));
            _cpld_regs->rf0_reg1.set(
                rm::rf0_reg1_t::LB_PREAMP_EN_CH1, bool2bin(value == PREAMP_LOWBAND));
        }
        if (ch == CH2 or ch == BOTH) {
            _cpld_regs->rf0_reg7.set(
                rm::rf0_reg7_t::SWPA1_CTRL_CH2, bool2bin(value == PREAMP_HIGHBAND));
            _cpld_regs->rf2_reg5.set(
                rm::rf2_reg5_t::SWPA2_CTRL_CH2, bool2bin(value == PREAMP_BYPASS));
            _cpld_regs->rf0_reg5.set(
                rm::rf0_reg5_t::HB_PREAMP_EN_CH2, bool2bin(value == PREAMP_HIGHBAND));
            _cpld_regs->rf2_reg6.set(
                rm::rf2_reg6_t::LB_PREAMP_EN_CH2, bool2bin(value == PREAMP_LOWBAND));
        }
        if (commit)
            _commit();
    }

    void set_preamp2(channel_t ch, bool enabled, bool commit = true) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (ch == CH1 or ch == BOTH) {
            _cpld_regs->rf2_reg7.set(
                rm::rf2_reg7_t::SWPA4_CTRL_CH1, bool2bin(not enabled));
            _cpld_regs->rf2_reg3.set(rm::rf2_reg3_t::PREAMP2_EN_CH1, bool2bin(enabled));
        }
        if (ch == CH2 or ch == BOTH) {
            _cpld_regs->rf0_reg6.set(
                rm::rf0_reg6_t::SWPA4_CTRL_CH2, bool2bin(not enabled));
            _cpld_regs->rf1_reg6.set(rm::rf1_reg6_t::PREAMP2_EN_CH2, bool2bin(enabled));
        }
        if (commit)
            _commit();
    }

    void set_lb_preamp_preselector(
        channel_t ch, bool enabled, bool commit = true) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (ch == CH1 or ch == BOTH) {
            _cpld_regs->rf0_reg7.set(
                rm::rf0_reg7_t::SWPA3_CTRL_CH1, bool2bin(not enabled));
        }
        if (ch == CH2 or ch == BOTH) {
            _cpld_regs->rf0_reg1.set(
                rm::rf0_reg1_t::SWPA3_CTRL_CH2, bool2bin(not enabled));
        }
        if (commit)
            _commit();
    }

    void set_signal_path(channel_t ch, signal_path_t path, bool commit = true) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (ch == CH1 or ch == BOTH) {
            _cpld_regs->rf2_reg2.set(
                rm::rf2_reg2_t::SW11_CTRL_CH1, bool2bin(path == PATH_LOWBAND));
            _cpld_regs->rf1_reg2.set(
                rm::rf1_reg2_t::SW12_CTRL_CH1, bool2bin(path == PATH_LOWBAND));
            _cpld_regs->rf1_reg6.set(
                rm::rf1_reg6_t::HB_PRESEL_PGA_EN_CH1, bool2bin(path == PATH_HIGHBAND));
            _cpld_regs->rf0_reg2.set(
                rm::rf0_reg2_t::SW6_CTRL_CH1, bool2bin(path == PATH_LOWBAND));
            _cpld_regs->if0_reg3.set(
                rm::if0_reg3_t::SW13_CTRL_CH1, bool2bin(path == PATH_LOWBAND));
            _cpld_regs->if0_reg2.set(
                rm::if0_reg2_t::AMP_LB_IF1_EN_CH1, bool2bin(path == PATH_LOWBAND));
            _cpld_regs->if0_reg0.set(
                rm::if0_reg0_t::AMP_HB_IF1_EN_CH1, bool2bin(path == PATH_HIGHBAND));
            _cpld_regs->rf1_reg2.set(
                rm::rf1_reg2_t::AMP_HB_EN_CH1, bool2bin(path == PATH_HIGHBAND));
            _cpld_regs->rf2_reg2.set(
                rm::rf2_reg2_t::AMP_LB_EN_CH1, bool2bin(path == PATH_LOWBAND));
        }
        if (ch == CH2 or ch == BOTH) {
            _cpld_regs->rf2_reg7.set(
                rm::rf2_reg7_t::SW11_CTRL_CH2, bool2bin(path == PATH_LOWBAND));
            _cpld_regs->rf1_reg7.set(
                rm::rf1_reg7_t::SW12_CTRL_CH2, bool2bin(path == PATH_LOWBAND));
            _cpld_regs->rf1_reg2.set(
                rm::rf1_reg2_t::HB_PRESEL_PGA_EN_CH2, bool2bin(path == PATH_HIGHBAND));
            _cpld_regs->rf0_reg6.set(
                rm::rf0_reg6_t::SW6_CTRL_CH2, bool2bin(path == PATH_HIGHBAND));
            _cpld_regs->if0_reg6.set(
                rm::if0_reg6_t::SW13_CTRL_CH2, bool2bin(path == PATH_HIGHBAND));
            _cpld_regs->if0_reg2.set(
                rm::if0_reg2_t::AMP_LB_IF1_EN_CH2, bool2bin(path == PATH_LOWBAND));
            _cpld_regs->if0_reg6.set(
                rm::if0_reg6_t::AMP_HB_IF1_EN_CH2, bool2bin(path == PATH_HIGHBAND));
            _cpld_regs->rf1_reg7.set(
                rm::rf1_reg7_t::AMP_HB_EN_CH2, bool2bin(path == PATH_HIGHBAND));
            _cpld_regs->rf2_reg7.set(
                rm::rf2_reg7_t::AMP_LB_EN_CH2, bool2bin(path == PATH_LOWBAND));
        }
        if (commit)
            _commit();
    }

    void set_lb_preselector(
        channel_t ch, preselector_path_t path, bool commit = true) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        uint32_t sw7val = 0, sw8val = 0;
        switch (path) {
            case PRESEL_PATH1:
                sw7val = 3;
                sw8val = 1;
                break;
            case PRESEL_PATH2:
                sw7val = 2;
                sw8val = 0;
                break;
            case PRESEL_PATH3:
                sw7val = 0;
                sw8val = 2;
                break;
            case PRESEL_PATH4:
                sw7val = 1;
                sw8val = 3;
                break;
            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
        if (ch == CH1 or ch == BOTH) {
            _cpld_regs->rf0_reg3.set(rm::rf0_reg3_t::SW7_CTRL_CH1, sw7val);
            _cpld_regs->rf2_reg3.set(rm::rf2_reg3_t::SW8_CTRL_CH1, sw8val);
        }
        if (ch == CH2 or ch == BOTH) {
            _cpld_regs->rf0_reg7.set(rm::rf0_reg7_t::SW7_CTRL_CH2, sw7val);
            _cpld_regs->rf2_reg7.set(rm::rf2_reg7_t::SW8_CTRL_CH2, sw8val);
        }
        if (commit)
            _commit();
    }

    void set_hb_preselector(
        channel_t ch, preselector_path_t path, bool commit = true) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        uint32_t sw9ch1val = 0, sw10ch1val = 0, sw9ch2val = 0, sw10ch2val = 0;
        switch (path) {
            case PRESEL_PATH1:
                sw9ch1val  = 3;
                sw10ch1val = 0;
                sw9ch2val  = 0;
                sw10ch2val = 3;
                break;
            case PRESEL_PATH2:
                sw9ch1val  = 1;
                sw10ch1val = 2;
                sw9ch2val  = 1;
                sw10ch2val = 1;
                break;
            case PRESEL_PATH3:
                sw9ch1val  = 2;
                sw10ch1val = 1;
                sw9ch2val  = 2;
                sw10ch2val = 2;
                break;
            case PRESEL_PATH4:
                sw9ch1val  = 0;
                sw10ch1val = 3;
                sw9ch2val  = 3;
                sw10ch2val = 0;
                break;
            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
        if (ch == CH1 or ch == BOTH) {
            _cpld_regs->rf0_reg5.set(rm::rf0_reg5_t::SW9_CTRL_CH1, sw9ch1val);
            _cpld_regs->rf1_reg3.set(rm::rf1_reg3_t::SW10_CTRL_CH1, sw10ch1val);
        }
        if (ch == CH2 or ch == BOTH) {
            _cpld_regs->rf0_reg3.set(rm::rf0_reg3_t::SW9_CTRL_CH2, sw9ch2val);
            _cpld_regs->rf1_reg7.set(rm::rf1_reg7_t::SW10_CTRL_CH2, sw10ch2val);
        }
        if (commit)
            _commit();
    }

    void set_input_atten(channel_t ch, uint8_t atten, bool commit = true) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (ch == CH1 or ch == BOTH) {
            _cpld_regs->rf0_reg0.set(rm::rf0_reg0_t::ATTEN_IN_CH1, atten & 0x1F);
        }
        if (ch == CH2 or ch == BOTH) {
            _cpld_regs->rf0_reg4.set(rm::rf0_reg4_t::ATTEN_IN_CH2, atten & 0x1F);
        }
        if (commit)
            _commit();
    }

    void set_lb_atten(channel_t ch, uint8_t atten, bool commit = true) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (ch == CH1 or ch == BOTH) {
            _cpld_regs->rf2_reg0.set(rm::rf2_reg0_t::ATTEN_LB_CH1, atten & 0x1F);
        }
        if (ch == CH2 or ch == BOTH) {
            _cpld_regs->rf2_reg4.set(rm::rf2_reg4_t::ATTEN_LB_CH2, atten & 0x1F);
        }
        if (commit)
            _commit();
    }

    void set_hb_atten(channel_t ch, uint8_t atten, bool commit = true) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (ch == CH1 or ch == BOTH) {
            _cpld_regs->rf1_reg0.set(rm::rf1_reg0_t::ATTEN_HB_CH1, atten & 0x1F);
        }
        if (ch == CH2 or ch == BOTH) {
            _cpld_regs->rf1_reg4.set(rm::rf1_reg4_t::ATTEN_HB_CH2, atten & 0x1F);
        }
        if (commit)
            _commit();
    }

    void set_lo1_source(channel_t ch, lo_source_t source, bool commit = true) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (ch == CH1 or ch == BOTH) {
            _cpld_regs->rf1_reg5.set(
                rm::rf1_reg5_t::SW14_CTRL_CH2, bool2bin(source != LO_COMPANION));
            _cpld_regs->rf1_reg1.set(rm::rf1_reg1_t::SW15_CTRL_CH1,
                bool2bin(source == LO_EXTERNAL || source == LO_REIMPORT));
            _cpld_regs->rf1_reg1.set(
                rm::rf1_reg1_t::SW16_CTRL_CH1, bool2bin(source != LO_INTERNAL));
            _lo1_src[size_t(CH1)] = source;
        }
        if (ch == CH2 or ch == BOTH) {
            _cpld_regs->rf1_reg1.set(
                rm::rf1_reg1_t::SW14_CTRL_CH1, bool2bin(source == LO_COMPANION));
            _cpld_regs->rf1_reg5.set(
                rm::rf1_reg5_t::SW15_CTRL_CH2, bool2bin(source != LO_INTERNAL));
            _cpld_regs->rf1_reg6.set(
                rm::rf1_reg6_t::SW16_CTRL_CH2, bool2bin(source == LO_INTERNAL));
            _lo1_src[size_t(CH2)] = source;
            _set_lo1_amp(_chan_enabled[size_t(CH1)],
                _chan_enabled[size_t(CH2)],
                _lo1_src[size_t(CH2)]);
        }
        if (commit)
            _commit();
    }

    void set_lo2_source(channel_t ch, lo_source_t source, bool commit = true) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (ch == CH1 or ch == BOTH) {
            _cpld_regs->if0_reg0.set(
                rm::if0_reg0_t::SW19_CTRL_CH2, bool2bin(source == LO_COMPANION));
            _cpld_regs->if0_reg1.set(
                rm::if0_reg1_t::SW20_CTRL_CH1, bool2bin(source == LO_COMPANION));
            _cpld_regs->if0_reg4.set(
                rm::if0_reg4_t::SW21_CTRL_CH1, bool2bin(source == LO_INTERNAL));
            _lo2_src[size_t(CH1)] = source;
        }
        if (ch == CH2 or ch == BOTH) {
            _cpld_regs->if0_reg4.set(rm::if0_reg4_t::SW19_CTRL_CH1,
                bool2bin(source == LO_EXTERNAL || source == LO_REIMPORT));
            _cpld_regs->if0_reg0.set(rm::if0_reg0_t::SW20_CTRL_CH2,
                bool2bin(source == LO_INTERNAL || source == LO_DISABLED));
            _cpld_regs->if0_reg4.set(
                rm::if0_reg4_t::SW21_CTRL_CH2, bool2bin(source == LO_INTERNAL));
            _lo2_src[size_t(CH2)] = source;
        }
        if (commit)
            _commit();
    }

    void set_lo1_export_source(lo_export_source_t source, bool commit = true) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        // SW22 may conflict with the cal switch but this attr takes priority and we
        // assume that the cal switch is disabled (by disabling it!)
        _set_cal_mode(CAL_DISABLED, source);
        _cpld_regs->rf1_reg3.set(
            rm::rf1_reg3_t::SW23_CTRL, bool2bin(source != LO_CH1_SYNTH));
        _lo1_export = source;

        if (commit)
            _commit();
    }

    void set_lo2_export_source(lo_export_source_t source, bool commit = true) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _cpld_regs->if0_reg7.set(
            rm::if0_reg7_t::SW24_CTRL_CH2, bool2bin(source == LO_CH2_SYNTH));
        _cpld_regs->if0_reg4.set(
            rm::if0_reg4_t::SW25_CTRL, bool2bin(source != LO_CH1_SYNTH));
        _cpld_regs->if0_reg3.set(
            rm::if0_reg3_t::SW24_CTRL_CH1, bool2bin(source != LO_CH1_SYNTH));
        _lo2_export = source;

        if (commit)
            _commit();
    }

    void set_antenna_mapping(antenna_mapping_t mapping, bool commit = true) override
    {
        std::lock_guard<std::mutex> lock(_mutex);

        enum switch_path_t { CONNECT, TERM, EXPORT, IMPORT, SWAP };
        switch_path_t path1, path2;

        switch (mapping) {
            case ANTX_NATIVE:
                path1 = CONNECT;
                path2 = CONNECT;
                break;
            case ANT1_SHARED:
                path1 = EXPORT;
                path2 = IMPORT;
                break;
            case ANT2_SHARED:
                path1 = IMPORT;
                path2 = EXPORT;
                break;
            case ANTX_SWAPPED:
                path1 = SWAP;
                path2 = SWAP;
                break;
            default:
                path1 = TERM;
                path2 = TERM;
                break;
        }

        _cpld_regs->rf0_reg5.set(
            rm::rf0_reg5_t::SW3_CTRL_CH1, bool2bin(path1 == EXPORT || path1 == SWAP));
        _cpld_regs->rf0_reg2.set(
            rm::rf0_reg2_t::SW4_CTRL_CH1, bool2bin(!(path1 == IMPORT || path1 == SWAP)));
        _cpld_regs->rf0_reg2.set(
            rm::rf0_reg2_t::SW5_CTRL_CH1, bool2bin(path1 == CONNECT));
        _cpld_regs->rf0_reg7.set(
            rm::rf0_reg7_t::SW3_CTRL_CH2, bool2bin(path2 == EXPORT || path2 == SWAP));
        _cpld_regs->rf0_reg6.set(
            rm::rf0_reg6_t::SW4_CTRL_CH2, bool2bin(path2 == IMPORT || path2 == SWAP));
        _cpld_regs->rf0_reg6.set(
            rm::rf0_reg6_t::SW5_CTRL_CH2, bool2bin(path2 == CONNECT));

        if (commit)
            _commit();
    }

    void set_crossover_cal_mode(cal_mode_t cal_mode, bool commit = true) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_lo1_export == LO_CH1_SYNTH && cal_mode == CAL_CH2) {
            throw uhd::runtime_error(
                "cannot enable cal crossover on CH2 when LO1 in CH1 is exported");
        }
        if (_lo1_export == LO_CH2_SYNTH && cal_mode == CAL_CH1) {
            throw uhd::runtime_error(
                "cannot enable cal crossover on CH1 when LO1 in CH2 is exported");
        }
        _set_cal_mode(cal_mode, _lo1_export);

        if (commit)
            _commit();
    }

    double set_lo1_synth_freq(channel_t ch, double freq, bool commit = true) override
    {
        std::lock_guard<std::mutex> lock(_mutex);

        double coerced_freq = 0.0;
        if (ch == CH1 or ch == BOTH) {
            coerced_freq =
                _lo1_iface[size_t(CH1)]->set_frequency(freq, TWINRX_LO1_MOD2, false);
            _lo1_freq[size_t(CH1)] = tune_freq_t(freq);
        }
        if (ch == CH2 or ch == BOTH) {
            coerced_freq =
                _lo1_iface[size_t(CH2)]->set_frequency(freq, TWINRX_LO1_MOD2, false);
            _lo1_freq[size_t(CH2)] = tune_freq_t(freq);
        }

        if (commit)
            _commit();
        return coerced_freq;
    }

    double set_lo2_synth_freq(channel_t ch, double freq, bool commit = true) override
    {
        std::lock_guard<std::mutex> lock(_mutex);

        double coerced_freq = 0.0;
        if (ch == CH1 or ch == BOTH) {
            coerced_freq = _lo2_iface[size_t(CH1)]->set_frequency(freq, false, false);
            _lo2_freq[size_t(CH1)] = tune_freq_t(freq);
        }
        if (ch == CH2 or ch == BOTH) {
            coerced_freq = _lo2_iface[size_t(CH2)]->set_frequency(freq, false, false);
            _lo2_freq[size_t(CH2)] = tune_freq_t(freq);
        }

        if (commit)
            _commit();
        return coerced_freq;
    }

    double set_lo1_charge_pump(channel_t ch, double current, bool commit = true) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        double coerced_current = 0.0;
        if (ch == CH1 or ch == BOTH) {
            coerced_current =
                _lo1_iface[size_t(CH1)]->set_charge_pump_current(current, false);
        }
        if (ch == CH2 or ch == BOTH) {
            coerced_current =
                _lo1_iface[size_t(CH2)]->set_charge_pump_current(current, false);
        }

        if (commit) {
            _commit();
        }
        return coerced_current;
    }

    double set_lo2_charge_pump(channel_t ch, double current, bool commit = true) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        double coerced_current = 0.0;
        if (ch == CH1 or ch == BOTH) {
            coerced_current =
                _lo2_iface[size_t(CH1)]->set_charge_pump_current(current, false);
        }
        if (ch == CH2 or ch == BOTH) {
            coerced_current =
                _lo2_iface[size_t(CH2)]->set_charge_pump_current(current, false);
        }

        if (commit) {
            _commit();
        }
        return coerced_current;
    }

    uhd::meta_range_t get_lo1_charge_pump_range() override
    {
        // assume that both channels have the same range
        return _lo1_iface[size_t(CH1)]->get_charge_pump_current_range();
    }

    uhd::meta_range_t get_lo2_charge_pump_range() override
    {
        // assume that both channels have the same range
        return _lo2_iface[size_t(CH1)]->get_charge_pump_current_range();
    }

    bool read_lo1_locked(channel_t ch) override
    {
        std::lock_guard<std::mutex> lock(_mutex);

        bool locked = true;
        if (ch == CH1 or ch == BOTH) {
            locked = locked
                     && (_gpio_iface->get_field(twinrx_gpio::FIELD_LO1_MUXOUT_CH1) == 1);
        }
        if (ch == CH2 or ch == BOTH) {
            locked = locked
                     && (_gpio_iface->get_field(twinrx_gpio::FIELD_LO1_MUXOUT_CH2) == 1);
        }
        return locked;
    }

    bool read_lo2_locked(channel_t ch) override
    {
        std::lock_guard<std::mutex> lock(_mutex);

        bool locked = true;
        if (ch == CH1 or ch == BOTH) {
            locked = locked
                     && (_gpio_iface->get_field(twinrx_gpio::FIELD_LO2_MUXOUT_CH1) == 1);
        }
        if (ch == CH2 or ch == BOTH) {
            locked = locked
                     && (_gpio_iface->get_field(twinrx_gpio::FIELD_LO2_MUXOUT_CH2) == 1);
        }
        return locked;
    }

private: // Functions
    void _set_cal_mode(cal_mode_t cal_mode, lo_export_source_t lo1_export_src)
    {
        _cpld_regs->rf1_reg1.set(
            rm::rf1_reg1_t::SW17_CTRL_CH1, bool2bin(cal_mode != CAL_CH1));
        _cpld_regs->rf1_reg6.set(
            rm::rf1_reg6_t::SW17_CTRL_CH2, bool2bin(cal_mode != CAL_CH2));
        _cpld_regs->rf1_reg5.set(
            rm::rf1_reg5_t::SW18_CTRL_CH1, bool2bin(cal_mode != CAL_CH1));
        _cpld_regs->rf2_reg3.set(
            rm::rf2_reg3_t::SW18_CTRL_CH2, bool2bin(cal_mode != CAL_CH2));
        _cpld_regs->rf1_reg3.set(rm::rf1_reg3_t::SW22_CTRL_CH1,
            bool2bin((lo1_export_src != LO_CH1_SYNTH) || (cal_mode == CAL_CH1)));
        _cpld_regs->rf1_reg7.set(rm::rf1_reg7_t::SW22_CTRL_CH2,
            bool2bin((lo1_export_src != LO_CH2_SYNTH) || (cal_mode == CAL_CH2)));
    }

    void _set_lo1_amp(bool ch1_enabled, bool ch2_enabled, lo_source_t ch2_lo1_src)
    {
        // AMP_LO1_EN_CH1 also controls the amp for the external LO1 port,
        // which could be in use by ch2
        _cpld_regs->rf1_reg1.set(rm::rf1_reg1_t::AMP_LO1_EN_CH1,
            bool2bin(
                ch1_enabled
                || (ch2_enabled
                       && (ch2_lo1_src == LO_EXTERNAL || ch2_lo1_src == LO_REIMPORT))));
    }

    void _config_lo_route(lo_t lo, channel_t channel)
    {
        // Route SPI LEs through CPLD (will not assert them)
        _cpld_regs->rf0_reg2.set(rm::rf0_reg2_t::LO1_LE_CH1,
            bool2bin(lo == LO1 and (channel == CH1 or channel == BOTH)));
        _cpld_regs->rf0_reg2.set(rm::rf0_reg2_t::LO1_LE_CH2,
            bool2bin(lo == LO1 and (channel == CH2 or channel == BOTH)));
        _cpld_regs->rf0_reg2.flush();
        _cpld_regs->if0_reg2.set(rm::if0_reg2_t::LO2_LE_CH1,
            bool2bin(lo == LO2 and (channel == CH1 or channel == BOTH)));
        _cpld_regs->if0_reg2.set(rm::if0_reg2_t::LO2_LE_CH2,
            bool2bin(lo == LO2 and (channel == CH2 or channel == BOTH)));
        _cpld_regs->if0_reg2.flush();
    }

    void _write_lo_spi(dboard_iface::unit_t unit, const std::vector<uint32_t>& regs)
    {
        for (uint32_t reg : regs) {
            _db_iface->write_spi(unit, _spi_config, reg, 32);
        }
    }

    void _commit()
    {
        // Commit everything except the LO synthesizers
        _cpld_regs->flush();

        // Disable unused LO synthesizers
        _lo1_enable[size_t(CH1)] = _lo1_src[size_t(CH1)] == LO_INTERNAL
                                   || _lo1_src[size_t(CH2)] == LO_COMPANION
                                   || _lo1_export == LO_CH1_SYNTH;

        _lo1_enable[size_t(CH2)] = _lo1_src[size_t(CH2)] == LO_INTERNAL
                                   || _lo1_src[size_t(CH1)] == LO_COMPANION
                                   || _lo1_export == LO_CH2_SYNTH;
        _lo2_enable[size_t(CH1)] = _lo2_src[size_t(CH1)] == LO_INTERNAL
                                   || _lo2_src[size_t(CH2)] == LO_COMPANION
                                   || _lo2_export == LO_CH1_SYNTH;

        _lo2_enable[size_t(CH2)] = _lo2_src[size_t(CH2)] == LO_INTERNAL
                                   || _lo2_src[size_t(CH1)] == LO_COMPANION
                                   || _lo2_export == LO_CH2_SYNTH;

        _lo1_iface[size_t(CH1)]->set_output_enable(
            adf535x_iface::RF_OUTPUT_A, _lo1_enable[size_t(CH1)].get());
        _lo1_iface[size_t(CH2)]->set_output_enable(
            adf535x_iface::RF_OUTPUT_A, _lo1_enable[size_t(CH2)].get());

        _lo2_iface[size_t(CH1)]->set_output_enable(
            adf435x_iface::RF_OUTPUT_A, _lo2_enable[size_t(CH1)].get());
        _lo2_iface[size_t(CH2)]->set_output_enable(
            adf435x_iface::RF_OUTPUT_A, _lo2_enable[size_t(CH2)].get());

        // Commit LO1 frequency
        // Commit Channel 1's settings to both channels simultaneously if the frequency is
        // the same.
        bool simultaneous_commit_lo1 =
            _lo1_freq[size_t(CH1)].is_dirty() and _lo1_freq[size_t(CH2)].is_dirty()
            and _lo1_freq[size_t(CH1)].get() == _lo1_freq[size_t(CH2)].get()
            and _lo1_enable[size_t(CH1)].get() == _lo1_enable[size_t(CH2)].get();

        if (simultaneous_commit_lo1) {
            _config_lo_route(LO1, BOTH);
            // Only commit one of the channels. The route LO_CONFIG_BOTH
            // will ensure that the LEs for both channels are enabled
            _lo1_iface[size_t(CH1)]->commit();
            _lo1_freq[size_t(CH1)].mark_clean();
            _lo1_freq[size_t(CH2)].mark_clean();
            _lo1_enable[size_t(CH1)].mark_clean();
            _lo1_enable[size_t(CH2)].mark_clean();
        } else {
            if (_lo1_freq[size_t(CH1)].is_dirty()
                || _lo1_enable[size_t(CH1)].is_dirty()) {
                _config_lo_route(LO1, CH1);
                _lo1_iface[size_t(CH1)]->commit();
                _lo1_freq[size_t(CH1)].mark_clean();
                _lo1_enable[size_t(CH1)].mark_clean();
            }
            if (_lo1_freq[size_t(CH2)].is_dirty()
                || _lo1_enable[size_t(CH2)].is_dirty()) {
                _config_lo_route(LO1, CH2);
                _lo1_iface[size_t(CH2)]->commit();
                _lo1_freq[size_t(CH2)].mark_clean();
                _lo1_enable[size_t(CH2)].mark_clean();
            }
        }

        // Commit LO2 frequency
        bool simultaneous_commit_lo2 =
            _lo2_freq[size_t(CH1)].is_dirty() and _lo2_freq[size_t(CH2)].is_dirty()
            and _lo2_freq[size_t(CH1)].get() == _lo2_freq[size_t(CH2)].get()
            and _lo2_enable[size_t(CH1)].get() == _lo2_enable[size_t(CH2)].get();

        if (simultaneous_commit_lo2) {
            _config_lo_route(LO2, BOTH);
            // Only commit one of the channels. The route LO_CONFIG_BOTH
            // will ensure that the LEs for both channels are enabled
            _lo2_iface[size_t(CH1)]->commit();
            _lo2_freq[size_t(CH1)].mark_clean();
            _lo2_freq[size_t(CH2)].mark_clean();
            _lo2_enable[size_t(CH1)].mark_clean();
            _lo2_enable[size_t(CH2)].mark_clean();
        } else {
            if (_lo2_freq[size_t(CH1)].is_dirty()
                || _lo2_enable[size_t(CH1)].is_dirty()) {
                _config_lo_route(LO2, CH1);
                _lo2_iface[size_t(CH1)]->commit();
                _lo2_freq[size_t(CH1)].mark_clean();
                _lo2_enable[size_t(CH1)].mark_clean();
            }
            if (_lo2_freq[size_t(CH2)].is_dirty()
                || _lo2_enable[size_t(CH2)].is_dirty()) {
                _config_lo_route(LO2, CH2);
                _lo2_iface[size_t(CH2)]->commit();
                _lo2_freq[size_t(CH2)].mark_clean();
                _lo2_enable[size_t(CH2)].mark_clean();
            }
        }
    }

private: // Members
    static const size_t NUM_CHANS = 2;

    struct tune_freq_t : public uhd::math::fp_compare::fp_compare_delta<double>
    {
        tune_freq_t()
            : uhd::math::fp_compare::fp_compare_delta<double>(
                  0.0, uhd::math::FREQ_COMPARISON_DELTA_HZ)
        {
        }

        tune_freq_t(double freq)
            : uhd::math::fp_compare::fp_compare_delta<double>(
                  freq, uhd::math::FREQ_COMPARISON_DELTA_HZ)
        {
        }
    };

    std::mutex _mutex;
    dboard_iface::sptr _db_iface;
    twinrx_gpio::sptr _gpio_iface;
    twinrx_cpld_regmap::sptr _cpld_regs;
    spi_config_t _spi_config;
    double _lo1_pfd_freq;
    adf535x_iface::sptr _lo1_iface[NUM_CHANS];
    adf435x_iface::sptr _lo2_iface[NUM_CHANS];
    lo_source_t _lo1_src[NUM_CHANS];
    lo_source_t _lo2_src[NUM_CHANS];
    dirty_tracked<tune_freq_t> _lo1_freq[NUM_CHANS];
    dirty_tracked<tune_freq_t> _lo2_freq[NUM_CHANS];
    dirty_tracked<bool> _lo1_enable[NUM_CHANS];
    dirty_tracked<bool> _lo2_enable[NUM_CHANS];
    lo_export_source_t _lo1_export;
    lo_export_source_t _lo2_export;
    bool _chan_enabled[NUM_CHANS];
};

twinrx_ctrl::sptr twinrx_ctrl::make(dboard_iface::sptr db_iface,
    twinrx_gpio::sptr gpio_iface,
    twinrx_cpld_regmap::sptr cpld_regmap,
    const dboard_id_t rx_id)
{
    return std::make_shared<twinrx_ctrl_impl>(db_iface, gpio_iface, cpld_regmap, rx_id);
}

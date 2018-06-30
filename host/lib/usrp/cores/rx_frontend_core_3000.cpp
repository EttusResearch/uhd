//
// Copyright 2011-2012,2014-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/dict.hpp>
#include <uhdlib/usrp/cores/rx_frontend_core_3000.hpp>
#include <uhdlib/usrp/cores/dsp_core_utils.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/bind.hpp>

using namespace uhd;

#define REG_RX_FE_MAG_CORRECTION    (_base + 0 ) //18 bits
#define REG_RX_FE_PHASE_CORRECTION  (_base + 4 ) //18 bits
#define REG_RX_FE_OFFSET_I          (_base + 8 ) //18 bits
#define REG_RX_FE_OFFSET_Q          (_base + 12) //18 bits
#define REG_RX_FE_MAPPING           (_base + 16)
#define REG_RX_FE_HET_CORDIC_PHASE  (_base + 20)

#define FLAG_DSP_RX_MAPPING_SWAP_IQ     (1 << 0)
#define FLAG_DSP_RX_MAPPING_REAL_MODE   (1 << 1)
#define FLAG_DSP_RX_MAPPING_INVERT_Q    (1 << 2)
#define FLAG_DSP_RX_MAPPING_INVERT_I    (1 << 3)
#define FLAG_DSP_RX_MAPPING_REAL_DECIM  (1 << 4)
//#define FLAG_DSP_RX_MAPPING_RESERVED    (1 << 5)
//#define FLAG_DSP_RX_MAPPING_RESERVED    (1 << 6)
#define FLAG_DSP_RX_MAPPING_BYPASS_ALL  (1 << 7)

#define OFFSET_FIXED (1ul << 31)
#define OFFSET_SET   (1ul << 30)
#define FLAG_MASK (OFFSET_FIXED | OFFSET_SET)

using namespace uhd::usrp;

static uint32_t fs_to_bits(const double num, const size_t bits){
    return int32_t(boost::math::round(num * (1 << (bits-1))));
}

rx_frontend_core_3000::~rx_frontend_core_3000(void){
    /* NOP */
}

const std::complex<double> rx_frontend_core_3000::DEFAULT_DC_OFFSET_VALUE = std::complex<double>(0.0, 0.0);
const bool rx_frontend_core_3000::DEFAULT_DC_OFFSET_ENABLE = true;
const std::complex<double> rx_frontend_core_3000::DEFAULT_IQ_BALANCE_VALUE = std::complex<double>(0.0, 0.0);

class rx_frontend_core_3000_impl : public rx_frontend_core_3000{
public:
    rx_frontend_core_3000_impl(wb_iface::sptr iface, const size_t base):
        _i_dc_off(0), _q_dc_off(0),
        _adc_rate(0.0),
        _fe_conn(fe_connection_t("IQ")),
        _iface(iface), _base(base)
    {
        //NOP
    }

    void set_adc_rate(const double rate) {
        _adc_rate = rate;
    }

    void bypass_all(bool bypass_en) {
        if (bypass_en) {
            _iface->poke32(REG_RX_FE_MAPPING, FLAG_DSP_RX_MAPPING_BYPASS_ALL);
        } else {
            set_fe_connection(_fe_conn);
        }
    }

    void set_fe_connection(const fe_connection_t& fe_conn) {
        uint32_t mapping_reg_val = 0;
        switch (fe_conn.get_sampling_mode()) {
        case fe_connection_t::REAL:
        case fe_connection_t::HETERODYNE:
            mapping_reg_val = FLAG_DSP_RX_MAPPING_REAL_MODE|FLAG_DSP_RX_MAPPING_REAL_DECIM;
            break;
        default:
            mapping_reg_val = 0;
            break;
        }

        if (fe_conn.is_iq_swapped()) mapping_reg_val |= FLAG_DSP_RX_MAPPING_SWAP_IQ;
        if (fe_conn.is_i_inverted()) mapping_reg_val |= FLAG_DSP_RX_MAPPING_INVERT_I;
        if (fe_conn.is_q_inverted()) mapping_reg_val |= FLAG_DSP_RX_MAPPING_INVERT_Q;

        _iface->poke32(REG_RX_FE_MAPPING, mapping_reg_val);

        UHD_ASSERT_THROW(_adc_rate!=0.0)
        double cordic_freq = 0.0, actual_cordic_freq = 0.0;
        if (fe_conn.get_sampling_mode() == fe_connection_t::HETERODYNE) {
            //1. Remember the sign of the IF frequency.
            //   It will be discarded in the next step
            int if_freq_sign = boost::math::sign(fe_conn.get_if_freq());
            //2. Map IF frequency to the range [0, _adc_rate)
            double if_freq = std::abs(std::fmod(fe_conn.get_if_freq(), _adc_rate));
            //3. Map IF frequency to the range [-_adc_rate/2, _adc_rate/2)
            //   This is the aliased frequency
            if (if_freq > (_adc_rate / 2.0)) {
                if_freq -= _adc_rate;
            }
            //4. Set DSP offset to spin the signal in the opposite
            //   direction as the aliased frequency
            cordic_freq = if_freq * (-if_freq_sign);
        }
        int32_t freq_word;
        get_freq_and_freq_word(cordic_freq, _adc_rate, actual_cordic_freq, freq_word);
        _iface->poke32(REG_RX_FE_HET_CORDIC_PHASE, uint32_t(freq_word));

        _fe_conn = fe_conn;
    }

    void set_dc_offset_auto(const bool enb) {
        _set_dc_offset(enb ? 0 : OFFSET_FIXED);
    }

    std::complex<double> set_dc_offset(const std::complex<double> &off) {
        static const double scaler = double(1ul << 29);
        _i_dc_off = boost::math::iround(off.real()*scaler);
        _q_dc_off = boost::math::iround(off.imag()*scaler);

        _set_dc_offset(OFFSET_SET | OFFSET_FIXED);

        return std::complex<double>(_i_dc_off/scaler, _q_dc_off/scaler);
    }

    void _set_dc_offset(const uint32_t flags) {
        _iface->poke32(REG_RX_FE_OFFSET_I, flags | (_i_dc_off & ~FLAG_MASK));
        _iface->poke32(REG_RX_FE_OFFSET_Q, flags | (_q_dc_off & ~FLAG_MASK));
    }

    void set_iq_balance(const std::complex<double> &cor) {
        _iface->poke32(REG_RX_FE_MAG_CORRECTION, fs_to_bits(cor.real(), 18));
        _iface->poke32(REG_RX_FE_PHASE_CORRECTION, fs_to_bits(cor.imag(), 18));
    }

    void populate_subtree(uhd::property_tree::sptr subtree) {
        subtree->create<std::complex<double> >("dc_offset/value")
            .set(DEFAULT_DC_OFFSET_VALUE)
            .set_coercer(boost::bind(&rx_frontend_core_3000::set_dc_offset, this, _1))
        ;
        subtree->create<bool>("dc_offset/enable")
            .set(DEFAULT_DC_OFFSET_ENABLE)
            .add_coerced_subscriber(boost::bind(&rx_frontend_core_3000::set_dc_offset_auto, this, _1))
        ;
        subtree->create<std::complex<double> >("iq_balance/value")
            .set(DEFAULT_IQ_BALANCE_VALUE)
            .add_coerced_subscriber(boost::bind(&rx_frontend_core_3000::set_iq_balance, this, _1))
        ;
    }

    double get_output_rate() {
        switch (_fe_conn.get_sampling_mode()) {
        case fe_connection_t::REAL:
        case fe_connection_t::HETERODYNE:
            return _adc_rate / 2;
        default:
            return _adc_rate;
        }
    }

private:
    int32_t  _i_dc_off, _q_dc_off;
    double          _adc_rate;
    fe_connection_t _fe_conn;
    wb_iface::sptr  _iface;
    const size_t    _base;
};

rx_frontend_core_3000::sptr rx_frontend_core_3000::make(wb_iface::sptr iface, const size_t base){
    return sptr(new rx_frontend_core_3000_impl(iface, base));
}

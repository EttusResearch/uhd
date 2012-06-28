//
// Copyright 2010-2012 Ettus Research LLC
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

#include "codec_ctrl.hpp"
#include "ad9862_regs.hpp"
#include <uhd/utils/log.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/byteswap.hpp>
#include <boost/cstdint.hpp>
#include <boost/format.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/math/special_functions/sign.hpp>
#include <boost/assign/list_of.hpp>
#include <iomanip>

using namespace uhd;

const gain_range_t usrp1_codec_ctrl::tx_pga_gain_range(-20, 0, double(0.1));
const gain_range_t usrp1_codec_ctrl::rx_pga_gain_range(0, 20, 1);

/***********************************************************************
 * Codec Control Implementation
 **********************************************************************/
class usrp1_codec_ctrl_impl : public usrp1_codec_ctrl {
public:
    //structors
    usrp1_codec_ctrl_impl(spi_iface::sptr iface, int spi_slave);
    ~usrp1_codec_ctrl_impl(void);

    //aux adc and dac control
    double read_aux_adc(aux_adc_t which);
    void write_aux_dac(aux_dac_t which, double volts);

    //duc control
    void set_duc_freq(double freq, double);
    void enable_tx_digital(bool enb);

    //pga gain control
    void set_tx_pga_gain(double);
    double get_tx_pga_gain(void);
    void set_rx_pga_gain(double, char);
    double get_rx_pga_gain(char);
    
    //rx adc buffer control
    void bypass_adc_buffers(bool bypass);

private:
    spi_iface::sptr _iface;
    int _spi_slave;
    ad9862_regs_t _ad9862_regs;
    void send_reg(boost::uint8_t addr);
    void recv_reg(boost::uint8_t addr);

    double coarse_tune(double codec_rate, double freq);
    double fine_tune(double codec_rate, double freq);
};

/***********************************************************************
 * Codec Control Structors
 **********************************************************************/
usrp1_codec_ctrl_impl::usrp1_codec_ctrl_impl(spi_iface::sptr iface, int spi_slave){
    _iface = iface;
    _spi_slave = spi_slave;

    //soft reset
    _ad9862_regs.soft_reset = 1;
    this->send_reg(0);

    //initialize the codec register settings
    _ad9862_regs.sdio_bidir = ad9862_regs_t::SDIO_BIDIR_SDIO_SDO;
    _ad9862_regs.lsb_first = ad9862_regs_t::LSB_FIRST_MSB;
    _ad9862_regs.soft_reset = 0;

    //setup rx side of codec
    _ad9862_regs.byp_buffer_a = 1;
    _ad9862_regs.byp_buffer_b = 1;
    _ad9862_regs.buffer_a_pd = 1;
    _ad9862_regs.buffer_b_pd = 1;
    _ad9862_regs.rx_pga_a = 0;
    _ad9862_regs.rx_pga_b = 0;
    _ad9862_regs.rx_twos_comp = 1;
    _ad9862_regs.rx_hilbert = ad9862_regs_t::RX_HILBERT_DIS;

    //setup tx side of codec
    _ad9862_regs.two_data_paths = ad9862_regs_t::TWO_DATA_PATHS_BOTH;
    _ad9862_regs.interleaved = ad9862_regs_t::INTERLEAVED_INTERLEAVED;
    _ad9862_regs.tx_pga_gain = 199;
    _ad9862_regs.tx_hilbert = ad9862_regs_t::TX_HILBERT_DIS;
    _ad9862_regs.interp = ad9862_regs_t::INTERP_4;
    _ad9862_regs.tx_twos_comp = 1;
    _ad9862_regs.fine_mode = ad9862_regs_t::FINE_MODE_NCO;
    _ad9862_regs.coarse_mod = ad9862_regs_t::COARSE_MOD_BYPASS;
    _ad9862_regs.dac_a_coarse_gain = 0x3;
    _ad9862_regs.dac_b_coarse_gain = 0x3;

    //setup the dll
    _ad9862_regs.input_clk_ctrl = ad9862_regs_t::INPUT_CLK_CTRL_EXTERNAL;
    _ad9862_regs.dll_mult = ad9862_regs_t::DLL_MULT_2;
    _ad9862_regs.dll_mode = ad9862_regs_t::DLL_MODE_FAST;

    //setup clockout
    _ad9862_regs.clkout2_div_factor = ad9862_regs_t::CLKOUT2_DIV_FACTOR_2;

    //write the register settings to the codec
    for (boost::uint8_t addr = 0; addr <= 25; addr++) {
        this->send_reg(addr);
    }

    //always start conversions for aux ADC
    _ad9862_regs.start_a = 1;
    _ad9862_regs.start_b = 1;

    //aux adc clock
    _ad9862_regs.clk_4 = ad9862_regs_t::CLK_4_1_4;
    this->send_reg(34);
}

usrp1_codec_ctrl_impl::~usrp1_codec_ctrl_impl(void){UHD_SAFE_CALL(
    //set aux dacs to zero
    this->write_aux_dac(AUX_DAC_A, 0);
    this->write_aux_dac(AUX_DAC_B, 0);
    this->write_aux_dac(AUX_DAC_C, 0);
    this->write_aux_dac(AUX_DAC_D, 0);

    //power down
    _ad9862_regs.all_rx_pd = 1;
    this->send_reg(1);
    _ad9862_regs.tx_digital_pd = 1;
    _ad9862_regs.tx_analog_pd = ad9862_regs_t::TX_ANALOG_PD_BOTH;
    this->send_reg(8);
)}

/***********************************************************************
 * Codec Control Gain Control Methods
 **********************************************************************/
static const int mtpgw = 255; //maximum tx pga gain word

void usrp1_codec_ctrl_impl::set_tx_pga_gain(double gain){
    int gain_word = int(mtpgw*(gain - tx_pga_gain_range.start())/(tx_pga_gain_range.stop() - tx_pga_gain_range.start()));
    _ad9862_regs.tx_pga_gain = uhd::clip(gain_word, 0, mtpgw);
    this->send_reg(16);
}

double usrp1_codec_ctrl_impl::get_tx_pga_gain(void){
    return (_ad9862_regs.tx_pga_gain*(tx_pga_gain_range.stop() - tx_pga_gain_range.start())/mtpgw) + tx_pga_gain_range.start();
}

static const int mrpgw = 0x14; //maximum rx pga gain word

void usrp1_codec_ctrl_impl::set_rx_pga_gain(double gain, char which){
    int gain_word = int(mrpgw*(gain - rx_pga_gain_range.start())/(rx_pga_gain_range.stop() - rx_pga_gain_range.start()));
    gain_word = uhd::clip(gain_word, 0, mrpgw);
    switch(which){
    case 'A':
        _ad9862_regs.rx_pga_a = gain_word;
        this->send_reg(2);
        return;
    case 'B':
        _ad9862_regs.rx_pga_b = gain_word;
        this->send_reg(3);
        return;
    default: UHD_THROW_INVALID_CODE_PATH();
    }
}

double usrp1_codec_ctrl_impl::get_rx_pga_gain(char which){
    int gain_word;
    switch(which){
    case 'A': gain_word = _ad9862_regs.rx_pga_a; break;
    case 'B': gain_word = _ad9862_regs.rx_pga_b; break;
    default: UHD_THROW_INVALID_CODE_PATH();
    }
    return (gain_word*(rx_pga_gain_range.stop() - rx_pga_gain_range.start())/mrpgw) + rx_pga_gain_range.start();
}

/***********************************************************************
 * Codec Control AUX ADC Methods
 **********************************************************************/
static double aux_adc_to_volts(boost::uint8_t high, boost::uint8_t low)
{
    return double(((boost::uint16_t(high) << 2) | low)*3.3)/0x3ff;
}

double usrp1_codec_ctrl_impl::read_aux_adc(aux_adc_t which){
    switch(which){
    case AUX_ADC_A1:
        _ad9862_regs.select_a = ad9862_regs_t::SELECT_A_AUX_ADC1;
        this->send_reg(34); //start conversion and select mux
        this->recv_reg(28); //read the value (2 bytes, 2 reads)
        this->recv_reg(29);
        return aux_adc_to_volts(_ad9862_regs.aux_adc_a1_9_2, _ad9862_regs.aux_adc_a1_1_0);

    case AUX_ADC_A2:
        _ad9862_regs.select_a = ad9862_regs_t::SELECT_A_AUX_ADC2;
        this->send_reg(34); //start conversion and select mux
        this->recv_reg(26); //read the value (2 bytes, 2 reads)
        this->recv_reg(27);
        return aux_adc_to_volts(_ad9862_regs.aux_adc_a2_9_2, _ad9862_regs.aux_adc_a2_1_0);

    case AUX_ADC_B1:
        _ad9862_regs.select_b = ad9862_regs_t::SELECT_B_AUX_ADC1;
        this->send_reg(34); //start conversion and select mux
        this->recv_reg(32); //read the value (2 bytes, 2 reads)
        this->recv_reg(33);
        return aux_adc_to_volts(_ad9862_regs.aux_adc_b1_9_2, _ad9862_regs.aux_adc_b1_1_0);

    case AUX_ADC_B2:
        _ad9862_regs.select_b = ad9862_regs_t::SELECT_B_AUX_ADC2;
        this->send_reg(34); //start conversion and select mux
        this->recv_reg(30); //read the value (2 bytes, 2 reads)
        this->recv_reg(31);
        return aux_adc_to_volts(_ad9862_regs.aux_adc_b2_9_2, _ad9862_regs.aux_adc_b2_1_0);
    }
    UHD_THROW_INVALID_CODE_PATH();
}

/***********************************************************************
 * Codec Control AUX DAC Methods
 **********************************************************************/
void usrp1_codec_ctrl_impl::write_aux_dac(aux_dac_t which, double volts)
{
    //special case for aux dac d (aka sigma delta word)
    if (which == AUX_DAC_D) {
        boost::uint16_t dac_word = uhd::clip(boost::math::iround(volts*0xfff/3.3), 0, 0xfff);
        _ad9862_regs.sig_delt_11_4 = boost::uint8_t(dac_word >> 4);
        _ad9862_regs.sig_delt_3_0 = boost::uint8_t(dac_word & 0xf);
        this->send_reg(42);
        this->send_reg(43);
        return;
    }

    //calculate the dac word for aux dac a, b, c
    boost::uint8_t dac_word = uhd::clip(boost::math::iround(volts*0xff/3.3), 0, 0xff);

    //setup a lookup table for the aux dac params (reg ref, reg addr)
    typedef boost::tuple<boost::uint8_t*, boost::uint8_t> dac_params_t;
    uhd::dict<aux_dac_t, dac_params_t> aux_dac_to_params = boost::assign::map_list_of
        (AUX_DAC_A, dac_params_t(&_ad9862_regs.aux_dac_a, 36))
        (AUX_DAC_B, dac_params_t(&_ad9862_regs.aux_dac_b, 37))
        (AUX_DAC_C, dac_params_t(&_ad9862_regs.aux_dac_c, 38))
    ;

    //set the aux dac register
    UHD_ASSERT_THROW(aux_dac_to_params.has_key(which));
    boost::uint8_t *reg_ref, reg_addr;
    boost::tie(reg_ref, reg_addr) = aux_dac_to_params[which];
    *reg_ref = dac_word;
    this->send_reg(reg_addr);
}

/***********************************************************************
 * Codec Control SPI Methods
 **********************************************************************/
void usrp1_codec_ctrl_impl::send_reg(boost::uint8_t addr)
{
    boost::uint32_t reg = _ad9862_regs.get_write_reg(addr);

    UHD_LOGV(often)
        << "codec control write reg: 0x"
        << std::setw(8) << std::hex << reg << std::endl
    ;
    _iface->write_spi(_spi_slave,
                         spi_config_t::EDGE_RISE, reg, 16);
}

void usrp1_codec_ctrl_impl::recv_reg(boost::uint8_t addr)
{
    boost::uint32_t reg = _ad9862_regs.get_read_reg(addr);

    UHD_LOGV(often)
        << "codec control read reg: 0x"
        << std::setw(8) << std::hex << reg << std::endl
    ;

    boost::uint32_t ret = _iface->read_spi(_spi_slave,
                                        spi_config_t::EDGE_RISE, reg, 16);

    UHD_LOGV(often)
        << "codec control read ret: 0x"
        << std::setw(8) << std::hex << ret << std::endl
    ;

    _ad9862_regs.set_reg(addr, boost::uint16_t(ret));
}

/***********************************************************************
 * DUC tuning 
 **********************************************************************/
double usrp1_codec_ctrl_impl::coarse_tune(double codec_rate, double freq)
{
    double coarse_freq;

    double coarse_freq_1 = codec_rate / 8;
    double coarse_freq_2 = codec_rate / 4;
    double coarse_limit_1 = coarse_freq_1 / 2;
    double coarse_limit_2 = (coarse_freq_1 + coarse_freq_2) / 2;
    double max_freq = coarse_freq_2 + .09375 * codec_rate;
 
    if (freq < -max_freq) {
        return false;
    }
    else if (freq < -coarse_limit_2) {
        _ad9862_regs.neg_coarse_tune = ad9862_regs_t::NEG_COARSE_TUNE_NEG_SHIFT;
        _ad9862_regs.coarse_mod = ad9862_regs_t::COARSE_MOD_FDAC_4;
        coarse_freq = -coarse_freq_2;
    }
    else if (freq < -coarse_limit_1) {
        _ad9862_regs.neg_coarse_tune = ad9862_regs_t::NEG_COARSE_TUNE_NEG_SHIFT;
        _ad9862_regs.coarse_mod = ad9862_regs_t::COARSE_MOD_FDAC_8;
        coarse_freq = -coarse_freq_1;
    }
    else if (freq < coarse_limit_1) {
        _ad9862_regs.coarse_mod = ad9862_regs_t::COARSE_MOD_BYPASS;
        coarse_freq = 0; 
    }
    else if (freq < coarse_limit_2) {
        _ad9862_regs.neg_coarse_tune = ad9862_regs_t::NEG_COARSE_TUNE_POS_SHIFT;
        _ad9862_regs.coarse_mod = ad9862_regs_t::COARSE_MOD_FDAC_8;
        coarse_freq = coarse_freq_1;
    }
    else if (freq <= max_freq) {
        _ad9862_regs.neg_coarse_tune = ad9862_regs_t::NEG_COARSE_TUNE_POS_SHIFT;
        _ad9862_regs.coarse_mod = ad9862_regs_t::COARSE_MOD_FDAC_4;
        coarse_freq = coarse_freq_2;
    }
    else {
        return 0; 
    }

    return coarse_freq;
}

double usrp1_codec_ctrl_impl::fine_tune(double codec_rate, double target_freq)
{
    static const double scale_factor = std::pow(2.0, 24);

    boost::uint32_t freq_word = boost::uint32_t(
        boost::math::round(abs((target_freq / codec_rate) * scale_factor)));

    double actual_freq = freq_word * codec_rate / scale_factor;

    if (target_freq < 0) {
        _ad9862_regs.neg_fine_tune = ad9862_regs_t::NEG_FINE_TUNE_NEG_SHIFT;
        actual_freq = -actual_freq; 
    }
    else {
        _ad9862_regs.neg_fine_tune = ad9862_regs_t::NEG_FINE_TUNE_POS_SHIFT;
    } 

    _ad9862_regs.fine_mode = ad9862_regs_t::FINE_MODE_NCO;
    _ad9862_regs.ftw_23_16 = (freq_word >> 16) & 0xff;
    _ad9862_regs.ftw_15_8  = (freq_word >>  8) & 0xff;
    _ad9862_regs.ftw_7_0   = (freq_word >>  0) & 0xff;

    return actual_freq;
}

void usrp1_codec_ctrl_impl::set_duc_freq(double freq, double rate)
{
    double codec_rate = rate * 2;

    //correct for outside of rate (wrap around)
    freq = std::fmod(freq, rate);
    if (std::abs(freq) > rate/2.0)
        freq -= boost::math::sign(freq)*rate;

    double coarse_freq = coarse_tune(codec_rate, freq);
    double fine_freq = fine_tune(codec_rate / 4, freq - coarse_freq);

    UHD_LOG
        << "ad9862 tuning result:" << std::endl
        << "   requested:   " << freq << std::endl
        << "   actual:      " << coarse_freq + fine_freq << std::endl
        << "   coarse freq: " << coarse_freq << std::endl
        << "   fine freq:   " << fine_freq << std::endl
        << "   codec rate:  " << codec_rate << std::endl
    ;

    this->send_reg(20);
    this->send_reg(21);
    this->send_reg(22);
    this->send_reg(23);
}

void usrp1_codec_ctrl_impl::enable_tx_digital(bool enb){
    _ad9862_regs.tx_digital_pd = (enb)? 0 : 1;
    this->send_reg(8);
}

/***********************************************************************
 * Codec Control ADC buffer bypass
 * Disable this for AC-coupled daughterboards (TVRX)
 * By default it is initialized TRUE.
 **********************************************************************/
void usrp1_codec_ctrl_impl::bypass_adc_buffers(bool bypass) {
    _ad9862_regs.byp_buffer_a = bypass;
    _ad9862_regs.byp_buffer_b = bypass;
    this->send_reg(2);
}

/***********************************************************************
 * Codec Control Make
 **********************************************************************/
usrp1_codec_ctrl::sptr usrp1_codec_ctrl::make(spi_iface::sptr iface,
                                              int spi_slave)
{
    return sptr(new usrp1_codec_ctrl_impl(iface, spi_slave));
}

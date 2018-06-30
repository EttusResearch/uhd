//
// Copyright 2015-2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_X300_RADIO_CTRL_IMPL_HPP
#define INCLUDED_LIBUHD_RFNOC_X300_RADIO_CTRL_IMPL_HPP

#include "x300_clock_ctrl.hpp"
#include "x300_adc_ctrl.hpp"
#include "x300_dac_ctrl.hpp"
#include "x300_regs.hpp"
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/usrp/gpio_defs.hpp>
#include <uhdlib/rfnoc/radio_ctrl_impl.hpp>
#include <uhdlib/usrp/cores/rx_frontend_core_3000.hpp>
#include <uhdlib/usrp/cores/tx_frontend_core_200.hpp>
#include <uhdlib/usrp/cores/spi_core_3000.hpp>

namespace uhd {
    namespace rfnoc {

/*! \brief Provide access to an X300 radio.
 */
class x300_radio_ctrl_impl : public radio_ctrl_impl
{
public:
    typedef boost::shared_ptr<x300_radio_ctrl_impl> sptr;

    /************************************************************************
     * Structors
     ***********************************************************************/
    UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR_DECL(x300_radio_ctrl)
    virtual ~x300_radio_ctrl_impl();

    /************************************************************************
     * API calls
     ***********************************************************************/
    double set_rate(double rate);

    void set_tx_antenna(const std::string &ant, const size_t chan);
    void set_rx_antenna(const std::string &ant, const size_t chan);

    double set_tx_frequency(const double freq, const size_t chan);
    double set_rx_frequency(const double freq, const size_t chan);
    double set_rx_bandwidth(const double bandwidth, const size_t chan);
    double get_tx_frequency(const size_t chan);
    double get_rx_frequency(const size_t chan);
    double get_rx_bandwidth(const size_t chan);

    double set_tx_gain(const double gain, const size_t chan);
    double set_rx_gain(const double gain, const size_t chan);

    std::vector<std::string> get_rx_lo_names(const size_t chan);
    std::vector<std::string> get_rx_lo_sources(const std::string &name, const size_t chan);
    freq_range_t get_rx_lo_freq_range(const std::string &name, const size_t chan);

    void set_rx_lo_source(const std::string &src, const std::string &name, const size_t chan);
    const std::string get_rx_lo_source(const std::string &name, const size_t chan);

    void set_rx_lo_export_enabled(bool enabled, const std::string &name, const size_t chan);
    bool get_rx_lo_export_enabled(const std::string &name, const size_t chan);

    double set_rx_lo_freq(double freq, const std::string &name, const size_t chan);
    double get_rx_lo_freq(const std::string &name, const size_t chan);

    size_t get_chan_from_dboard_fe(const std::string &fe, const direction_t dir);
    std::string get_dboard_fe_from_chan(const size_t chan, const direction_t dir);

    std::vector<std::string> get_gpio_banks() const;
    void set_gpio_attr(const std::string &bank, const std::string &attr, const uint32_t value, const uint32_t mask);
    uint32_t get_gpio_attr(const std::string &bank, const std::string &attr);

    double get_output_samp_rate(size_t port);

    /************************************************************************
     * Hardware setup and control
     ***********************************************************************/
    /*! Set up the radio. No API calls may be made before this one.
     */
    void setup_radio(
        uhd::i2c_iface::sptr zpu_i2c,
        x300_clock_ctrl::sptr clock,
        bool ignore_cal_file,
        bool verbose
    );

    void reset_codec();

    void self_test_adc(
        uint32_t ramp_time_ms = 100);

    static void extended_adc_test(
        const std::vector<x300_radio_ctrl_impl::sptr>&, double duration_s);

    static void synchronize_dacs(
        const std::vector<x300_radio_ctrl_impl::sptr>& radios);

    static double self_cal_adc_xfer_delay(
        const std::vector<x300_radio_ctrl_impl::sptr>& radios,
        x300_clock_ctrl::sptr clock,
        boost::function<void(double)> wait_for_clk_locked,
        bool apply_delay);

protected:
    virtual bool check_radio_config();

private:
    class radio_regmap_t : public uhd::soft_regmap_t {
    public:
        typedef boost::shared_ptr<radio_regmap_t> sptr;
        class misc_outs_reg_t : public uhd::soft_reg32_wo_t {
        public:
            UHD_DEFINE_SOFT_REG_FIELD(DAC_ENABLED,          /*width*/ 1, /*shift*/ 0);  //[0]
            UHD_DEFINE_SOFT_REG_FIELD(DAC_RESET_N,          /*width*/ 1, /*shift*/ 1);  //[1]
            UHD_DEFINE_SOFT_REG_FIELD(ADC_RESET,            /*width*/ 1, /*shift*/ 2);  //[2]
            UHD_DEFINE_SOFT_REG_FIELD(ADC_DATA_DLY_STB,     /*width*/ 1, /*shift*/ 3);  //[3]
            UHD_DEFINE_SOFT_REG_FIELD(ADC_DATA_DLY_VAL,     /*width*/ 5, /*shift*/ 4);  //[8:4]
            UHD_DEFINE_SOFT_REG_FIELD(ADC_CHECKER_ENABLED,  /*width*/ 1, /*shift*/ 9);  //[9]
            UHD_DEFINE_SOFT_REG_FIELD(DAC_SYNC,             /*width*/ 1, /*shift*/ 10); //[10]

            misc_outs_reg_t(): uhd::soft_reg32_wo_t(regs::sr_addr(regs::MISC_OUTS)) {
                //Initial values
                set(DAC_ENABLED, 0);
                set(DAC_RESET_N, 0);
                set(ADC_RESET, 0);
                set(ADC_DATA_DLY_STB, 0);
                set(ADC_DATA_DLY_VAL, 16);
                set(ADC_CHECKER_ENABLED, 0);
                set(DAC_SYNC, 0);
            }
        } misc_outs_reg;

        class misc_ins_reg_t : public uhd::soft_reg64_ro_t {
        public:
            UHD_DEFINE_SOFT_REG_FIELD(ADC_CHECKER0_Q_LOCKED, /*width*/ 1, /*shift*/ 32);  //[0]
            UHD_DEFINE_SOFT_REG_FIELD(ADC_CHECKER0_I_LOCKED, /*width*/ 1, /*shift*/ 33);  //[1]
            UHD_DEFINE_SOFT_REG_FIELD(ADC_CHECKER1_Q_LOCKED, /*width*/ 1, /*shift*/ 34);  //[2]
            UHD_DEFINE_SOFT_REG_FIELD(ADC_CHECKER1_I_LOCKED, /*width*/ 1, /*shift*/ 35);  //[3]
            UHD_DEFINE_SOFT_REG_FIELD(ADC_CHECKER0_Q_ERROR,  /*width*/ 1, /*shift*/ 36);  //[4]
            UHD_DEFINE_SOFT_REG_FIELD(ADC_CHECKER0_I_ERROR,  /*width*/ 1, /*shift*/ 37);  //[5]
            UHD_DEFINE_SOFT_REG_FIELD(ADC_CHECKER1_Q_ERROR,  /*width*/ 1, /*shift*/ 38);  //[6]
            UHD_DEFINE_SOFT_REG_FIELD(ADC_CHECKER1_I_ERROR,  /*width*/ 1, /*shift*/ 39);  //[7]

            misc_ins_reg_t(): uhd::soft_reg64_ro_t(regs::rb_addr(regs::RB_MISC_IO)) { }
        } misc_ins_reg;

        radio_regmap_t(int radio_num) : soft_regmap_t("radio" + std::to_string(radio_num) + "_regmap") {
            add_to_map(misc_outs_reg, "misc_outs_reg", PRIVATE);
            add_to_map(misc_ins_reg, "misc_ins_reg", PRIVATE);
        }
    };

    struct x300_regs {
        static const uint32_t TX_FE_BASE    = 224;
        static const uint32_t RX_FE_BASE    = 232;
    };

    void _update_atr_leds(const std::string &rx_ant, const size_t chan);

    void _self_cal_adc_capture_delay(bool print_status);

    void _check_adc(const uint32_t val);

    void _set_db_eeprom(uhd::i2c_iface::sptr i2c, const size_t, const uhd::usrp::dboard_eeprom_t &);

    void set_rx_fe_corrections(const uhd::fs_path &db_path, const uhd::fs_path &rx_fe_corr_path, const double lo_freq);
    void set_tx_fe_corrections(const uhd::fs_path &db_path, const uhd::fs_path &tx_fe_corr_path, const double lo_freq);

    void _set_command_time(const uhd::time_spec_t &spec, const size_t port);
    void set_fe_cmd_time(const time_spec_t &time, const size_t chan);

private: // members
    enum radio_connection_t { PRIMARY, SECONDARY };

    radio_connection_t                  _radio_type;
    std::string                         _radio_slot;
    //! Radio clock rate is the rate at which the ADC and DAC are running at.
    // Not necessarily this block's sampling rate (tick rate).
    double                              _radio_clk_rate;

    radio_regmap_t::sptr                                    _regs;
    std::map<size_t, usrp::gpio_atr::gpio_atr_3000::sptr>   _leds;
    spi_core_3000::sptr                                     _spi;
    x300_adc_ctrl::sptr                                     _adc;
    x300_dac_ctrl::sptr                                     _dac;
    usrp::gpio_atr::gpio_atr_3000::sptr                     _fp_gpio;

    std::map<size_t, usrp::dboard_eeprom_t> _db_eeproms;
    usrp::dboard_manager::sptr              _db_manager;

    struct rx_fe_perif {
        std::string                 name;
        std::string                 db_fe_name;
        rx_frontend_core_3000::sptr core;
    };
    struct tx_fe_perif {
        std::string                 name;
        std::string                 db_fe_name;
        tx_frontend_core_200::sptr  core;
    };

    std::map<size_t, rx_fe_perif>   _rx_fe_map;
    std::map<size_t, tx_fe_perif>   _tx_fe_map;

    bool _ignore_cal_file;

}; /* class radio_ctrl_impl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_X300_RADIO_CTRL_IMPL_HPP */
// vim: sw=4 et:

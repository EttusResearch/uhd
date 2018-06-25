//
// Copyright 2014 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_AD9361_DEVICE_H
#define INCLUDED_AD9361_DEVICE_H

#include <ad9361_client.h>
#include <boost/noncopyable.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <uhd/types/filters.hpp>
#include <uhd/types/sensors.hpp>
#include <complex>
#include <vector>
#include <map>
#include "boost/assign.hpp"
#include "boost/bind.hpp"
#include "boost/function.hpp"

namespace uhd { namespace usrp {

class ad9361_device_t : public boost::noncopyable
{
public:
    enum direction_t { RX, TX };
    enum gain_mode_t {GAIN_MODE_MANUAL, GAIN_MODE_SLOW_AGC, GAIN_MODE_FAST_AGC};
    enum chain_t { CHAIN_1, CHAIN_2, CHAIN_BOTH };
    enum timing_mode_t { TIMING_MODE_1R1T, TIMING_MODE_2R2T };

    ad9361_device_t(ad9361_params::sptr client, ad9361_io::sptr io_iface) :
        _client_params(client), _io_iface(io_iface),
        _bbpll_freq(0.0), _adcclock_freq(0.0), _req_clock_rate(0.0),
        _req_coreclk(0.0), _rx_bbf_tunediv(0), _curr_gain_table(0),
        _rx1_gain(0.0), _rx2_gain(0.0), _tx1_gain(0.0), _tx2_gain(0.0),
        _tfir_factor(0), _rfir_factor(0),
        _rx1_agc_mode(GAIN_MODE_MANUAL), _rx2_agc_mode(GAIN_MODE_MANUAL),
        _rx1_agc_enable(false), _rx2_agc_enable(false),
        _use_dc_offset_tracking(false), _use_iq_balance_tracking(false)
    {

        /*
         * This Boost.Assign to_container() workaround is necessary because STL containers
         * apparently confuse newer versions of MSVC.
         *
         * Source: http://www.boost.org/doc/libs/1_55_0/libs/assign/doc/#portability
         */

        _rx_filters = (boost::assign::map_list_of("LPF_TIA", filter_query_helper(boost::bind(&ad9361_device_t::_get_filter_lp_tia_sec, this, _1),
                                                    boost::bind(&ad9361_device_t::_set_filter_lp_tia_sec, this, _1, _3)))
                                            ("LPF_BB", filter_query_helper(boost::bind(&ad9361_device_t::_get_filter_lp_bb, this, _1),
                                                    boost::bind(&ad9361_device_t::_set_filter_lp_bb, this, _1, _3)))
                                            ("HB_3", filter_query_helper(boost::bind(&ad9361_device_t::_get_filter_hb_3, this, _1), 0))
                                            ("DEC_3", filter_query_helper(boost::bind(&ad9361_device_t::_get_filter_dec_int_3, this, _1), 0))
                                            ("HB_2", filter_query_helper(boost::bind(&ad9361_device_t::_get_filter_hb_2, this, _1), 0))
                                            ("HB_1", filter_query_helper(boost::bind(&ad9361_device_t::_get_filter_hb_1, this, _1), 0))
                                            ("FIR_1", filter_query_helper(boost::bind(&ad9361_device_t::_get_filter_fir, this, _1, _2),
                                                    boost::bind(&ad9361_device_t::_set_filter_fir, this, _1, _2, _3)))).to_container(_rx_filters);

        _tx_filters = (boost::assign::map_list_of("LPF_SECONDARY", filter_query_helper(boost::bind(&ad9361_device_t::_get_filter_lp_tia_sec, this, _1),
                                                    boost::bind(&ad9361_device_t::_set_filter_lp_tia_sec, this, _1, _3)))
                                            ("LPF_BB", filter_query_helper(boost::bind(&ad9361_device_t::_get_filter_lp_bb, this, _1),
                                                    boost::bind(&ad9361_device_t::_set_filter_lp_bb, this, _1, _3)))
                                            ("HB_3", filter_query_helper(boost::bind(&ad9361_device_t::_get_filter_hb_3, this, _1), 0))
                                            ("INT_3", filter_query_helper(boost::bind(&ad9361_device_t::_get_filter_dec_int_3, this, _1), 0))
                                            ("HB_2", filter_query_helper(boost::bind(&ad9361_device_t::_get_filter_hb_2, this, _1), 0))
                                            ("HB_1", filter_query_helper(boost::bind(&ad9361_device_t::_get_filter_hb_1, this, _1), 0))
                                            ("FIR_1", filter_query_helper(boost::bind(&ad9361_device_t::_get_filter_fir, this, _1, _2),
                                                    boost::bind(&ad9361_device_t::_set_filter_fir, this, _1, _2, _3)))).to_container(_tx_filters);
    }

    /* Initialize the AD9361 codec. */
    void initialize();

    /* Set SPI interface */
    void set_io_iface(ad9361_io::sptr io_iface);

    /* This function sets the RX / TX rate between AD9361 and the FPGA, and
     * thus determines the interpolation / decimation required in the FPGA to
     * achieve the user's requested rate.
     */
    double set_clock_rate(const double req_rate);

    /* Set which of the four TX / RX chains provided by AD9361 are active.
     *
     * AD9361 provides two sets of chains, Side A and Side B. Each side
     * provides one TX antenna, and one RX antenna. The B200 maintains the USRP
     * standard of providing one antenna connection that is both TX & RX, and
     * one that is RX-only - for each chain. Thus, the possible antenna and
     * chain selections are:
     *
     */
    void set_active_chains(bool tx1, bool tx2, bool rx1, bool rx2);

    /* Setup Timing mode depending on active channels.
     *
     * LVDS interface can have two timing modes - 1R1T and 2R2T
     */
    void set_timing_mode(const timing_mode_t timing_mode);

    /* Tune the RX or TX frequency.
     *
     * This is the publicly-accessible tune function. It makes sure the tune
     * isn't a redundant request, and if not, passes it on to the class's
     * internal tune function.
     *
     * After tuning, it runs any appropriate calibrations. */
    double tune(direction_t direction, const double value);

    /* Get the current RX or TX frequency. */
    double get_freq(direction_t direction);

    /* Set the gain of RX1, RX2, TX1, or TX2.
     *
     * Note that the 'value' passed to this function is the actual gain value,
     * _not_ the gain index. This is the opposite of the eval software's GUI!
     * Also note that the RX chains are done in terms of gain, and the TX chains
     * are done in terms of attenuation. */
    double set_gain(direction_t direction, chain_t chain, const double value);

    /* Make AD9361 output its test tone. */
    void output_test_tone();

    void digital_test_tone(bool enb); // Digital output

    /* Turn on/off AD9361's TX port --> RX port loopback. */
    void data_port_loopback(const bool loopback_enabled);

    /* Read back the internal RSSI measurement data. */
    double get_rssi(chain_t chain);

    /*! Read the internal temperature sensor
     *\param calibrate return raw sensor readings or apply calibration factor.
     *\param num_samples number of measurements to average over
     */
    double get_average_temperature(const double cal_offset = -30.0, const size_t num_samples = 3);

    /* Turn on/off AD9361's RX DC offset correction */
    void set_dc_offset_auto(direction_t direction, const bool on);

    /* Turn on/off AD9361's RX IQ imbalance correction */
    void set_iq_balance_auto(direction_t direction, const bool on);

    /* Configure AD9361's AGC module to use either fast or slow AGC mode. */
    void set_agc_mode(chain_t chain, gain_mode_t gain_mode);

    /* Enable AD9361's AGC gain mode. */
    void set_agc(chain_t chain, bool enable);

    /* Set bandwidth of AD9361's analog LP filters.
     * Bandwidth should be RF bandwidth */
    double set_bw_filter(direction_t direction, const double rf_bw);

    /*
     * Filter API implementation
     * */
    filter_info_base::sptr get_filter(direction_t direction, chain_t chain, const std::string &name);

    void set_filter(direction_t direction, chain_t chain, const std::string &name, filter_info_base::sptr filter);

    std::vector<std::string> get_filter_names(direction_t direction);

    //Constants
    static const double AD9361_MAX_GAIN;
    static const double AD9361_MAX_CLOCK_RATE;
    static const double AD9361_MIN_CLOCK_RATE;
    static const double AD9361_CAL_VALID_WINDOW;
    static const double AD9361_MIN_BW;
    static const double AD9361_MAX_BW;
    static const double DEFAULT_RX_FREQ;
    static const double DEFAULT_TX_FREQ;

private:    //Methods
    void _program_fir_filter(direction_t direction, int num_taps, uint16_t *coeffs);
    void _setup_tx_fir(size_t num_taps, int32_t interpolation);
    void _setup_rx_fir(size_t num_taps, int32_t decimation);
    void _program_fir_filter(direction_t direction, chain_t chain, int num_taps, uint16_t *coeffs);
    void _setup_tx_fir(size_t num_taps);
    void _setup_rx_fir(size_t num_taps);
    void _calibrate_lock_bbpll();
    void _calibrate_synth_charge_pumps();
    double _calibrate_baseband_rx_analog_filter(double rfbw);
    double _calibrate_baseband_tx_analog_filter(double rfbw);
    double _calibrate_secondary_tx_filter(double rfbw);
    double _calibrate_rx_TIAs(double rfbw);
    void _setup_adc();
    void _calibrate_baseband_dc_offset();
    void _calibrate_rf_dc_offset();
    void _calibrate_rx_quadrature();
    void _tx_quadrature_cal_routine();
    void _calibrate_tx_quadrature();
    void _program_mixer_gm_subtable();
    void _program_gain_table();
    void _setup_gain_control(bool use_agc);
    void _setup_synth(direction_t direction, double vcorate);
    double _tune_bbvco(const double rate);
    void _reprogram_gains();
    double _tune_helper(direction_t direction, const double value);
    double _setup_rates(const double rate);
    double _get_temperature(const double cal_offset, const double timeout = 0.1);
    void _configure_bb_dc_tracking();
    void _configure_rx_iq_tracking();
    void _setup_agc(chain_t chain, gain_mode_t gain_mode);
    void _set_fir_taps(direction_t direction, chain_t chain, const std::vector<int16_t>& taps);
    std::vector<int16_t> _get_fir_taps(direction_t direction, chain_t chain);
    size_t _get_num_fir_taps(direction_t direction);
    size_t _get_fir_dec_int(direction_t direction);
    filter_info_base::sptr _get_filter_lp_tia_sec(direction_t direction);
    filter_info_base::sptr _get_filter_lp_bb(direction_t direction);
    filter_info_base::sptr _get_filter_dec_int_3(direction_t direction);
    filter_info_base::sptr _get_filter_hb_3(direction_t direction);
    filter_info_base::sptr _get_filter_hb_2(direction_t direction);
    filter_info_base::sptr _get_filter_hb_1(direction_t direction);
    filter_info_base::sptr _get_filter_fir(direction_t direction, chain_t chain);
    void _set_filter_fir(direction_t direction, chain_t channel, filter_info_base::sptr filter);
    void _set_filter_lp_bb(direction_t direction, filter_info_base::sptr filter);
    void _set_filter_lp_tia_sec(direction_t direction, filter_info_base::sptr filter);

private:    //Members
    struct chip_regs_t
    {
        chip_regs_t():
            vcodivs(0), inputsel(0), rxfilt(0), txfilt(0),
            bbpll(0), bbftune_config(0), bbftune_mode(0) {}
        uint8_t vcodivs;
        uint8_t inputsel;
        uint8_t rxfilt;
        uint8_t txfilt;
        uint8_t bbpll;
        uint8_t bbftune_config;
        uint8_t bbftune_mode;
    };

    struct filter_query_helper
    {
        filter_query_helper(
                boost::function<filter_info_base::sptr (direction_t, chain_t)> p_get,
                boost::function<void (direction_t, chain_t, filter_info_base::sptr)> p_set
                ) : get(p_get), set(p_set) {  }

        filter_query_helper(){ }

        boost::function<filter_info_base::sptr (direction_t, chain_t)> get;
        boost::function<void (direction_t, chain_t, filter_info_base::sptr)> set;
    };

    std::map<std::string, filter_query_helper> _rx_filters;
    std::map<std::string, filter_query_helper> _tx_filters;

    //Interfaces
    ad9361_params::sptr _client_params;
    ad9361_io::sptr     _io_iface;
    //Intermediate state
    double              _rx_freq, _tx_freq, _req_rx_freq, _req_tx_freq;
    double              _last_rx_cal_freq, _last_tx_cal_freq;
    double              _rx_analog_bw, _tx_analog_bw, _rx_bb_lp_bw, _tx_bb_lp_bw;
    double              _rx_tia_lp_bw, _tx_sec_lp_bw;
    //! Current baseband sampling rate (this is the actual rate the device is
    //  is running at)
    double              _baseband_bw;
    double              _bbpll_freq, _adcclock_freq;
    //! This was the last clock rate value that was requested.
    //  It is cached so we don't need to re-set the clock rate
    //  if another call to set_clock_rate() actually has the same value.
    double              _req_clock_rate;
    double              _req_coreclk;
    uint16_t     _rx_bbf_tunediv;
    uint8_t      _curr_gain_table;
    double              _rx1_gain, _rx2_gain, _tx1_gain, _tx2_gain;
    int32_t      _tfir_factor;
    int32_t      _rfir_factor;
    gain_mode_t         _rx1_agc_mode, _rx2_agc_mode;
    bool                _rx1_agc_enable, _rx2_agc_enable;
    //Register soft-copies
    chip_regs_t         _regs;
    //Synchronization
    boost::recursive_mutex  _mutex;
    bool _use_dc_offset_tracking;
    bool _use_iq_balance_tracking;
};

}}  //namespace

#endif /* INCLUDED_AD9361_DEVICE_H */

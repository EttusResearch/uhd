//
// Copyright 2014 Ettus Research LLC
//

#ifndef INCLUDED_AD9361_DEVICE_H
#define INCLUDED_AD9361_DEVICE_H

#include <ad9361_client.h>
#include <boost/noncopyable.hpp>
#include <boost/thread/recursive_mutex.hpp>

namespace uhd { namespace usrp {

class ad9361_device_t : public boost::noncopyable
{
public:
    enum direction_t { RX, TX };
    enum chain_t { CHAIN_1, CHAIN_2 };

    ad9361_device_t(ad9361_params::sptr client, ad9361_io::sptr io_iface) :
        _client_params(client), _io_iface(io_iface) {}

    /* Initialize the AD9361 codec. */
    void initialize();

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

    /* Tune the RX or TX frequency.
     *
     * This is the publicly-accessible tune function. It makes sure the tune
     * isn't a redundant request, and if not, passes it on to the class's
     * internal tune function.
     *
     * After tuning, it runs any appropriate calibrations. */
    double tune(direction_t direction, const double value);

    /* Set the gain of RX1, RX2, TX1, or TX2.
     *
     * Note that the 'value' passed to this function is the actual gain value,
     * _not_ the gain index. This is the opposite of the eval software's GUI!
     * Also note that the RX chains are done in terms of gain, and the TX chains
     * are done in terms of attenuation. */
    double set_gain(direction_t direction, chain_t chain, const double value);

    /* Make AD9361 output its test tone. */
    void output_test_tone();

    /* Turn on/off AD9361's TX port --> RX port loopback. */
    void data_port_loopback(const bool loopback_enabled);

    //Constants
    static const double AD9361_MAX_GAIN;
    static const double AD9361_MAX_CLOCK_RATE;
    static const double AD9361_RECOMMENDED_MAX_CLOCK_RATE;

private:    //Methods
    void _program_fir_filter(direction_t direction, int num_taps, boost::uint16_t *coeffs);
    void _setup_tx_fir(size_t num_taps);
    void _setup_rx_fir(size_t num_taps);
    void _calibrate_lock_bbpll();
    void _calibrate_synth_charge_pumps();
    double _calibrate_baseband_rx_analog_filter();
    double _calibrate_baseband_tx_analog_filter();
    void _calibrate_secondary_tx_filter();
    void _calibrate_rx_TIAs();
    void _setup_adc();
    void _calibrate_baseband_dc_offset();
    void _calibrate_rf_dc_offset();
    void _calibrate_rx_quadrature();
    void _tx_quadrature_cal_routine();
    void _calibrate_tx_quadrature();
    void _program_mixer_gm_subtable();
    void _program_gain_table();
    void _setup_gain_control();
    void _setup_synth(direction_t direction, double vcorate);
    double _tune_bbvco(const double rate);
    void _reprogram_gains();
    double _tune_helper(direction_t direction, const double value);
    double _setup_rates(const double rate);

private:    //Members
    typedef struct {
        boost::uint8_t vcodivs;
        boost::uint8_t inputsel;
        boost::uint8_t rxfilt;
        boost::uint8_t txfilt;
        boost::uint8_t bbpll;
        boost::uint8_t bbftune_config;
        boost::uint8_t bbftune_mode;
    } chip_regs_t;

    //Interfaces
    ad9361_params::sptr _client_params;
    ad9361_io::sptr     _io_iface;
    //Intermediate state
    double              _rx_freq, _tx_freq, _req_rx_freq, _req_tx_freq;
    double              _baseband_bw, _bbpll_freq, _adcclock_freq;
    double              _req_clock_rate, _req_coreclk;
    boost::uint16_t     _rx_bbf_tunediv;
    boost::uint8_t      _curr_gain_table;
    boost::uint32_t     _rx1_gain, _rx2_gain, _tx1_gain, _tx2_gain;
    boost::int32_t      _tfir_factor;
    //Register soft-copies
    chip_regs_t         _regs;
    //Synchronization
    boost::recursive_mutex  _mutex;
};

}}  //namespace

#endif /* INCLUDED_AD9361_DEVICE_H */

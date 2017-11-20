//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0
//

#include "magnesium_radio_ctrl_impl.hpp"
#include "magnesium_constants.hpp"
#include "magnesium_gain_table.hpp"
#include <uhd/utils/log.hpp>
#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <uhd/transport/chdr.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/exception.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <boost/format.hpp>
#include <sstream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;

namespace {
    /**************************************************************************
     * ADF4351 Controls
     *************************************************************************/
    /*!
     * \param lo_iface Reference to the LO object
     * \param freq Frequency (in Hz) of the tone to be generated from the LO
     * \param ref_clock_freq Frequency (in Hz) of the reference clock at the
     *                       PLL input of the LO
     * \param int_n_mode Integer-N mode on or off
     */
    double _lo_set_frequency(
        adf435x_iface::sptr lo_iface,
        const double freq,
        const double ref_clock_freq,
        const bool int_n_mode
    ) {
        UHD_LOG_TRACE("MG/ADF4351",
            "Attempting to tune low band LO to " << freq <<
            " Hz with ref clock freq " << ref_clock_freq);
        lo_iface->set_feedback_select(adf435x_iface::FB_SEL_DIVIDED);
        lo_iface->set_reference_freq(ref_clock_freq);
        lo_iface->set_prescaler(adf435x_iface::PRESCALER_4_5);
        const double actual_freq = lo_iface->set_frequency(freq, int_n_mode);
        lo_iface->set_output_power(
            adf435x_iface::RF_OUTPUT_A,
            adf435x_iface::OUTPUT_POWER_2DBM
        );
        lo_iface->set_output_power(
            adf435x_iface::RF_OUTPUT_B,
            adf435x_iface::OUTPUT_POWER_2DBM
        );
        lo_iface->set_charge_pump_current(
                adf435x_iface::CHARGE_PUMP_CURRENT_0_31MA);
        return actual_freq;
    }

    /*! Configure and enable LO
     *
     * Will tune it to requested frequency and enable outputs.
     *
     * \param lo_iface Reference to the LO object
     * \param lo_freq Frequency (in Hz) of the tone to be generated from the LO
     * \param ref_clock_freq Frequency (in Hz) of the reference clock at the
     *                       PLL input of the LO
     * \param int_n_mode Integer-N mode on or off
     * \returns the actual frequency the LO is running at
     */
    double _lo_enable(
        adf435x_iface::sptr lo_iface,
        const double lo_freq,
        const double ref_clock_freq,
        const bool int_n_mode
    ) {
        const double actual_lo_freq =
            _lo_set_frequency(lo_iface, lo_freq, ref_clock_freq, int_n_mode);
        lo_iface->set_output_enable(adf435x_iface::RF_OUTPUT_A, true);
        lo_iface->set_output_enable(adf435x_iface::RF_OUTPUT_B, true);
        lo_iface->commit();
        return actual_lo_freq;
    }

    /*! Disable LO
     */
    void _lo_disable(adf435x_iface::sptr lo_iface)
    {
        lo_iface->set_output_enable(adf435x_iface::RF_OUTPUT_A, false);
        lo_iface->set_output_enable(adf435x_iface::RF_OUTPUT_B, false);
        lo_iface->commit();
    }

    // TODO: remove this helper when there are only 2 radios
    fs_path master_fe_base_path(const std::string &radio_slot)
    {
        if (radio_slot == "B") {
            return fs_path("dboards") / "A";
        }
        if (radio_slot == "D") {
            return fs_path("dboards") / "C";
        }
        UHD_THROW_INVALID_CODE_PATH();
    }
}


/******************************************************************************
 * Structors
 *****************************************************************************/
UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR(magnesium_radio_ctrl)
{
    UHD_LOG_TRACE(unique_id(), "Entering magnesium_radio_ctrl_impl ctor...");
    UHD_LOG_DEBUG(unique_id(), "Note: Running in one-block-per-channel mode!");
    const char radio_slot_name[4] = {'A', 'B', 'C', 'D'};
    _radio_slot = radio_slot_name[get_block_id().get_block_count()];
    UHD_LOG_TRACE(unique_id(), "Radio slot: " << _radio_slot);
    _master = _radio_slot == "A" or _radio_slot == "C";
    UHD_LOG_DEBUG(unique_id(),
        "Radio type: " << (_master ? "master" : "slave"));
    _rpc_prefix =
        (_radio_slot == "A" or _radio_slot == "B") ? "db_0_" : "db_1_";

    _init_defaults();
    _init_peripherals();
    _init_prop_tree();
}

magnesium_radio_ctrl_impl::~magnesium_radio_ctrl_impl()
{
    UHD_LOG_TRACE(unique_id(), "magnesium_radio_ctrl_impl::dtor() ");
}


/******************************************************************************
 * API Calls
 *****************************************************************************/
double magnesium_radio_ctrl_impl::set_rate(double rate)
{
    // TODO: implement
    if (rate != get_rate()) {
        UHD_LOG_WARNING(unique_id(),
                "Attempting to set sampling rate to invalid value " << rate);
    }
    return get_rate();
}

void magnesium_radio_ctrl_impl::set_tx_antenna(
        const std::string &ant,
        const size_t chan
) {
    if (ant != get_tx_antenna(chan)) {
        throw uhd::value_error(str(
            boost::format("[%s] Requesting invalid TX antenna value: %s")
            % unique_id()
            % ant
        ));
    }
    // We can't actually set the TX antenna, so let's stop here.
}

void magnesium_radio_ctrl_impl::set_rx_antenna(
        const std::string &ant,
        const size_t chan
) {
    UHD_ASSERT_THROW(chan <= MAGNESIUM_NUM_CHANS);
    if (std::find(MAGNESIUM_RX_ANTENNAS.begin(),
                  MAGNESIUM_RX_ANTENNAS.end(),
                  ant) == MAGNESIUM_RX_ANTENNAS.end()) {
        throw uhd::value_error(str(
            boost::format("[%s] Requesting invalid RX antenna value: %s")
            % unique_id()
            % ant
        ));
    }
    UHD_LOG_TRACE(unique_id(),
        "Setting RX antenna to " << ant << " for chan " << chan);
    magnesium_cpld_ctrl::chan_sel_t chan_sel  =
        _master ? magnesium_cpld_ctrl::CHAN1 : magnesium_cpld_ctrl::CHAN2;
    _update_atr_switches(chan_sel, RX_DIRECTION, ant);

    radio_ctrl_impl::set_rx_antenna(ant, chan);
}

double magnesium_radio_ctrl_impl::set_tx_frequency(
        const double freq,
        const size_t chan
) {
    // Note: There is only one LO per tx or TX, so changing frequency will
    // affect the adjacent channel in the same direction. We have to make sure
    // that getters will always tell the truth! This is true for low and high
    // bands.
    UHD_LOG_TRACE(unique_id(),
        "set_tx_frequency(f=" << freq << ", chan=" << chan << ")");
    if (not _master) {
        const fs_path master_tx_fe_path =
            master_fe_base_path(_radio_slot) / fs_path("tx_frontends") / chan;
        UHD_LOG_DEBUG(unique_id(),
                      "Slave setting TX frequency");
        _tree->access<double>(master_tx_fe_path / "freq" / "value").set(freq);
        return _tree->access<double>(master_tx_fe_path / "freq" / "value").get();
    }
    _update_tx_freq_switches(freq, _tx_bypass_amp, chan);
    //double ad9371_freq = freq;
    double if_freq = 0.0;
    auto lo_iface = _tx_lo;

    if (freq < MAGNESIUM_LOWBAND_FREQ) { // Low band
        if_freq = MAGNESIUM_TX_IF_FREQ ;
        const double lo_freq = if_freq - freq;
        const bool int_n_mode = false; // FIXME no hardcode
        //const double actual_lo_freq =
            _lo_enable(lo_iface, lo_freq, _master_clock_rate, int_n_mode);
        //ad9371_freq = actual_lo_freq - freq;
    } else {
        _lo_disable(lo_iface);
    }

    //const double actual_ad9371_freq =
        _ad9371->set_frequency(freq, chan, TX_DIRECTION);
    radio_ctrl_impl::set_tx_frequency(freq, chan);
    return freq; // FIXME calc the actual frequency
}

double magnesium_radio_ctrl_impl::set_rx_frequency(
        const double freq,
        const size_t chan
) {
    // Note: There is only one LO per RX or TX, so changing frequency will
    // affect the adjacent channel in the same direction. We have to make sure
    // that getters will always tell the truth! This is true for low and high
    // bands.
    UHD_LOG_TRACE(unique_id(),
        "set_rx_frequency(f=" << freq << ", chan=" << chan << ")");
    if (not _master) {
        const fs_path master_rx_fe_path =
            master_fe_base_path(_radio_slot) / fs_path("rx_frontends") / chan;
        UHD_LOG_DEBUG(unique_id(),
                      "Slave setting RX frequency");
        _tree->access<double>(master_rx_fe_path / "freq" / "value").set(freq);
        return _tree->access<double>(master_rx_fe_path / "freq" / "value").get();
    }
    _update_rx_freq_switches(freq, _rx_bypass_lnas, chan);
    //double ad9371_freq = freq;
    double if_freq = 0.0;
    auto lo_iface = _rx_lo;

    if (freq < MAGNESIUM_LOWBAND_FREQ) { // Low band
        if_freq = MAGNESIUM_RX_IF_FREQ ;
        const double lo_freq = if_freq - freq;
        const bool int_n_mode = false; // FIXME no hardcode
        //const double actual_lo_freq =
            _lo_enable(lo_iface, lo_freq, _master_clock_rate, int_n_mode);
        //ad9371_freq = actual_lo_freq - freq;
    } else {
        _lo_disable(lo_iface);
    }

    //const double actual_ad9371_freq =
        _ad9371->set_frequency(freq, chan, RX_DIRECTION);
    radio_ctrl_impl::set_rx_frequency(freq, chan);
    return freq; // FIXME calc the actual frequency
}

double magnesium_radio_ctrl_impl::get_tx_frequency(
    const size_t chan)
{
    UHD_LOG_TRACE(unique_id(),
                  "get_tx_frequency(chan=" << chan << ")");
    if (not _master) {
        const fs_path master_tx_fe_path =
            master_fe_base_path(_radio_slot) / fs_path("tx_frontends") / chan;
        UHD_LOG_TRACE(unique_id(), "Slave getting TX frequency");
        return _tree->access<double>(master_tx_fe_path / "freq" / "value").get();
    }
    return radio_ctrl_impl::get_tx_frequency(chan);
}

double magnesium_radio_ctrl_impl::get_rx_frequency(
    const size_t chan)
{
    UHD_LOG_TRACE(unique_id(),
                  "get_rx_frequency(chan=" << chan << ")");
    if (not _master) {
        const fs_path master_rx_fe_path =
            master_fe_base_path(_radio_slot) / fs_path("rx_frontends") / chan;
        UHD_LOG_TRACE(unique_id(), "Slave getting RX frequency");
        return _tree->access<double>(master_rx_fe_path / "freq" / "value").get();
    }
    return radio_ctrl_impl::get_rx_frequency(chan);
}

double magnesium_radio_ctrl_impl::set_rx_bandwidth(
        const double bandwidth,
        const size_t chan
) {
    radio_ctrl_impl::set_rx_bandwidth(bandwidth, chan);
    return _ad9371->set_bandwidth(bandwidth, chan, RX_DIRECTION);
}

double magnesium_radio_ctrl_impl::set_tx_bandwidth(
        const double bandwidth,
        const size_t chan
) {
    //radio_ctrl_impl::set_rx_bandwidth(bandwidth, chan);
    return _ad9371->set_bandwidth(bandwidth, chan, TX_DIRECTION);
}

double magnesium_radio_ctrl_impl::set_tx_gain(
        const double gain,
        const size_t chan
) {
    radio_ctrl_impl::set_tx_gain(gain, chan);
    return _set_all_gain(
        gain,
        radio_ctrl_impl::get_tx_frequency(chan),
        chan,
        TX_DIRECTION
    );
}

double magnesium_radio_ctrl_impl::set_rx_gain(
        const double gain,
        const size_t chan
) {
    UHD_LOG_TRACE(unique_id(),
        "set_rx_gain(gain=" << gain << ", chan=" << chan << ")");
    radio_ctrl_impl::set_rx_gain(gain, chan);
    return _set_all_gain(
        gain,
        radio_ctrl_impl::get_rx_frequency(chan),
        chan,
        RX_DIRECTION
    );
}

std::vector<std::string> magnesium_radio_ctrl_impl::get_rx_lo_names(
        const size_t /*chan*/
) {
    return std::vector<std::string>{};
}

std::vector<std::string> magnesium_radio_ctrl_impl::get_rx_lo_sources(
        const std::string &/*name*/,
        const size_t /*chan*/
) {
    return std::vector<std::string>{};
}

freq_range_t magnesium_radio_ctrl_impl::get_rx_lo_freq_range(
        const std::string &/*name*/,
        const size_t /*chan*/
) {
    return freq_range_t{};
}

void magnesium_radio_ctrl_impl::set_rx_lo_source(
        const std::string &/*src*/,
        const std::string &/*name*/,
        const size_t /*chan*/
) {
    // FIXME
}

const std::string magnesium_radio_ctrl_impl::get_rx_lo_source(
        const std::string &/*name*/,
        const size_t /*chan*/
) {
    return ""; // FIXME
}

double magnesium_radio_ctrl_impl::set_rx_lo_freq(
        double /*freq*/,
        const std::string &/*name*/,
        const size_t /*chan*/
) {
    return 0.0; // FIXME
}

double magnesium_radio_ctrl_impl::get_rx_lo_freq(
        const std::string &/*name*/,
        const size_t /*chan*/
) {
    return 0.0; // FIXME
}

size_t magnesium_radio_ctrl_impl::get_chan_from_dboard_fe(
    const std::string &fe, const direction_t /* dir */
) {
    return boost::lexical_cast<size_t>(fe);
}

std::string magnesium_radio_ctrl_impl::get_dboard_fe_from_chan(
    const size_t chan,
    const direction_t /* dir */
) {
    return std::to_string(chan);
}

double magnesium_radio_ctrl_impl::get_output_samp_rate(size_t /* port */)
{
    return MAGNESIUM_RADIO_RATE;
}

void magnesium_radio_ctrl_impl::set_rpc_client(
    uhd::rpc_client::sptr rpcc,
    const uhd::device_addr_t &block_args
) {
    _rpcc = rpcc;
    _block_args = block_args;
    UHD_LOG_TRACE(unique_id(), "Instantiating AD9371 control object...");
    _ad9371 = magnesium_ad9371_iface::uptr(
        new magnesium_ad9371_iface(
            _rpcc,
            (_radio_slot == "A" or _radio_slot == "B") ? 0 : 1
        )
    );

    // Note: MCR gets set during the init() call (prior to this), which takes
    // in arguments from the device args. So if block_args contains a
    // master_clock_rate key, then it should better be whatever the device is
    // configured to do.
    _master_clock_rate = _rpcc->request_with_token<double>(
            _rpc_prefix + "get_master_clock_rate");
    if (block_args.cast<double>("master_clock_rate", _master_clock_rate)
            != _master_clock_rate) {
        throw uhd::runtime_error(str(
            boost::format("Master clock rate mismatch. Device returns %f MHz, "
                          "but should have been %f MHz.")
            % (_master_clock_rate / 1e6)
            % (block_args.cast<double>(
                    "master_clock_rate", _master_clock_rate) / 1e6)
        ));
    }
    UHD_LOG_DEBUG(unique_id(),
        "Master Clock Rate is: " << (_master_clock_rate / 1e6) << " MHz.");
    radio_ctrl_impl::set_rate(_master_clock_rate);

    // EEPROM paths subject to change FIXME
    const size_t db_idx = get_block_id().get_block_count();
    _tree->access<eeprom_map_t>(_root_path / "eeprom")
        .add_coerced_subscriber([this, db_idx](const eeprom_map_t& db_eeprom){
            this->_rpcc->notify_with_token("set_db_eeprom", db_idx, db_eeprom);
        })
        .set_publisher([this, db_idx](){
            return this->_rpcc->request_with_token<eeprom_map_t>(
                "get_db_eeprom", db_idx
            );
        })
    ;

    // Init sensors
    for (const auto &dir : std::vector<direction_t>{RX_DIRECTION, TX_DIRECTION}) {
        for (size_t chan_idx = 0; chan_idx < 1 /* num channels FIXME */; chan_idx++) {
            _init_mpm_sensors(dir, chan_idx);
        }
    }
}

bool magnesium_radio_ctrl_impl::get_lo_lock_status(
    const direction_t dir
) {
    if (not (bool(_rpcc))) {
        UHD_LOG_DEBUG(unique_id(),
            "Reported no LO lock due to lack of RPC connection.");
        return false;
    }

    const std::string trx = (dir == RX_DIRECTION) ? "rx" : "tx";
    const size_t chan = 0; // They're the same after all
    const double freq = (dir == RX_DIRECTION) ?
        get_rx_frequency(chan) :
        get_tx_frequency(chan);

    bool lo_lock = _rpcc->request_with_token<bool>(
        _rpc_prefix + "get_ad9371_lo_lock", trx);
    UHD_LOG_TRACE(unique_id(),
        "AD9371 " << trx << " LO reports lock: " << (lo_lock ? "Yes" : "No"));
    if (lo_lock && freq < MAGNESIUM_LOWBAND_FREQ) {
        lo_lock = lo_lock && _rpcc->request_with_token<bool>(
            _rpc_prefix + "get_lowband_lo_lock", trx);
        UHD_LOG_TRACE(unique_id(),
            "ADF4351 " << trx << " LO reports lock: "
            << (lo_lock ? "Yes" : "No"));
    }

    return lo_lock;
}

UHD_RFNOC_BLOCK_REGISTER(magnesium_radio_ctrl, "MagnesiumRadio");

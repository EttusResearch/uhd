//
// Copyright 2014-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/format.hpp>
#include <uhd/convert.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/direction.hpp>
#include <uhdlib/rfnoc/wb_iface_adapter.hpp>
#include <uhdlib/rfnoc/radio_ctrl_impl.hpp>
#include "../../transport/super_recv_packet_handler.hpp"
#include <tuple>

using namespace uhd;
using namespace uhd::rfnoc;

static const size_t BYTES_PER_SAMPLE = 4;
const std::string radio_ctrl::ALL_LOS = "all";

/****************************************************************************
 * Structors and init
 ***************************************************************************/
// Note: block_ctrl_base must be called before this, but has to be called by
// the derived class because of virtual inheritance
radio_ctrl_impl::radio_ctrl_impl() :
    _tick_rate(rfnoc::rate_node_ctrl::RATE_UNDEFINED)
{
    _num_rx_channels = get_output_ports().size();
    _num_tx_channels = get_input_ports().size();
    _continuous_streaming = std::vector<bool>(2, false);

    for (size_t i = 0; i < _num_rx_channels; i++) {
        _rx_streamer_active[i] = false;
    }
    for (size_t i = 0; i < _num_tx_channels; i++) {
        _tx_streamer_active[i] = false;
    }

    /////////////////////////////////////////////////////////////////////////
    // Setup peripherals
    /////////////////////////////////////////////////////////////////////////
    for (size_t i = 0; i < _get_num_radios(); i++) {
        _register_loopback_self_test(i);
        _perifs[i].ctrl = this->get_ctrl_iface(i);
        // FIXME there's currently no way to set the underflow policy

        if (i == 0) {
            time_core_3000::readback_bases_type time64_rb_bases;
            time64_rb_bases.rb_now = regs::rb_addr(regs::RB_TIME_NOW);
            time64_rb_bases.rb_pps = regs::rb_addr(regs::RB_TIME_PPS);
            _time64 = time_core_3000::make(_perifs[i].ctrl, regs::sr_addr(regs::TIME), time64_rb_bases);
            this->set_time_now(0.0);
        }

        //Reset the RX control engine
        sr_write(regs::RX_CTRL_HALT, 1, i);
    }

    ////////////////////////////////////////////////////////////////////
    // Register the time keeper
    ////////////////////////////////////////////////////////////////////
    if (not _tree->exists(fs_path("time") / "now")) {
        _tree->create<time_spec_t>(fs_path("time") / "now")
            .set_publisher([this](){ return this->get_time_now(); })
        ;
    }
    if (not _tree->exists(fs_path("time") / "pps")) {
        _tree->create<time_spec_t>(fs_path("time") / "pps")
            .set_publisher([this](){ return this->get_time_last_pps(); })
        ;
    }
    if (not _tree->exists(fs_path("time") / "cmd")) {
        _tree->create<time_spec_t>(fs_path("time") / "cmd");
    }
    _tree->access<time_spec_t>(fs_path("time") / "now")
        .add_coerced_subscriber([this](const time_spec_t& time_spec){
            this->set_time_now(time_spec);
        })
    ;
    _tree->access<time_spec_t>(fs_path("time") / "pps")
        .add_coerced_subscriber([this](const time_spec_t& time_spec){
            this->set_time_next_pps(time_spec);
        })
    ;
    for (size_t i = 0; i < _get_num_radios(); i++) {
        _tree->access<time_spec_t>("time/cmd")
            .add_coerced_subscriber([this, i](const time_spec_t& time_spec){
                this->set_command_tick_rate(this->_tick_rate, i);
                this->set_command_time(time_spec, i);
            })
        ;
    }
    // spp gets created in the XML file
    _tree->access<int>(get_arg_path("spp") / "value")
        .add_coerced_subscriber([this](const int spp){
            this->_update_spp(spp);
        })
        .update()
    ;
}

void radio_ctrl_impl::_register_loopback_self_test(size_t chan)
{
    size_t hash = size_t(time(NULL));
    for (size_t i = 0; i < 100; i++)
    {
        boost::hash_combine(hash, i);
        sr_write(regs::TEST, uint32_t(hash), chan);
        uint32_t result = user_reg_read32(regs::RB_TEST, chan);
        if (result != uint32_t(hash)) {
            UHD_LOGGER_ERROR("RFNOC RADIO") << "Register loopback test failed";
            UHD_LOGGER_ERROR("RFNOC RADIO")
                << boost::format("expected: %x result: %x")
                   % uint32_t(hash) % result
            ;
            return; // exit on any failure
        }
    }
    UHD_LOG_DEBUG(unique_id(), "Register loopback test passed");
}


/****************************************************************************
 * API calls
 ***************************************************************************/
double radio_ctrl_impl::set_rate(double rate)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _tick_rate = rate;
    _time64->set_tick_rate(_tick_rate);
    _time64->self_test();
    set_command_tick_rate(rate);
    return _tick_rate;
}

void radio_ctrl_impl::set_tx_antenna(const std::string &ant, const size_t chan)
{
    _tx_antenna[chan] = ant;
}

void radio_ctrl_impl::set_rx_antenna(const std::string &ant, const size_t chan)
{
    _rx_antenna[chan] = ant;
}

double radio_ctrl_impl::set_tx_frequency(const double freq, const size_t chan)
{
    return _tx_freq[chan] = freq;
}

double radio_ctrl_impl::set_rx_frequency(const double freq, const size_t chan)
{
    return _rx_freq[chan] = freq;
}

double radio_ctrl_impl::set_tx_gain(const double gain, const size_t chan)
{
    return _tx_gain[chan] = gain;
}

double radio_ctrl_impl::set_rx_gain(const double gain, const size_t chan)
{
    return _rx_gain[chan] = gain;
}

double radio_ctrl_impl::set_tx_bandwidth(
        const double bandwidth,
        const size_t chan
) {
    return _tx_bandwidth[chan] = bandwidth;
}

double radio_ctrl_impl::set_rx_bandwidth(const double bandwidth, const size_t chan)
{
    return _rx_bandwidth[chan] = bandwidth;
}

void radio_ctrl_impl::set_time_sync(const uhd::time_spec_t &time)
{
    _time64->set_time_sync(time);
}

double radio_ctrl_impl::get_rate() const
{
    return _tick_rate;
}

std::string radio_ctrl_impl::get_tx_antenna(const size_t chan) /* const */
{
    return _tx_antenna[chan];
}

std::string radio_ctrl_impl::get_rx_antenna(const size_t chan) /* const */
{
    return _rx_antenna[chan];
}

double radio_ctrl_impl::get_tx_frequency(const size_t chan) /* const */
{
    return _tx_freq[chan];
}

double radio_ctrl_impl::get_rx_frequency(const size_t chan) /* const */
{
    return _rx_freq[chan];
}

double radio_ctrl_impl::get_tx_gain(const size_t chan) /* const */
{
    return _tx_gain[chan];
}

double radio_ctrl_impl::get_rx_gain(const size_t chan) /* const */
{
    return _rx_gain[chan];
}

double radio_ctrl_impl::get_tx_bandwidth(const size_t chan) /* const */
{
    return _tx_bandwidth[chan];
}

double radio_ctrl_impl::get_rx_bandwidth(const size_t chan) /* const */
{
    return _rx_bandwidth[chan];
}

/******************************************************************************
 * LO controls
 *****************************************************************************/
std::vector<std::string> radio_ctrl_impl::get_rx_lo_names(const size_t /* chan */)
{
    return std::vector<std::string>();
}

std::vector<std::string> radio_ctrl_impl::get_rx_lo_sources(const std::string & /* name */, const size_t /* chan */)
{
    return std::vector<std::string>();
}

freq_range_t radio_ctrl_impl::get_rx_lo_freq_range(const std::string & /* name */, const size_t /* chan */)
{
    return freq_range_t();
}

void radio_ctrl_impl::set_rx_lo_source(const std::string & /* src */, const std::string & /* name */, const size_t /* chan */)
{
    throw uhd::not_implemented_error("set_rx_lo_source is not supported on this radio");
}

const std::string radio_ctrl_impl::get_rx_lo_source(const std::string & /* name */, const size_t /* chan */)
{
    return "internal";
}

void radio_ctrl_impl::set_rx_lo_export_enabled(bool /* enabled */, const std::string & /* name */, const size_t /* chan */)
{
    throw uhd::not_implemented_error("set_rx_lo_export_enabled is not supported on this radio");
}

bool radio_ctrl_impl::get_rx_lo_export_enabled(const std::string & /* name */, const size_t /* chan */)
{
    return false; // Not exporting non-existant LOs
}

double radio_ctrl_impl::set_rx_lo_freq(double /* freq */, const std::string & /* name */, const size_t /* chan */)
{
    throw uhd::not_implemented_error("set_rx_lo_freq is not supported on this radio");
}

double radio_ctrl_impl::get_rx_lo_freq(const std::string & /* name */, const size_t /* chan */)
{
    return 0;
}

std::vector<std::string> radio_ctrl_impl::get_tx_lo_names(
        const size_t /* chan */
) {
    return std::vector<std::string>();
}

std::vector<std::string> radio_ctrl_impl::get_tx_lo_sources(
        const std::string & /* name */,
        const size_t /* chan */
) {
    return std::vector<std::string>();
}

freq_range_t radio_ctrl_impl::get_tx_lo_freq_range(
        const std::string & /* name */,
        const size_t /* chan */
) {
    return freq_range_t();
}

void radio_ctrl_impl::set_tx_lo_source(
        const std::string & /* src */,
        const std::string & /* name */,
        const size_t /* chan */
) {
    throw uhd::not_implemented_error("set_tx_lo_source is not supported on this radio");
}

const std::string radio_ctrl_impl::get_tx_lo_source(
        const std::string & /* name */,
        const size_t /* chan */
) {
    return "internal";
}

void radio_ctrl_impl::set_tx_lo_export_enabled(
        const bool /* enabled */,
        const std::string & /* name */,
        const size_t /* chan */
) {
    throw uhd::not_implemented_error("set_tx_lo_export_enabled is not supported on this radio");
}

bool radio_ctrl_impl::get_tx_lo_export_enabled(
        const std::string & /* name */,
        const size_t /* chan */
) {
    return false; // Not exporting non-existant LOs
}

double radio_ctrl_impl::set_tx_lo_freq(
        const double /* freq */,
        const std::string & /* name */,
        const size_t /* chan */
) {
    throw uhd::not_implemented_error(
            "set_tx_lo_freq is not supported on this radio");
}

double radio_ctrl_impl::get_tx_lo_freq(
        const std::string & /* name */,
        const size_t chan
) {
    return get_tx_frequency(chan);
}

/***********************************************************************
 * RX Streamer-related methods (from source_block_ctrl_base)
 **********************************************************************/
//! Pass stream commands to the radio
void radio_ctrl_impl::issue_stream_cmd(
    const uhd::stream_cmd_t &stream_cmd,
    const size_t chan
) {
    std::lock_guard<std::mutex> lock(_mutex);
    UHD_RFNOC_BLOCK_TRACE()
        << "radio_ctrl_impl::issue_stream_cmd() " << chan
        << " " << char(stream_cmd.stream_mode) ;
    if (not _is_streamer_active(uhd::RX_DIRECTION, chan)) {
        UHD_RFNOC_BLOCK_TRACE()
            << "radio_ctrl_impl::issue_stream_cmd() called on inactive "
               "channel. Skipping.";
        return;
    }
    constexpr size_t max_num_samps = 0x0fffffff;
    if (stream_cmd.num_samps > max_num_samps) {
        UHD_LOG_ERROR("RFNOC RADIO",
            "Requesting too many samples in a single burst! "
            "Requested " + std::to_string(stream_cmd.num_samps) + ", maximum "
            "is " + std::to_string(max_num_samps) + ".");
        UHD_LOG_INFO("RFNOC RADIO",
            "Note that a decimation block will increase the number of samples "
            "per burst by the decimation factor. Your application may have "
            "requested fewer samples.");
        throw uhd::value_error("Requested too many samples in a single burst.");
    }
    _continuous_streaming[chan] =
        (stream_cmd.stream_mode == stream_cmd_t::STREAM_MODE_START_CONTINUOUS);

    //setup the mode to instruction flags
    typedef std::tuple<bool, bool, bool, bool> inst_t;
    static const std::map<stream_cmd_t::stream_mode_t, inst_t> mode_to_inst{
                                                            //reload, chain, samps, stop
        {stream_cmd_t::STREAM_MODE_START_CONTINUOUS,   inst_t(true,  true,  false, false)},
        {stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS,    inst_t(false, false, false, true)},
        {stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE, inst_t(false, false, true,  false)},
        {stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE, inst_t(false, true,  true,  false)}
    }
    ;

    //setup the instruction flag values
    bool inst_reload, inst_chain, inst_samps, inst_stop;
    std::tie(inst_reload, inst_chain, inst_samps, inst_stop) =
        mode_to_inst.at(stream_cmd.stream_mode);

    //calculate the word from flags and length
    const uint32_t cmd_word = 0
        | (uint32_t((stream_cmd.stream_now)? 1 : 0) << 31)
        | (uint32_t((inst_chain)?            1 : 0) << 30)
        | (uint32_t((inst_reload)?           1 : 0) << 29)
        | (uint32_t((inst_stop)?             1 : 0) << 28)
        | ((inst_samps) ? stream_cmd.num_samps : ((inst_stop)? 0 : 1));

    //issue the stream command
    const uint64_t ticks =
        (stream_cmd.stream_now)? 0 : stream_cmd.time_spec.to_ticks(get_rate());
    sr_write(regs::RX_CTRL_CMD, cmd_word, chan);
    sr_write(regs::RX_CTRL_TIME_HI, uint32_t(ticks >> 32), chan);
    sr_write(regs::RX_CTRL_TIME_LO, uint32_t(ticks >> 0),  chan); //latches the command
}

std::vector<size_t> radio_ctrl_impl::get_active_rx_ports()
{
    std::vector<size_t> active_rx_ports;
    typedef std::map<size_t, bool> map_t;
    for(map_t::value_type &m:  _rx_streamer_active) {
        if (m.second) {
            active_rx_ports.push_back(m.first);
        }
    }
    return active_rx_ports;
}

/***********************************************************************
 * Radio controls (radio_ctrl specific)
 **********************************************************************/
void radio_ctrl_impl::set_rx_streamer(bool active, const size_t port)
{
    UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl_impl::set_rx_streamer() " << port << " -> " << active ;
    if (port > _num_rx_channels) {
        throw uhd::value_error(str(
            boost::format("[%s] Can't (un)register RX streamer on port %d (invalid port)")
            % unique_id() % port
        ));
    }
    _rx_streamer_active[port] = active;
    if (not check_radio_config()) {
        throw std::runtime_error(str(
            boost::format("[%s]: Invalid radio configuration.")
            % unique_id()
        ));
    }
}

void radio_ctrl_impl::set_tx_streamer(bool active, const size_t port)
{
    UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl_impl::set_tx_streamer() " << port << " -> " << active ;
    if (port > _num_tx_channels) {
        throw uhd::value_error(str(
            boost::format("[%s] Can't (un)register TX streamer on port %d (invalid port)")
            % unique_id() % port
        ));
    }
    _tx_streamer_active[port] = active;
    if (not check_radio_config()) {
        throw std::runtime_error(str(
            boost::format("[%s]: Invalid radio configuration.")
            % unique_id()
        ));
    }
}

// Subscribers to block args:
// TODO move to nocscript
void radio_ctrl_impl::_update_spp(int spp)
{
    std::lock_guard<std::mutex> lock(_mutex);
    UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl_impl::_update_spp(): Requested spp: " << spp ;
    if (spp == 0) {
        spp = DEFAULT_PACKET_SIZE / BYTES_PER_SAMPLE;
    }
    UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl_impl::_update_spp(): Setting spp to: " << spp ;
    for (size_t i = 0; i < _num_rx_channels; i++) {
        sr_write(regs::RX_CTRL_MAXLEN, uint32_t(spp), i);
    }
}

void radio_ctrl_impl::set_time_now(const time_spec_t &time_spec)
{
    _time64->set_time_now(time_spec);
}

void radio_ctrl_impl::set_time_next_pps(const time_spec_t &time_spec)
{
    _time64->set_time_next_pps(time_spec);
}

time_spec_t radio_ctrl_impl::get_time_now()
{
    return _time64->get_time_now();
}

time_spec_t radio_ctrl_impl::get_time_last_pps()
{
    return _time64->get_time_last_pps();
}

void radio_ctrl_impl::set_time_source(const std::string &source)
{
    _tree->access<std::string>("time_source/value").set(source);
}

std::string radio_ctrl_impl::get_time_source()
{
    return _tree->access<std::string>("time_source/value").get();
}

std::vector<std::string> radio_ctrl_impl::get_time_sources()
{
    return _tree->access<std::vector<std::string>>("time_source/options").get();
}

void radio_ctrl_impl::set_clock_source(const std::string &source)
{
    _tree->access<std::string>("clock_source/value").set(source);
}

std::string radio_ctrl_impl::get_clock_source()
{
    return _tree->access<std::string>("clock_source/value").get();
}

std::vector<std::string> radio_ctrl_impl::get_clock_sources()
{
    return _tree->access<std::vector<std::string>>("clock_source/options").get();
}


std::vector<std::string> radio_ctrl_impl::get_gpio_banks() const
{
    return std::vector<std::string>();
}

void radio_ctrl_impl::set_gpio_attr(
        const std::string &,
        const std::string &,
        const uint32_t,
        const uint32_t
) {
    throw uhd::not_implemented_error("set_gpio_attr was not defined for this radio");
}

uint32_t radio_ctrl_impl::get_gpio_attr(const std::string &, const std::string &)
{
    throw uhd::not_implemented_error("get_gpio_attr was not defined for this radio");
}

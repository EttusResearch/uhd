//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "eiscat_radio_ctrl_impl.hpp"

#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <uhd/types/ranges.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/format.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;

namespace {
    const size_t SR_ANTENNA_GAIN_BASE     = 204;
    const size_t SR_ANTENNA_SELECT_BASE   = 192; // Note: On other dboards, 192 is DB_GPIO address space
    const size_t RB_CHOOSE_BEAMS          = 11;

    const double EISCAT_TICK_RATE         = 208e6; // Hz
    const double EISCAT_RADIO_RATE        = 104e6; // Hz
    const double EISCAT_CENTER_FREQ       = 208e6; // Hz
    const double EISCAT_DEFAULT_NULL_GAIN = 0.0; // dB. This is not the digital antenna gain, this a fake stub value.
    const double EISCAT_DEFAULT_BANDWIDTH = 104e6; // Hz
    const char*  EISCAT_DEFAULT_ANTENNA   = "BF";
    const size_t EISCAT_NUM_ANTENNAS      = 16;
    const size_t EISCAT_NUM_BEAMS         = 10;
    const size_t EISCAT_NUM_PORTS         = 5;
    const size_t EISCAT_MAX_GAIN_RANGE    = 18; // Bits, *signed*.
    const size_t EISCAT_UNIT_GAIN_RANGE   = 14; // Bits, *signed*.
    const int32_t EISCAT_MAX_GAIN         = (1<<(EISCAT_MAX_GAIN_RANGE-1))-1;
    const int32_t EISCAT_UNIT_GAIN        = (1<<(EISCAT_UNIT_GAIN_RANGE-1))-1;
    const int32_t EISCAT_MIN_GAIN         = -(1<<(EISCAT_MAX_GAIN_RANGE-1));
    const double EISCAT_DEFAULT_NORM_GAIN = 1.0; // Normalized. This is the actual digital gain value.
    const size_t EISCAT_BITS_PER_TAP      = 18;
    const eiscat_radio_ctrl_impl::fir_tap_t EISCAT_MAX_TAP_VALUE  = (1<<(EISCAT_BITS_PER_TAP-1))-1;
    const eiscat_radio_ctrl_impl::fir_tap_t EISCAT_MIN_TAP_VALUE  = -(1<<(EISCAT_BITS_PER_TAP-1));
    const size_t EISCAT_NUM_FIR_TAPS      = 10;
    const size_t EISCAT_NUM_FIR_SETS      = 1024; // BRAM must be at least EISCAT_NUM_FIR_TAPS * EISCAT_NUM_FIR_SETS
    const size_t EISCAT_FIR_INDEX_IMPULSE = 1002;
    const size_t EISCAT_FIR_INDEX_ZEROS   = 1003;

    const uint32_t EISCAT_CONTRIB_LOWER   = 0<<0;
    const uint32_t EISCAT_CONTRIB_UPPER   = 1<<0;
    const uint32_t EISCAT_SKIP_NEIGHBOURS = 1<<1;
    const uint32_t EISCAT_BYPASS_MATRIX   = 1<<2;
    const uint32_t EISCAT_OUTPUT_COUNTER  = 1<<3;
};


UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR(eiscat_radio_ctrl)
{
    UHD_LOG_TRACE("EISCAT", "eiscat_radio_ctrl_impl::ctor() ");
    _num_ports = get_output_ports().size();
    UHD_LOG_TRACE("EISCAT", "Number of channels: " << _num_ports);
    UHD_LOG_TRACE("EISCAT",
        "Tick rate is " << EISCAT_TICK_RATE/1e6 << " MHz"
    );

    /**** Configure the radio_ctrl itself ***********************************/
    // This also sets the command tick rate:
    radio_ctrl_impl::set_rate(EISCAT_TICK_RATE);
    for (size_t chan = 0; chan < _num_ports; chan++) {
        radio_ctrl_impl::set_rx_frequency(EISCAT_CENTER_FREQ, chan);
        radio_ctrl_impl::set_rx_gain(EISCAT_DEFAULT_NULL_GAIN, chan);
        radio_ctrl_impl::set_rx_antenna(EISCAT_DEFAULT_ANTENNA, chan);
        radio_ctrl_impl::set_rx_bandwidth(EISCAT_DEFAULT_BANDWIDTH, chan);
        // We might get tx async messages from upstream radios, we send them to the
        // nevernever by default or they interfere with our streamers or ctrl_iface
        // objects. The assumption is that FF:FF is never a valid SID.
        this->sr_write(uhd::rfnoc::SR_RESP_IN_DST_SID, 0xFFFF, chan);
    }

    /**** Set up arg-based control API **************************************/
    // None of these properties are defined in the XML file. Some of them have
    // non-Noc-Script-compatible types.
    _tree->create<bool>(get_arg_path("sysref", 0) / "value")
        .set(true)
        .add_coerced_subscriber([this](bool){
            try {
                this->send_sysref();
            } catch (const uhd::exception &ex) {
                UHD_LOGGER_WARNING("EISCAT")
                    << "Failed to send SYSREF: " << ex.what();
                throw uhd::runtime_error(str(
                    boost::format("Failed to send SYSREF: %s")
                    % ex.what()
                ));
            }
        })
        .set_publisher([](){ return true; })
    ;
    _tree->create<bool>(get_arg_path("assert_adcs_deframers", 0) / "value")
        .set(true)
        .set_publisher([this](){ return this->assert_adcs_deframers(); })
    ;
    _tree->create<bool>(get_arg_path("assert_deframer_status", 0) / "value")
        .set(true)
        .set_publisher([this](){ return this->assert_adcs_deframers(); })
    ;
    _tree->create<time_spec_t>(get_arg_path("fir_ctrl_time", 0) / "value")
        .add_coerced_subscriber([this](time_spec_t switch_time){
            this->set_fir_ctrl_time(switch_time);
        })
        .set(time_spec_t(0.0))
    ;
    for (size_t beam = 0; beam < EISCAT_NUM_BEAMS; beam++) {
        for (size_t ant = 0; ant < EISCAT_NUM_ANTENNAS; ant++) {
            const size_t fir_index = beam * EISCAT_NUM_ANTENNAS + ant;
            // These are not in the XML file
            _tree->create<int>(get_arg_path("fir_select", fir_index) / "value")
                .add_coerced_subscriber([beam, ant, this](const size_t ram_idx){
                    UHD_ASSERT_THROW(ram_idx < EISCAT_NUM_FIR_SETS);
                    this->select_filter(
                        beam,
                        ant,
                        ram_idx,
                        this->get_arg<time_spec_t>("fir_ctrl_time", 0),
                        false
                    );
                })
            ;
        }
    }
    for (size_t fir_set = 0; fir_set < EISCAT_NUM_FIR_SETS; fir_set++) {
        _tree->create<std::vector<fir_tap_t>>(
                                get_arg_path("fir_taps", fir_set) / "value")
            .add_coerced_subscriber(
                [this, fir_set](const std::vector<fir_tap_t> &taps){
                    this->write_fir_taps(fir_set, taps);
                }
            )
        ;
    }


    /**** Add subscribers for our special properties ************************/
    // The difference between this block and the previous that these *are*
    // defined in the XML file, and can have defaults set there.
    _tree->access<int>(get_arg_path("choose_beams", 0) / "value")
        .add_coerced_subscriber([this](int choose_beams){
            this->set_beam_selection(choose_beams);
        })
        .update()
    ;
    _tree->access<bool>(get_arg_path("enable_firs", 0) / "value")
        .add_coerced_subscriber([this](int enable){
            this->enable_firs(bool(enable));
        })
        .update()
    ;
    _tree->access<bool>(get_arg_path("enable_counter", 0) / "value")
        .add_coerced_subscriber([this](int enable){
            this->enable_counter(bool(enable));
        })
        .update()
    ;
    _tree->access<int>(get_arg_path("configure_beams", 0) / "value")
        .add_coerced_subscriber([this](int reg_value){
            this->configure_beams(uint32_t(reg_value));
        }) // No update! This would override the previous settings.
        .set_publisher([this](){
            return this->user_reg_read32(RB_CHOOSE_BEAMS);
        })
    ;

    /**** Configure the digital gain controls *******************************/
    for (size_t i = 0; i < EISCAT_NUM_ANTENNAS; i++) {
        _tree->access<double>(get_arg_path("gain", i) / "value")
            .set_coercer([](double gain){
                return std::max(-16.0, std::min(16.0, gain));
            })
            .add_coerced_subscriber([this, i](double gain){
                this->set_antenna_gain(i, gain);
            })
            .update()
        ;
    }

    /**** Set up legacy compatible properties *******************************/
    // For use with multi_usrp APIs etc.
    // For legacy prop tree init:
    fs_path fe_path = fs_path("dboards") / "A" / "rx_frontends";

    // The EISCAT dboards have 16 frontends total, but they map to 10 beams
    // each through a matrix of FIR filters and summations, and then only 5 of
    // those channels go out through the Noc-Shell.
    // UHD will thus get much less confused if we create 5 fake frontends (i.e.,
    // number of Noc-Block-ports). Since we have no control over the frontends,
    // nothing is lost here.
    for (size_t fe_idx = 0; fe_idx < _num_ports; fe_idx++) {
        _tree->create<std::string>(fe_path / fe_idx / "name")
            .set(str(boost::format("EISCAT Beam Contributions %d") % fe_idx))
        ;
        _tree->create<std::string>(fe_path / fe_idx / "connection")
            .set("I")
        ;
        _tree->create<double>(fe_path / fe_idx / "freq" / "value")
            .set_coercer([this](const double freq){
                return this->set_rx_frequency(freq, 0);
            })
            .set_publisher([this](){
                return this->get_rx_frequency(0);
            })
        ;
        _tree->create<meta_range_t>(fe_path / fe_idx / "freq" / "range")
            .set(meta_range_t(EISCAT_CENTER_FREQ, EISCAT_CENTER_FREQ))
        ;
        _tree->create<double>(fe_path / fe_idx / "gains" / "null" / "value")
            .set_coercer([this](const double gain){
                return this->set_rx_gain(gain, 0);
            })
            .set_publisher([this](){
                return this->get_rx_gain(0);
            })
        ;
        _tree->create<meta_range_t>(fe_path / fe_idx / "gains" / "null" / "range")
            .set(meta_range_t(EISCAT_DEFAULT_NULL_GAIN, EISCAT_DEFAULT_NULL_GAIN))
        ;
        _tree->create<double>(fe_path / fe_idx / "bandwidth" / "value")
            .set_coercer([this](const double bw){
                return this->set_rx_bandwidth(bw, 0);
            })
            .set_publisher([this](){
                return this->get_rx_bandwidth(0);
            })
        ;
        _tree->create<meta_range_t>(fe_path / fe_idx / "bandwidth" / "range")
            .set(meta_range_t(EISCAT_DEFAULT_BANDWIDTH, EISCAT_DEFAULT_BANDWIDTH))
        ;
        _tree->create<bool>(fe_path / fe_idx / "use_lo_offset")
            .set(false)
        ;
    }

    auto antenna_options = std::vector<std::string>{"BF"};
    for (size_t i = 0; i < EISCAT_NUM_ANTENNAS; i++) {
        antenna_options.push_back(str(boost::format("Rx%d") % i));
        antenna_options.push_back(str(boost::format("BF%d") % i));
    }
    antenna_options.push_back("FI0");
    antenna_options.push_back("FI250");
    antenna_options.push_back("FI500");
    antenna_options.push_back("FI750");
    for (size_t beam_idx = 0; beam_idx < _num_ports; beam_idx++) {
        _tree->create<std::string>(fe_path / beam_idx / "antenna" / "value")
            .set(EISCAT_DEFAULT_ANTENNA)
            .add_coerced_subscriber([this, beam_idx](const std::string &name){
                this->set_rx_antenna(name, beam_idx);
            })
            .set_publisher([this, beam_idx](){
                return this->get_rx_antenna(beam_idx);
            })
        ;
        _tree->create<std::vector<std::string>>(
                fe_path / beam_idx / "antenna" / "options")
            .set(antenna_options)
        ;
    }

    // We can actually stream data to an EISCAT board, so it needs some tx
    // frontends too:
    fe_path = fs_path("dboards") / "A" / "tx_frontends";
    for (size_t fe_idx = 0; fe_idx < _num_ports; fe_idx++) {
        _tree->create<std::string>(fe_path / fe_idx / "name")
            .set(str(boost::format("EISCAT Uplink %d") % fe_idx))
        ;
    }

    for (size_t i = 0; i < EISCAT_NUM_PORTS; i++) {
        _tree->create<uhd::time_spec_t>(get_arg_path("pseudo_stream_cmd", i) / "value")
            .add_coerced_subscriber([this, i](uhd::time_spec_t stream_time){
                if (stream_time != uhd::time_spec_t(0.0)) {
                    uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
                    cmd.stream_now = false;
                    cmd.time_spec = stream_time;
                    this->issue_stream_cmd(cmd, i);
                } else {
                    this->issue_stream_cmd(
                        uhd::stream_cmd_t(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS),
                        i
                    );
                }
            })
        ;
    }
    //FIXME elaborate this more, but for now it works.
    _tree->create<int>("rx_codecs/A/gains");
    _tree->create<std::string>("rx_codecs/A/name").set("ADS54J66");


    // There is only ever one EISCAT radio per mboard, so this should be unset
    // when we reach this line:
    UHD_ASSERT_THROW(not _tree->exists("tick_rate"));
    _tree->create<double>("tick_rate")
        .set(EISCAT_TICK_RATE)
        .set_coercer(boost::bind(&eiscat_radio_ctrl_impl::set_rate, this, _1))
    ;
}

eiscat_radio_ctrl_impl::~eiscat_radio_ctrl_impl()
{
    UHD_LOG_TRACE("EISCAT", "eiscat_radio_ctrl_impl::dtor() ");
}


/****************************************************************************
 * Public API calls
 ***************************************************************************/
void eiscat_radio_ctrl_impl::set_tx_antenna(const std::string &, const size_t)
{
    throw uhd::runtime_error("Cannot set Tx antenna on EISCAT daughterboard");
}

void eiscat_radio_ctrl_impl::set_rx_antenna(
        const std::string &ant,
        const size_t port
) {
    UHD_ASSERT_THROW(port < EISCAT_NUM_BEAMS);
    if (ant == "BF") {
        UHD_LOG_TRACE("EISCAT", "Setting antenna to 'BF' (which is a no-op)");
        return;
    }
    if (ant.size() < 3) {
        throw uhd::value_error(str(
            boost::format("EISCAT: Invalid antenna selection: %s")
            % ant
        ));
    }

    const std::string ant_mode = ant.substr(0, 2);
    const size_t antenna_idx = [&ant](){
        try {
            return boost::lexical_cast<size_t>(ant.substr(2));
        } catch (const boost::bad_lexical_cast&) {
            throw uhd::value_error(str(
                boost::format("EISCAT: Invalid antenna selection: %s")
                % ant
            ));
        }
    }();

    if (ant_mode == "BF") {
        int new_choose_beams =
            get_arg<int>("choose_beams") | EISCAT_SKIP_NEIGHBOURS;
        set_arg<int>("choose_beams", new_choose_beams);
        size_t beam_select_offset =
                (get_arg<int>("choose_beams") & EISCAT_CONTRIB_UPPER) ?
                EISCAT_NUM_PORTS : 0;
        const size_t beam_index = port + beam_select_offset;
        uhd::time_spec_t send_now(0.0);
        UHD_LOG_TRACE("EISCAT", str(
            boost::format("Setting block port %d to only receive from beam %d "
                          "connected to antenna %d via FIR matrix")
            % port
            % beam_index
            % antenna_idx
        ));
        for (size_t i = 0; i < EISCAT_NUM_ANTENNAS; i++) {
            select_filter(
                beam_index,
                i,
                (i == antenna_idx) ?
                    EISCAT_FIR_INDEX_IMPULSE : EISCAT_FIR_INDEX_ZEROS,
                send_now
            );
        }
        enable_firs(true);
    } else if (ant_mode == "RX" or ant_mode == "Rx") {
        int new_choose_beams =
            get_arg<int>("choose_beams") | EISCAT_SKIP_NEIGHBOURS;
        set_arg<int>("choose_beams", new_choose_beams);
        UHD_LOG_TRACE("EISCAT", str(
            boost::format("Setting port %d to only receive on antenna %d "
                          "directly, bypassing neighbours and FIR matrix")
            % port % antenna_idx
        ));
        sr_write(SR_ANTENNA_SELECT_BASE + port, antenna_idx);
        enable_firs(false);
    } else if (ant_mode == "FI") {
        size_t beam_index = port % EISCAT_NUM_PORTS;
        UHD_LOG_TRACE("EISCAT", str(
            boost::format("Setting port %d to filter index %d on all antennas "
                          "using beam indices %d and %d.")
            % port
            % antenna_idx
            % beam_index % (beam_index + EISCAT_NUM_PORTS)
        ));
        // Note: antenna_idx is not indexing a physical antenna in this scenario.
        uhd::time_spec_t send_now(0.0);
        for (size_t i = 0; i < EISCAT_NUM_ANTENNAS; i++) {
            select_filter(
                beam_index,
                i,
                antenna_idx,
                send_now
            );
            select_filter(
                beam_index + EISCAT_NUM_PORTS,
                i,
                antenna_idx,
                send_now
            );
        }
        enable_firs(true);
    } else if (ant_mode == "CN") {
        const size_t beam_index = port % EISCAT_NUM_PORTS;
        UHD_LOG_TRACE("EISCAT", str(
            boost::format("Setting port %d to filter index %d on all antennas "
                          "using beam indices %d and %d.")
            % port
            % antenna_idx
            % beam_index % (beam_index + EISCAT_NUM_PORTS)
        ));
        // Note: antenna_idx is not indexing a physical antenna in this scenario.
        uhd::time_spec_t send_now(0.0);
        for (size_t i = 0; i < EISCAT_NUM_ANTENNAS; i+=2) {
            select_filter(
                beam_index,
                i,
                0,
                send_now
            );
            select_filter(
                beam_index + EISCAT_NUM_PORTS,
                i,
                0,
                send_now
            );
            select_filter(
                beam_index,
                i+1,
                antenna_idx,
                send_now
            );
            select_filter(
                beam_index + EISCAT_NUM_PORTS,
                i+1,
                antenna_idx,
                send_now
            );
        }
        enable_firs(true);
    } else {
        throw uhd::value_error(str(
            boost::format("EISCAT: Invalid antenna selection: %s")
            % ant
        ));
    }
}

double eiscat_radio_ctrl_impl::get_tx_frequency(const size_t /* chan */)
{
    UHD_LOG_WARNING("EISCAT", "Ignoring attempt to read Tx frequency");
    return 0.0;
}

double eiscat_radio_ctrl_impl::set_tx_frequency(const double /* freq */, const size_t /* chan */)
{
    throw uhd::runtime_error("Cannot set Tx frequency on EISCAT daughterboard");
}

double eiscat_radio_ctrl_impl::set_rx_frequency(const double freq, const size_t chan)
{
    if (freq != get_rx_frequency(chan)) {
        UHD_LOG_WARNING("EISCAT", "Ignoring attempt to set Rx frequency");
    }
    return get_rx_frequency(chan);
}

double eiscat_radio_ctrl_impl::set_rx_bandwidth(const double bandwidth, const size_t chan)
{
    if (bandwidth != get_rx_bandwidth(chan)) {
        UHD_LOG_WARNING("EISCAT", "Ignoring attempt to set Rx bandwidth");
    }
    return get_rx_bandwidth(chan);
}


double eiscat_radio_ctrl_impl::set_tx_gain(const double /* gain */, const size_t /* chan */)
{
    throw uhd::runtime_error("Cannot set Tx gain on EISCAT daughterboard");
}

double eiscat_radio_ctrl_impl::set_rx_gain(const double gain, const size_t chan)
{
    // TODO: Add ability to set digital gain or make it explicit this function is not supported.
    if (gain != get_rx_gain(chan)) {
        UHD_LOG_WARNING("EISCAT", "Ignoring attempt to set Rx gain.");
    }
    return get_rx_gain(chan);
}

double eiscat_radio_ctrl_impl::set_rate(double rate)
{
    if (rate != get_rate()) {
        UHD_LOG_WARNING("EISCAT", "Attempting to set sampling rate to invalid value " << rate);
    }
    return get_rate();
}

size_t eiscat_radio_ctrl_impl::get_chan_from_dboard_fe(
        const std::string &fe,
        const uhd::direction_t /* dir */
) {
    return boost::lexical_cast<size_t>(fe);
}

std::string eiscat_radio_ctrl_impl::get_dboard_fe_from_chan(
        const size_t chan,
        const uhd::direction_t /* dir */
) {
    return std::to_string(chan);
}

double eiscat_radio_ctrl_impl::get_output_samp_rate(size_t /* port */)
{
    return EISCAT_RADIO_RATE;
}

void eiscat_radio_ctrl_impl::set_rx_streamer(bool active, const size_t port)
{
    UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl_impl::set_rx_streamer() " << port << " -> " << active ;
    if (port > EISCAT_NUM_PORTS) {
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

    if (list_upstream_nodes().empty() or not bool(get_arg<int>("use_prev"))) {
        UHD_LOG_DEBUG(unique_id(), "No prevs found, or prevs disabled, not passing on set_rx_streamer");
    } else {
        UHD_LOG_DEBUG(unique_id(), "set_rx_streamer(): We have prevs, so passing on set_rx_streamer");
        source_node_ctrl::sptr this_upstream_block_ctrl =
                boost::dynamic_pointer_cast<source_node_ctrl>(list_upstream_nodes().at(0).lock());
        if (this_upstream_block_ctrl) {
            this_upstream_block_ctrl->set_rx_streamer(active, port);
        } else {
            UHD_LOG_WARNING(unique_id(), "Oh noes, couldn't lock sptr!");
        }
    }
}

void eiscat_radio_ctrl_impl::issue_stream_cmd(const uhd::stream_cmd_t &stream_cmd, const size_t chan)
{
    std::lock_guard<std::mutex> lock(_mutex);

    // Turn on/off top ones
    if (list_upstream_nodes().empty() or not bool(get_arg<int>("use_prev"))) {
        UHD_LOG_DEBUG(unique_id(), "No prevs found, or prevs disabled, not passing on stream cmd");
    } else {
        UHD_LOG_DEBUG(unique_id(), "issue_stream_cmd(): We have prevs, so passing on stream command");
        source_node_ctrl::sptr this_upstream_block_ctrl =
                boost::dynamic_pointer_cast<source_node_ctrl>(list_upstream_nodes().at(0).lock());
        if (this_upstream_block_ctrl) {
            this_upstream_block_ctrl->issue_stream_cmd(
                    stream_cmd,
                    chan
            );
        } else {
            UHD_LOG_WARNING(unique_id(), "Oh noes, couldn't lock sptr!");
        }
    }

    // Turn on/off this one
    UHD_LOGGER_DEBUG(unique_id()) << "eiscat_radio_ctrl_impl::issue_stream_cmd() " << chan << " " << char(stream_cmd.stream_mode);
    if (not _is_streamer_active(uhd::RX_DIRECTION, chan)) {
        UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl_impl::issue_stream_cmd() called on inactive channel. Skipping." ;
        return;
    }
    UHD_ASSERT_THROW(stream_cmd.num_samps <= 0x0fffffff);
    _continuous_streaming[chan] = (stream_cmd.stream_mode == stream_cmd_t::STREAM_MODE_START_CONTINUOUS);

    if (stream_cmd.stream_mode == stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS &&
            stream_cmd.stream_now == false) {
        UHD_LOG_TRACE("EISCAT", "Stop cmd timed, setting cmd time!");
        set_command_time(stream_cmd.time_spec, chan);
    }

    //setup the mode to instruction flags
    typedef boost::tuple<bool, bool, bool, bool> inst_t;
    static const uhd::dict<stream_cmd_t::stream_mode_t, inst_t> mode_to_inst = boost::assign::map_list_of
                                                            //reload, chain, samps, stop
        (stream_cmd_t::STREAM_MODE_START_CONTINUOUS,   inst_t(true,  true,  false, false))
        (stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS,    inst_t(false, false, false, true))
        (stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE, inst_t(false, false, true,  false))
        (stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE, inst_t(false, true,  true,  false))
    ;

    //setup the instruction flag values
    bool inst_reload, inst_chain, inst_samps, inst_stop;
    boost::tie(inst_reload, inst_chain, inst_samps, inst_stop) = mode_to_inst[stream_cmd.stream_mode];

    //calculate the word from flags and length
    uint32_t cmd_word = 0;
    cmd_word |= uint32_t((stream_cmd.stream_now)? 1 : 0) << 31;
    cmd_word |= uint32_t((inst_chain)?            1 : 0) << 30;
    cmd_word |= uint32_t((inst_reload)?           1 : 0) << 29;
    cmd_word |= uint32_t((inst_stop)?             1 : 0) << 28;
    cmd_word |= (inst_samps)? stream_cmd.num_samps : ((inst_stop)? 0 : 1);

    //issue the stream command
    const uint64_t ticks = (stream_cmd.stream_now)? 0 : stream_cmd.time_spec.to_ticks(get_rate());
    sr_write(regs::RX_CTRL_CMD, cmd_word, chan);
    sr_write(regs::RX_CTRL_TIME_HI, uint32_t(ticks >> 32), chan);
    sr_write(regs::RX_CTRL_TIME_LO, uint32_t(ticks >> 0),  chan); //latches the command
    UHD_LOG_INFO(unique_id(), "issued stream command.");
    if (stream_cmd.stream_mode == stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS &&
            stream_cmd.stream_now == false) {
        UHD_LOG_TRACE("EISCAT", "Stop cmd timed, setting cmd time!");
        set_command_time(uhd::time_spec_t(0.0), chan);
    }

}

bool eiscat_radio_ctrl_impl::check_radio_config()
{
    const uint32_t config_beams = get_arg<int>("configure_beams");
    bool skipping_neighbours = config_beams & EISCAT_SKIP_NEIGHBOURS;
    bool upper_contrib = config_beams & EISCAT_CONTRIB_UPPER;
    const fs_path rx_fe_path = fs_path("dboards/A/rx_frontends");
    uint32_t chan_enables = 0;
    for (const auto &enb: _rx_streamer_active) {
        if (enb.second) {
            chan_enables |= (1<<enb.first);
        }
    }
    if (not skipping_neighbours) {
        chan_enables = chan_enables | (chan_enables << EISCAT_NUM_PORTS);
    } else if (upper_contrib) {
        chan_enables <<= EISCAT_NUM_PORTS;
    }
    UHD_LOG_TRACE("EISCAT", str(
        boost::format("check_radio_config(): Setting channel enables to 0x%02X"
                      " Using %s beams, %saccepting neighbour contributions")
        % chan_enables
        % (upper_contrib ? "upper" : "lower")
        % (skipping_neighbours ? "not " : "")
    ));
    sr_write("SR_RX_STREAM_ENABLE", chan_enables);

    return true;
}

void eiscat_radio_ctrl_impl::set_rpc_client(
    uhd::rpc_client::sptr rpcc,
    const uhd::device_addr_t &block_args
) {
    _rpcc = rpcc;
    _block_args = block_args;
    auto dboard_info =
        _rpcc->request<std::vector<std::map<std::string, std::string>>>(
                "get_dboard_info"
        );
    _num_dboards = dboard_info.size();
    UHD_LOG_DEBUG("EISCAT", "Using " << _num_dboards << " daughterboards.");
    if (_num_dboards == 1) {
        UHD_LOG_WARNING("EISCAT",
            "Found 1 dboard, expected 2 for optimal operation."
        );
    } else if (_num_dboards > 2) {
        UHD_LOG_ERROR("EISCAT", "Detected too many dboards: " << _num_dboards);
        throw uhd::runtime_error("Too many dboards detected.");
    }

    UHD_LOG_INFO(
        "EISCAT",
        "Finalizing dboard initialization; initializing JESD cores and ADCs."
    );

    /* Start of the ADC synchronization operation.
     * These steps must be repeated if any ADC fails its deframer check
     * Changing the sync line from SyncbAB to SyncnCD usually resolves the error
     */
    const size_t possible_sync_combinations = 16; // 2 sync lines ^ (2 ADCs * 2 Daughtercards)
    for (size_t iteration = 0; iteration < possible_sync_combinations; iteration++) {
        UHD_LOG_INFO(
            "EISCAT",
            "looping to initialize JESD cores and ADCs."
        );
        if (not assert_jesd_cores_initialized()) {
            throw uhd::runtime_error("Failed to initialize JESD cores and reset ADCs!");
        }
        send_sysref();

        if (not assert_adcs_deframers()) {
            throw uhd::runtime_error("Failed to initialize ADCs and JESD deframers!");
        }
        send_sysref();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        if (assert_deframer_status()) {
            return;
        }
    }

    // Unable to find a sync line combination which works
    throw uhd::runtime_error("Failed to finalize JESD core setup!");
}

/****************************************************************************
 * Internal methods
 ***************************************************************************/
void eiscat_radio_ctrl_impl::write_fir_taps(
    const size_t fir_idx,
    const std::vector<eiscat_radio_ctrl_impl::fir_tap_t> &taps
) {
    if (taps.size() > EISCAT_NUM_FIR_TAPS) {
        throw uhd::value_error(str(
            boost::format("Too many FIR taps for EISCAT filters (%d)")
            % taps.size()
        ));
    }
    for (const auto &tap: taps) {
        if (tap > EISCAT_MAX_TAP_VALUE or tap < EISCAT_MIN_TAP_VALUE) {
            throw uhd::value_error(str(
                boost::format("Filter tap for filter_idx %d exceeds dynamic range (%d bits are allowed)")
                % fir_idx % EISCAT_BITS_PER_TAP
            ));
        }
    }

    UHD_LOG_TRACE("EISCAT", str(
        boost::format("Writing %d filter taps for filter index %d")
        % taps.size() % fir_idx
    ));
    for (size_t i = 0; i < EISCAT_NUM_FIR_TAPS; i++) {
        // Payload:
        // - bottom 14 bits address, fir_idx * 16 + tap_index
        // - top 18 bits are value
        uint32_t reg_value = (fir_idx * 16) + i;;
        if (taps.size() > i) {
            reg_value |= (taps[i] & 0x3FFFF) << 14;
        }
        sr_write("SR_FIR_BRAM_WRITE_TAPS", reg_value);
    }
}

void eiscat_radio_ctrl_impl::select_filter(
        const size_t beam_index,
        const size_t antenna_index,
        const size_t fir_index,
        const uhd::time_spec_t &time_spec,
        const bool write_time
) {
    if (antenna_index >= EISCAT_NUM_ANTENNAS) {
        throw uhd::value_error(str(
            boost::format("Antenna index %d out of range. There are %d antennas in EISCAT.")
            % antenna_index % EISCAT_NUM_ANTENNAS
        ));
    }
    if (beam_index >= EISCAT_NUM_BEAMS) {
        throw uhd::value_error(str(
            boost::format("Beam index %d out of range. "
                          "There are %d beam channels in EISCAT.")
            % beam_index
            % EISCAT_NUM_BEAMS
        ));
    }

    UHD_LOGGER_TRACE("EISCAT")
        << "Selecting filter " << fir_index
        << " for beam " << beam_index
        << " and antenna " << antenna_index
    ;
    bool send_now = (time_spec == uhd::time_spec_t(0.0));
    uint32_t reg_value = 0
        | (fir_index * 16)
        | (antenna_index & 0xF) << 14
        | (beam_index & 0xF) << 18
        | send_now << 22
    ;
    if (not send_now) {
        UHD_LOG_TRACE("EISCAT", str(
            boost::format("Filter selection will be applied at "
                          "time %f (0x%016X == %u). %s")
            % time_spec.get_full_secs()
            % time_spec.to_ticks(EISCAT_TICK_RATE)
            % time_spec.to_ticks(EISCAT_TICK_RATE)
            % (write_time ? "Writing time regs now."
                          : "Assuming time regs already up-to-date.")
        ));
        if (write_time) {
            set_fir_ctrl_time(time_spec);
        }
    }
    sr_write("SR_FIR_COMMANDS_RELOAD", reg_value);
}

void eiscat_radio_ctrl_impl::set_fir_ctrl_time(
    const uhd::time_spec_t &time_spec
) {
    const uint64_t cmd_time_ticks = time_spec.to_ticks(EISCAT_TICK_RATE);
    sr_write(
        "SR_FIR_COMMANDS_CTRL_TIME_LO",
        uint32_t(cmd_time_ticks & 0xFFFFFFFF)
    );
    sr_write(
        "SR_FIR_COMMANDS_CTRL_TIME_HI",
        uint32_t((cmd_time_ticks >> 32) & 0xFFFFFFFF)
    );
}

void eiscat_radio_ctrl_impl::set_antenna_gain(
    const size_t antenna_idx,
    const double normalized_gain
) {
    if (normalized_gain < -16.0 or normalized_gain > 16.0) {
        throw uhd::value_error(str(
            boost::format("Invalid digital gain value for antenna %d: %f")
            % antenna_idx % normalized_gain
        ));
    }

    const auto fixpoint_gain = std::max<int32_t>(
        EISCAT_MIN_GAIN,
        std::min(
            EISCAT_MAX_GAIN,
            int32_t(normalized_gain * EISCAT_UNIT_GAIN)
        )
    );

    UHD_LOG_TRACE("EISCAT", str(
        boost::format("Setting digital gain value for antenna %d to %f (%d)")
        % antenna_idx % normalized_gain % fixpoint_gain
    ));
    sr_write(SR_ANTENNA_GAIN_BASE + antenna_idx, fixpoint_gain);
}

void eiscat_radio_ctrl_impl::configure_beams(uint32_t reg_value)
{
    UHD_LOGGER_TRACE("EISCAT")
        << "Selecting " <<
        ((reg_value & EISCAT_CONTRIB_UPPER) ? "upper" : "lower") << " beams.";
    UHD_LOGGER_TRACE("EISCAT")
        << ((reg_value & EISCAT_SKIP_NEIGHBOURS) ? "Disabling" : "Enabling")
        << " neighbour contributions.";
    UHD_LOGGER_TRACE("EISCAT")
        << ((reg_value & EISCAT_BYPASS_MATRIX) ? "Disabling" : "Enabling")
        << " FIR matrix.";
    UHD_LOGGER_TRACE("EISCAT")
        << ((reg_value & EISCAT_OUTPUT_COUNTER) ? "Enabling" : "Disabling")
        << " counter.";
    UHD_LOG_TRACE("EISCAT", str(
        boost::format("Setting SR_BEAMS_TO_NEIGHBOR to 0x%08X.")
        % reg_value
    ));
    sr_write("SR_BEAMS_TO_NEIGHBOR", reg_value);
}

void eiscat_radio_ctrl_impl::set_beam_selection(int beam_selection)
{
    UHD_ASSERT_THROW(beam_selection < 4 and beam_selection >= 0);
    const uint32_t old_value = user_reg_read32(RB_CHOOSE_BEAMS);
    const uint32_t new_value =
        (old_value & (~uint32_t(EISCAT_CONTRIB_UPPER|EISCAT_SKIP_NEIGHBOURS)))
        | (uint32_t(beam_selection)
                & uint32_t(EISCAT_CONTRIB_UPPER|EISCAT_SKIP_NEIGHBOURS))
    ;
    configure_beams(new_value);
}

void eiscat_radio_ctrl_impl::enable_firs(bool enable)
{
    const uint32_t old_value = user_reg_read32(RB_CHOOSE_BEAMS);
    const uint32_t new_value = enable ?
        (old_value & ~EISCAT_BYPASS_MATRIX)
        : old_value | EISCAT_BYPASS_MATRIX
    ;
    configure_beams(new_value);
}

void eiscat_radio_ctrl_impl::send_sysref()
{
    if (_block_args.has_key("use_mpm_sysref")) {
        _rpcc->notify_with_token("db_0_send_sysref");
    } else {
        // This value needs to be big enough that we actually hit it between
        // reading back the time, and applying the command:
        const int CMD_DELAY_MS = 100;
        auto sysref_time = get_time_now()
                + uhd::time_spec_t(double(CMD_DELAY_MS * 1000));
        uint64_t sysref_time_ticks = sysref_time.to_ticks(EISCAT_TICK_RATE);
        // The tick value must be even, or we'd still have the 180 degree phase
        // ambiguity! The actual value doesn't matter.
        sysref_time_ticks += sysref_time_ticks % 2;
        set_command_time(uhd::time_spec_t::from_ticks(
            sysref_time_ticks, EISCAT_TICK_RATE
        ));
        this->sr_write("SR_SYSREF", 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(CMD_DELAY_MS));
    }
}

void eiscat_radio_ctrl_impl::enable_counter(bool enable)
{
    const uint32_t old_value = user_reg_read32(RB_CHOOSE_BEAMS);
    const uint32_t new_value = enable ?
        old_value | EISCAT_OUTPUT_COUNTER
        : (old_value & ~EISCAT_OUTPUT_COUNTER)
    ;
    configure_beams(new_value);
}

bool eiscat_radio_ctrl_impl::assert_jesd_cores_initialized()
{
    if (_num_dboards == 1) {
        return _rpcc->request_with_token<bool>("db_0_init_jesd_core_reset_adcs");
    }
    return _rpcc->request_with_token<bool>("db_0_init_jesd_core_reset_adcs")
        and _rpcc->request_with_token<bool>("db_1_init_jesd_core_reset_adcs");
}

bool eiscat_radio_ctrl_impl::assert_adcs_deframers()
{
    if (_num_dboards == 1) {
        return _rpcc->request_with_token<bool>("db_0_init_adcs_and_deframers");
    }
    return _rpcc->request_with_token<bool>("db_0_init_adcs_and_deframers")
        and _rpcc->request_with_token<bool>("db_1_init_adcs_and_deframers");
}

bool eiscat_radio_ctrl_impl::assert_deframer_status()
{
    if (_num_dboards == 1) {
        return _rpcc->request_with_token<bool>("db_0_check_deframer_status");
    }
    return _rpcc->request_with_token<bool>("db_0_check_deframer_status")
        and _rpcc->request_with_token<bool>("db_1_check_deframer_status");
}

/****************************************************************************
 * Registry
 ***************************************************************************/
UHD_RFNOC_BLOCK_REGISTER(eiscat_radio_ctrl, "EISCATRadio");

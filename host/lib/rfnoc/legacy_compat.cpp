//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../usrp/device3/device3_impl.hpp"
#include <uhd/property_tree.hpp>
#include <uhd/rfnoc/radio_ctrl.hpp>
#include <uhd/rfnoc/ddc_block_ctrl.hpp>
#include <uhd/rfnoc/graph.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/stream.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/transport/chdr.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhdlib/rfnoc/legacy_compat.hpp>
#include <boost/make_shared.hpp>

#define UHD_LEGACY_LOG() UHD_LOGGER_TRACE("RFNOC")

using namespace uhd::rfnoc;
using uhd::usrp::subdev_spec_t;
using uhd::usrp::subdev_spec_pair_t;
using uhd::stream_cmd_t;

/************************************************************************
 * Constants and globals
 ***********************************************************************/
static const std::string RADIO_BLOCK_NAME = "Radio";
static const std::string DFIFO_BLOCK_NAME = "DmaFIFO";
static const std::string SFIFO_BLOCK_NAME = "FIFO";
static const std::string DDC_BLOCK_NAME = "DDC";
static const std::string DUC_BLOCK_NAME = "DUC";
static const size_t MAX_BYTES_PER_HEADER =
        uhd::transport::vrt::chdr::max_if_hdr_words64 * sizeof(uint64_t);
static const size_t BYTES_PER_SAMPLE = 4; // We currently only support sc16
static boost::mutex _make_mutex;
static const std::vector<std::string>
    LEGACY_BLOCKS_LIST =
        {
            RADIO_BLOCK_NAME,
            DFIFO_BLOCK_NAME,
            SFIFO_BLOCK_NAME,
            DDC_BLOCK_NAME,
            DUC_BLOCK_NAME
        };
typedef std::vector<source_block_ctrl_base::sptr> source_block_list_t;
typedef std::vector<sink_block_ctrl_base::sptr> sink_block_list_t;
typedef std::map<std::string, std::pair<source_block_list_t, sink_block_list_t>> block_name_to_block_map_t;
typedef std::pair<source_block_ctrl_base::sptr, size_t> source_port_t;
typedef std::pair<sink_block_ctrl_base::sptr, size_t> sink_port_t;
/************************************************************************
 * Static helpers
 ***********************************************************************/
static uhd::fs_path mb_root(const size_t mboard)
{
    return uhd::fs_path("/mboards") / mboard;
}

size_t num_ports(const uhd::property_tree::sptr &tree, const std::string &block_name, const std::string &in_out)
{
    return tree->list(
            uhd::fs_path("/mboards/0/xbar") /
            str(boost::format("%s_0") % block_name) /
            "ports" / in_out
    ).size();
}

size_t calc_num_tx_chans_per_radio(
    const uhd::property_tree::sptr &tree,
    const size_t num_radios_per_board,
    const bool has_ducs,
    const bool has_dmafifo
) {
    const size_t num_radio_ports = num_ports(tree, RADIO_BLOCK_NAME, "in");
    if (has_ducs) {
        return std::min(
            num_radio_ports,
            num_ports(tree, DUC_BLOCK_NAME, "in")
        );
    }

    if (not has_dmafifo) {
        return num_radio_ports;
    }

    const size_t num_dmafifo_ports_per_radio = num_ports(tree, DFIFO_BLOCK_NAME, "in") / num_radios_per_board;
    UHD_ASSERT_THROW(num_dmafifo_ports_per_radio);

    return std::min(
        num_radio_ports,
        num_dmafifo_ports_per_radio
    );
}

/*! Recreate passed property without bound subscribers. Maintains current property value.
*/
template <typename T>
static void recreate_property(const uhd::fs_path &path, uhd::property_tree::sptr &tree) {
    T temp = tree->access<T>(path).get();
    tree->remove(path);
    tree->create<T>(path).set(temp);
}

/************************************************************************
 * Class Definition
 ***********************************************************************/
class legacy_compat_impl : public legacy_compat
{
public:
    /************************************************************************
     * Structors and Initialization
     ***********************************************************************/
    legacy_compat_impl(
            uhd::device3::sptr device,
            const uhd::device_addr_t &args
    ) : _device(device),
        _tree(device->get_tree()),
        _has_ducs(not args.has_key("skip_duc") and not device->find_blocks(DUC_BLOCK_NAME).empty()),
        _has_ddcs(not args.has_key("skip_ddc") and not device->find_blocks(DDC_BLOCK_NAME).empty()),
        _has_dmafifo(not args.has_key("skip_dram") and not device->find_blocks(DFIFO_BLOCK_NAME).empty()),
        _has_sramfifo(not args.has_key("skip_sram") and not device->find_blocks(SFIFO_BLOCK_NAME).empty()),
        _num_mboards(_tree->list("/mboards").size()),
        _num_radios_per_board(device->find_blocks<radio_ctrl>("0/Radio").size()), // These might throw, maybe we catch that and provide a nicer error message.
        _num_tx_chans_per_radio(
            calc_num_tx_chans_per_radio(_tree, _num_radios_per_board, _has_ducs, _has_dmafifo)
        ),
        _num_rx_chans_per_radio(_has_ddcs ?
                std::min(num_ports(_tree, RADIO_BLOCK_NAME, "out"), num_ports(_tree, DDC_BLOCK_NAME, "out"))
                : num_ports(_tree, RADIO_BLOCK_NAME, "out")),
        _rx_spp(get_block_ctrl<radio_ctrl>(0, RADIO_BLOCK_NAME, 0)->get_arg<int>("spp")),
        _tx_spp(_rx_spp),
        _rx_channel_map(_num_mboards, std::vector<radio_port_pair_t>(_num_radios_per_board)),
        _tx_channel_map(_num_mboards, std::vector<radio_port_pair_t>(_num_radios_per_board))
    {
        _device->clear();
        check_available_periphs(); // Throws if invalid configuration.
        setup_prop_tree();
        if (_tree->exists("/mboards/0/mtu/send")) {
            _tx_spp = (_tree->access<size_t>("/mboards/0/mtu/send").get() - MAX_BYTES_PER_HEADER) / BYTES_PER_SAMPLE;
        }
        connect_blocks();
        if (args.has_key("skip_ddc")) {
            UHD_LEGACY_LOG() << "[legacy_compat] Skipping DDCs by user request." ;
        } else if (not _has_ddcs) {
            UHD_LOGGER_WARNING("RFNOC")
                << "[legacy_compat] No DDCs detected. You will only be able to receive at the radio frontend rate."
                ;
        }
        if (args.has_key("skip_duc")) {
            UHD_LEGACY_LOG() << "[legacy_compat] Skipping DUCs by user request." ;
        } else if (not _has_ducs) {
            UHD_LOGGER_WARNING("RFNOC") << "[legacy_compat] No DUCs detected. You will only be able to transmit at the radio frontend rate." ;
        }
        if (args.has_key("skip_dram")) {
            UHD_LEGACY_LOG() << "[legacy_compat] Skipping DRAM by user request." << std::endl;
        }
        if (args.has_key("skip_sram")) {
            UHD_LEGACY_LOG() << "[legacy_compat] Skipping SRAM by user request." << std::endl;
        }
        if (not _has_dmafifo and not _has_sramfifo) {
            UHD_LOGGER_WARNING("RFNOC") << "[legacy_compat] No FIFO detected. Higher transmit rates may encounter errors.";
        }

        for (size_t mboard = 0; mboard < _num_mboards; mboard++) {
            for (size_t radio = 0; radio < _num_radios_per_board; radio++) {
                _rx_channel_map[mboard][radio].radio_index = radio;
                _tx_channel_map[mboard][radio].radio_index = radio;
            }

            const double tick_rate = _tree->access<double>(mb_root(mboard) / "tick_rate").get();
            update_tick_rate_on_blocks(tick_rate, mboard);
        }
    }

    ~legacy_compat_impl()
    {
       remove_prop_subscribers();
    }

    /************************************************************************
     * API Calls
     ***********************************************************************/
    inline uhd::fs_path rx_dsp_root(const size_t mboard_idx, const size_t dsp_index, const size_t port_index)
    {
        return mb_root(mboard_idx) / "xbar" /
               str(boost::format("%s_%d") % DDC_BLOCK_NAME % dsp_index) /
               "legacy_api" / port_index;
    }

    uhd::fs_path rx_dsp_root(const size_t mboard_idx, const size_t chan)
    {
        // The DSP index is the same as the radio index
        size_t dsp_index = _rx_channel_map[mboard_idx][chan].radio_index;
        size_t port_index = _rx_channel_map[mboard_idx][chan].port_index;

        if (not _has_ddcs) {
            return mb_root(mboard_idx) / "rx_dsps" / dsp_index / port_index;
        }

        return rx_dsp_root(mboard_idx, dsp_index, port_index);
    }

    inline uhd::fs_path tx_dsp_root(const size_t mboard_idx, const size_t dsp_index, const size_t port_index)
    {
        return mb_root(mboard_idx) / "xbar" /
               str(boost::format("%s_%d") % DUC_BLOCK_NAME % dsp_index) /
               "legacy_api" / port_index;
    }

    uhd::fs_path tx_dsp_root(const size_t mboard_idx, const size_t chan)
    {
        // The DSP index is the same as the radio index
        size_t dsp_index = _tx_channel_map[mboard_idx][chan].radio_index;
        size_t port_index = _tx_channel_map[mboard_idx][chan].port_index;

        if (not _has_ducs) {
            return mb_root(mboard_idx) / "tx_dsps" / dsp_index / port_index;
        }

        return tx_dsp_root(mboard_idx, dsp_index, port_index);
    }

    uhd::fs_path rx_fe_root(const size_t mboard_idx, const size_t chan)
    {
        size_t radio_index = _rx_channel_map[mboard_idx][chan].radio_index;
        size_t port_index = _rx_channel_map[mboard_idx][chan].port_index;
        return uhd::fs_path(str(
                boost::format("/mboards/%d/xbar/%s_%d/rx_fe_corrections/%d/")
                % mboard_idx % RADIO_BLOCK_NAME % radio_index % port_index
        ));
    }

    uhd::fs_path tx_fe_root(const size_t mboard_idx, const size_t chan)
    {
        size_t radio_index = _tx_channel_map[mboard_idx][chan].radio_index;
        size_t port_index = _tx_channel_map[mboard_idx][chan].port_index;
        return uhd::fs_path(str(
                boost::format("/mboards/%d/xbar/%s_%d/tx_fe_corrections/%d/")
                % mboard_idx % RADIO_BLOCK_NAME % radio_index % port_index
        ));
    }
    //! Get all legacy blocks from the LEGACY_BLOCK_LIST return in a form of
    //  {BLOCK_NAME: <{source_block_pointer},{sink_block_pointer}>}
    block_name_to_block_map_t get_legacy_blocks(uhd::device3::sptr _device)
    {
        block_name_to_block_map_t result ;
        for(auto each_block_name: LEGACY_BLOCKS_LIST){
            std::vector<block_id_t> block_list = _device->find_blocks(each_block_name);
            std::pair<source_block_list_t, sink_block_list_t> ss_pair;
            source_block_list_t src_list;
            sink_block_list_t snk_list;
            for(auto each_block: block_list){
                uhd::rfnoc::source_block_ctrl_base::sptr src =
                    _device->get_block_ctrl<source_block_ctrl_base>(each_block);
                src_list.push_back(src);
                uhd::rfnoc::sink_block_ctrl_base::sptr snk =
                    _device->get_block_ctrl<sink_block_ctrl_base>(each_block);
                snk_list.push_back(snk);
            }
            ss_pair = std::make_pair(src_list, snk_list);
            result[each_block_name] = ss_pair;
        }
        return result;
    }

    void issue_stream_cmd(const stream_cmd_t &stream_cmd, size_t mboard, size_t chan)
    {
        UHD_LEGACY_LOG() << "[legacy_compat] issue_stream_cmd() " ;
        const size_t &radio_index = _rx_channel_map[mboard][chan].radio_index;
        const size_t &port_index  = _rx_channel_map[mboard][chan].port_index;
        if (_has_ddcs) {
            get_block_ctrl<ddc_block_ctrl>(mboard, DDC_BLOCK_NAME, radio_index)->issue_stream_cmd(stream_cmd, port_index);
        } else {
            get_block_ctrl<radio_ctrl>(mboard, RADIO_BLOCK_NAME, radio_index)->issue_stream_cmd(stream_cmd, port_index);
        }
    }

    //! Sets block_id<N> and block_port<N> in the streamer args, otherwise forwards the call
    uhd::rx_streamer::sptr get_rx_stream(const uhd::stream_args_t &args_)
    {
        uhd::stream_args_t args(args_);
        if (args.otw_format.empty()) {
            args.otw_format = "sc16";
        }
        _update_stream_args_for_streaming<uhd::RX_DIRECTION>(args, _rx_channel_map);
        UHD_LEGACY_LOG() << "[legacy_compat] rx stream args: " << args.args.to_string() ;
        uhd::rx_streamer::sptr streamer = _device->get_rx_stream(args);
        for(const size_t chan:  args.channels) {
            _rx_stream_cache[chan] = streamer;
        }
        return streamer;
    }

    //! Sets block_id<N> and block_port<N> in the streamer args, otherwise forwards the call.
    // If spp is in the args, update the radios. If it's not set, copy the value from the radios.
    uhd::tx_streamer::sptr get_tx_stream(const uhd::stream_args_t &args_)
    {
        uhd::stream_args_t args(args_);
        if (args.otw_format.empty()) {
            args.otw_format = "sc16";
        }
        _update_stream_args_for_streaming<uhd::TX_DIRECTION>(args, _tx_channel_map);
        UHD_LEGACY_LOG() << "[legacy_compat] tx stream args: " << args.args.to_string() ;
        uhd::tx_streamer::sptr streamer = _device->get_tx_stream(args);
        for(const size_t chan:  args.channels) {
            _tx_stream_cache[chan] = streamer;
        }
        return streamer;
    }

    double get_tick_rate(const size_t mboard_idx=0)
    {
        return _tree->access<double>(mb_root(mboard_idx) / "tick_rate").get();
    }

    uhd::meta_range_t lambda_get_samp_rate_range(
            const size_t mboard_idx,
            const size_t radio_idx,
            const size_t chan,
            uhd::direction_t dir
    ) {
        radio_ctrl::sptr radio_sptr = get_block_ctrl<radio_ctrl>(mboard_idx, RADIO_BLOCK_NAME, radio_idx);
        const double samp_rate = (dir == uhd::TX_DIRECTION) ?
            radio_sptr->get_input_samp_rate(chan) :
            radio_sptr->get_output_samp_rate(chan)
        ;

        return uhd::meta_range_t(samp_rate, samp_rate, 0.0);
    }

    void set_tick_rate(const double tick_rate, const size_t mboard_idx=0)
    {
        _tree->access<double>(mb_root(mboard_idx) / "tick_rate").set(tick_rate);
        update_tick_rate_on_blocks(tick_rate, mboard_idx);
    }

    void set_rx_rate(const double rate, const size_t chan)
    {
        if (not _has_ddcs) {
            return;
        }

        // Set DDC values:
        if (chan == uhd::usrp::multi_usrp::ALL_CHANS) {
            for (size_t mboard_idx = 0; mboard_idx < _rx_channel_map.size(); mboard_idx++) {
                for (size_t chan_idx = 0; chan_idx < _rx_channel_map[mboard_idx].size(); chan_idx++) {
                    const size_t dsp_index  = _rx_channel_map[mboard_idx][chan_idx].radio_index;
                    const size_t port_index = _rx_channel_map[mboard_idx][chan_idx].port_index;
                    _tree->access<double>(rx_dsp_root(mboard_idx, dsp_index, port_index) / "rate/value")
                        .set(rate)
                    ;
                }
            }
        } else {
            std::set<size_t> chans_to_change{chan};
            if (_rx_stream_cache.count(chan)) {
                uhd::rx_streamer::sptr str_ptr = _rx_stream_cache[chan].lock();
                if (str_ptr) {
                    for(const rx_stream_map_type::value_type &chan_streamer_pair:  _rx_stream_cache) {
                        if (chan_streamer_pair.second.lock() == str_ptr) {
                            chans_to_change.insert(chan_streamer_pair.first);
                        }
                    }
                }
            }
            for(const size_t this_chan:  chans_to_change) {
                size_t mboard, mb_chan;
                chan_to_mcp<uhd::RX_DIRECTION>(this_chan, _rx_channel_map, mboard, mb_chan);
                const size_t dsp_index  = _rx_channel_map[mboard][mb_chan].radio_index;
                const size_t port_index = _rx_channel_map[mboard][mb_chan].port_index;
                _tree->access<double>(rx_dsp_root(mboard, dsp_index, port_index) / "rate/value")
                    .set(rate)
                ;
            }
        }
        // Update streamers:
        boost::dynamic_pointer_cast<uhd::usrp::device3_impl>(_device)->update_rx_streamers(rate);
    }

    void set_tx_rate(const double rate, const size_t chan)
    {
        if (not _has_ducs) {
            return;
        }

        // Set DUC values:
        if (chan == uhd::usrp::multi_usrp::ALL_CHANS) {
            for (size_t mboard_idx = 0; mboard_idx < _tx_channel_map.size(); mboard_idx++) {
                for (size_t chan_idx = 0; chan_idx < _tx_channel_map[mboard_idx].size(); chan_idx++) {
                    const size_t dsp_index = _tx_channel_map[mboard_idx][chan_idx].radio_index;
                    const size_t port_index  = _tx_channel_map[mboard_idx][chan_idx].port_index;
                    _tree->access<double>(tx_dsp_root(mboard_idx, dsp_index, port_index) / "rate/value")
                        .set(rate)
                    ;
                }
            }
        } else {
            std::set<size_t> chans_to_change{chan};
            if (_tx_stream_cache.count(chan)) {
                uhd::tx_streamer::sptr str_ptr = _tx_stream_cache[chan].lock();
                if (str_ptr) {
                    for(const tx_stream_map_type::value_type &chan_streamer_pair:  _tx_stream_cache) {
                        if (chan_streamer_pair.second.lock() == str_ptr) {
                            chans_to_change.insert(chan_streamer_pair.first);
                        }
                    }
                }
            }
            for(const size_t this_chan:  chans_to_change) {
                size_t mboard, mb_chan;
                chan_to_mcp<uhd::TX_DIRECTION>(this_chan, _tx_channel_map, mboard, mb_chan);
                const size_t dsp_index  = _tx_channel_map[mboard][mb_chan].radio_index;
                const size_t port_index = _tx_channel_map[mboard][mb_chan].port_index;
                _tree->access<double>(tx_dsp_root(mboard, dsp_index, port_index) / "rate/value")
                    .set(rate)
                ;
            }
        }
        // Update streamers:
        boost::dynamic_pointer_cast<uhd::usrp::device3_impl>(_device)->update_tx_streamers(rate);
    }

private: // types
    struct radio_port_pair_t {
        radio_port_pair_t(const size_t radio=0, const size_t port=0) : radio_index(radio), port_index(port) {}
        size_t radio_index;
        size_t port_index;
    };
    //! Map: _rx_channel_map[mboard_idx][chan_idx] => (Radio, Port)
    // Container is not a std::map because we need to guarantee contiguous
    // ports and correct order anyway.
    typedef std::vector< std::vector<radio_port_pair_t> > chan_map_t;

private: // methods
    /************************************************************************
     * Private helpers
     ***********************************************************************/
    std::string get_slot_name(const size_t radio_index)
    {
        if (radio_index == 0){
            return "A";
        }else if (radio_index == 1){
            return "B";
        }else if (radio_index == 2){
            return "C";
        }else if (radio_index == 3){
            return "D";
        }else{
            throw uhd::index_error(str(
                boost::format("[legacy_compat]: radio index %u out of supported range.")
                % radio_index
            ));
        }
    }

    size_t get_radio_index(const std::string slot_name)
    {
        if (slot_name == "A"){
            return 0;
        }else if (slot_name == "B"){
            return 1;
        }else if (slot_name == "C"){
            return 2;
        }else if (slot_name == "D"){
            return  3;
        }else {
           throw uhd::key_error(str(
                boost::format("[legacy_compat]: radio slot name %s out of supported range.")
                % slot_name
            ));
        }
    }

    template <typename block_type>
    inline typename block_type::sptr get_block_ctrl(const size_t mboard_idx, const std::string &name, const size_t block_count)
    {
        block_id_t block_id(mboard_idx, name, block_count);
        return _device->get_block_ctrl<block_type>(block_id);
    }

    template <uhd::direction_t dir>
    inline void chan_to_mcp(
        const size_t chan, const chan_map_t &chan_map,
        size_t &mboard_idx, size_t &mb_chan_idx
    ) {
        mboard_idx = 0;
        mb_chan_idx = chan;
        while (mb_chan_idx >= chan_map[mboard_idx].size()) {
            mb_chan_idx  -= chan_map[mboard_idx++].size();
        }
        if (mboard_idx >= chan_map.size()) {
            throw uhd::index_error(str(
                boost::format("[legacy_compat]: %s channel %u out of range for given frontend configuration.")
                % (dir == uhd::TX_DIRECTION ? "TX" : "RX")
                % chan
            ));
        }
    }

    template <uhd::direction_t dir>
    void _update_stream_args_for_streaming(
            uhd::stream_args_t &args,
            const chan_map_t &chan_map
    ) {
        // If the user provides spp, that value is always applied. If it's
        // different from what we thought it was, we need to update the blocks.
        // If it's not provided, we provide our own spp value.
        const size_t args_spp = args.args.cast<size_t>("spp", 0);
        if (dir == uhd::RX_DIRECTION) {
            size_t target_spp = _rx_spp;
            if (args.args.has_key("spp") and args_spp != _rx_spp) {
                target_spp = args_spp;
                // TODO: Update flow control on the blocks
            } else {
                for (size_t mboard = 0; mboard < _num_mboards; mboard++) {
                    for (size_t radio = 0; radio < _num_radios_per_board; radio++) {
                        const size_t this_spp = get_block_ctrl<radio_ctrl>(mboard, RADIO_BLOCK_NAME, radio)->get_arg<int>("spp");
                        target_spp = std::min(this_spp, target_spp);
                    }
                }
            }
            for (size_t mboard = 0; mboard < _num_mboards; mboard++) {
                for (size_t radio = 0; radio < _num_radios_per_board; radio++) {
                    get_block_ctrl<radio_ctrl>(mboard, RADIO_BLOCK_NAME, radio)->set_arg<int>("spp", target_spp);
                }
            }
            _rx_spp = target_spp;
            args.args["spp"] = str(boost::format("%d") % _rx_spp);
        } else {
            if (args.args.has_key("spp") and args_spp != _tx_spp) {
                _tx_spp = args_spp;
                // TODO: Update flow control on the blocks
            } else {
                args.args["spp"] = str(boost::format("%d") % _tx_spp);
            }
        }

        if (args.channels.empty()) {
            args.channels = std::vector<size_t>(1, 0);
        }
        for (size_t i = 0; i < args.channels.size(); i++) {
            const size_t stream_arg_chan_idx = args.channels[i];
            // Determine which mboard, and on that mboard, which channel this is:
            size_t mboard_idx, this_mboard_chan_idx;
            chan_to_mcp<dir>(stream_arg_chan_idx, chan_map, mboard_idx, this_mboard_chan_idx);
            // Map that mboard and channel to a block:
            const size_t radio_index = chan_map[mboard_idx][this_mboard_chan_idx].radio_index;
            size_t port_index = chan_map[mboard_idx][this_mboard_chan_idx].port_index;
            auto block_and_port = _get_streamer_block_id_and_port<dir>(mboard_idx, radio_index, port_index);
            auto block_name = block_and_port.first.to_string();
            port_index = block_and_port.second;
            args.args[str(boost::format("block_id%d") % stream_arg_chan_idx)] = block_name;
            args.args[str(boost::format("block_port%d") % stream_arg_chan_idx)] = str(boost::format("%d") % port_index);
            // Map radio to channel (for in-band response)
            args.args[str(boost::format("radio_id%d") % stream_arg_chan_idx)] = block_id_t(mboard_idx, RADIO_BLOCK_NAME, radio_index).to_string();
            args.args[str(boost::format("radio_port%d") % stream_arg_chan_idx)] = str(boost::format("%d") % chan_map[mboard_idx][this_mboard_chan_idx].port_index);
        }
    }

    //! Given mboard_index(m), radio_index(r), and port_index(p),
    //  this function returns the index of a block on the input block list that match m,r,p
    template <typename T>
    size_t find_block(const std::vector<T> &port_list, const size_t &m, const size_t &r, const size_t &p)
    {
        size_t index  = 0;
        for (auto port : port_list)
        {
            auto block_id = (port.first)->get_block_id();
            if (p == port.second && r == block_id.get_block_count() && m == block_id.get_device_no())
            {
                return index;
            }
            index++;
        }
        throw uhd::runtime_error((boost::format("Could not find block in list for device %d, radio %d, and port %d") % m % r % p).str());
    }

    template <uhd::direction_t dir>
    std::pair<block_id_t, size_t> _get_streamer_block_id_and_port(
        const size_t &mboard_idx,
        const size_t &radio_index,
        const size_t &port_index)
    {
        block_name_to_block_map_t legacy_block_map = get_legacy_blocks(_device);
        if (dir == uhd::TX_DIRECTION){
            auto radio_snk_flat = _flatten_blocks_by_n_ports(legacy_block_map[RADIO_BLOCK_NAME].second);
            size_t index_snk = find_block<sink_port_t>(radio_snk_flat, mboard_idx, radio_index, port_index);
            if (_has_sramfifo){
                auto sfifo_snk_flat = _flatten_blocks_by_n_ports(legacy_block_map[SFIFO_BLOCK_NAME].second);
                UHD_ASSERT_THROW(index_snk < sfifo_snk_flat.size());
                auto sfifo_block = sfifo_snk_flat[index_snk].first->get_block_id();
                return std::make_pair(sfifo_block, sfifo_snk_flat[index_snk].second);
            }else if (_has_dmafifo){
                auto dfifo_snk_flat = _flatten_blocks_by_n_ports(legacy_block_map[DFIFO_BLOCK_NAME].second);
                UHD_ASSERT_THROW(index_snk < dfifo_snk_flat.size());
                auto dfifo_block = dfifo_snk_flat[index_snk].first->get_block_id();
                return std::make_pair(dfifo_block, dfifo_snk_flat[index_snk].second);
            }else{
                if (_has_ducs){
                    return std::make_pair(block_id_t(mboard_idx, DUC_BLOCK_NAME, radio_index).to_string(),port_index);
                    auto duc_snk_flat = _flatten_blocks_by_n_ports(legacy_block_map[DUC_BLOCK_NAME].second);
                    UHD_ASSERT_THROW(index_snk < duc_snk_flat.size());
                    auto duc_block = duc_snk_flat[index_snk].first->get_block_id();
                    return std::make_pair(duc_block, duc_snk_flat[index_snk].second);
                }else{
                    return std::make_pair(block_id_t(mboard_idx, RADIO_BLOCK_NAME, radio_index).to_string(),port_index);
                }
            }
        }else{
            auto radio_src_flat = _flatten_blocks_by_n_ports(legacy_block_map[RADIO_BLOCK_NAME].first);
            size_t index_src = find_block<source_port_t>(radio_src_flat, mboard_idx, radio_index, port_index);
            if (_has_ddcs){
                auto ddc_src_flat = _flatten_blocks_by_n_ports(legacy_block_map[DDC_BLOCK_NAME].first);
                UHD_ASSERT_THROW(index_src < ddc_src_flat.size());
                auto ddc_block = ddc_src_flat[index_src].first->get_block_id();
                return std::make_pair(ddc_block, ddc_src_flat[index_src].second);
            }
            else{
                return std::make_pair(block_id_t(mboard_idx, RADIO_BLOCK_NAME, radio_index).to_string(),port_index);
            }
        }
    }
    /************************************************************************
     * Initialization
     ***********************************************************************/
    /*! Check this device has all the required peripherals.
     *
     * Check rules:
     * - Every mboard needs the same number of radios.
     * - For every radio block, there must be DDC and a DUC block,
     *   with matching number of ports.
     *
     * \throw uhd::runtime_error if any of these checks fail.
     */
    void check_available_periphs()
    {
        if (_num_radios_per_board == 0) {
            throw uhd::runtime_error("For legacy APIs, all devices require at least one radio.");
        }
        block_id_t radio_block_id(0, RADIO_BLOCK_NAME);
        block_id_t duc_block_id(0, DUC_BLOCK_NAME);
        block_id_t ddc_block_id(0, DDC_BLOCK_NAME);
        block_id_t fifo_block_id(0, DFIFO_BLOCK_NAME, 0);
        for (size_t i = 0; i < _num_mboards; i++) {
            radio_block_id.set_device_no(i);
            duc_block_id.set_device_no(i);
            ddc_block_id.set_device_no(i);
            fifo_block_id.set_device_no(i);
            for (size_t k = 0; k < _num_radios_per_board; k++) {
                radio_block_id.set_block_count(k);
                duc_block_id.set_block_count(k);
                ddc_block_id.set_block_count(k);
                // Only one FIFO per crossbar, so don't set block count for that block
                if (not _device->has_block(radio_block_id)
                    or (_has_ducs and not _device->has_block(duc_block_id))
                    or (_has_ddcs and not _device->has_block(ddc_block_id))
                    or (_has_dmafifo and not _device->has_block(fifo_block_id))
                ) {
                    throw uhd::runtime_error("For legacy APIs, all devices require the same number of radios, DDCs and DUCs.");
                }

                const size_t this_spp = get_block_ctrl<radio_ctrl>(i, RADIO_BLOCK_NAME, k)->get_arg<int>("spp");
                if (this_spp != _rx_spp) {
                    UHD_LOGGER_WARNING("RFNOC") << str(
                            boost::format("[legacy compat] Radios have differing spp values: %s has %d, others have %d. UHD will use smaller spp value for all connections. Performance might be not optimal.")
                            % radio_block_id.to_string() % this_spp % _rx_spp
                    );
                }
            }
        }
    }

    /*! Initialize properties in property tree to match legacy mode
     */
    void setup_prop_tree()
    {
        for (size_t mboard_idx = 0; mboard_idx < _num_mboards; mboard_idx++) {
            uhd::fs_path root = mb_root(mboard_idx);
            // Subdev specs
            if (_tree->exists(root / "tx_subdev_spec")) {
                _tree->access<subdev_spec_t>(root / "tx_subdev_spec")
                    .add_coerced_subscriber(boost::bind(&legacy_compat_impl::set_subdev_spec, this, _1, mboard_idx, uhd::TX_DIRECTION))
                    .update()
                    .set_publisher(boost::bind(&legacy_compat_impl::get_subdev_spec, this, mboard_idx, uhd::TX_DIRECTION));
            } else {
                _tree->create<subdev_spec_t>(root / "tx_subdev_spec")
                    .add_coerced_subscriber(boost::bind(&legacy_compat_impl::set_subdev_spec, this, _1, mboard_idx, uhd::TX_DIRECTION))
                    .set_publisher(boost::bind(&legacy_compat_impl::get_subdev_spec, this, mboard_idx, uhd::TX_DIRECTION));
            }

            if (_tree->exists(root / "rx_subdev_spec")) {
                _tree->access<subdev_spec_t>(root / "rx_subdev_spec")
                    .add_coerced_subscriber(boost::bind(&legacy_compat_impl::set_subdev_spec, this, _1, mboard_idx, uhd::RX_DIRECTION))
                    .update()
                    .set_publisher(boost::bind(&legacy_compat_impl::get_subdev_spec, this, mboard_idx, uhd::RX_DIRECTION));
            } else {
                 _tree->create<subdev_spec_t>(root / "rx_subdev_spec")
                    .add_coerced_subscriber(boost::bind(&legacy_compat_impl::set_subdev_spec, this, _1, mboard_idx, uhd::RX_DIRECTION))
                    .set_publisher(boost::bind(&legacy_compat_impl::get_subdev_spec, this, mboard_idx, uhd::RX_DIRECTION));
            }

            if (not _has_ddcs) {
                for (size_t radio_idx = 0; radio_idx < _num_radios_per_board; radio_idx++) {
                    for (size_t chan = 0; chan < _num_rx_chans_per_radio; chan++) {
                        const uhd::fs_path rx_dsp_base_path(mb_root(mboard_idx) / "rx_dsps" / radio_idx / chan);
                        _tree->create<double>(rx_dsp_base_path / "rate/value")
                            .set(0.0)
                            .set_publisher(
                                boost::bind(
                                    &radio_ctrl::get_output_samp_rate,
                                    get_block_ctrl<radio_ctrl>(mboard_idx, RADIO_BLOCK_NAME, radio_idx),
                                    chan
                                )
                            )
                        ;
                        _tree->create<uhd::meta_range_t>(rx_dsp_base_path / "rate/range")
                            .set_publisher(
                                boost::bind(
                                    &legacy_compat_impl::lambda_get_samp_rate_range,
                                    this,
                                    mboard_idx, radio_idx, chan,
                                    uhd::RX_DIRECTION
                                )
                            )
                        ;
                        _tree->create<double>(rx_dsp_base_path / "freq/value")
                            .set_publisher([](){ return 0.0; })
                        ;
                        _tree->create<uhd::meta_range_t>(rx_dsp_base_path / "freq/range")
                            .set_publisher([](){ return uhd::meta_range_t(0.0, 0.0, 0.0); })
                        ;
                    }
                }
            } /* if not _has_ddcs */
            if (not _has_ducs) {
                for (size_t radio_idx = 0; radio_idx < _num_radios_per_board; radio_idx++) {
                    for (size_t chan = 0; chan < _num_tx_chans_per_radio; chan++) {
                        const uhd::fs_path tx_dsp_base_path(mb_root(mboard_idx) / "tx_dsps" / radio_idx / chan);
                        _tree->create<double>(tx_dsp_base_path / "rate/value")
                            .set(0.0)
                            .set_publisher(
                                boost::bind(
                                    &radio_ctrl::get_output_samp_rate,
                                    get_block_ctrl<radio_ctrl>(mboard_idx, RADIO_BLOCK_NAME, radio_idx),
                                    chan
                                )
                            )
                        ;
                        _tree->create<uhd::meta_range_t>(tx_dsp_base_path / "rate/range")
                            .set_publisher(
                                boost::bind(
                                    &legacy_compat_impl::lambda_get_samp_rate_range,
                                    this,
                                    mboard_idx, radio_idx, chan,
                                    uhd::TX_DIRECTION
                                )
                            )
                        ;
                        _tree->create<double>(tx_dsp_base_path / "freq/value")
                            .set_publisher([](){ return 0.0; })
                        ;
                        _tree->create<uhd::meta_range_t>(tx_dsp_base_path / "freq/range")
                            .set_publisher([](){ return uhd::meta_range_t(0.0, 0.0, 0.0); })
                        ;
                    }
                }
            } /* if not _has_ducs */
        }
    }


    /*! Remove properties with bound functions in property tree and recreate
     */
    void remove_prop_subscribers()
    {
        for (size_t mboard_idx = 0; mboard_idx < _num_mboards; mboard_idx++) {
            uhd::fs_path root = mb_root(mboard_idx);
            // Subdev specs
            if (_tree->exists(root / "tx_subdev_spec")) {
               recreate_property<subdev_spec_t>(root / "tx_subdev_spec", _tree);
            }

            if (_tree->exists(root / "rx_subdev_spec")) {
               recreate_property<subdev_spec_t>(root / "rx_subdev_spec", _tree);
            }
        }
    }
    //! Flatten block list into a list of <block, port_index>
    // For example block list {b0[0,1] ,b1[0,1]} (i.e block 0 with 2 port 0 and 1,etc ..)
    // this will return {<b0,0> <b1,0> <b0,1> <b1,1>}
    std::vector<source_port_t> _flatten_blocks_by_n_ports(source_block_list_t block_list)
    {
        std::vector<source_port_t> result;
        for (auto block: block_list ){
            for(auto port: block->get_output_ports()){

                 result.push_back(std::make_pair(block,port));
            }
        }
        //assign to block prior ports
        size_t port = 0;
        size_t i = 0;
        for (size_t j=0; j<result.size(); j++){
            auto block = block_list[j%block_list.size()];
            UHD_ASSERT_THROW(port < block->get_output_ports().size());
            if (i == block_list.size())
            {
                i = 0;
                port ++;
            }
            result[j] = std::make_pair(block,port);
            i++;
        }
        return result;
    }

    //! Flatten block list into a list of <block, port_index>
    // For example block list {b0[0,1] ,b1[0,1]} (i.e block 0 with 2 port 0 and 1,etc ..)
    // this will return {<b0,0> <b1,0> <b0,1> <b1,1>}
    std::vector<sink_port_t> _flatten_blocks_by_n_ports(sink_block_list_t block_list)
    {
        std::vector<sink_port_t> result;
        for (auto block: block_list ){
            for(auto port: block->get_input_ports()){
                 result.push_back(std::make_pair(block,port));
            }
        }
        //assign to block prior ports
        size_t port = 0;
        size_t i = 0;
        for (size_t j=0; j<result.size(); j++){
            auto block = block_list[j%block_list.size()];
            UHD_ASSERT_THROW(port < block->get_input_ports().size());
            if (i == block_list.size())
            {
                i = 0;
                port ++;
            }
            result[j] = std::make_pair(block,port);
            i++;
        }
        return result;
    }

    /*! Default block connections.
     *
     * Tx connections:
     *
     * [Host] => DMA FIFO => DUC => Radio
     *
     * Note: There is only one DMA FIFO per crossbar, with twice the number of ports.
     *
     * Rx connections:
     *
     * Radio => DDC => [Host]
     *
     * Streamers are *not* generated here.
     */
    void connect_blocks()
    {
        _graph = _device->create_graph("legacy");
        const size_t rx_bpp = _rx_spp * BYTES_PER_SAMPLE + MAX_BYTES_PER_HEADER;
        const size_t tx_bpp = _tx_spp * BYTES_PER_SAMPLE + MAX_BYTES_PER_HEADER;
        block_name_to_block_map_t legacy_block_map =  get_legacy_blocks(_device);
        size_t index = 0, sram_fifo_index = 0, dma_fifo_index = 0;
        auto ddc_snk_flat = _flatten_blocks_by_n_ports(legacy_block_map[DDC_BLOCK_NAME].second);
        auto duc_src_flat = _flatten_blocks_by_n_ports(legacy_block_map[DUC_BLOCK_NAME].first);
        auto duc_snk_flat = _flatten_blocks_by_n_ports(legacy_block_map[DUC_BLOCK_NAME].second);
        auto radio_src_flat = _flatten_blocks_by_n_ports(legacy_block_map[RADIO_BLOCK_NAME].first);
        auto radio_snk_flat = _flatten_blocks_by_n_ports(legacy_block_map[RADIO_BLOCK_NAME].second);
        auto sfifo_src_flat =  _flatten_blocks_by_n_ports(legacy_block_map[SFIFO_BLOCK_NAME].first);
        auto dfifo_src_flat =  _flatten_blocks_by_n_ports(legacy_block_map[DFIFO_BLOCK_NAME].first);
        for(auto each_src_radio_block:radio_src_flat){
            auto radio_block = each_src_radio_block.first->get_block_id();
            if (_has_ddcs) {
                UHD_ASSERT_THROW(index < ddc_snk_flat.size());
                auto ddc_block = ddc_snk_flat[index].first->get_block_id();
                _graph->connect(
                    radio_block, each_src_radio_block.second,
                    ddc_block, ddc_snk_flat[index].second,
                    rx_bpp
                );
            }
            index++;
        }
        index = 0;
        for(auto each_snk_radio_block:radio_snk_flat){
            auto radio_block = each_snk_radio_block.first->get_block_id();
            auto down_stream_block = radio_block;
            auto down_stream_port  = each_snk_radio_block.second;
            if (_has_ducs) {
                UHD_ASSERT_THROW(index < duc_snk_flat.size());
                UHD_ASSERT_THROW(index < duc_src_flat.size());
                auto duc_snk_block = duc_snk_flat[index].first->get_block_id();
                auto duc_src_block = duc_src_flat[index].first->get_block_id();
                _graph->connect(
                    duc_src_block, duc_src_flat[index].second,
                    radio_block, each_snk_radio_block.second,
                    rx_bpp);
                down_stream_block = duc_snk_block;
                down_stream_port = duc_snk_flat[index].second;
            }
            if (_has_sramfifo) {
                if(sram_fifo_index < sfifo_src_flat.size()){
                    auto sfifo_block =  sfifo_src_flat[sram_fifo_index].first->get_block_id();
                    _graph->connect(
                        sfifo_block, sfifo_src_flat[sram_fifo_index].second,
                        down_stream_block, down_stream_port,
                        tx_bpp
                    );
                    sram_fifo_index++;
                }else {
                    UHD_LOGGER_WARNING("RFNOC") << "[legacy compat] Running out of SRAM FIFO ports to connect.";
                }
            }else if (_has_dmafifo) {
                if(dma_fifo_index < dfifo_src_flat.size()){
                    auto dfifo_block = dfifo_src_flat[dma_fifo_index].first->get_block_id();
                    _graph->connect(
                        dfifo_block, dfifo_src_flat[dma_fifo_index].second,
                        down_stream_block, down_stream_port,
                        tx_bpp
                    );
                    dma_fifo_index++;
                }else {
                     UHD_LOGGER_WARNING("RFNOC") << "[legacy compat] Running out of DRAM FIFO ports to connect.";
                }
            }
            index++;
        }
    }


    /************************************************************************
     * Subdev translation
     ***********************************************************************/
    /*! Subdev -> (Radio, Port)
     *
     * Example: Device is X300, subdev spec is 'A:0 B:0', we have 2 radios.
     * Then we map to ((0, 0), (1, 0)). I.e., zero-th port on radio 0 and
     * radio 1, respectively.
     */
    void set_subdev_spec(const subdev_spec_t &spec, const size_t mboard, const uhd::direction_t dir)
    {
        UHD_ASSERT_THROW(mboard < _num_mboards);
        chan_map_t &chan_map = (dir == uhd::TX_DIRECTION) ? _tx_channel_map : _rx_channel_map;

        std::vector<radio_port_pair_t> new_mapping(spec.size());
        for (size_t i = 0; i < spec.size(); i++) {
            const size_t new_radio_index = get_radio_index(spec[i].db_name);
            radio_ctrl::sptr radio = get_block_ctrl<radio_ctrl>(mboard, "Radio", new_radio_index);
            size_t new_port_index = radio->get_chan_from_dboard_fe(spec[i].sd_name, dir);
            auto port_size  = (dir == uhd::TX_DIRECTION) ? radio->get_input_ports().size() :radio->get_output_ports().size();
            auto default_index = (dir == uhd::TX_DIRECTION)? radio->get_input_ports().at(0) : radio->get_output_ports().at(0);
            if (new_port_index >= port_size) {
                new_port_index = default_index;
            }

            radio_port_pair_t new_radio_port_pair(new_radio_index, new_port_index);
            new_mapping[i] = new_radio_port_pair;
        }
        chan_map[mboard] = new_mapping;
    }

    subdev_spec_t get_subdev_spec(const size_t mboard, const uhd::direction_t dir)
    {
        UHD_ASSERT_THROW(mboard < _num_mboards);
        subdev_spec_t subdev_spec;
        chan_map_t &chan_map = (dir == uhd::TX_DIRECTION) ? _tx_channel_map : _rx_channel_map;
        for (size_t chan_idx = 0; chan_idx < chan_map[mboard].size(); chan_idx++) {
            const size_t radio_index = chan_map[mboard][chan_idx].radio_index;
            const size_t port_index = chan_map[mboard][chan_idx].port_index;
            const std::string new_db_name = get_slot_name(radio_index);
            const std::string new_sd_name =
                get_block_ctrl<radio_ctrl>(mboard, "Radio", radio_index)->get_dboard_fe_from_chan(port_index, dir);
            subdev_spec_pair_t new_pair(new_db_name, new_sd_name);
            subdev_spec.push_back(new_pair);
        }

        return subdev_spec;
    }

    void update_tick_rate_on_blocks(const double tick_rate, const size_t mboard_idx)
    {
        block_id_t radio_block_id(mboard_idx, RADIO_BLOCK_NAME);
        block_id_t duc_block_id(mboard_idx, DUC_BLOCK_NAME);
        block_id_t ddc_block_id(mboard_idx, DDC_BLOCK_NAME);

        for (size_t radio = 0; radio < _num_radios_per_board; radio++) {
            radio_block_id.set_block_count(radio);
            duc_block_id.set_block_count(radio);
            ddc_block_id.set_block_count(radio);
            radio_ctrl::sptr radio_sptr = _device->get_block_ctrl<radio_ctrl>(radio_block_id);
            radio_sptr->set_rate(tick_rate);
            for (size_t chan = 0; chan < _num_rx_chans_per_radio and _has_ddcs; chan++) {
                const double radio_output_rate = radio_sptr->get_output_samp_rate(chan);
                _device->get_block_ctrl(ddc_block_id)->set_arg<double>("input_rate", radio_output_rate, chan);
            }
            for (size_t chan = 0; chan < _num_tx_chans_per_radio and _has_ducs; chan++) {
                const double radio_input_rate = radio_sptr->get_input_samp_rate(chan);
                _device->get_block_ctrl(duc_block_id)->set_arg<double>("output_rate", radio_input_rate, chan);
            }
        }
    }

private: // attributes
    uhd::device3::sptr _device;
    uhd::property_tree::sptr _tree;

    const bool _has_ducs;
    const bool _has_ddcs;
    const bool _has_dmafifo;
    const bool _has_sramfifo;
    const size_t _num_mboards;
    const size_t _num_radios_per_board;
    const size_t _num_tx_chans_per_radio;
    const size_t _num_rx_chans_per_radio;
    size_t _rx_spp;
    size_t _tx_spp;

    chan_map_t _rx_channel_map;
    chan_map_t _tx_channel_map;

    //! Stores a weak pointer for every streamer that's generated through this API.
    // Key is the channel number (same format as e.g. the set_rx_rate() call).
    typedef std::map< size_t, boost::weak_ptr<uhd::rx_streamer> > rx_stream_map_type;
    rx_stream_map_type _rx_stream_cache;
    typedef std::map< size_t, boost::weak_ptr<uhd::tx_streamer> > tx_stream_map_type;
    tx_stream_map_type _tx_stream_cache;

    graph::sptr _graph;
};

legacy_compat::sptr legacy_compat::make(
        uhd::device3::sptr device,
        const uhd::device_addr_t &args
) {
    boost::lock_guard<boost::mutex> lock(_make_mutex);
    UHD_ASSERT_THROW(bool(device));
    static std::map<void *, boost::weak_ptr<legacy_compat> > legacy_cache;

    if (legacy_cache.count(device.get()) and not legacy_cache.at(device.get()).expired()) {
        legacy_compat::sptr legacy_compat_copy = legacy_cache.at(device.get()).lock();
        UHD_ASSERT_THROW(bool(legacy_compat_copy));
        UHD_LEGACY_LOG() << "[legacy_compat] Using existing legacy compat object for this device." ;
        return legacy_compat_copy;
    }

    legacy_compat::sptr new_legacy_compat = boost::make_shared<legacy_compat_impl>(device, args);
    legacy_cache[device.get()] = new_legacy_compat;
    return new_legacy_compat;
}


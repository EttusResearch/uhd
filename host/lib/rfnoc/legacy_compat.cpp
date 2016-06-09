//
// Copyright 2016 Ettus Research LLC
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

#include "legacy_compat.hpp"
#include <uhd/property_tree.hpp>
#include <uhd/rfnoc/radio_ctrl.hpp>
#include <uhd/rfnoc/ddc_block_ctrl.hpp>
#include <uhd/rfnoc/graph.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/stream.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/utils/msg.hpp>
#include <boost/make_shared.hpp>

using namespace uhd::rfnoc;
using uhd::usrp::subdev_spec_t;
using uhd::usrp::subdev_spec_pair_t;
using uhd::stream_cmd_t;

/************************************************************************
 * Static helpers
 ***********************************************************************/
uhd::fs_path mb_root(const size_t mboard)
{
    return uhd::fs_path("/mboards") / mboard;
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
    legacy_compat_impl(uhd::device3::sptr device)
      : _device(device),
        _tree(device->get_tree()),
        _num_mboards(_tree->list("/mboards").size()),
        _num_radios_per_board(device->find_blocks<radio_ctrl>("0/Radio").size()), // These might throw, maybe we catch that and provide a nicer error message.
        _num_tx_chans_per_radio(_tree->list("/mboards/0/xbar/Radio_0/ports/in").size()),
        _num_rx_chans_per_radio(_tree->list("/mboards/0/xbar/Radio_0/ports/out").size()),
        _has_ducs(not device->find_blocks<radio_ctrl>("0/DUC").empty()),
        _has_ddcs(not device->find_blocks<radio_ctrl>("0/DDC").empty()),
        _has_dmafifo(not device->find_blocks<radio_ctrl>("0/DmaFIFO").empty()),
        _rx_channel_map(_num_mboards, std::vector<radio_port_pair_t>(_num_radios_per_board)),
        _tx_channel_map(_num_mboards, std::vector<radio_port_pair_t>(_num_radios_per_board))
    {
        check_available_periphs(); // Throws if invalid configuration.
        setup_prop_tree();
        connect_blocks();

        for (size_t mboard = 0; mboard < _num_mboards; mboard++) {
            for (size_t radio = 0; radio < _num_radios_per_board; radio++) {
                _rx_channel_map[mboard][radio].radio_index = radio;
                _tx_channel_map[mboard][radio].radio_index = radio;
            }
        }
    }

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
        // FIXME test for DRAM FIFOs
        block_id_t radio_block_id(0, "Radio");
        block_id_t duc_block_id(0, "DUC");
        block_id_t ddc_block_id(0, "DDC");
        block_id_t fifo_block_id(0, "DmaFIFO", 0);
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
            _tree->create<subdev_spec_t>(root / "tx_subdev_spec")
                .add_coerced_subscriber(boost::bind(&legacy_compat_impl::set_subdev_spec, this, _1, mboard_idx, uhd::TX_DIRECTION))
                .set_publisher(boost::bind(&legacy_compat_impl::get_subdev_spec, this, mboard_idx, uhd::TX_DIRECTION))
            ;
            _tree->create<subdev_spec_t>(root / "rx_subdev_spec")
                .add_coerced_subscriber(boost::bind(&legacy_compat_impl::set_subdev_spec, this, _1, mboard_idx, uhd::RX_DIRECTION))
                .set_publisher(boost::bind(&legacy_compat_impl::get_subdev_spec, this, mboard_idx, uhd::RX_DIRECTION))
            ;

            if (not _has_ddcs or not _has_ducs) {
                const uhd::fs_path dsp_base_path(uhd::fs_path("/stubs/dsp/") / mboard_idx);
                _tree->create<double>(dsp_base_path / "rate/value")
                    .set(get_tick_rate(mboard_idx))
                    .add_coerced_subscriber(boost::bind(&legacy_compat_impl::set_tick_rate, this, _1, mboard_idx))
                    .set_publisher(boost::bind(&legacy_compat_impl::get_tick_rate, this, mboard_idx))
                ;
                _tree->create<uhd::meta_range_t>(dsp_base_path / "rate/range")
                    .set(uhd::meta_range_t(get_tick_rate(), get_tick_rate(), 0.0))
                    .set_publisher(boost::bind(&legacy_compat_impl::lambda_get_tick_rate_range, this, mboard_idx))
                ;
                _tree->create<double>(dsp_base_path / "freq/value")
                    .set(0.0)
                ;
                _tree->create<uhd::meta_range_t>(dsp_base_path / "freq/range")
                    .set(uhd::meta_range_t(0.0, 0.0, 0.0));
                ;
            }
        }
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
        for (size_t mboard = 0; mboard < _num_mboards; mboard++) {
            for (size_t radio = 0; radio < _num_radios_per_board; radio++) {
                // Tx Channels
                for (size_t chan = 0; chan < _num_tx_chans_per_radio; chan++) {
                    if (_has_ducs) {
                        _graph->connect(
                            block_id_t(mboard, "DUC",   radio), chan,
                            block_id_t(mboard, "Radio", radio), chan
                        );
                        if (_has_dmafifo) {
                            // We have DMA FIFO *and* DUCs
                            _graph->connect(
                                block_id_t(mboard, "DmaFIFO", 0), radio * _num_radios_per_board + chan,
                                block_id_t(mboard, "DUC", radio), chan
                            );
                        }
                    } else if (_has_dmafifo) {
                            // We have DMA FIFO, *no* DUCs
                            _graph->connect(
                                block_id_t(mboard, "DmaFIFO",   0), radio * _num_radios_per_board + chan,
                                block_id_t(mboard, "Radio", radio), chan
                            );
                    }
                }
                // Rx Channels
                for (size_t chan = 0; chan < _num_rx_chans_per_radio; chan++) {
                    if (_has_ddcs) {
                        _graph->connect(
                            block_id_t(mboard, "DDC",   radio), chan,
                            block_id_t(mboard, "Radio", radio), chan
                        );
                    }
                }
            }
        }
    }

    /************************************************************************
     * API Calls
     ***********************************************************************/
    uhd::fs_path rx_dsp_root(const size_t mboard_idx, const size_t chan)
    {
        if (not _has_ddcs) {
            return uhd::fs_path("/stubs/dsp") / mboard_idx;
        }
        // The DSP index is the same as the radio index
        size_t dsp_index = _rx_channel_map[mboard_idx][chan].radio_index;
        size_t port_index = _rx_channel_map[mboard_idx][chan].port_index;
        return mb_root(mboard_idx) / "xbar" /
               str(boost::format("DDC_%d") % dsp_index) /
               "legacy_api" / port_index;
    }

    uhd::fs_path tx_dsp_root(const size_t mboard_idx, const size_t chan)
    {
        if (not _has_ducs) {
            return uhd::fs_path("/stubs/dsp") / mboard_idx;
        }
        // The DSP index is the same as the radio index
        size_t dsp_index = _tx_channel_map[mboard_idx][chan].radio_index;
        size_t port_index = _tx_channel_map[mboard_idx][chan].port_index;
        return mb_root(mboard_idx) / "xbar" /
               str(boost::format("DUC_%d") % dsp_index) /
               "legacy_api" / port_index;
    }

    uhd::fs_path rx_fe_root(const size_t mboard_idx, const size_t chan)
    {
        size_t radio_index = _rx_channel_map[mboard_idx][chan].radio_index;
        size_t port_index = _rx_channel_map[mboard_idx][chan].port_index;
        return uhd::fs_path(str(
                boost::format("/mboards/%d/xbar/Radio_%d/rx_fe_corrections/%d/")
                % mboard_idx % radio_index % port_index
        ));
    }

    uhd::fs_path tx_fe_root(const size_t mboard_idx, const size_t chan)
    {
        size_t radio_index = _tx_channel_map[mboard_idx][chan].radio_index;
        size_t port_index = _tx_channel_map[mboard_idx][chan].port_index;
        return uhd::fs_path(str(
                boost::format("/mboards/%d/xbar/Radio_%d/tx_fe_corrections/%d/")
                % mboard_idx % radio_index % port_index
        ));
    }

    void issue_stream_cmd(const stream_cmd_t &stream_cmd, size_t mboard, size_t chan)
    {
        UHD_MSG(status) << "[legacy_compat] issue_stream_cmd() " << std::endl;
        const size_t &radio_index = _rx_channel_map[mboard][chan].radio_index;
        const size_t &port_index  = _rx_channel_map[mboard][chan].port_index;
        if (_has_ddcs) {
            get_block_ctrl<ddc_block_ctrl>(mboard, "DDC", radio_index)->issue_stream_cmd(stream_cmd, port_index);
        } else {
            get_block_ctrl<radio_ctrl>(mboard, "Radio", radio_index)->issue_stream_cmd(stream_cmd, port_index);
        }
    }

    //! Sets block_id<N> and block_port<N> in the streamer args, otherwise forwards the call
    uhd::rx_streamer::sptr get_rx_stream(const uhd::stream_args_t &args_)
    {
        uhd::stream_args_t args(args_);
        size_t mboard_idx = 0;
        size_t chan_idx = 0;
        for (size_t i = 0; i < args.channels.size(); i++) {
            UHD_ASSERT_THROW(mboard_idx < _rx_channel_map.size());
            const size_t &radio_index = _rx_channel_map[mboard_idx][chan_idx].radio_index;
            const size_t &port_index = _rx_channel_map[mboard_idx][chan_idx].port_index;
            block_id_t block_id(mboard_idx, _has_ddcs ? "DDC" : "Radio", radio_index);
            args.args[str(boost::format("block_id%d") % i)] = block_id.to_string();
            args.args[str(boost::format("block_port%d") % i)] = str(boost::format("%d") % port_index);
            chan_idx++;
            if (chan_idx > _rx_channel_map[mboard_idx].size()) {
                chan_idx = 0;
                mboard_idx++;
            }
        }
        UHD_MSG(status) << "[legacy_compat] rx stream args: " << args.args.to_string() << std::endl;
        return _device->get_rx_stream(args);
    }

    //! Sets block_id<N> and block_port<N> in the streamer args, otherwise forwards the call
    uhd::tx_streamer::sptr get_tx_stream(const uhd::stream_args_t &args_)
    {
        uhd::stream_args_t args(args_);
        size_t mboard_idx = 0;
        size_t chan_idx = 0;
        const std::string block_name = _has_dmafifo ? "DmaFIFO" : (_has_ducs ? "DUC" : "Radio");
        for (size_t i = 0; i < args.channels.size(); i++) {
            UHD_ASSERT_THROW(mboard_idx < _tx_channel_map.size());
            const size_t &radio_index = _tx_channel_map[mboard_idx][chan_idx].radio_index;
            const size_t &port_index = _has_dmafifo ? chan_idx : _tx_channel_map[mboard_idx][chan_idx].port_index;
            block_id_t block_id(mboard_idx, block_name, radio_index);
            args.args[str(boost::format("block_id%d") % i)] = block_id.to_string();
            args.args[str(boost::format("block_port%d") % i)] = str(boost::format("%d") % port_index);
            chan_idx++;
            if (chan_idx > _tx_channel_map[mboard_idx].size()) {
                chan_idx = 0;
                mboard_idx++;
            }
        }
        UHD_MSG(status) << "[legacy_compat] tx stream args: " << args.args.to_string() << std::endl;
        return _device->get_tx_stream(args);
    }

    double get_tick_rate(const size_t mboard_idx=0)
    {
        return _tree->access<double>(mb_root(mboard_idx) / "tick_rate").get();
    }

    uhd::meta_range_t lambda_get_tick_rate_range(const size_t mboard_idx=0)
    {
        const double tick_rate = get_tick_rate(mboard_idx);
        return uhd::meta_range_t(tick_rate, tick_rate, 0.0);
    }

    void set_tick_rate(const double tick_rate, const size_t mboard_idx=0)
    {
        _tree->access<double>(mb_root(mboard_idx) / "tick_rate").set(tick_rate);
    }

private: // methods
    /************************************************************************
     * Private helpers
     ***********************************************************************/
    std::string get_slot_name(const size_t radio_index)
    {
        return (radio_index == 0) ? "A" : "B";
    }

    size_t get_radio_index(const std::string slot_name)
    {
        return (slot_name == "A") ? 0 : 1;
    }

    template <typename block_type>
    inline typename block_type::sptr get_block_ctrl(const size_t mboard_idx, const std::string &name, const size_t block_count)
    {
        block_id_t block_id(mboard_idx, name, block_count);
        return _device->get_block_ctrl<block_type>(block_id);
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
            const size_t new_port_index =
                get_block_ctrl<radio_ctrl>(mboard, "Radio", new_radio_index)->get_chan_from_dboard_fe(spec[i].sd_name, dir);
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


private: // attributes
    uhd::device3::sptr _device;
    uhd::property_tree::sptr _tree;

    const size_t _num_mboards;
    const size_t _num_radios_per_board;
    const size_t _num_tx_chans_per_radio;
    const size_t _num_rx_chans_per_radio;
    const bool _has_ducs;
    const bool _has_ddcs;
    const bool _has_dmafifo;

    struct radio_port_pair_t {
        radio_port_pair_t(const size_t radio=0, const size_t port=0) : radio_index(radio), port_index(port) {}
        size_t radio_index;
        size_t port_index;
    };
    //! Map: _rx_channel_map[mboard_idx][chan_idx] => (Radio, Port)
    // Container is not a std::map because we need to guarantee contiguous
    // ports and correct order anyway.
    typedef std::vector< std::vector<radio_port_pair_t> > chan_map_t;
    chan_map_t _rx_channel_map;
    chan_map_t _tx_channel_map;

    graph::sptr _graph;

};

legacy_compat::sptr legacy_compat::make(uhd::device3::sptr device)
{
    return boost::make_shared<legacy_compat_impl>(device);
}


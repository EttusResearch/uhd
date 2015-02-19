//
// Copyright 2014-2015 Ettus Research LLC
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

#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <uhd/convert.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/types/ranges.hpp>
#include "radio_ctrl.hpp"
#include "../../transport/super_recv_packet_handler.hpp"

using namespace uhd;
using namespace uhd::rfnoc;

class radio_ctrl_impl : public radio_ctrl
{
public:
    UHD_RFNOC_BLOCK_CONSTRUCTOR(radio_ctrl),
        _dboard_type(DBOARD_TYPE_UNKNOWN)
    {
        //// RX Streamer args
        // CPU format doesn't really matter, just init it to something
        _rx_stream_args = uhd::stream_args_t("fc32", "sc16");
        _tx_stream_args = uhd::stream_args_t("fc32", "sc16");
        _rx_bpi = uhd::convert::get_bytes_per_item(_rx_stream_args.otw_format);
        // Default: 1 Channel
        _rx_stream_args.channels = std::vector<size_t>(1, 0);
        // SPP is stored for calls to set_output_signature() etc.
        //_rx_spp = get_output_signature(0).packet_size / _rx_bpi;
        //if (_rx_spp == 0) {
            int _rx_spp = DEFAULT_PACKET_SIZE / _rx_bpi;
        //}
        UHD_MSG(status) << "radio_ctrl::radio_ctrl() _rx_spp==" << _rx_spp << std::endl;

        // TODO: Once the radio looks like a NoC-Block, remove this!
        _tree->remove(_root_path / "input_buffer_size");
        _tree->create<size_t>(_root_path / "input_buffer_size/0").set(520*1024/2);

        // TODO this is a hack
        stream_sig_t out_sig;
        out_sig.item_type = "sc16";
        out_sig.packet_size = _rx_spp * _rx_bpi;
        if (_tree->exists(_root_path / "output_sig/0")) {
            _tree->access<stream_sig_t>(_root_path / "output_sig/0").set(out_sig);
        } else {
            _tree->create<stream_sig_t>(_root_path / "output_sig/0").set(out_sig);
        }
        if (_tree->exists(_root_path / "input_sig/0")) {
            _tree->access<stream_sig_t>(_root_path / "input_sig/0").set(out_sig);
        } else {
            _tree->create<stream_sig_t>(_root_path / "input_sig/0").set(out_sig);
        }

        _tree->create<bool>(_root_path / "tx_active").set(false);
        _tree->create<bool>(_root_path / "rx_active").set(false);

        // TODO These should come from an XML file:
        fs_path arg_path = _root_path / "args";
        // spp:
        _tree->create<std::string>(arg_path / "spp" / "type").set("int");
        _tree->create<int>(arg_path / "spp" / "value").set(_rx_spp)
            .subscribe(boost::bind(&radio_ctrl_impl::_update_spp, this, _1))
        ;
        // rx args:
        _tree->create<std::string>(arg_path / "rx_args" / "type").set("string");
        _tree->create<std::string>(arg_path / "rx_args" / "value").set("")
            .subscribe(boost::bind(&radio_ctrl_impl::_update_rx_args, this, _1))
        ;
        _tree->create<std::string>(arg_path / "tx_args" / "type").set("string");
        _tree->create<std::string>(arg_path / "tx_args" / "value").set("")
            .subscribe(boost::bind(&radio_ctrl_impl::_update_tx_args, this, _1))
        ;
    }

    /***********************************************************************
     * Noc-Shell methods (from block_ctrl_base)
     **********************************************************************/
    //! Configure flow control in the VITA core
    void configure_flow_control_in(
            size_t cycles,
            size_t packets,
            size_t block_port
    ) {
        UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl::configure_flow_control_in()" << cycles << " " << packets << std::endl;
        UHD_ASSERT_THROW(block_port == 0);
        _perifs.deframer->configure_flow_control(cycles, packets);
    }

    //! Configure flow control in the VITA core
    void configure_flow_control_out(
            size_t buf_size_pkts,
            size_t,
            const uhd::sid_t &
    ) {
        UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl::configure_flow_control_out()" << buf_size_pkts << std::endl;
        _perifs.framer->configure_flow_control(buf_size_pkts);
    }

    //! Clear the Rx & Tx VITA cores
    void _clear()
    {
        _perifs.deframer->clear();
        _perifs.framer->clear();
    }

    //! Set packet size in the VITA framer
    bool set_output_signature(const stream_sig_t &out_sig_, size_t block_port)
    {
        UHD_ASSERT_THROW(block_port == 0);
        stream_sig_t out_sig = out_sig_;

        if (out_sig_.packet_size % _rx_bpi) {
            return false;
        }

        if (source_block_ctrl_base::set_output_signature(out_sig, block_port)) {
            set_arg<int>("spp", int(out_sig_.packet_size / _rx_bpi));
            return true;
        }

        return false;
    }

    //! Writes the full source address to the VITA core
    void set_destination(
            boost::uint32_t next_address,
            size_t out_block_port
    ) {
        UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl::set_destination()" << std::endl;
        UHD_ASSERT_THROW(out_block_port == 0);
        uhd::sid_t sid(next_address);
        if (sid.get_src() == 0) {
            sid.set_src(get_address());
        }
        UHD_MSG(status) << "  Setting sid to " << sid << std::endl;

        _perifs.framer->set_sid(sid.get());
    }


    /***********************************************************************
     * RX Streamer-related methods (from source_block_ctrl_base)
     **********************************************************************/
    //! Pass stream commands to the radio
    void issue_stream_cmd(const uhd::stream_cmd_t &stream_cmd)
    {
        UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl::issue_stream_cmd()" << std::endl;
        _perifs.framer->issue_stream_command(stream_cmd);
    }

    void handle_overrun(boost::weak_ptr<uhd::rx_streamer> streamer)
    {
        UHD_MSG(status) << "radio_ctrl::handle_overrun()" << std::endl;
        boost::shared_ptr<transport::sph::recv_packet_streamer> my_streamer =
                boost::dynamic_pointer_cast<transport::sph::recv_packet_streamer>(streamer.lock());
        if (not my_streamer) return; //If the rx_streamer has expired then overflow handling makes no sense.

        if (my_streamer->get_num_channels() == 1) {
            _perifs.framer->handle_overflow();
            return;
        }

        /////////////////////////////////////////////////////////////
        // MIMO overflow recovery time
        /////////////////////////////////////////////////////////////
        //find out if we were in continuous mode before stopping
        const bool in_continuous_streaming_mode = _perifs.framer->in_continuous_streaming_mode();
        //stop streaming
        my_streamer->issue_stream_cmd(stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
        //flush transports
        my_streamer->flush_all(0.001);
        //restart streaming
        if (in_continuous_streaming_mode)
        {
            stream_cmd_t stream_cmd(stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
            stream_cmd.stream_now = false;
            stream_cmd.time_spec = _perifs.time64->get_time_now() + time_spec_t(0.01);
            my_streamer->issue_stream_cmd(stream_cmd);
        }
    }


    /***********************************************************************
     * Rate controls (from rate_node_ctrl)
     **********************************************************************/
    /*! Returns the sampling rate the radio core is currently configured to.
     */
    double get_input_samp_rate(size_t /* port */)
    {
        uhd::fs_path dsp_path = "tx_dsps";
        return _tree->access<double>(dsp_path / get_block_id().get_block_count() / "rate/value").get();
    }


    /*! Returns the sampling rate the radio core is currently configured to.
     */
    double get_output_samp_rate(size_t /* port */)
    {
        uhd::fs_path dsp_path = "rx_dsps";
        return _tree->access<double>(dsp_path / get_block_id().get_block_count() / "rate/value").get();
    }


    /***********************************************************************
     * Tick controls (from tick_node_ctrl)
     **********************************************************************/
    double _get_tick_rate()
    {
        return _tree->access<double>("tick_rate").get();
    };

    /***********************************************************************
     * Scaling controls (from scalar_node_ctrl)
     **********************************************************************/
    double get_input_scale_factor(size_t /* port */)
    {
        return _perifs.duc->get_scaling_adjustment();
    }

    double get_output_scale_factor(size_t /* port */)
    {
        return _perifs.ddc->get_scaling_adjustment();
    }

    /***********************************************************************
     * Radio controls (radio_ctrl specific)
     **********************************************************************/
    void set_perifs(
        time_core_3000::sptr       time64,
        rx_vita_core_3000::sptr    framer,
        rx_dsp_core_3000::sptr     ddc,
        tx_vita_core_3000::sptr    deframer,
        tx_dsp_core_3000::sptr     duc,
        rx_frontend_core_200::sptr rx_fe,
        tx_frontend_core_200::sptr tx_fe
    ) {
        _perifs.time64 = time64;
        _perifs.framer = framer;
        _perifs.ddc = ddc;
        _perifs.deframer = deframer;
        _perifs.duc = duc;
        _perifs.rx_fe = rx_fe;
        _perifs.tx_fe = tx_fe;

        // Now we can access the perifs, update their settings:
        _tree->access<int>(_root_path / "args" / "spp" / "value").update();
        _tree->access<int>(_root_path / "args" / "rx_args" / "value").update();
        _tree->access<int>(_root_path / "args" / "tx_args" / "value").update();
    }

    void set_dboard_type(dboard_type_t type)
    {
        _dboard_type = type;
    }

    void update_muxes(uhd::direction_t dir)
    {
        UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl::update_muxes() " << std::endl;

        // 1) Figure out dboard and frontend name
        std::string db, fe;
        std::string fe_path = (dir == TX_DIRECTION) ? "tx_frontends" : "rx_frontends";
        device_addr_t args = (dir == TX_DIRECTION) ? _tx_stream_args.args : _rx_stream_args.args;
        switch (_dboard_type) {
            case DBOARD_TYPE_UNKNOWN:
                return;

            case DBOARD_TYPE_SLOT:
                db = (get_block_id().get_block_count() == 0) ? "A" : "B";
                fe = args.cast<std::string>("frontend", _tree->list("dboards" / db / fe_path).at(0));
                break;

            case DBOARD_TYPE_AD9361:
                db = "A";
                fe = (get_block_id().get_block_count() == 0) ? "A" : "B";
                break;

            default:
                UHD_THROW_INVALID_CODE_PATH();
        }

        // 2) Set the muxes
        uhd::fs_path dbpath = uhd::fs_path("dboards") / db / fe_path / fe / "connection";
        const std::string conn = _tree->access<std::string>(dbpath).get();
        UHD_MSG(status) << "  " << dbpath << " == " << conn << std::endl;
        if (dir == TX_DIRECTION) {
            _perifs.tx_fe->set_mux(conn);
        } else {
            const bool fe_swapped = (conn == "QI" or conn == "Q");
            _perifs.ddc->set_mux(conn, fe_swapped);
            _perifs.rx_fe->set_mux(fe_swapped);
        }
    }

protected:
    void set_rx_streamer(bool active)
    {
        UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl::set_rx_streamer() " << active << std::endl;
        _tree->access<bool>(_root_path / "rx_active").set(active);
    }

    void set_tx_streamer(bool active)
    {
        UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl::set_tx_streamer() " << active << std::endl;
        _tree->access<bool>(_root_path / "tx_active").set(active);
    }

private:

    void _update_spp(int spp)
    {
        UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl::_update_spp(): Requested spp: " << spp << std::endl;
        if (spp == 0) {
            spp = DEFAULT_PACKET_SIZE / _rx_bpi;
        }
        UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl::_update_spp(): Setting spp to: " << spp << std::endl;
        if (_perifs.framer)
            _perifs.framer->set_nsamps_per_packet(size_t(spp));
    }

    void _update_rx_args(const std::string &new_rx_args)
    {
        UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl::_update_rx_args()" << std::endl;
        _rx_stream_args.args = device_addr_t(new_rx_args);
        if (_perifs.framer) {
            UHD_MSG(status) << "  Setting VITA core" << std::endl;
            _perifs.framer->setup(_rx_stream_args);
        }
        if (_perifs.ddc) {
            UHD_MSG(status) << "  Setting DSP core " << _rx_stream_args.args.to_string() << std::endl;
            // Cares about otw_format, 'peak' and 'fullscale'
            _perifs.ddc->setup(_rx_stream_args);
        }
        if (_perifs.rx_fe) {
            UHD_MSG(status) << "  Updating muxes " << std::endl;
            update_muxes(uhd::RX_DIRECTION);
        }
    }

    void _update_tx_args(const std::string &new_tx_args)
    {
        UHD_RFNOC_BLOCK_TRACE() << "radio_ctrl::_update_tx_args()" << std::endl;
        _tx_stream_args.args = device_addr_t(new_tx_args);
        if (_perifs.deframer) {
            UHD_MSG(status) << "  Setting VITA core" << std::endl;
            _perifs.deframer->setup(_tx_stream_args);
        }
        if (_perifs.duc) {
            UHD_MSG(status) << "  Setting DSP core " << _tx_stream_args.args.to_string() << std::endl;
            // Cares about otw_format, 'peak' and 'fullscale'
            _perifs.duc->setup(_tx_stream_args);
        }
        if (_perifs.tx_fe) {
            UHD_MSG(status) << "  Updating muxes " << std::endl;
            update_muxes(uhd::TX_DIRECTION);
        }
    }

    //! Stores pointers to all streaming-related radio cores
    struct radio_v_perifs_t
    {
        time_core_3000::sptr       time64;
        rx_vita_core_3000::sptr    framer;
        rx_dsp_core_3000::sptr     ddc;
        tx_vita_core_3000::sptr    deframer;
        tx_dsp_core_3000::sptr     duc;
        rx_frontend_core_200::sptr rx_fe;
        tx_frontend_core_200::sptr tx_fe;
    } _perifs;

    uhd::stream_args_t _rx_stream_args;
    uhd::stream_args_t _tx_stream_args;
    //! Bytes per item
    size_t _rx_bpi;

    dboard_type_t _dboard_type;
};

UHD_RFNOC_BLOCK_REGISTER(radio_ctrl, "Radio");
// vim: sw=4 expandtab:

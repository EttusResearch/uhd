//
// Copyright 2013-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "e300_network.hpp"

#ifdef E300_NATIVE

#include "e300_impl.hpp"

#include "e300_sensor_manager.hpp"
#include "e300_fifo_config.hpp"
#include "e300_spi.hpp"
#include "e300_i2c.hpp"
#include "e300_defaults.hpp"
#include "e300_common.hpp"
#include "e300_remote_codec_ctrl.hpp"

#include <uhd/utils/log.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/paths.hpp>

#include <uhdlib/usrp/common/ad9361_ctrl.hpp>

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>

#include <fstream>
#include <chrono>
#include <thread>

using namespace uhd;
using namespace uhd::transport;
namespace asio = boost::asio;
namespace fs = boost::filesystem;

namespace uhd { namespace usrp { namespace e300 {

static const size_t E300_NETWORK_DEBUG = false;

static inline bool wait_for_recv_ready(int sock_fd, const size_t timeout_ms)
{
    //setup timeval for timeout
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeout_ms*1000;

    //setup rset for timeout
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(sock_fd, &rset);

    //call select with timeout on receive socket
    return ::select(sock_fd+1, &rset, NULL, NULL, &tv) > 0;
}

static boost::mutex endpoint_mutex;

/***********************************************************************
 * Receive tunnel - forwards recv interface to send socket
 **********************************************************************/
static void e300_recv_tunnel(
    const std::string &name,
    uhd::transport::zero_copy_if::sptr recver,
    boost::shared_ptr<asio::ip::udp::socket> sender,
    asio::ip::udp::endpoint *endpoint,
    bool *running
)
{
    asio::ip::udp::endpoint _tx_endpoint;
    try
    {
        while (*running)
        {
            //step 1 - get the buffer
            managed_recv_buffer::sptr buff = recver->get_recv_buff();
            if (not buff) continue;
            if (E300_NETWORK_DEBUG) UHD_LOGGER_INFO("E300") << name << " got " << buff->size();

            //step 1.5 -- update endpoint
            {
                boost::mutex::scoped_lock l(endpoint_mutex);
                _tx_endpoint = *endpoint;
            }

            //step 2 - send to the socket
            sender->send_to(asio::buffer(buff->cast<const void *>(), buff->size()), _tx_endpoint);
        }
    }
    catch(const std::exception &ex)
    {
        UHD_LOGGER_ERROR("E300") << "e300_recv_tunnel exit " << name << " " << ex.what();
    }
    catch(...)
    {
        UHD_LOGGER_ERROR("E300") << "e300_recv_tunnel exit " << name ;
    }
    UHD_LOGGER_INFO("E300") << "e300_recv_tunnel exit " << name;
    *running = false;
}

/***********************************************************************
 * Send tunnel - forwards recv socket to send interface
 **********************************************************************/
static void e300_send_tunnel(
    const std::string &name,
    boost::shared_ptr<asio::ip::udp::socket> recver,
    uhd::transport::zero_copy_if::sptr sender,
    asio::ip::udp::endpoint *endpoint,
    bool *running
)
{
    asio::ip::udp::endpoint _rx_endpoint;
    try
    {
        while (*running)
        {
            //step 1 - get the buffer
            managed_send_buffer::sptr buff = sender->get_send_buff();
            if (not buff) continue;

            //step 2 - recv from socket
            while (not wait_for_recv_ready(recver->native_handle(), 100) and *running){}
            if (not *running) break;
            const size_t num_bytes = recver->receive_from(asio::buffer(buff->cast<void *>(), buff->size()), _rx_endpoint);
            if (E300_NETWORK_DEBUG) UHD_LOGGER_INFO("E300") << name << " got " << num_bytes;

            //step 2.5 -- update endpoint
            {
                boost::mutex::scoped_lock l(endpoint_mutex);
                *endpoint = _rx_endpoint;
            }

            //step 3 - commit the buffer
            buff->commit(num_bytes);
        }
    }
    catch(const std::exception &ex)
    {
        UHD_LOGGER_ERROR("E300") << "e300_send_tunnel exit " << name << " " << ex.what() ;
    }
    catch(...)
    {
        UHD_LOGGER_ERROR("E300") << "e300_send_tunnel exit " << name ;
    }
    UHD_LOGGER_INFO("E300") << "e300_send_tunnel exit " << name;
    *running = false;
}

static void e300_codec_ctrl_tunnel(
    const std::string &name,
    boost::shared_ptr<asio::ip::udp::socket> socket,
    ad9361_ctrl::sptr _codec_ctrl,
    asio::ip::udp::endpoint *endpoint,
    bool *running
)
{
    asio::ip::udp::endpoint _endpoint;
    try
    {
        while (*running)
        {
            uint8_t in_buff[64] = {};
            uint8_t out_buff[64] = {};

            const size_t num_bytes = socket->receive_from(asio::buffer(in_buff), *endpoint);

            typedef e300_remote_codec_ctrl::transaction_t codec_xact_t;

            if (num_bytes < sizeof(codec_xact_t)) {
                std::cout << "Received short packet of " << num_bytes  << std::endl;
                continue;
            }

            codec_xact_t *in = reinterpret_cast<codec_xact_t*>(in_buff);
            codec_xact_t *out = reinterpret_cast<codec_xact_t*>(out_buff);
            std::memcpy(out, in, sizeof(codec_xact_t));

            std::string which_str;
            switch (uhd::ntohx<uint32_t>(in->which)) {
            case codec_xact_t::CHAIN_TX1:
                which_str = "TX1"; break;
            case codec_xact_t::CHAIN_TX2:
                which_str = "TX2"; break;
            case codec_xact_t::CHAIN_RX1:
                which_str = "RX1"; break;
            case codec_xact_t::CHAIN_RX2:
                which_str = "RX2"; break;
            default:
                which_str = ""; break;
            }

            switch (uhd::ntohx<uint32_t>(in->action)) {
            case codec_xact_t::ACTION_SET_GAIN:
                out->gain = _codec_ctrl->set_gain(which_str, in->gain);
                break;
            case codec_xact_t::ACTION_SET_CLOCK_RATE:
                out->rate = _codec_ctrl->set_clock_rate(in->rate);
                break;
            case codec_xact_t::ACTION_SET_ACTIVE_CHANS:
                _codec_ctrl->set_active_chains(
                    uhd::ntohx<uint32_t>(in->bits) & (1<<0),
                    uhd::ntohx<uint32_t>(in->bits) & (1<<1),
                    uhd::ntohx<uint32_t>(in->bits) & (1<<2),
                    uhd::ntohx<uint32_t>(in->bits) & (1<<3));
                break;
            case codec_xact_t::ACTION_TUNE:
                out->freq = _codec_ctrl->tune(which_str, in->freq);
                break;
            case codec_xact_t::ACTION_GET_FREQ:
                    out->freq = _codec_ctrl->get_freq(which_str);
                break;
            case codec_xact_t::ACTION_SET_LOOPBACK:
                _codec_ctrl->data_port_loopback(
                    uhd::ntohx<uint32_t>(in->bits) & 1);
                break;
            case codec_xact_t::ACTION_GET_RSSI:
                out->rssi = _codec_ctrl->get_rssi(which_str).to_real();
                break;
            case codec_xact_t::ACTION_GET_TEMPERATURE:
                out->temp = _codec_ctrl->get_temperature().to_real();
                break;
            case codec_xact_t::ACTION_SET_DC_OFFSET_AUTO:
                _codec_ctrl->set_dc_offset_auto(which_str, in->use_dc_correction == 1);
                break;
            case codec_xact_t::ACTION_SET_IQ_BALANCE_AUTO:
                _codec_ctrl->set_iq_balance_auto(which_str, in->use_iq_correction == 1);
            case codec_xact_t::ACTION_SET_AGC:
                _codec_ctrl->set_agc(which_str, in->use_agc == 1);
                break;
            case codec_xact_t::ACTION_SET_AGC_MODE:
                if(in->agc_mode == 0) {
                    _codec_ctrl->set_agc_mode(which_str, "slow");
                } else if (in->agc_mode == 1) {
                    _codec_ctrl->set_agc_mode(which_str, "fast");
                }
                break;
            case codec_xact_t::ACTION_SET_BW:
                out->bw = _codec_ctrl->set_bw_filter(which_str, in->bw);
                break;
            default:
                UHD_LOGGER_INFO("E300") << "Got unknown request?!";
                //Zero out actions to fail this request on client
                out->action = uhd::htonx<uint32_t>(0);
            }

            socket->send_to(asio::buffer(out_buff, 64), *endpoint);
        }
    }
    catch(const std::exception &ex)
    {
        UHD_LOGGER_ERROR("E300") << "e300_ctrl_tunnel exit " << name << " " << ex.what() ;
    }
    catch(...)
    {
        UHD_LOGGER_ERROR("E300") << "e300_ctrl_tunnel exit " << name ;
    }
    UHD_LOGGER_INFO("E300") << "e300_ctrl_tunnel exit " << name;
    *running = false;
}

static void e300_global_regs_tunnel(
    const std::string &name,
    boost::shared_ptr<asio::ip::udp::socket> socket,
    global_regs::sptr regs,
    asio::ip::udp::endpoint *endpoint,
    bool *running
)
{
    UHD_ASSERT_THROW(regs);
    asio::ip::udp::endpoint _endpoint;
    try
    {
        while (*running)
        {
            uint8_t in_buff[16] = {};

            const size_t num_bytes = socket->receive_from(asio::buffer(in_buff), *endpoint);

            if (num_bytes < 16) {
                std::cout << "Received short packet: " << num_bytes << std::endl;
                continue;
            }

            global_regs_transaction_t *in =
                reinterpret_cast<global_regs_transaction_t *>(in_buff);

            if(uhd::ntohx<uint32_t>(in->is_poke)) {
                regs->poke32(uhd::ntohx<uint32_t>(in->addr), uhd::ntohx<uint32_t>(in->data));
            }
            else {
                in->data = uhd::htonx<uint32_t>(regs->peek32(uhd::ntohx<uint32_t>(in->addr)));
                socket->send_to(asio::buffer(in_buff, 16), *endpoint);
            }
        }
    }
    catch(const std::exception &ex)
    {
        UHD_LOGGER_ERROR("E300") << "e300_gregs_tunnel exit " << name << " " << ex.what() ;
    }
    catch(...)
    {
        UHD_LOGGER_ERROR("E300") << "e300_gregs_tunnel exit " << name ;
    }
    UHD_LOGGER_INFO("E300") << "e300_gregs_tunnel exit " << name;
    *running = false;
}

static void e300_sensor_tunnel(
    const std::string &name,
    boost::shared_ptr<asio::ip::udp::socket> socket,
    e300_sensor_manager::sptr sensor_manager,
    asio::ip::udp::endpoint *endpoint,
    bool *running
)
{
    asio::ip::udp::endpoint _endpoint;
    try
    {
        while (*running)
        {
            uint8_t in_buff[128] = {};

            const size_t num_bytes = socket->receive_from(asio::buffer(in_buff), *endpoint);

            if (num_bytes < sizeof(sensor_transaction_t)) {
                std::cout << "Received short packet: " << num_bytes << std::endl;
                continue;
            }

            uhd::usrp::e300::sensor_transaction_t *in =
                reinterpret_cast<uhd::usrp::e300::sensor_transaction_t *>(in_buff);

            if (uhd::ntohx(in->which) == ZYNQ_TEMP) {
                sensor_value_t temp = sensor_manager->get_mb_temp();
                // TODO: This is ugly ... use proper serialization
                in->value = uhd::htonx<uint32_t>(
                    e300_sensor_manager::pack_float_in_uint32_t(temp.to_real()));
            } else if (uhd::ntohx(in->which) == REF_LOCK) {
                in->value = uhd::htonx<uint32_t>(
                    sensor_manager->get_ref_lock().to_bool() ? 1 : 0);
            } else
                UHD_LOGGER_INFO("E300") << "Got unknown request?!";

            socket->send_to(asio::buffer(in_buff, sizeof(sensor_transaction_t)), *endpoint);
        }
    }
    catch(const std::exception &ex)
    {
        UHD_LOGGER_ERROR("E300") << "e300_sensor_tunnel exit " << name << " " << ex.what() ;
    }
    catch(...)
    {
        UHD_LOGGER_ERROR("E300") << "e300_sensor_tunnel exit " << name ;
    }
    UHD_LOGGER_INFO("E300") << "e300_sensor_tunnel exit " << name;
    *running = false;
}

static void e300_i2c_tunnel(
    const std::string &name,
    boost::shared_ptr<asio::ip::udp::socket> socket,
    uhd::usrp::e300::i2c::sptr i2c,
    asio::ip::udp::endpoint *endpoint,
    bool *running
)
{
    UHD_ASSERT_THROW(i2c);
    asio::ip::udp::endpoint _endpoint;
    try
    {
        while (*running)
        {
            uint8_t in_buff[sizeof(uhd::usrp::e300::i2c_transaction_t)];

            const size_t num_bytes = socket->receive_from(asio::buffer(in_buff), *endpoint);

            if (num_bytes < sizeof(uhd::usrp::e300::i2c_transaction_t)) {
                std::cout << "Received short packet: " << num_bytes << std::endl;
                continue;
            }

            uhd::usrp::e300::i2c_transaction_t *in =
                reinterpret_cast<uhd::usrp::e300::i2c_transaction_t *>(in_buff);

            // byte addressed accesses go through here
            if(in->type & i2c::ONEBYTE) {
                if(in->type & i2c::WRITE) {
                    i2c->set_i2c_reg8(
                        in->addr,
                        uhd::ntohx<uint16_t>(in->reg), in->data);
                } else {
                    in->data = i2c->get_i2c_reg8(in->addr, uhd::ntohx<uint16_t>(in->reg));
                    socket->send_to(asio::buffer(in_buff, sizeof(in_buff)), *endpoint);
                }

            // 2 byte addressed accesses go through here
            } else if (in->type & i2c::TWOBYTE) {
                if(in->type & i2c::WRITE) {
                    i2c->set_i2c_reg16(
                        in->addr,
                        uhd::ntohx<uint16_t>(in->reg), in->data);
                } else {
                    in->data = i2c->get_i2c_reg16(in->addr, uhd::ntohx<uint16_t>(in->reg));
                    socket->send_to(asio::buffer(in_buff, sizeof(in_buff)), *endpoint);
                }

            } else {
                UHD_LOGGER_ERROR("E300") << "e300_i2c_tunnel could not handle message." ;
            }
        }
    }
    catch(const std::exception &ex)
    {
        UHD_LOGGER_ERROR("E300") << "e300_i2c_tunnel exit " << name << " " << ex.what() ;
    }
    catch(...)
    {
        UHD_LOGGER_ERROR("E300") << "e300_i2c_tunnel exit " << name ;
    }
    UHD_LOGGER_INFO("E300") << "e300_i2c_tunnel exit " << name;
    *running = false;
}




class network_server_impl : public network_server
{
public:
    network_server_impl(const uhd::device_addr_t &device_addr);
    virtual ~network_server_impl(void);
    void run(void);

private:
    struct xports_t
    {
        uhd::transport::zero_copy_if::sptr send_ctrl_xport;
        uhd::transport::zero_copy_if::sptr recv_ctrl_xport;
        uhd::transport::zero_copy_if::sptr tx_data_xport;
        uhd::transport::zero_copy_if::sptr tx_flow_xport;
        uhd::transport::zero_copy_if::sptr rx_data_xport;
        uhd::transport::zero_copy_if::sptr rx_flow_xport;
    };

private:
    void _run_server(
        const std::string &port,
        const std::string &what,
        const size_t fe);

private:
    boost::shared_ptr<e300_fifo_interface>   _fifo_iface;
    xports_t                                 _xports[2];
    boost::shared_ptr<ad9361_ctrl>           _codec_ctrl;
    boost::shared_ptr<global_regs>           _global_regs;
    boost::shared_ptr<e300_sensor_manager>   _sensor_manager;
    boost::shared_ptr<e300_eeprom_manager>   _eeprom_manager;
};

network_server_impl::~network_server_impl(void)
{
}

/***********************************************************************
 * The UDP server itself
 **********************************************************************/
void network_server_impl::_run_server(
    const std::string &port,
    const std::string &what,
    const size_t fe)
{
    asio::io_service io_service;
    asio::ip::udp::resolver resolver(io_service);
    asio::ip::udp::resolver::query query(asio::ip::udp::v4(), "0.0.0.0", port);
    asio::ip::udp::endpoint endpoint = *resolver.resolve(query);

    //boost::shared_ptr<asio::ip::udp::acceptor> acceptor(new asio::ip::udp::acceptor(io_service, endpoint));
    while (not boost::this_thread::interruption_requested())
    {
        UHD_LOGGER_INFO("E300") << "e300 run server on port " << port << " for " << what;
        try
        {
            //while (not wait_for_recv_ready(acceptor->native(), 100))
            //{
            //    if (boost::this_thread::interruption_requested()) return;
            //}
            boost::shared_ptr<asio::ip::udp::socket> socket;
            socket.reset(new asio::ip::udp::socket(io_service, endpoint));
            //acceptor->accept(*socket);
            UHD_LOGGER_INFO("E300") << "e300 socket accept on port " << port << " for " << what;
            //asio::ip::udp::no_delay option(true);
            //socket->set_option(option);
            boost::thread_group tg;
            bool running = true;
            xports_t &perif = _xports[fe];
            if (what == "RX") {
                tg.create_thread(boost::bind(&e300_recv_tunnel, "RX data tunnel", perif.rx_data_xport, socket, &endpoint, &running));
                tg.create_thread(boost::bind(&e300_send_tunnel, "RX flow tunnel", socket, perif.rx_flow_xport, &endpoint, &running));
            }
            if (what == "TX") {
                tg.create_thread(boost::bind(&e300_recv_tunnel, "TX flow tunnel", perif.tx_flow_xport, socket, &endpoint, &running));
                tg.create_thread(boost::bind(&e300_send_tunnel, "TX data tunnel", socket, perif.tx_data_xport, &endpoint, &running));
            }
            if (what == "CTRL") {
                tg.create_thread(boost::bind(&e300_recv_tunnel, "response tunnel", perif.recv_ctrl_xport, socket, &endpoint, &running));
                tg.create_thread(boost::bind(&e300_send_tunnel, "control tunnel", socket, perif.send_ctrl_xport, &endpoint, &running));
            }
            if (what == "CODEC") {
                tg.create_thread(boost::bind(&e300_codec_ctrl_tunnel, "CODEC tunnel", socket, _codec_ctrl, &endpoint, &running));
            }
            if (what == "I2C") {
                tg.create_thread(boost::bind(&e300_i2c_tunnel, "I2C tunnel", socket, _eeprom_manager->get_i2c_sptr(), &endpoint, &running));
            }
            if (what == "GREGS") {
                tg.create_thread(boost::bind(&e300_global_regs_tunnel, "GREGS tunnel", socket, _global_regs, &endpoint, &running));
            }
            if (what == "SENSOR") {
                tg.create_thread(boost::bind(&e300_sensor_tunnel, "SENSOR tunnel", socket, _sensor_manager, &endpoint, &running));
            }

            tg.join_all();
            socket->close();
            socket.reset();
        }
        catch(...){}
    }
}

void network_server_impl::run()
{
    for(;;)
    {
        boost::thread_group tg;
        tg.create_thread(boost::bind(&network_server_impl::_run_server, this, E300_SERVER_RX_PORT0, "RX",0));
        tg.create_thread(boost::bind(&network_server_impl::_run_server, this, E300_SERVER_TX_PORT0, "TX",0));
        tg.create_thread(boost::bind(&network_server_impl::_run_server, this, E300_SERVER_CTRL_PORT0, "CTRL",0));

        tg.create_thread(boost::bind(&network_server_impl::_run_server, this, E300_SERVER_RX_PORT1, "RX",1));
        tg.create_thread(boost::bind(&network_server_impl::_run_server, this, E300_SERVER_TX_PORT1, "TX",1));
        tg.create_thread(boost::bind(&network_server_impl::_run_server, this, E300_SERVER_CTRL_PORT1, "CTRL",1));

        tg.create_thread(boost::bind(&network_server_impl::_run_server, this, E300_SERVER_SENSOR_PORT, "SENSOR", 0 /*don't care */));

        tg.create_thread(boost::bind(&network_server_impl::_run_server, this, E300_SERVER_CODEC_PORT, "CODEC", 0 /*don't care */));
        tg.create_thread(boost::bind(&network_server_impl::_run_server, this, E300_SERVER_GREGS_PORT, "GREGS", 0 /*don't care */));
        tg.create_thread(boost::bind(&network_server_impl::_run_server, this, E300_SERVER_I2C_PORT, "I2C", 0 /*don't care */));
        tg.join_all();
    }
}
network_server_impl::network_server_impl(const uhd::device_addr_t &device_addr)
{
    _eeprom_manager = boost::make_shared<e300_eeprom_manager>(i2c::make_i2cdev(E300_I2CDEV_DEVICE));
    if (not device_addr.has_key("no_reload_fpga")) {
        // Load FPGA image if provided via args
        if (device_addr.has_key("fpga")) {
            common::load_fpga_image(device_addr["fpga"]);
        // Else load the FPGA image based on the product ID
        } else {
            //extract the FPGA path for the e300
            const uint16_t pid = boost::lexical_cast<uint16_t>(
                _eeprom_manager->get_mb_eeprom()["product"]);
            std::string fpga_image;
            switch(e300_eeprom_manager::get_mb_type(pid)) {
            case e300_eeprom_manager::USRP_E310_SG1_MB:
                fpga_image = find_image_path(E310_SG1_FPGA_FILE_NAME);
                break;
            case e300_eeprom_manager::USRP_E310_SG3_MB:
                fpga_image = find_image_path(E310_SG3_FPGA_FILE_NAME);
                break;
            case e300_eeprom_manager::USRP_E300_MB:
                fpga_image = find_image_path(E300_FPGA_FILE_NAME);
                break;
            case e300_eeprom_manager::UNKNOWN:
            default:
                UHD_LOGGER_WARNING("E300") << "Unknown motherboard type, loading e300 image."
                                 ;
                fpga_image = find_image_path(E300_FPGA_FILE_NAME);
                break;
            }
            common::load_fpga_image(fpga_image);
        }
    }

    uhd::transport::zero_copy_xport_params ctrl_xport_params;
    ctrl_xport_params.recv_frame_size = e300::DEFAULT_CTRL_FRAME_SIZE;
    ctrl_xport_params.num_recv_frames = e300::DEFAULT_CTRL_NUM_FRAMES;
    ctrl_xport_params.send_frame_size = e300::DEFAULT_CTRL_FRAME_SIZE;
    ctrl_xport_params.num_send_frames = e300::DEFAULT_CTRL_NUM_FRAMES;

    uhd::transport::zero_copy_xport_params data_xport_params;
    data_xport_params.recv_frame_size = device_addr.cast<size_t>("recv_frame_size", e300::DEFAULT_RX_DATA_FRAME_SIZE);
    data_xport_params.num_recv_frames = device_addr.cast<size_t>("num_recv_frames", e300::DEFAULT_RX_DATA_NUM_FRAMES);
    data_xport_params.send_frame_size = device_addr.cast<size_t>("send_frame_size", e300::DEFAULT_TX_DATA_FRAME_SIZE);
    data_xport_params.num_send_frames = device_addr.cast<size_t>("num_send_frames", e300::DEFAULT_TX_DATA_NUM_FRAMES);
    // until we figure out why this goes wrong we'll keep this hack around
    data_xport_params.recv_frame_size =
        std::min(e300::MAX_NET_RX_DATA_FRAME_SIZE, data_xport_params.recv_frame_size);
    data_xport_params.send_frame_size =
        std::min(e300::MAX_NET_TX_DATA_FRAME_SIZE, data_xport_params.send_frame_size);


    e300_fifo_config_t fifo_cfg;
    try {
        fifo_cfg = e300_read_sysfs();
    } catch (uhd::lookup_error &e) {
        throw uhd::runtime_error("Failed to get driver parameters from sysfs.");
    }
    _fifo_iface = e300_fifo_interface::make(fifo_cfg);
    _global_regs = global_regs::make(_fifo_iface->get_global_regs_base());

    // static mapping, boooohhhhhh
    _xports[0].send_ctrl_xport = _fifo_iface->make_send_xport(E300_R0_CTRL_STREAM, ctrl_xport_params);
    _xports[0].recv_ctrl_xport = _fifo_iface->make_recv_xport(E300_R0_CTRL_STREAM, ctrl_xport_params);
    _xports[0].tx_data_xport   = _fifo_iface->make_send_xport(E300_R0_TX_DATA_STREAM, data_xport_params);
    _xports[0].tx_flow_xport   = _fifo_iface->make_recv_xport(E300_R0_TX_DATA_STREAM, ctrl_xport_params);
    _xports[0].rx_data_xport   = _fifo_iface->make_recv_xport(E300_R0_RX_DATA_STREAM, data_xport_params);
    _xports[0].rx_flow_xport   = _fifo_iface->make_send_xport(E300_R0_RX_DATA_STREAM, ctrl_xport_params);

    _xports[1].send_ctrl_xport = _fifo_iface->make_send_xport(E300_R1_CTRL_STREAM, ctrl_xport_params);
    _xports[1].recv_ctrl_xport = _fifo_iface->make_recv_xport(E300_R1_CTRL_STREAM, ctrl_xport_params);
    _xports[1].tx_data_xport   = _fifo_iface->make_send_xport(E300_R1_TX_DATA_STREAM, data_xport_params);
    _xports[1].tx_flow_xport   = _fifo_iface->make_recv_xport(E300_R1_TX_DATA_STREAM, ctrl_xport_params);
    _xports[1].rx_data_xport   = _fifo_iface->make_recv_xport(E300_R1_RX_DATA_STREAM, data_xport_params);
    _xports[1].rx_flow_xport   = _fifo_iface->make_send_xport(E300_R1_RX_DATA_STREAM, ctrl_xport_params);

    ad9361_params::sptr client_settings = boost::make_shared<e300_ad9361_client_t>();
    _codec_ctrl = ad9361_ctrl::make_spi(client_settings, spi::make(E300_SPIDEV_DEVICE), 1);
    // This is horrible ... why do I have to sleep here?
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    _sensor_manager = e300_sensor_manager::make_local(_global_regs);
}

}}} // namespace

using namespace uhd::usrp::e300;

network_server::sptr network_server::make(const uhd::device_addr_t &device_addr)
{
    return sptr(new network_server_impl(device_addr));
}

#else

using namespace uhd::usrp::e300;

network_server::sptr network_server::make(const uhd::device_addr_t &)
{
    throw uhd::assertion_error("network_server::make() !E300_NATIVE");
}
#endif

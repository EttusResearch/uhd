//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "n230_resource_manager.hpp"

#include "n230_fw_ctrl_iface.hpp"
#include <uhd/transport/if_addrs.hpp>
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/platform.hpp>
#include <uhd/utils/paths.hpp>
#include <boost/format.hpp>
#include <boost/functional/hash.hpp>
#include <boost/make_shared.hpp>
#include "n230_fw_defs.h"
#include "n230_fw_host_iface.h"
#include <chrono>
#include <thread>

#define IF_DATA_I_MASK  0xFFF00000
#define IF_DATA_Q_MASK  0x0000FFF0

namespace uhd { namespace usrp { namespace n230 {

//Constants
static const uint8_t N230_HOST_SRC_ADDR_ETH0 = 0;
static const uint8_t N230_HOST_SRC_ADDR_ETH1 = 1;
static const uint8_t N230_HOST_DEST_ADDR     = 2;

static const uint8_t N230_ETH0_IFACE_ID  = 0;
static const uint8_t N230_ETH1_IFACE_ID  = 1;

class n230_ad9361_client_t : public ad9361_params {
public:
    ~n230_ad9361_client_t() {}
    double get_band_edge(frequency_band_t band) {
        switch (band) {
            case AD9361_RX_BAND0:   return 2.2e9;
            case AD9361_RX_BAND1:   return 4.0e9;
            case AD9361_TX_BAND0:   return 2.5e9;
            default:                return 0;
        }
    }
    clocking_mode_t get_clocking_mode() {
        return clocking_mode_t::AD9361_XTAL_N_CLK_PATH;
    }
    digital_interface_mode_t get_digital_interface_mode() {
        return AD9361_DDR_FDD_LVDS;
    }
    digital_interface_delays_t get_digital_interface_timing() {
        digital_interface_delays_t delays;
        delays.rx_clk_delay = 0;
        delays.rx_data_delay = 0;
        delays.tx_clk_delay = 0;
        delays.tx_data_delay = 2;
        return delays;
    }
};

n230_resource_manager::n230_resource_manager(
    const std::vector<std::string> ip_addrs,
    const bool safe_mode
) :
    _safe_mode(safe_mode),
    _last_host_enpoint(0)
{
    if (_safe_mode) UHD_LOGGER_WARNING("N230") << "Initializing device in safe mode\n";
    UHD_LOGGER_INFO("N230") << "Setup basic communication...";

    //Discover ethernet interfaces
    bool dual_eth_expected = (ip_addrs.size() > 1);
    for(const std::string& addr:  ip_addrs) {
        n230_eth_conn_t conn_iface;
        conn_iface.ip_addr = addr;

        uint32_t iface_id = 0xFFFFFFFF;
        try {
            iface_id = n230::n230_fw_ctrl_iface::get_iface_id(
                conn_iface.ip_addr, BOOST_STRINGIZE(N230_FW_COMMS_UDP_PORT), N230_FW_PRODUCT_ID);
        } catch (uhd::io_error&) {
            throw uhd::io_error(str(boost::format(
                "Could not communicate with the device over address %s") %
                conn_iface.ip_addr));
        }
        switch (iface_id) {
            case N230_ETH0_IFACE_ID: conn_iface.type = ETH0; break;
            case N230_ETH1_IFACE_ID: conn_iface.type = ETH1; break;
            default: {
                if (dual_eth_expected) {
                    throw uhd::runtime_error("N230 Initialization Error: Could not detect ethernet port number.");
                } else {
                    //For backwards compatibility, if only one port is specified, assume that a detection
                    //failure means that the device does not support dual-ethernet behavior.
                    conn_iface.type = ETH0; break;
                }
            }
        }
        _eth_conns.push_back(conn_iface);
    }
    if (_eth_conns.size() < 1) {
        throw uhd::runtime_error("N230 Initialization Error: No eth interfaces specified.)");
    }

    //Create firmware communication interface
    _fw_ctrl = n230::n230_fw_ctrl_iface::make(
        transport::udp_simple::make_connected(
            _get_conn(PRI_ETH).ip_addr, BOOST_STRINGIZE(N230_FW_COMMS_UDP_PORT)), N230_FW_PRODUCT_ID);
    if (_fw_ctrl.get() == NULL) {
        throw uhd::runtime_error("N230 Initialization Error: Could not create n230_ctrl_iface.)");
    }
    _check_fw_compat();

    //Start the device claimer
    _claimer_task = uhd::task::make(boost::bind(&n230_resource_manager::_claimer_loop, this));

    //Create common settings interface
    const sid_t core_sid = _generate_sid(CORE, _get_conn(PRI_ETH).type);

    transport::udp_zero_copy::buff_params dummy_out_params;
    transport::zero_copy_if::sptr core_xport =
        _create_transport(_get_conn(PRI_ETH), core_sid, device_addr_t(), dummy_out_params);
    if (core_xport.get() == NULL) {
        throw uhd::runtime_error("N230 Initialization Error: Could not create settings transport.)");
    }
    _core_ctrl = radio_ctrl_core_3000::make(
        fpga::CVITA_BIG_ENDIAN, core_xport, core_xport, core_sid.get());
    if (_core_ctrl.get() == NULL) {
        throw uhd::runtime_error("N230 Initialization Error: Could not create settings ctrl.)");
    }
    _check_fpga_compat();

    UHD_LOGGER_INFO("N230") << boost::format("Version signatures... Firmware:%s FPGA:%s...")
        % _fw_version.get_hash_str() % _fpga_version.get_hash_str();

    _core_radio_ctrl_reg.initialize(*_core_ctrl, true /*flush*/);
    _core_misc_reg.initialize(*_core_ctrl, true /*flush*/);
    _core_pps_sel_reg.initialize(*_core_ctrl, true /*flush*/);
    _core_status_reg.initialize(*_core_ctrl);

    //Create common SPI interface
    _core_spi_ctrl = n230_core_spi_core::make(_core_ctrl);
    if (_core_spi_ctrl.get() == NULL) {
        throw uhd::runtime_error("N230 Initialization Error: Could not create SPI ctrl.)");
    }

    //Create AD9361 interface
    UHD_LOGGER_INFO("N230") << "Initializing CODEC...";
    _codec_ctrl = ad9361_ctrl::make_spi(
        boost::make_shared<n230_ad9361_client_t>(), _core_spi_ctrl, fpga::AD9361_SPI_SLAVE_NUM);
    if (_codec_ctrl.get() == NULL) {
        throw uhd::runtime_error("N230 Initialization Error: Could not create Catalina ctrl.)");
    }
    _codec_ctrl->set_clock_rate(fpga::CODEC_DEFAULT_CLK_RATE);
    _codec_mgr = ad936x_manager::make(_codec_ctrl, fpga::NUM_RADIOS);
    _codec_mgr->init_codec();

    //Create AD4001 interface
    _ref_pll_ctrl = boost::make_shared<n230_ref_pll_ctrl>(_core_spi_ctrl);
    if (_ref_pll_ctrl.get() == NULL) {
        throw uhd::runtime_error("N230 Initialization Error: Could not create ADF4001 ctrl.)");
    }

    //Reset SERDES interface and synchronize to frame sync from AD9361
    _reset_codec_digital_interface();

    std::vector<time_core_3000::sptr> time_cores;
    std::vector<gpio_atr::gpio_atr_3000::sptr> gpio_cores;
    for (size_t i = 0; i < fpga::NUM_RADIOS; i++) {
        _initialize_radio(i);
        time_cores.push_back(_radios[i].time);
        gpio_cores.push_back(_radios[i].gpio_atr);
    }

    //Create clock and PPS control interface
    _clk_pps_ctrl = n230_clk_pps_ctrl::make(
        _codec_ctrl, _ref_pll_ctrl, _core_misc_reg, _core_pps_sel_reg, _core_status_reg, time_cores);
    if (_clk_pps_ctrl.get() == NULL) {
        throw uhd::runtime_error("N230 Initialization Error: Could not create clock and PPS ctrl.)");
    }

    //Create front-end control interface
    _frontend_ctrl = n230_frontend_ctrl::make(_core_ctrl, _core_misc_reg, _codec_ctrl, gpio_cores);
    if (_frontend_ctrl.get() == NULL) {
        throw uhd::runtime_error("N230 Initialization Error: Could not create front-end ctrl.)");
    }

    //Create miniSAS GPIO interfaces
    _ms0_gpio = gpio_atr::gpio_atr_3000::make(
        _core_ctrl, fpga::sr_addr(fpga::SR_CORE_MS0_GPIO), fpga::rb_addr(fpga::RB_CORE_MS0_GPIO));
    _ms0_gpio->set_atr_mode(gpio_atr::MODE_GPIO,gpio_atr::gpio_atr_3000::MASK_SET_ALL);
    _ms1_gpio = gpio_atr::gpio_atr_3000::make(
        _core_ctrl, fpga::sr_addr(fpga::SR_CORE_MS1_GPIO), fpga::rb_addr(fpga::RB_CORE_MS1_GPIO));
    _ms1_gpio->set_atr_mode(gpio_atr::MODE_GPIO,gpio_atr::gpio_atr_3000::MASK_SET_ALL);

    //Create GPSDO interface
    if (_core_status_reg.read(fpga::core_status_reg_t::GPSDO_STATUS) != fpga::GPSDO_ST_ABSENT) {
        UHD_LOGGER_INFO("N230") << "Detecting GPSDO.... ";
        try {
            const sid_t gps_uart_sid = _generate_sid(GPS_UART, _get_conn(PRI_ETH).type);
            transport::zero_copy_if::sptr gps_uart_xport =
                _create_transport(_get_conn(PRI_ETH), gps_uart_sid, device_addr_t(), dummy_out_params);
            _gps_uart = n230_uart::make(gps_uart_xport, uhd::htonx(gps_uart_sid.get()));
            _gps_uart->set_baud_divider(fpga::BUS_CLK_RATE/fpga::GPSDO_UART_BAUDRATE);
            _gps_uart->write_uart("\n"); //cause the baud and response to be setup
            std::this_thread::sleep_for(std::chrono::seconds(1)); //allow for a little propagation
            _gps_ctrl = gps_ctrl::make(_gps_uart);
        } catch(std::exception &e) {
            UHD_LOGGER_ERROR("N230") << "An error occurred making GPSDO control: " << e.what() ;
        }
        if (not is_gpsdo_present()) {
            _core_ctrl->poke32(fpga::sr_addr(fpga::SR_CORE_GPSDO_ST), fpga::GPSDO_ST_ABSENT);
        }
    }

    //Perform data self-tests
    _frontend_ctrl->set_stream_state(TXRX_STREAMING, TXRX_STREAMING);
    for (size_t i = 0; i < fpga::NUM_RADIOS; i++) {
        _frontend_ctrl->set_self_test_mode(LOOPBACK_RADIO);
        bool radio_selftest_pass = _radio_data_loopback_self_test(_radios[i].ctrl);
        if (!radio_selftest_pass) {
            throw uhd::runtime_error("N230 Initialization Error: Data loopback test failed.)");
        }

        _frontend_ctrl->set_self_test_mode(LOOPBACK_CODEC);
        bool codec_selftest_pass = _radio_data_loopback_self_test(_radios[i].ctrl);
        if (!codec_selftest_pass) {
            throw uhd::runtime_error("N230 Initialization Error: Codec loopback test failed.)");
        }
    }
    _frontend_ctrl->set_self_test_mode(LOOPBACK_DISABLED);
    _frontend_ctrl->set_stream_state(NONE_STREAMING, NONE_STREAMING);
}

n230_resource_manager::~n230_resource_manager()
{
    _claimer_task.reset();
    {   //Critical section
        boost::mutex::scoped_lock(_claimer_mutex);
        _fw_ctrl->poke32(N230_FW_HOST_SHMEM_OFFSET(claim_time), 0);
        _fw_ctrl->poke32(N230_FW_HOST_SHMEM_OFFSET(claim_src), 0);
    }
}

transport::zero_copy_if::sptr n230_resource_manager::create_transport(
    n230_data_dir_t direction,
    size_t radio_instance,
    const device_addr_t &params,
    sid_t& sid_pair,
    transport::udp_zero_copy::buff_params& buff_out_params)
{
    const n230_eth_conn_t& conn = _get_conn((radio_instance==1)?SEC_ETH:PRI_ETH);
    const sid_t temp_sid_pair =
        _generate_sid(direction==RX_DATA?RADIO_RX_DATA:RADIO_TX_DATA, conn.type, radio_instance);
    transport::zero_copy_if::sptr xport = _create_transport(conn, temp_sid_pair, params, buff_out_params);
    if (xport.get() == NULL) {
        throw uhd::runtime_error("N230 Create Data Transport: Could not create data transport.)");
    } else {
        sid_pair = temp_sid_pair;
    }
    return xport;
}

bool n230_resource_manager::is_device_claimed(n230_fw_ctrl_iface::sptr fw_ctrl)
{
    boost::mutex::scoped_lock(_claimer_mutex);

    //If timed out then device is definitely unclaimed
    if (fw_ctrl->peek32(N230_FW_HOST_SHMEM_OFFSET(claim_status)) == 0)
        return false;

    //otherwise check claim src to determine if another thread with the same src has claimed the device
    return fw_ctrl->peek32(N230_FW_HOST_SHMEM_OFFSET(claim_src)) != get_process_hash();
}

void n230_resource_manager::_claimer_loop()
{
    {   //Critical section
        boost::mutex::scoped_lock(_claimer_mutex);
        _fw_ctrl->poke32(N230_FW_HOST_SHMEM_OFFSET(claim_time), time(NULL));
        _fw_ctrl->poke32(N230_FW_HOST_SHMEM_OFFSET(claim_src), get_process_hash());
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(N230_CLAIMER_TIMEOUT_IN_MS / 2));
}

void n230_resource_manager::_initialize_radio(size_t instance)
{
    radio_resource_t& radio = _radios[instance];

    //Create common settings interface
    const sid_t ctrl_sid = _generate_sid(RADIO_CONTROL, _get_conn(PRI_ETH).type, instance);
    transport::udp_zero_copy::buff_params buff_out_params;
    transport::zero_copy_if::sptr ctrl_xport =
        _create_transport(_get_conn(PRI_ETH), ctrl_sid, device_addr_t(), buff_out_params);
    if (ctrl_xport.get() == NULL) {
        throw uhd::runtime_error("N230 Initialization Error: Could not create radio transport.)");
    }
    radio.ctrl = radio_ctrl_core_3000::make(
        fpga::CVITA_BIG_ENDIAN, ctrl_xport, ctrl_xport, ctrl_sid.get());
    if (radio.ctrl.get() == NULL) {
        throw uhd::runtime_error("N230 Initialization Error: Could not create radio ctrl.)");
    }

    //Perform register loopback test to verify the radio clock
    bool reg_selftest_pass = _radio_register_loopback_self_test(radio.ctrl);
    if (!reg_selftest_pass) {
        throw uhd::runtime_error("N230 Initialization Error: Register loopback test failed.)");
    }

    //Write-only ATR interface
    radio.gpio_atr = gpio_atr::gpio_atr_3000::make_write_only(radio.ctrl, fpga::sr_addr(fpga::SR_RADIO_ATR));
    radio.gpio_atr->set_atr_mode(gpio_atr::MODE_ATR,gpio_atr::gpio_atr_3000::MASK_SET_ALL);

    //Core VITA time interface
    time_core_3000::readback_bases_type time_bases;
    time_bases.rb_now = fpga::rb_addr(fpga::RB_RADIO_TIME_NOW);
    time_bases.rb_pps = fpga::rb_addr(fpga::RB_RADIO_TIME_PPS);
    radio.time = time_core_3000::make(radio.ctrl, fpga::sr_addr(fpga::SR_RADIO_TIME), time_bases);
    if (radio.time.get() == NULL) {
        throw uhd::runtime_error("N230 Initialization Error: Could not create time core.)");
    }

    //RX DSP
    radio.framer = rx_vita_core_3000::make(
        radio.ctrl, fpga::sr_addr(fpga::SR_RADIO_RX_CTRL));
    radio.ddc = rx_dsp_core_3000::make(radio.ctrl, fpga::sr_addr(fpga::SR_RADIO_RX_DSP), true /*old DDC?*/);
    if (radio.framer.get() == NULL || radio.ddc.get() == NULL) {
        throw uhd::runtime_error("N230 Initialization Error: Could not create RX DSP interface.)");
    }
    radio.ddc->set_link_rate(fpga::N230_LINK_RATE_BPS);

    //TX DSP
    radio.deframer = tx_vita_core_3000::make(radio.ctrl, fpga::sr_addr(fpga::SR_RADIO_TX_CTRL));
    radio.duc = tx_dsp_core_3000::make(radio.ctrl, fpga::sr_addr(fpga::SR_RADIO_TX_DSP));
    if (radio.deframer.get() == NULL || radio.duc.get() == NULL) {
        throw uhd::runtime_error("N230 Initialization Error: Could not create RX DSP interface.)");
    }
    radio.duc->set_link_rate(fpga::N230_LINK_RATE_BPS);

    //User settings
    radio.user_settings = user_settings_core_3000::make(radio.ctrl,
        fpga::sr_addr(fpga::SR_RADIO_USER_SR), fpga::rb_addr(fpga::SR_RADIO_USER_RB));
    if (radio.user_settings.get() == NULL) {
        throw uhd::runtime_error("N230 Initialization Error: Could not create user settings bus.)");
    }
}

uint8_t xb_ep_to_sid(fpga::xb_endpoint_t ep) {
    return static_cast<uint8_t>(ep) << 4;
}

const sid_t n230_resource_manager::_generate_sid(const n230_endpoint_t type, const n230_eth_port_t xport, size_t instance)
{
    fpga::xb_endpoint_t xb_dest_ep;
    uint8_t sid_dest_ep = 0;
    fpga::xb_endpoint_t xb_ret_ep = (xport == ETH1) ? fpga::N230_XB_DST_E1 : fpga::N230_XB_DST_E0;
    uint8_t sid_ret_addr = (xport == ETH1) ? N230_HOST_SRC_ADDR_ETH1 : N230_HOST_SRC_ADDR_ETH0;

    if (type == CORE or type == GPS_UART) {
        //Non-radio endpoints
        xb_dest_ep = (type == CORE) ? fpga::N230_XB_DST_GCTRL : fpga::N230_XB_DST_UART;
        sid_dest_ep = xb_ep_to_sid(xb_dest_ep);
    } else {
        //Radio endpoints
        xb_dest_ep = (instance == 1) ? fpga::N230_XB_DST_R1 : fpga::N230_XB_DST_R0;
        sid_dest_ep = xb_ep_to_sid(xb_dest_ep);
        switch (type) {
        case RADIO_TX_DATA:
            sid_dest_ep |= fpga::RADIO_DATA_SUFFIX;
            break;
        case RADIO_RX_DATA:
            sid_dest_ep |= fpga::RADIO_FC_SUFFIX;
            break;
        default:
            sid_dest_ep |= fpga::RADIO_CTRL_SUFFIX;
            break;
        }
    }

    //Increment last host logical endpoint
    sid_t sid(sid_ret_addr, ++_last_host_enpoint, N230_HOST_DEST_ADDR, sid_dest_ep);

    //Program the crossbar addr
    _fw_ctrl->poke32(fw::reg_addr(fw::WB_SBRB_BASE, fw::SR_ZPU_XB_LOCAL), sid.get_dst_addr());
    // Program CAM entry for returning packets to us
    // This type of packet does not match the XB_LOCAL address and is looked up in the lower half of the CAM
    _fw_ctrl->poke32(fw::reg_addr(fw::WB_XB_SBRB_BASE, sid.get_src_addr()), static_cast<uint32_t>(xb_ret_ep));
    // Program CAM entry for outgoing packets matching a N230 resource (for example a Radio)
    // This type of packet does matches the XB_LOCAL address and is looked up in the upper half of the CAM
    _fw_ctrl->poke32(fw::reg_addr(fw::WB_XB_SBRB_BASE, 256 + sid.get_dst_endpoint()), static_cast<uint32_t>(xb_dest_ep));

    return sid;
}

transport::zero_copy_if::sptr n230_resource_manager::_create_transport(
    const n230_eth_conn_t& eth_conn,
    const sid_t& sid, const device_addr_t &buff_params,
    transport::udp_zero_copy::buff_params& buff_params_out)
{
    transport::zero_copy_xport_params default_buff_args;
    default_buff_args.recv_frame_size = transport::udp_simple::mtu;
    default_buff_args.send_frame_size = transport::udp_simple::mtu;
    default_buff_args.num_recv_frames = 32;
    default_buff_args.num_send_frames = 32;

    transport::zero_copy_if::sptr xport = transport::udp_zero_copy::make(
        eth_conn.ip_addr, std::to_string(fpga::CVITA_UDP_PORT),
        default_buff_args, buff_params_out, buff_params);

    if (xport.get()) {
        _program_dispatcher(*xport, eth_conn.type, sid);
    }
    return xport;
}

void n230_resource_manager::_program_dispatcher(
    transport::zero_copy_if& xport, const n230_eth_port_t port, const sid_t& sid)
{
    //Send a mini packet with SID into the ZPU
    //ZPU will reprogram the ethernet framer
    transport::managed_send_buffer::sptr buff = xport.get_send_buff();
    buff->cast<uint32_t *>()[0] = 0; //eth dispatch looks for != 0
    buff->cast<uint32_t *>()[1] = uhd::htonx(sid.get());
    buff->commit(8);
    buff.reset();

    //reprogram the ethernet dispatcher's udp port (should be safe to always set)
    uint32_t disp_base_offset =
        ((port == ETH1) ? fw::SR_ZPU_ETHINT1 : fw::SR_ZPU_ETHINT0) + fw::SR_ZPU_ETHINT_DISPATCHER_BASE;
    _fw_ctrl->poke32(fw::reg_addr(fw::WB_SBRB_BASE, disp_base_offset + fw::ETH_FRAMER_SRC_UDP_PORT), fpga::CVITA_UDP_PORT);

    //Do a peek to an arbitrary address to guarantee that the
    //ethernet framer has been programmed before we return.
    _fw_ctrl->peek32(0);
}

void n230_resource_manager::_reset_codec_digital_interface()
{
    //Set timing registers
    _core_ctrl->poke32(fpga::sr_addr(fpga::SR_CORE_DATA_DELAY), fpga::CODEC_DATA_DELAY);
    _core_ctrl->poke32(fpga::sr_addr(fpga::SR_CORE_CLK_DELAY), fpga::CODEC_CLK_DELAY);

    _core_radio_ctrl_reg.write(fpga::core_radio_ctrl_reg_t::CODEC_ARST, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    _core_radio_ctrl_reg.write(fpga::core_radio_ctrl_reg_t::CODEC_ARST, 0);
}

bool n230_resource_manager::_radio_register_loopback_self_test(wb_iface::sptr iface)
{
    bool test_fail = false;
    size_t hash = static_cast<size_t>(time(NULL));
    for (size_t i = 0; i < 100; i++) {
        boost::hash_combine(hash, i);
        iface->poke32(fpga::sr_addr(fpga::SR_RADIO_TEST), uint32_t(hash));
        test_fail = iface->peek32(fpga::rb_addr(fpga::RB_RADIO_TEST)) != uint32_t(hash);
        if (test_fail) break; //exit loop on any failure
    }
    return !test_fail;
}

bool n230_resource_manager::_radio_data_loopback_self_test(wb_iface::sptr iface)
{
   bool test_fail = false;
    size_t hash = size_t(time(NULL));
    for (size_t i = 0; i < 100; i++) {
        boost::hash_combine(hash, i);
        const uint32_t word32 = uint32_t(hash) & (IF_DATA_I_MASK | IF_DATA_Q_MASK);
        iface->poke32(fpga::sr_addr(fpga::SR_RADIO_CODEC_IDLE), word32);
        iface->peek64(fpga::rb_addr(fpga::RB_RADIO_CODEC_DATA)); //block until request completes
        std::this_thread::sleep_for(std::chrono::microseconds(100)); //wait for loopback to propagate through codec
        const uint64_t rb_word64 = iface->peek64(fpga::rb_addr(fpga::RB_RADIO_CODEC_DATA));
        const uint32_t rb_tx = uint32_t(rb_word64 >> 32);
        const uint32_t rb_rx = uint32_t(rb_word64 & 0xffffffff);
        test_fail = word32 != rb_tx or word32 != rb_rx;
        if (test_fail){
            UHD_LOG_ERROR("N230", str(boost::format("mismatch (exp:%x, got:%x and %x)... ") % word32 % rb_tx % rb_rx));
            break; //exit loop on any failure
        }
    }

    /* Zero out the idle data. */
    iface->poke32(fpga::sr_addr(fpga::SR_RADIO_CODEC_IDLE), 0);
    return !test_fail;
}

std::string n230_resource_manager::_get_fpga_upgrade_msg() {
    std::string img_loader_path =
        (fs::path(uhd::get_pkg_path()) / "bin" / "uhd_image_loader").string();

    return str(boost::format(
            "\nDownload the appropriate FPGA images for this version of UHD.\n"
            "%s\n\n"
            "Then burn a new image to the on-board flash storage of your\n"
            "USRP N230 device using the image loader utility. Use this command:\n"
            "\n \"%s\" --args=\"type=n230,addr=%s\"\n")
        % print_utility_error("uhd_images_downloader.py")
        % img_loader_path % _get_conn(PRI_ETH).ip_addr);

}

void n230_resource_manager::_check_fw_compat()
{
    uint32_t compat_num = _fw_ctrl->peek32(N230_FW_HOST_SHMEM_OFFSET(fw_compat_num));
    _fw_version.compat_major = compat_num >> 16;
    _fw_version.compat_minor = compat_num;
    _fw_version.version_hash = _fw_ctrl->peek32(N230_FW_HOST_SHMEM_OFFSET(fw_version_hash));

    if (_fw_version.compat_major != N230_FW_COMPAT_NUM_MAJOR){
        throw uhd::runtime_error(str(boost::format(
            "Expected firmware compatibility number %d.x, but got %d.%d\n"
            "The firmware build is not compatible with the host code build.\n"
            "%s"
            ) % static_cast<uint32_t>(N230_FW_COMPAT_NUM_MAJOR)
              % static_cast<uint32_t>(_fw_version.compat_major)
              % static_cast<uint32_t>(_fw_version.compat_minor)
              % _get_fpga_upgrade_msg()));
    }
}

void n230_resource_manager::_check_fpga_compat()
{
    const uint64_t compat = _core_ctrl->peek64(fpga::rb_addr(fpga::RB_CORE_SIGNATUE));
    const uint32_t signature = uint32_t(compat >> 32);
    const uint16_t product_id = uint8_t(compat >> 24);
    _fpga_version.compat_major = static_cast<uint8_t>(compat >> 16);
    _fpga_version.compat_minor = static_cast<uint16_t>(compat);

    const uint64_t version_hash = _core_ctrl->peek64(fpga::rb_addr(fpga::RB_CORE_VERSION_HASH));
    _fpga_version.version_hash = uint32_t(version_hash);

    if (signature != 0x0ACE0BA5E || product_id != fpga::RB_N230_PRODUCT_ID)
        throw uhd::runtime_error("Signature check failed. Please contact support.");

    bool is_safe_image = (_fpga_version.compat_major > fpga::RB_N230_COMPAT_SAFE);

    if (is_safe_image && !_safe_mode) {
        throw uhd::runtime_error(
            "The device appears to have the failsafe FPGA image loaded\n"
            "This could have happened because the production FPGA image in the flash was either corrupt or non-existent\n"
            "To remedy this error, please burn a valid FPGA image to the flash.\n"
            "To continue using the failsafe image with UHD, create the UHD device with the \"safe_mode\" device arg.\n"
            "Radio functionality/performance not guaranteed when operating in safe mode.\n");
    } else if (_fpga_version.compat_major != fpga::RB_N230_COMPAT_MAJOR && !is_safe_image) {
        throw uhd::runtime_error(str(boost::format(
            "Expected FPGA compatibility number %d.x, but got %d.%d:\n"
            "The FPGA build is not compatible with the host code build.\n"
            "%s"
            ) % static_cast<uint32_t>(fpga::RB_N230_COMPAT_MAJOR)
              % static_cast<uint32_t>(_fpga_version.compat_major)
              % static_cast<uint32_t>(_fpga_version.compat_minor)
              % _get_fpga_upgrade_msg()));
    }
}

}}} //namespace

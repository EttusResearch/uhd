//
// Copyright 2012-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "b200_impl.hpp"
#include "b200_regs.hpp"
#include <uhd/config.hpp>
#include <uhd/transport/usb_control.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/cast.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/functional/hash.hpp>
#include <boost/make_shared.hpp>
#include <boost/weak_ptr.hpp>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <chrono>

#include "../../transport/libusb1_base.hpp"

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::usrp::gpio_atr;
using namespace uhd::transport;

namespace {
    constexpr int64_t  REENUMERATION_TIMEOUT_MS = 3000;
}

// B200 + B210:
class b200_ad9361_client_t : public ad9361_params {
public:
    ~b200_ad9361_client_t() {}
    double get_band_edge(frequency_band_t band) {
        switch (band) {
        case AD9361_RX_BAND0:   return 2.2e9; // Port C
        case AD9361_RX_BAND1:   return 4.0e9; // Port B
        case AD9361_TX_BAND0:   return 2.5e9; // Port B
        default:                return 0;
        }
    }
    clocking_mode_t get_clocking_mode() {
        return clocking_mode_t::AD9361_XTAL_N_CLK_PATH;
    }
    digital_interface_mode_t get_digital_interface_mode() {
        return AD9361_DDR_FDD_LVCMOS;
    }
    digital_interface_delays_t get_digital_interface_timing() {
        digital_interface_delays_t delays;
        delays.rx_clk_delay = 0;
        delays.rx_data_delay = 0xF;
        delays.tx_clk_delay = 0;
        delays.tx_data_delay = 0xF;
        return delays;
    }
};

// B205
class b2xxmini_ad9361_client_t : public ad9361_params {
public:
    ~b2xxmini_ad9361_client_t() {}
    double get_band_edge(frequency_band_t band) {
        switch (band) {
        case AD9361_RX_BAND0:   return 0; // Set these all to
        case AD9361_RX_BAND1:   return 0; // zero, so RF port A
        case AD9361_TX_BAND0:   return 0; // is used all the time
        default:                return 0; // On both Rx and Tx
        }
    }
    clocking_mode_t get_clocking_mode() {
        return clocking_mode_t::AD9361_XTAL_N_CLK_PATH;
    }
    digital_interface_mode_t get_digital_interface_mode() {
        return AD9361_DDR_FDD_LVCMOS;
    }
    digital_interface_delays_t get_digital_interface_timing() {
        digital_interface_delays_t delays;
        delays.rx_clk_delay = 0;
        delays.rx_data_delay = 0xF;
        delays.tx_clk_delay = 0;
        delays.tx_data_delay = 0xF;
        return delays;
    }
};

/***********************************************************************
 * Helpers
 **********************************************************************/
std::string check_option_valid(
        const std::string &name,
        const std::vector<std::string> &valid_options,
        const std::string &option
) {
    if (std::find(valid_options.begin(), valid_options.end(), option) == valid_options.end()) {
        throw uhd::runtime_error(str(
                boost::format("Invalid option chosen for: %s")
                % name
        ));
    }

    return option;
}

/***********************************************************************
 * Discovery
 **********************************************************************/
//! Look up the type of B-Series device we're currently running.
//  Throws a uhd::runtime_error if the USB PID and the product ID stored
//  in the MB EEPROM are invalid,
b200_product_t get_b200_product(const usb_device_handle::sptr& handle, const mboard_eeprom_t &mb_eeprom)
{
    // Try USB PID first
    uint16_t product_id = handle->get_product_id();
    if (B2XX_PID_TO_PRODUCT.has_key(product_id))
        return B2XX_PID_TO_PRODUCT[product_id];

    // Try EEPROM product ID code second
    if (mb_eeprom["product"].empty()) {
        throw uhd::runtime_error("B200: Missing product ID on EEPROM.");
    }
    product_id = boost::lexical_cast<uint16_t>(mb_eeprom["product"]);
    if (not B2XX_PRODUCT_ID.has_key(product_id)) {
        throw uhd::runtime_error(str(
            boost::format("B200 unknown product code: 0x%04x")
            % product_id
        ));
    }
    return B2XX_PRODUCT_ID[product_id];
}

std::vector<usb_device_handle::sptr> get_b200_device_handles(const device_addr_t &hint)
{
    std::vector<usb_device_handle::vid_pid_pair_t> vid_pid_pair_list;

    if(hint.has_key("vid") && hint.has_key("pid") && hint.has_key("type") && hint["type"] == "b200") {
        vid_pid_pair_list.push_back(usb_device_handle::vid_pid_pair_t(uhd::cast::hexstr_cast<uint16_t>(hint.get("vid")),
                                                                      uhd::cast::hexstr_cast<uint16_t>(hint.get("pid"))));
    } else {
        vid_pid_pair_list = b200_vid_pid_pairs;
    }

    //find the usrps and load firmware
    return usb_device_handle::get_device_list(vid_pid_pair_list);
}

static device_addrs_t b200_find(const device_addr_t &hint)
{
    device_addrs_t b200_addrs;

    //return an empty list of addresses when type is set to non-b200
    if (hint.has_key("type") and hint["type"] != "b200") return b200_addrs;

    //Return an empty list of addresses when an address or resource is specified,
    //since an address and resource is intended for a different, non-USB, device.
    for(device_addr_t hint_i:  separate_device_addr(hint)) {
        if (hint_i.has_key("addr") || hint_i.has_key("resource")) return b200_addrs;
    }

    // Important note:
    // The get device list calls are nested inside the for loop.
    // This allows the usb guts to decontruct when not in use,
    // so that re-enumeration after fw load can occur successfully.
    // This requirement is a courtesy of libusb1.0 on windows.
    size_t found = 0;
    for(usb_device_handle::sptr handle:  get_b200_device_handles(hint)) {
        //extract the firmware path for the b200
        std::string b200_fw_image;
        try{
            b200_fw_image = hint.get("fw", B200_FW_FILE_NAME);
            b200_fw_image = uhd::find_image_path(b200_fw_image, STR(UHD_IMAGES_DIR)); // FIXME
        }
        catch(uhd::exception &e){
            UHD_LOGGER_WARNING("B200") << e.what();
            return b200_addrs;
        }
        UHD_LOGGER_DEBUG("B200") << "the firmware image: " << b200_fw_image ;

        usb_control::sptr control;
        try{control = usb_control::make(handle, 0);}
        catch(const uhd::exception &){continue;} //ignore claimed

        //check if fw was already loaded
        if (!(handle->firmware_loaded()))
        {
            b200_iface::make(control)->load_firmware(b200_fw_image);
        }

        found++;
    }

    const auto timeout_time =
        std::chrono::steady_clock::now()
        + std::chrono::milliseconds(REENUMERATION_TIMEOUT_MS);
    //search for the device until found or timeout
    while (std::chrono::steady_clock::now() < timeout_time
            and b200_addrs.empty()
            and found != 0) {
        for(usb_device_handle::sptr handle:  get_b200_device_handles(hint)) {
            usb_control::sptr control;
            try{control = usb_control::make(handle, 0);}
            catch(const uhd::exception &){continue;} //ignore claimed

            b200_iface::sptr iface = b200_iface::make(control);
            const mboard_eeprom_t mb_eeprom = b200_impl::get_mb_eeprom(iface);

            device_addr_t new_addr;
            new_addr["type"] = "b200";
            new_addr["name"] = mb_eeprom["name"];
            new_addr["serial"] = handle->get_serial();
            try {
                // Turn the 16-Bit product ID into a string representation
                new_addr["product"] = B2XX_STR_NAMES[get_b200_product(handle, mb_eeprom)];
            } catch (const uhd::runtime_error &) {
                // No problem if this fails -- this is just device discovery, after all.
                new_addr["product"] = "B2??";
            }

            //this is a found b200 when the hint serial and name match or blank
            if (
                (not hint.has_key("name")   or hint["name"]   == new_addr["name"]) and
                (not hint.has_key("serial") or hint["serial"] == new_addr["serial"])
            ){
                b200_addrs.push_back(new_addr);
            }
        }
    }

    return b200_addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
static device::sptr b200_make(const device_addr_t &device_addr)
{
    uhd::transport::usb_device_handle::sptr handle;

    // We try twice, because the first time, the link might be in a bad state
    // and we might need to reset the link, but if that didn't help, trying
    // a third time is pointless.
    try {
        return device::sptr(new b200_impl(device_addr, handle));
    }
    catch (const uhd::usb_error &) {
        UHD_LOGGER_INFO("B200") << "Detected bad USB state; resetting." ;
        libusb::device_handle::sptr dev_handle(libusb::device_handle::get_cached_handle(
            boost::static_pointer_cast<libusb::special_handle>(handle)->get_device()
        ));
        dev_handle->clear_endpoints(B200_USB_CTRL_RECV_ENDPOINT, B200_USB_CTRL_SEND_ENDPOINT);
        dev_handle->clear_endpoints(B200_USB_DATA_RECV_ENDPOINT, B200_USB_DATA_SEND_ENDPOINT);
        dev_handle->reset_device();
    }

    return device::sptr(new b200_impl(device_addr, handle));
}

UHD_STATIC_BLOCK(register_b200_device)
{
    device::register_device(&b200_find, &b200_make, device::USRP);
}

/***********************************************************************
 * Structors
 **********************************************************************/
b200_impl::b200_impl(const uhd::device_addr_t& device_addr, usb_device_handle::sptr &handle) :
    _product(B200), // Some safe value
    _revision(0),
    _time_source(UNKNOWN),
    _tick_rate(0.0) // Forces a clock initialization at startup
{
    _tree = property_tree::make();
    _type = device::USRP;
    const fs_path mb_path = "/mboards/0";

    //try to match the given device address with something on the USB bus
    uint16_t vid = B200_VENDOR_ID;
    uint16_t pid = B200_PRODUCT_ID;
    bool specified_vid = false;
    bool specified_pid = false;

    if (device_addr.has_key("vid"))
    {
        vid = uhd::cast::hexstr_cast<uint16_t>(device_addr.get("vid"));
        specified_vid = true;
    }

    if (device_addr.has_key("pid"))
    {
        pid = uhd::cast::hexstr_cast<uint16_t>(device_addr.get("pid"));
        specified_pid = true;
    }

    std::vector<usb_device_handle::vid_pid_pair_t> vid_pid_pair_list;//search list for devices.

    // Search only for specified VID and PID if both specified
    if (specified_vid && specified_pid)
    {
        vid_pid_pair_list.push_back(usb_device_handle::vid_pid_pair_t(vid,pid));
    }
    // Search for all supported PIDs limited to specified VID if only VID specified
    else if (specified_vid)
    {
        vid_pid_pair_list.push_back(usb_device_handle::vid_pid_pair_t(vid, B200_PRODUCT_ID));
        vid_pid_pair_list.push_back(usb_device_handle::vid_pid_pair_t(vid, B200MINI_PRODUCT_ID));
        vid_pid_pair_list.push_back(usb_device_handle::vid_pid_pair_t(vid, B205MINI_PRODUCT_ID));
        vid_pid_pair_list.push_back(usb_device_handle::vid_pid_pair_t(vid, B200_PRODUCT_NI_ID));
        vid_pid_pair_list.push_back(usb_device_handle::vid_pid_pair_t(vid, B210_PRODUCT_NI_ID));
    }
    // Search for all supported VIDs limited to specified PID if only PID specified
    else if (specified_pid)
    {
        vid_pid_pair_list.push_back(usb_device_handle::vid_pid_pair_t(B200_VENDOR_ID,pid));
        vid_pid_pair_list.push_back(usb_device_handle::vid_pid_pair_t(B200_VENDOR_NI_ID,pid));
    }
    // Search for all supported devices if neither VID nor PID specified
    else
    {
        vid_pid_pair_list.push_back(usb_device_handle::vid_pid_pair_t(B200_VENDOR_ID,    B200_PRODUCT_ID));
        vid_pid_pair_list.push_back(usb_device_handle::vid_pid_pair_t(B200_VENDOR_ID,    B200MINI_PRODUCT_ID));
        vid_pid_pair_list.push_back(usb_device_handle::vid_pid_pair_t(B200_VENDOR_ID,    B205MINI_PRODUCT_ID));
        vid_pid_pair_list.push_back(usb_device_handle::vid_pid_pair_t(B200_VENDOR_NI_ID, B200_PRODUCT_NI_ID));
        vid_pid_pair_list.push_back(usb_device_handle::vid_pid_pair_t(B200_VENDOR_NI_ID, B210_PRODUCT_NI_ID));
    }

    std::vector<usb_device_handle::sptr> device_list = usb_device_handle::get_device_list(vid_pid_pair_list);

    //locate the matching handle in the device list
    for(usb_device_handle::sptr dev_handle:  device_list) {
        try {
            if (dev_handle->get_serial() == device_addr["serial"]){
                handle = dev_handle;
                break;
            }
        } catch (const uhd::exception&) { continue; }
    }
    UHD_ASSERT_THROW(handle.get() != NULL); //better be found

    //create control objects
    usb_control::sptr control = usb_control::make(handle, 0);
    _iface = b200_iface::make(control);
    this->check_fw_compat(); //check after making

    ////////////////////////////////////////////////////////////////////
    // setup the mboard eeprom
    ////////////////////////////////////////////////////////////////////
    const mboard_eeprom_t mb_eeprom = get_mb_eeprom(_iface);
    _tree->create<mboard_eeprom_t>(mb_path / "eeprom")
        .set(mb_eeprom)
        .add_coerced_subscriber(boost::bind(&b200_impl::set_mb_eeprom, this, _1));

    ////////////////////////////////////////////////////////////////////
    // Identify the device type
    ////////////////////////////////////////////////////////////////////
    std::string default_file_name;
    std::string product_name;
    try {
        // This will throw if the product ID is invalid:
        _product = get_b200_product(handle, mb_eeprom);
        default_file_name = B2XX_FPGA_FILE_NAME.get(_product);
        product_name      = B2XX_STR_NAMES.get(_product);
    } catch (const uhd::runtime_error &e) {
        // The only reason we may let this pass is if the user specified
        // the FPGA file name:
        if (not device_addr.has_key("fpga")) {
            throw e;
        }
        // In this case, we must provide a default product name:
        product_name = "B200?";
    }
    if (not mb_eeprom["revision"].empty()) {
        _revision = boost::lexical_cast<size_t>(mb_eeprom["revision"]);
    }

    UHD_LOGGER_INFO("B200") << "Detected Device: " << B2XX_STR_NAMES[_product] ;

    _gpsdo_capable = (not (_product == B200MINI or _product == B205MINI));

    ////////////////////////////////////////////////////////////////////
    // Set up frontend mapping
    ////////////////////////////////////////////////////////////////////
    // Explanation: The AD9361 has 2 frontends, FE1 and FE2.
    // On the B210 FE1 maps to the B-side (or radio 1), and FE2 maps
    // to the A-side (or radio 0). So, logically, the radios are swapped
    // between the host side and the AD9361-side.
    // B200 is more complicated: On Revs <= 4, the A-side is connected,
    // which means FE2 is used (like B210). On Revs >= 5, the left side
    // ("B-side") is connected, because these revs use an AD9364, which
    // does not have an FE2, so we don't swap FEs.

    // Swapped setup:
    _fe1 = 1;
    _fe2 = 0;
    _gpio_state.swap_atr = 1;
    // Unswapped setup:
    if (_product == B200MINI or _product == B205MINI or (_product == B200 and _revision >= 5)) {
        _fe1 = 0;                   //map radio0 to FE1
        _fe2 = 1;                   //map radio1 to FE2
        _gpio_state.swap_atr = 0; // ATRs for radio0 are mapped to FE1
    }

    ////////////////////////////////////////////////////////////////////
    // Load the FPGA image, then reset GPIF
    ////////////////////////////////////////////////////////////////////
    //extract the FPGA path for the B200
    std::string b200_fpga_image = find_image_path(
        device_addr.has_key("fpga")? device_addr["fpga"] : default_file_name
    );

    uint32_t status = _iface->load_fpga(b200_fpga_image);

    if(status != 0) {
        throw uhd::runtime_error(str(boost::format("fx3 is in state %1%") % status));
    }

    _iface->reset_gpif();

    ////////////////////////////////////////////////////////////////////
    // Create control transport
    ////////////////////////////////////////////////////////////////////
    uint8_t usb_speed = _iface->get_usb_speed();
    UHD_LOGGER_INFO("B200") << "Operating over USB " << (int) usb_speed << "." ;
    const std::string min_frame_size = (usb_speed == 3) ? "1024" : "512";

    device_addr_t ctrl_xport_args;
    ctrl_xport_args["recv_frame_size"] = min_frame_size;
    ctrl_xport_args["num_recv_frames"] = "16";
    ctrl_xport_args["send_frame_size"] = min_frame_size;
    ctrl_xport_args["num_send_frames"] = "16";

    // This may throw a uhd::usb_error, which will be caught by b200_make().
    _ctrl_transport = usb_zero_copy::make(
        handle,
        B200_USB_CTRL_RECV_INTERFACE, B200_USB_CTRL_RECV_ENDPOINT, //interface, endpoint
        B200_USB_CTRL_SEND_INTERFACE, B200_USB_CTRL_SEND_ENDPOINT, //interface, endpoint
        ctrl_xport_args
    );
    while (_ctrl_transport->get_recv_buff(0.0)){} //flush ctrl xport
    _tree->create<double>(mb_path / "link_max_rate").set((usb_speed == 3) ? B200_MAX_RATE_USB3 : B200_MAX_RATE_USB2);

    ////////////////////////////////////////////////////////////////////
    // Async task structure
    ////////////////////////////////////////////////////////////////////
    _async_task_data.reset(new AsyncTaskData());
    _async_task_data->async_md.reset(new async_md_type(1000/*messages deep*/));
    if (_gpsdo_capable)
    {
        _async_task_data->gpsdo_uart = b200_uart::make(_ctrl_transport, B200_TX_GPS_UART_SID);
    }
    _async_task = uhd::msg_task::make(boost::bind(&b200_impl::handle_async_task, this, _ctrl_transport, _async_task_data));

    ////////////////////////////////////////////////////////////////////
    // Local control endpoint
    ////////////////////////////////////////////////////////////////////
    _local_ctrl = radio_ctrl_core_3000::make(false/*lilE*/, _ctrl_transport, zero_copy_if::sptr()/*null*/, B200_LOCAL_CTRL_SID);
    _local_ctrl->hold_task(_async_task);
    _async_task_data->local_ctrl = _local_ctrl; //weak
    this->check_fpga_compat();

    /* Initialize the GPIOs, set the default bandsels to the lower range. Note
     * that calling update_bandsel calls update_gpio_state(). */
    update_bandsel("RX", 800e6);
    update_bandsel("TX", 850e6);

    ////////////////////////////////////////////////////////////////////
    // Create the GPSDO control
    ////////////////////////////////////////////////////////////////////
    if (_gpsdo_capable)
    {

        if ((_local_ctrl->peek32(RB32_CORE_STATUS) & 0xff) != B200_GPSDO_ST_NONE)
        {
            UHD_LOGGER_INFO("B200") << "Detecting internal GPSDO.... " << std::flush;
            try
            {
                _gps = gps_ctrl::make(_async_task_data->gpsdo_uart);
            }
            catch(std::exception &e)
            {
                UHD_LOGGER_ERROR("B200") << "An error occurred making GPSDO control: " << e.what();
            }
            if (_gps and _gps->gps_detected())
            {
                for(const std::string &name:  _gps->get_sensors())
                {
                    _tree->create<sensor_value_t>(mb_path / "sensors" / name)
                        .set_publisher(boost::bind(&gps_ctrl::get_sensor, _gps, name));
                }
            }
            else
            {
                _local_ctrl->poke32(TOREG(SR_CORE_GPSDO_ST), B200_GPSDO_ST_NONE);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////
    // Initialize the properties tree
    ////////////////////////////////////////////////////////////////////
    _tree->create<std::string>("/name").set("B-Series Device");
    _tree->create<std::string>(mb_path / "name").set(product_name);
    _tree->create<std::string>(mb_path / "codename").set((_product == B200MINI or _product == B205MINI) ? "Pixie" : "Sasquatch");

    ////////////////////////////////////////////////////////////////////
    // Create data transport
    // This happens after FPGA ctrl instantiated so any junk that might
    // be in the FPGAs buffers doesn't get pulled into the transport
    // before being cleared.
    ////////////////////////////////////////////////////////////////////
    device_addr_t data_xport_args;
    data_xport_args["recv_frame_size"] = device_addr.get("recv_frame_size", "8192");
    data_xport_args["num_recv_frames"] = device_addr.get("num_recv_frames", "16");
    data_xport_args["send_frame_size"] = device_addr.get("send_frame_size", "8192");
    data_xport_args["num_send_frames"] = device_addr.get("num_send_frames", "16");

    // This may throw a uhd::usb_error, which will be caught by b200_make().
    _data_transport = usb_zero_copy::make(
        handle,        // identifier
        B200_USB_DATA_RECV_INTERFACE, B200_USB_DATA_RECV_ENDPOINT, //interface, endpoint
        B200_USB_DATA_SEND_INTERFACE, B200_USB_DATA_SEND_ENDPOINT, //interface, endpoint
        data_xport_args    // param hints
    );
    while (_data_transport->get_recv_buff(0.0)){} //flush ctrl xport
    _demux = recv_packet_demuxer_3000::make(_data_transport);

    ////////////////////////////////////////////////////////////////////
    // create time and clock control objects
    ////////////////////////////////////////////////////////////////////
    _spi_iface = b200_local_spi_core::make(_local_ctrl);
    if (not (_product == B200MINI or _product == B205MINI)) {
        _adf4001_iface = boost::make_shared<b200_ref_pll_ctrl>(_spi_iface);
    }

    ////////////////////////////////////////////////////////////////////
    // Init codec - turns on clocks
    ////////////////////////////////////////////////////////////////////
    UHD_LOGGER_INFO("B200") << "Initialize CODEC control..." ;
    ad9361_params::sptr client_settings;
    if (_product == B200MINI or _product == B205MINI) {
        client_settings = boost::make_shared<b2xxmini_ad9361_client_t>();
    } else {
        client_settings = boost::make_shared<b200_ad9361_client_t>();
    }
    _codec_ctrl = ad9361_ctrl::make_spi(client_settings, _spi_iface, AD9361_SLAVENO);

    ////////////////////////////////////////////////////////////////////
    // create codec control objects
    ////////////////////////////////////////////////////////////////////
    {
        const fs_path codec_path = mb_path / ("rx_codecs") / "A";
        _tree->create<std::string>(codec_path / "name").set(product_name+" RX dual ADC");
        _tree->create<int>(codec_path / "gains"); //empty cuz gains are in frontend
    }
    {
        const fs_path codec_path = mb_path / ("tx_codecs") / "A";
        _tree->create<std::string>(codec_path / "name").set(product_name+" TX dual DAC");
        _tree->create<int>(codec_path / "gains"); //empty cuz gains are in frontend
    }

    ////////////////////////////////////////////////////////////////////
    // create clock control objects
    ////////////////////////////////////////////////////////////////////
    _tree->create<double>(mb_path / "tick_rate")
        .set_coercer(boost::bind(&b200_impl::set_tick_rate, this, _1))
        .set_publisher(boost::bind(&b200_impl::get_tick_rate, this))
        .add_coerced_subscriber(boost::bind(&b200_impl::update_tick_rate, this, _1));
    _tree->create<meta_range_t>(mb_path / "tick_rate/range")
        .set_publisher([this](){
            return this->_codec_ctrl->get_clock_rate_range();
        })
    ;
    _tree->create<time_spec_t>(mb_path / "time" / "cmd");
    _tree->create<bool>(mb_path / "auto_tick_rate").set(false);

    ////////////////////////////////////////////////////////////////////
    // and do the misc mboard sensors
    ////////////////////////////////////////////////////////////////////
    _tree->create<sensor_value_t>(mb_path / "sensors" / "ref_locked")
        .set_publisher(boost::bind(&b200_impl::get_ref_locked, this));

    ////////////////////////////////////////////////////////////////////
    // create frontend mapping
    ////////////////////////////////////////////////////////////////////
    std::vector<size_t> default_map(2, 0); default_map[1] = 1; // Set this to A->0 B->1 even if there's only A
    _tree->create<std::vector<size_t> >(mb_path / "rx_chan_dsp_mapping").set(default_map);
    _tree->create<std::vector<size_t> >(mb_path / "tx_chan_dsp_mapping").set(default_map);
    _tree->create<subdev_spec_t>(mb_path / "rx_subdev_spec")
        .set_coercer(boost::bind(&b200_impl::coerce_subdev_spec, this, _1))
        .set(subdev_spec_t())
        .add_coerced_subscriber(boost::bind(&b200_impl::update_subdev_spec, this, "rx", _1));
    _tree->create<subdev_spec_t>(mb_path / "tx_subdev_spec")
        .set_coercer(boost::bind(&b200_impl::coerce_subdev_spec, this, _1))
        .set(subdev_spec_t())
        .add_coerced_subscriber(boost::bind(&b200_impl::update_subdev_spec, this, "tx", _1));

    ////////////////////////////////////////////////////////////////////
    // setup radio control
    ////////////////////////////////////////////////////////////////////
    UHD_LOGGER_INFO("B200") << "Initialize Radio control..." ;
    const size_t num_radio_chains = ((_local_ctrl->peek32(RB32_CORE_STATUS) >> 8) & 0xff);
    UHD_ASSERT_THROW(num_radio_chains > 0);
    UHD_ASSERT_THROW(num_radio_chains <= 2);
    _radio_perifs.resize(num_radio_chains);
    _codec_mgr = ad936x_manager::make(_codec_ctrl, num_radio_chains);
    _codec_mgr->init_codec();
    for (size_t i = 0; i < _radio_perifs.size(); i++)
        this->setup_radio(i);

    //now test each radio module's connection to the codec interface
    for (radio_perifs_t &perif : _radio_perifs) {
        _codec_mgr->loopback_self_test(
            [&perif](const uint32_t value){
                perif.ctrl->poke32(TOREG(SR_CODEC_IDLE), value);
            },
            [&perif](){
                return perif.ctrl->peek64(RB64_CODEC_READBACK);
            }
        );
    }

    //register time now and pps onto available radio cores
    _tree->create<time_spec_t>(mb_path / "time" / "now")
        .set_publisher(boost::bind(&time_core_3000::get_time_now, _radio_perifs[0].time64))
        .add_coerced_subscriber(boost::bind(&b200_impl::set_time, this, _1))
        .set(0.0);
    //re-sync the times when the tick rate changes
    _tree->access<double>(mb_path / "tick_rate")
        .add_coerced_subscriber(boost::bind(&b200_impl::sync_times, this));
    _tree->create<time_spec_t>(mb_path / "time" / "pps")
        .set_publisher(boost::bind(&time_core_3000::get_time_last_pps, _radio_perifs[0].time64));
    for(radio_perifs_t &perif:  _radio_perifs)
    {
        _tree->access<time_spec_t>(mb_path / "time" / "pps")
            .add_coerced_subscriber(boost::bind(&time_core_3000::set_time_next_pps, perif.time64, _1));
    }

    //setup time source props
    const std::vector<std::string> time_sources =
        (_gpsdo_capable) ?
        std::vector<std::string>{"none", "internal", "external", "gpsdo"} :
        std::vector<std::string>{"none", "internal", "external"};
    _tree->create<std::vector<std::string> >(mb_path / "time_source" / "options")
        .set(time_sources);
    _tree->create<std::string>(mb_path / "time_source" / "value")
        .set_coercer(boost::bind(&check_option_valid, "time source", time_sources, _1))
        .add_coerced_subscriber(boost::bind(&b200_impl::update_time_source, this, _1));
    //setup reference source props
    const std::vector<std::string> clock_sources =
        (_gpsdo_capable) ?
        std::vector<std::string>{"internal", "external", "gpsdo"} :
        std::vector<std::string>{"internal", "external"};
    _tree->create<std::vector<std::string> >(mb_path / "clock_source" / "options")
        .set(clock_sources);
    _tree->create<std::string>(mb_path / "clock_source" / "value")
        .set_coercer(boost::bind(&check_option_valid, "clock source", clock_sources, _1))
        .add_coerced_subscriber(boost::bind(&b200_impl::update_clock_source, this, _1));

    ////////////////////////////////////////////////////////////////////
    // front panel gpio
    ////////////////////////////////////////////////////////////////////
    _radio_perifs[0].fp_gpio = gpio_atr_3000::make(_radio_perifs[0].ctrl, TOREG(SR_FP_GPIO), RB32_FP_GPIO);
    for(const gpio_attr_map_t::value_type attr:  gpio_attr_map)
    {       switch (attr.first){
                case usrp::gpio_atr::GPIO_SRC:
                    _tree->create<std::vector<std::string>>(mb_path / "gpio" / "FP0" / attr.second)
                        .set(std::vector<std::string>(32, usrp::gpio_atr::default_attr_value_map.at(attr.first)))
                        .add_coerced_subscriber([this](const std::vector<std::string>&){
                           throw uhd::runtime_error("This device does not support setting the GPIO_SRC attribute.");
                        });
                    break;
                case usrp::gpio_atr::GPIO_CTRL:
                case usrp::gpio_atr::GPIO_DDR:
                    _tree->create<std::vector<std::string>>(mb_path / "gpio" / "FP0" / attr.second)
                        .set(std::vector<std::string>(32, usrp::gpio_atr::default_attr_value_map.at(attr.first)))
                        .add_coerced_subscriber([this, attr](const std::vector<std::string> str_val){
                           uint32_t val = 0;
                           for(size_t i = 0 ; i < str_val.size() ; i++){
                               val += usrp::gpio_atr::gpio_attr_value_pair.at(attr.second).at(str_val[i])<<i;
                           }
                           _radio_perifs[0].fp_gpio->set_gpio_attr(attr.first, val);
                        });
                    break;
                case usrp::gpio_atr::GPIO_READBACK:
                    _tree->create<uint32_t>(mb_path / "gpio" / "FP0" / "READBACK")
                        .set_publisher(boost::bind(&gpio_atr_3000::read_gpio, _radio_perifs[0].fp_gpio));
                    break;
                default:
                    _tree->create<uint32_t>(mb_path / "gpio" / "FP0" / attr.second)
                    .set(0)
                    .add_coerced_subscriber(boost::bind(&gpio_atr_3000::set_gpio_attr, _radio_perifs[0].fp_gpio, attr.first, _1));
            }
    }

    ////////////////////////////////////////////////////////////////////
    // dboard eeproms but not really
    ////////////////////////////////////////////////////////////////////
    dboard_eeprom_t db_eeprom;
    _tree->create<dboard_eeprom_t>(mb_path / "dboards" / "A" / "rx_eeprom").set(db_eeprom);
    _tree->create<dboard_eeprom_t>(mb_path / "dboards" / "A" / "tx_eeprom").set(db_eeprom);
    _tree->create<dboard_eeprom_t>(mb_path / "dboards" / "A" / "gdb_eeprom").set(db_eeprom);

    ////////////////////////////////////////////////////////////////////
    // do some post-init tasks
    ////////////////////////////////////////////////////////////////////
    // Init the clock rate and the auto mcr appropriately
    if (not device_addr.has_key("master_clock_rate")) {
        UHD_LOGGER_INFO("B200") << "Setting master clock rate selection to 'automatic'." ;
    }
    // We can automatically choose a master clock rate, but not if the user specifies one
    const double default_tick_rate = device_addr.cast<double>("master_clock_rate", ad936x_manager::DEFAULT_TICK_RATE);
    _tree->access<double>(mb_path / "tick_rate").set(default_tick_rate);
    _tree->access<bool>(mb_path / "auto_tick_rate").set(not device_addr.has_key("master_clock_rate"));

    //subdev spec contains full width of selections
    subdev_spec_t rx_spec, tx_spec;
    for(const std::string &fe:  _tree->list(mb_path / "dboards" / "A" / "rx_frontends"))
    {
        rx_spec.push_back(subdev_spec_pair_t("A", fe));
    }
    for(const std::string &fe:  _tree->list(mb_path / "dboards" / "A" / "tx_frontends"))
    {
        tx_spec.push_back(subdev_spec_pair_t("A", fe));
    }
    _tree->access<subdev_spec_t>(mb_path / "rx_subdev_spec").set(rx_spec);
    _tree->access<subdev_spec_t>(mb_path / "tx_subdev_spec").set(tx_spec);

    //init to internal clock and time source
    _tree->access<std::string>(mb_path / "clock_source/value").set("internal");
    _tree->access<std::string>(mb_path / "time_source/value").set("internal");

    // Set the DSP chains to some safe value
    for (size_t i = 0; i < _radio_perifs.size(); i++) {
        _radio_perifs[i].ddc->set_host_rate(default_tick_rate / ad936x_manager::DEFAULT_DECIM);
        _radio_perifs[i].duc->set_host_rate(default_tick_rate / ad936x_manager::DEFAULT_INTERP);
    }
}

b200_impl::~b200_impl(void)
{
    UHD_SAFE_CALL
    (
         _async_task.reset();
    )
}

/***********************************************************************
 * setup radio control objects
 **********************************************************************/

void lambda_set_bool_prop(boost::weak_ptr<property_tree> tree_wptr, fs_path path, bool value, double)
{
    property_tree::sptr tree = tree_wptr.lock();
    if (tree) {
        tree->access<bool>(path).set(value);
    }
}

void b200_impl::setup_radio(const size_t dspno)
{
    radio_perifs_t &perif = _radio_perifs[dspno];
    const fs_path mb_path = "/mboards/0";

    ////////////////////////////////////////////////////////////////////
    // Set up transport
    ////////////////////////////////////////////////////////////////////
    const uint32_t sid = (dspno == 0) ? B200_CTRL0_MSG_SID : B200_CTRL1_MSG_SID;

    ////////////////////////////////////////////////////////////////////
    // radio control
    ////////////////////////////////////////////////////////////////////
    perif.ctrl = radio_ctrl_core_3000::make(
            false/*lilE*/,
            _ctrl_transport,
            zero_copy_if::sptr()/*null*/,
            sid);
    perif.ctrl->hold_task(_async_task);
    _async_task_data->radio_ctrl[dspno] = perif.ctrl; //weak
    _tree->access<time_spec_t>(mb_path / "time" / "cmd")
        .add_coerced_subscriber(boost::bind(&radio_ctrl_core_3000::set_time, perif.ctrl, _1));
    _tree->access<double>(mb_path / "tick_rate")
        .add_coerced_subscriber(boost::bind(&radio_ctrl_core_3000::set_tick_rate, perif.ctrl, _1));
    this->register_loopback_self_test(perif.ctrl);

    ////////////////////////////////////////////////////////////////////
    // Set up peripherals
    ////////////////////////////////////////////////////////////////////
    perif.atr = gpio_atr_3000::make_write_only(perif.ctrl, TOREG(SR_ATR));
    perif.atr->set_atr_mode(MODE_ATR, 0xFFFFFFFF);
    // create rx dsp control objects
    perif.framer = rx_vita_core_3000::make(perif.ctrl, TOREG(SR_RX_CTRL));
    perif.ddc = rx_dsp_core_3000::make(perif.ctrl, TOREG(SR_RX_DSP), true /*is_b200?*/);
    perif.ddc->set_link_rate(10e9/8); //whatever
    perif.ddc->set_mux(usrp::fe_connection_t(dspno == 1 ? "IbQb" : "IQ"));
    perif.ddc->set_freq(rx_dsp_core_3000::DEFAULT_CORDIC_FREQ);
    perif.deframer = tx_vita_core_3000::make_no_radio_buff(perif.ctrl, TOREG(SR_TX_CTRL));
    perif.duc = tx_dsp_core_3000::make(perif.ctrl, TOREG(SR_TX_DSP));
    perif.duc->set_link_rate(10e9/8); //whatever
    perif.duc->set_freq(tx_dsp_core_3000::DEFAULT_CORDIC_FREQ);

    ////////////////////////////////////////////////////////////////////
    // create time control objects
    ////////////////////////////////////////////////////////////////////
    time_core_3000::readback_bases_type time64_rb_bases;
    time64_rb_bases.rb_now = RB64_TIME_NOW;
    time64_rb_bases.rb_pps = RB64_TIME_PPS;
    perif.time64 = time_core_3000::make(perif.ctrl, TOREG(SR_TIME), time64_rb_bases);

    ////////////////////////////////////////////////////////////////////
    // connect rx dsp control objects
    ////////////////////////////////////////////////////////////////////
    const fs_path rx_dsp_path = mb_path / "rx_dsps" / dspno;
    perif.ddc->populate_subtree(_tree->subtree(rx_dsp_path));
    _tree->create<bool>(rx_dsp_path / "rate" / "set").set(false);
    _tree->access<double>(rx_dsp_path / "rate" / "value")
        .set_coercer(boost::bind(&b200_impl::coerce_rx_samp_rate, this, perif.ddc, dspno, _1))
        .add_coerced_subscriber(boost::bind(&lambda_set_bool_prop, boost::weak_ptr<property_tree>(_tree), rx_dsp_path / "rate" / "set", true, _1))
        .add_coerced_subscriber(boost::bind(&b200_impl::update_rx_samp_rate, this, dspno, _1))
    ;
    _tree->create<stream_cmd_t>(rx_dsp_path / "stream_cmd")
        .add_coerced_subscriber(boost::bind(&rx_vita_core_3000::issue_stream_command, perif.framer, _1));
    _tree->access<double>(mb_path / "tick_rate")
        .add_coerced_subscriber(boost::bind(&rx_vita_core_3000::set_tick_rate, perif.framer, _1))
        .add_coerced_subscriber(boost::bind(&b200_impl::update_rx_dsp_tick_rate, this, _1, perif.ddc, rx_dsp_path))
    ;

    ////////////////////////////////////////////////////////////////////
    // create tx dsp control objects
    ////////////////////////////////////////////////////////////////////
    const fs_path tx_dsp_path = mb_path / "tx_dsps" / dspno;
    perif.duc->populate_subtree(_tree->subtree(tx_dsp_path));
    _tree->create<bool>(tx_dsp_path / "rate" / "set").set(false);
    _tree->access<double>(tx_dsp_path / "rate" / "value")
        .set_coercer(boost::bind(&b200_impl::coerce_tx_samp_rate, this, perif.duc, dspno, _1))
        .add_coerced_subscriber(boost::bind(&lambda_set_bool_prop, boost::weak_ptr<property_tree>(_tree), tx_dsp_path / "rate" / "set", true, _1))
        .add_coerced_subscriber(boost::bind(&b200_impl::update_tx_samp_rate, this, dspno, _1))
    ;
    _tree->access<double>(mb_path / "tick_rate")
        .add_coerced_subscriber(boost::bind(&b200_impl::update_tx_dsp_tick_rate, this, _1, perif.duc, tx_dsp_path))
    ;

    ////////////////////////////////////////////////////////////////////
    // create RF frontend interfacing
    ////////////////////////////////////////////////////////////////////
    for (direction_t dir : std::vector<direction_t>{RX_DIRECTION, TX_DIRECTION}) {
        const std::string x = (dir == RX_DIRECTION) ? "rx" : "tx";
        const std::string key = std::string(((dir == RX_DIRECTION) ? "RX" : "TX")) + std::string(((dspno == _fe1) ? "1" : "2"));
        const fs_path rf_fe_path
            = mb_path / "dboards" / "A" / (x + "_frontends") / (dspno ? "B" : "A");

        // This will connect all the AD936x-specific items
        _codec_mgr->populate_frontend_subtree(
            _tree->subtree(rf_fe_path), key, dir
        );

        // Now connect all the b200_impl-specific items
        _tree->create<sensor_value_t>(rf_fe_path / "sensors" / "lo_locked")
            .set_publisher(boost::bind(&b200_impl::get_fe_pll_locked, this, dir == TX_DIRECTION))
        ;
        _tree->access<double>(rf_fe_path / "freq" / "value")
            .add_coerced_subscriber(boost::bind(&b200_impl::update_bandsel, this, key, _1))
        ;
        if (dir == RX_DIRECTION)
        {
            static const std::vector<std::string> ants{"TX/RX", "RX2"};
            _tree->create<std::vector<std::string> >(rf_fe_path / "antenna" / "options").set(ants);
            _tree->create<std::string>(rf_fe_path / "antenna" / "value")
                .add_coerced_subscriber(boost::bind(&b200_impl::update_antenna_sel, this, dspno, _1))
                .set("RX2")
            ;

        }
        else if (dir == TX_DIRECTION)
        {
            static const std::vector<std::string> ants(1, "TX/RX");
            _tree->create<std::vector<std::string> >(rf_fe_path / "antenna" / "options").set(ants);
            _tree->create<std::string>(rf_fe_path / "antenna" / "value").set("TX/RX");
        }
    }
}

/***********************************************************************
 * loopback tests
 **********************************************************************/

void b200_impl::register_loopback_self_test(wb_iface::sptr iface)
{
    bool test_fail = false;
    UHD_LOGGER_INFO("B200") << "Performing register loopback test... ";
    size_t hash = size_t(time(NULL));
    for (size_t i = 0; i < 100; i++)
    {
        boost::hash_combine(hash, i);
        iface->poke32(TOREG(SR_TEST), uint32_t(hash));
        test_fail = iface->peek32(RB32_TEST) != uint32_t(hash);
        if (test_fail) break; //exit loop on any failure
    }
    UHD_LOGGER_INFO("B200") << "Register loopback test " << ((test_fail)? "failed" : "passed") ;
}

/***********************************************************************
 * Sample and tick rate comprehension below
 **********************************************************************/
void b200_impl::enforce_tick_rate_limits(size_t chan_count, double tick_rate, const std::string &direction /*= ""*/)
{
    const size_t max_chans = 2;
    if (chan_count > max_chans)
    {
        throw uhd::value_error(boost::str(
            boost::format("cannot not setup %d %s channels (maximum is %d)")
                % chan_count
                % (direction.empty() ? "data" : direction)
                % max_chans
        ));
    }
    else
    {
        const double max_tick_rate = ad9361_device_t::AD9361_MAX_CLOCK_RATE / ((chan_count <= 1) ? 1 : 2);
        if (tick_rate - max_tick_rate >= 1.0)
        {
            throw uhd::value_error(boost::str(
                boost::format("current master clock rate (%.6f MHz) exceeds maximum possible master clock rate (%.6f MHz) when using %d %s channels")
                    % (tick_rate/1e6)
                    % (max_tick_rate/1e6)
                    % chan_count
                    % (direction.empty() ? "data" : direction)
            ));
        }
        const double min_tick_rate = ad9361_device_t::AD9361_MIN_CLOCK_RATE / ((chan_count <= 1) ? 1 : 2);
        if (min_tick_rate - tick_rate >= 1.0)
        {
            throw uhd::value_error(boost::str(
                boost::format("current master clock rate (%.6f MHz) is less than minimum possible master clock rate (%.6f MHz) when using %d %s channels")
                    % (tick_rate/1e6)
                    % (min_tick_rate/1e6)
                    % chan_count
                    % (direction.empty() ? "data" : direction)
            ));
        }
    }
}

double b200_impl::set_tick_rate(const double new_tick_rate)
{
    UHD_LOGGER_INFO("B200") << (boost::format("Asking for clock rate %.6f MHz... ") % (new_tick_rate/1e6)) << std::flush;
    check_tick_rate_with_current_streamers(new_tick_rate);   // Defined in b200_io_impl.cpp

    // Make sure the clock rate is actually changed before doing
    // the full Monty of setting regs and loopback tests etc.
    if (std::abs(new_tick_rate - _tick_rate) < 1.0) {
        UHD_LOGGER_INFO("B200") << "OK" ;
        return _tick_rate;
    }

    _tick_rate = _codec_ctrl->set_clock_rate(new_tick_rate);
    UHD_LOGGER_INFO("B200")  << (boost::format("Actually got clock rate %.6f MHz.") % (_tick_rate/1e6)) ;

    for(radio_perifs_t &perif:  _radio_perifs)
    {
        perif.time64->set_tick_rate(_tick_rate);
        perif.time64->self_test();
    }
    return _tick_rate;
}

/***********************************************************************
 * compat checks
 **********************************************************************/

void b200_impl::check_fw_compat(void)
{
    uint16_t compat_num = _iface->get_compat_num();
    uint32_t compat_major = (uint32_t) (compat_num >> 8);
    uint32_t compat_minor = (uint32_t) (compat_num & 0xFF);

    if (compat_major != B200_FW_COMPAT_NUM_MAJOR){
        throw uhd::runtime_error(str(boost::format(
            "Expected firmware compatibility number %d.%d, but got %d.%d:\n"
            "The firmware build is not compatible with the host code build.\n"
            "%s"
        )   % int(B200_FW_COMPAT_NUM_MAJOR) % int(B200_FW_COMPAT_NUM_MINOR)
            % compat_major % compat_minor % print_utility_error("uhd_images_downloader.py")));
    }
    _tree->create<std::string>("/mboards/0/fw_version").set(str(boost::format("%u.%u")
                % compat_major % compat_minor));
}

void b200_impl::check_fpga_compat(void)
{
    const uint64_t compat = _local_ctrl->peek64(0);
    const uint32_t signature = uint32_t(compat >> 32);
    const uint16_t compat_major = uint16_t(compat >> 16);
    const uint16_t compat_minor = uint16_t(compat & 0xffff);
    if (signature != 0xACE0BA5E) throw uhd::runtime_error(
        "b200::check_fpga_compat signature register readback failed");

    const uint16_t expected = ((_product == B200MINI or _product == B205MINI) ? B205_FPGA_COMPAT_NUM : B200_FPGA_COMPAT_NUM);
    if (compat_major != expected)
    {
        throw uhd::runtime_error(str(boost::format(
            "Expected FPGA compatibility number %d, but got %d:\n"
            "The FPGA build is not compatible with the host code build.\n"
            "%s"
        ) % int(expected) % compat_major % print_utility_error("uhd_images_downloader.py")));
    }
    _tree->create<std::string>("/mboards/0/fpga_version").set(str(boost::format("%u.%u")
                % compat_major % compat_minor));
}

/***********************************************************************
 * Reference time and clock
 **********************************************************************/

void b200_impl::update_clock_source(const std::string &source)
{
    // For B205, ref_sel selects whether or not to lock to the external clock source
    if (_product == B200MINI or _product == B205MINI)
    {
        if (source == "external" and _time_source == EXTERNAL)
        {
            throw uhd::value_error("external reference cannot be both a clock source and a time source");
        }

        if (source == "internal")
        {
            if (_gpio_state.ref_sel != 0)
            {
                _gpio_state.ref_sel = 0;
                this->update_gpio_state();
            }
        }
        else if (source == "external")
        {
            if (_gpio_state.ref_sel != 1)
            {
                _gpio_state.ref_sel = 1;
                this->update_gpio_state();
            }
        }
        else
        {
            throw uhd::key_error("update_clock_source: unknown source: " + source);
        }
        return;
    }

    // For all other devices, ref_sel selects the external or gpsdo clock source
    // and the ADF4001 selects whether to lock to it or not
    if (source == "internal")
    {
        _adf4001_iface->set_lock_to_ext_ref(false);
    }
    else if (source == "external")
    {
        if (_gpio_state.ref_sel != 0)
        {
            _gpio_state.ref_sel = 0;
            this->update_gpio_state();
        }
        _adf4001_iface->set_lock_to_ext_ref(true);
    }
    else if (source == "gpsdo")
    {
        if (not _gps or not _gps->gps_detected()) {
            throw uhd::key_error("update_clock_source: gpsdo selected, but no gpsdo detected!");
        }
        if (_gpio_state.ref_sel != 1)
        {
            _gpio_state.ref_sel = 1;
            this->update_gpio_state();
        }
        _adf4001_iface->set_lock_to_ext_ref(true);
    }
    else
    {
        throw uhd::key_error("update_clock_source: unknown source: " + source);
    }
}

void b200_impl::update_time_source(const std::string &source)
{
    if ((_product == B200MINI or _product == B205MINI) and source == "external" and _gpio_state.ref_sel == 1)
    {
        throw uhd::value_error("external reference cannot be both a time source and a clock source");
    }

    // We assume source is valid for this device (if it's gone through
    // the prop three, then it definitely is thanks to our coercer)
    time_source_t value;
    if (source == "none")
        value = NONE;
    else if (source == "internal")
        value = INTERNAL;
    else if (source == "external")
        value = EXTERNAL;
    else if (_gps and source == "gpsdo")
        value = GPSDO;
    else
        throw uhd::key_error("update_time_source: unknown source: " + source);
    if (_time_source != value)
    {
        _local_ctrl->poke32(TOREG(SR_CORE_SYNC), value);
        _time_source = value;
    }
}

void b200_impl::set_time(const uhd::time_spec_t& t)
{
    for(radio_perifs_t &perif:  _radio_perifs)
        perif.time64->set_time_sync(t);
    _local_ctrl->poke32(TOREG(SR_CORE_SYNC), 1 << 2 | uint32_t(_time_source));
    _local_ctrl->poke32(TOREG(SR_CORE_SYNC), _time_source);
}

void b200_impl::sync_times()
{
    set_time(_radio_perifs[0].time64->get_time_now());
}

/***********************************************************************
 * GPIO setup
 **********************************************************************/

void b200_impl::update_bandsel(const std::string& which, double freq)
{
    // B205 does not have bandsels
    if (_product == B200MINI or _product == B205MINI) {
        return;
    }

    if(which[0] == 'R') {
        if(freq < 2.2e9) {
            _gpio_state.rx_bandsel_a = 0;
            _gpio_state.rx_bandsel_b = 0;
            _gpio_state.rx_bandsel_c = 1;
        } else if((freq >= 2.2e9) && (freq < 4e9)) {
            _gpio_state.rx_bandsel_a = 0;
            _gpio_state.rx_bandsel_b = 1;
            _gpio_state.rx_bandsel_c = 0;
        } else if((freq >= 4e9) && (freq <= 6e9)) {
            _gpio_state.rx_bandsel_a = 1;
            _gpio_state.rx_bandsel_b = 0;
            _gpio_state.rx_bandsel_c = 0;
        } else {
            UHD_THROW_INVALID_CODE_PATH();
        }
    } else if(which[0] == 'T') {
        if(freq < 2.5e9) {
            _gpio_state.tx_bandsel_a = 0;
            _gpio_state.tx_bandsel_b = 1;
        } else if((freq >= 2.5e9) && (freq <= 6e9)) {
            _gpio_state.tx_bandsel_a = 1;
            _gpio_state.tx_bandsel_b = 0;
        } else {
            UHD_THROW_INVALID_CODE_PATH();
        }
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }

    update_gpio_state();
}

void b200_impl::update_gpio_state(void)
{
    const uint32_t misc_word = 0
        | (_gpio_state.swap_atr << 8)
        | (_gpio_state.tx_bandsel_a << 7)
        | (_gpio_state.tx_bandsel_b << 6)
        | (_gpio_state.rx_bandsel_a << 5)
        | (_gpio_state.rx_bandsel_b << 4)
        | (_gpio_state.rx_bandsel_c << 3)
        // Bit 2 currently not used.
        | (_gpio_state.mimo << 1)
        | (_gpio_state.ref_sel << 0)
    ;

    _local_ctrl->poke32(TOREG(SR_CORE_MISC), misc_word);
}

void b200_impl::update_atrs(void)
{
    if (_radio_perifs.size() > _fe1 and _radio_perifs[_fe1].atr)
    {
        radio_perifs_t &perif = _radio_perifs[_fe1];
        const bool enb_rx = bool(perif.rx_streamer.lock());
        const bool enb_tx = bool(perif.tx_streamer.lock());
        const bool is_rx2 = perif.ant_rx2;
        const size_t rxonly = (enb_rx)? ((is_rx2)? STATE_RX1_RX2 : STATE_RX1_TXRX) : STATE_OFF;
        const size_t txonly = (enb_tx)? (STATE_TX1_TXRX) : STATE_OFF;
        size_t fd = STATE_OFF;
        if (enb_rx and enb_tx) fd = STATE_FDX1_TXRX;
        if (enb_rx and not enb_tx) fd = rxonly;
        if (not enb_rx and enb_tx) fd = txonly;
        gpio_atr_3000::sptr atr = perif.atr;
        atr->set_atr_reg(ATR_REG_IDLE, STATE_OFF);
        atr->set_atr_reg(ATR_REG_RX_ONLY, rxonly);
        atr->set_atr_reg(ATR_REG_TX_ONLY, txonly);
        atr->set_atr_reg(ATR_REG_FULL_DUPLEX, fd);
    }
    if (_radio_perifs.size() > _fe2 and _radio_perifs[_fe2].atr)
    {
        radio_perifs_t &perif = _radio_perifs[_fe2];
        const bool enb_rx = bool(perif.rx_streamer.lock());
        const bool enb_tx = bool(perif.tx_streamer.lock());
        const bool is_rx2 = perif.ant_rx2;
        const size_t rxonly = (enb_rx)? ((is_rx2)? STATE_RX2_RX2 : STATE_RX2_TXRX) : STATE_OFF;
        const size_t txonly = (enb_tx)? (STATE_TX2_TXRX) : STATE_OFF;
        size_t fd = STATE_OFF;
        if (enb_rx and enb_tx) fd = STATE_FDX2_TXRX;
        if (enb_rx and not enb_tx) fd = rxonly;
        if (not enb_rx and enb_tx) fd = txonly;
        gpio_atr_3000::sptr atr = perif.atr;
        atr->set_atr_reg(ATR_REG_IDLE, STATE_OFF);
        atr->set_atr_reg(ATR_REG_RX_ONLY, rxonly);
        atr->set_atr_reg(ATR_REG_TX_ONLY, txonly);
        atr->set_atr_reg(ATR_REG_FULL_DUPLEX, fd);
    }
}

void b200_impl::update_antenna_sel(const size_t which, const std::string &ant)
{
    if (ant != "TX/RX" and ant != "RX2") throw uhd::value_error("b200: unknown RX antenna option: " + ant);
    _radio_perifs[which].ant_rx2 = (ant == "RX2");
    this->update_atrs();
}

void b200_impl::update_enables(void)
{
    //extract settings from state variables
    const bool enb_tx1 = (_radio_perifs.size() > _fe1) and bool(_radio_perifs[_fe1].tx_streamer.lock());
    const bool enb_rx1 = (_radio_perifs.size() > _fe1) and bool(_radio_perifs[_fe1].rx_streamer.lock());
    const bool enb_tx2 = (_radio_perifs.size() > _fe2) and bool(_radio_perifs[_fe2].tx_streamer.lock());
    const bool enb_rx2 = (_radio_perifs.size() > _fe2) and bool(_radio_perifs[_fe2].rx_streamer.lock());
    const size_t num_rx = (enb_rx1?1:0) + (enb_rx2?1:0);
    const size_t num_tx = (enb_tx1?1:0) + (enb_tx2?1:0);
    const bool mimo = num_rx == 2 or num_tx == 2;

    if ((num_rx + num_tx) == 3)
    {
        throw uhd::runtime_error("b200: 2 RX 1 TX and 1 RX 2 TX configurations not possible");
    }

    //setup the active chains in the codec
    _codec_ctrl->set_active_chains(enb_tx1, enb_tx2, enb_rx1, enb_rx2);
    if ((num_rx + num_tx) == 0) _codec_ctrl->set_active_chains(true, false, true, false); //enable something

    //figure out if mimo is enabled based on new state
    _gpio_state.mimo = (mimo)? 1 : 0;
    update_gpio_state();

    //atrs change based on enables
    this->update_atrs();
}

sensor_value_t b200_impl::get_ref_locked(void)
{
    const bool lock = (_local_ctrl->peek32(RB32_CORE_MISC) & 0x1) == 0x1;
    return sensor_value_t("Ref", lock, "locked", "unlocked");
}

sensor_value_t b200_impl::get_fe_pll_locked(const bool is_tx)
{
    const uint32_t st = _local_ctrl->peek32(RB32_CORE_PLL);
    const bool locked = is_tx ? ((st & 0x1) > 0) : ((st & 0x2) > 0);
    return sensor_value_t("LO", locked, "locked", "unlocked");
}

//
// Copyright 2012-2013 Ettus Research LLC
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

#include "b200_impl.hpp"
#include "b200_regs.hpp"
#include <uhd/transport/usb_control.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/images.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <boost/format.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/functional/hash.hpp>
#include <cstdio>
#include <ctime>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

const boost::uint16_t B200_VENDOR_ID  = 0x2500;
const boost::uint16_t B200_PRODUCT_ID = 0x0020;
const boost::uint16_t INIT_PRODUCT_ID = 0x00f0;

static const boost::posix_time::milliseconds REENUMERATION_TIMEOUT_MS(3000);

//! mapping of frontend to radio perif index
static const size_t FE1 = 1;
static const size_t FE2 = 0;

/***********************************************************************
 * Discovery
 **********************************************************************/
static device_addrs_t b200_find(const device_addr_t &hint)
{
    device_addrs_t b200_addrs;

    //return an empty list of addresses when type is set to non-b200
    if (hint.has_key("type") and hint["type"] != "b200") return b200_addrs;

    //Return an empty list of addresses when an address is specified,
    //since an address is intended for a different, non-USB, device.
    if (hint.has_key("addr")) return b200_addrs;

    unsigned int vid, pid;

    if(hint.has_key("vid") && hint.has_key("pid") && hint.has_key("type") && hint["type"] == "b200") {
        sscanf(hint.get("vid").c_str(), "%x", &vid);
        sscanf(hint.get("pid").c_str(), "%x", &pid);
    } else {
        vid = B200_VENDOR_ID;
        pid = B200_PRODUCT_ID;
    }

    // Important note:
    // The get device list calls are nested inside the for loop.
    // This allows the usb guts to decontruct when not in use,
    // so that re-enumeration after fw load can occur successfully.
    // This requirement is a courtesy of libusb1.0 on windows.

    //find the usrps and load firmware
    size_t found = 0;
    BOOST_FOREACH(usb_device_handle::sptr handle, usb_device_handle::get_device_list(vid, pid)) {
        //extract the firmware path for the b200
        std::string b200_fw_image;
        try{
            b200_fw_image = find_image_path(hint.get("fw", B200_FW_FILE_NAME));
        }
        catch(...){
            UHD_MSG(warning) << boost::format(
                "Could not locate B200 firmware.\n"
                "Please install the images package. %s\n"
            ) % print_images_error();
            return b200_addrs;
        }
        UHD_LOG << "the firmware image: " << b200_fw_image << std::endl;

        usb_control::sptr control;
        try{control = usb_control::make(handle, 0);}
        catch(const uhd::exception &){continue;} //ignore claimed

        //check if fw was already loaded
        if (handle->get_manufacturer() != "Ettus Research LLC")
        {
            b200_iface::make(control)->load_firmware(b200_fw_image);
        }

        found++;
    }

    const boost::system_time timeout_time = boost::get_system_time() + REENUMERATION_TIMEOUT_MS;

    //search for the device until found or timeout
    while (boost::get_system_time() < timeout_time and b200_addrs.empty() and found != 0)
    {
        BOOST_FOREACH(usb_device_handle::sptr handle, usb_device_handle::get_device_list(vid, pid))
        {
            usb_control::sptr control;
            try{control = usb_control::make(handle, 0);}
            catch(const uhd::exception &){continue;} //ignore claimed

            b200_iface::sptr iface = b200_iface::make(control);
            const mboard_eeprom_t mb_eeprom = mboard_eeprom_t(*iface, "B200");

            device_addr_t new_addr;
            new_addr["type"] = "b200";
            new_addr["name"] = mb_eeprom["name"];
            new_addr["serial"] = handle->get_serial();
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
    return device::sptr(new b200_impl(device_addr));
}

UHD_STATIC_BLOCK(register_b200_device)
{
    device::register_device(&b200_find, &b200_make);
}

/***********************************************************************
 * Structors
 **********************************************************************/
b200_impl::b200_impl(const device_addr_t &device_addr)
{
    _tree = property_tree::make();
    const fs_path mb_path = "/mboards/0";

    //try to match the given device address with something on the USB bus
    std::vector<usb_device_handle::sptr> device_list =
        usb_device_handle::get_device_list(B200_VENDOR_ID, B200_PRODUCT_ID);

    //locate the matching handle in the device list
    usb_device_handle::sptr handle;
    BOOST_FOREACH(usb_device_handle::sptr dev_handle, device_list) {
        if (dev_handle->get_serial() == device_addr["serial"]){
            handle = dev_handle;
            break;
        }
    }
    UHD_ASSERT_THROW(handle.get() != NULL); //better be found

    //create control objects
    usb_control::sptr control = usb_control::make(handle, 0);
    _iface = b200_iface::make(control);
    this->check_fw_compat(); //check after making

    ////////////////////////////////////////////////////////////////////
    // setup the mboard eeprom
    ////////////////////////////////////////////////////////////////////
    const mboard_eeprom_t mb_eeprom(*_iface, "B200");
    _tree->create<mboard_eeprom_t>(mb_path / "eeprom")
        .set(mb_eeprom)
        .subscribe(boost::bind(&b200_impl::set_mb_eeprom, this, _1));

    ////////////////////////////////////////////////////////////////////
    // Load the FPGA image, then reset GPIF
    ////////////////////////////////////////////////////////////////////
    std::string default_file_name;
    std::string product_name = "B200?";
    if (not mb_eeprom["product"].empty())
    {
        switch (boost::lexical_cast<boost::uint16_t>(mb_eeprom["product"]))
        {
        case 0x0001:
        case 0x7737:
            product_name = "B200";
            default_file_name = B200_FPGA_FILE_NAME;
            break;
        case 0x7738:
        case 0x0002:
            product_name = "B210";
            default_file_name = B210_FPGA_FILE_NAME;
            break;
        default: UHD_MSG(error) << "B200 unknown product code: " << mb_eeprom["product"] << std::endl;
        }
    }
    if (default_file_name.empty())
    {
        UHD_ASSERT_THROW(device_addr.has_key("fpga"));
    }

    //extract the FPGA path for the B200
    std::string b200_fpga_image = find_image_path(
        device_addr.has_key("fpga")? device_addr["fpga"] : default_file_name
    );

    boost::uint32_t status = _iface->load_fpga(b200_fpga_image);

    if(status != 0) {
        throw uhd::runtime_error(str(boost::format("fx3 is in state %1%") % status));
    }

    _iface->reset_gpif();

    ////////////////////////////////////////////////////////////////////
    // Create control transport
    ////////////////////////////////////////////////////////////////////
    boost::uint8_t usb_speed = _iface->get_usb_speed();
    UHD_MSG(status) << "Operating over USB " << (int) usb_speed << "." << std::endl;
    const std::string min_frame_size = (usb_speed == 3) ? "1024" : "512";

    device_addr_t ctrl_xport_args;
    ctrl_xport_args["recv_frame_size"] = min_frame_size;
    ctrl_xport_args["num_recv_frames"] = "16";
    ctrl_xport_args["send_frame_size"] = min_frame_size;
    ctrl_xport_args["num_send_frames"] = "16";

    _ctrl_transport = usb_zero_copy::make(
        handle,
        4, 8, //interface, endpoint
        3, 4, //interface, endpoint
        ctrl_xport_args
    );
    while (_ctrl_transport->get_recv_buff(0.0)){} //flush ctrl xport

    ////////////////////////////////////////////////////////////////////
    // Async task structure
    ////////////////////////////////////////////////////////////////////
    _async_task_data.reset(new AsyncTaskData());
    _async_task_data->async_md.reset(new async_md_type(1000/*messages deep*/));
    _async_task = uhd::task::make(boost::bind(&b200_impl::handle_async_task, this, _ctrl_transport, _async_task_data));

    ////////////////////////////////////////////////////////////////////
    // Local control endpoint
    ////////////////////////////////////////////////////////////////////
    _local_ctrl = radio_ctrl_core_3000::make(false/*lilE*/, _ctrl_transport, zero_copy_if::sptr()/*null*/, B200_LOCAL_CTRL_SID);
    _local_ctrl->hold_task(_async_task);
    _async_task_data->local_ctrl = _local_ctrl; //weak
    this->check_fpga_compat();

    /* Initialize the GPIOs, set the default bandsels to the lower range. Note
     * that calling update_bandsel calls update_gpio_state(). */
    _gpio_state = gpio_state();
    update_bandsel("RX", 800e6);
    update_bandsel("TX", 850e6);

    ////////////////////////////////////////////////////////////////////
    // Create the GPSDO control
    ////////////////////////////////////////////////////////////////////
    _async_task_data->gpsdo_uart = b200_uart::make(_ctrl_transport, B200_TX_GPS_UART_SID);
    _async_task_data->gpsdo_uart->set_baud_divider(B200_BUS_CLOCK_RATE/115200);
    _async_task_data->gpsdo_uart->write_uart("\n"); //cause the baud and response to be setup
    boost::this_thread::sleep(boost::posix_time::seconds(1)); //allow for a little propagation

    if ((_local_ctrl->peek32(RB32_CORE_STATUS) & 0xff) != B200_GPSDO_ST_NONE)
    {
        UHD_MSG(status) << "Detecting internal GPSDO.... " << std::flush;
        try
        {
            _gps = gps_ctrl::make(_async_task_data->gpsdo_uart);
        }
        catch(std::exception &e)
        {
            UHD_MSG(error) << "An error occurred making GPSDO control: " << e.what() << std::endl;
        }
        if (_gps and _gps->gps_detected())
        {
            //UHD_MSG(status) << "found" << std::endl;
            BOOST_FOREACH(const std::string &name, _gps->get_sensors())
            {
                _tree->create<sensor_value_t>(mb_path / "sensors" / name)
                    .publish(boost::bind(&gps_ctrl::get_sensor, _gps, name));
            }
        }
        else
        {
            UHD_MSG(status) << "not found" << std::endl;
            _local_ctrl->poke32(TOREG(SR_CORE_GPSDO_ST), B200_GPSDO_ST_NONE);
        }
    }

    ////////////////////////////////////////////////////////////////////
    // Initialize the properties tree
    ////////////////////////////////////////////////////////////////////
    _tree->create<std::string>("/name").set("B-Series Device");
    _tree->create<std::string>(mb_path / "name").set(product_name);
    _tree->create<std::string>(mb_path / "codename").set("Sasquatch");

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

    _data_transport = usb_zero_copy::make(
        handle,        // identifier
        2, 6,          // IN interface, endpoint
        1, 2,          // OUT interface, endpoint
        data_xport_args    // param hints
    );
    while (_data_transport->get_recv_buff(0.0)){} //flush ctrl xport
    _demux.reset(new recv_packet_demuxer_3000(_data_transport));

    ////////////////////////////////////////////////////////////////////
    // Init codec - turns on clocks
    ////////////////////////////////////////////////////////////////////
    UHD_MSG(status) << "Initialize CODEC control..." << std::endl;
    _codec_ctrl = ad9361_ctrl::make(_iface);
    this->reset_codec_dcm();

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
        .coerce(boost::bind(&b200_impl::set_tick_rate, this, _1))
        .publish(boost::bind(&b200_impl::get_tick_rate, this))
        .subscribe(boost::bind(&b200_impl::update_tick_rate, this, _1));
    _tree->create<time_spec_t>(mb_path / "time" / "cmd");

    ////////////////////////////////////////////////////////////////////
    // and do the misc mboard sensors
    ////////////////////////////////////////////////////////////////////
    _tree->create<sensor_value_t>(mb_path / "sensors" / "ref_locked")
        .publish(boost::bind(&b200_impl::get_ref_locked, this));

    ////////////////////////////////////////////////////////////////////
    // create frontend mapping
    ////////////////////////////////////////////////////////////////////
    _tree->create<subdev_spec_t>(mb_path / "rx_subdev_spec")
        .set(subdev_spec_t())
        .subscribe(boost::bind(&b200_impl::update_rx_subdev_spec, this, _1));
    _tree->create<subdev_spec_t>(mb_path / "tx_subdev_spec")
        .set(subdev_spec_t())
        .subscribe(boost::bind(&b200_impl::update_tx_subdev_spec, this, _1));

    ////////////////////////////////////////////////////////////////////
    // setup radio control
    ////////////////////////////////////////////////////////////////////
    UHD_MSG(status) << "Initialize Radio control..." << std::endl;
    const size_t num_radio_chains = ((_local_ctrl->peek32(RB32_CORE_STATUS) >> 8) & 0xff);
    UHD_ASSERT_THROW(num_radio_chains > 0);
    UHD_ASSERT_THROW(num_radio_chains <= 2);
    _radio_perifs.resize(num_radio_chains);
    for (size_t i = 0; i < _radio_perifs.size(); i++) this->setup_radio(i);

    //now test each radio module's connection to the codec interface
    _codec_ctrl->data_port_loopback(true);
    BOOST_FOREACH(radio_perifs_t &perif, _radio_perifs)
    {
        this->codec_loopback_self_test(perif.ctrl);
    }
    _codec_ctrl->data_port_loopback(false);

    ////////////////////////////////////////////////////////////////////
    // create time and clock control objects
    ////////////////////////////////////////////////////////////////////
    _spi_iface = spi_core_3000::make(_local_ctrl, TOREG(SR_CORE_SPI), RB32_CORE_SPI);
    _spi_iface->set_divider(B200_BUS_CLOCK_RATE/ADF4001_SPI_RATE);
    _adf4001_iface = boost::shared_ptr<adf4001_ctrl>(new adf4001_ctrl(_spi_iface, ADF4001_SLAVENO));

    //register time now and pps onto available radio cores
    _tree->create<time_spec_t>(mb_path / "time" / "now")
        .publish(boost::bind(&time_core_3000::get_time_now, _radio_perifs[0].time64));
    _tree->create<time_spec_t>(mb_path / "time" / "pps")
        .publish(boost::bind(&time_core_3000::get_time_last_pps, _radio_perifs[0].time64));
    for (size_t i = 0; i < _radio_perifs.size(); i++)
    {
        _tree->access<time_spec_t>(mb_path / "time" / "now")
            .subscribe(boost::bind(&time_core_3000::set_time_now, _radio_perifs[i].time64, _1));
        _tree->access<time_spec_t>(mb_path / "time" / "pps")
            .subscribe(boost::bind(&time_core_3000::set_time_next_pps, _radio_perifs[i].time64, _1));
    }

    //setup time source props
    _tree->create<std::string>(mb_path / "time_source" / "value")
        .subscribe(boost::bind(&b200_impl::update_time_source, this, _1));
    static const std::vector<std::string> time_sources = boost::assign::list_of("none")("external")("gpsdo");
    _tree->create<std::vector<std::string> >(mb_path / "time_source" / "options").set(time_sources);
    //setup reference source props
    _tree->create<std::string>(mb_path / "clock_source" / "value")
        .subscribe(boost::bind(&b200_impl::update_clock_source, this, _1));
    static const std::vector<std::string> clock_sources = boost::assign::list_of("internal")("external")("gpsdo");
    _tree->create<std::vector<std::string> >(mb_path / "clock_source" / "options").set(clock_sources);

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

    //init the clock rate to something reasonable
    _tree->access<double>(mb_path / "tick_rate").set(
        device_addr.cast<double>("master_clock_rate", B200_DEFAULT_TICK_RATE));

    //subdev spec contains full width of selections
    subdev_spec_t rx_spec, tx_spec;
    BOOST_FOREACH(const std::string &fe, _tree->list(mb_path / "dboards" / "A" / "rx_frontends"))
    {
        rx_spec.push_back(subdev_spec_pair_t("A", fe));
    }
    BOOST_FOREACH(const std::string &fe, _tree->list(mb_path / "dboards" / "A" / "tx_frontends"))
    {
        tx_spec.push_back(subdev_spec_pair_t("A", fe));
    }
    _tree->access<subdev_spec_t>(mb_path / "rx_subdev_spec").set(rx_spec);
    _tree->access<subdev_spec_t>(mb_path / "tx_subdev_spec").set(tx_spec);

    //init to internal clock and time source
    _tree->access<std::string>(mb_path / "clock_source/value").set("internal");
    _tree->access<std::string>(mb_path / "time_source/value").set("none");

    //GPS installed: use external ref, time, and init time spec
    if (_gps and _gps->gps_detected())
    {
        UHD_MSG(status) << "Setting references to the internal GPSDO" << std::endl;
        _tree->access<std::string>(mb_path / "time_source" / "value").set("gpsdo");
        _tree->access<std::string>(mb_path / "clock_source" / "value").set("gpsdo");
        UHD_MSG(status) << "Initializing time to the internal GPSDO" << std::endl;
        const time_t tp = time_t(_gps->get_sensor("gps_time").to_int()+1);
        _tree->access<time_spec_t>(mb_path / "time" / "pps").set(time_spec_t(tp));
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

void b200_impl::setup_radio(const size_t dspno)
{
    radio_perifs_t &perif = _radio_perifs[dspno];
    const fs_path mb_path = "/mboards/0";

    ////////////////////////////////////////////////////////////////////
    // radio control
    ////////////////////////////////////////////////////////////////////
    const boost::uint32_t sid = (dspno == 0)? B200_CTRL0_MSG_SID : B200_CTRL1_MSG_SID;
    perif.ctrl = radio_ctrl_core_3000::make(false/*lilE*/, _ctrl_transport, zero_copy_if::sptr()/*null*/, sid);
    perif.ctrl->hold_task(_async_task);
    _async_task_data->radio_ctrl[dspno] = perif.ctrl; //weak
    _tree->access<time_spec_t>(mb_path / "time" / "cmd")
        .subscribe(boost::bind(&radio_ctrl_core_3000::set_time, perif.ctrl, _1));
    this->register_loopback_self_test(perif.ctrl);
    perif.atr = gpio_core_200_32wo::make(perif.ctrl, TOREG(SR_ATR));

    ////////////////////////////////////////////////////////////////////
    // create rx dsp control objects
    ////////////////////////////////////////////////////////////////////
    perif.framer = rx_vita_core_3000::make(perif.ctrl, TOREG(SR_RX_CTRL));
    perif.ddc = rx_dsp_core_3000::make(perif.ctrl, TOREG(SR_RX_DSP));
    perif.ddc->set_link_rate(10e9/8); //whatever
    _tree->access<double>(mb_path / "tick_rate")
        .subscribe(boost::bind(&rx_vita_core_3000::set_tick_rate, perif.framer, _1))
        .subscribe(boost::bind(&rx_dsp_core_3000::set_tick_rate, perif.ddc, _1));
    const fs_path rx_dsp_path = mb_path / "rx_dsps" / str(boost::format("%u") % dspno);
    _tree->create<meta_range_t>(rx_dsp_path / "rate" / "range")
        .publish(boost::bind(&rx_dsp_core_3000::get_host_rates, perif.ddc));
    _tree->create<double>(rx_dsp_path / "rate" / "value")
        .coerce(boost::bind(&rx_dsp_core_3000::set_host_rate, perif.ddc, _1))
        .subscribe(boost::bind(&b200_impl::update_rx_samp_rate, this, dspno, _1))
        .set(1e6);
    _tree->create<double>(rx_dsp_path / "freq" / "value")
        .coerce(boost::bind(&rx_dsp_core_3000::set_freq, perif.ddc, _1))
        .set(0.0);
    _tree->create<meta_range_t>(rx_dsp_path / "freq" / "range")
        .publish(boost::bind(&rx_dsp_core_3000::get_freq_range, perif.ddc));
    _tree->create<stream_cmd_t>(rx_dsp_path / "stream_cmd")
        .subscribe(boost::bind(&rx_vita_core_3000::issue_stream_command, perif.framer, _1));

    ////////////////////////////////////////////////////////////////////
    // create tx dsp control objects
    ////////////////////////////////////////////////////////////////////
    perif.deframer = tx_vita_core_3000::make(perif.ctrl, TOREG(SR_TX_CTRL));
    perif.duc = tx_dsp_core_3000::make(perif.ctrl, TOREG(SR_TX_DSP));
    perif.duc->set_link_rate(10e9/8); //whatever
    _tree->access<double>(mb_path / "tick_rate")
        .subscribe(boost::bind(&tx_vita_core_3000::set_tick_rate, perif.deframer, _1))
        .subscribe(boost::bind(&tx_dsp_core_3000::set_tick_rate, perif.duc, _1));
    const fs_path tx_dsp_path = mb_path / "tx_dsps" / str(boost::format("%u") % dspno);
    _tree->create<meta_range_t>(tx_dsp_path / "rate" / "range")
        .publish(boost::bind(&tx_dsp_core_3000::get_host_rates, perif.duc));
    _tree->create<double>(tx_dsp_path / "rate" / "value")
        .coerce(boost::bind(&tx_dsp_core_3000::set_host_rate, perif.duc, _1))
        .subscribe(boost::bind(&b200_impl::update_tx_samp_rate, this, dspno, _1))
        .set(1e6);
    _tree->create<double>(tx_dsp_path / "freq" / "value")
        .coerce(boost::bind(&tx_dsp_core_3000::set_freq, perif.duc, _1))
        .set(0.0);
    _tree->create<meta_range_t>(tx_dsp_path / "freq" / "range")
        .publish(boost::bind(&tx_dsp_core_3000::get_freq_range, perif.duc));

    ////////////////////////////////////////////////////////////////////
    // create time control objects
    ////////////////////////////////////////////////////////////////////
    time_core_3000::readback_bases_type time64_rb_bases;
    time64_rb_bases.rb_now = RB64_TIME_NOW;
    time64_rb_bases.rb_pps = RB64_TIME_PPS;
    perif.time64 = time_core_3000::make(perif.ctrl, TOREG(SR_TIME), time64_rb_bases);

    ////////////////////////////////////////////////////////////////////
    // create RF frontend interfacing
    ////////////////////////////////////////////////////////////////////
    for(size_t direction = 0; direction < 2; direction++)
    {
        const std::string x = direction? "rx" : "tx";
        const std::string key = std::string((direction? "RX" : "TX")) + std::string(((dspno == FE1)? "1" : "2"));
        const fs_path rf_fe_path = mb_path / "dboards" / "A" / (x+"_frontends") / (dspno? "B" : "A");

        _tree->create<std::string>(rf_fe_path / "name").set("FE-"+key);
        _tree->create<int>(rf_fe_path / "sensors"); //empty TODO
        BOOST_FOREACH(const std::string &name, ad9361_ctrl::get_gain_names(key))
        {
            _tree->create<meta_range_t>(rf_fe_path / "gains" / name / "range")
                .set(ad9361_ctrl::get_gain_range(key));

            _tree->create<double>(rf_fe_path / "gains" / name / "value")
                .coerce(boost::bind(&ad9361_ctrl::set_gain, _codec_ctrl, key, _1))
                .set(0.0);
        }
        _tree->create<std::string>(rf_fe_path / "connection").set("IQ");
        _tree->create<bool>(rf_fe_path / "enabled").set(true);
        _tree->create<bool>(rf_fe_path / "use_lo_offset").set(false);
        _tree->create<double>(rf_fe_path / "bandwidth" / "value")
            .coerce(boost::bind(&ad9361_ctrl::set_bw_filter, _codec_ctrl, key, _1))
            .set(40e6);
        _tree->create<meta_range_t>(rf_fe_path / "bandwidth" / "range")
            .publish(boost::bind(&ad9361_ctrl::get_bw_filter_range, key));
        _tree->create<double>(rf_fe_path / "freq" / "value")
            .set(0.0)
            .coerce(boost::bind(&ad9361_ctrl::tune, _codec_ctrl, key, _1))
            .subscribe(boost::bind(&b200_impl::update_bandsel, this, key, _1));
        _tree->create<meta_range_t>(rf_fe_path / "freq" / "range")
            .publish(boost::bind(&ad9361_ctrl::get_rf_freq_range));

        //setup antenna stuff
        if (key[0] == 'R')
        {
            static const std::vector<std::string> ants = boost::assign::list_of("TX/RX")("RX2");
            _tree->create<std::vector<std::string> >(rf_fe_path / "antenna" / "options").set(ants);
            _tree->create<std::string>(rf_fe_path / "antenna" / "value")
                .subscribe(boost::bind(&b200_impl::update_antenna_sel, this, dspno, _1))
                .set("RX2");
        }
        if (key[0] == 'T')
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
    UHD_MSG(status) << "Performing register loopback test... " << std::flush;
    size_t hash = time(NULL);
    for (size_t i = 0; i < 100; i++)
    {
        boost::hash_combine(hash, i);
        iface->poke32(TOREG(SR_TEST), boost::uint32_t(hash));
        test_fail = iface->peek32(RB32_TEST) != boost::uint32_t(hash);
        if (test_fail) break; //exit loop on any failure
    }
    UHD_MSG(status) << ((test_fail)? "fail" : "pass") << std::endl;
}

void b200_impl::codec_loopback_self_test(wb_iface::sptr iface)
{
    bool test_fail = false;
    UHD_MSG(status) << "Performing CODEC loopback test... " << std::flush;
    size_t hash = size_t(time(NULL));
    for (size_t i = 0; i < 100; i++)
    {
        boost::hash_combine(hash, i);
        const boost::uint32_t word32 = boost::uint32_t(hash) & 0xfff0fff0;
        iface->poke32(TOREG(SR_CODEC_IDLE), word32);
        iface->peek64(RB64_CODEC_READBACK); //enough idleness for loopback to propagate
        const boost::uint64_t rb_word64 = iface->peek64(RB64_CODEC_READBACK);
        const boost::uint32_t rb_tx = boost::uint32_t(rb_word64 >> 32);
        const boost::uint32_t rb_rx = boost::uint32_t(rb_word64 & 0xffffffff);
        test_fail = word32 != rb_tx or word32 != rb_rx;
        if (test_fail) break; //exit loop on any failure
    }
    UHD_MSG(status) << ((test_fail)? "fail" : "pass") << std::endl;

    /* Zero out the idle data. */
    iface->poke32(TOREG(SR_CODEC_IDLE), 0);
}

/***********************************************************************
 * Sample and tick rate comprehension below
 **********************************************************************/
double b200_impl::set_tick_rate(const double rate)
{
    UHD_MSG(status) << "Asking for clock rate " << rate/1e6 << " MHz\n";
    _tick_rate = _codec_ctrl->set_clock_rate(rate);
    UHD_MSG(status) << "Actually got clock rate " << _tick_rate/1e6 << " MHz\n";

    //reset after clock rate change
    this->reset_codec_dcm();

    BOOST_FOREACH(radio_perifs_t &perif, _radio_perifs)
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
    boost::uint16_t compat_num = _iface->get_compat_num();
    boost::uint32_t compat_major = (boost::uint32_t) (compat_num >> 8);
    boost::uint32_t compat_minor = (boost::uint32_t) (compat_num & 0xFF);

    if (compat_major != B200_FW_COMPAT_NUM_MAJOR){
        throw uhd::runtime_error(str(boost::format(
            "Expected firmware compatibility number 0x%x, but got 0x%x.%x:\n"
            "The firmware build is not compatible with the host code build.\n"
            "%s"
        ) % int(B200_FW_COMPAT_NUM_MAJOR) % compat_major % compat_minor
          % print_images_error()));
    }
    _tree->create<std::string>("/mboards/0/fw_version").set(str(boost::format("%u.%u")
                % compat_major % compat_minor));
}

void b200_impl::check_fpga_compat(void)
{
    const boost::uint64_t compat = _local_ctrl->peek64(0);
    const boost::uint32_t signature = boost::uint32_t(compat >> 32);
    const boost::uint16_t compat_major = boost::uint16_t(compat >> 16);
    const boost::uint16_t compat_minor = boost::uint16_t(compat & 0xffff);
    if (signature != 0xACE0BA5E) throw uhd::runtime_error(
        "b200::check_fpga_compat signature register readback failed");

    if (compat_major != B200_FPGA_COMPAT_NUM){
        throw uhd::runtime_error(str(boost::format(
            "Expected FPGA compatibility number 0x%x, but got 0x%x.%x:\n"
            "The FPGA build is not compatible with the host code build.\n"
            "%s"
        ) % int(B200_FPGA_COMPAT_NUM) % compat_major % compat_minor
          % print_images_error()));
    }
    _tree->create<std::string>("/mboards/0/fpga_version").set(str(boost::format("%u.%u")
                % compat_major % compat_minor));
}

void b200_impl::set_mb_eeprom(const uhd::usrp::mboard_eeprom_t &mb_eeprom)
{
    mb_eeprom.commit(*_iface, "B200");
}


/***********************************************************************
 * Reference time and clock
 **********************************************************************/

void b200_impl::update_clock_source(const std::string &source)
{
    if (source == "internal"){
        _adf4001_iface->set_lock_to_ext_ref(false);
    }
    else if ((source == "external")
              or (source == "gpsdo")){

        _adf4001_iface->set_lock_to_ext_ref(true);
    } else {
        throw uhd::key_error("update_clock_source: unknown source: " + source);
    }

    _gpio_state.ref_sel = (source == "gpsdo")? 1 : 0;
    this->update_gpio_state();
}

void b200_impl::update_time_source(const std::string &source)
{
    if (source == "none"){}
    else if (source == "external"){}
    else if (source == "gpsdo"){}
    else throw uhd::key_error("update_time_source: unknown source: " + source);
    _local_ctrl->poke32(TOREG(SR_CORE_PPS_SEL), (source == "external")? 1 : 0);
}

/***********************************************************************
 * GPIO setup
 **********************************************************************/

void b200_impl::update_bandsel(const std::string& which, double freq)
{
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
    const boost::uint32_t misc_word = 0
        | (_gpio_state.tx_bandsel_a << 7)
        | (_gpio_state.tx_bandsel_b << 6)
        | (_gpio_state.rx_bandsel_a << 5)
        | (_gpio_state.rx_bandsel_b << 4)
        | (_gpio_state.rx_bandsel_c << 3)
        | (_gpio_state.codec_arst << 2)
        | (_gpio_state.mimo << 1)
        | (_gpio_state.ref_sel << 0)
    ;

    _local_ctrl->poke32(TOREG(RB32_CORE_MISC), misc_word);
}

void b200_impl::reset_codec_dcm(void)
{
    _gpio_state.codec_arst = 1;
    this->update_gpio_state();
    boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    _gpio_state.codec_arst = 0;
    this->update_gpio_state();
}

void b200_impl::update_atrs(void)
{
    if (_radio_perifs.size() > FE1 and _radio_perifs[FE1].atr)
    {
        radio_perifs_t &perif = _radio_perifs[FE1];
        const bool enb_rx = bool(perif.rx_streamer.lock());
        const bool enb_tx = bool(perif.tx_streamer.lock());
        const bool is_rx2 = perif.ant_rx2;
        const size_t rxonly = (enb_rx)? ((is_rx2)? STATE_RX1_RX2 : STATE_RX1_TXRX) : STATE_OFF;
        const size_t txonly = (enb_tx)? (STATE_TX1_TXRX) : STATE_OFF;
        size_t fd = STATE_OFF;
        if (enb_rx and enb_tx) fd = STATE_FDX1_TXRX;
        if (enb_rx and not enb_tx) fd = rxonly;
        if (not enb_rx and enb_tx) fd = txonly;
        gpio_core_200_32wo::sptr atr = perif.atr;
        atr->set_atr_reg(dboard_iface::ATR_REG_IDLE, STATE_OFF);
        atr->set_atr_reg(dboard_iface::ATR_REG_RX_ONLY, rxonly);
        atr->set_atr_reg(dboard_iface::ATR_REG_TX_ONLY, txonly);
        atr->set_atr_reg(dboard_iface::ATR_REG_FULL_DUPLEX, fd);
    }
    if (_radio_perifs.size() > FE2 and _radio_perifs[FE2].atr)
    {
        radio_perifs_t &perif = _radio_perifs[FE2];
        const bool enb_rx = bool(perif.rx_streamer.lock());
        const bool enb_tx = bool(perif.tx_streamer.lock());
        const bool is_rx2 = perif.ant_rx2;
        const size_t rxonly = (enb_rx)? ((is_rx2)? STATE_RX2_RX2 : STATE_RX2_TXRX) : STATE_OFF;
        const size_t txonly = (enb_tx)? (STATE_TX2_TXRX) : STATE_OFF;
        size_t fd = STATE_OFF;
        if (enb_rx and enb_tx) fd = STATE_FDX2_TXRX;
        if (enb_rx and not enb_tx) fd = rxonly;
        if (not enb_rx and enb_tx) fd = txonly;
        gpio_core_200_32wo::sptr atr = perif.atr;
        atr->set_atr_reg(dboard_iface::ATR_REG_IDLE, STATE_OFF);
        atr->set_atr_reg(dboard_iface::ATR_REG_RX_ONLY, rxonly);
        atr->set_atr_reg(dboard_iface::ATR_REG_TX_ONLY, txonly);
        atr->set_atr_reg(dboard_iface::ATR_REG_FULL_DUPLEX, fd);
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
    const bool enb_tx1 = (_radio_perifs.size() > FE1) and bool(_radio_perifs[FE1].tx_streamer.lock());
    const bool enb_rx1 = (_radio_perifs.size() > FE1) and bool(_radio_perifs[FE1].rx_streamer.lock());
    const bool enb_tx2 = (_radio_perifs.size() > FE2) and bool(_radio_perifs[FE2].tx_streamer.lock());
    const bool enb_rx2 = (_radio_perifs.size() > FE2) and bool(_radio_perifs[FE2].rx_streamer.lock());
    const size_t num_rx = (enb_rx1?1:0) + (enb_rx2?1:0);
    const size_t num_tx = (enb_tx1?1:0) + (enb_tx2?1:0);
    const bool mimo = num_rx == 2 or num_tx == 2;

    //setup the active chains in the codec
    _codec_ctrl->set_active_chains(enb_tx1, enb_tx2, enb_rx1, enb_rx2);
    if ((num_rx + num_tx) == 0) _codec_ctrl->set_active_chains(true, false, true, false); //enable something
    this->reset_codec_dcm(); //set_active_chains could cause a clock rate change - reset dcm

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

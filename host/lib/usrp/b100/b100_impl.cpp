//
// Copyright 2012-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "b100_impl.hpp"
#include "b100_regs.hpp"
#include <uhd/transport/usb_control.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/cast.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhdlib/usrp/common/apply_corrections.hpp>
#include <boost/format.hpp>
#include <cstdio>
#include <iostream>
#include <chrono>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

namespace {
    constexpr uint16_t B100_VENDOR_ID  = 0x2500;
    constexpr uint16_t B100_PRODUCT_ID = 0x0002;
    constexpr int64_t  REENUMERATION_TIMEOUT_MS = 3000;
}

/***********************************************************************
 * Discovery
 **********************************************************************/
static device_addrs_t b100_find(const device_addr_t &hint)
{
    device_addrs_t b100_addrs;

    //return an empty list of addresses when type is set to non-b100
    if (hint.has_key("type") and hint["type"] != "b100") return b100_addrs;

    //Return an empty list of addresses when an address or resource is specified,
    //since an address and resource is intended for a different, non-USB, device.
    if (hint.has_key("addr") || hint.has_key("resource")) return b100_addrs;

    uint16_t vid, pid;

    if(hint.has_key("vid") && hint.has_key("pid") && hint.has_key("type") && hint["type"] == "b100") {
        vid = uhd::cast::hexstr_cast<uint16_t>(hint.get("vid"));
        pid = uhd::cast::hexstr_cast<uint16_t>(hint.get("pid"));
    } else {
        vid = B100_VENDOR_ID;
        pid = B100_PRODUCT_ID;
    }

    // Important note:
    // The get device list calls are nested inside the for loop.
    // This allows the usb guts to decontruct when not in use,
    // so that re-enumeration after fw load can occur successfully.
    // This requirement is a courtesy of libusb1.0 on windows.

    //find the usrps and load firmware
    size_t found = 0;
    for(usb_device_handle::sptr handle: usb_device_handle::get_device_list(vid,  pid)) {
        //extract the firmware path for the b100
        std::string b100_fw_image;
        try{
            b100_fw_image = find_image_path(hint.get("fw", B100_FW_FILE_NAME));
        }
        catch(...){
            UHD_LOGGER_WARNING("B100") << boost::format("Could not locate B100 firmware. %s\n") % print_utility_error("uhd_images_downloader.py");
            return b100_addrs;
        }
        UHD_LOGGER_DEBUG("B100") << "the firmware image: " << b100_fw_image ;

        usb_control::sptr control;
        try{control = usb_control::make(handle, 0);}
        catch(const uhd::exception &){continue;} //ignore claimed

        fx2_ctrl::make(control)->usrp_load_firmware(b100_fw_image);
        found++;
    }

    //get descriptors again with serial number, but using the initialized VID/PID now since we have firmware
    vid = B100_VENDOR_ID;
    pid = B100_PRODUCT_ID;

    const auto timeout_time =
        std::chrono::steady_clock::now()
        + std::chrono::milliseconds(REENUMERATION_TIMEOUT_MS);

    //search for the device until found or timeout
    while (std::chrono::steady_clock::now() < timeout_time
            and b100_addrs.empty()
            and found != 0) {
        for (auto handle : usb_device_handle::get_device_list(vid, pid)) {
            usb_control::sptr control;
            try {
                control = usb_control::make(handle, 0);
            }
            catch (const uhd::exception &) {
                continue; //ignore claimed
            }
            fx2_ctrl::sptr fx2_ctrl = fx2_ctrl::make(control);
            const mboard_eeprom_t mb_eeprom =
                b100_impl::get_mb_eeprom(fx2_ctrl);
            device_addr_t new_addr;
            new_addr["type"] = "b100";
            new_addr["name"] = mb_eeprom["name"];
            new_addr["serial"] = handle->get_serial();
            //this is a found b100 when the hint serial and name match or blank
            if (
                (not hint.has_key("name")   or hint["name"]   == new_addr["name"]) and
                (not hint.has_key("serial") or hint["serial"] == new_addr["serial"])
               ) {
                b100_addrs.push_back(new_addr);
            }
        }
    }

    return b100_addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
static device::sptr b100_make(const device_addr_t &device_addr){
    return device::sptr(new b100_impl(device_addr));
}

UHD_STATIC_BLOCK(register_b100_device){
    device::register_device(&b100_find, &b100_make, device::USRP);
}

/***********************************************************************
 * Structors
 **********************************************************************/
b100_impl::b100_impl(const device_addr_t &device_addr){
    size_t initialization_count = 0;
    b100_impl_constructor_begin:
    initialization_count++;

    _type = device::USRP;
    _tree = property_tree::make();
    _ignore_cal_file = device_addr.has_key("ignore-cal-file");

    //extract the FPGA path for the B100
    std::string b100_fpga_image = find_image_path(
        device_addr.has_key("fpga")? device_addr["fpga"] : B100_FPGA_FILE_NAME
    );

    //try to match the given device address with something on the USB bus
    std::vector<usb_device_handle::sptr> device_list =
        usb_device_handle::get_device_list(B100_VENDOR_ID, B100_PRODUCT_ID);

    //locate the matching handle in the device list
    usb_device_handle::sptr handle;
    for(usb_device_handle::sptr dev_handle:  device_list) {
        if (dev_handle->get_serial() == device_addr["serial"]){
            handle = dev_handle;
            break;
        }
    }
    UHD_ASSERT_THROW(handle.get() != NULL); //better be found

    //create control objects
    usb_control::sptr fx2_transport = usb_control::make(handle, 0);
    _fx2_ctrl = fx2_ctrl::make(fx2_transport);
    this->check_fw_compat(); //check after making fx2
    //-- setup clock after making fx2 and before loading fpga --//
    _clock_ctrl = b100_clock_ctrl::make(_fx2_ctrl, device_addr.cast<double>("master_clock_rate", B100_DEFAULT_TICK_RATE));

    //load FPGA image, slave xfers are disabled while loading
    this->enable_gpif(false);
    _fx2_ctrl->usrp_load_fpga(b100_fpga_image);
    _fx2_ctrl->usrp_fpga_reset(false); //active low reset
    _fx2_ctrl->usrp_fpga_reset(true);

    //create the control transport
    device_addr_t ctrl_xport_args;
    ctrl_xport_args["recv_frame_size"] = "512";
    ctrl_xport_args["num_recv_frames"] = "16";
    ctrl_xport_args["send_frame_size"] = "512";
    ctrl_xport_args["num_send_frames"] = "16";

    //try to open ctrl transport
    //this could fail with libusb_submit_transfer under some conditions
    try{
        _ctrl_transport = usb_zero_copy::make(
            handle,
            4, 8, //interface, endpoint
            3, 4, //interface, endpoint
            ctrl_xport_args
        );
    }
    //try reset once in the case of failure
    catch(const uhd::exception &ex){
        if (initialization_count > 1) throw;
        UHD_LOGGER_WARNING("B100") <<
            "The control endpoint was left in a bad state.\n"
            "Attempting endpoint re-enumeration...\n" << ex.what() ;
        _fifo_ctrl.reset();
        _ctrl_transport.reset();
        _fx2_ctrl->usrp_fx2_reset();
        goto b100_impl_constructor_begin;
    }
    this->enable_gpif(true);

    ////////////////////////////////////////////////////////////////////
    // Initialize FPGA control communication
    ////////////////////////////////////////////////////////////////////
    fifo_ctrl_excelsior_config fifo_ctrl_config;
    fifo_ctrl_config.async_sid_base = B100_TX_ASYNC_SID;
    fifo_ctrl_config.num_async_chan = 1;
    fifo_ctrl_config.ctrl_sid_base = B100_CTRL_MSG_SID;
    fifo_ctrl_config.spi_base = TOREG(SR_SPI);
    fifo_ctrl_config.spi_rb = REG_RB_SPI;
    _fifo_ctrl = fifo_ctrl_excelsior::make(_ctrl_transport, fifo_ctrl_config);

    //perform a test peek operation
    try{
        _fifo_ctrl->peek32(0);
    }
    //try reset once in the case of failure
    catch(const uhd::exception &){
        if (initialization_count > 1) throw;
        UHD_LOGGER_WARNING("B100") <<
            "The control endpoint was left in a bad state.\n"
            "Attempting endpoint re-enumeration...\n" ;
        _fifo_ctrl.reset();
        _ctrl_transport.reset();
        _fx2_ctrl->usrp_fx2_reset();
        goto b100_impl_constructor_begin;
    }
    this->check_fpga_compat(); //check after reset and making control

    ////////////////////////////////////////////////////////////////////
    // Initialize peripherals after reset
    ////////////////////////////////////////////////////////////////////
    _fpga_i2c_ctrl = i2c_core_200::make(_fifo_ctrl, TOREG(SR_I2C), REG_RB_I2C);

    ////////////////////////////////////////////////////////////////////
    // Create data transport
    // This happens after FPGA ctrl instantiated so any junk that might
    // be in the FPGAs buffers doesn't get pulled into the transport
    // before being cleared.
    ////////////////////////////////////////////////////////////////////
    device_addr_t data_xport_args;
    data_xport_args["recv_frame_size"] = device_addr.get("recv_frame_size", "16384");
    data_xport_args["num_recv_frames"] = device_addr.get("num_recv_frames", "16");
    data_xport_args["send_frame_size"] = device_addr.get("send_frame_size", "16384");
    data_xport_args["num_send_frames"] = device_addr.get("num_send_frames", "16");

    //let packet padder know the LUT size in number of words32
    const size_t rx_lut_size = size_t(data_xport_args.cast<double>("recv_frame_size", 0.0));
    _fifo_ctrl->poke32(TOREG(SR_PADDER+0), rx_lut_size/sizeof(uint32_t));

    _data_transport = usb_zero_copy_make_wrapper(
        usb_zero_copy::make(
            handle,        // identifier
            2, 6,          // IN interface, endpoint
            1, 2,          // OUT interface, endpoint
            data_xport_args    // param hints
        ),
        B100_MAX_PKT_BYTE_LIMIT
    );

    ////////////////////////////////////////////////////////////////////
    // Initialize the properties tree
    ////////////////////////////////////////////////////////////////////
    _tree->create<std::string>("/name").set("B-Series Device");
    const fs_path mb_path = "/mboards/0";
    _tree->create<std::string>(mb_path / "name").set("B100");
    _tree->create<std::string>(mb_path / "codename").set("B-Hundo");
    _tree->create<std::string>(mb_path / "load_eeprom")
        .add_coerced_subscriber(boost::bind(&fx2_ctrl::usrp_load_eeprom, _fx2_ctrl, _1));

    ////////////////////////////////////////////////////////////////////
    // setup the mboard eeprom
    ////////////////////////////////////////////////////////////////////
    const mboard_eeprom_t mb_eeprom = this->get_mb_eeprom(_fx2_ctrl);
    _tree->create<mboard_eeprom_t>(mb_path / "eeprom")
        .set(mb_eeprom)
        .add_coerced_subscriber(boost::bind(&b100_impl::set_mb_eeprom, this, _1));

    ////////////////////////////////////////////////////////////////////
    // create clock control objects
    ////////////////////////////////////////////////////////////////////
    //^^^ clock created up top, just reg props here... ^^^
    _tree->create<double>(mb_path / "tick_rate")
        .set_publisher(boost::bind(&b100_clock_ctrl::get_fpga_clock_rate, _clock_ctrl))
        .add_coerced_subscriber(boost::bind(&fifo_ctrl_excelsior::set_tick_rate, _fifo_ctrl, _1))
        .add_coerced_subscriber(boost::bind(&b100_impl::update_tick_rate, this, _1));

    //add_coerced_subscriber the command time while we are at it
    _tree->create<time_spec_t>(mb_path / "time/cmd")
        .add_coerced_subscriber(boost::bind(&fifo_ctrl_excelsior::set_time, _fifo_ctrl, _1));

    ////////////////////////////////////////////////////////////////////
    // create codec control objects
    ////////////////////////////////////////////////////////////////////
    _codec_ctrl = b100_codec_ctrl::make(_fifo_ctrl);
    const fs_path rx_codec_path = mb_path / "rx_codecs/A";
    const fs_path tx_codec_path = mb_path / "tx_codecs/A";
    _tree->create<std::string>(rx_codec_path / "name").set("ad9522");
    _tree->create<meta_range_t>(rx_codec_path / "gains/pga/range").set(b100_codec_ctrl::rx_pga_gain_range);
    _tree->create<double>(rx_codec_path / "gains/pga/value")
        .set_coercer(boost::bind(&b100_impl::update_rx_codec_gain, this, _1))
        .set(0.0);
    _tree->create<std::string>(tx_codec_path / "name").set("ad9522");
    _tree->create<meta_range_t>(tx_codec_path / "gains/pga/range").set(b100_codec_ctrl::tx_pga_gain_range);
    _tree->create<double>(tx_codec_path / "gains/pga/value")
        .add_coerced_subscriber(boost::bind(&b100_codec_ctrl::set_tx_pga_gain, _codec_ctrl, _1))
        .set_publisher(boost::bind(&b100_codec_ctrl::get_tx_pga_gain, _codec_ctrl))
        .set(0.0);

    ////////////////////////////////////////////////////////////////////
    // and do the misc mboard sensors
    ////////////////////////////////////////////////////////////////////
    _tree->create<sensor_value_t>(mb_path / "sensors/ref_locked")
        .set_publisher(boost::bind(&b100_impl::get_ref_locked, this));

    ////////////////////////////////////////////////////////////////////
    // create frontend control objects
    ////////////////////////////////////////////////////////////////////
    _rx_fe = rx_frontend_core_200::make(_fifo_ctrl, TOREG(SR_RX_FE));
    _tx_fe = tx_frontend_core_200::make(_fifo_ctrl, TOREG(SR_TX_FE));

    _tree->create<subdev_spec_t>(mb_path / "rx_subdev_spec")
        .add_coerced_subscriber(boost::bind(&b100_impl::update_rx_subdev_spec, this, _1));
    _tree->create<subdev_spec_t>(mb_path / "tx_subdev_spec")
        .add_coerced_subscriber(boost::bind(&b100_impl::update_tx_subdev_spec, this, _1));

    const fs_path rx_fe_path = mb_path / "rx_frontends" / "A";
    const fs_path tx_fe_path = mb_path / "tx_frontends" / "A";

    _tree->create<std::complex<double> >(rx_fe_path / "dc_offset" / "value")
        .set_coercer(boost::bind(&rx_frontend_core_200::set_dc_offset, _rx_fe, _1))
        .set(std::complex<double>(0.0, 0.0));
    _tree->create<bool>(rx_fe_path / "dc_offset" / "enable")
        .add_coerced_subscriber(boost::bind(&rx_frontend_core_200::set_dc_offset_auto, _rx_fe, _1))
        .set(true);
    _tree->create<std::complex<double> >(rx_fe_path / "iq_balance" / "value")
        .add_coerced_subscriber(boost::bind(&rx_frontend_core_200::set_iq_balance, _rx_fe, _1))
        .set(std::complex<double>(0.0, 0.0));
    _tree->create<std::complex<double> >(tx_fe_path / "dc_offset" / "value")
        .set_coercer(boost::bind(&tx_frontend_core_200::set_dc_offset, _tx_fe, _1))
        .set(std::complex<double>(0.0, 0.0));
    _tree->create<std::complex<double> >(tx_fe_path / "iq_balance" / "value")
        .add_coerced_subscriber(boost::bind(&tx_frontend_core_200::set_iq_balance, _tx_fe, _1))
        .set(std::complex<double>(0.0, 0.0));

    ////////////////////////////////////////////////////////////////////
    // create rx dsp control objects
    ////////////////////////////////////////////////////////////////////
    const size_t num_rx_dsps = _fifo_ctrl->peek32(REG_RB_NUM_RX_DSP);
    for (size_t dspno = 0; dspno < num_rx_dsps; dspno++)
    {
        const size_t sr_off = dspno*32;
        _rx_dsps.push_back(rx_dsp_core_200::make(
            _fifo_ctrl,
            TOREG(SR_RX_DSP0+sr_off),
            TOREG(SR_RX_CTRL0+sr_off),
            B100_RX_SID_BASE + dspno
        ));

        _rx_dsps[dspno]->set_link_rate(B100_LINK_RATE_BPS);
        _tree->access<double>(mb_path / "tick_rate")
            .add_coerced_subscriber(boost::bind(&rx_dsp_core_200::set_tick_rate, _rx_dsps[dspno], _1));
        fs_path rx_dsp_path = mb_path / str(boost::format("rx_dsps/%u") % dspno);
        _tree->create<meta_range_t>(rx_dsp_path / "rate/range")
            .set_publisher(boost::bind(&rx_dsp_core_200::get_host_rates, _rx_dsps[dspno]));
        _tree->create<double>(rx_dsp_path / "rate/value")
            .set(1e6) //some default
            .set_coercer(boost::bind(&rx_dsp_core_200::set_host_rate, _rx_dsps[dspno], _1))
            .add_coerced_subscriber(boost::bind(&b100_impl::update_rx_samp_rate, this, dspno, _1));
        _tree->create<double>(rx_dsp_path / "freq/value")
            .set_coercer(boost::bind(&rx_dsp_core_200::set_freq, _rx_dsps[dspno], _1));
        _tree->create<meta_range_t>(rx_dsp_path / "freq/range")
            .set_publisher(boost::bind(&rx_dsp_core_200::get_freq_range, _rx_dsps[dspno]));
        _tree->create<stream_cmd_t>(rx_dsp_path / "stream_cmd")
            .add_coerced_subscriber(boost::bind(&rx_dsp_core_200::issue_stream_command, _rx_dsps[dspno], _1));
    }

    ////////////////////////////////////////////////////////////////////
    // create tx dsp control objects
    ////////////////////////////////////////////////////////////////////
    _tx_dsp = tx_dsp_core_200::make(
        _fifo_ctrl, TOREG(SR_TX_DSP), TOREG(SR_TX_CTRL), B100_TX_ASYNC_SID
    );
    _tx_dsp->set_link_rate(B100_LINK_RATE_BPS);
    _tree->access<double>(mb_path / "tick_rate")
        .add_coerced_subscriber(boost::bind(&tx_dsp_core_200::set_tick_rate, _tx_dsp, _1));
    _tree->create<meta_range_t>(mb_path / "tx_dsps/0/rate/range")
        .set_publisher(boost::bind(&tx_dsp_core_200::get_host_rates, _tx_dsp));
    _tree->create<double>(mb_path / "tx_dsps/0/rate/value")
        .set(1e6) //some default
        .set_coercer(boost::bind(&tx_dsp_core_200::set_host_rate, _tx_dsp, _1))
        .add_coerced_subscriber(boost::bind(&b100_impl::update_tx_samp_rate, this, 0, _1));
    _tree->create<double>(mb_path / "tx_dsps/0/freq/value")
        .set_coercer(boost::bind(&tx_dsp_core_200::set_freq, _tx_dsp, _1));
    _tree->create<meta_range_t>(mb_path / "tx_dsps/0/freq/range")
        .set_publisher(boost::bind(&tx_dsp_core_200::get_freq_range, _tx_dsp));

    ////////////////////////////////////////////////////////////////////
    // create time control objects
    ////////////////////////////////////////////////////////////////////
    time64_core_200::readback_bases_type time64_rb_bases;
    time64_rb_bases.rb_hi_now = REG_RB_TIME_NOW_HI;
    time64_rb_bases.rb_lo_now = REG_RB_TIME_NOW_LO;
    time64_rb_bases.rb_hi_pps = REG_RB_TIME_PPS_HI;
    time64_rb_bases.rb_lo_pps = REG_RB_TIME_PPS_LO;
    _time64 = time64_core_200::make(
        _fifo_ctrl, TOREG(SR_TIME64), time64_rb_bases
    );
    _tree->access<double>(mb_path / "tick_rate")
        .add_coerced_subscriber(boost::bind(&time64_core_200::set_tick_rate, _time64, _1));
    _tree->create<time_spec_t>(mb_path / "time/now")
        .set_publisher(boost::bind(&time64_core_200::get_time_now, _time64))
        .add_coerced_subscriber(boost::bind(&time64_core_200::set_time_now, _time64, _1));
    _tree->create<time_spec_t>(mb_path / "time/pps")
        .set_publisher(boost::bind(&time64_core_200::get_time_last_pps, _time64))
        .add_coerced_subscriber(boost::bind(&time64_core_200::set_time_next_pps, _time64, _1));
    //setup time source props
    _tree->create<std::string>(mb_path / "time_source/value")
        .add_coerced_subscriber(boost::bind(&time64_core_200::set_time_source, _time64, _1));
    _tree->create<std::vector<std::string> >(mb_path / "time_source/options")
        .set_publisher(boost::bind(&time64_core_200::get_time_sources, _time64));
    //setup reference source props
    _tree->create<std::string>(mb_path / "clock_source/value")
        .add_coerced_subscriber(boost::bind(&b100_impl::update_clock_source, this, _1));
    static const std::vector<std::string> clock_sources = {
        "internal", "external", "auto"
    };
    _tree->create<std::vector<std::string> >(mb_path / "clock_source/options").set(clock_sources);

    ////////////////////////////////////////////////////////////////////
    // create user-defined control objects
    ////////////////////////////////////////////////////////////////////
    _user = user_settings_core_200::make(_fifo_ctrl, TOREG(SR_USER_REGS));
    _tree->create<user_settings_core_200::user_reg_t>(mb_path / "user/regs")
        .add_coerced_subscriber(boost::bind(&user_settings_core_200::set_reg, _user, _1));

    ////////////////////////////////////////////////////////////////////
    // create dboard control objects
    ////////////////////////////////////////////////////////////////////

    //read the dboard eeprom to extract the dboard ids
    dboard_eeprom_t rx_db_eeprom, tx_db_eeprom, gdb_eeprom;
    rx_db_eeprom.load(*_fpga_i2c_ctrl, I2C_ADDR_RX_A);
    tx_db_eeprom.load(*_fpga_i2c_ctrl, I2C_ADDR_TX_A);
    gdb_eeprom.load(*_fpga_i2c_ctrl, I2C_ADDR_TX_A ^ 5);

    //disable rx dc offset if LFRX
    if (rx_db_eeprom.id == 0x000f) _tree->access<bool>(rx_fe_path / "dc_offset" / "enable").set(false);

    //create the properties and register subscribers
    _tree->create<dboard_eeprom_t>(mb_path / "dboards/A/rx_eeprom")
        .set(rx_db_eeprom)
        .add_coerced_subscriber(boost::bind(&b100_impl::set_db_eeprom, this, "rx", _1));
    _tree->create<dboard_eeprom_t>(mb_path / "dboards/A/tx_eeprom")
        .set(tx_db_eeprom)
        .add_coerced_subscriber(boost::bind(&b100_impl::set_db_eeprom, this, "tx", _1));
    _tree->create<dboard_eeprom_t>(mb_path / "dboards/A/gdb_eeprom")
        .set(gdb_eeprom)
        .add_coerced_subscriber(boost::bind(&b100_impl::set_db_eeprom, this, "gdb", _1));

    //create a new dboard interface and manager
    _dboard_manager = dboard_manager::make(
        rx_db_eeprom.id, tx_db_eeprom.id, gdb_eeprom.id,
        make_b100_dboard_iface(_fifo_ctrl, _fpga_i2c_ctrl, _fifo_ctrl/*spi*/, _clock_ctrl, _codec_ctrl),
        _tree->subtree(mb_path / "dboards/A")
    );

    //bind frontend corrections to the dboard freq props
    const fs_path db_tx_fe_path = mb_path / "dboards" / "A" / "tx_frontends";
    for(const std::string &name:  _tree->list(db_tx_fe_path)){
        _tree->access<double>(db_tx_fe_path / name / "freq" / "value")
            .add_coerced_subscriber(boost::bind(&b100_impl::set_tx_fe_corrections, this, _1));
    }
    const fs_path db_rx_fe_path = mb_path / "dboards" / "A" / "rx_frontends";
    for(const std::string &name:  _tree->list(db_rx_fe_path)){
        _tree->access<double>(db_rx_fe_path / name / "freq" / "value")
            .add_coerced_subscriber(boost::bind(&b100_impl::set_rx_fe_corrections, this, _1));
    }

    //initialize io handling
    _recv_demuxer.reset(new recv_packet_demuxer_3000(_data_transport));

    //allocate streamer weak ptrs containers
    _rx_streamers.resize(_rx_dsps.size());
    _tx_streamers.resize(1/*known to be 1 dsp*/);

    ////////////////////////////////////////////////////////////////////
    // do some post-init tasks
    ////////////////////////////////////////////////////////////////////
    this->update_rates();

    _tree->access<double>(mb_path / "tick_rate") //now add_coerced_subscriber the clock rate setter
        .add_coerced_subscriber(boost::bind(&b100_clock_ctrl::set_fpga_clock_rate, _clock_ctrl, _1));

    //reset cordic rates and their properties to zero
    for(const std::string &name:  _tree->list(mb_path / "rx_dsps")){
        _tree->access<double>(mb_path / "rx_dsps" / name / "freq" / "value").set(0.0);
    }
    for(const std::string &name:  _tree->list(mb_path / "tx_dsps")){
        _tree->access<double>(mb_path / "tx_dsps" / name / "freq" / "value").set(0.0);
    }

    _tree->access<subdev_spec_t>(mb_path / "rx_subdev_spec").set(subdev_spec_t("A:" + _tree->list(mb_path / "dboards/A/rx_frontends").at(0)));
    _tree->access<subdev_spec_t>(mb_path / "tx_subdev_spec").set(subdev_spec_t("A:" + _tree->list(mb_path / "dboards/A/tx_frontends").at(0)));
    _tree->access<std::string>(mb_path / "clock_source/value").set("internal");
    _tree->access<std::string>(mb_path / "time_source/value").set("none");
    _tree->create<double>(mb_path / "link_max_rate").set(B100_MAX_RATE_USB2);
}

b100_impl::~b100_impl(void){
    //NOP
}

void b100_impl::check_fw_compat(void){
    unsigned char data[4]; //useless data buffer
    const uint16_t fw_compat_num = _fx2_ctrl->usrp_control_read(
        VRQ_FW_COMPAT, 0, 0, data, sizeof(data)
    );
    if (fw_compat_num != B100_FW_COMPAT_NUM){
        throw uhd::runtime_error(str(boost::format(
            "Expected firmware compatibility number %d, but got %d:\n"
            "The firmware build is not compatible with the host code build.\n"
            "%s"
        ) % int(B100_FW_COMPAT_NUM) % fw_compat_num % print_utility_error("uhd_images_downloader.py")));
    }
    _tree->create<std::string>("/mboards/0/fw_version").set(str(boost::format("%u.0") % fw_compat_num));
}

void b100_impl::check_fpga_compat(void){
    const uint32_t fpga_compat_num = _fifo_ctrl->peek32(REG_RB_COMPAT);
    uint16_t fpga_major = fpga_compat_num >> 16, fpga_minor = fpga_compat_num & 0xffff;
    if (fpga_major == 0){ //old version scheme
        fpga_major = fpga_minor;
        fpga_minor = 0;
    }
    if (fpga_major != B100_FPGA_COMPAT_NUM){
        throw uhd::runtime_error(str(boost::format(
            "Expected FPGA compatibility number %d, but got %d:\n"
            "The FPGA build is not compatible with the host code build."
            "%s"
        ) % int(B100_FPGA_COMPAT_NUM) % fpga_major % print_utility_error("uhd_images_downloader.py")));
    }
    _tree->create<std::string>("/mboards/0/fpga_version").set(str(boost::format("%u.%u") % fpga_major % fpga_minor));
}

double b100_impl::update_rx_codec_gain(const double gain){
    //set gain on both I and Q, readback on one
    //TODO in the future, gains should have individual control
    _codec_ctrl->set_rx_pga_gain(gain, 'A');
    _codec_ctrl->set_rx_pga_gain(gain, 'B');
    return _codec_ctrl->get_rx_pga_gain('A');
}

void b100_impl::set_db_eeprom(const std::string &type, const uhd::usrp::dboard_eeprom_t &db_eeprom){
    if (type == "rx") db_eeprom.store(*_fpga_i2c_ctrl, I2C_ADDR_RX_A);
    if (type == "tx") db_eeprom.store(*_fpga_i2c_ctrl, I2C_ADDR_TX_A);
    if (type == "gdb") db_eeprom.store(*_fpga_i2c_ctrl, I2C_ADDR_TX_A ^ 5);
}

void b100_impl::update_clock_source(const std::string &source){

    if (source == "pps_sync"){
        _clock_ctrl->use_external_ref();
        _fifo_ctrl->poke32(TOREG(SR_MISC+2), 1);
        return;
    }
    if (source == "_pps_sync_"){
        _clock_ctrl->use_external_ref();
        _fifo_ctrl->poke32(TOREG(SR_MISC+2), 3);
        return;
    }
    _fifo_ctrl->poke32(TOREG(SR_MISC+2), 0);

    if      (source == "auto")     _clock_ctrl->use_auto_ref();
    else if (source == "internal") _clock_ctrl->use_internal_ref();
    else if (source == "external") _clock_ctrl->use_external_ref();
    else throw uhd::runtime_error("unhandled clock configuration reference source: " + source);
}

////////////////// some GPIF preparation related stuff /////////////////
void b100_impl::enable_gpif(const bool en) {
    _fx2_ctrl->usrp_control_write(VRQ_ENABLE_GPIF, en ? 1 : 0, 0, 0, 0);
}

void b100_impl::clear_fpga_fifo(void) {
    _fx2_ctrl->usrp_control_write(VRQ_CLEAR_FPGA_FIFO, 0, 0, 0, 0);
}

sensor_value_t b100_impl::get_ref_locked(void){
    const bool lock = _clock_ctrl->get_locked();
    return sensor_value_t("Ref", lock, "locked", "unlocked");
}

void b100_impl::set_rx_fe_corrections(const double lo_freq){
    if(not _ignore_cal_file){
        apply_rx_fe_corrections(this->get_tree()->subtree("/mboards/0"), "A", lo_freq);
    }
}

void b100_impl::set_tx_fe_corrections(const double lo_freq){
    if(not _ignore_cal_file){
        apply_tx_fe_corrections(this->get_tree()->subtree("/mboards/0"), "A", lo_freq);
    }
}

//
// Copyright 2011 Ettus Research LLC
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

#include "b100_impl.hpp"
#include "b100_ctrl.hpp"
#include "fpga_regs_standard.h"
#include "usrp_i2c_addr.h"
#include "usrp_commands.h"
#include <uhd/transport/usb_control.hpp>
#include "ctrl_packet.hpp"
#include <uhd/utils/msg.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/images.hpp>
#include <uhd/utils/safe_call.hpp>
#include <boost/format.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include <boost/lexical_cast.hpp>
#include "b100_regs.hpp"
#include <cstdio>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

const boost::uint16_t B100_VENDOR_ID  = 0x2500;
const boost::uint16_t B100_PRODUCT_ID = 0x0002;
const boost::uint16_t FX2_VENDOR_ID    = 0x04b4;
const boost::uint16_t FX2_PRODUCT_ID   = 0x8613;

/***********************************************************************
 * Discovery
 **********************************************************************/
static device_addrs_t b100_find(const device_addr_t &hint)
{
    device_addrs_t b100_addrs;

    //return an empty list of addresses when type is set to non-b100
    if (hint.has_key("type") and hint["type"] != "b100") return b100_addrs;

    //Return an empty list of addresses when an address is specified,
    //since an address is intended for a different, non-USB, device.
    if (hint.has_key("addr")) return b100_addrs;

    unsigned int vid, pid;

    if(hint.has_key("vid") && hint.has_key("pid") && hint.has_key("type") && hint["type"] == "b100") {
        sscanf(hint.get("vid").c_str(), "%x", &vid);
        sscanf(hint.get("pid").c_str(), "%x", &pid);
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
    BOOST_FOREACH(usb_device_handle::sptr handle, usb_device_handle::get_device_list(vid, pid)) {
        //extract the firmware path for the b100
        std::string b100_fw_image;
        try{
            b100_fw_image = find_image_path(hint.get("fw", B100_FW_FILE_NAME));
        }
        catch(...){
            UHD_MSG(warning) << boost::format(
                "Could not locate B100 firmware.\n"
                "Please install the images package.\n"
            );
            return b100_addrs;
        }
        UHD_LOG << "the  firmware image: " << b100_fw_image << std::endl;

        usb_control::sptr control;
        try{control = usb_control::make(handle, 0);}
        catch(const uhd::exception &){continue;} //ignore claimed

        fx2_ctrl::make(control)->usrp_load_firmware(b100_fw_image);
    }

    //get descriptors again with serial number, but using the initialized VID/PID now since we have firmware
    vid = B100_VENDOR_ID;
    pid = B100_PRODUCT_ID;

    BOOST_FOREACH(usb_device_handle::sptr handle, usb_device_handle::get_device_list(vid, pid)) {
        usb_control::sptr control;
        try{control = usb_control::make(handle, 0);}
        catch(const uhd::exception &){continue;} //ignore claimed

        fx2_ctrl::sptr fx2_ctrl = fx2_ctrl::make(control);
        const mboard_eeprom_t mb_eeprom = mboard_eeprom_t(*fx2_ctrl, mboard_eeprom_t::MAP_B100);
        device_addr_t new_addr;
        new_addr["type"] = "b100";
        new_addr["name"] = mb_eeprom["name"];
        new_addr["serial"] = handle->get_serial();
        //this is a found b100 when the hint serial and name match or blank
        if (
            (not hint.has_key("name")   or hint["name"]   == new_addr["name"]) and
            (not hint.has_key("serial") or hint["serial"] == new_addr["serial"])
        ){
            b100_addrs.push_back(new_addr);
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
    device::register_device(&b100_find, &b100_make);
}

/***********************************************************************
 * Structors
 **********************************************************************/
b100_impl::b100_impl(const device_addr_t &device_addr){

    //extract the FPGA path for the B100
    std::string b100_fpga_image = find_image_path(
        device_addr.has_key("fpga")? device_addr["fpga"] : B100_FPGA_FILE_NAME
    );

    //try to match the given device address with something on the USB bus
    std::vector<usb_device_handle::sptr> device_list =
        usb_device_handle::get_device_list(B100_VENDOR_ID, B100_PRODUCT_ID);

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
    usb_control::sptr fx2_transport = usb_control::make(handle, 0);
    _fx2_ctrl = fx2_ctrl::make(fx2_transport);
    this->check_fw_compat(); //check after making fx2
    //-- setup clock after making fx2 and before loading fpga --//
    _clock_ctrl = b100_clock_ctrl::make(_fx2_ctrl, device_addr.cast<double>("master_clock_rate", B100_DEFAULT_TICK_RATE));
    _fx2_ctrl->usrp_load_fpga(b100_fpga_image);

    //create the control transport
    device_addr_t ctrl_xport_args;
    ctrl_xport_args["recv_frame_size"] = boost::lexical_cast<std::string>(CTRL_PACKET_LENGTH);
    ctrl_xport_args["num_recv_frames"] = "16";
    ctrl_xport_args["send_frame_size"] = boost::lexical_cast<std::string>(CTRL_PACKET_LENGTH);
    ctrl_xport_args["num_send_frames"] = "4";

    _ctrl_transport = usb_zero_copy::make(
        handle,
        4, 8, //interface, endpoint
        3, 4, //interface, endpoint
        ctrl_xport_args
    );

    ////////////////////////////////////////////////////////////////////
    // Create controller objects
    ////////////////////////////////////////////////////////////////////
    _fpga_ctrl = b100_ctrl::make(_ctrl_transport);
    this->enable_gpif(true); //TODO best place to put this?
    this->check_fpga_compat(); //check after making control

    ////////////////////////////////////////////////////////////////////
    // Reset buffers in data path
    ////////////////////////////////////////////////////////////////////
    _fpga_ctrl->poke32(B100_REG_GLOBAL_RESET, 0);
    _fpga_ctrl->poke32(B100_REG_CLEAR_RX, 0);
    _fpga_ctrl->poke32(B100_REG_CLEAR_TX, 0);
    this->reset_gpif(6);

    ////////////////////////////////////////////////////////////////////
    // Initialize peripherals after reset
    ////////////////////////////////////////////////////////////////////
    _fpga_i2c_ctrl = i2c_core_100::make(_fpga_ctrl, B100_REG_SLAVE(3));
    _fpga_spi_ctrl = spi_core_100::make(_fpga_ctrl, B100_REG_SLAVE(2));

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

    _data_transport = usb_zero_copy::make_wrapper(
        usb_zero_copy::make(
            handle,        // identifier
            2, 6,          // IN interface, endpoint
            1, 2,          // OUT interface, endpoint
            data_xport_args    // param hints
        )
    );

    ////////////////////////////////////////////////////////////////////
    // Initialize the properties tree
    ////////////////////////////////////////////////////////////////////
    _tree = property_tree::make();
    _tree->create<std::string>("/name").set("B-Series Device");
    const fs_path mb_path = "/mboards/0";
    _tree->create<std::string>(mb_path / "name").set("B100 (B-Hundo)");
    _tree->create<std::string>(mb_path / "load_eeprom")
        .subscribe(boost::bind(&fx2_ctrl::usrp_load_eeprom, _fx2_ctrl, _1));

    ////////////////////////////////////////////////////////////////////
    // setup the mboard eeprom
    ////////////////////////////////////////////////////////////////////
    const mboard_eeprom_t mb_eeprom(*_fx2_ctrl, mboard_eeprom_t::MAP_B100);
    _tree->create<mboard_eeprom_t>(mb_path / "eeprom")
        .set(mb_eeprom)
        .subscribe(boost::bind(&b100_impl::set_mb_eeprom, this, _1));

    ////////////////////////////////////////////////////////////////////
    // create clock control objects
    ////////////////////////////////////////////////////////////////////
    //^^^ clock created up top, just reg props here... ^^^
    _tree->create<double>(mb_path / "tick_rate")
        .publish(boost::bind(&b100_clock_ctrl::get_fpga_clock_rate, _clock_ctrl))
        .subscribe(boost::bind(&b100_impl::update_tick_rate, this, _1));

    ////////////////////////////////////////////////////////////////////
    // create codec control objects
    ////////////////////////////////////////////////////////////////////
    _codec_ctrl = b100_codec_ctrl::make(_fpga_spi_ctrl);
    const fs_path rx_codec_path = mb_path / "rx_codecs/A";
    const fs_path tx_codec_path = mb_path / "tx_codecs/A";
    _tree->create<std::string>(rx_codec_path / "name").set("ad9522");
    _tree->create<meta_range_t>(rx_codec_path / "gains/pga/range").set(b100_codec_ctrl::rx_pga_gain_range);
    _tree->create<double>(rx_codec_path / "gains/pga/value")
        .coerce(boost::bind(&b100_impl::update_rx_codec_gain, this, _1));
    _tree->create<std::string>(tx_codec_path / "name").set("ad9522");
    _tree->create<meta_range_t>(tx_codec_path / "gains/pga/range").set(b100_codec_ctrl::tx_pga_gain_range);
    _tree->create<double>(tx_codec_path / "gains/pga/value")
        .subscribe(boost::bind(&b100_codec_ctrl::set_tx_pga_gain, _codec_ctrl, _1))
        .publish(boost::bind(&b100_codec_ctrl::get_tx_pga_gain, _codec_ctrl));

    ////////////////////////////////////////////////////////////////////
    // and do the misc mboard sensors
    ////////////////////////////////////////////////////////////////////
    _tree->create<sensor_value_t>(mb_path / "sensors/ref_locked")
        .publish(boost::bind(&b100_impl::get_ref_locked, this));

    ////////////////////////////////////////////////////////////////////
    // create frontend control objects
    ////////////////////////////////////////////////////////////////////
    _rx_fe = rx_frontend_core_200::make(_fpga_ctrl, B100_REG_SR_ADDR(B100_SR_RX_FRONT));
    _tx_fe = tx_frontend_core_200::make(_fpga_ctrl, B100_REG_SR_ADDR(B100_SR_TX_FRONT));

    _tree->create<subdev_spec_t>(mb_path / "rx_subdev_spec")
        .subscribe(boost::bind(&b100_impl::update_rx_subdev_spec, this, _1));
    _tree->create<subdev_spec_t>(mb_path / "tx_subdev_spec")
        .subscribe(boost::bind(&b100_impl::update_tx_subdev_spec, this, _1));

    const fs_path rx_fe_path = mb_path / "rx_frontends" / "A";
    const fs_path tx_fe_path = mb_path / "tx_frontends" / "A";

    _tree->create<std::complex<double> >(rx_fe_path / "dc_offset" / "value")
        .coerce(boost::bind(&rx_frontend_core_200::set_dc_offset, _rx_fe, _1))
        .set(std::complex<double>(0.0, 0.0));
    _tree->create<bool>(rx_fe_path / "dc_offset" / "enable")
        .subscribe(boost::bind(&rx_frontend_core_200::set_dc_offset_auto, _rx_fe, _1))
        .set(true);
    _tree->create<std::complex<double> >(rx_fe_path / "iq_balance" / "value")
        .subscribe(boost::bind(&rx_frontend_core_200::set_iq_balance, _rx_fe, _1))
        .set(std::complex<double>(0.0, 0.0));
    _tree->create<std::complex<double> >(tx_fe_path / "dc_offset" / "value")
        .coerce(boost::bind(&tx_frontend_core_200::set_dc_offset, _tx_fe, _1))
        .set(std::complex<double>(0.0, 0.0));
    _tree->create<std::complex<double> >(tx_fe_path / "iq_balance" / "value")
        .subscribe(boost::bind(&tx_frontend_core_200::set_iq_balance, _tx_fe, _1))
        .set(std::complex<double>(0.0, 0.0));

    ////////////////////////////////////////////////////////////////////
    // create rx dsp control objects
    ////////////////////////////////////////////////////////////////////
    _rx_dsps.push_back(rx_dsp_core_200::make(
        _fpga_ctrl, B100_REG_SR_ADDR(B100_SR_RX_DSP0), B100_REG_SR_ADDR(B100_SR_RX_CTRL0), B100_RX_SID_BASE + 0
    ));
    _rx_dsps.push_back(rx_dsp_core_200::make(
        _fpga_ctrl, B100_REG_SR_ADDR(B100_SR_RX_DSP1), B100_REG_SR_ADDR(B100_SR_RX_CTRL1), B100_RX_SID_BASE + 1
    ));
    for (size_t dspno = 0; dspno < _rx_dsps.size(); dspno++){
        _rx_dsps[dspno]->set_link_rate(B100_LINK_RATE_BPS);
        _tree->access<double>(mb_path / "tick_rate")
            .subscribe(boost::bind(&rx_dsp_core_200::set_tick_rate, _rx_dsps[dspno], _1));
        fs_path rx_dsp_path = mb_path / str(boost::format("rx_dsps/%u") % dspno);
        _tree->create<meta_range_t>(rx_dsp_path / "rate/range")
            .publish(boost::bind(&rx_dsp_core_200::get_host_rates, _rx_dsps[dspno]));
        _tree->create<double>(rx_dsp_path / "rate/value")
            .set(1e6) //some default
            .coerce(boost::bind(&rx_dsp_core_200::set_host_rate, _rx_dsps[dspno], _1))
            .subscribe(boost::bind(&b100_impl::update_rx_samp_rate, this, dspno, _1));
        _tree->create<double>(rx_dsp_path / "freq/value")
            .coerce(boost::bind(&rx_dsp_core_200::set_freq, _rx_dsps[dspno], _1));
        _tree->create<meta_range_t>(rx_dsp_path / "freq/range")
            .publish(boost::bind(&rx_dsp_core_200::get_freq_range, _rx_dsps[dspno]));
        _tree->create<stream_cmd_t>(rx_dsp_path / "stream_cmd")
            .subscribe(boost::bind(&rx_dsp_core_200::issue_stream_command, _rx_dsps[dspno], _1));
    }

    ////////////////////////////////////////////////////////////////////
    // create tx dsp control objects
    ////////////////////////////////////////////////////////////////////
    _tx_dsp = tx_dsp_core_200::make(
        _fpga_ctrl, B100_REG_SR_ADDR(B100_SR_TX_DSP), B100_REG_SR_ADDR(B100_SR_TX_CTRL), B100_TX_ASYNC_SID
    );
    _tx_dsp->set_link_rate(B100_LINK_RATE_BPS);
    _tree->access<double>(mb_path / "tick_rate")
        .subscribe(boost::bind(&tx_dsp_core_200::set_tick_rate, _tx_dsp, _1));
    _tree->create<meta_range_t>(mb_path / "tx_dsps/0/rate/range")
        .publish(boost::bind(&tx_dsp_core_200::get_host_rates, _tx_dsp));
    _tree->create<double>(mb_path / "tx_dsps/0/rate/value")
        .set(1e6) //some default
        .coerce(boost::bind(&tx_dsp_core_200::set_host_rate, _tx_dsp, _1))
        .subscribe(boost::bind(&b100_impl::update_tx_samp_rate, this, 0, _1));
    _tree->create<double>(mb_path / "tx_dsps/0/freq/value")
        .coerce(boost::bind(&tx_dsp_core_200::set_freq, _tx_dsp, _1));
    _tree->create<meta_range_t>(mb_path / "tx_dsps/0/freq/range")
        .publish(boost::bind(&tx_dsp_core_200::get_freq_range, _tx_dsp));

    ////////////////////////////////////////////////////////////////////
    // create time control objects
    ////////////////////////////////////////////////////////////////////
    time64_core_200::readback_bases_type time64_rb_bases;
    time64_rb_bases.rb_secs_now = B100_REG_RB_TIME_NOW_SECS;
    time64_rb_bases.rb_ticks_now = B100_REG_RB_TIME_NOW_TICKS;
    time64_rb_bases.rb_secs_pps = B100_REG_RB_TIME_PPS_SECS;
    time64_rb_bases.rb_ticks_pps = B100_REG_RB_TIME_PPS_TICKS;
    _time64 = time64_core_200::make(
        _fpga_ctrl, B100_REG_SR_ADDR(B100_SR_TIME64), time64_rb_bases
    );
    _tree->access<double>(mb_path / "tick_rate")
        .subscribe(boost::bind(&time64_core_200::set_tick_rate, _time64, _1));
    _tree->create<time_spec_t>(mb_path / "time/now")
        .publish(boost::bind(&time64_core_200::get_time_now, _time64))
        .subscribe(boost::bind(&time64_core_200::set_time_now, _time64, _1));
    _tree->create<time_spec_t>(mb_path / "time/pps")
        .publish(boost::bind(&time64_core_200::get_time_last_pps, _time64))
        .subscribe(boost::bind(&time64_core_200::set_time_next_pps, _time64, _1));
    //setup time source props
    _tree->create<std::string>(mb_path / "time_source/value")
        .subscribe(boost::bind(&time64_core_200::set_time_source, _time64, _1));
    _tree->create<std::vector<std::string> >(mb_path / "time_source/options")
        .publish(boost::bind(&time64_core_200::get_time_sources, _time64));
    //setup reference source props
    _tree->create<std::string>(mb_path / "clock_source/value")
        .subscribe(boost::bind(&b100_impl::update_clock_source, this, _1));
    static const std::vector<std::string> clock_sources = boost::assign::list_of("internal")("external")("auto");
    _tree->create<std::vector<std::string> >(mb_path / "clock_source/options").set(clock_sources);

    ////////////////////////////////////////////////////////////////////
    // create dboard control objects
    ////////////////////////////////////////////////////////////////////

    //read the dboard eeprom to extract the dboard ids
    dboard_eeprom_t rx_db_eeprom, tx_db_eeprom, gdb_eeprom;
    rx_db_eeprom.load(*_fpga_i2c_ctrl, I2C_ADDR_RX_A);
    tx_db_eeprom.load(*_fpga_i2c_ctrl, I2C_ADDR_TX_A);
    gdb_eeprom.load(*_fpga_i2c_ctrl, I2C_ADDR_TX_A ^ 5);

    //create the properties and register subscribers
    _tree->create<dboard_eeprom_t>(mb_path / "dboards/A/rx_eeprom")
        .set(rx_db_eeprom)
        .subscribe(boost::bind(&b100_impl::set_db_eeprom, this, "rx", _1));
    _tree->create<dboard_eeprom_t>(mb_path / "dboards/A/tx_eeprom")
        .set(tx_db_eeprom)
        .subscribe(boost::bind(&b100_impl::set_db_eeprom, this, "tx", _1));
    _tree->create<dboard_eeprom_t>(mb_path / "dboards/A/gdb_eeprom")
        .set(gdb_eeprom)
        .subscribe(boost::bind(&b100_impl::set_db_eeprom, this, "gdb", _1));

    //create a new dboard interface and manager
    _dboard_iface = make_b100_dboard_iface(_fpga_ctrl, _fpga_i2c_ctrl, _fpga_spi_ctrl, _clock_ctrl, _codec_ctrl);
    _tree->create<dboard_iface::sptr>(mb_path / "dboards/A/iface").set(_dboard_iface);
    _dboard_manager = dboard_manager::make(
        rx_db_eeprom.id,
        ((gdb_eeprom.id == dboard_id_t::none())? tx_db_eeprom : gdb_eeprom).id,
        _dboard_iface
    );
    BOOST_FOREACH(const std::string &name, _dboard_manager->get_rx_subdev_names()){
        dboard_manager::populate_prop_tree_from_subdev(
            _tree->subtree(mb_path / "dboards/A/rx_frontends" / name),
            _dboard_manager->get_rx_subdev(name)
        );
    }
    BOOST_FOREACH(const std::string &name, _dboard_manager->get_tx_subdev_names()){
        dboard_manager::populate_prop_tree_from_subdev(
            _tree->subtree(mb_path / "dboards/A/tx_frontends" / name),
            _dboard_manager->get_tx_subdev(name)
        );
    }

    //initialize io handling
    this->io_init();

    ////////////////////////////////////////////////////////////////////
    // do some post-init tasks
    ////////////////////////////////////////////////////////////////////
    this->update_rates();

    _tree->access<double>(mb_path / "tick_rate") //now subscribe the clock rate setter
        .subscribe(boost::bind(&b100_clock_ctrl::set_fpga_clock_rate, _clock_ctrl, _1));

    _tree->access<subdev_spec_t>(mb_path / "rx_subdev_spec").set(subdev_spec_t("A:"+_dboard_manager->get_rx_subdev_names()[0]));
    _tree->access<subdev_spec_t>(mb_path / "tx_subdev_spec").set(subdev_spec_t("A:"+_dboard_manager->get_tx_subdev_names()[0]));
    _tree->access<std::string>(mb_path / "clock_source/value").set("internal");
    _tree->access<std::string>(mb_path / "time_source/value").set("none");
}

b100_impl::~b100_impl(void){
    //set an empty async callback now that we deconstruct
    _fpga_ctrl->set_async_cb(b100_ctrl::async_cb_type());
}

void b100_impl::check_fw_compat(void){
    unsigned char data[4]; //useless data buffer
    const boost::uint16_t fw_compat_num = _fx2_ctrl->usrp_control_read(
        VRQ_FW_COMPAT, 0, 0, data, sizeof(data)
    );
    if (fw_compat_num != B100_FW_COMPAT_NUM){
        throw uhd::runtime_error(str(boost::format(
            "Expected firmware compatibility number 0x%x, but got 0x%x:\n"
            "The firmware build is not compatible with the host code build."
        ) % B100_FW_COMPAT_NUM % fw_compat_num));
    }
}

void b100_impl::check_fpga_compat(void){
    const boost::uint16_t fpga_compat_num = _fpga_ctrl->peek16(B100_REG_MISC_COMPAT);
    if (fpga_compat_num != B100_FPGA_COMPAT_NUM){
        throw uhd::runtime_error(str(boost::format(
            "Expected FPGA compatibility number 0x%x, but got 0x%x:\n"
            "The FPGA build is not compatible with the host code build."
        ) % B100_FPGA_COMPAT_NUM % fpga_compat_num));
    }
}

double b100_impl::update_rx_codec_gain(const double gain){
    //set gain on both I and Q, readback on one
    //TODO in the future, gains should have individual control
    _codec_ctrl->set_rx_pga_gain(gain, 'A');
    _codec_ctrl->set_rx_pga_gain(gain, 'B');
    return _codec_ctrl->get_rx_pga_gain('A');
}

void b100_impl::set_mb_eeprom(const uhd::usrp::mboard_eeprom_t &mb_eeprom){
    mb_eeprom.commit(*_fx2_ctrl, mboard_eeprom_t::MAP_B100);
}

void b100_impl::set_db_eeprom(const std::string &type, const uhd::usrp::dboard_eeprom_t &db_eeprom){
    if (type == "rx") db_eeprom.store(*_fpga_i2c_ctrl, I2C_ADDR_RX_A);
    if (type == "tx") db_eeprom.store(*_fpga_i2c_ctrl, I2C_ADDR_TX_A);
    if (type == "gdb") db_eeprom.store(*_fpga_i2c_ctrl, I2C_ADDR_TX_A ^ 5);
}

void b100_impl::update_clock_source(const std::string &source){
    if      (source == "auto")     _clock_ctrl->use_auto_ref();
    else if (source == "internal") _clock_ctrl->use_internal_ref();
    else if (source == "external") _clock_ctrl->use_external_ref();
    else throw uhd::runtime_error("unhandled clock configuration reference source: " + source);
}

////////////////// some GPIF preparation related stuff /////////////////
void b100_impl::reset_gpif(const boost::uint16_t ep) {
    _fx2_ctrl->usrp_control_write(VRQ_RESET_GPIF, ep, ep, 0, 0);
}

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

//
// Copyright 2010-2012 Ettus Research LLC
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

#include "usrp1_impl.hpp"
#include <uhd/utils/log.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/transport/usb_control.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/images.hpp>
#include <boost/format.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/math/special_functions/round.hpp>
#include <cstdio>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

const boost::uint16_t USRP1_VENDOR_ID  = 0xfffe;
const boost::uint16_t USRP1_PRODUCT_ID = 0x0002;
const boost::uint16_t FX2_VENDOR_ID    = 0x04b4;
const boost::uint16_t FX2_PRODUCT_ID   = 0x8613;
static const boost::posix_time::milliseconds REENUMERATION_TIMEOUT_MS(3000);

const std::vector<usrp1_impl::dboard_slot_t> usrp1_impl::_dboard_slots = boost::assign::list_of
    (usrp1_impl::DBOARD_SLOT_A)(usrp1_impl::DBOARD_SLOT_B)
;

/***********************************************************************
 * Discovery
 **********************************************************************/
static device_addrs_t usrp1_find(const device_addr_t &hint)
{
    device_addrs_t usrp1_addrs;

    //return an empty list of addresses when type is set to non-usrp1
    if (hint.has_key("type") and hint["type"] != "usrp1") return usrp1_addrs;

    //Return an empty list of addresses when an address is specified,
    //since an address is intended for a different, non-USB, device.
    if (hint.has_key("addr")) return usrp1_addrs;

    unsigned int vid, pid;

    if(hint.has_key("vid") && hint.has_key("pid") && hint.has_key("type") && hint["type"] == "usrp1") {
        sscanf(hint.get("vid").c_str(), "%x", &vid);
        sscanf(hint.get("pid").c_str(), "%x", &pid);
    } else {
        vid = USRP1_VENDOR_ID;
        pid = USRP1_PRODUCT_ID;
    }

    // Important note:
    // The get device list calls are nested inside the for loop.
    // This allows the usb guts to decontruct when not in use,
    // so that re-enumeration after fw load can occur successfully.
    // This requirement is a courtesy of libusb1.0 on windows.

    //find the usrps and load firmware
    size_t found = 0;
    BOOST_FOREACH(usb_device_handle::sptr handle, usb_device_handle::get_device_list(vid, pid)) {
        //extract the firmware path for the USRP1
        std::string usrp1_fw_image;
        try{
            usrp1_fw_image = find_image_path(hint.get("fw", "usrp1_fw.ihx"));
        }
        catch(...){
            UHD_MSG(warning) << boost::format("Could not locate USRP1 firmware. %s") % print_images_error();
        }
        UHD_LOG << "USRP1 firmware image: " << usrp1_fw_image << std::endl;

        usb_control::sptr control;
        try{control = usb_control::make(handle, 0);}
        catch(const uhd::exception &){continue;} //ignore claimed

        fx2_ctrl::make(control)->usrp_load_firmware(usrp1_fw_image);
        found++;
    }

    //get descriptors again with serial number, but using the initialized VID/PID now since we have firmware
    vid = USRP1_VENDOR_ID;
    pid = USRP1_PRODUCT_ID;

    const boost::system_time timeout_time = boost::get_system_time() + REENUMERATION_TIMEOUT_MS;

    //search for the device until found or timeout
    while (boost::get_system_time() < timeout_time and usrp1_addrs.empty() and found != 0)
    {
        BOOST_FOREACH(usb_device_handle::sptr handle, usb_device_handle::get_device_list(vid, pid))
        {
            usb_control::sptr control;
            try{control = usb_control::make(handle, 0);}
            catch(const uhd::exception &){continue;} //ignore claimed

            fx2_ctrl::sptr fx2_ctrl = fx2_ctrl::make(control);
            const mboard_eeprom_t mb_eeprom(*fx2_ctrl, USRP1_EEPROM_MAP_KEY);
            device_addr_t new_addr;
            new_addr["type"] = "usrp1";
            new_addr["name"] = mb_eeprom["name"];
            new_addr["serial"] = handle->get_serial();
            //this is a found usrp1 when the hint serial and name match or blank
            if (
                (not hint.has_key("name")   or hint["name"]   == new_addr["name"]) and
                (not hint.has_key("serial") or hint["serial"] == new_addr["serial"])
            ){
                usrp1_addrs.push_back(new_addr);
            }
        }
    }

    return usrp1_addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
static device::sptr usrp1_make(const device_addr_t &device_addr){
    return device::sptr(new usrp1_impl(device_addr));
}

UHD_STATIC_BLOCK(register_usrp1_device){
    device::register_device(&usrp1_find, &usrp1_make);
}

/***********************************************************************
 * Structors
 **********************************************************************/
usrp1_impl::usrp1_impl(const device_addr_t &device_addr){
    UHD_MSG(status) << "Opening a USRP1 device..." << std::endl;

    //extract the FPGA path for the USRP1
    std::string usrp1_fpga_image = find_image_path(
        device_addr.get("fpga", "usrp1_fpga.rbf")
    );
    UHD_LOG << "USRP1 FPGA image: " << usrp1_fpga_image << std::endl;

    //try to match the given device address with something on the USB bus
    std::vector<usb_device_handle::sptr> device_list =
        usb_device_handle::get_device_list(USRP1_VENDOR_ID, USRP1_PRODUCT_ID);

    //locate the matching handle in the device list
    usb_device_handle::sptr handle;
    BOOST_FOREACH(usb_device_handle::sptr dev_handle, device_list) {
        if (dev_handle->get_serial() == device_addr["serial"]){
            handle = dev_handle;
            break;
        }
    }
    UHD_ASSERT_THROW(handle.get() != NULL); //better be found

    ////////////////////////////////////////////////////////////////////
    // Create controller objects
    ////////////////////////////////////////////////////////////////////
    //usb_control::sptr usb_ctrl = usb_control::make(handle);
    _fx2_ctrl = fx2_ctrl::make(usb_control::make(handle, 0));
    _fx2_ctrl->usrp_load_fpga(usrp1_fpga_image);
    _fx2_ctrl->usrp_init();
    _data_transport = usb_zero_copy::make(
        handle,        // identifier
        2, 6,          // IN interface, endpoint
        1, 2,          // OUT interface, endpoint
        device_addr    // param hints
    );
    _iface = usrp1_iface::make(_fx2_ctrl);
    _soft_time_ctrl = soft_time_ctrl::make(
        boost::bind(&usrp1_impl::rx_stream_on_off, this, _1)
    );
    _dbc["A"]; _dbc["B"]; //ensure that keys exist

    // Normal mode with no loopback or Rx counting
    _iface->poke32(FR_MODE, 0x00000000);
    _iface->poke32(FR_DEBUG_EN, 0x00000000);

    UHD_LOG
        << "USRP1 Capabilities" << std::endl
        << "    number of duc's: " << get_num_ddcs() << std::endl
        << "    number of ddc's: " << get_num_ducs() << std::endl
        << "    rx halfband:     " << has_rx_halfband() << std::endl
        << "    tx halfband:     " << has_tx_halfband() << std::endl
    ;

    ////////////////////////////////////////////////////////////////////
    // Initialize the properties tree
    ////////////////////////////////////////////////////////////////////
    _rx_dc_offset_shadow = 0;
    _tree = property_tree::make();
    _tree->create<std::string>("/name").set("USRP1 Device");
    const fs_path mb_path = "/mboards/0";
    _tree->create<std::string>(mb_path / "name").set("USRP1");
    _tree->create<std::string>(mb_path / "load_eeprom")
        .subscribe(boost::bind(&fx2_ctrl::usrp_load_eeprom, _fx2_ctrl, _1));

    ////////////////////////////////////////////////////////////////////
    // create user-defined control objects
    ////////////////////////////////////////////////////////////////////
    _tree->create<std::pair<boost::uint8_t, boost::uint32_t> >(mb_path / "user" / "regs")
        .subscribe(boost::bind(&usrp1_impl::set_reg, this, _1));

    ////////////////////////////////////////////////////////////////////
    // setup the mboard eeprom
    ////////////////////////////////////////////////////////////////////
    const mboard_eeprom_t mb_eeprom(*_fx2_ctrl, USRP1_EEPROM_MAP_KEY);
    _tree->create<mboard_eeprom_t>(mb_path / "eeprom")
        .set(mb_eeprom)
        .subscribe(boost::bind(&usrp1_impl::set_mb_eeprom, this, _1));

    ////////////////////////////////////////////////////////////////////
    // create clock control objects
    ////////////////////////////////////////////////////////////////////
    _master_clock_rate = 64e6;
    if (device_addr.has_key("mcr")){
        try{
            _master_clock_rate = boost::lexical_cast<double>(device_addr["mcr"]);
        }
        catch(const std::exception &e){
            UHD_MSG(error) << "Error parsing FPGA clock rate from device address: " << e.what() << std::endl;
        }
    }
    else if (not mb_eeprom["mcr"].empty()){
        try{
            _master_clock_rate = boost::lexical_cast<double>(mb_eeprom["mcr"]);
        }
        catch(const std::exception &e){
            UHD_MSG(error) << "Error parsing FPGA clock rate from EEPROM: " << e.what() << std::endl;
        }
    }
    UHD_MSG(status) << boost::format("Using FPGA clock rate of %fMHz...") % (_master_clock_rate/1e6) << std::endl;
    _tree->create<double>(mb_path / "tick_rate")
        .subscribe(boost::bind(&usrp1_impl::update_tick_rate, this, _1))
        .set(_master_clock_rate);

    ////////////////////////////////////////////////////////////////////
    // create codec control objects
    ////////////////////////////////////////////////////////////////////
    BOOST_FOREACH(const std::string &db, _dbc.keys()){
        _dbc[db].codec = usrp1_codec_ctrl::make(_iface, (db == "A")? SPI_ENABLE_CODEC_A : SPI_ENABLE_CODEC_B);
        const fs_path rx_codec_path = mb_path / "rx_codecs" / db;
        const fs_path tx_codec_path = mb_path / "tx_codecs" / db;
        _tree->create<std::string>(rx_codec_path / "name").set("ad9522");
        _tree->create<meta_range_t>(rx_codec_path / "gains/pga/range").set(usrp1_codec_ctrl::rx_pga_gain_range);
        _tree->create<double>(rx_codec_path / "gains/pga/value")
            .coerce(boost::bind(&usrp1_impl::update_rx_codec_gain, this, db, _1));
        _tree->create<std::string>(tx_codec_path / "name").set("ad9522");
        _tree->create<meta_range_t>(tx_codec_path / "gains/pga/range").set(usrp1_codec_ctrl::tx_pga_gain_range);
        _tree->create<double>(tx_codec_path / "gains/pga/value")
            .subscribe(boost::bind(&usrp1_codec_ctrl::set_tx_pga_gain, _dbc[db].codec, _1))
            .publish(boost::bind(&usrp1_codec_ctrl::get_tx_pga_gain, _dbc[db].codec));
    }

    ////////////////////////////////////////////////////////////////////
    // and do the misc mboard sensors
    ////////////////////////////////////////////////////////////////////
    //none for now...
    _tree->create<int>(mb_path / "sensors"); //phony property so this dir exists

    ////////////////////////////////////////////////////////////////////
    // create frontend control objects
    ////////////////////////////////////////////////////////////////////
    _tree->create<subdev_spec_t>(mb_path / "rx_subdev_spec")
        .set(subdev_spec_t())
        .subscribe(boost::bind(&usrp1_impl::update_rx_subdev_spec, this, _1));
    _tree->create<subdev_spec_t>(mb_path / "tx_subdev_spec")
        .set(subdev_spec_t())
        .subscribe(boost::bind(&usrp1_impl::update_tx_subdev_spec, this, _1));

    BOOST_FOREACH(const std::string &db, _dbc.keys()){
        const fs_path rx_fe_path = mb_path / "rx_frontends" / db;
        _tree->create<std::complex<double> >(rx_fe_path / "dc_offset" / "value")
            .coerce(boost::bind(&usrp1_impl::set_rx_dc_offset, this, db, _1))
            .set(std::complex<double>(0.0, 0.0));
        _tree->create<bool>(rx_fe_path / "dc_offset" / "enable")
            .subscribe(boost::bind(&usrp1_impl::set_enb_rx_dc_offset, this, db, _1))
            .set(true);
    }

    ////////////////////////////////////////////////////////////////////
    // create rx dsp control objects
    ////////////////////////////////////////////////////////////////////
    _tree->create<int>(mb_path / "rx_dsps"); //dummy in case we have none
    for (size_t dspno = 0; dspno < get_num_ddcs(); dspno++){
        fs_path rx_dsp_path = mb_path / str(boost::format("rx_dsps/%u") % dspno);
        _tree->create<meta_range_t>(rx_dsp_path / "rate/range")
            .publish(boost::bind(&usrp1_impl::get_rx_dsp_host_rates, this));
        _tree->create<double>(rx_dsp_path / "rate/value")
            .set(1e6) //some default rate
            .coerce(boost::bind(&usrp1_impl::update_rx_samp_rate, this, dspno, _1));
        _tree->create<double>(rx_dsp_path / "freq/value")
            .coerce(boost::bind(&usrp1_impl::update_rx_dsp_freq, this, dspno, _1));
        _tree->create<meta_range_t>(rx_dsp_path / "freq/range")
            .publish(boost::bind(&usrp1_impl::get_rx_dsp_freq_range, this));
        _tree->create<stream_cmd_t>(rx_dsp_path / "stream_cmd");
        if (dspno == 0){
            //only subscribe the callback for dspno 0 since it will stream all dsps
            _tree->access<stream_cmd_t>(rx_dsp_path / "stream_cmd")
                .subscribe(boost::bind(&soft_time_ctrl::issue_stream_cmd, _soft_time_ctrl, _1));
        }
    }

    ////////////////////////////////////////////////////////////////////
    // create tx dsp control objects
    ////////////////////////////////////////////////////////////////////
    _tree->create<int>(mb_path / "tx_dsps"); //dummy in case we have none
    for (size_t dspno = 0; dspno < get_num_ducs(); dspno++){
        fs_path tx_dsp_path = mb_path / str(boost::format("tx_dsps/%u") % dspno);
        _tree->create<meta_range_t>(tx_dsp_path / "rate/range")
            .publish(boost::bind(&usrp1_impl::get_tx_dsp_host_rates, this));
        _tree->create<double>(tx_dsp_path / "rate/value")
            .set(1e6) //some default rate
            .coerce(boost::bind(&usrp1_impl::update_tx_samp_rate, this, dspno, _1));
        _tree->create<double>(tx_dsp_path / "freq/value")
            .coerce(boost::bind(&usrp1_impl::update_tx_dsp_freq, this, dspno, _1));
        _tree->create<meta_range_t>(tx_dsp_path / "freq/range")
            .publish(boost::bind(&usrp1_impl::get_tx_dsp_freq_range, this));
    }

    ////////////////////////////////////////////////////////////////////
    // create time control objects
    ////////////////////////////////////////////////////////////////////
    _tree->create<time_spec_t>(mb_path / "time/now")
        .publish(boost::bind(&soft_time_ctrl::get_time, _soft_time_ctrl))
        .subscribe(boost::bind(&soft_time_ctrl::set_time, _soft_time_ctrl, _1));

    _tree->create<std::vector<std::string> >(mb_path / "clock_source/options").set(std::vector<std::string>(1, "internal"));
    _tree->create<std::vector<std::string> >(mb_path / "time_source/options").set(std::vector<std::string>(1, "none"));
    _tree->create<std::string>(mb_path / "clock_source/value").set("internal");
    _tree->create<std::string>(mb_path / "time_source/value").set("none");

    ////////////////////////////////////////////////////////////////////
    // create dboard control objects
    ////////////////////////////////////////////////////////////////////
    BOOST_FOREACH(const std::string &db, _dbc.keys()){

        //read the dboard eeprom to extract the dboard ids
        dboard_eeprom_t rx_db_eeprom, tx_db_eeprom, gdb_eeprom;
        rx_db_eeprom.load(*_fx2_ctrl, (db == "A")? (I2C_ADDR_RX_A) : (I2C_ADDR_RX_B));
        tx_db_eeprom.load(*_fx2_ctrl, (db == "A")? (I2C_ADDR_TX_A) : (I2C_ADDR_TX_B));
        gdb_eeprom.load(*_fx2_ctrl, (db == "A")? (I2C_ADDR_TX_A ^ 5) : (I2C_ADDR_TX_B ^ 5));

        //disable rx dc offset if LFRX
        if (rx_db_eeprom.id == 0x000f) _tree->access<bool>(mb_path / "rx_frontends" / db / "dc_offset" / "enable").set(false);

        //create the properties and register subscribers
        _tree->create<dboard_eeprom_t>(mb_path / "dboards" / db/ "rx_eeprom")
            .set(rx_db_eeprom)
            .subscribe(boost::bind(&usrp1_impl::set_db_eeprom, this, db, "rx", _1));
        _tree->create<dboard_eeprom_t>(mb_path / "dboards" / db/ "tx_eeprom")
            .set(tx_db_eeprom)
            .subscribe(boost::bind(&usrp1_impl::set_db_eeprom, this, db, "tx", _1));
        _tree->create<dboard_eeprom_t>(mb_path / "dboards" / db/ "gdb_eeprom")
            .set(gdb_eeprom)
            .subscribe(boost::bind(&usrp1_impl::set_db_eeprom, this, db, "gdb", _1));

        //create a new dboard interface and manager
        _dbc[db].dboard_iface = make_dboard_iface(
            _iface, _dbc[db].codec,
            (db == "A")? DBOARD_SLOT_A : DBOARD_SLOT_B,
            _master_clock_rate, rx_db_eeprom.id
        );
        _tree->create<dboard_iface::sptr>(mb_path / "dboards" / db/ "iface").set(_dbc[db].dboard_iface);
        _dbc[db].dboard_manager = dboard_manager::make(
            rx_db_eeprom.id, tx_db_eeprom.id, gdb_eeprom.id,
            _dbc[db].dboard_iface, _tree->subtree(mb_path / "dboards" / db)
        );

        //init the subdev specs if we have a dboard (wont leave this loop empty)
        if (rx_db_eeprom.id != dboard_id_t::none() or _rx_subdev_spec.empty()){
            _rx_subdev_spec = subdev_spec_t(db + ":" + _tree->list(mb_path / "dboards" / db / "rx_frontends").at(0));
        }
        if (tx_db_eeprom.id != dboard_id_t::none() or _tx_subdev_spec.empty()){
            _tx_subdev_spec = subdev_spec_t(db + ":" + _tree->list(mb_path / "dboards" / db / "tx_frontends").at(0));
        }
    }

    //initialize io handling
    this->io_init();

    ////////////////////////////////////////////////////////////////////
    // do some post-init tasks
    ////////////////////////////////////////////////////////////////////
    this->update_rates();

    //reset cordic rates and their properties to zero
    BOOST_FOREACH(const std::string &name, _tree->list(mb_path / "rx_dsps")){
        _tree->access<double>(mb_path / "rx_dsps" / name / "freq" / "value").set(0.0);
    }

    if (_tree->list(mb_path / "rx_dsps").size() > 0)
        _tree->access<subdev_spec_t>(mb_path / "rx_subdev_spec").set(_rx_subdev_spec);
    if (_tree->list(mb_path / "tx_dsps").size() > 0)
        _tree->access<subdev_spec_t>(mb_path / "tx_subdev_spec").set(_tx_subdev_spec);
 
}

usrp1_impl::~usrp1_impl(void){
    UHD_SAFE_CALL(
        this->enable_rx(false);
        this->enable_tx(false);
    )
    _soft_time_ctrl->stop(); //stops cmd task before proceeding
    _io_impl.reset(); //stops vandal before other stuff gets deconstructed
}

/*!
 * Capabilities Register
 *
 *    3                   2                   1                   0
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-----------------------------------------------+-+-----+-+-----+
 * |               Reserved                        |T|DUCs |R|DDCs |
 * +-----------------------------------------------+-+-----+-+-----+
 */
size_t usrp1_impl::get_num_ddcs(void){
    boost::uint32_t regval = _iface->peek32(FR_RB_CAPS);
    return (regval >> 0) & 0x0007;
}

size_t usrp1_impl::get_num_ducs(void){
    boost::uint32_t regval = _iface->peek32(FR_RB_CAPS);
    return (regval >> 4) & 0x0007;
}

bool usrp1_impl::has_rx_halfband(void){
    boost::uint32_t regval = _iface->peek32(FR_RB_CAPS);
    return (regval >> 3) & 0x0001;
}

bool usrp1_impl::has_tx_halfband(void){
    boost::uint32_t regval = _iface->peek32(FR_RB_CAPS);
    return (regval >> 7) & 0x0001;
}

/***********************************************************************
 * Properties callback methods below
 **********************************************************************/
void usrp1_impl::set_mb_eeprom(const uhd::usrp::mboard_eeprom_t &mb_eeprom){
    mb_eeprom.commit(*_fx2_ctrl, USRP1_EEPROM_MAP_KEY);
}

void usrp1_impl::set_db_eeprom(const std::string &db, const std::string &type, const uhd::usrp::dboard_eeprom_t &db_eeprom){
    if (type == "rx") db_eeprom.store(*_fx2_ctrl, (db == "A")? (I2C_ADDR_RX_A) : (I2C_ADDR_RX_B));
    if (type == "tx") db_eeprom.store(*_fx2_ctrl, (db == "A")? (I2C_ADDR_TX_A) : (I2C_ADDR_TX_B));
    if (type == "gdb") db_eeprom.store(*_fx2_ctrl, (db == "A")? (I2C_ADDR_TX_A ^ 5) : (I2C_ADDR_TX_B ^ 5));
}

double usrp1_impl::update_rx_codec_gain(const std::string &db, const double gain){
    //set gain on both I and Q, readback on one
    //TODO in the future, gains should have individual control
    _dbc[db].codec->set_rx_pga_gain(gain, 'A');
    _dbc[db].codec->set_rx_pga_gain(gain, 'B');
    return _dbc[db].codec->get_rx_pga_gain('A');
}

uhd::meta_range_t usrp1_impl::get_rx_dsp_freq_range(void){
    return meta_range_t(-_master_clock_rate/2, +_master_clock_rate/2);
}

uhd::meta_range_t usrp1_impl::get_tx_dsp_freq_range(void){
    //magic scalar comes from codec control:
    return meta_range_t(-_master_clock_rate*0.6875, +_master_clock_rate*0.6875);
}

void usrp1_impl::set_enb_rx_dc_offset(const std::string &db, const bool enb){
    const size_t shift = (db == "A")? 0 : 2;
    _rx_dc_offset_shadow &= ~(0x3 << shift); //clear bits
    _rx_dc_offset_shadow |= ((enb)? 0x3 : 0x0) << shift;
    _iface->poke32(FR_DC_OFFSET_CL_EN, _rx_dc_offset_shadow & 0xf);
}

std::complex<double> usrp1_impl::set_rx_dc_offset(const std::string &db, const std::complex<double> &offset){
    const boost::int32_t i_off = boost::math::iround(offset.real() * (1ul << 31));
    const boost::int32_t q_off = boost::math::iround(offset.imag() * (1ul << 31));

    if (db == "A"){
        _iface->poke32(FR_ADC_OFFSET_0, i_off);
        _iface->poke32(FR_ADC_OFFSET_1, q_off);
    }

    if (db == "B"){
        _iface->poke32(FR_ADC_OFFSET_2, i_off);
        _iface->poke32(FR_ADC_OFFSET_3, q_off);
    }

    return std::complex<double>(double(i_off) * (1ul << 31), double(q_off) * (1ul << 31));
}

void usrp1_impl::set_reg(const std::pair<boost::uint8_t, boost::uint32_t> &reg)
{
    _iface->poke32(reg.first, reg.second);
}

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

#include "apply_corrections.hpp"
#include "e100_impl.hpp"
#include "e100_regs.hpp"
#include <uhd/utils/msg.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/images.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <boost/assign/list_of.hpp>
#include <fstream>
#include <ctime>

using namespace uhd;
using namespace uhd::usrp;
namespace fs = boost::filesystem;

////////////////////////////////////////////////////////////////////////
// I2C addresses
////////////////////////////////////////////////////////////////////////
#define I2C_DEV_EEPROM  0x50 // 24LC02[45]:  7-bits 1010xxx
#define	I2C_ADDR_MBOARD (I2C_DEV_EEPROM | 0x0)
#define	I2C_ADDR_TX_DB  (I2C_DEV_EEPROM | 0x4)
#define	I2C_ADDR_RX_DB  (I2C_DEV_EEPROM | 0x5)

/***********************************************************************
 * Discovery
 **********************************************************************/
static device_addrs_t e100_find(const device_addr_t &hint){
    device_addrs_t e100_addrs;

    //return an empty list of addresses when type is set to non-usrp-e
    if (hint.has_key("type") and hint["type"] != "e100") return e100_addrs;

    //device node not provided, assume its 0
    if (not hint.has_key("node")){
        device_addr_t new_addr = hint;
        new_addr["node"] = "/dev/usrp_e0";
        return e100_find(new_addr);
    }

    //use the given device node name
    if (fs::exists(hint["node"])){
        device_addr_t new_addr;
        new_addr["type"] = "e100";
        new_addr["node"] = fs::system_complete(fs::path(hint["node"])).string();
        try{
            i2c_iface::sptr i2c_iface = e100_ctrl::make_dev_i2c_iface(E100_I2C_DEV_NODE);
            const mboard_eeprom_t mb_eeprom(*i2c_iface, E100_EEPROM_MAP_KEY);
            new_addr["name"] = mb_eeprom["name"];
            new_addr["serial"] = mb_eeprom["serial"];
        }
        catch(const std::exception &e){
            new_addr["name"] = "";
            new_addr["serial"] = "";
        }
        if (
            (not hint.has_key("name")   or hint["name"]   == new_addr["name"]) and
            (not hint.has_key("serial") or hint["serial"] == new_addr["serial"])
        ){
            e100_addrs.push_back(new_addr);
        }
    }

    return e100_addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
static device::sptr e100_make(const device_addr_t &device_addr){
    return device::sptr(new e100_impl(device_addr));
}

UHD_STATIC_BLOCK(register_e100_device){
    device::register_device(&e100_find, &e100_make);
}

static const uhd::dict<std::string, std::string> model_to_fpga_file_name = boost::assign::map_list_of
    ("E100", "usrp_e100_fpga_v2.bin")
    ("E110", "usrp_e110_fpga.bin")
;

/***********************************************************************
 * Structors
 **********************************************************************/
e100_impl::e100_impl(const uhd::device_addr_t &device_addr){
    _tree = property_tree::make();

    //read the eeprom so we can determine the hardware
    _dev_i2c_iface = e100_ctrl::make_dev_i2c_iface(E100_I2C_DEV_NODE);
    const mboard_eeprom_t mb_eeprom(*_dev_i2c_iface, E100_EEPROM_MAP_KEY);

    //determine the model string for this device
    const std::string model = device_addr.get("model", mb_eeprom.get("model", ""));
    if (not model_to_fpga_file_name.has_key(model)) throw uhd::runtime_error(str(boost::format(
        "\n"
        "  The specified model string \"%s\" is not recognized.\n"
        "  Perhaps the EEPROM is uninitialized, missing, or damaged.\n"
        "  Or, a monitor is pirating the I2C address of the EEPROM.\n"
    ) % model));

    //extract the fpga path and compute hash
    const std::string default_fpga_file_name = model_to_fpga_file_name[model];
    std::string e100_fpga_image;
    try{
        e100_fpga_image = find_image_path(device_addr.get("fpga", default_fpga_file_name));
    }
    catch(...){
        UHD_MSG(error) << boost::format("Could not find FPGA image. %s\n") % print_images_error();
        throw;
    }
    e100_load_fpga(e100_fpga_image);

    ////////////////////////////////////////////////////////////////////
    // Setup the FPGA clock over AUX SPI
    ////////////////////////////////////////////////////////////////////
    bool dboard_clocks_diff = true;
    if      (mb_eeprom.get("revision", "0") == "3") dboard_clocks_diff = false;
    else if (mb_eeprom.get("revision", "0") == "4") dboard_clocks_diff = true;
    else UHD_MSG(warning)
        << "Unknown E1XX revision number!\n"
        << "defaulting to differential dboard clocks to be safe.\n"
        << std::endl;
    const double master_clock_rate = device_addr.cast<double>("master_clock_rate", E100_DEFAULT_CLOCK_RATE);
    _aux_spi_iface = e100_ctrl::make_aux_spi_iface();
    _clock_ctrl = e100_clock_ctrl::make(_aux_spi_iface, master_clock_rate, dboard_clocks_diff);

    ////////////////////////////////////////////////////////////////////
    // setup the main interface into fpga
    //  - do this after aux spi, because we share gpio147
    ////////////////////////////////////////////////////////////////////
    const std::string node = device_addr["node"];
    _fpga_ctrl = e100_ctrl::make(node);

    ////////////////////////////////////////////////////////////////////
    // Initialize FPGA control communication
    ////////////////////////////////////////////////////////////////////
    fifo_ctrl_excelsior_config fifo_ctrl_config;
    fifo_ctrl_config.async_sid_base = E100_TX_ASYNC_SID;
    fifo_ctrl_config.num_async_chan = 1;
    fifo_ctrl_config.ctrl_sid_base = E100_CTRL_MSG_SID;
    fifo_ctrl_config.spi_base = TOREG(SR_SPI);
    fifo_ctrl_config.spi_rb = REG_RB_SPI;
    _fifo_ctrl = fifo_ctrl_excelsior::make(_fpga_ctrl, fifo_ctrl_config);

    //Perform wishbone readback tests, these tests also write the hash
    bool test_fail = false;
    UHD_MSG(status) << "Performing control readback test... " << std::flush;
    size_t hash = time(NULL);
    for (size_t i = 0; i < 100; i++){
        boost::hash_combine(hash, i);
        _fifo_ctrl->poke32(TOREG(SR_MISC+0), boost::uint32_t(hash));
        test_fail = _fifo_ctrl->peek32(REG_RB_CONFIG0) != boost::uint32_t(hash);
        if (test_fail) break; //exit loop on any failure
    }
    UHD_MSG(status) << ((test_fail)? " fail" : "pass") << std::endl;

    if (test_fail) UHD_MSG(error) << boost::format(
        "The FPGA is either clocked improperly\n"
        "or the FPGA build is not compatible.\n"
        "Subsequent errors may follow...\n"
    );

    //check that the compatibility is correct
    this->check_fpga_compat();

    ////////////////////////////////////////////////////////////////////
    // Create controller objects
    ////////////////////////////////////////////////////////////////////
    _fpga_i2c_ctrl = i2c_core_200::make(_fifo_ctrl, TOREG(SR_I2C), REG_RB_I2C);
    _data_transport = e100_make_mmap_zero_copy(_fpga_ctrl);

    ////////////////////////////////////////////////////////////////////
    // Initialize the properties tree
    ////////////////////////////////////////////////////////////////////
    _tree->create<std::string>("/name").set("E-Series Device");
    const fs_path mb_path = "/mboards/0";
    _tree->create<std::string>(mb_path / "name").set(model);
    _tree->create<std::string>(mb_path / "codename").set("Euwanee");

    ////////////////////////////////////////////////////////////////////
    // setup the mboard eeprom
    ////////////////////////////////////////////////////////////////////
    _tree->create<mboard_eeprom_t>(mb_path / "eeprom")
        .set(mb_eeprom)
        .subscribe(boost::bind(&e100_impl::set_mb_eeprom, this, _1));

    ////////////////////////////////////////////////////////////////////
    // create clock control objects
    ////////////////////////////////////////////////////////////////////
    //^^^ clock created up top, just reg props here... ^^^
    _tree->create<double>(mb_path / "tick_rate")
        .publish(boost::bind(&e100_clock_ctrl::get_fpga_clock_rate, _clock_ctrl))
        .subscribe(boost::bind(&fifo_ctrl_excelsior::set_tick_rate, _fifo_ctrl, _1))
        .subscribe(boost::bind(&e100_impl::update_tick_rate, this, _1));

    //subscribe the command time while we are at it
    _tree->create<time_spec_t>(mb_path / "time/cmd")
        .subscribe(boost::bind(&fifo_ctrl_excelsior::set_time, _fifo_ctrl, _1));

    ////////////////////////////////////////////////////////////////////
    // create codec control objects
    ////////////////////////////////////////////////////////////////////
    _codec_ctrl = e100_codec_ctrl::make(_fifo_ctrl/*spi*/);
    const fs_path rx_codec_path = mb_path / "rx_codecs/A";
    const fs_path tx_codec_path = mb_path / "tx_codecs/A";
    _tree->create<std::string>(rx_codec_path / "name").set("ad9522");
    _tree->create<meta_range_t>(rx_codec_path / "gains/pga/range").set(e100_codec_ctrl::rx_pga_gain_range);
    _tree->create<double>(rx_codec_path / "gains/pga/value")
        .coerce(boost::bind(&e100_impl::update_rx_codec_gain, this, _1));
    _tree->create<std::string>(tx_codec_path / "name").set("ad9522");
    _tree->create<meta_range_t>(tx_codec_path / "gains/pga/range").set(e100_codec_ctrl::tx_pga_gain_range);
    _tree->create<double>(tx_codec_path / "gains/pga/value")
        .subscribe(boost::bind(&e100_codec_ctrl::set_tx_pga_gain, _codec_ctrl, _1))
        .publish(boost::bind(&e100_codec_ctrl::get_tx_pga_gain, _codec_ctrl));

    ////////////////////////////////////////////////////////////////////
    // and do the misc mboard sensors
    ////////////////////////////////////////////////////////////////////
    _tree->create<sensor_value_t>(mb_path / "sensors/ref_locked")
        .publish(boost::bind(&e100_impl::get_ref_locked, this));

    ////////////////////////////////////////////////////////////////////
    // Create the GPSDO control
    ////////////////////////////////////////////////////////////////////
    static const fs::path GPSDO_VOLATILE_PATH("/media/ram/e100_internal_gpsdo.cache");
    if (not fs::exists(GPSDO_VOLATILE_PATH))
    {
        UHD_MSG(status) << "Detecting internal GPSDO.... " << std::flush;
        try{
            _gps = gps_ctrl::make(e100_ctrl::make_gps_uart_iface(E100_UART_DEV_NODE));
        }
        catch(std::exception &e){
            UHD_MSG(error) << "An error occurred making GPSDO control: " << e.what() << std::endl;
        }
        if (_gps and _gps->gps_detected())
        {
            UHD_MSG(status) << "found" << std::endl;
            BOOST_FOREACH(const std::string &name, _gps->get_sensors())
            {
                _tree->create<sensor_value_t>(mb_path / "sensors" / name)
                    .publish(boost::bind(&gps_ctrl::get_sensor, _gps, name));
            }
        }
        else
        {
            UHD_MSG(status) << "not found" << std::endl;
            std::ofstream(GPSDO_VOLATILE_PATH.string().c_str(), std::ofstream::binary) << "42" << std::endl;
        }
    }

    ////////////////////////////////////////////////////////////////////
    // create frontend control objects
    ////////////////////////////////////////////////////////////////////
    _rx_fe = rx_frontend_core_200::make(_fifo_ctrl, TOREG(SR_RX_FE));
    _tx_fe = tx_frontend_core_200::make(_fifo_ctrl, TOREG(SR_TX_FE));

    _tree->create<subdev_spec_t>(mb_path / "rx_subdev_spec")
        .subscribe(boost::bind(&e100_impl::update_rx_subdev_spec, this, _1));
    _tree->create<subdev_spec_t>(mb_path / "tx_subdev_spec")
        .subscribe(boost::bind(&e100_impl::update_tx_subdev_spec, this, _1));

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
    const size_t num_rx_dsps = _fifo_ctrl->peek32(REG_RB_NUM_RX_DSP);
    for (size_t dspno = 0; dspno < num_rx_dsps; dspno++)
    {
        const size_t sr_off = dspno*32;
        _rx_dsps.push_back(rx_dsp_core_200::make(
            _fifo_ctrl,
            TOREG(SR_RX_DSP0+sr_off),
            TOREG(SR_RX_CTRL0+sr_off),
            E100_RX_SID_BASE + dspno
        ));

        _rx_dsps[dspno]->set_link_rate(E100_RX_LINK_RATE_BPS);
        _tree->access<double>(mb_path / "tick_rate")
            .subscribe(boost::bind(&rx_dsp_core_200::set_tick_rate, _rx_dsps[dspno], _1));
        fs_path rx_dsp_path = mb_path / str(boost::format("rx_dsps/%u") % dspno);
        _tree->create<meta_range_t>(rx_dsp_path / "rate/range")
            .publish(boost::bind(&rx_dsp_core_200::get_host_rates, _rx_dsps[dspno]));
        _tree->create<double>(rx_dsp_path / "rate/value")
            .set(1e6) //some default
            .coerce(boost::bind(&rx_dsp_core_200::set_host_rate, _rx_dsps[dspno], _1))
            .subscribe(boost::bind(&e100_impl::update_rx_samp_rate, this, dspno, _1));
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
        _fifo_ctrl, TOREG(SR_TX_DSP), TOREG(SR_TX_CTRL), E100_TX_ASYNC_SID
    );
    _tx_dsp->set_link_rate(E100_TX_LINK_RATE_BPS);
    _tree->access<double>(mb_path / "tick_rate")
        .subscribe(boost::bind(&tx_dsp_core_200::set_tick_rate, _tx_dsp, _1));
    _tree->create<meta_range_t>(mb_path / "tx_dsps/0/rate/range")
        .publish(boost::bind(&tx_dsp_core_200::get_host_rates, _tx_dsp));
    _tree->create<double>(mb_path / "tx_dsps/0/rate/value")
        .set(1e6) //some default
        .coerce(boost::bind(&tx_dsp_core_200::set_host_rate, _tx_dsp, _1))
        .subscribe(boost::bind(&e100_impl::update_tx_samp_rate, this, 0, _1));
    _tree->create<double>(mb_path / "tx_dsps/0/freq/value")
        .coerce(boost::bind(&tx_dsp_core_200::set_freq, _tx_dsp, _1));
    _tree->create<meta_range_t>(mb_path / "tx_dsps/0/freq/range")
        .publish(boost::bind(&tx_dsp_core_200::get_freq_range, _tx_dsp));

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
        .subscribe(boost::bind(&e100_impl::update_clock_source, this, _1));
    std::vector<std::string> clock_sources = boost::assign::list_of("internal")("external")("auto");
    if (_gps and _gps->gps_detected()) clock_sources.push_back("gpsdo");
    _tree->create<std::vector<std::string> >(mb_path / "clock_source/options").set(clock_sources);

    ////////////////////////////////////////////////////////////////////
    // create user-defined control objects
    ////////////////////////////////////////////////////////////////////
    _user = user_settings_core_200::make(_fifo_ctrl, TOREG(SR_USER_REGS));
    _tree->create<user_settings_core_200::user_reg_t>(mb_path / "user/regs")
        .subscribe(boost::bind(&user_settings_core_200::set_reg, _user, _1));

    ////////////////////////////////////////////////////////////////////
    // create dboard control objects
    ////////////////////////////////////////////////////////////////////

    //read the dboard eeprom to extract the dboard ids
    dboard_eeprom_t rx_db_eeprom, tx_db_eeprom, gdb_eeprom;
    rx_db_eeprom.load(*_fpga_i2c_ctrl, I2C_ADDR_RX_DB);
    tx_db_eeprom.load(*_fpga_i2c_ctrl, I2C_ADDR_TX_DB);
    gdb_eeprom.load(*_fpga_i2c_ctrl, I2C_ADDR_TX_DB ^ 5);

    //disable rx dc offset if LFRX
    if (rx_db_eeprom.id == 0x000f) _tree->access<bool>(rx_fe_path / "dc_offset" / "enable").set(false);

    //create the properties and register subscribers
    _tree->create<dboard_eeprom_t>(mb_path / "dboards/A/rx_eeprom")
        .set(rx_db_eeprom)
        .subscribe(boost::bind(&e100_impl::set_db_eeprom, this, "rx", _1));
    _tree->create<dboard_eeprom_t>(mb_path / "dboards/A/tx_eeprom")
        .set(tx_db_eeprom)
        .subscribe(boost::bind(&e100_impl::set_db_eeprom, this, "tx", _1));
    _tree->create<dboard_eeprom_t>(mb_path / "dboards/A/gdb_eeprom")
        .set(gdb_eeprom)
        .subscribe(boost::bind(&e100_impl::set_db_eeprom, this, "gdb", _1));

    //create a new dboard interface and manager
    _dboard_iface = make_e100_dboard_iface(_fifo_ctrl, _fpga_i2c_ctrl, _fifo_ctrl/*spi*/, _clock_ctrl, _codec_ctrl);
    _tree->create<dboard_iface::sptr>(mb_path / "dboards/A/iface").set(_dboard_iface);
    _dboard_manager = dboard_manager::make(
        rx_db_eeprom.id, tx_db_eeprom.id, gdb_eeprom.id,
        _dboard_iface, _tree->subtree(mb_path / "dboards/A")
    );

    //bind frontend corrections to the dboard freq props
    const fs_path db_tx_fe_path = mb_path / "dboards" / "A" / "tx_frontends";
    BOOST_FOREACH(const std::string &name, _tree->list(db_tx_fe_path)){
        _tree->access<double>(db_tx_fe_path / name / "freq" / "value")
            .subscribe(boost::bind(&e100_impl::set_tx_fe_corrections, this, _1));
    }
    const fs_path db_rx_fe_path = mb_path / "dboards" / "A" / "rx_frontends";
    BOOST_FOREACH(const std::string &name, _tree->list(db_rx_fe_path)){
        _tree->access<double>(db_rx_fe_path / name / "freq" / "value")
            .subscribe(boost::bind(&e100_impl::set_rx_fe_corrections, this, _1));
    }

    //initialize io handling
    _recv_demuxer = recv_packet_demuxer::make(_data_transport, _rx_dsps.size(), E100_RX_SID_BASE);

    //allocate streamer weak ptrs containers
    _rx_streamers.resize(_rx_dsps.size());
    _tx_streamers.resize(1/*known to be 1 dsp*/);

    ////////////////////////////////////////////////////////////////////
    // do some post-init tasks
    ////////////////////////////////////////////////////////////////////
    this->update_rates();

    _tree->access<double>(mb_path / "tick_rate") //now subscribe the clock rate setter
        .subscribe(boost::bind(&e100_clock_ctrl::set_fpga_clock_rate, _clock_ctrl, _1));

    //reset cordic rates and their properties to zero
    BOOST_FOREACH(const std::string &name, _tree->list(mb_path / "rx_dsps")){
        _tree->access<double>(mb_path / "rx_dsps" / name / "freq" / "value").set(0.0);
    }
    BOOST_FOREACH(const std::string &name, _tree->list(mb_path / "tx_dsps")){
        _tree->access<double>(mb_path / "tx_dsps" / name / "freq" / "value").set(0.0);
    }

    _tree->access<subdev_spec_t>(mb_path / "rx_subdev_spec").set(subdev_spec_t("A:" + _tree->list(mb_path / "dboards/A/rx_frontends").at(0)));
    _tree->access<subdev_spec_t>(mb_path / "tx_subdev_spec").set(subdev_spec_t("A:" + _tree->list(mb_path / "dboards/A/tx_frontends").at(0)));
    _tree->access<std::string>(mb_path / "clock_source/value").set("internal");
    _tree->access<std::string>(mb_path / "time_source/value").set("none");

    //GPS installed: use external ref, time, and init time spec
    if (_gps and _gps->gps_detected()){
        _time64->enable_gpsdo();
        UHD_MSG(status) << "Setting references to the internal GPSDO" << std::endl;
        _tree->access<std::string>(mb_path / "time_source/value").set("gpsdo");
        _tree->access<std::string>(mb_path / "clock_source/value").set("gpsdo");
        UHD_MSG(status) << "Initializing time to the internal GPSDO" << std::endl;
        _time64->set_time_next_pps(time_spec_t(time_t(_gps->get_sensor("gps_time").to_int()+1)));
    }

}

e100_impl::~e100_impl(void){
    /* NOP */
}

double e100_impl::update_rx_codec_gain(const double gain){
    //set gain on both I and Q, readback on one
    //TODO in the future, gains should have individual control
    _codec_ctrl->set_rx_pga_gain(gain, 'A');
    _codec_ctrl->set_rx_pga_gain(gain, 'B');
    return _codec_ctrl->get_rx_pga_gain('A');
}

void e100_impl::set_mb_eeprom(const uhd::usrp::mboard_eeprom_t &mb_eeprom){
    mb_eeprom.commit(*_dev_i2c_iface, E100_EEPROM_MAP_KEY);
}

void e100_impl::set_db_eeprom(const std::string &type, const uhd::usrp::dboard_eeprom_t &db_eeprom){
    if (type == "rx") db_eeprom.store(*_fpga_i2c_ctrl, I2C_ADDR_RX_DB);
    if (type == "tx") db_eeprom.store(*_fpga_i2c_ctrl, I2C_ADDR_TX_DB);
    if (type == "gdb") db_eeprom.store(*_fpga_i2c_ctrl, I2C_ADDR_TX_DB ^ 5);
}

void e100_impl::update_clock_source(const std::string &source){

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
    else if (source == "gpsdo")    _clock_ctrl->use_external_ref();
    else throw uhd::runtime_error("unhandled clock configuration reference source: " + source);
}

sensor_value_t e100_impl::get_ref_locked(void){
    const bool lock = _clock_ctrl->get_locked();
    return sensor_value_t("Ref", lock, "locked", "unlocked");
}

void e100_impl::check_fpga_compat(void){
    const boost::uint32_t fpga_compat_num = _fifo_ctrl->peek32(REG_RB_COMPAT);
    boost::uint16_t fpga_major = fpga_compat_num >> 16, fpga_minor = fpga_compat_num & 0xffff;
    if (fpga_major == 0){ //old version scheme
        fpga_major = fpga_minor;
        fpga_minor = 0;
    }
    if (fpga_major != E100_FPGA_COMPAT_NUM){
        throw uhd::runtime_error(str(boost::format(
            "Expected FPGA compatibility number %d, but got %d:\n"
            "The FPGA build is not compatible with the host code build."
        ) % int(E100_FPGA_COMPAT_NUM) % fpga_major));
    }
    if (fpga_minor < 2){
        throw uhd::runtime_error(str(boost::format(
            "Expected FPGA compatibility minor number at least %d, but got %d:\n"
            "The FPGA build is not compatible with the host code build."
        ) % int(2) % fpga_minor));
    }
    _tree->create<std::string>("/mboards/0/fpga_version").set(str(boost::format("%u.%u") % fpga_major % fpga_minor));
}

void e100_impl::set_rx_fe_corrections(const double lo_freq){
    apply_rx_fe_corrections(this->get_tree()->subtree("/mboards/0"), "A", lo_freq);
}

void e100_impl::set_tx_fe_corrections(const double lo_freq){
    apply_tx_fe_corrections(this->get_tree()->subtree("/mboards/0"), "A", lo_freq);
}

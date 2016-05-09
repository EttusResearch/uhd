//
// Copyright 2015-2016 Ettus Research
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

#include "x300_radio_ctrl_impl.hpp"

#include "x300_dboard_iface.hpp"
#include "wb_iface_adapter.hpp"
#include "gpio_atr_3000.hpp"
#include <uhd/usrp/dboard_eeprom.hpp>
#include <boost/make_shared.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/usrp/dboard_iface.hpp>
#include <uhd/rfnoc/node_ctrl_base.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;
using namespace uhd::usrp::x300;

static const size_t IO_MASTER_RADIO = 0;

/****************************************************************************
 * Structors
 ***************************************************************************/
UHD_RFNOC_RADIO_BLOCK_CONSTRUCTOR(x300_radio_ctrl)
{
    UHD_RFNOC_BLOCK_TRACE() << "x300_radio_ctrl_impl::ctor() " << std::endl;

    ////////////////////////////////////////////////////////////////////
    // Set up basic info
    ////////////////////////////////////////////////////////////////////
    _radio_type = (get_block_id().get_block_count() == 0) ? PRIMARY : SECONDARY;
    _radio_slot = (get_block_id().get_block_count() == 0) ? "A" : "B";
    _radio_clk_rate = _tree->access<double>("tick_rate").get();

    ////////////////////////////////////////////////////////////////////
    // Set up peripherals
    ////////////////////////////////////////////////////////////////////
    wb_iface::sptr ctrl = _get_ctrl(IO_MASTER_RADIO);
    _regs = boost::make_shared<radio_regmap_t>(_radio_type==PRIMARY?0:1);
    _regs->initialize(*ctrl, true);

    //Only Radio0 has the ADC/DAC reset bits. Those bits are reserved for Radio1
    if (_radio_type==PRIMARY) {
        _regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::ADC_RESET, 1);
        _regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::DAC_RESET_N, 0);
        _regs->misc_outs_reg.flush();
        _regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::ADC_RESET, 0);
        _regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::DAC_RESET_N, 1);
        _regs->misc_outs_reg.flush();
    }
    _regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::DAC_ENABLED, 1);

    ////////////////////////////////////////////////////////////////
    // Setup peripherals
    ////////////////////////////////////////////////////////////////
    _spi = spi_core_3000::make(ctrl,
        radio_ctrl_impl::regs::sr_addr(radio_ctrl_impl::regs::SPI),
        radio_ctrl_impl::regs::RB_SPI);
    _leds = gpio_atr::gpio_atr_3000::make_write_only(ctrl, regs::sr_addr(regs::LEDS));
    _leds->set_atr_mode(usrp::gpio_atr::MODE_ATR, usrp::gpio_atr::gpio_atr_3000::MASK_SET_ALL);
    _adc = x300_adc_ctrl::make(_spi, DB_ADC_SEN);
    _dac = x300_dac_ctrl::make(_spi, DB_DAC_SEN, _radio_clk_rate);

    if (_radio_type==PRIMARY) {
        _fp_gpio = gpio_atr::gpio_atr_3000::make(ctrl, regs::sr_addr(regs::FP_GPIO), regs::RB_FP_GPIO);
        BOOST_FOREACH(const gpio_atr::gpio_attr_map_t::value_type attr, gpio_atr::gpio_attr_map) {
            _tree->create<boost::uint32_t>(fs_path("gpio") / "FP0" / attr.second)
                .set(0)
                .add_coerced_subscriber(boost::bind(&gpio_atr::gpio_atr_3000::set_gpio_attr, _fp_gpio, attr.first, _1));
        }
        _tree->create<boost::uint32_t>(fs_path("gpio") / "FP0" / "READBACK")
            .set_publisher(boost::bind(&gpio_atr::gpio_atr_3000::read_gpio, _fp_gpio));
    }

    ////////////////////////////////////////////////////////////////
    // create codec control objects
    ////////////////////////////////////////////////////////////////
    _tree->create<int>("rx_codecs" / _radio_slot / "gains"); //phony property so this dir exists
    _tree->create<int>("tx_codecs" / _radio_slot / "gains"); //phony property so this dir exists
    _tree->create<std::string>("rx_codecs" / _radio_slot / "name").set("ads62p48");
    _tree->create<std::string>("tx_codecs" / _radio_slot / "name").set("ad9146");

    _tree->create<meta_range_t>("rx_codecs" / _radio_slot / "gains" / "digital" / "range").set(meta_range_t(0, 6.0, 0.5));
    _tree->create<double>("rx_codecs" / _radio_slot / "gains" / "digital" / "value")
        .add_coerced_subscriber(boost::bind(&x300_adc_ctrl::set_gain, _adc, _1)).set(0)
    ;

    ////////////////////////////////////////////////////////////////
    // create front-end objects
    ////////////////////////////////////////////////////////////////
    for (size_t i = 0; i < _get_num_radios(); i++) {
        std::string fe_name = _radio_slot + ((i == 0) ? "0" : "1");

        _rx_fe_map[i].name = fe_name;
        _rx_fe_map[i].core = rx_frontend_core_3000::make(_get_ctrl(i), regs::sr_addr(x300_regs::RX_RE_BASE));
        _tree->create<int>("rx_frontends" / fe_name);
        _rx_fe_map[i].core->populate_subtree(_tree->subtree("rx_frontends" / fe_name));

        _tx_fe_map[i].name = fe_name;
        _tx_fe_map[i].core = tx_frontend_core_200::make(_get_ctrl(i), regs::sr_addr(x300_regs::TX_FE_BASE));
        _tree->create<int>("tx_frontends" / fe_name);
        _tx_fe_map[i].core->populate_subtree(_tree->subtree("tx_frontends" / fe_name));

        const fs_path rx_dsp_path = fs_path("rx_dsps") / fe_name;
        _tree->create<stream_cmd_t>(rx_dsp_path / "stream_cmd")
            .add_coerced_subscriber(boost::bind(&radio_ctrl_impl::issue_stream_cmd, this, _1, i));

        ////////////////////////////////////////////////////////////////////
        // add some dummy nodes on the prop tree (FIXME remove these)
        ////////////////////////////////////////////////////////////////////
        const fs_path tx_dsp_path = fs_path("tx_dsps") / fe_name;
        _tree->create<double>(tx_dsp_path / "freq/value").set(0.0);
        _tree->create<meta_range_t>(tx_dsp_path / "freq/range").set(meta_range_t(0.0, 0.0, 0.0));
        _tree->create<double>(rx_dsp_path / "freq/value").set(0.0);
        _tree->create<meta_range_t>(rx_dsp_path / "freq/range").set(meta_range_t(0.0, 0.0, 0.0));
        _tree->create<double>(tx_dsp_path / "rate/value")
            .add_coerced_subscriber(boost::bind(&radio_ctrl_impl::set_rate, this, _1))
            .set_publisher(boost::bind(&radio_ctrl_impl::get_rate, this))
        ;
        _tree->create<double>(rx_dsp_path / "rate/value")
            .add_coerced_subscriber(boost::bind(&radio_ctrl_impl::set_rate, this, _1))
            .set_publisher(boost::bind(&radio_ctrl_impl::get_rate, this))
        ;
    }

}

x300_radio_ctrl_impl::~x300_radio_ctrl_impl()
{
    // Tear down our part of the tree:
    _tree->remove(fs_path("rx_codecs" / _radio_slot));
    _tree->remove(fs_path("tx_codecs" / _radio_slot));
    for (size_t i = 0; i < _get_num_radios(); i++) {
        const std::string fe_name = _radio_slot + ((i == 0) ? "0" : "1");
        _tree->remove(fs_path("tx_dsps" / fe_name));
        _tree->remove(fs_path("rx_dsps" / fe_name));
        _tree->remove(fs_path("tx_frontends" / fe_name));
        _tree->remove(fs_path("rx_frontends" / fe_name));
    }
    if (_radio_type==PRIMARY) {
        BOOST_FOREACH(const gpio_atr::gpio_attr_map_t::value_type attr, gpio_atr::gpio_attr_map) {
            _tree->remove(fs_path("gpio") / "FP0" / attr.second);
        }
        _tree->remove(fs_path("gpio") / "FP0" / "READBACK");
    }

    // Reset peripherals
    if (_radio_type==PRIMARY) {
        _regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::ADC_RESET, 1);
        _regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::DAC_RESET_N, 0);
    }
    _regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::DAC_ENABLED, 0);
    _regs->misc_outs_reg.flush();
}

/****************************************************************************
 * API calls
 ***************************************************************************/
void x300_radio_ctrl_impl::set_tx_antenna(const std::string &ant, const size_t chan)
{
    _tree->access<std::string>(
        fs_path("dboards" / _radio_slot / "tx_frontends" / _tx_fe_map.at(chan).db_fe_name / "antenna" / "value")
    ).set(ant);
}

void x300_radio_ctrl_impl::set_rx_antenna(const std::string &ant, const size_t chan)
{
    _tree->access<std::string>(
        fs_path("dboards" / _radio_slot / "rx_frontends" / _rx_fe_map.at(chan).db_fe_name / "antenna" / "value")
    ).set(ant);
}

double x300_radio_ctrl_impl::set_tx_frequency(const double freq, const size_t chan)
{
    return _tree->access<double>(
        fs_path("dboards" / _radio_slot / "tx_frontends" / _tx_fe_map.at(chan).db_fe_name / "freq" / "value")
    ).set(freq).get();
}

double x300_radio_ctrl_impl::set_rx_frequency(const double freq, const size_t chan)
{
    return _tree->access<double>(
        fs_path("dboards" / _radio_slot / "rx_frontends" / _rx_fe_map.at(chan).db_fe_name / "freq" / "value")
    ).set(freq).get();
}

double x300_radio_ctrl_impl::set_tx_gain(const double gain, const size_t chan)
{
    //TODO: This is extremely hacky!
    fs_path path("dboards" / _radio_slot / "tx_frontends" / _tx_fe_map.at(chan).db_fe_name / "gains");
    std::vector<std::string> gain_stages = _tree->list(path);
    if (gain_stages.size() == 1) {
        return _tree->access<double>(path / gain_stages[0] / "value").set(gain).get();
    } else {
        throw uhd::assertion_error("set_tx_gain: could not apply gain for this daughterboard.");
    }
}

double x300_radio_ctrl_impl::set_rx_gain(const double gain, const size_t chan)
{
    //TODO: This is extremely hacky!
    fs_path path("dboards" / _radio_slot / "rx_frontends" / _tx_fe_map.at(chan).db_fe_name / "gains");
    std::vector<std::string> gain_stages = _tree->list(path);
    if (gain_stages.size() == 1) {
        return _tree->access<double>(path / gain_stages[0] / "value").set(gain).get();
    } else {
        throw uhd::assertion_error("set_tx_gain: could not apply gain for this daughterboard.");
    }
}

/****************************************************************************
 * Radio control and setup
 ***************************************************************************/
void x300_radio_ctrl_impl::setup_radio(uhd::i2c_iface::sptr zpu_i2c, x300_clock_ctrl::sptr clock, bool verbose)
{
    _self_cal_adc_capture_delay(verbose);

    ////////////////////////////////////////////////////////////////////
    // create RF frontend interfacing
    ////////////////////////////////////////////////////////////////////
    static const size_t BASE_ADDR       = 0x50;
    static const size_t RX_EEPROM_ADDR  = 0x5;
    static const size_t TX_EEPROM_ADDR  = 0x4;
    static const size_t GDB_EEPROM_ADDR = 0x1;
    const static std::vector<size_t> EEPROM_ADDRS =
        boost::assign::list_of(RX_EEPROM_ADDR)(TX_EEPROM_ADDR)(GDB_EEPROM_ADDR);
    const static std::vector<std::string> EEPROM_PATHS =
        boost::assign::list_of("rx_eeprom")("tx_eeprom")("gdb_eeprom");

    const size_t DB_OFFSET = (_radio_slot == "A") ? 0x0 : 0x2;
    const fs_path db_path = ("dboards" / _radio_slot);
    for (size_t i = 0; i < EEPROM_ADDRS.size(); i++) {
        const size_t addr = EEPROM_ADDRS[i] + DB_OFFSET;
        //Load EEPROM
        _db_eeproms[addr].load(*zpu_i2c, BASE_ADDR | addr);
        //Add to tree
        _tree->create<dboard_eeprom_t>(db_path / EEPROM_PATHS[i])
            .set(_db_eeproms[addr])
            .add_coerced_subscriber(boost::bind(&dboard_eeprom_t::store,
                _db_eeproms[addr], boost::ref(*zpu_i2c), (BASE_ADDR | addr)));
    }

    //create a new dboard interface
    x300_dboard_iface_config_t db_config;
    db_config.gpio = gpio_atr::db_gpio_atr_3000::make(_get_ctrl(IO_MASTER_RADIO),
        radio_ctrl_impl::regs::sr_addr(radio_ctrl_impl::regs::GPIO), radio_ctrl_impl::regs::RB_DB_GPIO);
    db_config.spi = _spi;
    db_config.rx_spi_slaveno = DB_RX_SEN;
    db_config.tx_spi_slaveno = DB_TX_SEN;
    db_config.i2c = zpu_i2c;
    db_config.clock = clock;
    db_config.rx_dsp.reset();   //TODO: This really should be the FE correction block
    db_config.which_rx_clk = (_radio_slot == "A") ? X300_CLOCK_WHICH_DB0_RX : X300_CLOCK_WHICH_DB1_RX;
    db_config.which_tx_clk = (_radio_slot == "A") ? X300_CLOCK_WHICH_DB0_TX : X300_CLOCK_WHICH_DB1_TX;
    db_config.dboard_slot = (_radio_slot == "A")? 0 : 1;
    db_config.cmd_time_ctrl = _get_ctrl(IO_MASTER_RADIO);

    //create a new dboard manager
    _db_manager = dboard_manager::make(
        _db_eeproms[RX_EEPROM_ADDR + DB_OFFSET].id,
        _db_eeproms[TX_EEPROM_ADDR + DB_OFFSET].id,
        _db_eeproms[GDB_EEPROM_ADDR + DB_OFFSET].id,
        x300_dboard_iface::make(db_config),
        _tree->subtree(db_path)
    );

    size_t rx_chan = 0, tx_chan = 0;
    subdev_spec_t rx_spec, tx_spec;
    BOOST_FOREACH(const std::string& fe, _db_manager->get_rx_frontends()) {
        _rx_fe_map[rx_chan++].db_fe_name = fe;
        rx_spec.push_back(subdev_spec_pair_t(_radio_slot, fe));
    }
    BOOST_FOREACH(const std::string& fe, _db_manager->get_tx_frontends()) {
        _tx_fe_map[tx_chan++].db_fe_name = fe;
        tx_spec.push_back(subdev_spec_pair_t(_radio_slot, fe));
    }
    _tree->access<subdev_spec_t>("rx_subdev_spec").set(rx_spec);
    _tree->access<subdev_spec_t>("tx_subdev_spec").set(tx_spec);

    //now that dboard is created -- register into rx antenna event
    if (_tree->exists(db_path / "rx_frontends" / _radio_slot / "antenna" / "value")) {
        _tree->access<std::string>(db_path / "rx_frontends" / _radio_slot / "antenna" / "value")
            .add_coerced_subscriber(boost::bind(&x300_radio_ctrl_impl::_update_atr_leds, this, _1));
    }
    _update_atr_leds(""); //init anyway, even if never called
}

void x300_radio_ctrl_impl::reset_codec()
{
    if (_radio_type==PRIMARY) {  //ADC/DAC reset lines only exist in Radio0
        _regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::ADC_RESET, 1);
        _regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::DAC_RESET_N, 0);
        _regs->misc_outs_reg.flush();
        _regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::ADC_RESET, 0);
        _regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::DAC_RESET_N, 1);
        _regs->misc_outs_reg.flush();
    }
}

void x300_radio_ctrl_impl::self_test_adc(boost::uint32_t ramp_time_ms)
{
    //Bypass all front-end corrections
    for (size_t i = 0; i < _get_num_radios(); i++) {
        _rx_fe_map[i].core->bypass_all(true);
    }

    //Test basic patterns
    _adc->set_test_word("ones", "ones");    _check_adc(0xfffcfffc);
    _adc->set_test_word("zeros", "zeros");  _check_adc(0x00000000);
    _adc->set_test_word("ones", "zeros");   _check_adc(0xfffc0000);
    _adc->set_test_word("zeros", "ones");   _check_adc(0x0000fffc);
    for (size_t k = 0; k < 14; k++) {
        _adc->set_test_word("zeros", "custom", 1 << k);
        _check_adc(1 << (k+2));
    }
    for (size_t k = 0; k < 14; k++) {
        _adc->set_test_word("custom", "zeros", 1 << k);
        _check_adc(1 << (k+18));
    }

    //Turn on ramp pattern test
    _adc->set_test_word("ramp", "ramp");
    _regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 0);
    //Short sleep to allow ramp to propogate through ADC
    boost::this_thread::sleep(boost::posix_time::microsec(1));
    _regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 1);

    boost::this_thread::sleep(boost::posix_time::milliseconds(ramp_time_ms));
    _regs->misc_ins_reg.refresh();

    std::string i_status, q_status;
    if (_regs->misc_ins_reg.get(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_I_LOCKED))
        if (_regs->misc_ins_reg.get(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_I_ERROR))
            i_status = "Bit Errors!";
        else
            i_status = "Good";
    else
        i_status = "Not Locked!";

    if (_regs->misc_ins_reg.get(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_Q_LOCKED))
        if (_regs->misc_ins_reg.get(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_Q_ERROR))
            q_status = "Bit Errors!";
        else
            q_status = "Good";
    else
        q_status = "Not Locked!";

    //Return to normal mode
    _adc->set_test_word("normal", "normal");

    if ((i_status != "Good") or (q_status != "Good")) {
        throw uhd::runtime_error(
            (boost::format("ADC self-test failed for %s. Ramp checker status: {ADC_A=%s, ADC_B=%s}")%unique_id()%i_status%q_status).str());
    }

    //Restore front-end corrections
    for (size_t i = 0; i < _get_num_radios(); i++) {
        _rx_fe_map[i].core->bypass_all(false);
    }
}

void x300_radio_ctrl_impl::extended_adc_test(const std::vector<x300_radio_ctrl_impl::sptr>& radios, double duration_s)
{
    static const size_t SECS_PER_ITER = 5;
    UHD_MSG(status) << boost::format("Running Extended ADC Self-Test (Duration=%.0fs, %ds/iteration)...\n")
        % duration_s % SECS_PER_ITER;

    size_t num_iters = static_cast<size_t>(ceil(duration_s/SECS_PER_ITER));
    size_t num_failures = 0;
    for (size_t iter = 0; iter < num_iters; iter++) {
        //Print date and time
        boost::posix_time::time_facet *facet = new boost::posix_time::time_facet("%d-%b-%Y %H:%M:%S");
        std::ostringstream time_strm;
        time_strm.imbue(std::locale(std::locale::classic(), facet));
        time_strm << boost::posix_time::second_clock::local_time();
        //Run self-test
        UHD_MSG(status) << boost::format("-- [%s] Iteration %06d... ") % time_strm.str() % (iter+1);
        try {
            for (size_t i = 0; i < radios.size(); i++) {
                radios[i]->self_test_adc((SECS_PER_ITER*1000)/radios.size());
            }
            UHD_MSG(status) << "passed" << std::endl;
        } catch(std::exception &e) {
            num_failures++;
            UHD_MSG(status) << e.what() << std::endl;
        }
    }
    if (num_failures == 0) {
        UHD_MSG(status) << "Extended ADC Self-Test PASSED\n";
    } else {
        throw uhd::runtime_error(
                (boost::format("Extended ADC Self-Test FAILED!!! (%d/%d failures)\n") % num_failures % num_iters).str());
    }
}

void x300_radio_ctrl_impl::synchronize_dacs(const std::vector<x300_radio_ctrl_impl::sptr>& radios)
{
    if (radios.size() < 2) return;  //Nothing to synchronize

    //**PRECONDITION**
    //This function assumes that all the VITA times in "radios" are synchronized
    //to a common reference. Currently, this function is called in get_tx_stream
    //which also has the same precondition.

    //Reinitialize and resync all DACs
    for (size_t i = 0; i < radios.size(); i++) {
        radios[i]->_dac->reset_and_resync();
    }

    //Get a rough estimate of the cumulative command latency
    boost::posix_time::ptime t_start = boost::posix_time::microsec_clock::local_time();
    for (size_t i = 0; i < radios.size(); i++) {
        radios[i]->user_reg_read64(regs::RB_TIME_NOW); //Discard value. We are just timing the call
    }
    boost::posix_time::time_duration t_elapsed =
        boost::posix_time::microsec_clock::local_time() - t_start;

    //Add 100% of headroom + uncertaintly to the command time
    boost::uint64_t t_sync_us = (t_elapsed.total_microseconds() * 2) + 13000 /*Scheduler latency*/;

    //Pick radios[0] as the time reference.
    uhd::time_spec_t sync_time =
        radios[0]->_time64->get_time_now() + uhd::time_spec_t(((double)t_sync_us)/1e6);

    //Send the sync command
    for (size_t i = 0; i < radios.size(); i++) {
        radios[i]->set_command_tick_rate(radios[i]->_radio_clk_rate, IO_MASTER_RADIO);
        radios[i]->_regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::DAC_SYNC, 0);
        radios[i]->set_command_time(sync_time, IO_MASTER_RADIO);
        //Arm FRAMEP/N sync pulse by asserting a rising edge
        radios[i]->_regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::DAC_SYNC, 1);
        radios[i]->_regs->misc_outs_reg.set(radio_regmap_t::misc_outs_reg_t::DAC_SYNC, 0);
        radios[i]->set_command_time(uhd::time_spec_t(0.0), IO_MASTER_RADIO);
    }

    //Wait and check status
    boost::this_thread::sleep(boost::posix_time::microseconds(t_sync_us));
    for (size_t i = 0; i < radios.size(); i++) {
        radios[i]->_dac->verify_sync();
    }
}

double x300_radio_ctrl_impl::self_cal_adc_xfer_delay(
    const std::vector<x300_radio_ctrl_impl::sptr>& radios,
    x300_clock_ctrl::sptr clock,
    boost::function<void(double)> wait_for_clk_locked,
    bool apply_delay)
{
    UHD_MSG(status) << "Running ADC transfer delay self-cal: " << std::flush;

    //Effective resolution of the self-cal.
    static const size_t NUM_DELAY_STEPS = 100;

    double master_clk_period = (1.0e9 / clock->get_master_clock_rate()); //in ns
    double delay_start = 0.0;
    double delay_range = 2 * master_clk_period;
    double delay_incr = delay_range / NUM_DELAY_STEPS;

    UHD_MSG(status) << "Measuring..." << std::flush;
    double cached_clk_delay = clock->get_clock_delay(X300_CLOCK_WHICH_ADC0);
    double fpga_clk_delay = clock->get_clock_delay(X300_CLOCK_WHICH_FPGA);

    //Iterate through several values of delays and measure ADC data integrity
    std::vector< std::pair<double,bool> > results;
    for (size_t i = 0; i < NUM_DELAY_STEPS; i++) {
        //Delay the ADC clock (will set both Ch0 and Ch1 delays)
        double delay = clock->set_clock_delay(X300_CLOCK_WHICH_ADC0, delay_incr*i + delay_start);
        wait_for_clk_locked(0.1);

        boost::uint32_t err_code = 0;
        for (size_t r = 0; r < radios.size(); r++) {
            //Test each channel (I and Q) individually so as to not accidentally trigger
            //on the data from the other channel if there is a swap

            // -- Test I Channel --
            //Put ADC in ramp test mode. Tie the other channel to all ones.
            radios[r]->_adc->set_test_word("ramp", "ones");
            //Turn on the pattern checker in the FPGA. It will lock when it sees a zero
            //and count deviations from the expected value
            radios[r]->_regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 0);
            radios[r]->_regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 1);
            //50ms @ 200MHz = 10 million samples
            boost::this_thread::sleep(boost::posix_time::milliseconds(50));
            if (radios[r]->_regs->misc_ins_reg.read(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_I_LOCKED)) {
                err_code += radios[r]->_regs->misc_ins_reg.get(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_I_ERROR);
            } else {
                err_code += 100;    //Increment error code by 100 to indicate no lock
            }

            // -- Test Q Channel --
            //Put ADC in ramp test mode. Tie the other channel to all ones.
            radios[r]->_adc->set_test_word("ones", "ramp");
            //Turn on the pattern checker in the FPGA. It will lock when it sees a zero
            //and count deviations from the expected value
            radios[r]->_regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 0);
            radios[r]->_regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 1);
            //50ms @ 200MHz = 10 million samples
            boost::this_thread::sleep(boost::posix_time::milliseconds(50));
            if (radios[r]->_regs->misc_ins_reg.read(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_Q_LOCKED)) {
                err_code += radios[r]->_regs->misc_ins_reg.get(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER1_Q_ERROR);
            } else {
                err_code += 100;    //Increment error code by 100 to indicate no lock
            }
        }
        //UHD_MSG(status) << (boost::format("XferDelay=%fns, Error=%d\n") % delay % err_code);
        results.push_back(std::pair<double,bool>(delay, err_code==0));
    }

    //Calculate the valid window
    int win_start_idx = -1, win_stop_idx = -1, cur_start_idx = -1, cur_stop_idx = -1;
    for (size_t i = 0; i < results.size(); i++) {
        std::pair<double,bool>& item = results[i];
        if (item.second) {  //If data is stable
            if (cur_start_idx == -1) {  //This is the first window
                cur_start_idx = i;
                cur_stop_idx = i;
            } else {                    //We are extending the window
                cur_stop_idx = i;
            }
        } else {
            if (cur_start_idx == -1) {  //We haven't yet seen valid data
                //Do nothing
            } else if (win_start_idx == -1) {   //We passed the first valid window
                win_start_idx = cur_start_idx;
                win_stop_idx = cur_stop_idx;
            } else {                    //Update cached window if current window is larger
                double cur_win_len = results[cur_stop_idx].first - results[cur_start_idx].first;
                double cached_win_len = results[win_stop_idx].first - results[win_start_idx].first;
                if (cur_win_len > cached_win_len) {
                    win_start_idx = cur_start_idx;
                    win_stop_idx = cur_stop_idx;
                }
            }
            //Reset current window
            cur_start_idx = -1;
            cur_stop_idx = -1;
        }
    }
    if (win_start_idx == -1) {
        throw uhd::runtime_error("self_cal_adc_xfer_delay: Self calibration failed. Convergence error.");
    }

    double win_center = (results[win_stop_idx].first + results[win_start_idx].first) / 2.0;
    double win_length = results[win_stop_idx].first - results[win_start_idx].first;
    if (win_length < master_clk_period/4) {
        throw uhd::runtime_error("self_cal_adc_xfer_delay: Self calibration failed. Valid window too narrow.");
    }

    //Cycle slip the relative delay by a clock cycle to prevent sample misalignment
    //fpga_clk_delay > 0 and 0 < win_center < 2*(1/MCR) so one cycle slip is all we need
    bool cycle_slip = (win_center-fpga_clk_delay >= master_clk_period);
    if (cycle_slip) {
        win_center -= master_clk_period;
    }

    if (apply_delay) {
        UHD_MSG(status) << "Validating..." << std::flush;
        //Apply delay
        win_center = clock->set_clock_delay(X300_CLOCK_WHICH_ADC0, win_center);  //Sets ADC0 and ADC1
        wait_for_clk_locked(0.1);
        //Validate
        for (size_t r = 0; r < radios.size(); r++) {
            radios[r]->self_test_adc(2000);
        }
    } else {
        //Restore delay
        clock->set_clock_delay(X300_CLOCK_WHICH_ADC0, cached_clk_delay);  //Sets ADC0 and ADC1
    }

    //Teardown
    for (size_t r = 0; r < radios.size(); r++) {
        radios[r]->_adc->set_test_word("normal", "normal");
        radios[r]->_regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 0);
    }
    UHD_MSG(status) << (boost::format(" done (FPGA->ADC=%.3fns%s, Window=%.3fns)\n") %
        (win_center-fpga_clk_delay) % (cycle_slip?" +cyc":"") % win_length);

    return win_center;
}
/****************************************************************************
 * Helpers
 ***************************************************************************/
void x300_radio_ctrl_impl::_update_atr_leds(const std::string &rx_ant)
{
    const bool is_txrx = (rx_ant == "TX/RX");
    const int rx_led = (1 << 2);
    const int tx_led = (1 << 1);
    const int txrx_led = (1 << 0);
    _leds->set_atr_reg(gpio_atr::ATR_REG_IDLE, 0);
    _leds->set_atr_reg(gpio_atr::ATR_REG_RX_ONLY, is_txrx? txrx_led : rx_led);
    _leds->set_atr_reg(gpio_atr::ATR_REG_TX_ONLY, tx_led);
    _leds->set_atr_reg(gpio_atr::ATR_REG_FULL_DUPLEX, rx_led | tx_led);
}

void x300_radio_ctrl_impl::_self_cal_adc_capture_delay(bool print_status)
{
    if (print_status) UHD_MSG(status) << "Running ADC capture delay self-cal..." << std::flush;

    static const boost::uint32_t NUM_DELAY_STEPS = 32;   //The IDELAYE2 element has 32 steps
    static const boost::uint32_t NUM_RETRIES     = 2;    //Retry self-cal if it fails in warmup situations
    static const boost::int32_t  MIN_WINDOW_LEN  = 4;

    boost::int32_t win_start = -1, win_stop = -1;
    boost::uint32_t iter = 0;
    while (iter++ < NUM_RETRIES) {
        for (boost::uint32_t dly_tap = 0; dly_tap < NUM_DELAY_STEPS; dly_tap++) {
            //Apply delay
            _regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_DATA_DLY_VAL, dly_tap);
            _regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_DATA_DLY_STB, 1);
            _regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_DATA_DLY_STB, 0);

            boost::uint32_t err_code = 0;

            // -- Test I Channel --
            //Put ADC in ramp test mode. Tie the other channel to all ones.
            _adc->set_test_word("ramp", "ones");
            //Turn on the pattern checker in the FPGA. It will lock when it sees a zero
            //and count deviations from the expected value
            _regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 0);
            _regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 1);
            //10ms @ 200MHz = 2 million samples
            boost::this_thread::sleep(boost::posix_time::milliseconds(10));
            if (_regs->misc_ins_reg.read(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER0_I_LOCKED)) {
                err_code += _regs->misc_ins_reg.get(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER0_I_ERROR);
            } else {
                err_code += 100;    //Increment error code by 100 to indicate no lock
            }

            // -- Test Q Channel --
            //Put ADC in ramp test mode. Tie the other channel to all ones.
            _adc->set_test_word("ones", "ramp");
            //Turn on the pattern checker in the FPGA. It will lock when it sees a zero
            //and count deviations from the expected value
            _regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 0);
            _regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 1);
            //10ms @ 200MHz = 2 million samples
            boost::this_thread::sleep(boost::posix_time::milliseconds(10));
            if (_regs->misc_ins_reg.read(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER0_Q_LOCKED)) {
                err_code += _regs->misc_ins_reg.get(radio_regmap_t::misc_ins_reg_t::ADC_CHECKER0_Q_ERROR);
            } else {
                err_code += 100;    //Increment error code by 100 to indicate no lock
            }

            if (err_code == 0) {
                if (win_start == -1) {      //This is the first window
                    win_start = dly_tap;
                    win_stop = dly_tap;
                } else {                    //We are extending the window
                    win_stop = dly_tap;
                }
            } else {
                if (win_start != -1) {      //A valid window turned invalid
                    if (win_stop - win_start >= MIN_WINDOW_LEN) {
                        break;              //Valid window found
                    } else {
                        win_start = -1;     //Reset window
                    }
                }
            }
            //UHD_MSG(status) << (boost::format("CapTap=%d, Error=%d\n") % dly_tap % err_code);
        }

        //Retry the self-cal if it fails
        if ((win_start == -1 || (win_stop - win_start) < MIN_WINDOW_LEN) && iter < NUM_RETRIES /*not last iteration*/) {
            win_start = -1;
            win_stop = -1;
            boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
        } else {
            break;
        }
    }
    _adc->set_test_word("normal", "normal");
    _regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_CHECKER_ENABLED, 0);

    if (win_start == -1) {
        throw uhd::runtime_error("self_cal_adc_capture_delay: Self calibration failed. Convergence error.");
    }

    if (win_stop-win_start < MIN_WINDOW_LEN) {
        throw uhd::runtime_error("self_cal_adc_capture_delay: Self calibration failed. Valid window too narrow.");
    }

    boost::uint32_t ideal_tap = (win_stop + win_start) / 2;
    _regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_DATA_DLY_VAL, ideal_tap);
    _regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_DATA_DLY_STB, 1);
    _regs->misc_outs_reg.write(radio_regmap_t::misc_outs_reg_t::ADC_DATA_DLY_STB, 0);

    if (print_status) {
        double tap_delay = (1.0e12 / _radio_clk_rate) / (2*32); //in ps
        UHD_MSG(status) << boost::format(" done (Tap=%d, Window=%d, TapDelay=%.3fps, Iter=%d)\n") % ideal_tap % (win_stop-win_start) % tap_delay % iter;
    }
}

void x300_radio_ctrl_impl::_check_adc(const boost::uint32_t val)
{
    //Wait for previous control transaction to flush
    user_reg_read64(regs::RB_TEST);
    //Wait for ADC test pattern to propagate
    boost::this_thread::sleep(boost::posix_time::microsec(5));
    //Read value of RX readback register and verify
    boost::uint32_t adc_rb = static_cast<boost::uint32_t>(user_reg_read64(regs::RB_TEST)>>32);
    adc_rb ^= 0xfffc0000; //adapt for I inversion in FPGA
    if (val != adc_rb) {
        throw uhd::runtime_error(
            (boost::format("ADC self-test failed for %s. (Exp=0x%x, Got=0x%x)")%unique_id()%val%adc_rb).str());
    }
}

/****************************************************************************
 * Register block
 ***************************************************************************/
UHD_RFNOC_BLOCK_REGISTER(x300_radio_ctrl, "X300Radio");

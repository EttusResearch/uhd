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

#include <uhd/property_tree.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/gain_group.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <cmath>

using namespace uhd;
using namespace uhd::usrp;

const std::string multi_usrp::ALL_GAINS = "";

/***********************************************************************
 * Helper methods
 **********************************************************************/
static void do_samp_rate_warning_message(
    double target_rate,
    double actual_rate,
    const std::string &xx
){
    static const double max_allowed_error = 1.0; //Sps
    if (std::abs(target_rate - actual_rate) > max_allowed_error){
        UHD_MSG(warning) << boost::format(
            "The hardware does not support the requested %s sample rate:\n"
            "Target sample rate: %f MSps\n"
            "Actual sample rate: %f MSps\n"
        ) % xx % (target_rate/1e6) % (actual_rate/1e6);
    }
}

static void do_tune_freq_warning_message(
    const tune_request_t &tune_req,
    double actual_freq,
    const std::string &xx
){
    //forget the warning when manual policy
    if (tune_req.dsp_freq_policy == tune_request_t::POLICY_MANUAL) return;
    if (tune_req.rf_freq_policy == tune_request_t::POLICY_MANUAL) return;

    const double target_freq = tune_req.target_freq;
    static const double max_allowed_error = 1.0; //Hz
    if (std::abs(target_freq - actual_freq) > max_allowed_error){
        UHD_MSG(warning) << boost::format(
            "The hardware does not support the requested %s frequency:\n"
            "Target frequency: %f MHz\n"
            "Actual frequency: %f MHz\n"
        ) % xx % (target_freq/1e6) % (actual_freq/1e6);
    }
}

static meta_range_t make_overall_tune_range(
    const meta_range_t &fe_range,
    const meta_range_t &dsp_range,
    const double bw
){
    meta_range_t range;
    BOOST_FOREACH(const range_t &sub_range, fe_range){
        range.push_back(range_t(
            sub_range.start() + std::max(dsp_range.start(), -bw),
            sub_range.stop() + std::min(dsp_range.stop(), bw),
            dsp_range.step()
        ));
    }
    return range;
}

/***********************************************************************
 * Gain helper functions
 **********************************************************************/
static double get_gain_value(property_tree::sptr subtree){
    return subtree->access<double>("value").get();
}

static void set_gain_value(property_tree::sptr subtree, const double gain){
    subtree->access<double>("value").set(gain);
}

static meta_range_t get_gain_range(property_tree::sptr subtree){
    return subtree->access<meta_range_t>("range").get();
}

static gain_fcns_t make_gain_fcns_from_subtree(property_tree::sptr subtree){
    gain_fcns_t gain_fcns;
    gain_fcns.get_range = boost::bind(&get_gain_range, subtree);
    gain_fcns.get_value = boost::bind(&get_gain_value, subtree);
    gain_fcns.set_value = boost::bind(&set_gain_value, subtree, _1);
    return gain_fcns;
}

/***********************************************************************
 * Tune Helper Functions
 **********************************************************************/
static const double RX_SIGN = +1.0;
static const double TX_SIGN = -1.0;

static tune_result_t tune_xx_subdev_and_dsp(
    const double xx_sign,
    property_tree::sptr dsp_subtree,
    property_tree::sptr rf_fe_subtree,
    const tune_request_t &tune_request
){
    //------------------------------------------------------------------
    //-- calculate the LO offset, only used with automatic policy
    //------------------------------------------------------------------
    double lo_offset = 0.0;
    if (rf_fe_subtree->access<bool>("use_lo_offset").get()){
        //If the frontend has lo_offset value and range properties, trust it for lo_offset
        if (rf_fe_subtree->exists("lo_offset/value")) lo_offset = rf_fe_subtree->access<double>("lo_offset/value").get();

        //If the local oscillator will be in the passband, use an offset.
        //But constrain the LO offset by the width of the filter bandwidth.
        const double rate = dsp_subtree->access<double>("rate/value").get();
        const double bw = rf_fe_subtree->access<double>("bandwidth/value").get();
        if (bw > rate) lo_offset = std::min((bw - rate)/2, rate/2);
    }

    //------------------------------------------------------------------
    //-- set the RF frequency depending upon the policy
    //------------------------------------------------------------------
    double target_rf_freq = 0.0;
    switch (tune_request.rf_freq_policy){
    case tune_request_t::POLICY_AUTO:
        target_rf_freq = tune_request.target_freq + lo_offset;
        rf_fe_subtree->access<double>("freq/value").set(target_rf_freq);
        break;

    case tune_request_t::POLICY_MANUAL:
        //If the rf_fe understands lo_offset settings, infer the desired lo_offset and set it
        //  Side effect: In TVRX2 for example, after setting the lo_offset (if_freq) with a
        //  POLICY_MANUAL, there is no way for the user to automatically get back to default
        //  if_freq without deconstruct/reconstruct the rf_fe objects.
        if (rf_fe_subtree->exists("lo_offset/value")) {
            rf_fe_subtree->access<double>("lo_offset/value").set(tune_request.rf_freq - tune_request.target_freq);
        }

        target_rf_freq = tune_request.rf_freq;
        rf_fe_subtree->access<double>("freq/value").set(target_rf_freq);
        break;

    case tune_request_t::POLICY_NONE: break; //does not set
    }
    const double actual_rf_freq = rf_fe_subtree->access<double>("freq/value").get();

    //------------------------------------------------------------------
    //-- calculate the dsp freq, only used with automatic policy
    //------------------------------------------------------------------
    double target_dsp_freq = actual_rf_freq - tune_request.target_freq;

    //invert the sign on the dsp freq for transmit
    target_dsp_freq *= xx_sign;

    //------------------------------------------------------------------
    //-- set the dsp frequency depending upon the dsp frequency policy
    //------------------------------------------------------------------
    switch (tune_request.dsp_freq_policy){
    case tune_request_t::POLICY_AUTO:
        dsp_subtree->access<double>("freq/value").set(target_dsp_freq);
        break;

    case tune_request_t::POLICY_MANUAL:
        target_dsp_freq = tune_request.dsp_freq;
        dsp_subtree->access<double>("freq/value").set(target_dsp_freq);
        break;

    case tune_request_t::POLICY_NONE: break; //does not set
    }
    const double actual_dsp_freq = dsp_subtree->access<double>("freq/value").get();

    //------------------------------------------------------------------
    //-- load and return the tune result
    //------------------------------------------------------------------
    tune_result_t tune_result;
    tune_result.target_rf_freq = target_rf_freq;
    tune_result.actual_rf_freq = actual_rf_freq;
    tune_result.target_dsp_freq = target_dsp_freq;
    tune_result.actual_dsp_freq = actual_dsp_freq;
    return tune_result;
}

static double derive_freq_from_xx_subdev_and_dsp(
    const double xx_sign,
    property_tree::sptr dsp_subtree,
    property_tree::sptr rf_fe_subtree
){
    //extract actual dsp and IF frequencies
    const double actual_rf_freq = rf_fe_subtree->access<double>("freq/value").get();
    const double actual_dsp_freq = dsp_subtree->access<double>("freq/value").get();

    //invert the sign on the dsp freq for transmit
    return actual_rf_freq - actual_dsp_freq * xx_sign;
}

/***********************************************************************
 * Multi USRP Implementation
 **********************************************************************/
class multi_usrp_impl : public multi_usrp{
public:
    multi_usrp_impl(const device_addr_t &addr){
        _dev = device::make(addr);
        _tree = _dev->get_tree();
    }

    device::sptr get_device(void){
        return _dev;
    }

    dict<std::string, std::string> get_usrp_rx_info(size_t chan){
        mboard_chan_pair mcp = rx_chan_to_mcp(chan);
        dict<std::string, std::string> usrp_info;

        mboard_eeprom_t mb_eeprom = _tree->access<mboard_eeprom_t>(mb_root(mcp.mboard) / "eeprom").get();
        dboard_eeprom_t db_eeprom = _tree->access<dboard_eeprom_t>(rx_rf_fe_root(mcp.chan).branch_path().branch_path() / "rx_eeprom").get();

        usrp_info["mboard_id"] = _tree->access<std::string>(mb_root(mcp.mboard) / "name").get();
        usrp_info["mboard_name"] = mb_eeprom["name"];
        usrp_info["mboard_serial"] = mb_eeprom["serial"];
        usrp_info["rx_id"] = db_eeprom.id.to_pp_string();
        usrp_info["rx_subdev_name"] = _tree->access<std::string>(rx_rf_fe_root(mcp.chan) / "name").get();
        usrp_info["rx_subdev_spec"] = _tree->access<subdev_spec_t>(mb_root(mcp.mboard) / "rx_subdev_spec").get().to_string();
        usrp_info["rx_serial"] = db_eeprom.serial;
        usrp_info["rx_antenna"] =  _tree->access<std::string>(rx_rf_fe_root(mcp.chan) / "antenna" / "value").get();

        return usrp_info;
    }

    dict<std::string, std::string> get_usrp_tx_info(size_t chan){
        mboard_chan_pair mcp = tx_chan_to_mcp(chan);
        dict<std::string, std::string> usrp_info;

        mboard_eeprom_t mb_eeprom = _tree->access<mboard_eeprom_t>(mb_root(mcp.mboard) / "eeprom").get();
        dboard_eeprom_t db_eeprom = _tree->access<dboard_eeprom_t>(tx_rf_fe_root(mcp.chan).branch_path().branch_path() / "tx_eeprom").get();

        usrp_info["mboard_id"] = _tree->access<std::string>(mb_root(mcp.mboard) / "name").get();
        usrp_info["mboard_name"] = mb_eeprom["name"];
        usrp_info["mboard_serial"] = mb_eeprom["serial"];
        usrp_info["tx_id"] = db_eeprom.id.to_pp_string();
        usrp_info["tx_subdev_name"] = _tree->access<std::string>(tx_rf_fe_root(mcp.chan) / "name").get();
        usrp_info["tx_subdev_spec"] = _tree->access<subdev_spec_t>(mb_root(mcp.mboard) / "tx_subdev_spec").get().to_string();
        usrp_info["tx_serial"] = db_eeprom.serial;
        usrp_info["tx_antenna"] = _tree->access<std::string>(tx_rf_fe_root(mcp.chan) / "antenna" / "value").get();

        return usrp_info;
    }

    /*******************************************************************
     * Mboard methods
     ******************************************************************/
    void set_master_clock_rate(double rate, size_t mboard){
        if (mboard != ALL_MBOARDS){
            _tree->access<double>(mb_root(mboard) / "tick_rate").set(rate);
            return;
        }
        for (size_t m = 0; m < get_num_mboards(); m++){
            set_master_clock_rate(rate, m);
        }
    }

    double get_master_clock_rate(size_t mboard){
        return _tree->access<double>(mb_root(mboard) / "tick_rate").get();
    }

    std::string get_pp_string(void){
        std::string buff = str(boost::format(
            "%s USRP:\n"
            "  Device: %s\n"
        )
            % ((get_num_mboards() > 1)? "Multi" : "Single")
            % (_tree->access<std::string>("/name").get())
        );
        for (size_t m = 0; m < get_num_mboards(); m++){
            buff += str(boost::format(
                "  Mboard %d: %s\n"
            ) % m
                % (_tree->access<std::string>(mb_root(m) / "name").get())
            );
        }

        //----------- rx side of life ----------------------------------
        for (size_t m = 0, chan = 0; m < get_num_mboards(); m++){
            for (; chan < (m + 1)*get_rx_subdev_spec(m).size(); chan++){
                buff += str(boost::format(
                    "  RX Channel: %u\n"
                    "    RX DSP: %s\n"
                    "    RX Dboard: %s\n"
                    "    RX Subdev: %s\n"
                ) % chan
                    % rx_dsp_root(chan).leaf()
                    % rx_rf_fe_root(chan).branch_path().branch_path().leaf()
                    % (_tree->access<std::string>(rx_rf_fe_root(chan) / "name").get())
                );
            }
        }

        //----------- tx side of life ----------------------------------
        for (size_t m = 0, chan = 0; m < get_num_mboards(); m++){
            for (; chan < (m + 1)*get_tx_subdev_spec(m).size(); chan++){
                buff += str(boost::format(
                    "  TX Channel: %u\n"
                    "    TX DSP: %s\n"
                    "    TX Dboard: %s\n"
                    "    TX Subdev: %s\n"
                ) % chan
                    % tx_dsp_root(chan).leaf()
                    % tx_rf_fe_root(chan).branch_path().branch_path().leaf()
                    % (_tree->access<std::string>(tx_rf_fe_root(chan) / "name").get())
                );
            }
        }

        return buff;
    }

    std::string get_mboard_name(size_t mboard){
        return _tree->access<std::string>(mb_root(mboard) / "name").get();
    }

    time_spec_t get_time_now(size_t mboard = 0){
        return _tree->access<time_spec_t>(mb_root(mboard) / "time/now").get();
    }

    time_spec_t get_time_last_pps(size_t mboard = 0){
        return _tree->access<time_spec_t>(mb_root(mboard) / "time/pps").get();
    }

    void set_time_now(const time_spec_t &time_spec, size_t mboard){
        if (mboard != ALL_MBOARDS){
            _tree->access<time_spec_t>(mb_root(mboard) / "time/now").set(time_spec);
            return;
        }
        for (size_t m = 0; m < get_num_mboards(); m++){
            set_time_now(time_spec, m);
        }
    }

    void set_time_next_pps(const time_spec_t &time_spec, size_t mboard){
        if (mboard != ALL_MBOARDS){
            _tree->access<time_spec_t>(mb_root(mboard) / "time/pps").set(time_spec);
            return;
        }
        for (size_t m = 0; m < get_num_mboards(); m++){
            set_time_next_pps(time_spec, m);
        }
    }

    void set_time_unknown_pps(const time_spec_t &time_spec){
        UHD_MSG(status) << "    1) catch time transition at pps edge" << std::endl;
        time_spec_t time_start = get_time_now();
        time_spec_t time_start_last_pps = get_time_last_pps();
        while(true){
            if (get_time_last_pps() != time_start_last_pps) break;
            if ((get_time_now() - time_start) > time_spec_t(1.1)){
                throw uhd::runtime_error(
                    "Board 0 may not be getting a PPS signal!\n"
                    "No PPS detected within the time interval.\n"
                    "See the application notes for your device.\n"
                );
            }
        }

        UHD_MSG(status) << "    2) set times next pps (synchronously)" << std::endl;
        set_time_next_pps(time_spec, ALL_MBOARDS);
        boost::this_thread::sleep(boost::posix_time::seconds(1));

        //verify that the time registers are read to be within a few RTT
        for (size_t m = 1; m < get_num_mboards(); m++){
            time_spec_t time_0 = this->get_time_now(0);
            time_spec_t time_i = this->get_time_now(m);
            if (time_i < time_0 or (time_i - time_0) > time_spec_t(0.01)){ //10 ms: greater than RTT but not too big
                UHD_MSG(warning) << boost::format(
                    "Detected time deviation between board %d and board 0.\n"
                    "Board 0 time is %f seconds.\n"
                    "Board %d time is %f seconds.\n"
                ) % m % time_0.get_real_secs() % m % time_i.get_real_secs();
            }
        }
    }

    bool get_time_synchronized(void){
        for (size_t m = 1; m < get_num_mboards(); m++){
            time_spec_t time_0 = this->get_time_now(0);
            time_spec_t time_i = this->get_time_now(m);
            if (time_i < time_0 or (time_i - time_0) > time_spec_t(0.01)) return false;
        }
        return true;
    }

    void set_command_time(const time_spec_t &time_spec, size_t mboard){
        if (mboard != ALL_MBOARDS){
            if (not _tree->exists(mb_root(mboard) / "time/cmd")){
                throw uhd::not_implemented_error("timed command feature not implemented on this hardware");
            }
            _tree->access<time_spec_t>(mb_root(mboard) / "time/cmd").set(time_spec);
            return;
        }
        for (size_t m = 0; m < get_num_mboards(); m++){
            set_command_time(time_spec, m);
        }
    }

    void clear_command_time(size_t mboard){
        if (mboard != ALL_MBOARDS){
            _tree->access<time_spec_t>(mb_root(mboard) / "time/cmd").set(time_spec_t(0.0));
            return;
        }
        for (size_t m = 0; m < get_num_mboards(); m++){
            clear_command_time(m);
        }
    }

    void issue_stream_cmd(const stream_cmd_t &stream_cmd, size_t chan){
        if (chan != ALL_CHANS){
            _tree->access<stream_cmd_t>(rx_dsp_root(chan) / "stream_cmd").set(stream_cmd);
            return;
        }
        for (size_t c = 0; c < get_rx_num_channels(); c++){
            issue_stream_cmd(stream_cmd, c);
        }
    }

    void set_clock_config(const clock_config_t &clock_config, size_t mboard){
        //set the reference source...
        std::string clock_source;
        switch(clock_config.ref_source){
        case clock_config_t::REF_INT: clock_source = "internal"; break;
        case clock_config_t::REF_SMA: clock_source = "external"; break;
        case clock_config_t::REF_MIMO: clock_source = "mimo"; break;
        default: clock_source = "unknown";
        }
        this->set_clock_source(clock_source, mboard);

        //set the time source
        std::string time_source;
        switch(clock_config.pps_source){
        case clock_config_t::PPS_INT: time_source = "internal"; break;
        case clock_config_t::PPS_SMA: time_source = "external"; break;
        case clock_config_t::PPS_MIMO: time_source = "mimo"; break;
        default: time_source = "unknown";
        }
        if (time_source == "external" and clock_config.pps_polarity == clock_config_t::PPS_NEG) time_source = "_external_";
        this->set_time_source(time_source, mboard);
    }

    void set_time_source(const std::string &source, const size_t mboard){
        if (mboard != ALL_MBOARDS){
            _tree->access<std::string>(mb_root(mboard) / "time_source" / "value").set(source);
            return;
        }
        for (size_t m = 0; m < get_num_mboards(); m++){
            this->set_time_source(source, m);
        }
    }

    std::string get_time_source(const size_t mboard){
        return _tree->access<std::string>(mb_root(mboard) / "time_source" / "value").get();
    }

    std::vector<std::string> get_time_sources(const size_t mboard){
        return _tree->access<std::vector<std::string> >(mb_root(mboard) / "time_source" / "options").get();
    }

    void set_clock_source(const std::string &source, const size_t mboard){
        if (mboard != ALL_MBOARDS){
            _tree->access<std::string>(mb_root(mboard) / "clock_source" / "value").set(source);
            return;
        }
        for (size_t m = 0; m < get_num_mboards(); m++){
            this->set_clock_source(source, m);
        }
    }

    std::string get_clock_source(const size_t mboard){
        return _tree->access<std::string>(mb_root(mboard) / "clock_source" / "value").get();
    }

    std::vector<std::string> get_clock_sources(const size_t mboard){
        return _tree->access<std::vector<std::string> >(mb_root(mboard) / "clock_source" / "options").get();
    }

    size_t get_num_mboards(void){
        return _tree->list("/mboards").size();
    }

    sensor_value_t get_mboard_sensor(const std::string &name, size_t mboard){
        return _tree->access<sensor_value_t>(mb_root(mboard) / "sensors" / name).get();
    }

    std::vector<std::string> get_mboard_sensor_names(size_t mboard){
        return _tree->list(mb_root(mboard) / "sensors");
    }

    void set_user_register(const boost::uint8_t addr, const boost::uint32_t data, size_t mboard){
        if (mboard != ALL_MBOARDS){
            typedef std::pair<boost::uint8_t, boost::uint32_t> user_reg_t;
            _tree->access<user_reg_t>(mb_root(mboard) / "user/regs").set(user_reg_t(addr, data));
            return;
        }
        for (size_t m = 0; m < get_num_mboards(); m++){
            set_user_register(addr, data, m);
        }
    }

    /*******************************************************************
     * RX methods
     ******************************************************************/
    void set_rx_subdev_spec(const subdev_spec_t &spec, size_t mboard){
        if (mboard != ALL_MBOARDS){
            _tree->access<subdev_spec_t>(mb_root(mboard) / "rx_subdev_spec").set(spec);
            return;
        }
        for (size_t m = 0; m < get_num_mboards(); m++){
            set_rx_subdev_spec(spec, m);
        }
    }

    subdev_spec_t get_rx_subdev_spec(size_t mboard){
        return _tree->access<subdev_spec_t>(mb_root(mboard) / "rx_subdev_spec").get();
    }

    size_t get_rx_num_channels(void){
        size_t sum = 0;
        for (size_t m = 0; m < get_num_mboards(); m++){
            sum += get_rx_subdev_spec(m).size();
        }
        return sum;
    }

    std::string get_rx_subdev_name(size_t chan){
        return _tree->access<std::string>(rx_rf_fe_root(chan) / "name").get();
    }

    void set_rx_rate(double rate, size_t chan){
        if (chan != ALL_CHANS){
            _tree->access<double>(rx_dsp_root(chan) / "rate" / "value").set(rate);
            do_samp_rate_warning_message(rate, get_rx_rate(chan), "RX");
            return;
        }
        for (size_t c = 0; c < get_rx_num_channels(); c++){
            set_rx_rate(rate, c);
        }
    }

    double get_rx_rate(size_t chan){
        return _tree->access<double>(rx_dsp_root(chan) / "rate" / "value").get();
    }

    meta_range_t get_rx_rates(size_t chan){
        return _tree->access<meta_range_t>(rx_dsp_root(chan) / "rate" / "range").get();
    }

    tune_result_t set_rx_freq(const tune_request_t &tune_request, size_t chan){
        tune_result_t r = tune_xx_subdev_and_dsp(RX_SIGN, _tree->subtree(rx_dsp_root(chan)), _tree->subtree(rx_rf_fe_root(chan)), tune_request);
        do_tune_freq_warning_message(tune_request, get_rx_freq(chan), "RX");
        return r;
    }

    double get_rx_freq(size_t chan){
        return derive_freq_from_xx_subdev_and_dsp(RX_SIGN, _tree->subtree(rx_dsp_root(chan)), _tree->subtree(rx_rf_fe_root(chan)));
    }

    freq_range_t get_rx_freq_range(size_t chan){
        return make_overall_tune_range(
            _tree->access<meta_range_t>(rx_rf_fe_root(chan) / "freq" / "range").get(),
            _tree->access<meta_range_t>(rx_dsp_root(chan) / "freq" / "range").get(),
            this->get_rx_bandwidth(chan)
        );
    }

    freq_range_t get_fe_rx_freq_range(size_t chan){
        return _tree->access<meta_range_t>(rx_rf_fe_root(chan) / "freq" / "range").get();
    }

    void set_rx_gain(double gain, const std::string &name, size_t chan){
        return rx_gain_group(chan)->set_value(gain, name);
    }

    double get_rx_gain(const std::string &name, size_t chan){
        return rx_gain_group(chan)->get_value(name);
    }

    gain_range_t get_rx_gain_range(const std::string &name, size_t chan){
        return rx_gain_group(chan)->get_range(name);
    }

    std::vector<std::string> get_rx_gain_names(size_t chan){
        return rx_gain_group(chan)->get_names();
    }

    void set_rx_antenna(const std::string &ant, size_t chan){
        _tree->access<std::string>(rx_rf_fe_root(chan) / "antenna" / "value").set(ant);
    }

    std::string get_rx_antenna(size_t chan){
        return _tree->access<std::string>(rx_rf_fe_root(chan) / "antenna" / "value").get();
    }

    std::vector<std::string> get_rx_antennas(size_t chan){
        return _tree->access<std::vector<std::string> >(rx_rf_fe_root(chan) / "antenna" / "options").get();
    }

    void set_rx_bandwidth(double bandwidth, size_t chan){
        _tree->access<double>(rx_rf_fe_root(chan) / "bandwidth" / "value").set(bandwidth);
    }

    double get_rx_bandwidth(size_t chan){
        return _tree->access<double>(rx_rf_fe_root(chan) / "bandwidth" / "value").get();
    }

    meta_range_t get_rx_bandwidth_range(size_t chan){
        return _tree->access<meta_range_t>(rx_rf_fe_root(chan) / "bandwidth" / "range").get();
    }

    dboard_iface::sptr get_rx_dboard_iface(size_t chan){
        return _tree->access<dboard_iface::sptr>(rx_rf_fe_root(chan).branch_path().branch_path() / "iface").get();
    }

    sensor_value_t get_rx_sensor(const std::string &name, size_t chan){
        return _tree->access<sensor_value_t>(rx_rf_fe_root(chan) / "sensors" / name).get();
    }

    std::vector<std::string> get_rx_sensor_names(size_t chan){
        return _tree->list(rx_rf_fe_root(chan) / "sensors");
    }

    void set_rx_dc_offset(const bool enb, size_t chan){
        if (chan != ALL_CHANS){
            _tree->access<bool>(rx_fe_root(chan) / "dc_offset" / "enable").set(enb);
            return;
        }
        for (size_t c = 0; c < get_rx_num_channels(); c++){
            this->set_rx_dc_offset(enb, c);
        }
    }

    void set_rx_dc_offset(const std::complex<double> &offset, size_t chan){
        if (chan != ALL_CHANS){
            _tree->access<std::complex<double> >(rx_fe_root(chan) / "dc_offset" / "value").set(offset);
            return;
        }
        for (size_t c = 0; c < get_rx_num_channels(); c++){
            this->set_rx_dc_offset(offset, c);
        }
    }

    void set_rx_iq_balance(const std::complex<double> &offset, size_t chan){
        if (chan != ALL_CHANS){
            _tree->access<std::complex<double> >(rx_fe_root(chan) / "iq_balance" / "value").set(offset);
            return;
        }
        for (size_t c = 0; c < get_rx_num_channels(); c++){
            this->set_rx_iq_balance(offset, c);
        }
    }

    /*******************************************************************
     * TX methods
     ******************************************************************/
    void set_tx_subdev_spec(const subdev_spec_t &spec, size_t mboard){
        if (mboard != ALL_MBOARDS){
            _tree->access<subdev_spec_t>(mb_root(mboard) / "tx_subdev_spec").set(spec);
            return;
        }
        for (size_t m = 0; m < get_num_mboards(); m++){
            set_tx_subdev_spec(spec, m);
        }
    }

    subdev_spec_t get_tx_subdev_spec(size_t mboard){
        return _tree->access<subdev_spec_t>(mb_root(mboard) / "tx_subdev_spec").get();
    }

    size_t get_tx_num_channels(void){
        size_t sum = 0;
        for (size_t m = 0; m < get_num_mboards(); m++){
            sum += get_tx_subdev_spec(m).size();
        }
        return sum;
    }

    std::string get_tx_subdev_name(size_t chan){
        return _tree->access<std::string>(tx_rf_fe_root(chan) / "name").get();
    }

    void set_tx_rate(double rate, size_t chan){
        if (chan != ALL_CHANS){
            _tree->access<double>(tx_dsp_root(chan) / "rate" / "value").set(rate);
            do_samp_rate_warning_message(rate, get_tx_rate(chan), "TX");
            return;
        }
        for (size_t c = 0; c < get_tx_num_channels(); c++){
            set_tx_rate(rate, c);
        }
    }

    double get_tx_rate(size_t chan){
        return _tree->access<double>(tx_dsp_root(chan) / "rate" / "value").get();
    }

    meta_range_t get_tx_rates(size_t chan){
        return _tree->access<meta_range_t>(tx_dsp_root(chan) / "rate" / "range").get();
    }

    tune_result_t set_tx_freq(const tune_request_t &tune_request, size_t chan){
        tune_result_t r = tune_xx_subdev_and_dsp(TX_SIGN, _tree->subtree(tx_dsp_root(chan)), _tree->subtree(tx_rf_fe_root(chan)), tune_request);
        do_tune_freq_warning_message(tune_request, get_tx_freq(chan), "TX");
        return r;
    }

    double get_tx_freq(size_t chan){
        return derive_freq_from_xx_subdev_and_dsp(TX_SIGN, _tree->subtree(tx_dsp_root(chan)), _tree->subtree(tx_rf_fe_root(chan)));
    }

    freq_range_t get_tx_freq_range(size_t chan){
        return make_overall_tune_range(
            _tree->access<meta_range_t>(tx_rf_fe_root(chan) / "freq" / "range").get(),
            _tree->access<meta_range_t>(tx_dsp_root(chan) / "freq" / "range").get(),
            this->get_tx_bandwidth(chan)
        );
    }

    freq_range_t get_fe_tx_freq_range(size_t chan){
        return _tree->access<meta_range_t>(tx_rf_fe_root(chan) / "freq" / "range").get();
    }

    void set_tx_gain(double gain, const std::string &name, size_t chan){
        return tx_gain_group(chan)->set_value(gain, name);
    }

    double get_tx_gain(const std::string &name, size_t chan){
        return tx_gain_group(chan)->get_value(name);
    }

    gain_range_t get_tx_gain_range(const std::string &name, size_t chan){
        return tx_gain_group(chan)->get_range(name);
    }

    std::vector<std::string> get_tx_gain_names(size_t chan){
        return tx_gain_group(chan)->get_names();
    }

    void set_tx_antenna(const std::string &ant, size_t chan){
        _tree->access<std::string>(tx_rf_fe_root(chan) / "antenna" / "value").set(ant);
    }

    std::string get_tx_antenna(size_t chan){
        return _tree->access<std::string>(tx_rf_fe_root(chan) / "antenna" / "value").get();
    }

    std::vector<std::string> get_tx_antennas(size_t chan){
        return _tree->access<std::vector<std::string> >(tx_rf_fe_root(chan) / "antenna" / "options").get();
    }

    void set_tx_bandwidth(double bandwidth, size_t chan){
        _tree->access<double>(tx_rf_fe_root(chan) / "bandwidth" / "value").set(bandwidth);
    }

    double get_tx_bandwidth(size_t chan){
        return _tree->access<double>(tx_rf_fe_root(chan) / "bandwidth" / "value").get();
    }

    meta_range_t get_tx_bandwidth_range(size_t chan){
        return _tree->access<meta_range_t>(tx_rf_fe_root(chan) / "bandwidth" / "range").get();
    }

    dboard_iface::sptr get_tx_dboard_iface(size_t chan){
        return _tree->access<dboard_iface::sptr>(tx_rf_fe_root(chan).branch_path().branch_path() / "iface").get();
    }

    sensor_value_t get_tx_sensor(const std::string &name, size_t chan){
        return _tree->access<sensor_value_t>(tx_rf_fe_root(chan) / "sensors" / name).get();
    }

    std::vector<std::string> get_tx_sensor_names(size_t chan){
        return _tree->list(tx_rf_fe_root(chan) / "sensors");
    }

    void set_tx_dc_offset(const std::complex<double> &offset, size_t chan){
        if (chan != ALL_CHANS){
            _tree->access<std::complex<double> >(tx_fe_root(chan) / "dc_offset" / "value").set(offset);
            return;
        }
        for (size_t c = 0; c < get_tx_num_channels(); c++){
            this->set_tx_dc_offset(offset, c);
        }
    }

    void set_tx_iq_balance(const std::complex<double> &offset, size_t chan){
        if (chan != ALL_CHANS){
            _tree->access<std::complex<double> >(tx_fe_root(chan) / "iq_balance" / "value").set(offset);
            return;
        }
        for (size_t c = 0; c < get_tx_num_channels(); c++){
            this->set_tx_iq_balance(offset, c);
        }
    }

private:
    device::sptr _dev;
    property_tree::sptr _tree;

    struct mboard_chan_pair{
        size_t mboard, chan;
        mboard_chan_pair(void): mboard(0), chan(0){}
    };

    mboard_chan_pair rx_chan_to_mcp(size_t chan){
        mboard_chan_pair mcp;
        mcp.chan = chan;
        for (mcp.mboard = 0; mcp.mboard < get_num_mboards(); mcp.mboard++){
            size_t sss = get_rx_subdev_spec(mcp.mboard).size();
            if (mcp.chan < sss) break;
            mcp.chan -= sss;
        }
        return mcp;
    }

    mboard_chan_pair tx_chan_to_mcp(size_t chan){
        mboard_chan_pair mcp;
        mcp.chan = chan;
        for (mcp.mboard = 0; mcp.mboard < get_num_mboards(); mcp.mboard++){
            size_t sss = get_tx_subdev_spec(mcp.mboard).size();
            if (mcp.chan < sss) break;
            mcp.chan -= sss;
        }
        return mcp;
    }

    fs_path mb_root(const size_t mboard){
        const std::string name = _tree->list("/mboards").at(mboard);
        return "/mboards/" + name;
    }

    fs_path rx_dsp_root(const size_t chan){
        mboard_chan_pair mcp = rx_chan_to_mcp(chan);
        const std::string name = _tree->list(mb_root(mcp.mboard) / "rx_dsps").at(mcp.chan);
        return mb_root(mcp.mboard) / "rx_dsps" / name;
    }

    fs_path tx_dsp_root(const size_t chan){
        mboard_chan_pair mcp = tx_chan_to_mcp(chan);
        const std::string name = _tree->list(mb_root(mcp.mboard) / "tx_dsps").at(mcp.chan);
        return mb_root(mcp.mboard) / "tx_dsps" / name;
    }

    fs_path rx_fe_root(const size_t chan){
        mboard_chan_pair mcp = rx_chan_to_mcp(chan);
        const subdev_spec_pair_t spec = get_rx_subdev_spec(mcp.mboard).at(mcp.chan);
        return mb_root(mcp.mboard) / "rx_frontends" / spec.db_name;
    }

    fs_path tx_fe_root(const size_t chan){
        mboard_chan_pair mcp = tx_chan_to_mcp(chan);
        const subdev_spec_pair_t spec = get_tx_subdev_spec(mcp.mboard).at(mcp.chan);
        return mb_root(mcp.mboard) / "tx_frontends" / spec.db_name;
    }

    fs_path rx_rf_fe_root(const size_t chan){
        mboard_chan_pair mcp = rx_chan_to_mcp(chan);
        const subdev_spec_pair_t spec = get_rx_subdev_spec(mcp.mboard).at(mcp.chan);
        return mb_root(mcp.mboard) / "dboards" / spec.db_name / "rx_frontends" / spec.sd_name;
    }

    fs_path tx_rf_fe_root(const size_t chan){
        mboard_chan_pair mcp = tx_chan_to_mcp(chan);
        const subdev_spec_pair_t spec = get_tx_subdev_spec(mcp.mboard).at(mcp.chan);
        return mb_root(mcp.mboard) / "dboards" / spec.db_name / "tx_frontends" / spec.sd_name;
    }

    gain_group::sptr rx_gain_group(size_t chan){
        mboard_chan_pair mcp = rx_chan_to_mcp(chan);
        const subdev_spec_pair_t spec = get_rx_subdev_spec(mcp.mboard).at(mcp.chan);
        gain_group::sptr gg = gain_group::make();
        BOOST_FOREACH(const std::string &name, _tree->list(mb_root(mcp.mboard) / "rx_codecs" / spec.db_name / "gains")){
            gg->register_fcns("ADC-"+name, make_gain_fcns_from_subtree(_tree->subtree(mb_root(mcp.mboard) / "rx_codecs" / spec.db_name / "gains" / name)), 0 /* low prio */);
        }
        BOOST_FOREACH(const std::string &name, _tree->list(rx_rf_fe_root(chan) / "gains")){
            gg->register_fcns(name, make_gain_fcns_from_subtree(_tree->subtree(rx_rf_fe_root(chan) / "gains" / name)), 1 /* high prio */);
        }
        return gg;
    }

    gain_group::sptr tx_gain_group(size_t chan){
        mboard_chan_pair mcp = tx_chan_to_mcp(chan);
        const subdev_spec_pair_t spec = get_tx_subdev_spec(mcp.mboard).at(mcp.chan);
        gain_group::sptr gg = gain_group::make();
        BOOST_FOREACH(const std::string &name, _tree->list(mb_root(mcp.mboard) / "tx_codecs" / spec.db_name / "gains")){
            gg->register_fcns("DAC-"+name, make_gain_fcns_from_subtree(_tree->subtree(mb_root(mcp.mboard) / "tx_codecs" / spec.db_name / "gains" / name)), 1 /* high prio */);
        }
        BOOST_FOREACH(const std::string &name, _tree->list(tx_rf_fe_root(chan) / "gains")){
            gg->register_fcns(name, make_gain_fcns_from_subtree(_tree->subtree(tx_rf_fe_root(chan) / "gains" / name)), 0 /* low prio */);
        }
        return gg;
    }
};

/***********************************************************************
 * The Make Function
 **********************************************************************/
multi_usrp::sptr multi_usrp::make(const device_addr_t &dev_addr){
    return sptr(new multi_usrp_impl(dev_addr));
}

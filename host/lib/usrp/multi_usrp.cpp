//
// Copyright 2010-2013 Ettus Research LLC
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
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/gain_group.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/convert.hpp>
#include <uhd/utils/soft_register.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <cmath>

using namespace uhd;
using namespace uhd::usrp;

const std::string multi_usrp::ALL_GAINS = "";

UHD_INLINE std::string string_vector_to_string(std::vector<std::string> values, std::string delimiter = std::string(" "))
{
    std::string out = "";
    for (std::vector<std::string>::iterator iter = values.begin(); iter != values.end(); iter++)
    {
        out += (iter != values.begin() ? delimiter : "") + *iter;
    }
    return out;
}

#define THROW_GAIN_NAME_ERROR(name,chan,dir) throw uhd::exception::runtime_error( \
            (boost::format("%s: gain \"%s\" not found for channel %d.\nAvailable gains: %s\n") % \
            __FUNCTION__ % name % chan % string_vector_to_string(get_##dir##_gain_names(chan))).str());

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

/*static void do_tune_freq_results_message(
    const tune_request_t &tune_req,
    const tune_result_t &tune_result,
    double actual_freq,
    const std::string &xx
){
    const double target_freq = tune_req.target_freq;
    const double clipped_target_freq = tune_result.clipped_rf_freq;
    const double target_rf_freq = tune_result.target_rf_freq;
    const double actual_rf_freq = tune_result.actual_rf_freq;
    const double target_dsp_freq = tune_result.target_dsp_freq;
    const double actual_dsp_freq = tune_result.actual_dsp_freq;

    if (tune_req.rf_freq_policy == tune_request_t::POLICY_MANUAL) return;
    if (tune_req.dsp_freq_policy == tune_request_t::POLICY_MANUAL) return;

    bool requested_freq_success = uhd::math::frequencies_are_equal(target_freq, clipped_target_freq);
    bool target_freq_success = uhd::math::frequencies_are_equal(clipped_target_freq, actual_freq);
    bool rf_lo_tune_success = uhd::math::frequencies_are_equal(target_rf_freq, actual_rf_freq);
    bool dsp_tune_success = uhd::math::frequencies_are_equal(target_dsp_freq, actual_dsp_freq);

    if(requested_freq_success and target_freq_success and rf_lo_tune_success
            and dsp_tune_success) {
        UHD_MSG(status) << boost::format(
                "Successfully tuned to %f MHz\n\n")
                % (actual_freq / 1e6);
    } else {
        boost::format base_message ("Tune Request: %f MHz\n");
        base_message % (target_freq / 1e6);
        std::string results_string = base_message.str();

        if(requested_freq_success and (not rf_lo_tune_success)) {
            boost::format rf_lo_message(
                "  The RF LO does not support the requested frequency:\n"
                "    Requested LO Frequency: %f MHz\n"
                "    RF LO Result: %f MHz\n"
                "  Attempted to use the DSP to reach the requested frequency:\n"
                "    Desired DSP Frequency: %f MHz\n"
                "    DSP Result: %f MHz\n"
                "  Successfully tuned to %f MHz\n\n");
            rf_lo_message % (target_rf_freq / 1e6) % (actual_rf_freq / 1e6)
                % (target_dsp_freq / 1e6) % (actual_dsp_freq / 1e6)
                % (actual_freq / 1e6);

            results_string += rf_lo_message.str();

            UHD_MSG(status) << results_string;

            return;
        }

        if(not requested_freq_success) {
            boost::format failure_message(
                "  The requested %s frequency is outside of the system range, and has been clipped:\n"
                "    Target Frequency: %f MHz\n"
                "    Clipped Target Frequency: %f MHz\n");
            failure_message % xx % (target_freq / 1e6) % (clipped_target_freq / 1e6);

            results_string += failure_message.str();
        }

        if(not rf_lo_tune_success) {
            boost::format rf_lo_message(
                "  The RF LO does not support the requested frequency:\n"
                "    Requested LO Frequency: %f MHz\n"
                "    RF LO Result: %f MHz\n"
                "  Attempted to use the DSP to reach the requested frequency:\n"
                "    Desired DSP Frequency: %f MHz\n"
                "    DSP Result: %f MHz\n");
            rf_lo_message % (target_rf_freq / 1e6) % (actual_rf_freq / 1e6)
                % (target_dsp_freq / 1e6) % (actual_dsp_freq / 1e6);

            results_string += rf_lo_message.str();

        } else if(not dsp_tune_success) {
            boost::format dsp_message(
                "  The DSP does not support the requested frequency:\n"
                "    Requested DSP Frequency: %f MHz\n"
                "    DSP Result: %f MHz\n");
            dsp_message % (target_dsp_freq / 1e6) % (actual_dsp_freq / 1e6);

            results_string += dsp_message.str();
        }

        if(target_freq_success) {
            boost::format success_message(
                "  Successfully tuned to %f MHz\n\n");
            success_message % (actual_freq / 1e6);

            results_string += success_message.str();
        } else {
            boost::format failure_message(
                "  Failed to tune to target frequency\n"
                "    Target Frequency: %f MHz\n"
                "    Actual Frequency: %f MHz\n\n");
            failure_message % (clipped_target_freq / 1e6) % (actual_freq / 1e6);

            results_string += failure_message.str();
        }

        UHD_MSG(warning) << results_string << std::endl;
    }
}*/

/*! The CORDIC can be used to shift the baseband below / past the tunable
 * limits of the actual RF front-end. The baseband filter, located on the
 * daughterboard, however, limits the useful instantaneous bandwidth. We
 * allow the user to tune to the edge of the filter, where the roll-off
 * begins.  This prevents the user from tuning past the point where less
 * than half of the spectrum would be useful. */
static meta_range_t make_overall_tune_range(
    const meta_range_t &fe_range,
    const meta_range_t &dsp_range,
    const double bw
){
    meta_range_t range;
    BOOST_FOREACH(const range_t &sub_range, fe_range){
        range.push_back(range_t(
            sub_range.start() + std::max(dsp_range.start(), -bw/2),
            sub_range.stop() + std::min(dsp_range.stop(), bw/2),
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
    //-- calculate the tunable frequency ranges of the system
    //------------------------------------------------------------------
    freq_range_t tune_range = make_overall_tune_range(
            rf_fe_subtree->access<meta_range_t>("freq/range").get(),
            dsp_subtree->access<meta_range_t>("freq/range").get(),
            rf_fe_subtree->access<double>("bandwidth/value").get()
        );

    freq_range_t dsp_range = dsp_subtree->access<meta_range_t>("freq/range").get();
    freq_range_t rf_range = rf_fe_subtree->access<meta_range_t>("freq/range").get();

    double clipped_requested_freq = tune_range.clip(tune_request.target_freq);

    //------------------------------------------------------------------
    //-- If the RF FE requires an LO offset, build it into the tune request
    //------------------------------------------------------------------

    /*! The automatically calculated LO offset is only used if the
     * 'use_lo_offset' field in the daughterboard property tree is set to TRUE,
     * and the tune policy is set to AUTO. To use an LO offset normally, the
     * user should specify the MANUAL tune policy and lo_offset as part of the
     * tune_request. This lo_offset is based on the requirements of the FE, and
     * does not reflect a user-requested lo_offset, which is handled later. */
    double lo_offset = 0.0;
    if (rf_fe_subtree->access<bool>("use_lo_offset").get()){
        // If the frontend has lo_offset value and range properties, trust it
        // for lo_offset
        if (rf_fe_subtree->exists("lo_offset/value")) {
            lo_offset = rf_fe_subtree->access<double>("lo_offset/value").get();
        }

        //If the local oscillator will be in the passband, use an offset.
        //But constrain the LO offset by the width of the filter bandwidth.
        const double rate = dsp_subtree->access<double>("rate/value").get();
        const double bw = rf_fe_subtree->access<double>("bandwidth/value").get();
        if (bw > rate) lo_offset = std::min((bw - rate)/2, rate/2);
    }

    //------------------------------------------------------------------
    //-- poke the tune request args into the dboard
    //------------------------------------------------------------------
    if (rf_fe_subtree->exists("tune_args")) {
        rf_fe_subtree->access<device_addr_t>("tune_args").set(tune_request.args);
    }

    //------------------------------------------------------------------
    //-- set the RF frequency depending upon the policy
    //------------------------------------------------------------------
    double target_rf_freq = 0.0;

    switch (tune_request.rf_freq_policy){
        case tune_request_t::POLICY_AUTO:
            target_rf_freq = clipped_requested_freq + lo_offset;
            break;

        case tune_request_t::POLICY_MANUAL:
            // If the rf_fe understands lo_offset settings, infer the desired
            // lo_offset and set it. Side effect: In TVRX2 for example, after
            // setting the lo_offset (if_freq) with a POLICY_MANUAL, there is no
            // way for the user to automatically get back to default if_freq
            // without deconstruct/reconstruct the rf_fe objects.
            if (rf_fe_subtree->exists("lo_offset/value")) {
                rf_fe_subtree->access<double>("lo_offset/value")
                    .set(tune_request.rf_freq - tune_request.target_freq);
            }

            target_rf_freq = rf_range.clip(tune_request.rf_freq);
            break;

        case tune_request_t::POLICY_NONE:
            break; //does not set
    }

    //------------------------------------------------------------------
    //-- Tune the RF frontend
    //------------------------------------------------------------------
    if (tune_request.rf_freq_policy != tune_request_t::POLICY_NONE) {
        rf_fe_subtree->access<double>("freq/value").set(target_rf_freq);
    }
    const double actual_rf_freq = rf_fe_subtree->access<double>("freq/value").get();

    //------------------------------------------------------------------
    //-- Set the DSP frequency depending upon the DSP frequency policy.
    //------------------------------------------------------------------
    double target_dsp_freq = 0.0;
    switch (tune_request.dsp_freq_policy) {
        case tune_request_t::POLICY_AUTO:
            /* If we are using the AUTO tuning policy, then we prevent the
             * CORDIC from spinning us outside of the range of the baseband
             * filter, regardless of what the user requested. This could happen
             * if the user requested a center frequency so far outside of the
             * tunable range of the FE that the CORDIC would spin outside the
             * filtered baseband. */
            target_dsp_freq = actual_rf_freq - clipped_requested_freq;

            //invert the sign on the dsp freq for transmit (spinning up vs down)
            target_dsp_freq *= xx_sign;

            break;

        case tune_request_t::POLICY_MANUAL:
            /* If the user has specified a manual tune policy, we will allow
             * tuning outside of the baseband filter, but will still clip the
             * target DSP frequency to within the bounds of the CORDIC to
             * prevent undefined behavior (likely an overflow). */
            target_dsp_freq = dsp_range.clip(tune_request.dsp_freq);
            break;

        case tune_request_t::POLICY_NONE:
            break; //does not set
    }

    //------------------------------------------------------------------
    //-- Tune the DSP
    //------------------------------------------------------------------
    if (tune_request.dsp_freq_policy != tune_request_t::POLICY_NONE) {
        dsp_subtree->access<double>("freq/value").set(target_dsp_freq);
    }
    const double actual_dsp_freq = dsp_subtree->access<double>("freq/value").get();

    //------------------------------------------------------------------
    //-- Load and return the tune result
    //------------------------------------------------------------------
    tune_result_t tune_result;
    tune_result.clipped_rf_freq = clipped_requested_freq;
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
        _dev = device::make(addr, device::USRP);
        _tree = _dev->get_tree();
    }

    device::sptr get_device(void){
        return _dev;
    }

    dict<std::string, std::string> get_usrp_rx_info(size_t chan){
        mboard_chan_pair mcp = rx_chan_to_mcp(chan);
        dict<std::string, std::string> usrp_info;

        mboard_eeprom_t mb_eeprom = _tree->access<mboard_eeprom_t>(mb_root(mcp.mboard) / "eeprom").get();
        dboard_eeprom_t db_eeprom = _tree->access<dboard_eeprom_t>(rx_rf_fe_root(chan).branch_path().branch_path() / "rx_eeprom").get();

        usrp_info["mboard_id"] = _tree->access<std::string>(mb_root(mcp.mboard) / "name").get();
        usrp_info["mboard_name"] = mb_eeprom["name"];
        usrp_info["mboard_serial"] = mb_eeprom["serial"];
        usrp_info["rx_id"] = db_eeprom.id.to_pp_string();
        usrp_info["rx_subdev_name"] = _tree->access<std::string>(rx_rf_fe_root(chan) / "name").get();
        usrp_info["rx_subdev_spec"] = _tree->access<subdev_spec_t>(mb_root(mcp.mboard) / "rx_subdev_spec").get().to_string();
        usrp_info["rx_serial"] = db_eeprom.serial;
        usrp_info["rx_antenna"] =  _tree->access<std::string>(rx_rf_fe_root(chan) / "antenna" / "value").get();

        return usrp_info;
    }

    dict<std::string, std::string> get_usrp_tx_info(size_t chan){
        mboard_chan_pair mcp = tx_chan_to_mcp(chan);
        dict<std::string, std::string> usrp_info;

        mboard_eeprom_t mb_eeprom = _tree->access<mboard_eeprom_t>(mb_root(mcp.mboard) / "eeprom").get();
        dboard_eeprom_t db_eeprom = _tree->access<dboard_eeprom_t>(tx_rf_fe_root(chan).branch_path().branch_path() / "tx_eeprom").get();

        usrp_info["mboard_id"] = _tree->access<std::string>(mb_root(mcp.mboard) / "name").get();
        usrp_info["mboard_name"] = mb_eeprom["name"];
        usrp_info["mboard_serial"] = mb_eeprom["serial"];
        usrp_info["tx_id"] = db_eeprom.id.to_pp_string();
        usrp_info["tx_subdev_name"] = _tree->access<std::string>(tx_rf_fe_root(chan) / "name").get();
        usrp_info["tx_subdev_spec"] = _tree->access<subdev_spec_t>(mb_root(mcp.mboard) / "tx_subdev_spec").get().to_string();
        usrp_info["tx_serial"] = db_eeprom.serial;
        usrp_info["tx_antenna"] = _tree->access<std::string>(tx_rf_fe_root(chan) / "antenna" / "value").get();

        return usrp_info;
    }

    /*******************************************************************
     * Mboard methods
     ******************************************************************/
    void set_master_clock_rate(double rate, size_t mboard){
        if (mboard != ALL_MBOARDS){
            if (_tree->exists(mb_root(mboard) / "auto_tick_rate")) {
                _tree->access<bool>(mb_root(mboard) / "auto_tick_rate").set(false);
            }
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
        boost::system_time end_time = boost::get_system_time() + boost::posix_time::milliseconds(1100);
        time_spec_t time_start_last_pps = get_time_last_pps();
        while (time_start_last_pps == get_time_last_pps())
        {
            if (boost::get_system_time() > end_time)
            {
                throw uhd::runtime_error(
                    "Board 0 may not be getting a PPS signal!\n"
                    "No PPS detected within the time interval.\n"
                    "See the application notes for your device.\n"
                );
            }
            boost::this_thread::sleep(boost::posix_time::milliseconds(1));
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

    void set_clock_source_out(const bool enb, const size_t mboard)
    {
        if (mboard != ALL_MBOARDS)
        {
            if (_tree->exists(mb_root(mboard) / "clock_source" / "output"))
            {
                _tree->access<bool>(mb_root(mboard) / "clock_source" / "output").set(enb);
            }
            else
            {
                throw uhd::runtime_error("multi_usrp::set_clock_source_out - not supported on this device");
            }
            return;
        }
        for (size_t m = 0; m < get_num_mboards(); m++)
        {
            this->set_clock_source_out(enb, m);
        }
    }

    void set_time_source_out(const bool enb, const size_t mboard)
    {
        if (mboard != ALL_MBOARDS)
        {
            if (_tree->exists(mb_root(mboard) / "time_source" / "output"))
            {
                _tree->access<bool>(mb_root(mboard) / "time_source" / "output").set(enb);
            }
            else
            {
                throw uhd::runtime_error("multi_usrp::set_time_source_out - not supported on this device");
            }
            return;
        }
        for (size_t m = 0; m < get_num_mboards(); m++)
        {
            this->set_time_source_out(enb, m);
        }
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
    rx_streamer::sptr get_rx_stream(const stream_args_t &args) {
        _check_link_rate(args, false);
        return this->get_device()->get_rx_stream(args);
    }

    void set_rx_subdev_spec(const subdev_spec_t &spec, size_t mboard){
        if (mboard != ALL_MBOARDS){
            _tree->access<subdev_spec_t>(mb_root(mboard) / "rx_subdev_spec").set(spec);
            return;
        }
        for (size_t m = 0; m < get_num_mboards(); m++){
            set_rx_subdev_spec(spec, m);
        }
    }

    subdev_spec_t get_rx_subdev_spec(size_t mboard)
    {
        subdev_spec_t spec = _tree->access<subdev_spec_t>(mb_root(mboard) / "rx_subdev_spec").get();
        if (spec.empty())
        {
            try
            {
                const std::string db_name = _tree->list(mb_root(mboard) / "dboards").at(0);
                const std::string fe_name = _tree->list(mb_root(mboard) / "dboards" / db_name / "rx_frontends").at(0);
                spec.push_back(subdev_spec_pair_t(db_name, fe_name));
                _tree->access<subdev_spec_t>(mb_root(mboard) / "rx_subdev_spec").set(spec);
            }
            catch(const std::exception &e)
            {
                throw uhd::index_error(str(boost::format("multi_usrp::get_rx_subdev_spec(%u) failed to make default spec - %s") % mboard % e.what()));
            }
            UHD_MSG(status) << "Selecting default RX front end spec: " << spec.to_pp_string() << std::endl;
        }
        return spec;
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
        tune_result_t result = tune_xx_subdev_and_dsp(RX_SIGN,
                _tree->subtree(rx_dsp_root(chan)),
                _tree->subtree(rx_rf_fe_root(chan)),
                tune_request);
        //do_tune_freq_results_message(tune_request, result, get_rx_freq(chan), "RX");
        return result;
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
        /* Check if any AGC mode is enable and if so warn the user */
        if (chan != ALL_CHANS) {
            if (_tree->exists(rx_rf_fe_root(chan) / "gain" / "agc")) {
                bool agc = _tree->access<bool>(rx_rf_fe_root(chan) / "gain" / "agc" / "enable").get();
                if(agc) {
                    UHD_MSG(warning) << "AGC enabled for this channel. Setting will be ignored." << std::endl;
                }
            }
        } else {
            for (size_t c = 0; c < get_rx_num_channels(); c++){
                if (_tree->exists(rx_rf_fe_root(c) / "gain" / "agc")) {
                    bool agc = _tree->access<bool>(rx_rf_fe_root(chan) / "gain" / "agc" / "enable").get();
                    if(agc) {
                        UHD_MSG(warning) << "AGC enabled for this channel. Setting will be ignored." << std::endl;
                    }
                }
            }
        }
        /* Apply gain setting.
         * If device is in AGC mode it will ignore the setting. */
        try {
            return rx_gain_group(chan)->set_value(gain, name);
        } catch (uhd::key_error &) {
            THROW_GAIN_NAME_ERROR(name,chan,rx);
        }
    }

    void set_normalized_rx_gain(double gain, size_t chan = 0)
    {
      if (gain > 1.0 || gain < 0.0) {
        throw uhd::runtime_error("Normalized gain out of range, must be in [0, 1].");
      }
      gain_range_t gain_range = get_rx_gain_range(ALL_GAINS, chan);
      double abs_gain = (gain * (gain_range.stop() - gain_range.start())) + gain_range.start();
      set_rx_gain(abs_gain, ALL_GAINS, chan);
    }

    void set_rx_agc(bool enable, size_t chan = 0)
    {
        if (chan != ALL_CHANS){
            if (_tree->exists(rx_rf_fe_root(chan) / "gain" / "agc" / "enable")) {
                _tree->access<bool>(rx_rf_fe_root(chan) / "gain" / "agc" / "enable").set(enable);
            } else {
                UHD_MSG(warning) << "AGC is not available on this device." << std::endl;
            }
            return;
        }
        for (size_t c = 0; c < get_rx_num_channels(); c++){
            this->set_rx_agc(enable, c);
        }

    }

    double get_rx_gain(const std::string &name, size_t chan){
        try {
            return rx_gain_group(chan)->get_value(name);
        } catch (uhd::key_error &) {
            THROW_GAIN_NAME_ERROR(name,chan,rx);
        }
    }

    double get_normalized_rx_gain(size_t chan)
    {
      gain_range_t gain_range = get_rx_gain_range(ALL_GAINS, chan);
      double gain_range_width = gain_range.stop() - gain_range.start();
      // In case we have a device without a range of gains:
      if (gain_range_width == 0.0) {
          return 0;
      }
      double norm_gain = (get_rx_gain(ALL_GAINS, chan) - gain_range.start()) / gain_range_width;
      // Avoid rounding errors:
      if (norm_gain > 1.0) return 1.0;
      if (norm_gain < 0.0) return 0.0;
      return norm_gain;
    }

    gain_range_t get_rx_gain_range(const std::string &name, size_t chan){
        try {
            return rx_gain_group(chan)->get_range(name);
        } catch (uhd::key_error &) {
            THROW_GAIN_NAME_ERROR(name,chan,rx);
        }
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
            if (_tree->exists(rx_fe_root(chan) / "dc_offset" / "enable")) {
                _tree->access<bool>(rx_fe_root(chan) / "dc_offset" / "enable").set(enb);
            } else if (_tree->exists(rx_rf_fe_root(chan) / "dc_offset" / "enable")) {
                /*For B2xx devices the dc-offset correction is implemented in the rf front-end*/
                _tree->access<bool>(rx_rf_fe_root(chan) / "dc_offset" / "enable").set(enb);
            } else {
                UHD_MSG(warning) << "Setting DC offset compensation is not possible on this device." << std::endl;
            }
            return;
        }
        for (size_t c = 0; c < get_rx_num_channels(); c++){
            this->set_rx_dc_offset(enb, c);
        }
    }

    void set_rx_dc_offset(const std::complex<double> &offset, size_t chan){
        if (chan != ALL_CHANS){
            if (_tree->exists(rx_fe_root(chan) / "dc_offset" / "value")) {
                _tree->access<std::complex<double> >(rx_fe_root(chan) / "dc_offset" / "value").set(offset);
            } else {
                UHD_MSG(warning) << "Setting DC offset is not possible on this device." << std::endl;
            }
            return;
        }
        for (size_t c = 0; c < get_rx_num_channels(); c++){
            this->set_rx_dc_offset(offset, c);
        }
    }

    void set_rx_iq_balance(const bool enb, size_t chan){
        if (chan != ALL_CHANS){
            if (_tree->exists(rx_rf_fe_root(chan) / "iq_balance" / "enable")) {
                _tree->access<bool>(rx_rf_fe_root(chan) / "iq_balance" / "enable").set(enb);
            } else {
                UHD_MSG(warning) << "Setting IQ imbalance compensation is not possible on this device." << std::endl;
            }
            return;
        }
        for (size_t c = 0; c < get_rx_num_channels(); c++){
            this->set_rx_iq_balance(enb, c);
        }
    }

    void set_rx_iq_balance(const std::complex<double> &offset, size_t chan){
        if (chan != ALL_CHANS){
            if (_tree->exists(rx_fe_root(chan) / "iq_balance" / "value")) {
                _tree->access<std::complex<double> >(rx_fe_root(chan) / "iq_balance" / "value").set(offset);
            } else {
                UHD_MSG(warning) << "Setting IQ balance is not possible on this device." << std::endl;
            }
            return;
        }
        for (size_t c = 0; c < get_rx_num_channels(); c++){
            this->set_rx_iq_balance(offset, c);
        }
    }

    std::vector<std::string> get_filter_names(const std::string &search_mask)
    {
        std::vector<std::string> ret;

        for (size_t chan = 0; chan < get_rx_num_channels(); chan++){

            if (_tree->exists(rx_rf_fe_root(chan) / "filters")) {
                std::vector<std::string> names = _tree->list(rx_rf_fe_root(chan) / "filters");
                for(size_t i = 0; i < names.size(); i++)
                {
                    std::string name = rx_rf_fe_root(chan) / "filters" / names[i];
                    if((search_mask.empty()) or boost::contains(name, search_mask)) {
                        ret.push_back(name);
                    }
                }
            }
            if (_tree->exists(rx_dsp_root(chan) / "filters")) {
                std::vector<std::string> names = _tree->list(rx_dsp_root(chan) / "filters");
                for(size_t i = 0; i < names.size(); i++)
                {
                    std::string name = rx_dsp_root(chan) / "filters" / names[i];
                    if((search_mask.empty()) or (boost::contains(name, search_mask))) {
                        ret.push_back(name);
                    }
                }
            }

        }

        for (size_t chan = 0; chan < get_tx_num_channels(); chan++){

            if (_tree->exists(tx_rf_fe_root(chan) / "filters")) {
                std::vector<std::string> names = _tree->list(tx_rf_fe_root(chan) / "filters");
                for(size_t i = 0; i < names.size(); i++)
                {
                    std::string name = tx_rf_fe_root(chan) / "filters" / names[i];
                    if((search_mask.empty()) or (boost::contains(name, search_mask))) {
                        ret.push_back(name);
                    }
                }
            }
            if (_tree->exists(rx_dsp_root(chan) / "filters")) {
                std::vector<std::string> names = _tree->list(tx_dsp_root(chan) / "filters");
                for(size_t i = 0; i < names.size(); i++)
                {
                    std::string name = tx_dsp_root(chan) / "filters" / names[i];
                    if((search_mask.empty()) or (boost::contains(name, search_mask))) {
                        ret.push_back(name);
                    }
                }
            }

        }

        return ret;
    }

    filter_info_base::sptr get_filter(const std::string &path)
    {
        std::vector<std::string> possible_names = get_filter_names("");
        std::vector<std::string>::iterator it;
        it = find(possible_names.begin(), possible_names.end(), path);
        if (it == possible_names.end()) {
            throw uhd::runtime_error("Attempting to get non-existing filter: "+path);
        }

        return _tree->access<filter_info_base::sptr>(path / "value").get();
    }

    void set_filter(const std::string &path, filter_info_base::sptr filter)
    {
        std::vector<std::string> possible_names = get_filter_names("");
        std::vector<std::string>::iterator it;
        it = find(possible_names.begin(), possible_names.end(), path);
        if (it == possible_names.end()) {
            throw uhd::runtime_error("Attempting to set non-existing filter: "+path);
        }

        _tree->access<filter_info_base::sptr>(path / "value").set(filter);
    }

    /*******************************************************************
     * TX methods
     ******************************************************************/
    tx_streamer::sptr get_tx_stream(const stream_args_t &args) {
        _check_link_rate(args, true);
        return this->get_device()->get_tx_stream(args);
    }

    void set_tx_subdev_spec(const subdev_spec_t &spec, size_t mboard){
        if (mboard != ALL_MBOARDS){
            _tree->access<subdev_spec_t>(mb_root(mboard) / "tx_subdev_spec").set(spec);
            return;
        }
        for (size_t m = 0; m < get_num_mboards(); m++){
            set_tx_subdev_spec(spec, m);
        }
    }

    subdev_spec_t get_tx_subdev_spec(size_t mboard)
    {
        subdev_spec_t spec = _tree->access<subdev_spec_t>(mb_root(mboard) / "tx_subdev_spec").get();
        if (spec.empty())
        {
            try
            {
                const std::string db_name = _tree->list(mb_root(mboard) / "dboards").at(0);
                const std::string fe_name = _tree->list(mb_root(mboard) / "dboards" / db_name / "tx_frontends").at(0);
                spec.push_back(subdev_spec_pair_t(db_name, fe_name));
                _tree->access<subdev_spec_t>(mb_root(mboard) / "tx_subdev_spec").set(spec);
            }
            catch(const std::exception &e)
            {
                throw uhd::index_error(str(boost::format("multi_usrp::get_tx_subdev_spec(%u) failed to make default spec - %s") % mboard % e.what()));
            }
            UHD_MSG(status) << "Selecting default TX front end spec: " << spec.to_pp_string() << std::endl;
        }
        return spec;
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
        tune_result_t result = tune_xx_subdev_and_dsp(TX_SIGN,
                _tree->subtree(tx_dsp_root(chan)),
                _tree->subtree(tx_rf_fe_root(chan)),
                tune_request);
        //do_tune_freq_results_message(tune_request, result, get_tx_freq(chan), "TX");
        return result;
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
        try {
            return tx_gain_group(chan)->set_value(gain, name);
        } catch (uhd::key_error &) {
            THROW_GAIN_NAME_ERROR(name,chan,tx);
        }
    }

    void set_normalized_tx_gain(double gain, size_t chan = 0)
    {
      if (gain > 1.0 || gain < 0.0) {
        throw uhd::runtime_error("Normalized gain out of range, must be in [0, 1].");
      }
      gain_range_t gain_range = get_tx_gain_range(ALL_GAINS, chan);
      double abs_gain = (gain * (gain_range.stop() - gain_range.start())) + gain_range.start();
      set_tx_gain(abs_gain, ALL_GAINS, chan);
    }


    double get_tx_gain(const std::string &name, size_t chan){
        try {
            return tx_gain_group(chan)->get_value(name);
        } catch (uhd::key_error &) {
            THROW_GAIN_NAME_ERROR(name,chan,tx);
        }
    }

    double get_normalized_tx_gain(size_t chan)
    {
      gain_range_t gain_range = get_tx_gain_range(ALL_GAINS, chan);
      double gain_range_width = gain_range.stop() - gain_range.start();
      // In case we have a device without a range of gains:
      if (gain_range_width == 0.0) {
          return 0.0;
      }
      double norm_gain = (get_rx_gain(ALL_GAINS, chan) - gain_range.start()) / gain_range_width;
      // Avoid rounding errors:
      if (norm_gain > 1.0) return 1.0;
      if (norm_gain < 0.0) return 0.0;
      return norm_gain;
    }

    gain_range_t get_tx_gain_range(const std::string &name, size_t chan){
        try {
            return tx_gain_group(chan)->get_range(name);
        } catch (uhd::key_error &) {
            THROW_GAIN_NAME_ERROR(name,chan,tx);
        }
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
            if (_tree->exists(tx_fe_root(chan) / "dc_offset" / "value")) {
                _tree->access<std::complex<double> >(tx_fe_root(chan) / "dc_offset" / "value").set(offset);
            } else {
                UHD_MSG(warning) << "Setting DC offset is not possible on this device." << std::endl;
            }
            return;
        }
        for (size_t c = 0; c < get_tx_num_channels(); c++){
            this->set_tx_dc_offset(offset, c);
        }
    }

    void set_tx_iq_balance(const std::complex<double> &offset, size_t chan){
        if (chan != ALL_CHANS){
            if (_tree->exists(tx_fe_root(chan) / "iq_balance" / "value")) {
                _tree->access<std::complex<double> >(tx_fe_root(chan) / "iq_balance" / "value").set(offset);
            } else {
                UHD_MSG(warning) << "Setting IQ balance is not possible on this device." << std::endl;
            }
            return;
        }
        for (size_t c = 0; c < get_tx_num_channels(); c++){
            this->set_tx_iq_balance(offset, c);
        }
    }

    /*******************************************************************
     * GPIO methods
     ******************************************************************/
    std::vector<std::string> get_gpio_banks(const size_t mboard)
    {
        std::vector<std::string> banks;
        if (_tree->exists(mb_root(mboard) / "gpio"))
        {
            BOOST_FOREACH(const std::string &name, _tree->list(mb_root(mboard) / "gpio"))
            {
                banks.push_back(name);
            }
        }
        BOOST_FOREACH(const std::string &name, _tree->list(mb_root(mboard) / "dboards"))
        {
            banks.push_back("RX"+name);
            banks.push_back("TX"+name);
        }
        return banks;
    }

    void set_gpio_attr(const std::string &bank, const std::string &attr, const boost::uint32_t value, const boost::uint32_t mask, const size_t mboard)
    {
        if (_tree->exists(mb_root(mboard) / "gpio" / bank))
        {
            const boost::uint32_t current = _tree->access<boost::uint32_t>(mb_root(mboard) / "gpio" / bank / attr).get();
            const boost::uint32_t new_value = (current & ~mask) | (value & mask);
            _tree->access<boost::uint32_t>(mb_root(mboard) / "gpio" / bank / attr).set(new_value);
            return;
        }
        if (bank.size() > 2 and bank[1] == 'X')
        {
            const std::string name = bank.substr(2);
            const dboard_iface::unit_t unit = (bank[0] == 'R')? dboard_iface::UNIT_RX : dboard_iface::UNIT_TX;
            dboard_iface::sptr iface = _tree->access<dboard_iface::sptr>(mb_root(mboard) / "dboards" / name / "iface").get();
            if (attr == "CTRL") iface->set_pin_ctrl(unit, boost::uint16_t(value), boost::uint16_t(mask));
            if (attr == "DDR") iface->set_gpio_ddr(unit, boost::uint16_t(value), boost::uint16_t(mask));
            if (attr == "OUT") iface->set_gpio_out(unit, boost::uint16_t(value), boost::uint16_t(mask));
            if (attr == "ATR_0X") iface->set_atr_reg(unit, dboard_iface::ATR_REG_IDLE, boost::uint16_t(value), boost::uint16_t(mask));
            if (attr == "ATR_RX") iface->set_atr_reg(unit, dboard_iface::ATR_REG_RX_ONLY, boost::uint16_t(value), boost::uint16_t(mask));
            if (attr == "ATR_TX") iface->set_atr_reg(unit, dboard_iface::ATR_REG_TX_ONLY, boost::uint16_t(value), boost::uint16_t(mask));
            if (attr == "ATR_XX") iface->set_atr_reg(unit, dboard_iface::ATR_REG_FULL_DUPLEX, boost::uint16_t(value), boost::uint16_t(mask));
        }
    }

    boost::uint32_t get_gpio_attr(const std::string &bank, const std::string &attr, const size_t mboard)
    {
        if (_tree->exists(mb_root(mboard) / "gpio" / bank))
        {
            return boost::uint32_t(_tree->access<boost::uint64_t>(mb_root(mboard) / "gpio" / bank / attr).get());
        }
        if (bank.size() > 2 and bank[1] == 'X')
        {
            const std::string name = bank.substr(2);
            const dboard_iface::unit_t unit = (bank[0] == 'R')? dboard_iface::UNIT_RX : dboard_iface::UNIT_TX;
            dboard_iface::sptr iface = _tree->access<dboard_iface::sptr>(mb_root(mboard) / "dboards" / name / "iface").get();
            if (attr == "CTRL") return iface->get_pin_ctrl(unit);
            if (attr == "DDR") return iface->get_gpio_ddr(unit);
            if (attr == "OUT") return iface->get_gpio_out(unit);
            if (attr == "ATR_0X") return iface->get_atr_reg(unit, dboard_iface::ATR_REG_IDLE);
            if (attr == "ATR_RX") return iface->get_atr_reg(unit, dboard_iface::ATR_REG_RX_ONLY);
            if (attr == "ATR_TX") return iface->get_atr_reg(unit, dboard_iface::ATR_REG_TX_ONLY);
            if (attr == "ATR_XX") return iface->get_atr_reg(unit, dboard_iface::ATR_REG_FULL_DUPLEX);
            if (attr == "READBACK") return iface->read_gpio(unit);
        }
        return 0;
    }

    void write_register(const std::string &path, const boost::uint32_t field, const boost::uint64_t value, const size_t mboard)
    {
        if (_tree->exists(mb_root(mboard) / "registers"))
        {
            uhd::soft_regmap_accessor_t::sptr accessor =
                _tree->access<uhd::soft_regmap_accessor_t::sptr>(mb_root(mboard) / "registers").get();
            uhd::soft_register_base& reg = accessor->lookup(path);

            if (not reg.is_writable()) {
                throw uhd::runtime_error("multi_usrp::write_register - register not writable: " + path);
            }

            switch (reg.get_bitwidth()) {
            case 16:
                if (reg.is_readable())
                    uhd::soft_register_base::cast<uhd::soft_reg16_rw_t>(reg).write(field, static_cast<boost::uint16_t>(value));
                else
                    uhd::soft_register_base::cast<uhd::soft_reg16_wo_t>(reg).write(field, static_cast<boost::uint16_t>(value));
            break;

            case 32:
                if (reg.is_readable())
                    uhd::soft_register_base::cast<uhd::soft_reg32_rw_t>(reg).write(field, static_cast<boost::uint32_t>(value));
                else
                    uhd::soft_register_base::cast<uhd::soft_reg32_wo_t>(reg).write(field, static_cast<boost::uint32_t>(value));
            break;

            case 64:
                if (reg.is_readable())
                    uhd::soft_register_base::cast<uhd::soft_reg64_rw_t>(reg).write(field, value);
                else
                    uhd::soft_register_base::cast<uhd::soft_reg64_wo_t>(reg).write(field, value);
            break;

            default:
                throw uhd::assertion_error("multi_usrp::write_register - register has invalid bitwidth");
            }

        } else {
            throw uhd::not_implemented_error("multi_usrp::write_register - register IO not supported for this device");
        }
    }

    boost::uint64_t read_register(const std::string &path, const boost::uint32_t field, const size_t mboard)
    {
        if (_tree->exists(mb_root(mboard) / "registers"))
        {
            uhd::soft_regmap_accessor_t::sptr accessor =
                _tree->access<uhd::soft_regmap_accessor_t::sptr>(mb_root(mboard) / "registers").get();
            uhd::soft_register_base& reg = accessor->lookup(path);

            if (not reg.is_readable()) {
                throw uhd::runtime_error("multi_usrp::read_register - register not readable: " + path);
            }

            switch (reg.get_bitwidth()) {
            case 16:
                if (reg.is_writable())
                    return static_cast<boost::uint64_t>(uhd::soft_register_base::cast<uhd::soft_reg16_rw_t>(reg).read(field));
                else
                    return static_cast<boost::uint64_t>(uhd::soft_register_base::cast<uhd::soft_reg16_ro_t>(reg).read(field));
            break;

            case 32:
                if (reg.is_writable())
                    return static_cast<boost::uint64_t>(uhd::soft_register_base::cast<uhd::soft_reg32_rw_t>(reg).read(field));
                else
                    return static_cast<boost::uint64_t>(uhd::soft_register_base::cast<uhd::soft_reg32_ro_t>(reg).read(field));
            break;

            case 64:
                if (reg.is_writable())
                    return uhd::soft_register_base::cast<uhd::soft_reg64_rw_t>(reg).read(field);
                else
                    return uhd::soft_register_base::cast<uhd::soft_reg64_ro_t>(reg).read(field);
            break;

            default:
                throw uhd::assertion_error("multi_usrp::read_register - register has invalid bitwidth: " + path);
            }
        } else {
            throw uhd::not_implemented_error("multi_usrp::read_register - register IO not supported for this device");
        }
    }

    std::vector<std::string> enumerate_registers(const size_t mboard)
    {
        if (_tree->exists(mb_root(mboard) / "registers"))
        {
            uhd::soft_regmap_accessor_t::sptr accessor =
                _tree->access<uhd::soft_regmap_accessor_t::sptr>(mb_root(mboard) / "registers").get();
            return accessor->enumerate();
        } else {
            return std::vector<std::string>();
        }
    }

    register_info_t get_register_info(const std::string &path, const size_t mboard = 0)
    {
        if (_tree->exists(mb_root(mboard) / "registers"))
        {
            uhd::soft_regmap_accessor_t::sptr accessor =
                _tree->access<uhd::soft_regmap_accessor_t::sptr>(mb_root(mboard) / "registers").get();
            uhd::soft_register_base& reg = accessor->lookup(path);

            register_info_t info;
            info.bitwidth = reg.get_bitwidth();
            info.readable = reg.is_readable();
            info.writable = reg.is_writable();
            return info;
        } else {
            throw uhd::not_implemented_error("multi_usrp::read_register - register IO not supported for this device");
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
        if (mcp.mboard >= get_num_mboards())
        {
            throw uhd::index_error(str(boost::format("multi_usrp: RX channel %u out of range for configured RX frontends") % chan));
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
        if (mcp.mboard >= get_num_mboards())
        {
            throw uhd::index_error(str(boost::format("multi_usrp: TX channel %u out of range for configured TX frontends") % chan));
        }
        return mcp;
    }

    fs_path mb_root(const size_t mboard)
    {
        try
        {
            const std::string name = _tree->list("/mboards").at(mboard);
            return "/mboards/" + name;
        }
        catch(const std::exception &e)
        {
            throw uhd::index_error(str(boost::format("multi_usrp::mb_root(%u) - %s") % mboard % e.what()));
        }
    }

    fs_path rx_dsp_root(const size_t chan)
    {
        mboard_chan_pair mcp = rx_chan_to_mcp(chan);
        if (_tree->exists(mb_root(mcp.mboard) / "rx_chan_dsp_mapping")) {
            std::vector<size_t> map = _tree->access<std::vector<size_t> >(mb_root(mcp.mboard) / "rx_chan_dsp_mapping").get();
            UHD_ASSERT_THROW(map.size() > mcp.chan);
            mcp.chan = map[mcp.chan];
        }

        try
        {
            const std::string name = _tree->list(mb_root(mcp.mboard) / "rx_dsps").at(mcp.chan);
            return mb_root(mcp.mboard) / "rx_dsps" / name;
        }
        catch(const std::exception &e)
        {
            throw uhd::index_error(str(boost::format("multi_usrp::rx_dsp_root(%u) - mcp(%u) - %s") % chan % mcp.chan % e.what()));
        }
    }

    fs_path tx_dsp_root(const size_t chan)
    {
        mboard_chan_pair mcp = tx_chan_to_mcp(chan);
        if (_tree->exists(mb_root(mcp.mboard) / "tx_chan_dsp_mapping")) {
            std::vector<size_t> map = _tree->access<std::vector<size_t> >(mb_root(mcp.mboard) / "tx_chan_dsp_mapping").get();
            UHD_ASSERT_THROW(map.size() > mcp.chan);
            mcp.chan = map[mcp.chan];
        }
        try
        {
            const std::string name = _tree->list(mb_root(mcp.mboard) / "tx_dsps").at(mcp.chan);
            return mb_root(mcp.mboard) / "tx_dsps" / name;
        }
        catch(const std::exception &e)
        {
            throw uhd::index_error(str(boost::format("multi_usrp::tx_dsp_root(%u) - mcp(%u) - %s") % chan % mcp.chan % e.what()));
        }
    }

    fs_path rx_fe_root(const size_t chan)
    {
        mboard_chan_pair mcp = rx_chan_to_mcp(chan);
        try
        {
            const subdev_spec_pair_t spec = get_rx_subdev_spec(mcp.mboard).at(mcp.chan);
            return mb_root(mcp.mboard) / "rx_frontends" / spec.db_name;
        }
        catch(const std::exception &e)
        {
            throw uhd::index_error(str(boost::format("multi_usrp::rx_fe_root(%u) - mcp(%u) - %s") % chan % mcp.chan % e.what()));
        }
    }

    fs_path tx_fe_root(const size_t chan)
    {
        mboard_chan_pair mcp = tx_chan_to_mcp(chan);
        try
        {
            const subdev_spec_pair_t spec = get_tx_subdev_spec(mcp.mboard).at(mcp.chan);
            return mb_root(mcp.mboard) / "tx_frontends" / spec.db_name;
        }
        catch(const std::exception &e)
        {
            throw uhd::index_error(str(boost::format("multi_usrp::tx_fe_root(%u) - mcp(%u) - %s") % chan % mcp.chan % e.what()));
        }
    }

    fs_path rx_rf_fe_root(const size_t chan)
    {
        mboard_chan_pair mcp = rx_chan_to_mcp(chan);
        try
        {
            const subdev_spec_pair_t spec = get_rx_subdev_spec(mcp.mboard).at(mcp.chan);
            return mb_root(mcp.mboard) / "dboards" / spec.db_name / "rx_frontends" / spec.sd_name;
        }
        catch(const std::exception &e)
        {
            throw uhd::index_error(str(boost::format("multi_usrp::rx_rf_fe_root(%u) - mcp(%u) - %s") % chan % mcp.chan % e.what()));
        }
    }

    fs_path tx_rf_fe_root(const size_t chan)
    {
        mboard_chan_pair mcp = tx_chan_to_mcp(chan);
        try
        {
            const subdev_spec_pair_t spec = get_tx_subdev_spec(mcp.mboard).at(mcp.chan);
            return mb_root(mcp.mboard) / "dboards" / spec.db_name / "tx_frontends" / spec.sd_name;
        }
        catch(const std::exception &e)
        {
            throw uhd::index_error(str(boost::format("multi_usrp::tx_rf_fe_root(%u) - mcp(%u) - %s") % chan % mcp.chan % e.what()));
        }
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

    //! \param is_tx True for tx
    // Assumption is that all mboards use the same link
    bool _check_link_rate(const stream_args_t &args, bool is_tx) {
        bool link_rate_is_ok = true;
        size_t bytes_per_sample = convert::get_bytes_per_item(args.otw_format.empty() ? "sc16" : args.otw_format);
        double max_link_rate = 0;
        double sum_rate = 0;
        BOOST_FOREACH(const size_t chan, args.channels) {
            mboard_chan_pair mcp = is_tx ? tx_chan_to_mcp(chan) : rx_chan_to_mcp(chan);
            if (_tree->exists(mb_root(mcp.mboard) / "link_max_rate")) {
                max_link_rate = std::max(
                    max_link_rate,
                   _tree->access<double>(mb_root(mcp.mboard) / "link_max_rate").get()
                );
            }
            sum_rate += is_tx ? get_tx_rate(chan) : get_rx_rate(chan);
        }
        if (max_link_rate > 0 and (max_link_rate / bytes_per_sample) < sum_rate) {
            UHD_MSG(warning) << boost::format(
                "The total sum of rates (%f MSps on %u channels) exceeds the maximum capacity of the connection.\n"
                "This can cause %s."
            ) % (sum_rate/1e6) % args.channels.size() % (is_tx ? "underruns (U)" : "overflows (O)")  << std::endl;
            link_rate_is_ok = false;
        }

        return link_rate_is_ok;
    }
};

multi_usrp::~multi_usrp(void){
    /* NOP */
}

/***********************************************************************
 * The Make Function
 **********************************************************************/
multi_usrp::sptr multi_usrp::make(const device_addr_t &dev_addr){
    UHD_LOG << "multi_usrp::make with args " << dev_addr.to_pp_string() << std::endl;
    return sptr(new multi_usrp_impl(dev_addr));
}

//
// Copyright 2010-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/property_tree.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/usrp/gpio_defs.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/gain_group.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/convert.hpp>
#include <uhd/utils/soft_register.hpp>
#include <uhdlib/usrp/gpio_defs.hpp>
#include <uhdlib/rfnoc/legacy_compat.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <cmath>
#include <bitset>
#include <chrono>
#include <thread>

using namespace uhd;
using namespace uhd::usrp;

const std::string multi_usrp::ALL_GAINS = "";
const std::string multi_usrp::ALL_LOS = "all";

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
        UHD_LOGGER_WARNING("MULTI_USRP") << boost::format(
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
        UHD_LOGGER_INFO("MULTI_USRP") << boost::format(
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

            UHD_LOGGER_INFO("MULTI_USRP") << results_string;

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

        UHD_LOGGER_WARNING("MULTI_USRP") << results_string ;
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
    for(const range_t &sub_range:  fe_range){
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
    if (rf_fe_subtree->exists("use_lo_offset") and
        rf_fe_subtree->access<bool>("use_lo_offset").get()){
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
        _is_device3 = bool(boost::dynamic_pointer_cast<uhd::device3>(_dev));

        if (is_device3()) {
            _legacy_compat = rfnoc::legacy_compat::make(get_device3(), addr);
        }
    }

    device::sptr get_device(void){
        return _dev;
    }

    bool is_device3(void) {
        return _is_device3;
    }

    device3::sptr get_device3(void) {
        if (not is_device3()) {
            throw uhd::type_error("Cannot call get_device3() on a non-generation 3 device.");
        }
        return boost::dynamic_pointer_cast<uhd::device3>(_dev);
    }

    dict<std::string, std::string> get_usrp_rx_info(size_t chan){
        mboard_chan_pair mcp = rx_chan_to_mcp(chan);
        dict<std::string, std::string> usrp_info;
        const auto mb_eeprom =
            _tree->access<mboard_eeprom_t>(mb_root(mcp.mboard) / "eeprom").get();
        usrp_info["mboard_id"] = _tree->access<std::string>(mb_root(mcp.mboard) / "name").get();
        usrp_info["mboard_name"] = mb_eeprom.get("name", "n/a");
        usrp_info["mboard_serial"] = mb_eeprom["serial"];
        usrp_info["rx_subdev_name"] = _tree->access<std::string>(rx_rf_fe_root(chan) / "name").get();
        usrp_info["rx_subdev_spec"] = _tree->access<subdev_spec_t>(mb_root(mcp.mboard) / "rx_subdev_spec").get().to_string();
        usrp_info["rx_antenna"] =  _tree->access<std::string>(rx_rf_fe_root(chan) / "antenna" / "value").get();
        if (_tree->exists(rx_rf_fe_root(chan).branch_path().branch_path() / "rx_eeprom")) {
            const auto db_eeprom =
                _tree->access<dboard_eeprom_t>(
                        rx_rf_fe_root(chan).branch_path().branch_path()
                        / "rx_eeprom"
                ).get();
            usrp_info["rx_serial"] = db_eeprom.serial;
            usrp_info["rx_id"] = db_eeprom.id.to_pp_string();
        }
        const auto rfnoc_path = mb_root(mcp.mboard) / "xbar";
        if (_tree->exists(rfnoc_path)) {
            const auto spec = get_rx_subdev_spec(mcp.mboard).at(mcp.chan);
            const auto radio_index = get_radio_index(spec.db_name);
            const auto radio_path = rfnoc_path / str(boost::format("Radio_%d") % radio_index);
            const auto eeprom_path = radio_path / "eeprom";
            if (_tree->exists(eeprom_path)) {
                const auto db_eeprom = _tree->access<eeprom_map_t>(eeprom_path).get();
                usrp_info["rx_serial"] = db_eeprom.count("serial") ?
                    std::string(db_eeprom.at("serial").begin(), db_eeprom.at("serial").end())
                    : "n/a"
                ;
                usrp_info["rx_id"] = db_eeprom.count("pid") ?
                   std::string(db_eeprom.at("pid").begin(), db_eeprom.at("pid").end())
                   : "n/a"
                ;
            }
        }
        return usrp_info;
    }

    dict<std::string, std::string> get_usrp_tx_info(size_t chan){
        mboard_chan_pair mcp = tx_chan_to_mcp(chan);
        dict<std::string, std::string> usrp_info;
        const auto mb_eeprom =
            _tree->access<mboard_eeprom_t>(mb_root(mcp.mboard) / "eeprom").get();
        usrp_info["mboard_id"] = _tree->access<std::string>(mb_root(mcp.mboard) / "name").get();
        usrp_info["mboard_name"] = mb_eeprom["name"];
        usrp_info["mboard_serial"] = mb_eeprom["serial"];
        usrp_info["tx_subdev_name"] = _tree->access<std::string>(tx_rf_fe_root(chan) / "name").get();
        usrp_info["tx_subdev_spec"] = _tree->access<subdev_spec_t>(mb_root(mcp.mboard) / "tx_subdev_spec").get().to_string();
        usrp_info["tx_antenna"] = _tree->access<std::string>(tx_rf_fe_root(chan) / "antenna" / "value").get();
        if (_tree->exists(tx_rf_fe_root(chan).branch_path().branch_path() / "tx_eeprom")) {
            const auto db_eeprom =
                _tree->access<dboard_eeprom_t>(
                        tx_rf_fe_root(chan).branch_path().branch_path()
                        / "tx_eeprom"
                ).get();
            usrp_info["tx_serial"] = db_eeprom.serial;
            usrp_info["tx_id"] = db_eeprom.id.to_pp_string();
        }
        const auto rfnoc_path = mb_root(mcp.mboard) / "xbar";
        if (_tree->exists(rfnoc_path)) {
            const auto spec = get_tx_subdev_spec(mcp.mboard).at(mcp.chan);
            const auto radio_index = get_radio_index(spec.db_name);
            const auto radio_path = rfnoc_path / str(boost::format("Radio_%d")%radio_index);
            const auto path = radio_path / "eeprom";
            if(_tree->exists(path)) {
                const auto db_eeprom = _tree->access<eeprom_map_t>(path).get();
                usrp_info["tx_serial"] = db_eeprom.count("serial") ?
                    std::string(db_eeprom.at("serial").begin(), db_eeprom.at("serial").end())
                    : "n/a"
                ;
                usrp_info["tx_id"] = db_eeprom.count("pid") ?
                   std::string(db_eeprom.at("pid").begin(), db_eeprom.at("pid").end())
                   : "n/a"
                ;
            }
        }
        return usrp_info;
    }

    /*******************************************************************
     * Mboard methods
     ******************************************************************/
    void set_master_clock_rate(double rate, size_t mboard){
        if (mboard != ALL_MBOARDS){
            if (_tree->exists(mb_root(mboard) / "auto_tick_rate")
                    and _tree->access<bool>(mb_root(mboard) / "auto_tick_rate").get()) {
                _tree->access<bool>(mb_root(mboard) / "auto_tick_rate").set(false);
                UHD_LOGGER_INFO("MULTI_USRP") << "Setting master clock rate selection to 'manual'.";
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

    meta_range_t get_master_clock_rate_range(const size_t mboard)
    {
        if (_tree->exists(mb_root(mboard) / "tick_rate/range")) {
            return _tree->access<meta_range_t>(
                mb_root(mboard) / "tick_rate/range"
            ).get();
        }
        // The USRP may not have a range defined, in which case we create a
        // fake range with a single value:
        const double tick_rate = get_master_clock_rate(mboard);
        return meta_range_t(
            tick_rate,
            tick_rate,
            0
        );
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
        UHD_LOGGER_INFO("MULTI_USRP")
            << "    1) catch time transition at pps edge";
        auto end_time = std::chrono::steady_clock::now()
                            + std::chrono::milliseconds(1100);
        time_spec_t time_start_last_pps = get_time_last_pps();
        while (time_start_last_pps == get_time_last_pps())
        {
            if (std::chrono::steady_clock::now() > end_time) {
                throw uhd::runtime_error(
                    "Board 0 may not be getting a PPS signal!\n"
                    "No PPS detected within the time interval.\n"
                    "See the application notes for your device.\n"
                );
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        UHD_LOGGER_INFO("MULTI_USRP")
            << "    2) set times next pps (synchronously)";
        set_time_next_pps(time_spec, ALL_MBOARDS);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        //verify that the time registers are read to be within a few RTT
        for (size_t m = 1; m < get_num_mboards(); m++){
            time_spec_t time_0 = this->get_time_now(0);
            time_spec_t time_i = this->get_time_now(m);
            if (time_i < time_0 or (time_i - time_0) > time_spec_t(0.01)){ //10 ms: greater than RTT but not too big
                UHD_LOGGER_WARNING("MULTI_USRP") << boost::format(
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
            if (is_device3()) {
                mboard_chan_pair mcp = rx_chan_to_mcp(chan);
                _legacy_compat->issue_stream_cmd(stream_cmd, mcp.mboard, mcp.chan);
            } else {
                _tree->access<stream_cmd_t>(rx_dsp_root(chan) / "stream_cmd").set(stream_cmd);
            }
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
        if (_tree->exists(mb_root(mboard) / "sensors")) {
            return _tree->list(mb_root(mboard) / "sensors");
        }
        return {};
    }

    void set_user_register(const uint8_t addr, const uint32_t data, size_t mboard){
        if (mboard != ALL_MBOARDS){
            typedef std::pair<uint8_t, uint32_t> user_reg_t;
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
        if (is_device3()) {
            UHD_LOGGER_DEBUG("MULTI_USRP") << "[multi_usrp] Getting rx streamer from legacy compat" << std::endl;
            return _legacy_compat->get_rx_stream(args);
        }
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
            UHD_LOGGER_INFO("MULTI_USRP") << "Selecting default RX front end spec: " << spec.to_pp_string();
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
        if (is_device3()) {
            _legacy_compat->set_rx_rate(rate, chan);
            if (chan == ALL_CHANS) {
                for (size_t c = 0; c < get_rx_num_channels(); c++){
                    do_samp_rate_warning_message(rate, get_rx_rate(c), "RX");
                }
            } else {
                do_samp_rate_warning_message(rate, get_rx_rate(chan), "RX");
            }
            return;
        }

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
        tune_request_t local_request = tune_request;

        // If any mixer is driven by an external LO the daughterboard assumes that no CORDIC correction is
        // necessary. Since the LO might be sourced from another daughterboard which would normally apply a
        // cordic correction we must disable all DSP tuning to ensure identical configurations across daughterboards.
        if (tune_request.dsp_freq_policy == tune_request.POLICY_AUTO and
            tune_request.rf_freq_policy  == tune_request.POLICY_AUTO)
        {
            for (size_t c = 0; c < get_rx_num_channels(); c++) {
                UHD_VAR(get_rx_lo_source(ALL_LOS, c));
                if (get_rx_lo_source(ALL_LOS, c) == "external") {
                    local_request.dsp_freq_policy = tune_request.POLICY_MANUAL;
                    local_request.dsp_freq = 0;
                    break;
                }
            }
        }

        tune_result_t result = tune_xx_subdev_and_dsp(RX_SIGN,
                _tree->subtree(rx_dsp_root(chan)),
                _tree->subtree(rx_rf_fe_root(chan)),
                local_request);
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

    /**************************************************************************
     * LO controls
     *************************************************************************/
    std::vector<std::string> get_rx_lo_names(size_t chan = 0){
        std::vector<std::string> lo_names;
        if (_tree->exists(rx_rf_fe_root(chan) / "los")) {
            for(const std::string &name:  _tree->list(rx_rf_fe_root(chan) / "los")) {
                lo_names.push_back(name);
            }
        }
        return lo_names;
    }

    void set_rx_lo_source(const std::string &src, const std::string &name = ALL_LOS, size_t chan = 0){
        if (_tree->exists(rx_rf_fe_root(chan) / "los")) {
            if (name == ALL_LOS) {
                if (_tree->exists(rx_rf_fe_root(chan) / "los" / ALL_LOS)) {
                    //Special value ALL_LOS support atomically sets the source for all LOs
                    _tree->access<std::string>(rx_rf_fe_root(chan) / "los" / ALL_LOS / "source" / "value").set(src);
                } else {
                    for(const std::string &n:  _tree->list(rx_rf_fe_root(chan) / "los")) {
                        this->set_rx_lo_source(src, n, chan);
                    }
                }
            } else {
                if (_tree->exists(rx_rf_fe_root(chan) / "los")) {
                    _tree->access<std::string>(rx_rf_fe_root(chan) / "los" / name / "source" / "value").set(src);
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            throw uhd::runtime_error("This device does not support manual configuration of LOs");
        }
    }

    const std::string get_rx_lo_source(const std::string &name = ALL_LOS, size_t chan = 0){
        if (_tree->exists(rx_rf_fe_root(chan) / "los")) {
            if (name == ALL_LOS) {
                    //Special value ALL_LOS support atomically sets the source for all LOs
                return _tree->access<std::string>(rx_rf_fe_root(chan) / "los" / ALL_LOS / "source" / "value").get();
            } else {
                if (_tree->exists(rx_rf_fe_root(chan) / "los")) {
                    return _tree->access<std::string>(rx_rf_fe_root(chan) / "los" / name / "source" / "value").get();
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            // If the daughterboard doesn't expose it's LO(s) then it can only be internal
            return "internal";
        }
    }

    std::vector<std::string> get_rx_lo_sources(const std::string &name = ALL_LOS, size_t chan = 0) {
        if (_tree->exists(rx_rf_fe_root(chan) / "los")) {
            if (name == ALL_LOS) {
                if (_tree->exists(rx_rf_fe_root(chan) / "los" / ALL_LOS)) {
                    //Special value ALL_LOS support atomically sets the source for all LOs
                    return _tree->access< std::vector<std::string> >(rx_rf_fe_root(chan) / "los" / ALL_LOS / "source" / "options").get();
                } else {
                    return std::vector<std::string>();
                }
            } else {
                if (_tree->exists(rx_rf_fe_root(chan) / "los")) {
                    return _tree->access< std::vector<std::string> >(rx_rf_fe_root(chan) / "los" / name / "source" / "options").get();
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            // If the daughterboard doesn't expose it's LO(s) then it can only be internal
            return std::vector<std::string> (1, "internal");
        }
    }

    void set_rx_lo_export_enabled(bool enabled, const std::string &name = ALL_LOS, size_t chan = 0){
        if (_tree->exists(rx_rf_fe_root(chan) / "los")) {
            if (name == ALL_LOS) {
                if (_tree->exists(rx_rf_fe_root(chan) / "los" / ALL_LOS)) {
                    //Special value ALL_LOS support atomically sets the source for all LOs
                    _tree->access<bool>(rx_rf_fe_root(chan) / "los" / ALL_LOS / "export").set(enabled);
                } else {
                    for(const std::string &n:  _tree->list(rx_rf_fe_root(chan) / "los")) {
                        this->set_rx_lo_export_enabled(enabled, n, chan);
                    }
                }
            } else {
                if (_tree->exists(rx_rf_fe_root(chan) / "los")) {
                    _tree->access<bool>(rx_rf_fe_root(chan) / "los" / name / "export").set(enabled);
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            throw uhd::runtime_error("This device does not support manual configuration of LOs");
        }
    }

    bool get_rx_lo_export_enabled(const std::string &name = ALL_LOS, size_t chan = 0){
        if (_tree->exists(rx_rf_fe_root(chan) / "los")) {
            if (name == ALL_LOS) {
                    //Special value ALL_LOS support atomically sets the source for all LOs
                return _tree->access<bool>(rx_rf_fe_root(chan) / "los" / ALL_LOS / "export").get();
            } else {
                if (_tree->exists(rx_rf_fe_root(chan) / "los")) {
                    return _tree->access<bool>(rx_rf_fe_root(chan) / "los" / name / "export").get();
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            // If the daughterboard doesn't expose it's LO(s), assume it cannot export
            return false;
        }
    }

    double set_rx_lo_freq(double freq, const std::string &name = ALL_LOS, size_t chan = 0){
        if (_tree->exists(rx_rf_fe_root(chan) / "los")) {
            if (name == ALL_LOS) {
                throw uhd::runtime_error("LO frequency must be set for each stage individually");
            } else {
                if (_tree->exists(rx_rf_fe_root(chan) / "los")) {
                    _tree->access<double>(rx_rf_fe_root(chan) / "los" / name / "freq" / "value").set(freq);
                    return _tree->access<double>(rx_rf_fe_root(chan) / "los" / name / "freq" / "value").get();
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            throw uhd::runtime_error("This device does not support manual configuration of LOs");
        }
    }

    double get_rx_lo_freq(const std::string &name = ALL_LOS, size_t chan = 0){
        if (_tree->exists(rx_rf_fe_root(chan) / "los")) {
            if (name == ALL_LOS) {
                throw uhd::runtime_error("LO frequency must be retrieved for each stage individually");
            } else {
                if (_tree->exists(rx_rf_fe_root(chan) / "los")) {
                    return _tree->access<double>(rx_rf_fe_root(chan) / "los" / name / "freq" / "value").get();
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            // Return actual RF frequency if the daughterboard doesn't expose it's LO(s)
            return _tree->access<double>(rx_rf_fe_root(chan) / "freq" /" value").get();
        }
    }

    freq_range_t get_rx_lo_freq_range(const std::string &name = ALL_LOS, size_t chan = 0){
        if (_tree->exists(rx_rf_fe_root(chan) / "los")) {
            if (name == ALL_LOS) {
                throw uhd::runtime_error("LO frequency range must be retrieved for each stage individually");
            } else {
                if (_tree->exists(rx_rf_fe_root(chan) / "los")) {
                    return _tree->access<freq_range_t>(rx_rf_fe_root(chan) / "los" / name / "freq" / "range").get();
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            // Return the actual RF range if the daughterboard doesn't expose it's LO(s)
            return _tree->access<meta_range_t>(rx_rf_fe_root(chan) / "freq" / "range").get();
        }
    }

    std::vector<std::string> get_tx_lo_names(const size_t chan = 0){
        std::vector<std::string> lo_names;
        if (_tree->exists(tx_rf_fe_root(chan) / "los")) {
            for (const std::string &name : _tree->list(tx_rf_fe_root(chan) / "los")) {
                lo_names.push_back(name);
            }
        }
        return lo_names;
    }

    void set_tx_lo_source(
            const std::string &src,
            const std::string &name = ALL_LOS,
            const size_t chan = 0
    ) {
        if (_tree->exists(tx_rf_fe_root(chan) / "los")) {
            if (name == ALL_LOS) {
                if (_tree->exists(tx_rf_fe_root(chan) / "los" / ALL_LOS)) {
                    // Special value ALL_LOS support atomically sets the source
                    // for all LOs
                    _tree->access<std::string>(
                            tx_rf_fe_root(chan) / "los" / ALL_LOS /
                            "source" / "value"
                    ).set(src);
                } else {
                    for (const auto &n : _tree->list(tx_rf_fe_root(chan) / "los")) {
                        this->set_tx_lo_source(src, n, chan);
                    }
                }
            } else {
                if (_tree->exists(tx_rf_fe_root(chan) / "los")) {
                    _tree->access<std::string>(
                        tx_rf_fe_root(chan) / "los" / name / "source" /
                            "value"
                    ).set(src);
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            throw uhd::runtime_error("This device does not support manual "
                                     "configuration of LOs");
        }
    }

    const std::string get_tx_lo_source(
            const std::string &name = ALL_LOS,
            const size_t chan = 0
    ) {
        if (_tree->exists(tx_rf_fe_root(chan) / "los")) {
            if (_tree->exists(tx_rf_fe_root(chan) / "los")) {
                return _tree->access<std::string>(
                    tx_rf_fe_root(chan) / "los" / name / "source" / "value"
                ).get();
            } else {
                throw uhd::runtime_error("Could not find LO stage " + name);
            }
        } else {
            // If the daughterboard doesn't expose its LO(s) then it can only
            // be internal
            return "internal";
        }
    }

    std::vector<std::string> get_tx_lo_sources(
            const std::string &name = ALL_LOS,
            const size_t chan = 0
    ) {
        if (_tree->exists(tx_rf_fe_root(chan) / "los")) {
            if (name == ALL_LOS) {
                if (_tree->exists(tx_rf_fe_root(chan) / "los" / ALL_LOS)) {
                    // Special value ALL_LOS support atomically sets the source
                    // for all LOs
                    return _tree->access<std::vector<std::string>>(
                        tx_rf_fe_root(chan) / "los" / ALL_LOS /
                            "source" / "options"
                    ).get();
                } else {
                    return std::vector<std::string>();
                }
            } else {
                if (_tree->exists(tx_rf_fe_root(chan) / "los")) {
                    return _tree->access< std::vector<std::string> >(tx_rf_fe_root(chan) / "los" / name / "source" / "options").get();
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            // If the daughterboard doesn't expose its LO(s) then it can only
            // be internal
            return std::vector<std::string>(1, "internal");
        }
    }

    void set_tx_lo_export_enabled(
            const bool enabled,
            const std::string &name = ALL_LOS,
            const size_t chan=0
    ) {
        if (_tree->exists(tx_rf_fe_root(chan) / "los")) {
            if (name == ALL_LOS) {
                if (_tree->exists(tx_rf_fe_root(chan) / "los" / ALL_LOS)) {
                    //Special value ALL_LOS support atomically sets the source for all LOs
                    _tree->access<bool>(tx_rf_fe_root(chan) / "los" / ALL_LOS / "export").set(enabled);
                } else {
                    for(const std::string &n:  _tree->list(tx_rf_fe_root(chan) / "los")) {
                        this->set_tx_lo_export_enabled(enabled, n, chan);
                    }
                }
            } else {
                if (_tree->exists(tx_rf_fe_root(chan) / "los")) {
                    _tree->access<bool>(tx_rf_fe_root(chan) / "los" / name / "export").set(enabled);
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            throw uhd::runtime_error("This device does not support manual configuration of LOs");
        }
    }

    bool get_tx_lo_export_enabled(
            const std::string &name = ALL_LOS,
            const size_t chan = 0
    ) {
        if (_tree->exists(tx_rf_fe_root(chan) / "los")) {
            if (_tree->exists(tx_rf_fe_root(chan) / "los")) {
                return _tree->access<bool>(
                    tx_rf_fe_root(chan) / "los" / name / "export"
                ).get();
            } else {
                throw uhd::runtime_error("Could not find LO stage " + name);
            }
        } else {
            // If the daughterboard doesn't expose its LO(s), assume it cannot
            // export
            return false;
        }
    }

    double set_tx_lo_freq(
            const double freq,
            const std::string &name = ALL_LOS,
            const size_t chan = 0
    ) {
        if (_tree->exists(tx_rf_fe_root(chan) / "los")) {
            if (name == ALL_LOS) {
                throw uhd::runtime_error("LO frequency must be set for each "
                                         "stage individually");
            } else {
                if (_tree->exists(tx_rf_fe_root(chan) / "los")) {
                    return _tree->access<double>(
                        tx_rf_fe_root(chan) / "los" / name / "freq" / "value"
                    ).set(freq).get();
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            throw uhd::runtime_error("This device does not support manual "
                                     "configuration of LOs");
        }
    }

    double get_tx_lo_freq(
            const std::string &name = ALL_LOS,
            const size_t chan = 0
    ) {
        if (_tree->exists(tx_rf_fe_root(chan) / "los")) {
            if (name == ALL_LOS) {
                throw uhd::runtime_error("LO frequency must be retrieved for "
                                         "each stage individually");
            } else {
                if (_tree->exists(tx_rf_fe_root(chan) / "los")) {
                    return _tree->access<double>(tx_rf_fe_root(chan) / "los" / name / "freq" / "value").get();
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            // Return actual RF frequency if the daughterboard doesn't expose
            // its LO(s)
            return _tree->access<double>(
                tx_rf_fe_root(chan) / "freq" /" value"
            ).get();
        }
    }

    freq_range_t get_tx_lo_freq_range(
            const std::string &name = ALL_LOS,
            const size_t chan = 0
    ) {
        if (_tree->exists(tx_rf_fe_root(chan) / "los")) {
            if (name == ALL_LOS) {
                throw uhd::runtime_error("LO frequency range must be retrieved "
                                         "for each stage individually");
            } else {
                if (_tree->exists(tx_rf_fe_root(chan) / "los")) {
                    return _tree->access<freq_range_t>(
                        tx_rf_fe_root(chan) / "los" / name / "freq" / "range"
                    ).get();
                } else {
                    throw uhd::runtime_error("Could not find LO stage " + name);
                }
            }
        } else {
            // Return the actual RF range if the daughterboard doesn't expose
            // its LO(s)
            return _tree->access<meta_range_t>(
                tx_rf_fe_root(chan) / "freq" / "range"
            ).get();
        }
    }

    /**************************************************************************
     * Gain control
     *************************************************************************/
    void set_rx_gain(double gain, const std::string &name, size_t chan){
        /* Check if any AGC mode is enable and if so warn the user */
        if (chan != ALL_CHANS) {
            if (_tree->exists(rx_rf_fe_root(chan) / "gain" / "agc")) {
                bool agc = _tree->access<bool>(rx_rf_fe_root(chan) / "gain" / "agc" / "enable").get();
                if(agc) {
                    UHD_LOGGER_WARNING("MULTI_USRP") << "AGC enabled for this channel. Setting will be ignored." ;
                }
            }
        } else {
            for (size_t c = 0; c < get_rx_num_channels(); c++){
                if (_tree->exists(rx_rf_fe_root(c) / "gain" / "agc")) {
                    bool agc = _tree->access<bool>(rx_rf_fe_root(chan) / "gain" / "agc" / "enable").get();
                    if(agc) {
                        UHD_LOGGER_WARNING("MULTI_USRP") << "AGC enabled for this channel. Setting will be ignored." ;
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

    void set_rx_gain_profile(const std::string& profile, const size_t chan){
        if (chan != ALL_CHANS) {
            if (_tree->exists(rx_rf_fe_root(chan) / "gains/all/profile/value")) {
                _tree->access<std::string>(rx_rf_fe_root(chan) / "gains/all/profile/value").set(profile);
            }
        } else {
            for (size_t c = 0; c < get_rx_num_channels(); c++){
                if (_tree->exists(rx_rf_fe_root(c) / "gains/all/profile/value")) {
                    _tree->access<std::string>(rx_rf_fe_root(chan) / "gains/all/profile/value").set(profile);
                }
            }
        }
    }

    std::string get_rx_gain_profile(const size_t chan)
    {
        if (chan != ALL_CHANS) {
            if (_tree->exists(rx_rf_fe_root(chan) / "gains/all/profile/value")) {
                return _tree->access<std::string>(
                    rx_rf_fe_root(chan) / "gains/all/profile/value"
                ).get();
            }
        } else {
            throw uhd::runtime_error("Can't get RX gain profile from "
                                     "all channels at once!");
        }
        return "";
    }

    std::vector<std::string> get_rx_gain_profile_names(const size_t chan)
    {
        if (chan != ALL_CHANS) {
            if (_tree->exists(rx_rf_fe_root(chan) / "gains/all/profile/options")) {
                return _tree->access<std::vector<std::string>>(
                    rx_rf_fe_root(chan) / "gains/all/profile/options"
                ).get();
            }
        } else {
            throw uhd::runtime_error("Can't get RX gain profile names from "
                                     "all channels at once!");
        }
        return std::vector<std::string>();
    }

    void set_normalized_rx_gain(double gain, size_t chan = 0)
    {
        if (gain > 1.0 || gain < 0.0) {
            throw uhd::runtime_error("Normalized gain out of range, "
                                     "must be in [0, 1].");
        }
        const gain_range_t gain_range = get_rx_gain_range(ALL_GAINS, chan);
        const double abs_gain =
            (gain * (gain_range.stop() - gain_range.start()))
            + gain_range.start();
        set_rx_gain(abs_gain, ALL_GAINS, chan);
    }

    void set_rx_agc(bool enable, size_t chan = 0)
    {
        if (chan != ALL_CHANS){
            if (_tree->exists(rx_rf_fe_root(chan) / "gain" / "agc" / "enable")) {
                _tree->access<bool>(rx_rf_fe_root(chan) / "gain" / "agc" / "enable").set(enable);
            } else {
                UHD_LOGGER_WARNING("MULTI_USRP") << "AGC is not available on this device." ;
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
        std::vector<std::string> sensor_names;
        if (_tree->exists(rx_rf_fe_root(chan) / "sensors")) {
            sensor_names = _tree->list(rx_rf_fe_root(chan) / "sensors");
        }
        return sensor_names;
    }

    void set_rx_dc_offset(const bool enb, size_t chan){
        if (chan != ALL_CHANS){
            if (_tree->exists(rx_fe_root(chan) / "dc_offset" / "enable")) {
                _tree->access<bool>(rx_fe_root(chan) / "dc_offset" / "enable").set(enb);
            } else if (_tree->exists(rx_rf_fe_root(chan) / "dc_offset" / "enable")) {
                /*For B2xx devices the dc-offset correction is implemented in the rf front-end*/
                _tree->access<bool>(rx_rf_fe_root(chan) / "dc_offset" / "enable").set(enb);
            } else {
                UHD_LOGGER_WARNING("MULTI_USRP") << "Setting DC offset compensation is not possible on this device." ;
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
                UHD_LOGGER_WARNING("MULTI_USRP") << "Setting DC offset is not possible on this device." ;
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
                UHD_LOGGER_WARNING("MULTI_USRP") << "Setting IQ imbalance compensation is not possible on this device." ;
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
                UHD_LOGGER_WARNING("MULTI_USRP") << "Setting IQ balance is not possible on this device." ;
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
        if (is_device3()) {
            return _legacy_compat->get_tx_stream(args);
        }
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
            UHD_LOGGER_INFO("MULTI_USRP") << "Selecting default TX front end spec: " << spec.to_pp_string();
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
        if (is_device3()) {
            _legacy_compat->set_tx_rate(rate, chan);
            if (chan == ALL_CHANS) {
                for (size_t c = 0; c < get_tx_num_channels(); c++){
                    do_samp_rate_warning_message(rate, get_tx_rate(c), "TX");
                }
            } else {
                do_samp_rate_warning_message(rate, get_tx_rate(chan), "TX");
            }
            return;
        }

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

    void set_tx_gain_profile(const std::string& profile, const size_t chan){
        if (chan != ALL_CHANS) {
            if (_tree->exists(tx_rf_fe_root(chan) / "gains/all/profile/value")) {
                _tree->access<std::string>(tx_rf_fe_root(chan) / "gains/all/profile/value").set(profile);
            }
        } else {
            for (size_t c = 0; c < get_tx_num_channels(); c++){
                if (_tree->exists(tx_rf_fe_root(c) / "gains/all/profile/value")) {
                    _tree->access<std::string>(tx_rf_fe_root(chan) / "gains/all/profile/value").set(profile);
                }
            }
        }
    }

    std::string get_tx_gain_profile(const size_t chan)
    {
        if (chan != ALL_CHANS) {
            if (_tree->exists(tx_rf_fe_root(chan) / "gains/all/profile/value")) {
                return _tree->access<std::string>(
                    tx_rf_fe_root(chan) / "gains/all/profile/value"
                ).get();
            }
        } else {
            throw uhd::runtime_error("Can't get TX gain profile from "
                                     "all channels at once!");
        }
        return "";
    }

    std::vector<std::string> get_tx_gain_profile_names(const size_t chan)
    {
        if (chan != ALL_CHANS) {
            if (_tree->exists(tx_rf_fe_root(chan) / "gains/all/profile/options")) {
                return _tree->access<std::vector<std::string>>(
                    tx_rf_fe_root(chan) / "gains/all/profile/options"
                ).get();
            }
        } else {
            throw uhd::runtime_error("Can't get TX gain profile names from "
                                     "all channels at once!");
        }
        return std::vector<std::string>();
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
      double norm_gain = (get_tx_gain(ALL_GAINS, chan) - gain_range.start()) / gain_range_width;
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
        std::vector<std::string> sensor_names;
        if (_tree->exists(rx_rf_fe_root(chan) / "sensors")) {
            sensor_names = _tree->list(tx_rf_fe_root(chan) / "sensors");
        }
        return sensor_names;
    }

    void set_tx_dc_offset(const std::complex<double> &offset, size_t chan){
        if (chan != ALL_CHANS){
            if (_tree->exists(tx_fe_root(chan) / "dc_offset" / "value")) {
                _tree->access<std::complex<double> >(tx_fe_root(chan) / "dc_offset" / "value").set(offset);
            } else {
                UHD_LOGGER_WARNING("MULTI_USRP") << "Setting DC offset is not possible on this device." ;
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
                UHD_LOGGER_WARNING("MULTI_USRP") << "Setting IQ balance is not possible on this device." ;
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
            for(const std::string &name:  _tree->list(mb_root(mboard) / "gpio"))
            {
                banks.push_back(name);
            }
        }
        for(const std::string &name:  _tree->list(mb_root(mboard) / "dboards"))
        {
            banks.push_back("RX"+name);
            banks.push_back("TX"+name);
        }
        return banks;
    }

    void set_gpio_attr(
        const std::string &bank,
        const std::string &attr,
        const uint32_t value,
        const uint32_t mask,
        const size_t mboard
    ) {
        std::vector<std::string> attr_value;
        if (_tree->exists(mb_root(mboard) / "gpio" / bank)) {
            if (_tree->exists(mb_root(mboard) / "gpio" / bank / attr)){
                const auto attr_type = gpio_atr::gpio_attr_rev_map.at(attr);
                switch (attr_type) {
                    case gpio_atr::GPIO_SRC:
                        throw uhd::runtime_error(
                            "Can't set SRC attribute using integer value!"
                        );
                        break;
                    case gpio_atr::GPIO_CTRL:
                    case gpio_atr::GPIO_DDR: {
                        attr_value = _tree->access<std::vector<std::string>>(
                            mb_root(mboard) / "gpio" / bank / attr
                        ).get();
                        UHD_ASSERT_THROW(attr_value.size() <= 32);
                        std::bitset<32> bit_mask = std::bitset<32>(mask);
                        std::bitset<32> bit_value = std::bitset<32>(value);
                        for (size_t i = 0; i < bit_mask.size(); i++) {
                            if (bit_mask[i] == 1) {
                                attr_value[i] = gpio_atr::attr_value_map.at(attr_type).at(bit_value[i]);
                            }
                        }
                        _tree->access<std::vector<std::string>>(
                            mb_root(mboard) / "gpio" / bank / attr
                        ).set(attr_value);
                    }
                    break;
                    default:{
                        const uint32_t current = _tree->access<uint32_t>(
                            mb_root(mboard) / "gpio" / bank / attr).get();
                        const uint32_t new_value = (current & ~mask) | (value & mask);
                        _tree->access<uint32_t>(mb_root(mboard) / "gpio" / bank / attr).set(new_value);
                    }
                    break;
                }
                return;
            } else {
                throw uhd::runtime_error(str(
                    boost::format("The hardware has no gpio attribute: `%s':\n")
                    % attr
                ));
            }
        }
        if (bank.size() > 2 and bank[1] == 'X') {
            const std::string name = bank.substr(2);
            const dboard_iface::unit_t unit =
                (bank[0] == 'R')
                ? dboard_iface::UNIT_RX
                : dboard_iface::UNIT_TX;
            auto iface = _tree->access<dboard_iface::sptr>(
                mb_root(mboard) / "dboards" / name / "iface").get();
            if (attr == gpio_atr::gpio_attr_map.at(gpio_atr::GPIO_CTRL))
                iface->set_pin_ctrl(unit, uint16_t(value), uint16_t(mask));
            if (attr == gpio_atr::gpio_attr_map.at(gpio_atr::GPIO_DDR))
                iface->set_gpio_ddr(unit, uint16_t(value), uint16_t(mask));
            if (attr == gpio_atr::gpio_attr_map.at(gpio_atr::GPIO_OUT))
                iface->set_gpio_out(unit, uint16_t(value), uint16_t(mask));
            if (attr == gpio_atr::gpio_attr_map.at(gpio_atr::GPIO_ATR_0X))
                iface->set_atr_reg(unit, gpio_atr::ATR_REG_IDLE, uint16_t(value), uint16_t(mask));
            if (attr == gpio_atr::gpio_attr_map.at(gpio_atr::GPIO_ATR_RX))
                iface->set_atr_reg(unit, gpio_atr::ATR_REG_RX_ONLY, uint16_t(value), uint16_t(mask));
            if (attr == gpio_atr::gpio_attr_map.at(gpio_atr::GPIO_ATR_TX))
                iface->set_atr_reg(unit, gpio_atr::ATR_REG_TX_ONLY, uint16_t(value), uint16_t(mask));
            if (attr == gpio_atr::gpio_attr_map.at(gpio_atr::GPIO_ATR_XX))
                iface->set_atr_reg(unit, gpio_atr::ATR_REG_FULL_DUPLEX, uint16_t(value), uint16_t(mask));
            if (attr == gpio_atr::gpio_attr_map.at(gpio_atr::GPIO_SRC)) {
                throw uhd::runtime_error("Setting gpio source does not supported in daughter board.");
            }
            return;
        }
        throw uhd::runtime_error(str(
            boost::format("The hardware has no GPIO bank `%s'")
            % bank
        ));
    }

    void set_gpio_attr(
            const std::string &bank,
            const std::string &attr,
            const std::string &str_value,
            const uint32_t mask,
            const size_t mboard
    ) {
        const auto attr_type = gpio_atr::gpio_attr_rev_map.at(attr);
        if (_tree->exists(mb_root(mboard) / "gpio" / bank)) {
            if (_tree->exists(mb_root(mboard) / "gpio" / bank / attr)) {
                switch (attr_type){
                    case gpio_atr::GPIO_SRC:
                    case gpio_atr::GPIO_CTRL:
                    case gpio_atr::GPIO_DDR:{
                        auto attr_value =
                            _tree->access<std::vector<std::string>>(
                                mb_root(mboard) / "gpio" / bank / attr).get();
                        UHD_ASSERT_THROW(attr_value.size() <= 32);
                        std::bitset<32> bit_mask = std::bitset<32>(mask);
                        for (size_t i = 0 ; i < bit_mask.size(); i++) {
                            if (bit_mask[i] == 1) {
                                attr_value[i] = str_value;
                            }
                        }
                        _tree->access<std::vector<std::string>>(
                               mb_root(mboard) / "gpio" / bank / attr
                        ).set(attr_value);
                    }
                    break;
                    default: {
                        const uint32_t value =
                            gpio_atr::gpio_attr_value_pair.at(attr).at(str_value) == 0 ? -1 : 0;
                        const uint32_t current = _tree->access<uint32_t>(
                            mb_root(mboard) / "gpio" / bank / attr).get();
                        const uint32_t new_value =
                            (current & ~mask) | (value & mask);
                        _tree->access<uint32_t>(
                            mb_root(mboard) / "gpio" / bank / attr
                        ).set(new_value);
                    }
                    break;
                }
                return;
            } else {
                throw uhd::runtime_error(str(
                    boost::format("The hardware has no gpio attribute `%s'")
                    % attr
                ));
            }
        }
        // If the bank is not in the prop tree, convert string value to integer
        // value and have it handled by the other set_gpio_attr()
        const uint32_t value =
            gpio_atr::gpio_attr_value_pair.at(attr).at(str_value) == 0
            ? -1
            : 0;
        set_gpio_attr(
            bank,
            attr,
            value,
            mask,
            mboard
        );
    }

    uint32_t get_gpio_attr(
            const std::string &bank,
            const std::string &attr,
            const size_t mboard
    ) {
        std::vector<std::string> str_val;

        if (_tree->exists(mb_root(mboard) / "gpio" / bank)) {
            if (_tree->exists(mb_root(mboard) / "gpio" / bank / attr)) {
                const auto attr_type = gpio_atr::gpio_attr_rev_map.at(attr);
                switch (attr_type){
                    case gpio_atr::GPIO_SRC:
                        throw uhd::runtime_error("Can't set SRC attribute using integer value");
                    case gpio_atr::GPIO_CTRL:
                    case gpio_atr::GPIO_DDR: {
                        str_val = _tree->access<std::vector<std::string>>(
                            mb_root(mboard) / "gpio" / bank / attr).get();
                        uint32_t val = 0;
                        for(size_t i = 0 ; i < str_val.size() ; i++) {
                            val += usrp::gpio_atr::gpio_attr_value_pair.at(attr).at(str_val[i]) << i;
                        }
                        return val;
                    }
                    default:
                        return uint32_t(_tree->access<uint64_t>(
                            mb_root(mboard) / "gpio" / bank / attr).get());
                }
                return 0;
            } else {
                throw uhd::runtime_error(str(
                    boost::format("The hardware has no gpio attribute: `%s'")
                    % attr
                ));
            }
        }
        if (bank.size() > 2 and bank[1] == 'X') {
            const std::string name = bank.substr(2);
            const dboard_iface::unit_t unit = (bank[0] == 'R')? dboard_iface::UNIT_RX : dboard_iface::UNIT_TX;
            auto iface = _tree->access<dboard_iface::sptr>(
                mb_root(mboard) / "dboards" / name / "iface").get();
            if (attr == "CTRL")     return iface->get_pin_ctrl(unit);
            if (attr == "DDR")      return iface->get_gpio_ddr(unit);
            if (attr == "OUT")      return iface->get_gpio_out(unit);
            if (attr == "ATR_0X")   return iface->get_atr_reg(unit, gpio_atr::ATR_REG_IDLE);
            if (attr == "ATR_RX")   return iface->get_atr_reg(unit, gpio_atr::ATR_REG_RX_ONLY);
            if (attr == "ATR_TX")   return iface->get_atr_reg(unit, gpio_atr::ATR_REG_TX_ONLY);
            if (attr == "ATR_XX")   return iface->get_atr_reg(unit, gpio_atr::ATR_REG_FULL_DUPLEX);
            if (attr == "READBACK") return iface->read_gpio(unit);
        }
        throw uhd::runtime_error(str(
            boost::format("The hardware has no gpio bank `%s'")
            % bank
        ));
    }

    std::vector<std::string> get_gpio_string_attr(
        const std::string &bank,
        const std::string &attr,
        const size_t mboard
    ) {
        const auto attr_type = gpio_atr::gpio_attr_rev_map.at(attr);
        auto str_val = std::vector<std::string>(32, gpio_atr::default_attr_value_map.at(attr_type));
        if (_tree->exists(mb_root(mboard) / "gpio" / bank)) {
            if (_tree->exists(mb_root(mboard) / "gpio" / bank / attr)) {
                const auto attr_type = gpio_atr::gpio_attr_rev_map.at(attr);
                switch (attr_type){
                    case gpio_atr::GPIO_SRC:
                    case gpio_atr::GPIO_CTRL:
                    case gpio_atr::GPIO_DDR:
                        return _tree->access<std::vector<std::string>>(mb_root(mboard) / "gpio" / bank / attr).get();
                    default: {
                        uint32_t value = uint32_t(_tree->access<uint32_t>(mb_root(mboard) / "gpio" / bank / attr).get());
                        std::bitset<32> bit_value = std::bitset<32>(value);
                        for (size_t i = 0; i < bit_value.size(); i++)
                        {
                            str_val[i] = bit_value[i] == 0 ? "LOW" : "HIGH";
                        }
                        return str_val;
                    }
                }
            }
            else {
                throw uhd::runtime_error(str(
                    boost::format("The hardware has no gpio attribute: `%s'")
                    % attr
                ));
            }
        }
        throw uhd::runtime_error(str(
            boost::format("The hardware has no support for given gpio bank name `%s'")
            % bank
        ));
    }

    void write_register(const std::string &path, const uint32_t field, const uint64_t value, const size_t mboard)
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
                    uhd::soft_register_base::cast<uhd::soft_reg16_rw_t>(reg).write(field, static_cast<uint16_t>(value));
                else
                    uhd::soft_register_base::cast<uhd::soft_reg16_wo_t>(reg).write(field, static_cast<uint16_t>(value));
            break;

            case 32:
                if (reg.is_readable())
                    uhd::soft_register_base::cast<uhd::soft_reg32_rw_t>(reg).write(field, static_cast<uint32_t>(value));
                else
                    uhd::soft_register_base::cast<uhd::soft_reg32_wo_t>(reg).write(field, static_cast<uint32_t>(value));
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

    uint64_t read_register(const std::string &path, const uint32_t field, const size_t mboard)
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
                    return static_cast<uint64_t>(uhd::soft_register_base::cast<uhd::soft_reg16_rw_t>(reg).read(field));
                else
                    return static_cast<uint64_t>(uhd::soft_register_base::cast<uhd::soft_reg16_ro_t>(reg).read(field));
            break;

            case 32:
                if (reg.is_writable())
                    return static_cast<uint64_t>(uhd::soft_register_base::cast<uhd::soft_reg32_rw_t>(reg).read(field));
                else
                    return static_cast<uint64_t>(uhd::soft_register_base::cast<uhd::soft_reg32_ro_t>(reg).read(field));
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
        }
        throw uhd::not_implemented_error("multi_usrp::read_register - register IO not supported for this device");
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
    bool _is_device3;
    uhd::rfnoc::legacy_compat::sptr _legacy_compat;

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
            const std::string tree_path = "/mboards/" + std::to_string(mboard);
            if (_tree->exists(tree_path)) {
                return tree_path;
            } else {
                throw uhd::index_error(str(boost::format("multi_usrp::mb_root(%u) - path not found") % mboard));
            }
        }
        catch(const std::exception &e)
        {
            throw uhd::index_error(str(boost::format("multi_usrp::mb_root(%u) - %s") % mboard % e.what()));
        }
    }

    fs_path rx_dsp_root(const size_t chan)
    {
        mboard_chan_pair mcp = rx_chan_to_mcp(chan);
        if (is_device3()) {
            return _legacy_compat->rx_dsp_root(mcp.mboard, mcp.chan);
        }

        if (_tree->exists(mb_root(mcp.mboard) / "rx_chan_dsp_mapping")) {
            std::vector<size_t> map = _tree->access<std::vector<size_t> >(mb_root(mcp.mboard) / "rx_chan_dsp_mapping").get();
            UHD_ASSERT_THROW(map.size() > mcp.chan);
            mcp.chan = map[mcp.chan];
        }

        try
        {
            const std::string tree_path = mb_root(mcp.mboard) / "rx_dsps" / mcp.chan;
            if (_tree->exists(tree_path)) {
                return tree_path;
            } else {
                throw uhd::index_error(str(boost::format("multi_usrp::rx_dsp_root(%u) - mcp(%u) - path not found") % chan % mcp.chan));
            }
        }
        catch(const std::exception &e)
        {
            throw uhd::index_error(str(boost::format("multi_usrp::rx_dsp_root(%u) - mcp(%u) - %s") % chan % mcp.chan % e.what()));
        }
    }

    fs_path tx_dsp_root(const size_t chan)
    {
        mboard_chan_pair mcp = tx_chan_to_mcp(chan);
        if (is_device3()) {
            return _legacy_compat->tx_dsp_root(mcp.mboard, mcp.chan);
        }

        if (_tree->exists(mb_root(mcp.mboard) / "tx_chan_dsp_mapping")) {
            std::vector<size_t> map = _tree->access<std::vector<size_t> >(mb_root(mcp.mboard) / "tx_chan_dsp_mapping").get();
            UHD_ASSERT_THROW(map.size() > mcp.chan);
            mcp.chan = map[mcp.chan];
        }
        try
        {
            const std::string tree_path = mb_root(mcp.mboard) / "tx_dsps" / mcp.chan;
            if (_tree->exists(tree_path)) {
                return tree_path;
            } else {
                throw uhd::index_error(str(boost::format("multi_usrp::tx_dsp_root(%u) - mcp(%u) - path not found") % chan % mcp.chan));
            }
        }
        catch(const std::exception &e)
        {
            throw uhd::index_error(str(boost::format("multi_usrp::tx_dsp_root(%u) - mcp(%u) - %s") % chan % mcp.chan % e.what()));
        }
    }

    fs_path rx_fe_root(const size_t chan)
    {
        mboard_chan_pair mcp = rx_chan_to_mcp(chan);
        if (is_device3()) {
            return _legacy_compat->rx_fe_root(mcp.mboard, mcp.chan);
        }
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
        if (is_device3()) {
            return _legacy_compat->tx_fe_root(mcp.mboard, mcp.chan);
        }
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

    size_t get_radio_index(const std::string slot_name)
    {
        if (slot_name == "A") {
            return 0;
        } else if (slot_name == "B") {
            return 1;
        } else if (slot_name == "C") {
            return 2;
        } else if (slot_name == "D") {
            return  3;
        } else {
           throw uhd::key_error(str(
                boost::format("[multi_usrp]: radio slot name %s out of supported range.")
                % slot_name
            ));
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
        for(const std::string &name:  _tree->list(mb_root(mcp.mboard) / "rx_codecs" / spec.db_name / "gains")){
            gg->register_fcns("ADC-"+name, make_gain_fcns_from_subtree(_tree->subtree(mb_root(mcp.mboard) / "rx_codecs" / spec.db_name / "gains" / name)), 0 /* low prio */);
        }
        for(const std::string &name:  _tree->list(rx_rf_fe_root(chan) / "gains")){
            gg->register_fcns(name, make_gain_fcns_from_subtree(_tree->subtree(rx_rf_fe_root(chan) / "gains" / name)), 1 /* high prio */);
        }
        return gg;
    }

    gain_group::sptr tx_gain_group(size_t chan){
        mboard_chan_pair mcp = tx_chan_to_mcp(chan);
        const subdev_spec_pair_t spec = get_tx_subdev_spec(mcp.mboard).at(mcp.chan);
        gain_group::sptr gg = gain_group::make();
        for(const std::string &name:  _tree->list(mb_root(mcp.mboard) / "tx_codecs" / spec.db_name / "gains")){
            gg->register_fcns("DAC-"+name, make_gain_fcns_from_subtree(_tree->subtree(mb_root(mcp.mboard) / "tx_codecs" / spec.db_name / "gains" / name)), 1 /* high prio */);
        }
        for(const std::string &name:  _tree->list(tx_rf_fe_root(chan) / "gains")){
            gg->register_fcns(name, make_gain_fcns_from_subtree(_tree->subtree(tx_rf_fe_root(chan) / "gains" / name)), 0 /* low prio */);
        }
        return gg;
    }

    //! \param is_tx True for tx
    // Assumption is that all mboards use the same link
    // and that the rate sum is evenly distributed among the mboards
    bool _check_link_rate(const stream_args_t &args, bool is_tx) {
        bool link_rate_is_ok = true;
        size_t bytes_per_sample = convert::get_bytes_per_item(args.otw_format.empty() ? "sc16" : args.otw_format);
        double max_link_rate = 0;
        double sum_rate = 0;
        for(const size_t chan:  args.channels) {
            mboard_chan_pair mcp = is_tx ? tx_chan_to_mcp(chan) : rx_chan_to_mcp(chan);
            if (_tree->exists(mb_root(mcp.mboard) / "link_max_rate")) {
                max_link_rate = std::max(
                    max_link_rate,
                   _tree->access<double>(mb_root(mcp.mboard) / "link_max_rate").get()
                );
            }
            sum_rate += is_tx ? get_tx_rate(chan) : get_rx_rate(chan);
        }
        sum_rate /= get_num_mboards();
        if (max_link_rate > 0 and (max_link_rate / bytes_per_sample) < sum_rate) {
            UHD_LOGGER_WARNING("MULTI_USRP") << boost::format(
                "The total sum of rates (%f MSps on %u channels) exceeds the maximum capacity of the connection.\n"
                "This can cause %s."
            ) % (sum_rate/1e6) % args.channels.size() % (is_tx ? "underruns (U)" : "overflows (O)")  ;
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
    UHD_LOGGER_TRACE("MULTI_USRP") << "multi_usrp::make with args " << dev_addr.to_pp_string() ;
    return sptr(new multi_usrp_impl(dev_addr));
}

//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_X300_DEV_ARGS_HPP
#define INCLUDED_X300_DEV_ARGS_HPP

#include "x300_impl.hpp"
#include "x300_defaults.hpp"
#include <uhdlib/usrp/constrained_device_args.hpp>

namespace uhd { namespace usrp { namespace x300 {

class x300_device_args_t : public constrained_device_args_t
{
public:
    x300_device_args_t():
        _master_clock_rate("master_clock_rate", DEFAULT_TICK_RATE),
        _dboard_clock_rate("dboard_clock_rate", -1),
        _system_ref_rate("system_ref_rate", DEFAULT_SYSREF_RATE),
        _clock_source("clock_source", DEFAULT_CLOCK_SOURCE),
        _time_source("time_source", DEFAULT_TIME_SOURCE),
        _first_addr("addr", ""),
        _second_addr("second_addr", ""),
        _resource("resource", ""),
        _self_cal_adc_delay("self_cal_adc_delay", false),
        _ext_adc_self_test("ext_adc_self_test", false),
        _ext_adc_self_test_duration("ext_adc_self_test", 30.0),
        _recover_mb_eeprom("recover_mb_eeprom", false),
        _ignore_cal_file("ignore_cal_file", false),
        _niusrprio_rpc_port("niusrprio_rpc_port", NIUSRPRIO_DEFAULT_RPC_PORT),
        _has_fw_file("fw", false),
        _fw_file("fw", ""),
        _blank_eeprom("blank_eeprom", false),
        _enable_tx_dual_eth("enable_tx_dual_eth", false)
    {
        // nop
    }

    double get_master_clock_rate() const {
        return _master_clock_rate.get();
    }
    double get_dboard_clock_rate() const {
        return _dboard_clock_rate.get();
    }
    double get_system_ref_rate() const {
        return _system_ref_rate.get();
    }
    std::string get_clock_source() const {
        return _clock_source.get();
    }
    std::string get_time_source() const {
        return _time_source.get();
    }
    std::string get_first_addr() const {
        return _first_addr.get();
    }
    std::string get_second_addr() const {
        return _second_addr.get();
    }
    bool get_self_cal_adc_delay() const {
        return _self_cal_adc_delay.get();
    }
    bool get_ext_adc_self_test() const {
        return _ext_adc_self_test.get();
    }
    double get_ext_adc_self_test_duration() const {
        return _ext_adc_self_test_duration.get();
    }
    bool get_recover_mb_eeprom() const {
        return _recover_mb_eeprom.get();
    }
    bool get_ignore_cal_file() const {
        return _ignore_cal_file.get();
    }
    // must be a number in the string
    // default NIUSRPRIO_DEFAULT_RPC_PORT
    std::string get_niusrprio_rpc_port() const {
        return std::to_string(_niusrprio_rpc_port.get());
    }
    std::string get_resource() const {
        return _resource.get();
    }
    // must be valid file, key == fw, default x300::FW_FILE_NAME
    std::string get_fw_file() const {
        return _fw_file.get();
    }
    // true if the key is set
    bool has_fw_file() const {
        return _has_fw_file.get();
    }
    bool get_blank_eeprom() const {
        return _blank_eeprom.get();
    }
    bool get_enable_tx_dual_eth() const {
        return _enable_tx_dual_eth.get();
    }


    inline virtual std::string to_string() const {
        return  _master_clock_rate.to_string() + ", " +
            "";
    }

private:
    virtual void _parse(const device_addr_t& dev_args) {
        //Extract parameters from dev_args
#define PARSE_DEFAULT(arg) parse_arg_default(dev_args, arg);
        PARSE_DEFAULT(_master_clock_rate)
        if (dev_args.has_key(_master_clock_rate.key())) {
            _master_clock_rate.parse(dev_args[_master_clock_rate.key()]);
        }
        if (dev_args.has_key(_dboard_clock_rate.key())) {
            _dboard_clock_rate.parse(dev_args[_dboard_clock_rate.key()]);
        } else {
            // Some daughterboards may require other rates, but this default
            // works best for all newer daughterboards (i.e. CBX, WBX, SBX,
            // UBX, and TwinRX).
            if (_master_clock_rate.get() >= MIN_TICK_RATE &&
                _master_clock_rate.get() <= MAX_TICK_RATE) {
                _dboard_clock_rate.set(_master_clock_rate.get() / 4);
            } else {
                throw uhd::value_error(
                    "Can't infer daughterboard clock rate. Specify "
                    "dboard_clk_rate in the device args."
                );
            }
        }
        PARSE_DEFAULT(_system_ref_rate)
        PARSE_DEFAULT(_clock_source)
        PARSE_DEFAULT(_time_source)
        PARSE_DEFAULT(_first_addr)
        PARSE_DEFAULT(_second_addr)
        PARSE_DEFAULT(_resource)
        PARSE_DEFAULT(_resource)
        PARSE_DEFAULT(_self_cal_adc_delay)
        if (dev_args.has_key("ext_adc_self_test")) {
            _ext_adc_self_test.set(true);
            try {
                PARSE_DEFAULT(_ext_adc_self_test_duration);
            } catch (const uhd::value_error&) {
                // That's OK, because we don't have to specify the parameter.
            }
        }
        PARSE_DEFAULT(_recover_mb_eeprom)
        PARSE_DEFAULT(_ignore_cal_file)
        PARSE_DEFAULT(_niusrprio_rpc_port)
        if (dev_args.has_key("fw")) {
            _has_fw_file.set(true);
            PARSE_DEFAULT(_fw_file);
        }
        PARSE_DEFAULT(_blank_eeprom)
        if (dev_args.has_key("enable_tx_dual_eth")){
            _enable_tx_dual_eth.set(true);
        }

        //Sanity check params
        _enforce_range(_master_clock_rate, MIN_TICK_RATE, MAX_TICK_RATE);
        _enforce_discrete(_system_ref_rate, EXTERNAL_FREQ_OPTIONS);
        _enforce_discrete(_clock_source, CLOCK_SOURCE_OPTIONS);
        _enforce_discrete(_time_source, TIME_SOURCE_OPTIONS);
        // TODO: If _fw_file is set, make sure it's actually a file
    }

    constrained_device_args_t::num_arg<double>     _master_clock_rate;
    constrained_device_args_t::num_arg<double>     _dboard_clock_rate;
    constrained_device_args_t::num_arg<double>     _system_ref_rate;
    constrained_device_args_t::str_arg<false>      _clock_source;
    constrained_device_args_t::str_arg<false>      _time_source;
    constrained_device_args_t::str_arg<false>      _first_addr;
    constrained_device_args_t::str_arg<false>      _second_addr;
    constrained_device_args_t::str_arg<true>       _resource;
    constrained_device_args_t::bool_arg            _self_cal_adc_delay;
    constrained_device_args_t::bool_arg            _ext_adc_self_test;
    constrained_device_args_t::num_arg<double>     _ext_adc_self_test_duration;
    constrained_device_args_t::bool_arg            _recover_mb_eeprom;
    constrained_device_args_t::bool_arg            _ignore_cal_file;
    constrained_device_args_t::num_arg<size_t>     _niusrprio_rpc_port;
    constrained_device_args_t::bool_arg            _has_fw_file;
    constrained_device_args_t::str_arg<true>       _fw_file;
    constrained_device_args_t::bool_arg            _blank_eeprom;
    constrained_device_args_t::bool_arg            _enable_tx_dual_eth;
};

}}} //namespace

#endif //INCLUDED_X300_DEV_ARGS_HPP

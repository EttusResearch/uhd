//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_X300_DEV_ARGS_HPP
#define INCLUDED_X300_DEV_ARGS_HPP

#include "x300_defaults.hpp"
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/constrained_device_args.hpp>

namespace uhd { namespace usrp { namespace x300 {

class x300_device_args_t : public constrained_device_args_t
{
public:
    x300_device_args_t()
        : _master_clock_rate("master_clock_rate", DEFAULT_TICK_RATE)
        , _dboard_clock_rate("dboard_clock_rate", -1)
        , _system_ref_rate("system_ref_rate", DEFAULT_SYSREF_RATE)
        , _clock_source("clock_source", DEFAULT_CLOCK_SOURCE)
        , _time_source("time_source", DEFAULT_TIME_SOURCE)
        , _first_addr("addr", "")
        , _second_addr("second_addr", "")
        , _resource("resource", "")
        , _self_cal_adc_delay("self_cal_adc_delay", false)
        , _ext_adc_self_test("ext_adc_self_test", false)
        , _ext_adc_self_test_duration(
              "ext_adc_self_test", DEFAULT_EXT_ADC_SELF_TEST_DURATION)
        , _recover_mb_eeprom("recover_mb_eeprom", false)
        , _ignore_cal_file("ignore_cal_file", false)
        , _niusrprio_rpc_port("niusrprio_rpc_port", NIUSRPRIO_DEFAULT_RPC_PORT)
        , _has_fw_file("fw", false)
        , _fw_file("fw", "")
        , _blank_eeprom("blank_eeprom", false)
        , _enable_tx_dual_eth("enable_tx_dual_eth", false)
        , _use_dpdk("use_dpdk", false)
        , _fpga_option("fpga", "")
        , _download_fpga("download-fpga", false)
        , _recv_frame_size("recv_frame_size", DATA_FRAME_MAX_SIZE)
        , _send_frame_size("send_frame_size", DATA_FRAME_MAX_SIZE)
    {
        // nop
    }

    double get_master_clock_rate() const
    {
        return _master_clock_rate.get();
    }
    double get_dboard_clock_rate() const
    {
        return _dboard_clock_rate.get();
    }
    double get_system_ref_rate() const
    {
        return _system_ref_rate.get();
    }
    std::string get_clock_source() const
    {
        return _clock_source.get();
    }
    std::string get_time_source() const
    {
        return _time_source.get();
    }
    std::string get_first_addr() const
    {
        return _first_addr.get();
    }
    std::string get_second_addr() const
    {
        return _second_addr.get();
    }
    bool get_self_cal_adc_delay() const
    {
        return _self_cal_adc_delay.get();
    }
    bool get_ext_adc_self_test() const
    {
        return _ext_adc_self_test.get();
    }
    double get_ext_adc_self_test_duration() const
    {
        return _ext_adc_self_test_duration.get();
    }
    bool get_recover_mb_eeprom() const
    {
        return _recover_mb_eeprom.get();
    }
    bool get_ignore_cal_file() const
    {
        return _ignore_cal_file.get();
    }
    // must be a number in the string
    // default NIUSRPRIO_DEFAULT_RPC_PORT
    std::string get_niusrprio_rpc_port() const
    {
        return std::to_string(_niusrprio_rpc_port.get());
    }
    std::string get_resource() const
    {
        return _resource.get();
    }
    // must be valid file, key == fw, default x300::FW_FILE_NAME
    std::string get_fw_file() const
    {
        return _fw_file.get();
    }
    // true if the key is set
    bool has_fw_file() const
    {
        return _has_fw_file.get();
    }
    bool get_blank_eeprom() const
    {
        return _blank_eeprom.get();
    }
    bool get_enable_tx_dual_eth() const
    {
        return _enable_tx_dual_eth.get();
    }
    bool get_use_dpdk() const
    {
        return _use_dpdk.get();
    }
    std::string get_fpga_option() const
    {
        return _fpga_option.get();
    }
    bool get_download_fpga() const
    {
        return _download_fpga.get();
    }
    size_t get_recv_frame_size() const
    {
        return _recv_frame_size.get();
    }
    size_t get_send_frame_size() const
    {
        return _send_frame_size.get();
    }
    device_addr_t get_orig_args() const
    {
        return _orig_args;
    }

    inline std::string to_string() const override
    {
        // We leave out blank_eeprom for safety reasons
        return (!_first_addr.get().empty() ? (_first_addr.to_string() + ", ") : "")
               + (!_second_addr.get().empty() ? (_second_addr.to_string() + ", ") : "")
               + _master_clock_rate.to_string() + ", " + _dboard_clock_rate.to_string()
               + ", "
               + (_system_ref_rate.get() != DEFAULT_SYSREF_RATE
                         ? (_system_ref_rate.to_string() + ", ")
                         : "")
               + (_time_source.get() != DEFAULT_TIME_SOURCE
                         ? (_time_source.to_string() + ", ")
                         : "")
               + (_clock_source.get() != DEFAULT_CLOCK_SOURCE
                         ? (_clock_source.to_string() + ", ")
                         : "")
               + (_resource.get().empty() ? "" : (_resource.to_string() + ", "))
               + (_self_cal_adc_delay.get() ? (_self_cal_adc_delay.to_string() + ", ")
                                            : "")
               + (_ext_adc_self_test.get() ? (_ext_adc_self_test.to_string() + ", ") : "")
               + (_ext_adc_self_test.get()
                             && (_ext_adc_self_test_duration.get()
                                    != DEFAULT_EXT_ADC_SELF_TEST_DURATION)
                         ? (_ext_adc_self_test.to_string() + ", ")
                         : "")
               + (_recover_mb_eeprom.get() ? (_recover_mb_eeprom.to_string() + ", ") : "")
               + (_ignore_cal_file.get() ? (_ignore_cal_file.to_string() + ", ") : "")
               + ((!_resource.get().empty()
                      && _niusrprio_rpc_port.get() != NIUSRPRIO_DEFAULT_RPC_PORT)
                         ? (_niusrprio_rpc_port.to_string() + ", ")
                         : "")
               + (_has_fw_file.get() ? _fw_file.to_string() + ", " : "")
               + (_enable_tx_dual_eth.get() ? (_enable_tx_dual_eth.to_string() + ", ")
                                            : "")
               + (_fpga_option.get().empty() ? "" : _fpga_option.to_string() + ", ")
               + (_download_fpga.get() ? _download_fpga.to_string() + ", " : "");
    }

private:
    void _parse(const device_addr_t& dev_args) override
    {
        _orig_args = dev_args;
        // Extract parameters from dev_args
#define PARSE_DEFAULT(arg) parse_arg_default(dev_args, arg);
        PARSE_DEFAULT(_master_clock_rate)
        if (dev_args.has_key(_master_clock_rate.key())) {
            _master_clock_rate.parse(dev_args[_master_clock_rate.key()]);
        }
        if (dev_args.has_key(_dboard_clock_rate.key())) {
            _dboard_clock_rate.parse(dev_args[_dboard_clock_rate.key()]);
        } else {
            // This default clock rate works best for most daughterboards (i.e. DBSRX2,
            // WBX, SBX, CBX, and TwinRX).
            if (_master_clock_rate.get() >= MIN_TICK_RATE
                && _master_clock_rate.get() <= MAX_TICK_RATE) {
                _dboard_clock_rate.set(_master_clock_rate.get() / 2);
            } else {
                throw uhd::value_error("Can't infer daughterboard clock rate. Specify "
                                       "dboard_clk_rate in the device args.");
            }
        }
        PARSE_DEFAULT(_system_ref_rate)
        PARSE_DEFAULT(_clock_source)
        PARSE_DEFAULT(_time_source)
        PARSE_DEFAULT(_first_addr)
        PARSE_DEFAULT(_second_addr)
        PARSE_DEFAULT(_resource)
        PARSE_DEFAULT(_fpga_option)
        PARSE_DEFAULT(_download_fpga)
        if (_first_addr.get().empty() && !_second_addr.get().empty()) {
            UHD_LOG_WARNING("X300",
                "Specifying `second_addr' without `addr'is inconsistent and has "
                "undefined behaviour. This will be no longer allowed in future "
                "versions of UHD.");
        } else if (!_first_addr.get().empty() && _second_addr.get().empty()
                   && _first_addr.get() == _second_addr.get()) {
            UHD_LOG_WARNING("X300",
                "Specifying `addr' identical to `second_addr' has no effect. "
                "`second_addr' will be ignored.");
        }
        if (!_resource.get().empty() && !_first_addr.get().empty()) {
            UHD_LOG_WARNING("X300",
                "Specifying both `resource' and `addr' is inconsistent and has "
                "undefined behaviour. This will be no longer allowed in future "
                "versions of UHD.");
        }
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
        if (dev_args.has_key("enable_tx_dual_eth")) {
            _enable_tx_dual_eth.set(true);
        }
        if (dev_args.has_key("use_dpdk")) {
#ifdef HAVE_DPDK
            _use_dpdk.set(true);
#else
            UHD_LOG_WARNING(
                "DPDK", "Detected use_dpdk argument, but DPDK support not built in.");
#endif
        }
        PARSE_DEFAULT(_recv_frame_size)
        PARSE_DEFAULT(_send_frame_size)

        // Sanity check params
        _enforce_range(_master_clock_rate, MIN_TICK_RATE, MAX_TICK_RATE);
        _enforce_discrete(_system_ref_rate, EXTERNAL_FREQ_OPTIONS);
        _enforce_discrete(_clock_source, CLOCK_SOURCE_OPTIONS);
        _enforce_discrete(_time_source, TIME_SOURCE_OPTIONS);
        // TODO: If _fw_file is set, make sure it's actually a file
    }

    constrained_device_args_t::num_arg<double> _master_clock_rate;
    constrained_device_args_t::num_arg<double> _dboard_clock_rate;
    constrained_device_args_t::num_arg<double> _system_ref_rate;
    constrained_device_args_t::str_arg<false> _clock_source;
    constrained_device_args_t::str_arg<false> _time_source;
    constrained_device_args_t::str_arg<false> _first_addr;
    constrained_device_args_t::str_arg<false> _second_addr;
    constrained_device_args_t::str_arg<true> _resource;
    constrained_device_args_t::bool_arg _self_cal_adc_delay;
    constrained_device_args_t::bool_arg _ext_adc_self_test;
    constrained_device_args_t::num_arg<double> _ext_adc_self_test_duration;
    constrained_device_args_t::bool_arg _recover_mb_eeprom;
    constrained_device_args_t::bool_arg _ignore_cal_file;
    constrained_device_args_t::num_arg<size_t> _niusrprio_rpc_port;
    constrained_device_args_t::bool_arg _has_fw_file;
    constrained_device_args_t::str_arg<true> _fw_file;
    constrained_device_args_t::bool_arg _blank_eeprom;
    constrained_device_args_t::bool_arg _enable_tx_dual_eth;
    constrained_device_args_t::bool_arg _use_dpdk;
    constrained_device_args_t::str_arg<true> _fpga_option;
    constrained_device_args_t::bool_arg _download_fpga;
    constrained_device_args_t::num_arg<size_t> _recv_frame_size;
    constrained_device_args_t::num_arg<size_t> _send_frame_size;

    device_addr_t _orig_args;
};

}}} // namespace uhd::usrp::x300

#endif // INCLUDED_X300_DEV_ARGS_HPP

#
# Copyright 2021 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import re
import sys
from mako.template import Template

class Function:
    def __init__(self, return_type, function_name, args, no_claim=False):
        self.name = function_name
        self.does_return = return_type != "void"
        self.return_type = return_type
        self.arg_names = [arg[1] for arg in args]
        self.rpcname = f"\"{function_name}\""
        self.args = [" ".join(arg) for arg in args]
        self.has_rpcprefix = False
        self.no_claim = no_claim

    def enable_rpcprefix(self):
        self.rpcname = f"_rpc_prefix + \"{self.name}\""
        self.has_rpcprefix = True

class Interface:
    def __init__(self, basename, functions, has_rpcprefix=False):
        self.basename = basename
        self.functions = functions
        self.has_rpcprefix = has_rpcprefix
        if has_rpcprefix:
            for fn in self.functions:
                fn.enable_rpcprefix()

def fn_from_string(function_string, no_claim=False):
    m = re.match(r"^([a-zA-Z:<>,_0-9 ]+)\s+([a-zA-Z0-9_]+)\(([a-zA-Z0-9,_:&<> ]*)\)$", function_string)
    return_type = m.group(1)
    function_name = m.group(2)
    args = m.group(3)
    args = [arg.strip() for arg in args.split(",")]
    args = [arg.split(" ") for arg in args if len(arg) > 0]
    args = [(" ".join(arg[:-1]), arg[-1]) for arg in args]
    return Function(return_type, function_name, args, no_claim)

IFACES = [
    Interface("mpmd_rpc", [
        fn_from_string("size_t get_num_timekeepers()"),
        fn_from_string("std::vector<std::string> get_mb_sensors()"),
        fn_from_string("sensor_value_t::sensor_map_t get_mb_sensor(const std::string& sensor)"),
        fn_from_string("std::vector<std::string> get_gpio_banks()"),
        fn_from_string("std::vector<std::string> get_gpio_srcs(const std::string& bank)"),
        fn_from_string("bool supports_feature(const std::string& feature)"),
        fn_from_string("void set_tick_period(size_t tick_index, uint64_t period_ns)"),
        fn_from_string("uint64_t get_timekeeper_time(size_t timekeeper_idx, bool last_pps)"),
        fn_from_string("void set_timekeeper_time(size_t timekeeper_idx, uint64_t ticks, bool last_pps)"),
        fn_from_string("void set_time_source(const std::string& source)"),
        fn_from_string("std::string get_time_source()"),
        fn_from_string("std::vector<std::string> get_time_sources()"),
        fn_from_string("void set_clock_source(const std::string& source)"),
        fn_from_string("std::string get_clock_source()"),
        fn_from_string("std::vector<std::string> get_clock_sources()"),
        Function("void", "set_sync_source", [("const std::map<std::string, std::string>&", "source")]),
        fn_from_string("std::map<std::string, std::string> get_sync_source()"),
        fn_from_string("std::vector<std::map<std::string, std::string>> get_sync_sources()"),
        fn_from_string("void set_clock_source_out(bool enb)"),
        fn_from_string("void set_trigger_io(const std::string& direction)"),
        fn_from_string("std::map<std::string, std::string> get_mb_eeprom()"),
        fn_from_string("std::vector<std::string> get_gpio_src(const std::string& bank)"),
        fn_from_string("void set_gpio_src(const std::string& bank, const std::vector<std::string>& src)"),

        # ref_clk_calibration
        fn_from_string("void set_ref_clk_tuning_word(uint32_t tuning_word)"),
        fn_from_string("uint32_t get_ref_clk_tuning_word()"),
        fn_from_string("void store_ref_clk_tuning_word(uint32_t tuning_word)"),
    ]),
    Interface("x400_rpc", [
        fn_from_string("std::vector<std::map<std::string, std::string>> get_dboard_info()", no_claim=True),
        fn_from_string("void set_cal_frozen(bool state, size_t block_count, size_t chan)"),
        fn_from_string("std::vector<int> get_cal_frozen(size_t block_count, size_t chan)"),
        fn_from_string("double rfdc_set_nco_freq(const std::string& trx, size_t block_count, size_t chan, double freq)"),
        fn_from_string("double rfdc_get_nco_freq(const std::string& trx, size_t block_count, size_t chan)"),
        fn_from_string("double get_master_clock_rate()"),
        fn_from_string("std::map<std::string, std::vector<uint8_t>> get_db_eeprom(size_t db_idx)"),
        fn_from_string("bool get_threshold_status(size_t db_number, size_t chan, size_t threshold_block)"),
        fn_from_string("void set_dac_mux_enable(size_t motherboard_channel_number, int enable)"),
        fn_from_string("void set_dac_mux_data(size_t i, size_t q)"),
        fn_from_string("double get_spll_freq()"),
        fn_from_string("void setup_threshold(size_t db_number, size_t chan, size_t threshold_block, const std::string& mode, size_t delay, size_t under, size_t over)"),
        fn_from_string("bool is_db_gpio_ifc_present(size_t db_idx)"),

        # Digital I/O functions
        fn_from_string("void dio_set_voltage_level(const std::string& port, const std::string& level)"),
        fn_from_string("void dio_set_port_mapping(const std::string& mapping)"),
        fn_from_string("void dio_set_pin_directions(const std::string& port, uint32_t values)"),

        # GPIO
        fn_from_string("std::vector<std::string> get_gpio_src(const std::string& bank)"),
    ]),
    Interface("dio_rpc", [
        fn_from_string("std::vector<std::string> dio_get_supported_voltage_levels(const std::string& port)"),
        fn_from_string("void dio_set_voltage_level(const std::string& port, const std::string& level)"),
        fn_from_string("std::string dio_get_voltage_level(const std::string& port)"),
        fn_from_string("void dio_set_port_mapping(const std::string& mapping)"),
        fn_from_string("void dio_set_pin_directions(const std::string& port, uint32_t values)"),
        fn_from_string("void dio_set_external_power(const std::string& port, bool enable)"),
        fn_from_string("std::string dio_get_external_power_state(const std::string& port)"),
    ]),
    Interface("dboard_base_rpc", [
        fn_from_string("std::vector<std::string> get_sensors(const std::string& trx)"),
        fn_from_string("sensor_value_t::sensor_map_t get_sensor(const std::string& trx, const std::string& sensor, size_t chan)"),
    ], has_rpcprefix=True),
    Interface("zbx_rpc", [
        fn_from_string("double get_dboard_prc_rate()"),
        fn_from_string("double get_dboard_sample_rate()"),
        fn_from_string("void enable_iq_swap(bool is_band_inverted, const std::string& trx, size_t chan)"),
    ], has_rpcprefix=True),
]

COMMON_TMPL = """<% import time %>\
/***********************************************************************
 * This file was generated by ${file} on ${time.strftime("%c")}
 **********************************************************************/

//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/sensors.hpp>
#include <uhdlib/utils/rpc.hpp>
#include <stddef.h>
#include <memory>
#include <string>
#include <vector>

namespace uhd { namespace usrp {

%for iface in ifaces:
    class ${iface.basename}_iface {
    public:
        using sptr = std::shared_ptr<${iface.basename}_iface>;

        virtual ~${iface.basename}_iface() = default;

        %for function in iface.functions:
            virtual ${function.return_type} ${function.name}(${",".join(function.args)}) = 0;
        %endfor

        // Deprecated
        virtual uhd::rpc_client::sptr get_raw_rpc_client() = 0;
    };

    class ${iface.basename} : public ${iface.basename}_iface {
    public:
        %if iface.has_rpcprefix:
            ${iface.basename}(uhd::rpc_client::sptr rpc, const std::string& rpc_prefix) : _rpcc(rpc), _rpc_prefix(rpc_prefix) {}
        %else:
            ${iface.basename}(uhd::rpc_client::sptr rpc) : _rpcc(rpc) {}
        %endif

        %for function in iface.functions:
            ${function.return_type} ${function.name}(${",".join(function.args)}) override
            {
                %if function.no_claim:
                    %if function.does_return:
                return _rpcc->request<${function.return_type}>
                    %else:
                _rpcc->notify
                    %endif
                %else:
                    %if function.does_return:
                return _rpcc->request_with_token<${function.return_type}>
                    %else:
                _rpcc->notify_with_token
                    %endif
                %endif
                    (${",".join([function.rpcname] + function.arg_names)});
            }
        %endfor

        // Deprecated
        uhd::rpc_client::sptr get_raw_rpc_client() override { return _rpcc; }

    private:
        uhd::rpc_client::sptr _rpcc;
        %if iface.has_rpcprefix:
            const std::string _rpc_prefix;
        %endif
    };
%endfor

}}
"""

def parse_tmpl(_tmpl_text, **kwargs):
    return Template(_tmpl_text).render(**kwargs)

if __name__ == '__main__':
    out_file = sys.argv[1]

    code = parse_tmpl(COMMON_TMPL,
        ifaces=IFACES,
        file=__file__,
    )

    with open(out_file, 'w') as f:
        f.write(code)

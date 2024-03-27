//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/fir_filter_block_control.hpp>
#include <uhd/rfnoc/multichan_register_iface.hpp>
#include <uhd/rfnoc/property.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <uhdlib/utils/compat_check.hpp>

using namespace uhd::rfnoc;

// Block compatability version
const uint16_t fir_filter_block_control::MAJOR_COMPAT = 1;
const uint16_t fir_filter_block_control::MINOR_COMPAT = 1;

// Register offsets
const uint32_t fir_filter_block_control::REG_FIR_BLOCK_SIZE           = 1 << 4;
const uint32_t fir_filter_block_control::REG_FIR_MAX_NUM_COEFFS_ADDR  = 0x00;
const uint32_t fir_filter_block_control::REG_FIR_LOAD_COEFF_ADDR      = 0x04;
const uint32_t fir_filter_block_control::REG_FIR_LOAD_COEFF_LAST_ADDR = 0x08;
// This register is available starting at compat version 1.1
const uint32_t fir_filter_block_control::REG_COMPAT_NUM_ADDR = 0x0C;

// User property names
const char* const PROP_KEY_MAX_NUM_COEFFS = "max_num_coeffs";

namespace {
constexpr uhd::compat_num32 MIN_VERSION_LOAD_BACKWARDS{1, 1};
}

class fir_filter_block_control_impl : public fir_filter_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(fir_filter_block_control),
        _fir_filter_reg_iface(*this, 0, REG_FIR_BLOCK_SIZE),
        _compat(get_noc_id() == FIR_FILTER_BLOCK
                    ? uhd::compat_num32(1, 0)
                    : uhd::compat_num32(regs().peek32(REG_COMPAT_NUM_ADDR))),
        _coeff_load_backwards(_compat >= MIN_VERSION_LOAD_BACKWARDS)
    {
        UHD_ASSERT_THROW(get_num_input_ports() == get_num_output_ports());
        RFNOC_LOG_DEBUG("Compat version: " << _compat.to_string());
        if (_compat.get_major() > MAJOR_COMPAT) {
            UHD_LOG_THROW(uhd::runtime_error,
                get_unique_id(),
                "Compat version on FPGA is ahead of known maximum compat version ("
                    << MAJOR_COMPAT << ")");
        }
        _register_props();
    }

    size_t get_max_num_coefficients(const size_t chan = 0) const override
    {
        if (chan >= get_num_input_ports()) {
            std::string error_msg =
                "Cannot get max number of coefficients for FIR Filter channel "
                + std::to_string(chan)
                + ", channel value must be less than "
                  "or equal to "
                + std::to_string(get_num_input_ports() - 1);
            throw uhd::value_error(error_msg);
        }
        return _max_num_coeffs.at(chan);
    }

    void set_coefficients(
        const std::vector<int16_t>& coeffs, const size_t chan = 0) override
    {
        if (chan >= get_num_input_ports()) {
            std::string error_msg = "Cannot set coefficients for FIR Filter channel "
                                    + std::to_string(chan)
                                    + ", channel value must be less than "
                                      "or equal to "
                                    + std::to_string(get_num_input_ports() - 1);
            throw uhd::value_error(error_msg);
        }
        if (coeffs.size() > _max_num_coeffs.at(chan)) {
            std::string error_msg = "Too many filter coefficients specified (max "
                                    + std::to_string(_max_num_coeffs.at(chan)) + ")";
            throw uhd::value_error(error_msg);
        }

        _coeffs[chan] = coeffs;
        // Expand coefficients to the number supported by the hardware,
        // by padding with zeroes
        _coeffs[chan].resize(_max_num_coeffs.at(chan), 0);
        _program_coefficients(chan);
    }

    std::vector<int16_t> get_coefficients(const size_t chan = 0) const override
    {
        if (chan >= get_num_input_ports()) {
            std::string error_msg = "Cannot get coefficients for FIR Filter channel "
                                    + std::to_string(chan)
                                    + ", channel value must be less than "
                                      "or equal to "
                                    + std::to_string(get_num_input_ports() - 1);
            throw uhd::value_error(error_msg);
        }
        return _coeffs.at(chan);
    }

private:
    void _register_props()
    {
        const size_t num_chans = get_num_input_ports();
        _max_num_coeffs.reserve(num_chans);
        _coeffs.reserve(num_chans);
        _prop_max_num_coeffs.reserve(num_chans);
        _prop_type_in.reserve(num_chans);
        _prop_type_out.reserve(num_chans);

        for (size_t chan = 0; chan < num_chans; chan++) {
            const uint32_t max_num_coeffs =
                _fir_filter_reg_iface.peek32(REG_FIR_MAX_NUM_COEFFS_ADDR, chan);
            _max_num_coeffs.emplace_back(max_num_coeffs);

            // set impulse as default filter for each channel
            std::vector<int16_t> impulse_coeffs(max_num_coeffs, 0);
            impulse_coeffs.front() = std::numeric_limits<int16_t>::max();
            _coeffs.emplace_back(impulse_coeffs);
            _program_coefficients(chan);

            // register user properties
            _prop_max_num_coeffs.emplace_back(property_t<int>{PROP_KEY_MAX_NUM_COEFFS,
                static_cast<int>(max_num_coeffs),
                {res_source_info::USER, chan}});
            register_property(&_prop_max_num_coeffs.back());
            add_property_resolver({&ALWAYS_DIRTY},
                {&_prop_max_num_coeffs.back()},
                [this, chan, max_num_coeffs]() {
                    _prop_max_num_coeffs.at(chan).set(max_num_coeffs);
                });

            // register edge properties
            _prop_type_in.emplace_back(property_t<std::string>{
                PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::INPUT_EDGE, chan}});
            _prop_type_out.emplace_back(property_t<std::string>{
                PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::OUTPUT_EDGE, chan}});
            register_property(&_prop_type_in.back());
            register_property(&_prop_type_out.back());

            // add resolvers for type
            add_property_resolver({&_prop_type_in.back()},
                {&_prop_type_in.back()},
                [this, chan]() { _prop_type_in.at(chan).set(IO_TYPE_SC16); });
            add_property_resolver({&_prop_type_out.back()},
                {&_prop_type_out.back()},
                [this, chan]() { _prop_type_out.at(chan).set(IO_TYPE_SC16); });
        }
    }

    void _program_coefficients(const size_t chan = 0)
    {
        // Creat local reference to coefficients
        auto& coeffs            = _coeffs.at(chan);
        const size_t num_coeffs = coeffs.size();

        // Write coefficients. The order depends on _coeff_load_backwards.
        const std::vector<uint32_t> coeffs_addr(num_coeffs - 1, REG_FIR_LOAD_COEFF_ADDR);
        // Here we store all coefficients except the last one (which goes to a
        // different address), converted to 32-bits for easy poking.
        std::vector<uint32_t> coeffs_minus_last(num_coeffs - 1);

        const auto last_coeff = _coeff_load_backwards ? coeffs.front() : coeffs.back();
        if (_coeff_load_backwards) {
            std::transform(coeffs.rbegin(),
                coeffs.rend() - 1,
                coeffs_minus_last.begin(),
                [](int16_t value) -> uint32_t { return static_cast<uint32_t>(value); });
        } else {
            std::transform(coeffs.begin(),
                coeffs.end() - 1,
                coeffs_minus_last.begin(),
                [](int16_t value) -> uint32_t { return static_cast<uint32_t>(value); });
        }

        _fir_filter_reg_iface.multi_poke32(coeffs_addr, coeffs_minus_last, chan);
        // ...and the final coefficient (num_coeffs-1)
        _fir_filter_reg_iface.poke32(
            REG_FIR_LOAD_COEFF_LAST_ADDR, static_cast<uint32_t>(last_coeff), chan);
    }

    /**************************************************************************
     * Register interface
     *************************************************************************/
    multichan_register_iface _fir_filter_reg_iface;

    /**************************************************************************
     * Attributes
     *************************************************************************/
    std::vector<property_t<std::string>> _prop_type_in;
    std::vector<property_t<std::string>> _prop_type_out;
    std::vector<property_t<int>> _prop_max_num_coeffs;

    //! Maximum number of coefficients supported by the FIR filter
    std::vector<size_t> _max_num_coeffs;

    //! Current fir filter coefficients
    std::vector<std::vector<int16_t>> _coeffs;

    //! Compat number
    const uhd::compat_num32 _compat;

    //! Direction of coefficient loading
    const bool _coeff_load_backwards;
};

static std::vector<noc_id_t> FIR_NOC_IDS{FIR_FILTER_BLOCK, FIR_FILTER_BLOCK_V2};
UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    fir_filter_block_control, FIR_NOC_IDS, "FIR", CLOCK_KEY_GRAPH, "bus_clk")

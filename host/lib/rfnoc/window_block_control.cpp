//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/multichan_register_iface.hpp>
#include <uhd/rfnoc/property.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <uhd/rfnoc/window_block_control.hpp>

using namespace uhd::rfnoc;

const uint32_t window_block_control::REG_WINDOW_BLOCK_SIZE             = 1 << 4;
const uint32_t window_block_control::REG_WINDOW_LEN_OFFSET             = 0x00;
const uint32_t window_block_control::REG_WINDOW_MAX_LEN_OFFSET         = 0x04;
const uint32_t window_block_control::REG_WINDOW_LOAD_COEFF_OFFSET      = 0x08;
const uint32_t window_block_control::REG_WINDOW_LOAD_COEFF_LAST_OFFSET = 0x0C;

// User property names
const char* const PROP_KEY_MAX_LEN = "max_len";

class window_block_control_impl : public window_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(window_block_control),
        _window_reg_iface(*this, 0, REG_WINDOW_BLOCK_SIZE)
    {
        UHD_ASSERT_THROW(get_num_input_ports() == get_num_output_ports());
        _register_props();
    }

    size_t get_max_num_coefficients(const size_t chan) const override
    {
        return _max_len.at(chan);
    }

    void set_coefficients(const std::vector<int16_t>& coeffs, const size_t chan) override
    {
        if (coeffs.size() > _max_len.at(chan)) {
            std::string error_msg = "Too many window coefficients specified (max "
                                    + std::to_string(_max_len.at(chan)) + ")";
            throw uhd::value_error(error_msg);
        }

        _coeffs[chan] = coeffs;
        _program_coefficients(chan);
    }

    std::vector<int16_t> get_coefficients(const size_t chan) const override
    {
        return _coeffs.at(chan);
    }

private:
    void _register_props()
    {
        const size_t num_chans = get_num_input_ports();
        _max_len.reserve(num_chans);
        _coeffs.reserve(num_chans);
        _prop_max_len.reserve(num_chans);
        _prop_type_in.reserve(num_chans);
        _prop_type_out.reserve(num_chans);

        for (size_t chan = 0; chan < num_chans; chan++) {
            const uint32_t max_len =
                _window_reg_iface.peek32(REG_WINDOW_MAX_LEN_OFFSET, chan);
            _max_len.emplace_back(max_len);

            // set a default rectangular window for each channel
            std::vector<int16_t> rect_coeffs(
                max_len, std::numeric_limits<int16_t>::max());
            _coeffs.emplace_back(rect_coeffs);
            _program_coefficients(chan);

            // register user properties
            _prop_max_len.emplace_back(property_t<int>{PROP_KEY_MAX_LEN,
                static_cast<int>(max_len),
                {res_source_info::USER, chan}});
            register_property(&_prop_max_len.back());
            add_property_resolver({&ALWAYS_DIRTY},
                {&_prop_max_len.back()},
                [this, chan, max_len]() { _prop_max_len.at(chan).set(max_len); });

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

    void _program_coefficients(size_t chan)
    {
        // Write coefficients [0..num_coeffs-2]...
        const size_t num_coeffs = _coeffs.at(chan).size();
        std::vector<uint32_t> coeffs_addr(num_coeffs - 1, REG_WINDOW_LOAD_COEFF_OFFSET);
        std::vector<uint32_t> coeffs_minus_last(num_coeffs - 1);
        std::transform(_coeffs.at(chan).begin(),
            _coeffs.at(chan).end() - 1,
            coeffs_minus_last.begin(),
            [](int16_t value) -> uint32_t { return static_cast<uint32_t>(value); });

        _window_reg_iface.multi_poke32(coeffs_addr, coeffs_minus_last, chan);
        // ...and the final coefficient (num_coeffs-1)
        _window_reg_iface.poke32(REG_WINDOW_LOAD_COEFF_LAST_OFFSET,
            static_cast<uint32_t>(_coeffs.at(chan).at(num_coeffs - 1)),
            chan);
        // Set window size
        _window_reg_iface.poke32(
            REG_WINDOW_LEN_OFFSET, static_cast<uint32_t>(num_coeffs), chan);
    }

    //! Maximum length of window
    std::vector<size_t> _max_len;

    //! Current window coefficients
    std::vector<std::vector<int16_t>> _coeffs;

    /**************************************************************************
     * Attributes
     *************************************************************************/
    std::vector<property_t<std::string>> _prop_type_in;
    std::vector<property_t<std::string>> _prop_type_out;
    std::vector<property_t<int>> _prop_max_len;

    /**************************************************************************
     * Register interface
     *************************************************************************/
    multichan_register_iface _window_reg_iface;
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    window_block_control, WINDOW_BLOCK, "Window", CLOCK_KEY_GRAPH, "bus_clk")

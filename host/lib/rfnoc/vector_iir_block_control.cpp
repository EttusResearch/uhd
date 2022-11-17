//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/convert.hpp>
#include <uhd/exception.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/multichan_register_iface.hpp>
#include <uhd/rfnoc/property.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <uhd/rfnoc/vector_iir_block_control.hpp>
#include <string>

using namespace uhd::rfnoc;

const uint32_t vector_iir_block_control::REG_BLOCK_SIZE   = 1 << 4;
const uint32_t vector_iir_block_control::REG_DELAY_OFFSET = 0;
const uint32_t vector_iir_block_control::REG_ALPHA_OFFSET = 4;
const uint32_t vector_iir_block_control::REG_BETA_OFFSET  = 8;

constexpr uint32_t DELAY_REG_MAX_DELAY_SHIFT = 16;
constexpr uint32_t DELAY_REG_DELAY_MASK      = 0x0000ffff;

// User property names
const char* const PROP_KEY_ALPHA     = "alpha";
const char* const PROP_KEY_BETA      = "beta";
const char* const PROP_KEY_DELAY     = "delay";
const char* const PROP_KEY_MAX_DELAY = "max_delay";

class vector_iir_block_control_impl : public vector_iir_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(vector_iir_block_control),
        _vector_iir_reg_iface(*this, 0, REG_BLOCK_SIZE)
    {
        UHD_ASSERT_THROW(get_num_input_ports() == get_num_output_ports());
        _register_props();
    }

    void set_alpha(const double alpha, const size_t chan) override
    {
        set_property<double>(PROP_KEY_ALPHA, alpha, chan);
    }

    double get_alpha(const size_t chan) const override
    {
        return _prop_alpha.at(chan).get();
    }

    void set_beta(const double beta, const size_t chan) override
    {
        set_property<double>(PROP_KEY_BETA, beta, chan);
    }

    double get_beta(const size_t chan) const override
    {
        return _prop_beta.at(chan).get();
    }

    void set_delay(const uint16_t delay, const size_t chan) override
    {
        set_property<int>(PROP_KEY_DELAY, delay, chan);
    }

    uint16_t get_delay(const size_t chan) const override
    {
        return _prop_delay.at(chan).get();
    }

    uint16_t get_max_delay(const size_t chan) const override
    {
        return _prop_max_delay.at(chan).get();
    }

    /**************************************************************************
     * Initialization
     *************************************************************************/
private:
    void _register_props()
    {
        const size_t num_inputs = get_num_input_ports();
        _prop_type_in.reserve(num_inputs);
        _prop_type_out.reserve(num_inputs);
        _prop_alpha.reserve(num_inputs);
        _prop_beta.reserve(num_inputs);
        _prop_delay.reserve(num_inputs);
        _prop_max_delay.reserve(num_inputs);

        for (size_t chan = 0; chan < num_inputs; chan++) {
            const uint16_t max_delay =
                static_cast<uint16_t>(_vector_iir_reg_iface.peek32(REG_DELAY_OFFSET, chan)
                                      >> DELAY_REG_MAX_DELAY_SHIFT);

            // register user properties
            _prop_alpha.emplace_back(
                property_t<double>{PROP_KEY_ALPHA, 0.9, {res_source_info::USER, chan}});
            _prop_beta.emplace_back(
                property_t<double>{PROP_KEY_BETA, 0.9, {res_source_info::USER, chan}});
            _prop_delay.emplace_back(property_t<int>{
                PROP_KEY_DELAY, max_delay, {res_source_info::USER, chan}});
            _prop_max_delay.emplace_back(property_t<int>{
                PROP_KEY_MAX_DELAY, max_delay, {res_source_info::USER, chan}});

            register_property(&_prop_alpha.back(), [this, chan]() {
                double alpha = _prop_alpha.at(chan).get();
                if (alpha < 0.0 || alpha > 1.0) {
                    throw uhd::value_error("Alpha value must be in [0.0, 1.0]");
                }
                _vector_iir_reg_iface.poke32(
                    REG_ALPHA_OFFSET, uint32_t(alpha * pow(2, 31)), chan);
            });
            register_property(&_prop_beta.back(), [this, chan]() {
                double beta = _prop_beta.at(chan).get();
                if (beta < 0.0 || beta > 1.0) {
                    throw uhd::value_error("Beta value must be in [0.0, 1.0]");
                }
                _vector_iir_reg_iface.poke32(
                    REG_BETA_OFFSET, uint32_t(beta * pow(2, 31)), chan);
            });
            register_property(&_prop_delay.back(), [this, chan, max_delay]() {
                int length = _prop_delay.at(chan).get();
                if (length < 5 || length > static_cast<int>(max_delay)) {
                    throw uhd::value_error(
                        "Delay value must be in [5, " + std::to_string(max_delay) + "]");
                }
                _vector_iir_reg_iface.poke32(
                    REG_DELAY_OFFSET, uint32_t(length) & DELAY_REG_DELAY_MASK, chan);
            });

            register_property(&_prop_max_delay.back());
            add_property_resolver({&ALWAYS_DIRTY},
                {&_prop_max_delay.back()},
                [this, chan, max_delay]() { _prop_max_delay.at(chan).set(max_delay); });

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

    /**************************************************************************
     * Attributes
     *************************************************************************/
    std::vector<property_t<std::string>> _prop_type_in;
    std::vector<property_t<std::string>> _prop_type_out;
    std::vector<property_t<double>> _prop_alpha;
    std::vector<property_t<double>> _prop_beta;
    std::vector<property_t<int>> _prop_delay;
    std::vector<property_t<int>> _prop_max_delay;


    /**************************************************************************
     * Register interface
     *************************************************************************/
    multichan_register_iface _vector_iir_reg_iface;
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    vector_iir_block_control, VECTOR_IIR_BLOCK, "VectorIIR", CLOCK_KEY_GRAPH, "bus_clk")

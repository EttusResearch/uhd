//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/fir_filter_block_control.hpp>
#include <uhd/rfnoc/property.hpp>
#include <uhd/rfnoc/registry.hpp>

using namespace uhd::rfnoc;

const uint32_t fir_filter_block_control::REG_FIR_MAX_NUM_COEFFS_ADDR  = 0;
const uint32_t fir_filter_block_control::REG_FIR_LOAD_COEFF_ADDR      = 4;
const uint32_t fir_filter_block_control::REG_FIR_LOAD_COEFF_LAST_ADDR = 8;

class fir_filter_block_control_impl : public fir_filter_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(fir_filter_block_control)
    , _max_num_coeffs(this->regs().peek32(REG_FIR_MAX_NUM_COEFFS_ADDR)),
        _coeffs(_max_num_coeffs, int16_t(0))
    {
        // register edge properties
        register_property(&_prop_type_in);
        register_property(&_prop_type_out);

        // add resolvers for type (keeps it constant)
        add_property_resolver({&_prop_type_in}, {&_prop_type_in}, [this]() {
            _prop_type_in.set(IO_TYPE_SC16);
        });
        add_property_resolver({&_prop_type_out}, {&_prop_type_out}, [this]() {
            _prop_type_out.set(IO_TYPE_SC16);
        });

        // initialize hardware with an impulse response
        _coeffs[0] = std::numeric_limits<int16_t>::max();
        _program_coefficients();
    }

    size_t get_max_num_coefficients() const override
    {
        return _max_num_coeffs;
    }

    void set_coefficients(const std::vector<int16_t>& coeffs) override
    {
        if (coeffs.size() > _max_num_coeffs) {
            std::string error_msg =
                "Too many filter coefficients specified (max " +
                std::to_string(_max_num_coeffs) + ")";
            throw uhd::value_error(error_msg);
        }

        // save the new coefficients...
        _coeffs = coeffs;
        // ...and expand it to the number supported by the hardware,
        // padding with zeroes
        _coeffs.resize(_max_num_coeffs, 0);
        _program_coefficients();
    }

    std::vector<int16_t> get_coefficients() const override
    {
        return _coeffs;
    }

private:
    void _program_coefficients()
    {
        // Write coefficients [0..num_coeffs-2]..
        std::vector<uint32_t> coeffs_addr(_max_num_coeffs - 1, REG_FIR_LOAD_COEFF_ADDR);
        std::vector<uint32_t> coeffs_minus_last(_max_num_coeffs - 1);
        std::transform(_coeffs.begin(),
            _coeffs.end() - 1,
            coeffs_minus_last.begin(),
            [](int16_t value) -> uint32_t { return static_cast<uint32_t>(value); });

        this->regs().multi_poke32(coeffs_addr, coeffs_minus_last);
        // ...and the final coefficients (num_coeffs-1)
        this->regs().poke32(
            REG_FIR_LOAD_COEFF_LAST_ADDR, _coeffs.at(_max_num_coeffs - 1));
    }

    //! Number of coefficients supported by the FIR filter
    const size_t _max_num_coeffs;

    //! Current FIR filter coefficients
    std::vector<int16_t> _coeffs;

    /**************************************************************************
     * Attributes
     *************************************************************************/
    property_t<std::string> _prop_type_in = property_t<std::string>{
        PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::INPUT_EDGE}};
    property_t<std::string> _prop_type_out = property_t<std::string>{
        PROP_KEY_TYPE, IO_TYPE_SC16, {res_source_info::OUTPUT_EDGE}};
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    fir_filter_block_control, FIR_FILTER_BLOCK, "FIR", CLOCK_KEY_GRAPH, "bus_clk")

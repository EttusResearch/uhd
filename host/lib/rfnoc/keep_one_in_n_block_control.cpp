//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/keep_one_in_n_block_control.hpp>
#include <uhd/rfnoc/multichan_register_iface.hpp>
#include <uhd/rfnoc/property.hpp>
#include <uhd/rfnoc/registry.hpp>

using namespace uhd::rfnoc;

//! Space (in bytes) between register banks per channel
const uint32_t REG_BANK_OFFSET = 2048;

const uint32_t keep_one_in_n_block_control::REG_N_OFFSET       = 0 * 8;
const uint32_t keep_one_in_n_block_control::REG_MODE_OFFSET    = 1 * 8;
const uint32_t keep_one_in_n_block_control::REG_WIDTH_N_OFFSET = 2 * 8;

// User property names
const std::string PROP_KEY_N    = "n";
const std::string PROP_KEY_MODE = "mode";

class keep_one_in_n_block_control_impl : public keep_one_in_n_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(keep_one_in_n_block_control),
        _keep_one_in_n_reg_iface(*this, 0, REG_BANK_OFFSET),
        _max_n((2 << regs().peek32(REG_WIDTH_N_OFFSET)) - 1) // Fixed HDL parameter
    {
        UHD_ASSERT_THROW(get_num_input_ports() == get_num_output_ports());
        _register_props();
    }

    size_t get_max_n() const override
    {
        return _max_n;
    }

    void set_n(const size_t n, const size_t chan = 0) override
    {
        set_property<int>(PROP_KEY_N, static_cast<int>(n), chan);
    }

    size_t get_n(const size_t chan = 0) const override
    {
        return _n.at(chan).get();
    }

    void set_mode(const mode mode, const size_t chan = 0) override
    {
        set_property<int>(PROP_KEY_MODE, static_cast<int>(mode), chan);
    }

    mode get_mode(const size_t chan = 0) const override
    {
        return static_cast<mode>(_mode.at(chan).get());
    }

protected:
    //! Block-specific register interface
    multichan_register_iface _keep_one_in_n_reg_iface;

private:
    void _register_props()
    {
        const size_t num_chans = get_num_input_ports();
        _n.reserve(num_chans);
        _mode.reserve(num_chans);

        for (size_t chan = 0; chan < num_chans; chan++) {
            _n.push_back(property_t<int>(PROP_KEY_N, 1, {res_source_info::USER, chan}));
            _mode.push_back(property_t<int>(PROP_KEY_MODE,
                static_cast<int>(mode::SAMPLE_MODE),
                {res_source_info::USER, chan}));

            register_property(&_n.back());
            register_property(&_mode.back());

            add_property_resolver({&_n.back()}, {&_n.back()}, [this, chan]() {
                const int max_n = this->_max_n;
                const int n     = this->_n.at(chan).get();
                if (n < 1) {
                    throw uhd::value_error("Value of n must be positive");
                } else if (n > max_n) {
                    throw uhd::value_error(
                        "Value of n must be less than " + std::to_string(max_n));
                }
                this->regs().poke32(REG_N_OFFSET, n);
            });
            add_property_resolver({&_mode.back()}, {&_mode.back()}, [this, chan]() {
                const int mode = this->_mode.at(chan).get();
                this->regs().poke32(REG_MODE_OFFSET, mode);
            });
        }
    }

    /**************************************************************************
     * Attributes
     *************************************************************************/
    //! Maximum value for N
    const int _max_n;

    //! Properties for scaling_in (one per port)
    std::vector<property_t<int>> _n;

    //! Sample mode or Packet mode
    std::vector<property_t<int>> _mode;
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(keep_one_in_n_block_control,
    KEEP_ONE_IN_N_BLOCK,
    "KeepOneInN",
    CLOCK_KEY_GRAPH,
    "bus_clk")

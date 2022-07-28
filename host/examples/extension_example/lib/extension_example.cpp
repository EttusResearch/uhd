//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "extension_example.hpp"
#include <uhd/experts/expert_container.hpp>
#include <uhd/extension/extension.hpp>
#include <uhd/utils/log.hpp>
#include <cassert>

using namespace ext_example;

// This method is required to create the instance of the extension
extension_example::sptr extension_example::make(
    uhd::extension::extension::factory_args fargs)
{
    auto tree = fargs.radio_ctrl->get_tree();
    return std::make_shared<extension_example_impl>(
        fargs.radio_ctrl, fargs.radio_ctrl->get_tree());
}

// Experts are triggered by changing parameters and can then set other parameters. In
// the extension example this is demonstrated for setting the TX and RX gains. Furthermore
// they can take the task to write values to the actual extension device via every kind of
// interface.

// The gain_expert is called when the trx gain setter has changed the gain value in the
// property tree. As a result it can write a coerced value to the same item in the tree
// again, it can change other properties in the tree or it can run any functionality.
class gain_expert : public uhd::experts::worker_node_t
{
public:
    gain_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path overall_fe_path,
        const uhd::fs_path ext_example_fe_path,
        const size_t channel,
        const uhd::direction_t trx,
        uhd::rfnoc::radio_control::sptr radio)
        : uhd::experts::worker_node_t(ext_example_fe_path / "example_gain_expert")
        , _overall_fe_path(overall_fe_path)
        , _radio(radio)
        , _trx(trx)
        , _chan(channel)
        , _gain_in(db, overall_fe_path / "gains" / "all" / "value" / "desired")
        , _gain_out(db, overall_fe_path / "gains" / "all" / "value" / "coerced")
    {
        bind_accessor(_gain_in);
        bind_accessor(_gain_out);
    }

private:
    void resolve() override
    {
        UHD_LOG_DEBUG("extension_example::gain_expert", "Resolving");

        if (_trx == RX_DIRECTION) {
            // In the RX direction we'll manipulate the gain value by 3dB up:
            _gain_out = _gain_in + 3;
            UHD_LOG_DEBUG("extension_example::gain_expert",
                "Adding 3dB for demonstration purposes.");
        } else {
            // In the TX direction we'll manipulate the gain value by 3dB down:
            _gain_out = _gain_in - 3;
            UHD_LOG_DEBUG("extension_example::gain_expert",
                "Subtracting 3dB for demonstration purposes.");
        }
    }

    const uhd::fs_path _overall_fe_path;
    uhd::rfnoc::radio_control::sptr _radio;
    const uhd::direction_t _trx;
    const size_t _chan;

    // Inputs from user/API
    uhd::experts::data_reader_t<double> _gain_in;

    // Output to user/API
    uhd::experts::data_writer_t<double> _gain_out;
};

extension_example_impl::extension_example_impl(
    uhd::rfnoc::radio_control::sptr radio, uhd::property_tree::sptr tree)
    : nameless_gain_mixin([](uhd::direction_t, size_t) { return "all"; })
    , _radio(radio)
    , _tree(tree)
{
    _expert_container =
        uhd::experts::expert_factory::create_container("extension_example_radio");

    constexpr size_t NUM_CHANNELS = 2;
    for (size_t chan = 0; chan < NUM_CHANNELS; chan++) {
        init_path(chan, TX_DIRECTION);
        init_path(chan, RX_DIRECTION);
    }
}

// Initialize the property tree paths that are required for demonstrating the experts
void extension_example_impl::init_path(const size_t chan, const uhd::direction_t trx)
{
    const auto frontend_name = trx == TX_DIRECTION ? fs_path("tx_frontends")
                                                   : fs_path("rx_frontends");

    const auto fe_path = fs_path("ext_example") / frontend_name / chan;

    const auto overall_fe_path = frontend_name / chan;

    const auto dboard_fe_path = fs_path("dboard") / frontend_name / chan;

    // Gain properties
    uhd::experts::expert_factory::add_dual_prop_node<double>(_expert_container,
        _tree,
        fe_path / "gains" / "all" / "value",
        0,
        uhd::experts::AUTO_RESOLVE_ON_WRITE);

    uhd::experts::expert_factory::add_dual_prop_node<double>(_expert_container,
        _tree,
        overall_fe_path / "gains" / "all" / "value",
        0,
        uhd::experts::AUTO_RESOLVE_ON_WRITE);

    uhd::experts::expert_factory::add_worker_node<gain_expert>(_expert_container,
        _expert_container->node_retriever(),
        overall_fe_path,
        fe_path,
        chan,
        trx,
        _radio);
}

// By using custom implementations of UHD methods return values but also functionality can
// be influenced.
freq_range_t extension_example_impl::get_tx_frequency_range(const size_t) const
{
    return freq_range_t(0.0, 100.0);
}

freq_range_t extension_example_impl::get_rx_frequency_range(const size_t) const
{
    return freq_range_t(0.0, 200.0);
}

/*!
 * Register the extension example in the extension registry of UHD. The first argument
 * is the name under which the extension can be loaded during initialization of MultiUSRP,
 * the second argument is the class that implements the extension API.
 */
UHD_REGISTER_EXTENSION(extension_example, ext_example::extension_example);

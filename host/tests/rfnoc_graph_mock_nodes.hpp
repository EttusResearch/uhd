//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_TESTS_MOCK_NODES_HPP
#define INCLUDED_LIBUHD_TESTS_MOCK_NODES_HPP

#include <uhd/rfnoc/node.hpp>

using namespace uhd::rfnoc;

constexpr int MAX_DECIM       = 512;
constexpr double DEFAULT_RATE = 1e9;
constexpr int DEFAULT_DECIM   = 1;

/*! Mock Radio node
 *
 * - "Full Duplex"
 * - Has two master clock rates: 100e6 and 200e6
 * - RSSI is a read-only prop that always needs updating
 */
class mock_radio_node_t : public node_t
{
public:
    mock_radio_node_t(const size_t radio_idx) : _radio_idx(radio_idx)
    {
        register_property(&_samp_rate_in);
        register_property(&_samp_rate_out);
        register_property(&_master_clock_rate);
        register_property(&_rssi);

        // Resolver for the input rate: We don't actually try and be clever, we
        // always reset the rate back to the TX rate.
        add_property_resolver({&_samp_rate_in},
            {&_samp_rate_in},
            [& samp_rate_in        = _samp_rate_in,
                &master_clock_rate = _master_clock_rate,
                this]() {
                UHD_LOG_INFO(get_unique_id(), " Calling resolver for `samp_rate_in'...");
                samp_rate_in = master_clock_rate.get();
            });
        add_property_resolver({&_samp_rate_out},
            {&_samp_rate_out},
            [this]() {
                UHD_LOG_INFO(get_unique_id(), " Calling resolver for `samp_rate_out'...");
                if (this->disable_samp_out_resolver) {
                    _samp_rate_out = this->force_samp_out_value;
                    UHD_LOG_DEBUG(get_unique_id(),
                        "Forcing samp_rate_out to " << _samp_rate_out.get());
                    return;
                }
                this->_samp_rate_out = this->_master_clock_rate.get();
            });
        add_property_resolver({&_master_clock_rate},
            {&_master_clock_rate, &_samp_rate_in, &_samp_rate_out},
            [& samp_rate_out       = _samp_rate_out,
                &samp_rate_in      = _samp_rate_in,
                &master_clock_rate = _master_clock_rate,
                this]() {
                UHD_LOG_INFO(get_unique_id(), " Calling resolver for `master_clock_rate'...");
                if (_master_clock_rate.get() > 150e6) {
                    _master_clock_rate = 200e6;
                } else {
                    _master_clock_rate = 100e6;
                }
                _samp_rate_in = _master_clock_rate.get();
                if (!this->disable_samp_out_resolver) {
                    _samp_rate_out = _master_clock_rate.get();
                } else {
                    _samp_rate_out = this->force_samp_out_value;
                    UHD_LOG_DEBUG(get_unique_id(),
                        "Forcing samp_rate_out to " << _samp_rate_out.get());
                }
            });
        // By depending on ALWAYS_DIRTY, this property is always updated:
        add_property_resolver({&ALWAYS_DIRTY},
            {&_rssi},
            [this]() {
                UHD_LOG_INFO(get_unique_id(), " Calling resolver for `rssi'...");
                rssi_resolver_count++;
                _rssi = static_cast<double>(rssi_resolver_count);
            });
    }

    std::string get_unique_id() const { return "MOCK_RADIO" + std::to_string(_radio_idx); }

    size_t get_num_input_ports() const
    {
        return 1;
    }

    size_t get_num_output_ports() const
    {
        return 1;
    }

    // Some public attributes that help debugging
    size_t rssi_resolver_count = 0;
    bool disable_samp_out_resolver = false;
    double force_samp_out_value = 23e6;

private:
    const size_t _radio_idx;

    property_t<double> _samp_rate_in{
        "samp_rate", 200e6, {res_source_info::INPUT_EDGE}};
    property_t<double> _samp_rate_out{
        "samp_rate", 200e6, {res_source_info::OUTPUT_EDGE}};
    property_t<double> _master_clock_rate{
        "master_clock_rate", 200e6, {res_source_info::USER}};
    property_t<double> _rssi{"rssi", 0, {res_source_info::USER}};
};

/*! Mock DDC node
 *
 * - Single channel
 * - Does simple coercion of decimation
 * - Keeps output and input rates consistent with decimation
 */
class mock_ddc_node_t : public node_t
{
public:
    mock_ddc_node_t()
    {
        register_property(&_samp_rate_in);
        register_property(&_samp_rate_out);
        register_property(&_decim);

        // Resolver for _decim: This gets executed when the user directly
        // modifies _decim. The desired behaviour is to coerce it first, then
        // keep the input rate constant, and re-calculate the output rate.
        add_property_resolver({&_decim},
            {&_decim, &_samp_rate_out},
            [& decim           = _decim,
                &samp_rate_out = _samp_rate_out,
                &samp_rate_in  = _samp_rate_in]() {
                UHD_LOG_INFO("MOCK DDC", "Calling resolver for `decim'...");
                decim         = coerce_decim(decim.get());
                samp_rate_out = samp_rate_in.get() / decim.get();
            });
        // Resolver for the input rate: We try and match decim so that the output
        // rate is not modified. If decim needs to be coerced, only then the
        // output rate is modified.
        add_property_resolver({&_samp_rate_in},
            {&_decim, &_samp_rate_out},
            [& decim           = _decim,
                &samp_rate_out = _samp_rate_out,
                &samp_rate_in  = _samp_rate_in]() {
                UHD_LOG_INFO("MOCK DDC", "Calling resolver for `samp_rate_in'...");
                decim = coerce_decim(int(samp_rate_in.get() / samp_rate_out.get()));
                samp_rate_out = samp_rate_in.get() / decim.get();
            });
        // Resolver for the output rate: Like the previous one, but flipped.
        add_property_resolver({&_samp_rate_out},
            {&_decim, &_samp_rate_in},
            [& decim           = _decim,
                &samp_rate_out = _samp_rate_out,
                &samp_rate_in  = _samp_rate_in]() {
                UHD_LOG_INFO("MOCK DDC", "Calling resolver for `samp_rate_out'...");
                decim = coerce_decim(int(samp_rate_in.get() / samp_rate_out.get()));
                samp_rate_in = samp_rate_out.get() * decim.get();
            });
    }

    std::string get_unique_id() const { return "MOCK_DDC"; }

    size_t get_num_input_ports() const
    {
        return 1;
    }

    size_t get_num_output_ports() const
    {
        return 1;
    }

    // Simplified coercer: Let's pretend like we can hit all even rates or 1
    // for all rates <= MAX_DECIM
    static int coerce_decim(const int requested_decim)
    {
        if (requested_decim <= 1) {
            return 1;
        }
        return std::min(requested_decim - (requested_decim % 2), MAX_DECIM);
    }


    // We make the properties global so we can inspect them, but that's not what
    // your supposed to do. However, we do keep the underscore notation, since that's
    // what they be called if they were in the class like they're supposed to.
    property_t<double> _samp_rate_in{
        "samp_rate", DEFAULT_RATE, {res_source_info::INPUT_EDGE}};
    property_t<double> _samp_rate_out{
        "samp_rate", DEFAULT_RATE, {res_source_info::OUTPUT_EDGE}};
    property_t<int> _decim{"decim", DEFAULT_DECIM, {res_source_info::USER}};

private:
    // This is where you normally put the properties
};


/*! FIFO
 *
 * Not much here -- we use it to test dynamic prop forwarding.
 */
class mock_fifo_t : public node_t
{
public:
    mock_fifo_t(const size_t num_ports) : _num_ports(num_ports)
    {
        set_prop_forwarding_policy(forwarding_policy_t::ONE_TO_ONE);
    }

    std::string get_unique_id() const { return "MOCK_FIFO"; }

    size_t get_num_input_ports() const
    {
        return _num_ports;
    }

    size_t get_num_output_ports() const
    {
        return _num_ports;
    }


private:
    const size_t _num_ports;
};

#endif /* INCLUDED_LIBUHD_TESTS_MOCK_NODES_HPP */

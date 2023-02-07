//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>
#include <uhd/rfnoc/register_iface.hpp>
#include <uhdlib/rfnoc/clock_iface.hpp>

using namespace uhd::rfnoc;

noc_block_base::make_args_t::~make_args_t() = default;

/******************************************************************************
 * Structors
 *****************************************************************************/
noc_block_base::noc_block_base(make_args_ptr make_args)
    : register_iface_holder(std::move(make_args->reg_iface))
    , _noc_id(make_args->noc_id)
    , _block_id(make_args->block_id)
    , _num_input_ports(make_args->num_input_ports)
    , _num_output_ports(make_args->num_output_ports)
    , _chdr_w(make_args->chdr_w)
    , _ctrlport_clock_iface(make_args->ctrlport_clk_iface)
    , _tb_clock_iface(make_args->tb_clk_iface)
    , _mb_controller(std::move(make_args->mb_control))
    , _block_args(make_args->args)
    , _tree(make_args->tree)
{
    RFNOC_LOG_TRACE("Using timebase clock: `"
                    << _tb_clock_iface->get_name() << "'. Current frequency: "
                    << (_tb_clock_iface->get_freq() / 1e6) << " MHz");
    RFNOC_LOG_TRACE("Using ctrlport clock: `"
                    << _ctrlport_clock_iface->get_name() << "'. Current frequency: "
                    << (_ctrlport_clock_iface->get_freq() / 1e6) << " MHz");
    // First, create one tick_rate property for every port
    _tick_rate_props.reserve(get_num_input_ports() + get_num_output_ports());
    for (size_t input_port = 0; input_port < get_num_input_ports(); input_port++) {
        _tick_rate_props.push_back(property_t<double>(PROP_KEY_TICK_RATE,
            _tb_clock_iface->get_freq(),
            {res_source_info::INPUT_EDGE, input_port}));
    }
    for (size_t output_port = 0; output_port < get_num_output_ports(); output_port++) {
        _tick_rate_props.push_back(property_t<double>(PROP_KEY_TICK_RATE,
            _tb_clock_iface->get_freq(),
            {res_source_info::OUTPUT_EDGE, output_port}));
    }
    // Register all the tick_rate properties and create a default resolver
    prop_ptrs_t tick_rate_prop_refs;
    tick_rate_prop_refs.reserve(_tick_rate_props.size());
    for (auto& prop : _tick_rate_props) {
        tick_rate_prop_refs.push_back(&prop);
        register_property(&prop);
    }
    for (auto& prop : _tick_rate_props) {
        auto prop_refs_copy = tick_rate_prop_refs;
        add_property_resolver(
            {&prop}, std::move(prop_refs_copy), [this, source_prop = &prop]() {
                // _set_tick_rate() will update _tick_rate, but only if that's
                // a valid operation for this block
                this->_set_tick_rate(source_prop->get());
                // Now, _tick_rate is valid and we will pass its value to all
                // tick_rate properties
                for (property_t<double>& tick_rate_prop : _tick_rate_props) {
                    tick_rate_prop = get_tick_rate();
                }
            });
    }
    // Now, the same thing for MTU props
    // Create one mtu property for every port
    _mtu_props.reserve(_num_input_ports + _num_output_ports);
    for (size_t input_port = 0; input_port < _num_input_ports; input_port++) {
        _mtu_props.push_back(property_t<size_t>(
            PROP_KEY_MTU, make_args->mtu, {res_source_info::INPUT_EDGE, input_port}));
        _mtu.insert({{res_source_info::INPUT_EDGE, input_port}, make_args->mtu});
    }
    for (size_t output_port = 0; output_port < _num_output_ports; output_port++) {
        _mtu_props.push_back(property_t<size_t>(
            PROP_KEY_MTU, make_args->mtu, {res_source_info::OUTPUT_EDGE, output_port}));
        _mtu.insert({{res_source_info::OUTPUT_EDGE, output_port}, make_args->mtu});
    }
    // Register all the mtu properties and create a default resolver
    prop_ptrs_t mtu_prop_refs;
    mtu_prop_refs.reserve(_mtu_props.size());
    for (auto& prop : _mtu_props) {
        mtu_prop_refs.push_back(&prop);
        register_property(&prop);
    }
    // If an MTU edge property value changes, this resolver will coerce the
    // value to the smaller of the new value or the existing MTU value on that
    // edge (`_mtu`). This is default behavior that *must* be present for all
    // MTU edge properties on all NoC blocks regardless of the current MTU
    // forwarding policy. Yes, this even happens with the default `DROP`
    // forwarding policy.
    //
    // Why is this behavior implemented in its own separate resolver and not
    // rolled into `set_mtu_forwarding_policy()`, which is responsible for
    // registering resolvers that have accurate output sensitivity lists based
    // on the desired MTU forwarding policy? The reason is to allow blocks to
    // implement custom MTU forwarding policies, but to ensure that this
    // default behavior is always present (remember, this behavior should
    // happen regardless of the MTU forwarding policy).
    //
    // Let's take the DDC block for example, whose MTU forwarding policy is
    // `ONE_TO_ONE`. When a DDC block is constructed, `noc_block_base` is
    // constructed first because it is a superclass of the DDC block. The
    // default MTU property resolvers are added to the list of resolvers for
    // the block. Then the DDC block constructor runs, and calls
    // `set_mtu_forwarding_policy(ONE_TO_ONE)`. We can't go and remove the
    // existing MTU edge property resolvers to modify them, or even modify
    // them in place--there's simply no way to do that given that the list of
    // property resolvers is private to `node_t` (`noc_block_base`'s
    // superclass), and there is no way to modify that list except to add new
    // entries to it via `add_property_resolver()`. So calling
    // `set_mtu_forwarding_policy()` adds a *new* set of resolvers for each MTU
    // property that adds the *additional* behaviors dictated by the forwarding
    // policy.
    //
    // This implies there is now an ordering dependency on how dirty properties
    // are resolved. The default coercing resolver *MUST* execute first, to
    // determine what the MTU value should be, and *THEN* any propagation of
    // that value based on the forwarding policy can take place. UHD satisfies
    // this ordering dependency because:
    //   a) The list of property resolvers for a node is maintained in a
    //      vector, and adding new resolvers is always performed by a
    //      `push_back()` (see `node_t::add_property_resolver()`), so it is
    //      guaranteed that the default coercing resolver is added first since
    //      it's done in the `noc_block_base` constructor.
    //   b) `resolve_props()` and `init_props()`, `node_t` functions which
    //      perform property resolution, always iterate the resolver vector
    //      in order (i.e., items 0..n-1), ensuring that the resolvers are
    //      called in the same order in which they were added to the vector.
    for (auto& prop : _mtu_props) {
        add_property_resolver({&prop}, {&prop}, [this, source_prop = &prop]() {
            const res_source_info src_edge = source_prop->get_src_info();
            // Coerce the MTU to its appropriate min value
            const size_t new_mtu = std::min(source_prop->get(), _mtu.at(src_edge));
            source_prop->set(new_mtu);
            _mtu.at(src_edge) = source_prop->get();
            RFNOC_LOG_TRACE("MTU is now " << _mtu.at(src_edge) << " on edge "
                                          << src_edge.to_string());
        });
    }
}

noc_block_base::~noc_block_base()
{
    for (const auto& node : _tree->list("")) {
        _tree->remove(node);
    }
}

void noc_block_base::set_num_input_ports(const size_t num_ports)
{
    if (num_ports > get_num_input_ports()) {
        throw uhd::value_error(
            "New number of input ports must not exceed current number!");
    }

    _num_input_ports = num_ports;
}

void noc_block_base::set_num_output_ports(const size_t num_ports)
{
    if (num_ports > get_num_output_ports()) {
        throw uhd::value_error(
            "New number of output ports must not exceed current number!");
    }

    _num_output_ports = num_ports;
}


double noc_block_base::get_tick_rate() const
{
    return _tb_clock_iface->get_freq();
}

void noc_block_base::set_tick_rate(const double tick_rate)
{
    if (tick_rate == get_tick_rate()) {
        return;
    }
    // Update this node
    RFNOC_LOG_TRACE("Setting tb clock freq to " << tick_rate / 1e6 << " MHz");
    _tb_clock_iface->set_freq(tick_rate);
    // Now trigger property propagation
    if (!_tick_rate_props.empty()) {
        auto src_info = _tick_rate_props.at(0).get_src_info();
        set_property<double>(PROP_KEY_TICK_RATE, tick_rate, src_info);
    }
}

void noc_block_base::_set_tick_rate(const double tick_rate)
{
    if (tick_rate == get_tick_rate()) {
        return;
    }
    if (tick_rate <= 0) {
        RFNOC_LOG_WARNING("Attempting to set tick rate to 0. Skipping.")
        return;
    }
    if (_tb_clock_iface->get_name() == CLOCK_KEY_GRAPH) {
        RFNOC_LOG_TRACE("Updating tick rate to " << (tick_rate / 1e6) << " MHz");
        _tb_clock_iface->set_freq(tick_rate);
    } else {
        RFNOC_LOG_WARNING("Cannot change tick rate to "
                          << (tick_rate / 1e6)
                          << " MHz, this clock is not configurable by the graph!");
    }
}

void noc_block_base::set_mtu_forwarding_policy(const forwarding_policy_t policy)
{
    // Error if the MTU forwarding policy has already been set--it can only be
    // set once per instance of the block
    if (_mtu_fwd_policy_set) {
        RFNOC_LOG_ERROR("Attempt to re-set MTU forwarding policy");
        throw uhd::runtime_error("MTU forwarding policy can only be set once per "
                                 "NoC block instance");
    }
    _mtu_fwd_policy_set = true;

    if (policy == forwarding_policy_t::DROP || policy == forwarding_policy_t::ONE_TO_ONE
        || policy == forwarding_policy_t::ONE_TO_ALL
        || policy == forwarding_policy_t::ONE_TO_FAN) {
        _mtu_fwd_policy = policy;
    } else {
        RFNOC_LOG_ERROR("Setting invalid MTU forwarding policy!");
        throw uhd::value_error("MTU forwarding policy must be either DROP, ONE_TO_ONE, "
                               "ONE_TO_ALL, or ONE_TO_FAN!");
    }

    // The behavior for DROP is already implemented in the default resolvers
    // that are registered in the `noc_block_base` constructor, so calling
    // `set_mtu_forwarding_policy(DROP)` is effectively a no-op (but legal,
    // so we should support it).
    if (policy == forwarding_policy_t::DROP) {
        return;
    }

    // Determine the set of MTU properties that should be in the output
    // sensitivity list based on the selected MTU forwarding policy. Note
    // that the input property itself (`prop` in the iteration below) is
    // not added to the output sensitivity list because it will have already
    // been resolved by its own separate default coercing resolver which was
    // registered at `noc_block_base` construction time. See the massive
    // comment in the constructor for why.
    for (auto& prop : _mtu_props) {
        const res_source_info src_edge = prop.get_src_info();
        prop_ptrs_t output_props{};
        output_props.reserve(_mtu_props.size());
        for (auto& other_prop : _mtu_props) {
            const res_source_info dst_edge = other_prop.get_src_info();
            bool add_to_output_props       = false;
            switch (_mtu_fwd_policy) {
                case forwarding_policy_t::ONE_TO_ONE:
                    add_to_output_props = res_source_info::invert_edge(dst_edge.type)
                                              == src_edge.type
                                          && dst_edge.instance == src_edge.instance;
                    break;
                case forwarding_policy_t::ONE_TO_ALL:
                    add_to_output_props = dst_edge.type != src_edge.type
                                          && dst_edge.instance != src_edge.instance;
                    break;
                case forwarding_policy_t::ONE_TO_FAN:
                    add_to_output_props = res_source_info::invert_edge(dst_edge.type)
                                          == src_edge.type;
                    break;
                default:
                    UHD_THROW_INVALID_CODE_PATH();
            }

            if (add_to_output_props) {
                output_props.push_back(&other_prop);
            }
        }

        add_property_resolver({&prop},
            std::move(output_props),
            [this, dst_props = output_props, source_prop = &prop]() {
                const res_source_info src_edge = source_prop->get_src_info();
                const size_t new_mtu = std::min(source_prop->get(), _mtu.at(src_edge));
                // Set the new MTU on all the dependent output MTU edge props
                for (auto& dst_prop : dst_props) {
                    auto mtu_prop =
                        dynamic_cast<uhd::rfnoc::property_t<size_t>*>(dst_prop);
                    RFNOC_LOG_TRACE("Forwarding new MTU value to edge "
                                    << mtu_prop->get_src_info().to_string());
                    mtu_prop->set(new_mtu);
                    _mtu.at(mtu_prop->get_src_info()) = mtu_prop->get();
                }
            });
    }
}

void noc_block_base::set_mtu(const res_source_info& edge, const size_t new_mtu)
{
    if (edge.type != res_source_info::INPUT_EDGE
        && edge.type != res_source_info::OUTPUT_EDGE) {
        throw uhd::value_error(
            "set_mtu() must be called on either an input or output edge!");
    }
    set_property<size_t>(PROP_KEY_MTU, new_mtu, edge);
}


size_t noc_block_base::get_mtu(const res_source_info& edge)
{
    if (!_mtu.count(edge)) {
        throw uhd::value_error(
            std::string("Cannot get MTU on edge: ") + edge.to_string());
    }
    return _mtu.at(edge);
}

size_t noc_block_base::get_chdr_hdr_len(const bool account_for_ts) const
{
    const size_t header_len_bytes = chdr_w_to_bits(_chdr_w) / 8;
    // 64-bit CHDR requires two lines for the header if we use a timestamp,
    // everything else requires one line
    const size_t num_hdr_lines = (account_for_ts && _chdr_w == CHDR_W_64) ? 2 : 1;
    return header_len_bytes * num_hdr_lines;
}

size_t noc_block_base::get_max_payload_size(
    const res_source_info& edge, const bool account_for_ts)
{
    return get_mtu(edge) - get_chdr_hdr_len(account_for_ts);
}

property_base_t* noc_block_base::get_mtu_prop_ref(const res_source_info& edge)
{
    for (size_t mtu_prop_idx = 0; mtu_prop_idx < _mtu_props.size(); mtu_prop_idx++) {
        if (_mtu_props.at(mtu_prop_idx).get_src_info() == edge) {
            return &_mtu_props.at(mtu_prop_idx);
        }
    }
    throw uhd::value_error(
        std::string("Could not find MTU property for edge: ") + edge.to_string());
}

void noc_block_base::shutdown()
{
    RFNOC_LOG_TRACE("Calling deinit()");
    deinit();
    RFNOC_LOG_TRACE("Invalidating register interface");
    update_reg_iface();
}

void noc_block_base::post_init()
{
    // Verify the block set its MTU forwarding policy. If not, set it to the
    // default value.
    if (!_mtu_fwd_policy_set) {
        RFNOC_LOG_INFO("Setting default MTU forward policy.");
        set_mtu_forwarding_policy(_mtu_fwd_policy);
    }
}

std::shared_ptr<mb_controller> noc_block_base::get_mb_controller()
{
    return _mb_controller;
}

void noc_block_base::deinit()
{
    RFNOC_LOG_TRACE("deinit() called, but not implemented.");
}

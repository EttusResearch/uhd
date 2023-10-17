//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../rfnoc_graph_mock_nodes.hpp"
#include <uhd/rfnoc/actions.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/multichan_register_iface.hpp>
#include <uhd/rfnoc/register_iface_holder.hpp>
#include <uhd/rfnoc/siggen_block_control.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

// Redeclare this here, since it's only defined outside of UHD_API
noc_block_base::make_args_t::~make_args_t() = default;

constexpr size_t NUM_PORTS   = 4;
constexpr size_t DEFAULT_MTU = 8000;

/*
 * This class extends mock_reg_iface_t, handling three particular registers
 */
class siggen_mock_reg_iface_t : public mock_reg_iface_t
{
public:
    siggen_mock_reg_iface_t(size_t num_ports)
        : _num_ports(num_ports)
        , enables(_num_ports, false)
        , spps(_num_ports, 0)
        , waveforms(_num_ports, siggen_waveform::CONSTANT)
        , gains(_num_ports, 0)
        , constants(_num_ports, 0)
        , phase_increments(_num_ports, 0)
        , phasors(_num_ports, 0)
    {
    }

    void _poke_cb(
        uint32_t addr, uint32_t data, uhd::time_spec_t /*time*/, bool /*ack*/) override
    {
        const size_t port = addr / siggen_block_control::REG_BLOCK_SIZE;
        if (port >= _num_ports) {
            throw uhd::assertion_error("Invalid port index");
        }

        const size_t offset = addr % siggen_block_control::REG_BLOCK_SIZE;
        if (offset == siggen_block_control::REG_ENABLE_OFFSET) {
            enables[port] = (data > 0);
        } else if (offset == siggen_block_control::REG_SPP_OFFSET) {
            spps[port] = data;
        } else if (offset == siggen_block_control::REG_WAVEFORM_OFFSET) {
            waveforms[port] = static_cast<siggen_waveform>(data);
        } else if (offset == siggen_block_control::REG_GAIN_OFFSET) {
            gains[port] = data;
        } else if (offset == siggen_block_control::REG_CONSTANT_OFFSET) {
            constants[port] = data;
        } else if (offset == siggen_block_control::REG_PHASE_INC_OFFSET) {
            phase_increments[port] = data;
        } else if (offset == siggen_block_control::REG_CARTESIAN_OFFSET) {
            phasors[port] = data;
        } else {
            throw uhd::assertion_error("Invalid write to out of bounds offset");
        }
    }

    void _peek_cb(uint32_t addr, uhd::time_spec_t /*time*/) override
    {
        const size_t port = addr / siggen_block_control::REG_BLOCK_SIZE;
        if (port >= _num_ports) {
            throw uhd::assertion_error("Invalid port index");
        }

        const size_t offset = addr % siggen_block_control::REG_BLOCK_SIZE;
        if (offset == siggen_block_control::REG_ENABLE_OFFSET) {
            read_memory[addr] = enables[port] ? 1 : 0;
        } else if (offset == siggen_block_control::REG_SPP_OFFSET) {
            read_memory[addr] = spps[port];
        } else if (offset == siggen_block_control::REG_WAVEFORM_OFFSET) {
            read_memory[addr] = static_cast<uint32_t>(waveforms[port]);
        } else if (offset == siggen_block_control::REG_GAIN_OFFSET) {
            read_memory[addr] = gains[port];
        } else if (offset == siggen_block_control::REG_CONSTANT_OFFSET) {
            read_memory[addr] = constants[port];
        } else if (offset == siggen_block_control::REG_PHASE_INC_OFFSET) {
            read_memory[addr] = phase_increments[port];
        } else if (offset == siggen_block_control::REG_CARTESIAN_OFFSET) {
            read_memory[addr] = phasors[port];
        } else {
            throw uhd::assertion_error("Invalid read from out of bounds offset");
        }
    }

    template <class T>
    static const T clamp(const double v)
    {
        constexpr T min_t = std::numeric_limits<T>::min();
        constexpr T max_t = std::numeric_limits<T>::max();
        return (v < min_t) ? min_t : (v > max_t) ? max_t : T(v);
    }

    static uint32_t gain_to_register(double gain)
    {
        const int16_t gain_fp = clamp<int16_t>(gain * 32768.0);
        return static_cast<uint32_t>(gain_fp) & 0xffff;
    }

    static uint32_t constant_to_register(std::complex<double> constant)
    {
        const int16_t constant_i_fp = clamp<int16_t>(constant.real() * 32768.0);
        const int16_t constant_q_fp = clamp<int16_t>(constant.imag() * 32768.0);
        return (uint32_t(constant_i_fp) << 16) | (uint32_t(constant_q_fp) & 0xffff);
    }

    static uint32_t phase_increment_to_register(double phase_inc)
    {
        const int16_t phase_inc_scaled_rads_fp =
            clamp<int16_t>((phase_inc / uhd::math::PI) * 8192.0);
        return static_cast<uint32_t>(phase_inc_scaled_rads_fp) & 0xffff;
    }

    static uint32_t phasor_to_register(std::complex<double> phasor)
    {
        phasor /= 1.164435344782938; /* CORDIC scale value--see siggen_block_control */
        const int16_t phasor_i_fp = clamp<int16_t>(phasor.real() * 32767.0);
        const int16_t phasor_q_fp = clamp<int16_t>(phasor.imag() * 32767.0);
        return (uint32_t(phasor_i_fp) << 16) | (uint32_t(phasor_q_fp) & 0xffff);
    }

private:
    const size_t _num_ports;

public:
    std::vector<bool> enables;
    std::vector<uint32_t> spps;
    std::vector<siggen_waveform> waveforms;
    std::vector<uint32_t> gains;
    std::vector<uint32_t> constants;
    std::vector<uint32_t> phase_increments;
    std::vector<uint32_t> phasors;
};

/*
 * siggen_block_fixture is a class which is instantiated before each test
 * case is run. It sets up the block container, mock register interface,
 * and siggen_block_control object, all of which are accessible to the test
 * case. The instance of the object is destroyed at the end of each test
 * case.
 */
struct siggen_block_fixture
{
    siggen_block_fixture()
        : reg_iface(std::make_shared<siggen_mock_reg_iface_t>(NUM_PORTS))
        , block_container(get_mock_block(SIGGEN_BLOCK,
              NUM_PORTS,
              NUM_PORTS,
              uhd::device_addr_t(),
              DEFAULT_MTU,
              ANY_DEVICE,
              reg_iface))
        , test_siggen(block_container.get_block<siggen_block_control>())
    {
        node_accessor.init_props(test_siggen.get());
    }

    std::shared_ptr<siggen_mock_reg_iface_t> reg_iface;
    mock_block_container block_container;
    std::shared_ptr<siggen_block_control> test_siggen;
    node_accessor_t node_accessor{};
};

/*
 * This test case ensures that the hardware is programmed correctly with
 * defaults when the siggen block is constructed.
 */
BOOST_FIXTURE_TEST_CASE(siggen_test_construction, siggen_block_fixture)
{
    for (size_t port = 0; port < NUM_PORTS; port++) {
        BOOST_CHECK_EQUAL(reg_iface->enables.at(port), 0);
        BOOST_CHECK(reg_iface->waveforms.at(port) == siggen_waveform::CONSTANT);
        BOOST_CHECK_EQUAL(
            reg_iface->gains.at(port), siggen_mock_reg_iface_t::gain_to_register(1.0));
        BOOST_CHECK_EQUAL(reg_iface->constants.at(port),
            siggen_mock_reg_iface_t::constant_to_register({1.0, 1.0}));
        BOOST_CHECK_EQUAL(reg_iface->phase_increments.at(port),
            siggen_mock_reg_iface_t::phase_increment_to_register(1.0));
        BOOST_CHECK_EQUAL(reg_iface->phasors.at(port),
            siggen_mock_reg_iface_t::phasor_to_register({0.0, 0.0}));
        constexpr int bpi = 4; // sc16 has 4 bytes per item
        const int expected_spp =
            test_siggen->get_max_payload_size({res_source_info::OUTPUT_EDGE, port}) / bpi;
        BOOST_CHECK_EQUAL(reg_iface->spps.at(port), expected_spp);
    }
}

/*
 * This test case exercises the API and ensures that the registers are
 * programmed appropriately.
 */
BOOST_FIXTURE_TEST_CASE(siggen_test_api, siggen_block_fixture)
{
    for (size_t port = 0; port < NUM_PORTS; port++) {
        test_siggen->set_enable(true, port);
        BOOST_CHECK_EQUAL(reg_iface->enables.at(port), 1);
        BOOST_CHECK_EQUAL(test_siggen->get_enable(port), true);

        // Set constant mode on the function generator, and then make sure
        // that the gain register stays fixed at 1 and that changing the
        // amplitude has no effect on the register or the attribute value
        // (which should always return 1 in this mode).
        const double amplitude = 0.25 + (port * 0.1);
        test_siggen->set_waveform(siggen_waveform::CONSTANT, port);
        BOOST_CHECK(reg_iface->waveforms.at(port) == siggen_waveform::CONSTANT);
        BOOST_CHECK(test_siggen->get_waveform(port) == siggen_waveform::CONSTANT);
        test_siggen->set_amplitude(amplitude, port);
        BOOST_CHECK_EQUAL(
            reg_iface->gains.at(port), siggen_mock_reg_iface_t::gain_to_register(1.0));
        BOOST_CHECK_EQUAL(test_siggen->get_amplitude(port), 1.0);

        // Set sine wave mode on the function generator, and then make sure
        // that the gain register stays fixed at 1, but that the Cartesian
        // register changes to reflect the desired sinusoidal amplitude.
        test_siggen->set_waveform(siggen_waveform::SINE_WAVE, port);
        BOOST_CHECK(reg_iface->waveforms.at(port) == siggen_waveform::SINE_WAVE);
        test_siggen->set_amplitude(amplitude, port);
        BOOST_CHECK_EQUAL(
            reg_iface->gains.at(port), siggen_mock_reg_iface_t::gain_to_register(1.0));
        BOOST_CHECK_EQUAL(reg_iface->phasors.at(port),
            siggen_mock_reg_iface_t::phasor_to_register({amplitude, 0.0}));
        BOOST_CHECK_EQUAL(test_siggen->get_amplitude(port), amplitude);

        // Set noise mode on the function generator, and then make sure
        // that the gain register changes according to the amplitude.
        test_siggen->set_waveform(siggen_waveform::NOISE, port);
        BOOST_CHECK(reg_iface->waveforms.at(port) == siggen_waveform::NOISE);
        BOOST_CHECK(test_siggen->get_waveform(port) == siggen_waveform::NOISE);
        test_siggen->set_amplitude(amplitude, port);
        BOOST_CHECK_EQUAL(reg_iface->gains.at(port),
            siggen_mock_reg_iface_t::gain_to_register(amplitude));
        BOOST_CHECK_EQUAL(test_siggen->get_amplitude(port), amplitude);

        const std::complex<double> constant{-0.5 - (port * 0.05), 0.5 + (port * 0.05)};
        test_siggen->set_constant(constant, port);
        BOOST_CHECK_EQUAL(reg_iface->constants.at(port),
            siggen_mock_reg_iface_t::constant_to_register(constant));
        BOOST_CHECK_EQUAL(test_siggen->get_constant(port), constant);

        const double phase_inc = (port * uhd::math::PI / 16.0);
        test_siggen->set_sine_phase_increment(phase_inc, port);
        BOOST_CHECK_EQUAL(reg_iface->phase_increments.at(port),
            siggen_mock_reg_iface_t::phase_increment_to_register(phase_inc));
        BOOST_CHECK_EQUAL(test_siggen->get_sine_phase_increment(port), phase_inc);

        const double freq      = 1000 + (100 * port);
        const double samp_rate = 1e6;
        test_siggen->set_sine_frequency(freq, samp_rate, port);
        const double calculated_phase_inc = freq / samp_rate * 2.0 * uhd::math::PI;
        BOOST_CHECK_EQUAL(reg_iface->phase_increments.at(port),
            siggen_mock_reg_iface_t::phase_increment_to_register(calculated_phase_inc));
        BOOST_CHECK_EQUAL(
            test_siggen->get_sine_phase_increment(port), calculated_phase_inc);

        size_t spp = 100 + (200 * port);
        test_siggen->set_samples_per_packet(spp, port);
        BOOST_CHECK_EQUAL(reg_iface->spps.at(port), spp);
        BOOST_CHECK_EQUAL(test_siggen->get_samples_per_packet(port), spp);
    }
}

/*
 * This test case exercises the range checking performed on the siggen
 * settings, ensuring that the appropriate exception is thrown when out of
 * range.
 */
BOOST_FIXTURE_TEST_CASE(siggen_test_ranges, siggen_block_fixture)
{
    for (size_t port = 0; port < NUM_PORTS; port++) {
        BOOST_CHECK_THROW(
            test_siggen->set_property<int>("waveform", 100, port), uhd::value_error);

        const double bad_amplitude = 100 + (port * 100);
        // I got a bad amplitude!
        BOOST_CHECK_THROW(
            test_siggen->set_amplitude(bad_amplitude, port), uhd::value_error);
        BOOST_CHECK_THROW(
            test_siggen->set_amplitude(-bad_amplitude, port), uhd::value_error);

        const std::complex<double> bad_constant_i{-100.0 + (port * 100), 0.1};
        BOOST_CHECK_THROW(
            test_siggen->set_constant(bad_constant_i, port), uhd::value_error);
        BOOST_CHECK_THROW(
            test_siggen->set_constant(-bad_constant_i, port), uhd::value_error);

        const std::complex<double> bad_constant_q{-0.1, 100.0 + (port * 100)};
        BOOST_CHECK_THROW(
            test_siggen->set_constant(bad_constant_q, port), uhd::value_error);
        BOOST_CHECK_THROW(
            test_siggen->set_constant(-bad_constant_q, port), uhd::value_error);

        const double bad_phase_inc = 5 * uhd::math::PI;
        BOOST_CHECK_THROW(
            test_siggen->set_sine_phase_increment(bad_phase_inc, port), uhd::value_error);
        BOOST_CHECK_THROW(test_siggen->set_sine_phase_increment(-bad_phase_inc, port),
            uhd::value_error);

        const double bad_samp_rate_zero = 0.0;
        BOOST_CHECK_THROW(test_siggen->set_sine_frequency(10.0, bad_samp_rate_zero, port),
            uhd::value_error);

        const double bad_samp_rate = 30.0 + (port * 10);
        const double bad_freq      = bad_samp_rate * 10.0;
        BOOST_CHECK_THROW(test_siggen->set_sine_frequency(bad_freq, bad_samp_rate, port),
            uhd::value_error);
    }
}

/*
 * This test case exercises the coercion of the SPP parameter to ensure that
 * it does not surpass the MTU.
 */
BOOST_FIXTURE_TEST_CASE(siggen_test_spp_coercion, siggen_block_fixture)
{
    const size_t max_samps =
        (DEFAULT_MTU - test_siggen->get_chdr_hdr_len()) / 4 /* bytes/samp */;
    const size_t too_high_spp = max_samps + 10;
    test_siggen->set_samples_per_packet(too_high_spp, 0);
    BOOST_CHECK_EQUAL(test_siggen->get_samples_per_packet(0), max_samps);
}

/*
 * This test case ensures that the siggen block controller can be added
 * to a graph.
 */
BOOST_FIXTURE_TEST_CASE(siggen_test_graph, siggen_block_fixture)
{
    detail::graph_t graph{};
    detail::graph_t::graph_edge_t edge_info{
        0, 0, detail::graph_t::graph_edge_t::DYNAMIC, true};

    mock_terminator_t mock_sink_term(NUM_PORTS, {}, "MOCK_SINK");

    mock_sink_term.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::INPUT_EDGE, 0});

    UHD_LOG_INFO("TEST", "Creating graph...");
    for (size_t port = 0; port < NUM_PORTS; port++) {
        graph.connect(test_siggen.get(),
            &mock_sink_term,
            {port, port, detail::graph_t::graph_edge_t::DYNAMIC, true});
    }
    UHD_LOG_INFO("TEST", "Committing graph...");
    graph.commit();
    UHD_LOG_INFO("TEST", "Commit complete.");
}

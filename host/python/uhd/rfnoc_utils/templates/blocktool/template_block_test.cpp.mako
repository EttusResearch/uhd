//
// Copyright ${year} ${copyright_holder}
//
// ${license}
//

#include <uhd/rfnoc/detail/graph.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/mock_nodes.hpp>
#include <uhd/rfnoc/node_accessor.hpp>
#include <rfnoc/${MODULE_NAME}/${blockname}_block_control.hpp>
#include <boost/test/unit_test.hpp>
#include <memory>

using namespace uhd::rfnoc;
using namespace uhd::rfnoc::test;
using namespace rfnoc::${MODULE_NAME};

// Redeclare this here, since it's only defined outside of UHD_API
noc_block_base::make_args_t::~make_args_t() = default;

// Fix these based on your block design
constexpr size_t NUM_INPUT_CHANS = 1;
constexpr size_t NUM_OUTPUT_CHANS = 1;

/*
 * ${blockname}_block_fixture is a class which is instantiated before each test
 * case is run. It sets up the block container, ${blockname}_block_control
 * object and node accessor, all of which are accessible to the test case.
 * The instance of the object is destroyed at the end of each test case.
 */
struct ${blockname}_block_fixture
{
    ${blockname}_block_fixture()
        : block_container(get_mock_block(
            ${config["noc_id"]}, NUM_INPUT_CHANS, NUM_OUTPUT_CHANS, uhd::device_addr_t()))
        , test_${blockname}(block_container.get_block<${blockname}_block_control>())
    {
        node_accessor.init_props(test_${blockname}.get());
    }

    mock_block_container block_container;
    std::shared_ptr<${blockname}_block_control> test_${blockname};
    // The node_accessor is a C++ construct to bypass the public/private
    // division of the underlying C++ class. This should never be used in
    // production outside of unit tests, but here, it lets us peek inside the
    // class to verify it's working as expected.
    node_accessor_t node_accessor{};
};

/*
 * This just does some basic tests, like instantiate a block controller, check
 * the NoC ID is correctly read back, etc. Add your own tests here depending on
 * the block's functionality. Typical things to test include:
 * - Setting/getting properties, make sure they trigger the right actions
 * - Check register writes/reads get correctly executed (the block_container
 *   has a mock register space)
 * - Check action interface by using the mock source/sink to input actions
 * - Check property propagation by using the mock source/sink to modify and
 *   read edge properties
 */
BOOST_FIXTURE_TEST_CASE(${blockname}_test_basic, ${blockname}_block_fixture)
{
    detail::graph_t graph{};

    mock_terminator_t mock_source_term(NUM_INPUT_CHANS);
    mock_terminator_t mock_sink_term(NUM_OUTPUT_CHANS);

    constexpr size_t chan = 0;

    UHD_LOG_INFO("TEST", "Creating graph...");
    detail::graph_t::graph_edge_t edge_info{
        chan, chan, detail::graph_t::graph_edge_t::DYNAMIC, true};
    // In your real tests, connect the block appropriately
    //graph.connect(&mock_source_term, test_${blockname}.get(), edge_info);
    //graph.connect(test_${blockname}.get(), &mock_sink_term, edge_info);

    UHD_LOG_INFO("TEST", "Committing graph...");
    graph.commit();
    UHD_LOG_INFO("TEST", "Commit complete.");

    // Now implement some proper tests.
    BOOST_CHECK_EQUAL(test_${blockname}->get_noc_id(), ${config["noc_id"]});
}

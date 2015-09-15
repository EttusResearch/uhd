//
// Copyright 2014-2015 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <iostream>
#include <map>
#include <boost/cstdint.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <uhd/rfnoc/blockdef.hpp>

using namespace uhd::rfnoc;

BOOST_AUTO_TEST_CASE(test_lookup) {
    std::map<boost::uint64_t, std::string> blocknames = boost::assign::list_of< std::pair<boost::uint64_t, std::string> >
        (0,                  "NullSrcSink")
        (0xFF70000000000000, "FFT")
        (0xF112000000000001, "FIR")
        (0xF1F0000000000000, "FIFO")
        (0xD053000000000000, "Window")
        (0x5CC0000000000000, "SchmidlCox")
    ;

    std::cout << blocknames.size() << std::endl;

    for (std::map<boost::uint64_t, std::string>::iterator it = blocknames.begin(); it != blocknames.end(); ++it) {
        std::cout << "Testing " << it->second << " => " << str(boost::format("%016X") % it->first) << std::endl;
        blockdef::sptr block_definition = blockdef::make_from_noc_id(it->first);
        // If the previous function fails, it'll return a NULL pointer
        BOOST_REQUIRE(block_definition);
        BOOST_CHECK(block_definition->is_block());
        BOOST_CHECK_EQUAL(block_definition->get_name(), it->second);
    }
}

BOOST_AUTO_TEST_CASE(test_ports) {
    // Create an FFT:
    blockdef::sptr block_definition = blockdef::make_from_noc_id(0xFF70000000000000);
    blockdef::ports_t in_ports = block_definition->get_input_ports();
    BOOST_REQUIRE_EQUAL(in_ports.size(), 1);
    BOOST_CHECK_EQUAL(in_ports[0]["name"], "in");
    BOOST_CHECK_EQUAL(in_ports[0]["type"], "sc16");
    BOOST_CHECK(in_ports[0].has_key("vlen"));
    BOOST_CHECK(in_ports[0].has_key("pkt_size"));

    blockdef::ports_t out_ports = block_definition->get_output_ports();
    BOOST_REQUIRE_EQUAL(out_ports.size(), 1);
    BOOST_CHECK_EQUAL(out_ports[0]["name"], "out");
    BOOST_CHECK(out_ports[0].has_key("vlen"));
    BOOST_CHECK(out_ports[0].has_key("pkt_size"));

    BOOST_CHECK_EQUAL(block_definition->get_all_port_numbers().size(), 1);
    BOOST_CHECK_EQUAL(block_definition->get_all_port_numbers()[0], 0);
}

BOOST_AUTO_TEST_CASE(test_args) {
    // Create an FFT:
    blockdef::sptr block_definition = blockdef::make_from_noc_id(0xFF70000000000000);
    blockdef::args_t args = block_definition->get_args();
    BOOST_REQUIRE(args.size() >= 3);
    BOOST_CHECK_EQUAL(args[0]["name"], "spp");
    BOOST_CHECK_EQUAL(args[0]["type"], "int");
    BOOST_CHECK_EQUAL(args[0]["value"], "256");
}

BOOST_AUTO_TEST_CASE(test_regs) {
    // Create an FFT:
    blockdef::sptr block_definition = blockdef::make_from_noc_id(0xFF70000000000000);
    blockdef::registers_t sregs = block_definition->get_settings_registers();
    BOOST_REQUIRE_EQUAL(sregs.size(), 3);
    BOOST_CHECK_EQUAL(sregs["FFT_RESET"], 131);
    BOOST_CHECK_EQUAL(sregs["FFT_SIZE_LOG2"], 132);
    BOOST_CHECK_EQUAL(sregs["MAGNITUDE_OUT"], 133);
    blockdef::registers_t user_regs = block_definition->get_readback_registers();
    BOOST_REQUIRE_EQUAL(user_regs.size(), 2);
    BOOST_CHECK_EQUAL(user_regs["RB_FFT_RESET"], 0);
    BOOST_CHECK_EQUAL(user_regs["RB_MAGNITUDE_OUT"], 1);
}


//
// Copyright 2010 Ettus Research LLC
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

#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <cstdlib>
#include <complex>
#include <iostream>

namespace po = boost::program_options;

/*!
 * Test the burst ack message:
 *    Send a burst of many samples that will fragment internally.
 *    We expect to get an burst ack async message.
 */
bool test_burst_ack_message(uhd::usrp::multi_usrp::sptr usrp){
    std::cout << "Test burst ack message... " << std::flush;

    uhd::tx_metadata_t md;
    md.start_of_burst = true;
    md.end_of_burst   = true;
    md.has_time_spec  = false;

    //3 times max-sps guarantees a SOB, no burst, and EOB packet
    std::vector<std::complex<float> > buff(usrp->get_device()->get_max_send_samps_per_packet()*3);

    usrp->get_device()->send(
        &buff.front(), buff.size(), md,
        uhd::io_type_t::COMPLEX_FLOAT32,
        uhd::device::SEND_MODE_FULL_BUFF
    );

    uhd::async_metadata_t async_md;
    if (not usrp->get_device()->recv_async_msg(async_md)){
        std::cout << boost::format(
            "failed:\n"
            "    Async message recv timed out.\n"
        ) << std::endl;
        return false;
    }

    switch(async_md.event_code){
    case uhd::async_metadata_t::EVENT_CODE_BURST_ACK:
        std::cout << boost::format(
            "success:\n"
            "    Got event code burst ack message.\n"
        ) << std::endl;
        return true;

    default:
        std::cout << boost::format(
            "failed:\n"
            "    Got unexpected event code 0x%x.\n"
        ) % async_md.event_code << std::endl;
        return false;
    }
}

/*!
 * Test the underflow message:
 *    Send a start of burst packet with no following end of burst.
 *    We expect to get an underflow(within a burst) async message.
 */
bool test_underflow_message(uhd::usrp::multi_usrp::sptr usrp){
    std::cout << "Test underflow message... " << std::flush;

    uhd::tx_metadata_t md;
    md.start_of_burst = true;
    md.end_of_burst   = false;
    md.has_time_spec  = false;

    usrp->get_device()->send(
        NULL, 0, md,
        uhd::io_type_t::COMPLEX_FLOAT32,
        uhd::device::SEND_MODE_FULL_BUFF
    );

    uhd::async_metadata_t async_md;
    if (not usrp->get_device()->recv_async_msg(async_md, 1)){
        std::cout << boost::format(
            "failed:\n"
            "    Async message recv timed out.\n"
        ) << std::endl;
        return false;
    }

    switch(async_md.event_code){
    case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW:
        std::cout << boost::format(
            "success:\n"
            "    Got event code underflow message.\n"
        ) << std::endl;
        return true;

    default:
        std::cout << boost::format(
            "failed:\n"
            "    Got unexpected event code 0x%x.\n"
        ) % async_md.event_code << std::endl;
        return false;
    }
}

/*!
 * Test the time error message:
 *    Send a burst packet that occurs at a time in the past.
 *    We expect to get a time error async message.
 */
bool test_time_error_message(uhd::usrp::multi_usrp::sptr usrp){
    std::cout << "Test time error message... " << std::flush;

    uhd::tx_metadata_t md;
    md.start_of_burst = true;
    md.end_of_burst   = true;
    md.has_time_spec  = true;
    md.time_spec      = uhd::time_spec_t(100.0); //send at 100s

    usrp->set_time_now(uhd::time_spec_t(200.0)); //time at 200s

    usrp->get_device()->send(
        NULL, 0, md,
        uhd::io_type_t::COMPLEX_FLOAT32,
        uhd::device::SEND_MODE_FULL_BUFF
    );

    uhd::async_metadata_t async_md;
    if (not usrp->get_device()->recv_async_msg(async_md)){
        std::cout << boost::format(
            "failed:\n"
            "    Async message recv timed out.\n"
        ) << std::endl;
        return false;
    }

    switch(async_md.event_code){
    case uhd::async_metadata_t::EVENT_CODE_TIME_ERROR:
        std::cout << boost::format(
            "success:\n"
            "    Got event code time error message.\n"
        ) << std::endl;
        return true;

    default:
        std::cout << boost::format(
            "failed:\n"
            "    Got unexpected event code 0x%x.\n"
        ) % async_md.event_code << std::endl;
        return false;
    }
}

void flush_async_md(uhd::usrp::multi_usrp::sptr usrp){
    uhd::async_metadata_t async_md;
    while (usrp->get_device()->recv_async_msg(async_md, 1.0)){}
}

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args;
    double rate;
    size_t ntests;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args",   po::value<std::string>(&args)->default_value(""), "multi uhd device address args")
        ("rate",   po::value<double>(&rate)->default_value(1.5e6),   "rate of outgoing samples")
        ("ntests", po::value<size_t>(&ntests)->default_value(10),    "number of tests to run")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("UHD Test Async Messages %s") % desc << std::endl;
        return ~0;
    }

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    //set the tx sample rate
    std::cout << boost::format("Setting TX Rate: %f Msps...") % (rate/1e6) << std::endl;
    usrp->set_tx_rate(rate);
    std::cout << boost::format("Actual TX Rate: %f Msps...") % (usrp->get_tx_rate()/1e6) << std::endl << std::endl;

    //------------------------------------------------------------------
    // begin asyc messages test
    //------------------------------------------------------------------
    static const uhd::dict<std::string, boost::function<bool(uhd::usrp::multi_usrp::sptr)> >
        tests = boost::assign::map_list_of
        ("Test Burst ACK ", &test_burst_ack_message)
        ("Test Underflow ", &test_underflow_message)
        ("Test Time Error", &test_time_error_message)
    ;

    //init result counts
    uhd::dict<std::string, size_t> failures, successes;
    BOOST_FOREACH(const std::string &key, tests.keys()){
        failures[key] = 0;
        successes[key] = 0;
    }

    //run the tests, pick at random
    for (size_t n = 0; n < ntests; n++){
        std::string key = tests.keys()[std::rand() % tests.size()];
        bool pass = tests[key](usrp);
        flush_async_md(usrp);

        //store result
        if (pass) successes[key]++;
        else      failures[key]++;
    }

    //print the result summary
    std::cout << std::endl << "Summary:" << std::endl << std::endl;
    BOOST_FOREACH(const std::string &key, tests.keys()){
        std::cout << boost::format(
            "%s   ->   %3d successes, %3d failures"
        ) % key % successes[key] % failures[key] << std::endl;
    }

    //finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return 0;
}

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
#include <uhd/usrp/single_usrp.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <complex>
#include <iostream>

namespace po = boost::program_options;

static const size_t async_to_ms = 100;

/*!
 * Test that no messages are received:
 *    Send a burst of many samples that will fragment internally.
 *    We expect to not get any async messages.
 */
void test_no_async_message(uhd::usrp::single_usrp::sptr sdev){
    uhd::device::sptr dev = sdev->get_device();
    std::cout << "Test no async message... " << std::flush;

    uhd::tx_metadata_t md;
    md.start_of_burst = true;
    md.end_of_burst   = true;
    md.has_time_spec  = false;

    //3 times max-sps guarantees a SOB, no burst, and EOB packet
    std::vector<std::complex<float> > buff(dev->get_max_send_samps_per_packet()*3);

    dev->send(
        &buff.front(), buff.size(), md,
        uhd::io_type_t::COMPLEX_FLOAT32,
        uhd::device::SEND_MODE_FULL_BUFF
    );

    uhd::async_metadata_t async_md;
    if (dev->recv_async_msg(async_md, async_to_ms)){
        std::cout << boost::format(
            "failed:\n"
            "    Got unexpected event code 0x%x.\n"
        ) % async_md.event_code << std::endl;
        //clear the async messages
        while (dev->recv_async_msg(async_md, 0)){};
    }
    else{
        std::cout << boost::format(
            "success:\n"
            "    Did not get an async message.\n"
        ) << std::endl;
    }
}

/*!
 * Test the underflow message:
 *    Send a start of burst packet with no following end of burst.
 *    We expect to get an underflow(within a burst) async message.
 */
void test_underflow_message(uhd::usrp::single_usrp::sptr sdev){
    uhd::device::sptr dev = sdev->get_device();
    std::cout << "Test underflow message... " << std::flush;

    uhd::tx_metadata_t md;
    md.start_of_burst = true;
    md.end_of_burst   = false;
    md.has_time_spec  = false;

    dev->send(NULL, 0, md,
        uhd::io_type_t::COMPLEX_FLOAT32,
        uhd::device::SEND_MODE_FULL_BUFF
    );

    uhd::async_metadata_t async_md;
    if (not dev->recv_async_msg(async_md, async_to_ms)){
        std::cout << boost::format(
            "failed:\n"
            "    Async message recv timed out.\n"
        ) << std::endl;
        return;
    }

    switch(async_md.event_code){
    case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW:
        std::cout << boost::format(
            "success:\n"
            "    Got event code underflow message.\n"
        ) << std::endl;
        break;

    default:
        std::cout << boost::format(
            "failed:\n"
            "    Got unexpected event code 0x%x.\n"
        ) % async_md.event_code << std::endl;
    }
}

/*!
 * Test the time error message:
 *    Send a burst packet that occurs at a time in the past.
 *    We expect to get a time error async message.
 */
void test_time_error_message(uhd::usrp::single_usrp::sptr sdev){
    uhd::device::sptr dev = sdev->get_device();
    std::cout << "Test time error message... " << std::flush;

    uhd::tx_metadata_t md;
    md.start_of_burst = true;
    md.end_of_burst   = true;
    md.has_time_spec  = true;
    md.time_spec      = uhd::time_spec_t(100.0); //send at 100s

    sdev->set_time_now(uhd::time_spec_t(200.0)); //time at 200s

    dev->send(NULL, 0, md,
        uhd::io_type_t::COMPLEX_FLOAT32,
        uhd::device::SEND_MODE_FULL_BUFF
    );

    uhd::async_metadata_t async_md;
    if (not dev->recv_async_msg(async_md, async_to_ms)){
        std::cout << boost::format(
            "failed:\n"
            "    Async message recv timed out.\n"
        ) << std::endl;
        return;
    }

    switch(async_md.event_code){
    case uhd::async_metadata_t::EVENT_CODE_TIME_ERROR:
        std::cout << boost::format(
            "success:\n"
            "    Got event code time error message.\n"
        ) << std::endl;
        break;

    default:
        std::cout << boost::format(
            "failed:\n"
            "    Got unexpected event code 0x%x.\n"
        ) % async_md.event_code << std::endl;
    }
}

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args;
    double rate;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args")
        ("rate", po::value<double>(&rate)->default_value(1.5e6), "rate of outgoing samples")
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
    uhd::usrp::single_usrp::sptr sdev = uhd::usrp::single_usrp::make(args);
    std::cout << boost::format("Using Device: %s") % sdev->get_pp_string() << std::endl;

    //set the tx sample rate
    std::cout << boost::format("Setting TX Rate: %f Msps...") % (rate/1e6) << std::endl;
    sdev->set_tx_rate(rate);
    std::cout << boost::format("Actual TX Rate: %f Msps...") % (sdev->get_tx_rate()/1e6) << std::endl << std::endl;

    //------------------------------------------------------------------
    // begin asyc messages test
    //------------------------------------------------------------------
    test_no_async_message(sdev);
    test_underflow_message(sdev);
    test_time_error_message(sdev);

    //finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return 0;
}

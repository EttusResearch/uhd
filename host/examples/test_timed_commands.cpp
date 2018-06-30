//
// Copyright 2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/thread.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <complex>

namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("UHD Test Timed Commands %s") % desc << std::endl;
        return ~0;
    }

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    //check if timed commands are supported
    std::cout << std::endl;
    std::cout << "Testing support for timed commands on this hardware... " << std::flush;
    try{
        usrp->set_command_time(uhd::time_spec_t(0.0));
        usrp->clear_command_time();
    }
    catch (const std::exception &e){
        std::cout << "fail" << std::endl;
        std::cerr << "Got exception: " << e.what() << std::endl;
        std::cerr << "Timed commands are not supported on this hardware." << std::endl;
        return ~0;
    }
    std::cout << "pass" << std::endl;

    //readback time really fast, time diff is small
    std::cout << std::endl;
    std::cout << "Perform fast readback of registers:" << std::endl;
    uhd::time_spec_t total_time;
    for (size_t i = 0; i < 100; i++){
        const uhd::time_spec_t t0 = usrp->get_time_now();
        const uhd::time_spec_t t1 = usrp->get_time_now();
        total_time += (t1-t0);
    }
    std::cout << boost::format(
        " Difference between paired reads: %f us"
    ) % (total_time.get_real_secs()/100*1e6) << std::endl;

    //test timed control command
    //issues get_time_now() command twice a fixed time apart
    //outputs difference for each response time vs. the expected time
    //and difference between actual and expected time deltas
    std::cout << std::endl;
    std::cout << "Testing control timed command:" << std::endl;
    const uhd::time_spec_t span = uhd::time_spec_t(0.1);
    const uhd::time_spec_t now = usrp->get_time_now();
    const uhd::time_spec_t cmd_time1 = now + uhd::time_spec_t(0.1);
    const uhd::time_spec_t cmd_time2 = cmd_time1 + span;
    usrp->set_command_time(cmd_time1);
    uhd::time_spec_t response_time1 = usrp->get_time_now();
    usrp->set_command_time(cmd_time2);
    uhd::time_spec_t response_time2 = usrp->get_time_now();
    usrp->clear_command_time();
    std::cout << boost::format(
        " Span      : %f us\n"
        " Now       : %f us\n"
        " Response 1: %f us\n"
        " Response 2: %f us"
    ) % (span.get_real_secs()*1e6) % (now.get_real_secs()*1e6) % (response_time1.get_real_secs()*1e6) % (response_time2.get_real_secs()*1e6) << std::endl;
    std::cout << boost::format(
        " Difference of response time 1: %f us"
    ) % ((response_time1 - cmd_time1).get_real_secs()*1e6) << std::endl;
    std::cout << boost::format(
        " Difference of response time 2: %f us"
    ) % ((response_time2 - cmd_time2).get_real_secs()*1e6) << std::endl;
    std::cout << boost::format(
        " Difference between actual and expected time delta: %f us"
    ) % ((response_time2 - response_time1 - span).get_real_secs()*1e6) << std::endl;

    //use a timed command to start a stream at a specific time
    //this is not the right way start streaming at time x,
    //but it should approximate it within control RTT/2
    //setup streaming
    std::cout << std::endl;
    std::cout << "About to start streaming using timed command:" << std::endl;

    //create a receive streamer
    uhd::stream_args_t stream_args("fc32"); //complex floats
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps = 100;
    stream_cmd.stream_now = false;
    const uhd::time_spec_t stream_time = usrp->get_time_now() + uhd::time_spec_t(0.1);
    stream_cmd.time_spec = stream_time;
    rx_stream->issue_stream_cmd(stream_cmd);

    //meta-data will be filled in by recv()
    uhd::rx_metadata_t md;

    //allocate buffer to receive with samples
    std::vector<std::complex<float> > buff(stream_cmd.num_samps);

    const size_t num_rx_samps = rx_stream->recv(&buff.front(), buff.size(), md, 1.0);
    if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE){
        throw std::runtime_error(str(boost::format(
            "Receiver error %s"
        ) % md.strerror()));
    }
    std::cout << boost::format(
        " Received packet: %u samples, %u full secs, %f frac secs"
    ) % num_rx_samps % md.time_spec.get_full_secs() % md.time_spec.get_frac_secs() << std::endl;
    std::cout << boost::format(
        " Stream time was: %u full secs, %f frac secs"
    ) % stream_time.get_full_secs() % stream_time.get_frac_secs() << std::endl;
    std::cout << boost::format(
        " Difference between stream time and first packet: %f us"
    ) % ((md.time_spec-stream_time).get_real_secs()*1e6) << std::endl;

    //finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}

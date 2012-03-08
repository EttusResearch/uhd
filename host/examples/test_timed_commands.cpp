//
// Copyright 2012 Ettus Research LLC
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
        "Difference between paired reads: %f us"
    ) % (total_time.get_real_secs()/100*1e6) << std::endl;

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
    stream_cmd.stream_now = true;
    const uhd::time_spec_t stream_time = usrp->get_time_now() + uhd::time_spec_t(0.1);
    usrp->set_command_time(stream_time);
    usrp->issue_stream_cmd(stream_cmd);
    usrp->clear_command_time();

    //meta-data will be filled in by recv()
    uhd::rx_metadata_t md;

    //allocate buffer to receive with samples
    std::vector<std::complex<float> > buff(stream_cmd.num_samps);

    const size_t num_rx_samps = rx_stream->recv(&buff.front(), buff.size(), md);
    if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE){
        throw std::runtime_error(str(boost::format(
            "Unexpected error code 0x%x"
        ) % md.error_code));
    }
    std::cout << boost::format(
        "Received packet: %u samples, %u full secs, %f frac secs"
    ) % num_rx_samps % md.time_spec.get_full_secs() % md.time_spec.get_frac_secs() << std::endl;
    std::cout << boost::format(
        "Stream time was: %u full secs, %f frac secs"
    ) % stream_time.get_full_secs() % stream_time.get_frac_secs() << std::endl;
    std::cout << boost::format(
        "Difference between stream time and first packet: %f us"
    ) % ((md.time_spec-stream_time).get_real_secs()/100*1e6) << std::endl;

    //finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return 0;
}

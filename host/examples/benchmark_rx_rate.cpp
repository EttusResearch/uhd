//
// Copyright 2010-2011 Ettus Research LLC
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
#include <boost/math/special_functions/round.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <complex>

namespace po = boost::program_options;

static inline void test_device(
    uhd::usrp::multi_usrp::sptr usrp,
    double rx_rate_sps,
    double duration_secs
){
    const size_t max_samps_per_packet = usrp->get_device()->get_max_recv_samps_per_packet();
    std::cout << boost::format("Testing receive rate %f Msps (%f second run)") % (rx_rate_sps/1e6) % duration_secs << std::endl;

    //allocate recv buffer and metatdata
    uhd::rx_metadata_t md;
    std::vector<std::complex<float> > buff(max_samps_per_packet);

    //flush the buffers in the recv path
    while(usrp->get_device()->recv(
        &buff.front(), buff.size(), md,
        uhd::io_type_t::COMPLEX_FLOAT32,
        uhd::device::RECV_MODE_ONE_PACKET
    )){
        /* NOP */
    };

    //declare status variables
    bool got_first_packet = false;
    size_t total_recv_packets = 0;
    size_t total_lost_samples = 0;
    size_t total_recv_samples = 0;
    uhd::time_spec_t initial_time_spec;
    uhd::time_spec_t next_expected_time_spec;

    usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    do {
        size_t num_rx_samps = usrp->get_device()->recv(
            &buff.front(), buff.size(), md,
            uhd::io_type_t::COMPLEX_FLOAT32,
            uhd::device::RECV_MODE_ONE_PACKET
        );

        //handle the error codes
        switch(md.error_code){
        case uhd::rx_metadata_t::ERROR_CODE_NONE:
        case uhd::rx_metadata_t::ERROR_CODE_OVERFLOW:
            break;

        default:
            std::cerr << "Error code: " << md.error_code << std::endl;
            std::cerr << "Unexpected error on recv, exit test..." << std::endl;
            return;
        }

        if (not md.has_time_spec){
            std::cerr << "Metadata missing time spec, exit test..." << std::endl;
            return;
        }

        total_recv_samples += num_rx_samps;
        total_recv_packets++;

        if (not got_first_packet){
            initial_time_spec = md.time_spec;
            next_expected_time_spec = initial_time_spec;
            got_first_packet = true;
        }

        double approx_lost_samps = rx_rate_sps*(md.time_spec - next_expected_time_spec).get_real_secs();
        total_lost_samples += std::max(0, boost::math::iround(approx_lost_samps));
        next_expected_time_spec = md.time_spec + uhd::time_spec_t(0, num_rx_samps, rx_rate_sps);

    } while((next_expected_time_spec - initial_time_spec) < uhd::time_spec_t(duration_secs));
    usrp->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);

    //print a summary
    std::cout << std::endl; //go to newline, recv may spew SXSYSZ...
    std::cout << boost::format("    Received packets: %d") % total_recv_packets << std::endl;
    std::cout << boost::format("    Received samples: %d") % total_recv_samples << std::endl;
    std::cout << boost::format("    Lost samples: %d") % total_lost_samples << std::endl;
    size_t packets_lost = boost::math::iround(double(total_lost_samples)/max_samps_per_packet);
    std::cout << boost::format("    Lost packets: %d (approximate)") % packets_lost << std::endl;
    double actual_rx_rate_sps = (total_recv_samples*rx_rate_sps)/(total_recv_samples+total_lost_samples);
    std::cout << boost::format("    Sustained receive rate: %f Msps") % (actual_rx_rate_sps/1e6) << std::endl;
    std::cout << std::endl << std::endl;
}

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args;
    double duration;
    double only_rate;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args")
        ("duration", po::value<double>(&duration)->default_value(10.0), "duration for each test in seconds")
        ("rate", po::value<double>(&only_rate), "specify to perform a single test as this rate (sps)")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("UHD Benchmark RX Rate %s") % desc << std::endl;
        return ~0;
    }

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    if (not vm.count("rate")){
        usrp->set_rx_rate(500e3); //initial rate
        while(true){
            double rate = usrp->get_rx_rate();
            test_device(usrp, rate, duration);
            usrp->set_rx_rate(rate*2); //double the rate
            if (usrp->get_rx_rate() == rate) break;
        }
    }
    else{
        usrp->set_rx_rate(only_rate);
        double rate = usrp->get_rx_rate();
        test_device(usrp, rate, duration);
    }

    //finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return 0;
}

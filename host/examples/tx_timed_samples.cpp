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
#include <uhd/usrp/single_usrp.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <complex>

namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args;
    time_t seconds_in_future;
    size_t total_num_samps;
    size_t samps_per_packet;
    double rate, freq;
    float ampl;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args")
        ("secs", po::value<time_t>(&seconds_in_future)->default_value(3), "number of seconds in the future to transmit")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(1000), "total number of samples to transmit")
        ("spp", po::value<size_t>(&samps_per_packet)->default_value(1000), "number of samples per packet")
        ("rate", po::value<double>(&rate)->default_value(100e6/16), "rate of outgoing samples")
        ("freq", po::value<double>(&freq)->default_value(0), "rf center frequency in Hz")
        ("ampl", po::value<float>(&ampl)->default_value(float(0.3)), "amplitude of each sample")
        ("dilv", "specify to disable inner-loop verbose")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("UHD TX Timed Samples %s") % desc << std::endl;
        return ~0;
    }

    bool verbose = vm.count("dilv") == 0;

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::single_usrp::sptr sdev = uhd::usrp::single_usrp::make(args);
    uhd::device::sptr dev = sdev->get_device();
    std::cout << boost::format("Using Device: %s") % sdev->get_pp_string() << std::endl;

    //set the tx sample rate
    std::cout << boost::format("Setting TX Rate: %f Msps...") % (rate/1e6) << std::endl;
    sdev->set_tx_rate(rate);
    std::cout << boost::format("Actual TX Rate: %f Msps...") % (sdev->get_tx_rate()/1e6) << std::endl << std::endl;

    //set the tx center frequency
    std::cout << boost::format("Setting TX Freq: %f Mhz...") % (freq/1e6) << std::endl;
    sdev->set_tx_freq(freq);
    std::cout << boost::format("Actual TX Freq: %f Mhz...") % (sdev->get_tx_freq()/1e6) << std::endl << std::endl;

    std::cout << boost::format("Setting device timestamp to 0...") << std::endl;
    sdev->set_time_now(uhd::time_spec_t(0.0));

    //allocate data to send
    std::vector<std::complex<float> > buff(samps_per_packet, std::complex<float>(ampl, ampl));

    //send the data in multiple packets
    size_t num_packets = (total_num_samps+samps_per_packet-1)/samps_per_packet;
    for (size_t i = 0; i < num_packets; i++){
        //setup the metadata flags and time spec
        uhd::tx_metadata_t md;
        md.start_of_burst = (i == 0);              //only first packet has SOB
        md.end_of_burst   = (i == num_packets-1);  //only last packet has EOB
        md.has_time_spec  = (i == 0);              //only first packet has time
        md.time_spec = uhd::time_spec_t(seconds_in_future);

        size_t samps_to_send = std::min(total_num_samps - samps_per_packet*i, samps_per_packet);

        //send the entire packet (driver fragments internally)
        size_t num_tx_samps = dev->send(
            &buff.front(), samps_to_send, md,
            uhd::io_type_t::COMPLEX_FLOAT32,
            uhd::device::SEND_MODE_FULL_BUFF,
            //send will backup into the host this many seconds before sending:
            seconds_in_future + 0.1 //timeout (delay before transmit + padding)
        );
        if (num_tx_samps < samps_to_send) std::cout << "Send timeout..." << std::endl;
        if(verbose) std::cout << std::endl << boost::format("Sent %d samples") % num_tx_samps << std::endl;
    }

    //finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return 0;
}

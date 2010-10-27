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
#include <fstream>
#include <complex>

#include "sys/types.h"
#include "sys/socket.h"
#include "arpa/inet.h"

namespace po = boost::program_options;


int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args;
    time_t seconds_in_future;
    size_t total_num_samps;
    double rx_rate, freq;
    short srv_port;
    std::string srv_ip;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args")
        ("secs", po::value<time_t>(&seconds_in_future)->default_value(3), "number of seconds in the future to receive")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(1000), "total number of samples to receive")
        ("rxrate", po::value<double>(&rx_rate)->default_value(100e6/16), "rate of incoming samples")
        ("freq", po::value<double>(&freq)->default_value(0), "rf center frequency in Hz")
        ("port", po::value<short>(&srv_port)->default_value(7124), "server port")
        ("ip", po::value<std::string>(&srv_ip)->default_value("192.168.1.10"), "server ip")
        ("dilv", "specify to disable inner-loop verbose")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("UHD RX to UDP %s") % desc << std::endl;
        return ~0;
    }

    bool verbose = vm.count("dilv") == 0;

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::single_usrp::sptr sdev = uhd::usrp::single_usrp::make(args);
    uhd::device::sptr dev = sdev->get_device();
    std::cout << boost::format("Using Device: %s") % sdev->get_pp_string() << std::endl;

    //set properties on the device
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rx_rate/1e6) << std::endl;
    sdev->set_rx_rate(rx_rate);
    std::cout << boost::format("Actual RX Rate: %f Msps...") % (sdev->get_rx_rate()/1e6) << std::endl;
    std::cout << boost::format("Setting device timestamp to 0...") << std::endl;
    sdev->set_rx_freq(freq);
    sdev->set_time_now(uhd::time_spec_t(0.0));

    uhd::gain_range_t rx_gain = sdev->get_rx_gain_range();
    std::cout << "Setting RX Gain to: " << rx_gain.max << std::endl;
    sdev->set_rx_gain(rx_gain.max);

    sleep(1);
    std::cout << "LO Locked = " << sdev->get_rx_lo_locked() << std::endl;

    //setup streaming
    std::cout << std::endl;
    std::cout << boost::format(
        "Begin streaming %u samples, %d seconds in the future..."
    ) % total_num_samps % seconds_in_future << std::endl;
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps = total_num_samps;
    stream_cmd.stream_now = false;
    stream_cmd.time_spec = uhd::time_spec_t(seconds_in_future);
    sdev->issue_stream_cmd(stream_cmd);

    //loop until total number of samples reached
    size_t num_acc_samps = 0; //number of accumulated samples
    uhd::rx_metadata_t md;
    std::vector<std::complex<float> > buff(368);
    std::ofstream outfile("out.dat", std::ofstream::binary);

    int s;
    struct sockaddr_in si_other;
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
	std::cout << "Failid to open socket." << std::endl;
    	return -1;
    }	

    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(srv_port);
    if (inet_aton(srv_ip.c_str(), &si_other.sin_addr) == 0) {
	    std::cout << "Bad inet addr: " << srv_ip << std::endl;
	    return -1;
    }

    while(num_acc_samps < total_num_samps){
        size_t num_rx_samps = dev->recv(
            &buff.front(), buff.size(), md,
            uhd::io_type_t::COMPLEX_FLOAT32,
            uhd::device::RECV_MODE_ONE_PACKET
        );

        //handle the error codes
        switch(md.error_code){
        case uhd::rx_metadata_t::ERROR_CODE_NONE:
            break;

        case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
            if (num_acc_samps == 0) continue;
            std::cout << boost::format(
                "Got timeout before all samples received, possible packet loss, exiting loop..."
            ) << std::endl;
            goto done_loop;

        default:
            std::cout << boost::format(
                "Got error code 0x%x, exiting loop..."
            ) % md.error_code << std::endl;
            goto done_loop;
        }

//	outfile.write((const char*)&buff[0], num_rx_samps * sizeof(std::complex<short>));
	if (sendto(s, (const char *)&buff[0], num_rx_samps * sizeof(std::complex<float>), 0 , (const sockaddr *)&si_other, sizeof(si_other)) == -1)
		std::cout << "Sendto failed." << std::endl;

        if(verbose) std::cout << boost::format(
            "Got packet: %u samples, %u full secs, %f frac secs"
        ) % num_rx_samps % md.time_spec.get_full_secs() % md.time_spec.get_frac_secs() << std::endl;

        num_acc_samps += num_rx_samps;
    } done_loop:

    outfile.close();

    //finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return 0;
}

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

#include <uhd/simple_device.hpp>
#include <uhd/props.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <complex>

namespace po = boost::program_options;

int main(int argc, char *argv[]){
    //variables to be set by po
    std::string transport_args;
    int seconds_in_future;
    size_t total_num_samps;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&transport_args)->default_value(""), "simple uhd transport args")
        ("secs", po::value<int>(&seconds_in_future)->default_value(3), "number of seconds in the future to receive")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(1000), "total number of samples to receive")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm); 

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("UHD RX Timed Samples %s") % desc << std::endl;
        return ~0;
    }

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...")
        % transport_args << std::endl;
    uhd::simple_device::sptr sdev = uhd::simple_device::make(transport_args);
    uhd::device::sptr dev = sdev->get_device();
    std::cout << boost::format("Using Device: %s") % sdev->get_name() << std::endl;

    //set properties on the device
    double rx_rate = sdev->get_rx_rates()[4]; //pick some rate
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rx_rate/1e6) << std::endl;
    sdev->set_rx_rate(rx_rate);
    std::cout << boost::format("Setting device timestamp to 0...") << std::endl;
    sdev->set_time_now(uhd::time_spec_t(0));

    //setup streaming
    std::cout << std::endl;
    std::cout << boost::format("Begin streaming %u seconds in the future...")
        % seconds_in_future << std::endl;
    sdev->set_streaming_at(uhd::time_spec_t(seconds_in_future));

    //loop until total number of samples reached
    size_t num_acc_samps = 0; //number of accumulated samples
    while(num_acc_samps < total_num_samps){
        uhd::rx_metadata_t md;
        std::complex<float> buff[1000];
        size_t num_rx_samps = dev->recv(boost::asio::buffer(buff, sizeof(buff)), md, "32fc");
        if (num_rx_samps == 0) continue; //wait for packets with contents

        std::cout << boost::format("Got packet: %u samples, %u secs, %u ticks")
            % num_rx_samps % md.time_spec.secs % md.time_spec.ticks << std::endl;

        num_acc_samps += num_rx_samps;
    }

    //finished, stop streaming
    sdev->set_streaming(false);
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return 0;
}

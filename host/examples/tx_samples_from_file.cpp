//
// Copyright 2011 Ettus Research LLC
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
#include <boost/thread.hpp>
#include <iostream>
#include <fstream>
#include <complex>

namespace po = boost::program_options;

template<typename samp_type> void send_from_file(
    uhd::usrp::multi_usrp::sptr usrp,
    const uhd::io_type_t &io_type,
    const std::string &file,
    size_t samps_per_buff
){
    uhd::tx_metadata_t md;
    md.start_of_burst = false;
    md.end_of_burst = false;
    std::vector<samp_type> buff(samps_per_buff);
    std::ifstream infile(file.c_str(), std::ifstream::binary);

    //loop until the entire file has been read
    while(not md.end_of_burst){

        infile.read((char*)&buff.front(), buff.size()*sizeof(samp_type));
        size_t num_tx_samps = infile.gcount()/sizeof(samp_type);

        md.end_of_burst = infile.eof();

        usrp->get_device()->send(
            &buff.front(), num_tx_samps, md, io_type,
            uhd::device::SEND_MODE_FULL_BUFF
        );
    }

    infile.close();
}

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args, file, type;
    size_t spb;
    double rate, freq, gain;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "multi uhd device address args")
        ("file", po::value<std::string>(&file)->default_value("usrp_samples.dat"), "name of the file to read binary samples from")
        ("type", po::value<std::string>(&type)->default_value("float"), "choose the sample type: double, float, or short")
        ("spb", po::value<size_t>(&spb)->default_value(10000), "samples per buffer")
        ("rate", po::value<double>(&rate)->default_value(100e6/16), "rate of outgoing samples")
        ("freq", po::value<double>(&freq)->default_value(0), "rf center frequency in Hz")
        ("gain", po::value<double>(&gain)->default_value(0), "gain for the RF chain")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("UHD TX samples from file %s") % desc << std::endl;
        return ~0;
    }

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    //set the rx sample rate
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rate/1e6) << std::endl;
    usrp->set_tx_rate(rate);
    std::cout << boost::format("Actual RX Rate: %f Msps...") % (usrp->get_tx_rate()/1e6) << std::endl << std::endl;

    //set the rx center frequency
    std::cout << boost::format("Setting RX Freq: %f Mhz...") % (freq/1e6) << std::endl;
    usrp->set_tx_freq(freq);
    std::cout << boost::format("Actual RX Freq: %f Mhz...") % (usrp->get_tx_freq()/1e6) << std::endl << std::endl;

    //set the rx rf gain
    std::cout << boost::format("Setting RX Gain: %f dB...") % gain << std::endl;
    usrp->set_tx_gain(gain);
    std::cout << boost::format("Actual RX Gain: %f dB...") % usrp->get_tx_gain() << std::endl << std::endl;

    boost::this_thread::sleep(boost::posix_time::seconds(1)); //allow for some setup time

    //send from file
    if (type == "double") send_from_file<std::complex<double> >(usrp, uhd::io_type_t::COMPLEX_FLOAT64, file, spb);
    else if (type == "float") send_from_file<std::complex<float> >(usrp, uhd::io_type_t::COMPLEX_FLOAT32, file, spb);
    else if (type == "short") send_from_file<std::complex<short> >(usrp, uhd::io_type_t::COMPLEX_INT16, file, spb);
    else throw std::runtime_error("Unknown type " + type);

    //finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return 0;
}

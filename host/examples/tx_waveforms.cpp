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
#include <uhd/utils/static.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <boost/program_options.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/function.hpp>
#include <iostream>
#include <complex>
#include <csignal>
#include <cmath>

namespace po = boost::program_options;

static bool stop_signal_called = false;
void sig_int_handler(int){stop_signal_called = true;}

/***********************************************************************
 * Waveform generators
 **********************************************************************/
float gen_const(float){
    return 1;
}

float gen_square(float x){
    return float((std::fmod(x, 1) < float(0.5))? 0 : 1);
}

float gen_ramp(float x){
    return std::fmod(x, 1)*2 - 1;
}

#define sine_table_len 2048
static float sine_table[sine_table_len];
UHD_STATIC_BLOCK(gen_sine_table){
    static const double tau = 2*std::acos(-1.0);
    for (size_t i = 0; i < sine_table_len; i++)
        sine_table[i] = float(std::sin((tau*i)/sine_table_len));
}

float gen_sine(float x){
    return sine_table[size_t(x*sine_table_len)%sine_table_len];
}

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args, wave_type, ant, subdev;
    size_t spb;
    double rate, freq, gain, wave_freq, bw;
    float ampl;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args")
        ("spb", po::value<size_t>(&spb)->default_value(10000), "samples per buffer")
        ("rate", po::value<double>(&rate), "rate of outgoing samples")
        ("freq", po::value<double>(&freq), "RF center frequency in Hz")
        ("ampl", po::value<float>(&ampl)->default_value(float(0.3)), "amplitude of the waveform")
        ("gain", po::value<double>(&gain), "gain for the RF chain")
        ("ant", po::value<std::string>(&ant), "daughterboard antenna selection")
        ("subdev", po::value<std::string>(&subdev), "daughterboard subdevice specification")
        ("bw", po::value<double>(&bw), "daughterboard IF filter bandwidth in Hz")
        ("wave-type", po::value<std::string>(&wave_type)->default_value("CONST"), "waveform type (CONST, SQUARE, RAMP, SINE)")
        ("wave-freq", po::value<double>(&wave_freq)->default_value(0), "waveform frequency in Hz")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("UHD TX Waveforms %s") % desc << std::endl;
        return ~0;
    }

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

    //always select the subdevice first, the channel mapping affects the other settings
    if (vm.count("subdev")) usrp->set_tx_subdev_spec(subdev);

    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    //set the sample rate
    if (not vm.count("rate")){
        std::cerr << "Please specify the sample rate with --rate" << std::endl;
        return ~0;
    }
    std::cout << boost::format("Setting TX Rate: %f Msps...") % (rate/1e6) << std::endl;
    usrp->set_tx_rate(rate);
    std::cout << boost::format("Actual TX Rate: %f Msps...") % (usrp->get_tx_rate()/1e6) << std::endl << std::endl;

    //set the center frequency
    if (not vm.count("freq")){
        std::cerr << "Please specify the center frequency with --freq" << std::endl;
        return ~0;
    }

    for(size_t chan = 0; chan < usrp->get_tx_num_channels(); chan++) {
        std::cout << boost::format("Setting TX Freq: %f MHz...") % (freq/1e6) << std::endl;
        usrp->set_tx_freq(freq, chan);
        std::cout << boost::format("Actual TX Freq: %f MHz...") % (usrp->get_tx_freq(chan)/1e6) << std::endl << std::endl;

        //set the rf gain
        if (vm.count("gain")){
            std::cout << boost::format("Setting TX Gain: %f dB...") % gain << std::endl;
            usrp->set_tx_gain(gain, chan);
            std::cout << boost::format("Actual TX Gain: %f dB...") % usrp->get_tx_gain(chan) << std::endl << std::endl;
        }

        //set the IF filter bandwidth
        if (vm.count("bw")){
            std::cout << boost::format("Setting TX Bandwidth: %f MHz...") % bw << std::endl;
            usrp->set_tx_bandwidth(bw, chan);
            std::cout << boost::format("Actual TX Bandwidth: %f MHz...") % usrp->get_tx_bandwidth(chan) << std::endl << std::endl;
        }

        //set the antenna
        if (vm.count("ant")) usrp->set_tx_antenna(ant, chan);
    }

    std::cout << boost::format("Setting device timestamp to 0...") << std::endl;
    usrp->set_time_now(uhd::time_spec_t(0.0));

    //for the const wave, set the wave freq for small samples per period
    if (wave_freq == 0 and wave_type == "CONST"){
        wave_freq = usrp->get_tx_rate()/2;
    }

    //error when the waveform is not possible to generate
    if (std::abs(wave_freq) > usrp->get_tx_rate()/2){
        throw std::runtime_error("wave freq out of Nyquist zone");
    }
    if (usrp->get_tx_rate()/std::abs(wave_freq) > sine_table_len/2 and wave_type == "SINE"){
        throw std::runtime_error("sine freq too small for table");
    }

    //store the generator function for the selected waveform
    boost::function<float(float)> wave_gen;
    if      (wave_type == "CONST")  wave_gen = &gen_const;
    else if (wave_type == "SQUARE") wave_gen = &gen_square;
    else if (wave_type == "RAMP")   wave_gen = &gen_ramp;
    else if (wave_type == "SINE")   wave_gen = &gen_sine;
    else throw std::runtime_error("unknown waveform type: " + wave_type);

    //allocate the buffer and precalculate values
    const float cps = float(wave_freq/usrp->get_tx_rate());
    const float i_off = (wave_freq > 0)? float(0.25) : 0;
    const float q_off = (wave_freq < 0)? float(0.25) : 0;
    float theta = 0;

    std::vector<std::complex<float> *> buffs;
    for(size_t i=0; i < usrp->get_num_mboards(); i++)
        buffs.push_back(new std::complex<float>[spb]);

    //setup the metadata flags
    uhd::tx_metadata_t md;
    md.start_of_burst = false; //no for continuous streaming
    md.end_of_burst   = false;
    md.has_time_spec = true;
    md.time_spec = uhd::time_spec_t(0.1);

    std::signal(SIGINT, &sig_int_handler);
    std::cout << "Press Ctrl + C to stop streaming..." << std::endl;

    //send data until the signal handler gets called
    while(not stop_signal_called){
        //fill the buffer with the waveform
        for (size_t n = 0; n < spb; n++){
            BOOST_FOREACH(std::complex<float> *buff, buffs) {
                buff[n] = std::complex<float>(
                    ampl*wave_gen(i_off + theta),
                    ampl*wave_gen(q_off + theta)
                );
            }
            theta += cps;
        }

        //bring the theta back into range [0, 1)
        theta = std::fmod(theta, 1);

        //send the entire contents of the buffer
        usrp->get_device()->send(
            buffs, spb, md,
            uhd::io_type_t::COMPLEX_FLOAT32,
            uhd::device::SEND_MODE_FULL_BUFF
        );

        md.has_time_spec = false;
    }

    //send a mini EOB packet
    md.end_of_burst   = true;
    usrp->get_device()->send("", 0, md,
        uhd::io_type_t::COMPLEX_FLOAT32,
        uhd::device::SEND_MODE_FULL_BUFF
    );

    //finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    BOOST_FOREACH(std::complex<float> *buff, buffs){
        delete buff;
    }
    
    return 0;
}

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
#include <uhd/usrp/simple_usrp.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread_time.hpp> //system time
#include <boost/format.hpp>
#include <iostream>
#include <complex>
#include <cmath>

namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args, wave_type;
    size_t total_duration, amspb;
    double rate, freq, wave_freq;
    float ampl, gain;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "simple uhd device address args")
        ("duration", po::value<size_t>(&total_duration)->default_value(3), "number of seconds to transmit")
        ("amspb", po::value<size_t>(&amspb)->default_value(10000), "approximate mimimum samples per buffer")
        ("rate", po::value<double>(&rate)->default_value(100e6/16), "rate of outgoing samples")
        ("freq", po::value<double>(&freq)->default_value(0), "rf center frequency in Hz")
        ("ampl", po::value<float>(&ampl)->default_value(float(0.3)), "amplitude of the waveform")
        ("gain", po::value<float>(&gain)->default_value(float(0)), "gain for the RF chain")
        ("wave-type", po::value<std::string>(&wave_type)->default_value("SINE"), "waveform type (CONST, SQUARE, RAMP, SINE)")
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
    uhd::usrp::simple_usrp::sptr sdev = uhd::usrp::simple_usrp::make(args);
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

    //set the tx rf gain
    std::cout << boost::format("Setting TX Gain: %f dB...") % gain << std::endl;
    sdev->set_tx_gain(gain);
    std::cout << boost::format("Actual TX Gain: %f dB...") % sdev->get_tx_gain() << std::endl << std::endl;

    //for the const wave, set the wave freq for small samples per period
    if (wave_freq == 0 and wave_type == "CONST"){
        wave_freq = sdev->get_tx_rate()/2;
    }

    //error when the waveform is not possible to generate
    if (std::abs(wave_freq)/sdev->get_tx_rate() < 0.5/amspb){
        throw std::runtime_error("wave freq/tx rate too small");
    }
    if (std::abs(wave_freq) > sdev->get_tx_rate()/2){
        throw std::runtime_error("wave freq out of Nyquist zone");
    }

    //fill a buffer with one period worth of samples
    std::vector<float> period(size_t(sdev->get_tx_rate()/std::abs(wave_freq)));
    std::cout << boost::format("Samples per waveform period: %d") % period.size() << std::endl;
    for (size_t n = 0; n < period.size(); n++){
        if      (wave_type == "CONST")  period[n] = ampl;
        else if (wave_type == "SQUARE") period[n] = (n > period.size()/2)? ampl : 0;
        else if (wave_type == "RAMP")   period[n] = float((n/double(period.size()-1)) * 2*ampl - ampl);
        else if (wave_type == "SINE")   period[n] = ampl*float(std::sin(2*M_PI*n/double(period.size())));
        else throw std::runtime_error("unknown waveform type: " + wave_type);
    }

    //allocate data to send (fill with several periods worth of IQ samples)
    const size_t periods_per_buff = std::max<size_t>(1, amspb/period.size());
    std::vector<std::complex<float> > buff(period.size()*periods_per_buff);
    std::cout << boost::format("Samples per send buffer: %d") % buff.size() << std::endl;
    const size_t i_ahead = (wave_freq > 0)? period.size()/4 : 0;
    const size_t q_ahead = (wave_freq < 0)? period.size()/4 : 0;
    for (size_t n = 0; n < buff.size(); n++) buff[n] = std::complex<float>(
        period[(n+i_ahead)%period.size()], period[(n+q_ahead)%period.size()] //I,Q
    );

    //setup the metadata flags
    uhd::tx_metadata_t md;
    md.start_of_burst = true; //always SOB (good for continuous streaming)
    md.end_of_burst   = false;

    //send the data in multiple packets
    boost::system_time end_time(boost::get_system_time() + boost::posix_time::seconds(total_duration));
    while(end_time > boost::get_system_time()) dev->send(
        &buff.front(), buff.size(), md,
        uhd::io_type_t::COMPLEX_FLOAT32,
        uhd::device::SEND_MODE_FULL_BUFF
    );

    //send a mini EOB packet
    md.start_of_burst = false;
    md.end_of_burst   = true;
    dev->send(NULL, 0, md,
        uhd::io_type_t::COMPLEX_FLOAT32,
        uhd::device::SEND_MODE_FULL_BUFF
    );

    //finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return 0;
}

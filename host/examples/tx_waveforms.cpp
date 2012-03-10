//
// Copyright 2010-2012 Ettus Research LLC
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
#include <uhd/exception.hpp>
#include <boost/program_options.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <complex>
#include <csignal>
#include <cmath>

namespace po = boost::program_options;

/***********************************************************************
 * Signal handlers
 **********************************************************************/
static bool stop_signal_called = false;
void sig_int_handler(int){stop_signal_called = true;}

/***********************************************************************
 * Waveform generators
 **********************************************************************/
static const size_t wave_table_len = 8192;

class wave_table_class{
public:
    wave_table_class(const std::string &wave_type, const float ampl):
        _wave_table(wave_table_len)
    {
        //compute real wave table with 1.0 amplitude
        std::vector<double> real_wave_table(wave_table_len);
        if (wave_type == "CONST"){
            for (size_t i = 0; i < wave_table_len; i++)
                real_wave_table[i] = 1.0;
        }
        else if (wave_type == "SQUARE"){
            for (size_t i = 0; i < wave_table_len; i++)
                real_wave_table[i] = (i < wave_table_len/2)? 0.0 : 1.0;
        }
        else if (wave_type == "RAMP"){
            for (size_t i = 0; i < wave_table_len; i++)
                real_wave_table[i] = 2.0*i/(wave_table_len-1) - 1.0;
        }
        else if (wave_type == "SINE"){
            static const double tau = 2*std::acos(-1.0);
            for (size_t i = 0; i < wave_table_len; i++)
                real_wave_table[i] = std::sin((tau*i)/wave_table_len);
        }
        else throw std::runtime_error("unknown waveform type: " + wave_type);

        //compute i and q pairs with 90% offset and scale to amplitude
        for (size_t i = 0; i < wave_table_len; i++){
            const size_t q = (i+(3*wave_table_len)/4)%wave_table_len;
            _wave_table[i] = std::complex<float>(ampl*real_wave_table[i], ampl*real_wave_table[q]);
        }
    }

    inline std::complex<float> operator()(const size_t index) const{
        return _wave_table[index % wave_table_len];
    }

private:
    std::vector<std::complex<float> > _wave_table;
};

/***********************************************************************
 * Main function
 **********************************************************************/
int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args, wave_type, ant, subdev, ref, otw;
    size_t spb;
    double rate, freq, gain, wave_freq, bw;
    float ampl;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args")
        ("spb", po::value<size_t>(&spb)->default_value(0), "samples per buffer, 0 for default")
        ("rate", po::value<double>(&rate), "rate of outgoing samples")
        ("freq", po::value<double>(&freq), "RF center frequency in Hz")
        ("ampl", po::value<float>(&ampl)->default_value(float(0.3)), "amplitude of the waveform [0 to 0.7]")
        ("gain", po::value<double>(&gain), "gain for the RF chain")
        ("ant", po::value<std::string>(&ant), "daughterboard antenna selection")
        ("subdev", po::value<std::string>(&subdev), "daughterboard subdevice specification")
        ("bw", po::value<double>(&bw), "daughterboard IF filter bandwidth in Hz")
        ("wave-type", po::value<std::string>(&wave_type)->default_value("CONST"), "waveform type (CONST, SQUARE, RAMP, SINE)")
        ("wave-freq", po::value<double>(&wave_freq)->default_value(0), "waveform frequency in Hz")
        ("ref", po::value<std::string>(&ref)->default_value("internal"), "clock reference (internal, external, mimo)")
        ("otw", po::value<std::string>(&otw)->default_value("sc16"), "specify the over-the-wire sample mode")
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

    //Lock mboard clocks
    usrp->set_clock_source(ref);

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

    boost::this_thread::sleep(boost::posix_time::seconds(1)); //allow for some setup time

    //for the const wave, set the wave freq for small samples per period
    if (wave_freq == 0 and wave_type == "CONST"){
        wave_freq = usrp->get_tx_rate()/2;
    }

    //error when the waveform is not possible to generate
    if (std::abs(wave_freq) > usrp->get_tx_rate()/2){
        throw std::runtime_error("wave freq out of Nyquist zone");
    }
    if (usrp->get_tx_rate()/std::abs(wave_freq) > wave_table_len/2){
        throw std::runtime_error("wave freq too small for table");
    }

    //pre-compute the waveform values
    const wave_table_class wave_table(wave_type, ampl);
    const size_t step = boost::math::iround(wave_freq/usrp->get_tx_rate() * wave_table_len);
    size_t index = 0;

    //create a transmit streamer
    //linearly map channels (index0 = channel0, index1 = channel1, ...)
    uhd::stream_args_t stream_args("fc32", otw);
    for (size_t chan = 0; chan < usrp->get_tx_num_channels(); chan++)
        stream_args.channels.push_back(chan); //linear mapping
    uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);

    //allocate a buffer which we re-use for each channel
    if (spb == 0) spb = tx_stream->get_max_num_samps()*10;
    std::vector<std::complex<float> > buff(spb);
    std::vector<std::complex<float> *> buffs(usrp->get_tx_num_channels(), &buff.front());

    //setup the metadata flags
    uhd::tx_metadata_t md;
    md.start_of_burst = true;
    md.end_of_burst   = false;
    md.has_time_spec  = true;
    md.time_spec = uhd::time_spec_t(0.1);

    std::cout << boost::format("Setting device timestamp to 0...") << std::endl;
    usrp->set_time_now(uhd::time_spec_t(0.0));

    //Check Ref and LO Lock detect
    std::vector<std::string> sensor_names;
    sensor_names = usrp->get_tx_sensor_names(0);
    if (std::find(sensor_names.begin(), sensor_names.end(), "lo_locked") != sensor_names.end()) {
        uhd::sensor_value_t lo_locked = usrp->get_tx_sensor("lo_locked",0);
        std::cout << boost::format("Checking TX: %s ...") % lo_locked.to_pp_string() << std::endl;
        UHD_ASSERT_THROW(lo_locked.to_bool());
    }
    sensor_names = usrp->get_mboard_sensor_names(0);
    if ((ref == "mimo") and (std::find(sensor_names.begin(), sensor_names.end(), "mimo_locked") != sensor_names.end())) {
        uhd::sensor_value_t mimo_locked = usrp->get_mboard_sensor("mimo_locked",0);
        std::cout << boost::format("Checking TX: %s ...") % mimo_locked.to_pp_string() << std::endl;
        UHD_ASSERT_THROW(mimo_locked.to_bool());
    }
    if ((ref == "external") and (std::find(sensor_names.begin(), sensor_names.end(), "ref_locked") != sensor_names.end())) {
        uhd::sensor_value_t ref_locked = usrp->get_mboard_sensor("ref_locked",0);
        std::cout << boost::format("Checking TX: %s ...") % ref_locked.to_pp_string() << std::endl;
        UHD_ASSERT_THROW(ref_locked.to_bool());
    }

    std::signal(SIGINT, &sig_int_handler);
    std::cout << "Press Ctrl + C to stop streaming..." << std::endl;

    //send data until the signal handler gets called
    while(not stop_signal_called){
        //fill the buffer with the waveform
        for (size_t n = 0; n < buff.size(); n++){
            buff[n] = wave_table(index += step);
        }

        //send the entire contents of the buffer
        tx_stream->send(buffs, buff.size(), md);

        md.start_of_burst = false;
        md.has_time_spec = false;
    }

    //send a mini EOB packet
    md.end_of_burst = true;
    tx_stream->send("", 0, md);

    //finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;
    return 0;
}

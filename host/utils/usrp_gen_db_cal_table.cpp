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
#include <uhd/usrp/multi_usrp.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread/thread.hpp>
#include <boost/math/special_functions/round.hpp>
#include <iostream>
#include <fstream>
#include <complex>
#include <cmath>

namespace po = boost::program_options;

/***********************************************************************
 * Constants
 **********************************************************************/
static const double e = 2.71828183;
static const double tau = 6.28318531;
static const double alpha = 0.0001; //very tight iir filter
static const size_t wave_table_len = 8192;
static const float ampl = 0.7; //transmitted wave amplitude
static const double tx_wave_freq = 507.123e3; //freq of tx sine wave in Hz

/***********************************************************************
 * Sinusoid wave table
 **********************************************************************/
static std::vector<std::complex<float> > gen_table(void){
    std::vector<std::complex<float> > wave_table(wave_table_len);
    std::vector<double> real_wave_table(wave_table_len);
    for (size_t i = 0; i < wave_table_len; i++)
        real_wave_table[i] = std::sin((tau*i)/wave_table_len);

    //compute i and q pairs with 90% offset and scale to amplitude
    for (size_t i = 0; i < wave_table_len; i++){
        const size_t q = (i+(3*wave_table_len)/4)%wave_table_len;
        wave_table[i] = std::complex<float>(ampl*real_wave_table[i], ampl*real_wave_table[q]);
    }

    return wave_table;
}

static std::complex<float> wave_table_lookup(size_t &index){
    static const std::vector<std::complex<float> > wave_table = gen_table();
    index %= wave_table_len;
    return wave_table[index];
}

/***********************************************************************
 * Compute power of a tone
 **********************************************************************/
static double compute_tone_dbrms(
    const std::vector<std::complex<float> > &samples,
    const double freq //freq is fractional
){
    //shift the samples so the tone at freq is down at DC
    std::vector<std::complex<double> > shifted(samples.size());
    for (size_t i = 0; i < shifted.size(); i++){
        shifted[i] = std::complex<double>(samples[i]) * std::pow(e, std::complex<double>(0, -freq*tau*i));
    }

    std::vector<std::complex<float> > buff(shifted.size());
    for (size_t i = 0; i < buff.size(); i++) buff[i] = shifted[i];
    std::ofstream outfile("/home/jblum/Desktop/gen.dat", std::ofstream::binary);
    outfile.write((const char*)&buff.front(), buff.size() * 8);

    //filter the samples with a narrow low pass
    std::complex<double> iir_output = 0, iir_last = 0;
    double output = 0;
    for (size_t i = 0; i < shifted.size(); i++){
        iir_output = alpha * shifted[i] + (1-alpha)*iir_last;
        iir_last = iir_output;
        output += std::abs(iir_output);
    }

    return 20*std::log10(output/shifted.size());
}

/***********************************************************************
 * Transmit thread
 **********************************************************************/
static void tx_thread(uhd::usrp::multi_usrp::sptr usrp){
    uhd::set_thread_priority_safe();

    //create a transmit streamer
    uhd::stream_args_t stream_args("fc32"); //complex floats
    uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);

    //setup variables and allocate buffer
    uhd::tx_metadata_t md;
    md.has_time_spec = false;
    std::vector<std::complex<float> > buff(tx_stream->get_max_num_samps()*10);

    //values for the wave table lookup
    size_t index = 0;
    const double tx_rate = usrp->get_tx_rate();
    const size_t step = boost::math::iround(wave_table_len * tx_wave_freq/tx_rate);

    //fill buff and send until interrupted
    while (not boost::this_thread::interruption_requested()){
        for (size_t i = 0; i < buff.size(); i++){
            buff[i] = wave_table_lookup(index);
            index += step;
        }
        tx_stream->send(&buff.front(), buff.size(), md);
    }

    //send a mini EOB packet
    md.end_of_burst = true;
    tx_stream->send("", 0, md);
}

/***********************************************************************
 * Main
 **********************************************************************/
int UHD_SAFE_MAIN(int argc, char *argv[]){
    std::string args;
    double rate;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "device address args [default = \"\"]")
        ("rate", po::value<double>(&rate)->default_value(12.5e6), "RX and TX sample rate in Hz")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("USRP Generate Daughterboard Calibration Table %s") % desc << std::endl;
        std::cout <<
            "This application measures leakage between RX and TX on an XCVR daughterboard to self-calibrate.\n"
            << std::endl;
        return ~0;
    }

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

    //set the sample rates
    usrp->set_rx_rate(rate);
    usrp->set_tx_rate(rate);

    //tune the transmitter with no cordic
    uhd::tune_request_t tx_tune_req(2.155e9);
    tx_tune_req.dsp_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
    tx_tune_req.dsp_freq = 0;
    usrp->set_tx_freq(tx_tune_req);
    {
        boost::system_time start = boost::get_system_time();
        while (not usrp->get_tx_sensor("lo_locked").to_bool()){
            if (boost::get_system_time() > start + boost::posix_time::milliseconds(100)){
                throw std::runtime_error("timed out waiting for TX LO to lock");
            }
        }
    }

    //tune the receiver
    usrp->set_rx_freq(2.155e9 - .9344e6);
    {
        boost::system_time start = boost::get_system_time();
        while (not usrp->get_rx_sensor("lo_locked").to_bool()){
            if (boost::get_system_time() > start + boost::posix_time::milliseconds(100)){
                throw std::runtime_error("timed out waiting for RX LO to lock");
            }
        }
    }

    //set max receiver gain
    usrp->set_rx_gain(usrp->get_rx_gain_range().stop());

    //create a receive streamer
    uhd::stream_args_t stream_args("fc32"); //complex floats
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    //create a transmitter thread
    boost::thread_group threads;
    threads.create_thread(boost::bind(&tx_thread, usrp));

    //receive some samples
    std::vector<std::complex<float> > buff(100000);
    usrp->set_time_now(uhd::time_spec_t(0.0));
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps = buff.size();
    stream_cmd.stream_now = false;
    stream_cmd.time_spec = uhd::time_spec_t(0.1);
    usrp->issue_stream_cmd(stream_cmd);
    uhd::rx_metadata_t md;
    const size_t num_rx_samps = rx_stream->recv(&buff.front(), buff.size(), md);

    //validate the received data
    if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE){
        throw std::runtime_error(str(boost::format(
            "Unexpected error code 0x%x"
        ) % md.error_code));
    }
    if (num_rx_samps != buff.size()){
        throw std::runtime_error("did not get all the samples requested");
    }

    //stop the transmitter
    threads.interrupt_all();
    threads.join_all();

    const double actual_rx_rate = usrp->get_rx_rate();
    const double actual_tx_freq = usrp->get_tx_freq();
    const double actual_rx_freq = usrp->get_rx_freq();
    const double bb_tone_freq = actual_tx_freq + tx_wave_freq - actual_rx_freq;
    const double bb_imag_freq = actual_tx_freq - tx_wave_freq - actual_rx_freq;

    const double tone_dbrms = compute_tone_dbrms(buff, bb_tone_freq/actual_rx_rate);
    const double imag_dbrms = compute_tone_dbrms(buff, bb_imag_freq/actual_rx_rate);

    std::cout << "bb_tone_freq " << bb_tone_freq << std::endl;
    std::cout << "bb_imag_freq " << bb_imag_freq << std::endl;
    std::cout << "tone_dbrms " << tone_dbrms << std::endl;
    std::cout << "imag_dbrms " << imag_dbrms << std::endl;
    std::cout << "supression " << (tone_dbrms - imag_dbrms) << std::endl;

    return 0;
}

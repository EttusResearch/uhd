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
#include <uhd/utils/paths.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/math/special_functions/round.hpp>
#include <iostream>
#include <fstream>
#include <complex>
#include <cmath>
#include <ctime>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

/***********************************************************************
 * Constants
 **********************************************************************/
static const double e = 2.71828183;
static const double tau = 6.28318531;
static const double alpha = 0.0001; //very tight iir filter
static const size_t wave_table_len = 8192;

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
        wave_table[i] = std::complex<float>(real_wave_table[i], real_wave_table[q]);
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
static void tx_thread(uhd::usrp::multi_usrp::sptr usrp, const double tx_wave_freq, const double tx_wave_ampl){
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
            buff[i] = float(tx_wave_ampl) * wave_table_lookup(index);
            index += step;
        }
        tx_stream->send(&buff.front(), buff.size(), md);
    }

    //send a mini EOB packet
    md.end_of_burst = true;
    tx_stream->send("", 0, md);
}

/***********************************************************************
 * Tune RX and TX routine
 **********************************************************************/
static void tune_rx_and_tx(uhd::usrp::multi_usrp::sptr usrp, const double tx_lo_freq, const double rx_offset){
    //tune the transmitter with no cordic
    uhd::tune_request_t tx_tune_req(tx_lo_freq);
    tx_tune_req.dsp_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
    tx_tune_req.dsp_freq = 0;
    usrp->set_tx_freq(tx_tune_req);

    //tune the receiver
    usrp->set_rx_freq(usrp->get_tx_freq() - rx_offset);

    //wait for the LOs to become locked
    boost::system_time start = boost::get_system_time();
    while (not usrp->get_tx_sensor("lo_locked").to_bool() or not usrp->get_rx_sensor("lo_locked").to_bool()){
        if (boost::get_system_time() > start + boost::posix_time::milliseconds(100)){
            throw std::runtime_error("timed out waiting for TX and/or RX LO to lock");
        }
    }
}

/***********************************************************************
 * Data capture routine
 **********************************************************************/
static void capture_samples(uhd::usrp::multi_usrp::sptr usrp, uhd::rx_streamer::sptr rx_stream, std::vector<std::complex<float> > &buff){
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps = buff.size();
    stream_cmd.stream_now = true;
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
}

/***********************************************************************
 * Store data to file
 **********************************************************************/
struct result_t{double freq, real_corr, imag_corr, sup;};
static void store_results(uhd::usrp::multi_usrp::sptr usrp, const std::vector<result_t> &results){
    //extract eeprom serial
    uhd::property_tree::sptr tree = usrp->get_device()->get_tree();
    const uhd::fs_path db_path = "/mboards/0/dboards/A/tx_eeprom";
    const uhd::usrp::dboard_eeprom_t db_eeprom = tree->access<uhd::usrp::dboard_eeprom_t>(db_path).get();
    if (db_eeprom.serial.empty()) throw std::runtime_error("TX dboard has empty serial!");

    //make the calibration file path
    fs::path cal_data_path = fs::path(uhd::get_app_path()) / ".uhd";
    fs::create_directory(cal_data_path);
    cal_data_path = cal_data_path / "cal";
    fs::create_directory(cal_data_path);
    cal_data_path = cal_data_path / ("tx_fe_cal_v0.1_" + db_eeprom.serial + ".csv");

    //fill the calibration file
    std::ofstream cal_data(cal_data_path.string().c_str());
    cal_data << boost::format("name, TX Frontend Calibration\n");
    cal_data << boost::format("serial, %s\n") % db_eeprom.serial;
    cal_data << boost::format("timestamp, %d\n") % time(NULL);
    cal_data << boost::format("version, 0, 1\n");
    cal_data << boost::format("DATA STARTS HERE\n");
    cal_data << "tx_lo_frequency, tx_iq_correction_real, tx_iq_correction_imag, measured_suppression\n";

    for (size_t i = 0; i < results.size(); i++){
        cal_data
            << results[i].freq << ", "
            << results[i].real_corr << ", "
            << results[i].imag_corr << ", "
            << results[i].sup << "\n"
        ;
    }

    std::cout << "wrote cal data to " << cal_data_path << std::endl;
}

/***********************************************************************
 * Main
 **********************************************************************/
int UHD_SAFE_MAIN(int argc, char *argv[]){
    std::string args;
    double rate, tx_wave_freq, tx_wave_ampl, rx_offset, freq_step;
    size_t nsamps;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "device address args [default = \"\"]")
        ("rate", po::value<double>(&rate)->default_value(12.5e6), "RX and TX sample rate in Hz")
        ("tx_wave_freq", po::value<double>(&tx_wave_freq)->default_value(507.123e3), "Transmit wave frequency in Hz")
        ("tx_wave_ampl", po::value<double>(&tx_wave_ampl)->default_value(0.7), "Transmit wave amplitude in counts")
        ("rx_offset", po::value<double>(&rx_offset)->default_value(.9344e6), "RX LO offset from the TX LO in Hz")
        ("freq_step", po::value<double>(&freq_step)->default_value(20e6), "Step size for LO sweep in Hz")
        ("nsamps", po::value<size_t>(&nsamps)->default_value(10000), "Samples per data capture")
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

    //set max receiver gain
    usrp->set_rx_gain(usrp->get_rx_gain_range().stop());

    //create a receive streamer
    uhd::stream_args_t stream_args("fc32"); //complex floats
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    //create a transmitter thread
    boost::thread_group threads;
    threads.create_thread(boost::bind(&tx_thread, usrp, tx_wave_freq, tx_wave_ampl));

    //re-usable buffer for samples
    std::vector<std::complex<float> > buff(nsamps);

    const uhd::meta_range_t freq_range = usrp->get_tx_freq_range();
    std::vector<result_t> results;

    for (double tx_lo = freq_range.start()+freq_step; tx_lo < freq_range.stop()-freq_step; tx_lo += freq_step){

        tune_rx_and_tx(usrp, tx_lo, rx_offset);

        double phase_corr_start = -.3;
        double phase_corr_stop = .3;
        double phase_corr_step;

        double ampl_corr_start = -.3;
        double ampl_corr_stop = .3;
        double ampl_corr_step;

        std::complex<double> best_correction;
        double best_suppression = 0;
        double best_phase_corr = 0;
        double best_ampl_corr = 0;

        for (size_t i = 0; i < 7; i++){

            phase_corr_step = (phase_corr_stop - phase_corr_start)/4;
            ampl_corr_step = (ampl_corr_stop - ampl_corr_start)/4;

            for (double phase_corr = phase_corr_start; phase_corr <= phase_corr_stop; phase_corr += phase_corr_step){
            for (double ampl_corr = ampl_corr_start; ampl_corr <= ampl_corr_stop; ampl_corr += ampl_corr_step){

                const std::complex<double> correction = std::polar(ampl_corr+1, phase_corr*tau);
                usrp->set_tx_iq_balance(correction);

                //receive some samples
                capture_samples(usrp, rx_stream, buff);

                const double actual_rx_rate = usrp->get_rx_rate();
                const double actual_tx_freq = usrp->get_tx_freq();
                const double actual_rx_freq = usrp->get_rx_freq();
                const double bb_tone_freq = actual_tx_freq + tx_wave_freq - actual_rx_freq;
                const double bb_imag_freq = actual_tx_freq - tx_wave_freq - actual_rx_freq;

                const double tone_dbrms = compute_tone_dbrms(buff, bb_tone_freq/actual_rx_rate);
                const double imag_dbrms = compute_tone_dbrms(buff, bb_imag_freq/actual_rx_rate);
                const double suppression = tone_dbrms - imag_dbrms;

                //std::cout << "bb_tone_freq " << bb_tone_freq << std::endl;
                //std::cout << "bb_imag_freq " << bb_imag_freq << std::endl;
                //std::cout << "tone_dbrms " << tone_dbrms << std::endl;
                //std::cout << "imag_dbrms " << imag_dbrms << std::endl;
                //std::cout << "suppression " << (tone_dbrms - imag_dbrms) << std::endl;

                if (suppression > best_suppression){
                    best_correction = correction;
                    best_suppression = suppression;
                    best_phase_corr = phase_corr;
                    best_ampl_corr = ampl_corr;
                    //std::cout << "   suppression! " << suppression << std::endl;
                }

            }}

            //std::cout << "best_phase_corr " << best_phase_corr << std::endl;
            //std::cout << "best_ampl_corr " << best_ampl_corr << std::endl;
            //std::cout << "best_suppression " << best_suppression << std::endl;

            phase_corr_start = best_phase_corr - phase_corr_step;
            phase_corr_stop = best_phase_corr + phase_corr_step;
            ampl_corr_start = best_ampl_corr - ampl_corr_step;
            ampl_corr_stop = best_ampl_corr + ampl_corr_step;
        }

        if (best_suppression > 30){ //most likely valid, keep result
            result_t result;
            result.freq = tx_lo;
            result.real_corr = best_correction.real();
            result.imag_corr = best_correction.imag();
            result.sup = best_suppression;
            results.push_back(result);
        }
        std::cout << "." << std::flush;

    }
    std::cout << std::endl;

    //stop the transmitter
    threads.interrupt_all();
    threads.join_all();

    store_results(usrp, results);

    return 0;
}

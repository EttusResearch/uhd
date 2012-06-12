//
// Copyright 2012 Ettus Research LLC
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
#include <complex>
#include <vector>

namespace po = boost::program_options;

/************************************************************************
 * Misc functions
************************************************************************/

std::string return_MHz_string(double freq){
    std::string nice_string = std::string(str(boost::format("%5.2f MHz") % (freq / 1e6)));
    return nice_string;
}

std::string return_USRP_config_string(uhd::usrp::multi_usrp::sptr usrp, bool test_tx, bool test_rx){
    uhd::dict<std::string, std::string> tx_info = usrp->get_usrp_tx_info();
    uhd::dict<std::string, std::string> rx_info = usrp->get_usrp_rx_info();
    std::string info_string;
    std::string mboard_id, mboard_serial;
    std::string tx_serial, tx_subdev_name, tx_subdev_spec;
    std::string rx_serial, rx_subdev_name, rx_subdev_spec;

    mboard_id = tx_info.get("mboard_id");
    if(tx_info.get("mboard_serial") != "") mboard_serial = tx_info.get("mboard_serial");
    else mboard_serial = "no serial";

    info_string = std::string(str(boost::format("Motherboard: %s (%s)\n") % mboard_id % mboard_serial));

    if(test_tx){
        if(tx_info.get("tx_serial") != "") tx_serial = tx_info.get("tx_serial");
        else tx_serial = "no serial"; 
        tx_subdev_name = tx_info.get("tx_subdev_name");
        tx_subdev_spec = tx_info.get("tx_subdev_spec");

        info_string += std::string(str(boost::format("TX: %s (%s, %s)") % tx_subdev_name % tx_serial % tx_subdev_spec));
    }
    if(test_tx and test_rx) info_string += "\n";
    if(test_rx){
        if(rx_info.get("rx_serial") != "") rx_serial = rx_info.get("rx_serial");
        else rx_serial = "no serial";
        rx_subdev_name = rx_info.get("rx_subdev_name");
        rx_subdev_spec = rx_info.get("rx_subdev_spec");

        info_string += std::string(str(boost::format("RX: %s (%s, %s)") % rx_subdev_name % rx_serial % rx_subdev_spec));
    }

    return info_string;
}

/************************************************************************
 * TX Frequency/Gain Coercion
************************************************************************/

std::string tx_test(uhd::usrp::multi_usrp::sptr usrp, bool test_gain, bool verbose){

    //Establish frequency range

    std::vector<double> freqs;
    std::vector<double> xcvr_freqs;

    BOOST_FOREACH(const uhd::range_t &range, usrp->get_fe_tx_freq_range()){
        double freq_begin = range.start();
        double freq_end = range.stop();
        double freq_step;

        if(usrp->get_usrp_tx_info().get("tx_subdev_name") == "XCVR2450 TX"){
            xcvr_freqs.push_back(freq_begin);
            xcvr_freqs.push_back(freq_end);
        }

        if(freq_end - freq_begin > 1000e6) freq_step = 100e6;
        else if(freq_end - freq_begin < 300e6) freq_step = 10e6;
        else freq_step = 50e6;

        double current_freq = freq_begin;

        while(current_freq < freq_end){
            freqs.push_back(current_freq);
            current_freq += freq_step;
        }
        if(freq_end != *freqs.end()) freqs.push_back(freq_end);
    }

    std::vector<double> gains;

    if(test_gain){

        //Establish gain range

        double gain_begin = usrp->get_tx_gain_range().start();
        if(gain_begin < 0.0) gain_begin = 0.0;
        double gain_end = usrp->get_tx_gain_range().stop();

        double current_gain = gain_begin;
        while(current_gain < gain_end){
            gains.push_back(current_gain);
            current_gain++;
        }
        if(gain_end != *gains.end()) gains.push_back(gain_end);

    }

    //Establish error-storing variables

    std::vector<double> bad_tune_freqs;
    std::vector<double> no_lock_freqs;
    std::vector< std::vector< double > > bad_gain_vals;
    std::vector<std::string> dboard_sensor_names = usrp->get_tx_sensor_names();
    std::vector<std::string> mboard_sensor_names = usrp->get_mboard_sensor_names();
    bool has_sensor = (std::find(dboard_sensor_names.begin(), dboard_sensor_names.end(), "lo_locked")) != dboard_sensor_names.end();

    for(std::vector<double>::iterator f = freqs.begin(); f != freqs.end(); ++f){

        //Testing for successful frequency tune

        usrp->set_tx_freq(*f);
        boost::this_thread::sleep(boost::posix_time::microseconds(long(1000)));

        double actual_freq = usrp->get_tx_freq();

        if(*f == 0.0){
            if(floor(actual_freq + 0.5) == 0.0){
                if(verbose) std::cout << boost::format("\nTX frequency successfully tuned to %s.") % return_MHz_string(*f) << std::endl;
            }
            else{
                if(verbose) std::cout << boost::format("\nTX frequency tuned to %s instead of %s.") % return_MHz_string(actual_freq) % return_MHz_string(*f) << std::endl;
            }
        }
        else{
            if((*f / actual_freq > 0.9999) and (*f / actual_freq < 1.0001)){
                if(verbose) std::cout << boost::format("\nTX frequency successfully tuned to %s.") % return_MHz_string(*f) << std::endl;
            }
            else{
                if(verbose) std::cout << boost::format("\nTX frequency tuned to %s instead of %s.") % return_MHz_string(actual_freq) % return_MHz_string(*f) << std::endl;
                bad_tune_freqs.push_back(*f);
            }
        }

        //Testing for successful lock

        if(has_sensor){
            bool is_locked = false;
            for(int i = 0; i < 1000; i++){
                boost::this_thread::sleep(boost::posix_time::microseconds(1000));
                if(usrp->get_tx_sensor("lo_locked",0).to_bool()){
                    is_locked = true;
                    break;
                }
            }
            if(is_locked){
                if(verbose) std::cout << boost::format("LO successfully locked at TX frequency %s.") % return_MHz_string(*f) << std::endl;
            }
            else{
                if(verbose) std::cout << boost::format("LO did not successfully lock at TX frequency %s.") % return_MHz_string(*f) << std::endl;
                no_lock_freqs.push_back(*f);
            }
        }

        if(test_gain){
            
            //Testing for successful gain tune

            for(std::vector<double>::iterator g = gains.begin(); g != gains.end(); ++g){
                usrp->set_tx_gain(*g);
                boost::this_thread::sleep(boost::posix_time::microseconds(1000));
                
                double actual_gain = usrp->get_tx_gain();

                if(*g == 0.0){
                    if(actual_gain == 0.0){
                        if(verbose) std::cout << boost::format("TX gain successfully set to %5.2f at TX frequency %s.") % *g % return_MHz_string(*f) << std::endl;
                    }    
                    else{
                        if(verbose) std::cout << boost::format("TX gain set to %5.2f instead of %5.2f at TX frequency %s.") % actual_gain % *g % return_MHz_string(*f) << std::endl;
                        std::vector<double> bad_gain_freq;
                        bad_gain_freq.push_back(*f);
                        bad_gain_freq.push_back(*g);
                        bad_gain_vals.push_back(bad_gain_freq);
                    }
                }
                else{
                    if((*g / actual_gain) > 0.9 and (*g / actual_gain) < 1.1){
                        if(verbose) std::cout << boost::format("TX gain successfully set to %5.2f at TX frequency %s.") % *g % return_MHz_string(*f) << std::endl;
                    }
                    else{
                        if(verbose) std::cout << boost::format("TX gain set to %5.2f instead of %5.2f at TX frequency %s.") % actual_gain % *g % return_MHz_string(*f) << std::endl;
                        std::vector<double> bad_gain_freq;
                        bad_gain_freq.push_back(*f);
                        bad_gain_freq.push_back(*g);
                        bad_gain_vals.push_back(bad_gain_freq);
                    }
                }
            }
        }
    }

    std::string tx_results = "TX Summary:\n";
    if(usrp->get_usrp_tx_info().get("tx_subdev_name") == "XCVR2450 TX"){
        tx_results += std::string(str(boost::format("Frequency Range: %s - %s, %s - %s\n") % return_MHz_string(xcvr_freqs.at(0)) % return_MHz_string(xcvr_freqs.at(1)) %
            return_MHz_string(xcvr_freqs.at(2)) % return_MHz_string(xcvr_freqs.at(3))));
    }
    else tx_results += std::string(str(boost::format("Frequency Range: %s - %s\n") % return_MHz_string(freqs.front()) % return_MHz_string(freqs.back())));
    if(test_gain) tx_results += std::string(str(boost::format("Gain Range: %5.2f - %5.2f\n") % gains.front() % gains.back()));

    if(bad_tune_freqs.empty()) tx_results += "USRP successfully tuned to all frequencies.";
    else{
        tx_results += "USRP did not successfully tune to the following frequencies: ";
        for(std::vector<double>::iterator i = bad_tune_freqs.begin(); i != bad_tune_freqs.end(); ++i){
            if(i != bad_tune_freqs.begin()) tx_results += ", ";
            tx_results += return_MHz_string(*i);
        }
    }
    if(has_sensor){

        tx_results += "\n";
        if(no_lock_freqs.empty()) tx_results += "LO successfully locked at all frequencies.";
        else{
            tx_results += "LO did not lock at the following frequencies: ";
            for(std::vector<double>::iterator i = no_lock_freqs.begin(); i != no_lock_freqs.end(); ++i){
                if(i != no_lock_freqs.begin()) tx_results += ", ";
                tx_results += return_MHz_string(*i);
            }
        }
    }
    if(test_gain){
        tx_results += "\n";
        if(bad_gain_vals.empty()) tx_results += "USRP successfully set all specified gain values at all frequencies.";
        else{
            tx_results += "USRP did not successfully set gain under the following circumstances:";
            for(std::vector< std::vector<double> >::iterator i = bad_gain_vals.begin(); i != bad_gain_vals.end(); ++i){
                std::vector<double> bad_pair = *i;
                double bad_freq = bad_pair.front();
                double bad_gain = bad_pair.back();
                tx_results += std::string(str(boost::format("\nFrequency: %s, Gain: %5.2f") % return_MHz_string(bad_freq) % bad_gain));
            }
        }
    }

    return tx_results;
}

/************************************************************************
 * RX Frequency/Gain Coercion
************************************************************************/

std::string rx_test(uhd::usrp::multi_usrp::sptr usrp, bool test_gain, bool verbose){

    //Establish frequency range

    std::vector<double> freqs;
    std::vector<double> xcvr_freqs;

    BOOST_FOREACH(const uhd::range_t &range, usrp->get_fe_rx_freq_range()){
        double freq_begin = range.start();
        double freq_end = range.stop();

        if(usrp->get_usrp_rx_info().get("rx_subdev_name") == "XCVR2450 RX"){
            xcvr_freqs.push_back(freq_begin);
            xcvr_freqs.push_back(freq_end);
        }

        double freq_step;

        if(freq_end - freq_begin > 1000e6) freq_step = 100e6;
        else if(freq_end - freq_begin < 300e6) freq_step = 10e6;
        else freq_step = 50e6;

        double current_freq = freq_begin;

        while(current_freq < freq_end){
            freqs.push_back(current_freq);
            current_freq += freq_step;
        }
    }

    std::vector<double> gains;

    if(test_gain){

        //Establish gain range

        double gain_begin = usrp->get_rx_gain_range().start();
        if(gain_begin < 0.0) gain_begin = 0.0;
        double gain_end = usrp->get_rx_gain_range().stop();

        double current_gain = gain_begin;
        while(current_gain < gain_end){
            gains.push_back(current_gain);
            current_gain++;
        }
        if(gain_end != *gains.end()) gains.push_back(gain_end);

    }

    //Establish error-storing variables

    std::vector<double> bad_tune_freqs;
    std::vector<double> no_lock_freqs;
    std::vector< std::vector< double > > bad_gain_vals;
    std::vector<std::string> dboard_sensor_names = usrp->get_rx_sensor_names();
    std::vector<std::string> mboard_sensor_names = usrp->get_mboard_sensor_names();
    bool has_sensor = (std::find(dboard_sensor_names.begin(), dboard_sensor_names.end(), "lo_locked")) != dboard_sensor_names.end();

    for(std::vector<double>::iterator f = freqs.begin(); f != freqs.end(); ++f){

        //Testing for successful frequency tune

        usrp->set_rx_freq(*f);
        boost::this_thread::sleep(boost::posix_time::microseconds(long(1000)));

        double actual_freq = usrp->get_rx_freq();

        if(*f == 0.0){
            if(floor(actual_freq + 0.5) == 0.0){
                if(verbose) std::cout << boost::format("\nRX frequency successfully tuned to %s.") % return_MHz_string(*f) << std::endl;
            }
            else{
                if(verbose) std::cout << boost::format("\nRX frequency tuned to %s instead of %s.") % return_MHz_string(actual_freq) % return_MHz_string(*f) << std::endl;
            }
        }
        else{
            if((*f / actual_freq > 0.9999) and (*f / actual_freq < 1.0001)){
                if(verbose) std::cout << boost::format("\nRX frequency successfully tuned to %s.") % return_MHz_string(*f) << std::endl;
            }
            else{
                if(verbose) std::cout << boost::format("\nRX frequency tuned to %s instead of %s.") % return_MHz_string(actual_freq) % return_MHz_string(*f) << std::endl;
                bad_tune_freqs.push_back(*f);
            }
        }

        //Testing for successful lock

        if(has_sensor){
            bool is_locked = false;
            for(int i = 0; i < 1000; i++){
                boost::this_thread::sleep(boost::posix_time::microseconds(1000));
                if(usrp->get_rx_sensor("lo_locked",0).to_bool()){
                    is_locked = true;
                    break;
                }
            }
            if(is_locked){
                if(verbose) std::cout << boost::format("LO successfully locked at RX frequency %s.") % return_MHz_string(*f) << std::endl;
            }
            else{
                if(verbose) std::cout << boost::format("LO did not successfully lock at RX frequency %s.") % return_MHz_string(*f) << std::endl;
                no_lock_freqs.push_back(*f);
            }
        }

        if(test_gain){

            //Testing for successful gain tune

            for(std::vector<double>::iterator g = gains.begin(); g != gains.end(); ++g){
                usrp->set_rx_gain(*g);
                boost::this_thread::sleep(boost::posix_time::microseconds(1000));

                double actual_gain = usrp->get_rx_gain();

                if(*g == 0.0){
                    if(actual_gain == 0.0){
                        if(verbose) std::cout << boost::format("RX gain successfully set to %5.2f at RX frequency %s.") % *g % return_MHz_string(*f) << std::endl;
                    }
                    else{
                        if(verbose) std::cout << boost::format("RX gain set to %5.2f instead of %5.2f at RX frequency %s.") % actual_gain % *g % return_MHz_string(*f) << std::endl;
                        std::vector<double> bad_gain_freq;
                        bad_gain_freq.push_back(*f);
                        bad_gain_freq.push_back(*g);
                        bad_gain_vals.push_back(bad_gain_freq);
                    }
                }
                else{
                    if((*g / actual_gain) > 0.9 and (*g / actual_gain) < 1.1){
                        if(verbose) std::cout << boost::format("RX gain successfully set to %5.2f at RX frequency %s.") % *g % return_MHz_string(*f) << std::endl;
                    }
                    else{
                        if(verbose) std::cout << boost::format("RX gain set to %5.2f instead of %5.2f at RX frequency %s.") % actual_gain % *g % return_MHz_string(*f) << std::endl;
                        std::vector<double> bad_gain_freq;
                        bad_gain_freq.push_back(*f);
                        bad_gain_freq.push_back(*g);
                        bad_gain_vals.push_back(bad_gain_freq);
                    }
                }
            }
        }
    }

    std::string rx_results = "RX Summary:\n";
    if(usrp->get_usrp_rx_info().get("rx_subdev_name") == "XCVR2450 RX"){
        rx_results += std::string(str(boost::format("Frequency Range: %s - %s, %s - %s\n") % return_MHz_string(xcvr_freqs.at(0)) % return_MHz_string(xcvr_freqs.at(1)) %
            return_MHz_string(xcvr_freqs.at(2)) % return_MHz_string(xcvr_freqs.at(3))));
    }
    else rx_results += std::string(str(boost::format("Frequency Range: %s - %s\n") % return_MHz_string(freqs.front()) % return_MHz_string(freqs.back())));
    if(test_gain) rx_results += std::string(str(boost::format("Gain Range: %5.2f - %5.2f\n") % gains.front() % gains.back()));

    if(bad_tune_freqs.empty()) rx_results += "USRP successfully tuned to all frequencies.";
    else{
        rx_results += "USRP did not successfully tune to the following frequencies: ";
        for(std::vector<double>::iterator i = bad_tune_freqs.begin(); i != bad_tune_freqs.end(); ++i){
            if(i != bad_tune_freqs.begin()) rx_results += ", ";
            rx_results += return_MHz_string(*i);
        }
    }
    if(has_sensor){

        rx_results += "\n";
        if(no_lock_freqs.empty()) rx_results += "LO successfully locked at all frequencies.";
        else{
            rx_results += "LO did not successfully lock at the following frequencies: ";
            for(std::vector<double>::iterator i = no_lock_freqs.begin(); i != no_lock_freqs.end(); ++i){
                if( i != no_lock_freqs.begin()) rx_results += ", ";
                rx_results += return_MHz_string(*i);
            }
        }
    }
    if(test_gain){
        rx_results += "\n";
        if(bad_gain_vals.empty()) rx_results += "USRP successfully set all specified gain values at all frequencies.";
        else{
            rx_results += "USRP did not successfully set gain under the following circumstances:";
            for(std::vector< std::vector<double> >::iterator i = bad_gain_vals.begin(); i != bad_gain_vals.end(); ++i){
                std::vector<double> bad_pair = *i;
                double bad_freq = bad_pair.front();
                double bad_gain = bad_pair.back();
                rx_results += std::string(str(boost::format("\nFrequency: %s, Gain: %5.2f") % return_MHz_string(bad_freq) % bad_gain));
            }
        }
    }

    return rx_results;
}

/************************************************************************
 * Initial Setup
************************************************************************/

int UHD_SAFE_MAIN(int argc, char *argv[]){

    //Variables
    std::string args;
    double gain_step;
    std::string ref;
    std::string tx_results;
    std::string rx_results;
    std::string usrp_config;

    //Set up the program options
    po::options_description desc("Allowed Options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "Specify the UHD device")
        ("gain_step", po::value<double>(&gain_step)->default_value(1.0), "Specify the delta between gain scans")
        ("tx", "Specify to test TX frequency and gain coercion")
        ("rx", "Specify to test RX frequency and gain coercion")
        ("ref", po::value<std::string>(&ref)->default_value("internal"), "Waveform type: internal, external, or mimo")
        ("no_tx_gain", "Do not test TX gain")
        ("no_rx_gain", "Do not test RX gain")
        ("verbose", "Output every frequency and gain check instead of just final summary")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //Create a USRP device
    std::cout << std::endl;
    uhd::device_addrs_t device_addrs = uhd::device::find(args);
    std::cout << boost::format("Creating the USRP device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
    std::cout << std::endl << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;
    usrp->set_tx_rate(1e6);
    usrp->set_rx_rate(1e6);

    //Boolean variables based on command line input
    bool test_tx = vm.count("tx") > 0;
    bool test_rx = vm.count("rx") > 0;
    bool test_tx_gain = !(vm.count("no_tx_gain") > 0) and (usrp->get_tx_gain_range().stop() > 0);
    bool test_rx_gain = !(vm.count("no_rx_gain") > 0) and (usrp->get_rx_gain_range().stop() > 0);
    bool verbose = vm.count("verbose") > 0;

    //Help messages, errors
    if(vm.count("help") > 0){
        std::cout << "UHD Daughterboard Coercion Test\n"
                     "This program tests your USRP daughterboard(s) to\n"
                     "make sure that they can successfully tune to all\n"
                     "frequencies and gains in their advertised ranges.\n\n";
        std::cout << desc << std::endl;
        return ~0;
    }

    if(ref != "internal" and ref != "external" and ref != "mimo"){
        std::cout << desc << std::endl;
        std::cout << "REF must equal internal, external, or mimo." << std::endl;
        return ~0;
    }
 
    if(vm.count("tx") + vm.count("rx") == 0){
        std::cout << desc << std::endl;
        std::cout << "Specify --tx to test for TX frequency coercion\n"
                     "Specify --rx to test for RX frequency coercion\n";
        return ~0;
    }

    if(test_rx and usrp->get_usrp_rx_info().get("rx_id") == "Basic RX (0x0001)"){
        std::cout << desc << std::endl;
        std::cout << "This test does not work with the Basic RX daughterboard." << std::endl;
        return ~0;
    }
    else if(test_rx and usrp->get_usrp_rx_info().get("rx_id") == "Unknown (0xffff)"){
        std::cout << desc << std::endl;
        std::cout << "This daughterboard is unrecognized, or there is no RX daughterboard." << std::endl;
        return ~0;
    }

    if(test_tx and usrp->get_usrp_tx_info().get("tx_id") == "Basic TX (0x0000)"){
        std::cout << desc << std::endl;
        std::cout << "This test does not work with the Basic TX daughterboard." << std::endl;
        return ~0;
    }
    else if(test_tx and usrp->get_usrp_tx_info().get("tx_id") == "Unknown (0xffff)"){
        std::cout << desc << std::endl;
        std::cout << "This daughterboard is unrecognized, or there is no TX daughterboard." << std::endl;
        return ~0;
    }

    //Setting clock source
    usrp->set_clock_source(ref);
    boost::this_thread::sleep(boost::posix_time::seconds(1));

    std::vector<std::string> sensor_names = usrp->get_mboard_sensor_names(0);
    if ((ref == "mimo") and (std::find(sensor_names.begin(), sensor_names.end(), "mimo_locked") != sensor_names.end())) {
        uhd::sensor_value_t mimo_locked = usrp->get_mboard_sensor("mimo_locked",0);
        std::cout << boost::format("Checking MIMO lock: %s ...") % mimo_locked.to_pp_string() << std::endl;
        UHD_ASSERT_THROW(mimo_locked.to_bool());
    }   
    if ((ref == "external") and (std::find(sensor_names.begin(), sensor_names.end(), "ref_locked") != sensor_names.end())) {
        uhd::sensor_value_t ref_locked = usrp->get_mboard_sensor("ref_locked",0);
        std::cout << boost::format("Checking REF lock: %s ...") % ref_locked.to_pp_string() << std::endl;
        UHD_ASSERT_THROW(ref_locked.to_bool());
    }   
    usrp_config = return_USRP_config_string(usrp, test_tx, test_rx);
    if(test_tx) tx_results = tx_test(usrp, test_tx_gain, verbose);
    if(test_rx) rx_results = rx_test(usrp, test_rx_gain, verbose);

    if(verbose) std::cout << std::endl;
    std::cout << usrp_config << std::endl << std::endl;
    if(test_tx) std::cout << tx_results << std::endl;
    if(test_tx and test_rx) std::cout << std::endl;
    if(test_rx) std::cout << rx_results << std::endl;

    return 0;
}

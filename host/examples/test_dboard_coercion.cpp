//
// Copyright 2012,2014,20160 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/thread.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/math/special_functions/round.hpp>
#include <iostream>
#include <complex>
#include <utility>
#include <vector>
#include <chrono>
#include <thread>

static const double SAMP_RATE = 1e6;

namespace po = boost::program_options;

typedef std::pair<double, double> double_pair; //BOOST_FOREACH doesn't like commas
typedef std::vector<std::pair<double, double> > pair_vector;

/************************************************************************
 * Misc functions
************************************************************************/

std::string MHz_str(double freq){
    return std::string(str(boost::format("%5.2f MHz") % (freq / 1e6)));
}

std::string return_usrp_config_string(uhd::usrp::multi_usrp::sptr usrp, int chan, bool test_tx, bool test_rx, bool is_b2xx){
    uhd::dict<std::string, std::string> tx_info = usrp->get_usrp_tx_info(chan);
    uhd::dict<std::string, std::string> rx_info = usrp->get_usrp_rx_info(chan);
    std::string info_string;
    std::string mboard_id, mboard_serial;
    std::string tx_serial, tx_subdev_name, tx_subdev_spec;
    std::string rx_serial, rx_subdev_name, rx_subdev_spec;

    mboard_id = tx_info.get("mboard_id");
    if(tx_info.get("mboard_serial") == "") mboard_serial = "no serial";
    else mboard_serial = tx_info.get("mboard_serial");

    info_string = str(boost::format("Motherboard: %s (%s)\n") % mboard_id % mboard_serial);

    if(test_tx){
        if(tx_info.get("tx_serial") == "") tx_serial = "no serial";
        else tx_serial = tx_info.get("tx_serial");
        tx_subdev_name = tx_info.get("tx_subdev_name");
        tx_subdev_spec = tx_info.get("tx_subdev_spec");

        info_string += is_b2xx ? str(boost::format("TX: %s (%s)")
                                 % tx_subdev_name % tx_subdev_spec)
                               : str(boost::format("TX: %s (%s, %s)")
                                 % tx_subdev_name % tx_serial % tx_subdev_spec);
    }
    if(test_tx and test_rx) info_string += "\n";
    if(test_rx){
        if(rx_info.get("rx_serial") == "") rx_serial = "no serial";
        else rx_serial = rx_info.get("rx_serial");
        rx_subdev_name = rx_info.get("rx_subdev_name");
        rx_subdev_spec = rx_info.get("rx_subdev_spec");

        info_string += is_b2xx ? str(boost::format("RX: %s (%s)")
                                 % rx_subdev_name % rx_subdev_spec)
                               : str(boost::format("RX: %s (%s, %s)")
                                 % rx_subdev_name % rx_serial % rx_subdev_spec);
    }

    return info_string;
}

std::string coercion_test(uhd::usrp::multi_usrp::sptr usrp, std::string type, int chan,
                          bool test_gain, double freq_step, double gain_step, bool verbose){

    //Getting USRP info
    uhd::dict<std::string, std::string> usrp_info = (type == "TX") ? usrp->get_usrp_tx_info(chan)
                                                                   : usrp->get_usrp_rx_info(chan);
    std::string subdev_name = (type == "TX") ? usrp_info.get("tx_subdev_name")
                                             : usrp_info.get("rx_subdev_name");
    std::string subdev_spec = (type == "TX") ? usrp_info.get("tx_subdev_spec")
                                             : usrp_info.get("rx_subdev_spec");

    //Establish frequency range
    std::vector<double> freqs;
    std::vector<double> xcvr_freqs; //XCVR2450 has two ranges
    uhd::freq_range_t freq_ranges  = (type == "TX") ? usrp->get_fe_tx_freq_range(chan)
                                                    : usrp->get_fe_rx_freq_range(chan);

    std::cout << boost::format("\nTesting %s coercion...") % type << std::endl;

    for(const uhd::range_t &range:  freq_ranges){
        double freq_begin = range.start();
        double freq_end = range.stop();

        if(subdev_name.find("XCVR2450") == 0){
            xcvr_freqs.push_back(freq_begin);
            xcvr_freqs.push_back(freq_end);
        }

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
        uhd::gain_range_t gain_range = (type == "TX") ? usrp->get_tx_gain_range(chan)
                                                      : usrp->get_rx_gain_range(chan);

        double gain_begin = gain_range.start();
        //Start gain at 0 if range begins negative
        if(gain_begin < 0.0) gain_begin = 0.0;

        double gain_end = gain_range.stop();

        double current_gain = gain_begin;
        while(current_gain < gain_end){
            gains.push_back(current_gain);
            current_gain += gain_step;
        }
        gains.push_back(gain_end);
    }

    //Establish error-storing variables
    std::vector<double> bad_tune_freqs;
    std::vector<double> no_lock_freqs;
    pair_vector bad_gain_vals;

    //Sensor names
    std::vector<std::string> dboard_sensor_names = (type == "TX") ? usrp->get_tx_sensor_names(chan)
                                                                  : usrp->get_rx_sensor_names(chan);
    std::vector<std::string> mboard_sensor_names = usrp->get_mboard_sensor_names();

    bool has_sensor = (std::find(dboard_sensor_names.begin(), dboard_sensor_names.end(), "lo_locked")) != dboard_sensor_names.end();

    for(double freq:  freqs){

        //Testing for successful frequency tune
        if(type == "TX") usrp->set_tx_freq(freq,chan);
        else usrp->set_rx_freq(freq,chan);

        std::this_thread::sleep_for(std::chrono::microseconds(long(1000)));
        double actual_freq = (type == "TX") ? usrp->get_tx_freq(chan)
                                            : usrp->get_rx_freq(chan);

        if(freq == 0.0){
            if(floor(actual_freq + 0.5) == 0.0){
                if(verbose) std::cout << boost::format("\n%s frequency successfully tuned to %s.")
                                         % type % MHz_str(freq) << std::endl;
            }
            else{
                if(verbose) std::cout << boost::format("\n%s frequency tuned to %s instead of %s.")
                                         % type % MHz_str(actual_freq) % MHz_str(freq) << std::endl;
                bad_tune_freqs.push_back(freq);
            }
        }
        else{
            if((freq / actual_freq > 0.9999) and (freq / actual_freq < 1.0001)){
                if(verbose) std::cout << boost::format("\n%s frequency successfully tuned to %s.")
                                         % type % MHz_str(freq) << std::endl;
            }
            else{
                if(verbose) std::cout << boost::format("\n%s frequency tuned to %s instead of %s.")
                                         % type % MHz_str(actual_freq) % MHz_str(freq) << std::endl;
                bad_tune_freqs.push_back(freq);
            }
        }

        //Testing for successful lock
        if (has_sensor) {
            bool is_locked = false;
            for(int i = 0; i < 1000; i++){
                is_locked = (type == "TX") ?
                    usrp->get_tx_sensor("lo_locked", 0).to_bool() :
                    usrp->get_rx_sensor("lo_locked", 0).to_bool();
                if (is_locked) {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::microseconds(1000));
            }
            if(is_locked){
                if(verbose) std::cout << boost::format("LO successfully locked at %s frequency %s.")
                                         % type % MHz_str(freq) << std::endl;
            }
            else{
                if(verbose) std::cout << boost::format("LO did not successfully lock at %s frequency %s.")
                                         % type % MHz_str(freq) << std::endl;
                no_lock_freqs.push_back(freq);
            }
        }

        if(test_gain){

            //Testing for successful gain tune

            for(double gain:  gains){
                if(type == "TX") usrp->set_tx_gain(gain,chan);
                else usrp->set_rx_gain(gain,chan);

                std::this_thread::sleep_for(std::chrono::microseconds(1000));

                double actual_gain = (type == "TX") ? usrp->get_tx_gain(chan)
                                                    : usrp->get_rx_gain(chan);

                if(gain == 0.0){
                    if(actual_gain == 0.0){
                        if(verbose) std::cout << boost::format("Gain successfully set to %5.2f at %s frequency %s.")
                                                 % gain % type % MHz_str(freq) << std::endl;
                    }
                    else{
                        if(verbose) std::cout << boost::format("Gain set to %5.2f instead of %5.2f at %s frequency %s.")
                                                 % actual_gain % gain % type % MHz_str(freq) << std::endl;
                        bad_gain_vals.push_back(std::make_pair(freq, gain));
                    }
                }
                else{
                    if((gain / actual_gain) > 0.9999 and (gain / actual_gain) < 1.0001){
                        if(verbose) std::cout << boost::format("Gain successfully set to %5.2f at %s frequency %s.")
                                                 % gain % type % MHz_str(freq) << std::endl;
                    }
                    else{
                        if(verbose) std::cout << boost::format("Gain set to %5.2f instead of %5.2f at %s frequency %s.")
                                                 % actual_gain % gain % type % MHz_str(freq) << std::endl;
                        bad_gain_vals.push_back(std::make_pair(freq, gain));
                    }
                }
            }
        }
    }

    std::string results = str(boost::format("%s Summary:\n") % type);
    if(subdev_name.find("XCVR2450") == 0){
        results += str(boost::format("Frequency Range: %s - %s, %s - %s\n")
                       % MHz_str(xcvr_freqs[0]) % MHz_str(xcvr_freqs[1])
                       % MHz_str(xcvr_freqs[2]) % MHz_str(xcvr_freqs[3]));
    }
    else results += str(boost::format("Frequency Range: %s - %s (Step: %s)\n")
                        % MHz_str(freqs.front()) % MHz_str(freqs.back()) % MHz_str(freq_step));
    if(test_gain) results += str(boost::format("Gain Range:%5.2f - %5.2f (Step:%5.2f)\n")
                             % gains.front() % gains.back() % gain_step);

    if(bad_tune_freqs.empty()) results += "USRP successfully tuned to all frequencies.";
    else if(bad_tune_freqs.size() > 10 and not verbose){
        //If tuning fails at many values, don't print them all
        results += str(boost::format("USRP did not successfully tune at %d frequencies.")
                       % bad_tune_freqs.size());
    }
    else{
        results += "USRP did not successfully tune to the following frequencies: ";
        for(double bad_freq:  bad_tune_freqs){
            if(bad_freq != *bad_tune_freqs.begin()) results += ", ";
            results += MHz_str(bad_freq);
        }
    }
    if(has_sensor){

        results += "\n";
        if(no_lock_freqs.empty()) results += "LO successfully locked at all frequencies.";
        else if(no_lock_freqs.size() > 10 and not verbose){
            //If locking fails at many values, don't print them all
            results += str(boost::format("USRP did not successfully lock at %d frequencies.")
                           % no_lock_freqs.size());
        }
        else{
            results += "LO did not lock at the following frequencies: ";
            for(double bad_freq:  no_lock_freqs){
                if(bad_freq != *no_lock_freqs.begin()) results += ", ";
                results += MHz_str(bad_freq);
            }
        }
    }
    if(test_gain){
        results += "\n";
        if(bad_gain_vals.empty()) results += "USRP successfully set all specified gain values at all frequencies.";
        else if(bad_gain_vals.size() > 10 and not verbose){
            //If gain fails at many values, don't print them all
            results += str(boost::format("USRP did not successfully set gain at %d values.")
                           % bad_gain_vals.size());
        }
        else{
            results += "USRP did not successfully set gain under the following circumstances:";
            for(double_pair bad_pair:  bad_gain_vals){
                double bad_freq = bad_pair.first;
                double bad_gain = bad_pair.second;
                results += str(boost::format("\nFrequency: %s, Gain: %5.2f") % MHz_str(bad_freq) % bad_gain);
            }
        }
    }

    return results;
}

/************************************************************************
 * Initial Setup
************************************************************************/

int UHD_SAFE_MAIN(int argc, char *argv[]){

    //Variables
    int chan;
    std::string args;
    double freq_step, gain_step;
    std::string ref;
    std::string tx_results;
    std::string rx_results;
    std::string usrp_config;

    //Set up the program options
    po::options_description desc("Allowed Options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "Specify the UHD device")
        ("chan", po::value<int>(&chan)->default_value(0), "Specify multi_usrp channel")
        ("freq-step", po::value<double>(&freq_step)->default_value(100e6), "Specify the delta between frequency scans")
        ("gain-step", po::value<double>(&gain_step)->default_value(1.0), "Specify the delta between gain scans")
        ("tx", "Specify to test TX frequency and gain coercion")
        ("rx", "Specify to test RX frequency and gain coercion")
        ("ref", po::value<std::string>(&ref)->default_value("internal"), "Waveform type: internal, external, or mimo")
        ("no-tx-gain", "Do not test TX gain")
        ("no-rx-gain", "Do not test RX gain")
        ("verbose", "Output every frequency and gain check instead of just final summary")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //Help messages, errors
    if(vm.count("help") > 0){
        std::cout << "UHD Daughterboard Coercion Test\n"
                     "This program tests your USRP daughterboard(s) to\n"
                     "make sure that they can successfully tune to all\n"
                     "frequencies and gains in their advertised ranges.\n\n";
        std::cout << desc << std::endl;
        return EXIT_SUCCESS;
    }

    if(vm.count("tx") + vm.count("rx") == 0){
        std::cout << desc << std::endl;
        std::cout << "Specify --tx to test for TX frequency coercion\n"
                     "Specify --rx to test for RX frequency coercion\n";
        return EXIT_FAILURE;
    }

    //Create a USRP device
    std::cout << std::endl;
    uhd::device_addrs_t device_addrs = uhd::device::find(args, uhd::device::USRP);
    std::cout << boost::format("Creating the USRP device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
    std::cout << std::endl << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;
    usrp->set_tx_rate(SAMP_RATE);
    usrp->set_rx_rate(SAMP_RATE);

    //Boolean variables based on command line input
    bool test_tx = vm.count("tx") > 0;
    bool test_rx = vm.count("rx") > 0;
    bool test_tx_gain = !(vm.count("no-tx-gain") > 0) and (usrp->get_tx_gain_range().stop() > 0);
    bool test_rx_gain = !(vm.count("no-rx-gain") > 0) and (usrp->get_rx_gain_range().stop() > 0);
    bool verbose = vm.count("verbose") > 0;

    if(ref != "internal" and ref != "external" and ref != "mimo"){
        std::cout << desc << std::endl;
        std::cout << "REF must equal internal, external, or mimo." << std::endl;
        return EXIT_FAILURE;
    }

    //Use TX mboard ID to determine if this is a B2xx, will still return value if there is no TX
    std::string tx_mboard_id = usrp->get_usrp_tx_info(chan).get("mboard_id");
    bool is_b2xx = (tx_mboard_id == "B200" or tx_mboard_id == "B210");

    //Don't perform daughterboard validity checks for B200/B210
    if((not is_b2xx) and test_tx){
        std::string tx_dboard_name = usrp->get_usrp_tx_info(chan).get("tx_id");
        if(tx_dboard_name == "Basic TX (0x0000)" or tx_dboard_name == "LF TX (0x000e)"){
            std::cout << desc << std::endl;
            std::cout << boost::format("This test does not work with the %s daughterboard.")
                         % tx_dboard_name << std::endl;
            return EXIT_FAILURE;
        }
        else if(tx_dboard_name == "Unknown (0xffff)"){
            std::cout << desc << std::endl;
            std::cout << "This daughterboard is unrecognized, or there is no TX daughterboard." << std::endl;
            return EXIT_FAILURE;
        }
    }

    //Don't perform daughterboard validity checks for B200/B210
    if((not is_b2xx) and test_rx){
        std::string rx_dboard_name = usrp->get_usrp_rx_info(chan).get("rx_id");
        if(rx_dboard_name == "Basic RX (0x0001)" or rx_dboard_name == "LF RX (0x000f)"){
            std::cout << desc << std::endl;
            std::cout << boost::format("This test does not work with the %s daughterboard.")
                         % rx_dboard_name << std::endl;
            return EXIT_FAILURE;
        }
        else if(rx_dboard_name == "Unknown (0xffff)"){
            std::cout << desc << std::endl;
            std::cout << "This daughterboard is unrecognized, or there is no RX daughterboard." << std::endl;
            return EXIT_FAILURE;
        }
    }

    //Setting clock source
    usrp->set_clock_source(ref);
    std::this_thread::sleep_for(std::chrono::seconds(1));

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
    usrp_config = return_usrp_config_string(usrp, chan, test_tx, test_rx, is_b2xx);
    if(test_tx) tx_results = coercion_test(usrp, "TX", chan, test_tx_gain, freq_step, gain_step, verbose);
    if(test_rx) rx_results = coercion_test(usrp, "RX", chan, test_rx_gain, freq_step, gain_step, verbose);

    std::cout << std::endl << usrp_config << std::endl << std::endl;
    if(test_tx) std::cout << tx_results << std::endl;
    if(test_tx and test_rx) std::cout << std::endl;
    if(test_rx) std::cout << rx_results << std::endl;

    return EXIT_SUCCESS;
}

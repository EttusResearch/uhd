//
// Copyright 2010-2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/thread.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <csignal>
#include <iostream>
#include <complex>

namespace po = boost::program_options;

static bool stop_signal_called = false;
void sig_int_handler(int){stop_signal_called = true;}

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args, channel_list;
    double seconds_in_future;
    size_t total_num_samps;
    double rate;
    float ampl;
    double freq;
    double rep_rate;
    double gain;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "multi uhd device address args")
        ("secs", po::value<double>(&seconds_in_future)->default_value(1.5), "delay before first burst")
        ("repeat", "repeat burst")
        ("rep-delay", po::value<double>(&rep_rate)->default_value(0.5), "delay between bursts")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(10000), "total number of samples to transmit")
        ("rate", po::value<double>(&rate)->default_value(100e6/16), "rate of outgoing samples")
        ("ampl", po::value<float>(&ampl)->default_value(float(0.3)), "amplitude of each sample")
        ("freq", po::value<double>(&freq)->default_value(0), "center frequency")
        ("gain", po::value<double>(&gain)->default_value(0), "gain")
        ("dilv", "specify to disable inner-loop verbose")
        ("channels", po::value<std::string>(&channel_list)->default_value("0"), "which channel(s) to use (specify \"0\", \"1\", \"0,1\", etc")
        ("int-n", "tune USRP with integer-n tuning")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("UHD TX Timed Samples %s") % desc << std::endl;
        return ~0;
    }

    bool verbose = vm.count("dilv") == 0;
    bool repeat = vm.count("repeat") != 0;

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    //detect which channels to use
    std::vector<std::string> channel_strings;
    std::vector<size_t> channel_nums;
    boost::split(channel_strings, channel_list, boost::is_any_of("\"',"));
    for(size_t ch = 0; ch < channel_strings.size(); ch++){
        size_t chan = std::stoi(channel_strings[ch]);
        if(chan >= usrp->get_tx_num_channels()){
            throw std::runtime_error("Invalid channel(s) specified.");
        }
        else channel_nums.push_back(std::stoi(channel_strings[ch]));
    }

    //set the tx sample rate
    std::cout << boost::format("Setting TX Rate: %f Msps...") % (rate/1e6) << std::endl;
    usrp->set_tx_rate(rate);
    std::cout << boost::format("Actual TX Rate: %f Msps...") % (usrp->get_tx_rate()/1e6) << std::endl << std::endl;

    std::cout << boost::format("Setting TX Freq: %f MHz...") % (freq/1e6) << std::endl;
    for(size_t i=0; i < channel_nums.size(); i++){
        uhd::tune_request_t tune_request(freq);
        if(vm.count("int-n")) tune_request.args = uhd::device_addr_t("mode_n=integer");
        usrp->set_tx_freq(tune_request, channel_nums[i]);
    }
    std::cout << boost::format("Actual TX Freq: %f MHz...") % (usrp->get_tx_freq()/1e6) << std::endl << std::endl;

    std::cout << boost::format("Setting TX Gain: %f...") % (gain) << std::endl;
    for(size_t i=0; i < channel_nums.size(); i++) usrp->set_tx_gain(gain, channel_nums[i]);
    std::cout << boost::format("Actual TX Gain: %f...") % (usrp->get_tx_gain()) << std::endl << std::endl;

    std::cout << boost::format("Setting device timestamp to 0...") << std::endl;
    usrp->set_time_now(uhd::time_spec_t(0.0));

    //create a transmit streamer
    uhd::stream_args_t stream_args("fc32"); //complex floats
    stream_args.channels = channel_nums;
    uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);

    //allocate buffer with data to send
    const size_t spb = tx_stream->get_max_num_samps();

    std::vector<std::complex<float> > buff(spb, std::complex<float>(ampl, ampl));
    std::vector<std::complex<float> *> buffs(channel_nums.size(), &buff.front());

    std::signal(SIGINT, &sig_int_handler);
    if(repeat) std::cout << "Press Ctrl + C to quit..." << std::endl;

    double time_to_send = seconds_in_future;

    do {
        //setup metadata for the first packet
        uhd::tx_metadata_t md;
        md.start_of_burst = true;
        md.end_of_burst = false;
        md.has_time_spec = true;
        md.time_spec = uhd::time_spec_t(time_to_send);

        //the first call to send() will block this many seconds before sending:
        double timeout = std::max(rep_rate, seconds_in_future) + 0.1; //timeout (delay before transmit + padding)

        size_t num_acc_samps = 0; //number of accumulated samples
        while(num_acc_samps < total_num_samps){
            size_t samps_to_send = total_num_samps - num_acc_samps;
            if (samps_to_send > spb)
            {
                samps_to_send = spb;
            } else {
                md.end_of_burst = true;
            }

            //send a single packet
            size_t num_tx_samps = tx_stream->send(
                buffs, samps_to_send, md, timeout
            );
            //do not use time spec for subsequent packets
            md.has_time_spec = false;
            md.start_of_burst = false;

            if (num_tx_samps < samps_to_send)
            {
                std::cerr << "Send timeout..." << std::endl;
                if (stop_signal_called)
                {
                    exit(EXIT_FAILURE);
                }
            }

            if(verbose)
            {
                std::cout << boost::format("Sent packet: %u samples") % num_tx_samps << std::endl;
            }

            num_acc_samps += num_tx_samps;
        }

        time_to_send += rep_rate;

        std::cout << std::endl << "Waiting for async burst ACK... " << std::flush;
        uhd::async_metadata_t async_md;
        size_t acks = 0;
        //loop through all messages for the ACK packets (may have underflow messages in queue)
        while (acks < channel_nums.size() and tx_stream->recv_async_msg(async_md, seconds_in_future))
        {
            if (async_md.event_code == uhd::async_metadata_t::EVENT_CODE_BURST_ACK)
            {
                acks++;
            }
        }
        std::cout << (acks == channel_nums.size() ? "success" : "fail") << std::endl;
    } while (not stop_signal_called and repeat);

    //finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}

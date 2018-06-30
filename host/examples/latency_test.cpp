//
// Copyright 2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/thread.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <complex>

namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args;
    size_t nsamps;
    double rate;
    double rtt;
    size_t nruns;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args",   po::value<std::string>(&args)->default_value(""), "single uhd device address args")
        ("nsamps", po::value<size_t>(&nsamps)->default_value(100),   "number of samples per run")
        ("nruns",  po::value<size_t>(&nruns)->default_value(1000),   "number of tests to perform")
        ("rtt",    po::value<double>(&rtt)->default_value(0.001),    "delay between receive and transmit (seconds)")
        ("rate",   po::value<double>(&rate)->default_value(100e6/4), "sample rate for receive and transmit (sps)")
        ("verbose", "specify to enable inner-loop verbose")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("UHD - Latency Test %s") % desc << std::endl;
        std::cout <<
        "    Latency test receives a packet at time t,\n"
        "    and tries to send a packet at time t + rtt,\n"
        "    where rtt is the round trip time sample time\n"
        "    from device to host and back to the device.\n"
        "    This can be used to test latency between UHD and the device.\n"
        "    If the value rtt is chosen too small, the transmit packet will.\n"
        "    arrive too late at the device indicate an error.\n"
        "    The smallest value of rtt that does not indicate an error is an\n"
        "    approximation for the time it takes for a sample packet to\n"
        "    go to UHD and back to the device."
        << std::endl;
        return EXIT_SUCCESS;
    }

    bool verbose = vm.count("verbose") != 0;

    //create a usrp device
    std::cout << std::endl;
    //std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
    //std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    usrp->set_time_now(uhd::time_spec_t(0.0));

    //set the tx sample rate
    usrp->set_tx_rate(rate);
    std::cout
        << boost::format("Actual TX Rate: %f Msps...")
            % (usrp->get_tx_rate()/1e6)
        << std::endl;

    //set the rx sample rate
    usrp->set_rx_rate(rate);
    std::cout
        << boost::format("Actual RX Rate: %f Msps...")
            % (usrp->get_rx_rate()/1e6)
        << std::endl;

    //allocate a buffer to use
    std::vector<std::complex<float> > buffer(nsamps);

    //create RX and TX streamers
    uhd::stream_args_t stream_args("fc32"); //complex floats
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
    uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);

    //initialize result counts
    int time_error = 0;
    int ack = 0;
    int underflow = 0;
    int other = 0;

    for(size_t nrun = 0; nrun < nruns; nrun++){

        /***************************************************************
         * Issue a stream command some time in the near future
         **************************************************************/
        uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
        stream_cmd.num_samps = buffer.size();
        stream_cmd.stream_now = false;
        stream_cmd.time_spec = usrp->get_time_now() + uhd::time_spec_t(0.01);
        rx_stream->issue_stream_cmd(stream_cmd);

        /***************************************************************
         * Receive the requested packet
         **************************************************************/
        uhd::rx_metadata_t rx_md;
        size_t num_rx_samps = rx_stream->recv(
            &buffer.front(), buffer.size(), rx_md
        );

        if (verbose) {
            std::cout << boost::format(
                "Run %d: Got packet: %u samples, %u full secs, %f frac secs"
                )
                % nrun
                % num_rx_samps
                % rx_md.time_spec.get_full_secs()
                % rx_md.time_spec.get_frac_secs()
            << std::endl;
        } else {
            std::cout << "." << std::flush;
        }

        /***************************************************************
         * Transmit a packet with delta time after received packet
         **************************************************************/
        uhd::tx_metadata_t tx_md;
        tx_md.start_of_burst = true;
        tx_md.end_of_burst = true;
        tx_md.has_time_spec = true;
        tx_md.time_spec = rx_md.time_spec + uhd::time_spec_t(rtt);
        size_t num_tx_samps = tx_stream->send(
            &buffer.front(), buffer.size(), tx_md
        );
        if (verbose) {
            std::cout
                << boost::format("Sent %d samples") % num_tx_samps
                << std::endl;
        }

        /***************************************************************
         * Check the async messages for result
         **************************************************************/
        uhd::async_metadata_t async_md;
        if (not tx_stream->recv_async_msg(async_md)){
            std::cout << boost::format("failed:\n    Async message recv timed out.\n") << std::endl;
            continue;
        }
        switch(async_md.event_code){
        case uhd::async_metadata_t::EVENT_CODE_TIME_ERROR:
            time_error++;
            break;

        case uhd::async_metadata_t::EVENT_CODE_BURST_ACK:
            ack++;
            break;

        case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW:
            underflow++;
            break;

        default:
            std::cerr << boost::format(
                "failed:\n    Got unexpected event code 0x%x.\n"
            ) % async_md.event_code << std::endl;
            other++;
            break;
        }
    }

    while (true) {
        uhd::async_metadata_t async_md;
        if (not tx_stream->recv_async_msg(async_md)) {
            break;
        }
        switch(async_md.event_code){
        case uhd::async_metadata_t::EVENT_CODE_TIME_ERROR:
            time_error++;
            break;

        case uhd::async_metadata_t::EVENT_CODE_BURST_ACK:
            ack++;
            break;

        case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW:
            underflow++;
            break;

        default:
            std::cerr << boost::format(
                "failed:\n    Got unexpected event code 0x%x.\n"
            ) % async_md.event_code << std::endl;
            other++;
            break;
        }
    }
    if (!verbose) {
        std::cout << std::endl;
    }

    /***************************************************************
     * Print the summary
     **************************************************************/
    std::cout << "Summary\n"
              << "================\n"
              << "Number of runs:   " << nruns << std::endl
              << "RTT value tested: " << (rtt*1e3) << " ms" << std::endl
              << "ACKs received:    " << ack << "/" << nruns << std::endl
              << "Underruns:        " << underflow << std::endl
              << "Late packets:     " << time_error << std::endl
              << "Other errors:     " << other << std::endl
              << std::endl;
    return EXIT_SUCCESS;
}

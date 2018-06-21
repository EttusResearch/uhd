//
// Copyright 2014-2016 Ettus Research LLC
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

#include <uhd/types/tune_request.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/device3.hpp>
#include <uhd/rfnoc/radio_ctrl.hpp>
#include <uhd/rfnoc/source_block_ctrl_base.hpp>
#include <uhd/exception.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <fstream>
#include <csignal>
#include <complex>

namespace po = boost::program_options;

static bool stop_signal_called = false;
void sig_int_handler(int){stop_signal_called = true;}

template<typename samp_type> void recv_to_file(
    uhd::rx_streamer::sptr rx_stream,
    const std::string &file,
    const size_t samps_per_buff,
    const double rx_rate,
    const unsigned long long num_requested_samples,
    double time_requested = 0.0,
    bool bw_summary = false,
    bool stats = false,
    bool enable_size_map = false,
    bool continue_on_bad_packet = false
){
    unsigned long long num_total_samps = 0;

    uhd::rx_metadata_t md;
    std::vector<samp_type> buff(samps_per_buff);
    std::ofstream outfile;
    if (not file.empty()) {
        outfile.open(file.c_str(), std::ofstream::binary);
    }
    bool overflow_message = true;

    //setup streaming
    uhd::stream_cmd_t stream_cmd((num_requested_samples == 0)?
        uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS:
        uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE
    );
    stream_cmd.num_samps = size_t(num_requested_samples);
    stream_cmd.stream_now = true;
    stream_cmd.time_spec = uhd::time_spec_t();
    std::cout << "Issueing stream cmd" << std::endl;
    rx_stream->issue_stream_cmd(stream_cmd);
    std::cout << "Done" << std::endl;

    boost::system_time start = boost::get_system_time();
    unsigned long long ticks_requested = (long)(time_requested * (double)boost::posix_time::time_duration::ticks_per_second());
    boost::posix_time::time_duration ticks_diff;
    boost::system_time last_update = start;
    unsigned long long last_update_samps = 0;

    typedef std::map<size_t,size_t> SizeMap;
    SizeMap mapSizes;

    while(not stop_signal_called and (num_requested_samples != num_total_samps or num_requested_samples == 0)) {
        boost::system_time now = boost::get_system_time();

        size_t num_rx_samps = rx_stream->recv(&buff.front(), buff.size(), md, 3.0, enable_size_map);

        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
            std::cout << boost::format("Timeout while streaming") << std::endl;
            break;
        }
        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW){
            if (overflow_message) {
                overflow_message = false;
                std::cerr << boost::format(
                    "Got an overflow indication. Please consider the following:\n"
                    "  Your write medium must sustain a rate of %fMB/s.\n"
                    "  Dropped samples will not be written to the file.\n"
                    "  Please modify this example for your purposes.\n"
                    "  This message will not appear again.\n"
                ) % (rx_rate*sizeof(samp_type)/1e6);
            }
            continue;
        }
        if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE){
            std::string error = str(boost::format("Receiver error: %s") % md.strerror());
            if (continue_on_bad_packet){
                std::cerr << error << std::endl;
                continue;
            }
            else
                throw std::runtime_error(error);
        }

        if (enable_size_map) {
            SizeMap::iterator it = mapSizes.find(num_rx_samps);
            if (it == mapSizes.end())
                mapSizes[num_rx_samps] = 0;
            mapSizes[num_rx_samps] += 1;
        }

        num_total_samps += num_rx_samps;

        if (outfile.is_open()) {
            outfile.write((const char*)&buff.front(), num_rx_samps*sizeof(samp_type));
        }

        if (bw_summary) {
            last_update_samps += num_rx_samps;
            boost::posix_time::time_duration update_diff = now - last_update;
            if (update_diff.ticks() > boost::posix_time::time_duration::ticks_per_second()) {
                double t = (double)update_diff.ticks() / (double)boost::posix_time::time_duration::ticks_per_second();
                double r = (double)last_update_samps / t;
                std::cout << boost::format("\t%f Msps") % (r/1e6) << std::endl;
                last_update_samps = 0;
                last_update = now;
            }
        }

        ticks_diff = now - start;
        if (ticks_requested > 0){
            if ((unsigned long long)ticks_diff.ticks() > ticks_requested)
                break;
        }
    }

    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    std::cout << "Issueing stop stream cmd" << std::endl;
    rx_stream->issue_stream_cmd(stream_cmd);
    std::cout << "Done" << std::endl;

    // Run recv until nothing is left
    int num_post_samps = 0;
    do {
        num_post_samps = rx_stream->recv(&buff.front(), buff.size(), md, 3.0);
    } while(num_post_samps and md.error_code == uhd::rx_metadata_t::ERROR_CODE_NONE);

    if (outfile.is_open())
        outfile.close();

    if (stats) {
        std::cout << std::endl;

        double t = (double)ticks_diff.ticks() / (double)boost::posix_time::time_duration::ticks_per_second();
        std::cout << boost::format("Received %d samples in %f seconds") % num_total_samps % t << std::endl;
        double r = (double)num_total_samps / t;
        std::cout << boost::format("%f Msps") % (r/1e6) << std::endl;

        if (enable_size_map) {
            std::cout << std::endl;
            std::cout << "Packet size map (bytes: count)" << std::endl;
            for (SizeMap::iterator it = mapSizes.begin(); it != mapSizes.end(); it++)
                std::cout << it->first << ":\t" << it->second << std::endl;
        }
    }
}

typedef boost::function<uhd::sensor_value_t (const std::string&)> get_sensor_fn_t;

bool check_locked_sensor(std::vector<std::string> sensor_names, const char* sensor_name, get_sensor_fn_t get_sensor_fn, double setup_time){
    if (std::find(sensor_names.begin(), sensor_names.end(), sensor_name) == sensor_names.end())
        return false;

    boost::system_time start = boost::get_system_time();
    boost::system_time first_lock_time;

    std::cout << boost::format("Waiting for \"%s\": ") % sensor_name;
    std::cout.flush();

    while (true) {
        if ((not first_lock_time.is_not_a_date_time()) and
                (boost::get_system_time() > (first_lock_time + boost::posix_time::seconds(setup_time))))
        {
            std::cout << " locked." << std::endl;
            break;
        }
        if (get_sensor_fn(sensor_name).to_bool()){
            if (first_lock_time.is_not_a_date_time())
                first_lock_time = boost::get_system_time();
            std::cout << "+";
            std::cout.flush();
        }
        else {
            first_lock_time = boost::system_time();	//reset to 'not a date time'

            if (boost::get_system_time() > (start + boost::posix_time::seconds(setup_time))){
                std::cout << std::endl;
                throw std::runtime_error(str(boost::format("timed out waiting for consecutive locks on sensor \"%s\"") % sensor_name));
            }
            std::cout << "_";
            std::cout.flush();
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    }
    std::cout << std::endl;
    return true;
}

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args, file, format, ant, subdev, ref, wirefmt, streamargs, radio_args, block_id, block_args;
    size_t total_num_samps, spb, radio_id, radio_chan;
    double rate, freq, gain, bw, total_time, setup_time;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("file", po::value<std::string>(&file)->default_value("usrp_samples.dat"), "name of the file to write binary samples to")
        ("format", po::value<std::string>(&format)->default_value("sc16"), "File sample format: sc16, fc32, or fc64")
        ("duration", po::value<double>(&total_time)->default_value(0), "total number of seconds to receive")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(0), "total number of samples to receive")
        ("spb", po::value<size_t>(&spb)->default_value(10000), "samples per buffer")
        ("streamargs", po::value<std::string>(&streamargs)->default_value(""), "stream args")
        ("progress", "periodically display short-term bandwidth")
        ("stats", "show average bandwidth on exit")
        ("sizemap", "track packet size and display breakdown on exit")
        ("null", "run without writing to file")
        ("continue", "don't abort on a bad packet")

        ("args", po::value<std::string>(&args)->default_value(""), "USRP device address args")
        ("setup", po::value<double>(&setup_time)->default_value(1.0), "seconds of setup time")

        ("radio-id", po::value<size_t>(&radio_id)->default_value(0), "Radio ID to use (0 or 1).")
        ("radio-chan", po::value<size_t>(&radio_chan)->default_value(0), "Radio channel")
        ("radio-args", po::value<std::string>(&radio_args), "Radio channel")
        ("rate", po::value<double>(&rate)->default_value(1e6), "RX rate of the radio block")
        ("freq", po::value<double>(&freq)->default_value(0.0), "RF center frequency in Hz")
        ("gain", po::value<double>(&gain), "gain for the RF chain")
        ("ant", po::value<std::string>(&ant), "antenna selection")
        ("bw", po::value<double>(&bw), "analog frontend filter bandwidth in Hz")
        ("ref", po::value<std::string>(&ref), "reference source (internal, external, mimo)")
        ("skip-lo", "skip checking LO lock status")
        ("int-n", "tune USRP with integer-N tuning")

        ("block-id", po::value<std::string>(&block_id)->default_value(""), "If block ID is specified, this block is inserted between radio and host.")
        ("block-args", po::value<std::string>(&block_args)->default_value(""), "These args are passed straight to the block.")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")) {
        std::cout << boost::format("UHD/RFNoC RX samples to file %s") % desc << std::endl;
        std::cout
            << std::endl
            << "This application streams data from a single channel of a USRP device to a file.\n"
            << std::endl;
        return ~0;
    }

    bool bw_summary = vm.count("progress") > 0;
    bool stats = vm.count("stats") > 0;
    if (vm.count("null") > 0) {
        file = "";
    }
    bool enable_size_map = vm.count("sizemap") > 0;
    bool continue_on_bad_packet = vm.count("continue") > 0;

    if (enable_size_map) {
        std::cout << "Packet size tracking enabled - will only recv one packet at a time!" << std::endl;
    }

    if (format != "sc16" and format != "fc32" and format != "fc64") {
        std::cout << "Invalid sample format: " << format << std::endl;
        return EXIT_FAILURE;
    }

    /************************************************************************
     * Create device and block controls
     ***********************************************************************/
    std::cout << std::endl;
    std::cout << boost::format("Creating the USRP device with: %s...") % args << std::endl;
    uhd::device3::sptr usrp = uhd::device3::make(args);
    // Create handle for radio object
    uhd::rfnoc::block_id_t radio_ctrl_id(0, "Radio", radio_id);
    // This next line will fail if the radio is not actually available
    uhd::rfnoc::radio_ctrl::sptr radio_ctrl = usrp->get_block_ctrl< uhd::rfnoc::radio_ctrl >(radio_ctrl_id);
    std::cout << "Using radio " << radio_id << ", channel " << radio_chan << std::endl;

    /************************************************************************
     * Set up radio
     ***********************************************************************/
    radio_ctrl->set_args(radio_args);
    if (vm.count("ref")) {
        std::cout << "TODO -- Need to implement API call to set clock source." << std::endl;
        //Lock mboard clocks TODO
        //usrp->set_clock_source(ref);
    }

    //set the sample rate
    if (rate <= 0.0){
        std::cerr << "Please specify a valid sample rate" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rate/1e6) << std::endl;
    radio_ctrl->set_rate(rate);
    std::cout << boost::format("Actual RX Rate: %f Msps...") % (radio_ctrl->get_rate()/1e6) << std::endl << std::endl;

    //set the center frequency
    if (vm.count("freq")) {
        std::cout << boost::format("Setting RX Freq: %f MHz...") % (freq/1e6) << std::endl;
        uhd::tune_request_t tune_request(freq);
        if (vm.count("int-n")) {
            //tune_request.args = uhd::device_addr_t("mode_n=integer"); TODO
        }
        radio_ctrl->set_rx_frequency(freq, radio_chan);
        std::cout << boost::format("Actual RX Freq: %f MHz...") % (radio_ctrl->get_rx_frequency(radio_chan)/1e6) << std::endl << std::endl;
    }

    //set the rf gain
    if (vm.count("gain")) {
        std::cout << boost::format("Setting RX Gain: %f dB...") % gain << std::endl;
        radio_ctrl->set_rx_gain(gain, radio_chan);
        std::cout << boost::format("Actual RX Gain: %f dB...") % radio_ctrl->get_rx_gain(radio_chan) << std::endl << std::endl;
    }

    //set the IF filter bandwidth
    if (vm.count("bw")) {
        //std::cout << boost::format("Setting RX Bandwidth: %f MHz...") % (bw/1e6) << std::endl;
        //radio_ctrl->set_rx_bandwidth(bw, radio_chan); // TODO
        //std::cout << boost::format("Actual RX Bandwidth: %f MHz...") % (radio_ctrl->get_rx_bandwidth(radio_chan)/1e6) << std::endl << std::endl;
    }

    //set the antenna
    if (vm.count("ant")) {
        radio_ctrl->set_rx_antenna(ant, radio_chan);
    }

    boost::this_thread::sleep(boost::posix_time::milliseconds(long(setup_time*1000))); //allow for some setup time


    //check Ref and LO Lock detect
    if (not vm.count("skip-lo")){
        // TODO
        //check_locked_sensor(usrp->get_rx_sensor_names(0), "lo_locked", boost::bind(&uhd::usrp::multi_usrp::get_rx_sensor, usrp, _1, radio_id), setup_time);
        //if (ref == "external")
            //check_locked_sensor(usrp->get_mboard_sensor_names(0), "ref_locked", boost::bind(&uhd::usrp::multi_usrp::get_mboard_sensor, usrp, _1, radio_id), setup_time);
    }

    size_t spp = radio_ctrl->get_arg<int>("spp");

    /************************************************************************
     * Set up streaming
     ***********************************************************************/
    uhd::device_addr_t streamer_args(streamargs);

    uhd::rfnoc::graph::sptr rx_graph = usrp->create_graph("rfnoc_rx_to_file");
    usrp->clear();
    // Set the stream args on the radio:
    if (block_id.empty()) {
        // If no extra block is required, connect to the radio:
        streamer_args["block_id"] = radio_ctrl_id.to_string();
        streamer_args["block_port"] = str(boost::format("%d") % radio_chan);
    } else {
        // Otherwise, see if the requested block exists and connect it to the radio:
        if (not usrp->has_block(block_id)) {
            std::cout << "Block does not exist on current device: " << block_id << std::endl;
            return EXIT_FAILURE;
        }

        uhd::rfnoc::source_block_ctrl_base::sptr blk_ctrl =
            usrp->get_block_ctrl<uhd::rfnoc::source_block_ctrl_base>(block_id);

        if (not block_args.empty()) {
            // Set the block args on the other block:
            blk_ctrl->set_args(uhd::device_addr_t(block_args));
        }
        // Connect:
        std::cout << "Connecting " << radio_ctrl_id << " ==> " << blk_ctrl->get_block_id() << std::endl;
        rx_graph->connect(radio_ctrl_id, radio_chan, blk_ctrl->get_block_id(), uhd::rfnoc::ANY_PORT);
        streamer_args["block_id"] = blk_ctrl->get_block_id().to_string();

        spp = blk_ctrl->get_args().cast<size_t>("spp", spp);
    }

    //create a receive streamer
    std::cout << "Samples per packet: " << spp << std::endl;
    uhd::stream_args_t stream_args(format, "sc16"); // We should read the wire format from the blocks
    stream_args.args = streamer_args;
    stream_args.args["spp"] = boost::lexical_cast<std::string>(spp);
    std::cout << "Using streamer args: " << stream_args.args.to_string() << std::endl;
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    if (total_num_samps == 0) {
        std::signal(SIGINT, &sig_int_handler);
        std::cout << "Press Ctrl + C to stop streaming..." << std::endl;
    }
#define recv_to_file_args() \
    (rx_stream, file, spb, rate, total_num_samps, total_time, bw_summary, stats, enable_size_map, continue_on_bad_packet)
    //recv to file
    if (format == "fc64") recv_to_file<std::complex<double> >recv_to_file_args();
    else if (format == "fc32") recv_to_file<std::complex<float> >recv_to_file_args();
    else if (format == "sc16") recv_to_file<std::complex<short> >recv_to_file_args();
    else throw std::runtime_error("Unknown data format: " + format);

    //finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}

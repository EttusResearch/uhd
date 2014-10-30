//
// Copyright 2014 Ettus Research LLC
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

// This is a simple example for RFNoC apps written in UHD.
// It connects a null source block to any other block on the
// crossbar (provided it has stream-through capabilities)
// and then streams the result to the host, writing it into a file.

#include <iostream>
#include <fstream>
#include <csignal>
#include <complex>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/exception.hpp>
#include <uhd/usrp/rfnoc/block_ctrl.hpp>
#include <uhd/usrp/rfnoc/null_block_ctrl.hpp>

namespace po = boost::program_options;

static bool stop_signal_called = false;
void sig_int_handler(int){stop_signal_called = true;}


template<typename samp_type> void recv_to_file(
    uhd::usrp::multi_usrp::sptr usrp,
    const std::string &cpu_format,
    const std::string &file,
    size_t samps_per_buff,
    unsigned long long num_requested_samples,
    double time_requested = 0.0,
    bool bw_summary = false,
    bool stats = false,
    bool null = false,
    bool continue_on_bad_packet = false
) {
    unsigned long long num_total_samps = 0;
    //create a receive streamer
    uhd::stream_args_t stream_args(cpu_format, "sc16");
    // Note: Any settings in stream_args will trump those
    // previously set!
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    uhd::rx_metadata_t md;
    std::vector<samp_type> buff(samps_per_buff);
    std::ofstream outfile;
    if (not null or file == "stdout")
        outfile.open(file.c_str(), std::ofstream::binary);
    bool overflow_message = true;

    //setup streaming
    uhd::stream_cmd_t stream_cmd((num_requested_samples == 0)?
        uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS:
        uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE
    );
    stream_cmd.num_samps = num_requested_samples;
    stream_cmd.stream_now = true;
    stream_cmd.time_spec = uhd::time_spec_t();
    std::cout << "Issueing start stream cmd" << std::endl;
    // This actually goes to the null source; the processing block
    // should propagate it.
    rx_stream->issue_stream_cmd(stream_cmd);
    std::cout << "Done" << std::endl;

    boost::system_time start = boost::get_system_time();
    unsigned long long ticks_requested = (long)(time_requested * (double)boost::posix_time::time_duration::ticks_per_second());
    boost::posix_time::time_duration ticks_diff;
    boost::system_time last_update = start;
    unsigned long long last_update_samps = 0;

    while(
        not stop_signal_called
        and (num_requested_samples != num_total_samps or num_requested_samples == 0)
    ) {
        boost::system_time now = boost::get_system_time();

        size_t num_rx_samps = rx_stream->recv(&buff.front(), buff.size(), md, 3.0);

        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
            std::cout << boost::format("Timeout while streaming") << std::endl;
            break;
        }
        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW){
            if (overflow_message){
                overflow_message = false;
                std::cerr << boost::format(
                        "Got an overflow indication. Please consider the following:\n"
                        "  Your write medium must sustain a rate of %fMB/s.\n"
                        "  Dropped samples will not be written to the file.\n"
                        "  Please modify this example for your purposes.\n"
                        "  This message will not appear again.\n"
                        ) % (usrp->get_rx_rate()*sizeof(samp_type)/1e6);
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
        num_total_samps += num_rx_samps;

        if (outfile.is_open())
            outfile.write((const char*)&buff.front(), num_rx_samps*sizeof(samp_type));

        if (bw_summary){
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

    if (outfile.is_open())
        outfile.close();

    if (stats){
        std::cout << std::endl;
        double t = (double)ticks_diff.ticks() / (double)boost::posix_time::time_duration::ticks_per_second();
        std::cout << boost::format("Received %d samples in %f seconds") % num_total_samps % t << std::endl;
        double r = (double)num_total_samps / t;
        std::cout << boost::format("%f Msps") % (r/1e6) << std::endl;
    }
}


void pretty_print_flow_graph(std::vector<std::string> blocks)
{
    std::string sep_str = "==>";
    std::cout << std::endl;
    // Line 1
    for (size_t n = 0; n < blocks.size(); n++) {
        const std::string name = blocks[n];
        std::cout << "+";
        for (size_t i = 0; i < name.size() + 2; i++) {
            std::cout << "-";
        }
        std::cout << "+";
        if (n == blocks.size() - 1) {
            break;
        }
        for (size_t i = 0; i < sep_str.size(); i++) {
            std::cout << " ";
        }
    }
    std::cout << std::endl;
    // Line 2
    for (size_t n = 0; n < blocks.size(); n++) {
        const std::string name = blocks[n];
        std::cout << "| " << name << " |";
        if (n == blocks.size() - 1) {
            break;
        }
        std::cout << sep_str;
    }
    std::cout << std::endl;
    // Line 3
    for (size_t n = 0; n < blocks.size(); n++) {
        const std::string name = blocks[n];
        std::cout << "+";
        for (size_t i = 0; i < name.size() + 2; i++) {
            std::cout << "-";
        }
        std::cout << "+";
        if (n == blocks.size() - 1) {
            break;
        }
        for (size_t i = 0; i < sep_str.size(); i++) {
            std::cout << " ";
        }
    }
    std::cout << std::endl << std::endl;
}

///////////////////// MAIN ////////////////////////////////////////////////////
int UHD_SAFE_MAIN(int argc, char *argv[])
{
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args, file, type, nullid, blockid, blockid2;
    size_t total_num_samps, spb, spp;
    size_t num_proc_blocks = 1;
    double rate, total_time, setup_time, block_rate;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value("type=x300"), "multi uhd device address args")
        ("file", po::value<std::string>(&file)->default_value("usrp_samples.dat"), "name of the file to write binary samples to, set to stdout to print")
        ("null", "run without writing to file")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(0), "total number of samples to receive")
        ("time", po::value<double>(&total_time)->default_value(0), "total number of seconds to receive")
        ("spb", po::value<size_t>(&spb)->default_value(10000), "samples per buffer")
        ("spp", po::value<size_t>(&spp)->default_value(64), "samples per packet (on FPGA and wire)")
        ("block_rate", po::value<double>(&block_rate)->default_value(160e6), "The clock rate of the processing block.")
        ("rate", po::value<double>(&rate)->default_value(1e6), "rate at which samples are produced in the null source")
        ("setup", po::value<double>(&setup_time)->default_value(1.0), "seconds of setup time")
        ("type", po::value<std::string>(&type)->default_value("short"), "sample type: double, float, or short")
        ("progress", "periodically display short-term bandwidth")
        ("stats", "show average bandwidth on exit")
        ("continue", "don't abort on a bad packet")
        ("nullid", po::value<std::string>(&nullid)->default_value("0/NullSrcSink_0"), "The block ID for the null source.")
        ("blockid", po::value<std::string>(&blockid)->default_value(""), "The block ID for the processing block.")
        ("blockid2", po::value<std::string>(&blockid2)->default_value(""), "Optional: The block ID for the 2nd processing block.")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout
            << boost::format("[RFNOC] Connect a null source to another (processing) block, and stream the result to file %s.") % desc
            << std::endl;
        return ~0;
    }

    bool bw_summary = vm.count("progress") > 0;
    bool stats = vm.count("stats") > 0;
    bool null = vm.count("null") > 0 or vm.count("file") == 0;
    bool continue_on_bad_packet = vm.count("continue") > 0;

    // Check settings
    if (not uhd::rfnoc::block_id_t::is_valid_block_id(nullid)) {
        std::cout << "Must specify a valid block ID for the null source." << std::endl;
        return ~0;
    }
    if (not uhd::rfnoc::block_id_t::is_valid_block_id(blockid)) {
        std::cout << "Must specify a valid block ID for the processing block." << std::endl;
        return ~0;
    }
    if (not blockid2.empty()) {
        if (not uhd::rfnoc::block_id_t::is_valid_block_id(blockid2)) {
            std::cout << "Invalid block ID for the 2nd processing block." << std::endl;
            return ~0;
        }
        num_proc_blocks = 2;
    }

    // Set up SIGINT handler. For indefinite streaming, display info on how to stop.
    std::signal(SIGINT, &sig_int_handler);
    if (total_num_samps == 0) {
        std::cout << "Press Ctrl + C to stop streaming..." << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////
    //////// 1. Setup a USRP device /////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    std::cout << std::endl;
    std::cout << boost::format("Creating the USRP device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

    // Check it's a Gen-3 device
    if (not usrp->is_device3()) {
        std::cout << "This example only works with generation-3 devices running RFNoC." << std::endl;
        return ~0;
    }
    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;
    boost::this_thread::sleep(boost::posix_time::seconds(setup_time)); //allow for some setup time
    // Reset device streaming state
    usrp->get_device3()->clear();

    /////////////////////////////////////////////////////////////////////////
    //////// 2. Get block control objects ///////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    // For the processing blocks, we don't care what type these block is,
    // so we make it a block_ctrl_base (default):
    uhd::rfnoc::block_ctrl_base::sptr proc_block_ctrl = usrp->get_device3()->find_block_ctrl(blockid);
    uhd::rfnoc::block_ctrl_base::sptr proc_block_ctrl2;
    if (num_proc_blocks == 2) {
        proc_block_ctrl2 = usrp->get_device3()->find_block_ctrl(blockid2);
    }

    // For the null source control, we want to use the subclassed access,
    // so we create a null_block_ctrl:
    uhd::rfnoc::null_block_ctrl::sptr null_src_ctrl = usrp->get_device3()->find_block_ctrl<uhd::rfnoc::null_block_ctrl>(nullid);

    std::vector<std::string> blocks;
    blocks.push_back(null_src_ctrl->get_block_id());
    blocks.push_back(proc_block_ctrl->get_block_id());
    if (num_proc_blocks == 2) {
        blocks.push_back(proc_block_ctrl2->get_block_id());
    }
    blocks.push_back("HOST");
    pretty_print_flow_graph(blocks);

    /////////////////////////////////////////////////////////////////////////
    //////// 3. Set channel definitions /////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    // Here, we define that there is only 1 channel, and it points
    // to the final processing block.
    usrp->clear_channels(); // The default is to use the radios. Let's not do that.
    if (num_proc_blocks == 2) {
        usrp->set_channel(proc_block_ctrl2->get_block_id()); // Defaults to being channel 0.
    } else {
        usrp->set_channel(proc_block_ctrl->get_block_id()); // Defaults to being channel 0.
    }

    /////////////////////////////////////////////////////////////////////////
    //////// 4. Configure blocks (packet size and rate) /////////////////////
    /////////////////////////////////////////////////////////////////////////
    std::cout << "Samples per packet coming from null source: " << spp << std::endl;
    // This could be made a setter in the null block control, but we can always
    // change these kind of things by changing the output signature of a block:
    uhd::rfnoc::stream_sig_t out_sig = null_src_ctrl->get_output_signature(0);
    const size_t BYTES_PER_SAMPLE = 4;
    out_sig.packet_size = spp * BYTES_PER_SAMPLE;
    if (not null_src_ctrl->set_output_signature(out_sig)) {
        std::cout << "[ERROR] Could not set samples per packet!" << std::endl;
        return ~0;
    }

    // To access properties, there's two ways. Either, you can directly
    // call setters and getters:
    std::cout << str(boost::format("Requesting rate:   %.2f Msps (%.2f MByte/s).") % (rate / 1e6) % (rate * 4 / 1e6)) << std::endl;
    const size_t SAMPLES_PER_LINE = 2;
    // This assumes the clock rate is 166.6 MHz
    null_src_ctrl->set_line_rate(rate / SAMPLES_PER_LINE, 166.6e6);
    // Now, it's possible that this requested rate is not available.
    // We could go ahead and read the cycle delay from the property tree.
    //
    // But let's use the getter here:
    double actual_rate_mega = null_src_ctrl->set_line_rate(rate / SAMPLES_PER_LINE, 166.6e6) / 1e6 * 2;
    std::cout << str(boost::format("Actually got rate: %.2f Msps (%.2f MByte/s).") % actual_rate_mega % (actual_rate_mega * 4)) << std::endl;

    /////////////////////////////////////////////////////////////////////////
    //////// 5. Connect blocks //////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    std::cout << "Connecting blocks..." << std::endl;
    usrp->connect( // Yes, it's that easy!
            null_src_ctrl->get_block_id(),
            proc_block_ctrl->get_block_id()
    );
    if (num_proc_blocks == 2) {
        usrp->connect(
            proc_block_ctrl->get_block_id(),
            proc_block_ctrl2->get_block_id()
        );
    }

    /////////////////////////////////////////////////////////////////////////
    //////// 6. Spawn receiver //////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
#define recv_to_file_args(format) \
        (usrp, format, file, spb, total_num_samps, total_time, bw_summary, stats, null, continue_on_bad_packet)
    //recv to file
    if (type == "double") recv_to_file<std::complex<double> >recv_to_file_args("fc64");
    else if (type == "float") recv_to_file<std::complex<float> >recv_to_file_args("fc32");
    else if (type == "short") recv_to_file<std::complex<short> >recv_to_file_args("sc16");
    else throw std::runtime_error("Unknown type " + type);

    // Finished!
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}
// vim: sw=4 expandtab:

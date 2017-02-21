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

#include <uhd/device3.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/exception.hpp>
#include <uhd/rfnoc/block_ctrl.hpp>
#include <uhd/rfnoc/null_block_ctrl.hpp>
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
    size_t samps_per_buff,
    unsigned long long num_requested_samples,
    double time_requested = 0.0,
    bool bw_summary = false,
    bool stats = false,
    bool continue_on_bad_packet = false
) {
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
    stream_cmd.num_samps = num_requested_samples;
    stream_cmd.stream_now = true;
    stream_cmd.time_spec = uhd::time_spec_t();
    std::cout << "Issuing start stream cmd" << std::endl;
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
                std::cerr << "Got an overflow indication. If writing to disk, your\n"
                             "write medium may not be able to keep up.\n";
            }
            continue;
        }
        if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE){
            std::string error = str(boost::format("Receiver error: %s") % md.strerror());
            if (continue_on_bad_packet){
                std::cerr << error << std::endl;
                continue;
            }
            else {
                throw std::runtime_error(error);
            }
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
    std::cout << "Issuing stop stream cmd" << std::endl;
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
    std::string args, file, format, nullid, blockid, blockid2;
    size_t total_num_samps, spb, spp;
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
        ("format", po::value<std::string>(&format)->default_value("sc16"), "File sample type: sc16, fc32, or fc64")
        ("progress", "periodically display short-term bandwidth")
        ("stats", "show average bandwidth on exit")
        ("continue", "don't abort on a bad packet")
        ("nullid", po::value<std::string>(&nullid)->default_value("0/NullSrcSink_0"), "The block ID for the null source.")
        ("blockid", po::value<std::string>(&blockid)->default_value("FIFO"), "The block ID for the processing block.")
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
    uhd::device3::sptr usrp = uhd::device3::make(args);

    boost::this_thread::sleep(boost::posix_time::seconds(setup_time)); //allow for some setup time
    // Reset device streaming state
    usrp->clear();
    uhd::rfnoc::graph::sptr rx_graph = usrp->create_graph("rx_graph");

    /////////////////////////////////////////////////////////////////////////
    //////// 2. Get block control objects ///////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    std::vector<std::string> blocks;

    // For the null source control, we want to use the subclassed access,
    // so we create a null_block_ctrl:
    uhd::rfnoc::null_block_ctrl::sptr null_src_ctrl;
    if (usrp->has_block<uhd::rfnoc::null_block_ctrl>(nullid)) {
        null_src_ctrl = usrp->get_block_ctrl<uhd::rfnoc::null_block_ctrl>(nullid);
        blocks.push_back(null_src_ctrl->get_block_id());
    } else {
        std::cout << "Error: Device has no null block." << std::endl;
        return ~0;
    }

    // For the processing blocks, we don't care what type the block is,
    // so we make it a block_ctrl_base (default):
    uhd::rfnoc::block_ctrl_base::sptr proc_block_ctrl, proc_block_ctrl2;
    if (usrp->has_block(blockid)) {
        proc_block_ctrl = usrp->get_block_ctrl(blockid);
        blocks.push_back(proc_block_ctrl->get_block_id());
    }
    if (not blockid2.empty() and usrp->has_block(blockid2)) {
        proc_block_ctrl2 = usrp->get_block_ctrl(blockid2);
        blocks.push_back(proc_block_ctrl2->get_block_id());
    }

    blocks.push_back("HOST");
    pretty_print_flow_graph(blocks);

    /////////////////////////////////////////////////////////////////////////
    //////// 3. Set channel definitions /////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    uhd::device_addr_t stream_args_args;
    //
    // Here, we define that there is only 1 channel, and it points
    // to the final processing block.
    if (proc_block_ctrl2 and proc_block_ctrl) {
        stream_args_args["block_id"] = blockid2;
    } else if (proc_block_ctrl) {
        stream_args_args["block_id"] = blockid;
    } else {
        stream_args_args["block_id"] = nullid;
    }

    /////////////////////////////////////////////////////////////////////////
    //////// 4. Configure blocks (packet size and rate) /////////////////////
    /////////////////////////////////////////////////////////////////////////
    std::cout << "Samples per packet coming from null source: " << spp << std::endl;
    // To access properties, there's two ways. You can access args as defined
    // in the XML file:
    const size_t BYTES_PER_SAMPLE = 4;
    null_src_ctrl->set_arg<int>("bpp", int(spp * BYTES_PER_SAMPLE));
    if (null_src_ctrl->get_arg<int>("bpp") != int(spp * BYTES_PER_SAMPLE)) {
        std::cout << "[ERROR] Could not set samples per packet!" << std::endl;
        return ~0;
    }

    // Or, if our block has its own getters + setters, you can call those:
    std::cout << str(boost::format("Requesting rate:   %.2f Msps (%.2f MByte/s).") % (rate / 1e6) % (rate * 4 / 1e6)) << std::endl;
    const size_t SAMPLES_PER_LINE = 2;
    null_src_ctrl->set_line_rate(rate / SAMPLES_PER_LINE, block_rate);
    // Now, it's possible that this requested rate is not available.
    // Let's read back the true rate with the getter:
    double actual_rate_mega = null_src_ctrl->get_line_rate(block_rate) / 1e6 * SAMPLES_PER_LINE;
    std::cout
        << str(
                boost::format("Actually got rate: %.2f Msps (%.2f MByte/s).")
                % actual_rate_mega % (actual_rate_mega * BYTES_PER_SAMPLE)
           )
        << std::endl;


    /////////////////////////////////////////////////////////////////////////
    //////// 5. Connect blocks //////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    std::cout << "Connecting blocks..." << std::endl;
    if (proc_block_ctrl) {
        rx_graph->connect( // Yes, it's that easy!
                null_src_ctrl->get_block_id(),
                proc_block_ctrl->get_block_id()
        );
    }
    if (proc_block_ctrl2 and proc_block_ctrl) {
        rx_graph->connect(
            proc_block_ctrl->get_block_id(),
            proc_block_ctrl2->get_block_id()
        );
    }

    /////////////////////////////////////////////////////////////////////////
    //////// 6. Spawn receiver //////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    uhd::stream_args_t stream_args(format, "sc16");
    stream_args.args = stream_args_args;
    stream_args.args["spp"] = boost::lexical_cast<std::string>(spp);
    UHD_LOGGER_DEBUG("RFNOC") << "Using streamer args: " << stream_args.args.to_string() << std::endl;
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

#define recv_to_file_args() \
        (rx_stream, file, spb, total_num_samps, total_time, bw_summary, stats, continue_on_bad_packet)
    //recv to file
    if (format == "fc64") recv_to_file<std::complex<double> >recv_to_file_args();
    else if (format == "fc32") recv_to_file<std::complex<float> >recv_to_file_args();
    else if (format == "sc16") recv_to_file<std::complex<short> >recv_to_file_args();
    else throw std::runtime_error("Unknown type sample type: " + format);

    // Finished!
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}
// vim: sw=4 expandtab:

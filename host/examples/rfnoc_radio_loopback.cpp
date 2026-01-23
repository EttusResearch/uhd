//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// Example UHD/RFNoC application: Connect an rx radio to a tx radio and
// run a loopback.

#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc/mb_controller.hpp>
#include <uhd/rfnoc/radio_control.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/utils/graph_utils.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/safe_main.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

namespace po = boost::program_options;
using uhd::rfnoc::radio_control;
using namespace std::chrono_literals;

/****************************************************************************
 * SIGINT handling
 ***************************************************************************/
static bool stop_signal_called = false;
void sig_int_handler(int)
{
    stop_signal_called = true;
}

/****************************************************************************
 * main
 ***************************************************************************/
int UHD_SAFE_MAIN(int argc, char* argv[])
{
    const std::string program_doc =
        "usage: rfnoc_radio_loopback [-h] [--args ARGS] [--spp SPP]\n"
        "                            [--rx-freq RX_FREQ] [--tx-freq TX_FREQ]\n"
        "                            [--rx-gain RX_GAIN] [--tx-gain TX_GAIN]\n"
        "                            [--rx-ant RX_ANT] [--tx-ant TX_ANT]\n"
        "                            [--rx-blockid RX_BLOCKID]\n"
        "                            [--tx-blockid TX_BLOCKID]\n"
        "                            [--rx-chan RX_CHAN] [--tx-chan TX_CHAN]\n"
        "                            [--rx-bw RX_BW] [--tx-bw TX_BW]\n"
        "                            [--rx-timestamps RX_TIMESTAMPS]\n"
        "                            [--setup SETUP] [--nsamps NSAMPS]\n"
        "                            [-r RATE] [--duration DURATION] [--int-n]\n"
        "                            [--ref {internal,external,mimo,gpsdo}]\n"
        "                            [--pps {internal,external,mimo,gpsdo}]\n"
        "                            [--dot]"
        "\n\n"
        "This example streams baseband data from a selected RFNoC RX radio block\n"
        "and channel to a TX radio block and channel.\n"
        "The connection is dynamically established between the RX radio and TX\n"
        "radio; if DDC and DUC blocks are present, data is routed through them\n"
        "for digital down/up conversion. Streaming can be performed for a\n"
        "user-defined duration or continuously.\n"
        "\n"
        "Key features:\n"
        "- Dynamically connects RX radio (or DDC) to TX radio (or DUC) using\n"
        "  RFNoC.\n"
        "- Flexible selection of radio blocks and channels.\n"
        "- Configurable sample rate, frequency, gain, bandwidth, and antenna for\n"
        "  both RX and TX.\n"
        "- Shows how to enable timestamping of RX packets.\n"
        "\n"
        "Usage example:\n"
        "Receive a signal at 3.5 GHz at re-transmit it at 2.4 GHz using the\n"
        "default radio sampling rate and using two different RFNoC radio blocks:\n"
        "  rfnoc_radio_loopback --args \"addr=192.168.10.2\"\n"
        "                       --rx-blockid \"0/Radio#0\" --tx-blockid \"0/Radio#1\"";
    // variables to be set by po
    std::string args, rx_ant, tx_ant, rx_blockid, tx_blockid, ref, pps;
    size_t total_num_samps, spp, rx_chan, tx_chan;
    double rate, rx_freq, tx_freq, rx_gain, tx_gain, rx_bw, tx_bw, total_time, setup_time;
    bool rx_timestamps;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help,h", "Show this help message and exit.")
        ("args", po::value<std::string>(&args)->default_value(""), "Single USRP device selection and "
            "configuration arguments."
            "\nSpecify key-value pairs (e.g., addr, serial, type, master_clock_rate) separated by commas."
            "\nSee the UHD manual for model-specific options."
            "\nExamples:"
            "\n  --args \"addr=192.168.10.2\""
            "\n  --args \"addr=192.168.10.2,master_clock_rate=200e6\""
            "\nIf not specified, UHD connects to the first available device.")
        ("spp", po::value<size_t>(&spp)->default_value(0), "Specifies the number of samples per packet sent "
            "from the RFNoC radio block. Use smaller values to reduce latency.")
        ("rx-freq", po::value<double>(&rx_freq)->default_value(0.0), "RX RF center frequency in Hz.")
        ("tx-freq", po::value<double>(&tx_freq)->default_value(0.0), "TX RF center frequency in Hz.")
        ("rx-gain", po::value<double>(&rx_gain)->default_value(0.0), "RX gain for the RF chain in dB.")
        ("tx-gain", po::value<double>(&tx_gain)->default_value(0.0), "TX gain for the RF chain in dB.")
        ("rx-ant", po::value<std::string>(&rx_ant), "RX antenna port selection string selecting a specific "
            "antenna port for USRP daughterboards having multiple antenna connectors per RF channel."
            "\nExample: --ant \"TX/RX\"")
        ("tx-ant", po::value<std::string>(&tx_ant), "TX antenna port selection string selecting a specific "
            "antenna port for USRP daughterboards having multiple antenna connectors per RF channel."
            "\nExample: --ant \"TX/RX\"")
        ("rx-blockid", po::value<std::string>(&rx_blockid)->default_value("0/Radio#0"), "Specifies the RFNoC "
            "radio block ID to use for receiving. Format is typically \"mboard_index/Radio#N\" (e.g., 0/Radio#0).")
        ("tx-blockid", po::value<std::string>(&tx_blockid)->default_value("0/Radio#1"), "Specifies the RFNoC "
            "radio block ID to use for transmitting. Format is typically \"mboard_index/Radio#N\" (e.g., 0/Radio#1).")
        ("rx-chan", po::value<size_t>(&rx_chan)->default_value(0), "Selects the channel index within the "
            "chosen RX radio block for data reception (e.g., 0 for the first channel).")
        ("tx-chan", po::value<size_t>(&tx_chan)->default_value(0), "Selects the channel index within the "
            "chosen TX radio block for data transmission (e.g., 0 for the first channel).")
        ("rx-bw", po::value<double>(&rx_bw), "Sets the analog frontend filter bandwidth for the RX path in "
            "Hz. Not all USRP devices support programmable bandwidth; if an unsupported value is requested, the device "
            "will use the nearest supported bandwidth instead.")
        ("tx-bw", po::value<double>(&tx_bw), "Sets the analog frontend filter bandwidth for the TX path in "
            "Hz. Not all USRP devices support programmable bandwidth; if an unsupported value is requested, the device "
            "will use the nearest supported bandwidth instead.")
        ("rx-timestamps", po::value<bool>(&rx_timestamps)->default_value(false), "Enables timestamping on "
            "received packets, associating each sample with a precise time value.")
        ("setup", po::value<double>(&setup_time)->default_value(0.1), "Sets the amount of time (in seconds) "
            "the program waits for hardware locks (e.g. LO) to ensure the device is properly synchronized before starting "
            "reception.")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(0), "Total number of samples to "
            "receive. The program stops when this number is reached.")
        ("rate,r", po::value<double>(&rate)->default_value(0.0), "Sets the sample rate for both RX and TX "
            "radio blocks. The specified rate must be supported by the selected radio; supported values differ between "
            "USRP models. Refer to the UHD manual for valid sample rates for your hardware.")
        ("duration", po::value<double>(&total_time)->default_value(0), "Total number of seconds to receive. "
            "The program stops when this number is reached.")
        ("int-n", "Use integer-N tuning for USRP RF synthesizers. With this mode, the LO can only be tuned in "
            "discrete steps, which are integer multiples of the reference frequency. This mode can improve phase noise "
            "and spurious performance at the cost of coarser frequency resolution.")
        ("ref", po::value<std::string>(&ref), "Sets the source for the frequency reference. Available values "
            "depend on the USRP model. Typical values are 'internal', 'external', 'mimo', and 'gpsdo'.")
        ("pps", po::value<std::string>(&pps), "Specifies the PPS source for time synchronization. Available "
            "values depend on the USRP model. Typical values are 'internal', 'external', 'mimo', and 'gpsdo'.")
        ("dot", "Outputs the RFNoC graph as a DOT-format representation, showing the connections between "
            "blocks for visualization or analysis.")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    // print the help message
    if (vm.count("help")) {
        std::cout << program_doc << std::endl;
        std::cout << desc << std::endl;
        return ~0;
    }
    po::notify(vm); // only called if --help was not requested

    /************************************************************************
     * Create device and block controls
     ***********************************************************************/
    std::cout << std::endl;
    std::cout << boost::format("Creating the RFNoC graph with args: %s...") % args
              << std::endl;
    uhd::rfnoc::rfnoc_graph::sptr graph = uhd::rfnoc::rfnoc_graph::make(args);

    // Create handles for radio objects
    uhd::rfnoc::block_id_t rx_radio_ctrl_id(rx_blockid);
    uhd::rfnoc::block_id_t tx_radio_ctrl_id(tx_blockid);
    // This next line will fail if the radio is not actually available
    uhd::rfnoc::radio_control::sptr rx_radio_ctrl =
        graph->get_block<uhd::rfnoc::radio_control>(rx_radio_ctrl_id);
    uhd::rfnoc::radio_control::sptr tx_radio_ctrl =
        graph->get_block<uhd::rfnoc::radio_control>(tx_radio_ctrl_id);
    std::cout << "Using RX radio " << rx_radio_ctrl_id << ", channel " << rx_chan
              << std::endl;
    std::cout << "Using TX radio " << tx_radio_ctrl_id << ", channel " << tx_chan
              << std::endl;
    size_t rx_mb_idx = rx_radio_ctrl_id.get_device_no();

    /************************************************************************
     * Set up radio
     ***********************************************************************/
    // Only forward properties once per block in the chain. In the case of
    // looping back to a single radio block, skip property propagation after
    // traversing back to the starting point of the chain.
    const bool skip_pp = rx_radio_ctrl_id == tx_radio_ctrl_id;
    // Connect the RX radio to the TX radio
    uhd::rfnoc::connect_through_blocks(
        graph, rx_radio_ctrl_id, rx_chan, tx_radio_ctrl_id, tx_chan, skip_pp);
    graph->commit();
    std::cout << "Active connections:" << std::endl;
    if (vm.count("dot")) {
        std::cout << graph->to_dot() << std::endl;
    } else {
        for (auto& edge : graph->enumerate_active_connections()) {
            std::cout << "* " << edge.to_string() << std::endl;
        }
    }

    rx_radio_ctrl->enable_rx_timestamps(rx_timestamps, rx_chan);

    // Set time and clock reference
    if (vm.count("ref") && vm.count("pps")) {
        for (size_t i = 0; i < graph->get_num_mboards(); ++i) {
            graph->get_mb_controller(i)->set_sync_source(ref, pps);
        }
    } else if (vm.count("ref")) {
        // Lock mboard clocks
        for (size_t i = 0; i < graph->get_num_mboards(); ++i) {
            graph->get_mb_controller(i)->set_clock_source(ref);
        }
    } else if (vm.count("pps")) {
        // Lock mboard clocks
        for (size_t i = 0; i < graph->get_num_mboards(); ++i) {
            graph->get_mb_controller(i)->set_time_source(pps);
        }
    }

    // set the sample rate
    if (rate <= 0.0) {
        rate = rx_radio_ctrl->get_rate();
    } else {
        std::cout << boost::format("Setting RX Rate: %f Msps...") % (rate / 1e6)
                  << std::endl;
        rate = rx_radio_ctrl->set_rate(rate);
        std::cout << boost::format("Actual RX Rate: %f Msps...") % (rate / 1e6)
                  << std::endl
                  << std::endl;
    }
    std::cout << "Actual Sample Rate: " << (rate / 1e6) << " Msps..." << std::endl
              << std::endl;

    // set the center frequency
    if (vm.count("rx-freq")) {
        std::cout << boost::format("Setting RX Freq: %f MHz...") % (rx_freq / 1e6)
                  << std::endl;
        uhd::tune_request_t tune_request(rx_freq);
        if (vm.count("int-n")) {
            tune_request.args = uhd::device_addr_t("mode_n=integer");
        }
        rx_radio_ctrl->set_rx_frequency(rx_freq, rx_chan);
        std::cout << boost::format("Actual RX Freq: %f MHz...")
                         % (rx_radio_ctrl->get_rx_frequency(rx_chan) / 1e6)
                  << std::endl
                  << std::endl;
    }
    if (vm.count("tx-freq")) {
        std::cout << boost::format("Setting TX Freq: %f MHz...") % (tx_freq / 1e6)
                  << std::endl;
        uhd::tune_request_t tune_request(tx_freq);
        if (vm.count("int-n")) {
            tune_request.args = uhd::device_addr_t("mode_n=integer");
        }
        tx_radio_ctrl->set_tx_frequency(tx_freq, tx_chan);
        std::cout << boost::format("Actual TX Freq: %f MHz...")
                         % (tx_radio_ctrl->get_tx_frequency(tx_chan) / 1e6)
                  << std::endl
                  << std::endl;
    }

    // set the rf gain
    if (vm.count("rx-gain")) {
        std::cout << boost::format("Setting RX Gain: %f dB...") % rx_gain << std::endl;
        rx_radio_ctrl->set_rx_gain(rx_gain, rx_chan);
        std::cout << boost::format("Actual RX Gain: %f dB...")
                         % rx_radio_ctrl->get_rx_gain(rx_chan)
                  << std::endl
                  << std::endl;
    }
    if (vm.count("tx-gain")) {
        std::cout << boost::format("Setting TX Gain: %f dB...") % tx_gain << std::endl;
        tx_radio_ctrl->set_tx_gain(tx_gain, tx_chan);
        std::cout << boost::format("Actual TX Gain: %f dB...")
                         % tx_radio_ctrl->get_tx_gain(tx_chan)
                  << std::endl
                  << std::endl;
    }

    // set the IF filter bandwidth
    if (vm.count("rx-bw")) {
        std::cout << boost::format("Setting RX Bandwidth: %f MHz...") % (rx_bw / 1e6)
                  << std::endl;
        rx_radio_ctrl->set_rx_bandwidth(rx_bw, rx_chan);
        std::cout << boost::format("Actual RX Bandwidth: %f MHz...")
                         % (rx_radio_ctrl->get_rx_bandwidth(rx_chan) / 1e6)
                  << std::endl
                  << std::endl;
    }
    if (vm.count("tx-bw")) {
        std::cout << boost::format("Setting TX Bandwidth: %f MHz...") % (tx_bw / 1e6)
                  << std::endl;
        tx_radio_ctrl->set_tx_bandwidth(tx_bw, tx_chan);
        std::cout << boost::format("Actual TX Bandwidth: %f MHz...")
                         % (tx_radio_ctrl->get_tx_bandwidth(tx_chan) / 1e6)
                  << std::endl
                  << std::endl;
    }

    // set the antennas
    if (vm.count("rx-ant")) {
        rx_radio_ctrl->set_rx_antenna(rx_ant, rx_chan);
    }
    if (vm.count("tx-ant")) {
        tx_radio_ctrl->set_tx_antenna(tx_ant, tx_chan);
    }

    // check Ref and LO Lock detect
    if (not vm.count("skip-lo")) {
        // TODO
        // check_locked_sensor(usrp->get_rx_sensor_names(0), "lo_locked",
        // boost::bind(&uhd::usrp::multi_usrp::get_rx_sensor, usrp, _1, radio_id),
        // setup_time); if (ref == "external")
        // check_locked_sensor(usrp->get_mboard_sensor_names(0), "ref_locked",
        // boost::bind(&uhd::usrp::multi_usrp::get_mboard_sensor, usrp, _1, radio_id),
        // setup_time);
    }

    if (vm.count("spp")) {
        std::cout << "Setting samples per packet to: " << spp << std::endl;
        rx_radio_ctrl->set_property<int>("spp", spp, 0);
        spp = rx_radio_ctrl->get_property<int>("spp", 0);
        std::cout << "Actual samples per packet = " << spp << std::endl;
    }

    // Allow for some setup time
    std::this_thread::sleep_for(1s * setup_time);

    // Arm SIGINT handler
    std::signal(SIGINT, &sig_int_handler);

    // Calculate timeout and set timers
    // We just need to check is nsamps was set, otherwise we'll use the duration
    if (total_num_samps > 0) {
        total_time = total_num_samps / rate;
        std::cout << boost::format("Expected streaming time: %.3f") % total_time
                  << std::endl;
    }

    // Start streaming
    uhd::stream_cmd_t stream_cmd((total_num_samps == 0)
                                     ? uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS
                                     : uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = size_t(total_num_samps);
    stream_cmd.stream_now = false;
    stream_cmd.time_spec =
        graph->get_mb_controller(rx_mb_idx)->get_timekeeper(rx_mb_idx)->get_time_now()
        + setup_time;
    std::cout << "Issuing start stream cmd..." << std::endl;
    rx_radio_ctrl->issue_stream_cmd(stream_cmd, rx_chan);
    std::cout << "Wait..." << std::endl;

    // Wait until we can exit
    uhd::time_spec_t elapsed_time = 0.0;
    while (not stop_signal_called) {
        std::this_thread::sleep_for(100ms);
        if (total_time > 0.0) {
            elapsed_time += 0.1;
            if (elapsed_time > total_time) {
                break;
            }
        }
    }

    // Stop radio
    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    std::cout << "Issuing stop stream cmd..." << std::endl;
    rx_radio_ctrl->issue_stream_cmd(stream_cmd, rx_chan);
    std::cout << "Done" << std::endl;
    // Allow for the samples and ACKs to propagate
    std::this_thread::sleep_for(100ms);

    return EXIT_SUCCESS;
}

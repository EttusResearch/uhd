//
// Copyright 2011-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/convert.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>
#include <atomic>
#include <chrono>
#include <complex>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace po = boost::program_options;
using namespace std::chrono_literals;

namespace {
constexpr auto CLOCK_TIMEOUT = 1000ms; // 1000mS timeout for external clock locking
} // namespace

using start_time_type = std::chrono::time_point<std::chrono::steady_clock>;

/***********************************************************************
 * Test result variables
 **********************************************************************/
std::atomic_ullong num_overruns{0};
std::atomic_ullong num_underruns{0};
std::atomic_ullong num_rx_samps{0};
std::atomic_ullong num_tx_samps{0};
std::atomic_ullong num_dropped_samps{0};
std::atomic_ullong num_seq_errors{0};
std::atomic_ullong num_seqrx_errors{0}; // "D"s
std::atomic_ullong num_late_commands{0};
std::atomic_ullong num_timeouts_rx{0};
std::atomic_ullong num_timeouts_tx{0};

inline auto time_delta(const start_time_type& ref_time)
{
    return std::chrono::steady_clock::now() - ref_time;
}

inline std::string time_delta_str(const start_time_type& ref_time)
{
    const auto delta   = time_delta(ref_time);
    const auto hours   = std::chrono::duration_cast<std::chrono::hours>(delta);
    const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(delta - hours);
    const auto seconds =
        std::chrono::duration_cast<std::chrono::seconds>(delta - hours - minutes);
    const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(
        delta - hours - minutes - seconds);

    return str(boost::format("%02d:%02d:%02d.%06d") % hours.count() % minutes.count()
               % seconds.count() % nanoseconds.count());
}

#define NOW() (time_delta_str(start_time))

/***********************************************************************
 * Benchmark RX Rate
 **********************************************************************/
void benchmark_rx_rate(uhd::usrp::multi_usrp::sptr usrp,
    const std::string& rx_cpu,
    uhd::rx_streamer::sptr rx_stream,
    size_t spb,
    bool random_nsamps,
    const start_time_type& start_time,
    std::atomic<bool>& burst_timer_elapsed,
    bool elevate_priority,
    double adjusted_rx_delay,
    double user_rx_delay,
    bool rx_stream_now)
{
    if (elevate_priority) {
        uhd::set_thread_priority_safe();
    }

    // print pre-test summary
    auto time_stamp   = NOW();
    auto rx_rate      = usrp->get_rx_rate() / 1e6;
    auto num_channels = rx_stream->get_num_channels();
    std::cout << boost::format("[%s] Testing receive rate %f Msps on %u channels\n")
                     % time_stamp % rx_rate % num_channels;

    // setup variables and allocate buffer
    uhd::rx_metadata_t md;
    if (spb == 0) {
        spb = rx_stream->get_max_num_samps();
    }
    std::vector<char> buff(spb * uhd::convert::get_bytes_per_item(rx_cpu));
    std::vector<void*> buffs;
    for (size_t ch = 0; ch < rx_stream->get_num_channels(); ch++)
        buffs.push_back(&buff.front()); // same buffer for each channel
    bool had_an_overflow = false;
    uhd::time_spec_t last_time;
    const double rate = usrp->get_rx_rate();

    uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    cmd.num_samps = spb;
    if (random_nsamps) {
        cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE;
        cmd.num_samps   = (rand() % spb) + 1;
    }

    cmd.time_spec  = usrp->get_time_now() + uhd::time_spec_t(adjusted_rx_delay);
    cmd.stream_now = rx_stream_now;
    rx_stream->issue_stream_cmd(cmd);

    const float burst_pkt_time = std::max<float>(0.100f, (2 * spb / rate));
    float recv_timeout         = burst_pkt_time + (adjusted_rx_delay);

    bool stop_called = false;
    while (true) {
        if (burst_timer_elapsed and not stop_called) {
            rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
            stop_called = true;
        }
        if (random_nsamps) {
            cmd.time_spec = usrp->get_time_now() + uhd::time_spec_t(user_rx_delay);
            cmd.num_samps = (rand() % spb) + 1;
            rx_stream->issue_stream_cmd(cmd);
        }
        try {
            num_rx_samps += rx_stream->recv(buffs, cmd.num_samps, md, recv_timeout)
                            * rx_stream->get_num_channels();
            recv_timeout = burst_pkt_time;
        } catch (uhd::io_error& e) {
            std::cerr << "[" << NOW() << "] Caught an IO exception. " << std::endl;
            std::cerr << e.what() << std::endl;
            return;
        }

        // handle the error codes
        switch (md.error_code) {
            case uhd::rx_metadata_t::ERROR_CODE_NONE:
                if (had_an_overflow) {
                    had_an_overflow          = false;
                    const long dropped_samps = (md.time_spec - last_time).to_ticks(rate);
                    if (dropped_samps < 0) {
                        std::cerr << "[" << NOW()
                                  << "] Timestamp after overrun recovery "
                                     "ahead of error timestamp! Unable to calculate "
                                     "number of dropped samples."
                                     "(Delta: "
                                  << dropped_samps << " ticks)\n";
                    }
                    num_dropped_samps += std::max<long>(1, dropped_samps);
                }
                if ((burst_timer_elapsed or stop_called) and md.end_of_burst) {
                    return;
                }
                break;

            // ERROR_CODE_OVERFLOW can indicate overflow or sequence error
            case uhd::rx_metadata_t::ERROR_CODE_OVERFLOW:
                last_time       = md.time_spec;
                had_an_overflow = true;
                // check out_of_sequence flag to see if it was a sequence error or
                // overflow
                if (!md.out_of_sequence) {
                    num_overruns++;
                } else {
                    num_seqrx_errors++;
                    std::cerr << "[" << NOW() << "] Detected Rx sequence error."
                              << std::endl;
                }
                break;

            case uhd::rx_metadata_t::ERROR_CODE_LATE_COMMAND:
                std::cerr << "[" << NOW() << "] Receiver error: " << md.strerror()
                          << ", restart streaming..." << std::endl;
                num_late_commands++;
                // Radio core will be in the idle state. Issue stream command to restart
                // streaming.
                cmd.time_spec  = usrp->get_time_now() + uhd::time_spec_t(0.05);
                cmd.stream_now = (buffs.size() == 1);
                rx_stream->issue_stream_cmd(cmd);
                break;

            case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
                if (burst_timer_elapsed) {
                    return;
                }
                std::cerr << "[" << NOW() << "] Receiver error: " << md.strerror()
                          << ", continuing..." << std::endl;
                num_timeouts_rx++;
                break;

                // Otherwise, it's an error
            default:
                std::cerr << "[" << NOW() << "] Receiver error: " << md.strerror()
                          << std::endl;
                std::cerr << "[" << NOW() << "] Unexpected error on recv, continuing..."
                          << std::endl;
                break;
        }
    }
}

/***********************************************************************
 * Benchmark TX Rate
 **********************************************************************/
void benchmark_tx_rate(uhd::usrp::multi_usrp::sptr usrp,
    const std::string& tx_cpu,
    uhd::tx_streamer::sptr tx_stream,
    std::atomic<bool>& burst_timer_elapsed,
    const start_time_type& start_time,
    const size_t spb,
    bool elevate_priority,
    double tx_delay,
    bool random_nsamps = false)
{
    if (elevate_priority) {
        uhd::set_thread_priority_safe();
    }

    // print pre-test summary
    auto time_stamp   = NOW();
    auto tx_rate      = usrp->get_tx_rate();
    auto num_channels = tx_stream->get_num_channels();
    std::cout << boost::format("[%s] Testing transmit rate %f Msps on %u channels\n")
                     % time_stamp % (tx_rate / 1e6) % num_channels;

    // setup variables and allocate buffer
    std::vector<char> buff(spb * uhd::convert::get_bytes_per_item(tx_cpu));
    std::vector<const void*> buffs;
    for (size_t ch = 0; ch < tx_stream->get_num_channels(); ch++)
        buffs.push_back(&buff.front()); // same buffer for each channel
    // Create the metadata, and populate the time spec at the latest possible moment
    uhd::tx_metadata_t md;
    md.has_time_spec = (tx_delay != 0.0);
    md.time_spec     = usrp->get_time_now() + uhd::time_spec_t(tx_delay);

    // Calculate timeout time
    // The timeout time cannot be reduced after the first packet as is done for
    // TX because the delay will only happen after the TX buffers in the FPGA
    // are full and that is dependent on several factors such as the device,
    // FPGA configuration, and device arguments.  The extra 100ms is to account
    // for overhead of the send() call (enough).
    const double burst_pkt_time = std::max<double>(0.1, (2.0 * spb / tx_rate));
    double timeout              = burst_pkt_time + tx_delay;

    if (random_nsamps) {
        std::srand((unsigned int)time(NULL));
        while (not burst_timer_elapsed) {
            size_t num_samps = (rand() % spb) + 1;
            num_tx_samps += tx_stream->send(buffs, num_samps, md, timeout)
                            * tx_stream->get_num_channels();
            md.has_time_spec = false;
        }
    } else {
        while (not burst_timer_elapsed) {
            const size_t num_tx_samps_sent_now =
                tx_stream->send(buffs, spb, md, timeout) * tx_stream->get_num_channels();
            num_tx_samps += num_tx_samps_sent_now;
            if (num_tx_samps_sent_now == 0) {
                num_timeouts_tx++;
                if ((num_timeouts_tx % 10000) == 1) {
                    std::cerr << "[" << NOW() << "] Tx timeouts: " << num_timeouts_tx
                              << std::endl;
                }
            }
            md.has_time_spec = false;
        }
    }

    // send a mini EOB packet
    md.end_of_burst = true;
    tx_stream->send(buffs, 0, md);
}

void benchmark_tx_rate_async_helper(uhd::tx_streamer::sptr tx_stream,
    const start_time_type& start_time,
    std::atomic<bool>& burst_timer_elapsed)
{
    // setup variables and allocate buffer
    uhd::async_metadata_t async_md;
    bool exit_flag = false;

    while (true) {
        if (burst_timer_elapsed) {
            exit_flag = true;
        }

        if (not tx_stream->recv_async_msg(async_md)) {
            if (exit_flag == true)
                return;
            continue;
        }

        // handle the error codes
        switch (async_md.event_code) {
            case uhd::async_metadata_t::EVENT_CODE_BURST_ACK:
                return;

            case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW:
            case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW_IN_PACKET:
                num_underruns++;
                break;

            case uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR:
            case uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR_IN_BURST:
                num_seq_errors++;
                break;

            default:
                std::cerr << "[" << NOW() << "] Event code: " << async_md.event_code
                          << std::endl;
                std::cerr << "Unexpected event on async recv, continuing..." << std::endl;
                break;
        }
    }
}

/***********************************************************************
 * Main code + dispatcher
 **********************************************************************/
int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // variables to be set by po
    std::string args;
    std::string rx_subdev, tx_subdev;
    std::string rx_stream_args, tx_stream_args;
    double duration;
    double rx_rate, tx_rate;
    std::string rx_otw, tx_otw;
    std::string rx_cpu, tx_cpu;
    std::string ref, pps;
    std::string channel_list, rx_channel_list, tx_channel_list;
    bool random_nsamps = false;
    std::atomic<bool> burst_timer_elapsed(false);
    size_t overrun_threshold, underrun_threshold, drop_threshold, seq_threshold;
    size_t rx_spp, tx_spp, rx_spb, tx_spb;
    double tx_delay, rx_delay, adjusted_tx_delay, adjusted_rx_delay;
    bool rx_stream_now = false;
    std::string priority;
    bool elevate_priority = false;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args")
        ("duration", po::value<double>(&duration)->default_value(10.0), "duration for the test in seconds")
        ("rx_subdev", po::value<std::string>(&rx_subdev), "specify the device subdev for RX")
        ("tx_subdev", po::value<std::string>(&tx_subdev), "specify the device subdev for TX")
        ("rx_stream_args", po::value<std::string>(&rx_stream_args)->default_value(""), "stream args for RX streamer")
        ("tx_stream_args", po::value<std::string>(&tx_stream_args)->default_value(""), "stream args for TX streamer")
        ("rx_rate", po::value<double>(&rx_rate), "specify to perform a RX rate test (sps)")
        ("tx_rate", po::value<double>(&tx_rate), "specify to perform a TX rate test (sps)")
        ("rx_spp", po::value<size_t>(&rx_spp), "samples/packet value for RX")
        ("tx_spp", po::value<size_t>(&tx_spp), "samples/packet value for TX")
        ("rx_spb", po::value<size_t>(&rx_spb), "samples/buffer value for RX")
        ("tx_spb", po::value<size_t>(&tx_spb), "samples/buffer value for TX")
        ("rx_otw", po::value<std::string>(&rx_otw)->default_value("sc16"), "specify the over-the-wire sample mode for RX")
        ("tx_otw", po::value<std::string>(&tx_otw)->default_value("sc16"), "specify the over-the-wire sample mode for TX")
        ("rx_cpu", po::value<std::string>(&rx_cpu)->default_value("fc32"), "specify the host/cpu sample mode for RX")
        ("tx_cpu", po::value<std::string>(&tx_cpu)->default_value("fc32"), "specify the host/cpu sample mode for TX")
        ("ref", po::value<std::string>(&ref), "clock reference (internal, external, mimo, gpsdo)")
        ("pps", po::value<std::string>(&pps), "PPS source (internal, external, mimo, gpsdo)")
        ("random", "Run with random values of samples in send() and recv() to stress-test the I/O.")
        ("channels", po::value<std::string>(&channel_list)->default_value("0"), "which channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
        ("rx_channels", po::value<std::string>(&rx_channel_list), "which RX channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
        ("tx_channels", po::value<std::string>(&tx_channel_list), "which TX channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
        ("overrun-threshold", po::value<size_t>(&overrun_threshold),
         "Number of overruns (O) which will declare the benchmark a failure.")
        ("underrun-threshold", po::value<size_t>(&underrun_threshold),
         "Number of underruns (U) which will declare the benchmark a failure.")
        ("drop-threshold", po::value<size_t>(&drop_threshold),
         "Number of dropped packets (D) which will declare the benchmark a failure.")
        ("seq-threshold", po::value<size_t>(&seq_threshold),
         "Number of dropped packets (D) which will declare the benchmark a failure.")
        // NOTE: tx_delay defaults to 0.25 while rx_delay defaults to 0.05 when left unspecified
        // in multi-channel and multi-streamer configurations.
        ("tx_delay", po::value<double>(&tx_delay)->default_value(0.0), "delay before starting TX in seconds")
        ("rx_delay", po::value<double>(&rx_delay)->default_value(0.0), "delay before starting RX in seconds")
        ("priority", po::value<std::string>(&priority)->default_value("normal"), "thread priority (normal, high)")
        ("multi_streamer", "Create a separate streamer per channel")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // print the help message
    if (vm.count("help") or (vm.count("rx_rate") + vm.count("tx_rate")) == 0) {
        std::cout << boost::format("UHD Benchmark Rate %s") % desc << std::endl;
        std::cout << "    Specify --rx_rate for a receive-only test.\n"
                     "    Specify --tx_rate for a transmit-only test.\n"
                     "    Specify both options for a full-duplex test.\n"
                  << std::endl;
        return ~0;
    }

    if (priority == "high") {
        uhd::set_thread_priority_safe();
        elevate_priority = true;
    }

    // Random number of samples?
    if (vm.count("random")) {
        std::cout << "Using random number of samples in send() and recv() calls."
                  << std::endl;
        random_nsamps = true;
    }

    adjusted_tx_delay = tx_delay;
    adjusted_rx_delay = rx_delay;

    // create a usrp device
    std::cout << std::endl;
    uhd::device_addrs_t device_addrs = uhd::device::find(args, uhd::device::USRP);
    if (not device_addrs.empty() and device_addrs.at(0).get("type", "") == "usrp1") {
        std::cerr << "*** Warning! ***" << std::endl;
        std::cerr << "Benchmark results will be inaccurate on USRP1 due to insufficient "
                     "features.\n"
                  << std::endl;
    }
    start_time_type start_time(std::chrono::steady_clock::now());
    std::cout << boost::format("[%s] Creating the usrp device with: %s...") % NOW() % args
              << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

    // always select the subdevice first, the channel mapping affects the other settings
    if (vm.count("rx_subdev")) {
        usrp->set_rx_subdev_spec(rx_subdev);
    }
    if (vm.count("tx_subdev")) {
        usrp->set_tx_subdev_spec(tx_subdev);
    }

    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;
    int num_mboards = usrp->get_num_mboards();

    boost::thread_group thread_group;

    if (vm.count("ref")) {
        if (ref == "mimo") {
            if (num_mboards != 2) {
                std::cerr
                    << "ERROR: ref = \"mimo\" implies 2 motherboards; your system has "
                    << num_mboards << " boards" << std::endl;
                return -1;
            }
            usrp->set_clock_source("mimo", 1);
        } else {
            usrp->set_clock_source(ref);
        }

        if (ref != "internal") {
            std::cout << "Now confirming lock on clock signals..." << std::endl;
            bool is_locked = false;
            auto end_time  = std::chrono::steady_clock::now() + CLOCK_TIMEOUT;
            for (int i = 0; i < num_mboards; i++) {
                if (ref == "mimo" and i == 0)
                    continue;
                while ((is_locked = usrp->get_mboard_sensor("ref_locked", i).to_bool())
                           == false
                       and std::chrono::steady_clock::now() < end_time) {
                    std::this_thread::sleep_for(1ms);
                }
                if (is_locked == false) {
                    std::cerr << "ERROR: Unable to confirm clock signal locked on board:"
                              << i << std::endl;
                    return -1;
                }
            }
        }
    }

    if (vm.count("pps")) {
        if (pps == "mimo") {
            if (num_mboards != 2) {
                std::cerr
                    << "ERROR: ref = \"mimo\" implies 2 motherboards; your system has "
                    << num_mboards << " boards" << std::endl;
                return -1;
            }
            // make mboard 1 a slave over the MIMO Cable
            usrp->set_time_source("mimo", 1);
        } else {
            usrp->set_time_source(pps);
        }
    }

    // check that the device has sufficient RX and TX channels available
    std::vector<std::string> channel_strings;
    std::vector<size_t> rx_channel_nums;
    if (vm.count("rx_rate")) {
        if (!vm.count("rx_channels")) {
            rx_channel_list = channel_list;
        }

        boost::split(channel_strings, rx_channel_list, boost::is_any_of("\"',"));
        for (size_t ch = 0; ch < channel_strings.size(); ch++) {
            size_t chan = std::stoul(channel_strings[ch]);
            if (chan >= usrp->get_rx_num_channels()) {
                throw std::runtime_error("Invalid channel(s) specified.");
            } else {
                rx_channel_nums.push_back(std::stoul(channel_strings[ch]));
            }
        }
    }

    std::vector<size_t> tx_channel_nums;
    if (vm.count("tx_rate")) {
        if (!vm.count("tx_channels")) {
            tx_channel_list = channel_list;
        }

        boost::split(channel_strings, tx_channel_list, boost::is_any_of("\"',"));
        for (size_t ch = 0; ch < channel_strings.size(); ch++) {
            size_t chan = std::stoul(channel_strings[ch]);
            if (chan >= usrp->get_tx_num_channels()) {
                throw std::runtime_error("Invalid channel(s) specified.");
            } else {
                tx_channel_nums.push_back(std::stoul(channel_strings[ch]));
            }
        }
    }

    std::cout << boost::format("[%s] Setting device timestamp to 0...") % NOW()
              << std::endl;
    if (pps == "mimo" or ref == "mimo") {
        // only set the master's time, the slave's is automatically sync'd
        usrp->set_time_now(uhd::time_spec_t(0.0), 0);
        // ensure that the setter has completed
        usrp->get_time_now();
        // wait for the time to sync
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } else if (rx_channel_nums.size() > 1 or tx_channel_nums.size() > 1) {
        usrp->set_time_unknown_pps(uhd::time_spec_t(0.0));
    } else {
        usrp->set_time_now(0.0);
    }

    // spawn the receive test thread
    if (vm.count("rx_rate")) {
        usrp->set_rx_rate(rx_rate);
        // Set an appropriate rx_delay value (if needed) to be used as the time_spec for
        // streaming.
        // A time_spec is needed to time align multiple channels or if the user specifies
        // a delay. Also delay start in case we are using multiple streamers to stream
        // multi channel data to avoid management transaction contention between threads
        // during setup.
        if ((rx_delay == 0.0 || vm.count("multi_streamer"))
            && rx_channel_nums.size() > 1) {
            adjusted_rx_delay = std::max(rx_delay, 0.05);
        }
        if (rx_delay == 0.0
            && (vm.count("multi_streamer") || rx_channel_nums.size() == 1)) {
            rx_stream_now = true;
        }

        size_t spb = 0;
        if (vm.count("rx_spp")) {
            std::cout << boost::format("Setting RX spp to %u\n") % rx_spp;
            usrp->set_rx_spp(rx_spp);
            spb = rx_spp;
        }
        if (vm.count("rx_spb")) {
            spb = rx_spb;
        }
        if (vm.count("multi_streamer")) {
            for (size_t count = 0; count < rx_channel_nums.size(); count++) {
                std::vector<size_t> this_streamer_channels{rx_channel_nums[count]};
                // create a receive streamer
                uhd::stream_args_t stream_args(rx_cpu, rx_otw);
                stream_args.channels             = this_streamer_channels;
                stream_args.args                 = uhd::device_addr_t(rx_stream_args);
                uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
                auto rx_thread = thread_group.create_thread([=, &burst_timer_elapsed]() {
                    benchmark_rx_rate(usrp,
                        rx_cpu,
                        rx_stream,
                        spb,
                        random_nsamps,
                        start_time,
                        burst_timer_elapsed,
                        elevate_priority,
                        adjusted_rx_delay,
                        rx_delay,
                        rx_stream_now);
                });
                uhd::set_thread_name(rx_thread, "bmark_rx_strm" + std::to_string(count));
            }
        } else {
            // create a receive streamer
            uhd::stream_args_t stream_args(rx_cpu, rx_otw);
            stream_args.channels             = rx_channel_nums;
            stream_args.args                 = uhd::device_addr_t(rx_stream_args);
            uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
            auto rx_thread = thread_group.create_thread([=, &burst_timer_elapsed]() {
                benchmark_rx_rate(usrp,
                    rx_cpu,
                    rx_stream,
                    spb,
                    random_nsamps,
                    start_time,
                    burst_timer_elapsed,
                    elevate_priority,
                    adjusted_rx_delay,
                    rx_delay,
                    rx_stream_now);
            });
            uhd::set_thread_name(rx_thread, "bmark_rx_stream");
        }
    }

    // spawn the transmit test thread
    if (vm.count("tx_rate")) {
        usrp->set_tx_rate(tx_rate);
        // Set an appropriate tx_delay value (if needed) to be used as the time_spec for
        // streaming.
        // A time_spec is needed to time align multiple channels or if the user specifies
        // a delay. Also delay start in case we are using multiple streamers to stream
        // multi channel data to avoid management transaction contention between threads
        // during setup.
        if ((tx_delay == 0.0 || vm.count("multi_streamer"))
            && tx_channel_nums.size() > 1) {
            adjusted_tx_delay = std::max(tx_delay, 0.25);
        }

        if (vm.count("multi_streamer")) {
            for (size_t count = 0; count < tx_channel_nums.size(); count++) {
                std::vector<size_t> this_streamer_channels{tx_channel_nums[count]};

                // create a transmit streamer
                uhd::stream_args_t stream_args(tx_cpu, tx_otw);
                stream_args.channels             = this_streamer_channels;
                stream_args.args                 = uhd::device_addr_t(tx_stream_args);
                uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);
                const size_t max_spp             = tx_stream->get_max_num_samps();
                size_t spp                       = max_spp;
                if (vm.count("tx_spp")) {
                    spp = std::min(spp, tx_spp);
                }
                size_t spb = spp;
                if (vm.count("tx_spb")) {
                    spb = tx_spb;
                }
                std::cout << boost::format("Setting TX spb to %u\n") % spb;
                auto tx_thread = thread_group.create_thread([=, &burst_timer_elapsed]() {
                    benchmark_tx_rate(usrp,
                        tx_cpu,
                        tx_stream,
                        burst_timer_elapsed,
                        start_time,
                        spb,
                        elevate_priority,
                        adjusted_tx_delay,
                        random_nsamps);
                });
                uhd::set_thread_name(tx_thread, "bmark_tx_strm" + std::to_string(count));
                auto tx_async_thread =
                    thread_group.create_thread([=, &burst_timer_elapsed]() {
                        benchmark_tx_rate_async_helper(
                            tx_stream, start_time, burst_timer_elapsed);
                    });
                uhd::set_thread_name(
                    tx_async_thread, "bmark_tx_hlpr" + std::to_string(count));
            }
        } else {
            // create a transmit streamer
            uhd::stream_args_t stream_args(tx_cpu, tx_otw);
            stream_args.channels             = tx_channel_nums;
            stream_args.args                 = uhd::device_addr_t(tx_stream_args);
            uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);
            const size_t max_spp             = tx_stream->get_max_num_samps();
            size_t spp                       = max_spp;
            if (vm.count("tx_spp")) {
                spp = std::min(spp, tx_spp);
            }
            size_t spb = spp;
            if (vm.count("tx_spb")) {
                spb = tx_spb;
            }
            std::cout << boost::format("Setting TX spp to %u\n") % spp;
            auto tx_thread = thread_group.create_thread([=, &burst_timer_elapsed]() {
                benchmark_tx_rate(usrp,
                    tx_cpu,
                    tx_stream,
                    burst_timer_elapsed,
                    start_time,
                    spb,
                    elevate_priority,
                    adjusted_tx_delay,
                    random_nsamps);
            });
            uhd::set_thread_name(tx_thread, "bmark_tx_stream");
            auto tx_async_thread =
                thread_group.create_thread([=, &burst_timer_elapsed]() {
                    benchmark_tx_rate_async_helper(
                        tx_stream, start_time, burst_timer_elapsed);
                });
            uhd::set_thread_name(tx_async_thread, "bmark_tx_helper");
        }
    }

    // Sleep for the required duration (add any initial delay).
    // If you are benchmarking Rx and Tx at the same time, Rx threads will run longer
    // than specified duration if tx_delay > rx_delay because of the overly simplified
    // logic below and vice versa.
    if (vm.count("rx_rate") and vm.count("tx_rate")) {
        duration += std::max(adjusted_rx_delay, adjusted_tx_delay);
    } else if (vm.count("rx_rate")) {
        duration += adjusted_rx_delay;
    } else {
        duration += adjusted_tx_delay;
    }
    const int64_t secs  = int64_t(duration);
    const int64_t usecs = int64_t((duration - secs) * 1e6);
    std::this_thread::sleep_for(
        std::chrono::seconds(secs) + std::chrono::microseconds(usecs));

    // interrupt and join the threads
    burst_timer_elapsed = true;
    thread_group.join_all();

    std::cout << "[" << NOW() << "] Benchmark complete." << std::endl << std::endl;

    // print summary
    const std::string threshold_err(" ERROR: Exceeds threshold!");
    const bool overrun_threshold_err = vm.count("overrun-threshold")
                                       and num_overruns > overrun_threshold;
    const bool underrun_threshold_err = vm.count("underrun-threshold")
                                        and num_underruns > underrun_threshold;
    const bool drop_threshold_err = vm.count("drop-threshold")
                                    and num_seqrx_errors > drop_threshold;
    const bool seq_threshold_err = vm.count("seq-threshold")
                                   and num_seq_errors > seq_threshold;
    std::cout << std::endl
              << boost::format("Benchmark rate summary:\n"
                               "  Num received samples:     %u\n"
                               "  Num dropped samples:      %u\n"
                               "  Num overruns detected:    %u\n"
                               "  Num transmitted samples:  %u\n"
                               "  Num sequence errors (Tx): %u\n"
                               "  Num sequence errors (Rx): %u\n"
                               "  Num underruns detected:   %u\n"
                               "  Num late commands:        %u\n"
                               "  Num timeouts (Tx):        %u\n"
                               "  Num timeouts (Rx):        %u\n")
                     % num_rx_samps % num_dropped_samps % num_overruns % num_tx_samps
                     % num_seq_errors % num_seqrx_errors % num_underruns
                     % num_late_commands % num_timeouts_tx % num_timeouts_rx
              << std::endl;
    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    if (overrun_threshold_err || underrun_threshold_err || drop_threshold_err
        || seq_threshold_err) {
        std::cout << "The following error thresholds were exceeded:\n";
        if (overrun_threshold_err) {
            std::cout << boost::format("  * Overruns (%d/%d)") % num_overruns
                             % overrun_threshold
                      << std::endl;
        }
        if (underrun_threshold_err) {
            std::cout << boost::format("  * Underruns (%d/%d)") % num_underruns
                             % underrun_threshold
                      << std::endl;
        }
        if (drop_threshold_err) {
            std::cout << boost::format("  * Dropped packets (RX) (%d/%d)")
                             % num_seqrx_errors % drop_threshold
                      << std::endl;
        }
        if (seq_threshold_err) {
            std::cout << boost::format("  * Dropped packets (TX) (%d/%d)")
                             % num_seq_errors % seq_threshold
                      << std::endl;
        }
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
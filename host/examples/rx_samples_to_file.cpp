//
// Copyright 2010-2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/convert.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#ifdef __linux__
#    include <boost/filesystem.hpp>
#    include <boost/process.hpp>
#endif
#include <chrono>
#include <complex>
#include <csignal>
#include <fstream>
#include <iostream>
#include <numeric>
#include <regex>
#include <thread>

using namespace std::chrono_literals;

namespace po = boost::program_options;

std::mutex recv_mutex;

static bool stop_signal_called = false;
static bool overflow_message   = true;

void sig_int_handler(int)
{
    stop_signal_called = true;
}

#ifdef __linux__
/*
 * Very simple disk write test using dd for at most 1 second.
 * Measures an upper bound of the maximum
 * sustainable stream to disk rate. Though the rate measured
 * varies depending on the system load at the time.
 *
 * Does not take into account OS cache or disk cache capacities
 * filling up over time to avoid extra complexity.
 *
 * Returns the measured write speed in bytes per second
 */
double disk_rate_check(const size_t sample_type_size,
    const size_t channel_count,
    size_t samps_per_buff,
    const std::string& file)
{
    std::string err_msg =
        "Disk benchmark tool 'dd' did not run or returned an unexpected output format";
    boost::process::ipstream pipe_stream;
    boost::filesystem::path temp_file =
        boost::filesystem::path(file).parent_path() / boost::filesystem::unique_path();

    std::string disk_check_proc_str =
        "dd if=/dev/zero of=" + temp_file.native()
        + " bs=" + std::to_string(samps_per_buff * channel_count * sample_type_size)
        + " count=100";

    try {
        boost::process::child c(
            disk_check_proc_str, boost::process::std_err > pipe_stream);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (c.running()) {
            c.terminate();
        }
    } catch (std::system_error& err) {
        std::cerr << err_msg << std::endl;
        if (boost::filesystem::exists(temp_file)) {
            boost::filesystem::remove(temp_file);
        }
        return 0;
    }
    // sig_int_handler will absorb SIGINT by this point, but other signals may
    // leave a temporary file on program exit.
    boost::filesystem::remove(temp_file);

    std::string line;
    std::string dd_output;
    while (pipe_stream && std::getline(pipe_stream, line) && !line.empty()) {
        dd_output += line;
    }

    // Parse dd output this format:
    //   1+0 records in
    //   1+0 records out
    //   80000000 bytes (80 MB, 76 MiB) copied, 0.245538 s, 326 MB/s
    // and capture the measured disk write speed (e.g. 326 MB/s)
    std::smatch dd_matchs;
    std::regex dd_regex(
        R"(\d+\+\d+ records in)"
        R"(\d+\+\d+ records out)"
        R"(\d+ bytes \(\d+(?:\.\d+)? [KMGTP]?B, \d+(?:\.\d+)? [KMGTP]?iB\) copied, \d+(?:\.\d+)? s, (\d+(?:\.\d+)?) ([KMGTP]?B/s))"

    );
    std::regex_match(dd_output, dd_matchs, dd_regex);

    if ((dd_output.length() == 0) || (dd_matchs[0].str() != dd_output)) {
        std::cerr << err_msg << std::endl;
    } else {
        double disk_rate_sigfigs = std::stod(dd_matchs[1]);

        switch (dd_matchs[2].str().at(0)) {
            case 'K':
                return disk_rate_sigfigs * 1e3;
            case 'M':
                return disk_rate_sigfigs * 1e6;
            case 'G':
                return disk_rate_sigfigs * 1e9;
            case 'T':
                return disk_rate_sigfigs * 1e12;
            case 'P':
                return disk_rate_sigfigs * 1e15;
            case 'B':
                return disk_rate_sigfigs;
            default:
                std::cerr << err_msg << std::endl;
        }
    }
    return 0;
}
#endif


template <typename samp_type>
void recv_to_file(uhd::usrp::multi_usrp::sptr usrp,
    const std::string& cpu_format,
    const std::string& wire_format,
    const std::vector<size_t>& channel_nums,
    const size_t total_num_channels,
    const std::string& file,
    size_t samps_per_buff,
    unsigned long long num_requested_samples,
    double& bw,
    double time_requested            = 0.0,
    bool stats                       = false,
    bool null                        = false,
    bool enable_size_map             = false,
    bool continue_on_bad_packet      = false,
    const std::string& thread_prefix = "")
{
    unsigned long long num_total_samps = 0;
    // create a receive streamer
    uhd::stream_args_t stream_args(cpu_format, wire_format);
    stream_args.channels             = channel_nums;
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    uhd::rx_metadata_t md;

    // Cannot use std::vector as second dimension type because recv will call
    // reinterpret_cast<char*> on each subarray, which is incompatible with
    // std::vector. Instead create new arrays and manage the memory ourselves
    std::vector<samp_type*> buffs(rx_stream->get_num_channels());
    try {
        for (size_t ch = 0; ch < rx_stream->get_num_channels(); ch++) {
            buffs[ch] = new samp_type[samps_per_buff];
        }
    } catch (std::bad_alloc& exc) {
        UHD_LOGGER_ERROR("UHD")
            << "Bad memory allocation. "
               "Try a smaller samples per buffer setting or free up additional memory";
        std::exit(EXIT_FAILURE);
    }

    std::vector<std::ofstream> outfiles(rx_stream->get_num_channels());
    std::string filename;
    for (size_t ch = 0; ch < rx_stream->get_num_channels(); ch++) {
        if (not null) {
            if (rx_stream->get_num_channels() == 1) { // single channel
                filename = file;
            } else { // multiple channels
                // check if file extension exists
                if (file.find('.') != std::string::npos) {
                    const std::string base_name = file.substr(0, file.find_last_of('.'));
                    const std::string extension = file.substr(file.find_last_of('.'));
                    filename = base_name + "_" + "ch" + std::to_string(channel_nums[ch])
                               + extension;
                } else {
                    // file extension does not exist
                    filename = file + "_" + "ch" + std::to_string(channel_nums[ch]);
                }
            }
            outfiles[ch].open(filename.c_str(), std::ofstream::binary);
        }
    }

    // setup streaming
    uhd::stream_cmd_t stream_cmd((num_requested_samples == 0)
                                     ? uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS
                                     : uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = size_t(num_requested_samples);
    stream_cmd.stream_now = rx_stream->get_num_channels() == 1;
    stream_cmd.time_spec  = usrp->get_time_now() + uhd::time_spec_t(0.05);
    rx_stream->issue_stream_cmd(stream_cmd);

    typedef std::map<size_t, size_t> SizeMap;
    SizeMap mapSizes;
    const auto start_time = std::chrono::steady_clock::now();
    const auto stop_time  = start_time + (1s * time_requested);
    // Track time and samps between updating the BW summary
    auto last_update                     = start_time;
    unsigned long long last_update_samps = 0;

    // Run this loop until either time expired (if a duration was given), until
    // the requested number of samples were collected (if such a number was
    // given), or until Ctrl-C was pressed.
    while (not stop_signal_called
           and (num_requested_samples != num_total_samps or num_requested_samples == 0)
           and (time_requested == 0.0 or std::chrono::steady_clock::now() <= stop_time)) {
        const auto now = std::chrono::steady_clock::now();

        size_t num_rx_samps =
            rx_stream->recv(buffs, samps_per_buff, md, 3.0, enable_size_map);

        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
            std::cout << std::endl
                      << thread_prefix << "Timeout while streaming" << std::endl;
            break;
        }
        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
            const std::lock_guard<std::mutex> lock(recv_mutex);
            if (overflow_message) {
                overflow_message = false;
                std::cerr
                    << boost::format(
                           "Got an overflow indication. Please consider the following:\n"
                           "  Your write medium must sustain a rate of %0.3fMB/s.\n"
                           "  Dropped samples will not be written to the file.\n"
                           "  Please modify this example for your purposes.\n"
                           "  This message will not appear again.\n")
                           % (usrp->get_rx_rate(channel_nums[0]) * total_num_channels
                               * sizeof(samp_type) / 1e6);
            }
            continue;
        }
        if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
            const std::lock_guard<std::mutex> lock(recv_mutex);
            std::string error = thread_prefix + "Receiver error: " + md.strerror();
            if (continue_on_bad_packet) {
                std::cerr << error << std::endl;
                continue;
            } else
                throw std::runtime_error(error);
        }

        if (enable_size_map) {
            const std::lock_guard<std::mutex> lock(recv_mutex);
            SizeMap::iterator it = mapSizes.find(num_rx_samps);
            if (it == mapSizes.end())
                mapSizes[num_rx_samps] = 0;
            mapSizes[num_rx_samps] += 1;
        }

        num_total_samps += num_rx_samps;

        for (size_t ch = 0; ch < rx_stream->get_num_channels(); ch++) {
            if (outfiles[ch].is_open()) {
                outfiles[ch].write(
                    (const char*)buffs[ch], num_rx_samps * sizeof(samp_type));
            }
        }

        last_update_samps += num_rx_samps;
        const auto time_since_last_update = now - last_update;
        if (time_since_last_update > 1s) {
            const std::lock_guard<std::mutex> lock(recv_mutex);
            const double time_since_last_update_s =
                std::chrono::duration<double>(time_since_last_update).count();
            bw                = double(last_update_samps) / time_since_last_update_s;
            last_update_samps = 0;
            last_update       = now;
        }
    }
    const auto actual_stop_time = std::chrono::steady_clock::now();

    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    rx_stream->issue_stream_cmd(stream_cmd);

    for (size_t i = 0; i < outfiles.size(); i++) {
        if (outfiles[i].is_open()) {
            outfiles[i].close();
        }
    }

    for (size_t i = 0; i < rx_stream->get_num_channels(); i++) {
        delete[] buffs[i];
    }


    if (stats) {
        const std::lock_guard<std::mutex> lock(recv_mutex);
        std::cout << std::endl;
        const double actual_duration_seconds =
            std::chrono::duration<float>(actual_stop_time - start_time).count();
        std::cout << boost::format("%sReceived %d samples in %f seconds") % thread_prefix
                         % num_total_samps % actual_duration_seconds
                  << std::endl;

        if (enable_size_map) {
            std::cout << std::endl;
            std::cout << "Packet size map (bytes: count)" << std::endl;
            for (SizeMap::iterator it = mapSizes.begin(); it != mapSizes.end(); it++)
                std::cout << it->first << ":\t" << it->second << std::endl;
        }
    }
}

typedef std::function<uhd::sensor_value_t(const std::string&)> get_sensor_fn_t;

bool check_locked_sensor(std::vector<std::string> sensor_names,
    const char* sensor_name,
    get_sensor_fn_t get_sensor_fn,
    double setup_time)
{
    if (std::find(sensor_names.begin(), sensor_names.end(), sensor_name)
        == sensor_names.end())
        return false;

    const auto setup_timeout = std::chrono::steady_clock::now() + (setup_time * 1s);
    bool lock_detected       = false;

    std::cout << "Waiting for \"" << sensor_name << "\": ";
    std::cout.flush();

    while (true) {
        if (lock_detected and (std::chrono::steady_clock::now() > setup_timeout)) {
            std::cout << " locked." << std::endl;
            break;
        }
        if (get_sensor_fn(sensor_name).to_bool()) {
            std::cout << "+";
            std::cout.flush();
            lock_detected = true;
        } else {
            if (std::chrono::steady_clock::now() > setup_timeout) {
                std::cout << std::endl;
                throw std::runtime_error(
                    str(boost::format(
                            "timed out waiting for consecutive locks on sensor \"%s\"")
                        % sensor_name));
            }
            std::cout << "_";
            std::cout.flush();
        }
        std::this_thread::sleep_for(100ms);
    }
    std::cout << std::endl;
    return true;
}

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // variables to be set by po
    std::string args, file, type, ant, subdev, ref, wirefmt, channels;
    size_t total_num_samps, spb;
    double rate, freq, gain, bw, total_time, setup_time, lo_offset;

    std::vector<std::thread> threads;

    std::vector<size_t> channel_list;
    std::vector<std::string> channel_strings;
    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "multi uhd device address args")
        ("file", po::value<std::string>(&file)->default_value("usrp_samples.dat"), "name of the file to write binary samples to")
        ("type", po::value<std::string>(&type)->default_value("short"), "sample type: double, float, or short")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(0), "total number of samples to receive")
        ("duration", po::value<double>(&total_time)->default_value(0), "total number of seconds to receive")
        ("spb", po::value<size_t>(&spb)->default_value(10000), "samples per buffer")
        ("rate", po::value<double>(&rate)->default_value(1e6), "rate of incoming samples")
        ("freq", po::value<double>(&freq)->default_value(0.0), "RF center frequency in Hz")
        ("lo-offset", po::value<double>(&lo_offset)->default_value(0.0),
            "Offset for frontend LO in Hz (optional)")
        ("gain", po::value<double>(&gain), "gain for the RF chain")
        ("ant", po::value<std::string>(&ant), "antenna selection")
        ("subdev", po::value<std::string>(&subdev), "subdevice specification")
        ("channels,channel", po::value<std::string>(&channels)->default_value("0"), "which channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
        ("bw", po::value<double>(&bw), "analog frontend filter bandwidth in Hz")
        ("ref", po::value<std::string>(&ref), "reference source (internal, external, mimo)")
        ("wirefmt", po::value<std::string>(&wirefmt)->default_value("sc16"), "wire format (sc8, sc16 or s16)")
        ("setup", po::value<double>(&setup_time)->default_value(1.0), "seconds of setup time")
        ("progress", "periodically display short-term bandwidth")
        ("stats", "show average bandwidth on exit")
        ("sizemap", "track packet size and display breakdown on exit. Use with multi_streamer option if CPU limits stream rate.")
        ("null", "run without writing to file")
        ("continue", "don't abort on a bad packet")
        ("skip-lo", "skip checking LO lock status")
        ("int-n", "tune USRP with integer-N tuning")
        ("multi_streamer", "Create a separate streamer per channel.")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // print the help message
    if (vm.count("help")) {
        std::cout << "UHD RX samples to file " << desc << std::endl;
        std::cout << std::endl
                  << "This application streams data from a single channel of a USRP "
                     "device to a file.\n"
                  << std::endl;
        return ~0;
    }

    bool bw_summary             = vm.count("progress") > 0;
    bool stats                  = vm.count("stats") > 0;
    bool null                   = vm.count("null") > 0;
    bool enable_size_map        = vm.count("sizemap") > 0;
    bool continue_on_bad_packet = vm.count("continue") > 0;
    bool multithread            = vm.count("multi_streamer") > 0;

    if (enable_size_map)
        std::cout << "Packet size tracking enabled - will only recv one packet at a time!"
                  << std::endl;

    // create a usrp device
    std::cout << std::endl;
    std::cout << "Creating the usrp device with: " << args << "..." << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

    // Parse channel selection string
    boost::split(channel_strings, channels, boost::is_any_of("\"',"));
    for (size_t ch = 0; ch < channel_strings.size(); ch++) {
        try {
            int chan = std::stoi(channel_strings[ch]);
            if (chan >= static_cast<int>(usrp->get_rx_num_channels()) || chan < 0) {
                throw std::runtime_error("Invalid channel(s) specified.");
            } else {
                channel_list.push_back(static_cast<size_t>(chan));
            }
        } catch (std::invalid_argument const& c) {
            throw std::runtime_error("Invalid channel(s) specified.");
        } catch (std::out_of_range const& c) {
            throw std::runtime_error("Invalid channel(s) specified.");
        }
    }

    // Lock mboard clocks
    if (vm.count("ref")) {
        usrp->set_clock_source(ref);
    }

    // always select the subdevice first, the channel mapping affects the other settings
    if (vm.count("subdev"))
        usrp->set_rx_subdev_spec(subdev);

    std::cout << "Using Device: " << usrp->get_pp_string() << std::endl;

    // set the sample rate
    if (rate <= 0.0) {
        std::cerr << "Please specify a valid sample rate" << std::endl;
        return ~0;
    }
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rate / 1e6) << std::endl;
    usrp->set_rx_rate(rate, uhd::usrp::multi_usrp::ALL_CHANS);
    std::cout << boost::format("Actual RX Rate: %f Msps...")
                     % (usrp->get_rx_rate(channel_list[0]) / 1e6)
              << std::endl
              << std::endl;

    // set the center frequency
    if (vm.count("freq")) { // with default of 0.0 this will always be true
        std::cout << boost::format("Setting RX Freq: %f MHz...") % (freq / 1e6)
                  << std::endl;
        std::cout << boost::format("Setting RX LO Offset: %f MHz...") % (lo_offset / 1e6)
                  << std::endl;
        uhd::tune_request_t tune_request(freq, lo_offset);
        if (vm.count("int-n"))
            tune_request.args = uhd::device_addr_t("mode_n=integer");
        for (size_t chan : channel_list)
            usrp->set_rx_freq(tune_request, chan);
        std::cout << boost::format("Actual RX Freq: %f MHz...")
                         % (usrp->get_rx_freq(channel_list[0]) / 1e6)
                  << std::endl
                  << std::endl;
    }

    // set the rf gain
    if (vm.count("gain")) {
        std::cout << boost::format("Setting RX Gain: %f dB...") % gain << std::endl;
        usrp->set_rx_gain(gain, uhd::usrp::multi_usrp::ALL_CHANS);
        std::cout << boost::format("Actual RX Gain: %f dB...")
                         % usrp->get_rx_gain(channel_list[0])
                  << std::endl
                  << std::endl;
    }

    // set the IF filter bandwidth
    if (vm.count("bw")) {
        std::cout << boost::format("Setting RX Bandwidth: %f MHz...") % (bw / 1e6)
                  << std::endl;
        for (size_t chan : channel_list)
            usrp->set_rx_bandwidth(bw, chan);
        std::cout << boost::format("Actual RX Bandwidth: %f MHz...")
                         % (usrp->get_rx_bandwidth(channel_list[0]) / 1e6)
                  << std::endl
                  << std::endl;
    }

    // set the antenna
    if (vm.count("ant"))
        for (size_t chan : channel_list)
            usrp->set_rx_antenna(ant, chan);

    std::this_thread::sleep_for(1s * setup_time);

    // check Ref and LO Lock detect
    if (not vm.count("skip-lo")) {
        for (size_t channel : channel_list) {
            std::cout << "Locking LO on channel " << channel << std::endl;
            check_locked_sensor(
                usrp->get_rx_sensor_names(channel),
                "lo_locked",
                [usrp, channel](const std::string& sensor_name) {
                    return usrp->get_rx_sensor(sensor_name, channel);
                },
                setup_time);
        }
        if (ref == "mimo") {
            check_locked_sensor(
                usrp->get_mboard_sensor_names(0),
                "mimo_locked",
                [usrp](const std::string& sensor_name) {
                    return usrp->get_mboard_sensor(sensor_name);
                },
                setup_time);
        }
        if (ref == "external") {
            check_locked_sensor(
                usrp->get_mboard_sensor_names(0),
                "ref_locked",
                [usrp](const std::string& sensor_name) {
                    return usrp->get_mboard_sensor(sensor_name);
                },
                setup_time);
        }
    }

    if (total_num_samps == 0) {
        std::signal(SIGINT, &sig_int_handler);
        std::cout << "Press Ctrl + C to stop streaming..." << std::endl;
    }

#ifdef __linux__
    const double req_disk_rate = usrp->get_rx_rate(channel_list[0]) * channel_list.size()
                                 * uhd::convert::get_bytes_per_item(wirefmt);
    const double disk_rate_meas = disk_rate_check(
        uhd::convert::get_bytes_per_item(wirefmt), channel_list.size(), spb, file);
    if (disk_rate_meas > 0 && req_disk_rate >= disk_rate_meas) {
        std::cerr
            << boost::format(
                   "  Disk write test indicates that an overflow is likely to occur.\n"
                   "  Your write medium must sustain a rate of %0.3fMB/s,\n"
                   "  but write test returned write speed of %0.3fMB/s.\n"
                   "  The disk write rate is also affected by system load\n"
                   "  and OS/disk caching capacity.\n")
                   % (req_disk_rate / 1e6) % (disk_rate_meas / 1e6);
    }
#endif

    std::vector<size_t> chans_in_thread;
    std::vector<double> rates(channel_list.size());

#define recv_to_file_args(format)                                                    \
    (usrp,                                                                           \
        format,                                                                      \
        wirefmt,                                                                     \
        chans_in_thread,                                                             \
        channel_list.size(),                                                         \
        multithread ? "ch" + std::to_string(chans_in_thread[0]) + "_" + file : file, \
        spb,                                                                         \
        total_num_samps,                                                             \
        rates[i],                                                                    \
        total_time,                                                                  \
        stats,                                                                       \
        null,                                                                        \
        enable_size_map,                                                             \
        continue_on_bad_packet,                                                      \
        th_prefix)

    for (size_t i = 0; i < channel_list.size(); i++) {
        std::string th_prefix = "";
        if (multithread) {
            chans_in_thread.clear();
            chans_in_thread.push_back(channel_list[i]);
            th_prefix = "Thread " + std::to_string(i) + ":\n";
        } else {
            chans_in_thread = channel_list;
        }
        threads.push_back(std::thread([=, &rates]() {
            // recv to file
            if (wirefmt == "s16") {
                if (type == "double")
                    recv_to_file<double> recv_to_file_args("f64");
                else if (type == "float")
                    recv_to_file<float> recv_to_file_args("f32");
                else if (type == "short")
                    recv_to_file<short> recv_to_file_args("s16");
                else
                    throw std::runtime_error("Unknown type " + type);
            } else {
                if (type == "double")
                    recv_to_file<std::complex<double>> recv_to_file_args("fc64");
                else if (type == "float")
                    recv_to_file<std::complex<float>> recv_to_file_args("fc32");
                else if (type == "short")
                    recv_to_file<std::complex<short>> recv_to_file_args("sc16");
                else
                    throw std::runtime_error("Unknown type " + type);
            }
        }));
        if (!multithread) {
            break;
        }
    }


    if (total_time == 0) {
        if (total_num_samps > 0) {
            total_time = std::ceil(total_num_samps / usrp->get_rx_rate());
        }
    }

    // Wait a bit extra for the first updates from each thread
    std::this_thread::sleep_for(500ms);

    const auto end_time = std::chrono::steady_clock::now() + (total_time - 1) * 1s;

    while (threads.size() > 0
           && (std::chrono::steady_clock::now() < end_time || total_time == 0)
           && !stop_signal_called) {
        std::this_thread::sleep_for(1s);

        // Remove any threads that are finished
        for (size_t i = 0; i < threads.size(); i++) {
            if (!threads[i].joinable()) {
                // Thread is not joinable, i.e. it has finished and 'joined' already
                // Remove the thread from the list.
                threads.erase(threads.begin() + i);
                // Clear last bandwidth value after thread is finished
                rates[i] = 0;
            }
        }
        // Report the bandwidth of remaining threads
        if (bw_summary && threads.size() > 0) {
            const std::lock_guard<std::mutex> lock(recv_mutex);
            std::cout << "\t"
                      << (std::accumulate(std::begin(rates), std::end(rates), 0) / 1e6
                             / threads.size())
                      << " Msps" << std::endl;
        }
    }

    // join any remaining threads
    for (size_t i = 0; i < threads.size(); i++) {
        if (threads[i].joinable()) {
            threads[i].join();
        }
    }

    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}

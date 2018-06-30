//
// Copyright 2011-2014 Ettus Research LLC
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

#include <uhd/utils/thread.hpp>
#include <uhd/convert.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <complex>
#include <cstdlib>
//#include <curses.h>
#include <termios.h>
#include <fstream>
#include <stdint.h>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <csignal>

namespace po = boost::program_options;

//#define HAS_RX_METADATA_OUT_OF_SEQUENCE

#define COLOUR_START        "\033["
#define COLOUR_END          "m"
#define COLOUR_RESET        COLOUR_START"0"COLOUR_END
#define COLOUR_BLACK        "0"
#define COLOUR_RED          "1"
#define COLOUR_GREEN        "2"
#define COLOUR_YELLOW       "3"
#define COLOUR_BLUE         "4"
#define COLOUR_MAGENTA      "5"
#define COLOUR_CYAN         "6"
#define COLOUR_WHITE        "7"
#define COLOUR_LOW          "3"
#define COLOUR_HIGH         "9"
#define COLOUR_BACK         "4"
#define COLOUR_BACK_HIGH    "10"
#define COLOUR_BOLD         "1"
#define COLOUR_UNDERLINE    "4"

#define HEADER              "[  ] "
#define HEADER_TX           "[" COLOUR_START COLOUR_HIGH COLOUR_RED   COLOUR_END "TX" COLOUR_RESET "] "
#define HEADER_RX           "[" COLOUR_START COLOUR_HIGH COLOUR_GREEN COLOUR_END "RX" COLOUR_RESET "] "
#define HEADER_AS           "[" COLOUR_START COLOUR_HIGH COLOUR_BLUE COLOUR_END "AS" COLOUR_RESET "] "
#define HEADER_ERROR        "[" COLOUR_START COLOUR_BACK_HIGH COLOUR_RED COLOUR_END "!!" COLOUR_RESET "] "
#define HEADER_WARN         "[" COLOUR_START COLOUR_BACK_HIGH COLOUR_YELLOW COLOUR_END "**" COLOUR_RESET "] "

#define TICKS_PER_SEC       boost::posix_time::time_duration::ticks_per_second()

/***********************************************************************
 * Test result variables
 **********************************************************************/
unsigned long long num_overflows = 0;
unsigned long long num_underflows = 0;
unsigned long long num_underflows_in_packet = 0;
unsigned long long num_rx_samps = 0;
unsigned long long num_tx_samps = 0;
unsigned long long num_dropped_samps = 0;
unsigned long long num_seq_errors = 0;
unsigned long long num_seq_errors_in_burst = 0;
unsigned long long num_tx_acks = 0;
unsigned long long num_late_packets = 0;
unsigned long long num_late_packets_msg = 0;
unsigned long long num_send_calls = 0;

static boost::condition_variable rx_thread_complete, tx_thread_complete, tx_async_thread_complete;
static boost::condition_variable begin, abort_event, rx_thread_begin, tx_thread_begin, recv_done/*, tx_async_begin*/;
static uhd::rx_metadata_t last_rx_md;
static size_t last_rx_samps;
static boost::mutex last_rx_md_mutex, begin_rx_mutex, begin_tx_mutex, begin_tx_async_begin, stop_mutex;
static volatile bool running = false;
static volatile boost::this_thread::disable_interruption *rx_interrupt_disabler, *tx_interrupt_disabler, *tx_async_interrupt_disabler;
static bool rx_thread_finished = false, tx_thread_finished = false, tx_async_thread_finished = false;

static bool stop_signal_called = false;

static void sig_int_handler(int signal)
{
    boost::mutex::scoped_lock l(stop_mutex);
    running = false;
    stop_signal_called = true;
    abort_event.notify_all();
}

typedef std::map<char,size_t> msg_count_map_t;

static boost::mutex msg_count_map_mutex;
static msg_count_map_t msg_count_map;
static boost::system_time start_time, last_msg_print_time;
static double msg_print_interval = 0.0;

// Derived from: http://cc.byexamples.com/2007/04/08/non-blocking-user-input-in-loop-without-ncurses/
static bool kbhit(size_t timeout = 0/*ms*/)
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return (FD_ISSET(0, &fds));
}

static void set_nonblock(bool enable)
{
    struct termios ttystate;

    //get the terminal state
    tcgetattr(STDIN_FILENO, &ttystate);

    if (enable)
    {
        //turn off canonical mode
        ttystate.c_lflag &= ~ICANON;
        //minimum of number input read.
        ttystate.c_cc[VMIN] = 1;
    }
    else
    {
        //turn on canonical mode
        ttystate.c_lflag |= ICANON;
    }
    //set the terminal attributes.
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}

static std::string get_stringified_time(struct timeval* tv = NULL)
{
    struct timeval _tv;
    if (tv == NULL)
    {
        tv = &_tv;
        gettimeofday(tv, NULL); // FIXME: Use boost::posix_time
    }
    time_t t = tv->tv_sec;
    struct tm *lt = localtime(&t);
    char s[20] = { 0 };
    strftime(s, sizeof(s), "%Y/%m/%d %H:%M:%S", lt);
    return
        std::string(COLOUR_START COLOUR_UNDERLINE COLOUR_END) +
        std::string(s) +
        boost::str(boost::format(".%06ld") % tv->tv_usec) +
        std::string(COLOUR_RESET)
    ;
}

static void _select_msg_colour(char c, std::stringstream& ss)
{
    switch (c)
    {
        case 'U':
            ss << COLOUR_MAGENTA;
            break;
        case 'L':
            ss << COLOUR_RED;
            break;
        case 'S':
            ss << COLOUR_CYAN;
            break;
        case 'O':
            ss << COLOUR_GREEN;
            break;
        case 'D':
            ss << COLOUR_YELLOW;
            break;
        default:
            ss << COLOUR_BLACK;
    }
}

static void print_msgs(void)
{
    if (msg_print_interval <= 0.0)
        return;

    boost::system_time time_now = boost::get_system_time();
    boost::posix_time::time_duration update_diff = time_now - last_msg_print_time;
    if (((double)update_diff.ticks() / (double)TICKS_PER_SEC) >= msg_print_interval)
    {
        boost::mutex::scoped_lock l(msg_count_map_mutex);

        if (msg_count_map.size() > 0)
        {
            std::stringstream ss;

            ss << HEADER_WARN "(" << get_stringified_time() << ") ";

            for (msg_count_map_t::iterator it = msg_count_map.begin(); it != msg_count_map.end(); ++it)
            {
                if (it != msg_count_map.begin())
                    ss << ", ";
                ss << COLOUR_START COLOUR_HIGH;
                _select_msg_colour(it->first, ss);
                ss << COLOUR_END;
                ss << it->first;
                ss << COLOUR_RESET;
                ss << boost::str(boost::format(": %05d") % it->second);
            }

            std::cout << ss.str() << std::endl << std::flush;

            last_msg_print_time = time_now;
            msg_count_map.clear();
        }
    }
}

static void msg_handler(uhd::msg::type_t type, const std::string& msg)
{
    if ((type == uhd::msg::fastpath) && (msg.size() == 1))
    {
        char c = msg.c_str()[0];

        if (c == 'L')
            ++num_late_packets_msg;

        if (msg_print_interval <= 0.0)
        {
            std::stringstream ss;

            ss << COLOUR_START COLOUR_BACK;

            _select_msg_colour(c, ss);

            ss << ";" COLOUR_HIGH COLOUR_WHITE COLOUR_END;
            ss << msg;
            ss << COLOUR_RESET;

            std::cout << ss.str() << std::flush;
        }
        else
        {
            {
                boost::mutex::scoped_lock l(msg_count_map_mutex);

                if (msg_count_map.find(c) == msg_count_map.end())
                    msg_count_map[c] = 1;
                else
                    msg_count_map[c] += 1;
            }

            print_msgs();
        }
    }
    else
        std::cout << msg << std::flush;
}

/***********************************************************************
 * Checker thread
 **********************************************************************/

void check_thread(uhd::usrp::multi_usrp::sptr usrp)
{
    {
        std::stringstream ss;
        ss << "(" << get_stringified_time() << ") Checker running..." << std::endl;
        std::cout << ss.str();
    }
    
    while (running)
    {
        uhd::sensor_value_t ref_locked = usrp->get_mboard_sensor("ref_locked");
        if (ref_locked.to_bool() == false) {
            std::stringstream ss;
            ss << HEADER_WARN"(" << get_stringified_time() << ") " << boost::format("ref_locked: unlocked") << std::endl;
            std::cout << ss.str();
        }
        
        boost::this_thread::sleep(boost::posix_time::seconds(0) + boost::posix_time::microseconds(1000 * 500)); // MAGIC
    }

    std::cout << "Checker exiting..." << std::endl;
}

/***********************************************************************
 * Benchmark RX Rate
 **********************************************************************/

typedef struct RxParams {
    size_t samps_per_packet;
    size_t samps_per_buff;
    uhd::time_spec_t start_time;
    double start_time_delay;
    double recv_timeout;
    bool one_packet_at_a_time;
    bool check_recv_time;
    double progress_interval;
    bool single_packets;
    bool size_map;
    size_t rx_sample_limit;
    std::vector<std::ofstream*> capture_files;
    bool set_rx_freq;
    double rx_freq;
    double rx_freq_delay;
    double rx_lo_offset;
    bool interleave_rx_file_samples;
    bool ignore_late_start;
    bool ignore_bad_packets;
    bool ignore_timeout;
    bool ignore_unexpected_error;
} RX_PARAMS;

static uint64_t recv_samp_count_progress = 0;
static boost::system_time recv_samp_count_progress_update;
static size_t rx_sleep_delay_now = 0;

void benchmark_rx_rate(
    uhd::usrp::multi_usrp::sptr usrp,
    const std::string &rx_cpu,
    uhd::rx_streamer::sptr rx_stream,
    RX_PARAMS& params)
{
    uhd::set_thread_priority_safe();

    boost::mutex::scoped_lock l(begin_rx_mutex);

    rx_interrupt_disabler = new boost::this_thread::disable_interruption();

    //setup variables and allocate buffer
    size_t bytes_per_samp = uhd::convert::get_bytes_per_item(rx_cpu);
    std::vector<char> buff(params.samps_per_buff * bytes_per_samp * rx_stream->get_num_channels());
    std::vector<void *> buffs;
    for (size_t ch = 0; ch < rx_stream->get_num_channels(); ch++)
        buffs.push_back(&buff.front() + (params.samps_per_buff * bytes_per_samp * ch)); //same buffer for each channel

    bool had_an_overflow = false;
    uhd::time_spec_t last_time;
    const double rate = usrp->get_rx_rate();
    const double master_clock_rate = usrp->get_master_clock_rate();
    uhd::rx_metadata_t md;

    if (params.set_rx_freq)
    {
        if (params.rx_freq_delay == 0)
        {
            std::cout << boost::format(HEADER_RX"Setting RX freq: %f (LO offset: %f Hz)") % params.rx_freq % params.rx_lo_offset << std::endl;

            uhd::tune_request_t tune_request = uhd::tune_request_t(params.rx_freq, params.rx_lo_offset);
            for (size_t ch = 0; ch < rx_stream->get_num_channels(); ch++)
                usrp->set_rx_freq(tune_request, ch);
        }
    }

    std::cout << HEADER_RX"Waiting..." << std::endl;
    rx_thread_begin.notify_all();
    begin.wait(l);
    l.unlock();

    {
        std::stringstream ss;
        ss << HEADER_RX"(" << get_stringified_time() << ") Running..." << std::endl;
        std::cout << ss.str();
    }

    uhd::time_spec_t time_now = usrp->get_time_now();
    uhd::time_spec_t diff = time_now - params.start_time;
    std::cout << boost::format(HEADER_RX"USRP time difference between right now and start time: %ld ticks (%f seconds)") % diff.to_ticks(rate) % diff.get_real_secs() << std::endl;

    uhd::time_spec_t actual_start_time = params.start_time + uhd::time_spec_t(params.start_time_delay);

    if (params.start_time_delay >= 0.0)
        std::cout << HEADER_RX"Will begin streaming at time " << boost::format("%.6f") % actual_start_time.get_real_secs() << std::endl;

    uhd::stream_cmd_t cmd((params.rx_sample_limit == 0) ? uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS : uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE); // FIXME: The other streaming modes
    cmd.num_samps = params.rx_sample_limit;
    cmd.time_spec = actual_start_time;
    cmd.stream_now = (params.start_time_delay == 0.0);
    rx_stream->issue_stream_cmd(cmd);

    if (params.set_rx_freq)
    {
        if (params.rx_freq_delay > 0)
        {
            std::cout << boost::format(HEADER_RX"Scheduling RX freq in %d seconds: %f (LO offset: %f Hz)") % params.rx_freq_delay % params.rx_freq % params.rx_lo_offset << std::endl;

            uhd::time_spec_t tune_time = params.start_time + uhd::time_spec_t(params.rx_freq_delay);
            usrp->set_command_time(tune_time);

            uhd::tune_request_t tune_request = uhd::tune_request_t(params.rx_freq, params.rx_lo_offset);
            for (size_t ch = 0; ch < rx_stream->get_num_channels(); ch++)
                usrp->set_rx_freq(tune_request, ch);

            usrp->clear_command_time();
        }
    }

    double timeout = params.recv_timeout;
    if (cmd.stream_now == false)
        timeout += params.start_time_delay;

    size_t num_recv_calls = 0;
    int64_t cur_timestamp = 0;

    boost::system_time time_last_progress;
    uint64_t samps_last_progress = 0;

    unsigned long long num_rx_samps_single_chan = 0;

    //while (not boost::this_thread::interruption_requested()){
    try {
        while (running)
        {
            if (rx_sleep_delay_now > 0) // UNSYNC'd
            {
                size_t delay = rx_sleep_delay_now;
                rx_sleep_delay_now = 0;
                usleep(delay);
            }

            //try {
                size_t recv_samps = rx_stream->recv(buffs, params.samps_per_buff, md, timeout, params.one_packet_at_a_time);
                ++num_recv_calls;

                if (params.progress_interval > 0.0)
                {
                    if (num_recv_calls == 1)
                    {
                        time_last_progress = boost::get_system_time();
                        //samps_last_progress = recv_samps;
                    }
                    else
                    {
                        samps_last_progress += recv_samps;

                        boost::system_time time_now = boost::get_system_time();
                        boost::posix_time::time_duration update_diff = time_now - time_last_progress;
                        if (((double)update_diff.ticks() / (double)TICKS_PER_SEC) >= params.progress_interval)
                        {
                            double d = ((double)samps_last_progress * (double)TICKS_PER_SEC) / ((double)update_diff.ticks());
                            std::stringstream ss;
                            ss << HEADER_RX"(" << get_stringified_time() << ") " << boost::format("%.6f Msps") % (d/1e6) << std::endl;
                            std::cout << ss.str();
                            time_last_progress = time_now;
                            samps_last_progress = 0;
                        }
                    }
                }

                if (recv_samps > 0)
                {
                    if ((num_recv_calls == 1) && (cmd.stream_now == false))
                        std::cout << HEADER_RX"(" << get_stringified_time() << ") Received first packet after delayed start with time " << boost::format("%.6f") % md.time_spec.get_real_secs() << std::endl;

                    if (params.check_recv_time) {
                        int64_t timestamp = md.time_spec.to_ticks(rate);
                        if ((cur_timestamp != 0) && (cur_timestamp != timestamp)) {
                            std::stringstream ss;
                            ss << HEADER_RX"(" << get_stringified_time() << ") ";
                            ss << boost::format("TS: %lld, expected: %lld (diff: %lld)") % timestamp % cur_timestamp % (timestamp - cur_timestamp) << std::endl;
                            std::cout << ss.str();
                        }
                        cur_timestamp = timestamp + recv_samps;
                    }

                    if (params.size_map)
                    {
                        // FIXME
                    }

                    timeout = params.recv_timeout;
                    num_rx_samps_single_chan += recv_samps;
                    num_rx_samps += recv_samps * rx_stream->get_num_channels();

                    if ((params.rx_sample_limit > 0) && (num_rx_samps_single_chan == params.rx_sample_limit))
                    {
                        std::stringstream ss;
                        ss << HEADER_RX"(" << get_stringified_time() << ") ";
                        ss << boost::format("Received all %lu requested samples") % params.rx_sample_limit << std::endl;
                        std::cout << ss.str();
                        break;
                    }

                    {
                        boost::mutex::scoped_lock lock(last_rx_md_mutex);
                        last_rx_md = md;
                        last_rx_samps = recv_samps;
                        recv_samp_count_progress += recv_samps;
                        recv_samp_count_progress_update = boost::get_system_time();
                        recv_done.notify_one();
                    }

                    if (params.capture_files.empty() == false)
                    {
                        size_t channel_count = rx_stream->get_num_channels();

                        if ((channel_count == 1) || ((channel_count > 1) && (params.capture_files.size() == 1)))
                        {
                            if (params.interleave_rx_file_samples)
                            {
                                for (size_t i = 0; i < recv_samps; ++i)
                                {
                                    for (size_t j = 0; j < channel_count; ++j)
                                    {
                                        params.capture_files[0]->write((const char*)buffs[j] + (bytes_per_samp * i), bytes_per_samp);
                                    }
                                }
                            }
                            else
                            {
                                for (size_t i = 0; i < channel_count; ++i)
                                {
                                    size_t num_bytes = recv_samps * bytes_per_samp;
                                    params.capture_files[0]->write((const char*)buffs[i], num_bytes);
                                }
                            }
                        }
                        else
                        {
                            for (size_t n = 0; n < channel_count; ++n)
                            {
                                size_t num_bytes = recv_samps * bytes_per_samp;
                                params.capture_files[n]->write((const char*)buffs[n], num_bytes);
                            }
                        }
                    }
                }
            //}
            //catch (...) {
              /* apparently, the boost thread interruption can sometimes result in
                 throwing exceptions not of type boost::exception, this catch allows
                 this thread to still attempt to issue the STREAM_MODE_STOP_CONTINUOUS
              */
            //  break;
            //}

            //handle the error codes
            switch(md.error_code)
            {
                case uhd::rx_metadata_t::ERROR_CODE_NONE:
                {
                    if (had_an_overflow)
                    {
                        had_an_overflow = false;
                        num_dropped_samps += (md.time_spec - last_time).to_ticks(rate); // FIXME: Check this as 'num_dropped_samps' has come out -ve
                    }
                    break;
                }

                // ERROR_CODE_OVERFLOW can indicate overflow or sequence error
                case uhd::rx_metadata_t::ERROR_CODE_OVERFLOW:   // 'recv_samps' should be 0
                    last_time = md.time_spec;
                    had_an_overflow = true;
#if HAS_RX_METADATA_OUT_OF_SEQUENCE
                    // check out_of_sequence flag to see if it was a sequence error or overflow
                    if (!md.out_of_sequence)
#endif // HAS_RX_METADATA_OUT_OF_SEQUENCE
                        num_overflows++;
                    break;

                case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
                {
                    std::stringstream ss;
                    ss << HEADER_RX"(" << get_stringified_time() << ") ";
                    ss << boost::format("Timeout") << std::endl;
                    std::cout << ss.str();
                    if (params.ignore_timeout == false)
                        sig_int_handler(-1);
                    break;
                }

                case uhd::rx_metadata_t::ERROR_CODE_LATE_COMMAND:
                {
                    std::stringstream ss;
                    ss << HEADER_RX"(" << get_stringified_time() << ") ";
                    ss << boost::format("Late command") << std::endl;
                    std::cout << ss.str();
                    if (params.ignore_late_start == false)
                        sig_int_handler(-1);
                    break;
                }

                case uhd::rx_metadata_t::ERROR_CODE_BAD_PACKET:
                {
                    std::stringstream ss;
                    ss << HEADER_RX"(" << get_stringified_time() << ") ";
                    ss << boost::format("Bad packet") << std::endl;
                    std::cout << ss.str();
                    if (params.ignore_bad_packets == false)
                        sig_int_handler(-1);
                    break;
                }

                default:
                {
                    std::stringstream ss;
                    ss << HEADER_RX"(" << get_stringified_time() << ") ";
                    ss << (boost::format("Unexpected error (code: %d)") % md.error_code) << std::endl;
                    std::cout << ss.str();
                    if (params.ignore_unexpected_error == false)
                        sig_int_handler(-1);
                    break;
                }
            }

            print_msgs();
        }
    }
    catch (const std::runtime_error& e)
    {
        std::cout << HEADER_RX"Caught exception:" <<  e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << HEADER_RX"Caught unhandled exception" << std::endl;
    }

    if (params.rx_sample_limit == 0)
    {
        std::cout << HEADER_RX"Stopping streaming..." << std::endl;
        rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
    }

    if (params.capture_files.empty() == false)
    {
        std::cout << HEADER_RX"Closing capture files..." << std::endl;

        for (size_t n = 0; n < params.capture_files.size(); ++n)
            delete params.capture_files[n];

        params.capture_files.clear();
    }

    l.lock();
    rx_thread_finished = true;

    if (rx_interrupt_disabler)
    {
        delete rx_interrupt_disabler;
        rx_interrupt_disabler = NULL;
    }

    std::cout << HEADER_RX"Exiting..." << std::endl;

    rx_thread_complete.notify_all();
}

/***********************************************************************
 * Benchmark TX Rate
 **********************************************************************/

static size_t tx_sleep_delay_now = 0;

typedef struct TxParams {
    uhd::time_spec_t start_time;
    double send_timeout;
    double send_start_delay;
    bool use_tx_eob;
    bool use_tx_timespec;
    bool tx_rx_sync;
    bool send_final_eob;
    double progress_interval;
    bool use_relative_timestamps;
    bool follow_rx_timestamps;
    double tx_time_offset;
    double rx_rate;
    size_t tx_burst_length;
    size_t tx_flush_length;
    double tx_full_scale;
    double tx_time_between_bursts;
    bool recover_late;
    bool set_tx_freq;
    double tx_freq;
    double tx_freq_delay;
    double tx_lo_offset;
} TX_PARAMS;

void benchmark_tx_rate(
    uhd::usrp::multi_usrp::sptr usrp,
    const std::string &tx_cpu,
    uhd::tx_streamer::sptr tx_stream,
    TX_PARAMS& params
    )
{
    uhd::set_thread_priority_safe();

    boost::mutex::scoped_lock l(begin_tx_mutex);

    tx_interrupt_disabler = new boost::this_thread::disable_interruption();

    //setup variables and allocate buffer
    const double rate = usrp->get_tx_rate();
    const size_t max_samps_per_packet = tx_stream->get_max_num_samps();

    if (params.tx_burst_length == 0)
    {
        params.tx_burst_length = max_samps_per_packet - params.tx_flush_length;
    }
    size_t total_length = params.tx_burst_length + params.tx_flush_length;

    uhd::time_spec_t packet_time = uhd::time_spec_t::from_ticks(total_length, rate);
    size_t total_packet_count = (total_length / max_samps_per_packet) + ((total_length % max_samps_per_packet) ? 1 : 0);
    if ((params.use_tx_eob) && (params.tx_time_between_bursts > 0))
        packet_time += uhd::time_spec_t(params.tx_time_between_bursts);
    size_t max_late_count = (size_t)(rate / (double)packet_time.to_ticks(rate)) * total_packet_count * tx_stream->get_num_channels();   // Also need to take into account number of radios

    // Will be much higher L values (e.g. 31K) on e.g. B200 when entire TX pipeline is full of late packets (large size due to total TX buffering throughout transport & DSP)

    std::cout << boost::format(HEADER_TX"TX burst length: %lu samples (flush: %lu samples), total: %lu (%f us)") % params.tx_burst_length % params.tx_flush_length % total_length % (1e6 * (double)total_length / rate) << std::endl;
    std::cout << boost::format(HEADER_TX"Max late packet count: %lu") % max_late_count << std::endl;

    std::vector<const void *> buffs;
    std::vector<char> buff(max_samps_per_packet * uhd::convert::get_bytes_per_item(tx_cpu), 0);
    float* pResponse = NULL;
    if (tx_cpu == "fc32")
    {
        std::cout << HEADER_TX"Generating ramp" << std::endl;

        pResponse = new float[total_length * 2];
        for (int i = 0; i < (params.tx_burst_length * 2); i += 2)
        {
            pResponse[i+0] = (params.tx_full_scale) * ((double)i / (double)(params.tx_burst_length * 2));
            pResponse[i+1] = 0.0f;
        }

        for (int i = (params.tx_burst_length * 2); i < (total_length * 2); ++i)
        {
            pResponse[i] = 0.0f;
        }

        for (size_t ch = 0; ch < tx_stream->get_num_channels(); ch++)
        {
            buffs.push_back(pResponse);
        }
    }
    else
    {
        std::cout << HEADER_TX"Generating silence" << std::endl;

        for (size_t ch = 0; ch < tx_stream->get_num_channels(); ch++)
        {
            buffs.push_back(&buff.front()); //same buffer for each channel
        }
    }

    if (params.set_tx_freq)
    {
        if (params.tx_freq_delay > 0)   // FIXME: Experiment to see whether this will also experience head-of-line blocking due to SPI xact fill-up (as what happened with RX)
        {
            std::cout << boost::format(HEADER_TX"Scheduling TX freq in %d seconds: %f (LO offset: %f Hz)") % params.tx_freq_delay % params.tx_freq % params.tx_lo_offset << std::endl;

            uhd::time_spec_t tune_time = params.start_time + uhd::time_spec_t(params.tx_freq_delay);
            usrp->set_command_time(tune_time);
        }
        else
            std::cout << boost::format(HEADER_TX"Setting TX freq: %f (LO offset: %f Hz)") % params.tx_freq % params.tx_lo_offset << std::endl;

        uhd::tune_request_t tune_request = uhd::tune_request_t(params.tx_freq, params.tx_lo_offset);
        for (size_t ch = 0; ch < tx_stream->get_num_channels(); ch++)
            usrp->set_tx_freq(tune_request, ch);

        if (params.tx_freq_delay > 0)
        {
            usrp->clear_command_time();
        }
    }

    if ((params.use_tx_timespec) && (params.tx_rx_sync == false)) {
        uhd::time_spec_t time_now  = usrp->get_time_now();
        uhd::time_spec_t diff = time_now - params.start_time;
        std::cout << boost::format(HEADER_TX"Now - start time: %ld ticks (%f seconds)") % diff.to_ticks(rate) % diff.get_real_secs() << std::endl;
    }

    uhd::tx_metadata_t md;
    //md.start_of_burst;    // Currently not used on any HW
    md.end_of_burst = params.use_tx_eob;

    double timeout = params.send_timeout;

    boost::system_time time_last_progress;
    boost::system_time time_first_send;
    uint64_t samps_last_progress = 0;

    std::cout << HEADER_TX"Waiting..." << std::endl;
    tx_thread_begin.notify_all();
    begin.wait(l);
    l.unlock();

    uhd::time_spec_t last_recv_time;

    if ((params.use_tx_timespec)/* && (params.send_start_delay > 0)*/) {
        if ((params.tx_rx_sync) || (params.follow_rx_timestamps)) {
            boost::mutex::scoped_lock lock(last_rx_md_mutex);
            {
                std::stringstream ss;
                ss << HEADER_TX"(" << get_stringified_time() << ") Waiting for first RX packet..." << std::endl;
                std::cout << ss.str();
            }
            recv_done.wait(lock);
            {
                std::stringstream ss;
                ss << HEADER_TX"(" << get_stringified_time() << ") First RX packet arrived with time "<< boost::format("%.6f") % last_rx_md.time_spec.get_real_secs() << std::endl;
                std::cout << ss.str();
            }

            last_recv_time = last_rx_md.time_spec;

            if (params.tx_rx_sync)
                params.start_time = last_rx_md.time_spec + uhd::time_spec_t(params.tx_time_offset);
        }
        else
        {
            std::stringstream ss;
            ss << HEADER_TX"(" << get_stringified_time() << ") "<< boost::format("TX will start %lld ticks in the future") % uhd::time_spec_t(params.send_start_delay).to_ticks(rate) << std::endl;
            std::cout << ss.str();

            params.start_time += uhd::time_spec_t(params.send_start_delay);
        }

        md.time_spec = params.start_time;
        md.has_time_spec = true;
        //timeout += params.send_start_delay;    // In case we fill up HW buf with large initial send buffer
    }

    {
        std::stringstream ss;
        ss << HEADER_TX"(" << get_stringified_time() << ") Running..." << std::endl;
        std::cout << ss.str();
    }

    // Important note:
    //  When idle, HW will attempt to honour first packet's time_spec
    //  If it's late, host will see L and HW will either drop it or send the next packet (depending on policy)
    //  Once the first packet has been transmitted, and there is no EOB, the next packet will be sent
    //      immediately following it, regardless of its time_spec
    //  If there is an EOB, the HW is returned to idle
    //  If there is an underrun, the HW is returned to idle

    // o Free-run
    // o Delayed start after common start
    // o Delayed start after first RX
    // o Calculate own TX time based on samples sent (will result in Ls)
    //  - Why does this produce Ls with EOB enabled? Because (at least on B200) there is a finite state-switch time, so it won't be able to service the very next packet every time (e.g. burst separate of 1e-6 works)
    // o Follow RX + N -> (really above) -> relative sync'd (calculated from samples count received)
    // * Wait for next slot -> sync'd & not relative (use % (mod) for slot)
    // o Detect Ls -> re-sync relative

    size_t loops_to_send = 0;
    uhd::time_spec_t next;
    boost::system_time time_now = boost::get_system_time();
    boost::system_time last_late_check_time = time_now;
    unsigned long long last_num_late_packets = 0;
    bool resync_time = false;
    uhd::time_spec_t follow_time_target = md.time_spec;

    //while (not boost::this_thread::interruption_requested()){
    while (running)
    {
        if (tx_sleep_delay_now) // UNSYNC'd
        {
            size_t delay = tx_sleep_delay_now;
            tx_sleep_delay_now = 0;
            usleep(delay);
        }

        if (params.follow_rx_timestamps)
        {
            boost::mutex::scoped_lock lock(last_rx_md_mutex);

            uhd::time_spec_t diff = last_rx_md.time_spec - last_recv_time;

            if ((resync_time) || /*(num_send_calls == 0) || */((diff.get_real_secs() == 0) && (md.time_spec >= follow_time_target)))
            {
                recv_done.wait(lock);

                if (resync_time)
                {
                    resync_time = false;

                    last_recv_time = last_rx_md.time_spec;
                    md.time_spec = last_rx_md.time_spec + uhd::time_spec_t(params.tx_time_offset);
                    follow_time_target = md.time_spec;

                    continue;
                }

                diff = last_rx_md.time_spec - last_recv_time;
            }

            if ((diff.get_real_secs() > 0) && (md.time_spec >= follow_time_target))
            {
                follow_time_target = md.time_spec + diff;

                last_recv_time = last_rx_md.time_spec;
            }
        }

        size_t nsent = tx_stream->send(buffs, total_length, md, timeout);
        ++num_send_calls;

        time_now = boost::get_system_time();

        if (params.progress_interval > 0.0)
        {
            if (num_send_calls == 1)
            {
                time_first_send = time_last_progress = boost::get_system_time();
                //samps_last_progress = nsent;
            }
            else
            {
                samps_last_progress += nsent;

                //boost::system_time time_now = boost::get_system_time();
                boost::posix_time::time_duration update_diff = time_now - time_last_progress;
                if (((double)update_diff.ticks() / (double)TICKS_PER_SEC) >= params.progress_interval)
                {
                    double d = ((double)samps_last_progress * (double)TICKS_PER_SEC) / ((double)update_diff.ticks());
                    std::stringstream ss;
                    ss << HEADER_TX"(" << get_stringified_time() << ") " << boost::format("%.6f Msps") % (d/1e6) << std::endl;
                    std::cout << ss.str();
                    time_last_progress = time_now;
                    samps_last_progress = 0;
                }
            }
        }

        if (nsent == 0)
        {
            if ((params.use_tx_timespec) && (params.send_start_delay > 0))
            {
                //boost::system_time time_now = boost::get_system_time();
                boost::posix_time::time_duration diff = time_now - time_first_send;
                if (((double)diff.ticks() / (double)TICKS_PER_SEC) >= (timeout + params.send_start_delay))
                {
                    // TX chain has filled after delayed start
                }
            }
            else
            {
                // TX chain has filled after immediate start
            }
        }
        else
        {
            if (nsent != total_length)
            {
                std::stringstream ss;
                ss << HEADER_WARN"(" << get_stringified_time() << ") " << boost::format("Only sent %lu of %lu samples") % nsent % total_length << std::endl;
                std::cout << ss.str();
            }

            if ((params.use_relative_timestamps) && (params.use_tx_timespec) && (md.has_time_spec))
            {
                md.time_spec += uhd::time_spec_t::from_ticks(nsent, rate);

                if (params.tx_time_between_bursts)
                    md.time_spec += uhd::time_spec_t(params.tx_time_between_bursts);
            }
        }

        if ((num_tx_samps == 0) && (nsent > 0))
        {
            if (md.has_time_spec)
            {
                std::stringstream ss;
                ss << HEADER_TX"(" << get_stringified_time() << ") ";
                ss << boost::format("First send completed having sent %d samples") % nsent << std::endl;
                std::cout << ss.str();
            }
        }

        num_tx_samps += nsent * tx_stream->get_num_channels();

        if ((params.use_tx_timespec == false) && (md.has_time_spec))
            md.has_time_spec = false;

        if (params.recover_late)
        {
            boost::posix_time::time_duration late_check_diff = time_now - last_late_check_time;
            if (((double)late_check_diff.ticks() / (double)TICKS_PER_SEC) >= 1.0)   // MAGIC
            {
                // UNSYNC'd
                // FIXME: Why doesn't num_late_packets go up with B200?
                unsigned long long diff = /*num_late_packets*/num_late_packets_msg - last_num_late_packets;
                if (diff >= max_late_count)
                {
                    std::stringstream ss;
                    ss << HEADER_TX"(" << get_stringified_time() << ") ";
                    ss << boost::format("Exceeded max late packet threshold: %llu >= %llu") % diff % max_late_count << std::endl;
                    std::cout << ss.str();

                    if ((params.tx_rx_sync) && (params.follow_rx_timestamps))
                        resync_time = true;
                    else
                        md.time_spec = usrp->get_time_now() + uhd::time_spec_t(params.tx_time_offset);
                }
                last_num_late_packets = /*num_late_packets*/num_late_packets_msg;
                last_late_check_time = time_now;
            }
        }

        print_msgs();
    }

    if (params.send_final_eob)
    {
        std::cout << HEADER_TX"Sending final EOB..." << std::endl;

        //send a mini EOB packet
        md.has_time_spec = false;
        md.end_of_burst = true;
        tx_stream->send(buffs, 0, md);
    }
    else
        std::cout << HEADER_TX"Not sending final EOB" << std::endl;

    if (pResponse)
    {
        delete [] pResponse;
        pResponse = NULL;
    }

    l.lock();
    tx_thread_finished = true;

    if (tx_interrupt_disabler)
    {
        delete tx_interrupt_disabler;
        tx_interrupt_disabler = NULL;
    }

    std::cout << HEADER_TX"Exiting..." << std::endl;

    tx_thread_complete.notify_all();
}

void benchmark_tx_rate_async_helper(
    uhd::tx_streamer::sptr tx_stream,
    double timeout)
{
    boost::mutex::scoped_lock l(begin_tx_async_begin);

    tx_async_interrupt_disabler = new boost::this_thread::disable_interruption();

    std::cout << HEADER_AS"Running..." << std::endl;

    //setup variables and allocate buffer
    uhd::async_metadata_t async_md;

    l.unlock();

    bool skip = false;

    //while (not boost::this_thread::interruption_requested()){
    while (running) {
        if (not tx_stream->recv_async_msg(async_md, (skip ? 0 : timeout)))
        {
            //std::cout << "-" << std::endl;
            skip = false;
            continue;
        }

        skip = true;

        //std::cout << "Async event code: " << async_md.event_code << std::endl;

        //handle the error codes
        switch(async_md.event_code)
        {
            case uhd::async_metadata_t::EVENT_CODE_BURST_ACK:
                num_tx_acks++;
                return;

            case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW:
                num_underflows++;
                break;

            case uhd::async_metadata_t::EVENT_CODE_UNDERFLOW_IN_PACKET:
                num_underflows_in_packet++;
                break;

            case uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR:
                num_seq_errors++;
                break;

            case uhd::async_metadata_t::EVENT_CODE_SEQ_ERROR_IN_BURST:
                num_seq_errors_in_burst++;
                break;

            case uhd::async_metadata_t::EVENT_CODE_TIME_ERROR:
                num_late_packets++;
                break;

            default:
                std::cerr << HEADER_AS"Event code: " << async_md.event_code << std::endl;
                std::cerr << HEADER_AS"Unexpected event on async recv, continuing..." << std::endl;
                break;
        }
    }

    l.lock();
    tx_async_thread_finished = true;

    if (tx_async_interrupt_disabler)
    {
        delete tx_async_interrupt_disabler;
        tx_async_interrupt_disabler = NULL;
    }

    std::cout << HEADER_AS"Exiting..." << std::endl;

    tx_async_thread_complete.notify_all();
}

std::vector<size_t> get_channels(const std::string& channel_list, size_t max = -1)
{
    std::vector<std::string> channel_strings;
    std::vector<size_t> channel_nums;

    if (channel_list.size() > 0)
        boost::split(channel_strings, channel_list, boost::is_any_of("\"',"));

    for (size_t ch = 0; ch < channel_strings.size(); ch++)
    {
        size_t chan = boost::lexical_cast<int>(channel_strings[ch]);
        if ((max >= 0) && (chan >= max)) {
            throw std::runtime_error("Invalid channel(s) specified.");
        }
        else {
            channel_nums.push_back(boost::lexical_cast<int>(channel_strings[ch]));
        }
    }

    return channel_nums;
}

/***********************************************************************
 * Main code + dispatcher
 **********************************************************************/
int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args;
    double duration;
    double rate = 1e6;
    double rx_rate = rate, tx_rate = rate;
    std::string rx_otw, tx_otw;
    std::string rx_cpu, tx_cpu;
    std::string mode;
    std::string channel_list = "0";
    std::string rx_channel_list/* = channel_list*/, tx_channel_list/* = channel_list*/;
    size_t samps_per_buff;
    size_t samps_per_packet;
    double master_clock_rate;
    double recv_timeout, send_timeout;
    double recv_start_delay, send_start_delay;
    double tx_async_timeout;
    double interrupt_timeout;
    double progress_interval = 0.0;
    double rx_progress_interval = progress_interval, tx_progress_interval = progress_interval;
    double tx_time_offset;
    size_t tx_burst_length; // Optionally in time (not samples)
    size_t tx_flush_length;
    size_t interactive_sleep;
    double tx_full_scale;
    double tx_freq = 0, tx_freq_init = 0;
    double rx_freq = 0, rx_freq_init = 0;
    double rx_lo_offset, tx_lo_offset;
    double tx_freq_delay = 0, rx_freq_delay = 0;
    double tx_gain = 0, rx_gain = 0;
    double tx_time_between_bursts;
    size_t tx_sleep_delay;
    size_t rx_sleep_delay;
    size_t rx_sample_limit;
    std::string rx_file;
    std::string time_source, clock_source;
    std::string tx_ant, rx_ant;
    std::string tx_subdev, rx_subdev;
    std::string set_time_mode;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args")
        ("duration", po::value<double>(&duration)->default_value(0.0), "duration for the test in seconds (0 = forever)")
        ("rate", po::value<double>(&rate)->default_value(rate), "specify to perform a TX & RX rate test (sps)")
        ("rx-rate", po::value<double>(&rx_rate), "specify to perform a RX rate test (sps)")
        ("tx-rate", po::value<double>(&tx_rate), "specify to perform a TX rate test (sps)")
        ("rx-subdev", po::value<std::string>(&rx_subdev)->default_value(""), "set RX sub-device specification")
        ("tx-subdev", po::value<std::string>(&tx_subdev)->default_value(""), "set TX sub-device specification")
        ("rx-otw", po::value<std::string>(&rx_otw)->default_value("sc16"), "specify the over-the-wire sample mode for RX")
        ("tx-otw", po::value<std::string>(&tx_otw)->default_value("sc16"), "specify the over-the-wire sample mode for TX")
        ("rx-cpu", po::value<std::string>(&rx_cpu)->default_value("fc32"), "specify the host/cpu sample mode for RX")
        ("tx-cpu", po::value<std::string>(&tx_cpu)->default_value("fc32"), "specify the host/cpu sample mode for TX")
        ("mode", po::value<std::string>(&mode)->default_value("none"), "multi-channel sync mode option: none, mimo (mimo overrides time and clock source")
        ("time", po::value<std::string>(&time_source), "time reference (external, mimo)")
        ("clock", po::value<std::string>(&clock_source), "clock reference (internal, external, mimo)")
        ("channels", po::value<std::string>(&channel_list)/*->default_value(channel_list)*/, "which channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
        ("rx-channels", po::value<std::string>(&rx_channel_list), "which RX channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
        ("tx-channels", po::value<std::string>(&tx_channel_list), "which TX channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
        ("spp", po::value<size_t>(&samps_per_packet)->default_value(0), "samples per packet (0: use default)")
        ("spb", po::value<size_t>(&samps_per_buff)->default_value(0), "samples per buffer (0: use max samples per packet)")
        ("mcr", po::value<double>(&master_clock_rate)->default_value(0.0), "master clock rate (0: use default)")
        ("rx-timeout", po::value<double>(&recv_timeout)->default_value(0.1), "recv timeout")
        ("tx-timeout", po::value<double>(&send_timeout)->default_value(0.1), "send timeout")
        ("tx-async-timeout", po::value<double>(&tx_async_timeout)->default_value(0.1), "recv_async_msg timeout")
        ("rx-start-delay", po::value<double>(&recv_start_delay)->default_value(0.0), "recv start delay (seconds)")
        ("tx-start-delay", po::value<double>(&send_start_delay)->default_value(0.0), "send start delay (seconds)")
        ("interrupt-timeout", po::value<double>(&interrupt_timeout)->default_value(0.0), "time before re-enabling boost thread interruption")
        ("msg-interval", po::value<double>(&msg_print_interval)->default_value(0.0), "seconds between printing UHD fastpath status messages")
        ("progress-interval", po::value<double>(&progress_interval)->default_value(progress_interval), "seconds between bandwidth updates (0 disables)")
        ("rx-progress-interval", po::value<double>(&rx_progress_interval), "seconds between RX bandwidth updates (0 disables)")
        ("tx-progress-interval", po::value<double>(&tx_progress_interval), "seconds between TX bandwidth updates (0 disables)")
        ("tx-offset", po::value<double>(&tx_time_offset)->default_value(0.0), "seconds that TX should be in front of RX when following")
        ("tx-length", po::value<size_t>(&tx_burst_length)->default_value(0), "TX burst length in samples (0: maximum packet size)")
        ("tx-flush", po::value<size_t>(&tx_flush_length)->default_value(0), "samples to flush TX with after burst")
        ("tx-burst-separation", po::value<double>(&tx_time_between_bursts), "seconds between TX bursts")
        ("interactive-sleep", po::value<size_t>(&interactive_sleep)->default_value(10), "interactive sleep period (ms)")
        ("tx-full-scale", po::value<double>(&tx_full_scale)->default_value(0.7), "full-scale TX sample value")
        ("tx-freq", po::value<double>(&tx_freq), "TX frequency (Hz)")
        ("tx-lo-offset", po::value<double>(&tx_lo_offset)->default_value(0.0), "TX LO offset (Hz)")
        ("tx-freq-init", po::value<double>(&tx_freq_init), "initial TX frequency before realising main TX frequency (Hz)")
        ("tx-freq-delay", po::value<double>(&tx_freq_delay), "seconds after which to set main TX frequency (Hz)")
        ("rx-freq", po::value<double>(&rx_freq), "RX frequency (Hz)")
        ("rx-lo-offset", po::value<double>(&rx_lo_offset)->default_value(0.0), "RX LO offset (Hz)")
        ("rx-freq-init", po::value<double>(&rx_freq_init), "initial RX frequency before realising main RX frequency (Hz)")
        ("rx-freq-delay", po::value<double>(&rx_freq_delay), "seconds after which to set main RX frequency (Hz)")
        ("tx-gain", po::value<double>(&tx_gain), "TX gain (Hz)")
        ("rx-gain", po::value<double>(&rx_gain), "RX gain (Hz)")
        ("tx-ant", po::value<std::string>(&tx_ant), "TX antenna")
        ("rx-ant", po::value<std::string>(&rx_ant), "RX antenna")
        ("tx-sleep-delay", po::value<size_t>(&tx_sleep_delay)->default_value(1000), "TX sleep delay (us)")
        ("rx-sleep-delay", po::value<size_t>(&rx_sleep_delay)->default_value(1000), "RX sleep delay (us)")
        ("rx-sample-limit", po::value<size_t>(&rx_sample_limit)->default_value(0), "total number of samples to receive (0 implies continuous streaming)")
        ("rx-file", po::value<std::string>(&rx_file)->default_value(""), "RX capture file path")
        ("set-time", po::value<std::string>(&set_time_mode)->default_value(""), "set mode (now, next_pps, unknown_pps)")
        //("allow-late", "allow late bursts")
        ("drop-late", "drop late bursts")
        ("still-set-rates", "still set rate on unused direction")
        ("rx-single-packets", "receive one packet at a time")
        ("check-rx-time", "check receive timespec rounding")
        ("tx-eob", "use EOB")
        ("tx-timespec", "use TX timespec")
        ("tx-rx-sync", "sync TX timestamps to RX")
        ("final-eob", "send final EOB")
        ("relative-tx", "use relative TX timestamps")
        ("tx-follows-rx", "TX timestamps follow RX")
        ("rx-size-map", "collect size map of RX packets (implies receive one packet at a time)")
        ("interactive", "interactive mode")
        ("recover-late", "recover from excessive late TX packets")
        ("disable-async", "disable the async message thread")
        ("interleave-rx-file-samples", "interleave individual samples (default is interleaving buffers)")
        ("ignore-late-start", "continue receiving even if stream command was late")
        ("ignore-bad-packets", "continue receiving after a bad packet")
        ("ignore-timeout", "continue receiving after timeout")
        ("ignore-unexpected", "continue receiving after unexpected error")
        // record TX/RX times
        // Optional interruption
        // simulate u / o at random / pulses
        // exit on O / other error
        // suppress msgs
        // recv/send jitter
        // capture each channel to separate files (if format string is spec'd)
        // check sensors
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("rx-rate") == 0)
        rx_rate = rate;
    if (vm.count("tx-rate") == 0)
        tx_rate = rate;
    //if (vm.count("rx-channels") == 0)
    //    rx_channel_list = channel_list;
    //if (vm.count("tx-channels") == 0)
    //    tx_channel_list = channel_list;
    if ((vm.count("rx-channels") == 0) && (vm.count("tx-channels") == 0))
        rx_channel_list = tx_channel_list = channel_list;
    if (vm.count("rx-progress-interval") == 0)
        rx_progress_interval = progress_interval;
    if (vm.count("tx-progress-interval") == 0)
        tx_progress_interval = progress_interval;

    //print the help message
    if (vm.count("help") or ((rx_rate + tx_rate) == 0)){
        std::cout << boost::format("UHD Kitchen Sink %s") % desc << std::endl;
        std::cout <<
        "    By default, performs single-channel full-duplex test at 1 Msps with continuous streaming.\n"
        "    Specify --channels to set RX & TX,\n"
        "        or just --rx-channels and/or --tx-channels.\n"
        "    Specify --rate to set both RX & TX.\n"
        "    Specify --rx-rate to set custom RX rate.\n"
        "    Specify --tx-rate to set custom TX rate.\n"
        << std::endl;
        return ~0;
    }

    bool allow_late_bursts = (/*vm.count("allow-late") > 0*/vm.count("drop-late") == 0);
    bool still_set_rates = (vm.count("still-set-rates") > 0);
    bool recv_single_packets = (vm.count("rx-single-packets") > 0);
    bool check_recv_time = (vm.count("check-rx-time") > 0);
    bool use_tx_eob = (vm.count("tx-eob") > 0);
    bool use_tx_timespec = (vm.count("tx-timespec") > 0);
    bool tx_rx_sync = (vm.count("tx-rx-sync") > 0);
    bool send_final_eob = (vm.count("final-eob") > 0);
    bool use_relative_tx_timestamps = (vm.count("relative-tx") > 0);
    bool tx_follows_rx = (vm.count("tx-follows-rx") > 0);
    bool rx_size_map = (vm.count("rx-size-map") > 0);
    bool interactive = (vm.count("interactive") > 0);
    bool recover_late = (vm.count("recover-late") > 0);
    bool enable_async = (vm.count("disable-async") == 0);
    bool interleave_rx_file_samples = (vm.count("interleave-rx-file-samples") > 0);
    bool ignore_late_start = (vm.count("ignore-late-start") > 0);
    bool ignore_bad_packets = (vm.count("ignore-bad-packets") > 0);
    bool ignore_timeout = (vm.count("ignore-timeout") > 0);
    bool ignore_unexpected_error = (vm.count("ignore-unexpected") > 0);

    boost::posix_time::time_duration interrupt_timeout_duration(boost::posix_time::seconds(long(interrupt_timeout)) + boost::posix_time::microseconds(long((interrupt_timeout - floor(interrupt_timeout))*1e6)));

    if (interactive)
    {
        //WINDOW* window = initscr();
        //newterm(NULL, stdin, NULL);
        //cbreak();
        //noecho();
        //nonl();
        //intrflush(window, FALSE);
        //keypad(window, TRUE);   // Enable function keys, arrow keys, ...
        //nodelay(window, 0);
        //timeout(interactive_sleep);
        set_nonblock(true);
    }

    try
    {
        //create a usrp device
        uhd::device_addrs_t device_addrs = uhd::device::find(args);
        if (not device_addrs.empty() and device_addrs.at(0).get("type", "") == "usrp1"){
            std::cerr << HEADER_WARN"Benchmark results will be inaccurate on USRP1 due to insufficient hardware features.\n" << std::endl;
        }
        std::cout << boost::format(HEADER "Creating the usrp device with args \"%s\"") % args << std::endl;
        uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
        std::cout << std::endl;
        std::cout << boost::format(HEADER "Using Device: %s") % usrp->get_pp_string() << std::endl;

        //std::vector<size_t> channel_nums = get_channels(channel_list);
        std::vector<size_t> rx_channel_nums = get_channels((/*rx_channel_list.size() ? */rx_channel_list/* : channel_list*/), usrp->get_rx_num_channels());
        std::vector<size_t> tx_channel_nums = get_channels((/*tx_channel_list.size() ? */tx_channel_list/* : channel_list*/), usrp->get_tx_num_channels());

        if ((rx_channel_nums.size() == 0) && (tx_channel_nums.size() == 0))
        {
            std::cout << HEADER_ERROR "Need at least one RX or one TX channel to run" << std::endl;
            return ~0;
        }

        bool rx_filename_has_format = false;
        if (rx_channel_nums.size() > 0)
        {
            std::string str0;
            try
            {
                str0 = boost::str(boost::format(rx_file) % 0);
                rx_filename_has_format = true;
            }
            catch (...)
            {
            }

            bool format_different = false;
            try
            {
                std::string str1(boost::str(boost::format(rx_file) % 1));
                format_different = (str0 != str1);
            }
            catch (...)
            {
            }

            if ((rx_filename_has_format) && (format_different == false))
            {
                std::cout << HEADER_ERROR "Multi-channel RX capture filename format did not produce unique names" << std::endl;
                return ~0;
            }
        }

        if ((tx_rx_sync) || (tx_follows_rx))
        {
            if (tx_channel_nums.size() == 0)
            {
                std::cout << HEADER_ERROR "Cannot sync/follow TX to RX without any TX channels" << std::endl;
                return ~0;
            }
            if (rx_channel_nums.size() == 0)
            {
                std::cout << HEADER_ERROR "Cannot sync/follow TX to RX without any RX channels" << std::endl;
                return ~0;
            }
        }

        if (master_clock_rate > 0)
        {
            std::cout << boost::format(HEADER "Requested master clock rate: %f") % master_clock_rate << std::endl;
            usrp->set_master_clock_rate(master_clock_rate);
        }

        std::cout << boost::format(HEADER "Actual master clock rate: %f") % usrp->get_master_clock_rate() << std::endl;

        if (mode == "mimo") // FIXME: Warn if time/clock sources manually set
        {
            usrp->set_clock_source("mimo", 0);  // FIXME: Check this (that it's specific to mboard 0)
            usrp->set_time_source("mimo", 0);

            std::cout << HEADER "Sleeping after setting clock & time sources" << std::endl;
            boost::this_thread::sleep(boost::posix_time::seconds(1));   // MAGIC
        }
        else
        {
            if (clock_source.empty() == false)	// Set clock first (stable clock for PPS registration)
            {
                usrp->set_clock_source(clock_source);
                std::cout << boost::format(HEADER "Clock source set to: %s") % clock_source << std::endl;
            }

            if (time_source.empty() == false)
            {
                usrp->set_time_source(time_source);
                std::cout << boost::format(HEADER "Time source set to: %s") % time_source << std::endl;
            }

            if (set_time_mode.empty() == false)
            {
                if (set_time_mode == "now")
                {
                    usrp->set_time_now(uhd::time_spec_t(0.0));
                    std::cout << boost::format(HEADER "Time set now") << std::endl;
                }
                else if (set_time_mode == "next_pps")
                {
                    usrp->set_time_next_pps(uhd::time_spec_t(0.0));
                    sleep(1);
                    std::cout << boost::format(HEADER "Time set next PPS") << std::endl;
                }
                else if (set_time_mode == "unknown_pps")
                {
                    usrp->set_time_unknown_pps(uhd::time_spec_t(0.0));
                    std::cout << boost::format(HEADER "Time set unknown PPS") << std::endl;
                }
                else
                {
                    std::cout << HEADER_WARN"Cannot set time with unknown mode: " << set_time_mode << std::endl;
                }
            }
        }

        if ((rx_channel_nums.size() > 0) || (still_set_rates))
        {
            if (rx_subdev.empty() == false)
            {
                usrp->set_rx_subdev_spec(rx_subdev);
                std::cout << boost::format(HEADER_RX"RX sub-device spec: %s") % usrp->get_rx_subdev_spec().to_string() << std::endl;
            }
            
            usrp->set_rx_rate(rx_rate);
            double actual_rx_rate = usrp->get_rx_rate();
            std::cout << boost::format(HEADER_RX"Actual RX rate: %f") % actual_rx_rate << std::endl;

            if (rx_ant.empty() == false)
            {
                std::cout << boost::format(HEADER_RX"Selecting RX antenna: %s") % rx_ant << std::endl;
                for (size_t ch = 0; ch < rx_channel_nums.size(); ch++)
                    usrp->set_rx_antenna(rx_ant, ch);
            }

            if (vm.count("rx-freq-init") > 0)
            {
                std::cout << boost::format(HEADER_RX"Setting initial RX freq: %f (LO offset: %f Hz)") % rx_freq_init % rx_lo_offset << std::endl;
                uhd::tune_request_t tune_request = uhd::tune_request_t(rx_freq_init, rx_lo_offset);
                for (size_t ch = 0; ch < rx_channel_nums.size(); ch++)
                    usrp->set_rx_freq(tune_request, ch);
            }

            if (vm.count("rx-gain") > 0)
            {
                std::cout << boost::format(HEADER_RX"Setting RX gain: %f") % rx_gain << std::endl;
                for (size_t ch = 0; ch < rx_channel_nums.size(); ch++)
                    usrp->set_rx_gain(rx_gain, ch);
            }
        }

        if ((tx_channel_nums.size() > 0) || (still_set_rates))
        {
            if (tx_subdev.empty() == false)
            {
                usrp->set_tx_subdev_spec(tx_subdev);
                std::cout << boost::format(HEADER_TX"TX sub-device spec: %s") % usrp->get_tx_subdev_spec().to_string() << std::endl;
            }
            
            usrp->set_tx_rate(tx_rate);
            double actual_tx_rate = usrp->get_tx_rate();
            std::cout << boost::format(HEADER_TX"Actual TX rate: %f") % actual_tx_rate << std::endl;

            if (tx_ant.empty() == false)
            {
                std::cout << boost::format(HEADER_TX"Selecting TX antenna: %s") % tx_ant << std::endl;
                for (size_t ch = 0; ch < tx_channel_nums.size(); ch++)
                    usrp->set_tx_antenna(tx_ant, ch);
            }

            if (vm.count("tx-freq-init") > 0)
            {
                std::cout << boost::format(HEADER_TX"Setting initial TX freq: %f Hz (LO offset: %f Hz)") % tx_freq_init % tx_lo_offset << std::endl;
                uhd::tune_request_t tune_request = uhd::tune_request_t(tx_freq_init, tx_lo_offset);
                for (size_t ch = 0; ch < tx_channel_nums.size(); ch++)
                    usrp->set_tx_freq(tune_request, ch);
            }

            if (vm.count("tx-gain") > 0)
            {
                std::cout << boost::format(HEADER_TX"Setting TX gain: %f") % tx_gain << std::endl;
                for (size_t ch = 0; ch < tx_channel_nums.size(); ch++)
                    usrp->set_tx_gain(tx_gain, ch);
            }
        }

        if (usrp->get_time_source(0) == "gpsdo")
        {
            std::cout << boost::format(HEADER "Waiting for GPSDO time to latch") << std::endl;
            sleep(1);
        }

        uhd::time_spec_t time_start = usrp->get_time_now();	// Usually DSP #0 on mboard #0

        std::cout << boost::format(HEADER "Time now:  %f seconds (%llu ticks)") % time_start.get_real_secs() % time_start.to_ticks(usrp->get_master_clock_rate()) << std::endl;

        boost::thread_group thread_group;

        {
            boost::mutex::scoped_lock l_tx(begin_tx_mutex);
            boost::mutex::scoped_lock l_rx(begin_rx_mutex);

            TX_PARAMS tx_params;
            RX_PARAMS rx_params;

            if (rx_channel_nums.size() > 0)
            {
                //create a receive streamer
                size_t bytes_per_rx_sample = uhd::convert::get_bytes_per_item(rx_cpu);
                std::cout << boost::format(HEADER_RX"CPU bytes per RX sample: %d for '%s'") % bytes_per_rx_sample % rx_cpu << std::endl;
                size_t wire_bytes_per_rx_sample = uhd::convert::get_bytes_per_item(rx_otw);
                std::cout << boost::format(HEADER_RX"OTW bytes per RX sample: %d for '%s'") % wire_bytes_per_rx_sample % rx_otw << std::endl;

                uhd::stream_args_t rx_stream_args(rx_cpu, rx_otw);
                rx_stream_args.channels = rx_channel_nums;
                if (samps_per_packet > 0)
                {
                    std::cout << boost::format(HEADER_RX"Samples per RX packet requested: %d") % samps_per_packet << std::endl;
                    rx_stream_args.args["spp"] = str(boost::format("%d") % samps_per_packet);
                }
                uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(rx_stream_args);
                samps_per_packet = rx_stream->get_max_num_samps();
                std::cout << boost::format(HEADER_RX"Max samples per RX packet: %d") % samps_per_packet << std::endl;

                if (samps_per_buff > 0)
                {
                    std::cout << boost::format(HEADER_RX"RX buffer size requested to be (samples): %d") % samps_per_buff << std::endl;
                }
                else
                {
                    std::cout << HEADER_RX"RX buffer size will accommodate one RX packet" << std::endl;
                    samps_per_buff = samps_per_packet;
                }

                std::cout << boost::format(HEADER_RX"RX buffer size (samples): %d") % samps_per_buff << std::endl;

                if (recv_start_delay > 0)
                    std::cout << boost::format(HEADER_RX"RX streaming will begin in: %d seconds") % recv_start_delay << std::endl;
                else
                    std::cout << boost::format(HEADER_RX"RX streaming will begin immediately") << std::endl;

                std::cout << boost::format(HEADER_RX"RX streamer timeout: %f seconds") % recv_timeout << std::endl;
                if (recv_start_delay > 0)
                    std::cout << boost::format(HEADER_RX"Initial RX streamer timeout: %f seconds") % (recv_timeout + recv_start_delay) << std::endl;

                if ((recv_single_packets) && (samps_per_buff > samps_per_packet))
                    std::cout << HEADER_RX"Will receive single packets" << std::endl;
                else
                    std::cout << HEADER_RX"Will receive as much as can fit in one host-side buffer" << std::endl;

                if (rx_sample_limit > 0)
                {
                    std::cout << boost::format(HEADER_RX"Will receive a total of %ld samples") % rx_sample_limit << std::endl;
                    const size_t upper_rx_sample_limit = 0x0FFFFFFF;
                    if (rx_sample_limit > upper_rx_sample_limit)
                        std::cout << boost::format(HEADER_WARN"Total number of requested samples (%ld) is greater than limit (%ld)") % rx_sample_limit % upper_rx_sample_limit << std::endl;
                }

                if (rx_size_map)
                {
                    if (recv_single_packets == false)
                    {
                        recv_single_packets = true;
                    }
                }

                if (rx_file.empty() == false)
                {
                    if (rx_filename_has_format == false)
                    {
                        if (rx_stream->get_num_channels() == 1)
                        {
                            std::cout << boost::format(HEADER_RX"Capturing single channel to \"%s\"") % rx_file << std::endl;
                        }
                        else
                        {
                            if (interleave_rx_file_samples)
                                std::cout << boost::format(HEADER_RX"Capturing all %d channels as interleaved samples to \"%s\"") % rx_stream->get_num_channels() % rx_file << std::endl;
                            else
                                std::cout << boost::format(HEADER_RX"Capturing all %d channels as interleaved buffers to \"%s\"") % rx_stream->get_num_channels() % rx_file << std::endl;
                        }

                        rx_params.capture_files.push_back(new std::ofstream(rx_file.c_str(), std::ios::out));
                    }
                    else
                    {
                        for (size_t n = 0; n < rx_stream->get_num_channels(); ++n)
                        {
                            std::cout << boost::format(HEADER_RX"Capturing channel %d to \"%s\"") % n % (boost::str(boost::format(rx_file) % n)) << std::endl;
                            std::string rx_file_name(boost::str(boost::format(rx_file) % n));
                            rx_params.capture_files.push_back(new std::ofstream(rx_file_name.c_str(), std::ios::out));
                        }
                    }
                }

                std::cout << boost::format(
                    HEADER_RX"Testing receive rate %f Msps on %u channels: %s"
                ) % (usrp->get_rx_rate()/1e6) % rx_stream->get_num_channels() % rx_channel_list << std::endl;

                rx_params.samps_per_packet = samps_per_packet;
                rx_params.samps_per_buff = samps_per_buff;
                rx_params.start_time = time_start;
                rx_params.start_time_delay = recv_start_delay;
                rx_params.recv_timeout = recv_timeout;
                rx_params.one_packet_at_a_time = recv_single_packets;
                rx_params.check_recv_time = check_recv_time;
                rx_params.progress_interval = rx_progress_interval;
                rx_params.size_map = rx_size_map;
                rx_params.rx_sample_limit = rx_sample_limit;
                rx_params.set_rx_freq = (vm.count("rx-freq") > 0);
                rx_params.rx_freq = rx_freq;
                rx_params.rx_freq_delay = rx_freq_delay;
                rx_params.rx_lo_offset = rx_lo_offset;
                rx_params.interleave_rx_file_samples = interleave_rx_file_samples;
                rx_params.ignore_late_start = ignore_late_start;
                rx_params.ignore_bad_packets = ignore_bad_packets;
                rx_params.ignore_timeout = ignore_timeout;
                rx_params.ignore_unexpected_error = ignore_unexpected_error;

                thread_group.create_thread(boost::bind(
                    &benchmark_rx_rate,
                    usrp,
                    rx_cpu,
                    rx_stream,
                    rx_params));
            }

            if (tx_channel_nums.size() > 0) {
                //create a transmit streamer
                size_t bytes_per_tx_sample = uhd::convert::get_bytes_per_item(tx_cpu);
                std::cout << boost::format(HEADER_TX"CPU bytes per TX sample: %d for '%s'") % bytes_per_tx_sample % tx_cpu << std::endl;
                size_t wire_bytes_per_tx_sample = uhd::convert::get_bytes_per_item(tx_otw);
                std::cout << boost::format(HEADER_TX"OTW bytes per TX sample: %d for '%s'") % wire_bytes_per_tx_sample % tx_otw << std::endl;

                uhd::stream_args_t tx_stream_args(tx_cpu, tx_otw);
                tx_stream_args.channels = tx_channel_nums;
                /*
                 * In the "next_burst" mode, the DSP drops incoming packets until a new burst is started.
                 * In the "next_packet" mode, the DSP starts transmitting again at the next packet.
                 */
                if (allow_late_bursts == false)
                {
                    std::cout << HEADER_TX"Underflow policy set to drop late bursts ('next_burst')" << std::endl;
                    tx_stream_args.args["underflow_policy"] = "next_burst";
                }
                else
                    std::cout << HEADER_TX"Default underflow policy: allow late bursts ('next_packet')" << std::endl;

                uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(tx_stream_args);

                std::cout << boost::format(HEADER_TX"Max TX samples per packet: %d") % tx_stream->get_max_num_samps() << std::endl;

                std::cout << boost::format(
                    HEADER_TX"Testing transmit rate %f Msps on %u channels: %s"
                ) % (usrp->get_tx_rate()/1e6) % tx_stream->get_num_channels() % tx_channel_list << std::endl;

                if ((send_start_delay > 0) && (use_tx_timespec == false))   // FIXME: Don't display warnings if tx-follows-rx
                {
                    std::cout << HEADER_WARN"Send start delay ignored as not using TX timespec" << std::endl;
                }
                else if (((send_start_delay <= 0) && (tx_rx_sync == false) && (tx_follows_rx == false)) && (use_tx_timespec))
                {
                    std::cout << HEADER_WARN"Cannot use TX timespec without a TX start delay, TX-RX start sync, or TX following RX" << std::endl;
                }
                else if ((send_start_delay > 0) && (use_tx_timespec))
                {
                    if (use_relative_tx_timestamps)
                        std::cout << HEADER_TX"Will relative timestamps" << std::endl;

                    // FIXME: tx_follows_rx
                }

                if (tx_rx_sync)
                {
                    if (use_tx_timespec == false)
                    {
                        std::cout << HEADER_WARN"Cannot sync to RX when not using TX timespec" << std::endl;
                    }
                    else if (tx_time_offset <= 0)
                    {
                        std::cout << HEADER_WARN"Cannot sync to RX with no TX time offset" << std::endl;
                    }
                }

                if (recover_late)
                {
                    if (tx_time_offset <= 0)
                    {
                        std::cout << HEADER_WARN"TX late recovery will not work with no TX time offset" << std::endl;
                    }
                }

                if (use_tx_eob)
                    std::cout << HEADER_TX"Will use EOB" << std::endl;
                else
                    std::cout << HEADER_TX"Will not use EOB" << std::endl;

                tx_params.start_time = time_start;
                tx_params.send_timeout = send_timeout;
                tx_params.send_start_delay = send_start_delay;
                tx_params.use_tx_eob = use_tx_eob;
                tx_params.use_tx_timespec = use_tx_timespec;
                tx_params.tx_rx_sync = tx_rx_sync;
                tx_params.send_final_eob = send_final_eob;
                tx_params.progress_interval = tx_progress_interval;
                tx_params.use_relative_timestamps = use_relative_tx_timestamps;
                tx_params.follow_rx_timestamps = tx_follows_rx;
                tx_params.tx_time_offset = tx_time_offset;
                tx_params.rx_rate = rx_rate;    // FIXME: Check validity
                tx_params.tx_burst_length = tx_burst_length;
                tx_params.tx_flush_length = tx_flush_length;
                tx_params.tx_full_scale = tx_full_scale;
                tx_params.tx_time_between_bursts = tx_time_between_bursts;
                tx_params.recover_late = recover_late;
                tx_params.set_tx_freq = (vm.count("tx-freq") > 0);
                tx_params.tx_freq = tx_freq;
                tx_params.tx_freq_delay = tx_freq_delay;
                tx_params.tx_lo_offset = tx_lo_offset;

                thread_group.create_thread(boost::bind(&benchmark_tx_rate,
                    usrp,
                    tx_cpu,
                    tx_stream,
                    tx_params));

                if (enable_async)
                {
                    thread_group.create_thread(boost::bind(&benchmark_tx_rate_async_helper,
                        tx_stream,
                        tx_async_timeout));
                }
            }

            running = true;
            std::cout << HEADER "Begin..." << std::endl;

            thread_group.create_thread(boost::bind(&check_thread, usrp));

            if (tx_channel_nums.size() > 0)
                tx_thread_begin.wait(l_tx);
            if (rx_channel_nums.size() > 0)
                rx_thread_begin.wait(l_rx);

            std::signal(SIGINT, &sig_int_handler);

            uhd::msg::register_handler(&msg_handler);

            begin.notify_all();

            // RTT is longer, so skip this
            //time_start = usrp->get_time_now();	// Usually DSP #0 on mboard #0
            //tx_params.start_time = rx_params.start_time = time_start;   // Update to ignore thread start-up time
            //std::cout << boost::format("Time now:  %f seconds (%llu ticks)") % time_start.get_real_secs() % time_start.to_ticks(usrp->get_master_clock_rate()) << std::endl;
        }   // TX/RX locks will be released

        std::cout << HEADER << "(" << get_stringified_time() << ") Running..." << std::endl;

        boost::mutex::scoped_lock l_stop(stop_mutex);
        if (stop_signal_called == false)
        {
            if ((rx_sample_limit > 0) && (tx_channel_nums.size() == 0))
            {
                rx_thread_complete.wait(l_stop);
            }
            else if (interactive)
            {
                if (duration > 0)
                {
                    // FIXME: Stop time

                    std::cout << HEADER "Waiting for Q to finish early..." << std::endl;
                }
                else
                    std::cout << HEADER "Waiting for Q..." << std::endl;

                do
                {
                    // FIXME: Stop time

                    if (kbhit(0))
                    {
                        char c = fgetc(stdin);
                        if (c == EOF)
                        {
                            std::cout << HEADER "EOF" << std::endl;
                            break;
                        }

                        if (tolower(c) == 'q')
                            break;

                        switch (c)
                        {
                            case 'L':
                            case 'U':
                                break;
                            case 'l':
                            case 'u':
                                tx_sleep_delay_now = tx_sleep_delay;
                                break;
                            case 'O':
                                break;
                            case 'o':
                                rx_sleep_delay_now = rx_sleep_delay;
                                break;
                        }
                    }

                    print_msgs();
                    
                    abort_event.timed_wait(l_stop, boost::posix_time::milliseconds(interactive_sleep));
                } while (stop_signal_called == false);
            }
            else if (duration > 0)
            {
                //sleep for the required duration
                std::cout << boost::format(HEADER "Main thread sleeping for: %f seconds (host wall clock)") % duration << std::endl;
                std::cout << HEADER "Waiting for CTRL+C to finish early..." << std::endl;
                const long secs = long(duration);
                const long usecs = long((duration - secs)*1e6);

                abort_event.timed_wait(l_stop, boost::posix_time::seconds(secs) + boost::posix_time::microseconds(usecs));
                //boost::this_thread::sleep(boost::posix_time::seconds(secs) + boost::posix_time::microseconds(usecs));
            }
            else
            {
                std::cout << HEADER "Waiting for CTRL+C..." << std::endl;
                abort_event.wait(l_stop);
            }
        }

        running = false;

        std::cout << HEADER << "(" << get_stringified_time() << ") Stopping..." << std::endl;

        // FIXME: Timed wait & re-enable interruptions

        if (rx_channel_nums.size() > 0) {
            std::cout << HEADER "Waiting for RX thread..." << std::endl;
            boost::mutex::scoped_lock l(begin_rx_mutex);
            while (rx_thread_finished == false)
            {
                if (interrupt_timeout == 0)
                    rx_thread_complete.wait(l);
                else
                {
                    if (rx_thread_complete.timed_wait(l, interrupt_timeout_duration) == false)
                    {
                        if (rx_interrupt_disabler)
                        {
                            std::cout << HEADER "Interrupting RX thread..." << std::endl;
                            delete rx_interrupt_disabler;
                            rx_interrupt_disabler = NULL;
                            thread_group.interrupt_all();
                        }
                        else
                        {
                            std::cout << HEADER_WARN"Interrupting RX thread failed - giving up" << std::endl;
                            break;
                        }
                    }
                }
            }
        }

        if (tx_channel_nums.size() > 0) {
            std::cout << HEADER "Waiting for TX thread..." << std::endl;
            boost::mutex::scoped_lock l(begin_tx_mutex);
            while (tx_thread_finished == false)
            {
                if (interrupt_timeout == 0)
                    tx_thread_complete.wait(l);
                else
                {
                    if (tx_thread_complete.timed_wait(l, interrupt_timeout_duration) == false)
                    {
                        if (tx_interrupt_disabler)
                        {
                            std::cout << HEADER "Interrupting TX thread..." << std::endl;
                            delete tx_interrupt_disabler;
                            tx_interrupt_disabler = NULL;
                            thread_group.interrupt_all();
                        }
                        else
                        {
                            std::cout << HEADER_WARN"Interrupting TX thread failed - giving up" << std::endl;
                            break;
                        }
                    }
                }
            }

            if (enable_async)
            {
                std::cout << HEADER "Waiting for TX async message thread..." << std::endl;
                boost::mutex::scoped_lock l_async(begin_tx_async_begin);
                while (tx_async_thread_finished == false)
                {
                    if (interrupt_timeout == 0)
                        tx_async_thread_complete.wait(l_async);
                    else
                    {
                        if (tx_async_thread_complete.timed_wait(l_async, interrupt_timeout_duration) == false)
                        {
                            if (tx_async_interrupt_disabler)
                            {
                                std::cout << HEADER "Interrupting TX async thread..." << std::endl;
                                delete tx_async_interrupt_disabler;
                                tx_async_interrupt_disabler = NULL;
                                thread_group.interrupt_all();
                            }
                            else
                            {
                                std::cout << HEADER_WARN"Interrupting TX async thread failed - giving up" << std::endl;
                                break;
                            }
                        }
                    }
                }
            }
        }

        //interrupt and join the threads
        thread_group.interrupt_all();
        std::cout << HEADER "Waiting for threads to join..." << std::endl;
        thread_group.join_all();
    }
    catch (const std::runtime_error& e)
    {
        std::cout << HEADER_ERROR "Unhandled exception: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << HEADER_ERROR "Caught an unknown exception" << std::endl;
    }

    //print summary
    std::cout << std::endl << boost::format(
        "Test summary:\n"
        "  Num received samples:              %u\n"
        "  Num dropped samples:               %u\n"
        "  Num overflows detected:            %u\n"
        "\n"
        "  Num transmitted samples:           %u\n"
        "  Num send calls:                    %u\n"
        "  Num ACKs:                          %u\n"
        "  Num sequence errors:               %u\n"
        "  Num sequence in burst errors:      %u\n"
        "  Num sequence errors (total):       %u\n"
        "  Num underflows detected:           %u\n"
        "  Num underflows in packet detected: %u\n"
        "  Num underflows detected (total):   %u\n"
        "  Num late packets:                  %u\n"
    ) % num_rx_samps % num_dropped_samps % num_overflows % num_tx_samps % num_send_calls % num_tx_acks % num_seq_errors % num_seq_errors_in_burst % (num_seq_errors + num_seq_errors_in_burst) % num_underflows % num_underflows_in_packet % (num_underflows + num_underflows_in_packet) % num_late_packets;

    //finished
    std::cout << std::endl << "Done!" << std::endl;

    if (interactive)
    {
        set_nonblock(false);
        //endwin();
    }

    return EXIT_SUCCESS;
}

//
// Copyright 2010-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "Responder.hpp"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <complex>
#include <csignal>
#include <cmath>
#include <sstream>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/filesystem.hpp>
#include <uhd/utils/thread.hpp>
#include <uhd/property_tree.hpp>

const std::string _eth_file("eths_info.txt");


// Redirect output to stderr
struct cerr_redirect {
    cerr_redirect( std::streambuf * new_buffer ) 
        : old( std::cerr.rdbuf( new_buffer ) )
    { }

    ~cerr_redirect( ) {
        std::cerr.rdbuf( old );
    }

private:
    std::streambuf * old;
};



// Catch keyboard interrupts for clean manual abort
static bool s_stop_signal_called = false;
static int s_signal = 0;
static void sig_int_handler(int signal)
{
    s_stop_signal_called = true;
    s_signal = signal;
}

// member of Responder to register sig int handler
void
Responder::register_stop_signal_handler()
{
    std::signal(SIGINT, &sig_int_handler);
}

// For ncurses. Print everything in stream to screen
void
Responder::FLUSH_SCREEN()
{
    printw(_ss.str().c_str());
    refresh();
    _ss.str("");
}

// Like FLUSH_SCREEN but with new line
void
Responder::FLUSH_SCREEN_NL()
{
    do {
        int y, x;
        getyx(_window, y, x);
        if (x > 0){
            printw("\n");
            y++;
        }
        FLUSH_SCREEN();
    } while (0);
}

// Constructor
Responder::Responder( Options& opt)
    : _opt(opt),
    _stats_filename(opt.stats_filename),
    _delay(opt.delay),
    _samps_per_packet(opt.samps_per_packet),
    _delay_step(opt.delay_step),
    _simulate_frequency(opt.simulate_frequency),
    _allow_late_bursts(opt.allow_late_bursts),
    _no_delay(opt.no_delay),
    //Initialize atributes not given by Options
    _num_total_samps(0), // printed on exit
    _overruns(0), // printed on exit
    _max_success(0), // < 0 --> write results to file
    _return_code(RETCODE_OK),
    _stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS),
    _timeout_burst_count(0),
    _timeout_eob_count(0),
    _y_delay_pos(-1),
    _x_delay_pos(-1), // Remember the cursor position of delay line.
    _last_overrun_count(0)
{
    time( &_dbginfo.start_time ); // for debugging

    // Disable logging to console
    uhd::log::set_console_level(uhd::log::off);

    if (uhd::set_thread_priority_safe(_opt.rt_priority, _opt.realtime) == false) // try to set realtime scheduling
    {
        cerr << "Failed to set real-time" << endl;
    }

    _return_code = calculate_dependent_values();


    // From this point on, everything is written to a ncurses window!
    create_ncurses_window();

    print_create_usrp_msg();
    try
    {
        _usrp = create_usrp_device();
    }
    catch (const std::runtime_error& e)
    {
        print_msg(e.what() );
        _return_code = RETCODE_RUNTIME_ERROR;
    }
    catch(...){
        print_msg("unhandled ERROR");
        _return_code = RETCODE_UNKNOWN_EXCEPTION;
        print_msg_and_wait("create USRP device failed!\nPress key to abort test...");
        return;
    }

    // Prepare array with response burst data.
    _pResponse = alloc_response_buffer_with_data(_response_length);

    // ensure that filename is set
    string test_id = _usrp->get_mboard_name();
    if (set_stats_filename(test_id) )
    {
        _return_code = RETCODE_BAD_ARGS; // make sure run() does return!
        FLUSH_SCREEN();
        if (_opt.batch_mode == false)
        {
            print_msg_and_wait("Press any key to end...");
        }
        return;
    }

    cerr_redirect(_ss_cerr.rdbuf());
    register_stop_signal_handler();
}

int
Responder::calculate_dependent_values()
{
    _response_length = _opt.response_length();
    _init_delay_count = (int64_t)(_opt.sample_rate * _opt.init_delay);
    _dc_offset_countdown = (int64_t)(_opt.sample_rate * _opt.dc_offset_delay);
    _level_calibration_countdown = (int64_t)_opt.level_calibration_count();
    _original_simulate_duration = _simulate_duration = _opt.simulate_duration(_simulate_frequency);

    if (_simulate_duration > 0)
    {
        // Skip settling period and calibration
        _init_delay_count = 0;
        _dc_offset_countdown = 0;
        _level_calibration_countdown = 0;

        double highest_delay = 0.0;
        if (_opt.test_iterations > 0)
            highest_delay = max(_opt.delay_max, _opt.delay_min);
        else if (_no_delay == false)
            highest_delay = _delay;

        uint64_t highest_delay_samples = _opt.highest_delay_samples(highest_delay);
        if ((highest_delay_samples + _response_length + _opt.flush_count) > _simulate_duration)
        {
            if (_opt.adjust_simulation_rate) // This is now done DURING the simulation based on active delay
            {
                //_simulate_frequency = max_possible_rate;
                //_simulate_duration = (uint64_t)((double)sample_rate / _simulate_frequency);
            }
            else
            {
                cerr << boost::format("Highest delay and response duration will exceed the pulse simulation rate (%ld + %ld > %ld samples)") % highest_delay_samples % _response_length % _simulate_duration << endl;
                int max_possible_rate = (int) get_max_possible_frequency(highest_delay_samples, _response_length);
                double max_possible_delay = (double)(_simulate_duration - (_response_length + _opt.flush_count)) / (double)_opt.sample_rate;
                cerr << boost::format("Simulation rate must be less than %i Hz, or maximum delay must be less than %f s") % max_possible_rate % max_possible_delay << endl;

                if (_opt.ignore_simulation_check == 0)
                    return RETCODE_BAD_ARGS;
            }
        }
    }
    else
    {
        boost::format fmt("Simulation frequency too high (%f Hz with sample_rate %f Msps)");
        fmt % _simulate_frequency % (_opt.sample_rate/1e6);
        cerr << fmt << endl;
        return RETCODE_BAD_ARGS;
    }

    if (_opt.test_iterations > 0)    // Force certain settings during test mode
    {
        _no_delay = false;
        _allow_late_bursts = false;
        _delay = _opt.delay_min;
    }
    return RETCODE_OK; // default return code
}

// print test title to ncurses window
void
Responder::print_test_title()
{
    if (_opt.test_title.empty() == false)
    {
        std::string title(_opt.test_title);
        boost::replace_all(title, "%", "%%");
        print_msg(title + "\n");
    }
}

void
Responder::print_usrp_status()
{
    std::string msg;
    msg += (boost::format("Using device:\n%s\n") % _usrp->get_pp_string() ).str();
    msg += (boost::format("Setting RX rate: %f Msps\n") % (_opt.sample_rate/1e6)).str();
    msg += (boost::format("Actual RX rate:  %f Msps\n") % (_usrp->get_rx_rate()/1e6) ).str();
    msg += (boost::format("Setting TX rate: %f Msps\n") % (_opt.sample_rate/1e6) ).str();
    msg += (boost::format("Actual TX rate:  %f Msps") % (_usrp->get_tx_rate()/1e6) ).str();
    print_msg(msg);
    print_tx_stream_status();
    print_rx_stream_status();
}

void
Responder::print_test_parameters()
{
    // Some status output shoud be printed here!
    size_t rx_max_num_samps = _rx_stream->get_max_num_samps();
    size_t tx_max_num_samps = _tx_stream->get_max_num_samps();
    std::string msg;

    msg += (boost::format("Samples per buffer: %d\n") % _opt.samps_per_buff).str();
    msg += (boost::format("Maximum number of samples: RX = %d, TX = %d\n") % rx_max_num_samps % tx_max_num_samps).str();
    msg += (boost::format("Response length: %ld samples (%f us)") % _response_length % (_opt.response_duration * 1e6) ).str();

    if (_simulate_duration > 0)
        msg += (boost::format("\nSimulating pulses at %f Hz (every %ld samples)") % _simulate_frequency % _simulate_duration ).str();

    if (_opt.test_iterations > 0)
    {
        msg += (boost::format("\nTest coverage: %f -> %f (%f steps)") % _opt.delay_min % _opt.delay_max % _opt.delay_step ).str();

        if (_opt.end_test_after_success_count > 0)
            msg += (boost::format("\nTesting will end after %d successful delays") % _opt.end_test_after_success_count ).str();
    }

    if ((_dc_offset_countdown == 0) && (_simulate_frequency == 0.0))
    {
        msg += "\nDC offset disabled";
    }
    print_msg(msg);
}

// e.g. B200 doesn't support this command. Check if possible and only set rx_dc_offset if available
void
Responder::set_usrp_rx_dc_offset(uhd::usrp::multi_usrp::sptr usrp, bool ena)
{
    uhd::property_tree::sptr tree = usrp->get_device()->get_tree();
    // FIXME: Path needs to be build in a programmatic way.
    bool dc_offset_exists = tree->exists( uhd::fs_path("/mboards/0/rx_frontends/A/dc_offset") );
    if(dc_offset_exists)
    {
        usrp->set_rx_dc_offset(ena);
    }
}

void
Responder::print_create_usrp_msg()
{
    std::string msg("Creating the USRP device");
    if (_opt.device_args.empty() == false)
        msg.append( (boost::format(" with args \"%s\"") % _opt.device_args ).str() );
    msg.append("...");
    print_msg(msg);
}

uhd::usrp::multi_usrp::sptr
Responder::create_usrp_device()
{
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(_opt.device_args);
    usrp->set_rx_rate(_opt.sample_rate); // set the rx sample rate
    usrp->set_tx_rate(_opt.sample_rate); // set the tx sample rate
    _tx_stream = create_tx_streamer(usrp);
    _rx_stream = create_rx_streamer(usrp);
    if ((_dc_offset_countdown == 0) && (_simulate_frequency == 0.0))
        set_usrp_rx_dc_offset(usrp, false);
    return usrp;
}

uhd::rx_streamer::sptr
Responder::create_rx_streamer(uhd::usrp::multi_usrp::sptr usrp)
{
    uhd::stream_args_t stream_args("fc32"); //complex floats
    if (_samps_per_packet > 0)
    {
        stream_args.args["spp"] = str(boost::format("%d") % _samps_per_packet);
    }
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
    _samps_per_packet = rx_stream->get_max_num_samps();

    return rx_stream;
}

void
Responder::print_rx_stream_status()
{
    std::string msg;
    msg += (boost::format("Samples per packet set to: %d\n") % _samps_per_packet).str();
    msg += (boost::format("Flushing burst with %d samples") % _opt.flush_count).str();
    if (_opt.skip_eob)
        msg += "\nSkipping End-Of-Burst";
    print_msg(msg);
}

uhd::tx_streamer::sptr
Responder::create_tx_streamer(uhd::usrp::multi_usrp::sptr usrp)
{
    uhd::stream_args_t tx_stream_args("fc32"); //complex floats
    if (_allow_late_bursts == false)
    {
        tx_stream_args.args["underflow_policy"] = "next_burst";
    }
    uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(tx_stream_args);
    return tx_stream;
}

void
Responder::print_tx_stream_status()
{
    std::string msg;
    if (_allow_late_bursts == false)
    {
        msg += "Underflow policy set to drop late bursts";
    }
    else
        msg += "Underflow policy set to allow late bursts";
    if (_opt.skip_send)
        msg += "\nNOT sending bursts";
    else if (_opt.combine_eob)
        msg += "\nCombining EOB into first send";
    print_msg(msg);
}

// handle transmit timeouts properly
void
Responder::handle_tx_timeout(int burst, int eob)
{
    if(_timeout_burst_count == 0 && _timeout_eob_count == 0)
        time( &_dbginfo.first_send_timeout );
    _timeout_burst_count += burst;
    _timeout_eob_count += eob;
    print_timeout_msg();
}

void
Responder::print_timeout_msg()
{
    move(_y_delay_pos+3, _x_delay_pos);
    print_msg( (boost::format("Send timeout, burst_count = %ld\teob_count = %ld\n") % _timeout_burst_count % _timeout_eob_count ).str() );
}

uhd::tx_metadata_t Responder::get_tx_metadata(uhd::time_spec_t rx_time, size_t n)
{
    uhd::tx_metadata_t tx_md;
    tx_md.start_of_burst = true;
    tx_md.end_of_burst = false;
    if ((_opt.skip_eob == false) && (_opt.combine_eob)) {
        tx_md.end_of_burst = true;
    }

    if (_no_delay == false) {
        tx_md.has_time_spec = true;
        tx_md.time_spec = rx_time + uhd::time_spec_t(0, n, _opt.sample_rate) + uhd::time_spec_t(_delay);
    } else {
        tx_md.has_time_spec = false;
    }
    return tx_md;
}

bool Responder::send_tx_burst(uhd::time_spec_t rx_time, size_t n)
{
    if (_opt.skip_send == true) {
        return false;
    }
    //send a single packet
    uhd::tx_metadata_t tx_md = get_tx_metadata(rx_time, n);
    const size_t length_to_send = _response_length + (_opt.flush_count - (tx_md.end_of_burst ? 0 : 1));

    size_t num_tx_samps = _tx_stream->send(_pResponse, length_to_send, tx_md, _opt.timeout); // send pulse!
    if (num_tx_samps < length_to_send) {
        handle_tx_timeout(1, 0);
    }
    if (_opt.skip_eob == false && _opt.combine_eob == false) {
        tx_md.start_of_burst = false;
        tx_md.end_of_burst = true;
        tx_md.has_time_spec = false;

        const size_t eob_length_to_send = 1;

        size_t eob_num_tx_samps = _tx_stream->send(&_pResponse[length_to_send], eob_length_to_send, tx_md); // send EOB
        if (eob_num_tx_samps < eob_length_to_send) {
            handle_tx_timeout(0, 1);
        }
    }

    return true;
}

// ensure that stats_filename is not empty.
bool
Responder::set_stats_filename(string test_id)
{
    if (_stats_filename.empty())
    {
        string file_friendly_test_id(test_id);
        boost::replace_all(file_friendly_test_id, " ", "_");
        boost::format fmt = boost::format("%slatency-stats.id_%s-rate_%i-spb_%i-spp_%i%s") % _opt.stats_filename_prefix % file_friendly_test_id % (int)_opt.sample_rate % _opt.samps_per_buff % _samps_per_packet % _opt.stats_filename_suffix;
        _stats_filename = str(fmt) + ".txt";
        _stats_log_filename = str(fmt) + ".log";
    }
    return check_for_existing_results();
}

// Check if results file can be overwritten
bool
Responder::check_for_existing_results()
{
    bool ex = false;
    if ((_opt.skip_if_results_exist) && (boost::filesystem::exists(_stats_filename)))
    {
        print_msg( (boost::format("Skipping invocation as results file already exists: %s") %  _stats_filename).str() );
        ex = true;
    }
    return ex;
}

// Allocate an array with a burst response
float*
Responder::alloc_response_buffer_with_data(uint64_t response_length) // flush_count, output_value, output_scale are const
{
    float* pResponse = new float[(response_length + _opt.flush_count) * 2];
    for (unsigned int i = 0; i < (response_length * 2); ++i)
        pResponse[i] = _opt.output_value * _opt.output_scale;
    for (unsigned int i = (response_length * 2); i < ((response_length + _opt.flush_count) * 2); ++i)
        pResponse[i] = 0.0f;
    return pResponse;
}

// print test parameters for current delay time
void
Responder::print_formatted_delay_line(const uint64_t simulate_duration, const uint64_t old_simulate_duration, const STATS& statsPrev, const double delay, const double simulate_frequency)
{
    if(_y_delay_pos < 0 || _x_delay_pos < 0){ // make sure it gets printed to the same position everytime
        getyx(_window, _y_delay_pos, _x_delay_pos);
    }
    double score = 0.0;
    if (statsPrev.detected > 0)
        score = 100.0 * (double)(statsPrev.detected - statsPrev.missed) / (double)statsPrev.detected;
    std::string form;
    boost::format fmt0("Delay now: %.6f (previous delay %.6f scored %.1f%% [%ld / %ld])");
    fmt0 % delay % statsPrev.delay % score % (statsPrev.detected - statsPrev.missed) % statsPrev.detected;
    form += fmt0.str();
    if (old_simulate_duration != simulate_duration)
    {
        boost::format fmt1(" [Simulation rate now: %.1f Hz (%ld samples)]");
        fmt1 % simulate_frequency % simulate_duration;
        form = form + fmt1.str();
    }
    move(_y_delay_pos, _x_delay_pos);
    print_msg(form);
}

// print message and wait for user interaction
void
Responder::print_msg_and_wait(std::string msg)
{
    msg = "\n" + msg;
    print_msg(msg);
    timeout(-1);
    getch();
    timeout(0);
}

// print message to ncurses window
void
Responder::print_msg(std::string msg)
{
    _ss << msg << endl;
    FLUSH_SCREEN();
}

// Check if error occured during call to receive
bool
Responder::handle_rx_errors(uhd::rx_metadata_t::error_code_t err, size_t num_rx_samps)
{
    // handle errors
    if (err == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT)
    {
        std::string msg = (boost::format("Timeout while streaming (received %ld samples)") % _num_total_samps).str();
        print_error_msg(msg);
        _return_code = RETCODE_RECEIVE_TIMEOUT;
        return true;
    }
    else if (err == uhd::rx_metadata_t::ERROR_CODE_BAD_PACKET)
    {
        std::string msg = (boost::format("Bad packet (received %ld samples)") % _num_total_samps).str();
        print_error_msg(msg);
        _return_code = RETCODE_BAD_PACKET;
        return true;
    }
    else if ((num_rx_samps == 0) && (err == uhd::rx_metadata_t::ERROR_CODE_NONE))
    {
        print_error_msg("Received no samples");
        _return_code = RETCODE_RECEIVE_FAILED;
        return true;
    }
    else if (err == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW)
    {
        ++_overruns;
        print_overrun_msg(); // update overrun info on console.
    }
    else if (err != uhd::rx_metadata_t::ERROR_CODE_NONE)
    {
        throw std::runtime_error(str(boost::format(
            "Unexpected error code 0x%x"
        ) % err));
    }
    return false;
}

// print overrun status message.
void
Responder::print_overrun_msg()
{
    if (_num_total_samps > (_last_overrun_count + (uint64_t)(_opt.sample_rate * 1.0)))
    {
        int y, x, y_max, x_max;
        getyx(_window, y, x);
        getmaxyx(_window, y_max, x_max);
        move(y_max-1, 0);
        print_msg( (boost::format("Overruns: %d") % _overruns).str() );
        move(y, x);
        _last_overrun_count = _num_total_samps;
    }
}

// print error message on last line of ncurses window
void
Responder::print_error_msg(std::string msg)
{
    int y, x, y_max, x_max;
    getyx(_window, y, x);
    getmaxyx(_window, y_max, x_max);
    move(y_max-2, 0);
    clrtoeol();
    print_msg(msg);
    move(y, x);
}

// calculate simulate frequency
double
Responder::get_simulate_frequency(double delay, uint64_t response_length, uint64_t original_simulate_duration)
{
    double simulate_frequency = _simulate_frequency;
    uint64_t highest_delay_samples = _opt.highest_delay_samples(delay);
    if ((_opt.optimize_simulation_rate) ||
        ((highest_delay_samples + response_length + _opt.flush_count) > original_simulate_duration))
    {
        simulate_frequency = get_max_possible_frequency(highest_delay_samples, response_length);
    }
    return simulate_frequency;
}

// calculate max possible simulate frequency
double
Responder::get_max_possible_frequency(uint64_t highest_delay_samples, uint64_t response_length) // only 2 args, others are all const!
{
    return std::floor((double)_opt.sample_rate / (double)(highest_delay_samples + response_length + _opt.flush_count + _opt.optimize_padding));
}

// Check if conditions to finish test are met.
bool
Responder::test_finished(size_t success_count)
{
    if (success_count == _opt.end_test_after_success_count)
    {
        print_msg( (boost::format("\nTest complete after %d successes.") % success_count).str() );
        return true;
    }
    if (((_opt.delay_min <= _opt.delay_max) && (_delay >= _opt.delay_max)) ||
        ((_opt.delay_min > _opt.delay_max) && (_delay <= _opt.delay_max)))
    {
        print_msg("\nTest complete.");
        return true;
    }
    return false;
}

// handle keyboard input in interactive mode
bool
Responder::handle_interactive_control()
{
    std::string msg = "";
    int c = wgetch(_window);
    if (c > -1)
    {
        // UP/DOWN Keys control delay step width
        if ((c == KEY_DOWN) || (c == KEY_UP))
        {
            double dMag = log10(_delay_step);
            int iMag = (int)floor(dMag);
            iMag += ((c == KEY_UP) ? 1 : -1);
            _delay_step = pow(10.0, iMag);
            msg += (boost::format("Step: %f") % _delay_step ).str();
        }
        // LEFT/RIGHT Keys control absolute delay length
        if ((c == KEY_LEFT) || (c == KEY_RIGHT))
        {
            double step = _delay_step * ((c == KEY_RIGHT) ? 1 : -1);
            if ((_delay + step) >= 0.0)
                _delay += step;
            msg += (boost::format("Delay: %f") % _delay).str();
        }
        // Enable/disable fixed delay <--> best effort mode
        if (c == 'd')
        {
            _no_delay = !_no_delay;

            if (_no_delay)
                msg += "Delay disabled (best effort)";
            else
                msg += (boost::format("Delay: %f") % _delay).str();
        }
        else if (c == 'q') // exit test
        {
            return true; // signal test to stop
        }
        else if (c == 'l') // change late burst policy
        {
            _allow_late_bursts = !_allow_late_bursts;

            if (_allow_late_bursts)
                msg += "Allowing late bursts";
            else
                msg += "Dropping late bursts";
        }
        print_interactive_msg(msg);
    }
    return false; // signal test to continue with updated values
}

// print updated interactive control value
void
Responder::print_interactive_msg(std::string msg)
{
    if(msg != "")
    {
        // move cursor back to beginning of line
        int y, x;
        getyx(_window, y, x);
        if (x > 0)
        {
            move(y, 0);
            clrtoeol();
        }
        print_msg(msg);
        move(y, 0);
    }
}

// check if transmit burst is late
bool
Responder::tx_burst_is_late()
{
    uhd::async_metadata_t async_md;
    if (_usrp->get_device()->recv_async_msg(async_md, 0))
    {
        if (async_md.event_code == uhd::async_metadata_t::EVENT_CODE_TIME_ERROR)
        {
            return true;
        }
    }
    return false;
}

void
Responder::create_ncurses_window()
{
    _window = initscr();
    cbreak();       // Unbuffered key input, except for signals (cf. 'raw')
    noecho();
    nonl();
    intrflush(_window, FALSE);
    keypad(_window, TRUE);   // Enable function keys, arrow keys, ...
    nodelay(_window, 0);
    timeout(0);
}

// print all fixed test parameters
void
Responder::print_init_test_status()
{
    // Clear the window and write new data.
    erase();
    refresh();
    print_test_title();
    print_usrp_status();
    print_test_parameters();

    std::string msg("");
    if (_opt.test_iterations > 0)
        msg.append("Press Ctrl + C to abort test");
    else
        msg.append("Press Q stop streaming");
    msg.append("\n");
    print_msg(msg);

    _y_delay_pos = -1; // reset delay display line pos.
    _x_delay_pos = -1;
}

// in interactive mode with second usrp sending bursts. calibrate trigger level
float
Responder::calibrate_usrp_for_test_run()
{
    bool calibration_finished = false;
    float threshold = 0.0f;
    double ave_high = 0, ave_low = 0;
    int ave_high_count = 0, ave_low_count = 0;
    bool level_calibration_stage_2 = false; // 1. stage = rough calibration ; 2. stage = fine calibration

    std::vector<std::complex<float> > buff(_opt.samps_per_buff);
    while (not s_stop_signal_called && !calibration_finished && _return_code == RETCODE_OK)
    {
        uhd::rx_metadata_t rx_md;
        size_t num_rx_samps = _rx_stream->recv(&buff.front(), buff.size(), rx_md, _opt.timeout);

        // handle errors
        if(handle_rx_errors(rx_md.error_code, num_rx_samps) )
        {
            break;
        }

        // Wait for USRP for DC offset calibration
        if (_dc_offset_countdown > 0)
        {
            _dc_offset_countdown -= (int64_t)num_rx_samps;
            if (_dc_offset_countdown > 0)
                continue;
            set_usrp_rx_dc_offset(_usrp, false);
            print_msg("DC offset calibration complete");
        }

        // Wait for certain time to minimize POWER UP effects
        if (_init_delay_count > 0)
        {
            _init_delay_count -= (int64_t)num_rx_samps;
            if (_init_delay_count > 0)
                continue;
            print_msg("Initial settling period elapsed");
        }

        ////////////////////////////////////////////////////////////
        // detect falling edges and calibrate detection values
        if (_level_calibration_countdown > 0)
        {
            if (level_calibration_stage_2 == false)
            {
                float average = 0.0f;
                for (size_t n = 0; n < num_rx_samps; n++)
                    average += buff[n].real() * _opt.invert;
                average /= (float)num_rx_samps;

                if (ave_low_count == 0)
                {
                    ave_low = average;
                    ++ave_low_count;
                }
                else if (average < ave_low)
                {
                    ave_low = average;
                    ++ave_low_count;
                }

                if (ave_high_count == 0)
                {
                    ave_high = average;
                    ++ave_high_count;
                }
                else if (average > ave_high)
                {
                    ave_high = average;
                    ++ave_high_count;
                }
            }
            else {
                for (size_t n = 0; n < num_rx_samps; n++)
                {
                    float f = buff[n].real() * _opt.invert;
                    if (f >= threshold)
                    {
                        ave_high += f;
                        ave_high_count++;
                    }
                    else
                    {
                        ave_low += f;
                        ave_low_count++;
                    }
                }
            }

            _level_calibration_countdown -= (int64_t)num_rx_samps;

            if (_level_calibration_countdown <= 0)
            {
                if (level_calibration_stage_2 == false)
                {
                    level_calibration_stage_2 = true;
                    _level_calibration_countdown = _opt.level_calibration_count();
                    threshold = ave_low + ((ave_high - ave_low) / 2.0);
                    print_msg( (boost::format("Phase #1: Ave low: %.3f (#%d), ave high: %.3f (#%d), threshold: %.3f") % ave_low % ave_low_count % ave_high % ave_high_count % threshold).str() );
                    ave_low_count = ave_high_count = 0;
                    ave_low = ave_high = 0.0f;
                    continue;
                }
                else
                {
                    ave_low /= (double)ave_low_count;
                    ave_high /= (double)ave_high_count;
                    threshold = ave_low + ((ave_high - ave_low) * _opt.trigger_level);
                    print_msg( (boost::format("Phase #2: Ave low: %.3f (#%d), ave high: %.3f (#%d), threshold: %.3f\n") % ave_low % ave_low_count % ave_high % ave_high_count % threshold).str() );

                    _stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
                    _stream_cmd.stream_now = true;
                    _usrp->issue_stream_cmd(_stream_cmd);

                    double diff = abs(ave_high - ave_low);
                    if (diff < _opt.pulse_detection_threshold)
                    {
                        _return_code = RETCODE_BAD_ARGS;
                        print_error_msg( (boost::format("Did not detect any pulses (difference %.6f < detection threshold %.6f)") % diff % _opt.pulse_detection_threshold).str() );
                        break;
                    }

                    _stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS;
                    _stream_cmd.stream_now = true;
                    _usrp->issue_stream_cmd(_stream_cmd);
                }
            }
            else
                continue;
        } // calibration finished
        calibration_finished = true;
    }
    return threshold;
}

// try to stop USRP properly after tests
void
Responder::stop_usrp_stream()
{
    try
    {
        if (_usrp)
        {
            _stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
            _stream_cmd.stream_now = true;
            _usrp->issue_stream_cmd(_stream_cmd);
        }
    }
    catch (...)
    {
        //
    }
}

// after each delay length update test parameters and print them
void
Responder::update_and_print_parameters(const STATS& statsPrev, const double delay)
{
    uint64_t old_simulate_duration = _simulate_duration;
    _simulate_frequency = get_simulate_frequency(delay, _response_length, _original_simulate_duration);
    _simulate_duration = _opt.simulate_duration(_simulate_frequency);
    print_formatted_delay_line(_simulate_duration, old_simulate_duration, statsPrev, delay, _simulate_frequency);
    _timeout_burst_count = 0;
    _timeout_eob_count = 0;
}

// detect or simulate burst level.
bool
Responder::get_new_state(uint64_t total_samps, uint64_t simulate_duration, float val, float threshold)
{
    bool new_state = false;
    if (simulate_duration > 0) // only simulated input bursts!
        new_state = (((total_samps) % simulate_duration) == 0);
    else
        new_state = (val >= threshold);    // TODO: Just measure difference in fall
    return new_state;
}

// detect a pulse, respond to it and count number of pulses.
// statsCurrent holds parameters.
uint64_t
Responder::detect_respond_pulse_count(STATS &statsCurrent, std::vector<std::complex<float> > &buff, uint64_t trigger_count, size_t num_rx_samps, float threshold, uhd::time_spec_t rx_time)
{
    // buff, threshold
    bool input_state = false;
    for (size_t n = 0; n < num_rx_samps; n++)
    {
        float f = buff[n].real() * _opt.invert;

        bool new_state = get_new_state(_num_total_samps + n, _simulate_duration, f, threshold);

        if ((new_state == false) && (input_state)) // == falling_edge
        {
            trigger_count++;
            statsCurrent.detected++;

            if ((_opt.test_iterations > 0)
                    && (_opt.skip_iterations > 0)
                    && (statsCurrent.skipped == 0)
                    && (_opt.skip_iterations == statsCurrent.detected))
            {
                memset(&statsCurrent, 0x00, sizeof(STATS));
                statsCurrent.delay = _delay;
                statsCurrent.detected = 1;
                statsCurrent.skipped = _opt.skip_iterations;

                trigger_count = 1;
            }

            if( !send_tx_burst(rx_time, n) )
            {
                statsCurrent.missed++;
            }

            if(tx_burst_is_late() )
            {
                statsCurrent.missed++;
            }
        }

        input_state = new_state;
    }
    return trigger_count;
}

// this is the actual "work" function. All the fun happens here
void
Responder::run_test(float threshold)
{
    STATS statsCurrent; //, statsPrev;
    memset(&statsCurrent, 0x00, sizeof(STATS));
    if (_opt.test_iterations > 0)
    {
        update_and_print_parameters(statsCurrent, _delay);
        statsCurrent.delay = _opt.delay_min;
    }
    ///////////////////////////////////////////////////////////////////////////
    uint64_t trigger_count = 0;
    size_t success_count = 0;
    uint64_t num_total_samps_test = 0;

    std::vector<std::complex<float> > buff(_opt.samps_per_buff);
    while (not s_stop_signal_called && _return_code == RETCODE_OK)
    {
        // get samples from rx stream.
        uhd::rx_metadata_t rx_md;
        size_t num_rx_samps = _rx_stream->recv(&buff.front(), buff.size(), rx_md, _opt.timeout);
        // handle errors
        if(handle_rx_errors(rx_md.error_code, num_rx_samps) )
        {
            break;
        }
        // detect falling edges, send respond pulse and check if response could be sent in time
        trigger_count = detect_respond_pulse_count(statsCurrent, buff, trigger_count, num_rx_samps, threshold, rx_md.time_spec);

        // increase counters for single test and overall test samples count.
        _num_total_samps += num_rx_samps;
        num_total_samps_test += num_rx_samps;

        // control section for interactive mode
        if (_opt.test_iterations == 0) // == "interactive'
        {
            if(handle_interactive_control() )
                break;
        }

        // control section for test mode
        if (_opt.test_iterations > 0) // == test mode / batch-mode
        {
            int step_return = test_step_finished(trigger_count, num_total_samps_test, statsCurrent, success_count);
            if(step_return == -2) // == test is finished with all desired delay steps
                break;
            else if(step_return == -1) // just continue test
                continue;
            else // test with one delay is finished
            {
                success_count = (size_t) step_return;
                trigger_count = 0;
                num_total_samps_test = 0;
                memset(&statsCurrent, 0x00, sizeof(STATS)); // reset current stats for next test iteration
                statsCurrent.delay = _delay;
            }
        } // end test mode control section
    }// exit outer loop after stop signal is called, test is finished or other break condition is met

    if (s_stop_signal_called)
        _return_code = RETCODE_MANUAL_ABORT;
}

// check if test with one specific delay is finished
int
Responder::test_step_finished(uint64_t trigger_count, uint64_t num_total_samps_test, STATS statsCurrent, size_t success_count)
{
    if ( ((_opt.test_iterations_is_sample_count == false) && (trigger_count >= _opt.test_iterations)) ||
         ((_opt.test_iterations_is_sample_count) && (num_total_samps_test > _opt.test_iterations)) )
    {
        add_stats_to_results(statsCurrent, _delay);

        if (statsCurrent.missed == 0) // == NO late bursts
            ++success_count;
        else
            success_count = 0;

        if(test_finished(success_count) )
            return -2; // test is completely finished

        _delay += _delay_step; // increase delay by one step

        update_and_print_parameters(statsCurrent, _delay);
        return success_count; // test is finished for one delay step
    }
    return -1; // == continue test
}

// save test results
void
Responder::add_stats_to_results(STATS statsCurrent, double delay)
{
    _max_success = max(_max_success, (statsCurrent.detected - statsCurrent.missed)); // > 0 --> save results
    uint64_t key = (uint64_t)(delay * 1e6);
    _mapStats[key] = statsCurrent;
}

// run tests and handle errors
int
Responder::run()
{
    if (_return_code != RETCODE_OK)
        return _return_code;
    if (_opt.pause)
        print_msg_and_wait("Press any key to begin...");
    time( &_dbginfo.start_time_test );

    // Put some info about the test on the console
    print_init_test_status();
    try {
        //setup streaming
        _stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS;
        _stream_cmd.stream_now = true;
        _usrp->issue_stream_cmd(_stream_cmd);

        if( !_opt.batch_mode ){
            float threshold = calibrate_usrp_for_test_run();
            if (_return_code != RETCODE_OK)
            {
                return _return_code;
            }
            run_test(threshold);
        }
        else
        {
            run_test();
        }
    }
    catch (const std::runtime_error& e)
    {
        print_msg(e.what() );
        _return_code = RETCODE_RUNTIME_ERROR;
    }
    catch (...)
    {
        print_msg("Unhandled exception");
        _return_code = RETCODE_UNKNOWN_EXCEPTION;
    }

    stop_usrp_stream();
    time( &_dbginfo.end_time_test );
    return (_return_code < 0 ? _return_code : _overruns);
}

/*
 *  Following functions are intended to be used by destructor only!
 */

// This method should print statistics after ncurses endwin.
void
Responder::print_final_statistics()
{
    cout << boost::format("Received %ld samples during test run") % _num_total_samps;
    if (_overruns > 0)
        cout << boost::format(" (%d overruns)") % _overruns;
    cout << endl;
}

// safe test results to a log file if enabled
void
Responder::write_log_file()
{
    try
    {
        if(_opt.log_file){
            std::map<std::string, std::string> hw_info = get_hw_info();
            ofstream logs(_stats_log_filename.c_str());

            logs << boost::format("title=%s") % _opt.test_title << endl;
            logs << boost::format("device=%s") %  _usrp->get_mboard_name() << endl;
            logs << boost::format("device_args=%s") % _opt.device_args << endl;
            logs << boost::format("type=%s") %  hw_info["type"] << endl;
            if (hw_info.size() > 0)
            {
                logs << boost::format("usrp_addr=%s") %  hw_info["usrp_addr"] << endl;
                logs << boost::format("usrp_name=%s") %  hw_info["name"] << endl;
                logs << boost::format("serial=%s") %  hw_info["serial"] << endl;
                logs << boost::format("host_interface=%s") %  hw_info["interface"] << endl;
                logs << boost::format("host_addr=%s") %  hw_info["host_addr"] << endl;
                logs << boost::format("host_mac=%s") %  hw_info["mac"] << endl;
                logs << boost::format("host_vendor=%s (id=%s)") %  hw_info["vendor"] % hw_info["vendor_id"] << endl;
                logs << boost::format("host_device=%s (id=%s)") %  hw_info["device"] % hw_info["device_id"] << endl;
            }
            logs << boost::format("sample_rate=%f") % _opt.sample_rate << endl;
            logs << boost::format("samps_per_buff=%i") % _opt.samps_per_buff << endl;
            logs << boost::format("samps_per_packet=%i") % _samps_per_packet << endl;
            logs << boost::format("delay_min=%f") % _opt.delay_min << endl;
            logs << boost::format("delay_max=%f") % _opt.delay_max << endl;
            logs << boost::format("delay_step=%f") % _delay_step << endl;
            logs << boost::format("delay=%f") % _delay << endl;
            logs << boost::format("init_delay=%f") % _opt.init_delay << endl;
            logs << boost::format("response_duration=%f") % _opt.response_duration << endl;
            logs << boost::format("response_length=%ld") % _response_length << endl;
            logs << boost::format("timeout=%f") % _opt.timeout << endl;
            logs << boost::format("timeout_burst_count=%ld") % _timeout_burst_count << endl;
            logs << boost::format("timeout_eob_count=%f") % _timeout_eob_count << endl;
            logs << boost::format("allow_late_bursts=%s") % (_allow_late_bursts ? "yes" : "no") << endl;
            logs << boost::format("skip_eob=%s") % (_opt.skip_eob ? "yes" : "no") << endl;
            logs << boost::format("combine_eob=%s") % (_opt.combine_eob ? "yes" : "no") << endl;
            logs << boost::format("skip_send=%s") % (_opt.skip_send ? "yes" : "no") << endl;
            logs << boost::format("no_delay=%s") % (_no_delay ? "yes" : "no") << endl;
            logs << boost::format("simulate_frequency=%f") % _simulate_frequency << endl;
            logs << boost::format("simulate_duration=%ld") % _simulate_duration << endl;
            logs << boost::format("original_simulate_duration=%ld") % _original_simulate_duration << endl;
            logs << boost::format("realtime=%s") % (_opt.realtime ? "yes" : "no") << endl;
            logs << boost::format("rt_priority=%f") % _opt.rt_priority << endl;
            logs << boost::format("test_iterations=%ld") % _opt.test_iterations << endl;
            logs << boost::format("end_test_after_success_count=%i") % _opt.end_test_after_success_count << endl;
            logs << boost::format("skip_iterations=%i") % _opt.skip_iterations << endl;
            logs << boost::format("overruns=%i") % _overruns << endl;
            logs << boost::format("num_total_samps=%ld") % _num_total_samps << endl;
            logs << boost::format("return_code=%i\t(%s)") % _return_code % enum2str(_return_code) << endl;
            logs << endl;

            write_debug_info(logs);

        }
    }
    catch(...)
    {
        cerr << "Failed to write log file to: " << _stats_log_filename << endl;
    }
}

// write debug info to log file
void
Responder::write_debug_info(ofstream& logs)
{
    logs << endl << "%% DEBUG INFO %%" << endl;

    logs << boost::format("dbg_time_start=%s") % get_gmtime_string(_dbginfo.start_time) << endl;
    logs << boost::format("dbg_time_end=%s") % get_gmtime_string(_dbginfo.end_time) << endl;
    logs << boost::format("dbg_time_duration=%d") % difftime( _dbginfo.end_time, _dbginfo.start_time ) << endl;
    logs << boost::format("dbg_time_start_test=%s") % get_gmtime_string(_dbginfo.start_time_test) << endl;
    logs << boost::format("dbg_time_end_test=%s") % get_gmtime_string(_dbginfo.end_time_test) << endl;
    logs << boost::format("dbg_time_duration_test=%d") % difftime( _dbginfo.end_time_test, _dbginfo.start_time_test ) << endl;
    logs << boost::format("dbg_time_first_send_timeout=%s") % get_gmtime_string(_dbginfo.first_send_timeout) << endl;
}

// convert a time string to desired format
std::string
Responder::get_gmtime_string(time_t time)
{
    tm* ftm;
    ftm = gmtime( &time );
    std::string strtime;
    strtime.append( (boost::format("%i") % (ftm->tm_year+1900) ).str() );
    strtime.append( (boost::format("-%02i") % ftm->tm_mon).str() );
    strtime.append( (boost::format("-%02i") % ftm->tm_mday).str() );
    strtime.append( (boost::format("-%02i") % ((ftm->tm_hour)) ).str() );
    strtime.append( (boost::format(":%02i") % ftm->tm_min).str() );
    strtime.append( (boost::format(":%02i") % ftm->tm_sec).str() );

    return strtime;
}

// read hardware info from file if available to include it in log file
std::map<std::string, std::string>
Responder::get_hw_info()
{
    std::map<std::string, std::string> result;
    std::vector<std::map<std::string,std::string> > eths = read_eth_info();
    if(eths.empty()){
        return result;
    }
    uhd::device_addr_t usrp_info = get_usrp_info();
    std::string uaddr = get_ip_subnet_addr(usrp_info["addr"]);

    for(unsigned int i = 0 ; i < eths.size() ; i++ )
    {
        if(get_ip_subnet_addr(eths[i]["addr"]) == uaddr)
        {
            result["type"] = usrp_info["type"];
            result["usrp_addr"] = usrp_info["addr"];
            result["name"] = usrp_info["name"];
            result["serial"] = usrp_info["serial"];
            result["interface"] = eths[i]["interface"];
            result["host_addr"] = eths[i]["addr"];
            result["mac"] = eths[i]["mac"];
            result["vendor"] = eths[i]["vendor"];
            result["vendor_id"] = eths[i]["vendor_id"];
            result["device"] = eths[i]["device"];
            result["device_id"] = eths[i]["device_id"];
            break; // Use first item found. Imitate device discovery.
        }
    }

    return result;
}

// subnet used to identify used network interface
std::string
Responder::get_ip_subnet_addr(std::string ip)
{
    return ip.substr(0, ip.rfind(".") + 1);
}

// get network interface info from file (should include all available interfaces)
std::vector<std::map<std::string,std::string> >
Responder::read_eth_info()
{
    const std::string eth_file(_eth_file);

    std::vector<std::map<std::string,std::string> > eths;
    try
    {
        ifstream eth_info(eth_file.c_str());
        if(!eth_info.is_open()){
            return eths;
        }
        const int len = 256;
        char cline[len];
        for(; !eth_info.eof() ;)
        {
            eth_info.getline(cline, len);
            std::string line(cline);
            if(line.find("## ETH Interface") != std::string::npos)
            {
                eth_info.getline(cline, len);
                std::string eth(cline);
//                cout << "interface=" << eth << endl;
                std::map<std::string,std::string> iface;
                iface["interface"] = eth;
                eths.push_back(iface);
            }
            const std::string ipstr("\tip ");
            if(line.find(ipstr) != std::string::npos)
            {
                std::string ip( line.replace(line.begin(), line.begin()+ipstr.length(), "") );
//                cout << "ip=" << ip << endl;
                eths.back()["addr"] = ip;
            }
            const std::string macstr("\tmac ");
            if(line.find(macstr) != std::string::npos)
            {
                std::string mac( line.replace(line.begin(), line.begin()+macstr.length(), "") );
//                cout << "mac=" << mac << endl;
                eths.back()["mac"] = mac;
            }
            const std::string vstr("\t\tvendor ");
            if(line.find(vstr) != std::string::npos)
            {
                std::string vendor( line.replace(line.begin(), line.begin()+vstr.length(), "") );
                std::string vid( vendor.substr(0,6) );
                vendor.replace(0, 7, "");
//                cout << "id=" << vid << endl;
//                cout << "vendor=" << vendor << endl;
                eths.back()["vendor"] = vendor;
                eths.back()["vendor_id"] = vid;
            }
            const std::string dstr("\t\tdevice ");
            if(line.find(dstr) != std::string::npos)
            {
                std::string device( line.replace(line.begin(), line.begin()+dstr.length(), "") );
                std::string did( device.substr(0,6) );
                device.replace(0, 7, "");
//                cout << "id=" << did << endl;
//                cout << "device=" << device << endl;
                eths.back()["device"] = device;
                eths.back()["device_id"] = did;
            }
        }

    }
    catch(...)
    {
        // nothing in yet
    }
    return eths;
}

// get info on used USRP
uhd::device_addr_t
Responder::get_usrp_info()
{
    uhd::device_addrs_t device_addrs = uhd::device::find(_opt.device_args);
    uhd::device_addr_t device_addr = device_addrs[0];
    return device_addr;
}

// write statistics of test run to file
void
Responder::write_statistics_to_file(StatsMap mapStats)
{
    try
    {
        ofstream results(_stats_filename.c_str());

        for (StatsMap::iterator it = mapStats.begin(); it != mapStats.end(); ++it)
        {
            STATS& stats = it->second;
            double d = 0.0;
            if (stats.detected > 0)
                d = 1.0 - ((double)stats.missed / (double)stats.detected);
            cout << "\t" << setprecision(6) << stats.delay << "\t\t" << setprecision(6) << d << endl;

            results << (stats.delay * _opt.time_mul) << " " << setprecision(6) << d << endl;
        }
        cout << "Statistics written to: " << _stats_filename << endl;

    }
    catch (...)
    {
        cout << "Failed to write statistics to: " << _stats_filename << endl;
    }
}

// make sure write files is intended
void
Responder::safe_write_statistics_to_file(StatsMap mapStats, uint64_t max_success, int return_code)
{
    if ((_opt.test_iterations > 0) && (_stats_filename.empty() == false) && (_opt.no_stats_file == false))
    {
        if (mapStats.empty())
        {
            cout << "No results to output (not writing statistics file)" << endl;
        }
        else if ((max_success == 0) && (return_code == RETCODE_MANUAL_ABORT))
        {
            cout << "Aborted before a single successful timed burst (not writing statistics file)" << endl;
        }
        else
        {
            write_statistics_to_file(mapStats);
        }
        write_log_file();
    }
}

// destructor, handle proper test shutdown
Responder::~Responder()
{
    endwin();
    if(_pResponse != NULL){
        delete[] _pResponse;
    }
    time( &_dbginfo.end_time );
    // Print final info about test run
    print_final_statistics();
    // check conditions and write statistics to file
    safe_write_statistics_to_file(_mapStats, _max_success, _return_code);
    cout << "program exited with code = " << enum2str(_return_code) << endl;
}

// make test output more helpful
std::string
Responder::enum2str(int return_code)
{
    switch(return_code)
    {
        case RETCODE_OK: return "OK";
        case RETCODE_BAD_ARGS: return "BAD_ARGS";
        case RETCODE_RUNTIME_ERROR: return "RUNTIME_ERROR";
        case RETCODE_UNKNOWN_EXCEPTION: return "UNKNOWN_EXCEPTION";
        case RETCODE_RECEIVE_TIMEOUT: return "RECEIVE_TIMEOUT";
        case RETCODE_RECEIVE_FAILED: return "RECEIVE_FAILED";
        case RETCODE_MANUAL_ABORT: return "MANUAL_ABORT";
        case RETCODE_BAD_PACKET: return "BAD_PACKET";
        case RETCODE_OVERFLOW: return "OVERFLOW";
    }
    return "UNKNOWN";
}


//
// Copyright 2010-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef RESPONDER_H
#define RESPONDER_H

#include <curses.h>
#include <map>
#include <ctime>
#include <stdint.h>

#include <uhd/usrp/multi_usrp.hpp>

using namespace std;


class Responder
{
    public:
        enum ReturnCodes
        {
            RETCODE_OK                  = 0,
            RETCODE_BAD_ARGS            = -1,
            RETCODE_RUNTIME_ERROR       = -2,
            RETCODE_UNKNOWN_EXCEPTION   = -3,
            RETCODE_RECEIVE_TIMEOUT     = -4,
            RETCODE_RECEIVE_FAILED      = -5,
            RETCODE_MANUAL_ABORT        = -6,
            RETCODE_BAD_PACKET          = -7,
            RETCODE_OVERFLOW            = -8
        };

        struct Options
        {
            string device_args;
            double delay;
            double sample_rate;
            double trigger_level;
            float output_scale;
            double response_duration;
            double dc_offset_delay;
            double init_delay;
            double timeout;
            size_t samps_per_buff;
            size_t samps_per_packet;
            double level_calibration_duration;
            std::string test_title;
            std::string stats_filename;
            std::string stats_filename_prefix;
            std::string stats_filename_suffix;
            double delay_min;
            double delay_max;
            double delay_step;
            double pulse_detection_threshold;
            uint64_t test_iterations;
            size_t end_test_after_success_count;
            size_t skip_iterations;
            double simulate_frequency;
            double time_mul;
            size_t flush_count;
            size_t optimize_padding;
            double rt_priority;
            bool ignore_simulation_check;
            bool test_iterations_is_sample_count;
            bool skip_eob;
            bool adjust_simulation_rate;
            bool optimize_simulation_rate;
            bool no_stats_file;
            bool log_file;
            bool batch_mode;
            bool skip_if_results_exist;
            bool skip_send;
            bool combine_eob;
            bool pause;
            bool realtime;
            float invert;
            float output_value;
            bool no_delay;
            bool allow_late_bursts;

            uint64_t level_calibration_count() const
            {
                return (uint64_t)(sample_rate * level_calibration_duration);
            }

            uint64_t response_length() const
            {
                return (uint64_t)(sample_rate * response_duration);
            }

            uint64_t highest_delay_samples(const double delay) const
            {
                return (uint64_t)(delay * (double)sample_rate);
            }

            uint64_t simulate_duration(const double simulate_frequency) const
            {
                if(simulate_frequency > 0.0) {
                    return (uint64_t)((double)sample_rate / simulate_frequency);
                }
                return 0;
            }
        };

        typedef struct Stats
        {
            double delay;
            uint64_t detected;
            uint64_t missed;
            uint64_t skipped;
        } STATS;

        typedef std::map<uint64_t,STATS> StatsMap;

        struct DebugInfo
        {
            time_t start_time;
            time_t end_time;
            time_t start_time_test;
            time_t end_time_test;
            time_t first_send_timeout;
        };
        Responder(Options& opt);
        virtual ~Responder();

        // Main entry point after constructor.
        int run();

        int get_return_code(){return _return_code;}

    protected:
    private:
        // These 2 variables are used for ncurses output.
        WINDOW* _window;
        std::stringstream _ss;
        std::stringstream _ss_cerr;

        // struct which holds all arguments as constants settable from outside the class
        const Options _opt;

        string _stats_filename; // Specify name of statistics file
        string _stats_log_filename; // Specify name for log file.
        double _delay; // may be altered in all modes.
        size_t _samps_per_packet; // This is one of the options of interest. Find out how well it performs.
        double _delay_step; // may be altered in interactive mode
        double _simulate_frequency; // updated during automatic test iterations

        // additional attributes
        bool _allow_late_bursts; // may be altered in interactive mode
        bool _no_delay; // may be altered in interactive mode

        // dependent variables
        uint64_t _response_length;
        int64_t _init_delay_count;
        int64_t _dc_offset_countdown;
        int64_t _level_calibration_countdown;
        uint64_t _simulate_duration;
        uint64_t _original_simulate_duration;

        // these variables store test conditions
        uint64_t _num_total_samps; // printed on exit
        size_t _overruns; // printed on exit
        StatsMap _mapStats; // store results
        uint64_t _max_success; // < 0 --> write results to file
        int _return_code;

        // Hold USRP, streams and commands
        uhd::usrp::multi_usrp::sptr _usrp;
        uhd::tx_streamer::sptr _tx_stream;
        uhd::rx_streamer::sptr _rx_stream;
        uhd::stream_cmd_t _stream_cmd;

        // Keep track of number of timeouts.
        uint64_t _timeout_burst_count;
        uint64_t _timeout_eob_count;

        // Transmit attributes
        float* _pResponse;

        // Control print parameters.
        int _y_delay_pos;
        int _x_delay_pos; // Remember the cursor position of delay line
        uint64_t _last_overrun_count;

        // Hold debug info during test. Will be included in log file.
        DebugInfo _dbginfo;

        /*
         * Here are the class's member methods.
         */
        // These methods are used for ncurses output
        void create_ncurses_window();
        void FLUSH_SCREEN();
        void FLUSH_SCREEN_NL();

        // Variable calculation helpers
        inline uint64_t get_response_length(double sample_rate, double response_duration)
                                    {return (uint64_t)(sample_rate * response_duration);}
        int calculate_dependent_values();

        // make sure existing results are not overwritten accidently
        bool set_stats_filename(string test_id);
        bool check_for_existing_results();

        // Functions that may cause Responder to finish
        void register_stop_signal_handler();
        bool test_finished(size_t success_count);
        int test_step_finished(uint64_t trigger_count, uint64_t num_total_samps_test, STATS statsCurrent, size_t success_count);

        // Check if sent burst could be transmitted.
        bool tx_burst_is_late();

        // Handle receiver errors such as overflows.
        bool handle_rx_errors(uhd::rx_metadata_t::error_code_t err, size_t num_rx_samps);

        // In interactive mode, handle Responder control and output.
        bool handle_interactive_control();
        void print_interactive_msg(std::string msg);

        // calibration important for interactive mode with 2nd USRP connected.
        float calibrate_usrp_for_test_run();

        // Run actual test
        void run_test(float threshold = 0.0f );

        // Detect falling edge
        bool get_new_state(uint64_t total_samps, uint64_t simulate_duration, float val, float threshold);
        uint64_t detect_respond_pulse_count(STATS &statsCurrent, std::vector<std::complex<float> > &buff, uint64_t trigger_count, size_t num_rx_samps, float threshold, uhd::time_spec_t rx_time);

        // Hold test results till they are printed to a file
        void add_stats_to_results(STATS statsCurrent, double delay);

        // Control USRP and necessary streamers
        uhd::usrp::multi_usrp::sptr create_usrp_device();
        void set_usrp_rx_dc_offset(uhd::usrp::multi_usrp::sptr usrp, bool ena);
        void stop_usrp_stream();
        uhd::tx_streamer::sptr create_tx_streamer(uhd::usrp::multi_usrp::sptr usrp);
        uhd::rx_streamer::sptr create_rx_streamer(uhd::usrp::multi_usrp::sptr usrp);

        // Send burst and handle results.
        bool send_tx_burst(uhd::time_spec_t rx_time, size_t n);
        void handle_tx_timeout(int burst, int eob);
        float* alloc_response_buffer_with_data(uint64_t response_length);
        uhd::tx_metadata_t get_tx_metadata(uhd::time_spec_t rx_time, size_t n);

        // Control test parameters
        void update_and_print_parameters(const STATS& statsPrev, const double delay);
        double get_simulate_frequency(double delay, uint64_t response_length, uint64_t original_simulate_duration);
        double get_max_possible_frequency(uint64_t highest_delay_samples, uint64_t response_length);

        // Helper methods to print status during test.
        void print_init_test_status();
        void print_test_title();
        void print_usrp_status();
        void print_create_usrp_msg();
        void print_tx_stream_status();
        void print_rx_stream_status();
        void print_test_parameters();
        void print_formatted_delay_line(const uint64_t simulate_duration, const uint64_t old_simulate_duration, const STATS& statsPrev, const double delay, const double simulate_frequency);
        void print_overrun_msg();
        void print_error_msg(std::string msg);
        void print_timeout_msg();
        void print_final_statistics();
        void print_msg_and_wait(std::string msg);
        void print_msg(std::string msg);

        // Safe results of test to file.
        void write_statistics_to_file(StatsMap mapStats);
        void safe_write_statistics_to_file(StatsMap mapStats, uint64_t max_success, int return_code);
        void write_log_file();

        // Write debug info to log file if requested.
        void write_debug_info(ofstream& logs);
        std::string get_gmtime_string(time_t time);
        std::string enum2str(int return_code);
        std::vector<std::map<std::string,std::string> > read_eth_info();
        uhd::device_addr_t get_usrp_info();
        std::map<std::string, std::string> get_hw_info();
        std::string get_ip_subnet_addr(std::string ip);
};

#endif // RESPONDER_H

//
// Copyright 2010-2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/program_options.hpp>
#include <uhd/utils/safe_main.hpp>
#include <Responder.hpp>

namespace po = boost::program_options;

static Responder::Options prog;

po::options_description
get_program_options_description()
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("title", po::value<std::string>(&prog.test_title)->default_value(""), "title to show during test")
        ("args", po::value<std::string>(&prog.device_args)->default_value(""), "single uhd device address args")
        ("stats-file", po::value<std::string>(&prog.stats_filename)->default_value(""), "test statistics output filename (empty: auto-generate)")
        ("stats-file-prefix", po::value<std::string>(&prog.stats_filename_prefix)->default_value(""), "test statistics output filename prefix")
        ("stats-file-suffix", po::value<std::string>(&prog.stats_filename_suffix)->default_value(""), "test statistics output filename suffix")

        ("rate", po::value<double>(&prog.sample_rate)->default_value(1e6), "rate of outgoing samples")
        ("level", po::value<double>(&prog.trigger_level)->default_value(0.5), "trigger level as fraction of high level")
        ("scale", po::value<float>(&prog.output_scale)->default_value(float(0.3)), "output scaling")
        ("duration", po::value<double>(&prog.response_duration)->default_value(0.001), "duration of response (seconds)")
        ("dc-offset-delay", po::value<double>(&prog.dc_offset_delay)->default_value(0), "duration of DC offset calibration (seconds)")   // This stage is not necessary
        ("init-delay", po::value<double>(&prog.init_delay)->default_value(0.5), "initialisation delay (seconds)")
        ("timeout", po::value<double>(&prog.timeout)->default_value(1.0), "stream timeout (seconds)")
        ("spb", po::value<size_t>(&prog.samps_per_buff)->default_value(1024), "samples per buffer")
        ("spp", po::value<size_t>(&prog.samps_per_packet)->default_value(0), "samples per packet (0: use default)")
        ("calib", po::value<double>(&prog.level_calibration_duration)->default_value(0.5), "level calibration duration (seconds)")
        ("invert", "input signal inversion")
        ("invert-output", "output signal inversion")
        ("no-delay", "disable timed delay")
        ("allow-late", "allow late bursts")
        ("delay", po::value<double>(&prog.delay)->default_value(0.005), "number of seconds in the future to reply")
        ("delay-min", po::value<double>(&prog.delay_min)->default_value(0.0001), "minimum delay")
        ("delay-max", po::value<double>(&prog.delay_max)->default_value(0.0050), "maximum delay")
        ("delay-step", po::value<double>(&prog.delay_step)->default_value(0.000001), "delay step")
        ("pdt", po::value<double>(&prog.pulse_detection_threshold)->default_value(1e-3), "pulse detection threshold")
        ("iterations", po::value<uint64_t>(&prog.test_iterations)->default_value(0), "test iterations")
        ("test-duration", "treat test iterations as duration")
        ("test-success", po::value<size_t>(&prog.end_test_after_success_count)->default_value(0), "end test after multiple successful delays (0: run through all delays)")
        ("skip-iterations", po::value<size_t>(&prog.skip_iterations)->default_value(50), "skip first iterations for each delay")
        ("simulate", po::value<double>(&prog.simulate_frequency)->default_value(0.0), "frequency of simulation event (Hz)")
        ("time-mul", po::value<double>(&prog.time_mul)->default_value(1.0), "statistics output time multiplier")
        ("ignore-simulation-check", "ignore if simulation rate exceeds maximum delay + response duration")
        ("flush", po::value<size_t>(&prog.flush_count)->default_value(16), "number of zero samples to add to a burst to flush hardware")
        ("skip-eob", "disable end-of-burst")
        ("adjust-simulation-rate", "adjust simulation rate if it will be too fast for maximum delay duration")
        ("optimize-simulation-rate", "make simulation rate as fast as possible for each delay")
        ("optimize-padding", po::value<size_t>(&prog.optimize_padding)->default_value(16), "time (as number of samples) to pad optimized simulation rate")
        ("no-stats-file", "do not output statistics file")
        ("log-file", "output log file")
        ("batch-mode", "disable user prompts")
        ("skip-if-exists", "skip the test if the results file exists")
        ("disable-send", "skip burst transmission")
        ("combine-eob", "combine EOB into first send")
        ("pause", "pause after device creation")
        ("priority", po::value<double>(&prog.rt_priority)->default_value(1.0), "scheduler priority")
        ("no-realtime", "don't enable real-time")
    ;
    return desc;
}

void
read_program_options(po::variables_map vm)
{
    // read out given options
    prog.realtime = (vm.count("no-realtime") == 0);

    prog.delay_step = abs(prog.delay_step);
    if (prog.delay_min > prog.delay_max)
    {
        prog.delay_step *= -1;
    }

    prog.allow_late_bursts = (vm.count("allow-late") > 0);
    prog.test_iterations_is_sample_count = (vm.count("test-duration") > 0);
    prog.invert = ((vm.count("invert") > 0) ? -1.0f : 1.0f);
    prog.output_value = ((vm.count("invert-output") > 0) ? -1.0f : 1.0f);
    prog.skip_eob = (vm.count("skip-eob") > 0);
    prog.no_delay = (vm.count("no-delay") > 0);
    prog.adjust_simulation_rate = (vm.count("adjust-simulation-rate") > 0);
    prog.optimize_simulation_rate = (vm.count("optimize-simulation-rate") > 0);
    prog.no_stats_file = (vm.count("no-stats-file") > 0);
    prog.log_file = (vm.count("log-file") > 0);
    prog.batch_mode = (vm.count("batch-mode") > 0);
    prog.skip_if_results_exist = (vm.count("skip-if-exists") > 0);
    prog.skip_send = (vm.count("disable-send") > 0);
    prog.combine_eob = (vm.count("combine-eob") > 0);
    prog.pause = (vm.count("pause") > 0);
    prog.ignore_simulation_check = vm.count("ignore-simulation-check");
}

/*
 * This is the MAIN function!
 * UHD_SAFE_MAIN catches all errors and prints them to stderr.
 */
int UHD_SAFE_MAIN(int argc, char *argv[]){
    po::options_description desc = get_program_options_description();
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    read_program_options(vm);

    // Print help message instead of executing Responder.
    if (vm.count("help")){
        cout << boost::format("UHD Latency Test %s") % desc;
        return Responder::RETCODE_OK;
    }

    //create a new instance of Responder and run it!
    boost::shared_ptr<Responder> my_responder(new Responder(prog));
    return my_responder->run();
}


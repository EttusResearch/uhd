//
// Copyright 2010-2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "ascii_art_dft.hpp" //implementation
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <curses.h>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <complex>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace po = boost::program_options;
using std::chrono::high_resolution_clock;

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // variables to be set by po
    std::string args, ant, subdev, ref;
    size_t num_bins;
    double rate, freq, gain, bw, frame_rate, step, power;
    float ref_lvl, dyn_rng;
    bool show_controls, show_gain_mode;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "multi uhd device address args")
        // hardware parameters
        ("rate", po::value<double>(&rate), "rate of incoming samples (sps)")
        ("freq", po::value<double>(&freq), "RF center frequency in Hz")
        ("gain", po::value<double>(&gain), "gain for the RF chain")
        ("power", po::value<double>(&power), "Transmit power (if USRP supports it)")
        ("ant", po::value<std::string>(&ant), "antenna selection")
        ("subdev", po::value<std::string>(&subdev), "subdevice specification")
        ("bw", po::value<double>(&bw), "analog frontend filter bandwidth in Hz")
        // display parameters
        ("num-bins", po::value<size_t>(&num_bins)->default_value(512), "the number of bins in the DFT")
        ("frame-rate", po::value<double>(&frame_rate)->default_value(5), "frame rate of the display (fps)")
        ("ref-lvl", po::value<float>(&ref_lvl)->default_value(0), "reference level for the display (dB)")
        ("dyn-rng", po::value<float>(&dyn_rng)->default_value(60), "dynamic range for the display (dB)")
        ("ref", po::value<std::string>(&ref), "reference source (internal, external, gpsdo, mimo)")
        ("step", po::value<double>(&step)->default_value(1e6), "tuning step for rate/bw/freq")
        ("show-controls", po::value<bool>(&show_controls)->default_value(true), "show the keyboard controls")
        ("int-n", "tune USRP with integer-N tuning")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // print the help message
    if (vm.count("help") or not vm.count("rate")) {
        std::cout << boost::format("UHD RX ASCII Art DFT %s") % desc << std::endl;
        return EXIT_FAILURE;
    }

    // create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args
              << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

    // Lock mboard clocks
    if (vm.count("ref")) {
        usrp->set_clock_source(ref);
    }

    // always select the subdevice first, the channel mapping affects the other settings
    if (vm.count("subdev"))
        usrp->set_rx_subdev_spec(subdev);

    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    // set the sample rate
    if (not vm.count("rate")) {
        std::cerr << "Please specify the sample rate with --rate" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rate / 1e6) << std::endl;
    usrp->set_rx_rate(rate);
    std::cout << boost::format("Actual RX Rate: %f Msps...") % (usrp->get_rx_rate() / 1e6)
              << std::endl
              << std::endl;

    // set the center frequency
    if (not vm.count("freq")) {
        std::cerr << "Please specify the center frequency with --freq" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << boost::format("Setting RX Freq: %f MHz...") % (freq / 1e6) << std::endl;
    uhd::tune_request_t tune_request(freq);
    if (vm.count("int-n"))
        tune_request.args = uhd::device_addr_t("mode_n=integer");
    usrp->set_rx_freq(tune_request);
    std::cout << boost::format("Actual RX Freq: %f MHz...") % (usrp->get_rx_freq() / 1e6)
              << std::endl
              << std::endl;

    // set the antenna
    // note: must be set before any operation that requires the calibration table (like
    // power reference) otherwise the driver will look up the wrong antenna for the table
    if (vm.count("ant"))
        usrp->set_rx_antenna(ant);

    // set the rf gain
    if (vm.count("power")) {
        if (!usrp->has_rx_power_reference()) {
            std::cout << "ERROR: USRP does not have a reference power API on channel "
                      << 0 << "!" << std::endl;
            return EXIT_FAILURE;
        }
        std::cout << "Setting RX reference power level: " << power << " dBm..."
                  << std::endl;
        usrp->set_rx_power_reference(power);
        std::cout << "Actual RX reference power level: " << usrp->get_rx_power_reference()
                  << " dBm..." << std::endl;
        if (vm.count("gain")) {
            std::cout << "WARNING: If you specify both --power and --gain, "
                         " the latter will be ignored."
                      << std::endl;
        }
        show_gain_mode = false;
    } else if (vm.count("gain")) {
        std::cout << boost::format("Setting RX Gain: %f dB...") % gain << std::endl;
        usrp->set_rx_gain(gain);
        std::cout << boost::format("Actual RX Gain: %f dB...") % usrp->get_rx_gain()
                  << std::endl
                  << std::endl;
        show_gain_mode = true;
    } else {
        gain           = usrp->get_rx_gain();
        show_gain_mode = true;
    }

    // set the analog frontend filter bandwidth
    if (vm.count("bw")) {
        std::cout << boost::format("Setting RX Bandwidth: %f MHz...") % (bw / 1e6)
                  << std::endl;
        usrp->set_rx_bandwidth(bw);
        std::cout << boost::format("Actual RX Bandwidth: %f MHz...")
                         % (usrp->get_rx_bandwidth() / 1e6)
                  << std::endl
                  << std::endl;
    } else {
        bw = usrp->get_rx_bandwidth();
    }

    std::this_thread::sleep_for(std::chrono::seconds(1)); // allow for some setup time

    // Check Ref and LO Lock detect
    std::vector<std::string> sensor_names;
    sensor_names = usrp->get_rx_sensor_names(0);
    if (std::find(sensor_names.begin(), sensor_names.end(), "lo_locked")
        != sensor_names.end()) {
        uhd::sensor_value_t lo_locked = usrp->get_rx_sensor("lo_locked", 0);
        std::cout << boost::format("Checking RX: %s ...") % lo_locked.to_pp_string()
                  << std::endl;
        UHD_ASSERT_THROW(lo_locked.to_bool());
    }
    sensor_names = usrp->get_mboard_sensor_names(0);
    if ((ref == "mimo")
        and (std::find(sensor_names.begin(), sensor_names.end(), "mimo_locked")
             != sensor_names.end())) {
        uhd::sensor_value_t mimo_locked = usrp->get_mboard_sensor("mimo_locked", 0);
        std::cout << boost::format("Checking RX: %s ...") % mimo_locked.to_pp_string()
                  << std::endl;
        UHD_ASSERT_THROW(mimo_locked.to_bool());
    }
    if ((ref == "external")
        and (std::find(sensor_names.begin(), sensor_names.end(), "ref_locked")
             != sensor_names.end())) {
        uhd::sensor_value_t ref_locked = usrp->get_mboard_sensor("ref_locked", 0);
        std::cout << boost::format("Checking RX: %s ...") % ref_locked.to_pp_string()
                  << std::endl;
        UHD_ASSERT_THROW(ref_locked.to_bool());
    }

    // create a receive streamer
    uhd::stream_args_t stream_args("fc32"); // complex floats
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    // allocate recv buffer and metatdata
    uhd::rx_metadata_t md;
    std::vector<std::complex<float>> buff(num_bins);
    //------------------------------------------------------------------
    //-- Initialize
    //------------------------------------------------------------------
    initscr(); // curses init
    rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    auto next_refresh = high_resolution_clock::now();

    //------------------------------------------------------------------
    //-- Main loop
    //------------------------------------------------------------------
    while (true) {
        // read a buffer's worth of samples every iteration
        size_t num_rx_samps = rx_stream->recv(&buff.front(), buff.size(), md);
        if (num_rx_samps != buff.size())
            continue;

        // check and update the display refresh condition
        if (high_resolution_clock::now() < next_refresh) {
            continue;
        }
        next_refresh = high_resolution_clock::now()
                       + std::chrono::microseconds(int64_t(1e6 / frame_rate));

        // calculate the dft and create the ascii art frame
        ascii_art_dft::log_pwr_dft_type lpdft(
            ascii_art_dft::log_pwr_dft(&buff.front(), num_rx_samps));
        std::string frame = ascii_art_dft::dft_to_plot(lpdft,
            COLS,
            (show_controls ? LINES - 6 : LINES),
            usrp->get_rx_rate(),
            usrp->get_rx_freq(),
            dyn_rng,
            ref_lvl);

        std::string border = std::string((COLS), '-');

        // curses screen handling: clear and print frame
        clear();

        if (show_controls) {
            printw("%s", border.c_str());
            if (show_gain_mode) {
                printw("[f-F]req: %4.3f MHz   |   [r-R]ate: %2.2f Msps   |"
                       "   [b-B]w: %2.2f MHz   |   [g-G]ain: %2.0f dB\n\n",
                    freq / 1e6,
                    rate / 1e6,
                    bw / 1e6,
                    gain);
            } else {
                printw("[f-F]req: %4.3f MHz   |   [r-R]ate: %2.2f Msps   |"
                       "   [b-B]w: %2.2f MHz   |   [p-P]ower: %2.0f dBm\n\n",
                    freq / 1e6,
                    rate / 1e6,
                    bw / 1e6,
                    power);
            }
            printw("[d-D]yn Range: %2.0f dB    |   Ref [l-L]evel: %2.0f dB   |"
                   "   fp[s-S] : %2.0f   |   [t-T]uning step: %3.3f M\n",
                dyn_rng,
                ref_lvl,
                frame_rate,
                step / 1e6);
            printw("(press c to toggle controls)\n");
            printw("%s", border.c_str());
        }
        printw("%s", frame.c_str());

        // curses key handling: no timeout, any key to exit
        timeout(0);
        int ch = getch();

        // Key handling.
        if (ch == 'r') {
            rate -= step;
            usrp->set_rx_rate(rate);
            freq = usrp->get_rx_freq();
        }

        else if (ch == 'R') {
            rate += step;
            usrp->set_rx_rate(rate);
            freq = usrp->get_rx_freq();
        }

        else if (ch == 'p' && !show_gain_mode) {
            power -= 1;
            usrp->set_rx_power_reference(power);
            power = usrp->get_rx_power_reference();
        }

        else if (ch == 'P' && !show_gain_mode) {
            power += 1;
            usrp->set_rx_power_reference(power);
            power = usrp->get_rx_power_reference();
        }

        else if (ch == 'g' && show_gain_mode) {
            gain -= 1;
            usrp->set_rx_gain(gain);
            gain = usrp->get_rx_gain();
        }

        else if (ch == 'G' && show_gain_mode) {
            gain += 1;
            usrp->set_rx_gain(gain);
            gain = usrp->get_rx_gain();
        }

        else if (ch == 'b') {
            bw -= step;
            usrp->set_rx_bandwidth(bw);
            bw = usrp->get_rx_bandwidth();
        }

        else if (ch == 'B') {
            bw += step;
            usrp->set_rx_bandwidth(bw);
            bw = usrp->get_rx_bandwidth();
        }

        else if (ch == 'f') {
            freq -= step;
            usrp->set_rx_freq(freq);
            freq = usrp->get_rx_freq();
        }

        else if (ch == 'F') {
            freq += step;
            usrp->set_rx_freq(freq);
            freq = usrp->get_rx_freq();
        }

        else if (ch == 'l')
            ref_lvl -= 10;
        else if (ch == 'L')
            ref_lvl += 10;
        else if (ch == 'd') {
            if (dyn_rng > 10) {
                dyn_rng -= 10;
            }
        } else if (ch == 'D')
            dyn_rng += 10;
        else if (ch == 's') {
            if (frame_rate > 1) {
                frame_rate -= 1;
            }
        } else if (ch == 'S') {
            frame_rate += 1;
        } else if (ch == 't') {
            if (step > 1) {
                step /= 2;
            }
        } else if (ch == 'T')
            step *= 2;
        else if (ch == 'c' || ch == 'C') {
            show_controls = !show_controls;
        }

        // Arrow keypress generates 3 characters:
        // '\033', '[', 'A'/'B'/'C'/'D' for Up / Down / Right / Left
        else if (ch == '\033') {
            getch();
            switch (getch()) {
                case 'A':
                case 'C':
                    freq += step;
                    usrp->set_rx_freq(freq);
                    freq = usrp->get_rx_freq();
                    break;

                case 'B':
                case 'D':
                    freq -= step;
                    usrp->set_rx_freq(freq);
                    freq = usrp->get_rx_freq();
                    break;
            }
        } else if (ch != KEY_RESIZE and ch != ERR)
            break;
    }

    //------------------------------------------------------------------
    //-- Cleanup
    //------------------------------------------------------------------
    rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
    endwin(); // curses done

    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return EXIT_SUCCESS;
}

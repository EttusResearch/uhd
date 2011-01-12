//
// Copyright 2010 Ettus Research LLC
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

#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/single_usrp.hpp>
#include "ascii_art_dft.hpp" //implementation
#include <boost/program_options.hpp>
#include <boost/thread.hpp> //gets time
#include <boost/format.hpp>
#include <curses.h>
#include <iostream>
#include <complex>

namespace po = boost::program_options;

int UHD_SAFE_MAIN(int argc, char *argv[]){
    uhd::set_thread_priority_safe();

    //variables to be set by po
    std::string args;
    size_t num_bins;
    double rate, freq, gain, frame_rate;
    float ref_lvl, dyn_rng;

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "single uhd device address args")
        // hardware parameters
        ("rate", po::value<double>(&rate), "rate of incoming samples (sps)")
        ("freq", po::value<double>(&freq)->default_value(0), "RF center frequency in Hz")
        ("gain", po::value<double>(&gain)->default_value(0), "gain for the RF chain")
        // display parameters
        ("num-bins", po::value<size_t>(&num_bins)->default_value(512), "the number of bins in the DFT")
        ("frame-rate", po::value<double>(&frame_rate)->default_value(5), "frame rate of the display (fps)")
        ("ref-lvl", po::value<float>(&ref_lvl)->default_value(0), "reference level for the display (dB)")
        ("dyn-rng", po::value<float>(&dyn_rng)->default_value(60), "dynamic range for the display (dB)")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help") or not vm.count("rate")){
        std::cout << boost::format("UHD RX ASCII Art DFT %s") % desc << std::endl;
        return ~0;
    }

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::single_usrp::sptr sdev = uhd::usrp::single_usrp::make(args);
    std::cout << boost::format("Using Device: %s") % sdev->get_pp_string() << std::endl;

    //set the rx sample rate
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rate/1e6) << std::endl;
    sdev->set_rx_rate(rate);
    std::cout << boost::format("Actual RX Rate: %f Msps...") % (sdev->get_rx_rate()/1e6) << std::endl << std::endl;

    //set the rx center frequency
    std::cout << boost::format("Setting RX Freq: %f Mhz...") % (freq/1e6) << std::endl;
    sdev->set_rx_freq(freq);
    std::cout << boost::format("Actual RX Freq: %f Mhz...") % (sdev->get_rx_freq()/1e6) << std::endl << std::endl;

    //set the rx rf gain
    std::cout << boost::format("Setting RX Gain: %f dB...") % gain << std::endl;
    sdev->set_rx_gain(gain);
    std::cout << boost::format("Actual RX Gain: %f dB...") % sdev->get_rx_gain() << std::endl << std::endl;

    //allocate recv buffer and metatdata
    uhd::rx_metadata_t md;
    std::vector<std::complex<float> > buff(num_bins);
    //------------------------------------------------------------------
    //-- Initialize
    //------------------------------------------------------------------
    initscr(); //curses init
    sdev->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    boost::system_time next_refresh = boost::get_system_time();

    //------------------------------------------------------------------
    //-- Main loop
    //------------------------------------------------------------------
    while (true){
        //read a buffer's worth of samples every iteration
        size_t num_rx_samps = sdev->get_device()->recv(
            &buff.front(), buff.size(), md,
            uhd::io_type_t::COMPLEX_FLOAT32,
            uhd::device::RECV_MODE_FULL_BUFF
        );
        if (num_rx_samps != buff.size()) continue;

        //check and update the display refresh condition
        if (boost::get_system_time() < next_refresh) continue;
        next_refresh = boost::get_system_time() + boost::posix_time::microseconds(long(1e6/frame_rate));

        //calculate the dft and create the ascii art frame
        acsii_art_dft::log_pwr_dft_type lpdft(
            acsii_art_dft::log_pwr_dft(&buff.front(), num_rx_samps)
        );
        std::string frame = acsii_art_dft::dft_to_plot(
            lpdft, COLS, LINES,
            sdev->get_rx_rate(),
            sdev->get_rx_freq(),
            dyn_rng, ref_lvl
        );

        //curses screen handling: clear and print frame
        clear();
        printw("%s", frame.c_str());

        //curses key handling: no timeout, any key to exit
        timeout(0);
        int ch = getch();
        if (ch != KEY_RESIZE and ch != ERR) break;
    }

    //------------------------------------------------------------------
    //-- Cleanup
    //------------------------------------------------------------------
    sdev->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
    endwin(); //curses done

    //finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;

    return 0;
}

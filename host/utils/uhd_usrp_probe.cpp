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

#include <uhd/utils/safe_main.hpp>
#include <uhd/device.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/usrp/device_props.hpp>
#include <uhd/usrp/mboard_props.hpp>
#include <uhd/usrp/dboard_props.hpp>
#include <uhd/usrp/codec_props.hpp>
#include <uhd/usrp/dsp_props.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <iostream>
#include <sstream>

namespace po = boost::program_options;
using namespace uhd;

static std::string indent(size_t level){
    return (level)? (indent(level-1) + " ") : "";
}

static std::string make_border(const std::string &text){
    std::stringstream ss;
    ss << boost::format("  _____________________________________________________") << std::endl;
    ss << boost::format(" /") << std::endl;
    std::vector<std::string> lines; boost::split(lines, text, boost::is_any_of("\n"));
    while (lines.back() == "") lines.pop_back(); //strip trailing newlines
    if (lines.size()) lines[0] = "    " + lines[0]; //indent the title line
    BOOST_FOREACH(const std::string &line, lines){
        ss << boost::format("|   %s") % line << std::endl;
    }
    //ss << boost::format(" \\____________________________________________________") << std::endl;
    return ss.str();
}

static std::string get_dsp_pp_string(const std::string &type, wax::obj dsp){
    std::stringstream ss;
    ss << boost::format("%s DSP: %s") % type % dsp[usrp::DSP_PROP_NAME].as<std::string>() << std::endl;
    //ss << std::endl;
    ss << boost::format("Codec Rate: %f Msps") % (dsp[usrp::DSP_PROP_CODEC_RATE].as<double>()/1e6) << std::endl;
    //ss << boost::format("Host Rate: %f Msps") % (dsp[usrp::DSP_PROP_HOST_RATE].as<double>()/1e6) << std::endl;
    //ss << boost::format("Freq Shift: %f Mhz") % (dsp[usrp::DSP_PROP_FREQ_SHIFT].as<double>()/1e6) << std::endl;
    return ss.str();
}

static std::string prop_names_to_pp_string(const prop_names_t &prop_names){
    std::stringstream ss; size_t count = 0;
    BOOST_FOREACH(const std::string &prop_name, prop_names){
        ss << ((count++)? ", " : "") << prop_name;
    }
    return ss.str();
}

static std::string get_subdev_pp_string(const std::string &type, wax::obj subdev){
    std::stringstream ss;
    ss << boost::format("%s Subdev: %s") % type % subdev[usrp::SUBDEV_PROP_NAME].as<std::string>() << std::endl;
    //ss << std::endl;

    prop_names_t ant_names(subdev[usrp::SUBDEV_PROP_ANTENNA_NAMES].as<prop_names_t>());
    ss << boost::format("Antennas: %s") % prop_names_to_pp_string(ant_names) << std::endl;

    freq_range_t freq_range(subdev[usrp::SUBDEV_PROP_FREQ_RANGE].as<freq_range_t>());
    ss << boost::format("Freq range: %.3f to %.3f Mhz") % (freq_range.min/1e6) % (freq_range.max/1e6) << std::endl;

    prop_names_t gain_names(subdev[usrp::SUBDEV_PROP_GAIN_NAMES].as<prop_names_t>());
    if (gain_names.size() == 0) ss << "Gain Elements: None" << std::endl;
    BOOST_FOREACH(const std::string &gain_name, gain_names){
        gain_range_t gain_range(subdev[named_prop_t(usrp::SUBDEV_PROP_GAIN_RANGE, gain_name)].as<gain_range_t>());
        ss << boost::format("Gain range %s: %.1f to %.1f step %.1f dB") % gain_name % gain_range.min % gain_range.max % gain_range.step << std::endl;
    }

    ss << boost::format("Connection Type: %c") % char(subdev[usrp::SUBDEV_PROP_CONNECTION].as<usrp::subdev_conn_t>()) << std::endl;
    ss << boost::format("Uses LO offset: %s") % (subdev[usrp::SUBDEV_PROP_USE_LO_OFFSET].as<bool>()? "Yes" : "No") << std::endl;

    return ss.str();
}

static std::string get_codec_pp_string(const std::string &type, wax::obj codec){
    std::stringstream ss;
    ss << boost::format("%s Codec: %s") % type % codec[usrp::CODEC_PROP_NAME].as<std::string>() << std::endl;
    //ss << std::endl;
    prop_names_t gain_names(codec[usrp::CODEC_PROP_GAIN_NAMES].as<prop_names_t>());
    if (gain_names.size() == 0) ss << "Gain Elements: None" << std::endl;
    BOOST_FOREACH(const std::string &gain_name, gain_names){
        gain_range_t gain_range(codec[named_prop_t(usrp::CODEC_PROP_GAIN_RANGE, gain_name)].as<gain_range_t>());
        ss << boost::format("Gain range %s: %.1f to %.1f step %.1f dB") % gain_name % gain_range.min % gain_range.max % gain_range.step << std::endl;
    }
    return ss.str();
}

static std::string get_dboard_pp_string(const std::string &type, wax::obj dboard){
    std::stringstream ss;
    ss << boost::format("%s Dboard: %s") % type % dboard[usrp::DBOARD_PROP_NAME].as<std::string>() << std::endl;
    //ss << std::endl;
    BOOST_FOREACH(const std::string &subdev_name, dboard[usrp::DBOARD_PROP_SUBDEV_NAMES].as<prop_names_t>()){
        ss << make_border(get_subdev_pp_string(type, dboard[named_prop_t(usrp::DBOARD_PROP_SUBDEV, subdev_name)]));
    }
    ss << make_border(get_codec_pp_string(type, dboard[usrp::DBOARD_PROP_CODEC]));
    return ss.str();
}

static std::string get_mboard_pp_string(wax::obj mboard){
    std::stringstream ss;
    ss << boost::format("Mboard: %s") % mboard[usrp::MBOARD_PROP_NAME].as<std::string>() << std::endl;
    //ss << std::endl;
    BOOST_FOREACH(const std::string &other_name, mboard[usrp::MBOARD_PROP_OTHERS].as<prop_names_t>()){
        try{
            ss << boost::format("%s: %s") % other_name % mboard[other_name].as<std::string>() << std::endl;
        } catch(...){}
    }
    BOOST_FOREACH(const std::string &dsp_name, mboard[usrp::MBOARD_PROP_RX_DSP_NAMES].as<prop_names_t>()){
        ss << make_border(get_dsp_pp_string("RX", mboard[named_prop_t(usrp::MBOARD_PROP_RX_DSP, dsp_name)]));
    }
    BOOST_FOREACH(const std::string &dsp_name, mboard[usrp::MBOARD_PROP_TX_DSP_NAMES].as<prop_names_t>()){
        ss << make_border(get_dsp_pp_string("TX", mboard[named_prop_t(usrp::MBOARD_PROP_TX_DSP, dsp_name)]));
    }
    BOOST_FOREACH(const std::string &dsp_name, mboard[usrp::MBOARD_PROP_RX_DBOARD_NAMES].as<prop_names_t>()){
        ss << make_border(get_dboard_pp_string("RX", mboard[named_prop_t(usrp::MBOARD_PROP_RX_DBOARD, dsp_name)]));
    }
    BOOST_FOREACH(const std::string &dsp_name, mboard[usrp::MBOARD_PROP_TX_DBOARD_NAMES].as<prop_names_t>()){
        ss << make_border(get_dboard_pp_string("TX", mboard[named_prop_t(usrp::MBOARD_PROP_TX_DBOARD, dsp_name)]));
    }
    return ss.str();
}


static std::string get_device_pp_string(device::sptr dev){
    std::stringstream ss;
    ss << boost::format("Device: %s") % (*dev)[usrp::DEVICE_PROP_NAME].as<std::string>() << std::endl;
    //ss << std::endl;
    BOOST_FOREACH(const std::string &mboard_name, (*dev)[usrp::DEVICE_PROP_MBOARD_NAMES].as<prop_names_t>()){
        ss << make_border(get_mboard_pp_string((*dev)[named_prop_t(usrp::DEVICE_PROP_MBOARD, mboard_name)]));
    }
    return ss.str();
}

int UHD_SAFE_MAIN(int argc, char *argv[]){
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>()->default_value(""), "device address args")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout << boost::format("UHD USRP Probe %s") % desc << std::endl;
        return ~0;
    }

    device::sptr dev = device::make(vm["args"].as<std::string>());

    std::cout << make_border(get_device_pp_string(dev)) << std::endl;

    return 0;
}

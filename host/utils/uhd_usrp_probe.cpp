//
// Copyright 2010-2011,2015-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/device.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/rfnoc/block_id.hpp>
#include <uhd/rfnoc_graph.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/utils/cast.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/version.hpp>
#include <boost/algorithm/string.hpp> //for split
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>

namespace po = boost::program_options;
using namespace uhd;

static std::string make_border(const std::string& text)
{
    std::stringstream ss;
    ss << boost::format("  _____________________________________________________")
       << std::endl;
    ss << boost::format(" /") << std::endl;
    std::vector<std::string> lines;
    boost::split(lines, text, boost::is_any_of("\n"));
    while (lines.back().empty())
        lines.pop_back(); // strip trailing newlines
    if (!lines.empty())
        lines[0] = "    " + lines[0]; // indent the title line
    for (const std::string& line : lines) {
        ss << boost::format("|   %s") % line << std::endl;
    }
    return ss.str();
}

static std::string get_dsp_pp_string(
    const std::string& type, property_tree::sptr tree, const fs_path& path)
{
    std::stringstream ss;
    ss << boost::format("%s DSP: %s") % type % path.leaf() << std::endl;
    ss << std::endl;
    meta_range_t freq_range = tree->access<meta_range_t>(path / "freq/range").get();
    ss << boost::format("Freq range: %.3f to %.3f MHz") % (freq_range.start() / 1e6)
              % (freq_range.stop() / 1e6)
       << std::endl;
    ;
    return ss.str();
}

static std::string prop_names_to_pp_string(const std::vector<std::string>& prop_names)
{
    std::stringstream ss;
    size_t count = 0;
    for (const std::string& prop_name : prop_names) {
        ss << ((count++) ? ", " : "") << prop_name;
    }
    return ss.str();
}

static std::string get_frontend_pp_string(
    const std::string& type, property_tree::sptr tree, const fs_path& path)
{
    std::stringstream ss;
    ss << boost::format("%s Frontend: %s") % type % path.leaf() << std::endl;

    ss << boost::format("Name: %s") % (tree->access<std::string>(path / "name").get())
       << std::endl;
    ss << boost::format("Antennas: %s")
              % prop_names_to_pp_string(
                    tree->access<std::vector<std::string>>(path / "antenna/options")
                        .get())
       << std::endl;
    if (tree->exists(path / "sensors")) {
        ss << boost::format("Sensors: %s")
                  % prop_names_to_pp_string(tree->list(path / "sensors"))
           << std::endl;
    }

    meta_range_t freq_range = tree->access<meta_range_t>(path / "freq/range").get();
    ss << boost::format("Freq range: %.3f to %.3f MHz") % (freq_range.start() / 1e6)
              % (freq_range.stop() / 1e6)
       << std::endl;

    std::vector<std::string> gain_names = tree->list(path / "gains");
    if (gain_names.empty())
        ss << "Gain Elements: None" << std::endl;
    for (const std::string& name : gain_names) {
        meta_range_t gain_range =
            tree->access<meta_range_t>(path / "gains" / name / "range").get();
        ss << boost::format("Gain range %s: %.1f to %.1f step %.1f dB") % name
                  % gain_range.start() % gain_range.stop() % gain_range.step()
           << std::endl;
    }
    if (tree->exists(path / "bandwidth" / "range")) {
        meta_range_t bw_range =
            tree->access<meta_range_t>(path / "bandwidth" / "range").get();
        ss << boost::format("Bandwidth range: %.1f to %.1f step %.1f Hz")
                  % bw_range.start() % bw_range.stop() % bw_range.step()
           << std::endl;
    }

    ss << boost::format("Connection Type: %s")
              % (tree->access<std::string>(path / "connection").get())
       << std::endl;
    ss << boost::format("Uses LO offset: %s")
              % ((tree->exists(path / "use_lo_offset")
                     and tree->access<bool>(path / "use_lo_offset").get())
                        ? "Yes"
                        : "No")
       << std::endl;

    return ss.str();
}

static std::string get_codec_pp_string(
    const std::string& type, property_tree::sptr tree, const fs_path& path)
{
    std::stringstream ss;
    if (tree->exists(path / "name")) {
        ss << boost::format("%s Codec: %s") % type % path.leaf() << std::endl;

        ss << boost::format("Name: %s") % (tree->access<std::string>(path / "name").get())
           << std::endl;
        std::vector<std::string> gain_names = tree->list(path / "gains");
        if (gain_names.empty())
            ss << "Gain Elements: None" << std::endl;
        for (const std::string& name : gain_names) {
            meta_range_t gain_range =
                tree->access<meta_range_t>(path / "gains" / name / "range").get();
            ss << boost::format("Gain range %s: %.1f to %.1f step %.1f dB") % name
                      % gain_range.start() % gain_range.stop() % gain_range.step()
               << std::endl;
        }
    }
    return ss.str();
}

static std::string get_dboard_pp_string(
    const std::string& type,
    const std::string& name,
    property_tree::sptr tree,
    const fs_path& path)
{
    std::stringstream ss;
    ss << boost::format("%s Dboard: %s") % type % name << std::endl;
    const std::string prefix = (type == "RX") ? "rx" : "tx";
    if (tree->exists(path / (prefix + "_eeprom"))) {
        usrp::dboard_eeprom_t db_eeprom =
            tree->access<usrp::dboard_eeprom_t>(path / (prefix + "_eeprom")).get();
        if (db_eeprom.id != usrp::dboard_id_t::none())
            ss << boost::format("ID: %s") % db_eeprom.id.to_pp_string() << std::endl;
        if (not db_eeprom.serial.empty())
            ss << boost::format("Serial: %s") % db_eeprom.serial << std::endl;
        if (not db_eeprom.revision.empty()) {
            ss << "Revision: " << db_eeprom.revision << std::endl;
        }
        if (type == "TX" and tree->exists(path / "gdb_eeprom")) {
            usrp::dboard_eeprom_t gdb_eeprom =
                tree->access<usrp::dboard_eeprom_t>(path / "gdb_eeprom").get();
            if (gdb_eeprom.id != usrp::dboard_id_t::none())
                ss << boost::format("ID: %s") % gdb_eeprom.id.to_pp_string() << std::endl;
            if (not gdb_eeprom.serial.empty())
                ss << boost::format("Serial: %s") % gdb_eeprom.serial << std::endl;
            if (not gdb_eeprom.revision.empty()) {
                ss << "Revision: " << gdb_eeprom.revision << std::endl;
            }
        }
    }
    if (tree->exists(path / (prefix + "_frontends"))) {
        for (const std::string& name : tree->list(path / (prefix + "_frontends"))) {
            ss << make_border(get_frontend_pp_string(
                type, tree, path / (prefix + "_frontends") / name));
        }
    }
    const fs_path codec_path =
        path.branch_path().branch_path() / (prefix + "_codecs") / path.leaf();
    if (tree->exists(codec_path)) {
        ss << make_border(get_codec_pp_string(type, tree, codec_path));
    }
    return ss.str();
}

static std::string get_rfnoc_blocks_pp_string(rfnoc::rfnoc_graph::sptr graph)
{
    std::stringstream ss;
    ss << "RFNoC blocks on this device:" << std::endl << std::endl;
    for (const auto& name : graph->find_blocks("")) {
        ss << "* " << name.to_string() << std::endl;
    }
    return ss.str();
}

static std::string get_rfnoc_connections_pp_string(rfnoc::rfnoc_graph::sptr graph)
{
    std::stringstream ss;
    ss << "Static connections on this device:" << std::endl << std::endl;
    for (const auto& edge : graph->enumerate_static_connections()) {
        ss << "* " << edge.to_string() << std::endl;
    }
    return ss.str();
}

static std::string get_rfnoc_pp_string(
    rfnoc::rfnoc_graph::sptr graph, property_tree::sptr tree)
{
    std::stringstream ss;
    ss << make_border(get_rfnoc_blocks_pp_string(graph));
    ss << make_border(get_rfnoc_connections_pp_string(graph));
    auto radio_blocks = graph->find_blocks("Radio");
    for (std::string block : radio_blocks) {
        ss << make_border(get_dboard_pp_string("TX", block, tree, "blocks" / block / "dboard"));
        ss << make_border(get_dboard_pp_string("RX", block, tree, "blocks" / block / "dboard"));
    }
    return ss.str();
}

static std::string get_mboard_pp_string(property_tree::sptr tree, const fs_path& path)
{
    std::stringstream ss;
    ss << boost::format("Mboard: %s") % (tree->access<std::string>(path / "name").get())
       << std::endl;

    if (tree->exists(path / "eeprom")) {
        usrp::mboard_eeprom_t mb_eeprom =
            tree->access<usrp::mboard_eeprom_t>(path / "eeprom").get();
        for (const std::string& key : mb_eeprom.keys()) {
            if (not mb_eeprom[key].empty())
                ss << boost::format("%s: %s") % key % mb_eeprom[key] << std::endl;
        }
    } else {
        ss << "No mboard EEPROM found." << std::endl;
    }
    if (tree->exists(path / "fw_version")) {
        ss << "FW Version: " << tree->access<std::string>(path / "fw_version").get()
           << std::endl;
    }
    if (tree->exists(path / "mpm_version")) {
        ss << "MPM Version: " << tree->access<std::string>(path / "mpm_version").get()
           << std::endl;
    }
    if (tree->exists(path / "fpga_version")) {
        ss << "FPGA Version: " << tree->access<std::string>(path / "fpga_version").get()
           << std::endl;
    }
    if (tree->exists(path / "fpga_version_hash")) {
        ss << "FPGA git hash: "
           << tree->access<std::string>(path / "fpga_version_hash").get() << std::endl;
    }
    if (tree->exists("/blocks")) {
        ss << "RFNoC capable: Yes" << std::endl;
    }
    ss << std::endl;
    try {
        if (tree->exists(path / "time_source" / "options")) {
            const std::vector<std::string> time_sources =
                tree->access<std::vector<std::string>>(path / "time_source" / "options")
                    .get();
            ss << "Time sources:  " << prop_names_to_pp_string(time_sources) << std::endl;
        }
        if (tree->exists(path / "clock_source" / "options")) {
            const std::vector<std::string> clock_sources =
                tree->access<std::vector<std::string>>(path / "clock_source" / "options")
                    .get();
            ss << "Clock sources: " << prop_names_to_pp_string(clock_sources)
               << std::endl;
        }
        if (tree->exists(path / "sensors")) {
            ss << "Sensors: " << prop_names_to_pp_string(tree->list(path / "sensors"))
               << std::endl;
        }
        if (tree->exists(path / "rx_dsps")) {
            for (const std::string& name : tree->list(path / "rx_dsps")) {
                ss << make_border(get_dsp_pp_string("RX", tree, path / "rx_dsps" / name));
            }
        }
        if (tree->exists(path / "dboards")) {
            for (const std::string& name : tree->list(path / "dboards")) {
                ss << make_border(
                    get_dboard_pp_string("RX", name, tree, path / "dboards" / name));
            }
            if (tree->exists(path / "tx_dsps")) {
                for (const std::string& name : tree->list(path / "tx_dsps")) {
                    ss << make_border(
                        get_dsp_pp_string("TX", tree, path / "tx_dsps" / name));
                }
            }
            for (const std::string& name : tree->list(path / "dboards")) {
                ss << make_border(
                    get_dboard_pp_string("TX", name, tree, path / "dboards" / name));
            }
        }
    } catch (const uhd::lookup_error& ex) {
        std::cout << "Exited device probe on " << ex.what() << std::endl;
    }
    return ss.str();
}


static std::string get_device_pp_string(property_tree::sptr tree)
{
    std::stringstream ss;
    ss << boost::format("Device: %s") % (tree->access<std::string>("/name").get())
       << std::endl;
    for (const std::string& name : tree->list("/mboards")) {
        ss << make_border(get_mboard_pp_string(tree, "/mboards/" + name));
    }
    return ss.str();
}

void print_tree(const uhd::fs_path& path, uhd::property_tree::sptr tree)
{
    std::cout << path << std::endl;
    for (const std::string& name : tree->list(path)) {
        print_tree(path / name, tree);
    }
}

namespace {

uint32_t str2uint32(const std::string& str)
{
    if (str.find("0x") == 0) {
        return cast::hexstr_cast<uint32_t>(str);
    }
    return boost::lexical_cast<uint32_t>(str);
}

void shell_print_help()
{
    std::cout << "Commands:\n\n"
              << "poke32 $addr $data     : Write $data to $addr\n"
              << "peek32 $addr           : Read from $addr and print\n"
              << "help                   : Show this\n"
              << "quit                   : Terminate shell\n"
              << std::endl;
}

void run_interactive_regs_shell(rfnoc::noc_block_base::sptr blk_ctrl)
{
    std::cout << "<<< Interactive Block Peeker/Poker >>>" << std::endl;
    std::cout << "Type 'help' to get a list of commands." << std::endl;
    while (true) {
        std::string input;
        std::cout << ">>> " << std::flush;
        std::getline(std::cin, input);
        std::stringstream ss(input);
        std::string command;
        ss >> command;
        if (command == "poke32") {
            std::string addr_s, data_s;
            uint32_t addr, data;
            try {
                ss >> addr_s >> data_s;
                addr = str2uint32(addr_s);
                data = str2uint32(data_s);
            } catch (std::exception&) {
                std::cout << "Usage: poke32 $addr $data" << std::endl;
                continue;
            }
            blk_ctrl->regs().poke32(addr, data);
        }
        if (command == "peek32") {
            std::string addr_s;
            uint32_t addr;
            try {
                ss >> addr_s;
                addr = str2uint32(addr_s);
            } catch (std::exception&) {
                std::cout << "Usage: peek32 $addr" << std::endl;
                continue;
            }
            std::cout << "==> " << std::hex << blk_ctrl->regs().peek32(addr) << std::dec
                      << std::endl;
        }

        if (input == "help") {
            shell_print_help();
        }
        if (input == "quit") {
            return;
        }
    }
}

} // namespace

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "help message")
        ("version", "print the version string and exit")
        ("args", po::value<std::string>()->default_value(""), "device address args")
        ("tree", "specify to print a complete property tree")
        ("string", po::value<std::string>(), "query a string value from the property tree")
        ("double", po::value<std::string>(), "query a double precision floating point value from the property tree")
        ("int", po::value<std::string>(), "query a integer value from the property tree")
        ("sensor", po::value<std::string>(), "query a sensor value from the property tree")
        ("range", po::value<std::string>(), "query a range (gain, bandwidth, frequency, ...)  from the property tree")
        ("vector", "when querying a string, interpret that as std::vector")
        ("init-only", "skip all queries, only initialize device")
        ("interactive-reg-iface", po::value<std::string>(), "RFNoC devices only: Spawn a shell to interactively peek and poke registers on RFNoC blocks")
    ;
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // print the help message
    if (vm.count("help")) {
        std::cout << boost::format("UHD USRP Probe %s") % desc << std::endl;
        return EXIT_FAILURE;
    }

    if (vm.count("version")) {
        std::cout << uhd::get_version_string() << std::endl;
        return EXIT_SUCCESS;
    }

    device::sptr dev         = device::make(vm["args"].as<std::string>());
    property_tree::sptr tree = dev->get_tree();
    rfnoc::rfnoc_graph::sptr graph;
    try {
        graph = rfnoc::rfnoc_graph::make(vm["args"].as<std::string>());
    } catch (uhd::key_error&) {
        // pass
    }

    if (vm.count("string")) {
        if (vm.count("vector")) {
            std::vector<std::string> str_vector =
                tree->access<std::vector<std::string>>(vm["string"].as<std::string>())
                    .get();
            std::cout << "(";
            for (const std::string& str : str_vector) {
                std::cout << str << ",";
            }
            std::cout << ")" << std::endl;
        } else {
            std::cout << tree->access<std::string>(vm["string"].as<std::string>()).get()
                      << std::endl;
        }
        return EXIT_SUCCESS;
    }

    if (vm.count("double")) {
        std::cout << tree->access<double>(vm["double"].as<std::string>()).get()
                  << std::endl;
        return EXIT_SUCCESS;
    }

    if (vm.count("int")) {
        std::cout << tree->access<int>(vm["int"].as<std::string>()).get() << std::endl;
        return EXIT_SUCCESS;
    }

    if (vm.count("sensor")) {
        std::cout << tree->access<uhd::sensor_value_t>(vm["sensor"].as<std::string>())
                         .get()
                         .value
                  << std::endl;
        return EXIT_SUCCESS;
    }

    if (vm.count("range")) {
        meta_range_t range =
            tree->access<meta_range_t>(vm["range"].as<std::string>()).get();
        std::cout << boost::format("%.1f:%.1f:%.1f") % range.start() % range.step()
                         % range.stop()
                  << std::endl;
        return EXIT_SUCCESS;
    }

    if (vm.count("interactive-reg-iface")) {
        if (!graph) {
            std::cout << "ERROR: --interactive-reg-iface requires an RFNoC device!"
                      << std::endl;
            return EXIT_FAILURE;
        }
        const rfnoc::block_id_t block_id(vm["interactive-reg-iface"].as<std::string>());
        auto block_ctrl = graph->get_block(block_id);
        run_interactive_regs_shell(block_ctrl);
        return EXIT_SUCCESS;
    }

    if (vm.count("tree") != 0) {
        print_tree("/", tree);
    } else if (not vm.count("init-only")) {
        std::string device_pp_string = get_device_pp_string(tree);
        if (graph) {
            device_pp_string += get_rfnoc_pp_string(graph, tree);
        }
        std::cout << make_border(device_pp_string) << std::endl;
    }

    return EXIT_SUCCESS;
}

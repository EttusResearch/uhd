//
// Copyright 2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/constants.hpp>
#include <uhd/rfnoc/blockdef.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/paths.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <cstdlib>

using namespace uhd;
using namespace uhd::rfnoc;
namespace fs = boost::filesystem;
namespace pt = boost::property_tree;

static const fs::path XML_BLOCKS_SUBDIR("blocks");
static const fs::path XML_COMPONENTS_SUBDIR("components");
static const fs::path XML_EXTENSION(".xml");


/****************************************************************************
 * port_t stuff
 ****************************************************************************/
const device_addr_t blockdef::port_t::PORT_ARGS(
        "name,"
        "type,"
        "vlen=0,"
        "pkt_size=0,"
        "optional=0,"
        "bursty=0,"
        "port,"
);

blockdef::port_t::port_t()
{
    // This guarantees that we can access these keys
    // even if they were never initialized:
    for(const std::string &key:  PORT_ARGS.keys()) {
        set(key, PORT_ARGS[key]);
    }
}

bool blockdef::port_t::is_variable(const std::string &key) const
{
    const std::string &val = get(key);
    return (val[0] == '$');
}

bool blockdef::port_t::is_keyword(const std::string &key) const
{
    const std::string &val = get(key);
    return (val[0] == '%');
}

bool blockdef::port_t::is_valid() const
{
    // Check we have all the keys:
    for(const std::string &key:  PORT_ARGS.keys()) {
        if (not has_key(key)) {
            return false;
        }
    }

    // Twelve of the clock, all seems well
    return true;
}

std::string blockdef::port_t::to_string() const
{
    std::string result;
    for(const std::string &key:  PORT_ARGS.keys()) {
        if (has_key(key)) {
            result += str(boost::format("%s=%s,") % key % get(key));
        }
    }

    return result;
}

/****************************************************************************
 * arg_t stuff
 ****************************************************************************/
const device_addr_t blockdef::arg_t::ARG_ARGS(
    // List all tags/args an <arg> can have here:
        "name,"
        "type,"
        "value,"
        "check,"
        "check_message,"
        "action,"
        "port=0,"
);

const std::set<std::string> blockdef::arg_t::VALID_TYPES = {
    // List all tags/args a <type> can have here:
    "string",
    "int",
    "int_vector",
    "double"
};

blockdef::arg_t::arg_t()
{
    // This guarantees that we can access these keys
    // even if they were never initialized:
    for(const std::string &key:  ARG_ARGS.keys()) {
        set(key, ARG_ARGS[key]);
    }
}

bool blockdef::arg_t::is_valid() const
{
    // 1. Check we have all the keys:
    for(const std::string &key:  ARG_ARGS.keys()) {
        if (not has_key(key)) {
            return false;
        }
    }

    // 2. Check arg type is valid
    if (not get("type").empty() and not VALID_TYPES.count(get("type"))) {
        return false;
    }

    // Twelve of the clock, all seems well
    return true;
}

std::string blockdef::arg_t::to_string() const
{
    std::string result;
    for(const std::string &key:  ARG_ARGS.keys()) {
        if (has_key(key)) {
            result += str(boost::format("%s=%s,") % key % get(key));
        }
    }

    return result;
}

/****************************************************************************
 * blockdef_impl stuff
 ****************************************************************************/
class blockdef_xml_impl : public blockdef
{
public:
    enum xml_repr_t {
        DESCRIBES_BLOCK,
        DESCRIBES_COMPONENT
    };

    //! Returns a list of base paths for the XML files.
    // It is assumed that block definitions are in a subdir with name
    // XML_BLOCKS_SUBDIR and component definitions in a subdir with name
    // XML_COMPONENTS_SUBDIR
    static std::vector<boost::filesystem::path> get_xml_paths()
    {
        std::vector<boost::filesystem::path> paths;

        // Path from environment variable
        if (std::getenv(XML_PATH_ENV.c_str()) != NULL) {
            paths.push_back(boost::filesystem::path(std::getenv(XML_PATH_ENV.c_str())));
        }

        // Finally, the default path
        const boost::filesystem::path pkg_path = uhd::get_pkg_path();
        paths.push_back(pkg_path / XML_DEFAULT_PATH);

        return paths;
    }

    //! Matches a NoC ID through substring matching
    static bool match_noc_id(const std::string &lhs_, uint64_t rhs_)
    {
        // Sanitize input: Make both values strings with all uppercase
        // characters and no leading 0x. Check inputs are valid.
        std::string lhs = boost::to_upper_copy(lhs_);
        std::string rhs = str(boost::format("%016X") % rhs_);
        if (lhs.size() > 2 and lhs[0] == '0' and lhs[1] == 'X') {
            lhs = lhs.substr(2);
        }
        UHD_ASSERT_THROW(rhs.size() == 16);
        if (lhs.size() < 4 or lhs.size() > 16) {
            throw uhd::value_error(str(boost::format(
                    "%s is not a valid NoC ID (must be hexadecimal, min 4 and max 16 characters)"
            ) % lhs_));
        }

        // OK, all good now. Next, we try and match the substring lhs in rhs:
        return (rhs.find(lhs) == 0);
    }

    //! Open the file at filename and see if it's a block definition for the given NoC ID
    static bool has_noc_id(uint64_t noc_id, const fs::path &filename)
    {
        pt::ptree propt;
        try {
            read_xml(filename.string(), propt);
            for(pt::ptree::value_type &v:  propt.get_child("nocblock.ids")) {
                if (v.first == "id" and match_noc_id(v.second.data(), noc_id)) {
                    return true;
                }
            }
        } catch (std::exception &e) {
            UHD_LOGGER_WARNING("RFNOC")
                << "has_noc_id(): caught exception " << e.what()
                << " while parsing file: " << filename.string();
            return false;
        }
        return false;
    }

    blockdef_xml_impl(const fs::path &filename, uint64_t noc_id, xml_repr_t type=DESCRIBES_BLOCK) :
        _type(type),
        _noc_id(noc_id)
    {
        UHD_LOGGER_DEBUG("RFNOC") <<
            boost::format("Reading XML file %s for NOC ID 0x%08X")
            % filename.string().c_str()
            % noc_id
        ;
        read_xml(filename.string(), _pt);
        try {
            // Check key is valid
            get_key();
            // Check name is valid
            get_name();
            // Check there's at least one port
            ports_t in = get_input_ports();
            ports_t out = get_output_ports();
            if (in.empty() and out.empty()) {
                throw uhd::runtime_error("Block does not define inputs or outputs.");
            }
            // Check args are valid
            get_args();
            // TODO any more checks?
        } catch (const std::exception &e) {
            throw uhd::runtime_error(str(
                        boost::format("Invalid block definition in %s: %s")
                        % filename.string() % e.what()
            ));
        }
    }

    bool is_block() const
    {
        return _type == DESCRIBES_BLOCK;
    }

    bool is_component() const
    {
        return _type == DESCRIBES_COMPONENT;
    }

    std::string get_key() const
    {
        try {
            return _pt.get<std::string>("nocblock.key");
        } catch (const pt::ptree_bad_path &) {
            return _pt.get<std::string>("nocblock.blockname");
        }
    }

    std::string get_name() const
    {
        return _pt.get<std::string>("nocblock.blockname");
    }

    uint64_t noc_id() const
    {
        return _noc_id;
    }

    ports_t get_input_ports()
    {
        return _get_ports("sink");
    }

    ports_t get_output_ports()
    {
        return _get_ports("source");
    }

    ports_t _get_ports(const std::string &port_type)
    {
        std::set<size_t> port_numbers;
        size_t n_ports = 0;
        ports_t ports;
        for(pt::ptree::value_type &v:  _pt.get_child("nocblock.ports")) {
            if (v.first != port_type) continue;
            // Now we have the correct sink or source node:
            port_t port;
            for(const std::string &key:  port_t::PORT_ARGS.keys()) {
                port[key] = v.second.get(key, port_t::PORT_ARGS[key]);
            }
            // We have to be extra-careful with the port numbers:
            if (port["port"].empty()) {
                port["port"] = std::to_string(n_ports);
            }
            size_t new_port_number;
            try {
                new_port_number = boost::lexical_cast<size_t>(port["port"]);
            } catch (const boost::bad_lexical_cast &e) {
                throw uhd::value_error(str(
                        boost::format("Invalid port number '%s' on port '%s'")
                        % port["port"] % port["name"]
                ));
            }
            if (port_numbers.count(new_port_number) or new_port_number > MAX_NUM_PORTS) {
                throw uhd::value_error(str(
                        boost::format("Port '%s' has invalid port number %d!")
                        % port["name"] % new_port_number
                ));
            }
            port_numbers.insert(new_port_number);
            n_ports++;
            ports.push_back(port);
        }
        return ports;
    }

    std::vector<size_t> get_all_port_numbers()
    {
        std::set<size_t> set_ports;
        for(const port_t &port:  get_input_ports()) {
            set_ports.insert(boost::lexical_cast<size_t>(port["port"]));
        }
        for(const port_t &port:  get_output_ports()) {
            set_ports.insert(boost::lexical_cast<size_t>(port["port"]));
        }
        return std::vector<size_t>(set_ports.begin(), set_ports.end());
    }


    blockdef::args_t get_args()
    {
        args_t args;
        bool is_valid = true;
        pt::ptree def;
        for(pt::ptree::value_type &v: _pt.get_child("nocblock.args",  def)) {
            arg_t arg;
            if (v.first != "arg") continue;
            for(const std::string &key:  arg_t::ARG_ARGS.keys()) {
                arg[key] = v.second.get(key, arg_t::ARG_ARGS[key]);
            }
            if (arg["type"].empty()) {
                arg["type"] = "string";
            }
            if (not arg.is_valid()) {
                UHD_LOGGER_WARNING("RFNOC")
                    << "Found invalid argument: " << arg.to_string();
                is_valid = false;
            }
            args.push_back(arg);
        }
        if (not is_valid) {
            throw uhd::runtime_error(str(
                    boost::format("Found invalid arguments for block %s.")
                    % get_name()
            ));
        }
        return args;
    }

    registers_t get_settings_registers()
    {
        return _get_regs("setreg");
    }

    registers_t get_readback_registers()
    {
        return _get_regs("readback");
    }

    registers_t _get_regs(const std::string &reg_type)
    {
        registers_t registers;
        pt::ptree def;
        for(pt::ptree::value_type &v: _pt.get_child("nocblock.registers",  def)) {
            if (v.first != reg_type) continue;
            registers[v.second.get<std::string>("name")] =
                boost::lexical_cast<size_t>(v.second.get<size_t>("address"));
        }
        return registers;
    }


private:

    //! Tells us if is this for a NoC block, or a component.
    const xml_repr_t _type;
    //! The NoC-ID as reported (there may be several valid NoC IDs, this is the one used)
    const uint64_t _noc_id;

    //! This is a boost property tree, not the same as
    // our property tree.
    pt::ptree _pt;

};

blockdef::sptr blockdef::make_from_noc_id(uint64_t noc_id)
{
    std::vector<fs::path> paths = blockdef_xml_impl::get_xml_paths();
    std::vector<fs::path> valid;

    // Check if any of the paths exist
    for (const auto& base_path : paths) {
        fs::path this_path = base_path / XML_BLOCKS_SUBDIR;
        if (fs::exists(this_path) and fs::is_directory(this_path)) {
            valid.push_back(this_path);
        }
    }

    if (valid.empty()) {
        throw uhd::assertion_error(
            "Failed to find a valid XML path for RFNoC blocks.\n"
            "Try setting the enviroment variable UHD_RFNOC_DIR "
            "to the correct location"
        );
    }

    // Iterate over all paths
    for (const auto& path : valid) {
        // Iterate over all .xml files
        fs::directory_iterator end_itr;
        for (fs::directory_iterator i(path); i != end_itr; ++i) {
            if (not fs::exists(*i) or fs::is_directory(*i) or fs::is_empty(*i)) {
                continue;
            }
            if (i->path().filename().extension() != XML_EXTENSION) {
                continue;
            }
            if (blockdef_xml_impl::has_noc_id(noc_id, i->path())) {
                return blockdef::sptr(new blockdef_xml_impl(i->path(), noc_id));
            }
        }
    }

    return blockdef::sptr();
}
// vim: sw=4 et:

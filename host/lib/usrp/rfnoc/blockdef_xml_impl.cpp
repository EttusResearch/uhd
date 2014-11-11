//
// Copyright 2014 Ettus Research LLC
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

#include <uhd/exception.hpp>
#include <uhd/usrp/rfnoc/constants.hpp>
#include <uhd/usrp/rfnoc/blockdef.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/paths.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <cstdlib>

using namespace uhd::rfnoc;
namespace fs = boost::filesystem;
namespace pt = boost::property_tree;

static const fs::path XML_BLOCKS_SUBDIR("blocks");
static const fs::path XML_COMPONENTS_SUBDIR("components");
static const fs::path XML_EXTENSION(".xml");

class blockdef_xml_impl : public blockdef
{
public:
    enum xml_repr_t {
        DESCRIBES_BLOCK,
        DESCRIBES_COMPONENT
    };

    //! Returns a list of base paths for the XML files.
    // It is assumed that block definitions are in a subdir called 'blocks'
    // and component definitions in a subdir called 'components'.
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
    //
    static bool match_noc_id(const std::string &lhs_, boost::uint64_t rhs_)
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
    static bool has_noc_id(boost::uint64_t noc_id, const fs::path &filename)
    {
        boost::property_tree::ptree propt;
        try {
            read_xml(filename.native(), propt);
            BOOST_FOREACH(pt::ptree::value_type &v, propt.get_child("nocblock.ids")) {
                if (v.first == "id" and match_noc_id(v.second.data(), noc_id)) {
                    return true;
                }
            }
        } catch (std::exception &e) {
            UHD_MSG(warning) << "has_noc_id(): caught exception " << e.what() << std::endl;
            return false;
        }
        return false;
    }

    blockdef_xml_impl(const fs::path &filename, boost::uint64_t noc_id, xml_repr_t type=DESCRIBES_BLOCK) :
        _type(type),
        _noc_id(noc_id)
    {
        read_xml(filename.native(), _pt);
        // TODO: check validity of XML
        // - Read all important stuff in constructor
        //   - Name
        //   - IO Sigs
    }

    bool is_block() const
    {
        return _type == DESCRIBES_BLOCK;
    }

    bool is_component() const
    {
        return _type == DESCRIBES_COMPONENT;
    }

    std::string get_name() const
    {
        return _pt.get<std::string>("nocblock.blockname");
    }

    boost::uint64_t noc_id() const
    {
        return _noc_id;
    }


private:

    //! Tells us if is this for a NoC block, or a component.
    const xml_repr_t _type;
    //! The NoC-ID as reported (there may be several valid NoC IDs, this is the one used)
    const boost::uint64_t _noc_id;

    //! This is a boost property tree, not the same as
    // our property tree.
    boost::property_tree::ptree _pt;

};

blockdef::sptr blockdef::make_from_noc_id(boost::uint64_t noc_id)
{
    std::vector<fs::path> paths = blockdef_xml_impl::get_xml_paths();
    // Iterate over all paths
    BOOST_FOREACH(const fs::path &base_path, paths) {
        UHD_VAR(base_path);
        fs::path this_path = base_path / XML_BLOCKS_SUBDIR;
        if (not fs::exists(this_path) or not fs::is_directory(this_path)) {
            continue;
        }
        // Iterate over all .xml files
        fs::directory_iterator end_itr;
        for (fs::directory_iterator i(this_path); i != end_itr; ++i) {
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

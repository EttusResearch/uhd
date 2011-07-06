//
// Copyright 2011 Ettus Research LLC
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

#include "validate_subdev_spec.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/assert_has.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>

using namespace uhd;
using namespace uhd::usrp;

namespace uhd{ namespace usrp{

    static std::ostream& operator<< (std::ostream &out, const subdev_spec_pair_t &pair){
        out << pair.db_name << ":" << pair.sd_name;
        return out;
    }

}}

void uhd::usrp::validate_subdev_spec(
    property_tree::sptr tree,
    const subdev_spec_t &spec,
    const std::string &type,
    const std::string &mb
){
    const size_t num_dsps = tree->list(str(boost::format("/mboards/%s/%s_dsps") % mb % type)).size();

    //sanity checking on the length
    if (spec.size() == 0) throw uhd::value_error(str(boost::format(
        "Empty %s subdevice specification is not supported.\n"
    ) % type));
    if (spec.size() > num_dsps) throw uhd::value_error(str(boost::format(
        "The subdevice specification \"%s\" is too long.\n"
        "The user specified %u channels, but there are only %u %s dsps on mboard %s.\n"
    ) % spec.to_string() % spec.size() % num_dsps % type % mb));

    //make a list of all possible specs
    subdev_spec_t all_specs;
    BOOST_FOREACH(const std::string &db, tree->list(str(boost::format("/mboards/%s/dboards") % mb))){
        BOOST_FOREACH(const std::string &sd, tree->list(str(boost::format("/mboards/%s/dboards/%s/%s_frontends") % mb % db % type))){
            all_specs.push_back(subdev_spec_pair_t(db, sd));
        }
    }

    //validate that the spec is possible
    BOOST_FOREACH(const subdev_spec_pair_t &pair, spec){
        uhd::assert_has(all_specs, pair, str(boost::format("%s subdevice specification on mboard %s") % type % mb));
    }

    //enable selected frontends, disable others
    BOOST_FOREACH(const subdev_spec_pair_t &pair, all_specs){
        const bool enb = uhd::has(spec, pair);
        tree->access<bool>(str(boost::format(
            "/mboards/%s/dboards/%s/%s_frontends/%s/enabled"
        ) % mb % pair.db_name % type % pair.sd_name)).set(enb);
    }
}

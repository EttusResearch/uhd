//
// Copyright 2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/usrp/common/validate_subdev_spec.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/assert_has.hpp>
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
    for(const std::string &db:  tree->list(str(boost::format("/mboards/%s/dboards") % mb))){
        for(const std::string &sd:  tree->list(str(boost::format("/mboards/%s/dboards/%s/%s_frontends") % mb % db % type))){
            all_specs.push_back(subdev_spec_pair_t(db, sd));
        }
    }

    //validate that the spec is possible
    for(const subdev_spec_pair_t &pair:  spec){
        uhd::assert_has(all_specs, pair, str(boost::format("%s subdevice specification on mboard %s") % type % mb));
    }

    //enable selected frontends, disable others
    for(const subdev_spec_pair_t &pair:  all_specs){
        const bool enb = uhd::has(spec, pair);
        tree->access<bool>(str(boost::format(
            "/mboards/%s/dboards/%s/%s_frontends/%s/enabled"
        ) % mb % pair.db_name % type % pair.sd_name)).set(enb);
    }
}

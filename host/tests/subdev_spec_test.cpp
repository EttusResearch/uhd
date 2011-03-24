//
// Copyright 2010-2011 Ettus Research LLC
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

#include <boost/test/unit_test.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <boost/foreach.hpp>
#include <iostream>

BOOST_AUTO_TEST_CASE(test_subdevice_spec){
    std::cout << "Testing subdevice specification..." << std::endl;

    //load the subdev spec with something
    uhd::usrp::subdev_spec_t sd_spec;
    sd_spec.push_back(uhd::usrp::subdev_spec_pair_t("A", "AB"));
    sd_spec.push_back(uhd::usrp::subdev_spec_pair_t("B", "AB"));

    //convert to and from args string
    std::cout << "Pretty Print: " << std::endl << sd_spec.to_pp_string();
    std::string markup_str = sd_spec.to_string();
    std::cout << "Markup String: " << markup_str << std::endl;
    uhd::usrp::subdev_spec_t new_sd_spec(markup_str);

    //they should be the same size
    BOOST_REQUIRE_EQUAL(sd_spec.size(), new_sd_spec.size());

    //the contents should match
    for (size_t i = 0; i < sd_spec.size(); i++){
        BOOST_CHECK_EQUAL(sd_spec.at(i).db_name, new_sd_spec.at(i).db_name);
        BOOST_CHECK_EQUAL(sd_spec.at(i).sd_name, new_sd_spec.at(i).sd_name);
    }
}

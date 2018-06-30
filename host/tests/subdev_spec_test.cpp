//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/test/unit_test.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <iostream>

BOOST_AUTO_TEST_CASE(test_subdevice_spec){
    std::cout << "Testing subdevice specification..." << std::endl;

    //load the subdev spec with something
    uhd::usrp::subdev_spec_t sd_spec;
    sd_spec.push_back(uhd::usrp::subdev_spec_pair_t("A", "AB"));
    sd_spec.push_back(uhd::usrp::subdev_spec_pair_t("B", "AB"));

    //create a subdev_spec with something different
    uhd::usrp::subdev_spec_t diff_sd_spec;
    diff_sd_spec.push_back(uhd::usrp::subdev_spec_pair_t("B", "BA"));
    diff_sd_spec.push_back(uhd::usrp::subdev_spec_pair_t("B", "BA"));

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

        BOOST_CHECK(sd_spec.at(i) == new_sd_spec.at(i));
        BOOST_CHECK(sd_spec.at(i) != diff_sd_spec.at(i));
    }
}

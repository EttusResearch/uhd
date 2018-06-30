//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/cal/power_container.hpp>
#include <uhd/exception.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <fstream>

using namespace uhd;
using namespace uhd::cal;

static const double eps = 1e-8;

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_power_container_bilinear){
////////////////////////////////////////////////////////////////////////

    // Create the data container
    power_container::sptr container = power_container::make();

    // Create some data points to add
    std::vector<double> pt0(2, 0.0);
    std::vector<double> pt1(2, 0.0);
    std::vector<double> pt2(2, 0.0);
    std::vector<double> pt3(2, 2.0);

    pt1[0] = 2.0;
    pt2[1] = 2.0;

    container->add(1.0, pt0);
    container->add(1.0, pt1);
    container->add(0.0, pt2);
    container->add(0.0, pt3);

    // Add points to interpolate against
    std::vector<double> test0(2, 1.0);
    std::vector<double> test1(2, 1.5);
    std::vector<double> test2(2, 0.0);
    test2[1] = 1.0;

    BOOST_CHECK_CLOSE(container->get(test0), 0.50, eps);
    BOOST_CHECK_CLOSE(container->get(test1), 0.25, eps);
    BOOST_CHECK_CLOSE(container->get(test2), 0.50, eps);
}


////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_power_temp_container){
////////////////////////////////////////////////////////////////////////

    // Create the data container
    power_container::sptr container = power_container::make();

    // Create some data points to add
    std::vector<double> pt0(3, 1.0);
    std::vector<double> pt1(3, 2.0);
    std::vector<double> pt2(3, 3.0);

    container->add(1.0, pt0);
    container->add(2.0, pt1);
    container->add(5.0, pt2);

    // Add points to interpolate against
    std::vector<double> test0(3, 1.99);
    std::vector<double> test1(3, 1.29);
    std::vector<double> test2;
    test2.push_back(2.59);
    test2.push_back(1.29);
    test2.push_back(2.99);

    BOOST_CHECK_CLOSE(container->get(test0), 2.0, eps);
    BOOST_CHECK_CLOSE(container->get(test1), 1.0, eps);
    BOOST_CHECK_CLOSE(container->get(test2), 5.0, eps);
}

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_power_container_metadata){
////////////////////////////////////////////////////////////////////////

    // Create the data container
    power_container::sptr container = power_container::make();

    // Create some metadata to add
    base_container::metadata_t data;

    std::string fake_serial = "F2A432";
    data["x300"] = fake_serial;

    // Add some metadata
    container->add_metadata(data);

    // Check to see if the metadata matches
    power_container::metadata_t recovered_data = container->get_metadata();

    BOOST_CHECK_EQUAL(recovered_data["x300"], fake_serial);
}

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_power_serialization){
////////////////////////////////////////////////////////////////////////

    // Create the data container
    power_container::sptr container = power_container::make();

    // Create some metadata to add
    base_container::metadata_t data;

    std::string fake_serial = "F2A432";
    data["x300"] = fake_serial;

    // Add some metadata
    container->add_metadata(data);

    // Create some data points to add
    std::vector<double> pt0(3, 1.0);
    std::vector<double> pt1(3, 2.0);
    std::vector<double> pt2(3, 3.0);

    container->add(1.0, pt0);
    container->add(2.0, pt1);
    container->add(5.0, pt2);

    std::string filename("test_power_serialization");

    // Create/open a file to store the container
    {
        std::ofstream ofile(filename.c_str());

        boost::archive::text_oarchive oarchive(ofile);
        oarchive << *container;
    }

    // Restore to another data container
    power_container::sptr new_container = power_container::make();

    {
        std::ifstream ifile(filename.c_str());
        boost::archive::text_iarchive iarchive(ifile);

        iarchive >> *new_container;
    }

    // Add points to interpolate against
    std::vector<double> test0(3, 1.99);
    std::vector<double> test1(3, 1.29);
    std::vector<double> test2;
    test2.push_back(2.59);
    test2.push_back(1.29);
    test2.push_back(2.99);

    power_container::metadata_t recovered_data = new_container->get_metadata();

    BOOST_CHECK_CLOSE(new_container->get(test0), 2.0, eps);
    BOOST_CHECK_CLOSE(new_container->get(test1), 1.0, eps);
    BOOST_CHECK_CLOSE(new_container->get(test2), 5.0, eps);

    // Check to see if the metadata matches
    BOOST_CHECK_EQUAL(recovered_data["x300"], fake_serial);

    std::remove(filename.c_str());
}

////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_interp_singular){
////////////////////////////////////////////////////////////////////////

    // Create the data container
    power_container::sptr container = power_container::make();

    // Create some data points to add
    // that result in a singular matrix
    std::vector<double> pt0(2, 1.0);
    std::vector<double> pt1(2, 2.0);
    std::vector<double> pt2(2, 3.0);
    std::vector<double> pt3(2, 4.0);

    container->add(1.0, pt0);
    container->add(2.0, pt1);
    container->add(3.0, pt2);
    container->add(4.0, pt3);

    std::vector<double> test(2, 2.5);
    BOOST_CHECK_CLOSE(container->get(test), 2.5, eps);
}

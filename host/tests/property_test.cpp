//
// Copyright 2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/test/unit_test.hpp>
#include <uhd/property_tree.hpp>
#include <boost/bind.hpp>
#include <exception>
#include <iostream>

struct coercer_type{
    int doit(int x){
        return x & ~0x3;
    }
};

struct setter_type{
    setter_type() : _count(0), _x(0) {}

    void doit(int x){
        _count++;
        _x = x;
    }

    int _count;
    int _x;
};

struct getter_type{
    getter_type() : _count(0), _x(0) {}

    int doit(void){
        _count++;
        return _x;
    }

    int _count;
    int _x;
};

BOOST_AUTO_TEST_CASE(test_prop_simple){
    uhd::property_tree::sptr tree = uhd::property_tree::make();
    uhd::property<int> &prop = tree->create<int>("/");

    BOOST_CHECK(prop.empty());
    prop.set(0);
    BOOST_CHECK(not prop.empty());

    prop.set(42);
    BOOST_CHECK_EQUAL(prop.get(), 42);
    prop.set(34);
    BOOST_CHECK_EQUAL(prop.get(), 34);
}

BOOST_AUTO_TEST_CASE(test_prop_with_desired_subscriber){
    uhd::property_tree::sptr tree = uhd::property_tree::make();
    uhd::property<int> &prop = tree->create<int>("/");

    setter_type setter;
    prop.add_desired_subscriber(boost::bind(&setter_type::doit, &setter, _1));

    prop.set(42);
    BOOST_CHECK_EQUAL(prop.get_desired(), 42);
    BOOST_CHECK_EQUAL(prop.get(), 42);
    BOOST_CHECK_EQUAL(setter._x, 42);

    prop.set(34);
    BOOST_CHECK_EQUAL(prop.get_desired(), 34);
    BOOST_CHECK_EQUAL(prop.get(), 34);
    BOOST_CHECK_EQUAL(setter._x, 34);
}

BOOST_AUTO_TEST_CASE(test_prop_with_coerced_subscriber){
    uhd::property_tree::sptr tree = uhd::property_tree::make();
    uhd::property<int> &prop = tree->create<int>("/");

    setter_type setter;
    prop.add_coerced_subscriber(boost::bind(&setter_type::doit, &setter, _1));

    prop.set(42);
    BOOST_CHECK_EQUAL(prop.get_desired(), 42);
    BOOST_CHECK_EQUAL(prop.get(), 42);
    BOOST_CHECK_EQUAL(setter._x, 42);

    prop.set(34);
    BOOST_CHECK_EQUAL(prop.get_desired(), 34);
    BOOST_CHECK_EQUAL(prop.get(), 34);
    BOOST_CHECK_EQUAL(setter._x, 34);
}

BOOST_AUTO_TEST_CASE(test_prop_manual_coercion){
    uhd::property_tree::sptr tree = uhd::property_tree::make();
    uhd::property<int> &prop = tree->create<int>("/", uhd::property_tree::MANUAL_COERCE);

    setter_type dsetter, csetter;
    prop.add_desired_subscriber(boost::bind(&setter_type::doit, &dsetter, _1));
    prop.add_coerced_subscriber(boost::bind(&setter_type::doit, &csetter, _1));

    BOOST_CHECK_EQUAL(dsetter._x, 0);
    BOOST_CHECK_EQUAL(csetter._x, 0);

    prop.set(42);
    BOOST_CHECK_EQUAL(prop.get_desired(), 42);
    BOOST_CHECK_EQUAL(dsetter._x, 42);
    BOOST_CHECK_EQUAL(csetter._x, 0);

    prop.set_coerced(34);
    BOOST_CHECK_EQUAL(prop.get_desired(), 42);
    BOOST_CHECK_EQUAL(prop.get(), 34);
    BOOST_CHECK_EQUAL(dsetter._x, 42);
    BOOST_CHECK_EQUAL(csetter._x, 34);
}

BOOST_AUTO_TEST_CASE(test_prop_with_publisher){
    uhd::property_tree::sptr tree = uhd::property_tree::make();
    uhd::property<int> &prop = tree->create<int>("/");

    BOOST_CHECK(prop.empty());
    getter_type getter;
    prop.set_publisher(boost::bind(&getter_type::doit, &getter));
    BOOST_CHECK(not prop.empty());

    getter._x = 42;
    prop.set(0); //should not change
    BOOST_CHECK_EQUAL(prop.get(), 42);

    getter._x = 34;
    prop.set(0); //should not change
    BOOST_CHECK_EQUAL(prop.get(), 34);
}

BOOST_AUTO_TEST_CASE(test_prop_with_publisher_and_subscriber){
    uhd::property_tree::sptr tree = uhd::property_tree::make();
    uhd::property<int> &prop = tree->create<int>("/");

    getter_type getter;
    prop.set_publisher(boost::bind(&getter_type::doit, &getter));

    setter_type setter;
    prop.add_coerced_subscriber(boost::bind(&setter_type::doit, &setter, _1));

    getter._x = 42;
    prop.set(0);
    BOOST_CHECK_EQUAL(prop.get(), 42);
    BOOST_CHECK_EQUAL(setter._x, 0);

    getter._x = 34;
    prop.set(1);
    BOOST_CHECK_EQUAL(prop.get(), 34);
    BOOST_CHECK_EQUAL(setter._x, 1);
}

BOOST_AUTO_TEST_CASE(test_prop_with_coercion){
    uhd::property_tree::sptr tree = uhd::property_tree::make();
    uhd::property<int> &prop = tree->create<int>("/");

    setter_type setter;
    prop.add_coerced_subscriber(boost::bind(&setter_type::doit, &setter, _1));

    coercer_type coercer;
    prop.set_coercer(boost::bind(&coercer_type::doit, &coercer, _1));

    prop.set(42);
    BOOST_CHECK_EQUAL(prop.get(), 40);
    BOOST_CHECK_EQUAL(setter._x, 40);

    prop.set(34);
    BOOST_CHECK_EQUAL(prop.get(), 32);
    BOOST_CHECK_EQUAL(setter._x, 32);
}

BOOST_AUTO_TEST_CASE(test_prop_tree){
    uhd::property_tree::sptr tree = uhd::property_tree::make();

    tree->create<int>("/test/prop0");
    tree->create<int>("/test/prop1");

    BOOST_CHECK(tree->exists("/test"));
    BOOST_CHECK_THROW(tree->access<int>("/test"), std::exception);
    BOOST_CHECK(tree->exists("/test/prop0"));
    BOOST_CHECK(tree->exists("/test/prop1"));

    tree->access<int>("/test/prop0").set(42);
    tree->access<int>("/test/prop1").set(34);

    BOOST_CHECK_EQUAL(tree->access<int>("/test/prop0").get(), 42);
    BOOST_CHECK_EQUAL(tree->access<int>("/test/prop1").get(), 34);

    tree->remove("/test/prop0");
    BOOST_CHECK(not tree->exists("/test/prop0"));
    BOOST_CHECK(tree->exists("/test/prop1"));

    tree->remove("/test");
    BOOST_CHECK(not tree->exists("/test/prop0"));
    BOOST_CHECK(not tree->exists("/test/prop1"));

}

BOOST_AUTO_TEST_CASE(test_prop_subtree){
    uhd::property_tree::sptr tree = uhd::property_tree::make();
    tree->create<int>("/subdir1/subdir2");

    uhd::property_tree::sptr subtree1 = tree->subtree("/");
    const std::vector<std::string> tree_dirs1 = tree->list("/");
    const std::vector<std::string> subtree1_dirs = subtree1->list("");
    BOOST_CHECK_EQUAL_COLLECTIONS(tree_dirs1.begin(), tree_dirs1.end(), subtree1_dirs.begin(), subtree1_dirs.end());

    uhd::property_tree::sptr subtree2 = subtree1->subtree("subdir1");
    const std::vector<std::string> tree_dirs2 = tree->list("/subdir1");
    const std::vector<std::string> subtree2_dirs = subtree2->list("");
    BOOST_CHECK_EQUAL_COLLECTIONS(tree_dirs2.begin(), tree_dirs2.end(), subtree2_dirs.begin(), subtree2_dirs.end());

}


BOOST_AUTO_TEST_CASE(test_prop_operators)
{
    uhd::fs_path path1 = "/root/";
    path1 = path1 / "leaf";
    BOOST_CHECK_EQUAL(path1, "/root/leaf");

    uhd::fs_path path2 = "/root";
    path2 = path2 / "leaf";
    BOOST_CHECK_EQUAL(path2, "/root/leaf");

    uhd::fs_path path3 = "/root/";
    path3 = path3 / "/leaf/";
    BOOST_CHECK_EQUAL(path3, "/root/leaf/");

    uhd::fs_path path4 = "/root/";
    size_t x = 2;
    path4 = path4 / x;
    BOOST_CHECK_EQUAL(path4, "/root/2");
}

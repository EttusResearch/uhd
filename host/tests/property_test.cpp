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

#include <boost/test/unit_test.hpp>
#include <uhd/property.hpp>
#include <uhd/property_tree.hpp>
#include <boost/bind.hpp>
#include <iostream>

struct coercer_type{
    int doit(int x){
        return x & ~0x3;
    }
};

struct setter_type{
    void doit(int x){
        _x = x;
    }

    int _x;
};

BOOST_AUTO_TEST_CASE(test_prop_simple){
    uhd::property<int> prop;
    prop.set(42);
    BOOST_CHECK_EQUAL(prop.get(), 42);
    prop.set(34);
    BOOST_CHECK_EQUAL(prop.get(), 34);
}

BOOST_AUTO_TEST_CASE(test_prop_with_subscriber){
    uhd::property<int> prop;

    setter_type setter;
    prop.subscribe(boost::bind(&setter_type::doit, &setter, _1));

    prop.set(42);
    BOOST_CHECK_EQUAL(prop.get(), 42);
    BOOST_CHECK_EQUAL(setter._x, 42);

    prop.set(34);
    BOOST_CHECK_EQUAL(prop.get(), 34);
    BOOST_CHECK_EQUAL(setter._x, 34);
}

BOOST_AUTO_TEST_CASE(test_prop_with_coercion){
    uhd::property<int> prop;

    setter_type setter;
    prop.subscribe(boost::bind(&setter_type::doit, &setter, _1));

    coercer_type coercer;
    prop.subscribe_master(boost::bind(&coercer_type::doit, &coercer, _1));

    prop.set(42);
    BOOST_CHECK_EQUAL(prop.get(), 40);
    BOOST_CHECK_EQUAL(setter._x, 40);

    prop.set(34);
    BOOST_CHECK_EQUAL(prop.get(), 32);
    BOOST_CHECK_EQUAL(setter._x, 32);
}

BOOST_AUTO_TEST_CASE(test_prop_tree){
    uhd::property_tree::sptr tree = uhd::property_tree::make();

    tree->create("/test/prop0", uhd::property<int>());
    tree->create<int>("/test/prop1");

    BOOST_CHECK(tree->exists("/test"));
    BOOST_CHECK(tree->exists("/test/prop0"));
    BOOST_CHECK(tree->exists("/test/prop1"));

    tree->access<int>("/test/prop0").set(42);
    tree->access<int>("/test/prop1").set(34);

    tree->remove("/test/prop0");
    BOOST_CHECK(not tree->exists("/test/prop0"));
    BOOST_CHECK(tree->exists("/test/prop1"));

    tree->remove("/test");
    BOOST_CHECK(not tree->exists("/test/prop0"));
    BOOST_CHECK(not tree->exists("/test/prop1"));

}

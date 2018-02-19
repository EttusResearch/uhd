//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/test/unit_test.hpp>
#include <uhdlib/usrp/constrained_device_args.hpp>
#include <iostream>

using uhd::usrp::constrained_device_args_t;

namespace {
    enum test_enum_t { VALUE1, VALUE2, VALUE3=4 };

    static constexpr double      MAX_DOUBLE_ARG = 1e6;
    static constexpr double      MIN_DOUBLE_ARG = 0.125;

    static constexpr double      DEFAULT_DOUBLE_ARG = 2.25;
    static constexpr size_t      DEFAULT_SIZE_T_ARG = 42;
    static constexpr bool        DEFAULT_BOOL_ARG   = true;
    static constexpr test_enum_t DEFAULT_ENUM_ARG   = VALUE1;


    class test_device_args_t : public constrained_device_args_t
    {
    public:
        test_device_args_t() {}
        test_device_args_t(const std::string& dev_args) { parse(dev_args); }

        double get_double_arg() const {
            return _double_arg.get();
        }
        size_t get_size_t_arg() const {
            return _size_t_arg.get();
        }
        bool get_bool_arg() const {
            return _bool_arg.get();
        }
        test_enum_t get_enum_arg() const {
            return _enum_arg.get();
        }

        inline virtual std::string to_string() const {
            return  _double_arg.to_string() + ", " +
                    _size_t_arg.to_string() + ", " +
                    _bool_arg.to_string() + ", " +
                    _enum_arg.to_string();
        }
    private:
        virtual void _parse(const uhd::device_addr_t& dev_args) {
            if (dev_args.has_key(_double_arg.key()))
                _double_arg.parse(dev_args[_double_arg.key()]);
            if (dev_args.has_key(_size_t_arg.key()))
                _size_t_arg.parse(dev_args[_size_t_arg.key()]);
            if (dev_args.has_key(_bool_arg.key()))
                _bool_arg.parse(dev_args[_bool_arg.key()]);
            if (dev_args.has_key(_enum_arg.key()))
                _enum_arg.parse(dev_args[_enum_arg.key()]);
            _enforce_range(_double_arg, MIN_DOUBLE_ARG, MAX_DOUBLE_ARG);
        }

        constrained_device_args_t::num_arg<double>       _double_arg
            {"double_arg", DEFAULT_DOUBLE_ARG};
        constrained_device_args_t::num_arg<size_t>       _size_t_arg
            {"size_t_arg", DEFAULT_SIZE_T_ARG};
        constrained_device_args_t::bool_arg              _bool_arg
            {"bool_arg", DEFAULT_BOOL_ARG};
        constrained_device_args_t::enum_arg<test_enum_t> _enum_arg
            {"enum_arg", DEFAULT_ENUM_ARG,
                  {{"value1", VALUE1}, {"value2", VALUE2}, {"VALUE3", VALUE3}}};
    };

}

BOOST_AUTO_TEST_CASE(test_constrained_device_args) {
    test_device_args_t test_dev_args("double_arg=3.5,bool_arg=0,foo=bar");
    BOOST_CHECK_EQUAL(test_dev_args.get_double_arg(), 3.5);
    BOOST_CHECK_EQUAL(
        test_dev_args.get_size_t_arg(),
        DEFAULT_SIZE_T_ARG
    );
    BOOST_CHECK_EQUAL(test_dev_args.get_bool_arg(), false);
    BOOST_CHECK_EQUAL(
        test_dev_args.get_enum_arg(),
        DEFAULT_ENUM_ARG
    );
    BOOST_REQUIRE_THROW(
        test_dev_args.parse("double_arg=2e6"),
        uhd::value_error
    ); // Note: test_dev_args is now in a bad state until we fix it!
    test_dev_args.parse("double_arg=2.6"),
    test_dev_args.parse("enum_arg=vaLue2");
    BOOST_CHECK_EQUAL(
        test_dev_args.get_enum_arg(),
        VALUE2
    );
    test_dev_args.parse("enum_arg=VALUE3");
    BOOST_CHECK_EQUAL(
        test_dev_args.get_enum_arg(),
        VALUE3
    );
}


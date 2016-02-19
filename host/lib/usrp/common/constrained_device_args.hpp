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

#ifndef INCLUDED_LIBUHD_USRP_COMMON_CONSTRAINED_DEV_ARGS_HPP
#define INCLUDED_LIBUHD_USRP_COMMON_CONSTRAINED_DEV_ARGS_HPP

#include <uhd/types/device_addr.hpp>
#include <uhd/exception.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assign/list_of.hpp>
#include <vector>
#include <string>

namespace uhd {
namespace usrp {

    /*!
     * constrained_device_args_t provides a base and utilities to
     * map key=value pairs passed in through the device creation
     * args interface (device_addr_t).
     *
     * Inherit from this class to create typed device specific
     * arguments and use the base class methods to handle parsing
     * the device_addr or any key=value string to populate the args
     *
     * This file contains a library of different types of args the
     * the user can pass in. The library can be extended to support
     * non-intrinsic types by the client.
     *
     */
    class constrained_device_args_t {
    public: //Types

        /*!
         * Base argument type. All other arguments inherit from this.
         */
        class generic_arg {
        public:
            generic_arg(const std::string& key): _key(key) {}
            inline const std::string& key() const { return _key; }
            inline virtual std::string to_string() const = 0;
        private:
            std::string _key;
        };

        /*!
         * String argument type. Can be case sensitive or insensitive
         */
        template<bool case_sensitive>
        class str_arg : public generic_arg {
        public:
            str_arg(const std::string& name, const std::string& default_value) :
                generic_arg(name) { set(default_value); }

            inline void set(const std::string& value) {
                _value = case_sensitive ? value : boost::algorithm::to_lower_copy(value);
            }
            inline const std::string& get() const {
                return _value;
            }
            inline void parse(const std::string& str_rep) {
                set(str_rep);
            }
            inline virtual std::string to_string() const {
                return key() + "=" + get();
            }
            inline bool operator==(const std::string& rhs) const {
                return get() == boost::algorithm::to_lower_copy(rhs);
            }
        private:
            std::string _value;
        };
        typedef str_arg<false>  str_ci_arg;
        typedef str_arg<true>   str_cs_arg;

        /*!
         * Numeric argument type. The template type data_t allows the
         * client to constrain the type of the number.
         */
        template<typename data_t>
        class num_arg : public generic_arg {
        public:
            num_arg(const std::string& name, const data_t default_value) :
                generic_arg(name) { set(default_value); }

            inline void set(const data_t value) {
                _value = value;
            }
            inline const data_t get() const {
                return _value;
            }
            inline void parse(const std::string& str_rep) {
                try {
                    _value = boost::lexical_cast<data_t>(str_rep);
                } catch (std::exception& ex) {
                    throw uhd::value_error(str(boost::format(
                        "Error parsing numeric parameter %s: %s.") %
                        key() % ex.what()
                    ));
                }
            }
            inline virtual std::string to_string() const {
                return key() + "=" + boost::lexical_cast<std::string>(get());
            }
        private:
            data_t _value;
        };

        /*!
         * Enumeration argument type. The template type enum_t allows the
         * client to use their own enum and specify a string mapping for
         * the values of the enum
         *
         * NOTE: The constraint on enum_t is that the values must start with
         * 0 and be sequential
         */
        template<typename enum_t>
        class enum_arg : public generic_arg {
        public:
            enum_arg(
                const std::string& name,
                const enum_t default_value,
                const std::vector<std::string>& values) :
                    generic_arg(name), _str_values(values)
            { set(default_value); }

            inline void set(const enum_t value) {
                _value = value;
            }
            inline const enum_t get() const {
                return _value;
            }
            inline void parse(const std::string& str_rep, bool assert_invalid = true) {
                std::string valid_values_str;
                for (size_t i = 0; i < _str_values.size(); i++) {
                    if (boost::algorithm::to_lower_copy(str_rep) ==
                        boost::algorithm::to_lower_copy(_str_values[i]))
                    {
                        valid_values_str += ((i==0)?"":", ") + _str_values[i];
                        set(static_cast<enum_t>(static_cast<int>(i)));
                        return;
                    }
                }
                //If we reach here then, the string enum value was invalid
                if (assert_invalid) {
                    throw uhd::value_error(str(boost::format(
                        "Invalid device arg value: %s=%s (Valid: {%s})") %
                        key() % str_rep % valid_values_str
                    ));
                }
            }
            inline virtual std::string to_string() const {
                size_t index = static_cast<size_t>(static_cast<int>(_value));
                UHD_ASSERT_THROW(index < _str_values.size());
                return key() + "=" + _str_values[index];
            }

        private:
            enum_t                      _value;
            std::vector<std::string>    _str_values;
        };

        /*!
         * Boolean argument type.
         */
        class bool_arg : public generic_arg {
        public:
            bool_arg(const std::string& name, const bool default_value) :
                generic_arg(name) { set(default_value); }

            inline void set(const bool value) {
                _value = value;
            }
            inline bool get() const {
                return _value;
            }
            inline void parse(const std::string& str_rep) {
                try {
                    _value = (boost::lexical_cast<int>(str_rep) != 0);
                } catch (std::exception& ex) {
                    if (str_rep.empty()) {
                        //If str_rep is empty then the device_addr was set
                        //without a value which means that the user "set" the flag
                        _value = true;
                    } else if (boost::algorithm::to_lower_copy(str_rep) == "true" ||
                        boost::algorithm::to_lower_copy(str_rep) == "yes" ||
                        boost::algorithm::to_lower_copy(str_rep) == "y") {
                        _value = true;
                    } else if (boost::algorithm::to_lower_copy(str_rep) == "false" ||
                            boost::algorithm::to_lower_copy(str_rep) == "no" ||
                            boost::algorithm::to_lower_copy(str_rep) == "n") {
                        _value = false;
                    } else {
                        throw uhd::value_error(str(boost::format(
                            "Error parsing boolean parameter %s: %s.") %
                            key() % ex.what()
                        ));
                    }
                }
            }
            inline virtual std::string to_string() const {
                return key() + "=" + (get() ? "true" : "false");
            }
        private:
            bool _value;
        };

    public: //Methods
        constrained_device_args_t() {}
        virtual ~constrained_device_args_t() {}

        void parse(const std::string& str_args) {
            device_addr_t dev_args(str_args);
            _parse(dev_args);
        }

        void parse(const device_addr_t& dev_args) {
            _parse(dev_args);
        }

        inline virtual std::string to_string() const = 0;

    protected:  //Methods
        //Override _parse to provide an implementation to parse all
        //client specific device args
        virtual void _parse(const device_addr_t& dev_args) = 0;

        /*!
         * Utility: Ensure that the value of the device arg is between min and max
         */
        template<typename num_data_t>
        static inline void _enforce_range(const num_arg<num_data_t>& arg, const num_data_t& min, const num_data_t& max) {
            if (arg.get() > max || arg.get() < min) {
                throw uhd::value_error(str(boost::format(
                    "Invalid device arg value: %s (Minimum: %s, Maximum: %s)") %
                    arg.to_string() %
                    boost::lexical_cast<std::string>(min) % boost::lexical_cast<std::string>(max)));
            }
        }

        /*!
         * Utility: Ensure that the value of the device arg is is contained in valid_values
         */
        template<typename arg_t, typename data_t>
        static inline void _enforce_discrete(const arg_t& arg, const std::vector<data_t>& valid_values) {
            bool match = false;
            BOOST_FOREACH(const data_t& val, valid_values) {
                if (val == arg.get()) {
                    match = true;
                    break;
                }
            }
            if (!match) {
                std::string valid_values_str;
                for (size_t i = 0; i < valid_values.size(); i++) {
                    valid_values_str += ((i==0)?"":", ") + boost::lexical_cast<std::string>(valid_values[i]);
                    throw uhd::value_error(str(boost::format(
                        "Invalid device arg value: %s (Valid: {%s})") %
                        arg.to_string() % valid_values_str
                    ));
                }
            }
        }
    };
}} //namespaces

#endif /* INCLUDED_LIBUHD_USRP_COMMON_CONSTRAINED_DEV_ARGS_HPP */

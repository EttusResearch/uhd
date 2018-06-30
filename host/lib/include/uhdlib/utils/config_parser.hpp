//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_CONFIG_PARSER_HPP
#define INCLUDED_LIBUHD_CONFIG_PARSER_HPP

#include <uhd/exception.hpp>
#include <boost/property_tree/ptree.hpp>
#include <vector>

namespace uhd {

/*! Represent a config file (INI format)
 *
 * These files have the following format:
 * ```
 * ; Comment
 * [section]
 * key=value
 * ```
 *
 * The API of this class was designed to match the ConfigParser class from
 * Python https://docs.python.org/3/library/configparser.html, which we use in
 * MPM. This does mean the API is somewhat different from typical C++ APIs,
 * and does not name getters get_*.
 */
class config_parser
{
public:
    /*!
     * \param path Path to the INI file
     *
     * \throws uhd::runtime_error if the parsing failed.
     */
    config_parser(const std::string &path="");

    /*! Load another config file and update the current values.
     *
     * If a value exists in both the new and current file, the new value wins.
     */
    void read_file(const std::string &path);

    //! Return a list of sections
    std::vector<std::string> sections();

    //! Return a list of options (keys) in a section, or an empty list if
    //  section does not exist
    std::vector<std::string> options(const std::string &section);

    /*! Return the value of a key
     *
     * \param section The name of the section in which this key is
     * \param key Name of the key
     * \param def Default value, in case the key does not exist
     */
    template <typename T>
    T get(
            const std::string &section,
            const std::string &key,
            const T &def
    ) {
        try {
            const auto child = _pt.get_child(section);
            return child.get<T>(key, def);
        } catch (const boost::property_tree::ptree_bad_path &) {
            return def;
        }
    }

    /*! Return the value of a key
     *
     * \param section The name of the section in which this key is
     * \param key Name of the key
     *
     * \throws uhd::key_error if the key or the section don't exist
     */
    template <typename T>
    T get(
            const std::string &section,
            const std::string &key
    ) {
        try {
            const auto child = _pt.get_child(section);
            return child.get<T>(key);
        } catch (const boost::property_tree::ptree_bad_path &) {
            throw uhd::key_error(
                std::string("[config_parser] Key ") + key +
                " not found in section " + section
            );
        }
    }

    template <typename T>
    void set(
            const std::string &section,
            const std::string &key,
            const T &value
    ) {
        _pt.put<T>(section + "." + key, value);
    }

private:
    template <typename T>
    static std::vector<std::string> _options(T key_bearing_iterable)
    {
        std::vector<std::string> sections;
        for (const auto& node : key_bearing_iterable) {
            sections.push_back(node.first);
        }
        return sections;
    }

    boost::property_tree::ptree _pt;
};

} /* namespace uhd */

#endif /* INCLUDED_LIBUHD_CONFIG_PARSER_HPP */

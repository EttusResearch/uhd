//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/property_tree.hpp>
#include <uhd/rfnoc/rf_control/core_iface.hpp>
#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace uhd { namespace rfnoc { namespace rf_control {

/*! Interface for setting and getting the current antenna.
 */
class UHD_API antenna_iface
{
public:
    using sptr = std::shared_ptr<antenna_iface>;

    virtual ~antenna_iface() = default;

    /*! Return a list of valid antenna for channel \p chan.
     *
     * \return The selected antenna.
     */
    virtual std::vector<std::string> get_antennas(const size_t chan) const = 0;

    /*! Select antenna \p for channel \p chan.
     *
     * \throws uhd::value_error if \p ant is not a valid value.
     */
    virtual void set_antenna(const std::string& ant, const size_t chan) = 0;

    /*! Return the selected antenna for channel \p chan.
     *
     * \return The selected antenna.
     */
    virtual std::string get_antenna(const size_t chan) const = 0;
};

/*! Class for getting and setting antennas out of an enumerated set, where
 * the API calls for the antenna actually map to property nodes.
 */
class UHD_API enumerated_antenna : public antenna_iface
{
public:
    using prop_path = std::function<fs_path(const size_t chan)>;

    /*! Constructs an enumerated_antenna class.
     *
     * \param tree The property tree the nodes are on
     * \param prop_path_generator Closure to generate the property path given the channel.
     * \param possible_antennas A vector of legal antennas.
     * \param compat_map A map of alternative names for antennas.
     */
    enumerated_antenna(uhd::property_tree::sptr tree,
        prop_path prop_path_generator,
        const std::vector<std::string>& possible_antennas,
        const std::unordered_map<std::string, std::string>& compat_map);

    virtual ~enumerated_antenna() = default;

    std::vector<std::string> get_antennas(const size_t chan) const override;
    void set_antenna(const std::string& ant, const size_t chan) override;
    std::string get_antenna(const size_t chan) const override;

private:
    // The property tree & node used to implement the API
    uhd::property_tree::sptr _tree;
    prop_path _prop_path_generator;

    std::vector<std::string> _possible_antennas;
    const std::unordered_map<std::string, std::string>& _compat_map;
};

/*! Partially implements core_iface for antenna, redirecting to one of two
 * subobjects for RX or TX.
 */
class UHD_API antenna_radio_control_mixin : virtual public core_iface
{
public:
    virtual ~antenna_radio_control_mixin() = default;

    std::string get_tx_antenna(const size_t chan) const override;
    std::vector<std::string> get_tx_antennas(const size_t chan) const override;
    void set_tx_antenna(const std::string& ant, const size_t chan) override;
    std::string get_rx_antenna(const size_t chan) const override;
    std::vector<std::string> get_rx_antennas(const size_t chan) const override;
    void set_rx_antenna(const std::string& ant, const size_t chan) override;

protected:
    antenna_iface::sptr _rx_antenna;
    antenna_iface::sptr _tx_antenna;
};

}}} // namespace uhd::rfnoc::rf_control

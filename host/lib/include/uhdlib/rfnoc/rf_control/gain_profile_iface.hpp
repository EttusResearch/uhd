//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace uhd { namespace rfnoc { namespace rf_control {

/*! Interface for gain profile API commands
 *
 * This interface contains methods to configure the current gain profile of the
 * device. Note that this interface is RX/TX agnostic, it does not provide
 * specialized methods for RX and TX.
 */
class gain_profile_iface
{
public:
    using sptr = std::shared_ptr<gain_profile_iface>;
    using subscriber_type =
        std::function<void(const std::string& profile, const size_t chan)>;

    virtual ~gain_profile_iface() = default;

    /*! Return a list of TX gain profiles for this radio
     */
    virtual std::vector<std::string> get_gain_profile_names(const size_t chan) const = 0;

    /*! Set the gain profile
     */
    virtual void set_gain_profile(const std::string& profile, const size_t chan) = 0;

    /*! Return the gain profile
     */
    virtual std::string get_gain_profile(const size_t chan) const = 0;

    /*! Register a subscriber to a property tree node
     *
     * This is useful for all those cases where the gain profile is also a
     * property in the tree, and setting it here requires also updating the tree.
     */
    virtual void add_subscriber(subscriber_type&& sub) = 0;
};

/*! "Default" implementation for gain_profile_iface
 *
 * This class implements gain_profile_iface such that the device is always
 * and can only be configured using a single "default" gain profile. Setting
 * a gain profile which is not the default profile is an error, and getting the
 * gain profile will always return the default profile.
 */
class default_gain_profile : public gain_profile_iface
{
public:
    std::vector<std::string> get_gain_profile_names(const size_t chan) const override;

    void set_gain_profile(const std::string& profile, const size_t chan) override;
    std::string get_gain_profile(const size_t chan) const override;
    void add_subscriber(subscriber_type&& sub) override;

private:
    static const std::string DEFAULT_GAIN_PROFILE;
    subscriber_type _sub = nullptr;
};

/*! "Enumerated" implementation for gain_profile_iface
 *
 * This class implements gain_profile_iface so that the gain profile can only be
 * one of several different enumerated values. Setting an invalid gain profile
 * is an error. Retrieving a gain profile will always return one of the
 * enumerated gain profiles.
 */
class enumerated_gain_profile : public gain_profile_iface
{
public:
    enumerated_gain_profile(const std::vector<std::string>& possible_profiles,
        const std::string& default_profile,
        size_t num_channels);

    void set_gain_profile(const std::string& profile, const size_t chan) override;

    std::string get_gain_profile(const size_t chan) const override;

    std::vector<std::string> get_gain_profile_names(const size_t) const override;

    /*! Register a subscriber to a property tree node
     *
     * This is useful for all those cases where the gain profile is also a
     * property in the tree, and setting it here requires also updating the tree.
     */
    void add_subscriber(subscriber_type&& sub) override;

private:
    std::vector<std::string> _possible_profiles;

    std::vector<std::string> _gain_profile;

    subscriber_type _sub = nullptr;
};

}}} // namespace uhd::rfnoc::rf_control

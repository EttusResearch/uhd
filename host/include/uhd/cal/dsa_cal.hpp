//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_CAL_GAIN_HPP
#define INCLUDED_LIBUHD_CAL_GAIN_HPP

#include <uhd/cal/container.hpp>
#include <uhd/config.hpp>
#include <boost/optional.hpp>
#include <array>
#include <map>

namespace uhd { namespace usrp { namespace cal {

/*!  Class that stores DSA indices for all ZBX TX bands.
 */
class UHD_API zbx_tx_dsa_cal : public container
{
public:
    static constexpr uint32_t NUM_AMP         = 1;
    static constexpr uint32_t NUM_DSA         = 2 + NUM_AMP;
    static constexpr uint32_t NUM_GAIN_STAGES = 61;

    using sptr          = std::shared_ptr<zbx_tx_dsa_cal>;
    using step_settings = std::array<uint32_t, NUM_DSA>;

    /*! Add a new band description
     *
     * max_freq is the (inclusive) upper limit
     * for the band (lower limit derives from the other bands). Name is an
     * text representation (human readable) for the band. dsa_steps is an
     * array of DSA settings for all gains in the band.
     */
    virtual void add_frequency_band(const double max_freq,
        const std::string& name,
        std::array<step_settings, NUM_GAIN_STAGES> dsa_steps) = 0;

    /*! Retrieves DSA settings for frequency and gain_index.
     *
     * The settings are
     * retrieved from the band with the biggest max_freq that is smaller or
     * equal to freq. DSA settings are the settings at gain_index in that band.
     * Value errors are thrown if freq is larger that the largest freq_max of
     * all bands or gain_index is not within range.
     */
    virtual const step_settings get_dsa_setting(
        const double freq, const size_t gain_index) const = 0;

    /* Check whether two frequencies map to the same band.
     */
    virtual bool is_same_band(double freq1, double freq2) const = 0;

    /*! Retrieves DSA settings as flat list.
     * The values are flattened by frequency band, gain and values in that order.
     * Use NUM_DSA and NUM_GAIN_STAGES to find values in the list.
     */
    virtual std::vector<uint32_t> get_band_settings(double freq, uint8_t dsa) const = 0;

    /*!
     * Clear all stored values
     */
    virtual void clear() = 0;

    //! Factory for new cal data sets
    static sptr make(
        const std::string& name, const std::string& serial, const uint64_t timestamp);

    //! Default factory
    static sptr make();
};

/*!  Class that stores DSA indices for all ZBX TX bands.
 */
class UHD_API zbx_rx_dsa_cal : public container
{
public:
    static constexpr uint32_t NUM_DSA         = 4;
    static constexpr uint32_t NUM_GAIN_STAGES = 61;

    using sptr          = std::shared_ptr<zbx_rx_dsa_cal>;
    using step_settings = std::array<uint32_t, NUM_DSA>;

    /*! Add a new band description.
     *
     * max_freq is the (inclusive) upper limit
     * for the band (lower limit derives from the other bands). Name is an
     * text representation (human readable) for the band. dsa_steps is an
     * array of DSA settings for all gains in the band.
     */
    virtual void add_frequency_band(const double max_freq,
        const std::string& name,
        std::array<step_settings, NUM_GAIN_STAGES> dsa_steps) = 0;

    virtual bool is_same_band(double freq1, double freq2) const = 0;
    /*! Retrieves DSA settings for frequency and gain_index.
     *
     * The settings are
     * retrieved from the band with the biggest max_freq that is smaller or
     * equal to freq. DSA settings are the settings at gain_index in that band.
     * Value errors are thrown if freq is larger that the largest freq_max of
     * all bands or gain_index is not within range.
     */
    virtual const step_settings get_dsa_setting(
        const double freq, const size_t gain_index) const = 0;

    /*! Retrieves DSA settings as flat list.
     * The values are flattend by frequency band, gain and values in that order.
     * Use NUM_DSA and NUM_GAIN_STAGES to find values in the list.
     */
    virtual std::vector<uint32_t> get_band_settings(double freq, uint8_t dsa) const = 0;

    /*!
     * Clear all stored values
     */
    virtual void clear() = 0;

    //! Factory for new cal data sets
    static sptr make(
        const std::string& name, const std::string& serial, const uint64_t timestamp);

    //! Default factory
    static sptr make();
};

}}} // namespace uhd::usrp::cal

#endif /* INCLUDED_LIBUHD_CAL_GAIN_HPP */

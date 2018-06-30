//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_RFNOC_LEGACY_COMPAT_HPP
#define INCLUDED_RFNOC_LEGACY_COMPAT_HPP

#include <uhd/device3.hpp>
#include <uhd/stream.hpp>

namespace uhd { namespace rfnoc {

    /*! Legacy compatibility layer class.
     */
    class legacy_compat
    {
    public:
        typedef boost::shared_ptr<legacy_compat> sptr;

        virtual uhd::fs_path rx_dsp_root(const size_t mboard_idx, const size_t chan) = 0;

        virtual uhd::fs_path tx_dsp_root(const size_t mboard_idx, const size_t chan) = 0;

        virtual uhd::fs_path rx_fe_root(const size_t mboard_idx, const size_t chan) = 0;

        virtual uhd::fs_path tx_fe_root(const size_t mboard_idx, const size_t chan) = 0;

        virtual void issue_stream_cmd(const uhd::stream_cmd_t &stream_cmd, size_t mboard, size_t chan) = 0;

        virtual uhd::rx_streamer::sptr get_rx_stream(const uhd::stream_args_t &args) = 0;

        virtual uhd::tx_streamer::sptr get_tx_stream(const uhd::stream_args_t &args) = 0;

        virtual void set_rx_rate(const double rate, const size_t chan) = 0;

        virtual void set_tx_rate(const double rate, const size_t chan) = 0;

        static sptr make(
                uhd::device3::sptr device,
                const uhd::device_addr_t &args
        );
    };

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_RFNOC_LEGACY_COMPAT_HPP */

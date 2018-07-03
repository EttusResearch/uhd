//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "e300_remote_codec_ctrl.hpp"

#include <stdint.h>
#include <uhd/exception.hpp>
#include <uhd/utils/byteswap.hpp>
#include <cstring>
#include <iostream>

namespace uhd { namespace usrp { namespace e300 {

class e300_remote_codec_ctrl_impl : public e300_remote_codec_ctrl
{
public:
    e300_remote_codec_ctrl_impl(uhd::transport::zero_copy_if::sptr xport) : _xport(xport)
    {
    }

    virtual ~e300_remote_codec_ctrl_impl(void)
    {
    }


    double set_gain(const std::string &which, const double value)
    {
        _clear();
        _args.action = uhd::htonx<uint32_t>(transaction_t::ACTION_SET_GAIN);
        if (which == "TX1")      _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_TX1);
        else if (which == "TX2") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_TX2);
        else if (which == "RX1") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_RX1);
        else if (which == "RX2") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_RX2);
        else throw std::runtime_error("e300_remote_codec_ctrl_impl incorrect chain string.");
        _args.gain = value;

        _transact();
        return _retval.gain;
    }

    double set_clock_rate(const double rate)
    {
        _clear();
        _args.action = uhd::htonx<uint32_t>(
            transaction_t::ACTION_SET_CLOCK_RATE);
        _args.which = uhd::htonx<uint32_t>(
            transaction_t::CHAIN_NONE);  /*Unused*/
        _args.rate = rate;

        _transact();
        return _retval.gain;
    }

    void set_active_chains(bool tx1, bool tx2, bool rx1, bool rx2)
    {
        _clear();
        _args.action = uhd::htonx<uint32_t>(
            transaction_t::ACTION_SET_ACTIVE_CHANS);
        /*Unused*/
        _args.which = uhd::htonx<uint32_t>(
            transaction_t::CHAIN_NONE);
        _args.bits = uhd::htonx<uint32_t>(
                     (tx1 ? (1<<0) : 0) |
                     (tx2 ? (1<<1) : 0) |
                     (rx1 ? (1<<2) : 0) |
                     (rx2 ? (1<<3) : 0));

        _transact();
    }

    double tune(const std::string &which, const double value)
    {
        _clear();
        _args.action = uhd::htonx<uint32_t>(transaction_t::ACTION_TUNE);
        if (which == "TX1")      _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_TX1);
        else if (which == "TX2") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_TX2);
        else if (which == "RX1") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_RX1);
        else if (which == "RX2") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_RX2);
        else throw std::runtime_error("e300_remote_codec_ctrl_impl incorrect chain string.");
        _args.freq = value;

        _transact();
        return _retval.freq;
    }

    double get_freq(const std::string &which)
    {
        _clear();
        _args.action = uhd::htonx<uint32_t>(transaction_t::ACTION_GET_FREQ);
        if (which == "TX1")      _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_TX1);
        else if (which == "TX2") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_TX2);
        else if (which == "RX1") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_RX1);
        else if (which == "RX2") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_RX2);
        else throw std::runtime_error("e300_remote_codec_ctrl_impl incorrect chain string.");

        _transact();
        return _retval.freq;
    }

    void data_port_loopback(const bool on)
    {
        _clear();
        _args.action = uhd::htonx<uint32_t>(transaction_t::ACTION_SET_LOOPBACK);
        _args.which  = uhd::htonx<uint32_t>(transaction_t::CHAIN_NONE);  /*Unused*/
        _args.bits = uhd::htonx<uint32_t>(on ? 1 : 0);

        _transact();
    }

    sensor_value_t get_rssi(const std::string &which)
    {
        _clear();
        _args.action = uhd::htonx<uint32_t>(transaction_t::ACTION_GET_RSSI);
        if (which == "RX1") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_RX1);
        else if (which == "RX2") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_RX2);
        else throw std::runtime_error("e300_remote_codec_ctrl_impl incorrect chain string.");
        _args.bits = uhd::htonx<uint32_t>(0);

        _transact();
        return sensor_value_t("RSSI", _retval.rssi, "dB");
    }

    sensor_value_t get_temperature()
    {
        _clear();
        _args.action = uhd::htonx<uint32_t>(transaction_t::ACTION_GET_TEMPERATURE);
        _args.which  = uhd::htonx<uint32_t>(transaction_t::CHAIN_NONE);  /*Unused*/
        _args.bits = uhd::htonx<uint32_t>(0);

        _transact();
        return sensor_value_t("temp", _retval.temp, "C");
    }

    void set_dc_offset_auto(const std::string &which, const bool on)
    {
        _clear();
        _args.action = uhd::htonx<uint32_t>(transaction_t::ACTION_SET_DC_OFFSET_AUTO);
        if (which == "TX1")      _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_TX1);
        else if (which == "TX2") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_TX2);
        else if (which == "RX1") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_RX1);
        else if (which == "RX2") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_RX2);
        else throw std::runtime_error("e300_remote_codec_ctrl_impl incorrect chain string.");
        _args.use_dc_correction = on ? 1 : 0;

        _transact();
    }

    void set_iq_balance_auto(const std::string &which, const bool on)
    {
        _clear();
        _args.action = uhd::htonx<uint32_t>(transaction_t::ACTION_SET_IQ_BALANCE_AUTO);
        if (which == "TX1")     _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_TX1);
        else if (which == "TX2") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_TX2);
        else if (which == "RX1") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_RX1);
        else if (which == "RX2") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_RX2);
        else throw std::runtime_error("e300_remote_codec_ctrl_impl incorrect chain string.");
        _args.use_iq_correction = on ? 1 : 0;

        _transact();
    }

    void set_agc(const std::string &which, bool enable)
    {
        _clear();
       _args.action = uhd::htonx<uint32_t>(transaction_t::ACTION_SET_AGC);
       if (which == "TX1")      _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_TX1);
       else if (which == "TX2") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_TX2);
       else if (which == "RX1") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_RX1);
       else if (which == "RX2") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_RX2);
       else throw std::runtime_error("e300_remote_codec_ctrl_impl incorrect chain string.");
       _args.use_agc = enable ? 1 : 0;

       _transact();
    }

    void set_agc_mode(const std::string &which, const std::string &mode)
    {
        _clear();
       _args.action = uhd::htonx<uint32_t>(transaction_t::ACTION_SET_AGC_MODE);

       if (which == "TX1")      _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_TX1);
       else if (which == "TX2") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_TX2);
       else if (which == "RX1") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_RX1);
       else if (which == "RX2") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_RX2);
       else throw std::runtime_error("e300_remote_codec_ctrl_impl incorrect chain string.");

       if(mode == "slow") {
           _args.agc_mode = 0;
       } else if (mode == "fast") {
           _args.agc_mode = 1;
       } else {
           throw std::runtime_error("e300_remote_codec_ctrl_impl incorrect agc mode.");
       }

       _transact();
    }

    //! set the filter bandwidth for the frontend's analog low pass
    double set_bw_filter(const std::string &which, const double bw)
    {
        _clear();
        _args.action = uhd::htonx<uint32_t>(transaction_t::ACTION_SET_BW);
        if (which == "TX1")      _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_TX1);
        else if (which == "TX2") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_TX2);
        else if (which == "RX1") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_RX1);
        else if (which == "RX2") _args.which = uhd::htonx<uint32_t>(transaction_t::CHAIN_RX2);
        else throw std::runtime_error("e300_remote_codec_ctrl_impl incorrect chain string.");
        _args.bw = bw;

        _transact();
        return _retval.bw;
    }

    //! List all available filters by name
    std::vector<std::string> get_filter_names(const std::string &)
    {
        return std::vector<std::string>();
    }

    //! Return a list of all filters
    filter_info_base::sptr get_filter(const std::string &, const std::string &)
    {
        UHD_THROW_INVALID_CODE_PATH();
    }

    //! Write back a filter
    void set_filter(const std::string &, const std::string &, const filter_info_base::sptr)
    {
        UHD_LOGGER_WARNING("E300") << "Attempting to set filter on E300 in network mode." ;
    }

    void output_digital_test_tone(UHD_UNUSED(bool enb))
    {
        UHD_THROW_INVALID_CODE_PATH();
    }

    void set_timing_mode(UHD_UNUSED(const std::string &timing_mode))
    {
        UHD_THROW_INVALID_CODE_PATH();
    }
    
private:
    void _transact() {
        {
            uhd::transport::managed_send_buffer::sptr buff = _xport->get_send_buff(10.0);
            if (not buff or buff->size() < sizeof(_args))
                throw std::runtime_error("e300_remote_codec_ctrl_impl send timeout");
            std::memcpy(buff->cast<void *>(), &_args, sizeof(_args));
            buff->commit(sizeof(_args));
        }
        {
            uhd::transport::managed_recv_buffer::sptr buff = _xport->get_recv_buff(10.0);
            if (not buff or buff->size() < sizeof(_retval))
                throw std::runtime_error("e300_remote_codec_ctrl_impl recv timeout");
            std::memcpy(&_retval, buff->cast<const void *>(), sizeof(_retval));
        }

        if (_args.action != _retval.action)
            throw std::runtime_error("e300_remote_codec_ctrl_impl transaction failed.");
    }

    void _clear() {
        _args.action = 0;
        _args.which = 0;
        _args.bits = 0;
        _retval.action = 0;
        _retval.which = 0;
        _retval.bits = 0;
    }

    uhd::transport::zero_copy_if::sptr _xport;
    transaction_t                      _args;
    transaction_t                      _retval;
};

ad9361_ctrl::sptr e300_remote_codec_ctrl::make(uhd::transport::zero_copy_if::sptr xport)
{
    return sptr(new e300_remote_codec_ctrl_impl(xport));
}

}}};

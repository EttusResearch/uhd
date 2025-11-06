//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "../db_obx.hpp"
#include "obx_cpld_ctrl.hpp"
#include "obx_gpio_ctrl.hpp"
#include <uhd/experts/expert_nodes.hpp>
#include <uhdlib/usrp/common/max287x.hpp>

namespace uhd { namespace usrp { namespace dboard { namespace obx {

class obx_tx_frequency_expert : public uhd::experts::worker_node_t
{
public:
    obx_tx_frequency_expert(const uhd::experts::node_retriever_t& db,
        dboard_iface::sptr db_iface,
        obx_cpld_ctrl::sptr cpld,
        obx_gpio_ctrl::sptr gpio,
        max287x_iface::sptr lo1,
        max287x_iface::sptr lo2,
        double target_pfd_freq)
        : uhd::experts::worker_node_t("obx_tx_frequency_expert")
        , _db_iface(db_iface)
        , _cpld(cpld)
        , _gpio(gpio)
        , _lo1(lo1)
        , _lo2(lo2)
        , _target_pfd_freq(target_pfd_freq)
        , _freq_desired(db, "tx_freq/desired")
        , _tune_args(db, "tune_args")
        , _power_mode(db, "coerced_power_mode")
        , _sync_delay(db, "tx_sync_delay")
        , _freq_coerced(db, "tx_freq/coerced")
    {
        bind_accessor(_freq_desired);
        bind_accessor(_tune_args);
        bind_accessor(_power_mode);
        bind_accessor(_sync_delay);
        bind_accessor(_freq_coerced);
    }

private:
    void resolve() override;
    dboard_iface::sptr _db_iface;
    obx_cpld_ctrl::sptr _cpld;
    obx_gpio_ctrl::sptr _gpio;
    std::shared_ptr<max287x_iface> _lo1;
    std::shared_ptr<max287x_iface> _lo2;
    double _target_pfd_freq;
    // Inputs
    uhd::experts::data_reader_t<double> _freq_desired;
    uhd::experts::data_reader_t<device_addr_t> _tune_args;
    uhd::experts::data_reader_t<power_mode_t> _power_mode;
    uhd::experts::data_reader_t<int64_t> _sync_delay;
    //  Outputs
    uhd::experts::data_writer_t<double> _freq_coerced;
};

class obx_rx_frequency_expert : public uhd::experts::worker_node_t
{
public:
    obx_rx_frequency_expert(const uhd::experts::node_retriever_t& db,
        dboard_iface::sptr db_iface,
        obx_cpld_ctrl::sptr cpld,
        obx_gpio_ctrl::sptr gpio,
        max287x_iface::sptr lo1,
        max287x_iface::sptr lo2,
        double target_pfd_freq)
        : uhd::experts::worker_node_t("obx_rx_frequency_expert")
        , _db_iface(db_iface)
        , _cpld(cpld)
        , _gpio(gpio)
        , _lo1(lo1)
        , _lo2(lo2)
        , _target_pfd_freq(target_pfd_freq)
        , _freq_desired(db, "rx_freq/desired")
        , _tune_args(db, "tune_args")
        , _power_mode(db, "coerced_power_mode")
        , _sync_delay(db, "rx_sync_delay")
        , _freq_coerced(db, "rx_freq/coerced")
    {
        bind_accessor(_freq_desired);
        bind_accessor(_tune_args);
        bind_accessor(_power_mode);
        bind_accessor(_sync_delay);
        bind_accessor(_freq_coerced);
    }

private:
    void resolve() override;
    dboard_iface::sptr _db_iface;
    obx_cpld_ctrl::sptr _cpld;
    obx_gpio_ctrl::sptr _gpio;
    std::shared_ptr<max287x_iface> _lo1;
    std::shared_ptr<max287x_iface> _lo2;
    double _target_pfd_freq;
    // Inputs
    uhd::experts::data_reader_t<double> _freq_desired;
    uhd::experts::data_reader_t<device_addr_t> _tune_args;
    uhd::experts::data_reader_t<power_mode_t> _power_mode;
    uhd::experts::data_reader_t<int64_t> _sync_delay;
    //  Outputs
    uhd::experts::data_writer_t<double> _freq_coerced;
};

class obx_tx_frontend_expert : public uhd::experts::worker_node_t
{
public:
    obx_tx_frontend_expert(
        const uhd::experts::node_retriever_t& db, dboard_iface::sptr db_iface)
        : uhd::experts::worker_node_t("obx_tx_frontend_expert")
        , _db_iface(db_iface)
        , _freq_coerced(db, "tx_freq/coerced")
    {
        bind_accessor(_freq_coerced);
    }

private:
    void resolve() override;
    dboard_iface::sptr _db_iface;
    // Inputs
    uhd::experts::data_reader_t<double> _freq_coerced;
};

class obx_rx_frontend_expert : public uhd::experts::worker_node_t
{
public:
    obx_rx_frontend_expert(
        const uhd::experts::node_retriever_t& db, dboard_iface::sptr db_iface)
        : uhd::experts::worker_node_t("obx_rx_frontend_expert")
        , _db_iface(db_iface)
        , _freq_coerced(db, "rx_freq/coerced")
    {
        bind_accessor(_freq_coerced);
    }

private:
    void resolve() override;
    dboard_iface::sptr _db_iface;
    // Inputs
    uhd::experts::data_reader_t<double> _freq_coerced;
};

class obx_tx_antenna_expert : public uhd::experts::worker_node_t
{
public:
    obx_tx_antenna_expert(
        const uhd::experts::node_retriever_t& db, obx_cpld_ctrl::sptr cpld)
        : uhd::experts::worker_node_t("obx_tx_antenna_expert")
        , _cpld(cpld)
        , _antenna(db, "tx_antenna")
    {
        bind_accessor(_antenna);
    }

private:
    void resolve() override;
    obx_cpld_ctrl::sptr _cpld;
    // Inputs
    uhd::experts::data_reader_t<std::string> _antenna;
};

class obx_rx_antenna_expert : public uhd::experts::worker_node_t
{
public:
    obx_rx_antenna_expert(const uhd::experts::node_retriever_t& db,
        obx_cpld_ctrl::sptr cpld,
        obx_gpio_ctrl::sptr gpio)
        : uhd::experts::worker_node_t("obx_rx_antenna_expert")
        , _cpld(cpld)
        , _gpio(gpio)
        , _power_mode(db, "coerced_power_mode")
        , _antenna(db, "rx_antenna")
    {
        bind_accessor(_power_mode);
        bind_accessor(_antenna);
    }

private:
    void resolve() override;
    obx_cpld_ctrl::sptr _cpld;
    obx_gpio_ctrl::sptr _gpio;
    // Inputs
    uhd::experts::data_reader_t<power_mode_t> _power_mode;
    uhd::experts::data_reader_t<std::string> _antenna;
};

class obx_tx_gain_expert : public uhd::experts::worker_node_t
{
public:
    obx_tx_gain_expert(const uhd::experts::node_retriever_t& db, obx_gpio_ctrl::sptr gpio)
        : uhd::experts::worker_node_t("obx_tx_gain_expert")
        , _gpio(gpio)
        , _desired_gain(db, "tx_gain/desired")
        , _coerced_gain(db, "tx_gain/coerced")
    {
        bind_accessor(_desired_gain);
        bind_accessor(_coerced_gain);
    }

private:
    void resolve() override;
    obx_gpio_ctrl::sptr _gpio;
    // Inputs
    uhd::experts::data_reader_t<double> _desired_gain;
    // Outputs
    uhd::experts::data_writer_t<double> _coerced_gain;
};

class obx_rx_gain_expert : public uhd::experts::worker_node_t
{
public:
    obx_rx_gain_expert(const uhd::experts::node_retriever_t& db, obx_gpio_ctrl::sptr gpio)
        : uhd::experts::worker_node_t("obx_rx_gain_expert")
        , _gpio(gpio)
        , _desired_gain(db, "rx_gain/desired")
        , _coerced_gain(db, "rx_gain/coerced")
    {
        bind_accessor(_desired_gain);
        bind_accessor(_coerced_gain);
    }

private:
    void resolve() override;
    obx_gpio_ctrl::sptr _gpio;
    // Inputs
    uhd::experts::data_reader_t<double> _desired_gain;
    // Outputs
    uhd::experts::data_writer_t<double> _coerced_gain;
};

class obx_xcvr_mode_expert : public uhd::experts::worker_node_t
{
public:
    obx_xcvr_mode_expert(
        const uhd::experts::node_retriever_t& db, obx_cpld_ctrl::sptr cpld)
        : uhd::experts::worker_node_t("obx_xcvr_mode_expert")
        , _cpld(cpld)
        , _desired_xcvr_mode(db, "xcvr_mode")
        , _coerced_xcvr_mode(db, "coerced_xcvr_mode")
    {
        bind_accessor(_desired_xcvr_mode);
        bind_accessor(_coerced_xcvr_mode);
    }

private:
    void resolve() override;
    obx_cpld_ctrl::sptr _cpld;
    // Inputs
    uhd::experts::data_reader_t<std::string> _desired_xcvr_mode;
    // Outputs
    uhd::experts::data_writer_t<xcvr_mode_t> _coerced_xcvr_mode;
};

class obx_temp_comp_mode_expert : public uhd::experts::worker_node_t
{
public:
    obx_temp_comp_mode_expert(const uhd::experts::node_retriever_t& db,
        max287x_iface::sptr txlo1,
        max287x_iface::sptr txlo2,
        max287x_iface::sptr rxlo1,
        max287x_iface::sptr rxlo2)
        : uhd::experts::worker_node_t("obx_temp_comp_mode_expert")
        , _txlo1(txlo1)
        , _txlo2(txlo2)
        , _rxlo1(rxlo1)
        , _rxlo2(rxlo2)
        , _temp_comp_mode(db, "temp_comp_mode")
    {
        bind_accessor(_temp_comp_mode);
    }

private:
    void resolve() override;
    max287x_iface::sptr _txlo1;
    max287x_iface::sptr _txlo2;
    max287x_iface::sptr _rxlo1;
    max287x_iface::sptr _rxlo2;
    // Inputs
    uhd::experts::data_reader_t<std::string> _temp_comp_mode;
};

class obx_power_mode_expert : public uhd::experts::worker_node_t
{
public:
    obx_power_mode_expert(const uhd::experts::node_retriever_t& db,
        dboard_iface::sptr db_iface,
        obx_cpld_ctrl::sptr cpld)
        : uhd::experts::worker_node_t("obx_power_mode_expert")
        , _db_iface(db_iface)
        , _cpld(cpld)
        , _desired_power_mode(db, "power_mode")
        , _coerced_power_mode(db, "coerced_power_mode")
    {
        bind_accessor(_desired_power_mode);
        bind_accessor(_coerced_power_mode);
    }

private:
    void resolve() override;
    dboard_iface::sptr _db_iface;
    obx_cpld_ctrl::sptr _cpld;
    // Inputs
    uhd::experts::data_reader_t<std::string> _desired_power_mode;
    // Outputs
    uhd::experts::data_writer_t<power_mode_t> _coerced_power_mode;
};

}}}} // namespace uhd::usrp::dboard::obx

//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "fbx_constants.hpp"
#include "fbx_ctrl.hpp"
#include <uhd/experts/expert_nodes.hpp>
#include <uhd/property_tree.hpp>
#include <uhdlib/usrp/common/rpc.hpp>
#include <uhdlib/usrp/common/x400_rfdc_control.hpp>
#include <cmath>
#include <memory>

namespace uhd { namespace usrp { namespace fbx {

/*!---------------------------------------------------------
 * fbx_scheduling_expert
 *
 * This expert is responsible for scheduling time sensitive actions
 * in other experts. It responds to changes in the command time and
 * selectively causes experts to run in order to ensure a synchronized
 * system.
 *
 * There is one scheduling expert per channel, they are shared between RX and TX.
 * So, 2 scheduling experts total per radio block.
 * ---------------------------------------------------------
 */
class fbx_scheduling_expert : public experts::worker_node_t
{
public:
    fbx_scheduling_expert(const experts::node_retriever_t& db, const uhd::fs_path fe_path)
        : experts::worker_node_t(fe_path / "fbx_scheduling_expert")
        , _command_time(db, fe_path / "time/cmd")
        , _frontend_time(db, fe_path / "time/fe")
    {
        bind_accessor(_command_time);
        bind_accessor(_frontend_time);
    }

private:
    virtual void resolve() override;

    // Inputs:
    experts::data_reader_t<time_spec_t> _command_time;

    // Outputs:
    experts::data_writer_t<time_spec_t> _frontend_time;
};

/*!---------------------------------------------------------
 * fbx_tx_programming_expert
 *
 * This expert is responsible for programming the FBX with parameters determined by user
 * input or other experts. This includes antenna and LED setting.
 *
 * This expert should not trigger any other experts, these are all blind parameters
 *
 * One instance of this expert is required for each TX Channel (0,1,2,3); four total
 * --------------------------------------------------------
 */
class fbx_tx_programming_expert : public uhd::experts::worker_node_t
{
public:
    fbx_tx_programming_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path tx_fe_path,
        const uhd::fs_path rx_fe_path,
        const size_t chan,
        std::shared_ptr<fbx_ctrl> fbx_ctrl)
        : experts::worker_node_t(tx_fe_path / "fbx_tx_programming_expert")
        , _antenna(db, tx_fe_path / "antenna" / "value")
        , _command_time(db, rx_fe_path / "time" / "cmd")
        , _frequency(db, tx_fe_path / "freq" / "coerced")
        , _fbx_ctrl(fbx_ctrl)
        , _chan(chan)
    {
        bind_accessor(_antenna);
        bind_accessor(_command_time);
        bind_accessor(_frequency);
    }

private:
    void resolve() override;

    // Inputs from user/API
    uhd::experts::data_reader_t<std::string> _antenna;

    // Inputs from the Frequency FE expert
    // Note: this is just for node dependencies, we want to be notified if just the tune
    // frequency has been changed.
    uhd::experts::data_reader_t<time_spec_t> _command_time;
    uhd::experts::data_reader_t<double> _frequency;

    // Expects constructed SW&LED control objects
    std::shared_ptr<fbx_ctrl> _fbx_ctrl;
    const size_t _chan;
};

/*!---------------------------------------------------------
 * fbx_rx_programming_expert (RX Programming Expert)
 *
 * This expert is responsible for programming the FBX with parameters determined by user
 * input or other experts. This includes antenna and LED setting.
 *
 * This expert should not trigger any other experts, these are all blind parameters
 *
 * One instance of this expert is required for each RX Channel (0,1,2,3); four total
 * --------------------------------------------------------
 */
class fbx_rx_programming_expert : public uhd::experts::worker_node_t
{
public:
    fbx_rx_programming_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        const size_t chan,
        std::shared_ptr<fbx_ctrl> fbx_ctrl)
        : experts::worker_node_t(fe_path / "fbx_rx_programming_expert")
        , _antenna(db, fe_path / "antenna" / "value")
        , _command_time(db, fe_path / "time" / "cmd")
        , _frequency(db, fe_path / "freq" / "coerced")
        , _fbx_ctrl(fbx_ctrl)
        , _chan(chan)
    {
        bind_accessor(_antenna);
        bind_accessor(_command_time);
        bind_accessor(_frequency);
    }

private:
    void resolve() override;
    void _update_leds();

    // Inputs from user/API
    uhd::experts::data_reader_t<std::string> _antenna;

    // Inputs from the Frequency FE expert
    // Note: this is just for node dependencies, we want to be notified if just the tune
    // frequency has been changed.
    uhd::experts::data_reader_t<time_spec_t> _command_time;
    uhd::experts::data_reader_t<double> _frequency;

    // Expects constructed control objects
    std::shared_ptr<fbx_ctrl> _fbx_ctrl;
    const size_t _chan;
};

/*!---------------------------------------------------------
 * fbx_band_inversion_expert
 *
 * This expert is responsible for handling the band inversion calls to MPM on the target
 * device
 *
 * This expert should not trigger any others
 *
 * One instance of this expert is required for each Direction (TX/RX) and Channel (0-3);
 * eight total
 * --------------------------------------------------------
 */
class fbx_band_inversion_expert : public uhd::experts::worker_node_t
{
public:
    fbx_band_inversion_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        const uhd::direction_t trx,
        const size_t chan,
        const double rfdc_rate,
        uhd::usrp::fbx_rpc_iface::sptr rpcc)
        : uhd::experts::worker_node_t(fe_path / "fbx_band_inversion_expert")
        , _frequency(db, fe_path / "freq" / "coerced")
        , _rpcc(rpcc)
        , _trx(trx)
        , _chan(chan)
        , _rfdc_rate(rfdc_rate)
    {
        bind_accessor(_frequency);
    }

private:
    void resolve() override;

    // Inputs from Frequency RFDC Freq expert
    uhd::experts::data_reader_t<double> _frequency;

    uhd::usrp::fbx_rpc_iface::sptr _rpcc;
    const uhd::direction_t _trx;
    const size_t _chan;
    const double _rfdc_rate;
};

/*!---------------------------------------------------------
 * fbx_rfdc_freq_expert
 *
 * This expert is responsible for handling any rfdc frequency calls to MPM on the target
 * device
 *
 * This expert should not trigger any experts
 *
 * One instance of this expert is required for each Direction (TX/RX) and Channel (0-3);
 * eight total
 * --------------------------------------------------------
 */
class fbx_rfdc_freq_expert : public uhd::experts::worker_node_t
{
public:
    fbx_rfdc_freq_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        const uhd::direction_t trx,
        const size_t chan,
        const double rfdc_rate,
        const std::string rpc_prefix,
        int db_idx,
        uhd::usrp::x400_rpc_iface::sptr rpcc)
        : uhd::experts::worker_node_t(fe_path / "fbx_rfdc_freq_expert")
        , _rfdc_freq_desired(
              db, fe_path / "los" / RFDC_NCO / "freq" / "value" / "desired")
        , _rfdc_freq_coerced(
              db, fe_path / "los" / RFDC_NCO / "freq" / "value" / "coerced")
        , _frequency_desired(db, fe_path / "freq" / "desired")
        , _if_frequency_coerced(db, fe_path / "if_freq" / "coerced")
        , _coerced_frequency(db, fe_path / "freq" / "coerced")
        , _rpc_prefix(rpc_prefix)
        , _db_idx(db_idx)
        , _rpcc(rpcc)
        , _trx(trx)
        , _rfdc_rate(rfdc_rate)
        , _chan(chan)
    {
        bind_accessor(_rfdc_freq_desired);
        bind_accessor(_rfdc_freq_coerced);
        bind_accessor(_frequency_desired);
        bind_accessor(_if_frequency_coerced);
        bind_accessor(_coerced_frequency);
    }

private:
    void resolve() override;

    // Inputs from user/API
    uhd::experts::data_reader_t<double> _rfdc_freq_desired;

    // Outputs to user/API
    uhd::experts::data_writer_t<double> _rfdc_freq_coerced;

    // Inputs from Frequency FE expert
    uhd::experts::data_reader_t<double> _frequency_desired;

    // Outputs to sync expert
    uhd::experts::data_writer_t<double> _if_frequency_coerced;

    // Outputs to generic freq property
    uhd::experts::data_writer_t<double> _coerced_frequency;

    const std::string _rpc_prefix;
    const size_t _db_idx;
    uhd::usrp::x400_rpc_iface::sptr _rpcc;
    const uhd::direction_t _trx;
    const double _rfdc_rate;
    const size_t _chan;
};

using namespace uhd::rfnoc::x400;
using uhd::rfnoc::x400::rfdc_control;
/*!---------------------------------------------------------
 * fbx_sync_expert
 *
 * This expert is responsible for handling the phase alignment.
 * Per channel, there are up to 2 things whose phase need syncing: The NCO, and the
 * ADC/DAC gearboxes. However, the NCOs share a sync register. To minimize writes, we thus
 * need a single sync expert at the end of the graph, which combines all NCOs.
 * --------------------------------------------------------
 */
class fbx_sync_expert : public uhd::experts::worker_node_t
{
public:
    fbx_sync_expert(const uhd::experts::node_retriever_t& db,
        const size_t num_channels,
        const uhd::fs_path tx_fe_path,
        const uhd::fs_path rx_fe_path,
        rfdc_control::sptr rfdcc)
        : uhd::experts::worker_node_t("fbx_sync_expert")
        , _fe_time(get_fe_time(db, num_channels, rx_fe_path))
        , _nco_freqs(get_nco_freqs(db, num_channels, rx_fe_path, tx_fe_path))
        , _num_chans(num_channels)
        , _rfdcc(rfdcc)
    {
        for (auto& fe_time : _fe_time) {
            bind_accessor(fe_time);
        }

        for (auto& nco_freq : _nco_freqs) {
            bind_accessor(nco_freq.second);
        }
    }

private:
    /*
     * Helper function to get the right number of _fe_time elements depending on the
     * number of channels that are currently active.
     */
    std::vector<uhd::experts::data_reader_t<uhd::time_spec_t>> get_fe_time(
        const uhd::experts::node_retriever_t& db,
        const size_t num_channels,
        const uhd::fs_path rx_fe_path)
    {
        UHD_ASSERT_THROW(num_channels <= FBX_MAX_NUM_CHANS);
        std::vector<uhd::experts::data_reader_t<uhd::time_spec_t>> fe_time;
        fe_time.reserve(num_channels);
        for (size_t i = 0; i < num_channels; i++) {
            fe_time.emplace_back(db, rx_fe_path / i / "time/fe");
        }
        return fe_time;
    }

    /*
     * Helper function to get the right number of _nco_freqs elements depending on the
     * number of channels that are currently active.
     */
    std::map<rfdc_control::rfdc_type, uhd::experts::data_reader_t<double>> get_nco_freqs(
        const uhd::experts::node_retriever_t& db,
        const size_t num_channels,
        const uhd::fs_path rx_fe_path,
        const uhd::fs_path tx_fe_path)
    {
        UHD_ASSERT_THROW(num_channels <= FBX_MAX_NUM_CHANS);
        std::map<rfdc_control::rfdc_type, uhd::experts::data_reader_t<double>> nco_freqs;
        indexed_for(RX_RFDC,
            num_channels,
            [&nco_freqs, &db, &rx_fe_path](const auto& item, size_t idx) {
                nco_freqs.insert({item, {db, rx_fe_path / idx / "if_freq" / "coerced"}});
            });
        indexed_for(TX_RFDC,
            num_channels,
            [&nco_freqs, &db, &tx_fe_path](const auto& item, size_t idx) {
                nco_freqs.insert({item, {db, tx_fe_path / idx / "if_freq" / "coerced"}});
            });

        return nco_freqs;
    }

    void resolve() override;

    // Inputs from user/API
    // Command time: We have up to 4 channels, one time spec per channel
    std::vector<uhd::experts::data_reader_t<time_spec_t>> _fe_time;
    // We have 8 NCOs
    std::map<rfdc_control::rfdc_type, uhd::experts::data_reader_t<double>> _nco_freqs;

    // This expert has no outputs.

    // Attributes
    size_t _num_chans;
    rfdc_control::sptr _rfdcc;
    //! Store the sync state of the ADC gearboxes. If false, we assume they're
    // out of sync. This could also be a vector of booleans if we want to be
    // able to sync ADC gearboxes individually.
    bool _adcs_synced = false;
    //! Store the sync state of the DAC gearboxes. If false, we assume they're
    // out of sync. This could also be a vector of booleans if we want to be
    // able to sync DAC gearboxes individually.
    bool _dacs_synced = false;
};

}}} // namespace uhd::usrp::fbx

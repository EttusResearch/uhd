//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/dboard/fbx/fbx_ctrl.hpp>
#include <map>
#include <thread>

namespace uhd { namespace usrp { namespace fbx {

fbx_ctrl::fbx_ctrl(poke_fn_type&& poke_fn,
    peek_fn_type&& peek_fn,
    sleep_fn_type&& sleep_fn,
    const std::string& log_id)
    : _poke32(std::move(poke_fn))
    , _peek32(std::move(peek_fn))
    , _sleep(std::move(sleep_fn))
    , _log_id(log_id)
{
    UHD_LOG_TRACE(_log_id, "Entering fbx_ctrl ctor...");
    // Reset and stash the regs state. We can't assume the defaults in
    // gen_fbx_regs.py match whats on the hardware.
    commit(NO_CHAN, true);
    _regs.save_state();

    UHD_LOG_TRACE(_log_id, "FPGA communication successful.");
    set_classic_atr();
}

bool fbx_ctrl::poll_reg_with_timeout(
    const std::string& reg, uint32_t timeout_ms, std::function<bool(uint32_t)> check)
{
    auto addr = _regs.get_addr(reg);
    // Doing an initial peek to avoid running into the timeout if timed commands were
    // used. Peeks are blocking commands but may be triggered as timed commands (with
    // delay thus). So whenever a method calls this poll_reg_with_timeout(), chances are
    // high that we'll fail as false negative if we don't check initially and only then
    // start the timeout.
    if (check(_peek32(addr))) {
        return true;
    }
    using namespace std::chrono;
    const auto timeout_time =
        std::chrono::steady_clock::now()
        + std::chrono::milliseconds(static_cast<int64_t>(timeout_ms));
    while (std::chrono::steady_clock::now() < timeout_time) {
        if (check(_peek32(addr))) {
            return true;
        }
    }
    return false;
}

void fbx_ctrl::set_classic_atr()
{
    _regs.RF0_ATR_OPTION  = _regs.RF0_ATR_OPTION_CLASSIC;
    _regs.RF1_ATR_OPTION  = _regs.RF1_ATR_OPTION_CLASSIC;
    _regs.RF2_ATR_OPTION  = _regs.RF2_ATR_OPTION_CLASSIC;
    _regs.RF3_ATR_OPTION  = _regs.RF3_ATR_OPTION_CLASSIC;
    _regs.LED0_ATR_OPTION = _regs.LED0_ATR_OPTION_CLASSIC;
    _regs.LED1_ATR_OPTION = _regs.LED1_ATR_OPTION_CLASSIC;
    _regs.LED2_ATR_OPTION = _regs.LED2_ATR_OPTION_CLASSIC;
    _regs.LED3_ATR_OPTION = _regs.LED3_ATR_OPTION_CLASSIC;
    commit(NO_CHAN);
}

#define TRX_REG_TABLE(X) \
    X(0)                 \
    X(1)                 \
    X(2)                 \
    X(3)

#define TX_REG_ASSIGN(CHANNEL)                                                      \
    if (channel == CHANNEL) {                                                       \
        if (antenna == ANTENNA_TXRX) {                                              \
            _regs.RF##CHANNEL##_TX_RX_RFS[idx] = _regs.RF##CHANNEL##_TX_RX_RFS_RF1; \
            _regs.RF##CHANNEL##_TDDS[idx]      = _regs.RF##CHANNEL##_TDDS_RF2;      \
        } else if (antenna == ANTENNA_CAL_LOOPBACK) {                               \
            _regs.RF##CHANNEL##_TX_RX_RFS[idx] = _regs.RF##CHANNEL##_TX_RX_RFS_RF2; \
        } else {                                                                    \
            /* Since we have checked the validity of the antenna name before */     \
            /* we should never get here. */                                         \
            UHD_THROW_INVALID_CODE_PATH();                                          \
        }                                                                           \
        /* If we're defining the IDLE state and are not in CAL_LOOPBACK antenna */  \
        /* then overwrite previous settings with the switch settings for idle */    \
        /* (maximum isolation). This doesn't cause additional register writes */    \
        /* because this is buffered and only written when commit() is called. */    \
        /* If the antenna is set to CAL_LOOPBACK, we won't configure the IDLE */    \
        /* case differently than the TX state because the ADC selc cal doesn't */   \
        /* switch ATR states and operates in IDLE state. */                         \
        if (idx == ATR_ADDR_0X && antenna != ANTENNA_CAL_LOOPBACK) {                \
            _regs.RF##CHANNEL##_TX_RX_RFS[idx] = _regs.RF##CHANNEL##_TX_RX_RFS_RF2; \
            _regs.RF##CHANNEL##_RX_RFS[idx]    = _regs.RF##CHANNEL##_RX_RFS_RF1;    \
            _regs.RF##CHANNEL##_TDDS[idx]      = _regs.RF##CHANNEL##_TDDS_RF2;      \
        }                                                                           \
    } else

void fbx_ctrl::set_tx_antenna_switches(
    const size_t channel, const uint8_t idx, const std::string& antenna)
{
    UHD_ASSERT_THROW(channel < FBX_MAX_NUM_CHANS);
    UHD_ASSERT_THROW(
        std::find(TX_ANTENNAS.begin(), TX_ANTENNAS.end(), antenna) != TX_ANTENNAS.end());

    // Create code for all 4 channels per DB via macro:
    TRX_REG_TABLE(TX_REG_ASSIGN) {}

    commit(chan_t(channel));
}

/* Putting this into a macro as otherwise we'd have to write it down for all four channels
 * individually due to the indices in the variable names */
#define RX_REG_ASSIGN(CHANNEL)                                                      \
    if (channel == CHANNEL) {                                                       \
        if (antenna == ANTENNA_TXRX) {                                              \
            _regs.SYNC##CHANNEL##_CTRL      = _regs.SYNC##CHANNEL##_CTRL_RF1;       \
            _regs.RF##CHANNEL##_RX_RFS[idx] = _regs.RF##CHANNEL##_RX_RFS_RF1;       \
            _regs.RF##CHANNEL##_TDDS[idx]   = _regs.RF##CHANNEL##_TDDS_RF1;         \
        } else if (antenna == ANTENNA_RX) {                                         \
            _regs.SYNC##CHANNEL##_CTRL      = _regs.SYNC##CHANNEL##_CTRL_RF1;       \
            _regs.RF##CHANNEL##_RX_RFS[idx] = _regs.RF##CHANNEL##_RX_RFS_RF2;       \
            _regs.RF##CHANNEL##_TDDS[idx]   = _regs.RF##CHANNEL##_TDDS_RF2;         \
        } else if (antenna == ANTENNA_CAL_LOOPBACK) {                               \
            _regs.RF##CHANNEL##_RX_RFS[idx] = _regs.RF##CHANNEL##_RX_RFS_RF1;       \
            _regs.SYNC##CHANNEL##_CTRL      = _regs.SYNC##CHANNEL##_CTRL_RF4;       \
        } else if (antenna == ANTENNA_SYNC_INT) {                                   \
            _regs.SYNC4_CTRL                = _regs.SYNC4_CTRL_RF1;                 \
            _regs.SYNC##CHANNEL##_CTRL      = _regs.SYNC##CHANNEL##_CTRL_RF3;       \
            _regs.RF##CHANNEL##_RX_RFS[idx] = _regs.RF##CHANNEL##_RX_RFS_RF1;       \
        } else if (antenna == ANTENNA_SYNC_EXT) {                                   \
            _regs.SYNC4_CTRL                = _regs.SYNC4_CTRL_RF4;                 \
            _regs.SYNC##CHANNEL##_CTRL      = _regs.SYNC##CHANNEL##_CTRL_RF3;       \
            _regs.RF##CHANNEL##_RX_RFS[idx] = _regs.RF##CHANNEL##_RX_RFS_RF1;       \
        } else if (antenna == ANTENNA_TERMINATION) {                                \
            _regs.SYNC##CHANNEL##_CTRL      = _regs.SYNC##CHANNEL##_CTRL_RF2;       \
            _regs.RF##CHANNEL##_RX_RFS[idx] = _regs.RF##CHANNEL##_RX_RFS_RF1;       \
        } else {                                                                    \
            /* Since we have checked the validity of the antenna name before */     \
            /* we should never get here. */                                         \
            UHD_THROW_INVALID_CODE_PATH();                                          \
        }                                                                           \
        /* If we're defining the IDLE state and are not in CAL_LOOPBACK antenna */  \
        /* then overwrite previous settings with the switch settings for idle */    \
        /* (maximum isolation). This doesn't cause additional register writes */    \
        /* because this is buffered and only written when commit() is called. */    \
        /* If the antenna is set to CAL_LOOPBACK, we won't configure the IDLE */    \
        /* case differently than the RX state because the ADC selc cal doesn't */   \
        /* switch ATR states and operates in IDLE state. */                         \
        if (idx == ATR_ADDR_0X && antenna != ANTENNA_CAL_LOOPBACK) {                \
            _regs.RF##CHANNEL##_TX_RX_RFS[idx] = _regs.RF##CHANNEL##_TX_RX_RFS_RF2; \
            _regs.RF##CHANNEL##_RX_RFS[idx]    = _regs.RF##CHANNEL##_RX_RFS_RF1;    \
            _regs.RF##CHANNEL##_TDDS[idx]      = _regs.RF##CHANNEL##_TDDS_RF2;      \
        }                                                                           \
        /*Turn off SYNC4 for better isolation in case no one is using it. */        \
        if (!any_ch_connected_to_sync()) {                                          \
            _regs.SYNC4_CTRL = _regs.SYNC4_CTRL_OFF;                                \
        }                                                                           \
    } else

void fbx_ctrl::set_rx_antenna_switches(
    const size_t channel, const uint8_t idx, const std::string& antenna)
{
    UHD_ASSERT_THROW(channel < FBX_MAX_NUM_CHANS);
    UHD_ASSERT_THROW(
        std::find(RX_ANTENNAS.begin(), RX_ANTENNAS.end(), antenna) != RX_ANTENNAS.end());

    // Antenna settings: RX2, TX/RX, CAL_LOOPBACK, TERMINATION
    // Create code for all 4 channels via macro
    TRX_REG_TABLE(RX_REG_ASSIGN) {}

    commit(chan_t(channel));
}

std::string fbx_ctrl::get_rx_sync_switch_state()
{
    if (_regs.SYNC4_CTRL == _regs.SYNC4_CTRL_RF4) {
        return ANTENNA_SYNC_EXT;
    } else if (_regs.SYNC4_CTRL == _regs.SYNC4_CTRL_RF1) {
        return ANTENNA_SYNC_INT;
    } else if (_regs.SYNC4_CTRL == _regs.SYNC4_CTRL_RF2
               || _regs.SYNC4_CTRL == _regs.SYNC4_CTRL_RF3) {
        return ANTENNA_TERMINATION;
    } else if (_regs.SYNC4_CTRL == _regs.SYNC4_CTRL_OFF) {
        return ANTENNA_OFF;
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }
}

#define LED_REG_ASSIGN(CHANNEL)                                      \
    if (channel == CHANNEL) {                                        \
        _regs.CH##CHANNEL##_RX2_LED[idx] =                           \
            rx ? fbx_regs_t::CH##CHANNEL##_RX2_LED_ENABLE            \
               : fbx_regs_t::CH##CHANNEL##_RX2_LED_DISABLE;          \
        _regs.CH##CHANNEL##_TRX1_LED_RED[idx] =                      \
            trx_tx ? fbx_regs_t::CH##CHANNEL##_TRX1_LED_RED_ENABLE   \
                   : fbx_regs_t::CH##CHANNEL##_TRX1_LED_RED_DISABLE; \
        _regs.CH##CHANNEL##_TRX1_LED_GR[idx] =                       \
            trx_rx ? fbx_regs_t::CH##CHANNEL##_TRX1_LED_GR_ENABLE    \
                   : fbx_regs_t::CH##CHANNEL##_TRX1_LED_GR_DISABLE;  \
    } else

/******************************************************************************
 * LED control
 *****************************************************************************/
void fbx_ctrl::set_leds(const size_t channel,
    const uint8_t idx,
    const bool rx,
    const bool trx_rx,
    const bool trx_tx)
{
    UHD_ASSERT_THROW(channel < FBX_MAX_NUM_CHANS);

    // Create code for all 4 channels via macro
    TRX_REG_TABLE(LED_REG_ASSIGN) {}

    commit(chan_t(channel));
}

/**************************************************************************
 * Internal SYNC signal control
 *************************************************************************/
void fbx_ctrl::set_internal_sync_clk(const bool enable)
{
    _regs.SYNC_CLK = enable ? _regs.SYNC_CLK_ENABLE : _regs.SYNC_CLK_DISABLE;
    commit(NO_CHAN);
}

/******************************************************************************
 * Private methods
 *****************************************************************************/

void fbx_ctrl::commit(const chan_t chan, const bool save_all)
{
    UHD_LOG_TRACE(_log_id,
        "Storing register cache " << (save_all ? "completely" : "selectively")
                                  << " to register...");
    const auto changed_addrs = save_all ? _regs.get_all_addrs<size_t>()
                                        : _regs.get_changed_addrs<size_t>();
    // If something was configured that is connected to the I/O expander we need to run
    // the config sequence.
    bool io_exp_cfg_req         = false;
    const auto sync_range_start = _regs.get_addr("SYNC0_CTRL");
    const auto sync_range_stop  = _regs.get_addr("SYNC4_CTRL");
    const auto io_exp_setup     = _regs.get_addr("IO_EXP_SETUP");
    const auto io_exp_config    = _regs.get_addr("IO_EXP_CONFIG");
    auto check_first_bit        = [](uint32_t value) { return (0x1 & value) == 1; };
    for (const auto addr : changed_addrs) {
        // The I/O Expander config register is never called on its own but only if any
        // value in it was changed. Therefore we exclude it here but call it later on if
        // required.
        if (addr != io_exp_config) {
            _poke32(addr, _regs.get_reg(addr), save_all ? NO_CHAN : chan);

            // We need to poll the I/O Expander setup register after we have set it.
            // Usually this is only done once during startup.
            if (addr == io_exp_setup) {
                if (!poll_reg_with_timeout("IO_EXP_SETUP", 100, check_first_bit)) {
                    throw uhd::io_error(
                        "I/O Expander could not be initialized successfully.");
                }
            } else {
                // If any value on the I/O expander was changed set the io expander config
                // required flag to true
                if ((sync_range_start <= addr) && (addr <= sync_range_stop)) {
                    io_exp_cfg_req = true;
                }
            }
        }
    }
    // Apply the new values on the I/O expander by first poking, then polling the I/O
    // expander config register. Timeout if we don't get the proper value back.
    if (io_exp_cfg_req) {
        _poke32(io_exp_config, 0x1, NO_CHAN);
        // FIXME: In timed commands this will block the execution of the corresponding
        // method (e.g. set_trx_antenna()) because peek is a blocking command.
        if (!fbx_ctrl::poll_reg_with_timeout("IO_EXP_CONFIG", 100, check_first_bit)) {
            throw uhd::io_error(
                "I/O Expander configuration did not return properly within 100 ms.");
        }
    }
    _regs.save_state();
    UHD_LOG_TRACE(_log_id,
        "Storing cache complete: "
        "Updated "
            << changed_addrs.size() << " registers.");
}

bool fbx_ctrl::any_ch_connected_to_sync()
{
    // If any of the four channels has set its sync switch to RF3 it is routed towards
    // the last sync switch. In that case this last sync switch shouldn't be turned
    // off, otherwise we can safely do that.
    return (_regs.SYNC0_CTRL == _regs.SYNC0_CTRL_RF3
            || _regs.SYNC1_CTRL == _regs.SYNC1_CTRL_RF3
            || _regs.SYNC2_CTRL == _regs.SYNC2_CTRL_RF3
            || _regs.SYNC3_CTRL == _regs.SYNC3_CTRL_RF3);
}
}}} // namespace uhd::usrp::fbx

//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include <uhd/exception.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/paths.hpp>
#include <uhdlib/usrp/common/adrv9032_ctrl.hpp>
#include <uhdlib/utils/log.hpp>
#include <adi_adrv903x_cals.h>
#include <adi_adrv903x_core.h>
#include <adi_adrv903x_cpu.h>
#include <adi_adrv903x_datainterface.h>
#include <adi_adrv903x_radioctrl.h>
#include <adi_adrv903x_rx.h>
#include <adi_adrv903x_tx.h>
#include <adi_adrv903x_types.h>
#include <adi_adrv903x_utilities.h>
#include <adi_platform.h>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <string>

#define FILEPATH_STRNCPY(dest, src)           \
    {                                         \
        (void)strncpy((char*)dest, src, 255); \
        dest[255] = '\0';                     \
    }

#define CHECK_ADRV903x_FUNC(func) \
    {                             \
        ret_code = func;          \
        if (ret_code)             \
            return ret_code;      \
    }

namespace uhd { namespace usrp {

namespace {
// Return an uppercased copy of the input string. Each character is cast to
// unsigned char before calling std::toupper to avoid undefined behavior on
// negative char values.
std::string to_upper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return s;
}

// Parse a string of calibration names separated by '|' into an OR'd bitmask.
// Each token is trimmed of surrounding whitespace, uppercased, and looked up in
// cal_name_map. Empty/whitespace-only tokens are skipped and unknown names are
// logged and ignored.
uint64_t parse_cal_mask(const std::string& cals_str,
    const std::unordered_map<std::string, uint64_t>& cal_name_map)
{
    uint64_t calMask = 0;
    std::stringstream ss(cals_str);
    std::string token;
    while (std::getline(ss, token, '|')) {
        const auto start = token.find_first_not_of(" \t");
        if (start == std::string::npos) {
            continue; // skip empty/whitespace-only token
        }
        const auto end = token.find_last_not_of(" \t");
        token          = token.substr(start, end - start + 1);
        const auto it  = cal_name_map.find(to_upper(token));
        if (it == cal_name_map.end()) {
            UHD_LOG_WARNING("ADRV9032",
                "Calibration key: " + token
                    + " is not in calibration table. Ignoring this key.");
        } else {
            calMask |= it->second;
        }
    }
    return calMask;
}
} // namespace

// User Guide says max attenuation is 41.95 dB, but no signal is seen programming
// that level, so setting max attenuation to 41.5 instead. It can be re-evaluated later if
// that 0.45 dB of attenuation is needed.
static const double MAX_TX_ATTENUATION = 41.5;

class adrv9032_ctrl_impl : public adrv9032_ctrl
{
public:
    adrv9032_ctrl_impl(std::function<void(size_t, uint32_t)> channel_enable_fn,
        std::function<void(const uhd::time_spec_t&)> sleep_fn,
        std::string init_cal_args,
        std::string tracking_cal_args)
        : _channel_enable_fn(channel_enable_fn)
        , _sleep_fn(sleep_fn)
        , _init_cal_args(init_cal_args)
    {
        _adrv903x_device.common.errPtr = &_errData;
        _tracking_cal_mask = _parse_tracking_cals(to_upper(tracking_cal_args));
    }

    ~adrv9032_ctrl_impl()
    {
        if (_adrv903x_device.common.devHalInfo != NULL) {
            free(_adrv903x_device.common.devHalInfo);
            _adrv903x_device.common.devHalInfo = NULL;
        }
    }

    adi_adrv903x_ErrAction_e hardware_open(spi_core_adrv::sptr spi,
        std::function<void(uint8_t)> reset_poke_fn,
        double master_clock_rate) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        //  Associate the ADI platform HAL interface functions
        adi_hal_PlatformSetup(USRP_B310);

        auto fw_filepath = get_images_dir("") + "/ADRV9032_FW.bin";
        auto profile_filepath =
            get_images_dir("")
            + (uhd::math::frequencies_are_equal(master_clock_rate, 122.88e6)
                    ? "/ADRV9032_PROFILE_122_88.bin"
                    : "/ADRV9032_PROFILE_125.bin");
        auto stream_filepath =
            get_images_dir("")
            + (uhd::math::frequencies_are_equal(master_clock_rate, 122.88e6)
                    ? "/ADRV9032_STREAM_122_88.bin"
                    : "/ADRV9032_STREAM_125.bin");
        auto gain_table_filepath = get_images_dir("") + "/ADRV9032_RX_GAIN_TABLE.csv";

        // Verify all required image files exist before attempting to program the
        // ADRV9032. If any are missing, warn for each and then throw, directing the
        // user to run uhd_images_downloader.
        bool missing_file = false;
        for (const auto& filepath :
            {fw_filepath, profile_filepath, stream_filepath, gain_table_filepath}) {
            if (!std::filesystem::exists(filepath)) {
                UHD_LOG_WARNING(
                    "ADRV9032", "Required ADRV9032 image file not found: " << filepath);
                missing_file = true;
            }
        }
        if (missing_file) {
            throw uhd::runtime_error(
                "One or more required ADRV9032 image files are missing. Please run "
                "uhd_images_downloader to download the required images.");
        }

        // Set the Binary File Paths to program the ADRV9032
        FILEPATH_STRNCPY(_trxFileInfo.stream.filePath, stream_filepath.c_str());
        FILEPATH_STRNCPY(_trxFileInfo.cpu.filePath, fw_filepath.c_str());
        FILEPATH_STRNCPY(_trxFileInfo.cpuProfile.filePath, profile_filepath.c_str());
        FILEPATH_STRNCPY(
            _trxFileInfo.rxGainTable[0].filePath, gain_table_filepath.c_str());
        // Set all channels
        _trxFileInfo.rxGainTable[0].channelMask = ADI_ADRV903X_TX4 | ADI_ADRV903X_TX0;

        // Set up the Device Data Structure for the ADRV9032
        uhd_adrv9032_hal_cfg_t* devHalCfg            = new uhd_adrv9032_hal_cfg_t();
        adi_adrv903x_SpiConfigSettings_t spiSettings = {
            1, /* 1 => MSB First */
            1, /* 1 => Use 4-wire SPI*/
            ADI_ADRV903X_CMOSPAD_DRV_WEAK /* 5pF load @ 75MHz */
        };
        devHalCfg->spi_iface               = spi;
        devHalCfg->spi_slave               = 1;
        devHalCfg->spi_config              = spi_config_t::EDGE_RISE;
        devHalCfg->reset_poke_fn           = reset_poke_fn;
        _adrv903x_device.common.devHalInfo = devHalCfg;

        CHECK_ADRV903x_FUNC(adi_adrv903x_HwOpen(&_adrv903x_device, &spiSettings));
        CHECK_ADRV903x_FUNC(adi_adrv903x_SpiVerify(&_adrv903x_device));
        adi_adrv903x_ExtractInitDataOutput_e checkExtractInitData =
            ADI_ADRV903X_EXTRACT_INIT_DATA_NOT_POPULATED;
        CHECK_ADRV903x_FUNC(adi_adrv903x_InitDataExtract(&_adrv903x_device,
            &_trxFileInfo.cpuProfile,
            &_initStructApiVersion,
            &_initStructArmVersion,
            &_initStructStreamVersion,
            &_deviceInitStruct,
            &_utilityInit,
            &checkExtractInitData));
        switch (checkExtractInitData) {
            case ADI_ADRV903X_EXTRACT_INIT_DATA_LEGACY_PROFILE_BIN:
                UHD_LOG_TRACE(
                    "ADRV9032", "Using the Default Init and PostMcsInit Structures");
                break;
            case ADI_ADRV903X_EXTRACT_INIT_DATA_POPULATED:
                UHD_LOG_TRACE(
                    "ADRV9032", "Using the Profile Init and PostMcsInit Structures");
                break;
            case ADI_ADRV903X_EXTRACT_INIT_DATA_NOT_POPULATED:
            default:
                throw uhd::runtime_error(
                    "PreMcsInit and/or PostMcsInit Data Structures Not Populated");
        }
        CHECK_ADRV903x_FUNC(adi_adrv903x_HwReset(&_adrv903x_device));

        // Pre-Multi-Chip Sync Initialization
        CHECK_ADRV903x_FUNC(adi_adrv903x_PreMcsInit(
            &_adrv903x_device, &_deviceInitStruct, &_trxFileInfo));
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_PreMcsInit_NonBroadcast(&_adrv903x_device, &_deviceInitStruct));

        // Start Multi-Chip Sync
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_MultichipSyncSet_v2(&_adrv903x_device, ADI_ADRV903X_MCS_START));
        return ret_code;
    }

    adi_adrv903x_ErrAction_e mcs_check() override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        uint32_t mcsStatus                = 0;
        adi_common_hal_Wait_us(&_adrv903x_device.common, 1000);
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_MultichipSyncStatusGet(&_adrv903x_device, &mcsStatus));
        if (mcsStatus != 1) {
            throw uhd::runtime_error("MCS Failed");
        }
        return ret_code;
    }

    adi_adrv903x_ErrAction_e mcs_end() override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_MultichipSyncSet_v2(&_adrv903x_device, ADI_ADRV903X_MCS_OFF));
        return ret_code;
    }

    adi_adrv903x_ErrAction_e post_mcs_init() override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        _init_cal_args                    = to_upper(_init_cal_args);
        if (_init_cal_args != "DEFAULT" && _init_cal_args != "ALL") {
            _utilityInit.initCals.calMask = _parse_init_cals(_init_cal_args);
        }
        CHECK_ADRV903x_FUNC(adi_adrv903x_PostMcsInit(&_adrv903x_device, &_utilityInit));
        CHECK_ADRV903x_FUNC(adi_adrv903x_InitCalsDetailedStatusGet_v2(
            &_adrv903x_device, &_initCalErrData));

        if (_initCalErrData.channel[0].errCode != 0U
            || _initCalErrData.channel[4].errCode != 0U) {
            throw uhd::runtime_error(
                "One or more init cals failed! Channel 0 error code: "
                + std::to_string(_initCalErrData.channel[0].errCode)
                + ". Channel 1 error code: "
                + std::to_string(_initCalErrData.channel[4].errCode) + ".");
        }

        _get_current_lo_sources(_rx_chan0_lo_source,
            _rx_chan1_lo_source,
            _tx_chan0_lo_source,
            _tx_chan1_lo_source);
        return ret_code;
    }

    adi_adrv903x_ErrAction_e jesd_serializer_reset() override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        CHECK_ADRV903x_FUNC(adi_adrv903x_SerializerReset(&_adrv903x_device));
        return ret_code;
    }

    adi_adrv903x_ErrAction_e jesd_deframer_link_state_enable(
        const adi_adrv903x_DeframerSel_e deframer_sel, bool enable) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        CHECK_ADRV903x_FUNC(adi_adrv903x_DeframerLinkStateSet(
            &_adrv903x_device, deframer_sel, enable ? 1 : 0));
        return ret_code;
    }

    adi_adrv903x_ErrAction_e jesd_get_framer_sync_mode(
        const adi_adrv903x_FramerSel_e framer_sel, uint8_t& mode) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_FramerSyncbModeGet(&_adrv903x_device, framer_sel, &mode));
        return ret_code;
    }

    adi_adrv903x_ErrAction_e jesd_get_framer_sync_status(
        const adi_adrv903x_FramerSel_e framer_sel, uint8_t& status) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_FramerSyncbStatusGet(&_adrv903x_device, framer_sel, &status));
        return ret_code;
    }

    adi_adrv903x_ErrAction_e jesd_set_framer_sync_mode(
        const adi_adrv903x_FramerSel_e framer_sel, uint8_t mode) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_FramerSyncbModeSet(&_adrv903x_device, framer_sel, mode));
        return ret_code;
    }

    adi_adrv903x_ErrAction_e jesd_set_framer_sync_status(
        const adi_adrv903x_FramerSel_e framer_sel, uint8_t status) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_FramerSyncbStatusSet(&_adrv903x_device, framer_sel, status));
        return ret_code;
    }

    adi_adrv903x_ErrAction_e jesd_framer_link_state_enable(
        const adi_adrv903x_FramerSel_e framer_sel, bool enable) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        CHECK_ADRV903x_FUNC(adi_adrv903x_FramerLinkStateSet(
            &_adrv903x_device, framer_sel, enable ? 1 : 0));
        return ret_code;
    }

    adi_adrv903x_ErrAction_e jesd_deframer_sysref_request_enable(
        const adi_adrv903x_DeframerSel_e deframer_sel, bool enable) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        CHECK_ADRV903x_FUNC(adi_adrv903x_DeframerSysrefCtrlSet(
            &_adrv903x_device, deframer_sel, enable ? 1 : 0));
        return ret_code;
    }

    adi_adrv903x_ErrAction_e jesd_framer_sysref_request_enable(
        const adi_adrv903x_FramerSel_e framer_sel, bool enable) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        CHECK_ADRV903x_FUNC(adi_adrv903x_FramerSysrefCtrlSet(
            &_adrv903x_device, framer_sel, enable ? 1 : 0));
        return ret_code;
    }

    adi_adrv903x_ErrAction_e jesd_run_deframer_init_cals(
        const adi_adrv903x_DeframerSel_e deframer_sel) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        adi_adrv903x_InitCals_t initCals  = {ADI_ADRV903X_IC_SERDES, 0U, 0U, 0U, 0U};
        adi_adrv903x_DeframerCfg_t deframerCfg;
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_DeframerCfgGet(&_adrv903x_device, deframer_sel, &deframerCfg));
        initCals.calMask       = ADI_ADRV903X_IC_SERDES;
        initCals.rxChannelMask = deframerCfg.deserializerLanesEnabled;
        CHECK_ADRV903x_FUNC(adi_adrv903x_InitCalsRun(&_adrv903x_device, &initCals));
        // Using timeout of 6000ms to match ADI example code.
        CHECK_ADRV903x_FUNC(adi_adrv903x_InitCalsWait(&_adrv903x_device, 6000));
        return ret_code;
    }

    adi_adrv903x_ErrAction_e jesd_get_framer_status(
        const adi_adrv903x_FramerSel_e framer_sel, uint8_t& status) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        adi_adrv903x_FramerStatus_t framerStatus;
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_FramerStatusGet(&_adrv903x_device, framer_sel, &framerStatus));
        status = framerStatus.status;
        return ret_code;
    }

    adi_adrv903x_ErrAction_e jesd_get_deframer_status(
        const adi_adrv903x_DeframerSel_e deframer_sel, uint8_t& status) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_DfrmLinkConditionGet(&_adrv903x_device, deframer_sel, &status));
        return ret_code;
    }

    adi_adrv903x_ErrAction_e jesd_deframer_error_clear(
        const adi_adrv903x_DeframerSel_e deframer_sel) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;

        for (int i = 0; i < 8; ++i) {
            if (_adrv903x_device.initExtract.jesdSetting
                    .deframerSetting[int(deframer_sel)]
                    .deserialLaneEnabled) {
                CHECK_ADRV903x_FUNC(adi_adrv903x_DfrmErrCounterReset(
                    &_adrv903x_device, deframer_sel, i, 0x7));
            }
        }

        CHECK_ADRV903x_FUNC(adi_adrv903x_DeframerErrorCtrl(
            &_adrv903x_device, deframer_sel, ADI_ADRV903X_SERDES_ALL_ERR_CLEAR));

        return ret_code;
    }

    adi_adrv903x_ErrAction_e set_rf_lo_frequency(uhd::direction_t dir,
        const size_t chan,
        const double freq,
        const bool timed_tuning) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;

        // Get the LO that we are setting the frequency for.
        adi_adrv903x_LoSel_e loToConfigure =
            dir == RX_DIRECTION ? (chan == 0 ? _rx_chan0_lo_source : _rx_chan1_lo_source)
                                : (chan == 0 ? _tx_chan0_lo_source : _tx_chan1_lo_source);

        if (timed_tuning) {
            uint32_t rxChanMask = 0x0;
            uint32_t txChanMask = 0x0;

            // Set rxChanMask and txChanMask for all channels using the LO.
            if (loToConfigure == _rx_chan0_lo_source) {
                rxChanMask |= ADI_ADRV903X_RX4;
            }
            if (loToConfigure == _rx_chan1_lo_source) {
                rxChanMask |= ADI_ADRV903X_RX0;
            }
            if (loToConfigure == _tx_chan0_lo_source) {
                txChanMask |= ADI_ADRV903X_TX4;
            }
            if (loToConfigure == _tx_chan1_lo_source) {
                txChanMask |= ADI_ADRV903X_TX0;
            }

            // Disable relevant channels via pin control.
            _channel_enable_fn(0,
                (txChanMask & ADI_ADRV903X_TX4 ? 0x0 : ADRV9032_CHAN_TX)
                    | (rxChanMask & ADI_ADRV903X_RX4 ? 0x0 : ADRV9032_CHAN_RX));
            _channel_enable_fn(1,
                (txChanMask & ADI_ADRV903X_TX0 ? 0x0 : ADRV9032_CHAN_TX)
                    | (rxChanMask & ADI_ADRV903X_RX0 ? 0x0 : ADRV9032_CHAN_RX));
            // If we are doing fast tuning, then we are ignoring the ADI-recommended
            // process for disabling tracking cals, ADI has said disabling the channels is
            // enough to stop the tracking cals from running while the LO is changing.
            // This prevents very bad tracking calibration calculations.
            adi_adrv903x_LoConfig_t loConfig;
            // Convert LoSel to LoName, they are both just enums where LO0 is 0 and LO1
            // is 1.
            loConfig.loName = (adi_adrv903x_LoName)loToConfigure;
            // freq is being clipped in adrv9032_manager, so we can assume this is a value
            // between 450MHz and 7.1GHz.
            loConfig.loFrequency_Hz = static_cast<uint64_t>(freq);
            // Feature is not implemented. Use 0 (ADI_ADRV903X_NCO_NO_OPTION_SELECTED) as
            // default. Users are responsible for changing Band NCOs if changing RF LO.
            loConfig.loConfigSel = ADI_ADRV903X_NCO_NO_OPTION_SELECTED;
            CHECK_ADRV903x_FUNC(
                adi_adrv903x_LoFrequencySetNoReads(&_adrv903x_device, &loConfig));

            if (txChanMask != 0 && (_tracking_cal_mask & ADI_ADRV903X_TC_TX_QEC_MASK)) {
                // Sleep in the FPGA to ensure time to allow ADRV9032 CPU to be ready.
                _sleep_fn(ADRV9032_CMD_SLEEP_TIME);
                adi_adrv903x_TxQecReset_t txQecResetConfig = {
                    txChanMask, // Tx Channel Mask
                    ADI_ADRV903X_TX_QEC_TRACKING_HARD_RESET};

                CHECK_ADRV903x_FUNC(
                    adi_adrv903x_TxQecResetNoReads(&_adrv903x_device, &txQecResetConfig));
            }
            _channel_enable_fn(0, ADRV9032_CHAN_ALL);
            _channel_enable_fn(1, ADRV9032_CHAN_ALL);

        } else {
            // Go through the full ADI-recommended process for changing LO frequency
            CHECK_ADRV903x_FUNC(_set_pll_frequency(freq,
                loToConfigure,
                _rx_chan0_lo_source,
                _rx_chan1_lo_source,
                _tx_chan0_lo_source,
                _tx_chan1_lo_source));
        }

        return ret_code;
    }

    adi_adrv903x_ErrAction_e get_rf_lo_frequency(
        uhd::direction_t dir, const size_t chan, double& freq) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        adi_adrv903x_LoConfigReadback_t loConfigReadback;
        adi_adrv903x_LoSel_e loToQuery;
        if (dir == RX_DIRECTION) {
            CHECK_ADRV903x_FUNC(adi_adrv903x_RxLoSourceGet(&_adrv903x_device,
                chan == 0 ? ADI_ADRV903X_RX4 : ADI_ADRV903X_RX0,
                &loToQuery));
        } else {
            CHECK_ADRV903x_FUNC(adi_adrv903x_TxLoSourceGet(&_adrv903x_device,
                chan == 0 ? ADI_ADRV903X_TX4 : ADI_ADRV903X_TX0,
                &loToQuery));
        }
        loConfigReadback.loName = (adi_adrv903x_LoName)loToQuery;
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_LoFrequencyGet(&_adrv903x_device, &loConfigReadback));
        freq = loConfigReadback.loFrequency_Hz;
        return ret_code;
    }

    adi_adrv903x_ErrAction_e set_tx_nco_frequency(
        const size_t chan, const double freq, const bool timed_tuning) override
    {
        adi_adrv903x_ErrAction_e ret_code            = ADI_ADRV903X_ERR_ACT_NONE;
        adi_adrv903x_TxNcoMixConfig_t txNcoMixConfig = {
            (uint8_t)(chan == 0 ? ADI_ADRV903X_TX4 : ADI_ADRV903X_TX0), // NCO Channel
            1, // Enable NCO
            0, // NCO Phase
            (int32_t)(freq / 1e3), // NCO Frequency in KHz
        };
        if (timed_tuning) {
            CHECK_ADRV903x_FUNC(
                adi_adrv903x_TxNcoShifterSetNoReads(&_adrv903x_device, &txNcoMixConfig));
        } else {
            CHECK_ADRV903x_FUNC(
                adi_adrv903x_TxNcoShifterSet(&_adrv903x_device, &txNcoMixConfig));
        }
        return ret_code;
    }

    adi_adrv903x_ErrAction_e get_tx_nco_frequency(
        const size_t chan, double& freq) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        adi_adrv903x_TxNcoMixConfigReadbackResp_t txNcoMixConfigReadback = {};
        txNcoMixConfigReadback.chanSelect = chan == 0 ? ADI_ADRV903X_TX4
                                                      : ADI_ADRV903X_TX0; // NCO Channel
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_TxNcoShifterGet(&_adrv903x_device, &txNcoMixConfigReadback));
        freq = txNcoMixConfigReadback.frequencyKhz * 1e3; // Convert to Hz
        return ret_code;
    }

    adi_adrv903x_ErrAction_e set_rx_nco_frequency(
        const size_t chan, const double freq, const bool timed_tuning) override
    {
        adi_adrv903x_ErrAction_e ret_code      = ADI_ADRV903X_ERR_ACT_NONE;
        adi_adrv903x_RxNcoConfig_t rxNcoConfig = {
            (uint8_t)(chan == 0 ? ADI_ADRV903X_RX4 : ADI_ADRV903X_RX0), // NCO Channel
            ADI_ADRV903X_DDC_BAND_0, // DDC Band (TODO: Check this parameter AzDo 3951884)
            1, // Enable NCO
            0, // NCO Phase
            (int32_t)(freq / 1e3), // NCO Frequency in KHz
        };
        if (timed_tuning) {
            CHECK_ADRV903x_FUNC(
                adi_adrv903x_RxNcoShifterSetNoReads(&_adrv903x_device, &rxNcoConfig));
        } else {
            CHECK_ADRV903x_FUNC(
                adi_adrv903x_RxNcoShifterSet(&_adrv903x_device, &rxNcoConfig));
        }
        return ret_code;
    }

    adi_adrv903x_ErrAction_e get_rx_nco_frequency(
        const size_t chan, double& freq) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        adi_adrv903x_RxNcoConfigReadbackResp_t rxNcoConfigReadback = {};
        rxNcoConfigReadback.chanSelect = chan == 0 ? ADI_ADRV903X_RX4
                                                   : ADI_ADRV903X_RX0; // NCO Channel
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_RxNcoShifterGet(&_adrv903x_device, &rxNcoConfigReadback));
        freq = rxNcoConfigReadback.frequencyKhz * 1e3; // Convert to Hz
        return ret_code;
    }

    adi_adrv903x_ErrAction_e set_chan_pll_source(
        uhd::direction_t dir, const size_t chan, const std::string& source) override
    {
        adi_adrv903x_ErrAction_e ret_code  = ADI_ADRV903X_ERR_ACT_NONE;
        adi_adrv903x_LoSel_e LoSourceToSet = source == "LO0" ? ADI_ADRV903X_LOSEL_LO0
                                                             : ADI_ADRV903X_LOSEL_LO1;
        adi_adrv903x_LoSel_e currentLoSource =
            dir == RX_DIRECTION ? (chan == 0 ? _rx_chan0_lo_source : _rx_chan1_lo_source)
                                : (chan == 0 ? _tx_chan0_lo_source : _tx_chan1_lo_source);
        // If the current LO source for the channel is already the desired value, then
        // just return.
        if (currentLoSource == LoSourceToSet) {
            return ret_code;
        }

        if (dir == RX_DIRECTION) {
            if (chan == 0) {
                _rx_chan0_lo_source = LoSourceToSet;
            } else {
                _rx_chan1_lo_source = LoSourceToSet;
            }
        } else {
            if (chan == 0) {
                _tx_chan0_lo_source = LoSourceToSet;
            } else {
                _tx_chan1_lo_source = LoSourceToSet;
            }
        }

        // adi_adrv903x_CfgPllToChanCtrl states the need to "strictly need to reprogram
        // the PLL's" as a post condition, get the current LO frequencies to reprogram
        // afterwards.
        adi_adrv903x_LoConfigReadback_t loConfigReadback;
        loConfigReadback.loName = ADI_ADRV903X_LO0;
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_LoFrequencyGet(&_adrv903x_device, &loConfigReadback));
        double lo0_freq         = loConfigReadback.loFrequency_Hz;
        loConfigReadback.loName = ADI_ADRV903X_LO1;
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_LoFrequencyGet(&_adrv903x_device, &loConfigReadback));
        double lo1_freq = loConfigReadback.loFrequency_Hz;

        // The function to configure the PLL source for the channel requires passing in
        // the source for all channels. It takes in an int rather than an
        // adi_adrv903x_LoSel_e, and PLL1 is actually 0 and PLL0 is actually 1 in the
        // function call.
        CHECK_ADRV903x_FUNC(adi_adrv903x_CfgPllToChanCtrl(&_adrv903x_device,
            _tx_chan1_lo_source == ADI_ADRV903X_LOSEL_LO0 ? 1 : 0,
            _tx_chan0_lo_source == ADI_ADRV903X_LOSEL_LO0 ? 1 : 0,
            _rx_chan1_lo_source == ADI_ADRV903X_LOSEL_LO0 ? 1 : 0,
            _rx_chan0_lo_source == ADI_ADRV903X_LOSEL_LO0 ? 1 : 0));
        CHECK_ADRV903x_FUNC(_set_pll_frequency(lo0_freq,
            ADI_ADRV903X_LOSEL_LO0,
            _rx_chan0_lo_source,
            _rx_chan1_lo_source,
            _tx_chan0_lo_source,
            _tx_chan1_lo_source));
        CHECK_ADRV903x_FUNC(_set_pll_frequency(lo1_freq,
            ADI_ADRV903X_LOSEL_LO1,
            _rx_chan0_lo_source,
            _rx_chan1_lo_source,
            _tx_chan0_lo_source,
            _tx_chan1_lo_source));
        return ret_code;
    }

    adi_adrv903x_ErrAction_e get_chan_pll_source(
        uhd::direction_t dir, const size_t chan, std::string& source) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        adi_adrv903x_LoSel_e pllSource;
        if (dir == RX_DIRECTION) {
            CHECK_ADRV903x_FUNC(adi_adrv903x_RxLoSourceGet(&_adrv903x_device,
                chan == 0 ? ADI_ADRV903X_RX4 : ADI_ADRV903X_RX0,
                &pllSource));
        } else {
            CHECK_ADRV903x_FUNC(adi_adrv903x_TxLoSourceGet(&_adrv903x_device,
                chan == 0 ? ADI_ADRV903X_TX4 : ADI_ADRV903X_TX0,
                &pllSource));
        }
        source = (pllSource == ADI_ADRV903X_LOSEL_LO0) ? "LO0" : "LO1";
        return ret_code;
    }

    adi_adrv903x_ErrAction_e get_pll_locked(
        uhd::direction_t dir, const size_t chan, bool& locked) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        locked                            = false;
        uint32_t pllLockStatus            = 0;
        // From adi_adrv903x_PllStatusGet declaration:
        // pllLockStatus bit 0 = CLK PLL Lock status
        // pllLockStatus bit 1 = RF0 PLL Lock status
        // pllLockStatus bit 2 = RF1 PLL status
        // pllLockStatus bit 3 = Serdes PLL Lock Status
        CHECK_ADRV903x_FUNC(adi_adrv903x_PllStatusGet(&_adrv903x_device, &pllLockStatus));
        std::string source_lo;
        get_chan_pll_source(dir, chan, source_lo);
        locked = source_lo == "LO0" ? ((pllLockStatus >> 1) & 0x1)
                                    : ((pllLockStatus >> 2) & 0x1);
        return ret_code;
    }

    adi_adrv903x_ErrAction_e initialize_tx_update_atten_config() override
    {
        adi_adrv903x_ErrAction_e ret_code                = ADI_ADRV903X_ERR_ACT_NONE;
        adi_adrv903x_TxAttenUpdateCfg_t txAttenUpdateCfg = {
            {
                ADI_ADRV903X_TXATTEN_UPD_SRC_S0, // Attenuation Update Source
                ADI_ADRV903X_GPIO_INVALID, // GPIO Pin
            },
            {
                ADI_ADRV903X_TXATTEN_UPD_TRG_NONE, // Attenuation Update Trigger
                ADI_ADRV903X_GPIO_INVALID, // GPIO Pin
            },
        };
        CHECK_ADRV903x_FUNC(adi_adrv903x_TxAttenUpdateCfgSet(
            &_adrv903x_device, (ADI_ADRV903X_TX4 | ADI_ADRV903X_TX0), &txAttenUpdateCfg));
        return ret_code;
    }

    adi_adrv903x_ErrAction_e set_tx_gain(const double gain, const size_t chan) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        adi_adrv903x_TxAtten_t txAtten;
        txAtten.txChannelMask = chan == 0 ? ADI_ADRV903X_TX4 : ADI_ADRV903X_TX0;
        txAtten.txAttenuation_mdB =
            static_cast<uint16_t>((MAX_TX_ATTENUATION - gain) * 1000);
        CHECK_ADRV903x_FUNC(adi_adrv903x_TxAttenSet(&_adrv903x_device, &txAtten, 1));
        return ret_code;
    }

    adi_adrv903x_ErrAction_e get_tx_gain(const size_t chan, double& gain) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        adi_adrv903x_TxChannels tx_chan   = (chan == 0) ? ADI_ADRV903X_TX4
                                                        : ADI_ADRV903X_TX0;
        adi_adrv903x_TxAtten_t txAttenChanConfig;
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_TxAttenGet(&_adrv903x_device, tx_chan, &txAttenChanConfig));
        double txAtten = txAttenChanConfig.txAttenuation_mdB / 1000.0;
        gain           = MAX_TX_ATTENUATION - txAtten;
        return ret_code;
    }

    adi_adrv903x_ErrAction_e set_rx_gain(const double gain, const size_t chan) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;

        adi_adrv903x_RxGain_t rxGainChanConfig;
        rxGainChanConfig.rxChannelMask = chan == 0 ? ADI_ADRV903X_RX4 : ADI_ADRV903X_RX0;
        // Current gain table only uses indexes 183 to 255 to go from 0 to 35 dB, it
        // currently doesn't have clean increments, but most increments are 0.5 dB steps,
        // this treats each step as 0.5 dB. We will want to finalize the gain table with
        // the hardware team. (AZDO 3263363)
        rxGainChanConfig.gainIndex = static_cast<uint8_t>((gain * 2) + 183);
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_RxGainSet(&_adrv903x_device, &rxGainChanConfig, 1));
        return ret_code;
    }

    adi_adrv903x_ErrAction_e get_rx_gain(const size_t chan, double& gain) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        adi_adrv903x_RxChannels rx_chan = chan == 0 ? ADI_ADRV903X_RX4 : ADI_ADRV903X_RX0;
        adi_adrv903x_RxGain_t rxGainChanConfig;
        bool agc_enabled;
        CHECK_ADRV903x_FUNC(get_rx_agc(chan, agc_enabled));
        if (agc_enabled) {
            CHECK_ADRV903x_FUNC(
                adi_adrv903x_RxGainGet(&_adrv903x_device, rx_chan, &rxGainChanConfig));
        } else {
            CHECK_ADRV903x_FUNC(
                adi_adrv903x_RxMgcGainGet(&_adrv903x_device, rx_chan, &rxGainChanConfig));
        }
        // See note in set function for index to gain mapping.
        gain = (rxGainChanConfig.gainIndex - 183) / 2;
        return ret_code;
    }

    // Note this is currently untested as it is not used by any devices.
    adi_adrv903x_ErrAction_e set_rx_agc(const bool enable, const size_t chan) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        adi_adrv903x_RxGainCtrlModeCfg_t rxGainCtrlModeCfg;
        rxGainCtrlModeCfg.rxChannelMask = chan == 0 ? ADI_ADRV903X_RX0 : ADI_ADRV903X_RX4;
        rxGainCtrlModeCfg.gainCtrlMode  = enable ? ADI_ADRV903X_AGC : ADI_ADRV903X_MGC;
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_RxGainCtrlModeSet(&_adrv903x_device, &rxGainCtrlModeCfg, 1));
        return ret_code;
    }

    // Note this is currently untested as it is not used by any devices.
    adi_adrv903x_ErrAction_e get_rx_agc(const size_t chan, bool& enable) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        adi_adrv903x_RxChannels rx_chan = chan == 0 ? ADI_ADRV903X_RX0 : ADI_ADRV903X_RX4;
        adi_adrv903x_RxGainCtrlMode_e agc_mode;
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_RxGainCtrlModeGet(&_adrv903x_device, rx_chan, &agc_mode));
        enable = (agc_mode == ADI_ADRV903X_AGC);
        return ret_code;
    }

    adi_adrv903x_ErrAction_e set_pin_control(
        const bool enable, const uint32_t rxChanMask, const uint32_t txChanMask) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;

        adi_adrv903x_RadioCtrlModeCfg_t radioCtrlSpiModeCfg = {
            {enable ? ADI_ADRV903X_TX_EN_PIN_MODE : ADI_ADRV903X_TX_EN_SPI_MODE,
                txChanMask},
            {enable ? ADI_ADRV903X_RX_EN_PIN_MODE : ADI_ADRV903X_RX_EN_SPI_MODE,
                rxChanMask},
            {ADI_ADRV903X_ORX_EN_SPI_MODE, 0x0}};

        // For all channels in rxChanMaskPinMode and txChanMaskPinMode, set the pin mode
        // to SPI mode.
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_RadioCtrlCfgSet(&_adrv903x_device, &radioCtrlSpiModeCfg));
        return ret_code;
    }

    adi_adrv903x_ErrAction_e enable_txrx(
        const bool enable, const uint32_t rxChanMask, const uint32_t txChanMask) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;

        CHECK_ADRV903x_FUNC(adi_adrv903x_RxTxEnableSet(&_adrv903x_device,
            0x0,
            0x0,
            rxChanMask,
            enable ? rxChanMask : 0x0,
            txChanMask,
            enable ? txChanMask : 0x0));
        return ret_code;
    }

    adi_adrv903x_ErrAction_e enable_tracking_cals() override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;

        // Enable tracking cals on all channels
        adi_adrv903x_ChannelTrackingCals_t trackingChannelMask;
        trackingChannelMask.rxChannel  = ADI_ADRV903X_RX4 | ADI_ADRV903X_RX0;
        trackingChannelMask.txChannel  = ADI_ADRV903X_TX4 | ADI_ADRV903X_TX0;
        trackingChannelMask.orxChannel = 0x0;
        trackingChannelMask.laneSerdes = 0x0;

        // Enable all Tracking Cals which were set for all channels in rxChanMask and
        // txChanMask one-by-one.
        for (uint32_t trackingCal = (uint32_t)ADI_ADRV903X_TC_RX_QEC_MASK;
             trackingCal <= (uint32_t)ADI_ADRV903X_TC_RXSPUR_MASK;
             trackingCal <<= 1) {
            if (_tracking_cal_mask & trackingCal) {
                CHECK_ADRV903x_FUNC(
                    adi_adrv903x_TrackingCalsEnableSet_v2(&_adrv903x_device,
                        (adi_adrv903x_TrackingCalibrationMask_e)trackingCal,
                        &trackingChannelMask,
                        ADI_ADRV903X_TRACKING_CAL_ENABLE));
            }
        }

        return ret_code;
    }

    adi_adrv903x_ErrAction_e get_temperature(int16_t& temperature) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        uint16_t sensors_enabled_mask;
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_TemperatureEnableGet(&_adrv903x_device, &sensors_enabled_mask));
        adi_adrv903x_DevTempData_t tempData;
        CHECK_ADRV903x_FUNC(adi_adrv903x_TemperatureGet(
            &_adrv903x_device, sensors_enabled_mask, &tempData));
        temperature = tempData.tempDegreesCelsiusAvg;
        return ret_code;
    }

    adi_adrv903x_ErrAction_e get_firmware_version(std::string& version) override
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        adi_adrv903x_CpuFwVersion_t cpuFwVersion;
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_CpuFwVersionGet(&_adrv903x_device, &cpuFwVersion));
        version = std::to_string(cpuFwVersion.commVer.majorVer) + "."
                  + std::to_string(cpuFwVersion.commVer.minorVer) + "."
                  + std::to_string(cpuFwVersion.commVer.maintenanceVer) + "."
                  + std::to_string(cpuFwVersion.commVer.buildVer);
        return ret_code;
    }

private:
    // Parse a string of init cal names separated by '|' into an init cal bitmask.
    // Surrounding whitespace around each token is ignored and empty tokens are skipped.
    static adi_adrv903x_InitCalibrations_t _parse_init_cals(
        const std::string& init_cals_str)
    {
        static const std::unordered_map<std::string, uint64_t> cal_name_map = {
            {"RX_DC_OFFSET", ADI_ADRV903X_IC_RX_DC_OFFSET},
            {"ADC_RX", ADI_ADRV903X_IC_ADC_RX},
            {"ADC_ORX", ADI_ADRV903X_IC_ADC_ORX},
            {"ADC_TXLB", ADI_ADRV903X_IC_ADC_TXLB},
            {"TXDAC", ADI_ADRV903X_IC_TXDAC},
            {"TXBBF", ADI_ADRV903X_IC_TXBBF},
            {"TXLB_FILTER", ADI_ADRV903X_IC_TXLB_FILTER},
            {"TXLB_PATH_DLY", ADI_ADRV903X_IC_TXLB_PATH_DLY},
            {"TX_ATTEN_CAL", ADI_ADRV903X_IC_TX_ATTEN_CAL},
            {"HRM", ADI_ADRV903X_IC_HRM},
            {"TXQEC", ADI_ADRV903X_IC_TXQEC},
            {"TXLOL", ADI_ADRV903X_IC_TXLOL},
        };

        if (init_cals_str == "OFF") {
            return 0;
        }

        return parse_cal_mask(init_cals_str, cal_name_map);
    }

    static uint32_t _parse_tracking_cals(const std::string& tracking_cals_str)
    {
        static const std::unordered_map<std::string, uint64_t> cal_name_map = {
            {"RX_QEC", ADI_ADRV903X_TC_RX_QEC_MASK},
            {"RX_ADC", ADI_ADRV903X_TC_RX_ADC_MASK},
            {"RXSPUR", ADI_ADRV903X_TC_RXSPUR_MASK},
            {"TX_QEC", ADI_ADRV903X_TC_TX_QEC_MASK},
            {"TX_LB_ADC", ADI_ADRV903X_TC_TX_LB_ADC_MASK},
        };

        if (tracking_cals_str == "DEFAULT") {
            return ADI_ADRV903X_TC_RX_QEC_MASK | ADI_ADRV903X_TC_RX_ADC_MASK
                   | ADI_ADRV903X_TC_RXSPUR_MASK | ADI_ADRV903X_TC_TX_LB_ADC_MASK;
        }
        if (tracking_cals_str == "ALL") {
            return ADI_ADRV903X_TC_RX_QEC_MASK | ADI_ADRV903X_TC_RX_ADC_MASK
                   | ADI_ADRV903X_TC_RXSPUR_MASK | ADI_ADRV903X_TC_TX_QEC_MASK
                   | ADI_ADRV903X_TC_TX_LB_ADC_MASK;
        } else if (tracking_cals_str == "OFF") {
            return 0;
        }

        return static_cast<uint32_t>(parse_cal_mask(tracking_cals_str, cal_name_map));
    }

    adi_adrv903x_ErrAction_e _get_channel_pin_masks(
        uint32_t& rxChanMaskPinMode, uint32_t& txChanMaskPinMode)
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        adi_adrv903x_RadioCtrlModeCfg_t radioCtrlModeCfgChan4;
        adi_adrv903x_RadioCtrlModeCfg_t radioCtrlModeCfgChan0;
        CHECK_ADRV903x_FUNC(adi_adrv903x_RadioCtrlCfgGet(&_adrv903x_device,
            ADI_ADRV903X_RX4,
            ADI_ADRV903X_TX4,
            &radioCtrlModeCfgChan4));
        CHECK_ADRV903x_FUNC(adi_adrv903x_RadioCtrlCfgGet(&_adrv903x_device,
            ADI_ADRV903X_RX0,
            ADI_ADRV903X_TX0,
            &radioCtrlModeCfgChan0));

        if (radioCtrlModeCfgChan4.rxRadioCtrlModeCfg.rxEnableMode
            == ADI_ADRV903X_RX_EN_PIN_MODE) {
            rxChanMaskPinMode |= ADI_ADRV903X_RX4;
        }
        if (radioCtrlModeCfgChan0.rxRadioCtrlModeCfg.rxEnableMode
            == ADI_ADRV903X_RX_EN_PIN_MODE) {
            rxChanMaskPinMode |= ADI_ADRV903X_RX0;
        }
        if (radioCtrlModeCfgChan4.txRadioCtrlModeCfg.txEnableMode
            == ADI_ADRV903X_TX_EN_PIN_MODE) {
            txChanMaskPinMode |= ADI_ADRV903X_TX4;
        }
        if (radioCtrlModeCfgChan0.txRadioCtrlModeCfg.txEnableMode
            == ADI_ADRV903X_TX_EN_PIN_MODE) {
            txChanMaskPinMode |= ADI_ADRV903X_TX0;
        }
        return ret_code;
    }

    adi_adrv903x_ErrAction_e _get_current_lo_sources(adi_adrv903x_LoSel_e& rxChan0,
        adi_adrv903x_LoSel_e& rxChan1,
        adi_adrv903x_LoSel_e& txChan0,
        adi_adrv903x_LoSel_e& txChan1)
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_RxLoSourceGet(&_adrv903x_device, ADI_ADRV903X_RX4, &rxChan0));
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_RxLoSourceGet(&_adrv903x_device, ADI_ADRV903X_RX0, &rxChan1));
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_TxLoSourceGet(&_adrv903x_device, ADI_ADRV903X_TX4, &txChan0));
        CHECK_ADRV903x_FUNC(
            adi_adrv903x_TxLoSourceGet(&_adrv903x_device, ADI_ADRV903X_TX0, &txChan1));
        return ret_code;
    }

    adi_adrv903x_ErrAction_e _set_pll_frequency(const double freq,
        adi_adrv903x_LoSel_e loToConfigure,
        const adi_adrv903x_LoSel_e& rx_chan0,
        const adi_adrv903x_LoSel_e& rx_chan1,
        const adi_adrv903x_LoSel_e& tx_chan0,
        const adi_adrv903x_LoSel_e& tx_chan1)
    {
        adi_adrv903x_ErrAction_e ret_code = ADI_ADRV903X_ERR_ACT_NONE;
        uint32_t rxChanMask               = 0x0;
        uint32_t txChanMask               = 0x0;

        // Set rxChanMask and txChanMask for all channels using the LO.
        if (loToConfigure == rx_chan0) {
            rxChanMask |= ADI_ADRV903X_RX4;
        }
        if (loToConfigure == rx_chan1) {
            rxChanMask |= ADI_ADRV903X_RX0;
        }
        if (loToConfigure == tx_chan0) {
            txChanMask |= ADI_ADRV903X_TX4;
        }
        if (loToConfigure == tx_chan1) {
            txChanMask |= ADI_ADRV903X_TX0;
        }

        adi_adrv903x_ChannelTrackingCals_t trackingChannelMask;
        trackingChannelMask.rxChannel  = rxChanMask;
        trackingChannelMask.txChannel  = txChanMask;
        trackingChannelMask.orxChannel = 0x0;
        trackingChannelMask.laneSerdes = 0x0;
        // Disable all tracking cals which are enabled, one-by-one
        for (uint32_t trackingCal = (uint32_t)ADI_ADRV903X_TC_RX_QEC_MASK;
             trackingCal <= (uint32_t)ADI_ADRV903X_TC_RXSPUR_MASK;
             trackingCal <<= 1) {
            if (_tracking_cal_mask & trackingCal)
                CHECK_ADRV903x_FUNC(
                    adi_adrv903x_TrackingCalsEnableSet_v2(&_adrv903x_device,
                        (adi_adrv903x_TrackingCalibrationMask_e)trackingCal,
                        &trackingChannelMask,
                        ADI_ADRV903X_TRACKING_CAL_DISABLE));
        }

        // Disable relevant channels via pin control.
        _channel_enable_fn(0,
            (txChanMask & ADI_ADRV903X_TX4 ? 0x0 : ADRV9032_CHAN_TX)
                | (rxChanMask & ADI_ADRV903X_RX4 ? 0x0 : ADRV9032_CHAN_RX));
        _channel_enable_fn(1,
            (txChanMask & ADI_ADRV903X_TX0 ? 0x0 : ADRV9032_CHAN_TX)
                | (rxChanMask & ADI_ADRV903X_RX0 ? 0x0 : ADRV9032_CHAN_RX));

        // Set the frequency for the LO.
        adi_adrv903x_LoConfig_t loConfig;
        // Convert LoSel to LoName, they are both just enums where LO0 is 0 and LO1 is 1.
        loConfig.loName = (adi_adrv903x_LoName)loToConfigure;
        // freq is being clipped in adrv9032_manager, so we can assume this is a value
        // between 450MHz and 7.1GHz.
        loConfig.loFrequency_Hz = static_cast<uint64_t>(freq);
        // Feature is not implemented. Use 0 (ADI_ADRV903X_NCO_NO_OPTION_SELECTED) as
        // default. Users are responsible for changing Band NCOs if changing RF LO.
        loConfig.loConfigSel = ADI_ADRV903X_NCO_NO_OPTION_SELECTED;
        CHECK_ADRV903x_FUNC(adi_adrv903x_LoFrequencySet(&_adrv903x_device, &loConfig));

        if (txChanMask != 0 && (_tracking_cal_mask & ADI_ADRV903X_TC_TX_QEC_MASK)) {
            adi_adrv903x_TxQecReset_t txQecResetConfig = {txChanMask, // Tx Channel Mask
                ADI_ADRV903X_TX_QEC_TRACKING_HARD_RESET};

            CHECK_ADRV903x_FUNC(
                adi_adrv903x_TxQecReset(&_adrv903x_device, &txQecResetConfig));
        }

        // Enable all Tracking Cals which were set for all channels in rxChanMask and
        // txChanMask one-by-one.
        for (uint32_t trackingCal = (uint32_t)ADI_ADRV903X_TC_RX_QEC_MASK;
             trackingCal <= (uint32_t)ADI_ADRV903X_TC_RXSPUR_MASK;
             trackingCal <<= 1) {
            if (_tracking_cal_mask & trackingCal) {
                CHECK_ADRV903x_FUNC(
                    adi_adrv903x_TrackingCalsEnableSet_v2(&_adrv903x_device,
                        (adi_adrv903x_TrackingCalibrationMask_e)trackingCal,
                        &trackingChannelMask,
                        ADI_ADRV903X_TRACKING_CAL_ENABLE));
            }
        }

        // Bring all channels back to an enabled state via pin control.
        _channel_enable_fn(0, ADRV9032_CHAN_ALL);
        _channel_enable_fn(1, ADRV9032_CHAN_ALL);

        return ret_code;
    }

    // Device Data Structure for the ADRV903x
    adi_adrv903x_Device_t _adrv903x_device            = {};
    adi_adrv903x_InitCalErrData_t _initCalErrData     = {};
    adi_adrv903x_TrxFileInfo_t _trxFileInfo           = {};
    adi_common_ErrData_t _errData                     = {};
    adi_adrv903x_Version_t _initStructApiVersion      = {};
    adi_adrv903x_CpuFwVersion_t _initStructArmVersion = {};
    adi_adrv903x_Version_t _initStructStreamVersion   = {};
    adi_adrv903x_PostMcsInit_t _utilityInit           = {};
    adi_adrv903x_Init_t _deviceInitStruct             = {};

    adi_adrv903x_LoSel_e _rx_chan0_lo_source;
    adi_adrv903x_LoSel_e _rx_chan1_lo_source;
    adi_adrv903x_LoSel_e _tx_chan0_lo_source;
    adi_adrv903x_LoSel_e _tx_chan1_lo_source;

    std::function<void(size_t, uint32_t)> _channel_enable_fn;
    std::function<void(const uhd::time_spec_t&)> _sleep_fn;

    std::string _init_cal_args;
    uint32_t _tracking_cal_mask;
};

adrv9032_ctrl::sptr adrv9032_ctrl::make(
    std::function<void(size_t, uint32_t)> channel_enable_fn,
    std::function<void(const uhd::time_spec_t&)> sleep_fn,
    std::string init_cal_args,
    std::string tracking_cal_args)
{
    return std::make_shared<adrv9032_ctrl_impl>(
        channel_enable_fn, sleep_fn, init_cal_args, tracking_cal_args);
}

}} // namespace uhd::usrp

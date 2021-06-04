//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "mpm/rfdc/rfdc_ctrl.hpp"
#include <mpm/exception.hpp>
#include <set>

#define BUS_NAME "platform"

namespace mpm { namespace rfdc {

rfdc_ctrl::rfdc_ctrl()
{
    rfdc_inst_ptr         = &rfdc_inst;
    rfdc_inst_ptr->device = nullptr;
    rfdc_inst_ptr->io     = nullptr;

    // Populates default values to the struct
    XRFdc_MultiConverter_Init(&rfdc_dac_sync_config, nullptr, nullptr);
    XRFdc_MultiConverter_Init(&rfdc_adc_sync_config, nullptr, nullptr);
}

rfdc_ctrl::~rfdc_ctrl()
{
    if (rfdc_inst_ptr && rfdc_inst_ptr->device) {
        metal_device_close(rfdc_inst_ptr->device);
    }
    if (metal_init_complete) {
        metal_finish();
    }
}

void rfdc_ctrl::init(uint16_t rfdc_device_id)
{
    XRFdc_Config* config_ptr;
    char device_name[NAME_MAX];

    this->rfdc_device_id = rfdc_device_id;

    struct metal_init_params init_param = METAL_INIT_DEFAULTS;

    if (metal_init(&init_param)) {
        throw mpm::runtime_error("Failed to run metal initialization for rfdc.\n");
    }
    metal_init_complete = true;

    /* Get configuration data for te RFdc device */
    /* config_ptr is an entry of the XRFdc_ConfigTablePtr array managed by xrfdc_sinit.c
     * This memory is not explicitly freed because we do not have access to
     * XRFdc_ConfigTablePtr in this scope. */
    config_ptr = XRFdc_LookupConfig(rfdc_device_id);
    if (config_ptr == NULL) {
        throw mpm::runtime_error("Rfdc config lookup failed.\n");
    }

    /* Initializes the controller with loaded config information */
    if (XRFdc_CfgInitialize(rfdc_inst_ptr, config_ptr) != XRFDC_SUCCESS) {
        throw mpm::runtime_error("Rfdc controller init failed.\n");
    }

    /* Set UpdateMixerScale into valid state. For some reason the
       XRFdc config functions do not set this value. It will be
       overwritten when XRFdc_SetMixerSettings is called next. */
    rfdc_inst_ptr->UpdateMixerScale = 0;

    if (XRFdc_GetDeviceNameByDeviceId(device_name, rfdc_device_id) < 0) {
        throw mpm::runtime_error("Failed to find rfdc device with device id \n");
    }

    if (metal_device_open(BUS_NAME, device_name, &rfdc_inst_ptr->device)) {
        throw mpm::runtime_error("Failed to open device.\n");
    }

    /* Map RFDC device IO region. 0 is the IO region index on the device. */
    rfdc_inst_ptr->io = metal_device_io_region(rfdc_inst_ptr->device, 0);
    if (!rfdc_inst_ptr->io) {
        throw mpm::runtime_error("Failed to map RFDC regio\n");
    }

    /* Set all gain threshold stickies to manual clear mode */
    for (int tile_id = 0; tile_id <= XRFDC_TILE_ID_MAX; tile_id++) {
        for (int block_id = 0; block_id <= XRFDC_BLOCK_ID_MAX; block_id++) {
            for (int threshold_id = 0; threshold_id < THRESHOLDS_PER_BLOCK;
                 threshold_id++) {
                threshold_clr_modes[tile_id][block_id][threshold_id] =
                    THRESHOLD_CLRMD_UNKNOWN;
            }
            set_threshold_clr_mode(
                tile_id, block_id, THRESHOLD_BOTH, THRESHOLD_CLRMD_MANUAL);
        }
    }
}

bool rfdc_ctrl::startup_tile(int tile_id, bool is_dac)
{
    return XRFdc_StartUp(rfdc_inst_ptr, is_dac, tile_id) == XRFDC_SUCCESS;
}

bool rfdc_ctrl::shutdown_tile(int tile_id, bool is_dac)
{
    return XRFdc_Shutdown(rfdc_inst_ptr, is_dac, tile_id) == XRFDC_SUCCESS;
}

bool rfdc_ctrl::reset_tile(int tile_id, bool is_dac)
{
    return XRFdc_Reset(rfdc_inst_ptr, is_dac, tile_id) == XRFDC_SUCCESS;
}

bool rfdc_ctrl::trigger_update_event(
    uint32_t tile_id, uint32_t block_id, bool is_dac, event_type_options event_type)
{
    return XRFdc_UpdateEvent(rfdc_inst_ptr, is_dac, tile_id, block_id, event_type)
           == XRFDC_SUCCESS;
}

bool rfdc_ctrl::reset_mixer_settings(uint32_t tile_id, uint32_t block_id, bool is_dac)
{
    XRFdc_Mixer_Settings mixer_settings;

    mixer_settings.Freq           = 200;
    mixer_settings.PhaseOffset    = 0;
    mixer_settings.EventSource    = XRFDC_EVNT_SRC_SYSREF;
    mixer_settings.CoarseMixFreq  = 16;
    mixer_settings.MixerMode      = is_dac ? MIXER_MODE_C2R : MIXER_MODE_R2C;
    mixer_settings.FineMixerScale = 0;
    mixer_settings.MixerType      = XRFDC_MIXER_TYPE_FINE;

    return (XRFdc_SetMixerSettings(
                rfdc_inst_ptr, is_dac, tile_id, block_id, &mixer_settings)
               == XRFDC_SUCCESS);
}

bool rfdc_ctrl::set_gain_enable(
    uint32_t tile_id, uint32_t block_id, bool is_dac, bool enable)
{
    XRFdc_QMC_Settings qmc_settings;

    // Get current QMC settings for the values that will not be changed
    if (XRFdc_GetQMCSettings(rfdc_inst_ptr, is_dac, tile_id, block_id, &qmc_settings)
        != XRFDC_SUCCESS) {
        return false;
    }

    qmc_settings.EnableGain = enable;
    // Update the setting on a SYSREF trigger
    qmc_settings.EventSource = XRFDC_EVNT_SRC_SYSREF;

    return (XRFdc_SetQMCSettings(rfdc_inst_ptr, is_dac, tile_id, block_id, &qmc_settings)
               == XRFDC_SUCCESS);
}

bool rfdc_ctrl::set_gain(uint32_t tile_id, uint32_t block_id, bool is_dac, double gain)
{
    XRFdc_QMC_Settings qmc_settings;

    // Get current QMC settings for the values that will not be changed
    if (XRFdc_GetQMCSettings(rfdc_inst_ptr, is_dac, tile_id, block_id, &qmc_settings)
        != XRFDC_SUCCESS) {
        return false;
    }

    qmc_settings.EnableGain           = 1;
    qmc_settings.GainCorrectionFactor = gain;
    // Update the setting on a SYSREF trigger
    qmc_settings.EventSource = XRFDC_EVNT_SRC_SYSREF;

    return (XRFdc_SetQMCSettings(rfdc_inst_ptr, is_dac, tile_id, block_id, &qmc_settings)
               == XRFDC_SUCCESS);
}

bool rfdc_ctrl::set_threshold_settings(uint32_t tile_id,
    uint32_t block_id,
    threshold_id_options threshold_id,
    threshold_mode_options mode,
    uint32_t average_val,
    uint32_t under_val,
    uint32_t over_val)
{
    XRFdc_Threshold_Settings threshold_settings;

    // Get current threshold settings for the values that will not be changed
    if (XRFdc_GetThresholdSettings(rfdc_inst_ptr, tile_id, block_id, &threshold_settings)
        != XRFDC_SUCCESS) {
        return false;
    }
    threshold_settings.UpdateThreshold = threshold_id;

    // Index 0 and 1 of the threshold settings struct correspond to threshold 0 and 1.
    if (threshold_id == THRESHOLD_0 || threshold_id == THRESHOLD_BOTH) {
        threshold_settings.ThresholdMode[0]     = mode;
        threshold_settings.ThresholdAvgVal[0]   = average_val;
        threshold_settings.ThresholdUnderVal[0] = under_val;
        threshold_settings.ThresholdOverVal[0]  = over_val;
    }
    if (threshold_id == THRESHOLD_1 || threshold_id == THRESHOLD_BOTH) {
        threshold_settings.ThresholdMode[1]     = mode;
        threshold_settings.ThresholdAvgVal[1]   = average_val;
        threshold_settings.ThresholdUnderVal[1] = under_val;
        threshold_settings.ThresholdOverVal[1]  = over_val;
    }

    return XRFdc_SetThresholdSettings(
               rfdc_inst_ptr, tile_id, block_id, &threshold_settings)
           == XRFDC_SUCCESS;
}

bool rfdc_ctrl::clear_threshold_sticky(
    uint32_t tile_id, uint32_t block_id, threshold_id_options threshold_id)
{
    bool result;
    threshold_clr_mode_options old_clear_mode_0 = THRESHOLD_CLRMD_UNKNOWN,
                               old_clear_mode_1 = THRESHOLD_CLRMD_UNKNOWN;

    // Check current threshold clear mode
    old_clear_mode_0 = get_threshold_clr_mode(tile_id, block_id, THRESHOLD_0);
    old_clear_mode_1 = get_threshold_clr_mode(tile_id, block_id, THRESHOLD_1);

    // Set the clear mode to manual
    if (!set_threshold_clr_mode(
            tile_id, block_id, threshold_id, THRESHOLD_CLRMD_MANUAL)) {
        return false;
    }

    // Clear the sticky
    // Do not return on a failure as the clear mode still needs to be returned to the
    // previous value.
    result = (XRFdc_ThresholdStickyClear(rfdc_inst_ptr, tile_id, block_id, threshold_id)
              == XRFDC_SUCCESS);

    // Set the threshold clear mode back to the original setting
    // If the old setting is the same or UNKNOWN this will do nothing.
    result = result
             && set_threshold_clr_mode(tile_id, block_id, THRESHOLD_0, old_clear_mode_0);
    result = result
             && set_threshold_clr_mode(tile_id, block_id, THRESHOLD_1, old_clear_mode_1);
    return result;
}

bool rfdc_ctrl::set_threshold_clr_mode(uint32_t tile_id,
    uint32_t block_id,
    threshold_id_options threshold_id,
    threshold_clr_mode_options clear_mode)
{
    bool result;
    bool mode_matches         = false;
    uint32_t old_clear_mode_0 = THRESHOLD_CLRMD_UNKNOWN,
             old_clear_mode_1 = THRESHOLD_CLRMD_UNKNOWN;

    if ((tile_id > XRFDC_TILE_ID_MAX) || (block_id > XRFDC_BLOCK_ID_MAX)) {
        return false;
    }
    // Do not change the clear mode to UNKNOWN
    if (clear_mode == THRESHOLD_CLRMD_UNKNOWN) {
        return false;
    }

    // Check current threshold clear mode
    switch (threshold_id) {
        case THRESHOLD_0:
            old_clear_mode_0 = get_threshold_clr_mode(tile_id, block_id, THRESHOLD_0);
            mode_matches     = (old_clear_mode_0 == clear_mode);
            break;
        case THRESHOLD_1:
            old_clear_mode_1 = get_threshold_clr_mode(tile_id, block_id, THRESHOLD_1);
            mode_matches     = (old_clear_mode_1 == clear_mode);
            break;
        case THRESHOLD_BOTH:
            old_clear_mode_0 = get_threshold_clr_mode(tile_id, block_id, THRESHOLD_0);
            old_clear_mode_1 = get_threshold_clr_mode(tile_id, block_id, THRESHOLD_1);
            mode_matches =
                ((old_clear_mode_0 == clear_mode) && (old_clear_mode_1 == clear_mode));
            break;
    }
    // Do not change the clear mode if the new value matches the existing value
    if (mode_matches) {
        return true;
    }

    result = (XRFdc_SetThresholdClrMode(
                  rfdc_inst_ptr, tile_id, block_id, threshold_id, clear_mode)
              == XRFDC_SUCCESS);
    // If the setting was not successful, save the clear mode as unknown
    if (!result) {
        clear_mode = THRESHOLD_CLRMD_UNKNOWN;
    }

    // Set the new threshold clear mode
    switch (threshold_id) {
        case THRESHOLD_0:
            threshold_clr_modes[tile_id][block_id][0] = clear_mode;
            break;
        case THRESHOLD_1:
            threshold_clr_modes[tile_id][block_id][1] = clear_mode;
            break;
        case THRESHOLD_BOTH:
            threshold_clr_modes[tile_id][block_id][0] = clear_mode;
            threshold_clr_modes[tile_id][block_id][1] = clear_mode;
            break;
    }
    return result;
}

rfdc_ctrl::threshold_clr_mode_options rfdc_ctrl::get_threshold_clr_mode(
    uint32_t tile_id, uint32_t block_id, threshold_id_options threshold_id)
{
    int threshold_index;

    // The XRFdc Threshold ID values (1-2) do not match the array indexes (0-1)
    if (threshold_id == THRESHOLD_0) {
        threshold_index = 0;
    } else if (threshold_id == THRESHOLD_1) {
        threshold_index = 1;
    }
    // An invalid Threshold ID was given
    else {
        return THRESHOLD_CLRMD_UNKNOWN;
    }
    if ((tile_id > XRFDC_TILE_ID_MAX) || (block_id > XRFDC_BLOCK_ID_MAX)
        || (threshold_index >= THRESHOLDS_PER_BLOCK)) {
        return THRESHOLD_CLRMD_UNKNOWN;
    }

    return threshold_clr_modes[tile_id][block_id][threshold_index];
}

bool rfdc_ctrl::set_decoder_mode(
    uint32_t tile_id, uint32_t block_id, decoder_mode_options decoder_mode)
{
    return XRFdc_SetDecoderMode(rfdc_inst_ptr, tile_id, block_id, decoder_mode)
           == XRFDC_SUCCESS;
}

bool rfdc_ctrl::reset_nco_phase(uint32_t tile_id, uint32_t block_id, bool is_dac)
{
    XRFdc_Mixer_Settings mixer_settings;

    // Get current mixer settings for the values that will not be changed
    if (XRFdc_GetMixerSettings(rfdc_inst_ptr, is_dac, tile_id, block_id, &mixer_settings)
        != XRFDC_SUCCESS) {
        return false;
    }

    // Reset the phase on a SYSREF trigger
    mixer_settings.EventSource = XRFDC_EVNT_SRC_SYSREF;

    // Set the mixer settings to set the NCO event source
    if (XRFdc_SetMixerSettings(rfdc_inst_ptr, is_dac, tile_id, block_id, &mixer_settings)
        != XRFDC_SUCCESS) {
        return false;
    }

    return (XRFdc_ResetNCOPhase(rfdc_inst_ptr, is_dac, tile_id, block_id)
               == XRFDC_SUCCESS);
}

bool rfdc_ctrl::set_nco_freq(
    uint32_t tile_id, uint32_t block_id, bool is_dac, double freq)
{
    XRFdc_Mixer_Settings mixer_settings;

    // Get current mixer settings for the values that will not be changed
    if (XRFdc_GetMixerSettings(rfdc_inst_ptr, is_dac, tile_id, block_id, &mixer_settings)
        != XRFDC_SUCCESS) {
        return false;
    }

    // The XRFdc API expects the NCO frequency in MHz
    mixer_settings.Freq = freq / 1e6;
    // Only the fine mixer uses an NCO to shift the data frequency
    mixer_settings.MixerType = XRFDC_MIXER_TYPE_FINE;
    // Update the setting on a tile-wide event trigger
    mixer_settings.EventSource = XRFDC_EVNT_SRC_TILE;

    return (XRFdc_SetMixerSettings(
                rfdc_inst_ptr, is_dac, tile_id, block_id, &mixer_settings)
               == XRFDC_SUCCESS)
               && trigger_update_event(tile_id, block_id, is_dac, MIXER_EVENT);
}

double rfdc_ctrl::get_nco_freq(uint32_t tile_id, uint32_t block_id, bool is_dac)
{
    XRFdc_Mixer_Settings mixer_settings;

    if (XRFdc_GetMixerSettings(rfdc_inst_ptr, is_dac, tile_id, block_id, &mixer_settings)
        != XRFDC_SUCCESS) {
        throw mpm::runtime_error("Error in RFDC code: Failed to get mixer settings");
    }
    // The XRFdc API returns the frequency in MHz
    return mixer_settings.Freq * 1e6;
}

bool rfdc_ctrl::set_nco_event_src(uint32_t tile_id, uint32_t block_id, bool is_dac)
{
    XRFdc_Mixer_Settings mixer_settings;

    // Get current mixer settings for the values that will not be changed
    if (XRFdc_GetMixerSettings(rfdc_inst_ptr, is_dac, tile_id, block_id, &mixer_settings)
        != XRFDC_SUCCESS) {
        return false;
    }

    // Reset the phase on a SYSREF trigger
    mixer_settings.EventSource = XRFDC_EVNT_SRC_SYSREF;

    // Set the mixer settings to set the NCO event source
    return (XRFdc_SetMixerSettings(rfdc_inst_ptr, is_dac, tile_id, block_id, &mixer_settings)
            == XRFDC_SUCCESS);
}

bool rfdc_ctrl::set_mixer_mode(
    uint32_t tile_id, uint32_t block_id, bool is_dac, mixer_mode_options mixer_mode)
{
    XRFdc_Mixer_Settings mixer_settings;

    // Get current mixer settings for the values that will not be changed
    if (XRFdc_GetMixerSettings(rfdc_inst_ptr, is_dac, tile_id, block_id, &mixer_settings)
        != XRFDC_SUCCESS) {
        return false;
    }

    mixer_settings.MixerMode = mixer_mode;
    // Update the setting on a tile-wide event trigger
    mixer_settings.EventSource = XRFDC_EVNT_SRC_TILE;

    return (XRFdc_SetMixerSettings(
                rfdc_inst_ptr, is_dac, tile_id, block_id, &mixer_settings)
               == XRFDC_SUCCESS)
               && trigger_update_event(tile_id, block_id, is_dac, MIXER_EVENT);
}

bool rfdc_ctrl::set_nyquist_zone(
    uint32_t tile_id, uint32_t block_id, bool is_dac, nyquist_zone_options nyquist_zone)
{
    return XRFdc_SetNyquistZone(rfdc_inst_ptr, is_dac, tile_id, block_id, nyquist_zone)
           == XRFDC_SUCCESS;
}

bool rfdc_ctrl::set_calibration_mode(
    uint32_t tile_id, uint32_t block_id, calibration_mode_options calibration_mode)
{
    return XRFdc_SetCalibrationMode(rfdc_inst_ptr, tile_id, block_id, calibration_mode)
           == XRFDC_SUCCESS;
}

bool rfdc_ctrl::enable_inverse_sinc_filter(
    uint32_t tile_id, uint32_t block_id, bool enable)
{
    return XRFdc_SetInvSincFIR(rfdc_inst_ptr, tile_id, block_id, enable) == XRFDC_SUCCESS;
}

bool rfdc_ctrl::set_sample_rate(uint32_t tile_id, bool is_dac, double sample_rate)
{
    // The XRFdc API expects the sample rate in MHz
    double sample_rate_mhz = sample_rate / 1e6;
    return XRFdc_DynamicPLLConfig(rfdc_inst_ptr,
               is_dac,
               tile_id,
               XRFDC_EXTERNAL_CLK,
               sample_rate_mhz,
               sample_rate_mhz)
           == XRFDC_SUCCESS;
}

double rfdc_ctrl::get_sample_rate(uint32_t tile_id, uint32_t block_id, bool is_dac)
{
    XRFdc_BlockStatus block_status;

    if (XRFdc_GetBlockStatus(rfdc_inst_ptr, is_dac, tile_id, block_id, &block_status)
        != XRFDC_SUCCESS) {
        throw mpm::runtime_error("Error in RFDC code: Failed to get Block status");
    }
    // The XRFdc API returns the sampling frequency in GHz
    return block_status.SamplingFreq * 1e9;
}

bool rfdc_ctrl::set_if(uint32_t tile_id, uint32_t block_id, bool is_dac, double if_freq)
{
    nyquist_zone_options nyquist_zone;
    mixer_mode_options mixer_mode;
    bool enable_inverse_sinc;
    double nco_freq;

    double nyquist_cutoff = get_sample_rate(tile_id, block_id, is_dac) / 2;

    if (if_freq <= nyquist_cutoff) { // First Nyquist Zone
        nyquist_zone        = ODD_NYQUIST_ZONE;
        mixer_mode          = is_dac ? MIXER_MODE_C2R : MIXER_MODE_R2C;
        enable_inverse_sinc = true;
    } else { // Second Nyquist Zone
        nyquist_zone        = EVEN_NYQUIST_ZONE;
        mixer_mode          = is_dac ? MIXER_MODE_C2R : MIXER_MODE_R2C;
        enable_inverse_sinc = false;
    }

    return set_nyquist_zone(tile_id, block_id, is_dac, nyquist_zone)
           && set_mixer_mode(tile_id, block_id, is_dac, mixer_mode)
           && (is_dac ? enable_inverse_sinc_filter(tile_id, block_id, enable_inverse_sinc)
                      : true)
           && set_nco_freq(tile_id, block_id, is_dac, if_freq)
           && set_nco_event_src(tile_id, block_id, is_dac);
}

bool rfdc_ctrl::set_decimation_factor(
    uint32_t tile_id, uint32_t block_id, interp_decim_options decimation_factor)
{
    return XRFdc_SetDecimationFactor(rfdc_inst_ptr, tile_id, block_id, decimation_factor)
           == XRFDC_SUCCESS;
}

rfdc_ctrl::interp_decim_options rfdc_ctrl::get_decimation_factor(
    uint32_t tile_id, uint32_t block_id)
{
    uint32_t decimation_factor;
    if (XRFdc_GetDecimationFactor(rfdc_inst_ptr, tile_id, block_id, &decimation_factor)
        != XRFDC_SUCCESS) {
        throw mpm::runtime_error("Error in RFDC code: Failed to get decimation factor");
    }
    return (interp_decim_options)decimation_factor;
}

bool rfdc_ctrl::set_interpolation_factor(
    uint32_t tile_id, uint32_t block_id, interp_decim_options interpolation_factor)
{
    return XRFdc_SetInterpolationFactor(
               rfdc_inst_ptr, tile_id, block_id, interpolation_factor)
           == XRFDC_SUCCESS;
}

rfdc_ctrl::interp_decim_options rfdc_ctrl::get_interpolation_factor(
    uint32_t tile_id, uint32_t block_id)
{
    uint32_t interpolation_factor;
    if (XRFdc_GetInterpolationFactor(
            rfdc_inst_ptr, tile_id, block_id, &interpolation_factor)
        != XRFDC_SUCCESS) {
        throw mpm::runtime_error(
            "Error in RFDC code: Failed to get interpolation factor");
    }
    return (interp_decim_options)interpolation_factor;
}

bool rfdc_ctrl::set_data_read_rate(
    uint32_t tile_id, uint32_t block_id, uint32_t valid_read_words)
{
    return XRFdc_SetFabRdVldWords(rfdc_inst_ptr, tile_id, block_id, valid_read_words)
           == XRFDC_SUCCESS;
}

uint32_t rfdc_ctrl::get_data_read_rate(uint32_t tile_id, uint32_t block_id, bool is_dac)
{
    uint32_t valid_read_words;
    if (XRFdc_GetFabRdVldWords(
            rfdc_inst_ptr, is_dac, tile_id, block_id, &valid_read_words)
        != XRFDC_SUCCESS) {
        throw mpm::runtime_error("Error in RFDC code: Failed to get data read rate");
    }
    return valid_read_words;
}

bool rfdc_ctrl::set_data_write_rate(
    uint32_t tile_id, uint32_t block_id, uint32_t valid_write_words)
{
    return XRFdc_SetFabWrVldWords(rfdc_inst_ptr, tile_id, block_id, valid_write_words)
           == XRFDC_SUCCESS;
}

uint32_t rfdc_ctrl::get_data_write_rate(uint32_t tile_id, uint32_t block_id, bool is_dac)
{
    uint32_t valid_write_words;
    if (XRFdc_GetFabWrVldWords(
            rfdc_inst_ptr, is_dac, tile_id, block_id, &valid_write_words)
        != XRFDC_SUCCESS) {
        throw mpm::runtime_error("Error in RFDC code: Failed to get data write rate");
    }
    return valid_write_words;
}

bool rfdc_ctrl::set_fabric_clk_div(
    uint32_t tile_id, bool is_dac, fabric_clk_div_options divider)
{
    return XRFdc_SetFabClkOutDiv(rfdc_inst_ptr, is_dac, tile_id, divider)
           == XRFDC_SUCCESS;
}

rfdc_ctrl::fabric_clk_div_options rfdc_ctrl::get_fabric_clk_div(
    uint32_t tile_id, bool is_dac)
{
    uint16_t divider;
    if (XRFdc_GetFabClkOutDiv(rfdc_inst_ptr, is_dac, tile_id, &divider)
        != XRFDC_SUCCESS) {
        throw mpm::runtime_error(
            "Error in RFDC code: Failed to get fabric clock divider");
    }
    return (fabric_clk_div_options)divider;
}

bool rfdc_ctrl::set_data_fifo_state(uint32_t tile_id, bool is_dac, bool enable)
{
    return XRFdc_SetupFIFO(rfdc_inst_ptr, is_dac, tile_id, enable) == XRFDC_SUCCESS;
}

bool rfdc_ctrl::get_data_fifo_state(uint32_t tile_id, bool is_dac)
{
    uint8_t enabled;
    if (XRFdc_GetFIFOStatus(rfdc_inst_ptr, is_dac, tile_id, &enabled) != XRFDC_SUCCESS) {
        throw mpm::runtime_error("Error in RFDC code: Failed to get FIFO status");
    }
    return (bool)enabled;
}

void rfdc_ctrl::clear_data_fifo_interrupts(
    const uint32_t tile_id, const uint32_t block_id, const bool is_dac)
{
    if (XRFdc_IntrClr(rfdc_inst_ptr,
            static_cast<u32>(is_dac),
            tile_id,
            block_id,
            XRFDC_IXR_FIFOUSRDAT_MASK)
        != XRFDC_SUCCESS) {
        throw mpm::runtime_error(
            "Error in RFDC code: Failed to clear data FIFO interrupts");
    }
}

bool rfdc_ctrl::sync_tiles(const std::vector<uint32_t>& tiles, bool is_dac, uint32_t latency)
{
    XRFdc_MultiConverter_Sync_Config* sync_config = is_dac ? &rfdc_dac_sync_config
                                                           : &rfdc_adc_sync_config;
    sync_config->Tiles = 0;
    sync_config->Target_Latency = latency;

    for (auto tile = tiles.begin(); tile != tiles.end(); ++tile) {
        // sync_config->Tiles is a bitmask, we need to "bump" each bit (0->1)
        // that corresponds to the specified indices
        sync_config->Tiles |= (1 << *tile);
    }

    return XRFDC_MTS_OK
           == XRFdc_MultiConverter_Sync(
                  &rfdc_inst, is_dac ? XRFDC_DAC_TILE : XRFDC_ADC_TILE, sync_config);
}

uint32_t rfdc_ctrl::get_tile_latency(uint32_t tile_index, bool is_dac)
{
    XRFdc_MultiConverter_Sync_Config* sync_config = is_dac ? &rfdc_dac_sync_config
                                                           : &rfdc_adc_sync_config;
    // If user has called sync with this tile_index, this
    // attribute should be populated in our sync config
    if ((1 << tile_index) & sync_config->Tiles) {
        return sync_config->Latency[tile_index];
    }
    if (is_dac) {
        throw mpm::runtime_error("rfdc_ctrl: Failed to get DAC Tile Latency");
    } else {
        throw mpm::runtime_error("rfdc_ctrl: Failed to get ADC Tile Latency");
    }
}

uint32_t rfdc_ctrl::get_tile_offset(uint32_t tile_index, bool is_dac)
{
    XRFdc_MultiConverter_Sync_Config* sync_config = is_dac ? &rfdc_dac_sync_config
                                                           : &rfdc_adc_sync_config;
    // If user has called sync with this tile_index, this
    // attribute should be populated in our sync config
    if ((1 << tile_index) & sync_config->Tiles) {
        return sync_config->Offset[tile_index];
    }
    if (is_dac) {
        throw mpm::runtime_error("rfdc_ctrl: Failed to get DAC Tile Offset");
    } else {
        throw mpm::runtime_error("rfdc_ctrl: Failed to get ADC Tile Offset");
    }
}

void rfdc_ctrl::set_cal_frozen(
    const uint32_t tile_id, const uint32_t block_id, const bool frozen)
{
    XRFdc_Cal_Freeze_Settings cal_freeze_settings;
    cal_freeze_settings.CalFrozen         = false;
    cal_freeze_settings.DisableFreezePin  = true;
    cal_freeze_settings.FreezeCalibration = frozen;
    if (XRFdc_SetCalFreeze(&rfdc_inst, tile_id, block_id, &cal_freeze_settings)
        != XRFDC_SUCCESS) {
        throw mpm::runtime_error(
            "Error in RFDC code: Failed to set calibration freeze status");
    }
}

bool rfdc_ctrl::get_cal_frozen(const uint32_t tile_id, const uint32_t block_id)
{
    XRFdc_Cal_Freeze_Settings cal_freeze_settings;
    if (XRFdc_GetCalFreeze(&rfdc_inst, tile_id, block_id, &cal_freeze_settings)
        != XRFDC_SUCCESS) {
        throw mpm::runtime_error(
            "Error in RFDC code: Failed to get calibration freeze status");
    }
    return cal_freeze_settings.CalFrozen;
}

void rfdc_ctrl::set_adc_cal_coefficients(uint32_t tile_id, uint32_t block_id, uint32_t cal_block, std::vector<uint32_t> coefs)
{
    if (coefs.size() != 8)
    {
        throw mpm::runtime_error("set_adc_cal_coefficients requires that exactly 8 coefficients be passed");
    }

    XRFdc_Calibration_Coefficients cs;
    cs.Coeff0 = coefs[0];
    cs.Coeff1 = coefs[1];
    cs.Coeff2 = coefs[2];
    cs.Coeff3 = coefs[3];
    cs.Coeff4 = coefs[4];
    cs.Coeff5 = coefs[5];
    cs.Coeff6 = coefs[6];
    cs.Coeff7 = coefs[7];

    if (XRFdc_SetCalCoefficients(&rfdc_inst, tile_id, block_id, cal_block, &cs) != XRFDC_SUCCESS) {
        throw mpm::runtime_error("Error returned from XRFdc_SetCalCoefficients");
    }
}

std::vector<uint32_t> rfdc_ctrl::get_adc_cal_coefficients(uint32_t tile_id, uint32_t block_id, uint32_t cal_block)
{
    std::vector<uint32_t> result;
    XRFdc_Calibration_Coefficients cs;
    if (XRFdc_GetCalCoefficients(&rfdc_inst, tile_id, block_id, cal_block, &cs) != XRFDC_SUCCESS) {
        throw mpm::runtime_error("Error returned from XRFdc_GetCalCoefficients");
    }

    result.push_back(cs.Coeff0);
    result.push_back(cs.Coeff1);
    result.push_back(cs.Coeff2);
    result.push_back(cs.Coeff3);
    result.push_back(cs.Coeff4);
    result.push_back(cs.Coeff5);
    result.push_back(cs.Coeff6);
    result.push_back(cs.Coeff7);

    return result;
}

}} // namespace mpm::rfdc

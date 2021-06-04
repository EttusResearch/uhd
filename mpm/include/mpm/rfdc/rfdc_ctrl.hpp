//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "xrfdc.h"
#include "xrfdc_mts.h"
#include <boost/noncopyable.hpp>
#include <vector>

#ifdef LIBMPM_PYTHON
#    include <pybind11/stl.h>
#endif

#define THRESHOLDS_PER_BLOCK 2

namespace mpm { namespace rfdc {

/**
 * A class to control the Xilinx RFdc driver.
 * This will be imported into a MPM shared library.
 */
class rfdc_ctrl : public boost::noncopyable
{
    XRFdc rfdc_inst;
    XRFdc* rfdc_inst_ptr;
    XRFdc_MultiConverter_Sync_Config rfdc_dac_sync_config;
    XRFdc_MultiConverter_Sync_Config rfdc_adc_sync_config;
    uint16_t rfdc_device_id;

public:
    /**
     * These macros are placed within enums so they can be imported to Python.
     * They are originally defined in xrfdc.h
     */
    enum threshold_id_options {
        THRESHOLD_0    = XRFDC_UPDATE_THRESHOLD_0,
        THRESHOLD_1    = XRFDC_UPDATE_THRESHOLD_1,
        THRESHOLD_BOTH = XRFDC_UPDATE_THRESHOLD_BOTH
    };
    enum threshold_mode_options {
        TRSHD_OFF          = XRFDC_TRSHD_OFF,
        TRSHD_STICKY_OVER  = XRFDC_TRSHD_STICKY_OVER,
        TRSHD_STICKY_UNDER = XRFDC_TRSHD_STICKY_UNDER,
        TRSHD_HYSTERESIS   = XRFDC_TRSHD_HYSTERISIS
    };
    enum threshold_clr_mode_options {
        THRESHOLD_CLRMD_MANUAL = XRFDC_THRESHOLD_CLRMD_MANUAL_CLR,
        THRESHOLD_CLRMD_AUTO   = XRFDC_THRESHOLD_CLRMD_AUTO_CLR,
        // The XRFdc Threshold clear modes currently only go up to 2
        // This assumes there will never be a clear mode of value 99
        THRESHOLD_CLRMD_UNKNOWN = 99
    };
    enum decoder_mode_options {
        DECODER_MAX_SNR_MODE = XRFDC_DECODER_MAX_SNR_MODE, // for non-randomized decoder
        DECODER_MAX_LINEARITY_MODE =
            XRFDC_DECODER_MAX_LINEARITY_MODE // for randomized decoder
    };
    enum nyquist_zone_options {
        ODD_NYQUIST_ZONE  = XRFDC_ODD_NYQUIST_ZONE,
        EVEN_NYQUIST_ZONE = XRFDC_EVEN_NYQUIST_ZONE
    };
    enum mixer_mode_options {
        MIXER_MODE_OFF = XRFDC_MIXER_MODE_OFF,
        MIXER_MODE_C2C = XRFDC_MIXER_MODE_C2C, // Complex to complex
        MIXER_MODE_C2R = XRFDC_MIXER_MODE_C2R, // Complex to real
        MIXER_MODE_R2C = XRFDC_MIXER_MODE_R2C, // Real to complex
        MIXER_MODE_R2R = XRFDC_MIXER_MODE_R2R // Real to real
    };
    /**
     * See section "RF-ADC Settings" of the Xilinx
     * "RF Data Converter Interface User Guide" to learn
     * more about the calibration modes.
     */
    enum calibration_mode_options {
        CALIB_MODE1 = XRFDC_CALIB_MODE1,
        CALIB_MODE2 = XRFDC_CALIB_MODE2
    };
    enum event_type_options {
        MIXER_EVENT = XRFDC_EVENT_MIXER,
        CRSE_DLY_EVENT = XRFDC_EVENT_CRSE_DLY,
        QMC_EVENT = XRFDC_EVENT_QMC,
    };
    enum interp_decim_options {
        INTERP_DECIM_OFF = XRFDC_INTERP_DECIM_OFF,
        INTERP_DECIM_1X  = XRFDC_INTERP_DECIM_1X,
        INTERP_DECIM_2X  = XRFDC_INTERP_DECIM_2X,
        INTERP_DECIM_4X  = XRFDC_INTERP_DECIM_4X,
        INTERP_DECIM_8X  = XRFDC_INTERP_DECIM_8X,
    };
    enum fabric_clk_div_options {
        DIV_1  = XRFDC_FAB_CLK_DIV1,
        DIV_2  = XRFDC_FAB_CLK_DIV2,
        DIV_4  = XRFDC_FAB_CLK_DIV4,
        DIV_8  = XRFDC_FAB_CLK_DIV8,
        DIV_16 = XRFDC_FAB_CLK_DIV16,
    };

    /**
     * Assignes the rfdc_inst_ptr to an instance of the Xilinx RFdc driver
     */
    rfdc_ctrl();

    /**
     * Closes the libmetal device
     */
    ~rfdc_ctrl();

    /**
     * Initializes the driver by reading configuration settings
     * from the device found in the device tree and applying them to
     * the driver instance.
     * Throws an exception if init fails.
     *
     * @param    rfdc_device_id the device ID of the rfdc device
     */
    void init(uint16_t rfdc_device_id);

    /**
     * Starts up the requested tile while retaining register values.
     *
     * @param    tile_id the ID of the tile to start.
     *           Pass -1 to select all tiles.
     * @param    is_dac whether the tile is a DAC (true) or ADC (false)
     *
     * @return   true if the operation was successful
     */
    bool startup_tile(int tile_id, bool is_dac);

    /**
     * Shuts down the requested tile while retaining register values.
     *
     * @param    tile_id the ID of the tile to stop.
     *           Pass -1 to select all tiles.
     * @param    is_dac whether the tile is a DAC (true) or ADC (false)
     *
     * @return   true if the operation was successful
     */
    bool shutdown_tile(int tile_id, bool is_dac);

    /**
     * Restarts the requested tile while resetting registers to default values.
     *
     * @param    tile_id the ID of the tile to restart.
     *           Pass -1 to select all tiles.
     * @param    is_dac whether the tile is a DAC (true) or ADC (false)
     *
     * @return   true if the operation was successful
     */
    bool reset_tile(int tile_id, bool is_dac);

    /**
     * Triggers an update event for a given component.
     *
     * @param    tile_id the tile ID of the block to trigger
     * @param    block_id the block ID of the block to trigger
     * @param    is_dac whether the block is a DAC (true) or ADC (false)
     * @param    event_type which component of block to update
     *           See event_type_options for valid values.
     *
     * @return   true if the operation was successful
     */
    bool trigger_update_event(uint32_t tile_id, uint32_t block_id,
       bool is_dac, event_type_options event_type);

    /**
     * Enable/Disable gain correction for a given block.
     *
     * @param    tile_id the tile ID of the block to set
     * @param    block_id the block ID of the block to set
     * @param    is_dac whether the block is a DAC (true) or ADC (false)
     * @param    enable whether to enable or disable gain correction
     *
     * @return   true if the operation was successful
     */
    bool set_gain_enable(uint32_t tile_id, uint32_t block_id, bool is_dac, bool enable);

    /**
     * Set gain correction on a given ADC or DAC block
     *
     * @param    tile_id the tile ID of the block to set
     * @param    block_id the block ID of the block to set
     * @param    is_dac whether the block is a DAC (true) or ADC (false)
     * @param    gain the gain correction to set.
     *           Valid values are 0.0-2.0
     *
     * @return   true if the operation was successful
     */
    bool set_gain(uint32_t tile_id, uint32_t block_id, bool is_dac, double gain);

    /**
     * Set the threshold settings for a given ADC
     *
     * @param    tile_id the tile ID of the block to set
     * @param    block_id the block ID of the block to set
     * @param    threshold_id the ID of the threshold to modify.
     *           See threshold_id_options for valid values.
     * @param    mode the threshold mode to set.
     *           See threshold_mode_options for valid values.
     * @param    average_val the average threshold value
     * @param    under_val the under threshold value
     * @param    over_val the over threshold value
     *
     * @return   true if the operation was successful
     */
    bool set_threshold_settings(uint32_t tile_id,
        uint32_t block_id,
        threshold_id_options threshold_id,
        threshold_mode_options mode,
        uint32_t average_val,
        uint32_t under_val,
        uint32_t over_val);

    /**
     * Clears the sticky line which indicates a threshold has been breached.
     * This will also set the sticky clear mode to be manual.
     *
     * @param    tile_id the tile ID of the block to set
     * @param    block_id the block ID of the block to set
     * @param    threshold_id the ID of the threshold to modify.
     *           See threshold_id_options for valid values.
     *
     * @return   true if the operation was successful
     */
    bool clear_threshold_sticky(
        uint32_t tile_id, uint32_t block_id, threshold_id_options threshold_id);

    /**
     * Sets whether the threshold breach sticky is cleared manually
     * or automatically (when QMC gain is changed).
     *
     * @param    tile_id the tile ID of the block to set
     * @param    block_id the block ID of the block to set
     * @param    threshold_id the ID of the threshold to modify.
     *           See threshold_id_options for valid values.
     * @param    mode What mode to set for the threshold sticky clear mode.
     *           See threshold_clr_mode_options for valid values.
     *
     * @return   true if the operation was successful
     */
    bool set_threshold_clr_mode(uint32_t tile_id,
        uint32_t block_id,
        threshold_id_options threshold_id,
        threshold_clr_mode_options clear_mode);

    /**
     * Gets the threshold sticky clear mode
     *
     * @param    tile_id the tile ID of the block to set
     * @param    block_id the block ID of the block to set
     * @param    threshold_id the ID of the threshold to modify.
     *           See threshold_id_options for valid values.
     *           Note: THRESHOLD_BOTH is not a valid threshold_id for this
     *           method and will result in THRESHOLD_CLRMD_UNKNOWN.
     * @param    mode What mode to set for the threshold sticky clear mode.
     *           See threshold_clr_mode_options for valid values.
     *
     * @return   threshold_clr_mode_options which is currently set.
     *           A value of THRESHOLD_CLRMD_UNKNOWN indicates that the hardware
     *           setting is currently unknown or an invalid ID was given.
     */
    threshold_clr_mode_options get_threshold_clr_mode(
        uint32_t tile_id, uint32_t block_id, threshold_id_options threshold_id);

    /**
     * Sets the decoder mode of a given DAC
     * An auto-clear takes place when the gain setting is changed.
     *
     * @param    tile_id the tile ID of the block to set
     * @param    block_id the block ID of the block to set
     * @param    decoder_mode the desired decoder mode for the DAC.
     *           See decoder_mode_options for valid values.
     *
     * @return   true if the operation was successful
     */
    bool set_decoder_mode(
        uint32_t tile_id, uint32_t block_id, decoder_mode_options decoder_mode);

    /**
     * Resets the NCO phase of the current block phase accumulator.
     *
     * @param    tile_id the tile ID of the block to set
     * @param    block_id the block ID of the block to set
     * @param    is_dac whether the block is a DAC (true) or ADC (false)
     *
     * @return   true if the operation was successful
     */
    bool reset_nco_phase(uint32_t tile_id, uint32_t block_id, bool is_dac);

    /**
     * Sets the NCO event source for a given DAC or ADC
     *
     * @param    tile_id the tile ID of the block to set
     * @param    block_id the block ID of the block to set
     * @param    is_dac whether the block is a DAC (true) or ADC (false)
     *
     * @return   true if the operation was successful
     */
    bool set_nco_event_src(uint32_t tile_id, uint32_t block_id, bool is_dac);

    /**
     * Sets the NCO frequency for a given DAC or ADC
     *
     * @param    tile_id the tile ID of the block to set
     * @param    block_id the block ID of the block to set
     * @param    is_dac whether the block is a DAC (true) or ADC (false)
     * @param    freq the NCO frequency to set
     *           Frequencies are specified in Hz.
     *
     * @return   true if the operation was successful
     */
    bool set_nco_freq(uint32_t tile_id, uint32_t block_id, bool is_dac, double freq);

    /**
     * Gets the NCO frequency for a given DAC or ADC
     *
     * @param    tile_id the tile ID of the block
     * @param    block_id the block ID of the block
     * @param    is_dac whether the block is a DAC (true) or ADC (false)
     *
     * @return   freq of the NCO in Hz
     */
    double get_nco_freq(uint32_t tile_id, uint32_t block_id, bool is_dac);

    /**
     * Sets the mixer mode of the given block
     *
     * @param    tile_id the tile ID of the block to set
     * @param    block_id the block ID of the block to set
     * @param    is_dac whether the block is a DAC (true) or ADC (false)
     * @param    mixer_mode the mixer mode to set.
     *           See mixer_mode_options for valid values
     *
     * @return   true if the operation was successful
     */
    bool set_mixer_mode(
        uint32_t tile_id, uint32_t block_id, bool is_dac, mixer_mode_options mixer_mode);

    /**
     * Sets the Nyquist Zone of a give block
     *
     * @param    tile_id the tile ID of the block to set
     * @param    block_id the block ID of the block to set
     * @param    is_dac whether the block is a DAC (true) or ADC (false)
     * @param    nyquist_zone the nyquist zone to set
     *           See nyquist_zone_options for valid values.
     *
     * @return   true if the operation was successful
     */
    bool set_nyquist_zone(uint32_t tile_id,
        uint32_t block_id,
        bool is_dac,
        nyquist_zone_options nyquist_zone);

    /**
     * Sets the calibration mode of a given ADC
     *
     * @param    tile_id the tile ID of the block to set
     * @param    block_id the block ID of the block to set
     * @param    calibration_mode the calibration mode to set.
     *           See calibration_mode_options for valid values.
     *           See section "RF-ADC Settings" of the Xilinx
     *           "RF Data Converter Interface User Guide" to learn
     *           more about the modes.
     *
     * @return   true if the operation was successful
     */
    bool set_calibration_mode(
        uint32_t tile_id, uint32_t block_id, calibration_mode_options calibration_mode);

    /**
     * Enables/Disables the Inverse-Sinc filter on a DAC block.
     *
     * @param    tile_id the tile ID of the block to set
     * @param    block_id the block ID of the block to set
     * @param    enable enables the filter if true, disables if false
     *
     * @return   true if the operation was successful
     */
    bool enable_inverse_sinc_filter(uint32_t tile_id, uint32_t block_id, bool enable);

    /**
     * Sets the sample rate for a given tile.
     *
     * @param    tile_id the ID of the tile to set
     * @param    is_dac whether the tile is a DAC (true) or ADC (false)
     * @param    sample_rate the rate in Hz to sample at
     *
     * @return   true if the operation was successful
     */
    bool set_sample_rate(uint32_t tile_id, bool is_dac, double sample_rate);

    /**
     * Gets the sample rate for a given block.
     *
     * @param    tile_id the ID of the tile to set
     * @param    block_id the ID of the block to set
     * @param    is_dac whether the tile is a DAC (true) or ADC (false)
     *
     * @return   sample rate of the block in Hz
     */
    double get_sample_rate(uint32_t tile_id, uint32_t block_id, bool is_dac);

    /**
     * Specifies the IF for the given ADC or DAC.
     * Setting this will determine the Nyquist zone, mixer mode,
     * Inverse Sinc filter, and mixer NCO frequency.
     *
     * @param    tile_id the tile ID of the block to set
     * @param    block_id the block ID of the block to set
     * @param    is_dac whether the block is a DAC (true) or ADC (false)
     * @param    if_freq the IF frequency expected for the block.
     *           Frequencies are specified in Hz.
     *
     * @return   true all resulting settings were successfully changed
     */
    bool set_if(uint32_t tile_id, uint32_t block_id, bool is_dac, double if_freq);

    /**
     * Sets the decimation factor for a given ADC block
     *
     * @param    tile_id the ID of the tile to set
     * @param    block_id the block ID of the block to set
     * @param    decimation_factor the desired factor
     *           See interp_decim_options for valid values
     *
     * @return   true if the operation was successful
     */
    bool set_decimation_factor(
        uint32_t tile_id, uint32_t block_id, interp_decim_options decimation_factor);

    /**
     * Gets the decimation factor for a given ADC block
     *
     * @param    tile_id the ID of the tile to get
     * @param    block_id the block ID of the block to get
     *
     * @return   the actual decimation factor
     *           See interp_decim_options for valid values
     */
    interp_decim_options get_decimation_factor(uint32_t tile_id, uint32_t block_id);

    /**
     * Sets the interpolation factor for a given DAC block
     *
     * @param    tile_id the ID of the tile to set
     * @param    block_id the block ID of the block to set
     * @param    interpolation_factor the desired factor
     *           See interp_decim_options for valid values.
     *
     * @return   true if the operation was successful
     */
    bool set_interpolation_factor(
        uint32_t tile_id, uint32_t block_id, interp_decim_options interpolation_factor);

    /**
     * Gets the interpolation factor for a given DAC block
     *
     * @param    tile_id the ID of the tile to get
     * @param    block_id the block ID of the block to get
     *
     * @return   the actual interpolation factor
     *           See interp_decim_options for valid values
     */
    interp_decim_options get_interpolation_factor(uint32_t tile_id, uint32_t block_id);

    /**
     * Sets the number of valid read words for a given ADC block
     *
     * @param    tile_id the ID of the tile to set
     * @param    block_id the block ID of the block to set
     * @param    valid_read_words the number of valid read words
     *
     * @return   true if the operation was successful
     */
    bool set_data_read_rate(
        uint32_t tile_id, uint32_t block_id, uint32_t valid_read_words);

    /**
     * Gets the number of valid read words for a given ADC/DAC block
     *
     * @param    tile_id the ID of the tile to get
     * @param    block_id the block ID of the block to get
     * @param    is_dac whether the tile is a DAC (true) or ADC (false)
     *
     * @return   the valid read words
     */
    uint32_t get_data_read_rate(uint32_t tile_id, uint32_t block_id, bool is_dac);

    /**
     * Sets the number of valid write words for a given DAC block
     *
     * @param    tile_id the ID of the tile to set
     * @param    block_id the block ID of the block to set
     * @param    valid_write_words the number of valid write words
     *
     * @return   true if the operation was successful
     */
    bool set_data_write_rate(
        uint32_t tile_id, uint32_t block_id, uint32_t valid_write_words);

    /**
     * Gets the number of valid write words for a given ADC/DAC block
     *
     * @param    tile_id the ID of the tile to get
     * @param    block_id the block ID of the block to get
     * @param    is_dac whether the tile is a DAC (true) or ADC (false)

     *
     * @return   the valid write words
     */
    uint32_t get_data_write_rate(uint32_t tile_id, uint32_t block_id, bool is_dac);

    /**
     * Sets the clock fabric output divider of a given tile
     *
     * @param    tile_id the ID of the tile to set
     * @param    is_dac whether the tile is a DAC (true) or ADC (false)
     * @param    divider the divider to set
     *           See fabric_clk_div_options for valid values
     *
     * @return   true if the operation was successful
     */
    bool set_fabric_clk_div(
        uint32_t tile_id, bool is_dac, fabric_clk_div_options divider);

    /**
     * Gets the fabric clock divider rate of a Tile
     *
     * @param    tile_id the ID of the tile to get
     * @param    is_dac whether the tile is a DAC (true) or ADC (false)
     *
     * @return   the fabric clock divider
     *           See fabric_clk_div_options for valid values
     */
    fabric_clk_div_options get_fabric_clk_div(uint32_t tile_id, bool is_dac);

    /**
     * Sets the FIFO for an ADC/DAC
     *
     * @param    tile_id the ID of the tile to get
     * @param    is_dac whether the tile is a DAC (true) or ADC (false)
     * @param    enable enables (true) or disables (false) the FIFO

     *
     * @return   true if the operation was successful
     */
    bool set_data_fifo_state(uint32_t tile_id, bool is_dac, bool enable);

    /**
     * Gets the FIFO for an ADC/DAC
     *
     * @param    tile_id the ID of the tile to get
     * @param    is_dac whether the tile is a DAC (true) or ADC (false)

     *
     * @return   true if FIFO is enabled, false if it is disabled
     */
    bool get_data_fifo_state(uint32_t tile_id, bool is_dac);

    /**
     * Clears the interrupts for the data FIFO (FIFOUSRDAT)
     *
     * @param    tile_id the ID of the tile to get
     * @param    block_id specify ADC/DAC block
     * @param    is_dac whether the tile is a DAC (true) or ADC (false)
     */
    void clear_data_fifo_interrupts(
        const uint32_t tile_id, const uint32_t block_id, const bool is_dac);

    /**
     * Perform Multi-tile Synchronization on ADC or DAC tiles
     *
     * @param    tiles tiles vector to specify which DAC/ADC tiles to synchronize
     * @param    is_dac whether the tile is a DAC (true) or ADC (false)
     *
     * @return   true if synchronization completed successfully
     */
    bool sync_tiles(const std::vector<uint32_t>& tiles, bool is_dac, uint32_t latency);

    /**
     * Get post-sync latency between ADC or DAC tiles
     *
     * @param    tile_index specify ADC or DAC target tile
     * @param    is_dac whether the tile is a DAC (true) or ADC (false)
     *
     * @return   the measured relative latency value of each tile
     */
    uint32_t get_tile_latency(uint32_t tile_index, bool is_dac);

    /**
     * Get post-sync offset between ADC or DAC tile and reference tile
     *
     * @param    tile_index specify ADC or DAC target tile
     * @param    is_dac whether the tile is a DAC (true) or ADC (false)
     *
     * @return   value the interface data was delayed to achieve alignment
     */
    uint32_t get_tile_offset(uint32_t tile_index, bool is_dac);

    /**
     * Sets whether or not the ADC calibration blocks are frozen
     *
     * @param    tile_id specify ADC target tile
     * @param    block_id specify ADC block
     * @param    frozen specify whether or not the ADC calibration blocks should be frozen
     */
    void set_cal_frozen(uint32_t tile_id, uint32_t block_id, bool frozen);

    /**
     * Sets whether or not the ADC calibration blocks are frozen
     *
     * @param    tile_id specify ADC target tile
     * @param    block_id specify ADC block
     *
     * @return   true if the cal blocks are frozen, false if not
     */
    bool get_cal_frozen(uint32_t tile_id, uint32_t block_id);

    void set_adc_cal_coefficients(uint32_t tile_id, uint32_t block_id, uint32_t cal_block, std::vector<uint32_t> coefs);
    std::vector<uint32_t> get_adc_cal_coefficients(uint32_t tile_id, uint32_t block_id, uint32_t cal_block);

    /**
     * Resets an internal mixer with known valid settings.
     */
    bool reset_mixer_settings( uint32_t tile_id, uint32_t block_id, bool is_dac);

private:
    /* Indicates whether libmetal was initialized successfully and can
     * be safely deinitialized.
     */
    bool metal_init_complete = false;

    // Stores the current threshold clear mode according to
    // [Tile ID][Block ID][Threshold ID]
    threshold_clr_mode_options threshold_clr_modes[XRFDC_TILE_ID_MAX + 1]
                                                  [XRFDC_BLOCK_ID_MAX + 1]
                                                  [THRESHOLDS_PER_BLOCK];
};
}}; /* namespace mpm::rfdc */

#ifdef LIBMPM_PYTHON
void export_rfdc(py::module& top_module)
{
    using namespace mpm::rfdc;
    auto m = top_module.def_submodule("rfdc");

    py::class_<rfdc_ctrl, std::shared_ptr<rfdc_ctrl>>(m, "rfdc_ctrl")
        .def(py::init())
        .def("init", &rfdc_ctrl::init)
        .def("startup_tile", &rfdc_ctrl::startup_tile)
        .def("shutdown_tile", &rfdc_ctrl::shutdown_tile)
        .def("reset_tile", &rfdc_ctrl::reset_tile)
        .def("trigger_update_event", &rfdc_ctrl::trigger_update_event)
        .def("set_gain_enable", &rfdc_ctrl::set_gain_enable)
        .def("set_gain", &rfdc_ctrl::set_gain)
        .def("set_threshold_settings", &rfdc_ctrl::set_threshold_settings)
        .def("clear_threshold_sticky", &rfdc_ctrl::clear_threshold_sticky)
        .def("set_threshold_clr_mode", &rfdc_ctrl::set_threshold_clr_mode)
        .def("get_threshold_clr_mode", &rfdc_ctrl::get_threshold_clr_mode)
        .def("set_decoder_mode", &rfdc_ctrl::set_decoder_mode)
        .def("reset_nco_phase", &rfdc_ctrl::reset_nco_phase)
        .def("set_nco_event_src", &rfdc_ctrl::set_nco_event_src)
        .def("set_nco_freq", &rfdc_ctrl::set_nco_freq)
        .def("get_nco_freq", &rfdc_ctrl::get_nco_freq)
        .def("reset_mixer_settings", &rfdc_ctrl::reset_mixer_settings)
        .def("set_mixer_mode", &rfdc_ctrl::set_mixer_mode)
        .def("set_nyquist_zone", &rfdc_ctrl::set_nyquist_zone)
        .def("set_calibration_mode", &rfdc_ctrl::set_calibration_mode)
        .def("enable_inverse_sinc_filter", &rfdc_ctrl::enable_inverse_sinc_filter)
        .def("set_sample_rate", &rfdc_ctrl::set_sample_rate)
        .def("get_sample_rate", &rfdc_ctrl::get_sample_rate)
        .def("set_if", &rfdc_ctrl::set_if)
        .def("set_decimation_factor", &rfdc_ctrl::set_decimation_factor)
        .def("get_decimation_factor", &rfdc_ctrl::get_decimation_factor)
        .def("set_interpolation_factor", &rfdc_ctrl::set_interpolation_factor)
        .def("get_interpolation_factor", &rfdc_ctrl::get_interpolation_factor)
        .def("set_data_read_rate", &rfdc_ctrl::set_data_read_rate)
        .def("get_data_read_rate", &rfdc_ctrl::get_data_read_rate)
        .def("set_data_write_rate", &rfdc_ctrl::set_data_write_rate)
        .def("get_data_write_rate", &rfdc_ctrl::get_data_write_rate)
        .def("set_fabric_clk_div", &rfdc_ctrl::set_fabric_clk_div)
        .def("get_fabric_clk_div", &rfdc_ctrl::get_fabric_clk_div)
        .def("set_data_fifo_state", &rfdc_ctrl::set_data_fifo_state)
        .def("get_data_fifo_state", &rfdc_ctrl::get_data_fifo_state)
        .def("clear_data_fifo_interrupts", &rfdc_ctrl::clear_data_fifo_interrupts)
        .def("sync_tiles", &rfdc_ctrl::sync_tiles)
        .def("get_tile_latency", &rfdc_ctrl::get_tile_latency)
        .def("get_tile_offset", &rfdc_ctrl::get_tile_offset)
        .def("set_cal_frozen", &rfdc_ctrl::set_cal_frozen)
        .def("get_cal_frozen", &rfdc_ctrl::get_cal_frozen)
        .def("set_adc_cal_coefficients", &rfdc_ctrl::set_adc_cal_coefficients)
        .def("get_adc_cal_coefficients", &rfdc_ctrl::get_adc_cal_coefficients);

    py::enum_<mpm::rfdc::rfdc_ctrl::threshold_id_options>(m, "threshold_id_options")
        .value("THRESHOLD_0", mpm::rfdc::rfdc_ctrl::THRESHOLD_0)
        .value("THRESHOLD_1", mpm::rfdc::rfdc_ctrl::THRESHOLD_1)
        .value("THRESHOLD_BOTH", mpm::rfdc::rfdc_ctrl::THRESHOLD_BOTH);

    py::enum_<mpm::rfdc::rfdc_ctrl::threshold_mode_options>(m, "threshold_mode_options")
        .value("TRSHD_OFF", mpm::rfdc::rfdc_ctrl::TRSHD_OFF)
        .value("TRSHD_STICKY_OVER", mpm::rfdc::rfdc_ctrl::TRSHD_STICKY_OVER)
        .value("TRSHD_STICKY_UNDER", mpm::rfdc::rfdc_ctrl::TRSHD_STICKY_UNDER)
        .value("TRSHD_HYSTERESIS", mpm::rfdc::rfdc_ctrl::TRSHD_HYSTERESIS);

    py::enum_<mpm::rfdc::rfdc_ctrl::threshold_clr_mode_options>(
        m, "threshold_clr_mode_options")
        .value("THRESHOLD_CLRMD_MANUAL", mpm::rfdc::rfdc_ctrl::THRESHOLD_CLRMD_MANUAL)
        .value("THRESHOLD_CLRMD_AUTO", mpm::rfdc::rfdc_ctrl::THRESHOLD_CLRMD_AUTO)
        .value("THRESHOLD_CLRMD_UNKNOWN", mpm::rfdc::rfdc_ctrl::THRESHOLD_CLRMD_UNKNOWN);

    py::enum_<mpm::rfdc::rfdc_ctrl::decoder_mode_options>(m, "decoder_mode_options")
        .value("DECODER_MAX_SNR_MODE", mpm::rfdc::rfdc_ctrl::DECODER_MAX_SNR_MODE)
        .value("DECODER_MAX_LINEARITY_MODE",
            mpm::rfdc::rfdc_ctrl::DECODER_MAX_LINEARITY_MODE);

    py::enum_<mpm::rfdc::rfdc_ctrl::nyquist_zone_options>(m, "nyquist_zone_options")
        .value("ODD_NYQUIST_ZONE", mpm::rfdc::rfdc_ctrl::ODD_NYQUIST_ZONE)
        .value("EVEN_NYQUIST_ZONE", mpm::rfdc::rfdc_ctrl::EVEN_NYQUIST_ZONE);

    py::enum_<mpm::rfdc::rfdc_ctrl::mixer_mode_options>(m, "mixer_mode_options")
        .value("MIXER_MODE_OFF", mpm::rfdc::rfdc_ctrl::MIXER_MODE_OFF)
        .value("MIXER_MODE_C2C", mpm::rfdc::rfdc_ctrl::MIXER_MODE_C2C)
        .value("MIXER_MODE_C2R", mpm::rfdc::rfdc_ctrl::MIXER_MODE_C2R)
        .value("MIXER_MODE_R2C", mpm::rfdc::rfdc_ctrl::MIXER_MODE_R2C)
        .value("MIXER_MODE_R2R", mpm::rfdc::rfdc_ctrl::MIXER_MODE_R2R);

    py::enum_<mpm::rfdc::rfdc_ctrl::calibration_mode_options>(
        m, "calibration_mode_options")
        .value("CALIB_MODE1", mpm::rfdc::rfdc_ctrl::CALIB_MODE1)
        .value("CALIB_MODE2", mpm::rfdc::rfdc_ctrl::CALIB_MODE2);

    py::enum_<mpm::rfdc::rfdc_ctrl::event_type_options>(m, "event_type_options")
        .value("MIXER_EVENT", mpm::rfdc::rfdc_ctrl::MIXER_EVENT)
        .value("CRSE_DLY_EVENT", mpm::rfdc::rfdc_ctrl::CRSE_DLY_EVENT)
        .value("QMC_EVENT", mpm::rfdc::rfdc_ctrl::QMC_EVENT);

    py::enum_<mpm::rfdc::rfdc_ctrl::interp_decim_options>(m, "interp_decim_options")
        .value("INTERP_DECIM_OFF", mpm::rfdc::rfdc_ctrl::INTERP_DECIM_OFF)
        .value("INTERP_DECIM_1X", mpm::rfdc::rfdc_ctrl::INTERP_DECIM_1X)
        .value("INTERP_DECIM_2X", mpm::rfdc::rfdc_ctrl::INTERP_DECIM_2X)
        .value("INTERP_DECIM_4X", mpm::rfdc::rfdc_ctrl::INTERP_DECIM_4X)
        .value("INTERP_DECIM_8X", mpm::rfdc::rfdc_ctrl::INTERP_DECIM_8X);

    py::enum_<mpm::rfdc::rfdc_ctrl::fabric_clk_div_options>(m, "fabric_clk_div_options")
        .value("DIV_1", mpm::rfdc::rfdc_ctrl::DIV_1)
        .value("DIV_2", mpm::rfdc::rfdc_ctrl::DIV_2)
        .value("DIV_4", mpm::rfdc::rfdc_ctrl::DIV_4)
        .value("DIV_8", mpm::rfdc::rfdc_ctrl::DIV_8)
        .value("DIV_16", mpm::rfdc::rfdc_ctrl::DIV_16);
}
#endif

//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>

namespace uhd { namespace rfnoc {

enum class fosphor_waterfall_mode { MAX_HOLD, AVERAGE };
enum class fosphor_waterfall_predivision_ratio {
    RATIO_1_1,
    RATIO_1_8,
    RATIO_1_64,
    RATIO_1_256
};

/*! Fosphor Control Class
 *
 * \ingroup rfnoc_blocks
 *
 * The Fosphor Block is an RFNoC block that accepts FFT data as signed
 * complex 16-bit data and produces two streams of eight-bit data, a
 * stream of histogram data and a stream of waterfall data.
 *
 * \section Histogram
 *
 * Each time the Fosphor block receives an FFT input packet, the power values
 * in each of the N frequency bins are quantized into one of 64 power bins
 * (X axis represents individual FFT frequency bins; Y axis represents the
 * power bins):
 *
 *     63                                .
 *      :                              .   .
 *      :                  . .       .       .     . .
 *      0  . . . . . . . .     . . .           . .     . . . . . . .
 *         0 1 2 3 4 5 - - - - - - - - - - - - - - - - - - - - - - N-1
 *
 * Each time an FFT power value is quantized to a bin, the bin count
 * is increased by one (illustrated by a '+'):
 *
 *     63                                +
 *      :                              +   +
 *      :                  + +       +       +     + +
 *      0  + + + + + + + +     + + +           + +     + + + + + + +
 *         0 1 2 3 4 5 - - - - - - - - - - - - - - - - - - - - - - N-1
 *
 * As more FFT packets are received, the counts in each bin accumulate.
 * Over time, the count in the 'closest' power bin to each sample in the FFT
 * accumulates at the highest rate. However, adjacent power bins' counts may
 * increase due to quantization noise and variances in the input FFT signal
 * (highest counts illustrated by '*', followed by '+' and '.'):
 *
 *     63                              . * +
 *      :          .       . .       + * . * +
 *      :  + + . . + + . + * * + + + * +     * + + * * . + . + + .
 *      0  * * * * * * * * + + * * * +       + * * + + * * * * * *
 *         0 1 2 3 4 5 - - - - - - - - - - - - - - - - - - - - - - N-1
 *
 * The Fosphor block also calculates the average power level and maximum
 * power level encountered in each FFT frequency bin. The rate at which
 * the accumulated counts, average power level, and maximum power level
 * values rise and fall over time is configurable.
 *
 * An instance of histogram data output consists of 66 packets:
 *
 * * 64 packets, one per quantized power level, of N values, representing the
 * accumulated count for each frequency bin for that particular quantized
 * power level;
 * * One packet of N values, representing the average power level in each
 * frequency bin; and
 * * One packet of N values, representing the maximum power level in each
 * frequency bin.
 *
 * \section Waterfall
 *
 * The waterfall stream consists of history data of either the average or
 * maximum power level values in each bin, depending on the selected waterfall
 * mode. In max hold mode, each waterfall packet consists of N values,
 * representing the maximum power level in each frequency bin. The rate
 * that packets are produced relative to the number of input FFT packets is
 * configurable via the waterfall decimation parameter.
 *
 * In average mode, each waterfall packet consists of N values, representing
 * the _sum_ of the average power level in each frequency bin accumulated
 * between packets. (Thus, if the decimation rate is increased, the values
 * returned are higher than if the decimation rate is decreased.) The
 * waterfall predivision ratio parameter can be used to scale the values
 * prior to accumulation to counteract this effect.
 *
 * These streams are intended to be inputs to the GNU Radio Fosphor
 * display block, which renders the streams in a entertaining graphical
 * format.
 */
class UHD_API fosphor_block_control : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(fosphor_block_control)

    // Block registers
    static const uint32_t REG_ENABLE_ADDR;
    static const uint32_t REG_CLEAR_ADDR;
    static const uint32_t REG_RANDOM_ADDR;
    static const uint32_t REG_DECIM_ADDR;
    static const uint32_t REG_OFFSET_ADDR;
    static const uint32_t REG_SCALE_ADDR;
    static const uint32_t REG_TRISE_ADDR;
    static const uint32_t REG_TDECAY_ADDR;
    static const uint32_t REG_ALPHA_ADDR;
    static const uint32_t REG_EPSILON_ADDR;
    static const uint32_t REG_WF_CTRL_ADDR;
    static const uint32_t REG_WF_DECIM_ADDR;

    /*! Set the histogram stream enable flag
     *
     * Enables or disables the stream of histogram data from the block.
     *
     * \param enable_histogram Histogram stream enable/disable flag
     */
    virtual void set_enable_histogram(const bool enable_histogram) = 0;

    /*! Get the histogram stream enable flag
     *
     * Returns the current histogram enable value.
     *
     * \returns Histogram stream enable/disable flag
     */
    virtual bool get_enable_histogram() const = 0;

    /*! Set the waterfall stream enable flag
     *
     * Enables or disables the stream of waterfall data from the block.
     *
     * \param enable_waterfall Histogram stream enable/disable flag
     */
    virtual void set_enable_waterfall(const bool enable_waterfall) = 0;

    /*! Get the waterfall stream enable flag
     *
     * Returns the current waterfall enable value.
     *
     * \returns Histogram stream enable/disable flag
     */
    virtual bool get_enable_waterfall() const = 0;

    /*! Clear the Fosphor block's stored history
     *
     * Clears the accumulated history in the Fosphor block, resetting
     * average and max hold values.
     */
    virtual void clear_history() = 0;

    /*! Set the dither enable flag
     *
     * Enables or disables dithering. Dithering adds quantization error
     * to the incoming signal.
     *
     * \param enable_dither Dither enable/disable flag
     */
    virtual void set_enable_dither(const bool enable_dither) = 0;

    /*! Get the dither enable flag
     *
     * Returns the current dither enable value.
     *
     * \returns Dither enable/disable flag
     */
    virtual bool get_enable_dither() const = 0;

    /*! Set the noise enable flag
     *
     * Enables or disables the addition of random noise to the incoming
     * signal.
     *
     * \param enable_noise Noise enable/disable flag
     */
    virtual void set_enable_noise(const bool enable_noise) = 0;

    /*! Get the noise enable flag
     *
     * Returns the current noise enable value.
     *
     * \returns Noise enable/disable flag
     */
    virtual bool get_enable_noise() const = 0;

    /*! Set the histogram decimation factor
     *
     * Sets the ratio of histogram outputs to FFT packet inputs.
     * For every \p decimation FFT input packets, one histogram
     * output cluster (64 histogram packets, plus a maximum and
     * average values packet) is produced. The minimum value for
     * \p decimation is 2.
     *
     * \param decimation Histogram decimation factor
     */
    virtual void set_histogram_decimation(const uint16_t decimation) = 0;

    /*! Get the histogram decimation factor
     *
     * Returns the current histogram decimation factor.
     *
     * \returns Histogram decimation factor
     */
    virtual uint16_t get_histogram_decimation() const = 0;

    /*! Set the histogram offset factor
     *
     * Sets the offset factor to apply to FFT power levels before determining
     * the appropriate histogram bin.
     *
     * \param offset The histogram offset factor to apply
     */
    virtual void set_histogram_offset(const uint16_t offset) = 0;

    /*! Get the histogram offset factor
     *
     * Returns the current histogram offset factor.
     *
     * \returns The histogram offset factor
     */
    virtual uint16_t get_histogram_offset() const = 0;

    /*! Set the histogram scale factor
     *
     * Sets the scale factor to apply to FFT power levels before determining
     * the appropriate histogram bin. The scaling factor is \p scale / 256.
     *
     * \param scale The histogram scale factor to apply
     */
    virtual void set_histogram_scale(const uint16_t scale) = 0;

    /*! Get the history scale factor
     *
     * Returns the current histogram scale factor.
     *
     * \returns The histogram scale factor
     */
    virtual uint16_t get_histogram_scale() const = 0;

    /*! Set the histogram rise rate factor
     *
     * Sets the rate at which the hit count in each frequency and power bin
     * increases when accumulating (i.e., there are hits in the particular
     * bin). The higher the value, the more quickly the values increase,
     * leading to a phosphorescent-like effect on the Fosphor display similar
     * to the gradual illumination of a CRT display in the area where the
     * electron beam is pointing.
     *
     * \param rise_rate The histogram rise rate factor to apply
     */
    virtual void set_histogram_rise_rate(const uint16_t rise_rate) = 0;

    /*! Get the histogram rise rate factor
     *
     * Returns the current histogram rise rate factor.
     *
     * \returns The histogram rise rate factor
     */
    virtual uint16_t get_histogram_rise_rate() const = 0;

    /*! Set the histogram decay rate factor
     *
     * Sets the rate at which the hit count in each frequency and power bin
     * decreases when not accumulating (i.e., there are no hits in the
     * particular bin). The lower the value, the more slowly the values
     * decrease, leading to a phosphorescent-like effect on the Fosphor
     * display similar to the gradual fading of a CRT display when the
     * electron beam is extinguished.
     *
     * \param decay_rate The histogram decay rate factor to apply
     */
    virtual void set_histogram_decay_rate(const uint16_t decay_rate) = 0;

    /*! Get the histogram decay rate factor
     *
     * Returns the current histogram decay rate factor.
     *
     * \returns The histogram decay rate factor
     */
    virtual uint16_t get_histogram_decay_rate() const = 0;

    /*! Set the power level moving average weighting
     *
     * Sets the weighing to be applied to the average power level value
     * for each FFT frequency bin. The higher the value, the higher the
     * weight is given to older samples (and thus the more slowly the average
     * values change over time in each bin).
     *
     * \param alpha The power level moving average weighting to apply
     */
    virtual void set_spectrum_alpha(const uint16_t alpha) = 0;

    /*! Get the power level moving average weighting
     *
     * Returns the weighting that is applied to older samples when calculating
     * the average power level for each FFT frequency bin.
     *
     * \returns The power level moving average weighting
     */
    virtual uint16_t get_spectrum_alpha() const = 0;

    /*! Set the maximum hold decay rate
     *
     * Sets the rate at which the maximum value for each FFT frequency
     * bin decays. The higher the value, the faster the decay rate.
     * A value of 0 retains the maximum values indefinitely.
     *
     * \param epsilon The histogram scale factor to apply
     */
    virtual void set_spectrum_max_hold_decay(const uint16_t epsilon) = 0;

    /*! Get the maximum hold decay rate
     *
     * Returns the rate at which the maximum value for each FFT frequency
     * bin decays.
     *
     * \returns The maximum hold decay rate
     */
    virtual uint16_t get_spectrum_max_hold_decay() const = 0;

    /*! Set the waterfall predivision ratio
     *
     * Sets the scaling factor applied to waterfall values.
     *
     * \param waterfall_predivision The waterfall predivision ratio to apply
     */
    virtual void set_waterfall_predivision(
        const fosphor_waterfall_predivision_ratio waterfall_predivision) = 0;

    /*! Get the waterfall predivision ratio
     *
     * Returns the current waterfall predivision ratio.
     *
     * \returns The waterfall predivision ratio
     */
    virtual fosphor_waterfall_predivision_ratio get_waterfall_predivision() const = 0;

    /*! Set the waterfall mode setting
     *
     * Sets the source of the waterfall history data. When \p waterfall_mode
     * is set to `MAX_HOLD`, the waterfall data is comprised of the max
     * power values from each FFT frequency bin. When \p waterfall_mode is set
     * to `AVERAGE`, the waterfall data is comprised of the accumulated
     * average value from each FFT frequency bin between waterfall output
     * packets.
     *
     * \param waterfall_mode The waterfall mode setting
     */
    virtual void set_waterfall_mode(const fosphor_waterfall_mode waterfall_mode) = 0;

    /*! Get the waterfall mode setting
     *
     * Returns the current waterfall mode setting.
     *
     * \returns The waterfall mode setting
     */
    virtual fosphor_waterfall_mode get_waterfall_mode() const = 0;

    /*! Set the waterfall decimation factor
     *
     * Sets the ratio of waterfall outputs to FFT packet inputs.
     * For every \p waterfall_decimation FFT input packets, one waterfall
     * output packet is produced. The minimum value for
     * \p waterfall_decimation is 2.
     *
     * \param waterfall_decimation The waterfall decimation factor to apply
     */
    virtual void set_waterfall_decimation(const uint16_t waterfall_decimation) = 0;

    /*! Get the histogram decimation factor
     *
     * Returns the current waterfall decimation factor.
     *
     * \returns The waterfall decimation factor
     */
    virtual uint16_t get_waterfall_decimation() const = 0;
};

}} // namespace uhd::rfnoc

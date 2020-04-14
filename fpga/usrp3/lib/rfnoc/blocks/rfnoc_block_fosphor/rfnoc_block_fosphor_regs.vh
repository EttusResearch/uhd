//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_fosphor_vh (Header)
//
// Description:
//
//   Fosphor RFNoC block register descriptions. See the block controller
//   (fosphor_block_control.hpp) for additional documentation.
//


// REG_ENABLE (R/W)
//
// This register enables or disables the stream of histogram data from the
// block. The streams are always generated internally and this register causes
// them to be discarded or not. This register should only be updated when the
// Fosphor block is idle to avoid enabling it while packets are in flight.
// 
// [1] : Enable waterfall output stream
// [0] : Enable histogram output stream
//
localparam REG_ENABLE = 'h00;
//
localparam REG_ENABLE_LEN = 2;

// REG_CLEAR (W)
//
// Controls reset of the Fosphor IP and clearing of the accumulated history
// (average and max hold values). Note that reset is not a superset of clear,
// and both reset and clear should not be asserted in the same write. To reset
// and clear, set only the reset bit in the first write then set only the clear
// bit in a second write.
// 
// [1] : Reset (strobe). This is a self-clearing strobe bit to reset the
//       internal Fosphor core.
// [0] : Clear (strobe). This is a self-clearing strobe bit to clear the
//       history of the fosphor core.
//
localparam REG_CLEAR = 'h04;
//
localparam REG_CLEAR_LEN = 2;
//
localparam REG_RESET_POS = 1;
localparam REG_CLEAR_POS = 0;

// REG_RANDOM (R/W)
//
// Enables or disables the addition of random noise and/or dithering to the
// incoming signal.
// 
// [1] : Noise enable. Adds random numbers to the signal.
// [0] : Dither enable. Randomizes the least-significant bits of the signal.
//
localparam REG_RANDOM = 'h08;
//
localparam REG_RANDOM_LEN = 2;

// REG_HIST_DECIM (R/W)
// 
// [11:0] : Histogram decimation. This determines the amount of histogram data
//          that is output relative to the amount of input FFT data. The actual
//          decimation is N:1 where N=VALUE+2. That is, you'll get 1 histogram
//          output packet for ever N FFT packets received, on average. However,
//          histogram data is always output as a burst of 66 packets (64
//          histogram, 1 maximum value, 1 average value).
//
localparam REG_HIST_DECIM = 'h0C;
//
localparam REG_HIST_DECIM_LEN = 12;

// REG_OFFSET (R/W)
//
// Histogram offset to apply to the FFT power levels before determining the
// appropriate histogram bin.
// 
// [15:0] : Offset
//
localparam REG_OFFSET = 'h10;
//
localparam REG_OFFSET_LEN = 16;

// REG_SCALE (R/W)
//
// Histogram scaling factor. Controls the scaling factor to apply to FFT power
// levels before determining the appropriate histogram bin. The scaling factor
// is scale / 256.
// 
// [15:0] : Scale
//
localparam REG_SCALE = 'h14;
//
localparam REG_SCALE_LEN = 16;

// REG_TRISE (R/W)
//
// Histogram rise rate. Controls the rate at which the hit count in each
// frequency bin increases when there are hits in the particular bin. The
// higher the value, the more quickly the values increase.
// 
// [15:0] : Rise time
//
localparam REG_TRISE = 'h18;
//
localparam REG_TRISE_LEN = 16;

// REG_TDECAY (R/W)
//
// Histogram decay rate. Controls the rate at which the hit count in each
// frequency and power bin decreases when there are no hits in a particular
// bin. The higher the value, the more quickly the values decrease.
// 
// [15:0] : Decay time
//
localparam REG_TDECAY = 'h1C;
//
localparam REG_TDECAY_LEN = 16;

// REG_ALPHA (R/W)
//
// Controls the weighting to be applied to the average power level value for
// each FFT frequency bin. The higher the value, the higher the weight that is
// given to older samples and the more slowly the average values change over
// time in each bin.
//
// [15:0] : Alpha
//
localparam REG_ALPHA = 'h20;
//
localparam REG_ALPHA_LEN = 16;

// REG_EPSILON (R/W)
//
// Controls the rate at which the maximum value for each FFT frequency bin
// decays. The higher the value, the faster the decay rate. A value of 0
// retains the maximum values indefinitely.
// 
// [15:0] : Epsilon
//
localparam REG_EPSILON = 'h24;
//
localparam REG_EPSILON_LEN = 16;

// REG_WF_CTRL (R/W)
// 
// Waterfall Control register
//
//   [7] : Waterfall mode. Controls the source of the waterfall history data.
//         When set to "Max Hold", the waterfall data is comprised of the max
//         power values from each frequency bin. When set to "Average", the
//         waterfall data is comprised of the accumulated average value from
//         each frequency bin between waterfall output packets. Can take on the
//         following values:
//
//           0 = Max Hold
//           1 = Average
//
// [1:0] : Waterfall pre-division. Controls the scaling factor applied to
//         waterfall values. Can take on the following values:
//
//           0 = 1:1
//           1 = 1:8
//           2 = 1:64
//           3 = 1:256
//
localparam REG_WF_CTRL = 'h28;
//
localparam REG_WF_CTRL_LEN = 8;
//
localparam REG_WF_MODE_POS = 7;
//
localparam REG_WF_DIV_POS  = 0;
localparam REG_WF_DIV_LEN  = 2;

// REG_WF_DECIM (R/W)
// 
// [7:0] : Waterfall decimation. This controls the amount of waterfall data
//         that is output relative to the amount of input FFT data. The actual
//         decimation is N:1 where N=VALUE+2. That is, you'll get 1 waterfall
//         output packet for ever N FFT packets received.
//
localparam REG_WF_DECIM = 'h2C;
//
localparam REG_WF_DECIM_LEN = 8;

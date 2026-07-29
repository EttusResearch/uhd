// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  axis_hb_utils_pkg.sv
//
// Description: Utility package for the AXI halfband filter cascade decimator/interpolator
//

package axis_hb_utils_pkg;

  // Coefficient parameters
  localparam int COEFF_WIDTH           = 18;
  localparam int COEFF_FRACTIONAL_BITS = 17; // Q1.17 format (1 sign + 17 fractional bits, range [-1.0, 1.0))
  // The HBF DC gain (sum of all coefficients interpreted as Q1.17) is approximately 2.0.
  // The coefficients were designed with the center tap representing ~1.0 in Q1.17, so the
  // passband gain is 2x unity. COEFF_GAIN_BITS = log2(2) = 1 quantifies this extra gain as
  // a bit-shift amount. Use this to scale the filter gain back to unity if needed.
  localparam int COEFF_GAIN_BITS       = 1;

  // Default halfband filter coefficients (47 taps, 18 bits each)
  localparam int HB47_NUM_COEFFS = 47;
  localparam bit [COEFF_WIDTH-1:0] HB47_COEFF_VEC[HB47_NUM_COEFFS] = '{
    {-18'sd62},
    {18'sd0},
    {18'sd194},
    {18'sd0},
    {-18'sd440},
    {18'sd0},
    {18'sd855},
    {18'sd0},
    {-18'sd1505},
    {18'sd0},
    {18'sd2478},
    {18'sd0},
    {-18'sd3900},
    {18'sd0},
    {18'sd5990},
    {18'sd0},
    {-18'sd9187},
    {18'sd0},
    {18'sd14632},
    {18'sd0},
    {-18'sd26536},
    {18'sd0},
    {18'sd83009},
    {18'sd131071},
    {18'sd83009},
    {18'sd0},
    {-18'sd26536},
    {18'sd0},
    {18'sd14632},
    {18'sd0},
    {-18'sd9187},
    {18'sd0},
    {18'sd5990},
    {18'sd0},
    {-18'sd3900},
    {18'sd0},
    {18'sd2478},
    {18'sd0},
    {-18'sd1505},
    {18'sd0},
    {18'sd855},
    {18'sd0},
    {-18'sd440},
    {18'sd0},
    {18'sd194},
    {18'sd0},
    {-18'sd62}
  };

  // Default halfband filter coefficients (63 taps, 18 bits each)
  localparam int HB63_NUM_COEFFS = 63;
  localparam bit [COEFF_WIDTH-1:0] HB63_COEFF_VEC[HB63_NUM_COEFFS] = '{
    {-18'sd35},
    {18'sd0},
    {18'sd95},
    {18'sd0},
    {-18'sd195},
    {18'sd0},
    {18'sd352},
    {18'sd0},
    {-18'sd582},
    {18'sd0},
    {18'sd907},
    {18'sd0},
    {-18'sd1354},
    {18'sd0},
    {18'sd1953},
    {18'sd0},
    {-18'sd2751},
    {18'sd0},
    {18'sd3813},
    {18'sd0},
    {-18'sd5249},
    {18'sd0},
    {18'sd7264},
    {18'sd0},
    {-18'sd10296},
    {18'sd0},
    {18'sd15494},
    {18'sd0},
    {-18'sd27083},
    {18'sd0},
    {18'sd83196},
    {18'sd131071},
    {18'sd83196},
    {18'sd0},
    {-18'sd27083},
    {18'sd0},
    {18'sd15494},
    {18'sd0},
    {-18'sd10296},
    {18'sd0},
    {18'sd7264},
    {18'sd0},
    {-18'sd5249},
    {18'sd0},
    {18'sd3813},
    {18'sd0},
    {-18'sd2751},
    {18'sd0},
    {18'sd1953},
    {18'sd0},
    {-18'sd1354},
    {18'sd0},
    {18'sd907},
    {18'sd0},
    {-18'sd582},
    {18'sd0},
    {18'sd352},
    {18'sd0},
    {-18'sd195},
    {18'sd0},
    {18'sd95},
    {18'sd0},
    {-18'sd35}
  };
  // Max number of stages supported in the cascade interpolator
  localparam int HB_INTP_MAX_NUM_HB = 3;
  // Max number of stages supported in the cascade decimator
  localparam int HB_DECIM_MAX_NUM_HB = 3;

endpackage : axis_hb_utils_pkg

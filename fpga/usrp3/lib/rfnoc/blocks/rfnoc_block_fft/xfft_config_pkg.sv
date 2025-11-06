//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: xfft_config_pkg
//
// Description:
//
//   This package helps helps build and interpret the s_axis_config_tdata bus
//   used to configure the Xilinx FFT core. The bus changes depending on the
//   parameters of the IP. See the Xilinx Fast Fourier Transform product guide
//   (PG109) for details. You can verify the values for a specific
//   configuration by clicking the Implementation tab in the Vivado IP
//   customization GUI.
//


package xfft_config_pkg;

  // Set to 1 if cyclic prefix is enabled on the Xilinx IP. Set to 0 otherwise.
  localparam bit CP_ENABLE = 0;

  localparam int MAX_W = 256;

  // Returns the width of the SCAL_SCH field of the config_tdata bus.
  function automatic int fft_scale_w(int max_fft_size_log2);
    // max_fft_size_log2 rounded up to the nearest multiple of 2
    return ((max_fft_size_log2+1) / 2) * 2;
  endfunction

  // Returns the FFT scale value needed to get the default 1/N scaling for a
  // given FFT size.
  function automatic int fft_scale_default(int size_log2);
    // Should be [ 10 10 ... 10] if N is a power of 4 (size_log2 is even)
    // Should be [ 01 10 ... 10] if N is not a power of 4 (size log2 is odd)
    int scale;
    for (int i = 0; i < (size_log2+1)/2; i++) begin
      scale[i*2 +: 2] = i == size_log2/2 ? 2'b01: 2'b10;
    end
    return scale;
  endfunction

  // Returns the width of the FWD_INV field of the config_tdata bus.
  function automatic int fft_fwd_inv_w(int max_fft_size_log2);
    return 1;
  endfunction

  // Returns the width of the CP_LEN field of the config_tdata bus.
  function automatic int fft_cp_len_w(int max_fft_size_log2);
    if (CP_ENABLE) begin
      // Always the same as NFFT if present
      return max_fft_size_log2;
    end else begin
      return 0;
    end
  endfunction

  // Returns the width of the NFFT field of the config_tdata bus.
  function automatic int fft_nfft_w(int max_fft_size_log2);
    return 5;
  endfunction

  // Returns the LSB position of the SCAL_SCH field of the config_tdata bus.
  function automatic int fft_scale_pos(int max_fft_size_log2);
    return fft_fwd_inv_pos(max_fft_size_log2) + 1;
  endfunction

  // Returns the LSB position of the FWD_INV field of the config_tdata bus.
  function automatic int fft_fwd_inv_pos(int max_fft_size_log2);
    if (CP_ENABLE) begin
      if (fft_cp_len_pos(max_fft_size_log2) + fft_cp_len_w(max_fft_size_log2) > 16)
        return 24;
      else
        return 16;
    end else begin
      return 8;
    end
  endfunction

  // Returns the LSB position of the CP_LEN field of the config_tdata bus.
  function automatic int fft_cp_len_pos(int max_fft_size_log2);
     return 8;
   endfunction

  // Returns the LSB position of the NFFT field of the config_tdata bus.
  function automatic int fft_nfft_pos(int max_fft_size_log2);
    return 0;
  endfunction

  // Returns the width of the config_tdata bus.
  function automatic int fft_config_w(int max_fft_size_log2);
    // It's the length needed to hold SCALE_SCH rounded up to the nearest byte
    return ((fft_scale_w(max_fft_size_log2) + fft_scale_pos(max_fft_size_log2) + 7) /  8) * 8;
  endfunction

  // Generates a mask of all ones that is num_bits wide.
  function automatic logic [MAX_W-1:0] mask(int num_bits);
    logic [MAX_W-1:0] bits;
    bits = (1 << num_bits) - 1;
    return bits;
  endfunction

  // Builds the config_tdata value from the provided settings.
  function automatic bit [MAX_W-1:0] build_fft_config(
    int max_fft_size_log2,
    int scale_sch,
    bit fwd_inv,
    int nfft,
    int cp_len = 0
  );
    bit [MAX_W-1:0] cfg;
    assert (max_fft_size_log2 >= 4 && max_fft_size_log2 <= 16) else
      $fatal(1, "This FFT size is not yet supported");
    assert (!CP_ENABLE || cp_len == 0) else
      $fatal(1, "Cyclic prefix must be 0 if not in use");
    cfg =
      ((scale_sch & mask(fft_scale_w  (max_fft_size_log2))) << fft_scale_pos  (max_fft_size_log2)) |
      ((fwd_inv   & mask(fft_fwd_inv_w(max_fft_size_log2))) << fft_fwd_inv_pos(max_fft_size_log2)) |
      ((cp_len    & mask(fft_cp_len_w (max_fft_size_log2))) << fft_cp_len_pos (max_fft_size_log2)) |
      ((nfft      & mask(fft_nfft_w   (max_fft_size_log2))) << fft_nfft_pos   (max_fft_size_log2));
    return cfg;
  endfunction


  //synthesis translate_off

  // Takes as input the config_tdata bus and prints the settings encoding on it.
  function automatic void print_fft_config(int max_fft_size_log2, logic [MAX_W-1:0] cfg);
    int scale_sch, fwd_inv, cp_len, nfft;

    scale_sch = (cfg >> fft_scale_pos  (max_fft_size_log2)) & mask(fft_scale_w  (max_fft_size_log2));
    fwd_inv   = (cfg >> fft_fwd_inv_pos(max_fft_size_log2)) & mask(fft_fwd_inv_w(max_fft_size_log2));
    cp_len    = (cfg >> fft_cp_len_pos (max_fft_size_log2)) & mask(fft_cp_len_w (max_fft_size_log2));
    nfft      = (cfg >> fft_nfft_pos   (max_fft_size_log2)) & mask(fft_nfft_w   (max_fft_size_log2));

    $display("SCALE_SCH_0 : 0b%0b", scale_sch);
    $display("FWD_INV_0   : %0d", fwd_inv);
    if (CP_ENABLE) $display("CP_LEN      : %0d", cp_len);
    $display("NFFT        : %0d", nfft);
  endfunction

  // The output of this function should match what's shown in the
  // Implementation Details tab of the Xilinx Fast Fourier Transform IP
  // generation wizard.
  function automatic void print_fft_config_fields(int max_fft_size_log2);
    $display("SCALE_SCH_0(%0d:%0d)  bit%0d",
      fft_scale_w(max_fft_size_log2) + fft_scale_pos(max_fft_size_log2) - 1,
      fft_scale_pos(max_fft_size_log2), fft_scale_w(max_fft_size_log2));
    $display("FWD_INV_0(%0d:%0d)  bit%0d",
      fft_fwd_inv_w(max_fft_size_log2) + fft_fwd_inv_pos(max_fft_size_log2) - 1,
      fft_fwd_inv_pos(max_fft_size_log2), fft_fwd_inv_w(max_fft_size_log2));
    if (CP_ENABLE) begin
      $display("CP_LEN(%0d:%0d)  uint%0d",
        fft_cp_len_w(max_fft_size_log2) + fft_cp_len_pos(max_fft_size_log2) - 1,
        fft_cp_len_pos(max_fft_size_log2), fft_cp_len_w(max_fft_size_log2));
    end
    $display("NFFT(%0d:%0d)  uint%0d",
      fft_nfft_w(max_fft_size_log2) + fft_nfft_pos(max_fft_size_log2) - 1,
      fft_nfft_pos(max_fft_size_log2), fft_nfft_w(max_fft_size_log2));
  endfunction

  //synthesis translate_on

endpackage : xfft_config_pkg

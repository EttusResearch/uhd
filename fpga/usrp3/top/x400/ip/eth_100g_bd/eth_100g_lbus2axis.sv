//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: eth_100g_lbus2axi
//
// Description:
//   Translate from lbus (xilinx segmented ifc) to
//   AXI4S.
//
//   Built using example provided from Xilinx
//
// Parameters:
//  - FIFO_DEPTH - FIFO will be 2** deep
//  - NUM_SEG    - Number of lbus segments coming in
//
//  Notes on timing difficulty
//   The path back to pop is challenged
//     -LBUS is popped out of the FIFO (SRL read can be slow)
//     -LBUS is rotated N to 1 Mux (N= number of segments) For 100g N=4
//     -Find where EOP is (search for the first 1)
//     -Unrotate the number of words and use that to calculate pop
//
//   Fifo Output
//     Data starts from the SRL and is indexed by the read pointer
//     Data_Valid comes from a comparison on fullness
//     Invalid control is forced to zero (necessary for algorithm)
//     It's not necessary to force all the data to zero just the control plane.
//
//   Fifo output data is rotated (4 to 1) mux then reinterpreted as lbus data
//
//   The rotated control signals are analyzed to determine
//     no_eop, no_sop, some_empty, no_ena
//
//     eop is specifically inspected in a 4in,4out function to find a pseudo
//     one hot. this is unrotated along with enable, and combined with
//     datavalid to determine the next pop, which controls incrementing of the
//     rd_pointer.
//

import PkgEth100gLbus::*;

module eth_100g_lbus2axi #(
   parameter FIFO_DEPTH = 5,
   parameter NUM_SEG = 4
)
(

  // AXIS IF
  AxiStreamIf.master    axis,

  // Lbus Segments
  input lbus_t lbus_in [NUM_SEG-1:0]

);

  localparam SEG_BYTES        = SEG_DATA_WIDTH/8;
  localparam SEG_MTY_WIDTH    = $clog2(SEG_BYTES);
  localparam SEG_SHMEAR_WIDTH = SEG_DATA_WIDTH + SEG_MTY_WIDTH + 4;

  //////////////////////////////////////////////////////////////////////////////////
  //////////////////  Data Input to FIFO                                 ///////////
  //////////////////////////////////////////////////////////////////////////////////

  lbus_t  lbus_fout_p[NUM_SEG-1:0]; //{ena,err,eop,sop,mty,data}
  lbus_t  lbus_fout[NUM_SEG-1:0]; //{ena,err,eop,sop,mty,data}
  //FIFO Logic
  logic                push;
  logic [NUM_SEG-1:0]  pop;

  logic [NUM_SEG-1:0]  full;
  logic [NUM_SEG-1:0]  empty;

  // always push the fifo on all lanes
  assign push = lbus_in[0].ena;

  // For each lane of incoming data place it into a separate FIFO
  generate
  genvar b1,gseg1;
  begin : gen_seg_fifo
    for(gseg1 = 0; gseg1 < NUM_SEG; gseg1=gseg1+1) begin

      //////////////////////////////////////////////////////////////////////////////////
      // INLINE FIFO
      //////////////////////////////////////////////////////////////////////////////////

      // simulation error if we push a full fifo
      always_comb begin
        if (push) begin
          assert (!full[gseg1]) else $error("Pushing full fifo!");
        end
      end

      // limit fanout to improve timing
      (* max_fanout = 75 *) logic [4:0] a;

      for (b1=0;b1<SEG_DATA_WIDTH;b1=b1+1) begin : gen_srl_data
        SRLC32E srl_data(
          .Q(lbus_fout_p[gseg1].data[b1]), .Q31(),
          .A(a),
          .CE(push),.CLK(axis.clk),.D(lbus_in[gseg1].data[b1])
        );
      end
      for (b1=0;b1<SEG_MTY_WIDTH;b1=b1+1) begin : gen_srl_mty
        SRLC32E srl_mty(
          .Q(lbus_fout_p[gseg1].mty[b1]), .Q31(),
          .A(a),
          .CE(push),.CLK(axis.clk),.D(lbus_in[gseg1].mty[b1])
        );
      end
      SRLC32E srl_err(
        .Q(lbus_fout_p[gseg1].err), .Q31(),
        .A(a),
        .CE(push),.CLK(axis.clk),.D(lbus_in[gseg1].err)
      );

      // empty on prebuffer and SRL
      logic my_empty;
      always @(posedge axis.clk)
      begin
        if(axis.rst) begin
          a <= 0;
          my_empty <= 1;
          full[gseg1] <= 0;
        end else if(pop[gseg1] & ~push) begin
          full[gseg1] <= 0;
          if(a==0) begin
            my_empty <= 1;
          end else begin
            a <= a - 1;
          end
        end else if(push & ~pop[gseg1]) begin
          my_empty <= 0;
          if(~my_empty) begin
            a <= a + 1;
          end
          if(a == 30) begin
            full[gseg1] <= 1;
          end
        end
      end

      // FIFO for time sensitive control signals. This creates a separate 31 deep fifo from
      // DFF's on just 3 signals.  The data signals continue to use an SRL to save space.
      // The design bellow is a FIFO followed by a single DFF regsiter that is automatically
      // prefilled when the FIFO has data.
      logic [4:0]  w_ptr,r_ptr,r_ptr_d,fullness;
      logic [31:0] ena_mem, sop_mem, eop_mem;

      // Final fifo stage after memory to remove address muxing from timing path
      // this adds one clock of latency to empty flag as it will take 2 clocks to propagate
      // into fifo.

      //push critical timing signals to final flop. This adds 1 clock of latency on
      // the final empty flag, but removes muxing of the memory elements
      logic push_dff;

      //using r_ptr_d to avoid extra latency in fullness change
      always_comb begin
        if (pop[gseg1]) begin
          r_ptr_d = r_ptr+1;
        end else begin
          r_ptr_d = r_ptr;
        end
        fullness = w_ptr-r_ptr_d;
        push_dff = (fullness != 0) & (pop[gseg1] | empty[gseg1]);
      end

      // speedier fifo implementation on these three control signals
      // THE goal of this complexity is to have the outputs be a direct FF output
      // instead of a muxed memory output.
      always @(posedge axis.clk)
      begin
        if(axis.rst) begin
          ena_mem <= '0;
          sop_mem <= '0;
          eop_mem <= '0;
          lbus_fout_p[gseg1].ena <= 1'b0;
          lbus_fout_p[gseg1].sop <= 1'b0;
          lbus_fout_p[gseg1].eop <= 1'b0;
          w_ptr <= 0;
          r_ptr <= 0;
          empty[gseg1] <= 1'b1;
        end else begin
          if(push) begin
            ena_mem[w_ptr] <= lbus_in[gseg1].ena;
            sop_mem[w_ptr] <= lbus_in[gseg1].sop;
            eop_mem[w_ptr] <= lbus_in[gseg1].eop;
            w_ptr <= w_ptr+1;
          end

          r_ptr <= r_ptr_d;

          if (push_dff) begin
            empty[gseg1] <= 1'b0;
            lbus_fout_p[gseg1].ena <= ena_mem[r_ptr_d];
            lbus_fout_p[gseg1].sop <= sop_mem[r_ptr_d];
            lbus_fout_p[gseg1].eop <= eop_mem[r_ptr_d];
          end else if (pop[gseg1]) begin
            empty[gseg1] <= 1'b1;
          end
        end
      end

      // clear the enables if this fifo segment is not valid
      always_comb begin
        //default assignment
        lbus_fout[gseg1] = lbus_fout_p[gseg1];
        if (empty[gseg1]) begin
          // clear ena,err,eop,sop,mty (But not data - saves fanout!)
          lbus_fout[gseg1].ena = 0;
          lbus_fout[gseg1].err = 0;
          lbus_fout[gseg1].eop = 0;
          lbus_fout[gseg1].sop = 0;
          lbus_fout[gseg1].mty = '0;
        end else begin
          // clear bits if the segment isn't enabled
          lbus_fout[gseg1].eop = lbus_fout_p[gseg1].eop && lbus_fout_p[gseg1].ena;
          lbus_fout[gseg1].sop = lbus_fout_p[gseg1].sop && lbus_fout_p[gseg1].ena;
          lbus_fout[gseg1].err = lbus_fout_p[gseg1].err && lbus_fout_p[gseg1].ena;
        end
      end

    end
  end : gen_seg_fifo
  endgenerate

  // post rotation lbus signals
  lbus_t lbus_rot [NUM_SEG-1:0];

  // rotated signals as vectors for decision making
  logic  [NUM_SEG-1:0]         ena;
  logic  [NUM_SEG-1:0]         sop;
  logic  [NUM_SEG-1:0]         eop;
  logic  [NUM_SEG-1:0]         rot_ena;
  logic  [NUM_SEG-1:0]         rot_sop;
  logic  [NUM_SEG-1:0]         rot_eop;
  logic  [NUM_SEG-1:0]         rot_empty;

  always_comb begin
    foreach (rot_ena[s]) begin
      ena[s] = lbus_fout[s].ena;
      sop[s] = lbus_fout[s].sop;
      eop[s] = lbus_fout[s].eop;
      rot_ena[s] = lbus_rot[s].ena;
      rot_sop[s] = lbus_rot[s].sop;
      rot_eop[s] = lbus_rot[s].eop;
    end
  end


  logic  [$clog2(NUM_SEG)-1:0] rot;

  //////////////////////////////////////////////////////////////////////////////////
  //////////////////  Generate Decision Information              ///////////
  //////////////////////////////////////////////////////////////////////////////////
  logic                        no_sop;
  logic                        no_eop;
  logic                        no_ena;
  logic                        some_empty;
  logic                        send_idle;

  always_comb begin
    no_sop     = sop == 0;
    no_eop     = eop == 0;
    no_ena     = ena == 0;
    // check for an empy byte
    some_empty = 1'b0;
    foreach (ena[seg]) begin : segment_loop
      if (ena[seg] == 0) begin
        some_empty = 1'b1;
      end
    end : segment_loop;
  end

  always_comb begin
    if (no_ena) begin
      send_idle = 1'b0;
    end else begin
      // generally either there is an EOP with some empty segments
      // or all empty segments on an unrotated bus.  I'm not sure
      // what this implies on an rotated bus
      send_idle = no_eop & some_empty;
    end
  end

  //////////////////////////////////////////////////////////////////////////////////
  //////////////////  Calculate Pop                                      ///////////
  //////////////////////////////////////////////////////////////////////////////////
  // After rotation figure out how far till eop

  //==========================================================================
  // one-hot to thermometer code
  //  The goal is to find how far down till we reach the first eop
  //  This represents the bytes we will trasnfer this clock
  //==========================================================================
  // Xilinx example
  // case (in_reqs)
  //   4'b1000: onehot2thermo = 4'b1111;
  //
  //   4'b1100: onehot2thermo = 4'b0111;
  //   4'b0100: onehot2thermo = 4'b0111;
  //
  //   4'b1110: onehot2thermo = 4'b0011;
  //   4'b0110: onehot2thermo = 4'b0011;
  //   4'b0010: onehot2thermo = 4'b0011;
  //
  //   4'b1111: onehot2thermo = 4'b0001;
  //   4'b0111: onehot2thermo = 4'b0001;
  //   4'b0011: onehot2thermo = 4'b0001;
  //   4'b0001: onehot2thermo = 4'b0001;
  //
  //   default: onehot2thermo = 4'b0000;
  // endcase
  logic [NUM_SEG-1:0] rot_xfer_now;
  logic [NUM_SEG-1:0] mask  [NUM_SEG-1:0];
  logic [NUM_SEG-1:0] m1hot [NUM_SEG-1:0];
  logic [NUM_SEG-1:0] meop  [NUM_SEG-1:0];
  logic [NUM_SEG-1:0] match;

  always_comb begin
    rot_xfer_now = '0;
    foreach (rot_eop[s]) begin
      // The function
      // XXX1=>0001
      // XX10=>0011
      // X100=>0111
      // 1000=>1111
      // MASK
      // 2**(0+1)-1 = 0001
      // 2**(1+1)-1 = 0011
      // 2**(2+1)-1 = 0111
      // 2**(3+1)-1 = 1111
      mask[s]  = 2**(s+1)-1; // Constant
      // MASK
      // 2**0 = 0001
      // 2**1 = 0010
      // 2**2 = 0100
      // 2**3 = 1000
      m1hot[s] = 2**s;  // Constant
      // Mask valid_eop
      meop[s]  = rot_eop & mask[s];
      // compare against 1hot
      match[s] = meop[s] == m1hot[s];
      if (match[s]) begin
        rot_xfer_now = mask[s];
      end
    end
  end

  // unrotate the values and calculate pop
  logic [NUM_SEG-1:0] xfer_now;
  logic [NUM_SEG-1:0] filler_seg;

  always_comb begin
    if (send_idle)
      xfer_now = '0;
    else if (no_eop | no_sop)
      xfer_now = '1;
    else
      // rotate left
      xfer_now = {rot_xfer_now,rot_xfer_now} >> (NUM_SEG - rot);
  end

  // Flush out valid segments with no enable
  assign filler_seg = ~ena & ~empty;

  assign pop = (xfer_now | filler_seg) & ~empty;

  //////////////////////////////////////////////////////////////////////////////////
  //////////////////  Calculate Rotate for the next clock                ///////////
  //////////////////////////////////////////////////////////////////////////////////
  logic  [$clog2(NUM_SEG)-1:0]  next_rot;
  always_comb begin
    next_rot = 0;
    foreach (rot_empty[s]) begin
      if (~rot_empty[s] & lbus_rot[s].sop) begin
        next_rot = s;
      end
    end
  end

  always @(posedge axis.clk)
  begin
    if(axis.rst) begin
      rot <= '0;
    //no valid data on any segment
    end else if( no_ena ) begin
      rot <= '0;
    // If EOP, but no SoP
    end else if( no_sop & ~no_eop & some_empty) begin
      rot <= '0;
    // If SOP, accumulate rotation to push to seg 0
    end else if( ~no_sop ) begin
      rot <= rot+next_rot;
    end
  end

  //////////////////////////////////////////////////////////////////////////////////
  //////////////////  Rotation of segments from fifo output             ///////////
  //////////////////////////////////////////////////////////////////////////////////
  generate
  genvar b2,gseg2;
  begin : rotate_lbus
    //perform a bitwise rotation.
    for(b2 = 0; b2 < SEG_SHMEAR_WIDTH; b2=b2+1) begin
      logic [NUM_SEG-1:0] slice, slice_rotated;

      //copy a horizontal slice across the segments
      for(gseg2 = 0; gseg2 < NUM_SEG; gseg2=gseg2+1) begin
        assign slice[gseg2] = lbus_fout[gseg2][b2];
      end

      // rotate the slice (should make SEG_SHMEAR_WIDTH copies of NUM_SEG to 1 mux)
      assign slice_rotated = {slice,slice} >> rot; //rotate_right

      // Copy slice back to the struct
      for(gseg2 = 0; gseg2 < NUM_SEG; gseg2=gseg2+1) begin
        assign lbus_rot[gseg2][b2] = slice_rotated[gseg2];
      end

    end
  end : rotate_lbus
  endgenerate

  always_comb begin : rotate_data_valid
    rot_empty = {empty,empty} >> rot; //rotate_right
  end : rotate_data_valid

  //////////////////////////////////////////////////////////////////////////////////
  //////////////////  LBUS out DFF                             /////////////////////
  //////////////////////////////////////////////////////////////////////////////////
  // This pipe stage is mainly to allow suming MTY bits and to add space for
  // Vivado to try to pipeline the output
  // post rotation lbus signals
  lbus_t lbus_out [NUM_SEG-1:0];
  logic [NUM_SEG-1:0] axi_seg_valid;

  always_ff @(posedge axis.clk)
  begin
    if (axis.rst) begin
      foreach (lbus_out[seg]) begin : segment_loop
        lbus_out[seg] <= '0;
      end
      axi_seg_valid   <= '0;
    end else begin
      lbus_out <= lbus_rot;
      if (send_idle)
        axi_seg_valid <= '0;
      else if (no_eop)
        axi_seg_valid <= '1;
      else
        axi_seg_valid <= rot_xfer_now;
      end
  end

  ////////////////////////////////////////////////////////////////////////////
  // Generate AXI
  ////////////////////////////////////////////////////////////////////////////
  logic    [axis.DATA_WIDTH - 1:0]           axis_tdata_w;
  logic    [$clog2(axis.DATA_WIDTH/8) - 1:0] axis_tuser_bytes_w;
  logic    [axis.DATA_WIDTH/8 - 1:0]         axis_tkeep_w;
  logic    [NUM_SEG-1:0]                     axis_tlast_w;
  logic    [NUM_SEG-1:0]                     axis_tvalid_w;
  logic    [NUM_SEG-1:0]                     axis_tuser_err_w;

  always_comb begin : axis_translate
    axis_tuser_bytes_w = 'd0; // init to zero before summing
    foreach (axis_tvalid_w[seg]) begin : segment_loop
      axis_tvalid_w[seg]     = lbus_out[seg].ena & axi_seg_valid[seg];
      axis_tlast_w[seg]      = lbus_out[seg].eop;
      axis_tuser_err_w[seg]  = lbus_out[seg].err;

       // sum all the segment mty vectors
      if (lbus_out[seg].ena && axi_seg_valid[seg]) begin
        axis_tuser_bytes_w += SEG_DATA_WIDTH/8 - lbus_out[seg].mty;
      end

      // 512 bit word = 64 bytes = 4 X 128 bit(16 byte) segments
      // assign bytes : LbusOrder
      // S0 : S0B0..S0B15
      // S1 : S1B0..S1B15
      // S2 : S2B0..S2B15
      // S3 : S3B0..S3B15
      // AXI (swap Endianess on each segment)
      // AXI = S3B15..S3B0, S2B15..S2B0, S1B15..S1B0, S0B15..S0B0
      for(int b = 0; b < SEG_BYTES; b=b+1) begin : tdata_loop
        //           (  1   *        128      )-8- 0*8) 120+:8 = S0B0
        //           (  1   *        128      )-8- 1*8) 112+:8 = S0B1
        // ...
        //           (  1   *        128      )-8-14*8)   8+:8 = S0B14
        //           (  1   *        128      )-8-15*8)   0+:8 = S0B15
        ////////////////////////////////////
        //           (  2   *        128      )-8- 0*8) 248+:8 = S1B0
        //           (  2   *        128      )-8- 1*8) 240+:8 = S1B1
        // ...
        //           (  2   *        128      )-8-14*8) 136+:8 = S1B14
        //           (  2   *        128      )-8-15*8) 128+:8 = S1B15
        ////////////////////////////////////
        // ...
        ////////////////////////////////////
        //           (  4   *        128      )-8- 0*8) 504+:8 = S3B0
        //           (  4   *        128      )-8- 1*8) 496+:8 = S3B1
        // ...
        //           (  4   *        128      )-8-14*8) 136+:8 = S3B14
        //           (  4   *        128      )-8-15*8) 384+:8 = S3B15
        axis_tdata_w[((seg+1)*axis.DATA_WIDTH/NUM_SEG-8-b*8) +: 8] = lbus_out[seg].data[b*8 +: 8];
      end : tdata_loop
    end : segment_loop
  end : axis_translate

  // convert bytes to keep
  always_comb begin
    axis_tkeep_w = '1;
    if (axis_tlast_w != 0 && axis_tuser_bytes_w != 0) begin
      foreach(axis_tkeep_w[b]) begin
        axis_tkeep_w[b] = axis_tuser_bytes_w > b;
      end
    end
  end

  //////////////////////////////////////////////////////////////////////////////////
  //////////////////  AXIS output flop                         /////////////////////
  //////////////////////////////////////////////////////////////////////////////////

  localparam AXIS_MTY_WIDTH = $clog2(axis.BYTES_PER_WORD);

  always_ff @(posedge axis.clk)
  begin
    if (axis.rst) begin
      axis.tdata     <= '0;
      axis.tvalid    <= 1'b0;
      axis.tlast     <= 1'b0;
      axis.tuser     <= '0;
      axis.tkeep     <= '0;
    end else begin
      axis.tdata     <= axis_tdata_w;
      axis.tvalid    <= |axis_tvalid_w;
      axis.tlast     <= |axis_tlast_w;

      if (axis.TKEEP == 1) begin
        axis.tkeep     <= axis_tkeep_w;
      end else begin
        axis.tkeep     <= 'X;
      end

      // trailing bytes in last word
      axis.tuser[AXIS_MTY_WIDTH-1:0]  <= axis_tuser_bytes_w;
      // MSB is error
      axis.tuser[AXIS_MTY_WIDTH]      <= |axis_tuser_err_w;
    end
  end

endmodule

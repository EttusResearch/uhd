//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module : axi4s_remove_bytes_start
// Description:
//   Specialized case of the Remove Bytes problem.  This
//   sub design is called when REM_START=0 and not EXACT
//
//   This implementation requires that the user field
//   holds the number of valid bytes in the word, and the MSB of the user field
//   indicates if the MAC had an error.
//
//    The block will NOT hold off the input if it goes to the BONUS State.
//
// Parameters:
//   REM_END    - Last byte to remove (Truncate case is invalid)
//

module axi4s_remove_bytes_start #(
  REM_END=3
)(
  interface   i,  // AxiStreamIf or AxiStreamPacketIf
  interface   o   // AxiStreamIf or AxiStreamPacketIf
);

  //   tUSER - always {error,numbytes}
  //                   +1   ,0 to numbytes-1 (0 is a full word)
  localparam UWIDTH = $clog2(i.BYTES_PER_WORD)+1;
  localparam ERROR = UWIDTH-1; // MSB is the error bit.

  // END is inclusive so +1
  localparam BYTES_REMOVED = REM_END+1;

  // how many bytes into the word for start and end point
  localparam END_BYTE   = REM_END  % i.BYTES_PER_WORD;
  localparam END_WORD   = REM_END  / i.BYTES_PER_WORD;

  //                    SHIFT_POINT
  //              BYTE_CARY | BYTE_SHIFT  //
  // EXAMPLES (32 bit)
  //  REM_END = 0         3 | 1  i[0]   a[3:1]
  //  REM_END = 1         2 | 2  i[1:0] a[3:2]
  //  REM_END = 2         1 | 3  i[2:0] a[3]
  //  REM_END = 3         4 | 0 // EXACT - not valid here
  //  we use BYTE_CARY bytes from a
  //  we use BYTE_SHIFT bytes from i
  localparam BYTE_SHIFT = BYTES_REMOVED % i.BYTES_PER_WORD;
  localparam BYTE_CARRY = i.BYTES_PER_WORD - BYTE_SHIFT;

  initial begin
    assert(END_BYTE != i.BYTES_PER_WORD-1 ) else
      $fatal(1,"Required to be not EXACT");
  end

 `include "axi4s.vh"

  AxiStreamPacketIf #(.DATA_WIDTH(i.DATA_WIDTH),.USER_WIDTH(i.USER_WIDTH),
    .TKEEP(0),.MAX_PACKET_BYTES(i.MAX_PACKET_BYTES))
    s0(i.clk,i.rst);
  AxiStreamPacketIf #(.DATA_WIDTH(i.DATA_WIDTH),.USER_WIDTH(i.USER_WIDTH),
    .TKEEP(0),.MAX_PACKET_BYTES(i.MAX_PACKET_BYTES))
    s1(i.clk,i.rst);
  AxiStreamPacketIf #(.DATA_WIDTH(i.DATA_WIDTH),.USER_WIDTH(i.USER_WIDTH),
    .TKEEP(0),.MAX_PACKET_BYTES(i.MAX_PACKET_BYTES))
    s2(i.clk,i.rst);

  // move from AxiStreamIfc to AxiStreamPacketIf
  always_comb begin
    `AXI4S_ASSIGN(s0,i)
  end

  //---------------------------------------
  // remove state machine
  //---------------------------------------
  typedef enum {ST_REMOVING, ST_POST_REMOVE} remove_state_t;
  remove_state_t remove_state      = ST_REMOVING;
  remove_state_t next_remove_state = ST_REMOVING;


  logic s0_wr,s1_rd;
  always_comb s0_wr = s0.tvalid && s0.tready;
  always_comb s1_rd = s1.tvalid && s1.tready;
  // FIFO to buffer up 1 previous value.  MUST be 1!
  always_ff @(posedge s1.clk) begin
    if (s1.rst) begin
      // upstream
      s1.tdata <= 0;
      s1.tuser <= 0;
      s1.tlast <= 0;
      s1.tvalid <= 0; // fullness
    end else begin

      // store data on write
      if (s0_wr) begin
        // check that we never attempt
        // a write that will overflow
        // Only 2 valid write times
        // (1) Write while empty(tvalid=0)
        // (2) Write while reading
        assert (!s1.tvalid || s1_rd) else
              $fatal(1,"Overflow write");

        s1.tdata <= s0.tdata;
        s1.tuser <= s0.tuser;
        s1.tlast <= s0.tlast;
        s1.tvalid <= 1; //full

      // if we read without a write
      // declare it empty!
      end else if (s1_rd) begin
        s1.tdata <= 'X;
        s1.tuser <= 'X;
        s1.tlast <= 1'bX;
        s1.tvalid <= 0; //empty
      end

    end
  end

  // ready to write when not full, or being read
  // passes s1_ready back cominatorially
  always_comb s0.tready = !s1.tvalid || s1_rd;

  //***************** DATA SHIFTING ***********************/
  function automatic logic [s0.DATA_WIDTH-1-BYTE_SHIFT*8:0] bs_part([s0.DATA_WIDTH-1:0] data);
    begin
      return data[s0.DATA_WIDTH-1:BYTE_SHIFT*8];
    end
  endfunction

  // Note these should all be static shifts.  We don't want to infer a barrel shifter.

  // EXAMPLE                             XX XX XX
  //   for 8 byte word    H0 G0 F0 E0 D0 C0 B0 A0 with START BYTE = 0(A0)  END_BYTE = 2(C0) BYTE_SHIFT=3
  //   1st word would be  C1 B1 A1/H0 G0 F0 E0 D0
  //                      [23:0] C1 B1 A1   / [63:24] H0 G0 F0 E0 D0
  // same as shift_data above
  // EXAMPLE              XX XX XX XX XX XX XX XX
  //   for 8 byte word    H0 G0 F0 E0 D0 C0 B0 A0
  //                                     XX XX XX
  //                      H1 G1 F1 E1 D1 C1 B1 A1 with START BYTE = 0(A0)  END_BYTE = 2(10)(C1) BYTE_SHIFT=3
  //   1st word would be  C2 B2 A2/H1 G1 F1 E1 D1
  //                      [23:0] C2 B2 A2   / [63:24] H1 G1 F1 E1 D1
  // same as shift_data above
  // NOTE: Entire words are thrown away at start, so no caching required
  logic [s0.DATA_WIDTH-1:0] shift_data;
  always_comb shift_data = {s0.tdata,bs_part(s1.tdata)};

  //-----------------------------------------------------------------------
  // user write function
  //   this module ASSUMES user includes error in the MSB and the rest is the
  // number of bytes in the word
  //-----------------------------------------------------------------------
  function automatic [UWIDTH-1:0] uwrite(error=0,[UWIDTH-2:0] bytes=0);
    begin
      return {error,bytes};
    end
  endfunction

  //-----------------------------------------------------------------------
  // get_error -extract error from tuser
  //-----------------------------------------------------------------------
  function automatic get_error([UWIDTH-1:0] tuser);
    begin
      return tuser[ERROR];
    end
  endfunction

  //-----------------------------------------------------------------------
  // get_bytes -extract num_bytes from tuser
  //-----------------------------------------------------------------------
  function automatic [UWIDTH-1:0] get_bytes([UWIDTH-1:0] tuser);
    logic [UWIDTH-1:0] bytes;
    begin
      if (tuser[UWIDTH-2:0] == 0) bytes = s0.BYTES_PER_WORD;
      else                        bytes = tuser[UWIDTH-2:0];
      return bytes;
    end
  endfunction

  logic [UWIDTH-1:0] s0_byte_count, s1_byte_count;
  always_comb s0_byte_count = get_bytes(s0.tuser);
  always_comb s1_byte_count = get_bytes(s1.tuser);

  logic s1_error_bit, s1_error_bit_old;
  logic s0_error_bit, s0_error_bit_old;

  always_ff @(posedge s0.clk) begin
    if (s0.rst) begin
      s0_error_bit_old <= 0;
      s1_error_bit_old <= 0;
    end else begin
      // clear at the end of the packet
      if (s1.tlast && s1.tvalid && s1.tready) begin
        s1_error_bit_old <= 0;
      // but they set based on the input
      end else if (s1.tvalid && s1.tready) begin
        s1_error_bit_old <= s1_error_bit;
      end
      // clear at the end of the packet
      if (s0.tlast && s0.tvalid && s0.tready) begin
        s0_error_bit_old <= 0;
      // but they set based on the input
      end else if (s1.tvalid && s1.tready) begin
        s0_error_bit_old <= s0_error_bit;
      end

    end
  end

  assign s1_error_bit = get_error(s1.tuser) && s1.tvalid || s1_error_bit_old;
  assign s0_error_bit = get_error(s0.tuser) && s0.tvalid || s0_error_bit_old;

  logic s1_reached_end;
  always_comb s1_reached_end = s1.reached_packet_byte(REM_END);

  // Remove Statemachine
  always_comb begin : remove_next_state
    // default assignment of next_state
    next_remove_state = remove_state;
    // In REM_START=0 case, always shift the same ammount.
    s2.tdata = shift_data;
    s2.tuser = s1.tuser;
    s2.tlast = s1.tlast;
    s2.tvalid = s1.tvalid;
    s1.tready = s2.tready;
    case (remove_state)
      // *****************************************************
      // REMOVING - burn words until we have data to
      // start sending again
      // *****************************************************
      ST_REMOVING: begin
        //defaults
        s2.tuser  = 0;
        s2.tlast  = 0;
        s2.tvalid = 0;
        s1.tready = 1;

        //reached removal point
        if (s1.tvalid && s1_reached_end) begin

          // if we didn't get enough bytes to fill the word
          // squelch the packet
          if (s1.tlast && s1_byte_count <= BYTE_SHIFT) begin
            s2.tlast  = 1;
            s2.tvalid = 0;
            s1.tready = 1;
          // ending in top half of word
          end else if (s0.tlast && s0_byte_count <= BYTE_SHIFT) begin
            s2.tlast  = 1;
            s2.tuser = uwrite(s1_error_bit || s0_error_bit,s0_byte_count + BYTE_CARRY);
            s2.tvalid = s0.tvalid && s1.tvalid;
            s2.tlast = 1;
            s1.tready = s2.tready && s0.tvalid;
           // ending in bottom half of word
          end else if (s1.tlast) begin
            s2.tlast  = 1;
            s2.tuser = uwrite(s1_error_bit,s1_byte_count - BYTE_SHIFT);
            s2.tvalid = s1.tvalid;
            s1.tready = s2.tready;
          // end of removal start sending data!
          end else begin
            s2.tuser  = uwrite(s1_error_bit || s0_error_bit,s0.BYTES_PER_WORD);
            s2.tvalid = s0.tvalid && s1.tvalid;
            s2.tlast = 0;
            s1.tready = s2.tready && s0.tvalid;
            if (s2.tready && s0.tvalid && s1.tvalid) begin
            next_remove_state = ST_POST_REMOVE;
            end
          end
        end

      end

      // *****************************************************
      // POST_REMOVAL waiting for end
      // *****************************************************
      ST_POST_REMOVE: begin

        //defaults
        s2.tuser  = uwrite(s1_error_bit || s0_error_bit,s0.BYTES_PER_WORD);
        s2.tvalid = s0.tvalid && s1.tvalid;
        s2.tlast = 0;
        s1.tready = s2.tready && s0.tvalid;

         // word is {i[BS-1:0],a[MSB:BS]}
         // 32 bit example
         // BS1   i[0]  : a[3:1] ->
         // BS2   i[1:0]: a[3:2]
         // BS3   i[2:0]: a[3]
        // ending in top half of word
        if (s0.tlast && s0_byte_count <= BYTE_SHIFT) begin
          s2.tuser = uwrite(s1_error_bit || s0_error_bit,s0_byte_count + BYTE_CARRY);
          s2.tvalid = s0.tvalid && s1.tvalid;
          s2.tlast = 1;
          if (s2.tready && s0.tvalid && s1.tvalid) begin
            next_remove_state = ST_REMOVING;
          end
        // ending in bottom half of word
        end else if (s1.tlast) begin
          s2.tuser = uwrite(s1_error_bit,s1_byte_count - BYTE_SHIFT);
          s2.tvalid = s1.tvalid;
          s2.tlast = 1;
          if (s2.tready && s1.tvalid) begin
            next_remove_state = ST_REMOVING;
          end
        // ending in second half of shift word
        end
      end

      // We should never get here
      default: begin
        next_remove_state = ST_REMOVING;
      end
    endcase
  end

  always_ff @(posedge s1.clk) begin
    if (s1.rst) begin
      remove_state <= ST_REMOVING;
    end else begin
      remove_state <= next_remove_state;
    end
  end

  // move from  AxiStreamPacketIf to AxiStreamIfc
  always_comb begin
    `AXI4S_ASSIGN(o,s2)
  end

endmodule : axi4s_remove_bytes_start

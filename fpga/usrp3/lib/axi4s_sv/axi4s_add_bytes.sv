//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi4s_add_bytes
//
// Description:
//
// Add zero filled bytes to a packet.
//   tUser = {error,trailing bytes};
//
//  LIMITATIONS
//    The block only adds bytes to the beginning of a word.
//
// Parameters:
//   ADD_START  - Add bytes before this point (0 means start)
//                0 is the only supported value right now
//   ADD_BYTES  - Number of bytes to add
//   SYNC       - When 1 we wait for the start word to be
//                valid before we start shifting.
//                When 0 we aggressively pad 0 early, but
//                it means the extra space may be added before
//                we setup the values we want to overwrite onto
//                that space.

module axi4s_add_bytes #(
  int ADD_START = 0,
  int ADD_BYTES = 6,
  bit SYNC      = 1
) (
   interface.slave  i,  // AxiStreamIf or AxiStreamPacketIf
   interface.master o   // AxiStreamIf or AxiStreamPacketIf
);

  localparam BYTES_PER_WORD = i.DATA_WIDTH/8;
  //   tUSER - always {error,numbytes}
  localparam UWIDTH = $clog2(BYTES_PER_WORD+1);

  //packet position in bytes of the last removed byte.
  localparam ADD_END      = ADD_START + ADD_BYTES-1;
  //packet position in bytes of the 1st byte after removal.
  localparam ADD_RESTART  = ADD_END+1;

  ////////////// Byte offsets in a word /////////////////
  localparam START_BYTE   = ADD_START   % BYTES_PER_WORD;
  localparam END_BYTE     = ADD_END     % BYTES_PER_WORD;
  localparam RESTART_BYTE = ADD_RESTART % BYTES_PER_WORD;

  // An Important shift offset
  localparam BYTE_SHIFT = (BYTES_PER_WORD - RESTART_BYTE)%BYTES_PER_WORD;
  // Subcase Recognition
  // EXACT case - the removal expression is removing an entire word
  localparam EXACT      = BYTE_SHIFT == 0;

  `include "axi4s.vh"
  // Parameter Checks
  initial begin
    assert (i.DATA_WIDTH == o.DATA_WIDTH) else
      $fatal(1, "DATA_WIDTH mismatch");
    assert (i.USER_WIDTH == o.USER_WIDTH) else
      $fatal(1, "USER_WIDTH mismatch");
    assert (i.USER_WIDTH >= UWIDTH) else
      $fatal(1, "i.USER_WIDTH is to small");
    assert (o.USER_WIDTH >= UWIDTH) else
      $fatal(1, "o.USER_WIDTH is to small");
    assert (ADD_START == 0) else
      $fatal(1, "Only tested for ADD_START = 0");
  end

  AxiStreamPacketIf #(.DATA_WIDTH(i.DATA_WIDTH),.USER_WIDTH(i.USER_WIDTH),
    .TKEEP(0),.MAX_PACKET_BYTES(i.MAX_PACKET_BYTES))
    s0(i.clk,i.rst);
  AxiStreamPacketIf #(.DATA_WIDTH(i.DATA_WIDTH),.USER_WIDTH(i.USER_WIDTH),
    .TKEEP(0),.MAX_PACKET_BYTES(i.MAX_PACKET_BYTES))
    s1(i.clk,i.rst);

  // move from AxiStreamIfc to AxiStreamPacketIf
  always_comb begin
    `AXI4S_ASSIGN(s0,i)
  end

  logic reached_start;
  logic reached_end;
  logic byte_overflow;
  logic [i.DATA_WIDTH-1:0] zero_data;
  logic [i.DATA_WIDTH-1:0] last_tdata;
  logic [i.DATA_WIDTH-1:0] remaining_shift_data;
  logic [i.DATA_WIDTH-1:0] last_shift_data;
  logic [i.DATA_WIDTH-1:0] first_shifted_data;

  logic error_bit, error_bit_old;

  // Cache a couple of words from the bus
  always_ff @(posedge s0.clk) begin
    if (s0.rst) begin
      last_tdata <= 0;
    end else if (s0.tvalid && s0.tready) begin
      last_tdata <= s0.tdata;
    end
  end

  if (EXACT) begin
    always_comb begin
      //  If END_BYTE=3
      zero_data            = 'b0;
      first_shifted_data   = s0.tdata;
      remaining_shift_data = s0.tdata;
      last_shift_data      = s0.tdata;
     end
  end else begin
    always_comb begin
      zero_data            = 'b0;
      //  If END_BYTE=2                  [7:0]                              [23:0]
      //  If END_BYTE=1                  [15:0]                             [15:0]
      //  If END_BYTE=0                  [23:0]                             [7:0]
      first_shifted_data   = {s0.tdata[BYTE_SHIFT*8-1:0],zero_data[END_BYTE*8+7:0]};
      //  If END_BYTE=0                  [23:0]                             [31:24]
      remaining_shift_data = {s0.tdata[BYTE_SHIFT*8-1:0],last_tdata[s0.DATA_WIDTH-1:BYTE_SHIFT*8]};
      //  If END_BYTE=0                  [23:0]                             [31:24]
      last_shift_data      = {zero_data[BYTE_SHIFT*8-1:0],s0.tdata[s0.DATA_WIDTH-1:BYTE_SHIFT*8]};
    end
  end

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
      return tuser[UWIDTH-1];
    end
  endfunction

  //-----------------------------------------------------------------------
  // get_bytes -extract num_bytes from tuser
  //-----------------------------------------------------------------------
  function automatic [UWIDTH-1:0] get_bytes([UWIDTH-1:0] tuser);
    logic [UWIDTH-1:0] bytes;
    begin
      if (tuser[UWIDTH-2:0] == 0) bytes = BYTES_PER_WORD;
      else                        bytes = tuser[UWIDTH-2:0];
      return bytes;
    end
  endfunction

  //---------------------------------------
  // remove state machine
  //---------------------------------------
  typedef enum {ST_PRE_ADD, ST_ADDING, ST_POST_ADD,ST_BONUS} add_state_t;

  add_state_t add_state      = ST_PRE_ADD;
  add_state_t next_add_state = ST_PRE_ADD;


  always_ff @(posedge s0.clk) begin
    if (s0.rst) begin
      error_bit_old <= 0;
    end else begin

      // must hold until output completes
      if (s1.tlast && s1.tvalid && s1.tready) begin
        error_bit_old <= 0;
      // but they set based on the input
      end else if (s0.tvalid && s0.tready) begin
        error_bit_old <= error_bit;
      end
    end
  end

  // Find the landmark bytes
  always_comb error_bit = get_error(s0.tuser) || error_bit_old;

  always_comb begin
    reached_start = s1.reached_packet_byte(ADD_START);
    reached_end   = s1.reached_packet_byte(ADD_START+ADD_BYTES);
  end

  if (EXACT) begin
    always_comb byte_overflow = 0;
  end else begin
    always_comb byte_overflow = get_bytes(s0.tuser) > BYTE_SHIFT;
  end

  // because s0.tready feeds back and generates a
  // change event for the entire interface,
  // it can trigger an infinite loop of assignment
  // even when nothing is changing.  This breaks
  // the feedback loop.
  logic s0_tready;
  always_comb s0.tready = s0_tready;

  // ADD state machine
  always_comb begin

    // default assignment of next_state
    next_add_state = add_state;
    s1.tuser = s0.tuser;
    s1.tlast = s0.tlast;
    s1.tvalid = s0.tvalid;
    s1.tdata  = first_shifted_data;
    s0_tready = s1.tready;

    case (add_state)
      // *****************************************************
      // PRE_ADD - wait till we reach ADD_START
      // *****************************************************
      ST_PRE_ADD: begin

        if (!SYNC || s0.tvalid) begin
          // reached start and end in same clock and end of word
          if (reached_start && reached_end && s0.tlast) begin

            // if final word has more bytes than we can fit.
            if (byte_overflow) begin
              s1.tlast  = 0;
              s1.tvalid = s0.tvalid;
              s0_tready = 0; // don't advance
              s1.tdata = first_shifted_data;
              s1.tuser = uwrite(error_bit,BYTES_PER_WORD);

              if (s0.tvalid && s1.tready) begin
                next_add_state = ST_BONUS;
              end
            // we can finish this clock because final word
            // didn't overflow into an additional word.
            end else begin
              s1.tlast  = 1;
              s1.tvalid = s0.tvalid;
              s0_tready = s1.tready;
              s1.tdata = first_shifted_data;
              s1.tuser = uwrite(error_bit,get_bytes(s0.tuser) + RESTART_BYTE);
              // NO state advance
            end
          // reached start and end, and not the end of the packet
          end else if (reached_start && reached_end && !s0.tlast) begin
            s1.tlast  = 0;
            s1.tvalid = s0.tvalid;
            s0_tready = s1.tready;
            s1.tdata = first_shifted_data;
            s1.tuser = uwrite(error_bit,BYTES_PER_WORD);

            if (s0.tvalid && s1.tready) begin
              next_add_state = ST_POST_ADD;
            end

          // reached start but not the end of byte insertion
          end else if (reached_start && !reached_end) begin
            s1.tlast  = 0;
            s1.tvalid = 1;
            s0_tready = 0; // don't advance
            s1.tdata = zero_data;
            s1.tuser = uwrite(0,BYTES_PER_WORD);

            if (s1.tready) begin
              next_add_state = ST_ADDING;
            end

          end
        end
      end //ST_PRE_REMOVE


      // *****************************************************
      // REMOVING - burn words until we have data to
      // start sending again
      // *****************************************************
      ST_ADDING: begin
        //defaults
        s1.tlast  = 0;
        s1.tvalid = 1;
        s0_tready = 0; // don't advance
        s1.tdata  = zero_data;
        s1.tuser  = uwrite(0,BYTES_PER_WORD);

        // reached the end of incoming packet and data insertion
        if (reached_end && s0.tlast) begin
          // if final word has more bytes than we can fit.
          if (byte_overflow) begin
            s1.tlast  = 0;
            s1.tvalid = s0.tvalid;
            s0_tready = 0; // don't advance
            s1.tdata = first_shifted_data;
            s1.tuser = uwrite(error_bit,BYTES_PER_WORD);

            if (s0.tvalid && s1.tready) begin
              next_add_state = ST_BONUS;
            end
          end else begin
          // we can finish this clock because final word
          // didn't overflow into an additional word.
            s1.tlast  = 1;
            s1.tvalid = s0.tvalid;
            s0_tready = s1.tready;
            s1.tdata = first_shifted_data;
            s1.tuser = uwrite(error_bit,get_bytes(s0.tuser) + RESTART_BYTE);

            if (s0.tvalid && s1.tready) begin
              next_add_state = ST_PRE_ADD;
            end
          end

        //  reached the end of data insertion - not end of packet
        end else if (reached_end && !s0.tlast) begin
          s1.tlast  = 0;
          s1.tvalid = s0.tvalid;
          s0_tready = s1.tready;
          s1.tdata = first_shifted_data;
          s1.tuser = uwrite(error_bit,BYTES_PER_WORD);

          if (s0.tvalid && s1.tready) begin
            next_add_state = ST_POST_ADD;
          end

        end
      end
      // *****************************************************
      // POST_ADD waiting for end
      // *****************************************************
      ST_POST_ADD: begin
        //defaults
        s1.tlast  = 0;
        s1.tvalid = s0.tvalid;
        s0_tready = s1.tready;
        s1.tdata  = remaining_shift_data;
        s1.tuser  = uwrite(error_bit,BYTES_PER_WORD);
        // reached the end, but we have extra bytes to send
        if (s0.tlast && byte_overflow) begin
          s1.tlast = 0;
          s0_tready = 0; // don't let a advance

          if (s0.tvalid && s1.tready) begin
            next_add_state = ST_BONUS;
          end

        // reached the end, and don't need the bonus state
        end else if (s0.tlast) begin
          s1.tlast = 1;
          s1.tuser = uwrite(error_bit,get_bytes(s0.tuser) + RESTART_BYTE);

          if (s1.tready && s0.tvalid) begin
            next_add_state = ST_PRE_ADD;
          end

        end
      end

      // *****************************************************
      // BONUS write out any overflow words
      // *****************************************************
      ST_BONUS: begin
        //defaults
        s1.tdata  = last_shift_data;
        s1.tuser  = uwrite(error_bit,get_bytes(s0.tuser)+ RESTART_BYTE);
        s1.tlast  = 1;
        s1.tvalid = s0.tvalid;
        s0_tready = s1.tready;

        if (s1.tready && s0.tvalid) begin
          next_add_state = ST_PRE_ADD;
        end

      end

      // We should never get here
      default: begin
        next_add_state = ST_PRE_ADD;
      end
    endcase
  end

  always_ff @(posedge s0.clk) begin
    if (s0.rst) begin
      add_state <= ST_PRE_ADD;
    end else begin
      add_state <= next_add_state;
    end
  end

  always_comb begin
    `AXI4S_ASSIGN(o,s1)
  end


endmodule : axi4s_add_bytes

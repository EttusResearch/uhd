//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module : axi4s_remove_bytes
//
// Description:
//   Remove bytes from a packet.  1 removal can happen per
//   packet.  The removal is made by delaying the output
//   by a clock, and then combining the new and old word
//   and providing a combination of shifted words.
//   This implementation requires that the user field
//   holds the number of valid bytes in the word, and the MSB of the user field
//   indicates if the MAC had an error.
//
//   The block will hold off the input if it goes to the BONUS State.
//
//   This block is intended to remove data from the beginning or middle
//   of a packet.  You can truncate a packet by setting REM_END to -1.
//
//  LIMITATIONS
//    The block will set the error bit if you put in a packet between
//    REM_END and REM_START length, and it is unable to cleanly signal
//    and end to the packet. (there is no way to send a zero byte valid
//    packet using tuser protocol.
//    Packets must be terminated with tlast.
//
// Parameters:
//   REM_START  - First byte to remove (0 means start)
//   REM_END    - Last byte to remove (-1 means truncate from REM START)
//

module axi4s_remove_bytes #(
  REM_START=0,
  REM_END=8
)(
   interface.slave  i,  // AxiStreamIf or AxiStreamPacketIf
   interface.master o   // AxiStreamIf or AxiStreamPacketIf
);

  localparam BYTES_PER_WORD = i.DATA_WIDTH/8;
  //   tUSER - always {error,numbytes}
  localparam UWIDTH = $clog2(BYTES_PER_WORD+1);
  localparam ERROR = UWIDTH-1; // MSB is the error bit.

  localparam TRUNCATE = REM_END < 0;
  // END is inclusive so +1
  localparam BYTES_REMOVED = TRUNCATE ? 1 :
                                        REM_END-REM_START+1;

  // how many bytes into the word for start and end point
  localparam START_BYTE = REM_START% BYTES_PER_WORD;
  localparam START_WORD = REM_START/ BYTES_PER_WORD;
  localparam END_BYTE   = TRUNCATE ? BYTES_PER_WORD-1 :
                                     REM_END  % BYTES_PER_WORD;
  localparam END_WORD   = TRUNCATE ? 65535 : // max word counter value
                                     REM_END  / BYTES_PER_WORD;

  localparam FIRST_BYTE_AFTER = (END_BYTE+1) % BYTES_PER_WORD;

  localparam BYTE_SHIFT = BYTES_REMOVED % BYTES_PER_WORD;
  localparam BYTE_CARRY = BYTES_PER_WORD - BYTE_SHIFT;
  // CASE differentiators
  localparam SINGLE       = BYTES_REMOVED <= BYTES_PER_WORD;
  localparam START_AT_LSB = START_BYTE == 0;
  localparam END_AT_MSB   = END_BYTE == BYTES_PER_WORD-1;
  localparam EXACT        = START_AT_LSB && END_AT_MSB;
  localparam MIDDLE       = END_BYTE >= START_BYTE;

  `include "axi4s.vh"

  // Parameter Checks
  initial begin
    assert (i.DATA_WIDTH == o.DATA_WIDTH) else
      $fatal(1, "DATA_WIDTH mismatch");
  end

  AxiStreamPacketIf #(.DATA_WIDTH(i.DATA_WIDTH),.USER_WIDTH(i.USER_WIDTH),
    .TKEEP(0),.MAX_PACKET_BYTES(i.MAX_PACKET_BYTES))
    s0(i.clk,i.rst);
  AxiStreamPacketIf #(.DATA_WIDTH(i.DATA_WIDTH),.USER_WIDTH(i.USER_WIDTH),
    .TKEEP(0),.MAX_PACKET_BYTES(i.MAX_PACKET_BYTES))
    s1(i.clk,i.rst);


  // implement specialized cases
  if (REM_START == 0 && !EXACT) begin : start_not_exact

    // START at zero but still shifted
    axi4s_remove_bytes_start #(
      .REM_END(REM_END)
    ) axi4s_remove_bytes_start_i (
      .i(i), .o(o)
    );

  end else begin : general

    // move from AxiStreamIfc to AxiStreamPacketIf
    always_comb begin
      `AXI4S_ASSIGN(s0,i)
    end

    typedef enum {MS_EXACT, MS_START_AT_LSB, MS_END_AT_MSB,
      SINGLE_MIDDLE,MULTI_MIDDLE, MS_WRAP} case_t;
    case_t MCASE;

    logic reached_start;
    logic reached_end;
    logic reached_end_plus;
    // memory for holding old values
    logic [s0.DATA_WIDTH-1:0] last_tdata;
    logic [s0.DATA_WIDTH-1:0] first_tdata;
    logic [UWIDTH-1:0] first_tuser;

    // various flavors of data shifting
    logic [s0.DATA_WIDTH-1:0] trunc_data;
    logic [s0.DATA_WIDTH-1:0] remaining_shift_data;
    logic [s0.DATA_WIDTH-1:0] prefirst_shifted_data;
    logic [s0.DATA_WIDTH-1:0] first_shifted_data;
    logic [s0.DATA_WIDTH-1:0] one_word_data;
    logic [s0.DATA_WIDTH-1:0] bonus_data;

    logic [15:0] word_count;  // Oversized to 65536 words
    logic error_bit, error_bit_old;
    logic [UWIDTH-1:0] in_byte_count;

    //---------------------------------------
    // remove state machine
    //---------------------------------------
    typedef enum {ST_PRE_REMOVE, ST_TRUNCATE, ST_REMOVING,
      ST_POST_REMOVE, ST_BONUS} remove_state_t;
    remove_state_t remove_state      = ST_PRE_REMOVE;
    remove_state_t next_remove_state = ST_PRE_REMOVE;

    always_comb in_byte_count = get_bytes(s0.tuser);

    // Cache a couple of words from the bus
    always_ff @(posedge s0.clk) begin
      if (s0.rst) begin
        last_tdata = 0;
        first_tdata = 0;
        first_tuser = 0;
      end else
      if (s0.tvalid && s0.tready &&
         (MCASE == MULTI_MIDDLE || MCASE==MS_START_AT_LSB))
        last_tdata = s0.tdata;
      if (s0.tvalid && s0.tready &&
         (reached_start || next_remove_state==ST_POST_REMOVE ||
          (remove_state!=ST_REMOVING && remove_state!= ST_TRUNCATE))) begin
        first_tdata = s0.tdata;
        first_tuser = s0.tuser;
      end
    end

    //***************** DATA SHIFTING CASES ***********************/

    //-----------------------------------------------------------------------
    // user write function
    //   this module ASSUMES user includes error in the MSB and the rest is the
    // number of bytes in the word
    //-----------------------------------------------------------------------
    function automatic logic [START_BYTE*8-1:0] start_part([s0.DATA_WIDTH-1:0] data);
      begin
        // workaround :: modelsim optimizer can fail if there is a possibility of a 0+:0
        localparam MY_START_BYTE = START_BYTE ? START_BYTE : 1;
        return data[0+:MY_START_BYTE*8];
      end
    endfunction


    function automatic logic [s0.DATA_WIDTH-1-FIRST_BYTE_AFTER*8:0] end_part([s0.DATA_WIDTH-1:0] data);
      begin
        return data[s0.DATA_WIDTH-1:FIRST_BYTE_AFTER*8];
      end
    endfunction

    function automatic logic [s0.DATA_WIDTH-1-BYTE_SHIFT*8:0] bs_part([s0.DATA_WIDTH-1:0] data);
      begin
        return data[s0.DATA_WIDTH-1:BYTE_SHIFT*8];
      end
    endfunction

//    Examples
//
//    ENDING CASE 1
//    Incoming packet               outgoing packet
//  ///////////////////////////////////////////////
//    D0 C0 B0 A0 <- word 0
//    D1 XX XX XX <- R(6:4))        D0 C0 B0 A0
//    D2 C2 B2 A2                   C2 B2 A2 D1
//    D0 C0 B0 A0 <- next packet             D2
//                                  D0 C0 B0 A0
//
//    ENDING CASE2
//    Incoming packet               outgoing packet
//  ///////////////////////////////////////////////
//    D0 C0 B0 A0 <- word 0
//    D1 XX XX XX <- R(6:4))        D0 C0 B0 A0
//       C2 B2 A2                   C2 B2 A2 D1
//    D0 C0 B0 A0 <- next packet
//                                  D0 C0 B0 A0
//    Middle of Word case
//    Incoming packet               outgoing packet
//  ///////////////////////////////////////////////
//    D0 C0 B0 A0 <- word 0
//    D1 XX XX A1 <- R(7:6)         D0 C0 B0 A0
//    D2 C2 B2 A2                   B2 A2 D1 A1
//    D3 C3 B3 A3                   B3 A3 D2 C2
//    D0 C0 B0 A0 <- next packet          D3 C3
//
//    Easy Truncation (can handle dynamically)
//    Incoming packet               outgoing packet
//  ///////////////////////////////////////////////
//    D0 C0 B0 A0 <- word 0
//    D1 C1 B1 A1                   D0 C0 B0 A0
//       XX XX XX <- R(11:8))       D1 C1 B1 A1 <- TLAST HERE
//    D0 C0 B0 A0 <- next packet
//                                  D0 C0 B0 A0
//    Truncation case requiring REM_END=-1
//     because last word is to far away to see tlast.
//    Incoming packet               outgoing packet
//  ///////////////////////////////////////////////
//    D0 C0 B0 A0 <- word 0
//    XX XX XX XX <- R(-1:4))        D0 C0 B0 A0 <- TLAST HERE
//    XX XX XX XX
//    XX XX XX XX
//    D0 C0 B0 A0 <- next packet
//                                   D0 C0 B0 A0
//    Remove from Front
//    Incoming packet               outgoing packet
//  ///////////////////////////////////////////////
//    XX XX XX XX <- R(0:7)
//    XX XX XX XX <-
//       C2 B2 A2
//    D0 C0 B0 A0 <- next packet        C2 B2 A2
//                                   D0 C0 B0 A0
//
//    Remove 1 byte on back to back 1 word packets
//    Incoming packet               outgoing packet
//  ///////////////////////////////////////////////
//    D0 C0 XX A0 <- R(1:1)
//    D0 C0 XX A0 <- R(1:1)             D0 C0 A0
//    D0 C0 XX A0 <- R(1:1)             D0 C0 A0
//                                      D0 C0 A0
//
//

    // Note these should all be static shifts.  We don't want to infer a barrel shifter.
    if (EXACT) begin // Remove whole words
      always_comb begin
        MCASE = MS_EXACT;
        first_shifted_data   = s0.tdata;
        remaining_shift_data = s0.tdata;
        one_word_data        = s0.tdata;
        trunc_data           = first_tdata;
        bonus_data           = 'bX;
      end
    end else if (START_AT_LSB) begin // Remove start of word shift case
      // EXAMPLE                             XX XX XX
      //   for 8 byte word    H0 G0 F0 E0 D0 C0 B0 A0 with START BYTE = 0(A0)  END_BYTE = 2(C0) BYTE_SHIFT=3
      //   1st word would be  C1 B1 A1/H0 G0 F0 E0 D0
      //                      [23:0] C1 B1 A1   / [63:24] H0 G0 F0 E0 D0
      // same as remaining_shift_data above
      // EXAMPLE              XX XX XX XX XX XX XX XX
      //   for 8 byte word    H0 G0 F0 E0 D0 C0 B0 A0
      //                                     XX XX XX
      //                      H1 G1 F1 E1 D1 C1 B1 A1 with START BYTE = 0(A0)  END_BYTE = 2(10)(C1) BYTE_SHIFT=3
      //   1st word would be  C2 B2 A2/H1 G1 F1 E1 D1
      //                      [23:0] C2 B2 A2   / [63:24] H1 G1 F1 E1 D1
      // same as remaining_shift_data above
      // NOTE: Entire words are thrown away at start, so no caching required
      always_comb begin
        MCASE = MS_START_AT_LSB;
        first_shifted_data   = {s0.tdata,bs_part(last_tdata)};
        if (BYTE_SHIFT==0)
          remaining_shift_data = s0.tdata;
        else
          remaining_shift_data = {s0.tdata,bs_part(last_tdata)};
        bonus_data           = 'bX;
        one_word_data        = end_part(s0.tdata);
        trunc_data           = first_tdata;
        bonus_data           = 'bX;
        bonus_data           = bs_part(s0.tdata);
      end
    end else if (END_AT_MSB) begin // Remove end of word shift case
      // EXAMPLE              XX XX
      //   for 8 byte word    H0 G0 F0 E0 D0 C0 B0 A0 with START BYTE = 6(G0)  END_BYTE = 7(H0) BYTE_SHIFT=2
      //   1st word would be  B1 A1/F0 E0 D0 C0 B0 A0
      //                      [15:0] B1 A1 / [47:0] F0 E0 D0 C0 B0 A0
      // EXAMPLE              XX XX
      //   for 8 byte word    H0 G0 F0 E0 D0 C0 B0 A0
      //                      XX XX XX XX XX XX XX XX
      //                      H1 G1 F1 E1 D1 C1 B1 A1 with START BYTE = 6(G0)  END_BYTE = 7(15)(H0) BYTE_SHIFT=2
      //   1st word would be  B2 A2/F0 E0 D0 C0 B0 A0
      // NOTE: Uses 1st Data (from when we reach the first word
      //                      [15:0] B2 A2 / [47:0] F0 E0 D0 C0 B0 A0
      always_comb begin
        MCASE = MS_END_AT_MSB;
        first_shifted_data   = {s0.tdata,start_part(first_tdata)};
        if (BYTE_SHIFT==0)
          remaining_shift_data = s0.tdata;
        else
          remaining_shift_data = {s0.tdata,bs_part(first_tdata)};
        one_word_data        = s0.tdata;
        trunc_data           = first_tdata;
        bonus_data           = 'bX;
        bonus_data           = bs_part(s0.tdata);
      end
    end else if(MIDDLE) begin // Remove middle of word shift case
      // EXAMPLE                 XX XX XX XX XX XX
      //   for 8 byte word    H0 G0 F0 E0 D0 C0 B0 A0 with START BYTE = 1(B0)  END_BYTE = 6(G0) BYTE_SHIFT=6
      //   1st word would be  F1 E1 D1 C1 B1 A1/H0/A0
      //                      [47:0] F1 E1 D1 C1 B1 A1                   [63:56] H0                                [7:0] A0
      // EXAMPLE              XX XX XX XX XX XX XX
      //   for 8 byte word    H0 G0 F0 E0 D0 C0 B0 A0
      //                         XX XX XX XX XX XX XX
      //                      H1 G1 F1 E1 D1 C1 B1 A1 with START BYTE = 1(B0)  END_BYTE = 6(14)(G0) BYTE_SHIFT=6
      //   1st word would be  F2 E2 D2 C2 B2 A2/H1/A0
      // NOTE: Uses first Data from when we reach the first word.
      //       Also, must advance one clock beyond end for this case.
      //                      [47:0] F2 E2 D2 C2 B2 A2               [63:56] H1                           [7:0] A0
      //   remaining words    F2 E2 D2 C2 B2 A2/H1 G1
      //                     [47:0] F2 E2 D2 C2 B2 A2  [63:48] H1 G1
      // same as remaining_shift_data above
      always_comb begin
        if (SINGLE) begin
          MCASE = SINGLE_MIDDLE;
          first_shifted_data    = {s0.tdata,end_part(first_tdata),start_part(first_tdata)};
        end else begin
          MCASE = MULTI_MIDDLE;
          prefirst_shifted_data = {end_part(s0.tdata),start_part(first_tdata)};
          first_shifted_data    = {s0.tdata,end_part(last_tdata),start_part(first_tdata)};
        end
        if (BYTE_SHIFT==0)
          remaining_shift_data = s0.tdata;
        else
          remaining_shift_data = {s0.tdata,bs_part(first_tdata)};
        one_word_data        = {end_part(s0.tdata),start_part(s0.tdata)};
        trunc_data           = first_tdata;
        bonus_data           = 'bX;
        bonus_data           = bs_part(s0.tdata);
      end
    end else begin //wrapped case
      // EXAMPLE              XX XX
      //   for 8 byte word    H0 G0 F0 E0 D0 C0 B0 A0 with START BYTE = 6(G0)  END_BYTE = 2(10)(C1) BYTE_SHIFT=5
      //                                     XX XX XX
      //                      H1 G1 F1 E1 D1 C1 B1 A1
      //
      //   1st word would be  E1 D1/F0 E0 D0 C0 B0 A0
      //                      [39:24] E1 D1 / [47:0] F0 E0 D0 C0 B0 A0
      //   remaining words    E2 D2 C2 B2 A2/H1 G1 F1
      //                      [39:0] E2 D2 C2 B2 A2 / H1 G1 F1 [63:40]
      //   same as remaining_shift_data_above      // EXAMPLE              XX XX
      //   for 8 byte word    H0 G0 F0 E0 D0 C0 B0 A0
      //                      XX XX XX XX XX XX XX XX
      //                      H1 G1 F1 E1 D1 C1 B1 A1
      //                                     XX XX XX
      //                      H2 G2 F2 E2 D2 C2 B2 A2 with START BYTE = 6(G0)  END_BYTE = 2(10)(C1) BYTE_SHIFT=5
      //
      //   1st word would be  E2 D2/F0 E0 D0 C0 B0 A0
      // NOTE: Uses 1st Data (from when we reach the first word                                                    ;
      //                      [39:24] E2 D2 /                                   [47:0] F0 E0 D0 C0 B0 A0
      always_comb begin
        MCASE = MS_WRAP;
        first_shifted_data   = {end_part(s0.tdata),start_part(first_tdata)};
        if (BYTE_SHIFT==0)
          remaining_shift_data = s0.tdata;
        else
          remaining_shift_data = {s0.tdata,bs_part(first_tdata)};
        one_word_data        = s0.tdata;
        trunc_data           = first_tdata;
        bonus_data           = 'bX;
        bonus_data           = bs_part(s0.tdata);

      end
    end


    typedef enum {PASS_THRU,BONUS,REM_SHIFT_DATA,FIRST_SHIFT_DATA,
      PREFIRST_SHIFT_DATA,TRUNCATE_DATA,ONE_WORD} data_mux_sel_t;
    data_mux_sel_t data_mux_sel = PASS_THRU;

    always_comb begin : data_mux
      s1.tdata =  s0.tdata;
      case (data_mux_sel)
        PASS_THRU           : s1.tdata = s0.tdata;
        ONE_WORD            : s1.tdata = one_word_data;
        FIRST_SHIFT_DATA    : s1.tdata = first_shifted_data;
        PREFIRST_SHIFT_DATA : if (MCASE==MULTI_MIDDLE)
                                s1.tdata = prefirst_shifted_data;
                              else
                                s1.tdata = first_shifted_data;
        REM_SHIFT_DATA      : if (!TRUNCATE)
                                s1.tdata = remaining_shift_data;
        TRUNCATE_DATA       : if (TRUNCATE)
                                s1.tdata = trunc_data;
        BONUS               : if (!TRUNCATE && !EXACT)
                                s1.tdata = bonus_data;
        default             : s1.tdata = s0.tdata;
      endcase
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
        return tuser[ERROR];
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

    // Debug state used to determine which sub-case is taken in simulation
    typedef enum {D_IDLE, D_REACHED_START, D_TRUNCATE, D_LAST, D_NOT_LAST,
      D_LAST_WO_END, D_LAST_W_END, D_LAST_W_END_BONUS, D_LAST_W_END_PLUS,
      D_REACHED_END_PLUS} debug_t;
    debug_t debug = D_IDLE;

    always_ff @(posedge s0.clk) begin
      if (s0.rst) begin
        error_bit_old <= 0;
      end else begin
        // must hold until bonus completes
        if (s1.tlast && s1.tvalid && s1.tready && remove_state==ST_BONUS) begin
          error_bit_old <= 0;
        // or clear if not going to bonus
        end else if (s0.tlast && s0.tvalid && s0.tready && next_remove_state!=ST_BONUS) begin
          error_bit_old <= 0;
        // but they set based on the input
        end else if (s0.tvalid && s0.tready) begin
          error_bit_old <= error_bit;
        end
      end
    end

    assign error_bit = get_error(s0.tuser) || error_bit_old;

    // When truncating we want to hold the last valid word until
    // the end so we can accumulate any errors that might of occured
    if (TRUNCATE && START_BYTE==0) begin
      always_comb reached_start = s0.reached_packet_byte(REM_START-1);
    end else begin
      always_comb reached_start = s0.reached_packet_byte(REM_START);
    end

    // the WRAP case leans forward one word since it bridges to
    // the next word so it needs to reach end_plus early
    always_comb begin : reached_end_comb
      if (MCASE==MS_WRAP) begin
        reached_end       = s0.reached_packet_byte(REM_END);
        reached_end_plus  = s0.reached_packet_byte(REM_END);
      end else begin
        reached_end       = s0.reached_packet_byte(REM_END);
        reached_end_plus  = s0.reached_packet_byte(REM_END+BYTES_PER_WORD);
      end
    end

    // because s0.tready feeds back and generates a
    // change event for the entire interface,
    // it can trigger an infinite loop of assignment
    // even when nothing is changing.  This breaks
    // the feedback loop.
    logic s0_tready;
    always_comb s0.tready = s0_tready;

    // Remove State Machine
    always_comb begin : remove_next_state
      // default assignment of next_state
      next_remove_state = remove_state;
      debug = D_IDLE;
      data_mux_sel = PASS_THRU;
      s1.tuser = s0.tuser;
      s1.tlast = s0.tlast;
      s1.tvalid = s0.tvalid;
      s0_tready = s1.tready;
      case (remove_state)

        // *****************************************************
        // PRE_REMOVE - wait till we reach REM_START
        // *****************************************************
        ST_PRE_REMOVE: begin
          //defaults
          data_mux_sel = PASS_THRU;
          s1.tuser = s0.tuser;
          s1.tlast = s0.tlast;
          s1.tvalid = s0.tvalid;
          s0_tready = s1.tready;

          if (reached_start) begin // based only on word count
            debug = D_REACHED_START;

            // Truncating word end
            if (TRUNCATE && s0.tlast) begin
              s1.tlast = 1;
              data_mux_sel = ONE_WORD;
              debug = D_TRUNCATE;

              // get number of bytes based on if we had enough to surpass END_BYTE
              if (START_BYTE == 0) // Exact case
                s1.tuser = uwrite(error_bit,in_byte_count);
              else if (in_byte_count < START_BYTE)
                s1.tuser = uwrite(error_bit,in_byte_count);
              else
                s1.tuser = uwrite(error_bit,START_BYTE);

            end else if (TRUNCATE && !s0.tlast) begin
              s1.tlast = 1;
              data_mux_sel = ONE_WORD;
              debug = D_TRUNCATE;

              if (s1.tready && s0.tvalid) begin
                s1.tlast  = 0;
                s1.tvalid = 0;
                next_remove_state = ST_TRUNCATE;
              end

            // packet ends
            end else if (s0.tlast) begin
              s1.tlast = 1;
              data_mux_sel = ONE_WORD;
              debug = D_LAST;
              // get number of bytes based on if we had enough to surpass END_BYTE
              if (in_byte_count < START_BYTE)
                s1.tuser = uwrite(error_bit,in_byte_count);
              else if (START_WORD != END_WORD)
                s1.tuser = uwrite(error_bit,START_BYTE);
              else if (in_byte_count < END_BYTE+1)
                s1.tuser = uwrite(error_bit,START_BYTE);
              else
                s1.tuser = uwrite(error_bit,in_byte_count - BYTES_REMOVED);

              // if we are on the first word of the removal and have no way to terminate the packet
              // set error.
              if ((START_WORD != END_WORD && START_BYTE == 0) || EXACT)
                s1.tuser[ERROR] = 1;

              // if removal starts at the start of the packet, squelch the packet.
              if ( (START_WORD != END_WORD && REM_START == 0) ||
                  // also if we don't have enough data to publish 1 byte
                   ((in_byte_count <= END_BYTE+1) && REM_START == 0)) begin
                s1.tlast = 0;
                s1.tvalid = 0;
              end

            end else begin // not the last word
              debug = D_NOT_LAST;

              s1.tvalid = 0;
              if (s0.tvalid) begin
                // we will always need to wait for some more data before
                // forming the next word if this was not the start of the packet
                next_remove_state = ST_REMOVING;
              end
            end
          end
        end //ST_PRE_REMOVE

        // *****************************************************
        // TRUNCATE - wait for end of packet to put out the
        // last word (so we can see if error bit asserts)
        // *****************************************************
        ST_TRUNCATE: begin

          if (TRUNCATE) begin // Simplify synthesis
            // get number of bytes based on if we had enough to surpass END_BYTE
            if (get_bytes(first_tuser) < START_BYTE)
              s1.tuser = uwrite(error_bit,get_bytes(first_tuser));
            else
              s1.tuser = uwrite(error_bit,START_BYTE);

            data_mux_sel = TRUNCATE_DATA;
            s1.tlast  = s0.tlast;
            s1.tvalid = s0.tlast && s0.tvalid;
            s0_tready = 1;
            if (s1.tready && s0.tvalid && s0.tlast) begin
              next_remove_state = ST_PRE_REMOVE;
            end
          end
        end

        // *****************************************************
        // REMOVING - burn words until we have data to
        // start sending again
        // *****************************************************
        ST_REMOVING: begin
          //defaults
          data_mux_sel = FIRST_SHIFT_DATA;
          s1.tuser  = 0;
          s1.tlast  = 0;
          s1.tvalid = 0;
          s0_tready = 1;

          // if we don't reach the end of the removal
          // it is an error case because we don't
          // have any valid data to send with the tlast.
          if (s0.tlast && !reached_end && !reached_end_plus) begin
            debug = D_LAST_WO_END;
            s1.tuser  = uwrite(1,0);
            s1.tlast  = 1;
            s1.tvalid = s0.tvalid;
            s0_tready = s1.tready;

            // started from zero we have pushed
            // zero data so we can just squelch the packet
            if (REM_START==0)
              s1.tvalid = 0;

            if (s1.tready && s0.tvalid) begin
              next_remove_state = ST_PRE_REMOVE;
            end

          // end of packet and we have some data to send
          // but we didn't buffer the extra word of end data yet
          end else if (s0.tlast && reached_end && !reached_end_plus) begin
            debug = D_LAST_W_END;

            if (MCASE==MULTI_MIDDLE)
              data_mux_sel = PREFIRST_SHIFT_DATA;
            else if (MCASE==MS_START_AT_LSB)
              data_mux_sel = ONE_WORD;
            s1.tlast  = 1;

            // if we are exact and started from zero we have pushed
            // zero data so we can just squelch the packet
            if (EXACT && REM_START==0)
              s1.tvalid = 0;
            else
              s1.tvalid = s0.tvalid;

            s0_tready = s1.tready;

            if (MCASE==MULTI_MIDDLE)
              if (in_byte_count <= FIRST_BYTE_AFTER)
                s1.tuser = uwrite(1,0); // not enough data to avoid error
              else
                s1.tuser = uwrite(error_bit,in_byte_count + BYTE_CARRY);
            else if (MCASE==MS_END_AT_MSB)
              s1.tuser = uwrite(1,0); // not enough data to avoid error
            else if (in_byte_count <= FIRST_BYTE_AFTER)
              if (REM_START == 0)
                s1.tvalid = 0;
              else
                s1.tuser = uwrite(1,0); // not enough data to avoid error
            else
              s1.tuser = uwrite(error_bit,in_byte_count - FIRST_BYTE_AFTER);

            // if we are exact and have already published some data
            // that data is unterminated and we have no way to
            // set a packet end.
            if (EXACT && REM_START!=0)
              s1.tuser[ERROR] = 1;

            if (s1.tready && s0.tvalid) begin
              next_remove_state = ST_PRE_REMOVE;
            end

          // end of packet and we have some some data to send
          // and we have more data then we can fit in the
          // the current word
          end else if (s0.tlast && reached_end_plus && in_byte_count > BYTE_SHIFT
                       && BYTE_SHIFT != 0) begin
            debug = D_LAST_W_END_BONUS;
            s1.tlast  = 0;
            s1.tvalid = s0.tvalid;
            s0_tready = 0; // don't let a advance

            if (s0.tvalid && s1.tready) begin
              next_remove_state = ST_BONUS;
            end

          // end of packet and we have some some data to send
          // and we were ready to send data anyways
          end else if(s0.tlast && reached_end_plus) begin
            debug = D_LAST_W_END_PLUS;
            s1.tlast  = 1;
            s1.tvalid = s0.tvalid;
            s0_tready = s1.tready;

            if (EXACT)
              s1.tuser = uwrite(error_bit,in_byte_count);
            else begin
              s1.tuser = uwrite(error_bit,in_byte_count + BYTE_CARRY);
            end

            if (MCASE==MS_WRAP && in_byte_count <= FIRST_BYTE_AFTER)
              s1.tuser = uwrite(1,0); // not enough data to avoid error

            if (s1.tready && s0.tvalid) begin
              next_remove_state = ST_PRE_REMOVE;
            end

          // we are ready to send the first byte after the shift
          end else if(!s0.tlast && reached_end_plus) begin
            debug = D_REACHED_END_PLUS;
            s1.tlast  = 0;
            s1.tvalid = s0.tvalid;
            s0_tready = s1.tready;
            s1.tuser = uwrite(error_bit,BYTES_PER_WORD);

            if (s1.tready && s0.tvalid) begin
              next_remove_state = ST_POST_REMOVE;
            end

          end
        end

        // *****************************************************
        // POST_REMOVAL waiting for end
        // *****************************************************
        ST_POST_REMOVE: begin
          //defaults
          data_mux_sel = REM_SHIFT_DATA;
          s1.tuser  = uwrite(error_bit,BYTES_PER_WORD);
          s1.tlast  = 0;
          s1.tvalid = s0.tvalid;
          s0_tready = s1.tready;
          // reached the end, but we have extra bytes to send
          if (s0.tlast && in_byte_count > BYTE_SHIFT
                      && BYTE_SHIFT != 0) begin
            s1.tlast = 0;
            s0_tready = 0; // don't let a advance

            if (s0.tvalid && s1.tready) begin
              next_remove_state = ST_BONUS;
            end

          // reached the end, and don't need the bonus state
          end else if (s0.tlast) begin
            s1.tlast = 1;
            s1.tuser  = uwrite(error_bit,in_byte_count + BYTES_PER_WORD-BYTE_SHIFT);

            if (s1.tready && s0.tvalid) begin
              next_remove_state = ST_PRE_REMOVE;
            end

          end
        end

        // *****************************************************
        // BONUS write out any overflow words
        // *****************************************************
        ST_BONUS: begin
          //defaults
          data_mux_sel = BONUS;
          s1.tuser  = uwrite(error_bit,in_byte_count-BYTE_SHIFT);
          s1.tlast  = 1;
          s1.tvalid = s0.tvalid;
          s0_tready = s1.tready;

          if (s1.tready && s0.tvalid) begin
            next_remove_state = ST_PRE_REMOVE;
          end

        end

        // We should never get here
        default: begin
          next_remove_state = ST_PRE_REMOVE;
        end
      endcase
    end

    always_ff @(posedge s0.clk) begin
      if (s0.rst) begin
        remove_state <= ST_PRE_REMOVE;
      end else begin
        remove_state <= next_remove_state;
      end
    end

    // move from AxiStreamIfc to AxiStreamPacketIf
    always_comb begin
      `AXI4S_ASSIGN(o,s1)
    end

  end

endmodule : axi4s_remove_bytes

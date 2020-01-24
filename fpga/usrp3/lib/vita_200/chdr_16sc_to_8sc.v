//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module chdr_16sc_to_8sc
  #(parameter BASE=0)
   (input clk, input reset,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    //input side of device
    input [63:0] i_tdata,
    input i_tlast,
    input i_tvalid,
    output i_tready,
    //output side of device
    output reg [63:0] o_tdata,
    output o_tlast,
    output o_tvalid,
    input o_tready,

    output [31:0] debug
    );
   
   //pipeline register
   reg [63:0] 	  hold_tdata;
   //bit assignments
   wire    chdr_has_hdr = 1'b1;
   wire    chdr_has_time = i_tdata[61];
   wire    chdr_has_tlr = 1'b0;

   wire [7:0]    rounded_i1;
   wire [7:0]    rounded_q1;
   wire [7:0]    rounded_i0;
   wire [7:0]    rounded_q0;

   wire [7:0]    rounded_i2;
   wire [7:0]    rounded_q2;
   wire [7:0]    rounded_i3;
   wire [7:0]    rounded_q3;

   //chdr length calculations
   wire [15:0]    chdr_header_lines8 = chdr_has_time? 16 : 8;

   wire [15:0]    chdr_almost_payload_lines8 = ((i_tdata[47:32] - chdr_header_lines8) >> 1);

   wire [15:0]    chdr_payload_lines8 = chdr_almost_payload_lines8 + chdr_header_lines8;
   wire [15:0]    my_newhome;

   wire    set_sid;
   
   setting_reg #(.my_addr(BASE), .width(17)) new_destination
     (.clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out({set_sid, my_newhome[15:0]}));

   localparam HEADER        = 2'd0;//IDLE
   localparam TIME          = 2'd1;
   localparam ODD           = 2'd2;
   localparam EVEN          = 2'd3;
   

   reg [1:0]       state;
   


   always @(posedge clk) begin
      if (reset) begin
        state <= HEADER;
        hold_tdata <= 0;
      end	
      else case(state)

        HEADER: begin
          if (i_tvalid && o_tready) begin
            state <= (i_tdata[61])? TIME : ODD;
          end

        end

        TIME: begin
          if (i_tvalid && o_tready) begin
            state <= (i_tlast)? HEADER: ODD;
            hold_tdata <= i_tdata;
          end
        end

        ODD: begin
          if (i_tvalid) begin
            if (i_tlast) begin
              if(o_tready)
                state <= HEADER;
            end
            else begin
              state <= EVEN;
              hold_tdata <= i_tdata;
            end
          end
        end

        EVEN: begin
          if (i_tvalid && o_tready)
            state <= (i_tlast) ? HEADER: ODD;
          hold_tdata <= i_tdata;
    
        end
              
       default: state <= HEADER;
       
     endcase
   end
   
   //assign 8 bit i and q signals from this line and last
   
   //new data processing
   round #(.bits_in(16),
     .bits_out(8))
   round_i2
     (.in(i_tdata[63:48]), 
      .out(rounded_i2[7:0])
      );

   round #(.bits_in(16),
           .bits_out(8))
   round_q2
     (.in(i_tdata[47:32]),
      .out(rounded_q2[7:0])
      );

   round #(.bits_in(16),
           .bits_out(8))
   round_i3
     (.in(i_tdata[31:16]),
      .out(rounded_i3[7:0])
      );

   round #(.bits_in(16),
           .bits_out(8))
   round_q3
     (.in(i_tdata[15:0]),
      .out(rounded_q3[7:0])
      );

   // old data processing 
   round #(.bits_in(16),
     .bits_out(8))
   round_i0(.in(hold_tdata[63:48]), .out(rounded_i0[7:0])
      );

   round #(.bits_in(16),
           .bits_out(8))
   round_q0
     (.in(hold_tdata[47:32]),
      .out(rounded_q0[7:0])
      );

   round #(.bits_in(16),
           .bits_out(8))
   round_i1
     (.in(hold_tdata[31:16]),
      .out(rounded_i1[7:0])
      );

   round #(.bits_in(16),
           .bits_out(8))
   round_q1
     (.in(hold_tdata[15:0]),
      .out(rounded_q1[7:0])
      );
   
   // main mux
   always @(*) 
     case(state)
       HEADER: o_tdata = {i_tdata[63:48], chdr_payload_lines8,
        set_sid ? {i_tdata[15:0], my_newhome[15:0]}:i_tdata[31:0]};
       TIME: o_tdata = i_tdata;
       ODD: o_tdata = {rounded_i2, rounded_q2, rounded_i3, rounded_q3,rounded_i0, rounded_q0, rounded_i1, rounded_q1};
       EVEN: o_tdata = {rounded_i0, rounded_q0, rounded_i1, rounded_q1,rounded_i2, rounded_q2, rounded_i3, rounded_q3};
       default : o_tdata = i_tdata;
     endcase
   
   assign o_tvalid = i_tvalid && (state != ODD || i_tlast);
   assign i_tready = o_tready || (state == ODD && !i_tlast);
   assign o_tlast  = i_tlast;

endmodule













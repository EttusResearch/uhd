// -- (c) Copyright 2010 Xilinx, Inc. All rights reserved. 
// --                                                             
// -- This file contains confidential and proprietary information 
// -- of Xilinx, Inc. and is protected under U.S. and             
// -- international copyright and other intellectual property     
// -- laws.                                                       
// --                                                             
// -- DISCLAIMER                                                  
// -- This disclaimer is not a license and does not grant any     
// -- rights to the materials distributed herewith. Except as     
// -- otherwise provided in a valid license issued to you by      
// -- Xilinx, and to the maximum extent permitted by applicable   
// -- law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND     
// -- WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES 
// -- AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING   
// -- BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-      
// -- INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and    
// -- (2) Xilinx shall not be liable (whether in contract or tort,
// -- including negligence, or under any other theory of          
// -- liability) for any loss or damage of any kind or nature     
// -- related to, arising under or in connection with these       
// -- materials, including for any direct, or any indirect,       
// -- special, incidental, or consequential loss or damage        
// -- (including loss of data, profits, goodwill, or any type of  
// -- loss or damage suffered as a result of any action brought   
// -- by a third party) even if such damage or loss was           
// -- reasonably foreseeable or Xilinx had been advised of the    
// -- possibility of the same.                                    
// --                                                             
// -- CRITICAL APPLICATIONS                                       
// -- Xilinx products are not designed or intended to be fail-    
// -- safe, or for use in any application requiring fail-safe     
// -- performance, such as life-support or safety devices or      
// -- systems, Class III medical devices, nuclear facilities,     
// -- applications related to the deployment of airbags, or any   
// -- other applications that could lead to death, personal       
// -- injury, or severe property or environmental damage          
// -- (individually and collectively, "Critical                   
// -- Applications"). Customer assumes the sole risk and          
// -- liability of any use of Xilinx products in Critical         
// -- Applications, subject only to applicable laws and           
// -- regulations governing limitations on product liability.     
// --                                                             
// -- THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS    
// -- PART OF THIS FILE AT ALL TIMES.                             
// --  
///////////////////////////////////////////////////////////////////////////////
//
// File name: axi_mc_w_channel.v
//
// Description:
// write data channel module is used to buffer the write data from AXI, mask extra transactions 
// that are not needed in BL8 mode and send them to the MC write data FIFO. 
// The use of register slice could result in write data arriving to this modules well before the 
// the commands are processed by the address modules. The data from AXI will be buffered 
// in the write data FIFO before being sent to the MC.
// The address channel modules will send signals to mask appropriate data to before being sent 
// to the MC. 
//
///////////////////////////////////////////////////////////////////////////////
`timescale 1ps/1ps
`default_nettype none

module mig_7series_v1_8_axi_mc_w_channel #
(
///////////////////////////////////////////////////////////////////////////////
// Parameter Definitions
///////////////////////////////////////////////////////////////////////////////
                    // Width of AXI xDATA and MCB xx_data
                    // Range: 32, 64, 128.
  parameter integer C_DATA_WIDTH              = 32,
                        // MC burst length. = 1 for BL4 or BC4, = 2 for BL8
  parameter integer C_MC_BURST_LEN              = 1,
                    // axi addr width 
  parameter integer C_AXI_ADDR_WIDTH            = 32
)
(
///////////////////////////////////////////////////////////////////////////////
// Port Declarations     
///////////////////////////////////////////////////////////////////////////////
  input  wire                                 clk         , 
  input  wire                                 reset   , 

  input  wire [C_DATA_WIDTH-1:0]              wdata,
  input  wire [C_DATA_WIDTH/8-1:0]            wstrb,
  input  wire                                 wvalid,
  output wire                                 wready,
  input  wire [7:0]                           wwlen, 

  input  wire                                 w_cmd_rdy,
  input  wire                                 w_ignore_begin,
  input  wire                                 w_ignore_end,
  input  wire                                 w_incr_burst,
  
 
  output wire                                 mc_app_wdf_wren,
  output wire [C_DATA_WIDTH/8-1:0]            mc_app_wdf_mask,
  output wire [C_DATA_WIDTH-1:0]              mc_app_wdf_data,
  output wire                                 mc_app_wdf_last,
  input  wire                                 mc_app_wdf_rdy,
  input  wire                                 mc_init_complete,

  output wire                                 w_complete
);

////////////////////////////////////////////////////////////////////////////////
// Local parameters
////////////////////////////////////////////////////////////////////////////////
localparam P_D_WIDTH = C_DATA_WIDTH+(C_DATA_WIDTH/8);
localparam P_D_DEPTH = (C_MC_BURST_LEN == 1) ? 8: 16;
localparam P_D_AWIDTH = (C_MC_BURST_LEN == 1) ? 3 : 4;

//states
localparam SM_IDLE         = 2'b00;
localparam SM_FIRST_DATA   = 2'b01;
localparam SM_SECOND_DATA  = 2'b10;
localparam SM_WAIT         = 2'b11;

// localparam for 2 deep skid buffer
localparam [1:0] 
  ZERO = 2'b10,
  ONE  = 2'b11,
  TWO  = 2'b01;
////////////////////////////////////////////////////////////////////////////////
// Wire and register declarations
////////////////////////////////////////////////////////////////////////////////


wire                      wdf_last;
wire [C_DATA_WIDTH/8-1:0] wdf_mask;
wire [C_DATA_WIDTH-1:0]   wdf_data;
wire                      assert_wren;
wire                      disable_data;
wire                      fifo_empty;
wire                      fifo_a_full;
wire                      fifo_rd_en;
reg                       fifo_wr_en;
reg  [(C_DATA_WIDTH/8 + C_DATA_WIDTH)-1:0] fifo_in;
wire [(C_DATA_WIDTH/8 + C_DATA_WIDTH)-1:0] fifo_out;
reg  [(C_DATA_WIDTH/8 + C_DATA_WIDTH)-1:0] fifo_out_bl2;
reg 			  mc_init_complete_r;
reg 			  mc_init_complete_r1;


////////////////////////////////////////////////////////////////////////////////
// BEGIN RTL
////////////////////////////////////////////////////////////////////////////////


assign wready  = (mc_init_complete_r1) ? ~fifo_a_full : 1'b0;
assign mc_app_wdf_wren   = assert_wren;
assign mc_app_wdf_mask   = (disable_data)?
                            {C_DATA_WIDTH/8{1'b1}} : wdf_mask;
assign mc_app_wdf_data   = wdf_data;
// last asserted for every write in BL4. 
assign mc_app_wdf_last   = (C_MC_BURST_LEN == 1) ? mc_app_wdf_wren : wdf_last;


// registered for timing
always @(posedge clk) begin
  mc_init_complete_r   <= mc_init_complete;
  mc_init_complete_r1  <= mc_init_complete_r;
end



generate
  if(C_MC_BURST_LEN == 1) begin : gen_bc1

  // declaration of signals used only in the bc1 mode
  reg  [4:0] 		  w_cnt_r;
  reg                     w_cmd_pend_r;

    always @(posedge clk) begin
      if(reset)begin
        w_cnt_r <= 5'd0;
        w_cmd_pend_r <= 1'b1;
      end else if (w_cmd_rdy && (w_cnt_r>5'd0))begin
        if((~fifo_empty & mc_app_wdf_rdy) &&
           (~w_cnt_r[4]))
          w_cnt_r <= w_cnt_r;
        else
          w_cnt_r <= w_cnt_r -1;
      end else if (w_cmd_rdy && (w_cnt_r==5'd0)) begin
        // use the flag to keep track of w_cmd_rdy pulse that
        // arrives when the count is zero. If there is a write 
        // to WDF during this condition set the flag which will
        // be used to increment the counter during the next WDF
        // write. Else don't increment the counter during the next
        // WDF write to account for the w_cmd_rdy in this condition.  
        w_cmd_pend_r <= (~fifo_empty & mc_app_wdf_rdy);
      end else if((~fifo_empty & mc_app_wdf_rdy)
                  && (~w_cnt_r[4]))begin
        w_cnt_r <= w_cnt_r + w_cmd_pend_r;
        w_cmd_pend_r <= 1'b1;
      end 
    end 


    // w_complete to axi_mc_cmd_fsm when one Bl8 or Bl4 worth of write data 
    // is pumped into to MC WDF.  

    assign w_complete = (w_cnt_r > 5'd0) | (mc_app_wdf_rdy & (~fifo_empty));
    // write enable signal to WDF
    assign assert_wren = (~fifo_empty && ~w_cnt_r[4]);
    // Disable data by asserting all the MASK signals based on the
    // ignore signals from the address modules 
    assign disable_data = 1'b0;
    // FIFO read enable signal. 
    assign fifo_rd_en =  ((~fifo_empty & mc_app_wdf_rdy) & (~w_cnt_r[4]));

  end else begin : gen_bc2

    // Declaration of signals used only in BC2 mode
    reg  [1:0]                w_state;
    reg                       load_data;
    reg [(C_DATA_WIDTH/8 + C_DATA_WIDTH)-1:0] fifo_out_r;
    reg [(C_DATA_WIDTH/8 + C_DATA_WIDTH)-1:0] fifo_out_r1;
    reg [(C_DATA_WIDTH/8 + C_DATA_WIDTH)-1:0] fifo_out_bl2_r;
    wire 		      read_enable;
    wire 		      data_ready;
    reg                       load_registered_data;
    wire                      store_data;
    reg                       w_cmd_rdy_r;
    reg                       w_cmd_rdy_edge_r;
    reg                       w_rdy_flag_r;
    reg                       w_ignore_begin_r;
    wire                      ignore_begin_cond;
    reg                       w_ignore_end_r;
    reg                       w_incr_burst_r;
    reg  [1:0]                state;
    reg  [1:0]                next_state;
    reg [7:0] 		      w_len_cnt_r;
    // registered for timing
    always @(posedge clk) begin
      w_cmd_rdy_r <= w_cmd_rdy;
      w_ignore_begin_r <= w_ignore_begin;
      w_ignore_end_r <= w_ignore_end;
      // getting the w_cmd_rdy edge. The edge is used
      // only for Bl8 transactions. During INCR trancsactions
      // the edge while going from SM_WAIT state needs to be ignored. 
      if(state != SM_WAIT)
        w_cmd_rdy_edge_r <= w_cmd_rdy & (~w_cmd_rdy_r);
    end


    // w_cmd_rdy can arrive at any state. Creating a flag if
    // it arrived in a state where it was not checked.

    always @(posedge clk) begin
      if(reset) 
        w_rdy_flag_r <= 1'b0;
      else if((state == SM_SECOND_DATA)||
              (state == SM_WAIT))
        w_rdy_flag_r <= 1'b0;
      else if( ~w_rdy_flag_r)
        w_rdy_flag_r <= w_cmd_rdy;
    end 

    // used only for ignore begin logic.
    // incr_burst will be asserted from the first command. 
    always @(posedge clk) begin
      if((state == SM_IDLE)| ~w_incr_burst)
        w_incr_burst_r <= 1'b0;
      else if((state == SM_SECOND_DATA) |
               (state == SM_WAIT))
        w_incr_burst_r <= w_incr_burst;
    end // always @ (posedge clk)

    //w_incr_burst_r will not be asserted for the first incr transactions
    // and for all wrap transactions 
    assign ignore_begin_cond = w_ignore_begin_r & (~w_incr_burst_r);

    // Storing the length of the transactions. Used for INCR burst
    // transactions to determine when the burst is about to end. 
    always @(posedge clk) begin
      if(state == SM_IDLE)
        w_len_cnt_r <= wwlen;
      else if((assert_wren & mc_app_wdf_rdy) 
          && (w_len_cnt_r > 8'd0))
        w_len_cnt_r <= w_len_cnt_r -1;
    end // always @ (posedge clk)


    always @(posedge clk) begin
      if (reset) begin
        state <= SM_IDLE;
      end else begin
        state <= next_state;
      end
    end

    // Next state transitions.
    // Simple state machine to push data into the MC write data FIFO(WDF). 
    // For BL4 only one data will be written into the WDF.  For BL8 two 
    // beats of data will be written into the WDF. 

    always @(*)
    begin
      next_state = state;
      case (state)
        SM_IDLE:
          if(w_cmd_rdy_edge_r)
            next_state = SM_FIRST_DATA;
          else 
            next_state = state;

        SM_FIRST_DATA:
          if(mc_app_wdf_rdy & data_ready)begin
            if(w_incr_burst)
              if(w_cmd_rdy | w_rdy_flag_r)
                next_state = SM_SECOND_DATA;
              else
                next_state = SM_WAIT;
            else
              next_state = SM_SECOND_DATA; 
          end 
          else 
            next_state = state;

        SM_SECOND_DATA:
          if(mc_app_wdf_rdy & (data_ready 
             | ignore_begin_cond | w_ignore_end_r))begin
            if(w_incr_burst)
              next_state = SM_FIRST_DATA;
            else 
              next_state = SM_IDLE;
          end else 
            next_state = state;

        SM_WAIT:
          if(w_cmd_rdy)begin 
            if(mc_app_wdf_rdy & (data_ready))begin
              if((w_incr_burst) &&(w_len_cnt_r >= C_MC_BURST_LEN))
                next_state = SM_FIRST_DATA;
              else
                next_state = SM_IDLE;
            end else
              next_state = SM_SECOND_DATA;
          end else
            next_state = state;

        default:
          next_state = SM_IDLE;
      endcase // case(state)
    end // always @ (*)

    // w_complete to axi_mc_cmd_fsm when one Bl8 or Bl4 worth of write data 
    // is pumped into to MC WDF.  

    assign w_complete = ((state == SM_SECOND_DATA) & mc_app_wdf_rdy & 
                        (data_ready | ignore_begin_cond | w_ignore_end_r)) |
                         (state == SM_WAIT ) & mc_app_wdf_rdy & data_ready;
    // write enable signal to WDF
    assign assert_wren = ((state == SM_FIRST_DATA)& data_ready) 
                         |((state == SM_SECOND_DATA) &
                        ( data_ready | ignore_begin_cond | w_ignore_end_r))
                         | ((state == SM_WAIT) & w_cmd_rdy & (data_ready));
    // Disable data by asserting all the MASK signals based on the
    // ignore signals from the address modules 
    assign disable_data = (((state == SM_FIRST_DATA) & ignore_begin_cond) |
                         ((state == SM_SECOND_DATA) & w_ignore_end_r));
    // read enable signal. 
    assign  read_enable = ((((state == SM_FIRST_DATA) & (~ignore_begin_cond)) |
                         ((state == SM_SECOND_DATA) & ~w_ignore_end_r) |
                         ((state == SM_WAIT) & w_cmd_rdy))
                        & (data_ready & mc_app_wdf_rdy));

    // anded with mc_app_wdf_wren to overcome a UI bug in MC
    assign wdf_last = ((state == SM_SECOND_DATA) | (state == SM_WAIT)) 
                  &  mc_app_wdf_wren;
    //2 deep skid buffer for timing
    // State Machine for handling output signals
    always @(posedge clk) 
    begin
    if(reset) 
      w_state <= ZERO;
    else
      w_state <= w_state; 
      case (w_state)
        ZERO: if (~fifo_empty) w_state <= ONE;
        ONE: begin
          if (read_enable & fifo_empty) w_state <= ZERO; 
          if (~read_enable & (~fifo_empty)) w_state <= TWO;  
        end
        TWO: if (read_enable) w_state <= ONE;   
      endcase // case (state)
    end 

    always @ (posedge clk)
    begin
      if( ((w_state == ZERO) && (~fifo_empty)) ||
        ((w_state == ONE) && (~fifo_empty)&& (read_enable)) ||
        ((w_state == TWO) && (read_enable)))
        load_data <= 1'b1;
      else
        load_data <= 1'b0;

        load_registered_data <= (w_state == TWO);
    end // always @ *


    // Always register the data on valid transaction. 

assign store_data = ~fifo_empty & w_state[1];


    always @(posedge clk) 
    begin
      fifo_out_bl2_r <= fifo_out_bl2;
      fifo_out_r <= fifo_out;
      if(store_data)
        fifo_out_r1 <= fifo_out;
    end // always @ (posedge clk)

    always @(*) 
    begin
      if(load_data)
        if(load_registered_data)
          fifo_out_bl2 = fifo_out_r1;
        else
          fifo_out_bl2 = fifo_out_r;
      else
        fifo_out_bl2 = fifo_out_bl2_r;
    end // always @ (posedge clk)

    assign fifo_rd_en = ((w_state==ZERO) || (w_state == ONE)) && (~fifo_empty);
    assign data_ready = (w_state == ONE)|| (w_state == TWO);

  end // if (C_MC_BURST_LEN == 1)
endgenerate 


// registered for timing
always @(posedge clk) begin 
  fifo_wr_en <= wvalid & wready;
  fifo_in <= {~wstrb, wdata};
end 
assign {wdf_mask, wdf_data} = (C_MC_BURST_LEN == 1) ?  
       fifo_out : fifo_out_bl2;


// wr data fifo
mig_7series_v1_8_axi_mc_simple_fifo #
  (
  .C_WIDTH                (P_D_WIDTH),
  .C_AWIDTH               (P_D_AWIDTH),
  .C_DEPTH                (P_D_DEPTH)
)
wr_data_fifo_0
(
  .clk     ( clk              ) ,
  .rst     ( reset            ) ,
  .wr_en   ( fifo_wr_en       ) ,
  .rd_en   ( fifo_rd_en       ) ,
  .din     ( fifo_in          ) ,
  .dout    ( fifo_out         ) ,
  .a_full  ( fifo_a_full      ) ,
  .full    (                  ) ,
  .a_empty (                  ) ,
  .empty   ( fifo_empty       ) 
);

endmodule
`default_nettype wire

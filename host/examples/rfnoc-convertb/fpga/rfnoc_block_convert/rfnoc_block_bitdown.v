//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_bitdown
//
// Description:
//
//   <Add block description here>
//
// Parameters:
//
//   THIS_PORTID : Control crossbar port to which this block is connected
//   CHDR_W      : AXIS-CHDR data bus width
//   MTU         : Maximum transmission unit (i.e., maximum packet size in
//                 CHDR words is 2**MTU).
//

`default_nettype none


module rfnoc_block_bitdown #(
  parameter [9:0] THIS_PORTID     = 10'd0,
  parameter       CHDR_W          = 64,
  parameter [5:0] MTU             = 10
)(
  // RFNoC Framework Clocks and Resets
  input  wire                   rfnoc_chdr_clk,
  input  wire                   rfnoc_ctrl_clk,
  input  wire                   ce_clk,
  // RFNoC Backend Interface
  input  wire [511:0]           rfnoc_core_config,
  output wire [511:0]           rfnoc_core_status,
  // AXIS-CHDR Input Ports (from framework)
  input  wire [(2)*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [(2)-1:0]        s_rfnoc_chdr_tlast,
  input  wire [(2)-1:0]        s_rfnoc_chdr_tvalid,
  output wire [(2)-1:0]        s_rfnoc_chdr_tready,
  // AXIS-CHDR Output Ports (to framework)
  output wire [(2)*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [(2)-1:0]        m_rfnoc_chdr_tlast,
  output wire [(2)-1:0]        m_rfnoc_chdr_tvalid,
  input  wire [(2)-1:0]        m_rfnoc_chdr_tready,
  // AXIS-Ctrl Input Port (from framework)
  input  wire [31:0]            s_rfnoc_ctrl_tdata,
  input  wire                   s_rfnoc_ctrl_tlast,
  input  wire                   s_rfnoc_ctrl_tvalid,
  output wire                   s_rfnoc_ctrl_tready,
  // AXIS-Ctrl Output Port (to framework)
  output wire [31:0]            m_rfnoc_ctrl_tdata,
  output wire                   m_rfnoc_ctrl_tlast,
  output wire                   m_rfnoc_ctrl_tvalid,
  input  wire                   m_rfnoc_ctrl_tready
);

  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  // Clocks and Resets
  wire               ctrlport_clk;
  wire               ctrlport_rst;
  wire               axis_data_clk;
  wire               axis_data_rst;
  // CtrlPort Master
  wire               m_ctrlport_req_wr;
  wire               m_ctrlport_req_rd;
  wire [19:0]        m_ctrlport_req_addr;
  wire [31:0]        m_ctrlport_req_data;
  reg               m_ctrlport_resp_ack;
  reg [31:0]        m_ctrlport_resp_data;
  // Payload Stream to User Logic: in_0
  wire [32*1-1:0]    m_in_0_payload_tdata;
  wire [1-1:0]       m_in_0_payload_tkeep;
  wire               m_in_0_payload_tlast,c_in_0_payload_tlast;
  wire               m_in_0_payload_tvalid;
  wire               m_in_0_payload_tready;
  // Context Stream to User Logic: in_0
  wire [CHDR_W-1:0]  m_in_0_context_tdata;
  wire [3:0]         m_in_0_context_tuser;
  wire               m_in_0_context_tlast;
  wire               m_in_0_context_tvalid;
  wire               m_in_0_context_tready;
  // Payload Stream to User Logic: in_1
  wire [32*1-1:0]    m_in_1_payload_tdata;
  wire [1-1:0]       m_in_1_payload_tkeep;
  wire               m_in_1_payload_tlast;
  wire               m_in_1_payload_tvalid;
  wire               m_in_1_payload_tready;
  // Context Stream to User Logic: in_1
  wire [CHDR_W-1:0]  m_in_1_context_tdata;
  wire [3:0]         m_in_1_context_tuser;
  wire               m_in_1_context_tlast;
  wire               m_in_1_context_tvalid;
  wire               m_in_1_context_tready;
  // Payload Stream from User Logic: out_0
  wire [16*1-1:0]    s_out_0_payload_tdata;
  wire [0:0]         s_out_0_payload_tkeep;
  wire               s_out_0_payload_tlast;
  wire               s_out_0_payload_tvalid;
  wire               s_out_0_payload_tready;
  // Context Stream from User Logic: out_0
  wire [CHDR_W-1:0]  s_out_0_context_tdata;
  wire [3:0]         s_out_0_context_tuser;
  wire               s_out_0_context_tlast;
  wire               s_out_0_context_tvalid;
  wire               s_out_0_context_tready;
  // Payload Stream from User Logic: out_1
  wire [16*1-1:0]    s_out_1_payload_tdata;
  wire [0:0]         s_out_1_payload_tkeep;
  wire               s_out_1_payload_tlast;
  wire               s_out_1_payload_tvalid;
  wire               s_out_1_payload_tready;
  // Context Stream from User Logic: out_1
  wire [CHDR_W-1:0]  s_out_1_context_tdata;
  wire [3:0]         s_out_1_context_tuser;
  wire               s_out_1_context_tlast;
  wire               s_out_1_context_tvalid;
  wire               s_out_1_context_tready;

  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  noc_shell_bitdown #(
    .CHDR_W              (CHDR_W),
    .THIS_PORTID         (THIS_PORTID),
    .MTU                 (MTU)
  ) noc_shell_bitdown_i (
    //---------------------
    // Framework Interface
    //---------------------

    // Clock Inputs
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
    .ce_clk              (ce_clk),
    // Reset Outputs
    .rfnoc_chdr_rst      (),
    .rfnoc_ctrl_rst      (),
    .ce_rst              (),
    // RFNoC Backend Interface
    .rfnoc_core_config   (rfnoc_core_config),
    .rfnoc_core_status   (rfnoc_core_status),
    // CHDR Input Ports  (from framework)
    .s_rfnoc_chdr_tdata  (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast  (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready (s_rfnoc_chdr_tready),
    // CHDR Output Ports (to framework)
    .m_rfnoc_chdr_tdata  (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast  (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready (m_rfnoc_chdr_tready),
    // AXIS-Ctrl Input Port (from framework)
    .s_rfnoc_ctrl_tdata  (s_rfnoc_ctrl_tdata),
    .s_rfnoc_ctrl_tlast  (s_rfnoc_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid (s_rfnoc_ctrl_tvalid),
    .s_rfnoc_ctrl_tready (s_rfnoc_ctrl_tready),
    // AXIS-Ctrl Output Port (to framework)
    .m_rfnoc_ctrl_tdata  (m_rfnoc_ctrl_tdata),
    .m_rfnoc_ctrl_tlast  (m_rfnoc_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid (m_rfnoc_ctrl_tvalid),
    .m_rfnoc_ctrl_tready (m_rfnoc_ctrl_tready),

    //---------------------
    // Client Interface
    //---------------------

    // CtrlPort Clock and Reset
    .ctrlport_clk              (ctrlport_clk),
    .ctrlport_rst              (ctrlport_rst),
    // CtrlPort Master
    .m_ctrlport_req_wr         (m_ctrlport_req_wr),
    .m_ctrlport_req_rd         (m_ctrlport_req_rd),
    .m_ctrlport_req_addr       (m_ctrlport_req_addr),
    .m_ctrlport_req_data       (m_ctrlport_req_data),
    .m_ctrlport_resp_ack       (m_ctrlport_resp_ack),
    .m_ctrlport_resp_data      (m_ctrlport_resp_data),

    // AXI-Stream Payload Context Clock and Reset
    .axis_data_clk (axis_data_clk),
    .axis_data_rst (axis_data_rst),
    // Payload Stream to User Logic: in_0
    .m_in_0_payload_tdata  (m_in_0_payload_tdata),
    .m_in_0_payload_tkeep  (m_in_0_payload_tkeep),
    .m_in_0_payload_tlast  (m_in_0_payload_tlast),
    .m_in_0_payload_tvalid (m_in_0_payload_tvalid),
    .m_in_0_payload_tready (m_in_0_payload_tready),
    // Context Stream to User Logic: in_0
    .m_in_0_context_tdata  (m_in_0_context_tdata),
    .m_in_0_context_tuser  (m_in_0_context_tuser),
    .m_in_0_context_tlast  (m_in_0_context_tlast),
    .m_in_0_context_tvalid (m_in_0_context_tvalid),
    .m_in_0_context_tready (m_in_0_context_tready),
    // Payload Stream to User Logic: in_1
    .m_in_1_payload_tdata  (m_in_1_payload_tdata),
    .m_in_1_payload_tkeep  (m_in_1_payload_tkeep),
    .m_in_1_payload_tlast  (m_in_1_payload_tlast),
    .m_in_1_payload_tvalid (m_in_1_payload_tvalid),
    .m_in_1_payload_tready (m_in_1_payload_tready),
    // Context Stream to User Logic: in_1
    .m_in_1_context_tdata  (m_in_1_context_tdata),
    .m_in_1_context_tuser  (m_in_1_context_tuser),
    .m_in_1_context_tlast  (m_in_1_context_tlast),
    .m_in_1_context_tvalid (m_in_1_context_tvalid),
    .m_in_1_context_tready (m_in_1_context_tready),
    // Payload Stream from User Logic: out_0
    .s_out_0_payload_tdata  (s_out_0_payload_tdata),
    .s_out_0_payload_tkeep  (s_out_0_payload_tkeep),
    .s_out_0_payload_tlast  (s_out_0_payload_tlast),
    .s_out_0_payload_tvalid (s_out_0_payload_tvalid),
    .s_out_0_payload_tready (s_out_0_payload_tready),
    // Context Stream from User Logic: out_0
    .s_out_0_context_tdata  (s_out_0_context_tdata),
    .s_out_0_context_tuser  (s_out_0_context_tuser),
    .s_out_0_context_tlast  (s_out_0_context_tlast),
    .s_out_0_context_tvalid (s_out_0_context_tvalid),
    .s_out_0_context_tready (s_out_0_context_tready),
    // Payload Stream from User Logic: out_1
    .s_out_1_payload_tdata  (s_out_1_payload_tdata),
    .s_out_1_payload_tkeep  (s_out_1_payload_tkeep),
    .s_out_1_payload_tlast  (s_out_1_payload_tlast),
    .s_out_1_payload_tvalid (s_out_1_payload_tvalid),
    .s_out_1_payload_tready (s_out_1_payload_tready),
    // Context Stream from User Logic: out_1
    .s_out_1_context_tdata  (s_out_1_context_tdata),
    .s_out_1_context_tuser  (s_out_1_context_tuser),
    .s_out_1_context_tlast  (s_out_1_context_tlast),
    .s_out_1_context_tvalid (s_out_1_context_tvalid),
    .s_out_1_context_tready (s_out_1_context_tready)
  );

  //---------------------------------------------------------------------------
  // User Registers
  //---------------------------------------------------------------------------
  //
  // There's only one register now, but we'll structure the register code to
  // make it easier to add more registers later.
  // Register use the ctrlport_clk clock.
  //
  //---------------------------------------------------------------------------

  // Note: Register addresses increment by 4
  localparam REG_USER_ADDR    = 0; // Address for example user register
  localparam REG_USER_DEFAULT = 0; // Default value for user register

  reg [31:0] reg_user = REG_USER_DEFAULT;

  always @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      reg_user = REG_USER_DEFAULT;
    end else begin
      // Default assignment
      m_ctrlport_resp_ack <= 0;

      // Read user register
      if (m_ctrlport_req_rd) begin // Read request
        case (m_ctrlport_req_addr)
          REG_USER_ADDR: begin
            m_ctrlport_resp_ack  <= 1;
            m_ctrlport_resp_data <= reg_user;
          end
        endcase
      end

      // Write user register
      if (m_ctrlport_req_wr) begin // Write requst
        case (m_ctrlport_req_addr)
          REG_USER_ADDR: begin
            m_ctrlport_resp_ack <= 1;
            reg_user            <= m_ctrlport_req_data[31:0];
          end
        endcase
      end
    end
  end

  //---------------------------------------------------------------------------
  // User Logic
  //---------------------------------------------------------------------------
  reg [15:0] f_out_0_payload_tdata=0, f_out_1_payload_tdata=0;
  reg [15:0] c_out_0_payload_tdata,c_out_1_payload_tdata;

  // < Replace this section with your logic >
  assign s_out_0_payload_tdata    =  c_out_0_payload_tdata;//{i_data,q_data} ;
  assign s_out_0_payload_tkeep    =  m_in_0_payload_tkeep  ;
  assign s_out_0_payload_tlast    =  m_in_0_payload_tlast  ;
  assign s_out_0_payload_tvalid   =  m_in_0_payload_tvalid ;
  assign m_in_0_payload_tready    =  s_out_0_payload_tready;
                               
  assign s_out_0_context_tdata    =  f_in_0_context_tdata  ;
  assign s_out_0_context_tuser    =  f_in_0_context_tuser  ;
  assign s_out_0_context_tlast    =  f_in_0_context_tlast  ;
  assign s_out_0_context_tvalid   =  f_in_0_context_tvalid ;
  assign m_in_0_context_tready    =  s_out_0_context_tready;
                               
  assign s_out_1_payload_tdata    =  c_out_1_payload_tdata;
  assign s_out_1_payload_tkeep    =  m_in_1_payload_tkeep  ;
  assign s_out_1_payload_tlast    =  m_in_1_payload_tlast  ;
  assign s_out_1_payload_tvalid   =  m_in_1_payload_tvalid ;
  assign m_in_1_payload_tready    =  s_out_1_payload_tready;
                               
  assign s_out_1_context_tdata    =  f_in_1_context_tdata  ;
  assign s_out_1_context_tuser    =  f_in_1_context_tuser  ;
  assign s_out_1_context_tlast    =  f_in_1_context_tlast  ;
  assign s_out_1_context_tvalid   =  f_in_1_context_tvalid ;
  assign m_in_1_context_tready    =  s_out_1_payload_tready;

wire [255:0] probe0;
wire [15:0] vio_probe0;
wire [3:0] vio_probe1;

assign probe0[31:0]    = m_in_0_payload_tdata   ; 
assign probe0[63:32]   = m_in_1_payload_tdata   ;
assign probe0[64]      = m_in_0_payload_tkeep   ;
assign probe0[65]      = m_in_1_payload_tkeep   ;
assign probe0[66]      = m_in_0_payload_tlast   ;
assign probe0[67]      = m_in_1_payload_tlast   ;
assign probe0[68]      = m_in_0_payload_tvalid  ;
assign probe0[69]      = m_in_1_payload_tvalid  ;
assign probe0[70]      = f_in_0_context_tlast   ;
assign probe0[71]      = m_in_1_context_tlast   ;
assign probe0[72]      = f_in_0_context_tvalid  ;
assign probe0[73]      = m_in_1_context_tvalid  ;
assign probe0[77:74]   = f_in_0_context_tuser   ;
assign probe0[81:78]   = m_in_1_context_tuser   ;
assign probe0[145:82]  = m_in_0_context_tdata   ;
assign probe0[157:146] = f_word_cnt;
assign probe0[173:158] = c_out_0_payload_tdata;
assign probe0[180:174] = f_pk_cnt;
assign probe0[209:181] = 0;
assign probe0[210]     = s_out_0_payload_tready   ;
assign probe0[211]     = s_out_0_context_tready   ;
assign probe0[212]     = s_out_1_payload_tready   ;
assign probe0[213]     = s_out_1_payload_tready   ;
assign probe0[229:214] = f_in_0_context_tdata[31:16];
assign probe0[245:230] = f_max_d;
assign probe0[254:246] = 0   ;
assign probe0[255] = f_error  ;

ila_0 bidown_ila (
  .clk(axis_data_clk), // input wire clk

  .probe0(probe0) // input wire [255:0] probe0
);

vio_0 bitdown_vio (
  .clk(axis_data_clk),                // input wire clk
  .probe_out0(vio_probe0),  // output wire [15 : 0] probe_out0
  .probe_out1(vio_probe1)
);
 reg [CHDR_W-1:0]  f_in_0_context_tdata;
 reg [3:0]         f_in_0_context_tuser;
 reg               f_in_0_context_tlast, f_error=0;
 reg               f_in_0_context_tvalid;
 reg               f_in_0_context_tready;
 reg [15:0]         f_max_d=0, f_pk_cnt=0;
 reg [11:0]        f_word_cnt=0;
 reg [7:0]         i_data,q_data,i1_data,q1_data;
 
 reg [CHDR_W-1:0]  f_in_1_context_tdata;
 reg [3:0]         f_in_1_context_tuser;
 reg               f_in_1_context_tlast;
 reg               f_in_1_context_tvalid;
 reg               f_in_1_context_tready;  
 reg               f_in_0_payload_tlast;
 
 assign c_in_0_payload_tlast = m_in_0_payload_tlast & ~f_in_0_payload_tlast;  
always @(posedge axis_data_clk) begin
  f_in_0_payload_tlast   <= m_in_0_payload_tlast;
  f_in_0_context_tuser   <= m_in_0_context_tuser  ;
  f_in_0_context_tlast   <= m_in_0_context_tlast  ;
  f_in_0_context_tvalid  <= m_in_0_context_tvalid ;
  f_in_0_context_tready  <= m_in_0_context_tready ;
  f_in_1_context_tuser   <= m_in_1_context_tuser  ;
  f_in_1_context_tlast   <= m_in_1_context_tlast  ;
  f_in_1_context_tvalid  <= m_in_1_context_tvalid ;
  f_in_1_context_tready  <= m_in_1_context_tready ;
end


always @(vio_probe1[2]) begin
  if(vio_probe1[2]) begin
   c_out_0_payload_tdata <= {q_data,i_data};
   c_out_1_payload_tdata <= {q1_data,i1_data};
  end else begin
   c_out_0_payload_tdata <= {i_data,q_data};
   c_out_1_payload_tdata <= {i1_data,q1_data};
  end
end 

always @(vio_probe1[1:0]) begin
  case (vio_probe1[1:0]) 
    0:begin
      i_data <= m_in_0_payload_tdata[15:8];
      q_data <= m_in_0_payload_tdata[31:24];
      i1_data <= m_in_1_payload_tdata[15:8];
      q1_data <= m_in_1_payload_tdata[31:24];
    end
    1:begin
      i_data <= m_in_0_payload_tdata[14:7];
      q_data <= m_in_0_payload_tdata[30:23];
    end
    2:begin
      i_data <= m_in_0_payload_tdata[13:6];
      q_data <= m_in_0_payload_tdata[29:22];
    end
    default:begin
      i_data <= m_in_0_payload_tdata[15:8];
      q_data <= m_in_0_payload_tdata[31:24];
      i1_data <= m_in_1_payload_tdata[15:8];
      q1_data <= m_in_1_payload_tdata[31:24];
    end
  endcase  
end 
 
always @(posedge axis_data_clk) begin
  if (axis_data_rst) begin
    f_in_0_context_tdata <=0;
    f_in_1_context_tdata <=0;
    f_max_d <= 0;
    f_error <= 0;
    f_word_cnt <= 0;
    f_pk_cnt <= 0;
  end else begin   
    if (c_in_0_payload_tlast) begin
      f_pk_cnt <= f_pk_cnt+1;
    end
    if ($signed(m_in_0_payload_tdata[15:0])>$signed(f_max_d)) begin
     f_max_d <= m_in_0_payload_tdata[15:0];
    end
    if (m_in_0_payload_tvalid) begin
     f_word_cnt <= f_word_cnt + 1;
    end else begin
     f_word_cnt <= 0;
    end
    if ((m_in_0_context_tvalid==1 && m_in_0_context_tuser==0)) begin 
      if (m_in_0_context_tdata[31:16] !=8000) begin
        f_error <= 1;
      end
    end else begin
      f_error <= 0;
    end
    
    
    if ((m_in_0_context_tvalid==1 && m_in_0_context_tuser==0) && (m_in_0_context_tdata[55:53]==6 || m_in_0_context_tdata[55:53]==7)) begin 	
	  f_in_0_context_tdata <= {m_in_0_context_tdata[CHDR_W-1:32],vio_probe0,m_in_0_context_tdata[15:0]};  
    end else begin
	  f_in_0_context_tdata  <= m_in_0_context_tdata  ;
	end
	
	if ((m_in_1_context_tvalid==1 && m_in_1_context_tuser==0) && (m_in_1_context_tdata[55:53]==6 || m_in_1_context_tdata[55:53]==7)) begin 	
	  f_in_1_context_tdata <= {m_in_1_context_tdata[CHDR_W-1:32],vio_probe0,m_in_1_context_tdata[15:0]};  
    end else begin
	  f_in_1_context_tdata  <= m_in_1_context_tdata  ;
	end
  end
end

endmodule // rfnoc_block_bitdown


`default_nettype wire

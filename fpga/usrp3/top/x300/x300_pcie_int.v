//
// Copyright 2013 Ettus Research LLC
//


`define GET_DMA_BUS(parallel_bus, chan_idx) parallel_bus[(DMA_STREAM_WIDTH*(chan_idx+1))-1:(DMA_STREAM_WIDTH*chan_idx)]
`define GET_FSIZE_BUS(parallel_bus, chan_idx) parallel_bus[(DMA_FRAME_SIZE_WIDTH*(chan_idx+1))-1:(DMA_FRAME_SIZE_WIDTH*chan_idx)]
`define GET_SWAP_BUS(parallel_bus, chan_idx) parallel_bus[(3*(chan_idx+1))-1:(3*chan_idx)]

module x300_pcie_int #(
    parameter DMA_STREAM_WIDTH     = 64,
    parameter NUM_TX_STREAMS       = 6,
    parameter NUM_RX_STREAMS       = 6,
    parameter REGPORT_ADDR_WIDTH   = 20,
    parameter REGPORT_DATA_WIDTH   = 32,
    parameter IOP2_MSG_WIDTH       = 64
) (
    //---------------------------------------------------------
    // Clocks and Resets
    //---------------------------------------------------------
    input           ioport2_clk,
    input           bus_clk,
    input           bus_rst,

    //---------------------------------------------------------
    // DMA streams to/from Chinch Interface to IoPort2 (Domain: ioport2_clk)
    //---------------------------------------------------------
    input  [(NUM_TX_STREAMS*DMA_STREAM_WIDTH)-1:0]  dmatx_tdata_iop2,
    input  [NUM_TX_STREAMS-1:0]                     dmatx_tvalid_iop2,
    output [NUM_TX_STREAMS-1:0]                     dmatx_tready_iop2,

    output [(NUM_RX_STREAMS*DMA_STREAM_WIDTH)-1:0]  dmarx_tdata_iop2,
    output [NUM_RX_STREAMS-1:0]                     dmarx_tvalid_iop2,
    input  [NUM_RX_STREAMS-1:0]                     dmarx_tready_iop2,

    //---------------------------------------------------------
    // DMA stream to/from crossbar (Domain: bus_clk)
    //---------------------------------------------------------
    output [DMA_STREAM_WIDTH-1:0]   dmatx_tdata,
    output                          dmatx_tvalid,
    output                          dmatx_tlast,
    input                           dmatx_tready,

    input  [DMA_STREAM_WIDTH-1:0]   dmarx_tdata,
    input                           dmarx_tvalid,
    input                           dmarx_tlast,
    output                          dmarx_tready,

    //---------------------------------------------------------
    // PCIe User register port (Domain: ioport2_clk)
    //---------------------------------------------------------
    input                           pcie_usr_reg_wr,
    input                           pcie_usr_reg_rd,
    input  [REGPORT_ADDR_WIDTH-1:0] pcie_usr_reg_addr,
    input  [REGPORT_DATA_WIDTH-1:0] pcie_usr_reg_data_in,
    input  [1:0]                    pcie_usr_reg_len,        
    output [REGPORT_DATA_WIDTH-1:0] pcie_usr_reg_data_out,
    output                          pcie_usr_reg_rc,
    output                          pcie_usr_reg_rdy,

    //---------------------------------------------------------
    // PCIe Chinch register port (Domain: ioport2_clk)
    //---------------------------------------------------------
    output                          chinch_reg_wr,
    output                          chinch_reg_rd,
    output [REGPORT_ADDR_WIDTH-1:0] chinch_reg_addr,
    output [REGPORT_DATA_WIDTH-1:0] chinch_reg_data_out,
    output [1:0]                    chinch_reg_len,        
    input  [REGPORT_DATA_WIDTH-1:0] chinch_reg_data_in,
    input                           chinch_reg_rc,
    input                           chinch_reg_rdy,

    //---------------------------------------------------------
    // Message FIFOs to/from the core logic (Domain: bus_clk)
    //---------------------------------------------------------
    input [IOP2_MSG_WIDTH-1:0]      rego_tdata,
    input                           rego_tvalid,    
    input                           rego_tlast,    
    output                          rego_tready,
    
    output [IOP2_MSG_WIDTH-1:0]     regi_tdata,
    output                          regi_tvalid,
    output                          regi_tlast,
    input                           regi_tready,
    
    //---------------------------------------------------------
    // Misc
    //---------------------------------------------------------
    input [15:0]    misc_status,
    output [127:0]  debug
);

    localparam REG_CLK_XING_FIFO_SIZE = 5;  //Will synthesize fifo_short_2clk
    localparam DMA_CLK_XING_FIFO_SIZE = 5;  //Will synthesize fifo_short_2clk
    localparam DMA_PKT_GATE_FIFO_SIZE = 11; //Room for 2 8k packets
    localparam DMA_FRAME_SIZE_WIDTH   = 16;
    localparam DMA_RX_DEST_WIDTH      = 3;

    //*******************************************************************************
    // Message FIFO translator + clock crossing
    //
    wire        msgo_tvalid, msgi_tvalid, msgo_tready, msgi_tready;
    wire [63:0] msgo_tdata, msgi_tdata;


    wire        pcie_out_valid, pcie_in_valid;
    wire [63:0] pcie_out_msg, pcie_in_msg;
    
    //Link chinch register port and user register port to AXI message FIFOs
    wire        iop2_rd_response, iop2_wr_request, iop2_rd_request;
    wire [31:0] iop2_data;

    wire        chinch_reg_is_half_word;
    ioport2_msg_decode pcie_out_msg_decoder (
        .message(pcie_out_msg),
        .rd_response(iop2_rd_response), .wr_request(iop2_wr_request), .rd_request(iop2_rd_request), .half_word(chinch_reg_is_half_word), 
        .address(chinch_reg_addr), .data(iop2_data));
    assign chinch_reg_len = chinch_reg_is_half_word ? 2'b01 : 2'b10;

    assign pcie_usr_reg_rc          = iop2_rd_response & pcie_out_valid;
    assign chinch_reg_wr            = iop2_wr_request & pcie_out_valid;
    assign chinch_reg_rd            = iop2_rd_request & pcie_out_valid;
    assign chinch_reg_data_out      = iop2_data;
    assign pcie_usr_reg_data_out    = iop2_data;

    ioport2_msg_encode pcie_in_msg_encoder (
        .rd_response(chinch_reg_rc), .wr_request(pcie_usr_reg_wr), .rd_request(pcie_usr_reg_rd), .half_word(pcie_usr_reg_len == 2'b01), 
        .address(pcie_usr_reg_addr), .data(chinch_reg_rc ? chinch_reg_data_in : pcie_usr_reg_data_in),
        .message(pcie_in_msg));

    assign pcie_in_valid = chinch_reg_rc | pcie_usr_reg_wr | pcie_usr_reg_rd;

    //Cross from the Ioport2 clock domain to the bus clock domain
    axi_fifo_2clk #(.WIDTH(64), .SIZE(REG_CLK_XING_FIFO_SIZE)) pcie_out_msg_fifo (
        .reset(bus_rst),
        .i_aclk(bus_clk), .i_tdata(msgo_tdata), .i_tvalid(msgo_tvalid), .i_tready(msgo_tready),
        .o_aclk(ioport2_clk), .o_tdata(pcie_out_msg), .o_tvalid(pcie_out_valid), .o_tready(chinch_reg_rdy | pcie_usr_reg_rc));

    axi_fifo_2clk #(.WIDTH(64), .SIZE(REG_CLK_XING_FIFO_SIZE)) pcie_in_msg_fifo (
        .reset(bus_rst),
        .i_aclk(ioport2_clk), .i_tdata(pcie_in_msg), .i_tvalid(pcie_in_valid), .i_tready(pcie_usr_reg_rdy),
        .o_aclk(bus_clk), .o_tdata(msgi_tdata), .o_tvalid(msgi_tvalid), .o_tready(msgi_tready));
    //
    //*******************************************************************************

    wire [NUM_TX_STREAMS-1:0]                           dmatx_clear, dmatx_enabled;
    wire [NUM_TX_STREAMS-1:0]                           dmatx_samp_stb, dmatx_pkt_stb, dmatx_busy, dmatx_error;
    wire [(NUM_TX_STREAMS*DMA_FRAME_SIZE_WIDTH)-1:0]    dmatx_frame_size;
    wire [(NUM_TX_STREAMS*3)-1:0]                       dmatx_swap;

    wire [NUM_RX_STREAMS-1:0]                           dmarx_clear, dmarx_enabled;
    wire [NUM_RX_STREAMS-1:0]                           dmarx_samp_stb, dmarx_pkt_stb, dmarx_busy, dmarx_error;
    wire [(NUM_RX_STREAMS*DMA_FRAME_SIZE_WIDTH)-1:0]    dmarx_frame_size;
    wire [(NUM_TX_STREAMS*3)-1:0]                       dmarx_swap;
    wire [DMA_STREAM_WIDTH-1:0]                         dmarx_header;
    wire [DMA_RX_DEST_WIDTH-1:0]                        dmarx_pkt_dest;

    //*******************************************************************************
    // PCIe message/register endpoints
    //
    wire [63:0] basic_regi_tdata,  dmatx_regi_tdata,  dmarx_regi_tdata;
    wire        basic_regi_tvalid, dmatx_regi_tvalid, dmarx_regi_tvalid;
    wire        basic_regi_tready, dmatx_regi_tready, dmarx_regi_tready;
    wire [63:0] basic_rego_tdata,  dmatx_rego_tdata,  dmarx_rego_tdata;
    wire        basic_rego_tvalid, dmatx_rego_tvalid, dmarx_rego_tvalid;
    wire        basic_rego_tready, dmatx_rego_tready, dmarx_rego_tready;
    
    pcie_iop2_msg_arbiter #(
        //(DO NOT USE)                                //0x00000 - 0x3FFFC: Reserved LVFPGA Core Space
        .E0_ADDR(20'h40000), .E0_MASK(20'hFFE00),     //0x40000 - 0x401FC: Basic PCIe registers
        .E1_ADDR(20'h40200), .E1_MASK(20'hFFE00),     //0x40200 - 0x403FC: TX DMA Config/Readback registers
        .E2_ADDR(20'h40400), .E2_MASK(20'hFFE00),     //0x40400 - 0x405FC: RX DMA Config/Readback registers
        .E3_ADDR(20'h60000), .E3_MASK(20'hE0000)      //0x60000 - 0x7FFFC: Client address space 
    ) iop2_msg_arbiter (
        .clk(bus_clk), .reset(bus_rst),
        //Master
        .regi_tdata(msgi_tdata), .regi_tvalid(msgi_tvalid), .regi_tready(msgi_tready),
        .rego_tdata(msgo_tdata), .rego_tvalid(msgo_tvalid), .rego_tready(msgo_tready),
        //Endpoint 0
        .e0_regi_tdata(basic_regi_tdata), .e0_regi_tvalid(basic_regi_tvalid), .e0_regi_tready(basic_regi_tready),
        .e0_rego_tdata(basic_rego_tdata), .e0_rego_tvalid(basic_rego_tvalid), .e0_rego_tready(basic_rego_tready),
        //Endpoint 1
        .e1_regi_tdata(dmatx_regi_tdata), .e1_regi_tvalid(dmatx_regi_tvalid), .e1_regi_tready(dmatx_regi_tready),
        .e1_rego_tdata(dmatx_rego_tdata), .e1_rego_tvalid(dmatx_rego_tvalid), .e1_rego_tready(dmatx_rego_tready),
        //Endpoint 2
        .e2_regi_tdata(dmarx_regi_tdata), .e2_regi_tvalid(dmarx_regi_tvalid), .e2_regi_tready(dmarx_regi_tready),
        .e2_rego_tdata(dmarx_rego_tdata), .e2_rego_tvalid(dmarx_rego_tvalid), .e2_rego_tready(dmarx_rego_tready),
        //Endpoint 3
        .e3_regi_tdata(regi_tdata), .e3_regi_tvalid(regi_tvalid), .e3_regi_tready(regi_tready),
        .e3_rego_tdata(rego_tdata), .e3_rego_tvalid(rego_tvalid), .e3_rego_tready(rego_tready)
    );
    assign regi_tlast = regi_tvalid;
    
    wire [15:0] fpga_status;
    assign fpga_status[7:0]    = {|(dmatx_error), 1'b0, dmatx_enabled};
    assign fpga_status[15:8]   = {|(dmarx_error), 1'b0, dmarx_enabled};

    pcie_basic_regs #(
        .SIGNATURE(32'h58333030 /*ASCII:"X300"*/), .CLK_FREQ(32'd166666667 /*bus_clk = 166.666667MHz*/)
    ) basic_regs (
        .clk(bus_clk), .reset(bus_rst),
        .regi_tdata(basic_regi_tdata), .regi_tvalid(basic_regi_tvalid), .regi_tready(basic_regi_tready),
        .rego_tdata(basic_rego_tdata), .rego_tvalid(basic_rego_tvalid), .rego_tready(basic_rego_tready),
        .misc_status({fpga_status, misc_status})
    );
    
    pcie_dma_ctrl #(
        .NUM_STREAMS(NUM_TX_STREAMS), .FRAME_SIZE_W(DMA_FRAME_SIZE_WIDTH),
        .REG_BASE_ADDR(20'h40200), .ENABLE_ROUTER(0)
    ) tx_dma_ctrl_regs (
        .clk(bus_clk), .reset(bus_rst),
        .regi_tdata(dmatx_regi_tdata), .regi_tvalid(dmatx_regi_tvalid), .regi_tready(dmatx_regi_tready),
        .rego_tdata(dmatx_rego_tdata), .rego_tvalid(dmatx_rego_tvalid), .rego_tready(dmatx_rego_tready),
        .set_enabled(dmatx_enabled), .set_clear(dmatx_clear), .set_frame_size(dmatx_frame_size),
        .sample_stb(dmatx_samp_stb), .packet_stb(dmatx_pkt_stb), 
        .swap_lanes(dmatx_swap), .stream_busy(dmatx_busy), .stream_err(dmatx_error), .rtr_sid(8'h00), .rtr_dst()
    );
    
    pcie_dma_ctrl #(
        .NUM_STREAMS(NUM_RX_STREAMS), .FRAME_SIZE_W(DMA_FRAME_SIZE_WIDTH),
        .REG_BASE_ADDR(20'h40400), .ENABLE_ROUTER(1), .ROUTER_SID_W(8), .ROUTER_DST_W(DMA_RX_DEST_WIDTH)
    ) rx_dma_ctrl_regs (
        .clk(bus_clk), .reset(bus_rst),
        .regi_tdata(dmarx_regi_tdata), .regi_tvalid(dmarx_regi_tvalid), .regi_tready(dmarx_regi_tready),
        .rego_tdata(dmarx_rego_tdata), .rego_tvalid(dmarx_rego_tvalid), .rego_tready(dmarx_rego_tready),
        .set_enabled(dmarx_enabled), .set_clear(dmarx_clear), .set_frame_size(dmarx_frame_size),
        .sample_stb(dmarx_samp_stb), .packet_stb(dmarx_pkt_stb), 
        .swap_lanes(dmarx_swap), .stream_busy(dmarx_busy), .stream_err(dmarx_error), .rtr_sid(dmarx_header[7:0]), .rtr_dst(dmarx_pkt_dest)
    );
    //
    //*******************************************************************************
   
    //*******************************************************************************
    // TX DMA Datapath
    //
    wire [(NUM_TX_STREAMS*DMA_STREAM_WIDTH)-1:0]    dmatx_tdata_bclk,  dmatx_tdata_in, dmatx_tdata_trun,  dmatx_tdata_gt, dmatx_tdata_swap;
    wire [NUM_TX_STREAMS-1:0]                       dmatx_tvalid_bclk, dmatx_tvalid_in, dmatx_tvalid_trun, dmatx_tvalid_gt;
    wire [NUM_TX_STREAMS-1:0]                       dmatx_tready_bclk, dmatx_tready_in, dmatx_tready_trun, dmatx_tready_gt;
    wire [NUM_TX_STREAMS-1:0]                       dmatx_tlast_trun,  dmatx_tlast_gt;

    wire [DMA_STREAM_WIDTH-1:0]                     dmatx_tdata_mux;
    wire                                            dmatx_tvalid_mux, dmatx_tlast_mux, dmatx_tready_mux;

    genvar i;
    generate
        for (i=0; i<NUM_TX_STREAMS; i=i+1) begin: tx_dma_stuff_generator
            axi_fifo_2clk #(.WIDTH(DMA_STREAM_WIDTH), .SIZE(DMA_CLK_XING_FIFO_SIZE)) tx_dma_clock_crossing_fifo (
                .reset(bus_rst),
                .i_aclk(ioport2_clk), .i_tdata(`GET_DMA_BUS(dmatx_tdata_iop2,i)), .i_tvalid(dmatx_tvalid_iop2[i]), .i_tready(dmatx_tready_iop2[i]),
                .o_aclk(bus_clk), .o_tdata(`GET_DMA_BUS(dmatx_tdata_bclk,i)), .o_tvalid(dmatx_tvalid_bclk[i]), .o_tready(dmatx_tready_bclk[i])
            );

            pcie_lossy_samp_gate tx_samp_gate (
                .i_tdata(`GET_DMA_BUS(dmatx_tdata_bclk,i)), .i_tvalid(dmatx_tvalid_bclk[i]), .i_tready(dmatx_tready_bclk[i]),
                .o_tdata(`GET_DMA_BUS(dmatx_tdata_in,i)), .o_tvalid(dmatx_tvalid_in[i]), .o_tready(dmatx_tready_in[i]),
                .drop(~dmatx_enabled[i]), .dropping(dmatx_busy[i])
            );

            data_swapper_64 tx_data_swapper (
                .swap_lanes(`GET_SWAP_BUS(dmatx_swap,i)), .i_tdata(`GET_DMA_BUS(dmatx_tdata_in,i)), .o_tdata(`GET_DMA_BUS(dmatx_tdata_swap,i))
            );

            cvita_dechunker tx_dma_dechunker (
                .clk(bus_clk), .reset(bus_rst), .clear(dmatx_clear[i]), .frame_size(`GET_FSIZE_BUS(dmatx_frame_size, i)),
                .i_tdata(`GET_DMA_BUS(dmatx_tdata_swap,i)), .i_tvalid(dmatx_tvalid_in[i]), .i_tready(dmatx_tready_in[i]),
                .o_tdata(`GET_DMA_BUS(dmatx_tdata_trun,i)), .o_tlast(dmatx_tlast_trun[i]), .o_tvalid(dmatx_tvalid_trun[i]), .o_tready(dmatx_tready_trun[i]),
                .error(dmatx_error[i])
            );
            assign dmatx_samp_stb[i] = dmatx_tvalid_trun[i] & dmatx_tready_trun[i];
            assign dmatx_pkt_stb[i] = dmatx_samp_stb[i] & dmatx_tlast_trun[i];

            axi_packet_gate #(.WIDTH(DMA_STREAM_WIDTH), .SIZE(DMA_PKT_GATE_FIFO_SIZE)) vita_chdr_gate (
                .clk(bus_clk), .reset(bus_rst), .clear(dmatx_clear[i]),
                .i_tdata(`GET_DMA_BUS(dmatx_tdata_trun,i)), .i_tlast(dmatx_tlast_trun[i]), .i_tvalid(dmatx_tvalid_trun[i]), .i_tready(dmatx_tready_trun[i]),
                .i_terror(dmatx_error[i]),
                .o_tdata(`GET_DMA_BUS(dmatx_tdata_gt,i)), .o_tlast(dmatx_tlast_gt[i]), .o_tvalid(dmatx_tvalid_gt[i]), .o_tready(dmatx_tready_gt[i])
            );
        end
    endgenerate

    axi_mux8 #(.PRIO(0), .WIDTH(DMA_STREAM_WIDTH)) output_dma_mux (
        .clk(bus_clk), .reset(bus_rst), .clear(|(dmatx_clear)),
        .i0_tdata(`GET_DMA_BUS(dmatx_tdata_gt,0)), .i0_tlast(dmatx_tlast_gt[0]), .i0_tvalid(dmatx_tvalid_gt[0]), .i0_tready(dmatx_tready_gt[0]),
        .i1_tdata(`GET_DMA_BUS(dmatx_tdata_gt,1)), .i1_tlast(dmatx_tlast_gt[1]), .i1_tvalid(dmatx_tvalid_gt[1]), .i1_tready(dmatx_tready_gt[1]),
        .i2_tdata(`GET_DMA_BUS(dmatx_tdata_gt,2)), .i2_tlast(dmatx_tlast_gt[2]), .i2_tvalid(dmatx_tvalid_gt[2]), .i2_tready(dmatx_tready_gt[2]),
        .i3_tdata(`GET_DMA_BUS(dmatx_tdata_gt,3)), .i3_tlast(dmatx_tlast_gt[3]), .i3_tvalid(dmatx_tvalid_gt[3]), .i3_tready(dmatx_tready_gt[3]),
        .i4_tdata(`GET_DMA_BUS(dmatx_tdata_gt,4)), .i4_tlast(dmatx_tlast_gt[4]), .i4_tvalid(dmatx_tvalid_gt[4]), .i4_tready(dmatx_tready_gt[4]),
        .i5_tdata(`GET_DMA_BUS(dmatx_tdata_gt,5)), .i5_tlast(dmatx_tlast_gt[5]), .i5_tvalid(dmatx_tvalid_gt[5]), .i5_tready(dmatx_tready_gt[5]),
        .i6_tdata(0), .i6_tlast(1'b0), .i6_tvalid(1'b0), .i6_tready(),
        .i7_tdata(0), .i7_tlast(1'b0), .i7_tvalid(1'b0), .i7_tready(),
        .o_tdata(dmatx_tdata_mux), .o_tlast(dmatx_tlast_mux), .o_tvalid(dmatx_tvalid_mux), .o_tready(dmatx_tready_mux)
    );

    axi_fifo_short #(.WIDTH(DMA_STREAM_WIDTH+1)) tx_pipeline_srl (
        .clk(bus_clk), .reset(bus_rst), .clear(|(dmatx_clear)),
        .i_tdata({dmatx_tlast_mux, dmatx_tdata_mux}), .i_tvalid(dmatx_tvalid_mux), .i_tready(dmatx_tready_mux),
        .o_tdata({dmatx_tlast, dmatx_tdata}), .o_tvalid(dmatx_tvalid), .o_tready(dmatx_tready),
        .space(), .occupied());
    //
    //*******************************************************************************
    
    //*******************************************************************************
    // RX DMA Datapath
    //
    wire [(NUM_RX_STREAMS*DMA_STREAM_WIDTH)-1:0]    dmarx_tdata_bclk,  dmarx_tdata_pad, dmarx_tdata_swap, dmarx_tdata_out;
    wire [NUM_RX_STREAMS-1:0]                       dmarx_tvalid_bclk, dmarx_tvalid_pad, dmarx_tvalid_out;
    wire [NUM_RX_STREAMS-1:0]                       dmarx_tready_bclk, dmarx_tready_pad, dmarx_tready_out;
    wire [NUM_RX_STREAMS-1:0]                       dmarx_tlast_bclk,  dmarx_tlast_pad, dmarx_tlast_out;

    wire [DMA_STREAM_WIDTH-1:0]                     dmarx_tdata_mux;
    wire                                            dmarx_tvalid_mux, dmarx_tlast_mux, dmarx_tready_mux;

    axi_fifo_short #(.WIDTH(DMA_STREAM_WIDTH+1)) rx_pipeline_srl (
        .clk(bus_clk), .reset(bus_rst), .clear(|(dmarx_clear)),
        .i_tdata({dmarx_tlast, dmarx_tdata}), .i_tvalid(dmarx_tvalid), .i_tready(dmarx_tready),
        .o_tdata({dmarx_tlast_mux, dmarx_tdata_mux}), .o_tvalid(dmarx_tvalid_mux), .o_tready(dmarx_tready_mux),
        .space(), .occupied());

    axi_demux8 #(.ACTIVE_CHAN(8'b00111111), .WIDTH(DMA_STREAM_WIDTH)) input_dma_demux (
        .clk(bus_clk), .reset(bus_rst), .clear(|(dmarx_clear)),
        .header(dmarx_header), .dest(dmarx_pkt_dest),
        .i_tdata(dmarx_tdata_mux), .i_tlast(dmarx_tlast_mux), .i_tvalid(dmarx_tvalid_mux), .i_tready(dmarx_tready_mux),
        .o0_tdata(`GET_DMA_BUS(dmarx_tdata_bclk,0)), .o0_tlast(dmarx_tlast_bclk[0]), .o0_tvalid(dmarx_tvalid_bclk[0]), .o0_tready(dmarx_tready_bclk[0]),
        .o1_tdata(`GET_DMA_BUS(dmarx_tdata_bclk,1)), .o1_tlast(dmarx_tlast_bclk[1]), .o1_tvalid(dmarx_tvalid_bclk[1]), .o1_tready(dmarx_tready_bclk[1]),
        .o2_tdata(`GET_DMA_BUS(dmarx_tdata_bclk,2)), .o2_tlast(dmarx_tlast_bclk[2]), .o2_tvalid(dmarx_tvalid_bclk[2]), .o2_tready(dmarx_tready_bclk[2]),
        .o3_tdata(`GET_DMA_BUS(dmarx_tdata_bclk,3)), .o3_tlast(dmarx_tlast_bclk[3]), .o3_tvalid(dmarx_tvalid_bclk[3]), .o3_tready(dmarx_tready_bclk[3]),
        .o4_tdata(`GET_DMA_BUS(dmarx_tdata_bclk,4)), .o4_tlast(dmarx_tlast_bclk[4]), .o4_tvalid(dmarx_tvalid_bclk[4]), .o4_tready(dmarx_tready_bclk[4]),
        .o5_tdata(`GET_DMA_BUS(dmarx_tdata_bclk,5)), .o5_tlast(dmarx_tlast_bclk[5]), .o5_tvalid(dmarx_tvalid_bclk[5]), .o5_tready(dmarx_tready_bclk[5]),
        .o6_tdata(), .o6_tlast(), .o6_tvalid(), .o6_tready(1'b0), //Unused port
        .o7_tdata(), .o7_tlast(), .o7_tvalid(), .o7_tready(1'b0)  //Unused port
    );
    
    genvar j;
    generate
        for (j=0; j<NUM_RX_STREAMS; j=j+1) begin: rx_dma_stuff_generator
            assign dmarx_samp_stb[j] = dmarx_tvalid_bclk[j] & dmarx_tready_bclk[j];
            assign dmarx_pkt_stb[j] = dmarx_samp_stb[j] & dmarx_tlast_bclk[j];

            cvita_chunker rx_dma_chunker (
                .clk(bus_clk), .reset(bus_rst), .clear(dmarx_clear[j]), .frame_size(`GET_FSIZE_BUS(dmarx_frame_size, j)),
                .i_tdata(`GET_DMA_BUS(dmarx_tdata_bclk,j)), .i_tlast(dmarx_tlast_bclk[j]), .i_tvalid(dmarx_tvalid_bclk[j]), .i_tready(dmarx_tready_bclk[j]),
                .o_tdata(`GET_DMA_BUS(dmarx_tdata_pad,j)), .o_tlast(dmarx_tlast_pad[j]), .o_tvalid(dmarx_tvalid_pad[j]), .o_tready(dmarx_tready_pad[j]),
                .error(dmarx_error[j])
            );

            data_swapper_64 rx_data_swapper (
                .swap_lanes(`GET_SWAP_BUS(dmarx_swap,j)), .i_tdata(`GET_DMA_BUS(dmarx_tdata_pad,j)), .o_tdata(`GET_DMA_BUS(dmarx_tdata_swap,j))
            );

            pcie_lossy_samp_gate rx_samp_gate (
                .i_tdata(`GET_DMA_BUS(dmarx_tdata_swap,j)), .i_tvalid(dmarx_tvalid_pad[j]), .i_tready(dmarx_tready_pad[j]),
                .o_tdata(`GET_DMA_BUS(dmarx_tdata_out,j)), .o_tvalid(dmarx_tvalid_out[j]), .o_tready(dmarx_tready_out[j]),
                .drop(~dmarx_enabled[j]), .dropping(dmarx_busy[j])
            );
            
            axi_fifo_2clk #(.WIDTH(DMA_STREAM_WIDTH), .SIZE(DMA_CLK_XING_FIFO_SIZE)) rx_dma_clock_crossing_fifo (
                .reset(bus_rst),
                .i_aclk(bus_clk), .i_tdata(`GET_DMA_BUS(dmarx_tdata_out,j)), .i_tvalid(dmarx_tvalid_out[j]), .i_tready(dmarx_tready_out[j]),
                .o_aclk(ioport2_clk), .o_tdata(`GET_DMA_BUS(dmarx_tdata_iop2,j)), .o_tvalid(dmarx_tvalid_iop2[j]), .o_tready(dmarx_tready_iop2[j])
            );
        end
    endgenerate
    //
    //*******************************************************************************
    
endmodule

`undef GET_DMA_BUS
`undef GET_FSIZE_BUS
`undef GET_SWAP_BUS

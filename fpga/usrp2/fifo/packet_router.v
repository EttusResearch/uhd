module packet_router
    #(
        parameter BUF_SIZE = 9,
        parameter UDP_BASE = 0,
        parameter CTRL_BASE = 0
    )
    (
        //wishbone interface for memory mapped CPU frames
        input wb_clk_i,
        input wb_rst_i,
        input wb_we_i,
        input wb_stb_i,
        input [15:0] wb_adr_i,
        input [31:0] wb_dat_i,
        output [31:0] wb_dat_o,
        output reg wb_ack_o,
        output wb_err_o,
        output wb_rty_o,

        //setting register interface
        input set_stb, input [7:0] set_addr, input [31:0] set_data,

        input stream_clk,
        input stream_rst,
        input stream_clr,

        //output status register
        output [31:0] status,

        output sys_int_o, //want an interrupt?

        output [31:0] debug,

        // Input Interfaces (in to router)
        input [35:0] ser_inp_data, input ser_inp_valid, output ser_inp_ready,
        input [35:0] dsp_inp_data, input dsp_inp_valid, output dsp_inp_ready,
        input [35:0] eth_inp_data, input eth_inp_valid, output eth_inp_ready,
        input [35:0] err_inp_data, input err_inp_valid, output err_inp_ready,

        // Output Interfaces (out of router)
        output [35:0] ser_out_data, output ser_out_valid, input ser_out_ready,
        output [35:0] dsp_out_data, output dsp_out_valid, input dsp_out_ready,
        output [35:0] eth_out_data, output eth_out_valid, input eth_out_ready
    );

    assign wb_err_o = 1'b0;  // Unused for now
    assign wb_rty_o = 1'b0;  // Unused for now
    always @(posedge wb_clk_i)
        wb_ack_o <= wb_stb_i & ~wb_ack_o;

    //which buffer: 0 = CPU read buffer, 1 = CPU write buffer
    wire which_buf = wb_adr_i[BUF_SIZE+2];

    ////////////////////////////////////////////////////////////////////
    // CPU interface to this packet router
    ////////////////////////////////////////////////////////////////////
    wire [35:0] cpu_inp_data,  cpu_out_data;
    wire        cpu_inp_valid, cpu_out_valid;
    wire        cpu_inp_ready, cpu_out_ready;

    ////////////////////////////////////////////////////////////////////
    // Communication interfaces
    ////////////////////////////////////////////////////////////////////
    wire [35:0] com_inp_data,  com_out_data,  udp_out_data;
    wire        com_inp_valid, com_out_valid, udp_out_valid;
    wire        com_inp_ready, com_out_ready, udp_out_ready;

    ////////////////////////////////////////////////////////////////////
    // Control signals (setting registers and status signals)
    //    - handshake lines for the CPU communication
    //    - setting registers to program the inspector
    ////////////////////////////////////////////////////////////////////

    //setting register for mode control
    wire [31:0] _sreg_mode_ctrl;
    wire master_mode_flag = _sreg_mode_ctrl[0];
    wire mode_changed;
    setting_reg #(.my_addr(CTRL_BASE+0)) sreg_mode_ctrl(
        .clk(stream_clk),.rst(stream_rst),
        .strobe(set_stb),.addr(set_addr),.in(set_data),
        .out(_sreg_mode_ctrl),.changed(mode_changed)
    );

    //setting register to program the IP address
    wire [31:0] my_ip_addr;
    setting_reg #(.my_addr(CTRL_BASE+1)) sreg_ip_addr(
        .clk(stream_clk),.rst(stream_rst),
        .strobe(set_stb),.addr(set_addr),.in(set_data),
        .out(my_ip_addr),.changed()
    );

    //setting register to program the UDP data ports
    wire [15:0] dsp0_udp_port, dsp1_udp_port;
    setting_reg #(.my_addr(CTRL_BASE+2)) sreg_data_ports(
        .clk(stream_clk),.rst(stream_rst),
        .strobe(set_stb),.addr(set_addr),.in(set_data),
        .out({dsp1_udp_port, dsp0_udp_port}),.changed()
    );

    //setting register for CPU output handshake
    wire [31:0] _sreg_cpu_out_ctrl;
    wire cpu_out_hs_ctrl = _sreg_cpu_out_ctrl[0];
    setting_reg #(.my_addr(CTRL_BASE+3)) sreg_cpu_out_ctrl(
        .clk(stream_clk),.rst(stream_rst | mode_changed),
        .strobe(set_stb),.addr(set_addr),.in(set_data),
        .out(_sreg_cpu_out_ctrl),.changed()
    );

    //setting register for CPU input handshake
    wire [31:0] _sreg_cpu_inp_ctrl;
    wire cpu_inp_hs_ctrl = _sreg_cpu_inp_ctrl[0];
    wire [BUF_SIZE-1:0] cpu_inp_line_count = _sreg_cpu_inp_ctrl[BUF_SIZE-1+16:0+16];
    setting_reg #(.my_addr(CTRL_BASE+4)) sreg_cpu_inp_ctrl(
        .clk(stream_clk),.rst(stream_rst | mode_changed),
        .strobe(set_stb),.addr(set_addr),.in(set_data),
        .out(_sreg_cpu_inp_ctrl),.changed()
    );

    //assign status output signals
    wire cpu_out_hs_stat;
    assign status[0] = cpu_out_hs_stat;
    wire [BUF_SIZE-1:0] cpu_out_line_count;
    assign status[BUF_SIZE-1+16:0+16] = cpu_out_line_count;
    wire cpu_inp_hs_stat;
    assign status[1] = cpu_inp_hs_stat;
    assign status[8] = master_mode_flag; //for the host to readback

    ////////////////////////////////////////////////////////////////////
    // Communication input source crossbar
    // When in master mode:
    //   - serdes input -> comm output combiner
    //   - ethernet input -> comm input inspector
    // When in slave mode:
    //   - serdes input -> comm input inspector
    //   - ethernet input -> null sink
    ////////////////////////////////////////////////////////////////////

    //streaming signals from the crossbar to the combiner
    wire [35:0] ext_inp_data;
    wire        ext_inp_valid;
    wire        ext_inp_ready;

    //dummy signals for valve/xbar below
    wire [35:0] _eth_inp_data;
    wire        _eth_inp_valid;
    wire        _eth_inp_ready;

    valve36 eth_inp_valve (
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr), .shutoff(~master_mode_flag),
        .data_i(eth_inp_data), .src_rdy_i(eth_inp_valid), .dst_rdy_o(eth_inp_ready),
        .data_o(_eth_inp_data), .src_rdy_o(_eth_inp_valid), .dst_rdy_i(_eth_inp_ready)
    );

    crossbar36 com_inp_xbar (
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr), .cross(~master_mode_flag),
        .data0_i(_eth_inp_data), .src0_rdy_i(_eth_inp_valid), .dst0_rdy_o(_eth_inp_ready),
        .data1_i(ser_inp_data), .src1_rdy_i(ser_inp_valid), .dst1_rdy_o(ser_inp_ready),
        .data0_o(com_inp_data), .src0_rdy_o(com_inp_valid), .dst0_rdy_i(com_inp_ready),
        .data1_o(ext_inp_data), .src1_rdy_o(ext_inp_valid), .dst1_rdy_i(ext_inp_ready)
    );

    ////////////////////////////////////////////////////////////////////
    // Communication output sink crossbar
    // When in master mode:
    //   - comm output -> ethernet output
    //   - insp output -> serdes output
    // When in slave mode:
    //   - com output -> serdes output
    //   - insp output -> null sink
    ////////////////////////////////////////////////////////////////////

    //streaming signals from the inspector to the crossbar
    wire [35:0] ext_out_data;
    wire        ext_out_valid;
    wire        ext_out_ready;

    //dummy signals for valve/xbar below
    wire [35:0] _eth_out_data;
    wire        _eth_out_valid;
    wire        _eth_out_ready;

    crossbar36 com_out_xbar (
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr), .cross(~master_mode_flag),
        .data0_i(com_out_data), .src0_rdy_i(com_out_valid), .dst0_rdy_o(com_out_ready),
        .data1_i(ext_out_data), .src1_rdy_i(ext_out_valid), .dst1_rdy_o(ext_out_ready),
        .data0_o(_eth_out_data), .src0_rdy_o(_eth_out_valid), .dst0_rdy_i(_eth_out_ready),
        .data1_o(ser_out_data), .src1_rdy_o(ser_out_valid), .dst1_rdy_i(ser_out_ready)
    );

    valve36 eth_out_valve (
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr), .shutoff(~master_mode_flag),
        .data_i(_eth_out_data), .src_rdy_i(_eth_out_valid), .dst_rdy_o(_eth_out_ready),
        .data_o(eth_out_data), .src_rdy_o(eth_out_valid), .dst_rdy_i(eth_out_ready)
    );

    ////////////////////////////////////////////////////////////////////
    // Communication output source combiner (feeds UDP proto machine)
    //   - DSP framer
    //   - CPU input
    //   - ERR input
    ////////////////////////////////////////////////////////////////////

    //streaming signals from the dsp framer to the combiner
    wire [35:0] dsp_frm_data;
    wire        dsp_frm_valid;
    wire        dsp_frm_ready;

    //dummy signals to join the the muxes below
    wire [35:0] _combiner0_data, _combiner1_data;
    wire        _combiner0_valid, _combiner1_valid;
    wire        _combiner0_ready, _combiner1_ready;

    fifo36_mux _com_output_combiner0(
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr),
        .data0_i(dsp_frm_data), .src0_rdy_i(dsp_frm_valid), .dst0_rdy_o(dsp_frm_ready),
        .data1_i(err_inp_data), .src1_rdy_i(err_inp_valid), .dst1_rdy_o(err_inp_ready),
        .data_o(_combiner0_data), .src_rdy_o(_combiner0_valid), .dst_rdy_i(_combiner0_ready)
    );

    fifo36_mux _com_output_combiner1(
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr),
        .data0_i(32'b0), .src0_rdy_i(1'b0), .dst0_rdy_o(), //mux out from dsp1 can go here
        .data1_i(cpu_inp_data), .src1_rdy_i(cpu_inp_valid), .dst1_rdy_o(cpu_inp_ready),
        .data_o(_combiner1_data), .src_rdy_o(_combiner1_valid), .dst_rdy_i(_combiner1_ready)
    );

    fifo36_mux com_output_source(
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr),
        .data0_i(_combiner0_data), .src0_rdy_i(_combiner0_valid), .dst0_rdy_o(_combiner0_ready),
        .data1_i(_combiner1_data), .src1_rdy_i(_combiner1_valid), .dst1_rdy_o(_combiner1_ready),
        .data_o(udp_out_data), .src_rdy_o(udp_out_valid), .dst_rdy_i(udp_out_ready)
    );

    ////////////////////////////////////////////////////////////////////
    // Interface CPU output to memory mapped wishbone
    ////////////////////////////////////////////////////////////////////
    localparam CPU_OUT_STATE_WAIT_SOF = 0;
    localparam CPU_OUT_STATE_WAIT_EOF = 1;
    localparam CPU_OUT_STATE_WAIT_CTRL_HI = 2;
    localparam CPU_OUT_STATE_WAIT_CTRL_LO = 3;

    reg [1:0] cpu_out_state;
    reg [BUF_SIZE-1:0] cpu_out_addr;
    assign cpu_out_line_count = cpu_out_addr;
    wire [BUF_SIZE-1:0] cpu_out_addr_next = cpu_out_addr + 1'b1;

    assign cpu_out_ready = (
        cpu_out_state == CPU_OUT_STATE_WAIT_SOF ||
        cpu_out_state == CPU_OUT_STATE_WAIT_EOF
    )? 1'b1 : 1'b0;
    assign cpu_out_hs_stat = (cpu_out_state == CPU_OUT_STATE_WAIT_CTRL_HI)? 1'b1 : 1'b0;

    RAMB16_S36_S36 cpu_out_buff(
        //port A = wishbone memory mapped address space (output only)
        .DOA(wb_dat_o),.ADDRA(wb_adr_i[BUF_SIZE+1:2]),.CLKA(wb_clk_i),.DIA(36'b0),.DIPA(4'h0),
        .ENA(wb_stb_i & (which_buf == 1'b0)),.SSRA(0),.WEA(wb_we_i),
        //port B = packet router interface to CPU (input only)
        .DOB(),.ADDRB(cpu_out_addr),.CLKB(stream_clk),.DIB(cpu_out_data[31:0]),.DIPB(4'h0),
        .ENB(cpu_out_ready & cpu_out_valid),.SSRB(0),.WEB(cpu_out_ready & cpu_out_valid)
    );

    always @(posedge stream_clk)
    if(stream_rst | stream_clr | mode_changed) begin
        cpu_out_state <= CPU_OUT_STATE_WAIT_SOF;
        cpu_out_addr <= 0;
    end
    else begin
        case(cpu_out_state)
        CPU_OUT_STATE_WAIT_SOF: begin
            if (cpu_out_ready & cpu_out_valid & cpu_out_data[32]) begin
                cpu_out_state <= CPU_OUT_STATE_WAIT_EOF;
                cpu_out_addr <= cpu_out_addr_next;
            end
        end

        CPU_OUT_STATE_WAIT_EOF: begin
            if (cpu_out_ready & cpu_out_valid & cpu_out_data[33]) begin
                cpu_out_state <= CPU_OUT_STATE_WAIT_CTRL_HI;
            end
            if (cpu_out_ready & cpu_out_valid) begin
                cpu_out_addr <= cpu_out_addr_next;
            end
        end

        CPU_OUT_STATE_WAIT_CTRL_HI: begin
            if (cpu_out_hs_ctrl == 1'b1) begin
                cpu_out_state <= CPU_OUT_STATE_WAIT_CTRL_LO;
            end
        end

        CPU_OUT_STATE_WAIT_CTRL_LO: begin
            if (cpu_out_hs_ctrl == 1'b0) begin
                cpu_out_state <= CPU_OUT_STATE_WAIT_SOF;
            end
            cpu_out_addr <= 0; //reset the address counter
        end

        endcase //cpu_out_state
    end

    ////////////////////////////////////////////////////////////////////
    // Interface CPU input to memory mapped wishbone
    ////////////////////////////////////////////////////////////////////
    localparam CPU_INP_STATE_WAIT_CTRL_HI = 0;
    localparam CPU_INP_STATE_WAIT_CTRL_LO = 1;
    localparam CPU_INP_STATE_UNLOAD = 2;

    reg [1:0] cpu_inp_state;
    reg [BUF_SIZE-1:0] cpu_inp_addr;
    wire [BUF_SIZE-1:0] cpu_inp_addr_next = cpu_inp_addr + 1'b1;

    reg [BUF_SIZE-1:0] cpu_inp_line_count_reg;

    assign cpu_inp_data[35:32] =
        (cpu_inp_addr == 1                     )? 4'b0001 : (
        (cpu_inp_addr == cpu_inp_line_count_reg)? 4'b0010 : (
    4'b0000));

    wire cpu_inp_enb = (cpu_inp_state == CPU_INP_STATE_UNLOAD)? (cpu_inp_ready & cpu_inp_valid) : 1'b1;
    assign cpu_inp_valid = (cpu_inp_state == CPU_INP_STATE_UNLOAD)? 1'b1 : 1'b0;
    assign cpu_inp_hs_stat = (cpu_inp_state == CPU_INP_STATE_WAIT_CTRL_HI)? 1'b1 : 1'b0;

    RAMB16_S36_S36 cpu_inp_buff(
        //port A = wishbone memory mapped address space (input only)
        .DOA(),.ADDRA(wb_adr_i[BUF_SIZE+1:2]),.CLKA(wb_clk_i),.DIA(wb_dat_i),.DIPA(4'h0),
        .ENA(wb_stb_i & (which_buf == 1'b1)),.SSRA(0),.WEA(wb_we_i),
        //port B = packet router interface from CPU (output only)
        .DOB(cpu_inp_data[31:0]),.ADDRB(cpu_inp_addr),.CLKB(stream_clk),.DIB(36'b0),.DIPB(4'h0),
        .ENB(cpu_inp_enb),.SSRB(0),.WEB(1'b0)
    );

    always @(posedge stream_clk)
    if(stream_rst | stream_clr | mode_changed) begin
        cpu_inp_state <= CPU_INP_STATE_WAIT_CTRL_HI;
        cpu_inp_addr <= 0;
    end
    else begin
        case(cpu_inp_state)
        CPU_INP_STATE_WAIT_CTRL_HI: begin
            if (cpu_inp_hs_ctrl == 1'b1) begin
                cpu_inp_state <= CPU_INP_STATE_WAIT_CTRL_LO;
            end
            cpu_inp_line_count_reg <= cpu_inp_line_count;
        end

        CPU_INP_STATE_WAIT_CTRL_LO: begin
            if (cpu_inp_hs_ctrl == 1'b0) begin
                cpu_inp_state <= CPU_INP_STATE_UNLOAD;
                cpu_inp_addr <= cpu_inp_addr_next;
            end
        end

        CPU_INP_STATE_UNLOAD: begin
            if (cpu_inp_ready & cpu_inp_valid) begin
                if (cpu_inp_data[33]) begin
                    cpu_inp_addr <= 0;
                    cpu_inp_state <= CPU_INP_STATE_WAIT_CTRL_HI;
                end
                else begin
                    cpu_inp_addr <= cpu_inp_addr_next;
                end
            end
        end

        endcase //cpu_inp_state
    end

    ////////////////////////////////////////////////////////////////////
    // Communication input inspector
    //   - inspect com input and send it to DSP, EXT, CPU, or BOTH
    ////////////////////////////////////////////////////////////////////
    localparam COM_INSP_STATE_READ_COM_PRE = 0;
    localparam COM_INSP_STATE_READ_COM = 1;
    localparam COM_INSP_STATE_WRITE_REGS = 2;
    localparam COM_INSP_STATE_WRITE_LIVE = 3;

    localparam COM_INSP_DEST_DSP = 0;
    localparam COM_INSP_DEST_EXT = 1;
    localparam COM_INSP_DEST_CPU = 2;
    localparam COM_INSP_DEST_BOF = 3;

    localparam COM_INSP_MAX_NUM_DREGS = 13; //padded_eth + ip + udp + seq + vrt_hdr
    localparam COM_INSP_DREGS_DSP_OFFSET = 11; //offset to start dsp at

    //output inspector interfaces
    wire [35:0] com_insp_out_dsp_data;
    wire        com_insp_out_dsp_valid;
    wire        com_insp_out_dsp_ready;

    wire [35:0] com_insp_out_ext_data;
    wire        com_insp_out_ext_valid;
    wire        com_insp_out_ext_ready;

    wire [35:0] com_insp_out_cpu_data;
    wire        com_insp_out_cpu_valid;
    wire        com_insp_out_cpu_ready;

    wire [35:0] com_insp_out_bof_data;
    wire        com_insp_out_bof_valid;
    wire        com_insp_out_bof_ready;

    //connect this fast-path signals directly to the DSP out
    assign dsp_out_data = com_insp_out_dsp_data;
    assign dsp_out_valid = com_insp_out_dsp_valid;
    assign com_insp_out_dsp_ready = dsp_out_ready;

    reg [1:0] com_insp_state;
    reg [1:0] com_insp_dest;
    reg [3:0] com_insp_dreg_count; //data registers to buffer headers
    wire [3:0] com_insp_dreg_count_next = com_insp_dreg_count + 1'b1;
    wire com_insp_dreg_counter_done = (com_insp_dreg_count_next == COM_INSP_MAX_NUM_DREGS)? 1'b1 : 1'b0;
    reg [35:0] com_insp_dregs [COM_INSP_MAX_NUM_DREGS-1:0];

    //extract various packet components:
    wire [47:0] com_insp_dregs_eth_dst_mac   = {com_insp_dregs[0][15:0], com_insp_dregs[1][31:0]};
    wire [15:0] com_insp_dregs_eth_type      = com_insp_dregs[3][15:0];
    wire [7:0]  com_insp_dregs_ipv4_proto    = com_insp_dregs[6][23:16];
    wire [31:0] com_insp_dregs_ipv4_dst_addr = com_insp_dregs[8][31:0];
    wire [15:0] com_insp_dregs_udp_dst_port  = com_insp_dregs[9][15:0];
    wire [15:0] com_insp_dregs_vrt_size      = com_inp_data[15:0];

    //Inspector output flags special case:
    //Inject SOF into flags at first DSP line.
    wire [3:0] com_insp_out_flags = (
        (com_insp_dreg_count == COM_INSP_DREGS_DSP_OFFSET) &&
        (com_insp_dest == COM_INSP_DEST_DSP)
    )? 4'b0001 : com_insp_dregs[com_insp_dreg_count][35:32];

    //The communication inspector ouput data and valid signals:
    //Mux between com input and data registers based on the state.
    wire [35:0] com_insp_out_data = (com_insp_state == COM_INSP_STATE_WRITE_REGS)?
        {com_insp_out_flags, com_insp_dregs[com_insp_dreg_count][31:0]} : com_inp_data
    ;
    wire com_insp_out_valid =
        (com_insp_state == COM_INSP_STATE_WRITE_REGS)? 1'b1          : (
        (com_insp_state == COM_INSP_STATE_WRITE_LIVE)? com_inp_valid : (
    1'b0));

    //The communication inspector ouput ready signal:
    //Mux between the various destination ready signals.
    wire com_insp_out_ready =
        (com_insp_dest == COM_INSP_DEST_DSP)? com_insp_out_dsp_ready : (
        (com_insp_dest == COM_INSP_DEST_EXT)? com_insp_out_ext_ready : (
        (com_insp_dest == COM_INSP_DEST_CPU)? com_insp_out_cpu_ready : (
        (com_insp_dest == COM_INSP_DEST_BOF)? com_insp_out_bof_ready : (
    1'b0))));

    //Always connected output data lines.
    assign com_insp_out_dsp_data = com_insp_out_data;
    assign com_insp_out_ext_data = com_insp_out_data;
    assign com_insp_out_cpu_data = com_insp_out_data;
    assign com_insp_out_bof_data = com_insp_out_data;

    //Destination output valid signals:
    //Comes from inspector valid when destination is selected, and otherwise low.
    assign com_insp_out_dsp_valid = (com_insp_dest == COM_INSP_DEST_DSP)? com_insp_out_valid : 1'b0;
    assign com_insp_out_ext_valid = (com_insp_dest == COM_INSP_DEST_EXT)? com_insp_out_valid : 1'b0;
    assign com_insp_out_cpu_valid = (com_insp_dest == COM_INSP_DEST_CPU)? com_insp_out_valid : 1'b0;
    assign com_insp_out_bof_valid = (com_insp_dest == COM_INSP_DEST_BOF)? com_insp_out_valid : 1'b0;

    //The communication inspector ouput ready signal:
    //Always ready when storing to data registers,
    //comes from inspector ready output when live,
    //and otherwise low.
    assign com_inp_ready =
        (com_insp_state == COM_INSP_STATE_READ_COM_PRE)  ? 1'b1               : (
        (com_insp_state == COM_INSP_STATE_READ_COM)      ? 1'b1               : (
        (com_insp_state == COM_INSP_STATE_WRITE_LIVE)    ? com_insp_out_ready : (
    1'b0)));

    always @(posedge stream_clk)
    if(stream_rst | stream_clr) begin
        com_insp_state <= COM_INSP_STATE_READ_COM_PRE;
        com_insp_dreg_count <= 0;
    end
    else begin
        case(com_insp_state)
        COM_INSP_STATE_READ_COM_PRE: begin
            if (com_inp_ready & com_inp_valid & com_inp_data[32]) begin
                com_insp_state <= COM_INSP_STATE_READ_COM;
                com_insp_dreg_count <= com_insp_dreg_count_next;
                com_insp_dregs[com_insp_dreg_count] <= com_inp_data;
            end
        end

        COM_INSP_STATE_READ_COM: begin
            if (com_inp_ready & com_inp_valid) begin
                com_insp_dregs[com_insp_dreg_count] <= com_inp_data;
                if (com_insp_dreg_counter_done | com_inp_data[33]) begin
                    com_insp_state <= COM_INSP_STATE_WRITE_REGS;
                    com_insp_dreg_count <= 0;

                    //---------- begin inspection decision -----------//
                    //EOF or bcast or not IPv4 or not UDP:
                    if (
                        com_inp_data[33] || (com_insp_dregs_eth_dst_mac == 48'hffffffffffff) ||
                        (com_insp_dregs_eth_type != 16'h800) || (com_insp_dregs_ipv4_proto != 8'h11)
                    ) begin
                        com_insp_dest <= COM_INSP_DEST_BOF;
                    end

                    //not my IP address:
                    else if (com_insp_dregs_ipv4_dst_addr != my_ip_addr) begin
                        com_insp_dest <= COM_INSP_DEST_EXT;
                    end

                    //UDP data port and VRT:
                    else if ((com_insp_dregs_udp_dst_port == dsp0_udp_port) && (com_insp_dregs_vrt_size != 16'h0)) begin
                        com_insp_dest <= COM_INSP_DEST_DSP;
                        com_insp_dreg_count <= COM_INSP_DREGS_DSP_OFFSET;
                    end

                    //other:
                    else begin
                        com_insp_dest <= COM_INSP_DEST_CPU;
                    end
                    //---------- end inspection decision -------------//

                end
                else begin
                    com_insp_dreg_count <= com_insp_dreg_count_next;
                end
            end
        end

        COM_INSP_STATE_WRITE_REGS: begin
            if (com_insp_out_ready & com_insp_out_valid) begin
                if (com_insp_out_data[33]) begin
                    com_insp_state <= COM_INSP_STATE_READ_COM_PRE;
                    com_insp_dreg_count <= 0;
                end
                else if (com_insp_dreg_counter_done) begin
                    com_insp_state <= COM_INSP_STATE_WRITE_LIVE;
                    com_insp_dreg_count <= 0;
                end
                else begin
                    com_insp_dreg_count <= com_insp_dreg_count_next;
                end
            end
        end

        COM_INSP_STATE_WRITE_LIVE: begin
            if (com_insp_out_ready & com_insp_out_valid & com_insp_out_data[33]) begin
                com_insp_state <= COM_INSP_STATE_READ_COM_PRE;
            end
        end

        endcase //com_insp_state
    end

    ////////////////////////////////////////////////////////////////////
    // Splitter and output muxes for the bof packets
    //   - split the bof packets into two streams
    //   - mux split packets into cpu out and ext out
    ////////////////////////////////////////////////////////////////////

    //dummy signals to join the the splitter and muxes below
    wire [35:0] _split_to_ext_data,  _split_to_cpu_data,  _cpu_out_data;
    wire        _split_to_ext_valid, _split_to_cpu_valid, _cpu_out_valid;
    wire        _split_to_ext_ready, _split_to_cpu_ready, _cpu_out_ready;

    splitter36 bof_out_splitter(
        .clk(stream_clk), .rst(stream_rst), .clr(stream_clr),
        .inp_data(com_insp_out_bof_data), .inp_valid(com_insp_out_bof_valid), .inp_ready(com_insp_out_bof_ready),
        .out0_data(_split_to_ext_data),   .out0_valid(_split_to_ext_valid),   .out0_ready(_split_to_ext_ready),
        .out1_data(_split_to_cpu_data),   .out1_valid(_split_to_cpu_valid),   .out1_ready(_split_to_cpu_ready)
    );

    fifo36_mux ext_out_mux(
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr),
        .data0_i(com_insp_out_ext_data), .src0_rdy_i(com_insp_out_ext_valid), .dst0_rdy_o(com_insp_out_ext_ready),
        .data1_i(_split_to_ext_data),    .src1_rdy_i(_split_to_ext_valid),    .dst1_rdy_o(_split_to_ext_ready),
        .data_o(ext_out_data),           .src_rdy_o(ext_out_valid),           .dst_rdy_i(ext_out_ready)
    );

    fifo36_mux cpu_out_mux(
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr),
        .data0_i(com_insp_out_cpu_data), .src0_rdy_i(com_insp_out_cpu_valid), .dst0_rdy_o(com_insp_out_cpu_ready),
        .data1_i(_split_to_cpu_data),    .src1_rdy_i(_split_to_cpu_valid),    .dst1_rdy_o(_split_to_cpu_ready),
        .data_o(_cpu_out_data),          .src_rdy_o(_cpu_out_valid),          .dst_rdy_i(_cpu_out_ready)
    );

    fifo_cascade #(.WIDTH(36), .SIZE(9/*512 lines plenty for short pkts*/)) cpu_out_fifo (
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr),
        .datain(_cpu_out_data), .src_rdy_i(_cpu_out_valid), .dst_rdy_o(_cpu_out_ready),
        .dataout(cpu_out_data), .src_rdy_o(cpu_out_valid),  .dst_rdy_i(cpu_out_ready)
    );

    ////////////////////////////////////////////////////////////////////
    // DSP input framer
    ////////////////////////////////////////////////////////////////////

    dsp_framer36 #(.BUF_SIZE(BUF_SIZE)) dsp0_framer36(
        .clk(stream_clk), .rst(stream_rst), .clr(stream_clr),
        .inp_data(dsp_inp_data), .inp_valid(dsp_inp_valid), .inp_ready(dsp_inp_ready),
        .out_data(dsp_frm_data), .out_valid(dsp_frm_valid), .out_ready(dsp_frm_ready)
    );

    ////////////////////////////////////////////////////////////////////
    // UDP TX Protocol machine
    ////////////////////////////////////////////////////////////////////

    //dummy signals to connect the components below
    wire [18:0] _udp_r2s_data, _udp_s2p_data, _udp_p2s_data, _udp_s2r_data;
    wire _udp_r2s_valid, _udp_s2p_valid, _udp_p2s_valid, _udp_s2r_valid;
    wire _udp_r2s_ready, _udp_s2p_ready, _udp_p2s_ready, _udp_s2r_ready;

    wire [35:0] _com_out_data;
    wire _com_out_valid, _com_out_ready;

    fifo36_to_fifo19 udp_fifo36_to_fifo19
     (.clk(stream_clk), .reset(stream_rst), .clear(stream_clr),
      .f36_datain(udp_out_data),   .f36_src_rdy_i(udp_out_valid),  .f36_dst_rdy_o(udp_out_ready),
      .f19_dataout(_udp_r2s_data), .f19_src_rdy_o(_udp_r2s_valid), .f19_dst_rdy_i(_udp_r2s_ready) );

    fifo_short #(.WIDTH(19)) udp_shortfifo19_inp
     (.clk(stream_clk), .reset(stream_rst), .clear(stream_clr),
      .datain(_udp_r2s_data),  .src_rdy_i(_udp_r2s_valid), .dst_rdy_o(_udp_r2s_ready),
      .dataout(_udp_s2p_data), .src_rdy_o(_udp_s2p_valid), .dst_rdy_i(_udp_s2p_ready),
      .space(), .occupied() );

    prot_eng_tx #(.BASE(UDP_BASE)) udp_prot_eng_tx
     (.clk(stream_clk), .reset(stream_rst), .clear(stream_clr),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .datain(_udp_s2p_data),  .src_rdy_i(_udp_s2p_valid), .dst_rdy_o(_udp_s2p_ready),
      .dataout(_udp_p2s_data), .src_rdy_o(_udp_p2s_valid), .dst_rdy_i(_udp_p2s_ready) );

    fifo_short #(.WIDTH(19)) udp_shortfifo19_out
     (.clk(stream_clk), .reset(stream_rst), .clear(stream_clr),
      .datain(_udp_p2s_data),  .src_rdy_i(_udp_p2s_valid), .dst_rdy_o(_udp_p2s_ready),
      .dataout(_udp_s2r_data), .src_rdy_o(_udp_s2r_valid), .dst_rdy_i(_udp_s2r_ready),
      .space(), .occupied() );

    fifo19_to_fifo36 udp_fifo19_to_fifo36
     (.clk(stream_clk), .reset(stream_rst), .clear(stream_clr),
      .f19_datain(_udp_s2r_data), .f19_src_rdy_i(_udp_s2r_valid), .f19_dst_rdy_o(_udp_s2r_ready),
      .f36_dataout(_com_out_data), .f36_src_rdy_o(_com_out_valid),  .f36_dst_rdy_i(_com_out_ready) );

    fifo36_mux com_out_mux(
        .clk(stream_clk), .reset(stream_rst), .clear(stream_clr),
        .data0_i(ext_inp_data),  .src0_rdy_i(ext_inp_valid),  .dst0_rdy_o(ext_inp_ready),
        .data1_i(_com_out_data), .src1_rdy_i(_com_out_valid), .dst1_rdy_o(_com_out_ready),
        .data_o(com_out_data),   .src_rdy_o(com_out_valid),   .dst_rdy_i(com_out_ready)
    );

    ////////////////////////////////////////////////////////////////////
    // Assign debugs
    ////////////////////////////////////////////////////////////////////

    assign debug = {
        //inputs to the router (8)
        dsp_inp_ready, dsp_inp_valid,
        ser_inp_ready, ser_inp_valid,
        eth_inp_ready, eth_inp_valid,
        cpu_inp_ready, cpu_inp_valid,

        //outputs from the router (8)
        dsp_out_ready, dsp_out_valid,
        ser_out_ready, ser_out_valid,
        eth_out_ready, eth_out_valid,
        cpu_out_ready, cpu_out_valid,

        //inspector interfaces (8)
        com_insp_out_dsp_ready, com_insp_out_dsp_valid,
        com_insp_out_ext_ready, com_insp_out_ext_valid,
        com_insp_out_cpu_ready, com_insp_out_cpu_valid,
        com_insp_out_bof_ready, com_insp_out_bof_valid,

        //other interfaces (8)
        ext_inp_ready, ext_inp_valid,
        com_out_ready, com_out_valid,
        ext_out_ready, ext_out_valid,
        com_inp_ready, com_inp_valid
    };

endmodule // packet_router

module packet_router
    #(parameter BUF_SIZE = 9)
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

        input stream_clk,
        input stream_rst,

        //input control register
        input [31:0] control,

        //output status register
        output [31:0] status,

        output sys_int_o, //want an interrupt?

        // Input Interfaces (in to router)
        input [35:0] ser_inp_data, input ser_inp_valid, output ser_inp_ready,
        input [35:0] dsp_inp_data, input dsp_inp_valid, output dsp_inp_ready,
        input [35:0] eth_inp_data, input eth_inp_valid, output eth_inp_ready,

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
    wire [35:0] cpu_inp_data;
    wire        cpu_inp_valid;
    wire        cpu_inp_ready;
    wire [35:0] cpu_out_data;
    wire        cpu_out_valid;
    wire        cpu_out_ready;

    ////////////////////////////////////////////////////////////////////
    // Communication interfaces
    ////////////////////////////////////////////////////////////////////
    wire [35:0] com_inp_data;
    wire        com_inp_valid;
    wire        com_inp_ready;
    wire [35:0] com_out_data;
    wire        com_out_valid;
    wire        com_out_ready;

    ////////////////////////////////////////////////////////////////////
    // status and control handshakes
    ////////////////////////////////////////////////////////////////////
    wire cpu_out_hs_ctrl = control[0];
    wire cpu_inp_hs_ctrl = control[1];
    wire [BUF_SIZE-1:0] cpu_inp_line_count = control[BUF_SIZE-1+16:0+16];

    wire cpu_out_hs_stat;
    assign status[0] = cpu_out_hs_stat;

    wire [BUF_SIZE-1:0] cpu_out_line_count;
    assign status[BUF_SIZE-1+16:0+16] = cpu_out_line_count;

    wire cpu_inp_hs_stat;
    assign status[1] = cpu_inp_hs_stat;

    ////////////////////////////////////////////////////////////////////
    // Communication input source combiner
    //   - combine streams from serdes and ethernet
    ////////////////////////////////////////////////////////////////////
    fifo36_mux com_input_source(
        .clk(stream_clk), .reset(stream_rst), .clear(1'b0),
        .data0_i(eth_inp_data), .src0_rdy_i(eth_inp_valid), .dst0_rdy_o(eth_inp_ready),
        .data1_i(ser_inp_data), .src1_rdy_i(ser_inp_valid), .dst1_rdy_o(ser_inp_ready),
        .data_o(com_inp_data), .src_rdy_o(com_inp_valid), .dst_rdy_i(com_inp_ready)
    );

    ////////////////////////////////////////////////////////////////////
    // Communication output sink crossbar
    //   - when the link is up (master): com -> eth and insp -> serdes
    //   - when the link is down (slave): com -> serdes (insp disconnected)
    ////////////////////////////////////////////////////////////////////
    wire eth_link_is_up = 1'b1; //TODO should come from input or register

    //streaming signals from the inspector to the crossbar
    wire [35:0] ser_crs_data;
    wire        ser_crs_valid;
    wire        ser_crs_ready;

    //connect the ethernet source output signals
    assign eth_out_data = com_out_data;
    assign eth_out_valid = (eth_link_is_up)? com_out_valid : 1'b0;

    //connect the serdes source output signals
    assign ser_out_data = (eth_link_is_up)? ser_crs_data : com_out_data;
    assign ser_out_valid = (eth_link_is_up)? ser_crs_valid : com_out_valid;

    //connect the crossbar sink output signals
    assign com_out_ready = (eth_link_is_up)? eth_out_ready : ser_out_ready;
    assign ser_crs_ready = (eth_link_is_up)? ser_out_ready : 1'b1/*null sink*/;

    ////////////////////////////////////////////////////////////////////
    // Communication output source combiner
    //   - combine streams from dsp framer, com inspector, and cpu
    ////////////////////////////////////////////////////////////////////

    //streaming signals from the dsp framer to the com mux
    wire [35:0] dsp_frm_data;
    wire        dsp_frm_valid;
    wire        dsp_frm_ready;

    fifo36_mux com_output_source(
        .clk(stream_clk), .reset(stream_rst), .clear(1'b0),
        .data0_i(dsp_frm_data), .src0_rdy_i(dsp_frm_valid), .dst0_rdy_o(dsp_frm_ready),
        .data1_i(cpu_inp_data), .src1_rdy_i(cpu_inp_valid), .dst1_rdy_o(cpu_inp_ready),
        .data_o(com_out_data), .src_rdy_o(com_out_valid), .dst_rdy_i(com_out_ready)
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

    wire cpu_out_reading = (
        cpu_out_state == CPU_OUT_STATE_WAIT_SOF ||
        cpu_out_state == CPU_OUT_STATE_WAIT_EOF
    )? 1'b1 : 1'b0;

    wire cpu_out_we = cpu_out_reading;
    assign cpu_out_ready = cpu_out_reading;
    assign cpu_out_hs_stat = (cpu_out_state == CPU_OUT_STATE_WAIT_CTRL_HI)? 1'b1 : 1'b0;

    RAMB16_S36_S36 cpu_out_buff(
        //port A = wishbone memory mapped address space (output only)
        .DOA(wb_dat_o),.ADDRA(wb_adr_i[BUF_SIZE+1:2]),.CLKA(wb_clk_i),.DIA(36'b0),.DIPA(4'h0),
        .ENA(wb_stb_i & (which_buf == 1'b0)),.SSRA(0),.WEA(wb_we_i),
        //port B = packet router interface to CPU (input only)
        .DOB(),.ADDRB(cpu_out_addr),.CLKB(stream_clk),.DIB(cpu_out_data),.DIPB(4'h0),
        .ENB(cpu_out_we),.SSRB(0),.WEB(cpu_out_we)
    );

    always @(posedge stream_clk)
    if(stream_rst) begin
        cpu_out_state <= CPU_OUT_STATE_WAIT_SOF;
        cpu_out_addr <= 0;
    end
    else begin
        case(cpu_out_state)
        CPU_OUT_STATE_WAIT_SOF: begin
            if (cpu_out_ready & cpu_out_valid & (cpu_out_data[32] == 1'b1)) begin
                cpu_out_state <= CPU_OUT_STATE_WAIT_EOF;
                cpu_out_addr <= cpu_out_addr_next;
            end
        end

        CPU_OUT_STATE_WAIT_EOF: begin
            if (cpu_out_ready & cpu_out_valid & (cpu_out_data[33] == 1'b1)) begin
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

    reg cpu_inp_flag_sof;
    reg cpu_inp_flag_eof;
    assign cpu_inp_data[35:32] = {2'b0, cpu_inp_flag_eof, cpu_inp_flag_sof};

    assign cpu_inp_valid = (cpu_inp_state == CPU_INP_STATE_UNLOAD)? 1'b1 : 1'b0;
    assign cpu_inp_hs_stat = (cpu_inp_state == CPU_INP_STATE_WAIT_CTRL_HI)? 1'b1 : 1'b0;

    RAMB16_S36_S36 cpu_inp_buff(
        //port A = wishbone memory mapped address space (input only)
        .DOA(),.ADDRA(wb_adr_i[BUF_SIZE+1:2]),.CLKA(wb_clk_i),.DIA(wb_dat_i),.DIPA(4'h0),
        .ENA(wb_stb_i & (which_buf == 1'b1)),.SSRA(0),.WEA(wb_we_i),
        //port B = packet router interface from CPU (output only)
        .DOB(cpu_inp_data[31:0]),.ADDRB(cpu_inp_addr),.CLKB(stream_clk),.DIB(36'b0),.DIPB(4'h0),
        .ENB(1'b1),.SSRB(0),.WEB(1'b0)
    );

    always @(posedge stream_clk)
    if(stream_rst) begin
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
            cpu_inp_addr <= 0; //reset the address counter
        end

        CPU_INP_STATE_WAIT_CTRL_LO: begin
            if (cpu_inp_hs_ctrl == 1'b0) begin
                cpu_inp_state <= CPU_INP_STATE_UNLOAD;
                cpu_inp_addr <= cpu_inp_addr_next; //BRAM has a setup delay and this is a bug
            end
            cpu_inp_flag_sof <= 1'b1;
            cpu_inp_flag_eof <= 1'b0;
        end

        CPU_INP_STATE_UNLOAD: begin
            if (cpu_inp_ready & cpu_inp_valid) begin
                cpu_inp_addr <= cpu_inp_addr_next;
                cpu_inp_flag_sof <= 1'b0;
                if (cpu_inp_addr == cpu_inp_line_count_reg) begin
                    cpu_inp_flag_eof <= 1'b1;
                end
                else begin
                    cpu_inp_flag_eof <= 1'b0;
                end
                if (cpu_inp_flag_eof) begin
                    cpu_inp_state <= CPU_INP_STATE_WAIT_CTRL_HI;
                end
            end
        end

        endcase //cpu_inp_state
    end

    ////////////////////////////////////////////////////////////////////
    // Communication input inspector
    //   - inspect com input and send it to CPU, DSP, or COM
    ////////////////////////////////////////////////////////////////////
    localparam COM_INSP_STATE_READ_COM_PRE = 0;
    localparam COM_INSP_STATE_READ_COM = 1;
    localparam COM_INSP_STATE_WRITE_REGS = 2;
    localparam COM_INSP_STATE_WRITE_LIVE = 3;

    localparam COM_INSP_DEST_DSP = 0;
    localparam COM_INSP_DEST_SER = 1;
    localparam COM_INSP_DEST_CPU = 2;

    localparam COM_INSP_MAX_NUM_DREGS = 12; //padded_eth + ip + udp + vrt_hdr
    localparam COM_INSP_DREGS_DSP_OFFSET = 11; //offset to start dsp at

    reg [1:0] com_insp_state;
    reg [1:0] com_insp_dest;
    reg [3:0] com_insp_dreg_count; //data registers to buffer headers
    wire [3:0] com_insp_dreg_count_next = com_insp_dreg_count + 1'b1;
    wire com_insp_dreg_counter_done = (com_insp_dreg_count_next == COM_INSP_MAX_NUM_DREGS)? 1'b1 : 1'b0;
    reg [35:0] com_insp_dregs [COM_INSP_MAX_NUM_DREGS-1:0];

    //Inspection logic:
    wire com_inp_dregs_is_data = 1'b1
        & (com_insp_dregs[3][15:0] == 16'h800)    //ethertype IPv4
        & (com_insp_dregs[6][23:16] == 8'h11)     //protocol UDP
        & (com_insp_dregs[9][15:0] == 16'd49153)  //UDP data port
        & (com_inp_data[31:0] != 32'h0)           //VRT hdr non-zero
    ;

    //Inspector output flags special case:
    //Inject SOF into flags at first DSP line.
    wire [3:0] com_insp_out_flags = ((com_insp_dreg_count == COM_INSP_DREGS_DSP_OFFSET) & (com_insp_dest == COM_INSP_DEST_DSP))?
        4'b0001 : com_insp_dregs[com_insp_dreg_count][35:32]
    ;

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
        (com_insp_dest == COM_INSP_DEST_CPU)? cpu_out_ready : (
        (com_insp_dest == COM_INSP_DEST_DSP)? dsp_out_ready : (
    1'b0));

    //Always connected output data lines.
    assign cpu_out_data = com_insp_out_data;
    assign dsp_out_data = com_insp_out_data;
    assign ser_crs_data = com_insp_out_data;

    //Destination output valid signals:
    //Comes from inspector valid when destination is selected, and otherwise low.
    assign cpu_out_valid = (com_insp_dest == COM_INSP_DEST_CPU)? com_insp_out_valid : 1'b0;
    assign dsp_out_valid = (com_insp_dest == COM_INSP_DEST_DSP)? com_insp_out_valid : 1'b0;
    assign ser_crs_valid = (com_insp_dest == COM_INSP_DEST_SER)? com_insp_out_valid : 1'b0;

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
    if(stream_rst) begin
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
                if (com_inp_dregs_is_data & com_insp_dreg_counter_done) begin
                    com_insp_dest <= COM_INSP_DEST_DSP;
                    com_insp_state <= COM_INSP_STATE_WRITE_REGS;
                    com_insp_dreg_count <= COM_INSP_DREGS_DSP_OFFSET;
                end
                else if (com_inp_data[33] | com_insp_dreg_counter_done) begin
                    com_insp_dest <= COM_INSP_DEST_CPU;
                    com_insp_state <= COM_INSP_STATE_WRITE_REGS;
                    com_insp_dreg_count <= 0;
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
    // DSP input framer
    //   - add a 1-line frame header to each DSP input packet
    //   - each header is composed of a byte count and flags
    ////////////////////////////////////////////////////////////////////

    localparam DSP_FRM_STATE_WAIT_SOF = 0;
    localparam DSP_FRM_STATE_WAIT_EOF = 1;
    localparam DSP_FRM_STATE_WRITE_HDR = 2;
    localparam DSP_FRM_STATE_WRITE = 3;

    reg [1:0] dsp_frm_state;
    reg [BUF_SIZE-1:0] dsp_frm_addr;
    reg [BUF_SIZE-1:0] dsp_frm_count;
    wire [BUF_SIZE-1:0] dsp_frm_addr_next = dsp_frm_addr + 1'b1;
    reg dsp_frm_valid_reg; //registered valid to deal with read delay

    //DSP input stream ready in the following states
    assign dsp_inp_ready =
        (dsp_frm_state == DSP_FRM_STATE_WAIT_SOF)? 1'b1 : (
        (dsp_frm_state == DSP_FRM_STATE_WAIT_EOF)? 1'b1 : (
    1'b0));

    //DSP framer output data mux (header or BRAM):
    //The header is generated here from the count.
    wire [31:0] dsp_frm_data_bram;
    wire [15:0] dsp_frm_bytes = {dsp_frm_count, 2'b00};
    assign dsp_frm_data =
        (dsp_frm_state == DSP_FRM_STATE_WRITE_HDR)? {4'b0001, 16'b1, dsp_frm_bytes} : (
        (dsp_frm_addr_next == dsp_frm_count)      ? {4'b0010, dsp_frm_data_bram}    : (
    {4'b0000, dsp_frm_data_bram}));
    assign dsp_frm_valid = dsp_frm_valid_reg;

    RAMB16_S36_S36 dsp_frm_buff(
        //port A = DSP input interface (writes to BRAM)
        .DOA(),.ADDRA(dsp_frm_addr),.CLKA(stream_clk),.DIA(dsp_inp_data[31:0]),.DIPA(4'h0),
        .ENA(dsp_inp_ready),.SSRA(0),.WEA(dsp_inp_ready),
        //port B = DSP framer interface (reads from BRAM)
        .DOB(dsp_frm_data_bram),.ADDRB(dsp_frm_addr),.CLKB(stream_clk),.DIB(36'b0),.DIPB(4'h0),
        .ENB(1'b1),.SSRB(0),.WEB(1'b0)
    );

    always @(posedge stream_clk)
    if(stream_rst) begin
        dsp_frm_state <= DSP_FRM_STATE_WAIT_SOF;
        dsp_frm_addr <= 0;
        dsp_frm_valid_reg <= 1'b0;
    end
    else begin
        case(dsp_frm_state)
        DSP_FRM_STATE_WAIT_SOF: begin
            if (dsp_inp_ready & dsp_inp_valid & dsp_inp_data[32]) begin
                dsp_frm_addr <= dsp_frm_addr_next;
                dsp_frm_state <= DSP_FRM_STATE_WAIT_EOF;
            end
        end

        DSP_FRM_STATE_WAIT_EOF: begin
            if (dsp_inp_ready & dsp_inp_valid) begin
                if (dsp_inp_data[33]) begin
                    dsp_frm_count <= dsp_frm_addr_next;
                    dsp_frm_addr <= 0;
                    dsp_frm_state <= DSP_FRM_STATE_WRITE_HDR;
                end
                else begin
                    dsp_frm_addr <= dsp_frm_addr_next;
                end
            end
        end

        DSP_FRM_STATE_WRITE_HDR: begin
            if (dsp_frm_ready & dsp_frm_valid) begin
                dsp_frm_addr <= 0;
                dsp_frm_state <= DSP_FRM_STATE_WRITE;
                dsp_frm_valid_reg <= 1'b0;
            end
            else begin
                dsp_frm_valid_reg <= 1'b1;
            end
        end

        DSP_FRM_STATE_WRITE: begin
            if (dsp_frm_ready & dsp_frm_valid) begin
                if (dsp_frm_data[33]) begin
                    dsp_frm_addr <= 0;
                    dsp_frm_state <= DSP_FRM_STATE_WAIT_SOF;
                end
                else begin
                    dsp_frm_addr <= dsp_frm_addr_next;
                end
                dsp_frm_valid_reg <= 1'b0;
            end
            else begin
                dsp_frm_valid_reg <= 1'b1;
            end
        end
        endcase //dsp_frm_state
    end

endmodule // packet_router

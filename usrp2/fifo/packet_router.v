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
        input [35:0] eth_inp_data, input eth_inp_valid, output eth_inp_ready,

        // Output Interfaces (out of router)
        output [35:0] dsp_out_data, output dsp_out_valid, input dsp_out_ready,
        output [35:0] eth_out_data, output eth_out_valid, input eth_out_ready
    );

    assign wb_err_o = 1'b0;  // Unused for now
    assign wb_rty_o = 1'b0;  // Unused for now
    always @(posedge wb_clk_i)
        wb_ack_o <= wb_stb_i & ~wb_ack_o;

    ////////////////////////////////////////////////////////////////////
    // CPU interface to this packet router
    ////////////////////////////////////////////////////////////////////
    wire [35:0] cpu_inp_data;
    wire        cpu_inp_valid;
    wire        cpu_inp_ready;
    wire [35:0] cpu_out_data;
    wire        cpu_out_valid;
    wire        cpu_out_ready;

    //which buffer: 0 = CPU read buffer, 1 = CPU write buffer
    wire which_buf = wb_adr_i[BUF_SIZE+2];

    ////////////////////////////////////////////////////////////////////
    // status and control handshakes
    ////////////////////////////////////////////////////////////////////
    wire cpu_inp_hs_ctrl = control[0];
    wire cpu_out_hs_ctrl = control[1];
    wire [BUF_SIZE-1:0] cpu_out_line_count = control[BUF_SIZE-1+16:0+16];

    wire cpu_inp_hs_stat;
    assign status[0] = cpu_inp_hs_stat;

    wire [BUF_SIZE-1:0] cpu_inp_line_count;
    assign status[BUF_SIZE-1+16:0+16] = cpu_inp_line_count;

    wire cpu_out_hs_stat;
    assign status[1] = cpu_out_hs_stat;

    ////////////////////////////////////////////////////////////////////
    // Ethernet input control
    ////////////////////////////////////////////////////////////////////
    //TODO: just connect eth input to cpu input for now
    //assign cpu_inp_data = eth_inp_data;
    //assign cpu_inp_valid = eth_inp_valid;
    //assign eth_inp_ready = cpu_inp_ready;

    ////////////////////////////////////////////////////////////////////
    // Ethernet output control
    ////////////////////////////////////////////////////////////////////
    //TODO: just connect eth output to cpu output for now
    assign eth_out_data = cpu_out_data;
    assign eth_out_valid = cpu_out_valid;
    assign cpu_out_ready = eth_out_ready;

    ////////////////////////////////////////////////////////////////////
    // Interface CPU input interface to memory mapped wishbone
    ////////////////////////////////////////////////////////////////////
    localparam CPU_INP_STATE_WAIT_SOF = 0;
    localparam CPU_INP_STATE_WAIT_EOF = 1;
    localparam CPU_INP_STATE_WAIT_CTRL_HI = 2;
    localparam CPU_INP_STATE_WAIT_CTRL_LO = 3;

    reg [1:0] cpu_inp_state;
    reg [BUF_SIZE-1:0] cpu_inp_addr;
    assign cpu_inp_line_count = cpu_inp_addr;
    wire [BUF_SIZE-1:0] cpu_inp_addr_next = cpu_inp_addr + 1'b1;

    wire cpu_inp_reading = (
        cpu_inp_state == CPU_INP_STATE_WAIT_SOF ||
        cpu_inp_state == CPU_INP_STATE_WAIT_EOF
    )? 1'b1 : 1'b0;

    wire cpu_inp_we = cpu_inp_reading;
    assign cpu_inp_ready = cpu_inp_reading;
    assign cpu_inp_hs_stat = (cpu_inp_state == CPU_INP_STATE_WAIT_CTRL_HI)? 1'b1 : 1'b0;

    RAMB16_S36_S36 cpu_inp_buff(
        //port A = wishbone memory mapped address space (output only)
        .DOA(wb_dat_o),.ADDRA(wb_adr_i[BUF_SIZE+1:2]),.CLKA(wb_clk_i),.DIA(36'b0),.DIPA(4'h0),
        .ENA(wb_stb_i & (which_buf == 1'b0)),.SSRA(0),.WEA(wb_we_i),
        //port B = packet router interface to CPU (input only)
        .DOB(),.ADDRB(cpu_inp_addr),.CLKB(stream_clk),.DIB(cpu_inp_data),.DIPB(4'h0),
        .ENB(cpu_inp_we),.SSRB(0),.WEB(cpu_inp_we)
    );

    always @(posedge stream_clk)
    if(stream_rst) begin
        cpu_inp_state <= CPU_INP_STATE_WAIT_SOF;
        cpu_inp_addr <= 0;
    end
    else begin
        case(cpu_inp_state)
        CPU_INP_STATE_WAIT_SOF: begin
            if (cpu_inp_ready & cpu_inp_valid & (cpu_inp_data[32] == 1'b1)) begin
                cpu_inp_state <= CPU_INP_STATE_WAIT_EOF;
                cpu_inp_addr <= cpu_inp_addr_next;
            end
        end

        CPU_INP_STATE_WAIT_EOF: begin
            if (cpu_inp_ready & cpu_inp_valid & (cpu_inp_data[33] == 1'b1)) begin
                cpu_inp_state <= CPU_INP_STATE_WAIT_CTRL_HI;
            end
            if (cpu_inp_ready & cpu_inp_valid) begin
                cpu_inp_addr <= cpu_inp_addr_next;
            end
        end

        CPU_INP_STATE_WAIT_CTRL_HI: begin
            if (cpu_inp_hs_ctrl == 1'b1) begin
                cpu_inp_state <= CPU_INP_STATE_WAIT_CTRL_LO;
            end
        end

        CPU_INP_STATE_WAIT_CTRL_LO: begin
            if (cpu_inp_hs_ctrl == 1'b0) begin
                cpu_inp_state <= CPU_INP_STATE_WAIT_SOF;
            end
            cpu_inp_addr <= 0; //reset the address counter
        end

        endcase //cpu_inp_state
    end

    ////////////////////////////////////////////////////////////////////
    // Interface CPU output interface to memory mapped wishbone
    ////////////////////////////////////////////////////////////////////
    localparam CPU_OUT_STATE_WAIT_CTRL_HI = 0;
    localparam CPU_OUT_STATE_WAIT_CTRL_LO = 1;
    localparam CPU_OUT_STATE_UNLOAD = 2;

    reg [1:0] cpu_out_state;
    reg [BUF_SIZE-1:0] cpu_out_addr;
    wire [BUF_SIZE-1:0] cpu_out_addr_next = cpu_out_addr + 1'b1;

    reg [BUF_SIZE-1:0] cpu_out_line_count_reg;

    reg cpu_out_flag_sof;
    reg cpu_out_flag_eof;
    assign cpu_out_data[35:32] = {2'b0, cpu_out_flag_eof, cpu_out_flag_sof};

    assign cpu_out_valid = (cpu_out_state == CPU_OUT_STATE_UNLOAD)? 1'b1 : 1'b0;
    assign cpu_out_hs_stat = (cpu_out_state == CPU_OUT_STATE_WAIT_CTRL_HI)? 1'b1 : 1'b0;

    RAMB16_S36_S36 cpu_out_buff(
        //port A = wishbone memory mapped address space (input only)
        .DOA(),.ADDRA(wb_adr_i[BUF_SIZE+1:2]),.CLKA(wb_clk_i),.DIA(wb_dat_i),.DIPA(4'h0),
        .ENA(wb_stb_i & (which_buf == 1'b1)),.SSRA(0),.WEA(wb_we_i),
        //port B = packet router interface from CPU (output only)
        .DOB(cpu_out_data[31:0]),.ADDRB(cpu_out_addr),.CLKB(stream_clk),.DIB(36'b0),.DIPB(4'h0),
        .ENB(1'b1),.SSRB(0),.WEB(1'b0)
    );

    always @(posedge stream_clk)
    if(stream_rst) begin
        cpu_out_state <= CPU_OUT_STATE_WAIT_CTRL_HI;
        cpu_out_addr <= 0;
    end
    else begin
        case(cpu_out_state)
        CPU_OUT_STATE_WAIT_CTRL_HI: begin
            if (cpu_out_hs_ctrl == 1'b1) begin
                cpu_out_state <= CPU_OUT_STATE_WAIT_CTRL_LO;
            end
            cpu_out_line_count_reg <= cpu_out_line_count;
            cpu_out_addr <= 0; //reset the address counter
        end

        CPU_OUT_STATE_WAIT_CTRL_LO: begin
            if (cpu_out_hs_ctrl == 1'b0) begin
                cpu_out_state <= CPU_OUT_STATE_UNLOAD;
                cpu_out_addr <= cpu_out_addr_next;
            end
            cpu_out_flag_sof <= 1'b1;
            cpu_out_flag_eof <= 1'b0;
        end

        CPU_OUT_STATE_UNLOAD: begin
            if (cpu_out_ready & cpu_out_valid) begin
                cpu_out_addr <= cpu_out_addr_next;
                cpu_out_flag_sof <= 1'b0;
                if (cpu_out_addr == cpu_out_line_count_reg) begin
                    cpu_out_flag_eof <= 1'b1;
                end
                else begin
                    cpu_out_flag_eof <= 1'b0;
                end
                if (cpu_out_flag_eof) begin
                    cpu_out_state <= CPU_OUT_STATE_WAIT_CTRL_HI;
                end
            end
        end

        endcase //cpu_out_state
    end

    ////////////////////////////////////////////////////////////////////
    // Ethernet input inspector
    //   - inspect Ethernet and send it to CPU or DSP
    ////////////////////////////////////////////////////////////////////
    localparam ETH_INSP_READ_ETH_PRE = 0;
    localparam ETH_INSP_READ_ETH = 1;
    localparam ETH_INSP_WRITE_DSP_REGS = 2;
    localparam ETH_INSP_WRITE_DSP_LIVE = 3;
    localparam ETH_INSP_WRITE_CPU_REGS = 4;
    localparam ETH_INSP_WRITE_CPU_LIVE = 5;

    localparam ETH_INSP_MAX_NUM_DREGS = 12; //padded_eth + ip + udp + vrt_hdr lines
    localparam ETH_INSP_DREGS_DSP_OFFSET = 10; //offset to start dsp at

    reg [2:0] eth_insp_state;
    reg [3:0] eth_insp_dreg_count; //data registers to buffer headers
    wire [3:0] eth_insp_dreg_count_next = eth_insp_dreg_count + 1'b1;
    reg [35:0] eth_insp_dregs [ETH_INSP_MAX_NUM_DREGS-1:0];

    wire eth_inp_dregs_is_data = 1'b0; //TODO (not data for now)

    /////////////////////////////////////
    //assign output signals to CPU input
    /////////////////////////////////////
    assign cpu_inp_data = (eth_insp_state == ETH_INSP_WRITE_CPU_REGS)?
        eth_insp_dregs[eth_insp_dreg_count] : eth_inp_data
    ;
    assign cpu_inp_valid =
        (eth_insp_state == ETH_INSP_WRITE_CPU_REGS)? 1'b1          : (
        (eth_insp_state == ETH_INSP_WRITE_CPU_LIVE)? eth_inp_valid : (
    1'b0));

    /////////////////////////////////////
    //assign output signals to DSP output
    /////////////////////////////////////
    wire [3:0] eth_insp_dsp_flags = (eth_insp_dreg_count == ETH_INSP_DREGS_DSP_OFFSET)?
        4'b0001 : 4'b0000
    ;
    assign dsp_out_data = (eth_insp_state == ETH_INSP_WRITE_DSP_REGS)?
        {eth_insp_dsp_flags, eth_insp_dregs[eth_insp_dreg_count][31:0]} : eth_inp_data
    ;
    assign dsp_out_valid =
        (eth_insp_state == ETH_INSP_WRITE_DSP_REGS)? 1'b1          : (
        (eth_insp_state == ETH_INSP_WRITE_DSP_LIVE)? eth_inp_valid : (
    1'b0));

    /////////////////////////////////////
    //assign output signal to ETH input
    /////////////////////////////////////
    assign eth_inp_ready =
        (eth_insp_state == ETH_INSP_READ_ETH_PRE)  ? 1'b1          : (
        (eth_insp_state == ETH_INSP_READ_ETH)      ? 1'b1          : (
        (eth_insp_state == ETH_INSP_WRITE_DSP_LIVE)? dsp_out_ready : (
        (eth_insp_state == ETH_INSP_WRITE_CPU_LIVE)? cpu_inp_ready : (
    1'b0))));

    always @(posedge stream_clk)
    if(stream_rst) begin
        eth_insp_state <= ETH_INSP_READ_ETH_PRE;
        eth_insp_dreg_count <= 0;
    end
    else begin
        case(eth_insp_state)
        ETH_INSP_READ_ETH_PRE: begin
            if (eth_inp_ready & eth_inp_valid & eth_inp_data[32]) begin
                eth_insp_state <= ETH_INSP_READ_ETH;
                eth_insp_dreg_count <= eth_insp_dreg_count_next;
                eth_insp_dregs[eth_insp_dreg_count] <= eth_inp_data;
            end
        end

        ETH_INSP_READ_ETH: begin
            if (eth_inp_ready & eth_inp_valid) begin
                eth_insp_dregs[eth_insp_dreg_count] <= eth_inp_data;
                if (eth_inp_dregs_is_data & (eth_insp_dreg_count_next == ETH_INSP_MAX_NUM_DREGS)) begin
                    eth_insp_state <= ETH_INSP_WRITE_DSP_REGS;
                    eth_insp_dreg_count <= ETH_INSP_DREGS_DSP_OFFSET;
                end
                else if (eth_inp_data[33] | (eth_insp_dreg_count_next == ETH_INSP_MAX_NUM_DREGS)) begin
                    eth_insp_state <= ETH_INSP_WRITE_CPU_REGS;
                    eth_insp_dreg_count <= 0;
                end
                else begin
                    eth_insp_dreg_count <= eth_insp_dreg_count_next;
                end
            end
        end

        ETH_INSP_WRITE_DSP_REGS: begin
            if (dsp_out_ready & dsp_out_valid) begin
                eth_insp_dreg_count <= eth_insp_dreg_count_next;
                if (eth_insp_dreg_count_next == ETH_INSP_MAX_NUM_DREGS) begin
                    eth_insp_state <= ETH_INSP_WRITE_DSP_LIVE;
                    eth_insp_dreg_count <= 0;
                end
            end

        end

        ETH_INSP_WRITE_DSP_LIVE: begin
            if (dsp_out_ready & dsp_out_valid & eth_inp_data[33]) begin
                eth_insp_state <= ETH_INSP_READ_ETH_PRE;
            end
        end

        ETH_INSP_WRITE_CPU_REGS: begin
            if (cpu_inp_ready & cpu_inp_valid) begin
                eth_insp_dreg_count <= eth_insp_dreg_count_next;
                if (cpu_inp_data[33]) begin
                    eth_insp_state <= ETH_INSP_READ_ETH_PRE;
                    eth_insp_dreg_count <= 0;
                end
                else if (eth_insp_dreg_count_next == ETH_INSP_MAX_NUM_DREGS) begin
                    eth_insp_state <= ETH_INSP_WRITE_CPU_LIVE;
                    eth_insp_dreg_count <= 0;
                end
            end
        end

        ETH_INSP_WRITE_CPU_LIVE: begin
            if (cpu_inp_ready & cpu_inp_valid & eth_inp_data[33]) begin
                eth_insp_state <= ETH_INSP_READ_ETH_PRE;
            end
        end

        endcase //eth_insp_state
    end

endmodule // packet_router

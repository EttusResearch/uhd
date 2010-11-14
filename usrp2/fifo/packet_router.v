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
        output [35:0] eth_out_data, output eth_out_valid, input eth_out_ready
    );

    //which buffer: 0 = CPU read buffer, 1 = CPU write buffer
    wire which_buf = wb_adr_i[BUF_SIZE+2];

    ////////////////////////////////////////////////////////////////////
    // status and controls
    ////////////////////////////////////////////////////////////////////
    wire eth_to_cpu_flag_ack = control[0];

    wire eth_to_cpu_flag_rdy;
    assign status[0] = eth_to_cpu_flag_rdy;

    ////////////////////////////////////////////////////////////////////
    // Ethernet input control
    ////////////////////////////////////////////////////////////////////

    localparam ETH_TO_CPU_STATE_WAIT_SOF = 0;
    localparam ETH_TO_CPU_STATE_WAIT_EOF = 1;
    localparam ETH_TO_CPU_STATE_WAIT_ACK_HI = 2;
    localparam ETH_TO_CPU_STATE_WAIT_ACK_LO = 3;

    reg [1:0] eth_to_cpu_state;
    reg [BUF_SIZE-1:0] eth_to_cpu_addr;
    wire [BUF_SIZE-1:0] eth_to_cpu_addr_next = eth_to_cpu_addr + 1'b1;
    
    wire eth_to_cpu_reading_input = (
        eth_to_cpu_state == ETH_TO_CPU_STATE_WAIT_SOF ||
        eth_to_cpu_state == ETH_TO_CPU_STATE_WAIT_EOF
    )? 1'b1 : 1'b0;

    wire eth_to_cpu_we = eth_to_cpu_reading_input;
    assign eth_inp_ready = eth_to_cpu_reading_input;
    assign eth_to_cpu_flag_rdy = (eth_to_cpu_state == ETH_TO_CPU_STATE_WAIT_ACK_HI)? 1'b1 : 1'b0;

    assign wb_err_o = 1'b0;  // Unused for now
    assign wb_rty_o = 1'b0;  // Unused for now
    always @(posedge wb_clk_i)
        wb_ack_o <= wb_stb_i & ~wb_ack_o;

    RAMB16_S36_S36 eth_to_cpu_buff(
        //port A = wishbone memory mapped address space
        .DOA(wb_dat_o),.ADDRA(wb_adr_i[BUF_SIZE+1:2]),.CLKA(wb_clk_i),.DIA(wb_dat_i),.DIPA(4'h0),
        .ENA(wb_stb_i & (which_buf == 1'b0)),.SSRA(0),.WEA(wb_we_i),
        //port B = input from ethernet packets
        .DOB(),.ADDRB(eth_to_cpu_addr),.CLKB(stream_clk),.DIB(eth_inp_data),.DIPB(4'h0),
        .ENB(eth_to_cpu_we),.SSRB(0),.WEB(eth_to_cpu_we)
    );

    always @(posedge stream_clk)
    if(stream_rst) begin
        eth_to_cpu_state <= ETH_TO_CPU_STATE_WAIT_SOF;
        eth_to_cpu_addr <= 0;
    end
    else begin
        case(eth_to_cpu_state)
        ETH_TO_CPU_STATE_WAIT_SOF: begin
            if (eth_inp_ready & eth_inp_valid & (eth_inp_data[32] == 1'b1)) begin
                eth_to_cpu_state <= ETH_TO_CPU_STATE_WAIT_EOF;
                eth_to_cpu_addr <= eth_to_cpu_addr_next;
            end
        end

        ETH_TO_CPU_STATE_WAIT_EOF: begin
            if (eth_inp_ready & eth_inp_valid & (eth_inp_data[33] == 1'b1)) begin
                eth_to_cpu_state <= ETH_TO_CPU_STATE_WAIT_ACK_HI;
            end
            if (eth_inp_ready & eth_inp_valid) begin
                eth_to_cpu_addr <= eth_to_cpu_addr_next;
            end
        end

        ETH_TO_CPU_STATE_WAIT_ACK_HI: begin
            if (eth_to_cpu_flag_ack == 1'b1) begin
                eth_to_cpu_state <= ETH_TO_CPU_STATE_WAIT_ACK_LO;
            end
        end

        ETH_TO_CPU_STATE_WAIT_ACK_LO: begin
            if (eth_to_cpu_flag_ack == 0'b1) begin
                eth_to_cpu_state <= ETH_TO_CPU_STATE_WAIT_SOF;
            end
        end

        endcase //eth_to_cpu_state
    end


endmodule // packet_router

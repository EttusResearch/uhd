//
// Copyright 2011-2012 Ettus Research LLC
//
// Packet dispatcher with fifo36 interface and 4 outputs.
//
// The packet dispatcher expects 2-byte padded ethernet frames.
// The frames will be inspected at ethernet, IPv4, UDP, and VRT layers.
// Packets are dispatched into the following streams:
//   * tx dsp stream
//   * tx control stream
//   * to cpu stream
//   * to external stream
//   * to both cpu and external
//
// The following registers are used for dispatcher control:
//   * base + 0 = this ipv4 address (32 bits)
//   * base + 1 = udp control port (upper 16 bits), udp dsp port (lower 16 bits)
//

module packet_dispatcher36_x4
    #(
        parameter BASE = 0
    )
    (
        //clocking and reset interface:
        input clk, input rst, input clr,

        //setting register interface:
        input set_stb, input [7:0] set_addr, input [31:0] set_data,

        //input stream interfaces:
        input [35:0] com_inp_data, input com_inp_valid, output com_inp_ready,

        //output stream interfaces:
        output [35:0] ext_out_data, output ext_out_valid, input ext_out_ready,
        output [35:0] dsp_out_data, output dsp_out_valid, input dsp_out_ready,
        output [35:0] ctl_out_data, output ctl_out_valid, input ctl_out_ready,
        output [35:0] cpu_out_data, output cpu_out_valid, input cpu_out_ready
    );

    //setting register to program the IP address
    wire [31:0] my_ip_addr;
    setting_reg #(.my_addr(BASE+0)) sreg_ip_addr(
        .clk(clk),.rst(rst),
        .strobe(set_stb),.addr(set_addr),.in(set_data),
        .out(my_ip_addr),.changed()
    );

    //setting register to program the UDP DSP port
    wire [15:0] dsp_udp_port, ctl_udp_port;
    setting_reg #(.my_addr(BASE+1), .width(32)) sreg_data_port(
        .clk(clk),.rst(rst),
        .strobe(set_stb),.addr(set_addr),.in(set_data),
        .out({ctl_udp_port, dsp_udp_port}),.changed()
    );

    ////////////////////////////////////////////////////////////////////
    // Communication input inspector
    //   - inspect com input and send it to DSP, EXT, CPU, or BOTH
    ////////////////////////////////////////////////////////////////////
    localparam PD_STATE_READ_COM_PRE = 0;
    localparam PD_STATE_READ_COM = 1;
    localparam PD_STATE_WRITE_REGS = 2;
    localparam PD_STATE_WRITE_LIVE = 3;

    localparam PD_DEST_DSP = 0;
    localparam PD_DEST_EXT = 1;
    localparam PD_DEST_CPU = 2;
    localparam PD_DEST_BOF = 3;
    localparam PD_DEST_CTL = 4;

    localparam PD_MAX_NUM_DREGS = 13; //padded_eth + ip + udp + seq + vrt_hdr
    localparam PD_DREGS_DSP_OFFSET = 11; //offset to start dsp at

    //output inspector interfaces
    wire [35:0] pd_out_dsp_data;
    wire        pd_out_dsp_valid;
    wire        pd_out_dsp_ready;

    wire [35:0] pd_out_ext_data;
    wire        pd_out_ext_valid;
    wire        pd_out_ext_ready;

    wire [35:0] pd_out_cpu_data;
    wire        pd_out_cpu_valid;
    wire        pd_out_cpu_ready;

    wire [35:0] pd_out_bof_data;
    wire        pd_out_bof_valid;
    wire        pd_out_bof_ready;

    wire [35:0] pd_out_ctl_data;
    wire        pd_out_ctl_valid;
    wire        pd_out_ctl_ready;

    reg [1:0] pd_state;
    reg [2:0] pd_dest;
    reg [3:0] pd_dreg_count; //data registers to buffer headers
    wire [3:0] pd_dreg_count_next = pd_dreg_count + 1'b1;
    wire pd_dreg_counter_done = (pd_dreg_count_next == PD_MAX_NUM_DREGS)? 1'b1 : 1'b0;
    reg [35:0] pd_dregs [PD_MAX_NUM_DREGS-1:0];

    reg is_eth_dst_mac_bcast;
    reg is_eth_type_ipv4;
    reg is_eth_ipv4_proto_udp;
    reg is_eth_ipv4_dst_addr_here;
    reg is_eth_udp_dsp_port_here;
    reg is_eth_udp_ctl_port_here;
    wire is_vrt_size_zero = (com_inp_data[15:0] == 16'h0); //needed on the same cycle, so it cant be registered

    //Inspector output flags special case:
    //Inject SOF into flags at first DSP line.
    wire [3:0] pd_out_flags = (
        (pd_dreg_count == PD_DREGS_DSP_OFFSET) &&
        (pd_dest == PD_DEST_DSP)
    )? 4'b0001 : pd_dregs[pd_dreg_count][35:32];

    //The communication inspector ouput data and valid signals:
    //Mux between com input and data registers based on the state.
    wire [35:0] pd_out_data = (pd_state == PD_STATE_WRITE_REGS)?
        {pd_out_flags, pd_dregs[pd_dreg_count][31:0]} : com_inp_data
    ;
    wire pd_out_valid =
        (pd_state == PD_STATE_WRITE_REGS)? 1'b1          : (
        (pd_state == PD_STATE_WRITE_LIVE)? com_inp_valid : (
    1'b0));

    //The communication inspector ouput ready signal:
    //Mux between the various destination ready signals.
    wire pd_out_ready =
        (pd_dest == PD_DEST_DSP)? pd_out_dsp_ready : (
        (pd_dest == PD_DEST_EXT)? pd_out_ext_ready : (
        (pd_dest == PD_DEST_CPU)? pd_out_cpu_ready : (
        (pd_dest == PD_DEST_BOF)? pd_out_bof_ready : (
        (pd_dest == PD_DEST_CTL)? pd_out_ctl_ready : (
    1'b0)))));

    //Always connected output data lines.
    assign pd_out_dsp_data = pd_out_data;
    assign pd_out_ext_data = pd_out_data;
    assign pd_out_cpu_data = pd_out_data;
    assign pd_out_bof_data = pd_out_data;
    assign pd_out_ctl_data = pd_out_data;

    //Destination output valid signals:
    //Comes from inspector valid when destination is selected, and otherwise low.
    assign pd_out_dsp_valid = (pd_dest == PD_DEST_DSP)? pd_out_valid : 1'b0;
    assign pd_out_ext_valid = (pd_dest == PD_DEST_EXT)? pd_out_valid : 1'b0;
    assign pd_out_cpu_valid = (pd_dest == PD_DEST_CPU)? pd_out_valid : 1'b0;
    assign pd_out_bof_valid = (pd_dest == PD_DEST_BOF)? pd_out_valid : 1'b0;
    assign pd_out_ctl_valid = (pd_dest == PD_DEST_CTL)? pd_out_valid : 1'b0;

    //The communication inspector ouput ready signal:
    //Always ready when storing to data registers,
    //comes from inspector ready output when live,
    //and otherwise low.
    assign com_inp_ready =
        (pd_state == PD_STATE_READ_COM_PRE)  ? 1'b1         : (
        (pd_state == PD_STATE_READ_COM)      ? 1'b1         : (
        (pd_state == PD_STATE_WRITE_LIVE)    ? pd_out_ready : (
    1'b0)));

    //inspect the incoming data and mark register booleans
    always @(posedge clk)
    if (com_inp_ready & com_inp_valid) begin
        case(pd_dreg_count)
        0: begin
            is_eth_dst_mac_bcast <= (com_inp_data[15:0] == 16'hffff);
        end
        1: begin
            is_eth_dst_mac_bcast <= is_eth_dst_mac_bcast && (com_inp_data[31:0] == 32'hffffffff);
        end
        3: begin
            is_eth_type_ipv4 <= (com_inp_data[15:0] == 16'h800);
        end
        6: begin
            is_eth_ipv4_proto_udp <= (com_inp_data[23:16] == 8'h11);
        end
        8: begin
            is_eth_ipv4_dst_addr_here <= (com_inp_data[31:0] == my_ip_addr);
        end
        9: begin
            is_eth_udp_dsp_port_here <= (com_inp_data[15:0] == dsp_udp_port);
            is_eth_udp_ctl_port_here <= (com_inp_data[15:0] == ctl_udp_port);
        end
        endcase //pd_dreg_count
    end

    always @(posedge clk)
    if(rst | clr) begin
        pd_state <= PD_STATE_READ_COM_PRE;
        pd_dreg_count <= 0;
    end
    else begin
        case(pd_state)
        PD_STATE_READ_COM_PRE: begin
            if (com_inp_ready & com_inp_valid & com_inp_data[32]) begin
                pd_state <= PD_STATE_READ_COM;
                pd_dreg_count <= pd_dreg_count_next;
                pd_dregs[pd_dreg_count] <= com_inp_data;
            end
        end

        PD_STATE_READ_COM: begin
            if (com_inp_ready & com_inp_valid) begin
                pd_dregs[pd_dreg_count] <= com_inp_data;
                if (pd_dreg_counter_done | com_inp_data[33]) begin
                    pd_state <= PD_STATE_WRITE_REGS;
                    pd_dreg_count <= 0;

                    //---------- begin inspection decision -----------//
                    //EOF or bcast or not IPv4 or not UDP:
                    if (
                        com_inp_data[33] || is_eth_dst_mac_bcast ||
                        ~is_eth_type_ipv4 || ~is_eth_ipv4_proto_udp
                    ) begin
                        pd_dest <= PD_DEST_BOF;
                    end

                    //not my IP address:
                    else if (~is_eth_ipv4_dst_addr_here) begin
                        pd_dest <= PD_DEST_EXT;
                    end

                    //UDP control port and VRT:
                    else if (is_eth_udp_ctl_port_here && ~is_vrt_size_zero) begin
                        pd_dest <= PD_DEST_CTL;
                        pd_dreg_count <= PD_DREGS_DSP_OFFSET;
                    end

                    //UDP data port and VRT:
                    else if (is_eth_udp_dsp_port_here && ~is_vrt_size_zero) begin
                        pd_dest <= PD_DEST_DSP;
                        pd_dreg_count <= PD_DREGS_DSP_OFFSET;
                    end

                    //other:
                    else begin
                        pd_dest <= PD_DEST_CPU;
                    end
                    //---------- end inspection decision -------------//

                end
                else begin
                    pd_dreg_count <= pd_dreg_count_next;
                end
            end
        end

        PD_STATE_WRITE_REGS: begin
            if (pd_out_ready & pd_out_valid) begin
                if (pd_out_data[33]) begin
                    pd_state <= PD_STATE_READ_COM_PRE;
                    pd_dreg_count <= 0;
                end
                else if (pd_dreg_counter_done) begin
                    pd_state <= PD_STATE_WRITE_LIVE;
                    pd_dreg_count <= 0;
                end
                else begin
                    pd_dreg_count <= pd_dreg_count_next;
                end
            end
        end

        PD_STATE_WRITE_LIVE: begin
            if (pd_out_ready & pd_out_valid & pd_out_data[33]) begin
                pd_state <= PD_STATE_READ_COM_PRE;
            end
        end

        endcase //pd_state
    end

    //connect this fast-path signals directly to the DSP out
    assign dsp_out_data = pd_out_dsp_data;
    assign dsp_out_valid = pd_out_dsp_valid;
    assign pd_out_dsp_ready = dsp_out_ready;

    assign ctl_out_data = pd_out_ctl_data;
    assign ctl_out_valid = pd_out_ctl_valid;
    assign pd_out_ctl_ready = ctl_out_ready;

    ////////////////////////////////////////////////////////////////////
    // Splitter and output muxes for the bof packets
    //   - split the bof packets into two streams
    //   - mux split packets into cpu out and ext out
    ////////////////////////////////////////////////////////////////////

    //dummy signals to join the the splitter and muxes below
    wire [35:0] _split_to_ext_data,  _split_to_cpu_data;
    wire        _split_to_ext_valid, _split_to_cpu_valid;
    wire        _split_to_ext_ready, _split_to_cpu_ready;

    splitter36 bof_out_splitter(
        .clk(clk), .rst(rst), .clr(clr),
        .inp_data(pd_out_bof_data), .inp_valid(pd_out_bof_valid), .inp_ready(pd_out_bof_ready),
        .out0_data(_split_to_ext_data), .out0_valid(_split_to_ext_valid), .out0_ready(_split_to_ext_ready),
        .out1_data(_split_to_cpu_data), .out1_valid(_split_to_cpu_valid), .out1_ready(_split_to_cpu_ready)
    );

    fifo36_mux ext_out_mux(
        .clk(clk), .reset(rst), .clear(clr),
        .data0_i(pd_out_ext_data), .src0_rdy_i(pd_out_ext_valid), .dst0_rdy_o(pd_out_ext_ready),
        .data1_i(_split_to_ext_data), .src1_rdy_i(_split_to_ext_valid), .dst1_rdy_o(_split_to_ext_ready),
        .data_o(ext_out_data), .src_rdy_o(ext_out_valid), .dst_rdy_i(ext_out_ready)
    );

    fifo36_mux cpu_out_mux(
        .clk(clk), .reset(rst), .clear(clr),
        .data0_i(pd_out_cpu_data), .src0_rdy_i(pd_out_cpu_valid), .dst0_rdy_o(pd_out_cpu_ready),
        .data1_i(_split_to_cpu_data), .src1_rdy_i(_split_to_cpu_valid), .dst1_rdy_o(_split_to_cpu_ready),
        .data_o(cpu_out_data), .src_rdy_o(cpu_out_valid), .dst_rdy_i(cpu_out_ready)
    );

endmodule // packet_dispatcher36_x3

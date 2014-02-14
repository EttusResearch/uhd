//
// Copyright 2012 Ettus Research LLC
//


// AXI stream to/from wishbone
// Input is an axi stream which wites into a BRAM.
// Output is an axi stream which reads from a BRAM.
// This RAM can also be accessed from a wishbone interface.

// From the wishbone interface we need to be able to:

// Ask the module if a completed packet is available.
// Read number of bytes/lines in the BRAM.
// Release the completed packet.

// Ask the module if an outgoing slot is available.
// Write number of bytes/lines in the BRAM.
// Release the completed packet.

module axi_stream_to_wb
#(
    parameter AWIDTH = 13, //WB addr width and buffering size in bytes
    parameter UWIDTH = 4, //stream user width
    parameter CTRL_ADDR = 0 //ctrl/status register
)
(
    //-- the wishbone interface
    input clk_i, input rst_i,
    input we_i, input stb_i, input cyc_i, output reg ack_o,
    input [AWIDTH-1:0] adr_i, input [31:0] dat_i, output reg [31:0] dat_o,

    //-- the axi stream interface input
    input [63:0] rx_tdata,
    input [3:0] rx_tuser,
    input rx_tlast,
    input rx_tvalid,
    output rx_tready,

    //-- the axi stream interface output
    output [63:0] tx_tdata,
    output [3:0] tx_tuser,
    output tx_tlast,
    output tx_tvalid,
    input tx_tready,

    output [31:0] debug_rx,
    output [31:0] debug_tx
);

    reg stb_i_del;
    always @(posedge clk_i) begin
        if (rst_i) stb_i_del <= 0;
        else stb_i_del <= stb_i;
    end

    reg ack_o_del;
    always @(posedge clk_i) begin
        if (rst_i)  ack_o_del <= 0;
        else        ack_o_del <= ack_o;
    end

    //drive the ack signal
    always @(posedge clk_i) begin
        if (rst_i)      ack_o <= 0;
        else if (we_i)  ack_o <= stb_i & ~ack_o;
        else            ack_o <= stb_i & stb_i_del & ~ack_o & ~ack_o_del;
    end

    //control registers, status
    reg [AWIDTH-1:0] tx_bytes, rx_bytes;
    reg tx_error, rx_error;
    wire rx_state_flag, tx_state_flag;
    reg rx_proc_flag, tx_proc_flag;

    //assign status
    wire [31:0] status;
    assign status[31] = rx_state_flag;
    assign status[30] = tx_state_flag;
    assign status[29] = rx_error;
    assign status[AWIDTH-1:0] = rx_bytes;

   // Create some piplining to break timing paths.
   reg 		ctrl_addressed;
   always @(posedge clk_i)
     if (rst_i)
       ctrl_addressed <= 1'b0;
     else if(adr_i == CTRL_ADDR)
       ctrl_addressed <= 1'b1;
     else
       ctrl_addressed <= 1'b0;
       
    //assign control
    always @(posedge clk_i) begin
        if (rst_i) begin
            rx_proc_flag <= 0;
            tx_proc_flag <= 0;
            tx_error <= 0;
            tx_bytes <= 0;
        end
        else if (we_i && ack_o && ctrl_addressed) begin
            rx_proc_flag <= dat_i[31];
            tx_proc_flag <= dat_i[30];
            tx_error <= dat_i[29];
            tx_bytes <= dat_i[AWIDTH-1:0];
        end
    end

    //------------------------------------------------------------------
    //-- block ram interface between wb and input stream
    //------------------------------------------------------------------
    reg [AWIDTH-4:0] rx_counter;
    wire [63:0] rx_bram_data64;
    ram_2port #(.DWIDTH(64), .AWIDTH(AWIDTH-3)) input_stream_bram
    (
        .clka(clk_i), .ena(rx_tready), .wea(rx_tvalid),
        .addra(rx_counter), .dia(rx_tdata), .doa(),
        .clkb(clk_i), .enb(stb_i), .web(1'b0),
        .addrb(adr_i[AWIDTH-1:3]), .dib({64{1'b1}}), .dob(rx_bram_data64)
    );

    //select the data source, status, or upper/lower 32 from bram
    wire [31:0] dat_o_pipeline;
    assign dat_o_pipeline = ctrl_addressed ? status : ((!adr_i[2])? rx_bram_data64[63:32]: rx_bram_data64[31:0]);
    always @(posedge clk_i) begin
        dat_o <= dat_o_pipeline;
    end

    //------------------------------------------------------------------
    //-- block ram interface between wb and output stream
    //------------------------------------------------------------------
    reg [AWIDTH-4:0] tx_counter;
    wire enb_out;
    wire [63:0] tx_bram_data64;
    ram_2port #(.DWIDTH(64), .AWIDTH(AWIDTH-3)) output_stream_bram
    (
        .clka(clk_i), .ena(enb_out), .wea(1'b0),
        .addra(tx_counter), .dia({64{1'b1}}), .doa(tx_tdata),
        .clkb(clk_i), .enb(stb_i), .web(we_i && adr_i[2]),
        .addrb(adr_i[AWIDTH-1:3]), .dib(tx_bram_data64), .dob()
    );

    //write 64 bit chunks, so register the lower write
    reg [31:0] dat_i_reg;
    always @(posedge clk_i) begin
        if (we_i && stb_i && !adr_i[2]) dat_i_reg <= dat_i;
    end
    assign tx_bram_data64 = {dat_i_reg, dat_i};

    //------------------------------------------------------------------
    //-- state machine to drive input stream
    //------------------------------------------------------------------
    localparam RX_STATE_READY = 0; //waits for proc flag 0
    localparam RX_STATE_WRITE = 1; //writes stream to bram
    localparam RX_STATE_RELEASE = 2; //waits for proc to flag 1
    reg [1:0] rx_state;

    always @(posedge clk_i) begin
        if (rst_i) begin
            rx_state <= RX_STATE_READY;
            rx_counter <= 0;
            rx_error <= 0;
            rx_bytes <= 0;
        end
        else case (rx_state)

        RX_STATE_READY: begin
            if (!rx_proc_flag) rx_state <= RX_STATE_WRITE;
            rx_counter <= 0;
        end

        RX_STATE_WRITE: begin
            if (rx_tready && rx_tvalid) begin
                rx_counter <= rx_counter + 1'b1;
                if (rx_tlast) begin
                    rx_state <= RX_STATE_RELEASE;
                    rx_bytes <= {rx_counter + 1'b1, rx_tuser[2:0]};
                    rx_error <= rx_tuser[3];
                end
            end
        end

        RX_STATE_RELEASE: begin
            if (rx_proc_flag) rx_state <= RX_STATE_READY;
            rx_counter <= 0;
        end

        default: rx_state <= RX_STATE_READY;
        endcase //rx_state
    end

    //flag tells the processor when it can grab some input buffer
    assign rx_state_flag = (rx_state == RX_STATE_RELEASE);

    //always ready to accept input data in the write state
    assign rx_tready = (rx_state == RX_STATE_WRITE);

    //------------------------------------------------------------------
    //-- state machine to drive output stream
    //------------------------------------------------------------------
    localparam TX_STATE_READY = 0;  //waits for proc flag 0
    localparam TX_STATE_WRITE = 1; //writes bram to stream
    localparam TX_STATE_RELEASE = 2; //waits for proc to flag 1
    reg [1:0] tx_state;

    always @(posedge clk_i) begin
        if (rst_i) begin
            tx_state <= TX_STATE_READY;
            tx_counter <= 0;
        end
        else case (tx_state)

        TX_STATE_READY: begin
            if (tx_proc_flag) begin
                tx_state <= TX_STATE_WRITE;
                tx_counter <= 1;
            end
            else tx_counter <= 0;
        end

        TX_STATE_WRITE: begin
            if (tx_tready && tx_tvalid) begin
                tx_counter <= tx_counter + 1'b1;
                if (tx_tlast) begin
                    tx_state <= TX_STATE_RELEASE;
                end
            end
        end

        TX_STATE_RELEASE: begin
            if (!tx_proc_flag) tx_state <= TX_STATE_READY;
            tx_counter <= 0;
        end

        default: tx_state <= TX_STATE_READY;
        endcase //tx_state
    end

    //flag tells the processor when it can grab available out buffer
    assign tx_state_flag = (tx_state == TX_STATE_READY);

    //the output user bus assignment (non-zero only at end)
    assign tx_tuser = (tx_tlast)? {tx_error, tx_bytes[2:0]} : 4'b0;

    //end of frame signal
    assign tx_tlast = (tx_counter == tx_bytes[AWIDTH-1:3]);

    //output is always valid in state write
    assign tx_tvalid = (tx_state == TX_STATE_WRITE);

    //enable the read so we can pre-read due to read 1 cycle delay
    assign enb_out = (tx_state == TX_STATE_WRITE)? (tx_tvalid && tx_tready) : 1'b1;

    assign debug_rx = {
        rx_state, rx_tlast, rx_tvalid, rx_tready, rx_tuser[2:0], //8
        rx_proc_flag, rx_state_flag, rx_tdata[21:0] //24
    };
    assign debug_tx = {
        tx_state, tx_tlast, tx_tvalid, tx_tready, tx_tuser[2:0], //8
        tx_proc_flag, tx_state_flag, tx_tdata[21:0] //24
    };

endmodule //axi_stream_to_wb

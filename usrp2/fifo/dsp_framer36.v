
// Frame DSP packets with a header line to be handled by the protocol machine

module dsp_framer36
    #(parameter BUF_SIZE = 9)
    (
        input clk, input rst, input clr,
        input [35:0] inp_data, input inp_valid, output inp_ready,
        output [35:0] out_data, output out_valid, input out_ready
    );

    localparam DSP_FRM_STATE_WAIT_SOF = 0;
    localparam DSP_FRM_STATE_WAIT_EOF = 1;
    localparam DSP_FRM_STATE_WRITE_HDR = 2;
    localparam DSP_FRM_STATE_WRITE = 3;

    reg [1:0] dsp_frm_state;
    reg [BUF_SIZE-1:0] dsp_frm_addr;
    reg [BUF_SIZE-1:0] dsp_frm_count;
    wire [BUF_SIZE-1:0] dsp_frm_addr_next = dsp_frm_addr + 1'b1;

    //DSP input stream ready in the following states
    assign inp_ready =
        (dsp_frm_state == DSP_FRM_STATE_WAIT_SOF)? 1'b1 : (
        (dsp_frm_state == DSP_FRM_STATE_WAIT_EOF)? 1'b1 : (
    1'b0));

    //DSP framer output data mux (header or BRAM):
    //The header is generated here from the count.
    wire [31:0] dsp_frm_data_bram;
    wire [15:0] dsp_frm_bytes = {dsp_frm_count, 2'b00};
    wire dsp_frm_enb = (dsp_frm_state == DSP_FRM_STATE_WRITE)? (out_ready & out_valid) : 1'b1;
    assign out_data =
        (dsp_frm_state == DSP_FRM_STATE_WRITE_HDR)? {4'b0001, 16'b1, dsp_frm_bytes} : (
        (dsp_frm_addr == dsp_frm_count)           ? {4'b0010, dsp_frm_data_bram}    : (
    {4'b0000, dsp_frm_data_bram}));
    assign out_valid = (
        (dsp_frm_state == DSP_FRM_STATE_WRITE_HDR) ||
        (dsp_frm_state == DSP_FRM_STATE_WRITE)
    )? 1'b1 : 1'b0;

    RAMB16_S36_S36 dsp_frm_buff(
        //port A = DSP input interface (writes to BRAM)
        .DOA(),.ADDRA(dsp_frm_addr),.CLKA(clk),.DIA(inp_data[31:0]),.DIPA(4'h0),
        .ENA(inp_ready),.SSRA(0),.WEA(inp_ready),
        //port B = DSP framer interface (reads from BRAM)
        .DOB(dsp_frm_data_bram),.ADDRB(dsp_frm_addr),.CLKB(clk),.DIB(36'b0),.DIPB(4'h0),
        .ENB(dsp_frm_enb),.SSRB(0),.WEB(1'b0)
    );

    always @(posedge clk)
    if(rst | clr) begin
        dsp_frm_state <= DSP_FRM_STATE_WAIT_SOF;
        dsp_frm_addr <= 0;
    end
    else begin
        case(dsp_frm_state)
        DSP_FRM_STATE_WAIT_SOF: begin
            if (inp_ready & inp_valid & inp_data[32]) begin
                dsp_frm_addr <= dsp_frm_addr_next;
                dsp_frm_state <= DSP_FRM_STATE_WAIT_EOF;
            end
        end

        DSP_FRM_STATE_WAIT_EOF: begin
            if (inp_ready & inp_valid) begin
                if (inp_data[33]) begin
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
            if (out_ready & out_valid) begin
                dsp_frm_addr <= dsp_frm_addr_next;
                dsp_frm_state <= DSP_FRM_STATE_WRITE;
            end
        end

        DSP_FRM_STATE_WRITE: begin
            if (out_ready & out_valid) begin
                if (out_data[33]) begin
                    dsp_frm_addr <= 0;
                    dsp_frm_state <= DSP_FRM_STATE_WAIT_SOF;
                end
                else begin
                    dsp_frm_addr <= dsp_frm_addr_next;
                end
            end
        end
        endcase //dsp_frm_state
    end

endmodule //dsp_framer36

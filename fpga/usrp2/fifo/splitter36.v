
// Split packets from a fifo based interface so it goes out identically on two interfaces

module splitter36
    (
        input clk, input rst, input clr,
        input [35:0] inp_data, input inp_valid, output inp_ready,
        output [35:0] out0_data, output out0_valid, input out0_ready,
        output [35:0] out1_data, output out1_valid, input out1_ready
    );

    localparam STATE_COPY_BOTH = 0;
    localparam STATE_COPY_ZERO = 1;
    localparam STATE_COPY_ONE = 2;

    reg [1:0] state;
    reg [35:0] data_reg;

    assign out0_data = (state == STATE_COPY_BOTH)? inp_data : data_reg;
    assign out1_data = (state == STATE_COPY_BOTH)? inp_data : data_reg;

    assign out0_valid =
        (state == STATE_COPY_BOTH)? inp_valid : (
        (state == STATE_COPY_ZERO)? 1'b1      : (
    1'b0));

    assign out1_valid =
        (state == STATE_COPY_BOTH)? inp_valid : (
        (state == STATE_COPY_ONE)?  1'b1      : (
    1'b0));

    assign inp_ready = (state == STATE_COPY_BOTH)? (out0_ready | out1_ready) : 1'b0;

    always @(posedge clk)
    if (rst | clr) begin
        state <= STATE_COPY_BOTH;
    end
    else begin
        case (state)

        STATE_COPY_BOTH: begin
            if ((out0_valid & out0_ready) & ~(out1_valid & out1_ready)) begin
                state <= STATE_COPY_ONE;
            end
            else if (~(out0_valid & out0_ready) & (out1_valid & out1_ready)) begin
                state <= STATE_COPY_ZERO;
            end
            data_reg <= inp_data;
        end

        STATE_COPY_ZERO: begin
            if (out0_valid & out0_ready) begin
                state <= STATE_COPY_BOTH;
            end
        end

        STATE_COPY_ONE: begin
            if (out1_valid & out1_ready) begin
                state <= STATE_COPY_BOTH;
            end
        end

        endcase //state
    end



endmodule //splitter36

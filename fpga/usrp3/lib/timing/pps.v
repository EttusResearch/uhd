//
// Copyright 2014 Ettus Research LLC
//

module pps_generator
    #(parameter CLK_FREQ=0, DUTY=25)
    (input clk, input reset, output pps);

    reg[31:0] count;

    always @(posedge clk) begin
        if (reset) begin
            count <= 32'b1;
        end else if (count >= CLK_FREQ) begin
            count <= 32'b1;
        end else begin
            count <= count + 1'b1;
        end
    end

    assign pps = (count < CLK_FREQ * DUTY / 100);
endmodule //pps_generator

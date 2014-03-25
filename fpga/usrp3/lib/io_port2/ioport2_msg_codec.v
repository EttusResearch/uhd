//
// Copyright 2013 Ettus Research LLC
//


//Message format:
//  msg[63]:    Completion {1 -> Read Completion, 0 -> Transaction Request}
//  msg[62]:    Write request*
//  msg[61]:    Read request*
//  msg[60]:    Half word {1 -> 16-bit transaction, 0 -> 32-bit transaction}*
//  msg[59:52]: Reserved
//  msg[51:32]: Address*
//  msg[31:0]:  Data
//
//  * Field only valid when the word is a transaction request.

module ioport2_msg_decode(
    input  [63:0]   message,
    output          rd_response,
    output          wr_request,
    output          rd_request,
    output          half_word,
    output [19:0]   address,
    output [31:0]   data,
    output [31:0]   control    
);
    assign rd_response  = message[63];
    assign wr_request   = message[62];
    assign rd_request   = message[61];
    assign half_word    = message[60];
    assign address      = message[51:32];
    assign data         = message[31:0];
    assign control      = message[63:32];
endmodule


module ioport2_msg_encode(
    input          rd_response,
    input          wr_request,
    input          rd_request,
    input          half_word,
    input  [19:0]  address,
    input  [31:0]  data,
    output [31:0]  control,
    output [63:0]  message
);
    assign control = rd_response ? {rd_response, 31'h0} : {rd_response, wr_request, rd_request, half_word, 8'h00, address};
    assign message = {control, data};
endmodule

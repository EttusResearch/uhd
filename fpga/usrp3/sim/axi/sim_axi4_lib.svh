//
// Copyright 2015 Ettus Research LLC
//

interface axi4_addr_t #(parameter AWIDTH=32, parameter IDWIDTH=4)
                       (input clk);

  logic [IDWIDTH-1:0] id;
  logic [AWIDTH-1:0]  addr;
  logic [7:0]         len;
  logic [2:0]         size;
  logic [1:0]         burst;
  logic               lock;
  logic [3:0]         cache;
  logic [2:0]         prot;
  logic [3:0]         qos;
  logic [3:0]         region;
  logic               user;
  logic               valid;
  logic               ready;

//  modport master(output id,addr,len,size,burst,lock,cache,prot,qos,valid, input ready);
//  modport slave(input id,addr,len,size,burst,lock,cache,prot,qos,valid, output ready);

endinterface

interface axi4_wdata_t #(parameter DWIDTH=64)
                       (input clk);

  logic [DWIDTH-1:0]      data;
  logic [(DWIDTH/8)-1:0]  strb;
  logic                   last;
  logic                   user;
  logic                   valid;
  logic                   ready;

//  modport master(output data,strb,last,valid, input ready);
//  modport slave(input data,strb,last,valid, output ready);

endinterface

interface axi4_resp_t #(parameter IDWIDTH=4)
                       (input clk);

  logic               ready;
  logic [IDWIDTH-1:0] id;
  logic [1:0]         resp;
  logic               user;
  logic               valid;

//  modport master(output ready, input id,resp,valid);
//  modport slave(input ready, output id,resp,valid);

endinterface

interface axi4_rdata_t #(parameter DWIDTH=64, parameter IDWIDTH=4)
                       (input clk);

  logic               ready;
  logic [IDWIDTH-1:0] id;
  logic [DWIDTH-1:0]  data;
  logic [1:0]         resp;
  logic               user;
  logic               last;
  logic               valid;

//  modport master(output ready, input id,data,resp,last,valid);
//  modport slave(input ready, output id,data,resp,last,valid);

endinterface

interface axi4_wr_t #(parameter DWIDTH=64, parameter AWIDTH=32, parameter IDWIDTH=4)
                     (input clk);

  axi4_addr_t  #(.AWIDTH(AWIDTH), .IDWIDTH(IDWIDTH)) addr (.clk(clk));
  axi4_wdata_t #(.DWIDTH(DWIDTH))                    data (.clk(clk));
  axi4_resp_t  #(.IDWIDTH(IDWIDTH))                  resp (.clk(clk));

//  modport master(output addr, output data, input resp);
//  modport slave(input addr, input data, output resp);

endinterface

interface axi4_rd_t #(parameter DWIDTH=64, parameter AWIDTH=32, parameter IDWIDTH=4)
                     (input clk);

  axi4_addr_t  #(.AWIDTH(AWIDTH), .IDWIDTH(IDWIDTH)) addr (.clk(clk));
  axi4_rdata_t #(.DWIDTH(DWIDTH), .IDWIDTH(IDWIDTH)) data (.clk(clk));

//  modport master(output addr, output data);
//  modport slave(input addr, input data);

endinterface

# AXI Interface Libraries

## AXI4 Stream (sim\_axis\_lib.vh)

Defines ``axis_t``, an AXI Stream bus interface that implements several tasks to send and 
receive data on the bus.

### Definition

    interface axis_t #(parameter DWIDTH = 64)
                      (input clk);
      logic [DWIDTH-1:0]  tdata;
      logic               tvalid;
      logic               tlast;
      logic               tready;
  
    modport master (output tdata, output tvalid, output tlast, input tready);
    modport slave (input tdata, input tvalid, input tlast, output tready);


### Operations

#### push\_word

    // Push a word onto the AXI-Stream bus and wait for it to transfer
    // Args:
    // - word: The data to push onto the bus
    // - eop (optional): End of packet (asserts tlast)

#### push\_bubble

    // Push a bubble cycle onto the AXI-Stream bus

#### pull\_word

    // Wait for a sample to be transferred on the AXI Stream
    // bus and return the data and last
    // Args:
    // - word: The data pulled from the bus
    // - eop: End of packet (tlast)

#### wait\_for\_bubble

    // Wait for a bubble cycle on the AXI Stream bus

#### wait\_for\_pkt

    // Wait for a packet to finish on the bus

#### push\_rand\_pkt

    // Push a packet with random data onto to the AXI Stream bus
    // Args:
    // - num_samps: Packet size.

#### push\_ramp\_pkt

    // Push a packet with a ramp on to the AXI Stream bus
    // Args:
    // - num_samps: Packet size.
    // - ramp_start: Start value for the ramp
    // - ramp_inc: Increment per clock cycle

## Compressed VITA [CHDR] (sim\_chdr\_lib.vh)

Defines ``cvita_stream_t``, an AXI Stream bus interface that implements the CHDR protocol and 
several tasks to send and receive data on the bus.

### CHDR

    typedef enum logic [1:0] {
      DATA=2'b00, FC=2'b01, CMD=2'b10, RESP=2'b11
    } cvita_pkt_t;
      
    typedef struct packed {
      logic [31:0]  sid;
      logic [15:0]  length;
      logic [11:0]  seqno;
      logic         eob;
      logic         has_time;
      cvita_pkt_t   pkt_type;
      logic [63:0]  timestamp;
    } cvita_hdr_t;

#### Operations

 - ``flatten_chdr_no_ts``: Flatten header struct to a 64-bit bus. No timestamp.
 - ``unflatten_chdr_no_ts``: Decode a 64-bit header and populate the ``cvita_hdr_t`` struct. No timestamp.
 - ``unflatten_chdr``: Decode a 64-bit header and populate the ``cvita_hdr_t`` struct. Timestamp supported.

### CVITA Stream Type

#### Definition

    interface cvita_stream_t (input clk);
      axis_t #(.DWIDTH(64)) axis (.clk(clk));

#### push\_hdr

    // Push a CVITA header into the stream
    // Args:
    // - hdr: The header to push
  
#### push\_data

    // Push a word onto the AXI-Stream bus and wait for it to transfer
    // Args:
    // - word: The data to push onto the bus
    // - eop: End of packet (asserts tlast)

#### push\_bubble

    // Push a bubble cycle on the AXI-Stream bus

#### pull\_word

    // Wait for a sample to be transferred on the AXI Stream
    // bus and return the data and last
    // Args:
    // - word: The data pulled from the bus
    // - eop: End of packet (tlast)

#### wait\_for\_bubble

    // Wait for a bubble cycle on the AXI Stream bus

#### wait\_for\_pkt

    // Wait for a packet to finish on the bus

#### wait\_for\_pkt\_get\_info

    // Wait for a packet to finish on the bus and extract the header and payload statistics.
    
    typedef struct packed {
      logic [31:0]  count;
      logic [63:0]  sum;
      logic [63:0]  min;
      logic [63:0]  max;
      logic [63:0]  crc;
    } cvita_stats_t;

#### push\_rand\_pkt

    // Push a packet with random data onto to the AXI Stream bus
    // Args:
    // - num_samps: Packet size.
    // - hdr: Header to attach to packet (length will be ignored)
    // - timestamp: Optional timestamp

#### push\_ramp\_pkt

    // Push a packet with a ramp on to the AXI Stream bus
    // Args:
    // - num_samps: Packet size.
    // - ramp_start: Start value for the ramp
    // - ramp_inc: Increment per clock cycle
    // - hdr: Header to attach to packet (length will be ignored)
    // - timestamp: Optional timestamp

## Memory Mapped AXI4 (sim\_axi4\_lib.vh)

Defines the following interfaces to group signals in the AXI4 bus.
WIP: No functions or tasks implemented yet.

#### Address

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
    
      modport master (output id,addr,len,size,burst,lock,cache,prot,qos,valid, input ready);
      modport slave (input id,addr,len,size,burst,lock,cache,prot,qos,valid, output ready);
    
    endinterface
    
#### Write Data

    interface axi4_wdata_t #(parameter DWIDTH=64)
                           (input clk);
    
      logic [DWIDTH-1:0]      data;
      logic [(DWIDTH/8)-1:0]  strb;
      logic                   last;
      logic                   user;
      logic                   valid;
      logic                   ready;
    
      modport master(output data,strb,last,valid, input ready);
      modport slave(input data,strb,last,valid, output ready);
    
    endinterface

#### Write Response

    interface axi4_resp_t #(parameter IDWIDTH=4)
                           (input clk);
    
      logic               ready;
      logic [IDWIDTH-1:0] id;
      logic [1:0]         resp;
      logic               user;
      logic               valid;
    
      modport master(output ready, input id,resp,valid);
      modport slave(input ready, output id,resp,valid);
    
    endinterface

#### Read Data

    interface axi4_rdata_t #(parameter DWIDTH=64, parameter IDWIDTH=4)
                           (input clk);
    
      logic               ready;
      logic [IDWIDTH-1:0] id;
      logic [DWIDTH-1:0]  data;
      logic [1:0]         resp;
      logic               user;
      logic               last;
      logic               valid;
    
      modport master(output ready, input id,data,resp,last,valid);
      modport slave(input ready, output id,data,resp,last,valid);
    
    endinterface

#### Meta: AXI4 Writer

    interface axi4_wr_t #(parameter DWIDTH=64, parameter AWIDTH=32, parameter IDWIDTH=4)
                         (input clk);
    
      axi4_addr_t  #(.AWIDTH(AWIDTH), .IDWIDTH(IDWIDTH)) addr (.clk(clk));
      axi4_wdata_t #(.DWIDTH(DWIDTH))                    data (.clk(clk));
      axi4_resp_t  #(.IDWIDTH(IDWIDTH))                  resp (.clk(clk));
    
      modport master(output addr, output data, input resp);
      modport slave(input addr, input data, output resp);
    
    endinterface
    
#### Meta: AXI4 Reader

    interface axi4_rd_t #(parameter DWIDTH=64, parameter AWIDTH=32, parameter IDWIDTH=4)
                         (input clk);
    
      axi4_addr_t  #(.AWIDTH(AWIDTH), .IDWIDTH(IDWIDTH)) addr (.clk(clk));
      axi4_rdata_t #(.DWIDTH(DWIDTH), .IDWIDTH(IDWIDTH)) data (.clk(clk));
    
      modport master(output addr, output data);
      modport slave(input addr, input data);
    
    endinterface

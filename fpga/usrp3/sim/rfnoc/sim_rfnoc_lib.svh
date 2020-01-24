//
// Copyright 2015 Ettus Research
//
// RFNoC sim lib's main goals are to:
// - Provide macros so the user can easily simulate a RFNoC design
// - Provide a fully compliant (i.e. has packetization, flow control) RFNoC interface 
//   to the user's test bench.
// The RFNoC interface comes from a Export I/O RFNoC block (noc_block_tb) that exposes 
// CVITA and AXI-Stream interfaces interfaces via the block's top level port list.
//
// Block Diagram:
//   -------------------------------------------------------------      -----------------------------
//  |                      AXI Crossbar                           |    |  bus_clk, bus_rst generator |
//   -------------------------------------------------------------      -----------------------------
//         |^                         |^                       |^
//         v|                         v|                       ||       -----------------------------
//   -------------   --------------------------------------    ||      |  ce_clk, ce_rst generator   |
//  |             | |             noc_block_tb             |   v|       -----------------------------
//  |  User DUT   | |       (Export I/O RFNoC Block)       | tb_config
//  | RFNoC Block | |                                      | (rfnoc_block_streamer)
//  |             | |  ----------------------------------  |
//   -------------  | |            NoC Shell             | |
//                  | |                                  | |
//                  | | (replicated x NUM_STREAMS)       | |
//                  | |   block port        cmdout ackin | |
//                  |  ----------------------------------  |
//                  |       |^                 ^     |     |
//                  |       |'-----------.     |     |     |
//                  |       v            |     |     |     |
//                  |    -------      -------  |     |     |  // Demux / Mux allows using both AXI Wrapper
//                  |   / Demux \    /  Mux  \ |     |     |  // and direct CVITA paths to NoC Shell
//                  |   ---------    --------- |     |     |  // simultaneously
//                  |     |   |        ^   ^   |     |     |
//                  |     |   | .------'   |   |     |     |
//                  |     |   | |          |   |     |     |
//                  |     |   --)------    |   |     |     |
//                  |     v     |     |.---'   |     |     |
//                  |  -------------  ||       |     |     |
//                  | | AXI Wrapper | ||       |     |     |
//                  |  -------------  ||       |     |     |
//                   -------|^--------||-------|-----|-----
//                          ||        ||       |     |
//                          v|        ||       |     v
//                   --------------------------------------
//                  |              tb_streamer             | 
//                  |           (rfnoc_block_streamer)     |
//                  |                                      |
//                  | Interface to provide user with easy  |
//                  | to use tasks for sending and         |
//                  | receiving data from RFNoC blocks in  |
//                  | the simulation.                      |
//                   --------------------------------------
//                                     |^
//                                     v|
//                   --------------------------------------
//                  |            User Testbench            |
//                   --------------------------------------
//                   
//
// Usage: Include sim_rfnoc_lib.sv, setup prerequisites with `RFNOC_SIM_INIT(), and add RFNoC blocks
//        with `RFNOC_ADD_BLOCK(). Connect RFNoC blocks into a flow graph using `RFNOC_CONNECT().
//
// Example:
// `include "sim_rfnoc_lib.svh"
//  module rfnoc_testbench();
//    localparam BUS_CLK_PERIOD = $ceil(1e9/166.67e6); // Bus clk at 166 MHz
//    localparam CE_CLK_PERIOD  = $ceil(1e9/200e6);    // Computation Engine clk at 200 MHz
//    localparam NUM_CE         = 2;   // Two computation engines
//    localparam NUM_STREAMS    = 1;   // One test bench stream
//    `RFNOC_SIM_INIT(NUM_CE, NUM_STREAMS, BUS_CLK_PERIOD, CE_CLK_PERIOD);
//    `RFNOC_ADD_BLOCK(noc_block_fir,0); // Instantiate FIR and connect to crossbar port 0
//    `RFNOC_ADD_BLOCK(noc_block_fft,1); // Instantiate FFT and connect to crossbar port 1
//    initial begin
//      // Note the special block 'noc_block_tb' which is added in `RFNOC_SIM_INIT()
//      `RFNOC_CONNECT(noc_block_tb,noc_block_fir,SC16,256);  // Connect test bench to FIR
//      `RFNOC_CONNECT(noc_block_fir,noc_block_fft,SC16,256); // Connect FIR to FFT. Packet size 256, stream's data type SC16
//    end
//  endmodule
//
// Warning: Most of the macros create specifically named signals used by other macros
`ifndef INCLUDED_RFNOC_SIM_LIB
`define INCLUDED_RFNOC_SIM_LIB

`include "sim_clks_rsts.vh"
`include "sim_cvita_lib.svh"
`include "sim_set_rb_lib.svh"
`include "noc_shell_regs.vh"
`include "data_types.vh"

// Interface with tasks for users to send and receive packets on CVITA and AXI-Stream interfaces
// Most test benches only need to use send(), recv(), write_user_reg(), read_user_reg()
interface rfnoc_block_streamer #(
  parameter AXIS_DWIDTH = 32,
  parameter CVITA_DWIDTH = 64,
  parameter SR_AWIDTH = 8,
  parameter SR_DWIDTH = 32,
  parameter RB_AWIDTH = 8,
  parameter RB_DWIDTH = 64,
  parameter STREAM_DWIDTH = 32, // Width used with send() / recv() calls
  parameter NUM_STREAMS = 1     // Limited to 16
)(
  input clk
);

  settings_bus_slave #(
    .SR_AWIDTH(SR_AWIDTH), .SR_DWIDTH(SR_DWIDTH),
    .RB_AWIDTH(RB_AWIDTH), .RB_DWIDTH(RB_DWIDTH),
    .NUM_BUSES(NUM_STREAMS))
  settings_bus_slave (
    .clk(clk));
  cvita_slave #(.DWIDTH(CVITA_DWIDTH)) s_cvita_ack(.clk(clk));
  cvita_master #(.DWIDTH(CVITA_DWIDTH)) m_cvita_cmd(.clk(clk));
  cvita_master #(.DWIDTH(CVITA_DWIDTH), .NUM_STREAMS(NUM_STREAMS)) m_cvita_data(.clk(clk));
  cvita_slave #(.DWIDTH(CVITA_DWIDTH), .NUM_STREAMS(NUM_STREAMS)) s_cvita_data(.clk(clk));
  axis_master #(.DWIDTH(AXIS_DWIDTH), .NUM_STREAMS(NUM_STREAMS)) m_axis_data(.clk(clk));
  axis_slave #(.DWIDTH(AXIS_DWIDTH), .NUM_STREAMS(NUM_STREAMS)) s_axis_data(.clk(clk));
  localparam bytes_per_cvita_line               = CVITA_DWIDTH/8;
  logic [15:0] src_sid[0:NUM_STREAMS-1]         = '{NUM_STREAMS{16'd0}};
  logic [15:0] dst_sid[0:NUM_STREAMS-1]         = '{NUM_STREAMS{16'd0}};
  logic [11:0] cmd_seqnum                       = 12'd0;
  logic [11:0] data_seqnum[0:NUM_STREAMS-1]     = '{NUM_STREAMS{12'd0}};
  int unsigned spp[0:NUM_STREAMS-1]             = '{NUM_STREAMS{0}};
  int unsigned ticks_per_word[0:NUM_STREAMS-1]  = '{NUM_STREAMS{1}};
  bit upstream_connected[0:NUM_STREAMS-1]       = '{NUM_STREAMS{0}};
  bit downstream_connected[0:NUM_STREAMS-1]     = '{NUM_STREAMS{0}};
  cvita_data_type_t data_type[0:NUM_STREAMS-1];

  function void check_upstream(int unsigned stream);
    check_stream(stream);
    assert (upstream_connected[stream] != 0) else begin
      $error("rfnoc_block_streamer::check_upstream(): Stream %0d: upstream is not connected! Check for missing RFNOC_CONNECT().", stream);
      $finish(0);
    end
  endfunction

  function void check_downstream(int unsigned stream);
    check_stream(stream);
    assert (downstream_connected[stream] != 0) else begin
      $error("rfnoc_block_streamer::check_downstream(): Stream %0d: downstream is not connected! Check for missing RFNOC_CONNECT().", stream);
      $finish(0);
    end
    assert (spp[stream] > 0) else begin
      $error("rfnoc_block_streamer::check_downstream(): Stream %0d: SPP cannot be 0!", stream);
      $finish(0);
    end
  endfunction

  function void check_stream(int unsigned stream);
    assert (stream < NUM_STREAMS) else begin
      $error("rfnoc_block_streamer::check_stream(): Stream %0d: Tried to perform operation on non-existent stream!", stream);
      $finish(0);
    end
  endfunction

  function void set_src_sid(input logic [15:0] _src_sid, input int unsigned stream = 0);
    check_stream(stream);
    src_sid[stream] = _src_sid;
  endfunction

  function logic [15:0] get_src_sid(input stream = 0);
    check_stream(stream);
    return(src_sid[stream]);
  endfunction

  function void set_dst_sid(input logic [15:0] _dst_sid, input int unsigned stream = 0);
    check_stream(stream);
    dst_sid[stream] = _dst_sid;
    data_seqnum[stream] = 12'd0;
  endfunction

  function logic [15:0] get_dst_sid(input int unsigned stream = 0);
    check_stream(stream);
    return(dst_sid[stream]);
  endfunction

  function void set_seqnum(input logic [11:0] _seqnum, input int unsigned stream = 0);
    check_stream(stream);
    data_seqnum[stream] = _seqnum;
  endfunction

  function logic [11:0] get_seqnum(input int unsigned stream);
    check_stream(stream);
    return(data_seqnum[stream]);
  endfunction

  function void clear_seqnum(input int unsigned stream);
    check_stream(stream);
    data_seqnum[stream] = 12'd0;
  endfunction

  function void set_cmd_seqnum(input logic [11:0] seqnum);
    cmd_seqnum = seqnum;
  endfunction

  function logic [11:0] get_cmd_seqnum;
    return(cmd_seqnum);
  endfunction

  function void clear_cmd_seqnum;
    cmd_seqnum = 12'd0;
  endfunction

  function void set_spp(input int unsigned _spp, input int unsigned stream = 0);
    check_stream(stream);
    spp[stream] = _spp;
  endfunction

  function int unsigned get_spp(input int unsigned stream = 0);
    check_stream(stream);
    return(spp[stream]);
  endfunction

  function void set_ticks_per_word(input int unsigned _ticks_per_word, input int unsigned stream = 0);
    check_stream(stream);
    ticks_per_word[stream] = _ticks_per_word;
  endfunction

  function int unsigned get_ticks_per_word(input int unsigned stream = 0);
    check_stream(stream);
    return(ticks_per_word[stream]);
  endfunction

  function void set_data_type(input cvita_data_type_t _data_type, input int unsigned stream = 0);
    check_stream(stream);
    data_type[stream] = _data_type;
  endfunction

  function cvita_data_type_t get_data_type(input int unsigned stream = 0);
    check_stream(stream);
    return(data_type[stream]);
  endfunction

  function void connect_upstream(input int unsigned stream);
    upstream_connected[stream] = 1;
  endfunction

  function void disconnect_upstream(input int unsigned stream);
    upstream_connected[stream] = 0;
  endfunction

  function bit get_upstream_connected(input int unsigned stream);
    return(upstream_connected[stream]);
  endfunction

  function void connect_downstream(input int unsigned stream);
    downstream_connected[stream] = 1;
  endfunction

  function void disconnect_downstream(input int unsigned stream);
    downstream_connected[stream] = 0;
  endfunction

  function bit get_downstream_connected(input int unsigned stream);
    return(downstream_connected[stream]);
  endfunction

  // Reset state of all signals / properties
  task automatic reset;
    begin
      settings_bus_slave.reset();
      s_cvita_ack.reset();
      m_cvita_cmd.reset();
      m_cvita_data.reset();
      s_cvita_data.reset();
      m_axis_data.reset();
      s_axis_data.reset();
      src_sid = '{NUM_STREAMS{16'd0}};
      dst_sid = '{NUM_STREAMS{16'd0}};
      cmd_seqnum = 12'd0;
      data_seqnum[0:NUM_STREAMS-1] = '{NUM_STREAMS{12'd0}};
      spp[0:NUM_STREAMS-1] = '{NUM_STREAMS{0}};
      upstream_connected = '{NUM_STREAMS{0}};
      downstream_connected = '{NUM_STREAMS{0}};
    end
  endtask

  // Send CVITA data packet(s) into the specified stream.
  // Handles header, breaking payload into multiple packets
  // (based on spp), and incrementing sequence number.
  //
  // Args:
  // - payload:   Packet payload
  // - metadata:  Settings for packet (Optional)
  // - stream:    Stream to send packet on (Optional)
  task automatic send (
    input cvita_payload_t payload,
    input cvita_metadata_t metadata = '{eob:1'b0, has_time:1'b0, timestamp:64'd0},
    input int unsigned stream = 0);
    begin
      cvita_pkt_t pkt;
      int unsigned bytes_per_word;
      int unsigned lines_per_pkt;
      int unsigned num_pkts;
      logic [63:0] timestamp;

      check_downstream(stream);

      bytes_per_word = data_type[stream].bytes_per_word;
      lines_per_pkt  = spp[stream]*bytes_per_word/bytes_per_cvita_line;
      num_pkts       = $ceil(real'(payload.size())/lines_per_pkt);
      timestamp      = metadata.timestamp;
      // Break up payload across multiple packets
      for (int unsigned i = 0; i < num_pkts; i++) begin
        if (i == num_pkts-1) begin
          pkt.payload = payload[i*lines_per_pkt:payload.size-1];
          pkt.hdr = '{pkt_type:DATA, has_time:metadata.has_time, eob:metadata.eob /* Set EOB on final packet */,
                      seqnum:data_seqnum[stream], length:8*(pkt.payload.size()+metadata.has_time+1),
                      src_sid:src_sid[stream], dst_sid:dst_sid[stream], timestamp:timestamp};
        end else begin
          pkt.payload = payload[i*lines_per_pkt:(i+1)*lines_per_pkt-1];
          pkt.hdr = '{pkt_type:DATA, has_time:metadata.has_time, eob:1'b0,
                      seqnum:data_seqnum[stream], length:8*(pkt.payload.size()+metadata.has_time+1),
                      src_sid:src_sid[stream], dst_sid:dst_sid[stream], timestamp:timestamp};
        end
        if (pkt.payload.size() != lines_per_pkt) begin
          $info("rfnoc_block_streamer::push(): Sending partial packet with %0d words (type: %s)",
                 bytes_per_cvita_line*pkt.payload.size()/bytes_per_word, data_type[stream].name);
        end
        push_pkt(pkt, stream);
        data_seqnum[stream] += 1'b1;
        timestamp += ticks_per_word[stream]*spp[stream];
      end
    end
  endtask

  // Receive CVITA data packet(s) from the specified stream.
  //
  // Args:
  // - payload:   Packet payload
  // - metadata:  Packet settings
  // - stream:    Stream to send packet on (Optional)
  task automatic recv (
    output cvita_payload_t payload,
    output cvita_metadata_t metadata,
    input int unsigned stream = 0);
    begin
      cvita_pkt_t pkt;
      check_upstream(stream);
      pull_pkt(pkt, stream);
      payload = pkt.payload;
      metadata.eob = pkt.hdr.eob;
      metadata.has_time = pkt.hdr.has_time;
      metadata.timestamp = pkt.hdr.timestamp;
    end
  endtask

  // Push a user defined CVITA packet into the stream.
  // Note: This is a direct packet interface, it is up
  //       to the caller setup the header correctly!
  // Args:
  // - pkt:     Packet payload
  // - stream:  Stream to send packet on (Optional)
  task automatic push_pkt (
    input cvita_pkt_t pkt,
    input int unsigned stream = 0);
    begin
      check_downstream(stream);
      m_cvita_data.push_pkt(pkt, stream);
    end
  endtask

  // Push a word onto the AXI-Stream bus and wait for it to transfer
  // Args:
  // - word:    Data to push onto the bus
  // - eop:     End of packet (asserts tlast) (Optional)
  // - stream:  Stream to use (Optional)
  task automatic push_word (
    input logic [AXIS_DWIDTH-1:0] word,
    input logic eop = 1'b0,
    input int unsigned stream = 0);
    begin
      check_downstream(stream);
      m_axis_data.push_word(word, eop, stream);
    end
  endtask

  // Pull a packet from the AXI Stream bus.
  // Args:
  // - pkt:     Packet data (queue)
  // - stream:  Stream to use (Optional)
  task automatic pull_pkt (
    output cvita_pkt_t pkt,
    input int unsigned stream = 0);
    begin
      check_upstream(stream);
      s_cvita_data.pull_pkt(pkt, stream);
    end
  endtask

  // Pull a word from the AXI Stream bus and
  // return the data and last
  // Args:
  // - word:    The data pulled from the bus
  // - eop:     End of packet (tlast)
  // - stream:  Stream to use (Optional)
  task automatic pull_word (
    output logic [AXIS_DWIDTH-1:0] word,
    output logic eop,
    input int unsigned stream = 0);
    begin
      check_upstream(stream);
      s_axis_data.pull_word(word, eop, stream);
    end
  endtask

  // Drop a CVITA packet
  // Args:
  // - stream:  Stream to use (Optional)
  task automatic drop_pkt (
    input int unsigned stream = 0);
    begin
      cvita_pkt_t dropped_pkt;
      check_upstream(stream);
      pull_pkt(dropped_pkt, stream);
    end
  endtask

  // Drop a word on the AXI-stream bus
  // Args:
  // - stream:  Stream to use (Optional)
  task automatic drop_word (
    input int unsigned stream = 0);
    begin
      logic [AXIS_DWIDTH-1:0] dropped_word;
      logic dropped_eop;
      check_upstream(stream);
      pull_word(dropped_word, dropped_eop, stream);
    end
  endtask

  // Push a command packet into the stream.
  // Args:
  // - dst_sid:   Destination SID (Stream ID, Optional)
  // - word:      Word to send
  // - response:  Response word
  // - metadata:  Packet settings (Optional)
  task automatic push_cmd (
    input logic [15:0] dst_sid = dst_sid[0],
    input logic [63:0] word,
    output logic [63:0] response,
    input cvita_metadata_t metadata = '{eob:1'b0, has_time:1'b0, timestamp:64'd0});
    begin
      cvita_pkt_t cmd_pkt;
      cvita_payload_t payload;
      cvita_metadata_t md;
      cmd_pkt.hdr = '{pkt_type:CMD, has_time:metadata.has_time, eob:metadata.eob,
                      seqnum:cmd_seqnum, length:8*(metadata.has_time+2),
                      src_sid:src_sid[0], dst_sid:dst_sid, timestamp:metadata.timestamp};
      // Vivado XSIM workaround... assigning to queue in a struct works but push_back() does not
      payload.push_back(word);
      cmd_pkt.payload = payload;
      m_cvita_cmd.push_pkt(cmd_pkt);
      pull_resp(response,md);
      cmd_seqnum += 1'b1;
    end
  endtask

  // Push a command packet on the stream.
  // Args:
  // - cmd_pkt:  Command CVITA packet
  task automatic push_cmd_pkt (
    input cvita_pkt_t cmd_pkt);
    begin
      m_cvita_cmd.push_pkt(cmd_pkt);
    end
  endtask

  // Pull a response packet from the stream.
  // Args:
  // - response:  Response word
  // - eob:       State of End of Burst flag
  task automatic pull_resp (
    output logic [63:0] response,
    output cvita_metadata_t metadata);
    begin
      cvita_pkt_t resp_pkt;
      s_cvita_ack.pull_pkt(resp_pkt);
      response = resp_pkt.payload[0];
      metadata.eob = resp_pkt.hdr.eob;
      metadata.has_time = resp_pkt.hdr.has_time;
      metadata.timestamp = resp_pkt.hdr.timestamp;
    end
  endtask

  // Pull a response packet from the stream.
  // Args:
  // - resp_pkt:  Response CVITA packet
  task automatic pull_resp_pkt (
    output cvita_pkt_t resp_pkt);
    begin
      s_cvita_ack.pull_pkt(resp_pkt);
    end
  endtask

  // Write register and return readback value
  // Args:
  // - dst_sid:     Destination SID (Stream ID)
  // - addr:        Register address
  // - word:        Value to write to register 
  // - readback:    Readback word
  // - block_port:  Block port
  task automatic write_reg_readback (
    input logic [15:0] _dst_sid = dst_sid[0],
    input logic [7:0] addr,
    input logic [31:0] data,
    output logic [63:0] readback,
    input int unsigned block_port = 0);
    begin
      push_cmd(_dst_sid+block_port, {24'd0,addr,data}, readback);
    end
  endtask

  // Timed write register and return readback value
  // Note: Readback can stall this call as response
  //       packet is output after register is written
  // Args:
  // - dst_sid:     Destination SID (Stream ID)
  // - addr:        Register address
  // - word:        Value to write to register 
  // - timestamp:   Command execution time
  // - readback:    Readback word
  // - block_port:  Block port
  task automatic write_reg_readback_timed (
    input logic [15:0] _dst_sid = dst_sid[0],
    input logic [7:0] addr,
    input logic [31:0] data,
    input logic [63:0] timestamp,
    output logic [63:0] readback,
    input int unsigned block_port = 0);
    begin
      cvita_metadata_t md;
      md.has_time = 1;
      md.timestamp = timestamp;
      push_cmd(_dst_sid+block_port, {24'd0,addr,data}, readback, md);
    end
  endtask

  // Write register, drop readback word
  // Args:
  // - dst_sid:     Destination SID (Stream ID)
  // - addr:        Register address
  // - word:        Value to write to register
  // - block_port:  Block port
  task automatic write_reg (
    input logic [15:0] _dst_sid = dst_sid[0],
    input logic [7:0] addr,
    input logic [31:0] data,
    input int unsigned block_port = 0);
    begin
      logic [63:0] readback;
      write_reg_readback(_dst_sid, addr, data, readback, block_port);
    end
  endtask

  // Timed write register, drop readback word
  // Note: Readback can stall this call as response
  //       packet is output after register is written
  // Args:
  // - dst_sid:     Destination SID (Stream ID)
  // - addr:        Register address
  // - word:        Value to write to register
  // - timestamp:   Command execution time
  // - block_port:  Block port
  task automatic write_reg_timed (
    input logic [15:0] _dst_sid = dst_sid[0],
    input logic [7:0] addr,
    input logic [31:0] data,
    input logic [63:0] timestamp,
    input int unsigned block_port = 0);
    begin
      logic [63:0] readback;
      write_reg_readback_timed(_dst_sid, addr, data, timestamp, readback, block_port);
    end
  endtask

  // Write user register, drop readback word
  // Args:
  // - dst_sid:     Destination SID (Stream ID)
  // - addr:        Register address
  // - word:        Value to write to register
  // - block_port:  Block port
  task automatic write_user_reg (
    input logic [15:0] _dst_sid = dst_sid[0],
    input logic [7:0] addr,
    input logic [31:0] data,
    input int unsigned block_port = 0);
    begin
      write_reg(_dst_sid, addr, data, block_port);
    end
  endtask

  // Timed write user register, drop readback word
  // Note: Readback can stall this call as response
  //       packet is output after register is written
  // Args:
  // - dst_sid:     Destination SID (Stream ID)
  // - addr:        Register address
  // - word:        Value to write to register 
  // - timestamp:   Command execution time
  // - block_port:  Block port
  task automatic write_user_reg_timed (
    input logic [15:0] _dst_sid = dst_sid[0],
    input logic [7:0] addr,
    input logic [31:0] data,
    input logic [63:0] timestamp,
    input int unsigned block_port = 0);
    begin
      write_reg_timed(_dst_sid, addr, data, timestamp, block_port);
    end
  endtask

  // Read user register
  // Args:
  // - dst_sid:     Destination SID (Stream ID)
  // - addr:        Register address
  // - readback:    Readback word
  // - block_port:  Block port
  task automatic read_user_reg (
    input logic [15:0] _dst_sid = dst_sid[0],
    input logic [7:0] addr, // Only 128 user registers (128-255)
    output logic [63:0] readback,
    input int unsigned block_port = 0);
    begin
      // Set user readback mux
      write_reg(_dst_sid, SR_RB_ADDR_USER, addr, block_port);
      read_reg(_dst_sid, RB_USER_RB_DATA, readback, block_port);
    end
  endtask

  // Read NoC shell register
  // Args:
  // - dst_sid:     Destination SID (Stream ID)
  // - addr:        Register address
  // - readback:    Readback word
  // - block_port:  Block port
  task automatic read_reg (
    input logic [15:0] _dst_sid = dst_sid[0],
    input logic [7:0] addr,
    output logic [63:0] readback,
    input int unsigned block_port = 0);
    begin
      // Set NoC Shell readback mux, response packet will have readback data
      write_reg_readback(_dst_sid, SR_RB_ADDR, addr, readback, block_port);
    end
  endtask

  // Read NoC shell FIFO size register
  // Args:
  // - dst_sid:     Destination SID (Stream ID)
  // - readback:    Readback word
  // - block_port:  Block port
  task automatic read_fifo_size_reg (
    input logic [15:0] _dst_sid = dst_sid[0],
    output logic [63:0] readback,
    input int unsigned block_port = 0);
    begin
      read_reg(_dst_sid, RB_FIFOSIZE, readback, block_port);
    end
  endtask
endinterface

// Setup a RFNoC simulation. Creates clocks (bus_clk, ce_clk), resets (bus_rst, ce_rst), an
// AXI crossbar, and Export IO RFNoC block instance. Export IO is a special RFNoC block used
// to expose CVITA and AXI-stream streams to the user.
//
// Note: Several of the called macros instantiate signals with hard coded names for the 
// test bench to use.
//
// Usage: `RFNOC_SIM_INIT()
//   - num_user_blocks:               Number of RFNoC blocks in simulation. Max 14.
//   - num_testbench_streams:         Number of streams available to user test bench
//   - bus_clk_period, ce_clk_period: Bus, RFNoC block clock frequencies
//
`define RFNOC_SIM_INIT(num_user_blocks, num_testbench_streams, bus_clk_period, ce_clk_period) \
  `DEFINE_CLK(bus_clk, bus_clk_period, 50); \
  `DEFINE_RESET(bus_rst, 0, 1000); \
  `DEFINE_CLK(ce_clk, ce_clk_period, 50); \
  `DEFINE_RESET(ce_rst, 0, 1000); \
  `RFNOC_ADD_AXI_CROSSBAR(0, num_user_blocks+2); \
  `RFNOC_ADD_TESTBENCH_BLOCK(tb, num_testbench_streams, num_user_blocks, ce_clk, ce_rst); \
  `RFNOC_ADD_CONFIG_PORT(tb_config, num_user_blocks+1);

// Instantiates an AXI crossbar and related signals. Instantiates several signals
// starting with the prefix 'xbar_'.
//
// Usage: `INST_AXI_CROSSBAR()
//   - _xbar_addr:    Crossbar address
//   - _num_ports:    Number of crossbar ports
//
`define RFNOC_ADD_AXI_CROSSBAR(_xbar_addr, _num_ports) \
  localparam [7:0] xbar_addr = _xbar_addr; \
  settings_bus_master #(.SR_AWIDTH(16), .SR_DWIDTH(32)) xbar_set_bus(.clk(bus_clk)); \
  settings_bus_master #(.RB_AWIDTH(8), .RB_DWIDTH(32)) xbar_rb_bus(.clk(bus_clk)); \
  initial begin \
    xbar_set_bus.reset(); \
    xbar_rb_bus.reset(); \
  end \
  logic [(_num_ports)*64-1:0]  xbar_i_tdata; \
  logic [_num_ports-1:0]       xbar_i_tlast, xbar_i_tvalid, xbar_i_tready; \
  logic [(_num_ports)*64-1:0]  xbar_o_tdata; \
  logic [_num_ports-1:0]       xbar_o_tlast, xbar_o_tvalid, xbar_o_tready; \
  logic xbar_set_stb, xbar_rb_stb; \
  logic [15:0] xbar_set_addr; \
  logic [8:0]  xbar_rb_addr; \
  logic [31:0] xbar_set_data; \
  logic [31:0] xbar_rb_data; \
  assign xbar_set_stb  = xbar_set_bus.settings_bus.set_stb; \
  assign xbar_set_addr = xbar_set_bus.settings_bus.set_addr; \
  assign xbar_set_data = xbar_set_bus.settings_bus.set_data; \
  assign xbar_rb_stb = xbar_rb_bus.settings_bus.set_stb; \
  assign xbar_rb_addr = xbar_rb_bus.settings_bus.rb_addr; \
  assign xbar_rb_bus.settings_bus.rb_data = xbar_rb_data; \
  always @(posedge bus_clk) xbar_rb_bus.settings_bus.rb_stb <= xbar_rb_stb; \
  axi_crossbar #( \
    .BASE(0), \
    .FIFO_WIDTH(64), \
    .DST_WIDTH(16), \
    .NUM_INPUTS(_num_ports), \
    .NUM_OUTPUTS(_num_ports)) \
  axi_crossbar ( \
    .clk(bus_clk), \
    .reset(bus_rst), \
    .clear(1'b0), \
    .local_addr(8'(_xbar_addr)), \
    .i_tdata(xbar_i_tdata), \
    .i_tvalid(xbar_i_tvalid), \
    .i_tlast(xbar_i_tlast), \
    .i_tready(xbar_i_tready), \
    .pkt_present(xbar_i_tvalid), \
    .set_stb(xbar_set_stb), \
    .set_addr(xbar_set_addr), \
    .set_data(xbar_set_data), \
    .o_tdata(xbar_o_tdata), \
    .o_tvalid(xbar_o_tvalid), \
    .o_tlast(xbar_o_tlast), \
    .o_tready(xbar_o_tready), \
    .rb_rd_stb(xbar_rb_stb), \
    .rb_addr(xbar_rb_addr[2*($clog2(_num_ports+1)-1):0]), \
    .rb_data(xbar_rb_data));

// Instantiate and connect a RFNoC block to a crossbar. Expects clock & reset
// signals to be defined with the names ce_clk & ce_rst.
//
// Usage: `RFNOC_ADD_BLOCK()
//  - noc_block_name:  Name of RFNoC block to instantiate, i.e. noc_block_fft
//  - port_num:        Crossbar port to connect RFNoC block to 
//
`define RFNOC_ADD_BLOCK(noc_block_name, port_num) \
  `RFNOC_ADD_BLOCK_EXTENDED(noc_block_name, port_num, ce_clk, ce_rst,)

// Instantiate and connect a RFNoC block to a crossbar. Includes extra parameters
// for custom clock / reset signals and expanding the RFNoC block's name.
//
// Usage: `RFNOC_ADD_BLOCK_EXTENDED()
//  - noc_block_name:  Name of RFNoC block to instantiate, i.e. noc_block_fft
//  - port_num:        Crossbar port to connect block to 
//  - ce_clk, ce_rst:  RFNoC block clock and reset
//  - append:          Append to instance name, useful if instantiating 
//                     several of the same kind of RFNoC block and need unique
//                     instance names. Otherwise leave blank.
//
`define RFNOC_ADD_BLOCK_EXTENDED(noc_block_name, port_num, ce_clk, ce_rst, append) \
  `RFNOC_ADD_BLOCK_CUSTOM(``noc_block_name``append``, port_num) \
  noc_block_name \
  noc_block_name``append ( \
    .bus_clk(bus_clk), \
    .bus_rst(bus_rst), \
    .ce_clk(ce_clk), \
    .ce_rst(ce_rst), \
    .i_tdata(``noc_block_name``append``_i_tdata), \
    .i_tlast(``noc_block_name``append``_i_tlast), \
    .i_tvalid(``noc_block_name``append``_i_tvalid), \
    .i_tready(``noc_block_name``append``_i_tready), \
    .o_tdata(``noc_block_name``append``_o_tdata), \
    .o_tlast(``noc_block_name``append``_o_tlast), \
    .o_tvalid(``noc_block_name``append``_o_tvalid), \
    .o_tready(``noc_block_name``append``_o_tready), \
    .debug());

// Only creates signals and assignments to connect RFNoC block
// to the crossbar. User is responsible for instantiating the
// block and connecting the signals.
// 
// Usage: `RFNOC_ADD_BLOCK_CUSTOM()
//  - noc_block_name:  Name of RFNoC block
//  - port_num:        Crossbar port to connect
`define RFNOC_ADD_BLOCK_CUSTOM(noc_block_name, port_num) \
  localparam [15:0] sid_``noc_block_name`` = {xbar_addr,4'd0+port_num,4'd0}; \
  logic [63:0] ``noc_block_name``_i_tdata; \
  logic        ``noc_block_name``_i_tlast; \
  logic        ``noc_block_name``_i_tvalid; \
  logic        ``noc_block_name``_i_tready; \
  logic [63:0] ``noc_block_name``_o_tdata; \
  logic        ``noc_block_name``_o_tlast; \
  logic        ``noc_block_name``_o_tvalid; \
  logic        ``noc_block_name``_o_tready; \
  assign ``noc_block_name``_i_tdata = xbar_o_tdata[64*(port_num)+63:64*(port_num)]; \
  assign ``noc_block_name``_i_tlast = xbar_o_tlast[port_num]; \
  assign ``noc_block_name``_i_tvalid = xbar_o_tvalid[port_num]; \
  assign xbar_o_tready[port_num] = ``noc_block_name``_i_tready; \
  assign xbar_i_tdata[64*(port_num)+63:64*(port_num)] = ``noc_block_name``_o_tdata; \
  assign xbar_i_tlast[port_num]  = ``noc_block_name``_o_tlast; \
  assign xbar_i_tvalid[port_num] = ``noc_block_name``_o_tvalid; \
  assign ``noc_block_name``_o_tready = xbar_i_tready[port_num]; \

// Instantiate and connect the export I/O RFNoC block to a crossbar. Export I/O is a block that exports
// the internal NoC Shell & AXI Wrapper I/O to the port list. The block is useful for test benches to
// use to interact with other RFNoC blocks via the standard RFNoC user interfaces.
// 
// Instantiates several signals starting with the prefix specified by name, usually 'tb'
//
// Usage: `RFNOC_ADD_TESTBENCH_BLOCK()
//  - name:            Instance name
//  - num_streams:     Sets number of block ports on noc_block_tb
//  - port_num:        Crossbar port to connect block to
//
`define RFNOC_ADD_TESTBENCH_BLOCK(name, num_streams, port_num, _clk, _rst) \
  `RFNOC_ADD_BLOCK_CUSTOM(noc_block_``name``, port_num) \
  rfnoc_block_streamer #(.NUM_STREAMS(num_streams)) ``name``_streamer(.clk(ce_clk)); \
  initial begin \
    ``name``_streamer.reset(); \
    for (int unsigned k = 0; k < num_streams; k++) begin \
      name``_streamer.set_src_sid(16'(sid_noc_block_``name + k), k); \
    end \
  end \
  logic [num_streams-1:0]      ``name``_set_stb; \
  logic [num_streams*8-1:0]    ``name``_set_addr; \
  logic [(num_streams)*32-1:0] ``name``_set_data; \
  logic [num_streams-1:0]      ``name``_rb_stb; \
  logic [num_streams*8-1:0]    ``name``_rb_addr; \
  logic [(num_streams)*64-1:0] ``name``_rb_data; \
  logic [63:0] ``name``_s_cvita_cmd_tdata; \
  logic        ``name``_s_cvita_cmd_tlast; \
  logic        ``name``_s_cvita_cmd_tvalid; \
  logic        ``name``_s_cvita_cmd_tready; \
  logic [63:0] ``name``_m_cvita_ack_tdata; \
  logic        ``name``_m_cvita_ack_tlast; \
  logic        ``name``_m_cvita_ack_tvalid; \
  logic        ``name``_m_cvita_ack_tready; \
  logic [(num_streams)*64-1:0] ``name``_m_cvita_data_tdata; \
  logic [num_streams-1:0]      ``name``_m_cvita_data_tlast; \
  logic [num_streams-1:0]      ``name``_m_cvita_data_tvalid; \
  logic [num_streams-1:0]      ``name``_m_cvita_data_tready; \
  logic [(num_streams)*64-1:0] ``name``_s_cvita_data_tdata; \
  logic [num_streams-1:0]      ``name``_s_cvita_data_tlast; \
  logic [num_streams-1:0]      ``name``_s_cvita_data_tvalid; \
  logic [num_streams-1:0]      ``name``_s_cvita_data_tready; \
  logic [(num_streams)*32-1:0] ``name``_m_axis_data_tdata; \
  logic [num_streams-1:0]      ``name``_m_axis_data_tlast; \
  logic [num_streams-1:0]      ``name``_m_axis_data_tvalid; \
  logic [num_streams-1:0]      ``name``_m_axis_data_tready; \
  logic [(num_streams)*32-1:0] ``name``_s_axis_data_tdata; \
  logic [num_streams-1:0]      ``name``_s_axis_data_tlast; \
  logic [num_streams-1:0]      ``name``_s_axis_data_tvalid; \
  logic [num_streams-1:0]      ``name``_s_axis_data_tready; \
  assign ``name``_s_cvita_cmd_tdata  = ``name``_streamer.m_cvita_cmd.axis.tdata; \
  assign ``name``_s_cvita_cmd_tlast  = ``name``_streamer.m_cvita_cmd.axis.tlast; \
  assign ``name``_s_cvita_cmd_tvalid = ``name``_streamer.m_cvita_cmd.axis.tvalid; \
  assign ``name``_streamer.m_cvita_cmd.axis.tready = ``name``_s_cvita_cmd_tready; \
  assign ``name``_streamer.s_cvita_ack.axis.tdata = ``name``_m_cvita_ack_tdata; \
  assign ``name``_streamer.s_cvita_ack.axis.tlast = ``name``_m_cvita_ack_tlast; \
  assign ``name``_streamer.s_cvita_ack.axis.tvalid = ``name``_m_cvita_ack_tvalid; \
  assign ``name``_m_cvita_ack_tready = ``name``_streamer.s_cvita_ack.axis.tready; \
  generate \
    for (genvar i = 0; i < num_streams; i = i + 1) begin \
      assign ``name``_streamer.settings_bus_slave.settings_bus.set_stb[i] = ``name``_set_stb[i]; \
      assign ``name``_streamer.settings_bus_slave.settings_bus.set_addr[8*i+7:8*i] = ``name``_set_addr[8*i+7:8*i]; \
      assign ``name``_streamer.settings_bus_slave.settings_bus.set_data[32*i+31:32*i] = ``name``_set_data[32*i+31:32*i]; \
      assign ``name``_streamer.settings_bus_slave.settings_bus.rb_addr[8*i+7:8*i] = ``name``_rb_addr[8*i+7:8*i]; \
      assign ``name``_rb_stb[i] = ``name``_streamer.settings_bus_slave.settings_bus.rb_stb[i]; \
      assign ``name``_rb_data[64*i+63:64*i] = ``name``_streamer.settings_bus_slave.settings_bus.rb_data[64*i+63:64*i]; \
      assign ``name``_s_cvita_data_tdata[i*64+63:i*64] = ``name``_streamer.m_cvita_data.axis.tdata[i*64+63:i*64]; \
      assign ``name``_s_cvita_data_tlast[i] = ``name``_streamer.m_cvita_data.axis.tlast[i]; \
      assign ``name``_s_cvita_data_tvalid[i] = ``name``_streamer.m_cvita_data.axis.tvalid[i]; \
      assign ``name``_streamer.m_cvita_data.axis.tready[i] = ``name``_s_cvita_data_tready[i]; \
      assign ``name``_streamer.s_cvita_data.axis.tdata[i*64+63:i*64] = ``name``_m_cvita_data_tdata[i*64+63:i*64]; \
      assign ``name``_streamer.s_cvita_data.axis.tlast[i] = ``name``_m_cvita_data_tlast[i]; \
      assign ``name``_streamer.s_cvita_data.axis.tvalid[i] = ``name``_m_cvita_data_tvalid[i]; \
      assign ``name``_m_cvita_data_tready[i] = ``name``_streamer.s_cvita_data.axis.tready[i]; \
      assign ``name``_s_axis_data_tdata[i*32+31:i*32] = ``name``_streamer.m_axis_data.axis.tdata[i*32+31:i*32]; \
      assign ``name``_s_axis_data_tlast[i] = ``name``_streamer.m_axis_data.axis.tlast[i]; \
      assign ``name``_s_axis_data_tvalid[i] = ``name``_streamer.m_axis_data.axis.tvalid[i]; \
      assign ``name``_streamer.m_axis_data.axis.tready[i] = ``name``_s_axis_data_tready[i]; \
      assign ``name``_streamer.s_axis_data.axis.tdata[i*32+31:i*32] = ``name``_m_axis_data_tdata[i*32+31:i*32]; \
      assign ``name``_streamer.s_axis_data.axis.tlast[i] = ``name``_m_axis_data_tlast[i]; \
      assign ``name``_streamer.s_axis_data.axis.tvalid[i] = ``name``_m_axis_data_tvalid[i]; \
      assign ``name``_m_axis_data_tready[i] = ``name``_streamer.s_axis_data.axis.tready[i]; \
    end \
  endgenerate \
  noc_block_export_io #(.NUM_PORTS(num_streams)) \
  noc_block_``name ( \
    .bus_clk(bus_clk), \
    .bus_rst(bus_rst), \
    .ce_clk(_clk), \
    .ce_rst(_rst), \
    .i_tdata(noc_block_``name``_i_tdata), \
    .i_tlast(noc_block_``name``_i_tlast), \
    .i_tvalid(noc_block_``name``_i_tvalid), \
    .i_tready(noc_block_``name``_i_tready), \
    .o_tdata(noc_block_``name``_o_tdata), \
    .o_tlast(noc_block_``name``_o_tlast), \
    .o_tvalid(noc_block_``name``_o_tvalid), \
    .o_tready(noc_block_``name``_o_tready), \
    .set_stb(``name``_set_stb), \
    .set_addr(``name``_set_addr), \
    .set_data(``name``_set_data), \
    .rb_stb(``name``_rb_stb), \
    .rb_addr(``name``_rb_addr), \
    .rb_data(``name``_rb_data), \
    .s_cvita_cmd_tdata(``name``_s_cvita_cmd_tdata), \
    .s_cvita_cmd_tlast(``name``_s_cvita_cmd_tlast), \
    .s_cvita_cmd_tvalid(``name``_s_cvita_cmd_tvalid), \
    .s_cvita_cmd_tready(``name``_s_cvita_cmd_tready), \
    .m_cvita_ack_tdata(``name``_m_cvita_ack_tdata), \
    .m_cvita_ack_tlast(``name``_m_cvita_ack_tlast), \
    .m_cvita_ack_tvalid(``name``_m_cvita_ack_tvalid), \
    .m_cvita_ack_tready(``name``_m_cvita_ack_tready), \
    .s_cvita_data_tdata(``name``_s_cvita_data_tdata), \
    .s_cvita_data_tlast(``name``_s_cvita_data_tlast), \
    .s_cvita_data_tvalid(``name``_s_cvita_data_tvalid), \
    .s_cvita_data_tready(``name``_s_cvita_data_tready), \
    .m_cvita_data_tdata(``name``_m_cvita_data_tdata), \
    .m_cvita_data_tlast(``name``_m_cvita_data_tlast), \
    .m_cvita_data_tvalid(``name``_m_cvita_data_tvalid), \
    .m_cvita_data_tready(``name``_m_cvita_data_tready), \
    .s_axis_data_tdata(``name``_s_axis_data_tdata), \
    .s_axis_data_tlast(``name``_s_axis_data_tlast), \
    .s_axis_data_tvalid(``name``_s_axis_data_tvalid), \
    .s_axis_data_tready(``name``_s_axis_data_tready), \
    .m_axis_data_tdata(``name``_m_axis_data_tdata), \
    .m_axis_data_tlast(``name``_m_axis_data_tlast), \
    .m_axis_data_tvalid(``name``_m_axis_data_tvalid), \
    .m_axis_data_tready(``name``_m_axis_data_tready), \
    .debug());

// Instantiate and connect a rfnoc block streamer instance directly to the crossbar,
// but only the command and acknowledge interfaces. This interface is only useful
// for sending command packets.
//
// Usage: `RFNOC_ADD_CONFIG_PORT()
//  - name:            Instance name
//  - port_num:        Crossbar port to connect to
//
`define RFNOC_ADD_CONFIG_PORT(name, port_num) \
  localparam [15:0] sid_``name = {xbar_addr,4'd0+port_num,4'd0}; \
  rfnoc_block_streamer #(.NUM_STREAMS(1)) ``name``(.clk(bus_clk)); \
  initial begin \
    ``name``.reset(); \
    ``name``.set_src_sid(16'(sid_``name), 0); \
  end \
  assign ``name``.s_cvita_ack.axis.tdata = xbar_o_tdata[64*(port_num)+63:64*(port_num)]; \
  assign ``name``.s_cvita_ack.axis.tvalid = xbar_o_tvalid[port_num]; \
  assign ``name``.s_cvita_ack.axis.tlast = xbar_o_tlast[port_num]; \
  assign xbar_o_tready[port_num] = ``name``.s_cvita_ack.axis.tready; \
  assign xbar_i_tdata[64*(port_num)+63:64*(port_num)] = ``name``.m_cvita_cmd.axis.tdata; \
  assign xbar_i_tvalid[port_num] = ``name``.m_cvita_cmd.axis.tvalid; \
  assign xbar_i_tlast[port_num] = ``name``.m_cvita_cmd.axis.tlast; \
  assign ``name``.m_cvita_cmd.axis.tready = xbar_i_tready[port_num];

// Connecting two RFNoC blocks requires setting up flow control and
// their next destination registers.
//
// Usage: `RFNOC_CONNECT()
//  - from_noc_block_name:   Name of producer (or upstream) RFNoC block
//  - to_noc_block_name:     Name of consuming (or downstream) RFNoC block
//  - data_type:             Data type for data stream (i.e. SC16, FC32, etc)
//  - spp:                   Samples per packet
//
`define RFNOC_CONNECT(from_noc_block_name,to_noc_block_name,data_type,spp) \
  `RFNOC_CONNECT_BLOCK_PORT(from_noc_block_name,0,to_noc_block_name,0,data_type,spp);

// Setup RFNoC block flow control per block port
//
// Usage: `RFNOC_CONNECT_BLOCK_PORT()
//  - from_noc_block_name:   Name of producer (or upstream) RFNoC block
//  - from_block_port:       Block port of producer RFNoC block
//  - to_noc_block_name:     Name of consumer (or downstream) RFNoC block
//  - to_block_port:         Block port of consumer RFNoC block
//  - data_type:             Data type for data stream (i.e. SC16, FC32, etc)
//  - spp:                   Samples per packet
//
`define RFNOC_CONNECT_BLOCK_PORT(from_noc_block_name,from_block_port,to_noc_block_name,to_block_port,data_type,spp) \
  $display("Connecting %s (SID: %0d:%0d) to %s (SID: %0d:%0d)", \
           `"from_noc_block_name`",sid_``from_noc_block_name >> 4,from_block_port, \
           `"to_noc_block_name`",sid_``to_noc_block_name >> 4,to_block_port); \
   // Clear block, write be any value \
  tb_config.write_reg(sid_``from_noc_block_name, SR_CLEAR_TX_FC, 32'h1, from_block_port); \
  tb_config.write_reg(sid_``from_noc_block_name, SR_CLEAR_TX_FC, 32'h0, from_block_port); \
  tb_config.write_reg(sid_``to_noc_block_name, SR_CLEAR_RX_FC, 32'h1, to_block_port); \
  tb_config.write_reg(sid_``to_noc_block_name, SR_CLEAR_RX_FC, 32'h0, to_block_port); \
  // Set block's src_sid, next_dst_sid, resp_in_dst_sid, resp_out_dst_sid \
  // Default response dst in / out SIDs to test bench block \
  tb_config.write_reg(sid_``from_noc_block_name, SR_SRC_SID, sid_``from_noc_block_name + from_block_port, from_block_port); \
  tb_config.write_reg(sid_``to_noc_block_name,   SR_SRC_SID, sid_``to_noc_block_name + to_block_port, to_block_port); \
  tb_config.write_reg(sid_``from_noc_block_name, SR_NEXT_DST_SID, sid_``to_noc_block_name + to_block_port, from_block_port); \
  tb_config.write_reg(sid_``from_noc_block_name, SR_RESP_IN_DST_SID, sid_noc_block_tb, from_block_port); \
  tb_config.write_reg(sid_``from_noc_block_name, SR_RESP_OUT_DST_SID, sid_noc_block_tb, from_block_port); \
  tb_config.write_reg(sid_``to_noc_block_name,   SR_RESP_IN_DST_SID, sid_noc_block_tb, to_block_port); \
  tb_config.write_reg(sid_``to_noc_block_name,   SR_RESP_OUT_DST_SID, sid_noc_block_tb, to_block_port); \
  // If connection involves testbench block, set the SID and packet size. \
  if (sid_``from_noc_block_name == sid_noc_block_tb) begin \
    tb_streamer.set_dst_sid(16'(sid_``to_noc_block_name + to_block_port), from_block_port); \
    tb_streamer.set_spp(spp, from_block_port); \
    tb_streamer.set_data_type(data_type, from_block_port); \
    tb_streamer.connect_downstream(from_block_port); \
  end \
  if (sid_``to_noc_block_name == sid_noc_block_tb) begin \
    tb_streamer.connect_upstream(to_block_port); \
  end \
  // Send a flow control response packet every (receive window buffer size)/16 bytes \
  tb_config.write_reg(sid_``to_noc_block_name,   SR_FLOW_CTRL_BYTES_PER_ACK, \
     {1 /* enable consumed */, 31'(8*2**(``to_noc_block_name``.noc_shell.STR_SINK_FIFOSIZE[to_block_port*8 +: 8])/16)}, \
     to_block_port); \
  // Set up window size (in bytes) \
  tb_config.write_reg(sid_``from_noc_block_name,   SR_FLOW_CTRL_WINDOW_SIZE, \
     32'(8*2**(``to_noc_block_name``.noc_shell.STR_SINK_FIFOSIZE[to_block_port*8 +: 8])), \
     from_block_port); \
  // Set up packet limit (Unused and commented out for now) \
  // tb_config.write_reg(sid_``from_noc_block_name,   SR_FLOW_CTRL_PKT_LIMIT, 32, from_block_port); \
  // Enable source flow control output and byte based flow control, disable packet limit \
  tb_config.write_reg(sid_``from_noc_block_name,   SR_FLOW_CTRL_EN, 3'b011, from_block_port); \

`endif

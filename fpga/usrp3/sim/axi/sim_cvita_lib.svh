//
// Copyright 2015 Ettus Research LLC
//
`ifndef INCLUDED_SIM_CVITA_LIB
`define INCLUDED_SIM_CVITA_LIB

`include "sim_axis_lib.svh"

typedef logic [63:0] cvita_payload_t[$];

// CVITA packet types
typedef enum logic [1:0] {
  DATA=2'b00, FC=2'b01, CMD=2'b10, RESP=2'b11
} cvita_pkt_type_t;

// CVITA Header
typedef struct packed {
  cvita_pkt_type_t  pkt_type;
  logic             has_time;
  logic             eob;
  logic [11:0]      seqnum;
  logic [15:0]      length;
  logic [15:0]      src_sid;
  logic [15:0]      dst_sid;
  logic [63:0]      timestamp;
} cvita_hdr_t;

// CVITA Packet, Header + Payload
typedef struct {
  cvita_hdr_t     hdr;
  cvita_payload_t payload;
} cvita_pkt_t;

// CVITA packet metadata
typedef struct {
  logic         eob = 1'b0;
  logic         has_time = 1'b0;
  logic [63:0]  timestamp = 'd0;
} cvita_metadata_t;

function logic[63:0] flatten_chdr_no_ts(input cvita_hdr_t hdr);
  return {hdr.pkt_type, hdr.has_time, hdr.eob, hdr.seqnum, hdr.length, hdr.src_sid, hdr.dst_sid};
endfunction

//TODO: This should be a function but it segfaults XSIM.
task automatic unflatten_chdr_no_ts;
  input logic[63:0] hdr_bits;
  output cvita_hdr_t hdr;
  begin
    hdr = '{
      pkt_type:cvita_pkt_type_t'(hdr_bits[63:62]), has_time:hdr_bits[61], eob:hdr_bits[60],
      seqnum:hdr_bits[59:48], length:hdr_bits[47:32], src_sid:hdr_bits[31:16], dst_sid:hdr_bits[15:0], timestamp:0  //Default timestamp
    };
  end
endtask

// Extracts header from CVITA packets.
// Args:
// - pkt: CVITA packet
// - hdr: CVITA header
task automatic extract_chdr(ref logic [63:0] pkt[$], ref cvita_hdr_t hdr);
  begin
    unflatten_chdr_no_ts(pkt[0],hdr);
    if (hdr.has_time) begin
      hdr.timestamp = pkt[1];
      // Delete both header and time stamp
      pkt = pkt[2:pkt.size-1];
    end else begin
      // Delete header
      pkt = pkt[1:pkt.size-1];
    end
  end
endtask

// Drops header from CVITA packets leaving only payload data.
// Args:
// - pkt: CVITA packet
task automatic drop_chdr(ref logic [63:0] pkt[$]);
  begin
    automatic cvita_hdr_t hdr;
    extract_chdr(pkt,hdr);
  end
endtask

task automatic unflatten_chdr;
  input logic[63:0] hdr_bits;
  input logic[63:0] timestamp;
  output cvita_hdr_t hdr;
  begin
    hdr = '{
      pkt_type:cvita_pkt_type_t'(hdr_bits[63:62]), has_time:hdr_bits[61], eob:hdr_bits[60],
      seqnum:hdr_bits[59:48], length:hdr_bits[47:32], src_sid:hdr_bits[31:16], dst_sid:hdr_bits[15:0], timestamp:timestamp
    };
  end
endtask

function logic chdr_compare(input cvita_hdr_t a, input cvita_hdr_t b);
  return ((a.pkt_type == b.pkt_type) && (a.has_time == b.has_time) && (a.eob == b.eob) &&
          (a.seqnum == b.seqnum) && (a.length == b.length) && (a.src_sid == b.src_sid) && (a.dst_sid == b.dst_sid));
endfunction

typedef struct packed {
  logic [31:0]  count;
  logic [63:0]  sum;
  logic [63:0]  min;
  logic [63:0]  max;
  logic [63:0]  crc;
} cvita_stats_t;

// Interface and tasks to send CVITA packets over a master AXI-Stream bus
interface cvita_master #(parameter DWIDTH = 64, parameter NUM_STREAMS = 1)(input clk);
  axis_t #(.DWIDTH(DWIDTH), .NUM_STREAMS(NUM_STREAMS)) axis(.clk(clk));

  // Check that stream is actually in use
  function void check_stream(int stream);
    assert (stream < NUM_STREAMS) else
      $error("cvita_master::check_stream(): Tried to perform operation on unused stream %0d", stream);
  endfunction

  // Reset signals / properties used by this interface
  task automatic reset;
    begin
      axis.tvalid = 0;
      axis.tlast  = 0;
      axis.tdata  = 0;
    end
  endtask

  // Push a word onto the AXI-Stream bus and wait for it to transfer
  // Args:
  // - word: The data to push onto the bus
  // - eop: End of packet (asserts tlast) (Optional)
  // - stream: Stream to use (Optional)
  task automatic push_word (
    input logic [DWIDTH-1:0] word,
    input logic eop = 1'b0,
    input int stream = 0);
    begin
      check_stream(stream);
      if (clk) @(negedge clk);                    // Align with negative edge
      axis.tvalid[stream] = 1;
      axis.tlast[stream] = eop;
      axis.tdata[DWIDTH*stream +: DWIDTH] = word;
      @(posedge clk);                             // Put sample on data bus
      while(~axis.tready[stream]) @(posedge clk); // Wait until receiver ready
      @(negedge clk);                             // Put sample on data bus
      axis.tvalid[stream] = 0;
      axis.tlast[stream] = 0;
    end
  endtask

  // Push a bubble cycle onto the AXI-Stream bus
  // Args:
  // - stream: Stream to use (Optional)
  task automatic push_bubble (input int stream = 0);
    begin
      check_stream(stream);
      axis.tvalid[stream] = 0;
      @(negedge clk);
    end
  endtask

  // Push a CVITA header into the stream
  // Args:
  // - hdr: The header to push
  // - stream: Stream to use (Optional)
  task automatic push_hdr (
    input cvita_hdr_t hdr,
    input int stream = 0);
    push_word(flatten_chdr_no_ts(hdr), 0, stream);
  endtask

  // Push a packet with random data onto to the AXI Stream bus
  // Args:
  // - num_samps: Packet size.
  // - hdr: Header to attach to packet (length will be ignored)
  // - stream: Stream to use (Optional)
  task automatic push_rand_pkt (
    input integer num_samps,
    input cvita_hdr_t hdr,
    input int stream = 0);
    begin
      cvita_hdr_t tmp_hdr = hdr;
      tmp_hdr.length = (num_samps * 8) + (hdr.has_time ? 16 : 8);
      @(negedge clk);
      push_hdr(tmp_hdr);
      if (hdr.has_time) push_word(hdr.timestamp, 0, stream);
      repeat(num_samps-1) begin
        push_word({$random,$random}, 0, stream);
      end
      push_word({$random,$random}, 1, stream);
    end
  endtask

  // Push a packet with a ramp on to the AXI Stream bus
  // Args:
  // - num_samps: Packet size.
  // - ramp_start: Start value for the ramp
  // - ramp_inc: Increment per clock cycle
  // - hdr: Header to attach to packet (length will be ignored)
  // - stream: Stream to use (Optional)
  task automatic push_ramp_pkt (
    input integer num_samps,
    input logic [63:0] ramp_start,
    input logic [63:0] ramp_inc,
    input cvita_hdr_t hdr,
    input int stream = 0);
    begin
      automatic integer counter = 0;
      cvita_hdr_t tmp_hdr = hdr;
      tmp_hdr.length = (num_samps * 8) + (hdr.has_time ? 16 : 8);
      @(negedge clk);
      push_hdr(tmp_hdr);
      if (hdr.has_time) push_word(hdr.timestamp, 0, stream);
      repeat(num_samps-1) begin
        push_word(ramp_start+(counter*ramp_inc), 0, stream);
        counter = counter + 1;
      end
      push_word(ramp_start+(counter*ramp_inc), 1, stream);
    end
  endtask

  // Push a packet on to the AXI Stream bus.
  // Args:
  // - pkt: Packet data (queue)
  // - stream: Stream to use (Optional)
  task automatic push_pkt (
    input cvita_pkt_t pkt,
    input int stream = 0);
    begin
      // Use $ceil() to match packets with partial lines
      int pkt_size;
      // Vivado XSIM workaround
      cvita_payload_t payload = pkt.payload;
      pkt_size = (pkt.hdr.has_time ? payload.size() + 2 : payload.size() + 1);
      assert (pkt.hdr.length == 8*pkt_size || pkt.hdr.length == 8*pkt_size-4)
        else $error("cvita_master::push_pkt(): Packet size does not match packet length in header! Header: %0d, Actual: %0d",
                    pkt.hdr.length, 8*pkt_size);
      if (pkt.hdr.length == 8) begin
        push_word(pkt.hdr[127:64], 1, stream);
      end else begin
        push_word(pkt.hdr[127:64], 0, stream);
        if (pkt.hdr.has_time) begin
          push_word(pkt.hdr[63:0], (pkt.hdr.length == 16), stream);
        end
        for (int i = 0; i < pkt.payload.size()-1; i = i + 1) begin
          push_word(pkt.payload[i], 0, stream);
        end
        push_word(pkt.payload[pkt.payload.size()-1], 1, stream);
      end
    end
  endtask

endinterface


// Interface and tasks to receive CVITA packets over a slave AXI-Stream bus
interface cvita_slave #(parameter DWIDTH = 64, parameter NUM_STREAMS = 1)(input clk);
  axis_t #(.DWIDTH(DWIDTH), .NUM_STREAMS(NUM_STREAMS)) axis(.clk(clk));

  // Check that stream is actually in use
  function void check_stream(int stream);
    assert (stream < NUM_STREAMS) else
      $error("cvita_slave::check_stream(): Tried to perform operation on unused stream %0d", stream);
  endfunction

  // Reset signals / properties used by this interface
  task automatic reset;
    begin
      axis.tready = 0;
    end
  endtask

  // Accept a sample on the AXI Stream bus and
  // return the data and last
  // Args:
  // - word: The data pulled from the bus
  // - eop: End of packet (tlast)
  // - stream: Stream to use (Optional)
  task automatic pull_word (
    output logic [DWIDTH-1:0] word,
    output logic eop,
    input int stream = 0);
    begin
      check_stream(stream);
      if (clk) @(negedge clk);
      axis.tready[stream] = 1;
      while(~axis.tvalid[stream]) @(posedge clk);
      word = axis.tdata[DWIDTH*stream +: DWIDTH];
      eop = axis.tlast[stream];
      @(negedge clk);
      axis.tready[stream] = 0;
    end
  endtask

  // Wait for a sample to be transferred on the AXI Stream
  // bus and return the data and last. Note, this task only
  // observes the bus and does not affect the AXI control
  // signals.
  // Args:
  // - word: The data pulled from the bus
  // - eop: End of packet (tlast)
  // - stream: Stream to use (Optional)
  task automatic copy_word (
    output logic [DWIDTH-1:0] word,
    output logic eop,
    input int stream = 0);
    begin
      check_stream(stream);
      while(~(axis.tready[stream]&axis.tvalid[stream])) @(posedge clk);  // Wait until sample is transferred
      word = axis.tdata[DWIDTH*stream +: DWIDTH];
      eop = axis.tlast[stream];
      @(negedge clk);
    end
  endtask

  // Wait for a bubble cycle on the AXI Stream bus
  // Args:
  // - stream: Stream to use (Optional)
  task automatic wait_for_bubble (
    input int stream = 0);
    begin
      check_stream(stream);
      while(axis.tready[stream]&axis.tvalid[stream]) @(posedge clk);
      @(negedge clk);
    end
  endtask

  // Wait for a packet to finish on the bus
  // Args:
  // - stream: Stream to use (Optional)
  task automatic wait_for_pkt (
    input int stream = 0);
    begin
      check_stream(stream);
      while(~(axis.tready[stream]&axis.tvalid[stream]&axis.tlast[stream])) @(posedge clk);
      @(negedge clk);
    end
  endtask

  // Pull a packet from the AXI Stream bus.
  // Args:
  // - pkt: Packet data (queue)
  // - stream: Stream to use (Optional)
  task automatic pull_pkt (
    output cvita_pkt_t pkt,
    input int stream = 0);
    begin
      cvita_payload_t payload;  // Vivado XSIM work around
      logic [63:0] word;
      int i = 0;
      logic eop = 0;
      int pkt_size;

      while(~eop) begin
        pull_word(word, eop, stream);
        if (i == 0) begin
          pkt.hdr[127:64] = word;
        end else if ((i == 1) && pkt.hdr.has_time) begin
          pkt.hdr[63:0] = word;
        end else begin
          payload.push_back(word);
        end
        i++;
      end
      pkt.payload = payload;
      pkt_size = (pkt.hdr.has_time ? payload.size() + 2 : payload.size() + 1);
      assert (pkt.hdr.length == 8*pkt_size || pkt.hdr.length == 8*pkt_size-4)
        else $error("cvita_slave::pull_pkt(): Packet size does not match packet length in header! Header: %0d, Actual: %0d",
                    pkt.hdr.length, pkt_size*8);
    end
  endtask

  // Pull a packet from the AXI Stream bus and
  // drop it instead of passing data to the user.
  // Args:
  // - stream: Stream to use (Optional)
  task automatic drop_pkt (
    input int stream = 0);
    begin
      cvita_pkt_t pkt;
      pull_pkt(pkt, stream);
    end
  endtask

  // Vivado XSIM workaround, cannot be made into a task (with ref inputs) due to segfault
  `define WAIT_FOR_PKT_GET_INFO__UPDATE \
    stats.count = stats.count + 1; \
    stats.sum   = stats.sum + axis.tdata[DWIDTH*stream +: DWIDTH]; \
    stats.crc   = stats.crc ^ axis.tdata[DWIDTH*stream +: DWIDTH]; \
    if (axis.tdata < stats.min) stats.min = axis.tdata[DWIDTH*stream +: DWIDTH]; \
    if (axis.tdata > stats.max) stats.max = axis.tdata[DWIDTH*stream +: DWIDTH];

  // Wait for a packet to finish on the bus
  task automatic wait_for_pkt_get_info (
    output cvita_hdr_t hdr,
    output cvita_stats_t stats,
    input int stream = 0);
    begin
      automatic logic is_hdr  = 1;
      automatic logic is_time = 0;
      stats.count = 32'h0;
      stats.sum   = 64'h0;
      stats.min   = 64'h7FFFFFFFFFFFFFFF;
      stats.max   = 64'h0;
      stats.crc   = 64'h0;
      check_stream(stream);
      @(posedge clk);
      //Corner case. We are already looking at the end
      //of a packet i.e. its just a header
      if (axis.tready[stream]&axis.tvalid[stream]&axis.tlast[stream]) begin
        unflatten_chdr_no_ts(axis.tdata[DWIDTH*stream +: DWIDTH], hdr);
        @(negedge clk);
      end else begin
        while(~(axis.tready[stream]&axis.tvalid[stream]&axis.tlast[stream])) begin
          if (axis.tready[stream]&axis.tvalid[stream]) begin
            if (is_hdr) begin
              unflatten_chdr_no_ts(axis.tdata[DWIDTH*stream +: DWIDTH], hdr);
              is_time = hdr.has_time;
              is_hdr = 0;
            end else if (is_time) begin
              hdr.timestamp = axis.tdata[DWIDTH*stream +: DWIDTH];
              is_time = 0;
            end else begin
              `WAIT_FOR_PKT_GET_INFO__UPDATE
            end
          end
          @(posedge clk);
        end
        `WAIT_FOR_PKT_GET_INFO__UPDATE
        @(negedge clk);
      end
    end
  endtask

  `undef WAIT_FOR_PKT_GET_INFO__UPDATE

endinterface

`endif
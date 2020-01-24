//
// Copyright 2016 Ettus Research
//
`ifndef INCLUDED_SIM_AXIS_LIB
`define INCLUDED_SIM_AXIS_LIB

interface axis_t #(parameter DWIDTH = 32, parameter NUM_STREAMS = 1)(input clk);
  logic [NUM_STREAMS*DWIDTH-1:0] tdata;
  logic [NUM_STREAMS-1:0]        tvalid;
  logic [NUM_STREAMS-1:0]        tlast;
  logic [NUM_STREAMS-1:0]        tready;

  modport master (
    output tdata,
    output tvalid,
    output tlast,
    input tready);

  modport slave (
    input tdata,
    input tvalid,
    input tlast,
    output tready);
endinterface

// Interface to push data onto a master AXI-stream bus
interface axis_master #(parameter DWIDTH = 32, parameter NUM_STREAMS = 1)(input clk);
  axis_t #(.DWIDTH(DWIDTH), .NUM_STREAMS(NUM_STREAMS)) axis(.clk(clk));

  // Check that stream is actually in use
  function void check_stream(int stream);
    assert (stream < NUM_STREAMS) else
      $error("axis_master::check_stream(): Tried to perform operation on unused stream %0d", stream);
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
  // - eop (optional): End of packet (asserts tlast)
  // - stream: Stream to use (Optional)
  task automatic push_word (
    input logic [DWIDTH-1:0] word,
    input logic eop = 1'b0,
    input int stream = 0);
    begin
      check_stream(stream);
      if (clk) @(negedge clk);                // Align with negative edge
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

  // Push a packet with random data onto to the AXI Stream bus
  // Args:
  // - num_samps: Packet size.
  task automatic push_rand_pkt (
    input int num_samps,
    input int stream = 0);
    begin
      check_stream(stream);
      if (clk) @(negedge clk);
      repeat(num_samps-1) begin
        push_word({(((DWIDTH-1)/32)+1){$random}}, 0, stream);
      end
      push_word({(((DWIDTH-1)/32)+1){$random}}, 1, stream);
    end
  endtask

  // Push a packet with a ramp on to the AXI Stream bus
  // Args:
  // - num_samps: Packet size.
  // - ramp_start: Start value for the ramp
  // - ramp_inc: Increment per clock cycle
  // - stream: Stream to use (Optional)
  task automatic push_ramp_pkt (
    input integer num_samps,
    input [DWIDTH-1:0] ramp_start,
    input [DWIDTH-1:0] ramp_inc,
    input int stream = 0);
    begin
      automatic integer counter = 0;
      check_stream(stream);
      if (clk) @(negedge clk);
      repeat(num_samps-1) begin
        push_word(ramp_start+(counter*ramp_inc), 0, stream);
        counter = counter + 1;
      end
      push_word(ramp_start+(counter*ramp_inc), 1, stream);
    end
  endtask

endinterface


// Interface to push data onto a master AXI-stream bus
interface axis_slave #(parameter DWIDTH = 32, parameter NUM_STREAMS = 1)(input clk);
  axis_t #(.DWIDTH(DWIDTH), .NUM_STREAMS(NUM_STREAMS)) axis(.clk(clk));

  // Check that stream is actually in use
  function void check_stream(int stream);
    assert (stream < NUM_STREAMS) else
      $error("axis_slave::check_stream(): Tried to perform operation on unused stream %0d", stream);
  endfunction

  // Reset signals / properties used by this interface
  task automatic reset;
    begin
      axis.tready = 0;
    end
  endtask

  // Pull a word from the AXI Stream bus and
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

  // Drop a word on the bus
  // Args:
  // - stream:  Stream to use (Optional)
  task automatic drop_word (
    input int stream = 0);
    begin
      logic [DWIDTH-1:0] dropped_word;
      logic dropped_eop;
      pull_word(dropped_word, dropped_eop, stream);
    end
  endtask

endinterface

`endif
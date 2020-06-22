//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: PkgAxisCtrlBfm
//
// Description: Package for an AXIS-Ctrl bus functional model (BFM), which
// consists primarily of the AxisCtrlPacket and AxisCtrlBfm classes.
//



package PkgAxisCtrlBfm;

  import PkgChdrUtils::*;
  import PkgAxiStreamBfm::*;
  export PkgAxiStreamBfm::*;


  //---------------------------------------------------------------------------
  // AXIS-Ctrl Packet Class
  //---------------------------------------------------------------------------

  class AxisCtrlPacket;

    //------------
    // Properties
    //------------

    axis_ctrl_header_t header;
    chdr_timestamp_t   timestamp;
    ctrl_op_word_t     op_word;
    ctrl_word_t        data[$];


    //---------
    // Methods
    //---------

    // Return a handle to a copy of this transaction
    function AxisCtrlPacket copy();
      AxisCtrlPacket temp;
      temp = new();
      temp.header    = this.header;
      temp.timestamp = this.timestamp;
      temp.op_word   = this.op_word;
      temp.data      = this.data;
      return temp;
    endfunction

    // Return true if this packet equals that of the argument
    function bit equal(AxisCtrlPacket packet);
      if (header != packet.header) return 0;
      if (op_word != packet.op_word) return 0;
      if (data.size() != packet.data.size()) return 0;
      if (header.has_time && timestamp != packet.timestamp) return 0;
      foreach (data[i]) begin
        if (data[i] != packet.data[i]) return 0;
      end
      return 1;
    endfunction : equal

    // Format the contents of the packet into a string
    function string sprint();
      string str;
      str = {str, $sformatf("AxisCtrlPacket:\n")};
      str = {str, $sformatf("- header: %p\n", header) };
      if (header.has_time)
        str = {str, $sformatf("- timestamp: %0d\n", timestamp) };
      str = {str, $sformatf("- op_word: '{status:%s,op_code:%s,byte_enable:0b%4b,address:0x%05x}\n",
        op_word.status.name, op_word.op_code.name, op_word.byte_enable, op_word.address)};
      str = {str, $sformatf("- data:\n") };
      foreach (data[i]) begin
        str = {str, $sformatf("%5d> 0x%08x\n", i, data[i]) };
      end
      return str;
    endfunction : sprint

    // Print the contents of the packet
    function void print();
      $display(sprint());
    endfunction : print

    function void write_ctrl(
      ref axis_ctrl_header_t header,
      ref ctrl_op_word_t     op_word,
      ref ctrl_word_t        data[$],
      input chdr_timestamp_t timestamp = 0
    );
      this.header    = header;
      this.data      = data;
      this.timestamp = timestamp;
      this.op_word   = op_word;
    endfunction : write_ctrl

  endclass : AxisCtrlPacket;



  //---------------------------------------------------------------------------
  // AXIS-Ctrl Bus Functional Model Class
  //---------------------------------------------------------------------------

  class AxisCtrlBfm extends AxiStreamBfm #(32);

    extern function new (
      virtual AxiStreamIf #(32).master master,
      virtual AxiStreamIf #(32).slave  slave
    );

    extern task put_ctrl(AxisCtrlPacket ctrl_packet);

    extern task try_put_ctrl(AxisCtrlPacket ctrl_packet);

    extern task get_ctrl(output AxisCtrlPacket ctrl_packet);

    extern function bit try_get_ctrl(output AxisCtrlPacket ctrl_packet);

    extern function AxisPacket_t axis_ctrl_to_axis(AxisCtrlPacket ctrl_packet);

    extern function AxisCtrlPacket axis_to_axis_ctrl(AxisPacket_t axis_packet);

  endclass : AxisCtrlBfm



  //---------------------------------------------------------------------------
  // AXIS-Ctrl BFM Methods
  //---------------------------------------------------------------------------

  // Class constructor. This must be given an interface for the master
  // connection and an interface for the slave connection.
  function AxisCtrlBfm::new (
    virtual AxiStreamIf #(32).master master,
    virtual AxiStreamIf #(32).slave  slave
  );
    super.new(master, slave);
  endfunction : new


  // Queue the provided packet for transmission
  task AxisCtrlBfm::put_ctrl(AxisCtrlPacket ctrl_packet);
    AxisPacket_t axis_packet;
    axis_packet = axis_ctrl_to_axis(ctrl_packet);
    super.put(axis_packet);
  endtask : put_ctrl


  // Attempt to queue the provided packet for transmission. Return 1 if
  // successful, return 0 if the queue is full.
  task AxisCtrlBfm::try_put_ctrl(AxisCtrlPacket ctrl_packet);
    AxisPacket_t axis_packet;
    axis_packet = axis_ctrl_to_axis(ctrl_packet);
    super.put(axis_packet);
  endtask : try_put_ctrl

  // Get the next packet when it becomes available (waits if necessary)
  task AxisCtrlBfm::get_ctrl(output AxisCtrlPacket ctrl_packet);
    AxisPacket_t axis_packet;
    super.get(axis_packet);
    ctrl_packet = axis_to_axis_ctrl(axis_packet);
  endtask : get_ctrl


  // Get the next packet if there's one available and return 1. Return 0 if
  // there's no packet available.
  function bit AxisCtrlBfm::try_get_ctrl(output AxisCtrlPacket ctrl_packet);
    AxisPacket_t axis_packet;
    if(!super.try_get(axis_packet)) return 0;
    ctrl_packet = axis_to_axis_ctrl(axis_packet);
    return 1;
  endfunction : try_get_ctrl


  // Convert an AXIS-Ctrl packet data structure to an AXI-Stream packet data
  // structure.
  function AxisCtrlBfm::AxisPacket_t AxisCtrlBfm::axis_ctrl_to_axis(AxisCtrlPacket ctrl_packet);
    AxisPacket_t axis_packet = new();
    int index;

    // Insert words 0 and 1 (header)
    axis_packet.data.push_back(ctrl_packet.header[31: 0]);
    axis_packet.data.push_back(ctrl_packet.header[63:32]);

    // Insert timestamp if has_time is set (words 2 and 3)
    if (ctrl_packet.header.has_time) begin
      axis_packet.data.push_back(ctrl_packet.timestamp[31: 0]);
      axis_packet.data.push_back(ctrl_packet.timestamp[63:32]);
    end

    // Insert word 4 (operation word)
    axis_packet.data.push_back(ctrl_packet.op_word[31: 0]);

    // Insert data
    foreach (ctrl_packet.data[i]) begin
      axis_packet.data.push_back(ctrl_packet.data[i]);
    end

    return axis_packet;
  endfunction : axis_ctrl_to_axis


  // Convert an AXI-Stream packet data structure to an AXIS-Ctrl packet data
  // structure.
  function AxisCtrlPacket AxisCtrlBfm::axis_to_axis_ctrl(AxisPacket_t axis_packet);
    AxisCtrlPacket ctrl_packet = new();
    int i;  // Use an index instead of pop_front() to workaround a ModelSim bug

    // Grab words 0 and 1 (header)
    ctrl_packet.header[31: 0] = axis_packet.data[0];
    ctrl_packet.header[63:32] = axis_packet.data[1];

    // If has_time is set, grab timestamp (words 2 and 3)
    i = 2;
    if (ctrl_packet.header.has_time) begin
      ctrl_packet.timestamp[31: 0] = axis_packet.data[2];
      ctrl_packet.timestamp[63:32] = axis_packet.data[3];
      i += 2;
    end

    // Grab word 4 (operation word)
    ctrl_packet.op_word[31: 0] = axis_packet.data[i];

    // Grab data
    ctrl_packet.data = axis_packet.data[(i+1):$];

    return ctrl_packet;
  endfunction : axis_to_axis_ctrl


endpackage : PkgAxisCtrlBfm

//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: PkgAxiStream
//
// Description: Package for a bi-directional AXI Stream bus functional model 
// (BFM). This consists of the AxiStreamIf interface, and the 
// AxiStreamPacket and AxiStreamBfm classes.
//



//-----------------------------------------------------------------------------
// Unidirectional AXI4-Stream interface
//-----------------------------------------------------------------------------

interface AxiStreamIf #(
  parameter int DATA_WIDTH = 64,
  parameter int USER_WIDTH = 1
) (
  input logic clk,
  input logic rst = 1'b0
);
  
  // Signals that make up a unidirectional AXI-Stream interface
  logic                    tready;
  logic                    tvalid;
  logic [DATA_WIDTH-1:0]   tdata;
  logic [USER_WIDTH-1:0]   tuser;
  logic [DATA_WIDTH/8-1:0] tkeep;
  logic                    tlast;

  // View from the master side
  modport master (
    input  clk, rst,
    output tvalid, tdata, tuser, tkeep, tlast,
    input  tready
  );

  // View from the slave side
  modport slave (
    input  clk, rst,
    input  tvalid, tdata, tuser, tkeep, tlast,
    output tready
  );

endinterface : AxiStreamIf



//-----------------------------------------------------------------------------
// AXI-Stream BFM Package
//-----------------------------------------------------------------------------

package PkgAxiStreamBfm;


  //---------------------------------------------------------------------------
  // AXI Stream Packet Class
  //---------------------------------------------------------------------------

  class AxiStreamPacket #(DATA_WIDTH = 64, USER_WIDTH = 1);

    //------------------
    // Type Definitions
    //------------------

    typedef logic [DATA_WIDTH-1:0]   data_t;  // Single bus TDATA word
    typedef logic [DATA_WIDTH/8-1:0] keep_t;  // Single TKEEP word
    typedef logic [USER_WIDTH-1:0]   user_t;  // Single TUSER word

    typedef AxiStreamPacket #(DATA_WIDTH, USER_WIDTH) AxisPacket;


    //------------
    // Properties
    //------------

    data_t data[$];
    user_t user[$];
    keep_t keep[$];


    //---------
    // Methods
    //---------

    // Return a handle to a copy of this transaction
    function AxisPacket copy();
      AxisPacket temp;
      temp = new();
      temp.data = this.data;
      temp.user = this.user;
      temp.keep = this.keep;
      return temp;
    endfunction


    // Delete the contents of the current packet
    function void empty();
      data = {};
      user = {};
      keep = {};
    endfunction;


    // Return true if this packet equals that of the argument
    virtual function bit equal(AxisPacket packet);
      // These variables are needed to workaround Vivado queue support issues
      data_t data_a, data_b;
      user_t user_a, user_b;
      keep_t keep_a, keep_b;

      if (data.size() != packet.data.size()) return 0;
      foreach (data[i]) begin
        data_a = data[i];
        data_b = packet.data[i];
        if (data_a !== data_b) return 0;
      end

      if (user.size() != packet.user.size()) return 0;
      foreach (data[i]) begin
        user_a = user[i];
        user_b = packet.user[i];
        if (user_a !== user_b) return 0;
      end

      if (keep.size() != packet.keep.size()) return 0;
      foreach (keep[i]) begin
        keep_a = keep[i];
        keep_b = packet.keep[i];
        if (keep_a !== keep_b) return 0;
      end

      return 1;
    endfunction : equal


    // Format the contents of the packet into a string
    function string sprint();
      string str = "";
      if (data.size() == user.size() && data.size() == keep.size()) begin
        str = { str, "data, user, keep:\n" };
        foreach (data[i]) begin
          str = { str, $sformatf("%5d> %X %X %b\n", i, data[i], user[i], keep[i]) };
        end
      end else begin
        str = { str, "data:\n" };
        foreach (data[i]) begin
          str = { str, $sformatf("%5d> %X\n", i, data[i]) };
        end
        str = { str, "user:\n" };
        foreach (user[i]) begin
          str = { str, $sformatf("%5d> %X\n", i, user[i]) };
        end
        str = { str, "keep:\n" };
        foreach (keep[i]) begin
          str = { str, $sformatf("%5d> %X\n", i, keep[i]) };
        end
      end
      return str;
    endfunction : sprint


    // Print the contents of the packet
    function void print();
      $display(sprint());
    endfunction : print

  endclass : AxiStreamPacket;



  //---------------------------------------------------------------------------
  // AXI Stream BFM Class
  //---------------------------------------------------------------------------

  class AxiStreamBfm #(
    parameter int DATA_WIDTH = 64,
    parameter int USER_WIDTH = 1
  );

    //------------------
    // Type Definitions
    //------------------

    typedef AxiStreamPacket #(DATA_WIDTH, USER_WIDTH) AxisPacket;
    typedef AxisPacket::data_t data_t;
    typedef AxisPacket::user_t user_t;
    typedef AxisPacket::keep_t keep_t;


    //------------
    // Properties
    //------------

    // Default stall probability, as a percentage (0-100).
    local const int DEF_STALL_PROB = 38;

    // Default values to use for idle bus cycles
    local const AxisPacket::data_t IDLE_DATA = {DATA_WIDTH{1'bX}};
    local const AxisPacket::user_t IDLE_USER = {(USER_WIDTH > 1 ? USER_WIDTH : 1){1'bX}};
    local const AxisPacket::keep_t IDLE_KEEP = {(DATA_WIDTH/8){1'bX}};

    // Virtual interfaces for master and slave connections to DUT
    local virtual AxiStreamIf #(DATA_WIDTH, USER_WIDTH).master master;
    local virtual AxiStreamIf #(DATA_WIDTH, USER_WIDTH).slave  slave;
    // NOTE: We should not need these flags if Vivado would be OK with null check
    //       without throwing unnecessary null-ptr deref exceptions.
    local bit master_en;
    local bit slave_en;

    // Queues to store the bus transactions
    mailbox #(AxisPacket) tx_packets;
    mailbox #(AxisPacket) rx_packets;

    // Properties for the stall behavior of the BFM
    protected int master_stall_prob = DEF_STALL_PROB;
    protected int slave_stall_prob  = DEF_STALL_PROB;


    //---------
    // Methods
    //---------

    // Returns 1 if the packets have the same contents, otherwise returns 0.
    function bit packets_equal(AxisPacket a, AxisPacket b);
      return a.equal(b);
    endfunction : packets_equal


    // Class constructor. This must be given an interface for the master 
    // connection and an interface for the slave connection.
    function new(
      virtual AxiStreamIf #(DATA_WIDTH, USER_WIDTH).master master,
      virtual AxiStreamIf #(DATA_WIDTH, USER_WIDTH).slave  slave
    );
      this.master_en = (master != null);
      this.slave_en = (slave != null);
      this.master = master;
      this.slave  = slave;
      tx_packets = new;
      rx_packets = new;
    endfunction : new


    // Queue the provided packet for transmission
    task put(AxisPacket packet);
      assert (master_en) else $fatal(1, "Cannot use TX operations for a null master");
      tx_packets.put(packet);
    endtask : put


    // Attempt to queue the provided packet for transmission. Return 1 if 
    // successful, return 0 if the queue is full.
    function bit try_put(AxisPacket packet);
      assert (master_en) else $fatal(1, "Cannot use TX operations for a null master");
      return tx_packets.try_put(packet);
    endfunction : try_put


    // Get the next packet when it becomes available (waits if necessary)
    task get(output AxisPacket packet);
      assert (slave_en) else $fatal(1, "Cannot use RX operations for a null slave");
      rx_packets.get(packet);
    endtask : get


    // Get the next packet if there's one available and return 1. Return 0 if 
    // there's no packet available.
    function bit try_get(output AxisPacket packet);
      assert (slave_en) else $fatal(1, "Cannot use RX operations for a null slave");
      return rx_packets.try_get(packet);
    endfunction : try_get


    // Get the next packet when it becomes available (wait if necessary), but 
    // don't remove it from the receive queue.
    task peek(output AxisPacket packet);
      assert (slave_en) else $fatal(1, "Cannot use RX operations for a null slave");
      rx_packets.peek(packet);
    endtask : peek


    // Get the next packet if there's one available and return 1, but don't 
    // remove it from the receive queue. Return 0 if there's no packet 
    // available.
    function bit try_peek(output AxisPacket packet);
      assert (slave_en) else $fatal(1, "Cannot use RX operations for a null slave");
      return rx_packets.try_peek(packet);
    endfunction : try_peek


    // Return the number of packets available in the receive queue
    function int num_received();
      assert (slave_en) else $fatal(1, "Cannot use RX operations for a null slave");
      return rx_packets.num();
    endfunction


    // Wait until num packets have started transmission (i.e., until num 
    // packets have been dequeued). Set num = -1 to wait until all currently 
    // queued packets have started transmission.
    task wait_send(int num = -1);
      int end_num;
      assert (master_en) else $fatal(1, "Cannot use TX operations for a null master");

      if (num == -1) end_num = 0;
      else begin
        end_num = tx_packets.num() - num;
        assert(end_num >= 0) else begin
          $fatal(1, "Not enough packets queued to wait for %0d packets", num);
        end
      end
      while(tx_packets.num() > end_num) @(posedge master.clk);
    endtask : wait_send


    // Wait until num packets have completed transmission. Set num = -1 to wait 
    // for all currently queued packets to complete transmission.
    task wait_complete(int num = -1);
      int end_num;
      assert (master_en) else $fatal(1, "Cannot use TX operations for a null master");

      if (num == -1) num = tx_packets.num();
      else begin
        assert(num <= tx_packets.num()) else begin
          $fatal(1, "Not enough packets queued to wait for %0d packets", num);
        end
      end

      repeat (num) begin
        @(posedge master.tlast);  // Wait for last word
        do begin                  // Wait until the last word is accepted
          @(posedge master.clk);
        end while(master.tready != 1);
      end
    endtask : wait_complete


    // Set the probability (as a percentage, 0 to 100) of the master interface 
    // stalling due to lack of data to send.
    function void set_master_stall_prob(int stall_probability = DEF_STALL_PROB);
      assert(stall_probability >= 0 && stall_probability <= 100) else begin
        $fatal(1, "Invalid master stall_probability value");
      end
      master_stall_prob = stall_probability;
    endfunction


    // Set the probability (as a percentage, 0 to 100) of the slave interface 
    // stalling due to lack of buffer space.
    function void set_slave_stall_prob(int stall_probability = DEF_STALL_PROB);
      assert(stall_probability >= 0 && stall_probability <= 100) else begin
        $fatal(1, "Invalid slave stall_probability value");
      end
      slave_stall_prob = stall_probability;
    endfunction


    // Get the probability (as a percentage, 0 to 100) of the master interface 
    // stalling due to lack of data to send.
    function int get_master_stall_prob(int stall_probability = DEF_STALL_PROB);
      return master_stall_prob;
    endfunction


    // Get the probability (as a percentage, 0 to 100) of the slave interface 
    // stalling due to lack of buffer space.
    function int get_slave_stall_prob(int stall_probability = DEF_STALL_PROB);
      return slave_stall_prob;
    endfunction


    // Create separate processes for driving the master and slave interfaces
    task run();
      fork
        if (master_en) master_body();
        if (slave_en)  slave_body();
      join_none
    endtask


    //----------------
    // Master Process
    //----------------

    local task master_body();
      AxisPacket packet;

      master.tvalid <= 0;
      master.tdata  <= IDLE_DATA;
      master.tuser  <= IDLE_USER;
      master.tkeep  <= IDLE_KEEP;
      master.tlast  <= 0;

      forever begin
        @(posedge master.clk);
        if (master.rst) continue;

        if (tx_packets.try_get(packet)) begin
          foreach (packet.data[i]) begin
            // Randomly deassert tvalid for next word and stall
            if ($urandom_range(99) < master_stall_prob) begin
              master.tvalid <= 0;
              master.tdata  <= IDLE_DATA;
              master.tuser  <= IDLE_USER;
              master.tkeep  <= IDLE_KEEP;
              master.tlast  <= 0;
              do begin
                @(posedge master.clk);
                if (master.rst) break;
              end while ($urandom_range(99) < master_stall_prob);
              if (master.rst) break;
            end

            // Send the next word
            master.tvalid <= 1;
            master.tdata  <= packet.data[i];
            master.tuser  <= packet.user[i];
            master.tkeep  <= packet.keep[i];
            if (i == packet.data.size()-1) master.tlast <= 1;

            do begin
              @(posedge master.clk);
              if (master.rst) break;
            end while (!master.tready);
          end
          master.tvalid <= 0;
          master.tdata  <= IDLE_DATA;
          master.tuser  <= IDLE_USER;
          master.tkeep  <= IDLE_KEEP;
          master.tlast  <= 0;
        end
      end
    endtask : master_body


    //---------------
    // Slave Process
    //---------------

    local task slave_body();
      AxisPacket packet = new();

      slave.tready <= 0;

      forever begin
        @(posedge slave.clk);
        if (slave.rst) continue;

        if (slave.tvalid) begin
          if (slave.tready) begin
            packet.data.push_back(slave.tdata);
            packet.user.push_back(slave.tuser);
            packet.keep.push_back(slave.tkeep);
            if (slave.tlast) begin
              rx_packets.put(packet.copy());
              packet.data = {};
              packet.user = {};
              packet.keep = {};
            end
          end
          slave.tready <= $urandom_range(99) < slave_stall_prob ? 0 : 1;
        end
      end
    endtask : slave_body

  endclass : AxiStreamBfm


endpackage : PkgAxiStreamBfm

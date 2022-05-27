//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: PkgAxiStreamBfm
//
// Description: Package for a bi-directional AXI Stream bus functional model
// (BFM). This consists of the AxiStreamPacket and AxiStreamBfm classes.
// It's based on the AxiStreamIf in lib/axi4s_svPkgAXI4S.sv
//


//-----------------------------------------------------------------------------
// AXI-Stream BFM Package
//-----------------------------------------------------------------------------

package PkgAxiStreamBfm;

  //---------------------------------------------------------------------------
  // Raw packets - packet of just bytes
  //---------------------------------------------------------------------------

  // Ethernet in particular defies normal word boundaries, so underneath treating
  // it as the most fundamental quantity - Bytes.
  typedef byte raw_pkt_t[$]; // Byte Queue

  // Push a packet with random data onto to the AXI Stream bus
  // Args:
  // - num_bytes: number of random bytes to add to the raw packet.
  // - pkt: packet with rand data
  task automatic get_rand_raw_pkt (
    input int num_bytes,
    output raw_pkt_t pkt);
    begin
      repeat(num_bytes) begin
        pkt.push_back($urandom);
      end
    end
  endtask

  // Push a packet with a ramp on to the AXI Stream bus
  // Args:
  // - num_samps: Packet size in bytes *8
  // - ramp_start: Start value for the ramp
  // - ramp_inc: Increment per clock cycle
  task automatic get_ramp_raw_pkt (
    input int num_samps,
    input logic [63:0] ramp_start,
    input logic [63:0] ramp_inc,
    input int SWIDTH=64,
    output raw_pkt_t pkt);
    begin
      logic[63:0]       word;
      automatic integer counter = 0;
      repeat(num_samps) begin
        word = ramp_start+(counter*ramp_inc);
        for (int i=0; i < SWIDTH ; i+=8) begin
          pkt.push_back(word[i +: 8]);
        end
        counter = counter + 1;
      end
    end
  endtask

  // Comparison Functions
  function automatic bit raw_pkt_compare(input raw_pkt_t a, input raw_pkt_t b);

    bit queue_match;
    queue_match = 1;
    // check each element of the queue and clear queue_match if they don't match.
    // workaround for Vivado bug - could be a==b
    foreach(a[i]) queue_match = queue_match && a[i] == b[i];
    return ((a.size() == b.size()) && queue_match);

  endfunction

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

    typedef AxiStreamPacket #(DATA_WIDTH, USER_WIDTH) AxisPacket_t;

    bit verbose=0;
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
    function AxisPacket_t copy();
      AxisPacket_t temp;
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

    // Delete a word from the current packet
    function void delete(int i);
      data.delete(i);
      user.delete(i);
      keep.delete(i);
    endfunction;

    // Return true if this packet equals that of the argument
    virtual function bit equal(AxisPacket_t packet);
      // These variables are needed to workaround Vivado queue support issues
      data_t data_a, data_b;
      user_t user_a, user_b;
      keep_t keep_a, keep_b;

      if (data.size() != packet.data.size()) return 0;
      foreach (data[i]) begin
        data_a = data[i];
        data_b = packet.data[i];
        if (data_a !== data_b) begin
          if (verbose) $display("AxisPacket data mismatch a[%2d]=%X b[%2d]=%X",i,data_a,i,data_b);
          return 0;
        end
      end

      if (user.size() != packet.user.size()) return 0;
      foreach (data[i]) begin
        user_a = user[i];
        user_b = packet.user[i];
        if (user_a !== user_b) begin
          if (verbose) $display("AxisPacket user mismatch a[%2d]=%X b[%2d]=%X",i,user_a,i,user_b);
          return 0;
        end
      end

      if (keep.size() != packet.keep.size()) return 0;
      foreach (keep[i]) begin
        keep_a = keep[i];
        keep_b = packet.keep[i];
        if (keep_a !== keep_b) begin
          if (verbose) $display("AxisPacket keep mismatch a[%2d]=%X b[%2d]=%X",i,user_a,i,user_b);
          return 0;
        end
      end

      return 1;
    endfunction : equal


    // Format the contents of the packet into a string
    function string sprint();
      string str = "";
      string data_str = "";
      if (data.size() == user.size() && data.size() == keep.size()) begin
        str = { str, "data, user, keep:\n" };
        foreach (data[i]) begin
          data_str = "";
          if (DATA_WIDTH > 64) begin
            for (int b=0; b < DATA_WIDTH; b +=64) begin
              data_str = { data_str, $sformatf("%3d: %X",b,data[i][b+:64])};
              if (b+64 < DATA_WIDTH) begin
                data_str = { data_str, $sformatf("\n       ")};
              end
            end
          end else begin
            data_str = { data_str, $sformatf("%X",data[i]) };
          end
          str = { str, $sformatf("%5d> %s %X %X \n", i, data_str, user[i], keep[i]) };
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

    // Add an array of bytes (little endian)
    function void push_bytes(raw_pkt_t raw, input user_t user = '0);
      data_t word;
      keep_t my_keep;
      while (raw.size() > 0) begin
         // fill tkeep
         // SIZE = TKEEP / 0 = 0000, 1 = 0001, 2 = 0011, etc
         my_keep = '1;
         if (raw.size <= DATA_WIDTH/8) begin
            foreach (my_keep[i]) my_keep[i] = i < (raw.size);
         end
         // fill the word with raw data from bottom up
         word = '0;
         for (int i = 0; i < DATA_WIDTH/8 ; i++) begin
            if (my_keep[i]) word[i*8 +: 8] = raw.pop_front();
         end
         this.data.push_back(word);
         this.keep.push_back(my_keep);
         this.user.push_back(user);
      end
    endfunction

    // Dump data contents as an array of bytes. (little endian)
    function raw_pkt_t dump_bytes();
      data_t    word;
      keep_t    my_keep;
      raw_pkt_t raw;
      assert (data.size == keep.size) else
         $fatal(1, "data and keep have different sizes!");
      foreach (data[i]) begin
         my_keep = this.keep[i];
         word    = this.data[i];
         for (int j = 0; j < DATA_WIDTH/8 ; j++) begin
            if (my_keep[j] !== 1'b0) raw.push_back(word[j*8 +: 8]);
         end;
      end
      return raw;
    endfunction

  endclass : AxiStreamPacket;



  //---------------------------------------------------------------------------
  // AXI Stream BFM Class
  //---------------------------------------------------------------------------

  class AxiStreamBfm #(
    int DATA_WIDTH = 64,
    int USER_WIDTH = 1,
    int MAX_PACKET_BYTES = 0,
    bit TDATA = 1,
    bit TUSER = 1,
    bit TKEEP = 1,
    bit TLAST = 1
  );

    //------------------
    // Type Definitions
    //------------------

    typedef AxiStreamPacket #(DATA_WIDTH, USER_WIDTH) AxisPacket_t;
    typedef AxisPacket_t::data_t data_t;
    typedef AxisPacket_t::user_t user_t;
    typedef AxisPacket_t::keep_t keep_t;


    //------------
    // Properties
    //------------

    // Default stall probability, as a percentage (0-100).
    local const int DEF_STALL_PROB = 38;

    // Default values to use for idle bus cycles
    local const AxisPacket_t::data_t IDLE_DATA = {DATA_WIDTH{1'bX}};
    local const AxisPacket_t::user_t IDLE_USER = {(USER_WIDTH > 1 ? USER_WIDTH : 1){1'bX}};
    local const AxisPacket_t::keep_t IDLE_KEEP = {(DATA_WIDTH/8){1'bX}};

    // Virtual interfaces for master and slave connections to DUT
    local virtual AxiStreamIf #(DATA_WIDTH,USER_WIDTH,MAX_PACKET_BYTES,
                            TDATA,TUSER,TKEEP,TLAST).master master;
    local virtual AxiStreamIf #(DATA_WIDTH,USER_WIDTH,MAX_PACKET_BYTES,
                            TDATA,TUSER,TKEEP,TLAST).slave  slave;
    // NOTE: We should not need these flags if Vivado would be OK with null check
    //       without throwing unnecessary null-ptr deref exceptions.
    local bit master_en;
    local bit slave_en;

    bit slave_tready_init = 0;
    // Queues to store the bus transactions
    mailbox #(AxisPacket_t) tx_packets;
    mailbox #(AxisPacket_t) rx_packets;

    // Properties for the stall behavior of the BFM
    protected int master_stall_prob = DEF_STALL_PROB;
    protected int slave_stall_prob  = DEF_STALL_PROB;

    // Number of clocks between packets
    int inter_packet_gap = 0;

    //---------
    // Methods
    //---------

    // Returns 1 if the packets have the same contents, otherwise returns 0.
    function bit packets_equal(AxisPacket_t a, AxisPacket_t b);
      return a.equal(b);
    endfunction : packets_equal


    // Class constructor. This must be given an interface for the master
    // connection and an interface for the slave connection.
    function new(
      virtual AxiStreamIf #(DATA_WIDTH,USER_WIDTH,MAX_PACKET_BYTES,
                            TDATA,TUSER,TKEEP,TLAST).master master,
      virtual AxiStreamIf #(DATA_WIDTH,USER_WIDTH,MAX_PACKET_BYTES,
                            TDATA,TUSER,TKEEP,TLAST).slave  slave
    );
      this.master_en = (master != null);
      this.slave_en = (slave != null);
      this.master = master;
      this.slave  = slave;
      tx_packets = new;
      rx_packets = new;
    endfunction : new


    // Queue the provided packet for transmission
    task put(AxisPacket_t packet);
      assert (master_en) else $fatal(1, "Cannot use TX operations for a null master");
      tx_packets.put(packet);
    endtask : put


    // Attempt to queue the provided packet for transmission. Return 1 if
    // successful, return 0 if the queue is full.
    function bit try_put(AxisPacket_t packet);
      assert (master_en) else $fatal(1, "Cannot use TX operations for a null master");
      return tx_packets.try_put(packet);
    endfunction : try_put


    // Get the next packet when it becomes available (waits if necessary)
    task get(output AxisPacket_t packet);
      assert (slave_en) else $fatal(1, "Cannot use RX operations for a null slave");
      rx_packets.get(packet);
    endtask : get


    // Get the next packet if there's one available and return 1. Return 0 if
    // there's no packet available.
    function bit try_get(output AxisPacket_t packet);
      assert (slave_en) else $fatal(1, "Cannot use RX operations for a null slave");
      return rx_packets.try_get(packet);
    endfunction : try_get


    // Get the next packet when it becomes available (wait if necessary), but
    // don't remove it from the receive queue.
    task peek(output AxisPacket_t packet);
      assert (slave_en) else $fatal(1, "Cannot use RX operations for a null slave");
      rx_packets.peek(packet);
    endtask : peek


    // Get the next packet if there's one available and return 1, but don't
    // remove it from the receive queue. Return 0 if there's no packet
    // available.
    function bit try_peek(output AxisPacket_t packet);
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

    // Determine if the slave interface is doing a transfer this clock
    function logic slave_idle();
      return !slave.tvalid;
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
      AxisPacket_t packet;

      master.tvalid <= 0;
      master.tdata  <= IDLE_DATA;
      master.tuser  <= IDLE_USER;
      master.tkeep  <= IDLE_KEEP;
      master.tlast  <= 0;

      forever begin
        repeat(inter_packet_gap) begin
          @(posedge master.clk);
          if (master.rst) continue;
        end

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
        end else begin
          @(posedge master.clk);
          if (master.rst) continue;
        end
      end
    endtask : master_body


    //---------------
    // Slave Process
    //---------------

    local task slave_body();
      AxisPacket_t packet = new();

      slave.tready <= slave_tready_init;

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

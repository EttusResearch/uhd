//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: PkgAxiLiteBfm
//
// Description: Package for AXI4-Lite bus functional model (BFM).
// This consists of the AxiLiteTransaction and AxiLiteBfm classes.
// It's based on the AxiLiteIf interface.
//


//-----------------------------------------------------------------------------
// AXI-Lite BFM Package
//-----------------------------------------------------------------------------

package PkgAxiLiteBfm;

  import PkgAxiLite::*;
  export PkgAxiLite::*;


  typedef enum  {READ,WRITE} xtype_t;

  //---------------------------------------------------------------------------
  // AXI Lite Transaction Class
  //---------------------------------------------------------------------------

  class AxiLiteTransaction #(DATA_WIDTH = 32, ADDR_WIDTH = 32);

    //------------------
    // Type Definitions
    //------------------

    localparam BYTES_PER_WORD = DATA_WIDTH/8;

    // local type defs
    typedef logic [DATA_WIDTH-1:0]     data_t;
    typedef logic [ADDR_WIDTH-1:0]     addr_t;
    typedef logic [BYTES_PER_WORD-1:0] strb_t;

    typedef AxiLiteTransaction #(DATA_WIDTH, ADDR_WIDTH) AxiLiteTransaction_t;

    //------------
    // Properties
    //------------

    xtype_t xtype;
    data_t  data;
    addr_t  addr;
    strb_t  strb = '1;
    resp_t  resp = OKAY;
    int     id;

    //---------
    // Methods
    //---------

    // Return a handle to a copy of this transaction
    function AxiLiteTransaction_t copy();
      AxiLiteTransaction_t temp;
      temp = new();
      temp.xtype = this.xtype;
      temp.data  = this.data;
      temp.addr  = this.addr;
      temp.strb  = this.strb;
      temp.resp  = this.resp;
      temp.id    = this.id;
      return temp;
    endfunction

    // Return true if this xaction equals that of the argument
    virtual function bit equal(AxiLiteTransaction_t xaction);

      return (this.xtype === xaction.xtype &&
              this.data  === xaction.data &&
              this.addr  === xaction.addr &&
              this.strb  === xaction.strb &&
              this.resp  === xaction.resp);

    endfunction : equal

    // Format the contents of the xaction into a string
    function string sprint();
      return $sformatf("%s a=%X d=%X strb=%X resp=%s id=%d",this.xtype.name,
                       this.addr,this.data,this.strb,this.resp.name,this.id);
    endfunction : sprint

    // Print the contents of the xaction
    function void print();
      $display(sprint());
    endfunction : print

  endclass : AxiLiteTransaction;



  //---------------------------------------------------------------------------
  // AXI Lite BFM Class
  //---------------------------------------------------------------------------

  class AxiLiteBfm #(
    int DATA_WIDTH = 32,
    int ADDR_WIDTH = 32
  );

    //------------------
    // Type Definitions
    //------------------

    typedef AxiLiteTransaction #(DATA_WIDTH, ADDR_WIDTH) AxiLiteTransaction_t;
    typedef AxiLiteTransaction_t::data_t data_t;
    typedef AxiLiteTransaction_t::addr_t addr_t;
    typedef AxiLiteTransaction_t::strb_t strb_t;

    //------------
    // Properties
    //------------

    // Default stall probability, as a percentage (0-100).
    local const int DEF_STALL_PROB = 38;

    // Virtual interfaces for master and slave connections to DUT
    local virtual AxiLiteIf #(DATA_WIDTH,ADDR_WIDTH).master master;
    local virtual AxiLiteIf #(DATA_WIDTH,ADDR_WIDTH).slave  slave;

    // NOTE: We should not need these flags if Vivado would be OK with null check
    //       without throwing unnecessary null-ptr deref exceptions.
    local bit master_en;
    local bit slave_en;

    // Each submitted transaction is given an id.  The id is used when
    // waiting for the response
    local int id=0;

    // submit/completion id's are used to enforce rd/wr ordering
    local int wr_submit_id=0;
    local int wr_complete_id=0;
    local int rd_submit_id=0;
    local int rd_complete_id=0;

    // Queues to store the bus transactions
    mailbox #(AxiLiteTransaction_t) master_rd_req_xactions;
    mailbox #(AxiLiteTransaction_t) master_rd_inflight_xactions;
    mailbox #(AxiLiteTransaction_t) master_rd_resp_xactions;

    mailbox #(AxiLiteTransaction_t) master_wr_req_xactions;
    mailbox #(AxiLiteTransaction_t) master_wr_inflight_xactions;
    mailbox #(AxiLiteTransaction_t) master_wr_resp_xactions;

    mailbox #(AxiLiteTransaction_t) slave_rd_req_xactions;
    mailbox #(AxiLiteTransaction_t) slave_rd_resp_xactions;

    mailbox #(AxiLiteTransaction_t) slave_wr_req_xactions;
    mailbox #(AxiLiteTransaction_t) slave_wr_resp_xactions;

    // Properties for the stall behavior of the BFM
    int stall_prob = DEF_STALL_PROB;

    // determine if ready parks innactive
    bit ready_idle_state = 1;
    // Read write ordering
    bit rds_wait_for_wrs = 1;
    bit wrs_wait_for_rds = 1;

    // Number of clocks betwen transactions
    int inter_xaction_gap = 0;

    //---------
    // Methods
    //---------

    // Returns 1 if the xactions have the same contents, otherwise returns 0.
    function bit xactions_equal(AxiLiteTransaction_t a, AxiLiteTransaction_t b);
      return a.equal(b);
    endfunction : xactions_equal


    // Class constructor. This must be given an interface for the master
    // connection and an interface for the slave connection.
    function new(
      virtual AxiLiteIf #(DATA_WIDTH,ADDR_WIDTH).master master=null,
      virtual AxiLiteIf #(DATA_WIDTH,ADDR_WIDTH).slave  slave=null
    );
      this.master_en = (master != null);
      this.slave_en = (slave != null);
      this.master = master;
      this.slave  = slave;

      master_rd_req_xactions = new;
      master_rd_inflight_xactions = new;
      master_rd_resp_xactions = new;
      master_wr_req_xactions = new;
      master_wr_inflight_xactions = new;
      master_wr_resp_xactions = new;

      slave_rd_req_xactions = new;
      slave_rd_resp_xactions = new;
      slave_wr_req_xactions = new;
      slave_wr_resp_xactions = new;

    endfunction : new

    // Internal functions - Users should use wr/wr_block
    // This function pushes the write_addr/write_data channels
    task automatic post_wr(addr_t addr, data_t data, strb_t strb='1,
                           AxiLiteTransaction_t xaction);
      int rd_submit_id_at_start = rd_submit_id;
      xaction.xtype = WRITE;
      xaction.addr  = addr;
      xaction.data  = data;
      xaction.strb  = strb;
      xaction.id    = ++this.id;
      wr_submit_id = xaction.id;

      if (wrs_wait_for_rds) begin
        // wait for any reads that may of been submitted before
        // this write to complete before exectuing
        while (rd_complete_id-rd_submit_id_at_start < 0) begin
          @(posedge master.clk);
          if (master.rst) break;
        end
      end

      put(xaction);
    endtask : post_wr

    // Internal functions - Users should use wr/wr_block
    // This function wait for the write respone channel
    task automatic get_wr_response(AxiLiteTransaction_t xaction);
      AxiLiteTransaction_t resp_xaction = null;

      forever begin
        while(!try_peek_wr(resp_xaction)) begin
          @(posedge master.clk);
          if (master.rst) break;
        end
        if (resp_xaction.id == xaction.id) begin
          break;
        end else begin
          @(posedge master.clk);
          if (master.rst) break;
        end
      end
      get_wr(xaction);
      wr_complete_id = xaction.id;
    endtask :get_wr_response

    // Simple blocking write
    task automatic wr_block(addr_t addr, data_t data, strb_t strb='1, output resp_t resp);
      AxiLiteTransaction_t xaction = new;
      post_wr(addr,data,strb,xaction);
      get_wr_response(xaction);
      resp = xaction.resp;
    endtask : wr_block

    // Non-blocking write that checks against an expected response
    task automatic wr(addr_t addr, data_t data, strb_t strb='1,resp_t resp=OKAY);
      AxiLiteTransaction_t xaction = new;
      post_wr(addr,data,strb,xaction);
      fork
        begin
          get_wr_response(xaction);
          assert (xaction.resp == resp) else begin
            xaction.print();
            $error({"mismatch on wr response. expected:",resp.name});
          end
        end
      join_none
    endtask : wr

    // Internal functions - Users should use rd/rd_block
    // This function pushes the read address channel
    task automatic post_rd(addr_t addr,AxiLiteTransaction_t xaction);
      int wr_submit_id_at_start = wr_submit_id;
      xaction.xtype = READ;
      xaction.addr  = addr;
      xaction.id    = ++this.id;
      rd_submit_id = xaction.id;

      // wait for any writes that may have been
      // submitted before this read to complete
      if (rds_wait_for_wrs) begin
        while (wr_complete_id-wr_submit_id_at_start < 0) begin
          @(posedge master.clk);
          if (master.rst) break;
        end
      end
      put(xaction);
    endtask : post_rd

    // Internal functions - Users should use rd/rd_block
    // This function waits for the read response channel
    task automatic get_rd_response(AxiLiteTransaction_t xaction);
      AxiLiteTransaction_t resp_xaction = null;

      forever begin
        while(!try_peek_rd(resp_xaction)) begin
          @(posedge master.clk);
          if (master.rst) break;
        end
        if (resp_xaction.id == xaction.id) begin
          break;
        end else begin
          @(posedge master.clk);
          if (master.rst) break;
        end
      end
      get_rd(xaction);
      rd_complete_id  = xaction.id;
    endtask : get_rd_response

    // Simple blocking read
    task automatic rd_block(addr_t addr, output data_t data, output resp_t resp);
      AxiLiteTransaction_t xaction = new;
      post_rd(addr,xaction);
      get_rd_response(xaction);
      data = xaction.data;
      resp = xaction.resp;
    endtask : rd_block

    // Non-blocking read that checks against an expected data response
    task automatic rd(addr_t addr, data_t data, resp_t resp=OKAY);
      AxiLiteTransaction_t xaction = new;
      post_rd(addr,xaction);
      fork
        begin
          get_rd_response(xaction);
          assert (xaction.resp == resp) else begin
            xaction.print();
            $error({"mismatch on rd response. expected:",resp.name});
          end
          assert (xaction.data === data) else begin
            xaction.print();
            $error({"mismatch on rd data. expected:",$sformatf("%x",data)});
          end
        end
      join_none
    endtask : rd

    task automatic block();
      int rd_submit_id_at_start = rd_submit_id;
      int wr_submit_id_at_start = wr_submit_id;
      while (rd_complete_id-rd_submit_id_at_start < 0 ||
             wr_complete_id-wr_submit_id_at_start < 0) begin
        @(posedge master.clk);
        if (master.rst) break;
      end
    endtask : block

    // Queue the provided xaction for transmission
    task put(AxiLiteTransaction_t xaction);
      if (xaction.xtype == READ && master_en) begin
        master_rd_req_xactions.put(xaction);
      end else if (xaction.xtype == WRITE && master_en) begin
        master_wr_req_xactions.put(xaction);
      end else if (xaction.xtype == READ && slave_en) begin
        slave_rd_resp_xactions.put(xaction);
      end else if (xaction.xtype == WRITE && slave_en) begin
        slave_wr_resp_xactions.put(xaction);
      end
    endtask : put

    // Attempt to queue the provided xaction for transmission. Return 1 if
    // successful, return 0 if the queue is full.
    function bit try_put(AxiLiteTransaction_t xaction);
      if (xaction.xtype == READ && master_en) begin
        return master_rd_req_xactions.try_put(xaction);
      end else if (xaction.xtype == WRITE && master_en) begin
        return master_wr_req_xactions.try_put(xaction);
      end else if (xaction.xtype == READ && slave_en) begin
        return slave_rd_resp_xactions.try_put(xaction);
      end else if (xaction.xtype == WRITE && slave_en) begin
        return slave_wr_resp_xactions.try_put(xaction);
      end
    endfunction : try_put

    // Get the next rd_xaction when it becomes available (waits if necessary)
    task get_rd(output AxiLiteTransaction_t xaction);
      if (master_en) begin
        master_rd_resp_xactions.get(xaction);
      end else if (slave_en) begin
        slave_rd_req_xactions.get(xaction);
      end
    endtask : get_rd

    // Get the next wr_xaction when it becomes available (waits if necessary)
    task get_wr(output AxiLiteTransaction_t xaction);
      if (master_en) begin
        master_wr_resp_xactions.get(xaction);
      end else if (slave_en) begin
        slave_wr_req_xactions.get(xaction);
      end
    endtask : get_wr

    // Get the next rd_xaction if there's one available and return 1. Return 0 if
    // there's no xaction available.
    function bit try_get_rd(output AxiLiteTransaction_t xaction);
      if (master_en) begin
        return master_rd_resp_xactions.try_get(xaction);
      end else if (slave_en) begin
        return slave_rd_req_xactions.try_get(xaction);
      end
    endfunction : try_get_rd

    // Get the next wr_xaction if there's one available and return 1. Return 0 if
    // there's no xaction available.
    function bit try_get_wr(output AxiLiteTransaction_t xaction);
      if (master_en) begin
        return master_wr_resp_xactions.try_get(xaction);
      end else if (slave_en) begin
        return slave_wr_req_xactions.try_get(xaction);
      end
    endfunction : try_get_wr

    // Get the next wr xaction when it becomes available (wait if necessary), but
    // don't remove it from the receive queue.
    task peek_rd(output AxiLiteTransaction_t xaction);
      if (master_en) begin
        master_rd_resp_xactions.peek(xaction);
      end else if (slave_en) begin
        slave_rd_req_xactions.peek(xaction);
      end
    endtask : peek_rd

    // Get the next wr xaction when it becomes available (wait if necessary), but
    // don't remove it from the receive queue.
    task peek_wr(output AxiLiteTransaction_t xaction);
      if (master_en) begin
        master_wr_resp_xactions.peek(xaction);
      end else if (slave_en) begin
        slave_wr_req_xactions.peek(xaction);
      end
    endtask : peek_wr

    // Get the next rd xaction if there's one available and return 1, but don't
    // remove it from the receive queue. Return 0 if there's no xaction
    // available.
    function bit try_peek_rd(output AxiLiteTransaction_t xaction);
      if (master_en) begin
        return master_rd_resp_xactions.try_peek(xaction);
      end else if (slave_en) begin
        return slave_rd_req_xactions.try_peek(xaction);
      end
    endfunction : try_peek_rd

    // Get the next wr xaction if there's one available and return 1, but don't
    // remove it from the receive queue. Return 0 if there's no xaction
    // available.
    function bit try_peek_wr(output AxiLiteTransaction_t xaction);
      if (master_en) begin
        return master_wr_resp_xactions.try_peek(xaction);
      end else if (slave_en) begin
        return slave_wr_req_xactions.try_peek(xaction);
      end
    endfunction : try_peek_wr


    // Return the number of xactions available in the receive queue
    function int num_rd_pending();
      if (master_en) begin
        return master_rd_resp_xactions.num() +
               master_rd_inflight_xactions.num() +
               master_rd_req_xactions.num();
      end else if (slave_en) begin
        $fatal(1,"Slave AxiLite not yet implemented!");
      end
    endfunction

    function int num_wr_pending();
      if (master_en) begin
        return master_wr_resp_xactions.num() +
               master_wr_inflight_xactions.num() +
               master_wr_req_xactions.num();
      end else if (slave_en) begin
        $fatal(1,"Slave AxiLite not yet implemented!");
      end
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

    // wait my_delay clocks - while not reset
    local task master_wait(my_delay);
      repeat(my_delay) begin
        @(posedge master.clk);
        if (master.rst) break;
      end
    endtask

    // stall for a random delay - while not reset
    local task master_stall(int my_stall_prob);
      if ($urandom_range(99) < my_stall_prob) begin
        do begin
          @(posedge master.clk);
          if (master.rst) break;
        end while ($urandom_range(99) < my_stall_prob);
      end
    endtask

    local task master_body();

      // start idle
      master.drive_w_idle();
      master.drive_aw_idle();
      master.bready = ready_idle_state;

      master.drive_read_idle();
      master.rready = ready_idle_state;

      fork // write_req / write_resp / rd_req / rd_resp
        //--------------------------------------
        begin : write_req_thread
          AxiLiteTransaction_t xaction;

          forever begin
            // add inter xaction gap
            master_wait(inter_xaction_gap);

            if (master_wr_req_xactions.try_get(xaction)) begin : req_transaction

              //drive the write address+data channel
              fork
                begin
                  // randomly delay driving
                  master_stall(stall_prob);
                  master.drive_aw(xaction.addr);
                  // wait for ready
                  do begin
                    @(posedge master.clk);
                    if (master.rst) break;
                  end while (!master.awready);
                  master.drive_aw_idle();
                end
                begin // wait for data ready
                  // randomly delay driving
                  master_stall(stall_prob);
                  master.drive_w(xaction.data,xaction.strb);

                  // push to inflight xactions
                  master_wr_inflight_xactions.put(xaction);

                  // wait for ready
                  do begin
                    @(posedge master.clk);
                    if (master.rst) break;
                  end while (!master.wready);
                  master.drive_w_idle();
                end
              join


            end else begin : no_req_transaction
              @(posedge master.clk);
              if (master.rst) continue;
            end : no_req_transaction;
          end //forever
        end : write_req_thread;

        //--------------------------------------
        begin : write_resp_thread
          AxiLiteTransaction_t xaction;

          forever begin

            if (master_wr_inflight_xactions.try_get(xaction)) begin : resp_transaction
              // wait for a little bit of time and assert ready
              if ($urandom_range(99) < 50) begin
                if (master.bready == 0) begin
                  // randomly delay ready
                  master_stall(stall_prob);
                  master.bready = 1;
                end
                master.bready = 1;
                // wait for valid
                do begin
                  @(posedge master.clk);
                  if (master.rst) break;
                end while (!master.bvalid);
              //wait for valid then wait a bit of time and assert ready
              end else begin
                // wait for valid
                do begin
                  @(posedge master.clk);
                  if (master.rst) break;
                end while (!master.bvalid);

                if (master.bready == 0) begin
                  // randomly delay ready while valid is true
                  master_stall(stall_prob);
                  master.bready = 1;
                  @(posedge master.clk);
                end
              end


              // put the response and clear ready
              master.bready = 0;
              xaction.resp = master.bresp;
              master_wr_resp_xactions.put(xaction);
              // randomly keep ready low for awhile
              master_stall(stall_prob);
              master.bready = ready_idle_state;

            end else begin : no_resp_transaction
              @(posedge master.clk);
              if (master.rst) continue;
            end

          end // forever
        end : write_resp_thread;

        begin : read_req_thread
          AxiLiteTransaction_t xaction;

          forever begin
            // add inter xaction gap
            master_wait(inter_xaction_gap);

            if (master_rd_req_xactions.try_get(xaction)) begin : req_transaction
              // randomly delay driving
              master_stall(stall_prob);

              //drive the read address channel
              master.drive_read(xaction.addr);
              // push to in-flight xactions
              master_rd_inflight_xactions.put(xaction);

              // wait for read address channel ready
              do begin
                @(posedge master.clk);
                if (master.rst) break;
              end while (!master.arready);
              master.drive_read_idle();


            end else begin : no_req_transaction
              @(posedge master.clk);
              if (master.rst) continue;
            end
          end // forever
        end : read_req_thread;

        //--------------------------------------
        begin : read_resp_thread
          AxiLiteTransaction_t xaction;

          forever begin

            if (master_rd_inflight_xactions.try_get(xaction)) begin : resp_transaction
              if ($urandom_range(99) < 50) begin
                if (master.rready == 0) begin
                  // randomly delay ready
                  master_stall(stall_prob);
                  master.rready = 1;
                end
                master.rready = 1;
                // wait for valid
                do begin
                  @(posedge master.clk);
                  if (master.rst) break;
                end while (!master.rvalid);
              // wait for valid then wait a bit of time and assert ready
              end else begin
                // wait for valid
                do begin
                  @(posedge master.clk);
                  if (master.rst) break;
                end while (!master.rvalid);

                if (master.rready == 0) begin
                  // randomly delay ready while valid is true
                  master_stall(stall_prob);
                  master.rready = 1;
                  @(posedge master.clk);
                end
              end

              // put the response and clear ready
              master.rready = 0;
              xaction.data = master.rdata;
              xaction.resp = master.rresp;
              master_rd_resp_xactions.put(xaction);
              // randomly keep ready low for awhile
              master_stall(stall_prob);
              master.rready = ready_idle_state;

            end else begin : no_resp_transaction
              @(posedge master.clk);
              if (master.rst) continue;
            end

          end // forever
        end : read_resp_thread;

      join_none
    endtask : master_body


    //---------------
    // Slave Process
    //---------------

    local task slave_body();
      AxiLiteTransaction_t xaction = new();

      // start idle
      slave.drive_write_resp_idle();
      slave.awready = 0;
      slave.wready = 0;

      slave.drive_read_resp_idle();
      slave.arready = 0;

      forever begin
        @(posedge slave.clk);
        if (slave.rst) continue;

        // not written!
        $fatal(1,"Slave AxiLite not yet implemented!");

      end
    endtask : slave_body

  endclass : AxiLiteBfm


endpackage : PkgAxiLiteBfm

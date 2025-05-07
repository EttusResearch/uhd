//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Package: ctrlport_bfm_pkg
//
// Description:
//
//   Package defining a Control Port bus functional model (BFM).
//   See the RFNoC Specification for a protocol description.
//

package ctrlport_bfm_pkg;

  import ctrlport_pkg::*;

  class ctrlport_bfm;

    // create combined request and response struct for mailbox
    typedef struct {
      ctrlport_request_t req;
      ctrlport_response_t resp;
      bit skip_data_check;
    } ctrlport_transfer_t;

    local mailbox #(ctrlport_transfer_t) transactions;

    // link to interface
    local virtual ctrlport_if iface;

    // data copy for response data
    local logic [CTRLPORT_DATA_W-1:0] reponse_data;

    // ------------------------------------------------------------------
    // Constructor
    // ------------------------------------------------------------------
    // Class constructor. This must be given an interface for the master
    // connection and an interface for the slave connection.
    function new(virtual ctrlport_if iface);
      this.iface = iface;
      transactions = new;
    endfunction

    // ------------------------------------------------------------------
    // Asynchronous transaction handling
    // ------------------------------------------------------------------
    // Queue an asynchronous transaction (read or write with full request)
    task async_request(
      ctrlport_request_t request,
      ctrlport_response_t expected_response,
      bit skip_data_check = '0
    );
      assert (request.rd || request.wr) else
        $error("Transaction must be read or write");
      transactions.put('{request, expected_response, skip_data_check});
    endtask

    // Queue a simple write transaction
    task async_write(
      logic[CTRLPORT_ADDR_W-1:0] addr,
      logic[CTRLPORT_DATA_W-1:0] data,
      ctrlport_status_t expected_status = STS_OKAY
    );
      ctrlport_request_t request;
      ctrlport_response_t response;

      request = '0;
      request.wr = '1;
      request.addr = addr;
      request.data = data;

      response = '0;
      response.status = expected_status;

      async_request(request, response);
    endtask;

    // Queue a simple read transaction with automated response checking
    task async_read(
      logic [CTRLPORT_ADDR_W-1:0] addr,
      logic [CTRLPORT_DATA_W-1:0] expected_data,
      ctrlport_status_t expected_status = STS_OKAY
    );
      ctrlport_request_t request;
      ctrlport_response_t response;

      request = '0;
      request.rd = '1;
      request.addr = addr;

      response = '0;
      response.status = expected_status;
      response.data = expected_data;

      async_request(request, response);
    endtask;

    // wait for all transactions to be processed
    task wait_complete();
      while(transactions.num() > 0) begin
        @(posedge iface.clk);
      end
    endtask

    // ------------------------------------------------------------------
    // Synchronous transaction handling
    // ------------------------------------------------------------------
    // Perform a request and get response
    task request(
      input ctrlport_request_t request,
      input ctrlport_response_t expected_response,
      output logic[CTRLPORT_DATA_W-1:0] data
    );
      async_request(request, expected_response);
      wait_complete();
      data = reponse_data;
    endtask;

    // Perform a simple write transaction
    task write(
      logic[CTRLPORT_ADDR_W-1:0] addr,
      logic[CTRLPORT_DATA_W-1:0] data,
      ctrlport_status_t expected_status = STS_OKAY
    );
      async_write(addr, data, expected_status);
      wait_complete();
    endtask;

    // Perform a simple read transaction, no response data check, return data from interface
    task read(
      input logic[CTRLPORT_ADDR_W-1:0] addr,
      output logic[CTRLPORT_DATA_W-1:0] data,
      input ctrlport_status_t expected_status = STS_OKAY
    );
      ctrlport_request_t request;
      ctrlport_response_t response;

      request = '0;
      request.rd = '1;
      request.addr = addr;

      response = '0;
      response.status = expected_status;

      async_request(request, response, '1);
      wait_complete();
      data = reponse_data;
    endtask;


    // ------------------------------------------------------------------
    // Signal handling
    // ------------------------------------------------------------------
    // Start the BFM.
    // This will join a separate thread for the interface handling.
    task run();
      fork
        request_body();
      join_none
    endtask

    // Control the master request interface
    local task request_body();
      ctrlport_transfer_t transfer;
      ctrlport_request_t request_idle;

      // initialize request interface
      request_idle = 'x;
      request_idle.wr = '0;
      request_idle.rd = '0;
      iface.req <= request_idle;

      // ignore first clock cycle to let response settle
      @(posedge iface.clk);

      forever begin
        // handle any available transaction (but leave it in the mailbox for now)
        if (transactions.try_peek(transfer)) begin
          // wait for reset to be deasserted
          while(iface.rst) begin
            @(posedge iface.clk);
            check_no_response();
          end

          // send request
          iface.req <= transfer.req;
          // check minimum time for response on interface
          @(negedge iface.clk);
          check_no_response();

          // reset request interface
          @(posedge iface.clk);
          iface.req <= request_idle;

          // wait for response
          while(!iface.resp.ack) begin
            @(posedge iface.clk);
          end

          // status is always expected to match
          assert (iface.resp.status == transfer.resp.status) else begin
            $error("Unexpected status received on interface. Expected: 0b%2b Received: 0b%2b",
              transfer.resp.status, iface.resp.status);
          end

          // check data if read and data check is not skipped
          if (transfer.req.rd && !transfer.skip_data_check) begin
            assert (iface.resp.data == transfer.resp.data) else begin
              $error("Unexpected data received on interface. Expected: 0x%h Received: 0x%h",
                transfer.resp.data, iface.resp.data);
            end
          end

          // save data for read operations
          reponse_data = iface.resp.data;

          // request is done -> remove from queue
          transactions.get(transfer);

        // wait at least one clock cycle for next check of mailbox
        end else begin
          @(posedge iface.clk);
          check_no_response();
        end
      end
    endtask

    // Check for no response in this clock cycle
    local task check_no_response();
      if (iface.resp.ack != '0) begin
        $error("Unexpected response received on interface: %p", iface.resp);
      end
    endtask

  endclass

endpackage

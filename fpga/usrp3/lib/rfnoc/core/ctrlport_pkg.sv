//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Package: ctrlport_pkg
//
// Description:
//
//   Defines constants for the control-port interface.
//

package ctrlport_pkg;

  `include "ctrlport.vh"

  // typedef for status
  typedef enum logic [1:0] {
    STS_OKAY    = CTRL_STS_OKAY,
    STS_CMDERR  = CTRL_STS_CMDERR,
    STS_TSERR   = CTRL_STS_TSERR,
    STS_WARNING = CTRL_STS_WARNING } ctrlport_status_t;

  // Defining signals as defined in Table 18 of RFNoC Specification v.1.0.1.
  // request related signals
  typedef struct packed {
    logic                           wr;
    logic                           rd;
    logic [    CTRLPORT_ADDR_W-1:0] addr;
    logic [  CTRLPORT_PORTID_W-1:0] port_id;
    logic [CTRLPORT_REM_EPID_W-1:0] remote_epid;
    logic [  CTRLPORT_PORTID_W-1:0] remote_portid;
    logic [    CTRLPORT_DATA_W-1:0] data;
    logic [ CTRLPORT_BYTE_EN_W-1:0] byte_en;
    logic                           has_time;
    // signal time - renamed to timestamp to avoid conflict with SystemVerilog keyword time
    logic [    CTRLPORT_TIME_W-1:0] timestamp;
  } ctrlport_request_t;

  // response related signals
  typedef struct packed {
    logic                       ack;
    ctrlport_status_t           status;
    logic [CTRLPORT_DATA_W-1:0] data;
  } ctrlport_response_t;

endpackage : ctrlport_pkg

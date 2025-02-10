//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_if_window
//
// Description:
//
//   Copy requests from slave to master interface when s_ctrlport_req_addr is in
//   address range specified by the parameters.
//
// Parameters:
//
//  BASE_ADDRESS : Base address of the window.
//                 This will be the first address in the range.
//  WINDOW_SIZE  : Size of the address space for the window.
//                 Last address in range will be BASE_ADDRESS + WINDOW_SIZE - 1.
//

module ctrlport_if_window #(
  int BASE_ADDRESS = 0,
  int WINDOW_SIZE  = 32
) (
  ctrlport_if.slave s_ctrlport,
  ctrlport_if.master m_ctrlport
);

  always_comb begin
    // Mask write and read flag
    automatic logic address_in_range = (s_ctrlport.req.addr >= BASE_ADDRESS) &&
      (s_ctrlport.req.addr < BASE_ADDRESS + WINDOW_SIZE);

    // initialize master request by slave request
    m_ctrlport.req = s_ctrlport.req;

    // overwrite write and read flag with in range flag
    m_ctrlport.req.wr = s_ctrlport.req.wr && address_in_range;
    m_ctrlport.req.rd = s_ctrlport.req.rd && address_in_range;

    // transfer reponse without change
    s_ctrlport.resp = m_ctrlport.resp;
  end

endmodule

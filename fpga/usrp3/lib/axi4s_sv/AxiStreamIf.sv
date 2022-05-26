//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Interface: AxiStreamIf
// Description:
//  AXI4-Stream is an ARM standard for representing streams or packets in
//  a design. For more information on the spec see:
//  https://static.docs.arm.com/ihi0051/a/IHI0051A_amba4_axi4_stream_v1_0_protocol_spec.pdf
//
//  The interface contains methods for
//  (1) Monitoring progress of a packet
//  (2) Extracting fields from a packet
//  (3) Overwriting fields in a packet
//
// Parameters:
//  - DATA_WIDTH - Width of tdata on AXI4S bus
//  - USER_WIDTH - Width of tuser on AXI4S bus
//  - MAX_PACKET_BYTES - Maximum number of bytes between tlast. If 0, no
//                       word_count resources are added.
//  - TDATA - use tData if 1
//  - TUSER - use tUser if 1
//  - TKEEP - use tKeep if 1
//  - TLAST - use tLast if 1
//
// Some Historic USRP code uses tuser without tkeep frequently.
// The usage of tuser varies, here are some examples:
// (1) Instead of using tkeep, user can encode number of valid bytes in a word
// (2) The MSB can indicate an FCS failure from the MAC
// (3) Header information is passed from xport to chdr system
//

//-----------------------------------------------------------------------------
// Unidirectional AXI4-Stream interface
//-----------------------------------------------------------------------------

// Minimal - Interface Capable of continuous assignment
interface AxiStreamIf #(
  int DATA_WIDTH = 64,

  int USER_WIDTH = 1,
  int MAX_PACKET_BYTES = 0,
  bit TDATA = 1,
  bit TUSER = 1,
  bit TKEEP = 1,
  bit TLAST = 1
) (
  input logic clk,
  input logic rst = 1'b0
);

// We don't want to check this, but this is an example of how to
// check an input parameter
//  initial begin
//    assert (DATA_WIDTH % 8 == 0) else begin
//      $display("DATA_WIDTH == %1d",DATA_WIDTH);
//    $fatal(1,"Invalid data width on AxiStreamIf");
//    end
//  end

  localparam BYTES_PER_WORD = DATA_WIDTH/8;
  localparam DWIDTH = TDATA ? DATA_WIDTH : 0;
  localparam UWIDTH = TUSER ? USER_WIDTH : 0;
  localparam KWIDTH = TKEEP ? DATA_WIDTH/8 : 0;
  localparam LWIDTH = TLAST ? 1 : 0;
  localparam PACKED_WIDTH = DWIDTH + UWIDTH + KWIDTH + LWIDTH ;

  // local type defs
  typedef logic [DATA_WIDTH-1:0]   data_t;
  typedef logic [USER_WIDTH-1:0]   user_t;
  typedef logic [DATA_WIDTH/8-1:0] keep_t;

  // Signals that make up a unidirectional AXI-Stream interface
  logic  tlast;
  logic  tvalid;
  logic  tready;
  data_t tdata;
  user_t tuser;
  keep_t tkeep;

  //---------------------------------------
  // Trailing Bytes/Keep functions
  // Trailing byte of 0 means full word else trailing bytes is the
  // number of bytes in the last word.
  //---------------------------------------
  // bits needed to represent trailing bytes
  localparam TRAILING_WIDTH = $clog2(DATA_WIDTH/8);
  typedef logic [TRAILING_WIDTH-1:0] trailing_bytes_t;

  // translate between bytes and tkeep
  function automatic trailing_bytes_t keep2trailing(keep_t keep);
    trailing_bytes_t bytes = '0;
    if (tlast == 1) begin
      // mux between values based on the high bit = 1
      for(int b = 0; b < DATA_WIDTH/8 ; b++) begin
        if (keep[b]) bytes = b+1;
      end
    end
    return bytes;
  endfunction : keep2trailing

  function automatic keep_t trailing2keep(trailing_bytes_t bytes);
    keep_t keep = '1;
    if (tlast == 1 && bytes != 0) begin
      foreach(keep[b]) begin
        keep[b] = bytes > b;
      end
    end
    return keep;
  endfunction : trailing2keep

  // View from the master side
  modport master (
    input  clk, rst,
    output tvalid, tdata, tuser, tkeep, tlast,
    input  tready,
    import keep2trailing,
    import trailing2keep
  );

  // View from the slave side
  modport slave (
    input  clk, rst,
    input  tvalid, tdata, tuser, tkeep, tlast,
    output tready,
    import keep2trailing,
    import trailing2keep
  );


endinterface : AxiStreamIf

// Full featured for packet modification.  Must use procedural assignment.
interface AxiStreamPacketIf #(
  int DATA_WIDTH = 64,
  int USER_WIDTH = 1,
  int MAX_PACKET_BYTES = 0,
  bit TDATA = 1,
  bit TUSER = 1,
  bit TKEEP = 1,
  bit TLAST = 1
) (
  input logic clk,
  input logic rst = 1'b0
);

// We don't want to check this, but this is an example of how to
// check an input parameter:
//  initial begin
//    assert (DATA_WIDTH % 8 == 0) else begin
//      $display("DATA_WIDTH == %1d",DATA_WIDTH);
//    $fatal(1,"Invalid data width on AxiStreamIf");
//    end
//  end

  localparam BYTES_PER_WORD = DATA_WIDTH/8;
  localparam MAX_PACKET_WORDS = MAX_PACKET_BYTES/BYTES_PER_WORD;
  // local type defs
  typedef logic [DATA_WIDTH-1:0]   data_t;
  typedef logic [USER_WIDTH-1:0]   user_t;
  typedef logic [DATA_WIDTH/8-1:0] keep_t;
  typedef logic [$clog2(MAX_PACKET_WORDS)-1:0] word_count_t;

  // Signals that make up a unidirectional AXI-Stream interface
  logic  tlast;
  logic  tvalid;
  logic  tready;
  data_t tdata;
  user_t tuser;
  keep_t tkeep;
  word_count_t word_count = 0;

  if (MAX_PACKET_BYTES>0) begin
    always_ff @(posedge clk) begin
      if (rst) begin
        word_count    <= 0;
      end else begin
        // reset at last
        if (tlast && tvalid && tready) begin
          word_count    <= 0;
        // increment when valid and ready
        end else if (tvalid && tready)begin
          word_count    <= word_count+1;
        end
      end
    end
  end

  //---------------------------------------
  // Packing functions
  //---------------------------------------

  localparam DWIDTH = TDATA ? DATA_WIDTH : 0;
  localparam UWIDTH = TUSER ? USER_WIDTH : 0;
  localparam KWIDTH = TKEEP ? DATA_WIDTH/8 : 0;
  localparam LWIDTH = TLAST ? 1 : 0;
  localparam PACKED_WIDTH = DWIDTH + UWIDTH + KWIDTH + LWIDTH ;
  typedef logic [PACKED_WIDTH-1:0] packed_t;

  function automatic packed_t pack(INC_DATA=1,INC_USER=1,INC_KEEP=1,INC_LAST=1);
    logic [PACKED_WIDTH-1:0] data = 'X;

    int USTART;
    int KSTART;
    int LSTART;

    USTART = INC_DATA ? DWIDTH :0;
    KSTART = INC_USER ? USTART+UWIDTH :USTART;
    LSTART = INC_KEEP ? KSTART+KWIDTH :KSTART;

    if (TDATA && INC_DATA) begin
      // in the LSB
      data[0+:DATA_WIDTH] = tdata;
    end
    if (TUSER && INC_USER) begin
      // in the 1st MIDDLE
      data[USTART+:USER_WIDTH] = tuser;
    end
    if (TKEEP && INC_KEEP) begin
      // in the 2nd MIDDLE
      data[KSTART+:DATA_WIDTH/8] = tkeep;
    end
    if (TLAST && INC_LAST) begin
      // in the MSB
      data[LSTART] = tlast;
    end
    return data;
  endfunction : pack

  task automatic unpack (packed_t data,INC_DATA=1,INC_USER=1,INC_KEEP=1,INC_LAST=1);

    int USTART;
    int KSTART;
    int LSTART;

    USTART = INC_DATA ? DWIDTH :0;
    KSTART = INC_USER ? USTART+UWIDTH :USTART;
    LSTART = INC_KEEP ? KSTART+KWIDTH :KSTART;

    if (TDATA && INC_DATA) begin
      // in the LSB
      tdata = data[0+:DATA_WIDTH];
    end
    if (TUSER && INC_USER) begin
      // in the 1st MIDDLE
      tuser = data[USTART+:USER_WIDTH];
    end
    if (TKEEP && INC_KEEP) begin
      // in the 2nd MIDDLE
      tkeep = data[KSTART+:DATA_WIDTH/8];
    end
    if (TLAST && INC_LAST) begin
      // in the MSB
      tlast = data[LSTART];
    end
  endtask : unpack

  //---------------------------------------
  // Trailing Bytes/Keep functions
  // Trailing byte of 0 means full word else trailing bytes is the
  // number of bytes in the last word.
  //---------------------------------------
  // bits needed to represent trailing bytes
  localparam TRAILING_WIDTH = $clog2(DATA_WIDTH/8);
  typedef logic [TRAILING_WIDTH-1:0] trailing_bytes_t;

  // translate between bytes and tkeep
  function automatic trailing_bytes_t keep2trailing(keep_t keep);
    trailing_bytes_t bytes = '0;
    if (tlast == 1) begin
      // mux between values based on the high bit = 1
      for(int b = 0; b < DATA_WIDTH/8 ; b++) begin
        if (keep[b]) bytes = b+1;
      end
    end
    return bytes;
  endfunction : keep2trailing

  function automatic keep_t trailing2keep(trailing_bytes_t bytes);
    keep_t keep = '1;
    if (tlast == 1 && bytes != 0) begin
      foreach(keep[b]) begin
        keep[b] = bytes > b;
      end
    end
    return keep;
  endfunction : trailing2keep

  // do the translation directly to/from the ifc
  function automatic keep_t get_trailing_bytes();
    localparam USER_TRAILING_WIDTH =
      USER_WIDTH >= TRAILING_WIDTH ? TRAILING_WIDTH : USER_WIDTH;
    assert (TUSER) else
      $fatal(1, "Can't get trailing if TUSER doesn't exist");
    assert (USER_WIDTH >= TRAILING_WIDTH) else
      $fatal(1, "USER_WIDTH is to narrow to contain trailing");
    return trailing2keep(tuser[USER_TRAILING_WIDTH-1:0]);
  endfunction : get_trailing_bytes

  task automatic set_trailing_bytes(keep_t keep);
    localparam USER_TRAILING_WIDTH =
      USER_WIDTH >= TRAILING_WIDTH ? TRAILING_WIDTH : USER_WIDTH;
    assert (TUSER) else
      $fatal(1, "Can't set trailing if TUSER doesn't exist");
    assert (USER_WIDTH >= TRAILING_WIDTH) else
      $fatal(1, "USER_WIDTH is to narrow to set trailing");
    tuser[USER_TRAILING_WIDTH-1:0] = keep2trailing(keep);
  endtask : set_trailing_bytes


  //---------------------------------------
  // Packet PROGRESS functions
  //---------------------------------------
  // Notify when a byte is reached(VALID)
  function automatic bit reached_packet_byte(
    input int      OFFSET
  );
    // constant because they only depend on offset
    int WORD_OFFSET;
    assert (MAX_PACKET_BYTES > 0) else
      $fatal(1,"checking packet on a non packet bus");
    // Constant
    WORD_OFFSET = OFFSET / (DATA_WIDTH/8);

    return (word_count == WORD_OFFSET);

  endfunction : reached_packet_byte

  // Notify when a byte is being transmitted(VALID && READY)
  function automatic bit xfering_packet_byte(
    input int      OFFSET
  );

    return reached_packet_byte(OFFSET) && tvalid && tready;

  endfunction : xfering_packet_byte

  // drive functions  (These are not particularly useful
  // but they guarantee the modules using the package don't
  // drive the interface with a continuous assignment
  task automatic drive_tlast(input  logic  last);
    tlast = last;
  endtask
  task automatic drive_tvalid(input  logic  valid);
    tvalid = valid;
  endtask
  task automatic drive_tready(input  logic  ready);
    tready = ready;
  endtask
  task automatic drive_tdata(input  logic  data);
    tdata = data;
  endtask
  task automatic drive_tuser(input  logic  user);
    tuser = user;
  endtask
  task automatic drive_tkeep(input  logic  keep);
    tkeep = keep;
  endtask
  function automatic word_count_t get_word_count();
    return word_count;
  endfunction

  //---------------------------------------
  // Packet FIELD READ functions
  //
  // USAGE:
  //
  // //  get the fields - don't use assign. assign will not activate with changes to in1.
  // always_comb begin  : get_fields
  //  eth_dst_addr_new   = in1.get_packet_field48(eth_dst_addr_old,DST_MAC_BYTE,.NETWORK_ORDER(1));
  //  reached_end_of_udp = in1.reached_packet_byte(DST_PORT_BYTE+3);// we have enough to decide
  // end
  //
  // //  use the fields
  // always_comb begin  : use_fields
  //    if (reached_end_of_udp) begin // Wait until you know the field is fully captured before using it!
  //       ** DO SOMETHING INTERESTING WITH  est_dst_addr_new
  // end


  //---------------------------------------
  // Grab a byte from a passing stream
  function automatic logic [7:0] get_packet_byte(
    input logic [7:0]  old_value,
    input int          OFFSET
  );
    // constant because they only depend on offset
    int BYTE_OFFSET;

    logic [7:0] new_value;

    // Constant
    BYTE_OFFSET = OFFSET % (DATA_WIDTH/8);

    // default : Stay the same
    new_value = old_value;

    if (reached_packet_byte(OFFSET)) begin
      new_value = tdata[BYTE_OFFSET*8+:8];
    end
    return new_value;
  endfunction : get_packet_byte

  // Grab a 16 bit field from a passing stream
  function automatic logic [15:0] get_packet_field16(
    input logic [15:0] old_value,
    input int          OFFSET,
    input int          NETWORK_ORDER=0
  );
    logic [15:0] new_value;

    localparam BYTES=$size(new_value)/8;
    for (int i=0;i < BYTES; i++) begin
      if (NETWORK_ORDER==1) begin
        new_value[i*8+:8] = get_packet_byte(old_value[i*8+:8],OFFSET+BYTES-1-i);
      end else begin
        new_value[i*8+:8] = get_packet_byte(old_value[i*8+:8],OFFSET+i);
      end
    end
    return new_value;

  endfunction : get_packet_field16

  // Grab a 32 bit field from a passing stream
  function automatic logic [31:0] get_packet_field32(
    input logic [31:0] old_value,
    input int          OFFSET,
    input int          NETWORK_ORDER=0
  );
    logic [31:0] new_value;

    localparam BYTES=$size(new_value)/8;
    for (int i=0;i < BYTES; i++) begin
      if (NETWORK_ORDER==1) begin
        new_value[i*8+:8] = get_packet_byte(old_value[i*8+:8],OFFSET+BYTES-1-i);
      end else begin
        new_value[i*8+:8] = get_packet_byte(old_value[i*8+:8],OFFSET+i);
      end
    end

    return new_value;

  endfunction : get_packet_field32

  // Grab a 48 bit field from a passing stream
  function automatic logic [47:0] get_packet_field48(
    input logic [47:0] old_value,
    input int          OFFSET,
    input int          NETWORK_ORDER=0
  );
    logic [47:0] new_value;

    localparam BYTES=$size(new_value)/8;
    for (int i=0;i < BYTES; i++) begin
      if (NETWORK_ORDER==1) begin
        new_value[i*8+:8] = get_packet_byte(old_value[i*8+:8],OFFSET+BYTES-1-i);
      end else begin
        new_value[i*8+:8] = get_packet_byte(old_value[i*8+:8],OFFSET+i);
      end
    end

    return new_value;

  endfunction : get_packet_field48

  //---------------------------------------
  // Packet FIELD OVERWRITE functions
  //
  // USAGE:
  //
  // // Each call muxes data as it goes by.  This chains together muxing which synthesis will simplify
  // always_comb begin : set_header_fields
  //  v2e5.tdata = v2e4.tdata;
  //  v2e5.put_packet_field48(mac_dst,DST_MAC_BYTE,.NETWORK_ORDER(1));
  // end
  //---------------------------------------

  // Overwrite a byte in a passing stream
  task automatic put_packet_byte(
     input logic [7:0]  new_value,
     input int          OFFSET
  );
    // constant because they only depend on offset
    int    BYTE_OFFSET;

    // Constant
    BYTE_OFFSET = OFFSET % (DATA_WIDTH/8);
    if (reached_packet_byte(OFFSET)) begin
       tdata[BYTE_OFFSET*8+:8] = new_value;
    end
  endtask : put_packet_byte

  // Overwrite a 16 bit field in a passing stream
  task automatic put_packet_field16(
    input logic [15:0] new_value,
    input int          OFFSET,
    input int          NETWORK_ORDER=0
  );
    localparam BYTES=$size(new_value)/8;
    for (int i=0;i < BYTES; i++) begin
      if (NETWORK_ORDER==1) begin
        put_packet_byte(new_value[i*8+:8],OFFSET+BYTES-1-i);
      end else begin
        put_packet_byte(new_value[i*8+:8],OFFSET+i);
      end
    end
  endtask : put_packet_field16

  // Overwrite a 32 bit field in a passing stream
  task automatic put_packet_field32(
    input logic [31:0] new_value,
    input int          OFFSET,
    input int          NETWORK_ORDER=0
  );
    localparam BYTES=$size(new_value)/8;
    for (int i=0;i < BYTES; i++) begin
      if (NETWORK_ORDER==1) begin
        put_packet_byte(new_value[i*8+:8],OFFSET+BYTES-1-i);
      end else begin
        put_packet_byte(new_value[i*8+:8],OFFSET+i);
      end
    end
  endtask : put_packet_field32

  // Overwrite a 48 bit field in a passing stream
  task automatic put_packet_field48(
    input logic [47:0] new_value,
    input int          OFFSET,
    input int          NETWORK_ORDER=0
  );
    localparam BYTES=$size(new_value)/8;
    for (int i=0;i < BYTES; i++) begin
      if (NETWORK_ORDER==1) begin
        put_packet_byte(new_value[i*8+:8],OFFSET+BYTES-1-i);
      end else begin
        put_packet_byte(new_value[i*8+:8],OFFSET+i);
      end
    end
  endtask : put_packet_field48


  // View from the master side
  modport master (
    input  clk, rst,
    output tvalid, tdata, tuser, tkeep, tlast,
    input  tready,
    //import methods
    import pack,
    import unpack,
    import keep2trailing,
    import trailing2keep,
    import get_trailing_bytes,
    import set_trailing_bytes,
    import get_word_count,
    import reached_packet_byte,
    import xfering_packet_byte,
    import get_packet_byte,
    import get_packet_field16,
    import get_packet_field32,
    import get_packet_field48,
    import put_packet_byte,
    import put_packet_field16,
    import put_packet_field32,
    import put_packet_field48
  );

  // View from the slave side
  modport slave (
    input  clk, rst,
    input  tvalid, tdata, tuser, tkeep, tlast,
    output tready,
    //import methods
    import pack,
    import unpack,
    import keep2trailing,
    import trailing2keep,
    import get_trailing_bytes,
    import set_trailing_bytes,
    import get_word_count,
    import reached_packet_byte,
    import xfering_packet_byte,
    import get_packet_byte,
    import get_packet_field16,
    import get_packet_field32,
    import get_packet_field48,
    import put_packet_byte,
    import put_packet_field16,
    import put_packet_field32,
    import put_packet_field48
  );


endinterface : AxiStreamPacketIf

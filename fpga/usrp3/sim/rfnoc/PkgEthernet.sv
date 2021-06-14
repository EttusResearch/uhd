//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: PkgEthernet
//
// Description: This package defines the data types used to represent ETHER,
// IPV4, and UDP.  It's based on a queue of bytes representation named
// raw_pkt_t.
//

package PkgEthernet;

  import PkgAxiStreamBfm::*;
  export PkgAxiStreamBfm::*;

  //************************************************************//
  //////////////////// ETHER PACKETS /////////////////////////////
  //************************************************************//

  // Ether type - subset of possibilities. Add more as needed.
  typedef enum logic [15:0] {
    // Byte order            1  0
    IPV4=16'h0800, IPV6=16'h86_DD, VLAN_TAGGED=16'h8100,
    DOUBLE_VLAN_TAGGED=16'h9100, ROCE=16'h8915
  } ether_type_t;

  // Some MAC addresses
  // Byte order                                      5  4  3  2  1  0
  localparam logic [47:0] DEF_DEST_MAC_ADDR   = 48'h5E_35_EB_71_46_F7;
  localparam logic [47:0] DEF_SRC_MAC_ADDR    = 48'h98_03_9B_8E_09_B9;
  localparam logic [47:0] DEF_BRIDGE_MAC_ADDR = 48'hBB_BB_BB_BB_BB_BB;

  // Ether Header
  typedef struct {
    logic [47:0]  dest_mac   = DEF_DEST_MAC_ADDR;
    logic [47:0]  src_mac    = DEF_SRC_MAC_ADDR;
    ether_type_t  ether_type = IPV4;
  } eth_hdr_t;

  // Ethernet Packet, Header + Payload
  typedef struct {
    eth_hdr_t     hdr;
    raw_pkt_t     payload;
    logic [31:0]  fcs;
    int           ipg     = 12; // interpacket gap
  } eth_pkt_t;

  // Break an eth_pkt into a queue of bytes
  function automatic raw_pkt_t flatten_eth_pkt(input eth_pkt_t pkt);
    raw_pkt_t pay;

    pay.push_back(pkt.hdr.dest_mac[47:40]);
    pay.push_back(pkt.hdr.dest_mac[39:32]);
    pay.push_back(pkt.hdr.dest_mac[31:24]);
    pay.push_back(pkt.hdr.dest_mac[23:16]);
    pay.push_back(pkt.hdr.dest_mac[15:8]);
    pay.push_back(pkt.hdr.dest_mac[7:0]);
    pay.push_back(pkt.hdr.src_mac[47:40]);
    pay.push_back(pkt.hdr.src_mac[39:32]);
    pay.push_back(pkt.hdr.src_mac[31:24]);
    pay.push_back(pkt.hdr.src_mac[23:16]);
    pay.push_back(pkt.hdr.src_mac[15:8]);
    pay.push_back(pkt.hdr.src_mac[7:0]);
    pay.push_back(pkt.hdr.ether_type[15:8]);
    pay.push_back(pkt.hdr.ether_type[7:0]);
    pay = {pay,pkt.payload};

    return pay;

  endfunction

  // Break a queue of bytes into a eth_pkt
  function automatic eth_pkt_t unflatten_eth_pkt(input raw_pkt_t pay);
    eth_pkt_t pkt;

    pkt.hdr.dest_mac[47:40]  = pay.pop_front();
    pkt.hdr.dest_mac[39:32]  = pay.pop_front();
    pkt.hdr.dest_mac[31:24]  = pay.pop_front();
    pkt.hdr.dest_mac[23:16]  = pay.pop_front();
    pkt.hdr.dest_mac[15:8]   = pay.pop_front();
    pkt.hdr.dest_mac[7:0]    = pay.pop_front();
    pkt.hdr.src_mac[47:40]   = pay.pop_front();
    pkt.hdr.src_mac[39:32]   = pay.pop_front();
    pkt.hdr.src_mac[31:24]   = pay.pop_front();
    pkt.hdr.src_mac[23:16]   = pay.pop_front();
    pkt.hdr.src_mac[15:8]    = pay.pop_front();
    pkt.hdr.src_mac[7:0]     = pay.pop_front();
    pkt.hdr.ether_type[15:8] = pay.pop_front();
    pkt.hdr.ether_type[7:0]  = pay.pop_front();
    pkt.payload = pay;

    return pkt;

  endfunction

  function automatic logic eth_pkt_compare(input eth_pkt_t a, input eth_pkt_t b);

    return ((a.hdr.dest_mac == b.hdr.dest_mac) &&
            (a.hdr.src_mac == b.hdr.src_mac) &&
            (a.hdr.ether_type == b.hdr.ether_type) &&
            raw_pkt_compare(a.payload,b.payload));

  endfunction

  //************************************************************//
  //////////////////// IPV4 PACKETS //////////////////////////////
  //************************************************************//

  // IP Protocol - subset of possibilities. add more as needed
  typedef enum logic [7:0] {
    UDP=8'd17, TCP=8'd6, ICMP=8'd1, IGMP=8'd2, ENCAP=8'd41
  } ip_protocol_t;

  // follow normal convention of an IP address
  function automatic logic [31:0] ip(logic [7:0] a,b,c,d);
    return {a,b,c,d};
  endfunction


  localparam logic [31:0] DEF_DEST_IP_ADDR   = ip(192,168,10,2);
  localparam logic [31:0] DEF_SRC_IP_ADDR    = ip(192,168,10,1);
  localparam logic [31:0] DEF_BRIDGE_IP_ADDR = 32'h33_33_33_33;

  // IPv4 Header
  typedef struct {
    logic  [3:0]      header_length  = 4'd5;
    logic  [3:0]      version        = 4'd4;
    logic  [5:0]      dscp           = 6'b0000_00;
    logic  [1:0]      ecn            = 2'b00;
    logic [15:0]      length         = 16'hXXXX; //flag for (fill it in please)
    logic [15:0]      identification = 16'h462E;
    logic             rsv_zero       = 1'b0;
    logic             dont_frag      = 1'b1;
    logic             more_frag      = 1'b0;
    logic [12:0]      frag_offset    = 16'd0;
    logic  [7:0]      time_to_live   = 16'd64;
    ip_protocol_t     protocol       = UDP;
    logic [15:0]      checksum       = 16'hXXXX; //flag for (fill it in please)
    logic [31:0]      src_ip         = DEF_SRC_IP_ADDR;
    logic [31:0]      dest_ip        = DEF_DEST_IP_ADDR;
  } ipv4_hdr_t;

  // IP Packet, Header + Payload
  typedef struct {
    ipv4_hdr_t    hdr;
    raw_pkt_t     payload;
  } ipv4_pkt_t;

  // The checksum for an IP header is the sum of all the 16 bit words that
  // make up the header with the checksum set to zero. Add back the carry over
  // from bits [31:16] then invert.
  // See https://en.wikipedia.org/wiki/IPv4_header_checksum
  function automatic logic [15:0] calc_ipv4_checksum(input raw_pkt_t pkt);

    // This is a bit oversized, but it's not costing anything.
    // 10 max sized words can at most add logbase2 of 10 bits.
    logic [31:0] checksum;

    checksum = 0;
    // Iterate over 16 bit chunks reading from a byte addressed memory.
    // There are 20 bytes in an ipv4 header.
    for (int i = 0 ; i < 20 ; i+=2 ) begin
      // BIG endian network ordering... so weird
      checksum += {pkt[i],pkt[i+1]};
    end
    checksum += checksum[31:16];
    checksum = ~checksum;

    return checksum[15:0];

  endfunction

  // Break an eth_pkt into a queue of bytes
  function automatic raw_pkt_t flatten_ipv4_pkt(input ipv4_pkt_t pkt);
    raw_pkt_t pay;

    logic [15:0] length;
    logic [15:0] checksum;
    logic [2:0]  flags;

    // If header or length is not set to default value then use the value in
    // the packet.
    if ($isunknown(pkt.hdr.length))
       length = pkt.payload.size()+20; // 20 because length includes IP header length.
    else
       length = pkt.hdr.length;

    flags = {pkt.hdr.more_frag,pkt.hdr.dont_frag,pkt.hdr.rsv_zero};
    // Start off with checksum as 0
    checksum = 0;

    // 20 byte IP header
    pay.push_back({pkt.hdr.version,pkt.hdr.header_length}); // byte 0
    pay.push_back({pkt.hdr.dscp,pkt.hdr.ecn});              // byte 1
    pay.push_back(length[15:8]);                            // byte 2
    pay.push_back(length[7:0]);                             // byte 3
    pay.push_back(pkt.hdr.identification[15:8]);            // byte 4
    pay.push_back(pkt.hdr.identification[7:0]);             // byte 5
    pay.push_back({flags,pkt.hdr.frag_offset[12:8]});       // byte 6
    pay.push_back(pkt.hdr.frag_offset[7:0]);                // byte 7
    pay.push_back(pkt.hdr.time_to_live);                    // byte 8
    pay.push_back(pkt.hdr.protocol);                        // byte 9
    pay.push_back(checksum[15:8]);                          // byte 10
    pay.push_back(checksum[7:0]);                           // byte 11
    pay.push_back(pkt.hdr.src_ip[31:24]);                   // byte 12
    pay.push_back(pkt.hdr.src_ip[23:16]);                   // byte 13
    pay.push_back(pkt.hdr.src_ip[15:8]);                    // byte 14
    pay.push_back(pkt.hdr.src_ip[7:0]);                     // byte 15
    pay.push_back(pkt.hdr.dest_ip[31:24]);                  // byte 16
    pay.push_back(pkt.hdr.dest_ip[23:16]);                  // byte 17
    pay.push_back(pkt.hdr.dest_ip[15:8]);                   // byte 18
    pay.push_back(pkt.hdr.dest_ip[7:0]);                    // byte 19
    pay = {pay,pkt.payload};

    if ($isunknown(pkt.hdr.checksum))
       checksum = calc_ipv4_checksum(pay);
    else
    checksum = pkt.hdr.checksum;
    // replace the checksum (bytes 11:10
    pay[10] = checksum[15:8];
    pay[11] = checksum[7:0];

    return pay;
  endfunction

  // Break a queue of bytes into a ip_pkt
  function automatic ipv4_pkt_t unflatten_ipv4_pkt(input raw_pkt_t pay);
    ipv4_pkt_t pkt;

    // 20 byte IP header
    {pkt.hdr.version,
     pkt.hdr.header_length}       = pay.pop_front(); // byte 0
    {pkt.hdr.dscp,pkt.hdr.ecn}    = pay.pop_front(); // byte 1
    pkt.hdr.length[15:8]          = pay.pop_front(); // byte 2
    pkt.hdr.length[7:0]           = pay.pop_front(); // byte 3
    pkt.hdr.identification[15:8]  = pay.pop_front(); // byte 4
    pkt.hdr.identification[7:0]   = pay.pop_front(); // byte 5
    {pkt.hdr.more_frag,
     pkt.hdr.dont_frag,
     pkt.hdr.rsv_zero,
     pkt.hdr.frag_offset[12:8]}   = pay.pop_front(); // byte 6
    pkt.hdr.frag_offset[7:0]      = pay.pop_front(); // byte 7
    pkt.hdr.time_to_live          = pay.pop_front(); // byte 8
    pkt.hdr.protocol              = ip_protocol_t'(pay.pop_front()); // byte 9
    pkt.hdr.checksum[15:8]        = pay.pop_front(); // byte 10
    pkt.hdr.checksum[7:0]         = pay.pop_front(); // byte 11
    pkt.hdr.src_ip[31:24]         = pay.pop_front(); // byte 12
    pkt.hdr.src_ip[23:16]         = pay.pop_front(); // byte 13
    pkt.hdr.src_ip[15:8]          = pay.pop_front(); // byte 14
    pkt.hdr.src_ip[7:0]           = pay.pop_front(); // byte 15
    pkt.hdr.dest_ip[31:24]        = pay.pop_front(); // byte 16
    pkt.hdr.dest_ip[23:16]        = pay.pop_front(); // byte 17
    pkt.hdr.dest_ip[15:8]         = pay.pop_front(); // byte 18
    pkt.hdr.dest_ip[7:0]          = pay.pop_front(); // byte 19
    pkt.payload = pay;

    return pkt;

  endfunction

  function automatic logic ipv4_pkt_compare(input ipv4_pkt_t a, input ipv4_pkt_t b);

    return ((a.hdr.header_length == b.hdr.header_length) &&
            (a.hdr.version == b.hdr.version) &&
            (a.hdr.dscp == b.hdr.dscp) &&
            (a.hdr.ecn == b.hdr.ecn) &&
            (a.hdr.length == b.hdr.length) &&
            (a.hdr.identification == b.hdr.identification) &&
            (a.hdr.rsv_zero == b.hdr.rsv_zero) &&
            (a.hdr.dont_frag == b.hdr.dont_frag) &&
            (a.hdr.more_frag == b.hdr.more_frag) &&
            (a.hdr.frag_offset == b.hdr.frag_offset) &&
            (a.hdr.time_to_live == b.hdr.time_to_live) &&
            (a.hdr.protocol == b.hdr.protocol) &&
            (a.hdr.checksum == b.hdr.checksum) &&
            (a.hdr.src_ip == b.hdr.src_ip) &&
            (a.hdr.dest_ip == b.hdr.dest_ip) &&
            raw_pkt_compare(a.payload,b.payload));

  endfunction

  //************************************************************//
  //////////////////// UDP PACKETS ///////////////////////////////
  //************************************************************//

  localparam logic [15:0] DEF_SRC_UDP_PORT    = 16'd49748;
  localparam logic [15:0] DEF_DEST_UDP_PORT   = 16'd49153;
  localparam logic [15:0] DEF_BRIDGE_UDP_PORT = 16'h66_55;

  // UDP Header
  typedef struct {
    logic [15:0]       src_port   = DEF_SRC_UDP_PORT;
    logic [15:0]       dest_port  = DEF_DEST_UDP_PORT;
    logic [15:0]       length     = 16'hXXXX; //flag for (fill it in please)
    logic [15:0]       checksum   = 16'hXXXX; //flag for (fill it in please)
  } udp_hdr_t;

  // UDP Packet, Header + Payload
  typedef struct {
    udp_hdr_t     hdr;
    raw_pkt_t     payload;
  } udp_pkt_t;

  function automatic logic [15:0] calc_udp_checksum(
    input logic [31:0] src_ip,
    input logic [31:0]  dest_ip,
    input raw_pkt_t pkt);

    logic [31:0] checksum;
    raw_pkt_t    virtual_header;

    // UDP checksum is calculated over a virtual header that is added to
    // the front of the packet.
    virtual_header.push_back(src_ip[31:24]);  // byte 0
    virtual_header.push_back(src_ip[23:16]);  // byte 1
    virtual_header.push_back(src_ip[15:8]);   // byte 2
    virtual_header.push_back(src_ip[7:0]);    // byte 3
    virtual_header.push_back(dest_ip[31:24]); // byte 4
    virtual_header.push_back(dest_ip[23:16]); // byte 5
    virtual_header.push_back(dest_ip[15:8]);  // byte 6
    virtual_header.push_back(dest_ip[7:0]);   // byte 7
    virtual_header.push_back(0);              // byte 8
    virtual_header.push_back(UDP);            // byte 9  UDP (Protocol enum) x11
    virtual_header.push_back(0);              // byte 10
    virtual_header.push_back(pkt[6]);         // byte 11 Length
    virtual_header.push_back(pkt[7]);         // byte 12 Length

    pkt = {virtual_header,pkt}; // add virtual header in front

    checksum = 0;
    // Iterate over 16 bit chunks reading from an array of bytes
    // need to traverse the virtual header / udp header / udp data
    for (int i = 0 ; i < pkt.size ; i+=2 ) begin
      // BIG endian network ordering... so weird
      checksum += {pkt[i],pkt[i+1]};
    end
    checksum += checksum[31:16];
    checksum = ~checksum;

    return checksum[15:0];

  endfunction

  // Break a udp_pkt into a queue of bytes
  function automatic raw_pkt_t flatten_udp_pkt(
    input logic [31:0] src_ip,
    input logic [31:0] dest_ip,
    input udp_pkt_t pkt);
    raw_pkt_t pay;

    logic [15:0] length;
    logic [15:0] checksum;

    // If header or length is not set to default value then use the value in
    // the packet.
    if ($isunknown(pkt.hdr.length))
      length = pkt.payload.size()+8; // 8 because length includes UDP header length.
    else
    length = pkt.hdr.length;

    //temporary checksum
    checksum = 0;

    pay.push_back(pkt.hdr.src_port[15:8]);  // byte 0
    pay.push_back(pkt.hdr.src_port[7:0]);   // byte 1
    pay.push_back(pkt.hdr.dest_port[15:8]); // byte 2
    pay.push_back(pkt.hdr.dest_port[7:0]);  // byte 3
    pay.push_back(length[15:8]);            // byte 4
    pay.push_back(length[7:0]);             // byte 5
    pay.push_back(checksum[15:8]);          // byte 6
    pay.push_back(checksum[7:0]);           // byte 7
    pay = {pay,pkt.payload};

    if ($isunknown(pkt.hdr.checksum))
      checksum = calc_udp_checksum(src_ip,dest_ip,pay);
    else
      checksum = pkt.hdr.checksum;

    pay[6] = checksum[15:8];
    pay[7] = checksum[7:0];

    return pay;

  endfunction

  // Break a queue of bytes into a udp_pkt
  function automatic udp_pkt_t unflatten_udp_pkt(input raw_pkt_t pay);
    udp_pkt_t pkt;

    pkt.hdr.src_port[15:8]    = pay.pop_front();
    pkt.hdr.src_port[7:0]     = pay.pop_front();
    pkt.hdr.dest_port[15:8]   = pay.pop_front();
    pkt.hdr.dest_port[7:0]    = pay.pop_front();
    pkt.hdr.length[15:8]      = pay.pop_front();
    pkt.hdr.length[7:0]       = pay.pop_front();
    pkt.hdr.checksum[15:8]    = pay.pop_front();
    pkt.hdr.checksum[7:0]     = pay.pop_front();
    pkt.payload = pay;

    return pkt;

  endfunction

  function automatic logic udp_pkt_compare(input udp_pkt_t a, input udp_pkt_t b);

    return ((a.hdr.src_port == b.hdr.src_port) &&
            (a.hdr.dest_port == b.hdr.dest_port) &&
            (a.hdr.length == b.hdr.length) &&
            raw_pkt_compare(a.payload,b.payload));

  endfunction

  typedef enum int {
     NO_PREAMBLE=0, NORMAL_PREAMBLE=1, ZERO_PREAMBLE=2
  } preamble_t;

  // Build up a raw UDP packet.
  // Args:
  // - pkt: Packet data (queue)
  // - stream: Stream to use (Optional)
  function automatic raw_pkt_t build_udp_pkt (
    input eth_hdr_t  eth_hdr,
    input ipv4_hdr_t ipv4_hdr,
    input udp_hdr_t  udp_hdr,
    input raw_pkt_t  pay,
    input int        preamble = NO_PREAMBLE);

    automatic udp_pkt_t  udp_pkt;
    automatic ipv4_pkt_t ipv4_pkt;
    automatic eth_pkt_t  eth_pkt;
    automatic raw_pkt_t  raw_pkt;

    udp_pkt.hdr      = udp_hdr;
    udp_pkt.payload  = pay;
    ipv4_pkt.hdr     = ipv4_hdr;
    ipv4_pkt.payload = flatten_udp_pkt(ipv4_hdr.src_ip,ipv4_hdr.dest_ip,udp_pkt);
    eth_pkt.hdr      = eth_hdr;
    eth_pkt.payload  = flatten_ipv4_pkt(ipv4_pkt);
    raw_pkt          = flatten_eth_pkt(eth_pkt);
    if (preamble==NORMAL_PREAMBLE) begin
      raw_pkt.push_front(8'hAB);
      raw_pkt.push_front(8'hAA);
      raw_pkt.push_front(8'hAA);
      raw_pkt.push_front(8'hAA);
      raw_pkt.push_front(8'hAA);
      raw_pkt.push_front(8'hAA);
    end else if (preamble==ZERO_PREAMBLE) begin
      raw_pkt.push_front(8'h00);
      raw_pkt.push_front(8'h00);
      raw_pkt.push_front(8'h00);
      raw_pkt.push_front(8'h00);
      raw_pkt.push_front(8'h00);
      raw_pkt.push_front(8'h00);
    end
    return raw_pkt;

  endfunction

  // Wait for a packet to finish on the bus
  // and decode it
  task automatic decode_udp_pkt (
    input  raw_pkt_t  raw_pkt,
    output eth_hdr_t  eth_hdr,
    output ipv4_hdr_t ipv4_hdr,
    output udp_hdr_t  udp_hdr,
    output raw_pkt_t  payload);

    eth_pkt_t  eth_pkt;
    ipv4_pkt_t ipv4_pkt;
    udp_pkt_t  udp_pkt;

    eth_pkt  = unflatten_eth_pkt(raw_pkt);
    ipv4_pkt = unflatten_ipv4_pkt(eth_pkt.payload);
    udp_pkt  = unflatten_udp_pkt(ipv4_pkt.payload);

    eth_hdr  = eth_pkt.hdr;
    ipv4_hdr = ipv4_pkt.hdr;
    udp_hdr  = udp_pkt.hdr;
    payload  = udp_pkt.payload;

  endtask

  //---------------------------------------------------------------------------
  // XPORT Stream Packet Class
  //---------------------------------------------------------------------------
  // Extensions to the AxiStreamPacket used in the XPORT code
  class XportStreamPacket #(
    int DATA_WIDTH = 64
  ) extends AxiStreamPacket #(DATA_WIDTH, $clog2((DATA_WIDTH/8)+1));

    typedef XportStreamPacket #(DATA_WIDTH) XportPacket_t;
    localparam UWIDTH = $clog2((DATA_WIDTH/8)+1);
    // Class constructor.
    function new ();
      super.new();
    endfunction : new

    // Return a handle to a copy of this transaction
    function XportPacket_t copy();
      XportPacket_t temp;
      temp = new();
      temp.data = this.data;
      temp.user = this.user;
      temp.keep = this.keep;
      return temp;
    endfunction

    // bring in data from an AxisPacket
    function void import_axis(AxisPacket_t axi_pkt);
      this.data = axi_pkt.data;
      this.user = axi_pkt.user;
      this.keep = axi_pkt.keep;
    endfunction : import_axis


    // figure out the value of tkeep based on tuser(trailing words)
    function keep_t calc_last_tkeep(user_t tuser);
      keep_t last_tkeep;
      // check if there is an X
      if (tuser[$clog2(DATA_WIDTH/8)-1:0] === 0) begin
        last_tkeep = '1;
      end else if ($isunknown(tuser[$clog2(DATA_WIDTH/8)-1:0])) begin
        last_tkeep = '1;
      end else begin
        last_tkeep = 0;
        foreach (calc_last_tkeep[i]) begin
          // set the bit if it's less than value of tuser
          last_tkeep[i] = i < tuser[$clog2(DATA_WIDTH/8)-1:0];
        end
      end
      return last_tkeep;
    endfunction : calc_last_tkeep;

    // figure out the value of tuser(traling words) based on tkeep
    function user_t calc_last_tuser(keep_t tkeep);
      user_t last_tuser;
      // check if there is an X
      if ($isunknown(tkeep)) begin
        last_tuser = '0;
      end else begin
        last_tuser = 0;
        foreach (tkeep[i]) begin
          if (tkeep[i]==1'b1) begin
            last_tuser = last_tuser+1;
          end
        end
      end
      // full word is 0
      if (last_tuser == DATA_WIDTH/8) begin
        last_tuser = 0;
      end
      return last_tuser;
    endfunction : calc_last_tuser;

    // clear bytes that aren't marked as valid by keep
    function void  clear_unused_bytes();
      keep_t last_tkeep;

      // check for empty packet, in which case there's nothing to clear
      if (this.data.size() == 0) return;

      last_tkeep = this.keep[$];
      // check that the last user is the same as the last keep
      assert (this.keep[$] == calc_last_tkeep(this.user[$])) else
        $error("clear_unused_bytes: final tkeep and tuser don't match");

      // set data bytes where the value isn't used to zero
      foreach (last_tkeep[i]) begin
        if (last_tkeep[i] == 0) this.data[$][i*8 +: 8] = 0;
      end
    endfunction : clear_unused_bytes;

    // take the tuser signal's and use them to set
    // tkeep signals
    task automatic tuser_to_tkeep(logic PRESERVE_TUSER=1);

      // set all the tuser values to zero
      // set all the tuser values to zero
      foreach (this.user[i]) begin
        this.keep[i] = '1;
        if(!PRESERVE_TUSER)
          this.user[i] = 0;
      end

      // figure out the value of tkeep for the last word
      this.keep[$] = calc_last_tkeep(this.user[$]);

      // set data bytes where the value isn't used to zero
      clear_unused_bytes;

    endtask : tuser_to_tkeep

    // take the tkeep signal's and use them to set
    // width in the user signals
    task automatic tkeep_to_tuser(int ERROR_PROB=0);

      // set all the tuser values to zero
      foreach (this.user[i]) begin
        this.user[i] = 0;
        this.user[i][UWIDTH-1] = $urandom_range(99) < ERROR_PROB;
      end

      // figure out the value of user for the last word
      this.user[$] = calc_last_tuser(this.keep[$]);
      this.user[$][UWIDTH-1] = $urandom_range(99) < ERROR_PROB;

      // set data bytes where the value isn't used to zero
      clear_unused_bytes;

    endtask : tkeep_to_tuser

    function automatic logic has_error();
      logic error;

      error = 0;
      foreach (this.user[i]) begin
        //catch if error was set
        error = error || this.user[i][UWIDTH-1];
      end
      return error;
    endfunction : has_error

    function automatic int byte_length();
      int bytes;
      int last_bytes;
      bytes = (this.data.size()-1)*DATA_WIDTH/8;
      last_bytes = this.user[$][UWIDTH-2:0];
      if (last_bytes == 0)
        bytes+=DATA_WIDTH/8;
      else
        bytes+=last_bytes;

      return bytes;

    endfunction : byte_length

    function automatic void clear_error();

      foreach (this.user[i]) begin
        this.user[i][UWIDTH-1] = 0;
      end

    endfunction : clear_error

    function automatic void clear_keep();

      foreach (this.keep[i]) begin
        this.keep[i] = 0;
      end

    endfunction : clear_keep

    function automatic void clear_user();

      foreach (this.user[i]) begin
        this.user[i] = 0;
      end

    endfunction : clear_user

    task automatic set_error();
      foreach (this.user[i]) begin
        this.user[i][UWIDTH-1] = 1;
      end
    endtask : set_error

    ///// compare_w_error
    // Check that this packet has expected error bit in tuser
    // Keep is not compared
    // If COMPARE_ERROR_PACKETS is 0
    //   Don't check packet contents
    // If COMPARE_ERROR_PACKETS is 1
    //   Check that this packet matches the expected packet
    function automatic logic compare_w_error(
      XportPacket_t expected,
      int COMPARE_ERROR_PACKETS=1
    );

      automatic XportPacket_t actual_copy = this.copy();
      automatic XportPacket_t expected_copy = expected.copy();

      logic exp_error=0;
      logic act_error=0;
      logic error_condition;

      exp_error = expected.has_error();
      act_error = this.has_error();

      actual_copy.clear_error();
      expected_copy.clear_error();
      actual_copy.clear_unused_bytes();
      expected_copy.clear_unused_bytes();

      error_condition = (!expected_copy.equal(actual_copy) &&
                         (!exp_error || COMPARE_ERROR_PACKETS)) ||
                         act_error != exp_error;
      if (error_condition) begin
        $display("Expected");
        expected.print();
        $display("Actual");
        this.print();
        if (!expected_copy.equal(actual_copy))
          $display("ERROR :: packet mismatch");
        if (act_error != exp_error)
          $display("ERROR :: error mismatch");
      end

      return error_condition;

    endfunction : compare_w_error

    ///// compare_w_sof
    // Check that this packet has expected sof bit in tuser
    // Keep is not compared
    // Check that this packet matches the expected packet
    function automatic logic compare_w_sof(XportPacket_t expected);

      automatic XportPacket_t actual_copy = this.copy();
      automatic XportPacket_t expected_copy = expected.copy();

      logic sof_error=0;
      foreach (this.user[i]) begin
        if (i==0) begin
          // set if top bit of user isn't set on the first word.
          sof_error = !this.user[i][UWIDTH-1];
        end else begin
          // set if top bit of user is set on any other word.
          sof_error = this.user[i][UWIDTH-1] || sof_error;
        end
      end

      // error bit doubles for SOF
      actual_copy.clear_error();
      expected_copy.clear_error();
      actual_copy.clear_unused_bytes();
      expected_copy.clear_unused_bytes();

      // set SOF in expected
      expected_copy.user[0][UWIDTH-1] = 0;

      if (!expected_copy.equal(actual_copy) ||
          sof_error) begin
        $display("Expected");
        expected_copy.print();
        $display("Actual");
        this.print();
        if (!expected_copy.equal(actual_copy))
          $display("ERROR :: packet mismatch");
        if (sof_error)
          $display("ERROR :: sof mismatch");
      end

      return !expected_copy.equal(actual_copy) ||
             sof_error;

    endfunction : compare_w_sof

    ///// compare_w_pad
    // Check that this packet has expected sof bit in tuser
    // Keep is not compared
    // Check that this packet matches the expected packet
    // if DUMB_ORIGINAL_WAY
    //   User is not compared
    // else
    //   Check that this packets tuser matches the expected packet
    function automatic logic compare_w_pad(
      XportPacket_t expected,
      logic DUMB_ORIGINAL_WAY=0
    );

      automatic XportPacket_t actual_copy = this.copy();
      automatic XportPacket_t expected_copy = expected.copy();

      // not using MSB as error here.
      actual_copy.clear_error();
      expected_copy.clear_error();
      actual_copy.clear_unused_bytes();
      expected_copy.clear_unused_bytes();

      // Add pad bytes to user
      if (DUMB_ORIGINAL_WAY) begin
        actual_copy.clear_keep();
        expected_copy.clear_keep();
        // I can't figure out a goodway to calculate the
        // expected tuser for non last word values.
        // So I'm just copying the actual
        foreach (expected_copy.user[i]) begin
          if (i != expected_copy.user.size()-1) begin
            expected_copy.user[i] = actual_copy.user[i];
          end
        end
      end

      if (!expected_copy.equal(actual_copy)) begin
        $display("Expected");
        expected_copy.print();
        $display("Actual");
        this.print();
        if (!expected_copy.equal(actual_copy))
          $display("ERROR :: packet mismatch");
      end

      return !expected_copy.equal(actual_copy);

    endfunction : compare_w_pad

    ///// compare_no_user
    // Check that this packet has expected sof bit in tuser
    // Keep is not compared
    // Check that this packet matches the expected packet
    // User is not compared
    function automatic logic compare_no_user(XportPacket_t expected, int PRINT_LVL=1);

      automatic XportPacket_t actual_copy = this.copy();
      automatic XportPacket_t expected_copy = expected.copy();

      // not using MSB as error here.
      actual_copy.clear_error();
      expected_copy.clear_error();
      actual_copy.clear_unused_bytes();
      expected_copy.clear_unused_bytes();

      // Add pad bytes to user
      foreach (expected_copy.user[i]) begin
       expected_copy.user[i] = 0;
       actual_copy.user[i] = 0;
      end

      if (PRINT_LVL==1) begin
        if (!expected_copy.equal(actual_copy)) begin
          $display("Expected");
          expected.print();
          $display("Actual");
          this.print();
          if (!expected_copy.equal(actual_copy))
            $display("ERROR :: packet mismatch");
        end
      end
      return !expected_copy.equal(actual_copy);

    endfunction : compare_no_user

  endclass : XportStreamPacket;

endpackage : PkgEthernet

//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: PkgChdrBfm
//
// Description: Package for a bi-directional CHDR bus functional model (BFM), 
// which consists primarily of the ChdrPacket and ChdrBfm classes.
//



package PkgChdrBfm;

  import PkgChdrUtils::*;
  import PkgAxiStreamBfm::*;


  //---------------------------------------------------------------------------
  // CHDR Packet Class
  //---------------------------------------------------------------------------

  class ChdrPacket #(int BUS_WIDTH = 64);

    typedef ChdrPacket #(BUS_WIDTH) ChdrPacket;

    chdr_header_t header;
    chdr_word_t   timestamp;
    chdr_word_t   metadata[$];
    chdr_word_t   data[$];

    extern function ChdrPacket copy();
    extern function bit        equal(ChdrPacket packet);
    extern function string     sprint(bit pretty = 1);
    extern function void       print(bit pretty = 1);

    // Accessors
    extern function void write_raw          (ref chdr_header_t         header,
                                             ref chdr_word_t           data[$],
                                             input chdr_word_t         metadata[$] = {},
                                             input chdr_word_t         timestamp = 0,
                                             input int                 data_byte_length = -1);
    extern function void read_raw           (output chdr_header_t      header, 
                                             output chdr_word_t        data[$],
                                             output chdr_word_t        metadata[$],
                                             output chdr_word_t        timestamp,
                                             output int                data_byte_length);
    extern function void write_stream_status(ref chdr_header_t         header, 
                                             ref chdr_str_status_t     status);
    extern function void read_stream_status (output chdr_header_t      header, 
                                             output chdr_str_status_t  status);
    extern function void write_stream_cmd   (ref chdr_header_t         header, 
                                             ref chdr_str_command_t    command);
    extern function void read_stream_cmd    (output chdr_header_t      header, 
                                             output chdr_str_command_t command);
    extern function void write_mgmt         (ref chdr_header_t         header, 
                                             ref chdr_mgmt_t           mgmt);
    extern function void read_mgmt          (output chdr_header_t      header, 
                                             output chdr_mgmt_t        mgmt);
    extern function void write_ctrl         (ref chdr_header_t         header,
                                             ref chdr_ctrl_header_t    ctrl_header,
                                             ref ctrl_op_word_t        ctrl_op_word,
                                             ref ctrl_word_t           ctrl_data[$],
                                             input chdr_word_t         ctrl_timestamp = 0);
    extern function void read_ctrl          (output chdr_header_t      header,
                                             output chdr_ctrl_header_t ctrl_header,
                                             output ctrl_op_word_t     ctrl_op_word,
                                             output ctrl_word_t        ctrl_data[$],
                                             output chdr_word_t        ctrl_timestamp);


    // Helper methods
    extern function int  header_bytes();
    extern function int  mdata_bytes();
    extern function int  data_bytes();
    extern function void update_lengths();

    extern function string sprint_raw();
    extern function string sprint_pretty();

  endclass : ChdrPacket;



  //---------------------------------------------------------------------------
  // CHDR BFM Class
  //---------------------------------------------------------------------------

  class ChdrBfm #(
    parameter int BUS_WIDTH  = 64,
    parameter int USER_WIDTH = 1
  ) extends AxiStreamBfm #(BUS_WIDTH, USER_WIDTH);

    typedef ChdrPacket #(BUS_WIDTH) ChdrPacket;

    // Number of 64-bit CHDR words per AXI word
    const int CHDR_PER_BUS = BUS_WIDTH / $bits(chdr_word_t);

    // Default fields used by high-level transaction methods
    chdr_epid_t     dst_epid;
    chdr_seq_num_t  seq_num;


    extern function new (
      virtual AxiStreamIf #(BUS_WIDTH, USER_WIDTH).master master,
      virtual AxiStreamIf #(BUS_WIDTH, USER_WIDTH).slave  slave
    );


    // Send Transactions
    extern task put_chdr(ChdrPacket chdr_packet);
    extern function bit try_put_chdr(ChdrPacket chdr_packet);


    // Receive Transactions
    extern task get_chdr(output ChdrPacket chdr_packet);
    extern function bit try_get_chdr(output ChdrPacket chdr_packet);
    extern task peek_chdr(output ChdrPacket chdr_packet);
    extern function bit try_peek_chdr(output ChdrPacket chdr_packet);


    // AXI-Stream/CHDR Conversion Functions
    extern function ChdrPacket axis_to_chdr (AxisPacket axis_packet);
    extern function AxisPacket chdr_to_axis (ChdrPacket chdr_packet);
      
  endclass : ChdrBfm



  //---------------------------------------------------------------------------
  // CHDR Packet Class Methods
  //---------------------------------------------------------------------------

  // Create a copy of this packet and return a handle to the copy
  function ChdrPacket::ChdrPacket ChdrPacket::copy();
    ChdrPacket temp;
    temp = new();
    temp.header    = this.header;
    temp.timestamp = this.timestamp;
    temp.metadata  = this.metadata;
    temp.data      = this.data;
    return temp;
  endfunction


  // Return true if this packet equals that of the argument
  function bit ChdrPacket::equal(ChdrPacket packet);
    if (header != packet.header) return 0;
    if (!chdr_word_queues_equal(data, packet.data)) return 0;
    if (!chdr_word_queues_equal(metadata, packet.metadata)) return 0;
    if (header.pkt_type == CHDR_DATA_WITH_TS && timestamp !== packet.timestamp) return 0;
    return 1;
  endfunction : equal


  // Format the contents of the packet into a string (don't dissect contents)
  function string ChdrPacket::sprint_raw();
    string str;
    str = {str, $sformatf("ChdrPacket:\n")};
    str = {str, $sformatf("- header: %p\n", header) };
    str = {str, $sformatf("- timestamp: %X\n", timestamp) };
    str = {str, $sformatf("- metadata:\n") };
    foreach (metadata[i]) begin
      str = {str, $sformatf("%5d> %X\n", i, metadata[i]) };
    end
    str = {str, $sformatf("- data:\n") };
    foreach (data[i]) begin
      str = {str, $sformatf("%5d> %X\n", i, data[i]) };
    end
    return str;
  endfunction : sprint_raw

  // Format the contents of the packet into a string (dissect contents)
  function string ChdrPacket::sprint_pretty();
    string str;
    str = {str, $sformatf("ChdrPacket:\n")};
    str = {str, $sformatf("- header: %p\n", header) };
    if (header.pkt_type == CHDR_DATA_WITH_TS) begin
      str = {str, $sformatf("- timestamp: %0d\n", timestamp) };
    end
    if (header.num_mdata != '0) begin
      str = {str, $sformatf("- metadata:\n") };
      foreach (metadata[i]) begin
        str = {str, $sformatf("%5d> %X\n", i, metadata[i]) };
      end
    end
    str = {str, $sformatf("- data (%s):\n", header.pkt_type.name) };
    if (header.pkt_type == CHDR_MANAGEMENT) begin
      chdr_header_t tmp_hdr;
      chdr_mgmt_t tmp_mgmt;
      read_mgmt(tmp_hdr, tmp_mgmt);
      str = {str, $sformatf("  > chdr_mgmt_header_t : %p\n", tmp_mgmt.header)};
      foreach (tmp_mgmt.ops[i]) begin
        str = {str, $sformatf("  > %3d: chdr_mgmt_op_t: '{op_payload:0x%12x,op_code:%s,ops_pending:%0d}\n",
          i, tmp_mgmt.ops[i].op_payload, tmp_mgmt.ops[i].op_code.name, tmp_mgmt.ops[i].ops_pending)};
      end
    end else if (header.pkt_type == CHDR_STRM_STATUS) begin
      chdr_header_t tmp_hdr;
      chdr_str_status_t tmp_sts;
      read_stream_status(tmp_hdr, tmp_sts);
      str = {str, $sformatf("  > chdr_str_status_t0 : '{status_info:0x%012x,buff_info:0x%04x,xfer_count_bytes:%0d,xfer_count_pkts:%0d...\n",
        tmp_sts.status_info,tmp_sts.buff_info,tmp_sts.xfer_count_bytes,tmp_sts.xfer_count_pkts)};
      str = {str, $sformatf("  > chdr_str_status_t1 :   capacity_pkts:%0d,capacity_bytes:%0d,status:%s,src_epid:%0d}\n",
        tmp_sts.capacity_pkts,tmp_sts.capacity_bytes,tmp_sts.status.name,tmp_sts.src_epid)};
    end else if (header.pkt_type == CHDR_STRM_CMD) begin
      chdr_header_t tmp_hdr;
      chdr_str_command_t tmp_cmd;
      read_stream_cmd(tmp_hdr, tmp_cmd);
      str = {str, $sformatf("  > chdr_str_command_t : %p\n", tmp_cmd)};
    end else if (header.pkt_type == CHDR_CONTROL) begin
      chdr_header_t tmp_hdr;
      chdr_word_t tmp_ts;
      chdr_ctrl_header_t tmp_ctrl_hdr;
      ctrl_op_word_t tmp_op_word;
      ctrl_word_t tmp_ctrl_data[$];
      read_ctrl(tmp_hdr, tmp_ctrl_hdr, tmp_op_word, tmp_ctrl_data, tmp_ts);
      str = {str, $sformatf("  > chdr_ctrl_header_t : %p\n", tmp_ctrl_hdr)};
      if (tmp_ctrl_hdr.has_time)
        str = {str, $sformatf("  > timestamp          : %0d\n", tmp_ts)};
      str = {str, $sformatf("  > ctrl_op_word_t     : '{status:%s,op_code:%s,byte_enable:0b%4b,address:0x%05x}\n",
        tmp_op_word.status.name, tmp_op_word.op_code.name, tmp_op_word.byte_enable, tmp_op_word.address)};
      foreach (tmp_ctrl_data[i]) begin
        str = {str, $sformatf("  > data %2d            : 0x%08x\n", i, tmp_ctrl_data[i])};
      end
    end else begin
      foreach (data[i]) begin
        str = {str, $sformatf("%5d> %X\n", i, data[i]) };
      end
    end
    return str;
  endfunction : sprint_pretty

  function string ChdrPacket::sprint(bit pretty = 1);
    if (pretty)
      return sprint_pretty();
    else
      return sprint_raw();
  endfunction: sprint

  // Print the contents of the packet
  function void ChdrPacket::print(bit pretty = 1);
    $display(sprint(pretty));
  endfunction : print


  // Populate the packet with the provided info. The packet Length and NumMData
  // fields are calculated and set in this method. Omitting the
  // data_byte_length argument, or providing a negative value, causes this
  // method to calculate the payload length based on the size of the data
  // array.
  function void ChdrPacket::write_raw (
    ref chdr_header_t header,
    ref chdr_word_t   data[$],
    input chdr_word_t metadata[$] = {},
    input chdr_word_t timestamp = 0,
    input int         data_byte_length = -1
  );
    this.header    = header;
    this.timestamp = timestamp;
    this.data      = data;
    this.metadata  = metadata;
    update_lengths();

    // Adjust length field according to data_byte_length
    if (data_byte_length >= 0) begin
      int array_num_bytes;

      // Make sure number of words for data_byte_length matches data length
      assert((data_byte_length+7) / 8 == data.size()) else begin
        $error("ChdrPacket::write_raw: data_byte_length doesn't correspond to number of words in data");
      end

      array_num_bytes = data.size() * $bits(chdr_word_t)/8;
      this.header.length -= (array_num_bytes - data_byte_length);
    end
  endfunction : write_raw


  // Read the contents of this packet
  function void ChdrPacket::read_raw (
    output chdr_header_t header, 
    output chdr_word_t   data[$],
    output chdr_word_t   metadata[$],
    output chdr_word_t   timestamp,
    output int           data_byte_length
  );
    header           = this.header;
    data             = this.data;
    metadata         = this.metadata;
    timestamp        = this.timestamp;
    data_byte_length = data_bytes();
  endfunction : read_raw


  // Populate this packet as a status packet
  function void ChdrPacket::write_stream_status (
    ref chdr_header_t     header,
    ref chdr_str_status_t status
  );
    header.pkt_type = CHDR_STRM_STATUS; // Update packet type in header
    this.header = header;
    data        = {};
    for (int i = 0; i < $bits(status); i += $bits(chdr_word_t)) begin
      data.push_back( status[i +: $bits(chdr_word_t)] );
    end
    update_lengths();
  endfunction : write_stream_status


  // Read this packet as a status packet
  function void ChdrPacket::read_stream_status (
    output chdr_header_t      header, 
    output chdr_str_status_t  status
  );
    // Make sure it's a stream status packet
    assert(this.header.pkt_type == CHDR_STRM_STATUS) else begin
      $error("ChdrPacket::read_status: Packet type is not CHDR_STRM_STATUS");
    end

    // Make sure we have enough payload
    assert($bits(status) <= $bits(data)) else begin
      $error("ChdrPacket::read_status: Not enough data for status payload");
    end

    header = this.header;
    for (int i = 0; i < $bits(status)/$bits(chdr_word_t); i++) begin
      status[i*$bits(chdr_word_t) +: $bits(chdr_word_t)] = data[i];
    end
  endfunction : read_stream_status


  // Populate this packet as a command packet
  function void ChdrPacket::write_stream_cmd (
    ref chdr_header_t      header,
    ref chdr_str_command_t command
  );
    header.pkt_type = CHDR_STRM_CMD; // Update packet type in header
    this.header = header;
    data = {};
    for (int i = 0; i < $bits(command); i += $bits(chdr_word_t)) begin
      data.push_back( command[i +: $bits(chdr_word_t)] );
    end
    update_lengths();
  endfunction : write_stream_cmd


  // Read this packet as a command packet
  function void ChdrPacket::read_stream_cmd (
    output chdr_header_t      header,
    output chdr_str_command_t command
  );
    // Make sure it's a stream command packet
    assert(this.header.pkt_type == CHDR_STRM_CMD) else begin
      $error("ChdrPacket::read_command: Packet type is not CHDR_STRM_CMD");
    end

    // Make sure we have enough payload
    assert($bits(command) <= $bits(data)) else begin
      $error("ChdrPacket::read_command: Not enough data for command payload");
    end

    header = this.header;
    for (int i = 0; i < $bits(command)/$bits(chdr_word_t); i++) begin
      command[i*$bits(chdr_word_t) +: $bits(chdr_word_t)] = data[i];
    end
  endfunction : read_stream_cmd


  // Populate this packet as a management packet
  function void ChdrPacket::write_mgmt (
    ref chdr_header_t header,
    ref chdr_mgmt_t   mgmt
  );
    header.pkt_type = CHDR_MANAGEMENT; // Update packet type in header
    this.header = header;
    data = {};

    // Insert the header
    data.push_back( mgmt.header );
    
    // Insert the ops
    foreach (mgmt.ops[i]) begin
      data.push_back( mgmt.ops[i] );
    end

    update_lengths();
  endfunction : write_mgmt


  // Read this packet as a management packet
  function void ChdrPacket::read_mgmt (
      output chdr_header_t header,
      output chdr_mgmt_t   mgmt
    );
    int num_ops;

    // Make sure it's a management packet
    assert(header.pkt_type == CHDR_MANAGEMENT) else begin
      $error("ChdrPacket::read_mgmt: Packet type is not CHDR_MANAGEMENT");
    end

    header = this.header;

    num_ops = data_bytes()/8 - 1;   // Num words, minus one for the header

    // Make sure we have enough payload
    assert(1 + num_ops <= data.size()) else begin
      $error("ChdrPacket::read_mgmt: Not enough data for management payload");
    end

    // Read the management header
    mgmt.header = data[0];

    // Read the management operations
    for (int i = 0; i < num_ops; i++) begin
      mgmt.ops.push_back(data[i+1]);
    end
  endfunction : read_mgmt


  // Populate this packet as a control packet
  function void ChdrPacket::write_ctrl (
    ref chdr_header_t      header,
    ref chdr_ctrl_header_t ctrl_header,
    ref ctrl_op_word_t     ctrl_op_word,
    ref ctrl_word_t        ctrl_data[$],
    input chdr_word_t      ctrl_timestamp = 0
  );
    bit partial_word;
    ctrl_word_t mandatory_data;
    header.pkt_type = CHDR_CONTROL; // Update packet type in header
    this.header = header;
    data = {};

    // Insert word 0 of control payload
    data.push_back(ctrl_header[63:0]);

    // Insert word 1 of control payload, if timestamp is used
    if (ctrl_header.has_time) begin
      data.push_back(ctrl_timestamp);
    end

    // Insert word 2 of control payload, the operation word
    // and first word of control data.
    mandatory_data = (ctrl_header.num_data > 0) ? ctrl_data[0] : '0;
    data.push_back({mandatory_data, ctrl_op_word[31:0]});
    // We have a half CHDR word if num_data is even
    partial_word = (ctrl_header.num_data[0] == '0);

    // Insert remaining data, if present
    for (int i = 1; i < ctrl_data.size(); i+=2) begin
      data.push_back({(partial_word ? '0 : ctrl_data[i+1]), ctrl_data[i]});
    end

    update_lengths();
  endfunction : write_ctrl


  // Read this packet as a control packet
  function void ChdrPacket::read_ctrl (
    output chdr_header_t      header,
    output chdr_ctrl_header_t ctrl_header,
    output ctrl_op_word_t     ctrl_op_word,
    output ctrl_word_t        ctrl_data[$],
    output chdr_word_t        ctrl_timestamp
  );
    chdr_word_t chdr_op_word;
    int dptr = 0;

    // Make sure it's a stream status packet
    assert(this.header.pkt_type == CHDR_CONTROL) else begin
      $error("ChdrPacket::read_status: Packet type is not CHDR_CONTROL");
    end

    // Make sure we have enough payload
    assert($bits(ctrl_header)+$bits(ctrl_op_word) <= $bits(data)) else begin
      $error("ChdrPacket::read_status: Not enough data for status payload");
    end

    header = this.header;

    // Word 0
    ctrl_header[63:0] = data[dptr++];

    // Word 1
    if (ctrl_header.has_time) begin 
      ctrl_timestamp = data[dptr++];
    end

    // Word 2, last 32-bits of control header and first word of control data
    chdr_op_word = data[dptr++];
    ctrl_op_word = chdr_op_word[31:0];
    ctrl_data.delete();
    if (ctrl_header.num_data > 0) begin
      ctrl_data[0] = chdr_op_word[63:32];
    end

    // Copy any remaining data words
    for (int i = dptr; i < data.size(); i++) begin
      ctrl_data.push_back(data[i][31:0]);
      if (i != data.size()-1 || ctrl_header.num_data[0] != 0)
        ctrl_data.push_back(data[i][63:32]);
    end
  endfunction : read_ctrl


  // Calculate the header size (including timestamp), in bytes, from the header
  // information.
  function int ChdrPacket::header_bytes();
    if (BUS_WIDTH == $bits(chdr_word_t) && header.pkt_type == CHDR_DATA_WITH_TS) begin
      header_bytes = 2 * (BUS_WIDTH / 8);  // Two words (header + timestamp)
    end else begin
      header_bytes = (BUS_WIDTH / 8);      // One word, regardless of timestamp
    end
  endfunction : header_bytes


  // Calculate the metadata size from the header information.
  function int ChdrPacket::mdata_bytes();
    mdata_bytes = header.num_mdata * BUS_WIDTH/8;
  endfunction : mdata_bytes


  // Calculate the data payload size, in bytes, from the header information
  function int ChdrPacket::data_bytes();
    data_bytes = header.length - header_bytes() - mdata_bytes();
  endfunction : data_bytes;


  // Update the length and num_mdata header fields of the packet based on the 
  // size of the metadata queue and the data queue.
  function void ChdrPacket::update_lengths();
    int num_bytes;
    int num_mdata;

    // Calculate NumMData based on the size of metadata queue
    num_mdata = metadata.size() / (BUS_WIDTH / $bits(chdr_word_t));
    if (metadata.size() % (BUS_WIDTH / $bits(chdr_word_t)) != 0) begin
      num_mdata++;
    end
    assert(num_mdata < 2**$bits(chdr_num_mdata_t)) else
      $fatal(1, "ChdrPacket::update_lengths():  Calculated NumMData exceeds maximum size");

    // Calculate the Length field
    num_bytes = data.size() * $bits(chdr_word_t) / 8;        // Payload length
    num_bytes = num_bytes + header_bytes() + mdata_bytes();  // Payload + header length
    assert(num_bytes < 2**$bits(chdr_length_t)) else
      $fatal(1, "ChdrPacket::update_lengths():  Calculated Length exceeds maximum size");

    // Update header
    header.num_mdata = num_mdata;
    header.length    = num_bytes;
  endfunction : update_lengths




  //---------------------------------------------------------------------------
  // CHDR BFM Class Methods
  //---------------------------------------------------------------------------


  // Class constructor. This must be given an interface for the master 
  // connection and an interface for the slave connection.
  function ChdrBfm::new (
    virtual AxiStreamIf #(BUS_WIDTH, USER_WIDTH).master master,
    virtual AxiStreamIf #(BUS_WIDTH, USER_WIDTH).slave  slave
  );
    super.new(master, slave);
    assert(BUS_WIDTH % 64 == 0) else begin
      $fatal(1, "ChdrBfm::new: CHDR bus width must be a multiple of 64 bits");
    end
  endfunction : new

  
  // Queue the provided packet for transmission
  task ChdrBfm::put_chdr (ChdrPacket chdr_packet);
    AxisPacket axis_packet;

    axis_packet = chdr_to_axis(chdr_packet);
    super.put(axis_packet);
  endtask : put_chdr


  // Attempt to queue the provided packet for transmission. Return 1 if 
  // successful, return 0 if the queue is full.
  function bit ChdrBfm::try_put_chdr (ChdrPacket chdr_packet);
    AxisPacket axis_packet;
    bit status;

    axis_packet = chdr_to_axis(chdr_packet);
    return super.try_put(axis_packet);
  endfunction : try_put_chdr
  

  // Get the next packet when it becomes available (wait if necessary)
  task ChdrBfm::get_chdr (output ChdrPacket chdr_packet);
    AxisPacket axis_packet;
    super.get(axis_packet);
    chdr_packet = axis_to_chdr(axis_packet);
  endtask : get_chdr


  // Get the next packet if there's one available and return 1. Return 0 if 
  // there's no packet available.
  function bit ChdrBfm::try_get_chdr (output ChdrPacket chdr_packet);
    AxisPacket axis_packet;
    if (!super.try_get(axis_packet)) return 0;
    chdr_packet = axis_to_chdr(axis_packet);
    return 1;
  endfunction : try_get_chdr


  // Get the next packet when it becomes available (wait if necessary), but 
  // don't remove it from the receive queue.
  task ChdrBfm::peek_chdr (output ChdrPacket chdr_packet);
    AxisPacket axis_packet;
    super.peek(axis_packet);
    chdr_packet = axis_to_chdr(axis_packet);
  endtask : peek_chdr


  // Get the next packet if there's one available and return 1, but don't 
  // remove it from the receive queue. Return 0 if there's no packet available.
  function bit ChdrBfm::try_peek_chdr (output ChdrPacket chdr_packet);
    AxisPacket axis_packet;
    if (!super.try_get(axis_packet)) return 0;
    chdr_packet = axis_to_chdr(axis_packet);
    return 1;
  endfunction : try_peek_chdr


  // Convert the data payload of an AXI Stream packet data structure to a CHDR 
  // packet data structure.
  function ChdrBfm::ChdrPacket ChdrBfm::axis_to_chdr (AxisPacket axis_packet);
    enum int { ST_HEADER, ST_TIMESTAMP, ST_METADATA, ST_PAYLOAD } rx_state;
    data_t word;
    int num_rx_mdata;
    ChdrPacket chdr_packet = new();

    rx_state = ST_HEADER;

    for(int i = 0; i < axis_packet.data.size(); i++) begin
      word = axis_packet.data[i];
      
      case (rx_state)
        ST_HEADER : begin
          chdr_packet.header = word[63:0];

          // Depending on the size of the word, we could have just the header 
          // or both the header and the timestamp in this word.
          if (chdr_packet.header.pkt_type == CHDR_DATA_WITH_TS) begin
            if ($bits(word) >= 128) begin
              chdr_packet.timestamp = word[127:64];
              rx_state = ST_METADATA;
            end else begin
              rx_state = ST_TIMESTAMP;
            end
          end else begin
            rx_state = ST_METADATA;
          end

          // Check if there's no metadata, in which case we can skip it
          if (rx_state == ST_METADATA && chdr_packet.header.num_mdata == 0) begin
            rx_state = ST_PAYLOAD;
          end
        end
        ST_TIMESTAMP : begin
          chdr_packet.timestamp = word;
          rx_state = (chdr_packet.header.num_mdata > 0) ? ST_METADATA : ST_PAYLOAD;
        end
        ST_METADATA : begin
          for(int w = 0; w < CHDR_PER_BUS; w++) begin
            // Grab the next chdr_word_t worth of bits
            //$display("Grabbing meta word %d (%016X)", w, word[w*$bits(chdr_word_t) +: $bits(chdr_word_t)]);
            chdr_packet.metadata.push_back(word[w*$bits(chdr_word_t) +: $bits(chdr_word_t)]);
          end
          num_rx_mdata++;
          if (num_rx_mdata == chdr_packet.header.num_mdata) rx_state = ST_PAYLOAD;
        end
        ST_PAYLOAD : begin
          for(int w = 0; w < CHDR_PER_BUS; w++) begin
            // Grab the next chdr_word_t worth of bits
            //$display("Grabbing data word %d (%016X)", w, word[w*$bits(chdr_word_t) +: $bits(chdr_word_t)]);
            chdr_packet.data.push_back(word[w*$bits(chdr_word_t) +: $bits(chdr_word_t)]);
          end
        end
      endcase

    end

    assert(rx_state == ST_PAYLOAD) else begin
      $error("ChdrBfm::axis_to_chdr: Malformed CHDR packet");
    end

    return chdr_packet;

  endfunction : axis_to_chdr


  // Convert a CHDR packet data structure to a an AXI-Stream packet data 
  // structure.
  function ChdrBfm::AxisPacket ChdrBfm::chdr_to_axis (ChdrPacket chdr_packet);
    int num_bus_words, num_chdr_words, expected_bus_words;
    data_t bus_word = 0;
    AxisPacket axis_packet = new();

    // Check that we have the right number of metadata words
    num_chdr_words = chdr_packet.metadata.size();
    num_bus_words = num_chdr_words / CHDR_PER_BUS;
    if (num_chdr_words % CHDR_PER_BUS != 0) num_bus_words++;
    assert (num_bus_words == chdr_packet.header.num_mdata) else begin
      $error("ChdrBfm::chdr_to_axis: Packet metadata size doesn't match header NumMData field");
    end

    // Calculate the number of words needed to represent this packet
    num_bus_words = 0;
    num_bus_words += chdr_packet.data.size() / CHDR_PER_BUS;
    if (chdr_packet.data.size() % CHDR_PER_BUS != 0) num_bus_words++;
    num_bus_words += chdr_packet.metadata.size() / CHDR_PER_BUS;
    if (chdr_packet.metadata.size() % CHDR_PER_BUS != 0) num_bus_words++;
    if (chdr_packet.header.pkt_type == CHDR_DATA_WITH_TS && CHDR_PER_BUS == 1) begin
      // Add two words, one for header and one for timestamp
      num_bus_words += 2;
    end else begin
      // Add one word only for header (which may or may not include a timestamp)
      num_bus_words += 1;
    end
  
    // Calculate the number of words represented by the Length field
    expected_bus_words = chdr_packet.header.length / (BUS_WIDTH/8);
    if (chdr_packet.header.length % (BUS_WIDTH/8) != 0) expected_bus_words++;

    // Make sure length field matches actual packet length
    assert (num_bus_words == expected_bus_words) else begin
      $error("ChdrBfm::chdr_to_axis: Packet size doesn't match header Length field");
    end

    // Insert header
    bus_word[63:0] = chdr_packet.header;
    if (BUS_WIDTH == 64) begin
      axis_packet.data.push_back(bus_word);
      if (chdr_packet.header.pkt_type == CHDR_DATA_WITH_TS) begin
        // Insert timestamp
        axis_packet.data.push_back(chdr_packet.timestamp);
      end
    end else begin
      // Copy the timestamp word from the header, regardless of whether or not 
      // this packet uses the timestamp field.
      bus_word[127:64] = chdr_packet.timestamp;
      axis_packet.data.push_back(bus_word);
    end

    // Insert metadata
    while (chdr_packet.metadata.size() > 0) begin
      bus_word = 0;
      for (int w = 0; w < CHDR_PER_BUS; w++) begin
        bus_word[w*$bits(chdr_word_t) +: $bits(chdr_word_t)] = chdr_packet.metadata.pop_front();
        if (chdr_packet.metadata.size() == 0) break;
      end
      axis_packet.data.push_back(bus_word);
    end

    // Insert payload
    while (chdr_packet.data.size() > 0) begin
      bus_word = 0;
      for (int word_count = 0; word_count < CHDR_PER_BUS; word_count++) begin
        bus_word[word_count*64 +: 64] = chdr_packet.data.pop_front();
        if (chdr_packet.data.size() == 0) break;
      end
      axis_packet.data.push_back(bus_word);
    end

    return axis_packet;

  endfunction : chdr_to_axis
  

endpackage : PkgChdrBfm

//
// Copyright 2020 Ettus Research, A National Instruments Brand
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

  class ChdrPacket #(parameter int CHDR_W = 64,
                     parameter int USER_WIDTH = 1);

    typedef ChdrPacket #(CHDR_W,USER_WIDTH)       ChdrPacket_t;
    typedef AxiStreamPacket #(CHDR_W, USER_WIDTH) AxisPacket_t;
    typedef ChdrData   #(CHDR_W)::chdr_word_t chdr_word_t;
    typedef AxisPacket_t::data_t data_t;

    const int BYTES_PER_CHDR_W = CHDR_W / 8;

    chdr_header_t    header;
    chdr_timestamp_t timestamp;
    chdr_word_t      metadata[$];
    chdr_word_t      data[$];

    bit disable_comparing_beyond_length = 1;

    extern function ChdrPacket_t copy();
    extern function bit          equal(ChdrPacket_t packet);
    extern function string       sprint(bit pretty = 1);
    extern function void         print(bit pretty = 1);

    // Accessors
    extern function void write_raw          (ref chdr_header_t         header,
                                             ref chdr_word_t           data[$],
                                             input chdr_word_t         metadata[$] = {},
                                             input chdr_timestamp_t    timestamp = 0,
                                             input int                 data_byte_length = -1);
    extern function void read_raw           (output chdr_header_t      header,
                                             output chdr_word_t        data[$],
                                             output chdr_word_t        metadata[$],
                                             output chdr_timestamp_t   timestamp,
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
                                             input chdr_timestamp_t    ctrl_timestamp = 0);
    extern function void read_ctrl          (output chdr_header_t      header,
                                             output chdr_ctrl_header_t ctrl_header,
                                             output ctrl_op_word_t     ctrl_op_word,
                                             output ctrl_word_t        ctrl_data[$],
                                             output chdr_timestamp_t   ctrl_timestamp);

    // Helper methods
    extern function int  header_bytes();
    extern function int  mdata_bytes();
    extern function int  data_bytes();
    extern function void update_lengths(int payload_bytes = 0);

    extern function string sprint_raw();
    extern function string sprint_pretty();

    extern function bit chdr_word_queues_equal(ref chdr_word_t a[$], 
                                               ref chdr_word_t b[$],
                                               input int data_byte_length = -1);

    // AXI-Stream/CHDR Conversion Functions
    extern function void         axis_to_chdr (AxisPacket_t axis_packet);
    extern function AxisPacket_t chdr_to_axis ();

  endclass : ChdrPacket;



  //---------------------------------------------------------------------------
  // CHDR BFM Class
  //---------------------------------------------------------------------------

  class ChdrBfm #(
    parameter int CHDR_W  = 64,
    parameter int USER_WIDTH = 1
  ) extends AxiStreamBfm #(CHDR_W, USER_WIDTH);

    typedef ChdrPacket #(CHDR_W)              ChdrPacket_t;
    typedef ChdrData   #(CHDR_W)::chdr_word_t chdr_word_t;

    const int BYTES_PER_CHDR_W = CHDR_W / 8;

    // Default fields used by high-level transaction methods
    chdr_epid_t     dst_epid;
    chdr_seq_num_t  seq_num;


    extern function new (
      virtual AxiStreamIf #(CHDR_W, USER_WIDTH).master master,
      virtual AxiStreamIf #(CHDR_W, USER_WIDTH).slave  slave
    );


    // Send Transactions
    extern task put_chdr(ChdrPacket_t chdr_packet);
    extern function bit try_put_chdr(ChdrPacket_t chdr_packet);


    // Receive Transactions
    extern task get_chdr(output ChdrPacket_t chdr_packet);
    extern function bit try_get_chdr(output ChdrPacket_t chdr_packet);
    extern task peek_chdr(output ChdrPacket_t chdr_packet);
    extern function bit try_peek_chdr(output ChdrPacket_t chdr_packet);
  endclass : ChdrBfm



  //---------------------------------------------------------------------------
  // CHDR Packet Class Methods
  //---------------------------------------------------------------------------

  // Create a copy of this packet and return a handle to the copy
  function ChdrPacket::ChdrPacket_t ChdrPacket::copy();
    ChdrPacket_t temp;
    temp = new();
    temp.header    = this.header;
    temp.timestamp = this.timestamp;
    temp.metadata  = this.metadata;
    temp.data      = this.data;
    return temp;
  endfunction


  // Return true if this packet equals that of the argument
  function bit ChdrPacket::equal(ChdrPacket_t packet);
    int payload_length;
    payload_length = header.length;
    payload_length -= BYTES_PER_CHDR_W; // subtract header bytes
    if (CHDR_W == 64) begin // subtract TS bytes if not in header word
      if (header.pkt_type == CHDR_DATA_WITH_TS) begin
        payload_length -= BYTES_PER_CHDR_W;
      end
    end
    payload_length -= packet.metadata.size() * BYTES_PER_CHDR_W; // subtract metadata length
    if (header != packet.header) return 0;
    if (!chdr_word_queues_equal(data, packet.data,payload_length)) return 0;
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
      chdr_timestamp_t tmp_ts;
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
    ref chdr_header_t      header,
    ref chdr_word_t        data[$],
    input chdr_word_t      metadata[$] = {},
    input chdr_timestamp_t timestamp = 0,
    input int              data_byte_length = -1
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
      assert((data_byte_length+(BYTES_PER_CHDR_W-1)) / BYTES_PER_CHDR_W == data.size()) else begin
        $error("ChdrPacket::write_raw: data_byte_length doesn't correspond to number of words in data");
      end

      array_num_bytes = data.size() * BYTES_PER_CHDR_W;
      this.header.length -= (array_num_bytes - data_byte_length);
    end
  endfunction : write_raw


  // Read the contents of this packet
  function void ChdrPacket::read_raw (
    output chdr_header_t    header,
    output chdr_word_t      data[$],
    output chdr_word_t      metadata[$],
    output chdr_timestamp_t timestamp,
    output int              data_byte_length
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
    for (int i = 0; i < $bits(status); i += CHDR_W) begin
      data.push_back( status[i +: CHDR_W] );
    end
    update_lengths($bits(status)/8);
  endfunction : write_stream_status


  // Read this packet as a status packet
  function void ChdrPacket::read_stream_status (
    output chdr_header_t      header,
    output chdr_str_status_t  status
  );
    // Make sure it's a stream status packet
    assert(this.header.pkt_type == CHDR_STRM_STATUS) else begin
      $error("ChdrPacket::read_stream_status: Packet type is not CHDR_STRM_STATUS");
    end

    // Make sure we have enough payload
    assert($bits(status) <= $bits(data)) else begin
      $error("ChdrPacket::read_stream_status: Not enough data for status payload");
    end

    header = this.header;
    for (int i = 0; i < $bits(status); i+= CHDR_W) begin
      status[i +: CHDR_W] = data[i/CHDR_W];
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
    for (int i = 0; i < $bits(command); i += CHDR_W) begin
      data.push_back( command[i +: CHDR_W] );
    end
    update_lengths($bits(command)/8);
  endfunction : write_stream_cmd


  // Read this packet as a command packet
  function void ChdrPacket::read_stream_cmd (
    output chdr_header_t      header,
    output chdr_str_command_t command
  );
    // Make sure it's a stream command packet
    assert(this.header.pkt_type == CHDR_STRM_CMD) else begin
      $error("ChdrPacket::read_stream_cmd: Packet type is not CHDR_STRM_CMD");
    end

    // Make sure we have enough payload
    assert($bits(command) <= $bits(data)) else begin
      $error("ChdrPacket::read_stream_cmd: Not enough data for command payload");
    end

    header = this.header;
    for (int i = 0; i < $bits(command); i += CHDR_W) begin
      command[i +: CHDR_W] = data[i/CHDR_W];
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

    num_ops = data_bytes()/BYTES_PER_CHDR_W - 1;   // Num words, minus one for the header

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
    input chdr_timestamp_t ctrl_timestamp = 0
  );
    bit partial_word;
    int byte_count = 0;
    ctrl_word_t mandatory_data;
    ChdrData #(CHDR_W, 64)::item_queue_t data64;

    header.pkt_type = CHDR_CONTROL; // Update packet type in header
    this.header = header;
    data64 = {};

    // Insert word 0 of control payload
    data64.push_back(ctrl_header);
    byte_count+=8;

    // Insert word 1 of control payload, if timestamp is used
    if (ctrl_header.has_time) begin
      data64.push_back(ctrl_timestamp);
      byte_count+=8;
    end

    // Make sure the amount of data passed matches the header
    assert (ctrl_header.num_data == ctrl_data.size()) else begin
      $error("ChdrPacket::write_ctrl: NumData doesn't match ctrl_data[] size");
    end

    // Insert word 2 of control payload, the operation word
    // and first word of control data.
    mandatory_data = (ctrl_header.num_data > 0) ? ctrl_data[0] : '0;
    data64.push_back({mandatory_data, ctrl_op_word[31:0]});
    byte_count+=8;
    // We have a half CHDR word if num_data is even
    partial_word = (ctrl_header.num_data[0] == '0);

    // Insert remaining data, if present
    for (int i = 1; i < ctrl_data.size(); i+=2) begin
      if (i == ctrl_data.size()-1) begin
        // num_data must be even in this case, so last word is half filled
        data64.push_back({ 32'b0, ctrl_data[i] });
        byte_count+=4;
      end else begin
        data64.push_back({ ctrl_data[i+1], ctrl_data[i] });
        byte_count+=8;
      end
    end

    // Convert from 64-bit words to CHDR_W-bit words
    data = ChdrData #(CHDR_W, 64)::item_to_chdr(data64);

    update_lengths(.payload_bytes(byte_count));
  endfunction : write_ctrl


  // Read this packet as a control packet
  function void ChdrPacket::read_ctrl (
    output chdr_header_t      header,
    output chdr_ctrl_header_t ctrl_header,
    output ctrl_op_word_t     ctrl_op_word,
    output ctrl_word_t        ctrl_data[$],
    output chdr_timestamp_t   ctrl_timestamp
  );
    ChdrData #(CHDR_W, 32)::item_queue_t data32;
    chdr_word_t chdr_op_word;
    int ctrl_packet_size;
    int dptr = 0;

    // Make sure it's a stream status packet
    assert(this.header.pkt_type == CHDR_CONTROL) else begin
      $error("ChdrPacket::read_ctrl: Packet type is not CHDR_CONTROL");
    end

    // Convert packet to 32-bit words for easier parsing
    data32 = ChdrData #(CHDR_W, 32)::chdr_to_item(data, data_bytes());

    // CHDR header
    header = this.header;

    // Ctrl header
    ctrl_header = { data32[dptr+1], data32[dptr] };
    dptr += 2;

    // Make sure we have enough payload. Calculate expected size in 32-bit
    // words.
    ctrl_packet_size = 3 + ctrl_header.num_data;     // header + op_word + data
    if (ctrl_header.has_time) ctrl_packet_size += 2; // timestamp
    assert (data32.size() >= ctrl_packet_size) else begin
      $error("ChdrPacket::read_ctrl: Not enough CHDR payload for control packet");
    end
    assert (data32.size() <= ctrl_packet_size) else begin
      $warning("ChdrPacket::read_ctrl: Excess CHDR payload for control packet");
    end

    // Timestamp (optional)
    if(ctrl_header.has_time) begin
      ctrl_timestamp = { data32[dptr+1], data32[dptr] };
      dptr += 2;
    end

    // Operation word
    ctrl_op_word = data32[dptr++];

    // Data words
    ctrl_data = {};
    while (dptr < data32.size()) begin
      ctrl_data.push_back(data32[dptr++]);
    end
  endfunction : read_ctrl


  // Calculate the header size (including timestamp), in bytes, from the header
  // information.
  function int ChdrPacket::header_bytes();
    if (CHDR_W == 64 && header.pkt_type == CHDR_DATA_WITH_TS) begin
      header_bytes = 2 * BYTES_PER_CHDR_W;  // Two words (header + timestamp)
    end else begin
      header_bytes = BYTES_PER_CHDR_W;      // One word, regardless of timestamp
    end
  endfunction : header_bytes


  // Calculate the metadata size from the header information.
  function int ChdrPacket::mdata_bytes();
    mdata_bytes = header.num_mdata * BYTES_PER_CHDR_W;
  endfunction : mdata_bytes


  // Calculate the data payload size, in bytes, from the header information
  function int ChdrPacket::data_bytes();
    data_bytes = header.length - header_bytes() - mdata_bytes();
  endfunction : data_bytes;


  // Update the length and num_mdata header fields of the packet based on the
  // size of the metadata queue and the data queue.
  function void ChdrPacket::update_lengths(int payload_bytes = 0);
    int num_bytes;
    int num_mdata;
    int my_payload_bytes;

    // Calculate NumMData based on the size of metadata queue
    num_mdata = metadata.size();
    assert(num_mdata < 2**$bits(chdr_num_mdata_t)) else
      $fatal(1, "ChdrPacket::update_lengths():  Calculated NumMData exceeds maximum size");

    if (payload_bytes == 0) begin
      // Calculate if optional argument not provided
      my_payload_bytes = data.size() * BYTES_PER_CHDR_W;
    end else begin
      // Use optional argument if provided
      my_payload_bytes = payload_bytes;
    end

    // Calculate the Length field
    num_bytes = header_bytes() +                             // Header
                num_mdata * BYTES_PER_CHDR_W +               // Metadata
                my_payload_bytes;                            // Payload
    assert(num_bytes < 2**$bits(chdr_length_t)) else
      $fatal(1, "ChdrPacket::update_lengths():  Calculated Length exceeds maximum size");

    // Update header
    header.num_mdata = num_mdata;
    header.length    = num_bytes;
  endfunction : update_lengths


  // Returns 1 if the queues have the same contents, up to the
  // data_byte_length, if present.
  function bit ChdrPacket::chdr_word_queues_equal(
    ref   chdr_word_t a[$],
    ref   chdr_word_t b[$],
    input int         data_byte_length = -1
  );
    chdr_word_t x, y;
    int bytes_remaining = data_byte_length;
    if (a.size() != b.size()) return 0;
    foreach (a[i]) begin
      x = a[i];
      y = b[i];
      if(data_byte_length > 0  && // only if optional argument is valid
        disable_comparing_beyond_length && // only if optional feature is enabled
        bytes_remaining < BYTES_PER_CHDR_W) begin // only compare bytes on last word
        for (int b = 0; b < bytes_remaining*8; b += 8) begin
          if (x[b+:8] !== y[b+:8]) return 0;
        end
      end else begin
        if (x !== y) return 0;
      end
      bytes_remaining -= BYTES_PER_CHDR_W;
    end
    return 1;
  endfunction : chdr_word_queues_equal


  // Convert the data payload of an AXI Stream packet data structure to a CHDR
  // packet data structure.
  function void ChdrPacket::axis_to_chdr (AxisPacket_t axis_packet);
    enum int { ST_HEADER, ST_TIMESTAMP, ST_METADATA, ST_PAYLOAD } rx_state;
    data_t word;
    int num_rx_mdata;
    int num_rx_bytes;
    ChdrPacket_t chdr_packet = new();

    rx_state = ST_HEADER;

    for(int i = 0; i < axis_packet.data.size(); i++) begin
      word = axis_packet.data[i];

      case (rx_state)
        ST_HEADER : begin
          num_rx_bytes += BYTES_PER_CHDR_W;
          header = word[63:0];

          // Depending on the size of the word, we could have just the header
          // or both the header and the timestamp in this word.
          if (header.pkt_type == CHDR_DATA_WITH_TS) begin
            if (CHDR_W >= 128) begin
              timestamp = word[127:64];
              rx_state = ST_METADATA;
            end else begin
              rx_state = ST_TIMESTAMP;
            end
          end else begin
            rx_state = ST_METADATA;
          end

          // Check if there's no metadata, in which case we can skip it
          if (rx_state == ST_METADATA && header.num_mdata == 0) begin
            rx_state = ST_PAYLOAD;
          end
        end
        ST_TIMESTAMP : begin
          num_rx_bytes += BYTES_PER_CHDR_W;
          timestamp = word;
          rx_state = (header.num_mdata > 0) ? ST_METADATA : ST_PAYLOAD;
        end
        ST_METADATA : begin
          metadata.push_back(word);
          num_rx_mdata++;
          num_rx_bytes += BYTES_PER_CHDR_W;
          if (num_rx_mdata == header.num_mdata) rx_state = ST_PAYLOAD;
        end
        ST_PAYLOAD : begin
          data.push_back(word);
          num_rx_bytes += BYTES_PER_CHDR_W;
        end
      endcase
    end

    assert (rx_state == ST_PAYLOAD) else begin
      $error("ChdrPacket::axis_to_chdr: Malformed CHDR packet");
    end

    // Check length field, noting that the last word may be partially filled
    assert (header.length >= num_rx_bytes-(BYTES_PER_CHDR_W-1) &&
            header.length <= num_rx_bytes) else begin
      $error("ChdrPacket::axis_to_chdr: Incorrect CHDR length");
    end
  endfunction : axis_to_chdr


  // Convert a CHDR packet data structure to a an AXI-Stream packet data
  // structure.
  function ChdrPacket::AxisPacket_t ChdrPacket::chdr_to_axis ();
    int num_words, expected_words;
    data_t bus_word = 0;
    AxisPacket_t axis_packet = new();

    // Check that we have the right number of metadata words
    assert (metadata.size() == header.num_mdata) else begin
      $error("ChdrPacket::chdr_to_axis: Packet metadata size doesn't match header NumMData field");
    end

    // Calculate the number of words needed to represent this packet
    num_words = data.size() + metadata.size();
    if (header.pkt_type == CHDR_DATA_WITH_TS && CHDR_W == 64) begin
      // Add two words, one for header and one for timestamp
      num_words += 2;
    end else begin
      // Add one word only for header (which may or may not include a timestamp)
      num_words += 1;
    end

    // Calculate the number of words represented by the Length field
    expected_words = header.length / BYTES_PER_CHDR_W;
    if (header.length % BYTES_PER_CHDR_W != 0) expected_words++;

    // Make sure length field matches actual packet length
    assert (num_words == expected_words) else begin
      $error("ChdrPacket::chdr_to_axis: Packet size doesn't match header Length field");
    end

    // Insert header
    bus_word[63:0] = header;
    if (CHDR_W == 64) begin
      axis_packet.data.push_back(bus_word);
      if (header.pkt_type == CHDR_DATA_WITH_TS) begin
        // Insert timestamp
        axis_packet.data.push_back(timestamp);
      end
    end else begin
      // Copy the timestamp word from the header, regardless of whether or not
      // this packet uses the timestamp field.
      bus_word[127:64] = timestamp;
      axis_packet.data.push_back(bus_word);
    end

    // Insert metadata
    foreach (metadata[i]) begin
      bus_word = metadata[i];
      axis_packet.data.push_back(bus_word);
    end

    // Insert payload
    foreach (data[i]) begin
      bus_word = data[i];
      axis_packet.data.push_back(bus_word);
    end

    return axis_packet;
  endfunction : chdr_to_axis



  //---------------------------------------------------------------------------
  // CHDR BFM Class Methods
  //---------------------------------------------------------------------------

  // Class constructor. This must be given an interface for the master
  // connection and an interface for the slave connection.
  function ChdrBfm::new (
    virtual AxiStreamIf #(CHDR_W, USER_WIDTH).master master,
    virtual AxiStreamIf #(CHDR_W, USER_WIDTH).slave  slave
  );
    super.new(master, slave);
    assert(CHDR_W % 64 == 0) else begin
      $fatal(1, "ChdrBfm::new: CHDR bus width must be a multiple of 64 bits");
    end
  endfunction : new


  // Queue the provided packet for transmission
  task ChdrBfm::put_chdr (ChdrPacket_t chdr_packet);
    AxisPacket_t axis_packet;

    axis_packet = chdr_packet.chdr_to_axis();
    super.put(axis_packet);
  endtask : put_chdr


  // Attempt to queue the provided packet for transmission. Return 1 if
  // successful, return 0 if the queue is full.
  function bit ChdrBfm::try_put_chdr (ChdrPacket_t chdr_packet);
    AxisPacket_t axis_packet;
    bit status;

    axis_packet = chdr_packet.chdr_to_axis();
    return super.try_put(axis_packet);
  endfunction : try_put_chdr


  // Get the next packet when it becomes available (wait if necessary)
  task ChdrBfm::get_chdr (output ChdrPacket_t chdr_packet);
    AxisPacket_t axis_packet;
    super.get(axis_packet);
    chdr_packet = new();
    chdr_packet.axis_to_chdr(axis_packet);
  endtask : get_chdr


  // Get the next packet if there's one available and return 1. Return 0 if
  // there's no packet available.
  function bit ChdrBfm::try_get_chdr (output ChdrPacket_t chdr_packet);
    AxisPacket_t axis_packet;
    if (!super.try_get(axis_packet)) return 0;
    chdr_packet = new();
    chdr_packet.axis_to_chdr(axis_packet);
    return 1;
  endfunction : try_get_chdr


  // Get the next packet when it becomes available (wait if necessary), but
  // don't remove it from the receive queue.
  task ChdrBfm::peek_chdr (output ChdrPacket_t chdr_packet);
    AxisPacket_t axis_packet;
    super.peek(axis_packet);
    chdr_packet = new();
    chdr_packet.axis_to_chdr(axis_packet);
  endtask : peek_chdr


  // Get the next packet if there's one available and return 1, but don't
  // remove it from the receive queue. Return 0 if there's no packet available.
  function bit ChdrBfm::try_peek_chdr (output ChdrPacket_t chdr_packet);
    AxisPacket_t axis_packet;
    if (!super.try_get(axis_packet)) return 0;
    chdr_packet = new();
    chdr_packet.axis_to_chdr(axis_packet);
    return 1;
  endfunction : try_peek_chdr

endpackage : PkgChdrBfm

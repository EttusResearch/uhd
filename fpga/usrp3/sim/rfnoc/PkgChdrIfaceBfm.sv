//
// Copyright 2020 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: PkgChdrIfaceBfm
//
// Description: This package includes a high-level bus functional model (BFM)
// for the AXIS-CHDR interface of a Transport Adapter or Stream Endpoint.
//

package PkgChdrIfaceBfm;

  import PkgChdrUtils::*;
  import PkgChdrBfm::*;


  typedef struct packed {
    chdr_vc_t        vc;
    chdr_eob_t       eob;
    chdr_eov_t       eov;
    bit              has_time;
    chdr_timestamp_t timestamp;
  } packet_info_t;


  // Return 1 if the packet info is equivalent, 0 otherwise.
  function automatic bit packet_info_equal(const ref packet_info_t a, b);
    // If there's no time then the timestamp value doesn't matter, so make them
    // the same for comparison.
    if (!a.has_time) begin
      packet_info_t a_copy = a;
      a_copy.timestamp = b.timestamp;
      return a_copy == b;
    end
    return a == b;
  endfunction : packet_info_equal


  class ChdrIfaceBfm #(CHDR_W = 64, ITEM_W = 32) extends ChdrBfm #(CHDR_W);

    // Redefine the ChdrPacket_t and chdr_word_t data types from ChdrBfm due to
    // a bug in Vivado 2019.1.
    typedef ChdrPacket #(CHDR_W)                      ChdrPacket_t;
    typedef ChdrData   #(CHDR_W, ITEM_W)::chdr_word_t chdr_word_t;
    typedef ChdrData   #(CHDR_W, ITEM_W)::item_t      item_t;

    localparam int BYTES_PER_CHDR_W = CHDR_W/8;
    localparam int BYTES_PER_ITEM_W = ITEM_W/8;

    chdr_seq_num_t seq_num;  // Sequence number
    
    protected int max_payload_length;  // Maximum number of payload bytes per packet
    protected int ticks_per_word;      // Timestamp increment per CHDR_W sized word


    // Class constructor to create a new BFM instance.
    //
    //   m_chdr:    Interface for the master connection (BFM's CHDR output)
    //   s_chdr:    Interface for the slave connection (BFM's CHDR input)
    //
    function new(
      virtual AxiStreamIf #(CHDR_W).master m_chdr,
      virtual AxiStreamIf #(CHDR_W).slave  s_chdr,
      input   int                          max_payload_length = 2**$bits(chdr_length_t),
      input   int                          ticks_per_word     = CHDR_W/ITEM_W
    );
      super.new(m_chdr, s_chdr);
      this.seq_num = 0;

      assert (CHDR_W % ITEM_W == 0) else begin
        $fatal(1, "ChdrIfaceBfm::new: CHDR_W must be a multiple of ITEM_W");
      end

      set_max_payload_length(max_payload_length);
      set_ticks_per_word(ticks_per_word);
    endfunction : new


    // Set the maximum payload size for packets. This value is used to split 
    // large send requests across multiple packets.
    //
    //   max_length:  Maximum payload length in bytes for each packet
    //
    function void set_max_payload_length(int max_payload_length);
      assert (max_payload_length % BYTES_PER_CHDR_W == 0) else begin
        $fatal(1, "ChdrIfaceBfm::set_max_payload_length: max_payload_length must be a multiple of CHDR_W in bytes");
      end
      this.max_payload_length = max_payload_length;
    endfunction


    // Return the maximum payload size for packets. This value is used to split 
    // large send requests across multiple packets.
    function int get_max_payload_length();
      return max_payload_length;
    endfunction


    // Set the timestamp ticks per CHDR_W sized word.
    //
    //   ticks_per_word:  Amount to increment the timestamp per CHDR_W sized word
    //
    function void set_ticks_per_word(int ticks_per_word);
      this.ticks_per_word = ticks_per_word;
    endfunction


    // Return the timestamp ticks per CHDR_W sized word.
    function int get_ticks_per_word();
      return ticks_per_word;
    endfunction


    // Send a CHDR data packet.
    //
    //   data:       Data words to insert into the CHDR packet.
    //   data_bytes: The number of data bytes in the CHDR packet. This
    //               is useful if the data is not a multiple of the
    //               chdr_word_t size.
    //   metadata:   Metadata words to insert into the CHDR packet. Omit this
    //               argument (or set to an empty array) to not include
    //               metadata.
    //   pkt_info:   Data structure containing packet header information.
    //
    task send (
      input chdr_word_t   data[$],
      input int           data_bytes  = -1,
      input chdr_word_t   metadata[$] = {},
      input packet_info_t pkt_info    = 0
    );
      ChdrPacket_t  chdr_packet;
      chdr_header_t chdr_header;

      // Build packet
      chdr_packet = new();
      chdr_header = '{
        vc       : pkt_info.vc,
        eob      : pkt_info.eob,
        eov      : pkt_info.eov,
        seq_num  : seq_num++,
        pkt_type : pkt_info.has_time ? CHDR_DATA_WITH_TS : CHDR_DATA_NO_TS,
        dst_epid : dst_epid,
        default  : 0
      };
      chdr_packet.write_raw(chdr_header, data, metadata, pkt_info.timestamp, data_bytes);

      // Send the packet
      put_chdr(chdr_packet);
    endtask : send


    // Send a CHDR data packet, filling the payload with items.
    //
    //   items:     Data items to insert into the CHDR packet.
    //   metadata:  Metadata words to insert into the CHDR packet. Omit this
    //              argument (or set to an empty array) to not include metadata.
    //   pkt_info:  Data structure containing packet header information.
    //
    task send_items (
      input item_t        items[$],
      input chdr_word_t   metadata[$] = {},
      input packet_info_t pkt_info    = 0
    );
      chdr_word_t data[$];
      data = ChdrData#(CHDR_W, ITEM_W)::item_to_chdr(items);
      send(data, items.size()*BYTES_PER_ITEM_W, metadata, pkt_info);
    endtask : send_items


    // Send data as one or more CHDR data packets. The input data and metadata
    // is automatically broken into max_payload_length'd packets. The
    // timestamp, if present, is set for the first packet and updated for
    // subsequent packets. The EOB and EOV are only applied to the last packet.
    //
    //   data:       Data words to insert into the CHDR packet.
    //   data_bytes: The number of data bytes in the CHDR packet. This
    //               is useful if the data is not a multiple of the
    //               chdr_word_t size.
    //   metadata:   Metadata words to insert into the CHDR packet. Omit this
    //               argument (or set to an empty array) to not include
    //               metadata.
    //   pkt_info:   Data structure containing packet header information.
    //
    task send_packets (
      input chdr_word_t   data[$],
      input int           data_bytes  = -1,
      input chdr_word_t   metadata[$] = {},
      input packet_info_t pkt_info    = 0
    );
      ChdrPacket_t    chdr_packet;
      chdr_header_t   chdr_header;
      chdr_pkt_type_t pkt_type;
      chdr_word_t     timestamp;
      int             num_pkts;
      int             payload_length;
      int             first_dword, last_dword;
      int             first_mword, last_mword;
      bit             eob, eov;
      chdr_word_t     temp_data[$];
      chdr_word_t     temp_mdata[$];

      num_pkts   = $ceil(real'(data.size()*BYTES_PER_CHDR_W) / max_payload_length);
      pkt_type   = pkt_info.has_time ? CHDR_DATA_WITH_TS : CHDR_DATA_NO_TS;
      timestamp  = pkt_info.timestamp;

      if (data_bytes < 0) data_bytes = data.size() * BYTES_PER_CHDR_W;

      // Make sure there's not too much metadata for this number of packets
      assert(metadata.size() <= num_pkts * (2**$bits(chdr_num_mdata_t)-1)) else
        $fatal(1, "ChdrIfaceBfm::send: Too much metadata for this send request");

      // Send the data, one packet at a time.
      for (int i = 0; i < num_pkts; i++) begin
        chdr_packet = new();

        // Figure out which data chunk to send next
        if (i == num_pkts-1) begin
          // The last packet, which may or may not be full-sized
          eob            = pkt_info.eob;
          eov            = pkt_info.eov;
          payload_length = data_bytes - (num_pkts-1) * max_payload_length;
          first_dword    = i*max_payload_length/BYTES_PER_CHDR_W;
          last_dword     = data.size()-1;
          first_mword    = i*(2**$bits(chdr_num_mdata_t)-1);
          last_mword     = metadata.size()-1;
        end else begin
          // A full-sized packet, not the last
          eob            = 1'b0;
          eov            = 1'b0;
          payload_length = max_payload_length;
          first_dword    = (i+0)*max_payload_length / BYTES_PER_CHDR_W;
          last_dword     = (i+1)*max_payload_length / BYTES_PER_CHDR_W - 1;
          first_mword    = (i+0)*(2**$bits(chdr_num_mdata_t)-1);
          last_mword     = (i+1)*(2**$bits(chdr_num_mdata_t)-1) - 1;
          last_mword     = last_mword > metadata.size() ? metadata.size() : last_mword;
        end

        // Build the packet
        chdr_header = '{
          vc       : pkt_info.vc,
          eob      : eob,
          eov      : eov,
          seq_num  : seq_num++,
          pkt_type : pkt_type,
          dst_epid : dst_epid,
          default  : 0
        };

        // Copy region of data and metadata to be sent in next packet
        temp_data = data[first_dword : last_dword];
        if (first_mword < metadata.size()) temp_mdata = metadata[first_mword : last_mword];
        else temp_mdata = {};

        // Build the packet
        chdr_packet.write_raw(
          chdr_header,
          temp_data, 
          temp_mdata,
          timestamp, 
          payload_length
        );

        // Send the packet
        put_chdr(chdr_packet);

        // Update timestamp for next packet (in case this is not the last)
        timestamp += max_payload_length/BYTES_PER_CHDR_W * ticks_per_word;
      end
    endtask : send_packets


    // Send one or more CHDR data packets, filling the payload with items. The
    // input data and metadata is automatically broken into
    // max_payload_length'd packets. The timestamp, if present, is set for the
    // first packet and updated for subsequent packets. The EOB and EOV are
    // only applied to the last packet.
    //
    //   items:     Data items to insert into the payload of the CHDR packets.
    //   metadata:  Metadata words to insert into the CHDR packet. Omit this
    //              argument (or set to an empty array) to not include metadata.
    //   pkt_info:  Data structure containing packet header information.
    //
    task send_packets_items (
      input item_t        items[$],
      input chdr_word_t   metadata[$] = {},
      input packet_info_t pkt_info    = 0
    );
      chdr_word_t data[$];
      data = ChdrData#(CHDR_W, ITEM_W)::item_to_chdr(items);
      send_packets(data, items.size()*BYTES_PER_ITEM_W, metadata, pkt_info);
    endtask : send_packets_items


    // Receive a CHDR data packet and extract its contents.
    //
    //   data:        Data words from the received CHDR packet.
    //   data_bytes:  The number of data bytes in the CHDR packet. This
    //                is useful if the data is not a multiple of the
    //                chdr_word_t size.
    //   metadata:    Metadata words from the received CHDR packet. This
    //                will be an empty array if there was no metadata.
    //   pkt_info:    Data structure to receive packet header information.
    //
    task recv_adv (
      output chdr_word_t   data[$],
      output int           data_bytes,
      output chdr_word_t   metadata[$],
      output packet_info_t pkt_info
    );
      ChdrPacket_t chdr_packet;
      get_chdr(chdr_packet);

      data               = chdr_packet.data;
      data_bytes         = chdr_packet.data_bytes();
      metadata           = chdr_packet.metadata;
      pkt_info.timestamp = chdr_packet.timestamp;
      pkt_info.vc        = chdr_packet.header.vc;
      pkt_info.eob       = chdr_packet.header.eob;
      pkt_info.eov       = chdr_packet.header.eov;
      pkt_info.has_time  = chdr_packet.header.pkt_type == CHDR_DATA_WITH_TS ? 1 : 0;
    endtask : recv_adv


    // Receive a CHDR data packet and extract its contents, putting the payload
    // into a queue of items.
    //
    //   items:     Items extracted from the payload of the received packet.
    //   metadata:  Metadata words from the received CHDR packet. This will be
    //              an empty array if there was no metadata.
    //   pkt_info:  Data structure to receive packet header information.
    //
    task recv_items_adv (
      output item_t        items[$],
      output chdr_word_t   metadata[$],
      output packet_info_t pkt_info
    );
      chdr_word_t data[$];
      int         data_bytes;

      recv_adv(data, data_bytes, metadata, pkt_info);
      items = ChdrData#(CHDR_W, ITEM_W)::chdr_to_item(data, data_bytes);
      assert (data_bytes % BYTES_PER_ITEM_W == 0) else begin
        $error({"ChdrIfaceBfm::recv_items_adv: ",
                "Received data was not a multiple of items"});
      end
    endtask : recv_items_adv


    // Receive a CHDR data packet and extract the data. Any metadata or
    // timestamp, if present, are discarded.
    //
    //   data:        Data words from the received CHDR packet.
    //   data_bytes:  The number of data bytes in the CHDR packet. This
    //                is useful if the data is not a multiple of the
    //                chdr_word_t size.
    //
    task recv(output chdr_word_t data[$], output int data_bytes);
      ChdrPacket_t chdr_packet;
      get_chdr(chdr_packet);
      data = chdr_packet.data;
      data_bytes = chdr_packet.data_bytes();
    endtask : recv


    // Receive a CHDR data packet and extract its payload into a queue of
    // items. Any metadata or timestamp, if present, are discarded.
    //
    //   items: Data items extracted from payload of the received CHDR packet.
    //
    task recv_items(
      output item_t items[$]
    );
      chdr_word_t data[$];
      int         data_bytes;

      recv(data, data_bytes);
      items = ChdrData#(CHDR_W, ITEM_W)::chdr_to_item(data, data_bytes);
      assert (data_bytes % BYTES_PER_ITEM_W == 0) else begin
        $error({"ChdrIfaceBfm::recv_items: ",
                "Received data was not a multiple of items"});
      end
    endtask : recv_items


    // Receive one ore more CHDR data packets and extract their contents,
    // putting the payload into a queue of items. Any metadata or timestamp, if
    // present, are discarded.
    //
    //   items:     Items extracted from the payload of the received packets.
    //   num_items: (Optional) Minimum number of items to receive. This must be
    //              provided, unless eob or eov are set. Defaults to -1, which
    //              means that the number of items is not limited.
    //   eob:       (Optional) Receive up until the next End of Burst (EOB).
    //              Default value is 1, so an entire burst is received.
    //   eov:       (Optional) Receive up until the next End of Vector (EOV).
    //
    task recv_packets_items(
      output item_t items[$],
      input int     num_items = -1,  // Receive a full burst by default
      input bit     eob       = 1,
      input bit     eov       = 0
    );
      chdr_word_t metadata[$];
      packet_info_t pkt_info;
      recv_packets_items_adv(items, metadata, pkt_info, num_items, eob, eov);
    endtask : recv_packets_items


    // Receive one or more CHDR data packets and extract their contents,
    // putting the payload into a queue of items and the metadata into a queue
    // of CHDR words.
    //
    //   items:     Items extracted from the payload of the received packets.
    //   metadata:  Metadata words from the received CHDR packets. This will be
    //              an empty array if there was no metadata.
    //   pkt_info:  Data structure to receive packet information. The
    //              timestamp, if present, will correspond to the time of the
    //              first sample, whereas vc/eob/eov will correspond to the
    //              state of the last packet.
    //   num_items: (Optional) Minimum number of items to receive. This must be
    //              provided, unless eob or eov are set. Defaults to -1, which
    //              means that the number of items is not limited.
    //   eob:       (Optional) Receive up until the next End of Burst (EOB).
    //              Default value is 1, so an entire burst is received.
    //   eov:       (Optional) Receive up until the next End of Vector (EOV).
    //
    task recv_packets_items_adv(
      output item_t        items[$],
      output chdr_word_t   metadata[$],
      output packet_info_t pkt_info,
      input int            num_items = -1,  // Receive a full burst by default
      input bit            eob       = 1,
      input bit            eov       = 0
    );
      chdr_word_t   pkt_data[$];
      chdr_word_t   pkt_metadata[$];
      int           pkt_data_bytes;
      int           item_count;
      packet_info_t time_info;
      item_t        new_items[$];

      if (num_items < 0) begin
        assert (eob || eov) else begin
          $fatal(1, {"ChdrIfaceBfm::recv_packets_items_adv: ", 
                     "eob or eov must be set when num_items is not limited"});
        end
        num_items = 32'h7FFF_FFFF;
      end

      time_info = 0;
      items = {};
      metadata = {};
      while (items.size() < num_items) begin
        // Receive the next packet
        recv_adv(pkt_data, pkt_data_bytes, pkt_metadata, pkt_info);

        if (items.size() == 0) begin
          // First packet, so grab the timestamp if it exists
          if (pkt_info.has_time) time_info = pkt_info;
        end

        // Enqueue the data
        new_items = ChdrData#(CHDR_W, ITEM_W)::chdr_to_item(pkt_data, pkt_data_bytes);
        items = {items, new_items};
        assert (pkt_data_bytes % BYTES_PER_ITEM_W == 0) else begin
          $error({"ChdrIfaceBfm::recv_packets_items_adv: ",
                  "Received data was not a multiple of items"});
        end

        // Enqueue the metadata
        metadata = {metadata, pkt_metadata};

        if ((eob && pkt_info.eob) || (eov && pkt_info.eov)) break;
      end
      
      // Restore timestamp from the first packet
      pkt_info.has_time  = time_info.has_time;
      pkt_info.timestamp = time_info.timestamp;
    endtask : recv_packets_items_adv

  endclass : ChdrIfaceBfm


endpackage : PkgChdrIfaceBfm

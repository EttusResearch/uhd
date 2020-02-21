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


  class ChdrIfaceBfm #(CHDR_W = 64) extends ChdrBfm #(CHDR_W);
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
      input   int                          ticks_per_word     = CHDR_W/32
    );
      super.new(m_chdr, s_chdr);
      this.seq_num = 0;
      set_max_payload_length(max_payload_length);
      set_ticks_per_word(ticks_per_word);
    endfunction : new


    // Set the maximum payload size for packets. This value is used to split 
    // large send requests across multiple packets.
    //
    //   max_length:  Maximum payload length in bytes for each packet
    //
    function void set_max_payload_length(int max_payload_length);
      assert (max_payload_length % (CHDR_W/8) == 0) else begin
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
      ChdrPacket      chdr_packet;
      chdr_header_t   chdr_header;

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


    // Send data as one or more CHDR data packets. The input data and metadata 
    // is automatically broken into max_payload_length'd packets. If multiple 
    // packets are needed, EOB and EOV are only applied to the last packet.
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
      ChdrPacket      chdr_packet;
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

      num_pkts   = $ceil(real'(data.size()*($bits(chdr_word_t)/8)) / max_payload_length);
      pkt_type   = pkt_info.has_time ? CHDR_DATA_WITH_TS : CHDR_DATA_NO_TS;
      timestamp  = pkt_info.timestamp;

      // Make sure there's not too much metadata for this number of packets
      assert(metadata.size()*$bits(chdr_word_t) < num_pkts * 2**$bits(chdr_num_mdata_t) * CHDR_W) else
        $fatal(1, "ChdrIfaceBfm::send: Too much metadata for this send request");

      // Send the data, one packet at a time.
      for (int i = 0; i < num_pkts; i++) begin
        chdr_packet = new();

        // Figure out which data chunk to send next
        if (i == num_pkts-1) begin
          // The last packet, which may or may not be full-sized
          eob            = pkt_info.eob;
          eov            = pkt_info.eov;
          payload_length = (data_bytes < 0) ? data_bytes : data_bytes % max_payload_length;
          first_dword    = i*max_payload_length/($bits(chdr_word_t)/8);
          last_dword     = data.size()-1;
          first_mword    = i*(2**$bits(chdr_num_mdata_t) * CHDR_W / $bits(chdr_word_t));
          last_mword     = metadata.size()-1;
        end else begin
          // A full-sized packet, not the last
          eob            = 1'b0;
          eov            = 1'b0;
          payload_length = max_payload_length;
          first_dword    = (i+0)*max_payload_length / ($bits(chdr_word_t)/8);
          last_dword     = (i+1)*max_payload_length / ($bits(chdr_word_t)/8) - 1;
          first_mword    = (i+0)*(2**$bits(chdr_num_mdata_t) * CHDR_W / $bits(chdr_word_t));
          last_mword     = (i+1)*(2**$bits(chdr_num_mdata_t) * CHDR_W / $bits(chdr_word_t)) - 1;
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
        timestamp += max_payload_length/(CHDR_W/8) * ticks_per_word;
      end
    endtask : send_packets


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
      ChdrPacket chdr_packet;
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


    // Receive a CHDR data packet and extract the data. Any metadata or
    // timestamp, if present, are discarded.
    //
    //   data:        Data words from the received CHDR packet.
    //   data_bytes:  The number of data bytes in the CHDR packet. This
    //                is useful if the data is not a multiple of the
    //                chdr_word_t size.
    //
    task recv(output chdr_word_t data[$], output int data_bytes);
      ChdrPacket chdr_packet;
      get_chdr(chdr_packet);
      data = chdr_packet.data;
      data_bytes = chdr_packet.data_bytes();
    endtask : recv

  endclass : ChdrIfaceBfm


endpackage : PkgChdrIfaceBfm

//
// Copyright 2020 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: PkgCtrlIfaceBfm
//
// Description: This package includes high-level bus functional models (BFM)
// for the AXIS-Ctrl interface of a Stream Endpoint.
//


package PkgCtrlIfaceBfm;

  import PkgChdrUtils::*;
  import PkgAxisCtrlBfm::*;


  class CtrlIfaceBfm extends AxisCtrlBfm;
    ctrl_port_t    dst_port;
    ctrl_port_t    src_port;
    ctrl_seq_num_t seq_num;

    // Class constructor to create a new BFM instance.
    //
    //   m_chdr:    Interface for the master connection (BFM's AXIS output)
    //   s_chdr:    Interface for the slave connection (BFM's AXIS input)
    //   src_port:  Source port to use in generated control packets
    //
    function new(
      virtual AxiStreamIf #(32).master m_chdr,
      virtual AxiStreamIf #(32).slave  s_chdr,
      ctrl_port_t dst_port,
      ctrl_port_t src_port
    );
      super.new(m_chdr, s_chdr);
      this.dst_port = dst_port;
      this.src_port = src_port;
      this.seq_num  = '0;
    endfunction : new


    // Send an AXIS-Ctrl read request packet and get the response.
    //
    //   addr:       Address for the read request
    //   word:       Data word that was returned in response to the read
    //
    task reg_read (
      input  ctrl_address_t addr,
      output ctrl_word_t    word
    );
      AxisCtrlPacket ctrl_packet;

      // Create the AXIS-Ctrl packet
      ctrl_packet = new();
      ctrl_packet.header = '{
        seq_num  : seq_num++,
        num_data : 1,
        src_port : src_port,
        dst_port : dst_port,
        default  : 0
      };
      ctrl_packet.op_word = '{
        status      : CTRL_STS_OKAY,
        op_code     : CTRL_OP_READ,
        byte_enable : '1,
        address     : addr,
        default     : 0
      };
      ctrl_packet.data = { 0 };

      // Send the control packet and get the response
      put_ctrl(ctrl_packet);
      get_ctrl(ctrl_packet);
      word = ctrl_packet.data[0];

      assert(ctrl_packet.header.is_ack == 1 &&
             ctrl_packet.op_word.status == CTRL_STS_OKAY) else begin
        $fatal(1, "CtrlIfaceBfm::reg_read: Did not receive CTRL_STS_OKAY status");
      end
    endtask : reg_read


    // Send an AXIS-Ctrl write request packet and get the response.
    //
    //   addr:       Address for the write request
    //   word:       Data word to write
    //
    task reg_write (
      ctrl_address_t addr,
      ctrl_word_t    word
    );
      AxisCtrlPacket ctrl_packet;

      // Create the AXIS-Ctrl packet
      ctrl_packet = new();
      ctrl_packet.header = '{
        seq_num  : seq_num++,
        num_data : 1,
        src_port : src_port,
        dst_port : dst_port,
        default  : 0
      };
      ctrl_packet.op_word = '{
        status      : CTRL_STS_OKAY,
        op_code     : CTRL_OP_WRITE,
        byte_enable : '1,
        address     : addr,
        default     : 0
      };

      // Send the packet and get the response
      ctrl_packet.data = { word };
      put_ctrl(ctrl_packet);
      get_ctrl(ctrl_packet);
      word = ctrl_packet.data[0];

      assert(ctrl_packet.header.is_ack == 1 &&
             ctrl_packet.op_word.status == CTRL_STS_OKAY) else begin
        $fatal(1, "CtrlIfaceBfm::reg_write: Did not receive CTRL_STS_OKAY status");
      end
    endtask : reg_write

  endclass : CtrlIfaceBfm


endpackage : PkgCtrlIfaceBfm

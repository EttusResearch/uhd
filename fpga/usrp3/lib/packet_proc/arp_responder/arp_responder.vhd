--
-- Copyright 2019 Ettus Research, A National Instruments brand
--
-- SPDX-License-Identifier: LGPL-3.0
--
-- Module: arp_responder
-- Description: Processing IP to send replies for ARP frames (for IPv4)
-- arp_responder checks the incoming ARP frame against the input port ip_addr,
-- and if the frame is a request for this module's ip_addr, the module will
-- format an ARP reply and send it on the outgoing AXI-S interface.
--
-- mac_addr and ip_addr must be kept stable for this module to function. They
-- are not registered within the IP.
--
-- s_axis_tuser indicates there is an error in the packet, and it should be discarded

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity arp_responder is
port (
  aclk          : in std_logic;
  aresetn       : in std_logic;
  mac_addr      : in std_logic_vector(47 downto 0);
  ip_addr       : in std_logic_vector(31 downto 0);
  s_axis_tdata  : in std_logic_vector(63 downto 0);
  s_axis_tvalid : in std_logic;
  s_axis_tready : out std_logic;
  s_axis_tkeep  : in std_logic_vector(7 downto 0);
  s_axis_tlast  : in std_logic;
  s_axis_tuser  : in std_logic;
  m_axis_tdata  : out std_logic_vector(63 downto 0);
  m_axis_tvalid : out std_logic;
  m_axis_tready : in std_logic;
  m_axis_tkeep  : out std_logic_vector(7 downto 0);
  m_axis_tlast  : out std_logic;
  m_axis_tuser  : out std_logic
);
end arp_responder;

architecture arch of arp_responder is
  type pkt_state_t is (PKT_IDLE, PKT_RECV, PKT_SEND, PKT_DROP);
  signal pkt_state : pkt_state_t;
  signal pkt_recv_count : unsigned(3 downto 0);
  signal pkt_send_count : unsigned(3 downto 0);

  --All of these are LSB-0 for bits, but first byte transmitted is byte 0
  --  In ChipScope, bytes will appear swapped versus typical diagrams
  signal src_mac_be : std_logic_vector(47 downto 0);
  signal sender_hw_addr_be : std_logic_vector(47 downto 0);
  signal sender_protocol_addr_be : std_logic_vector(31 downto 0);
  signal target_protocol_addr_be : std_logic_vector(31 downto 0);
  signal ip_addr_be : std_logic_vector(31 downto 0);
begin
  m_axis_tuser <= '0';
  m_axis_tvalid <= '1' when (pkt_state = PKT_SEND) else '0';
  ip_addr_be <= ip_addr(7 downto 0) & ip_addr(15 downto 8) &
                ip_addr(23 downto 16) & ip_addr(31 downto 24);

  s_axis_tready <= '1' when (pkt_state = PKT_IDLE) or (pkt_state = PKT_RECV) or (pkt_state = PKT_DROP)
                   else '0';

  tx_reply : process (src_mac_be, mac_addr, ip_addr, sender_hw_addr_be,
                      sender_protocol_addr_be, pkt_send_count)
  begin
    m_axis_tdata <= (others => 'X');
    m_axis_tkeep <= (others => '1');
    m_axis_tlast <= '0';
    case (to_integer(pkt_send_count)) is
    when 0 =>
      m_axis_tdata(47 downto 0)  <= src_mac_be;
      m_axis_tdata(63 downto 48) <= mac_addr(39 downto 32) & mac_addr(47 downto 40);
      m_axis_tkeep <= X"FF";
      m_axis_tlast <= '0';
    when 1 =>
      m_axis_tdata(31 downto 0)  <= mac_addr(7 downto 0) & mac_addr(15 downto 8) &
                                    mac_addr(23 downto 16) & mac_addr(31 downto 24);
      m_axis_tdata(47 downto 32) <= X"0608";
      m_axis_tdata(63 downto 48) <= X"0100";
      m_axis_tkeep <= X"FF";
      m_axis_tlast <= '0';
    when 2 =>
      m_axis_tdata(15 downto 0)  <= X"0008"; --PTYPE
      m_axis_tdata(23 downto 16) <= X"06";   --HLEN
      m_axis_tdata(31 downto 24) <= X"04";   --PLEN
      m_axis_tdata(47 downto 32) <= X"0200"; --OPER
      m_axis_tdata(63 downto 48) <= mac_addr(39 downto 32) & mac_addr(47 downto 40); --SHA
      m_axis_tkeep <= X"FF";
      m_axis_tlast <= '0';
    when 3 =>
      m_axis_tdata(31 downto 0)  <= mac_addr(7 downto 0) & mac_addr(15 downto 8) &
                                    mac_addr(23 downto 16) & mac_addr(31 downto 24); --SHA
      m_axis_tdata(63 downto 32) <= ip_addr(7 downto 0) & ip_addr(15 downto 8) &
                                    ip_addr(23 downto 16) & ip_addr(31 downto 24);  --SPA
      m_axis_tkeep <= X"FF";
      m_axis_tlast <= '0';
    when 4 =>
      m_axis_tdata(47 downto 0)  <= sender_hw_addr_be; --THA
      m_axis_tdata(63 downto 48) <= sender_protocol_addr_be(15 downto 0); --TPA
      m_axis_tkeep <= X"FF";
      m_axis_tlast <= '0';
    when 5 =>
      m_axis_tdata(15 downto 0)  <= sender_protocol_addr_be(31 downto 16); --TPA
      m_axis_tdata(63 downto 16) <= (others => '0');
      m_axis_tkeep <= X"03";
      m_axis_tlast <= '1';
    when others =>
      null;
    end case;
  end process;

  process (aclk)
    variable pkt_nonmatch : boolean := false;
  begin
  if rising_edge(aclk) then
    case (pkt_state) is
    when PKT_IDLE =>
      if (s_axis_tvalid = '1') and (s_axis_tlast = '0') and (s_axis_tuser = '0') then
        pkt_state <= PKT_RECV;
        pkt_recv_count <= to_unsigned(1, pkt_recv_count'length);
        src_mac_be(15 downto 0) <= s_axis_tdata(63 downto 48);
      end if;
    when PKT_RECV =>
      pkt_nonmatch := false;

      pkt_send_count <= to_unsigned(0, pkt_send_count'length);

      if (s_axis_tvalid = '1') and (s_axis_tuser = '1') then
        pkt_nonmatch := true;
      elsif (s_axis_tvalid = '1') and (s_axis_tuser = '0') then
        if (to_integer(pkt_recv_count) < 7) then
          pkt_recv_count <= pkt_recv_count + 1;
        end if;

        case (to_integer(pkt_recv_count)) is
        when 1 =>
          src_mac_be(47 downto 16) <= s_axis_tdata(31 downto 0);
          if (s_axis_tdata(47 downto 32) /= X"0608") or --eth_type
             (s_axis_tdata(63 downto 48) /= X"0100") or --HTYPE
             (s_axis_tlast = '1') then
            pkt_nonmatch := true;
          end if;
        when 2 =>
          sender_hw_addr_be(15 downto 0) <= s_axis_tdata(63 downto 48);
          if (s_axis_tdata(15 downto 0) /= X"0008") or --PTYPE
             (s_axis_tdata(23 downto 16) /= X"06") or --HLEN
             (s_axis_tdata(31 downto 24) /= X"04") or --PLEN
             (s_axis_tdata(47 downto 32) /= X"0100") or --OPER
             (s_axis_tlast = '1') then
            pkt_nonmatch := true;
          end if;
        when 3 =>
          sender_hw_addr_be(47 downto 16) <= s_axis_tdata(31 downto 0);
          sender_protocol_addr_be <= s_axis_tdata(63 downto 32);
          if (s_axis_tlast = '1') then
            pkt_nonmatch := true;
          end if;
        when 4 =>
          --THA = s_axis_tdata(47 downto 0)
          target_protocol_addr_be(15 downto 0) <= s_axis_tdata(63 downto 48);
          if (s_axis_tdata(63 downto 48) /= ip_addr_be(15 downto 0)) or
             (s_axis_tlast = '1') then
            pkt_nonmatch := true;
          end if;
        when 5 =>
          target_protocol_addr_be(31 downto 16) <= s_axis_tdata(15 downto 0);
          if (s_axis_tdata(15 downto 0) /= ip_addr_be(31 downto 16)) then
            pkt_nonmatch := true;
          end if;
        when others =>
          null;
        end case;
      end if;
      if (pkt_nonmatch) then
        if (s_axis_tlast = '1') then
          pkt_state <= PKT_IDLE;
        else
          pkt_state <= PKT_DROP;
        end if;
      elsif (s_axis_tlast = '1') then
        pkt_state <= PKT_SEND;
      end if;
    when PKT_SEND =>
      if (m_axis_tready = '1') then
        pkt_send_count <= pkt_send_count + 1;

        if (pkt_send_count = 5) then
          pkt_state <= PKT_IDLE;
        end if;
      end if;
    when PKT_DROP =>
      if (s_axis_tvalid = '1') and (s_axis_tlast = '1') then
        pkt_state <= PKT_IDLE;
      end if;
    end case;

    if aresetn = '0' then
      pkt_state <= PKT_IDLE;
    end if;
  end if;
  end process;
end arch;


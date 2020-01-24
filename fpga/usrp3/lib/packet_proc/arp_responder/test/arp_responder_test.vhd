--
-- Copyright 2019 Ettus Research, A National Instruments brand
--
-- SPDX-License-Identifier: LGPL-3.0
--
-- Module: arp_responder_test
-- Description: Simulation module to check the arp_responder IP
-- Sends a request to the arp_responder and checks for the expected reply

library ieee;
use ieee.std_logic_1164.all;

library work;
use work.arp_responder;

entity arp_responder_test is
end arp_responder_test;

architecture sim of arp_responder_test is
  signal test_fail     : boolean := false;
  signal aclk          : std_logic := '0';
  signal aresetn       : std_logic;
  signal mac_addr      : std_logic_vector(47 downto 0) := X"017136E7BE02";
  signal ip_addr       : std_logic_vector(31 downto 0) := X"04030201";
  signal s_axis_tdata  : std_logic_vector(63 downto 0);
  signal s_axis_tvalid : std_logic;
  signal s_axis_tready : std_logic;
  signal s_axis_tkeep  : std_logic_vector(7 downto 0);
  signal s_axis_tlast  : std_logic;
  signal s_axis_tuser  : std_logic;
  signal m_axis_tdata  : std_logic_vector(63 downto 0);
  signal m_axis_tvalid : std_logic;
  signal m_axis_tready : std_logic;
  signal m_axis_tkeep  : std_logic_vector(7 downto 0);
  signal m_axis_tlast  : std_logic;
  signal m_axis_tuser  : std_logic;

  constant HALFCYCLE : time := 4 ns;
  constant CYCLE : time := 2*HALFCYCLE;
  constant ARP_REQUEST_VECTOR : std_logic_vector(64*8-1 downto 0) :=
      X"0000000000000000" & --Padding
      X"0000000000000000" & --Padding
      X"000000000000" & --Packet filler
      X"04030201" &   --
      X"000000000000" &
      X"020B010A" &
      X"00D00D010101" &
      X"0100" &
      X"04" &
      X"06" &
      X"0008" &
      X"0100" &
      X"0608" &
      X"00D00D010101" &
      X"FFFFFFFFFFFF";
  constant ARP_REPLY_VECTOR : std_logic_vector(64*8-1 downto 0) :=
      X"0000000000000000" & --Padding
      X"0000000000000000" & --Padding
      X"000000000000" & --Packet filler
      X"020B010A" &
      X"00D00D010101" &
      X"04030201" &   --
      X"017136E7BE02" &
      X"0200" &
      X"04" &
      X"06" &
      X"0008" &
      X"0100" &
      X"0608" &
      X"017136E7BE02" &
      X"00D00D010101";
  --Dest --6
  --Src  --6
  --Ethertype --2
  --HTYPE = 0x0001 --2
  --PTYPE = 0x0800 --2
  --HLEN  = 0x06 --1
  --PLEN  = 0x04 --1
  --OPER  = 0x0001 --2
  --SHA   = --6
  --SPA   = --4
  --THA   = --6
  --TPA   = --4
  --Need to check...
  --  ARP request to us
  --  ARP request not to us
  --  Malformed packet
begin

  process
  begin
    wait for HALFCYCLE;
    aclk <= not aclk;
  end process;

  process
  begin
    wait for CYCLE;
    aresetn <= '0';
    wait for 3*CYCLE;
    aresetn <= '1';
    s_axis_tvalid <= '0';
    s_axis_tkeep <= X"00";
    s_axis_tlast <= '0';
    s_axis_tuser <= '0';
    m_axis_tready <= '0';
    wait for CYCLE;
    for i in 0 to 7 loop
      s_axis_tdata <= ARP_REQUEST_VECTOR(64*i+63 downto 64*i);
      s_axis_tkeep <= X"FF";
      s_axis_tvalid <= '1';
      if (i = 7) then
        s_axis_tlast <= '1';
      else
        s_axis_tlast <= '0';
      end if;
      if (i >= 3) then
        s_axis_tuser <= '1';
      else
        s_axis_tuser <= '0';
      end if;
      wait for CYCLE;
      s_axis_tvalid <= '0';
      wait for 7*CYCLE;
      --wait until s_axis_tready = '1';
    end loop;
    wait for CYCLE;
    s_axis_tvalid <= '0';
    s_axis_tuser <= '0';
    wait for CYCLE;
    for i in 0 to 7 loop
      s_axis_tdata <= ARP_REQUEST_VECTOR(64*i+63 downto 64*i);
      s_axis_tkeep <= X"FF";
      s_axis_tvalid <= '1';
      if (i = 7) then
        s_axis_tlast <= '1';
      else
        s_axis_tlast <= '0';
      end if;
      wait for CYCLE;
      s_axis_tvalid <= '0';
      wait for 7*CYCLE;
      --wait until s_axis_tready = '1';
    end loop;
    wait for CYCLE;
    s_axis_tvalid <= '0';
    wait for CYCLE;
    for i in 0 to 7 loop
      m_axis_tready <= '1';
      if (m_axis_tdata /= ARP_REPLY_VECTOR(64*i+63 downto 64*i)) then
        test_fail <= true;
        report "Reply vector mismatch";
      end if;
      wait for CYCLE;
      m_axis_tready <= '0';
      wait for 7*CYCLE;
    end loop;
    wait for CYCLE;
    if (test_fail) then
      report "Test FAILED" severity failure;
    else
      report "PASS: End of test" severity failure;
    end if;
  end process;

dut : entity arp_responder
port map (
  aclk          => aclk,
  aresetn       => aresetn,
  mac_addr      => mac_addr,
  ip_addr       => ip_addr,
  s_axis_tdata  => s_axis_tdata,
  s_axis_tvalid => s_axis_tvalid,
  s_axis_tready => s_axis_tready,
  s_axis_tkeep  => s_axis_tkeep,
  s_axis_tlast  => s_axis_tlast,
  s_axis_tuser  => s_axis_tuser,
  m_axis_tdata  => m_axis_tdata,
  m_axis_tvalid => m_axis_tvalid,
  m_axis_tready => m_axis_tready,
  m_axis_tkeep  => m_axis_tkeep,
  m_axis_tlast  => m_axis_tlast,
  m_axis_tuser  => m_axis_tuser
);
end sim;

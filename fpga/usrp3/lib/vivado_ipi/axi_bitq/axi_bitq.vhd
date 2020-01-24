--
-- Copyright 2018 Ettus Research, A National Instruments Company
--
-- SPDX-License-Identifier: LGPL-3.0
--
-- Module: axi_bitq
-- Description: Simple IP to shift bits in/out (primarily for JTAG)
-- axi_bitq is the processor interface to the bitq_fsm module

library ieee;
use ieee.std_logic_1164.all;

library work;
use work.bitq_fsm;

entity axi_bitq is
port (
  bit_clk  : inout std_logic;
  bit_in   : in    std_logic;
  bit_out  : inout std_logic;
  bit_stb  : inout std_logic;

  S_AXI_ACLK     : in  std_logic;
  S_AXI_ARESETN  : in  std_logic;
  S_AXI_AWADDR   : in  std_logic_vector(3 downto 0);
  S_AXI_AWVALID  : in  std_logic;
  S_AXI_AWREADY  : out std_logic;
  S_AXI_WDATA    : in  std_logic_vector(31 downto 0);
  S_AXI_WSTRB    : in  std_logic_vector(3 downto 0);
  S_AXI_WVALID   : in  std_logic;
  S_AXI_WREADY   : out std_logic;
  S_AXI_BRESP    : out std_logic_vector(1 downto 0);
  S_AXI_BVALID   : out std_logic;
  S_AXI_BREADY   : in  std_logic;
  S_AXI_ARADDR   : in  std_logic_vector(3 downto 0);
  S_AXI_ARVALID  : in  std_logic;
  S_AXI_ARREADY  : out std_logic;
  S_AXI_RDATA    : out std_logic_vector(31 downto 0);
  S_AXI_RRESP    : out std_logic_vector(1 downto 0);
  S_AXI_RVALID   : out std_logic;
  S_AXI_RREADY   : in  std_logic
);
end axi_bitq;

architecture arch of axi_bitq is
  signal read_token       : std_logic;
  signal write_addr       : std_logic_vector(3 downto 0);
  signal write_strb       : std_logic_vector(3 downto 0);
  signal write_addr_token : std_logic;
  signal write_data       : std_logic_vector(31 downto 0);
  signal write_data_token : std_logic;

  signal wr_data          : std_logic_vector(31 downto 0);
  signal stb_data         : std_logic_vector(31 downto 0);
  signal rd_data          : std_logic_vector(31 downto 0);
  signal prescalar        : std_logic_vector(7 downto 0);
  signal len              : std_logic_vector(4 downto 0);
  signal ready            : std_logic;
  signal start            : std_logic;
  signal bitq_rstn        : std_logic;
  signal bitq_soft_rst    : std_logic;

begin

  S_AXI_ARREADY <= not read_token;
  S_AXI_RVALID <= read_token;
  S_AXI_RRESP <= "00";

  S_AXI_AWREADY <= not write_addr_token;
  S_AXI_WREADY <= not write_data_token;
  S_AXI_BVALID <= write_addr_token and write_data_token;
  S_AXI_BRESP <= "00";

  --Register reads
  read_proc : process (S_AXI_ACLK)
    variable read_addr : std_logic_vector(S_AXI_ARADDR'left downto S_AXI_ARADDR'right+2);
  begin
  if rising_edge(S_AXI_ACLK) then
    read_addr := S_AXI_ARADDR(S_AXI_ARADDR'left downto S_AXI_ARADDR'right+2);

    if (S_AXI_ARESETN = '0') then
      read_token <= '0';
    elsif (S_AXI_ARVALID = '1') and (read_token = '0') then
      read_token <= '1';
    elsif (S_AXI_RREADY = '1') and (read_token = '1') then
      read_token <= '0';
    end if;

    if (S_AXI_ARVALID = '1') and (read_token = '0') then
      S_AXI_RDATA <= (others => '0');

      case read_addr is
      when "00" => 
        S_AXI_RDATA(31 downto 0) <= wr_data;
      when "01" =>
        S_AXI_RDATA(31 downto 0) <= stb_data;
      when "10" =>
        S_AXI_RDATA(7 downto 0) <= prescalar;
        S_AXI_RDATA(12 downto 8) <= len;
        S_AXI_RDATA(31) <= ready;
      when "11" =>
        S_AXI_RDATA(31 downto 0) <= rd_data;
      when others =>
        null;
      end case;

    end if;
  end if;
  end process read_proc;

  write_proc : process (S_AXI_ACLK)
  begin
  if rising_edge(S_AXI_ACLK) then
    if (S_AXI_ARESETN = '0') then
      write_addr_token <= '0';
      write_data_token <= '0';
      write_strb <= (others => '0');
    else
      if (S_AXI_AWVALID = '1') and (write_addr_token = '0') then
        write_addr_token <= '1';
      elsif (S_AXI_BREADY = '1') and (write_addr_token = '1') and (write_data_token = '1') then
        write_addr_token <= '0';
      end if;

      if (S_AXI_WVALID = '1') and (write_data_token = '0') then
        write_data_token <= '1';
      elsif (S_AXI_BREADY = '1') and (write_addr_token = '1') and (write_data_token = '1') then
        write_data_token <= '0';
      end if;
    end if;

    if (S_AXI_AWVALID = '1') and (write_addr_token = '0') then
      write_addr <= S_AXI_AWADDR;
    end if;

    if (S_AXI_WVALID = '1') and (write_data_token = '0') then
      write_data <= S_AXI_WDATA;
      write_strb <= S_AXI_WSTRB;
    end if;
  end if;
  end process write_proc;

  write_reg : process (S_AXI_ACLK)
  begin
  if rising_edge(S_AXI_ACLK) then
    bitq_soft_rst <= '0';
    start <= '0';

    if (S_AXI_ARESETN = '0') or (bitq_soft_rst = '1') then
      bitq_soft_rst <= '0';
      start <= '0';
    elsif (write_addr_token = '1') and (write_data_token = '1') then
      case write_addr(write_addr'left downto 2) is
      when "00" => 
        if (write_strb(0) = '1') and (ready = '1') then
          wr_data(7 downto 0) <= write_data(7 downto 0);
        end if;
        if (write_strb(1) = '1') and (ready = '1') then
          wr_data(15 downto 8) <= write_data(15 downto 8);
        end if;
        if (write_strb(2) = '1') and (ready = '1') then
          wr_data(23 downto 16) <= write_data(23 downto 16);
        end if;
        if (write_strb(3) = '1') and (ready = '1') then
          wr_data(31 downto 24) <= write_data(31 downto 24);
        end if;
      when "01" => 
        if (write_strb(0) = '1') and (ready = '1') then
          stb_data(7 downto 0) <= write_data(7 downto 0);
        end if;
        if (write_strb(1) = '1') and (ready = '1') then
          stb_data(15 downto 8) <= write_data(15 downto 8);
        end if;
        if (write_strb(2) = '1') and (ready = '1') then
          stb_data(23 downto 16) <= write_data(23 downto 16);
        end if;
        if (write_strb(3) = '1') and (ready = '1') then
          stb_data(31 downto 24) <= write_data(31 downto 24);
        end if;
      when "10" =>
        if (write_strb(0) = '1') and (ready = '1') then
          prescalar <= write_data(7 downto 0);
        end if;
        if (write_strb(1) = '1') and (ready = '1') then
          len <= write_data(12 downto 8);
          if (write_strb(3) = '0') or (write_data(31) = '0') then
            start <= '1';
          end if;
        end if;
        if (write_strb(3) = '1') then
          bitq_soft_rst <= write_data(31);
        end if;
      when "11" => --Read only register
        null;
      when others =>
        null;
      end case;
    end if;
  end if;
  end process write_reg;

  bitq_rstn <= '0' when (S_AXI_ARESETN = '0') or (bitq_soft_rst = '1') else '1';

  bitq_ctrl : entity bitq_fsm
  port map (
    clk  => S_AXI_ACLK,
    rstn => S_AXI_ARESETN,
    prescalar => prescalar,
  
    bit_clk  => bit_clk,
    bit_in   => bit_in,
    bit_out  => bit_out,
    bit_stb  => bit_stb,
    start    => start,
    len      => len,
    ready    => ready,
    wr_data  => wr_data,
    stb_data => stb_data,
    rd_data  => rd_data
  );

end arch;


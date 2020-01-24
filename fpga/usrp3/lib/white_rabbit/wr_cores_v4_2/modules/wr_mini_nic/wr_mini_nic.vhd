-------------------------------------------------------------------------------
-- Title      : Mini Embedded DMA Network Interface Controller
-- Project    : WhiteRabbit Core
-------------------------------------------------------------------------------
-- File       : wrsw_mini_nic.vhd
-- Author     : Grzegorz Daniluk, Tomasz Wlostowski
-- Company    : CERN BE-Co-HT
-- Created    : 2010-07-26
-- Last update: 2017-02-03
-- Platform   : FPGA-generic
-- Standard   : VHDL
-------------------------------------------------------------------------------
-- Description: Module implements a simple NIC with DMA controller. It
-- sends/receives the packets using WR switch fabric interface (see the
-- wrsw_endpoint.vhd for the details). Packets are stored and read from the
-- system memory via simple memory bus. WR endpoint-compatible TX timestamping
-- unit is also included.
-------------------------------------------------------------------------------
--
-- Copyright (c) 2010-2016 CERN
--
-- This source file is free software; you can redistribute it   
-- and/or modify it under the terms of the GNU Lesser General   
-- Public License as published by the Free Software Foundation; 
-- either version 2.1 of the License, or (at your option) any   
-- later version.                                               
--
-- This source is distributed in the hope that it will be       
-- useful, but WITHOUT ANY WARRANTY; without even the implied   
-- warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      
-- PURPOSE.  See the GNU Lesser General Public License for more 
-- details.                                                     
--
-- You should have received a copy of the GNU Lesser General    
-- Public License along with this source; if not, download it   
-- from http://www.gnu.org/licenses/lgpl-2.1.html
--
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author          Description
-- 2010-07-26  1.0      twlostow        Created
-- 2010-08-16  1.0      twlostow        Bugfixes, linux compatibility added
-- 2011-08-03  2.0      greg.d          rewritten to use pipelined Wishbone
-- 2011-10-45  2.1      twlostow        bugfixes...
-- 2016-10-27  3.0      greg.d          rewritten with Tx/Rx FIFOs
-------------------------------------------------------------------------------


library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.wr_fabric_pkg.all;
use work.wishbone_pkg.all;
use work.genram_pkg.all;
use work.minic_wbgen2_pkg.all;


entity wr_mini_nic is

  generic (
    g_interface_mode       : t_wishbone_interface_mode      := CLASSIC;
    g_address_granularity  : t_wishbone_address_granularity := WORD;
    g_tx_fifo_size         : integer                        := 1024;
    g_rx_fifo_size         : integer                        := 2048;
    g_buffer_little_endian : boolean                        := false);

  port (
    clk_sys_i : in std_logic;
    rst_n_i   : in std_logic;

-------------------------------------------------------------------------------
-- Pipelined Wishbone interface
-------------------------------------------------------------------------------

    -- WBP Master (TX)
    src_dat_o   : out std_logic_vector(15 downto 0);
    src_adr_o   : out std_logic_vector(1 downto 0);
    src_sel_o   : out std_logic_vector(1 downto 0);
    src_cyc_o   : out std_logic;
    src_stb_o   : out std_logic;
    src_we_o    : out std_logic;
    src_stall_i : in  std_logic;
    src_err_i   : in  std_logic;
    src_ack_i   : in  std_logic;

    -- WBP Slave (RX)
    snk_dat_i   : in  std_logic_vector(15 downto 0);
    snk_adr_i   : in  std_logic_vector(1 downto 0);
    snk_sel_i   : in  std_logic_vector(1 downto 0);
    snk_cyc_i   : in  std_logic;
    snk_stb_i   : in  std_logic;
    snk_we_i    : in  std_logic;
    snk_stall_o : out std_logic;
    snk_err_o   : out std_logic;
    snk_ack_o   : out std_logic;

-------------------------------------------------------------------------------
-- TXTSU i/f
-------------------------------------------------------------------------------

    txtsu_port_id_i     : in  std_logic_vector(4 downto 0);
    txtsu_frame_id_i    : in  std_logic_vector(16 - 1 downto 0);
    txtsu_tsval_i       : in  std_logic_vector(28 + 4 - 1 downto 0);
    txtsu_tsincorrect_i : in  std_logic;
    txtsu_stb_i         : in  std_logic;
    txtsu_ack_o         : out std_logic;

-------------------------------------------------------------------------------
-- Wishbone slave
-------------------------------------------------------------------------------    

    wb_cyc_i   : in  std_logic;
    wb_stb_i   : in  std_logic;
    wb_we_i    : in  std_logic;
    wb_sel_i   : in  std_logic_vector(c_wishbone_data_width/8-1 downto 0);
    wb_adr_i   : in  std_logic_vector(c_wishbone_address_width-1 downto 0);
    wb_dat_i   : in  std_logic_vector(c_wishbone_data_width-1 downto 0);
    wb_dat_o   : out std_logic_vector(c_wishbone_data_width-1 downto 0);
    wb_ack_o   : out std_logic;
    wb_stall_o : out std_logic;
    wb_int_o   : out std_logic
    );
end wr_mini_nic;

architecture behavioral of wr_mini_nic is

  constant c_NTX_TIMEOUT : integer := 100;
  constant c_WRF_BYTESEL : std_logic_vector(1 downto 0) := "11";

  component minic_wb_slave
    port (
      rst_n_i          : in  std_logic;
      clk_sys_i        : in  std_logic;
      wb_adr_i         : in  std_logic_vector(4 downto 0);
      wb_dat_i         : in  std_logic_vector(31 downto 0);
      wb_dat_o         : out std_logic_vector(31 downto 0);
      wb_cyc_i         : in  std_logic;
      wb_sel_i         : in  std_logic_vector(3 downto 0);
      wb_stb_i         : in  std_logic;
      wb_we_i          : in  std_logic;
      wb_ack_o         : out std_logic;
      wb_stall_o       : out std_logic;
      wb_int_o         : out std_logic;
      tx_ts_read_ack_o : out std_logic;
      irq_tx_i         : in  std_logic;
      irq_tx_ack_o     : out std_logic;
      irq_tx_mask_o    : out std_logic;
      irq_rx_i         : in  std_logic;
      irq_rx_ack_o     : out std_logic;
      irq_txts_i       : in  std_logic;
      regs_i           : in  t_minic_in_registers;
      regs_o           : out t_minic_out_registers
    );
  end component;

  function f_swap_endian_16
    (
      data : std_logic_vector(15 downto 0)
      ) return std_logic_vector is
  begin
    if(g_buffer_little_endian = true) then
      return data(7 downto 0) & data(15 downto 8);
    else
      return data;
    end if;
  end function f_swap_endian_16;


  signal src_cyc_int   : std_logic;
  signal src_stb_int   : std_logic;
  signal snk_stall_int : std_logic;


-----------------------------------------------------------------------------
-- FIFO interface signals
-----------------------------------------------------------------------------

  signal tx_fifo_d : std_logic_vector(17 downto 0);
  signal tx_fifo_q : std_logic_vector(17 downto 0);
  alias  txf_type is tx_fifo_q(17 downto 16);
  alias  txf_data is tx_fifo_q(15 downto 0);
  signal tx_fifo_we, tx_fifo_rd : std_logic;
  signal tx_fifo_empty, tx_fifo_full : std_logic;
  signal rx_fifo_d : std_logic_vector(17 downto 0);
  signal rx_fifo_q : std_logic_vector(17 downto 0);
  signal rx_fifo_we, rx_fifo_rd : std_logic;
  signal rx_fifo_empty, rx_fifo_full : std_logic;
  signal rx_fifo_afull : std_logic;

  signal txf_ferror : std_logic;
  signal txf_fnew   : std_logic;
  signal tx_status_word : t_wrf_status_reg;


-------------------------------------------------------------------------------
-- TX FSM stuff
-------------------------------------------------------------------------------

  type t_tx_fsm_state is (TX_IDLE, TX_STATUS, TX_PACKET, TX_FLUSH, TX_END_PACKET);

  signal ntx_timeout_is_zero : std_logic;
  signal ntx_timeout         : unsigned(7 downto 0);

  signal ntx_ack_count     : unsigned(2 downto 0);

  signal ntx_state         : t_tx_fsm_state;
  signal ntx_rst_ts_ready  : std_logic;

  signal ntx_stored_dat : std_logic_vector(15 downto 0);
  signal ntx_stored_type : std_logic_vector(1 downto 0);
  signal ntx_flush_last  : std_logic;

-------------------------------------------------------------------------------
-- RX FSM stuff
-------------------------------------------------------------------------------  

  signal snk_cyc_d0 : std_logic;
  signal nrx_sof : std_logic;
  signal nrx_eof : std_logic;
  alias rxf_type is rx_fifo_d(17 downto 16);
  alias rxf_data is rx_fifo_d(15 downto 0);

  type t_rx_fsm_state is (RX_WAIT_FRAME, RX_FRAME, RX_FULL);
  signal nrx_state    : t_rx_fsm_state;

-------------------------------------------------------------------------------
-- Classic Wishbone slave signals
-------------------------------------------------------------------------------  

  signal regs_in  : t_minic_in_registers;
  signal regs_out : t_minic_out_registers;

  signal wb_in  : t_wishbone_master_in;
  signal wb_out : t_wishbone_master_out;

  signal irq_tx     : std_logic;
  signal irq_rx_ack : std_logic;
  signal irq_rx     : std_logic;

  signal nrx_newpacket, nrx_newpacket_d0 : std_logic;
  signal ntx_newpacket, ntx_newpacket_d0 : std_logic;

  signal irq_txts    : std_logic;
  signal irq_tx_ack  : std_logic;
  signal irq_tx_mask : std_logic;


  --component chipscope_ila
  --  port (
  --    CONTROL : inout std_logic_vector(35 downto 0);
  --    CLK     : in    std_logic;
  --    TRIG0   : in    std_logic_vector(31 downto 0);
  --    TRIG1   : in    std_logic_vector(31 downto 0);
  --    TRIG2   : in    std_logic_vector(31 downto 0);
  --    TRIG3   : in    std_logic_vector(31 downto 0));
  --end component;

  --signal CONTROL : std_logic_vector(35 downto 0);
  --signal CLK     : std_logic;
  --signal TRIG0   : std_logic_vector(31 downto 0);
  --signal TRIG1   : std_logic_vector(31 downto 0);
  --signal TRIG2   : std_logic_vector(31 downto 0);
  --signal TRIG3   : std_logic_vector(31 downto 0);

  --component chipscope_icon
  --  port (
  --    CONTROL0 : inout std_logic_vector (35 downto 0));
  --end component;

begin  -- behavioral

  --chipscope_ila_1 : chipscope_ila
  --  port map (
  --    CONTROL => CONTROL,
  --    CLK     => clk_sys_i,
  --    TRIG0   => TRIG0,
  --    TRIG1   => TRIG1,
  --    TRIG2   => TRIG2,
  --    TRIG3   => TRIG3);

  --chipscope_icon_1 : chipscope_icon
  --  port map (
  --    CONTROL0 => CONTROL);

  regs_in.mcr_ver_i         <= x"1";
  regs_in.dbgr_irq_cnt_i    <= (others => '0');
  regs_in.dbgr_wb_irq_val_i <= '0';

-------------------------------------------------------------------------------
-- Tx / Rx FIFO
-----------------------------------------------------------------------------
  TX_FIFO: generic_sync_fifo
    generic map(
      g_data_width => 18, 
      g_size       => g_tx_fifo_size,
      g_with_almost_empty => false,
      g_with_almost_full  => false,
      g_with_count        => false,
      g_show_ahead        => true)
    port map (
      rst_n_i => rst_n_i,
      clk_i   => clk_sys_i,
      d_i     => tx_fifo_d,
      we_i    => tx_fifo_we,
      q_o     => tx_fifo_q,
      rd_i    => tx_fifo_rd,
      empty_o => tx_fifo_empty,
      full_o  => tx_fifo_full);

  RX_FIFO: generic_sync_fifo
    generic map(
      g_data_width => 18, 
      g_size       => g_rx_fifo_size,
      g_with_almost_empty => false,
      g_with_almost_full  => true,
      g_with_count        => false,
      g_almost_full_threshold => g_rx_fifo_size/2,
      g_show_ahead        => true)
    port map (
      rst_n_i => rst_n_i,
      clk_i   => clk_sys_i,
      d_i     => rx_fifo_d,
      we_i    => rx_fifo_we,
      q_o     => rx_fifo_q,
      rd_i    => rx_fifo_rd,
      empty_o => rx_fifo_empty,
      full_o  => rx_fifo_full,
      almost_full_o => rx_fifo_afull);

  tx_fifo_d  <= regs_out.tx_fifo_type_o & regs_out.tx_fifo_dat_o;
  tx_fifo_we <= regs_out.tx_fifo_dat_wr_o and regs_out.tx_fifo_type_wr_o;
  regs_in.mcr_tx_empty_i <= tx_fifo_empty;
  regs_in.mcr_tx_full_i  <= tx_fifo_full;

  regs_in.mcr_rx_empty_i  <= rx_fifo_empty;
  regs_in.rx_fifo_empty_i <= rx_fifo_empty;
  regs_in.mcr_rx_full_i   <= rx_fifo_full;
  regs_in.rx_fifo_full_i  <= rx_fifo_full;
  regs_in.rx_fifo_type_i  <= rx_fifo_q(17 downto 16);
  regs_in.rx_fifo_dat_i   <= rx_fifo_q(15 downto 0);

  -- sniff wb access to generate rx_fifo_rd every time the RX_FIFO register is
  -- read
  rx_fifo_rd <= '1' when(wb_out.cyc='1' and wb_out.stb='1' and wb_out.adr(7 downto 0)=x"02" and wb_in.ack='1') else
                '0';

-------------------------------------------------------------------------------
-- TX Path  (Host -> Fabric)
-------------------------------------------------------------------------------  

-- helper signals to avoid big IF conditions in the FSM
  ntx_timeout_is_zero <= '1' when (ntx_timeout = to_unsigned(0, ntx_timeout'length)) else '0';

  p_count_acks : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' or src_cyc_int = '0' or src_err_i = '1' then
        ntx_ack_count <= (others => '0');
      else
        if(src_stb_int = '1' and src_stall_i = '0' and src_ack_i = '0') then
          ntx_ack_count <= ntx_ack_count + 1;
        elsif(src_ack_i = '1' and not(src_stb_int = '1' and src_stall_i = '0')) then
          ntx_ack_count <= ntx_ack_count - 1;
        end if;
      end if;
    end if;
  end process;

  tx_status_word <= f_unmarshall_wrf_status(tx_fifo_q(15 downto 0));
  -- signals error in transmitted frame (set by software
  -- by writing again status register to TX Fifo
  txf_ferror <= '1' when (tx_fifo_empty = '0' and txf_type = c_WRF_STATUS and tx_status_word.error = '1') else
                '0';
  txf_fnew   <= '1' when (tx_fifo_empty = '0' and txf_type = c_WRF_STATUS and tx_status_word.error = '0') else
                '0';

  p_tx_fsm: process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if (rst_n_i = '0') then
        src_cyc_int <= '0';
        src_stb_int <= '0';
        src_sel_o   <= "11";
        src_adr_o   <= c_WRF_DATA;
        tx_fifo_rd  <= '0';
        ntx_rst_ts_ready <= '0';
        ntx_state   <= TX_IDLE;
        ntx_stored_dat <= (others=>'0');
        ntx_stored_type <= (others=>'0');
        ntx_flush_last <= '0';
        ntx_newpacket  <= '0';
      else
        case ntx_state is
          when TX_IDLE =>
            regs_in.mcr_tx_error_i <= '0';
            src_cyc_int <= '0';
            src_stb_int <= '0';
            src_sel_o   <= "11";
            src_adr_o   <= txf_type;
            ntx_timeout <= to_unsigned(c_NTX_TIMEOUT, ntx_timeout'length);
            ntx_flush_last <= '0';
            ntx_newpacket  <= '0';
            if (tx_fifo_empty = '0' and txf_fnew = '0') then
              -- if there is something in the fifo but it's not a status word,
              -- we read until we find a valid status. In this case we indicate
              -- that Minic is busy by driving wbreg bit tx_idle to 0.
              tx_fifo_rd <= '1';
              ntx_rst_ts_ready <= '0';
              regs_in.mcr_tx_idle_i  <= '0';
            elsif (tx_fifo_empty = '0' and txf_fnew = '1' and regs_out.mcr_tx_start_o = '1') then
              -- we have a new frame to be sent, proceed..
              src_cyc_int <= '1';
              tx_fifo_rd <= '1';
              ntx_rst_ts_ready <= '1';
              regs_in.mcr_tx_idle_i  <= '0';
              ntx_state  <= TX_STATUS;
            else
              -- wait quietly for something to be written to FIFO
              tx_fifo_rd <= '0';
              ntx_rst_ts_ready <= '0';
              regs_in.mcr_tx_idle_i  <= '1';
            end if;

          when TX_STATUS =>
            -- read first word of the frame from fifo and start transmission
            regs_in.mcr_tx_idle_i <= '0';
            ntx_rst_ts_ready      <= '0';
            src_cyc_int <= '1';
            src_stb_int <= '1';
            src_sel_o   <= "11";
            src_adr_o   <= c_WRF_STATUS;
            src_dat_o   <= f_swap_endian_16(txf_data);
            tx_fifo_rd <= '1';
            ntx_flush_last <= '0';
            ntx_newpacket  <= '0';
            ntx_state <= TX_PACKET;

          when TX_PACKET =>
            regs_in.mcr_tx_idle_i <= '0';
            ntx_rst_ts_ready      <= '0';
            src_cyc_int <= '1';
            ntx_newpacket  <= '0';
            if (tx_fifo_empty = '0' and src_stall_i = '0' and txf_ferror = '0' and txf_type = c_WRF_DATA) then
              -- normal situation, we send the payload of a frame
              src_adr_o   <= c_WRF_DATA;
              src_dat_o   <= f_swap_endian_16(txf_data);
              src_sel_o   <= "11";
              src_stb_int <= '1';
              tx_fifo_rd  <= '1';
              ntx_flush_last <= '0';
            elsif (tx_fifo_empty = '0' and src_stall_i = '0' and txf_ferror = '0' and txf_type = c_WRF_BYTESEL) then
              -- almost normal situation, only one byte of data is valid
              src_adr_o   <= c_WRF_DATA;
              src_dat_o   <= f_swap_endian_16(txf_data);
              src_sel_o   <= "10";
              src_stb_int <= '1';
              tx_fifo_rd  <= '1';
              ntx_flush_last <= '0';
            elsif (tx_fifo_empty = '0' and src_stall_i = '0' and txf_ferror = '0' and txf_type = c_WRF_OOB) then
              -- we got OOB in TXed frame, let's send it
              src_adr_o   <= c_WRF_OOB;
              src_dat_o   <= f_swap_endian_16(txf_data);
              src_sel_o   <= "11";
              src_stb_int <= '1';
              tx_fifo_rd  <= '1';
              ntx_flush_last <= '0';
            elsif ((tx_fifo_empty = '1' or txf_fnew = '1') and src_stall_i = '0') then
              -- we done with this frame
              src_adr_o   <= c_WRF_DATA;
              src_dat_o   <= f_swap_endian_16(txf_data);
              src_stb_int <= '0';
              src_sel_o   <= "11";
              tx_fifo_rd  <= '0';
              ntx_flush_last <= '0';
              ntx_state   <= TX_END_PACKET;
            else
              -- e.g. snk is stalling, we wait
              tx_fifo_rd  <= '0';
              src_stb_int <= '1';
              ntx_stored_dat <= txf_data;
              ntx_stored_type <= txf_type;
              if (tx_fifo_empty = '1') then
                ntx_flush_last <= '1';
              else
                ntx_flush_last <= '0';
              end if;
              ntx_state   <= TX_FLUSH;
            end if;

          when TX_FLUSH =>
            regs_in.mcr_tx_idle_i <= '0';
            ntx_rst_ts_ready      <= '0';
            src_cyc_int <= '1';
            ntx_newpacket  <= '0';
            if (src_stall_i = '0' and (ntx_stored_type = c_WRF_DATA or ntx_stored_type = c_WRF_OOB)) then
              src_adr_o   <= ntx_stored_type;
              src_dat_o   <= f_swap_endian_16(ntx_stored_dat);
              src_sel_o   <= "11";
            elsif (src_stall_i = '0' and ntx_stored_type = c_WRF_BYTESEL) then
              src_adr_o   <= c_WRF_DATA;
              src_dat_o   <= f_swap_endian_16(ntx_stored_dat);
              src_sel_o   <= "10";
            else
              src_stb_int <= '1';
              tx_fifo_rd  <= '0';
            end if;

            if (ntx_flush_last = '0' and src_stall_i = '0') then
              src_stb_int <= '1';
              tx_fifo_rd  <= '1';
              ntx_state   <= TX_PACKET;
            elsif (ntx_flush_last = '1' and src_stall_i = '0') then
              src_stb_int <= '0';
              tx_fifo_rd  <= '0';
              ntx_state   <= TX_END_PACKET;
            else
              src_stb_int <= '1';
              tx_fifo_rd  <= '0';
              ntx_state   <= TX_FLUSH;
            end if;

          when TX_END_PACKET =>
            regs_in.mcr_tx_idle_i <= '0';
            ntx_rst_ts_ready      <= '0';
            src_stb_int <= '0';
            -- timeout counter in case we never get all ACKs.
            ntx_timeout <= ntx_timeout - 1;
            if (ntx_ack_count = 0 or ntx_timeout_is_zero = '1') then
              regs_in.mcr_tx_error_i <= ntx_timeout_is_zero;
              src_cyc_int <= '0';
              src_sel_o   <= "11";
              tx_fifo_rd  <= '0';
              ntx_newpacket <= '1';
              ntx_state   <= TX_IDLE;
            end if;
        end case;
      end if;
    end if;
  end process;

-- these are never used:
  src_we_o  <= '1';
  src_stb_o <= src_stb_int;
  src_cyc_o <= src_cyc_int;

-------------------------------------------------------------------------------
-- RX Path (Fabric ->  Host)
-------------------------------------------------------------------------------  

  p_rx_gen_ack : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        snk_ack_o <= '0';
      else
        if(snk_cyc_i = '1' and snk_stb_i = '1' and snk_stall_int = '0') then
          snk_ack_o <= '1';
        else
          snk_ack_o <= '0';
        end if;
      end if;
    end if;

  end process;

  nrx_sof <= '1' when(snk_cyc_d0 = '0' and snk_cyc_i = '1') else
             '0';
  nrx_eof <= '1' when(snk_cyc_d0 = '1' and snk_cyc_i = '0') else
             '0';

  process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if (rst_n_i = '0') then
        snk_cyc_d0 <= '0';
        rx_fifo_we <= '0';
        rxf_type   <= (others=>'0');
        rxf_data   <= (others=>'0');
        snk_stall_int <= '0';
        regs_in.mcr_rx_ready_i <= '0';
        nrx_newpacket <= '0';
        nrx_state  <= RX_WAIT_FRAME;

      else
        snk_cyc_d0 <= snk_cyc_i;

        case nrx_state is
          when RX_WAIT_FRAME =>
            rx_fifo_we <= '0';
            rxf_type   <= (others=>'0');
            rxf_data   <= (others=>'0');
            regs_in.mcr_rx_error_i  <= '0';
            nrx_newpacket <= '0';
            if (regs_out.mcr_rx_en_o = '1') then
              snk_stall_int <= not nrx_sof;
            else
              -- RX path is disabled, don't stall any traffic
              snk_stall_int <= '0';
            end if;

            -- wait for software to enable RX path and a start of new frame
            if (regs_out.mcr_rx_en_o = '1' and nrx_sof = '1' and rx_fifo_full = '0') then
              nrx_state <= RX_FRAME;
            end if;

          when RX_FRAME =>
            snk_stall_int <= '0';
            -- receive frame, write it to FIFO
            if (snk_stb_i = '1' and snk_sel_i = "11") then
              rxf_type   <= snk_adr_i;
              rxf_data   <= f_swap_endian_16(snk_dat_i);
              rx_fifo_we <= '1';
            elsif (snk_stb_i = '1' and snk_sel_i = "10") then
              rxf_type   <= c_WRF_BYTESEL;
              rxf_data   <= f_swap_endian_16(snk_dat_i);
              rx_fifo_we <= '1';
            else
              rxf_type   <= (others=>'0');
              rxf_data   <= (others=>'0');
              rx_fifo_we <= '0';
            end if;

            if ((regs_out.mcr_rx_en_o = '0' or nrx_eof = '1') and rx_fifo_full = '0') then
              -- stop writing FIFO if sw disables RX path
              -- or if we're done with current frame
              regs_in.mcr_rx_ready_i <= '1';
              regs_in.mcr_rx_error_i  <= '0';
              nrx_newpacket <= '1';
              nrx_state              <= RX_WAIT_FRAME;
            elsif ((regs_out.mcr_rx_en_o = '0' or nrx_eof = '1') and rx_fifo_full = '1') then
              -- the difference with the previous condition is that if the fifo
              -- is full on the last word, we don't set rx_error, because the
              -- frame was not cut (it fits in the FIFO). Besides that, we have
              -- to go to RX_FULL state to wait for the FIFO to be half-empty
              -- and receive more frames.
              regs_in.mcr_rx_ready_i <= '1';
              regs_in.mcr_rx_error_i  <= '0';
              nrx_newpacket <= '1';
              nrx_state              <= RX_FULL;
            elsif (rx_fifo_full = '1') then
              -- error if fifo gets full needs to be recovered
              regs_in.mcr_rx_ready_i <= '1';
              regs_in.mcr_rx_error_i  <= '1';
              nrx_newpacket <= '1';
              nrx_state <= RX_FULL;
            else
              regs_in.mcr_rx_ready_i <= '0';
              regs_in.mcr_rx_error_i  <= '0';
              nrx_newpacket <= '0';

            end if;

          when RX_FULL =>
            snk_stall_int <= '0';
            rx_fifo_we <= '0';
            rxf_type   <= (others=>'0');
            rxf_data   <= (others=>'0');
            nrx_newpacket <= '0';

            -- recovering means disabling RX path and reading everything from
            -- the FIFO
            --if (regs_out.mcr_rx_en_o = '0' and rx_fifo_empty = '1') then
            --  nrx_state <= RX_WAIT_FRAME;
            --end if;
            if (snk_cyc_i = '0' and rx_fifo_afull = '0') then
              nrx_state <= RX_WAIT_FRAME;
            end if;
        end case;
      end if;
    end if;
  end process;


  snk_stall_o <= snk_stall_int;
  snk_err_o   <= '0';

-------------------------------------------------------------------------------
-- TX Timestamping unit
-------------------------------------------------------------------------------  
  tsu_fsm : process(clk_sys_i, rst_n_i)
  begin
    if rising_edge(clk_sys_i) then
      if(rst_n_i = '0') then
        regs_in.mcr_tx_ts_ready_i <= '0';
        regs_in.tsr0_valid_i <= '0';
        regs_in.tsr0_pid_i   <= (others => '0');
        regs_in.tsr0_fid_i   <= (others => '0');
        regs_in.tsr1_tsval_i <= (others => '0');
        txtsu_ack_o          <= '0';
      else
        -- Make sure the timestamp is written to the FIFO only once.

        if(ntx_rst_ts_ready = '1') then
          regs_in.mcr_tx_ts_ready_i <= '0';
        elsif(txtsu_stb_i = '1') then
          regs_in.mcr_tx_ts_ready_i <= '1';
          regs_in.tsr0_valid_i <= not txtsu_tsincorrect_i;
          regs_in.tsr0_fid_i   <= txtsu_frame_id_i;
          regs_in.tsr0_pid_i   <= txtsu_port_id_i;
          regs_in.tsr1_tsval_i <= txtsu_tsval_i;
          txtsu_ack_o          <= '1';
        else
          txtsu_ack_o <= '0';
        end if;
      end if;
    end if;
  end process;

  handle_irqs: process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        irq_tx           <= '0';
        irq_rx           <= '0';
        ntx_newpacket_d0 <= '0';
        nrx_newpacket_d0 <= '0';
      else
        ntx_newpacket_d0 <= ntx_newpacket;
        nrx_newpacket_d0 <= nrx_newpacket;

        if (ntx_newpacket_d0 = '0' and ntx_newpacket = '1' and irq_tx_mask = '1') then
          irq_tx <= '1';
        elsif (irq_tx_mask = '0' or irq_tx_ack = '1') then
          irq_tx <= '0';
        end if;

        if (nrx_newpacket_d0 = '0' and nrx_newpacket = '1') then
          irq_rx <= '1';
        elsif (irq_rx_ack = '1') then
          irq_rx <= '0';
        end if;
      end if;
    end if;
  end process;

  irq_txts <= '0';

  U_Slave_Adapter : wb_slave_adapter
    generic map (
      g_master_use_struct  => true,
      g_master_mode        => CLASSIC,
      g_master_granularity => WORD,
      g_slave_use_struct   => false,
      g_slave_mode         => g_interface_mode,
      g_slave_granularity  => g_address_granularity)
    port map (
      clk_sys_i  => clk_sys_i,
      rst_n_i    => rst_n_i,
      sl_adr_i   => wb_adr_i,
      sl_dat_i   => wb_dat_i,
      sl_sel_i   => wb_sel_i,
      sl_cyc_i   => wb_cyc_i,
      sl_stb_i   => wb_stb_i,
      sl_we_i    => wb_we_i,
      sl_dat_o   => wb_dat_o,
      sl_ack_o   => wb_ack_o,
      sl_stall_o => wb_stall_o,
      master_i   => wb_in,
      master_o   => wb_out);

  U_WB_Slave : minic_wb_slave
    port map (
      rst_n_i          => rst_n_i,
      clk_sys_i        => clk_sys_i,
      wb_adr_i         => wb_out.adr(4 downto 0),
      wb_dat_i         => wb_out.dat,
      wb_dat_o         => wb_in.dat,
      wb_cyc_i         => wb_out.cyc,
      wb_sel_i         => wb_out.sel,
      wb_stb_i         => wb_out.stb,
      wb_we_i          => wb_out.we,
      wb_ack_o         => wb_in.ack,
      wb_stall_o       => wb_in.stall,
      wb_int_o         => wb_int_o,
      regs_i           => regs_in,
      regs_o           => regs_out,
      tx_ts_read_ack_o => open,
      irq_tx_i         => irq_tx,
      irq_tx_ack_o     => irq_tx_ack,
      irq_tx_mask_o    => irq_tx_mask,
      irq_rx_i         => irq_rx,
      irq_rx_ack_o     => irq_rx_ack,
      irq_txts_i       => irq_txts);

  wb_in.err <= '0';
  wb_in.rty <= '0';
  wb_in.int <= '0';

  --TRIG0(0) <= regs_out.mcr_rx_en_o;
  --TRIG0(1) <= rx_fifo_empty;
  --TRIG0(2) <= rx_fifo_full;
  --TRIG0(3) <= rx_fifo_rd;
  --TRIG0(4) <= rx_fifo_we;
  --TRIG0(6 downto 5)  <= rx_fifo_q(17 downto 16);
  --TRIG0(22 downto 7) <= rx_fifo_q(15 downto 0);
  --TRIG0(24 downto 23) <= "00" when(nrx_state = RX_WAIT_FRAME) else
  --                       "01" when(nrx_state = RX_FRAME) else
  --                       "10" when(nrx_state = RX_FULL) else
  --                       "11";
  --TRIG0(25) <= nrx_sof;
  --TRIG0(26) <= nrx_eof;
  --TRIG0(27) <= snk_cyc_i;
  --TRIG0(28) <= snk_stb_i;
  --TRIG0(29) <= snk_stall_int;
  --TRIG0(31 downto 30) <= snk_adr_i;

  --TRIG1(15 downto 0) <= snk_dat_i;
  --TRIG1(16) <= rx_fifo_afull;
  --TRIG1(17) <= wb_out.cyc;
  --TRIG1(18) <= wb_out.stb;
  --TRIG1(19) <= wb_in.ack;
  --TRIG1(20) <= irq_rx;

  --TRIG2(31 downto 0) <= wb_out.adr;

end behavioral;

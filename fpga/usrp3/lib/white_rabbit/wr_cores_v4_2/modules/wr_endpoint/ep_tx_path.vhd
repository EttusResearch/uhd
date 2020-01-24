-------------------------------------------------------------------------------
-- Title      : Complete Transmit Path
-- Project    : White Rabbit MAC/Endpoint
-------------------------------------------------------------------------------
-- File       : ep_tx_path.vhd
-- Author     : Tomasz Włostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2012-11-01
-- Last update: 2017-02-03
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: Full transmit path of the endpoint (excluding the PCS).
-- Chains the wishbone interface and header processor, followed by the VLAN unit
-- (optional), packet injector (optional) and CRC inserter.
-------------------------------------------------------------------------------
--
-- Copyright (c) 2009 - 2012 CERN / BE-CO-HT
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


library ieee;
use ieee.std_logic_1164.all;

library work;
use work.wr_fabric_pkg.all;
use work.endpoint_private_pkg.all;
use work.endpoint_pkg.all;
use work.ep_wbgen2_pkg.all;

entity ep_tx_path is

  generic(
    g_with_vlans            : boolean;
    g_with_timestamper      : boolean;
    g_with_packet_injection : boolean;
    g_with_inj_ctrl         : boolean := true;
    g_force_gap_length      : integer;
    g_runt_padding          : boolean;
    g_use_new_crc           :	boolean
    );

  port (
    clk_sys_i : in std_logic;
    rst_n_i   : in std_logic;

------------------------------------------------------------------------------
-- Physical Coding Sublayer (PCS) interface
------------------------------------------------------------------------------

    pcs_fab_o   : out t_ep_internal_fabric;
    pcs_error_i : in  std_logic;
    pcs_busy_i  : in  std_logic;
    pcs_dreq_i  : in  std_logic;

-------------------------------------------------------------------------------
-- WRF Sink (see WRF specification for the details)
-------------------------------------------------------------------------------    

    snk_i : in  t_wrf_sink_in;
    snk_o : out t_wrf_sink_out;

-------------------------------------------------------------------------------
-- Flow Control Unit signals
-------------------------------------------------------------------------------    

-- TX send pause frame - when active, the framer will send a PAUSE frame
-- as soon as possible. The pause quanta must be provided on tx_pause_delay_i input.
    fc_pause_req_i   : in std_logic;
    fc_pause_delay_i : in std_logic_vector(15 downto 0);

-- TX send pause acknowledge - active after the current pause send request has
-- been completed
    fc_pause_ready_o : out std_logic;

-- When active, the framer will allow for packet transmission.
    fc_flow_enable_i : in std_logic;

-------------------------------------------------------------------------------
-- OOB/TSU signals
-------------------------------------------------------------------------------    

-- Port ID value
    txtsu_port_id_o      : out std_logic_vector(4 downto 0);
-- Frame ID value
    txtsu_fid_o          : out std_logic_vector(16 -1 downto 0);
-- Encoded timestamps
    txtsu_ts_value_o     : out std_logic_vector(28 + 4 - 1 downto 0);
    txtsu_ts_incorrect_o : out std_logic;

-- TX timestamp strobe: HI tells the TX timestamping unit that a timestamp is
-- available on txtsu_ts_value_o, txtsu_fid_o andd txtsu_port_id_o. The correctness
-- of the timestamping is indiacted on txtsu_ts_incorrect_o. Line remains HI
-- until assertion of txtsu_ack_i.
    txtsu_stb_o : out std_logic;

-- TX timestamp acknowledge: HI indicates that TXTSU has successfully received
-- the timestamp
    txtsu_ack_i : in std_logic;

------------------------------------------------------------------------------
-- Timestamp input from the timestamping unit
------------------------------------------------------------------------------
    txts_timestamp_i       : in std_logic_vector(31 downto 0);
    txts_timestamp_valid_i : in std_logic;

------------------------------------------------------------------------------
-- Packet Injector (for TRU & HW-RSTP)
------------------------------------------------------------------------------

    inject_req_i        : in  std_logic                     := '0';
    inject_ready_o      : out std_logic;
    inject_packet_sel_i : in  std_logic_vector(2 downto 0)  := "000";
    inject_user_value_i : in  std_logic_vector(15 downto 0) := x"0000";


-------------------------------------------------------------------------------
-- Control registers
-------------------------------------------------------------------------------
    ep_ctrl_i           : in std_logic :='1';
    regs_i : in t_ep_out_registers;
    regs_o : out t_ep_in_registers;
    dbg_o          : out std_logic_vector(33 downto 0)
    );


end ep_tx_path;

architecture rtl of ep_tx_path is

  type t_fab_pipe is array(integer range <>) of t_ep_internal_fabric;

  signal fab_pipe  : t_fab_pipe(4 downto 0);
  signal dreq_pipe : std_logic_vector(4 downto 0);

  signal vlan_mem_addr : std_logic_vector(9 downto 0);
  signal vlan_mem_data : std_logic_vector(17 downto 0);
  
  signal txtsu_stb     : std_logic;

  signal inject_req            : std_logic;
  signal inject_ready          : std_logic;
  signal inject_packet_sel     : std_logic_vector(2 downto 0);
  signal inject_user_value     : std_logic_vector(15 downto 0);
  signal inject_mode           : std_logic_vector( 1 downto 0);

  signal inj_ctr_req            : std_logic;
  signal inj_ctr_packet_sel     : std_logic_vector(2 downto 0);
  signal inj_ctr_user_value     : std_logic_vector(15 downto 0);
  signal inj_ctr_ena            : std_logic;
  signal inj_ctr_mode           : std_logic_vector( 1 downto 0);


begin  -- rtl

  U_Header_Processor : ep_tx_header_processor
    generic map (
      g_with_packet_injection => g_with_packet_injection,
      g_with_timestamper      => g_with_timestamper,
      g_force_gap_length      => g_force_gap_length,
      g_runt_padding          => g_runt_padding)
    port map (
      clk_sys_i              => clk_sys_i,
      rst_n_i                => rst_n_i,
      src_fab_o              => fab_pipe(0),
      src_dreq_i             => dreq_pipe(0),
      pcs_busy_i             => pcs_busy_i,
      pcs_error_i            => pcs_error_i,
      wb_snk_i               => snk_i,
      wb_snk_o               => snk_o,
      fc_pause_req_i         => fc_pause_req_i,
      fc_pause_delay_i       => fc_pause_delay_i,
      fc_pause_ready_o       => fc_pause_ready_o,
      fc_flow_enable_i       => fc_flow_enable_i,
      txtsu_port_id_o        => txtsu_port_id_o,
      txtsu_fid_o            => txtsu_fid_o,
      txtsu_ts_value_o       => txtsu_ts_value_o,
      txtsu_ts_incorrect_o   => txtsu_ts_incorrect_o,
      txtsu_stb_o            => txtsu_stb, --txtsu_stb_o,
      txtsu_ack_i            => txtsu_ack_i,
      txts_timestamp_i       => txts_timestamp_i,
      txts_timestamp_valid_i => txts_timestamp_valid_i,
      ep_ctrl_i              => ep_ctrl_i,
      regs_i                 => regs_i);

  txtsu_stb_o <= txtsu_stb;

  assert not (g_with_packet_injection and not g_with_vlans)
    report "wr_endpoint: packet injection requires VLAN support to be enabled" severity failure;

  gen_with_inj_ctrl: if(g_with_inj_ctrl and g_with_packet_injection) generate
    -- the injector control has two main purposes:
    -- 1) control ep_tx_packet_injector so that it continuously generatres frames with
    --    desired interframe gap
    -- 2) block the possible traffic coming from SWcore 
    --    - so that it does not disturb generation which is done only between frames from SWcore
    --    - each frame coming from SWcore is "scanned" by Ethertype in the RAM which is used
    --      for generation -> if ep_tx_vlan_unit is used (if dev/null was done after this module)
    --      when ep_tx_packet_injection is used to genrate frame... we have problem cause two
    --      modules read from the same RAM
    U_Injector_ctr: ep_tx_inject_ctrl
      generic map (
        g_min_if_gap_length   => 5)
      port map (
        clk_sys_i             => clk_sys_i,
        rst_n_i               => rst_n_i,
        snk_fab_i             => fab_pipe(0),
        snk_dreq_o            => dreq_pipe(0),
        src_fab_o             => fab_pipe(1),
        src_dreq_i            => dreq_pipe(1),
        inject_req_o          => inj_ctr_req,
        inject_ready_i        => inject_ready,
        inject_packet_sel_o   => inj_ctr_packet_sel,
        inject_user_value_o   => inj_ctr_user_value,
        inject_ctr_ena_o      => inj_ctr_ena,
        inject_ctr_mode_o     => inj_ctr_mode,
        regs_i                => regs_i,
        regs_o                => regs_o);
  end generate gen_with_inj_ctrl;

  inject_req        <= inj_ctr_req        when (inj_ctr_ena ='1') else inject_req_i;
  inject_packet_sel <= inj_ctr_packet_sel when (inj_ctr_ena ='1') else inject_packet_sel_i;
  inject_ready_o    <= inject_ready;

  gen_without_inj_ctrl: if((not g_with_inj_ctrl) or (not g_with_packet_injection)) generate
    fab_pipe(1)        <= fab_pipe(0);
    dreq_pipe(0)       <= dreq_pipe(1);   
    inj_ctr_req        <= '0';
    inj_ctr_mode       <= (others => '0');
    inj_ctr_packet_sel <= (others => '0');
    inj_ctr_user_value <= (others => '0');
    inj_ctr_ena        <= '0';
    regs_o             <= c_ep_in_registers_init_value;
  end generate gen_without_inj_ctrl;

  gen_with_vlan_unit : if(g_with_vlans) generate
    U_VLAN_Unit : ep_tx_vlan_unit
      port map (
        clk_sys_i         => clk_sys_i,
        rst_n_i           => rst_n_i,
        snk_fab_i         => fab_pipe(1),
        snk_dreq_o        => dreq_pipe(1),
        src_fab_o         => fab_pipe(2),
        src_dreq_i        => dreq_pipe(2),
        inject_mem_addr_i => vlan_mem_addr,
        inject_mem_data_o => vlan_mem_data,
        uram_offset_wr_i  => regs_i.vcr1_offset_wr_o,
        uram_offset_i     => regs_i.vcr1_offset_o,
        uram_data_i       => regs_i.vcr1_data_o);
  end generate gen_with_vlan_unit;

  gen_without_vlan_unit : if(not g_with_vlans) generate
    fab_pipe(2)  <= fab_pipe(1);
    dreq_pipe(1) <= dreq_pipe(2);
  end generate gen_without_vlan_unit;

  gen_with_injection : if(g_with_packet_injection) generate

    inject_user_value <= inj_ctr_user_value when (inj_ctr_ena ='1') else inject_user_value_i;
    inject_mode       <= inj_ctr_mode       when (inj_ctr_ena ='1') else "00";

    U_Injector : ep_tx_packet_injection
      port map (
        clk_sys_i           => clk_sys_i,
        rst_n_i             => rst_n_i,
        snk_fab_i           => fab_pipe(2),
        snk_dreq_o          => dreq_pipe(2),
        src_fab_o           => fab_pipe(3),
        src_dreq_i          => dreq_pipe(3),
        inject_req_i        => inject_req,
        inject_ready_o      => inject_ready,
        inject_packet_sel_i => inject_packet_sel,
        inject_user_value_i => inject_user_value,
        inject_mode_i       => inject_mode,
        mem_addr_o          => vlan_mem_addr,
        mem_data_i          => vlan_mem_data);
  end generate gen_with_injection;

  gen_without_injection : if (not g_with_packet_injection) generate
    fab_pipe(3)  <= fab_pipe(2);
    dreq_pipe(2) <= dreq_pipe(3);
    inject_ready <= '0';
  end generate gen_without_injection;

  U_Insert_CRC : ep_tx_crc_inserter
    generic map(
      g_use_new_crc => g_use_new_crc)
    port map (
      clk_sys_i  => clk_sys_i,
      rst_n_i    => rst_n_i,
      snk_fab_i  => fab_pipe(3),
      snk_dreq_o => dreq_pipe(3),
      src_fab_o  => fab_pipe(4),
      src_dreq_i => dreq_pipe(4),
      dbg_o      => dbg_o(33 downto 31));

  pcs_fab_o    <= fab_pipe(4);
  dreq_pipe(4) <= pcs_dreq_i;

--   GEN_DBG: for i in 0 to 3 generate
--     dbg_o(i)    <= fab_pipe(i).sof;
--     dbg_o(i+4)  <= fab_pipe(i).eof;
--   end generate GEN_DBG;

    dbg_o(0)    <= fab_pipe(0).sof;             -- 64
    dbg_o(1)    <= fab_pipe(2).sof;             -- 65
    dbg_o(2)    <= fab_pipe(3).sof;             -- 66
    dbg_o(3)    <= fab_pipe(4).sof;             -- 67
    dbg_o(4)    <= fab_pipe(0).eof;             -- 68
    dbg_o(5)    <= fab_pipe(2).eof;             -- 69
    dbg_o(6)    <= fab_pipe(3).eof;             -- 70
    dbg_o(7)    <= fab_pipe(4).eof;             -- 71

    dbg_o(8)    <= dreq_pipe(0);                -- 72
    dbg_o(9)    <= dreq_pipe(2);                -- 73
    dbg_o(10)   <= dreq_pipe(3);                -- 74 
    dbg_o(11)   <= fab_pipe(0).dvalid;          -- 75
    dbg_o(12)   <= fab_pipe(4).dvalid;          -- 32
    -- new 4 bits
    dbg_o(13)   <= dreq_pipe(4);                -- 33
    dbg_o(14)   <= txtsu_stb;                   -- 34
    dbg_o(15)   <= txtsu_ack_i;                 -- 35
    dbg_o(16)   <= fab_pipe(2).dvalid;          -- 36
--     dbg_o(28 downto 13) <= fab_pipe(2).data;
    dbg_o(17)   <= inj_ctr_req;                 -- 37    
    dbg_o(18)   <= inject_req;                  -- 38
    dbg_o(19)   <= inject_ready;                -- 39                inj_ctr_user_value;
    dbg_o(20)   <= inj_ctr_ena;                 -- 40
    dbg_o(23 downto 21) <= inject_packet_sel;   -- 41-43
    dbg_o(24)   <= inject_req_i;                -- 43
    dbg_o(28 downto 25) <= fab_pipe(3).data(3 downto 0);
--     dbg_o(28 downto 17) <= fab_pipe(3).data(11 downto 0);
    dbg_o(30 downto 29) <= fab_pipe(3).addr;
    
end rtl;

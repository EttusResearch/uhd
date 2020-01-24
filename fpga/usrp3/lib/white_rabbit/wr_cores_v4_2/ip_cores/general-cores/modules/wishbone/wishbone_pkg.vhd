-------------------------------------------------------------------------------
-- Title      : Wishbone package
-- Project    : General Cores
-------------------------------------------------------------------------------
-- File       : wishbone_pkg.vhd
-- Company    : CERN
-- Platform   : FPGA-generics
-- Standard   : VHDL '93
-------------------------------------------------------------------------------
-- Copyright (c) 2011-2017 CERN
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
-------------------------------------------------------------------------------

library ieee;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.genram_pkg.all;

package wishbone_pkg is

  constant c_wishbone_address_width : integer := 32;
  constant c_wishbone_data_width    : integer := 32;

  subtype t_wishbone_address is
    std_logic_vector(c_wishbone_address_width-1 downto 0);
  subtype t_wishbone_data is
    std_logic_vector(c_wishbone_data_width-1 downto 0);
  subtype t_wishbone_byte_select is
    std_logic_vector((c_wishbone_address_width/8)-1 downto 0);
  subtype t_wishbone_cycle_type is
    std_logic_vector(2 downto 0);
  subtype t_wishbone_burst_type is
    std_logic_vector(1 downto 0);

  type t_wishbone_interface_mode is (CLASSIC, PIPELINED);
  type t_wishbone_address_granularity is (BYTE, WORD);

  type t_wishbone_master_out is record
    cyc : std_logic;
    stb : std_logic;
    adr : t_wishbone_address;
    sel : t_wishbone_byte_select;
    we  : std_logic;
    dat : t_wishbone_data;
  end record t_wishbone_master_out;

  subtype t_wishbone_slave_in is t_wishbone_master_out;

  type t_wishbone_slave_out is record
    ack   : std_logic;
    err   : std_logic;
    rty   : std_logic;
    stall : std_logic;
    int   : std_logic;
    dat   : t_wishbone_data;
  end record t_wishbone_slave_out;
  subtype t_wishbone_master_in is t_wishbone_slave_out;

  subtype t_wishbone_device_descriptor is std_logic_vector(255 downto 0);

  type t_wishbone_byte_select_array is array(natural range <>) of t_wishbone_byte_select; 
  type t_wishbone_data_array is array(natural range <>) of t_wishbone_data; 
  type t_wishbone_address_array is array(natural range <>) of t_wishbone_address;
  type t_wishbone_master_out_array is array (natural range <>) of t_wishbone_master_out;
  --type t_wishbone_slave_in_array is array (natural range <>) of t_wishbone_slave_in;
  subtype t_wishbone_slave_in_array is t_wishbone_master_out_array;
  type t_wishbone_slave_out_array is array (natural range <>) of t_wishbone_slave_out;
  --type t_wishbone_master_in_array is array (natural range <>) of t_wishbone_master_in;
  subtype t_wishbone_master_in_array is t_wishbone_slave_out_array;

  constant cc_dummy_address : std_logic_vector(c_wishbone_address_width-1 downto 0) :=
    (others => 'X');
  constant cc_dummy_data : std_logic_vector(c_wishbone_data_width-1 downto 0) :=
    (others => 'X');
  constant cc_dummy_sel : std_logic_vector(c_wishbone_data_width/8-1 downto 0) :=
    (others => 'X');
  constant cc_dummy_slave_in : t_wishbone_slave_in :=
    ('0', '0', cc_dummy_address, cc_dummy_sel, 'X', cc_dummy_data);
  constant cc_dummy_master_out : t_wishbone_master_out := cc_dummy_slave_in;

  -- Dangerous! Will stall a bus.
  constant cc_dummy_slave_out : t_wishbone_slave_out :=
    ('X', 'X', 'X', 'X', 'X', cc_dummy_data);
  constant cc_dummy_master_in : t_wishbone_master_in := cc_dummy_slave_out;

  constant cc_dummy_address_array : t_wishbone_address_array(0 downto 0) := (0 => cc_dummy_address);

  -- A generally useful function.
  function f_ceil_log2(x   : natural) return natural;
  function f_bits2string(s : std_logic_vector) return string;

  function f_string2bits(s : string) return std_logic_vector;
  function f_string2svl (s : string) return std_logic_vector;
  function f_slv2string (slv : std_logic_vector) return string;

  function f_string_fix_len( s : string; ret_len : natural := 10; fill_char : character := '0'; justify_right : boolean := true ) return string;
  function f_hot_to_bin(x : std_logic_vector) return natural;
  
  -- *** Wishbone slave interface functions ***
  -- f_wb_wr:
  -- processes an incoming write reqest to a register while honoring the select lines
  -- valid modes are overwrite "owr", set "set" (bits are or'ed) and clear "clr" (bits are nand'ed)
  function f_wb_wr(pval : std_logic_vector; ival : std_logic_vector; sel : std_logic_vector; mode : string := "owr") return std_logic_vector;
------------------------------------------------------------------------------
-- SDB declaration
------------------------------------------------------------------------------

  constant c_sdb_device_length : natural := 512;  -- bits
  subtype  t_sdb_record is std_logic_vector(c_sdb_device_length-1 downto 0);
  type     t_sdb_record_array is array(natural range <>) of t_sdb_record;
  
  type t_sdb_product is record
    vendor_id : std_logic_vector(63 downto 0);
    device_id : std_logic_vector(31 downto 0);
    version   : std_logic_vector(31 downto 0);
    date      : std_logic_vector(31 downto 0);
    name      : string(1 to 19);
  end record t_sdb_product;

  type t_sdb_component is record
    addr_first : std_logic_vector(63 downto 0);
    addr_last  : std_logic_vector(63 downto 0);
    product    : t_sdb_product;
  end record t_sdb_component;

  constant c_sdb_endian_big    : std_logic := '0';
  constant c_sdb_endian_little : std_logic := '1';
  type     t_sdb_device is record
    abi_class     : std_logic_vector(15 downto 0);
    abi_ver_major : std_logic_vector(7 downto 0);
    abi_ver_minor : std_logic_vector(7 downto 0);
    wbd_endian    : std_logic;          -- 0 = big, 1 = little
    wbd_width     : std_logic_vector(3 downto 0);  -- 3=64-bit, 2=32-bit, 1=16-bit, 0=8-bit
    sdb_component : t_sdb_component;
  end record t_sdb_device;

  type t_sdb_msi is record
    wbd_endian    : std_logic;          -- 0 = big, 1 = little
    wbd_width     : std_logic_vector(3 downto 0);  -- 3=64-bit, 2=32-bit, 1=16-bit, 0=8-bit
    sdb_component : t_sdb_component;
  end record t_sdb_msi;

  type t_sdb_bridge is record
    sdb_child     : std_logic_vector(63 downto 0);
    sdb_component : t_sdb_component;
  end record t_sdb_bridge;

  type t_sdb_integration is record
    product : t_sdb_product;
  end record t_sdb_integration;

  type t_sdb_repo_url is record
    repo_url : string(1 to 63);
  end record t_sdb_repo_url;

  type t_sdb_synthesis is record
    syn_module_name  : string(1 to 16);
    syn_commit_id    : string(1 to 32);
    syn_tool_name    : string(1 to 8);
    syn_tool_version : std_logic_vector(31 downto 0);
    syn_date         : std_logic_vector(31 downto 0);
    syn_username     : string(1 to 15);
  end record t_sdb_synthesis;
  
  -- If you have a Wishbone master that does not receive MSI,
  -- list it in the layout as 'f_sdb_auto_msi(c_null_msi, false)'
  constant c_null_msi : t_sdb_msi := (
   wbd_endian    => c_sdb_endian_big,
   wbd_width     => x"0",
   sdb_component => (
   addr_first    => x"0000000000000000",
   addr_last     => x"0000000000000000",
   product => (
   vendor_id     => x"0000000000000000",
   device_id     => x"00000000",
   version       => x"00000000",
   date          => x"00000000",
   name          => "                   ")));  

  -- general crossbar building functions
  function f_sdb_create_array(g_enum_dev_id  : boolean := false;
                              g_dev_id_offs    : natural := 0;
                              g_enum_dev_name  : boolean := false;
                              g_dev_name_offs  : natural := 0;       
                              device           : t_sdb_device; 
                              instances        : natural := 1) return t_sdb_record_array;  
  function f_sdb_join_arrays(a : t_sdb_record_array; b : t_sdb_record_array) return t_sdb_record_array;
  function f_sdb_extract_base_addr(sdb_record : t_sdb_record) return std_logic_vector;
  function f_sdb_extract_end_addr(sdb_record : t_sdb_record)  return std_logic_vector;
  function f_sdb_automap_array(sdb_array : t_sdb_record_array; start_offset : t_wishbone_address := (others => '0')) return t_sdb_record_array;  
  function f_align_addr_offset(offs : unsigned; this_rng : unsigned; prev_rng : unsigned) return unsigned; 
  function f_sdb_create_rom_addr(sdb_array : t_sdb_record_array) return t_wishbone_address;


  -- Used to configure a device at a certain address
  function f_sdb_embed_device(device : t_sdb_device; address : t_wishbone_address) return t_sdb_record;
  function f_sdb_embed_bridge(bridge : t_sdb_bridge; address : t_wishbone_address) return t_sdb_record;
  function f_sdb_embed_msi(msi : t_sdb_msi; address : t_wishbone_address) return t_sdb_record;
  function f_sdb_embed_integration(integr : t_sdb_integration) return t_sdb_record;
  function f_sdb_embed_repo_url(url : t_sdb_repo_url) return t_sdb_record;
  function f_sdb_embed_synthesis(syn : t_sdb_synthesis) return t_sdb_record;

  function f_sdb_extract_device(sdb_record : t_sdb_record) return t_sdb_device;
  function f_sdb_extract_bridge(sdb_record : t_sdb_record) return t_sdb_bridge;
  function f_sdb_extract_msi(sdb_record : t_sdb_record) return t_sdb_msi;
  function f_sdb_extract_integration(sdb_record : t_sdb_record) return t_sdb_integration;
  function f_sdb_extract_repo_url(sdb_record : t_sdb_record) return t_sdb_repo_url;
  function f_sdb_extract_synthesis(sdb_record : t_sdb_record) return t_sdb_synthesis;

  -- Automatic crossbar mapping functions
  function f_sdb_auto_device(device : t_sdb_device; enable : boolean := true; name: string := "") return t_sdb_record;
  function f_sdb_auto_bridge(bridge : t_sdb_bridge; enable : boolean := true; name: string := "") return t_sdb_record;
  function f_sdb_auto_msi   (msi    : t_sdb_msi;    enable : boolean := true) return t_sdb_record;
  function f_sdb_auto_layout(records: t_sdb_record_array)                               return t_sdb_record_array;
  function f_sdb_auto_layout(slaves : t_sdb_record_array; masters : t_sdb_record_array) return t_sdb_record_array;
  function f_sdb_auto_sdb   (records: t_sdb_record_array)                               return t_wishbone_address;
  function f_sdb_auto_sdb   (slaves : t_sdb_record_array; masters : t_sdb_record_array) return t_wishbone_address;
  
  -- For internal use by the crossbar
  function f_sdb_bus_end(g_wraparound : boolean; g_layout : t_sdb_record_array; g_sdb_addr : t_wishbone_address; msi : boolean) return unsigned;
  function f_sdb_embed_product(product         : t_sdb_product) return std_logic_vector;  -- (319 downto 8)
  function f_sdb_embed_component(sdb_component : t_sdb_component; address : t_wishbone_address) return std_logic_vector;  -- (447 downto 8)
  function f_sdb_extract_product(sdb_record    : std_logic_vector(319 downto 8)) return t_sdb_product;
  function f_sdb_extract_component(sdb_record  : std_logic_vector(447 downto 8)) return t_sdb_component;

------------------------------------------------------------------------------
-- Components declaration
-------------------------------------------------------------------------------

  component wb_slave_adapter
    generic (
      g_master_use_struct  : boolean;
      g_master_mode        : t_wishbone_interface_mode;
      g_master_granularity : t_wishbone_address_granularity;
      g_slave_use_struct   : boolean;
      g_slave_mode         : t_wishbone_interface_mode;
      g_slave_granularity  : t_wishbone_address_granularity);
    port (
      clk_sys_i  : in  std_logic;
      rst_n_i    : in  std_logic;
      sl_adr_i   : in  std_logic_vector(c_wishbone_address_width-1 downto 0) := cc_dummy_address;
      sl_dat_i   : in  std_logic_vector(c_wishbone_data_width-1 downto 0)    := cc_dummy_data;
      sl_sel_i   : in  std_logic_vector(c_wishbone_data_width/8-1 downto 0)  := cc_dummy_sel;
      sl_cyc_i   : in  std_logic                                             := '0';
      sl_stb_i   : in  std_logic                                             := '0';
      sl_we_i    : in  std_logic                                             := '0';
      sl_dat_o   : out std_logic_vector(c_wishbone_data_width-1 downto 0);
      sl_err_o   : out std_logic;
      sl_rty_o   : out std_logic;
      sl_ack_o   : out std_logic;
      sl_stall_o : out std_logic;
      sl_int_o   : out std_logic;
      slave_i    : in  t_wishbone_slave_in                                   := cc_dummy_slave_in;
      slave_o    : out t_wishbone_slave_out;
      ma_adr_o   : out std_logic_vector(c_wishbone_address_width-1 downto 0);
      ma_dat_o   : out std_logic_vector(c_wishbone_data_width-1 downto 0);
      ma_sel_o   : out std_logic_vector(c_wishbone_data_width/8-1 downto 0);
      ma_cyc_o   : out std_logic;
      ma_stb_o   : out std_logic;
      ma_we_o    : out std_logic;
      ma_dat_i   : in  std_logic_vector(c_wishbone_data_width-1 downto 0)    := cc_dummy_data;
      ma_err_i   : in  std_logic                                             := '0';
      ma_rty_i   : in  std_logic                                             := '0';
      ma_ack_i   : in  std_logic                                             := '0';
      ma_stall_i : in  std_logic                                             := '0';
      ma_int_i   : in  std_logic                                             := '0';
      master_i   : in  t_wishbone_master_in                                  := cc_dummy_slave_out;
      master_o   : out t_wishbone_master_out);
  end component;

  component wb_async_bridge
    generic (
      g_simulation          : integer;
      g_interface_mode      : t_wishbone_interface_mode      := CLASSIC;
      g_address_granularity : t_wishbone_address_granularity := WORD;
      g_cpu_address_width   : integer);
    port (
      rst_n_i     : in    std_logic;
      clk_sys_i   : in    std_logic;
      cpu_cs_n_i  : in    std_logic;
      cpu_wr_n_i  : in    std_logic;
      cpu_rd_n_i  : in    std_logic;
      cpu_bs_n_i  : in    std_logic_vector(3 downto 0);
      cpu_addr_i  : in    std_logic_vector(g_cpu_address_width-1 downto 0);
      cpu_data_b  : inout std_logic_vector(31 downto 0);
      cpu_nwait_o : out   std_logic;
      wb_adr_o    : out   std_logic_vector(c_wishbone_address_width - 1 downto 0);
      wb_dat_o    : out   std_logic_vector(31 downto 0);
      wb_stb_o    : out   std_logic;
      wb_we_o     : out   std_logic;
      wb_sel_o    : out   std_logic_vector(3 downto 0);
      wb_cyc_o    : out   std_logic;
      wb_dat_i    : in    std_logic_vector (c_wishbone_data_width-1 downto 0);
      wb_ack_i    : in    std_logic;
      wb_stall_i  : in    std_logic := '0');
  end component;

  component xwb_async_bridge
    generic (
      g_simulation          : integer;
      g_interface_mode      : t_wishbone_interface_mode      := CLASSIC;
      g_address_granularity : t_wishbone_address_granularity := WORD;
      g_cpu_address_width   : integer);
    port (
      rst_n_i     : in    std_logic;
      clk_sys_i   : in    std_logic;
      cpu_cs_n_i  : in    std_logic;
      cpu_wr_n_i  : in    std_logic;
      cpu_rd_n_i  : in    std_logic;
      cpu_bs_n_i  : in    std_logic_vector(3 downto 0);
      cpu_addr_i  : in    std_logic_vector(g_cpu_address_width-1 downto 0);
      cpu_data_b  : inout std_logic_vector(31 downto 0);
      cpu_nwait_o : out   std_logic;
      master_o    : out   t_wishbone_master_out;
      master_i    : in    t_wishbone_master_in);
  end component;

  component xwb_bus_fanout
    generic (
      g_num_outputs          : natural;
      g_bits_per_slave       : integer;
      g_address_granularity  : t_wishbone_address_granularity := WORD;
      g_slave_interface_mode : t_wishbone_interface_mode      := CLASSIC);
    port (
      clk_sys_i : in  std_logic;
      rst_n_i   : in  std_logic;
      slave_i   : in  t_wishbone_slave_in;
      slave_o   : out t_wishbone_slave_out;
      master_i  : in  t_wishbone_master_in_array(0 to g_num_outputs-1);
      master_o  : out t_wishbone_master_out_array(0 to g_num_outputs-1));
  end component;

  component xwb_crossbar
    generic (
      g_num_masters : integer;
      g_num_slaves  : integer;
      g_registered  : boolean;
      g_address     : t_wishbone_address_array;
      g_mask        : t_wishbone_address_array);
    port (
      clk_sys_i : in  std_logic;
      rst_n_i   : in  std_logic;
      slave_i   : in  t_wishbone_slave_in_array(g_num_masters-1 downto 0);
      slave_o   : out t_wishbone_slave_out_array(g_num_masters-1 downto 0);
      master_i  : in  t_wishbone_master_in_array(g_num_slaves-1 downto 0);
      master_o  : out t_wishbone_master_out_array(g_num_slaves-1 downto 0);
      sdb_sel_o : out std_logic_vector(g_num_masters-1 downto 0)); -- leave open!
  end component;

  -- Use the f_xwb_bridge_*_sdb to bridge a crossbar to another
  function f_xwb_bridge_manual_sdb(     -- take a manual bus size
    g_size     : t_wishbone_address;
    g_sdb_addr : t_wishbone_address) return t_sdb_bridge;

  function f_xwb_bridge_layout_sdb(     -- determine bus size from layout
    g_wraparound : boolean := true;
    g_layout     : t_sdb_record_array;
    g_sdb_addr   : t_wishbone_address) return t_sdb_bridge;

  function f_xwb_msi_manual_sdb(        -- take a manual bus size
    g_size     : t_wishbone_address) return t_sdb_msi;

  function f_xwb_msi_layout_sdb(     -- determine MSI size from layout
    g_layout     : t_sdb_record_array) return t_sdb_msi;

  component xwb_sdb_crossbar
    generic (
      g_num_masters : integer;
      g_num_slaves  : integer;
      g_registered  : boolean := false;
      g_wraparound  : boolean := true;
      g_layout      : t_sdb_record_array;
      g_sdb_addr    : t_wishbone_address;
      g_sdb_name    : string := "WB4-Crossbar-GSI   ");
    port (
      clk_sys_i     : in  std_logic;
      rst_n_i       : in  std_logic;
      slave_i       : in  t_wishbone_slave_in_array  (g_num_masters-1 downto 0);
      slave_o       : out t_wishbone_slave_out_array (g_num_masters-1 downto 0);
      msi_master_i  : in  t_wishbone_master_in_array (g_num_masters-1 downto 0) := (others => cc_dummy_master_in);
      msi_master_o  : out t_wishbone_master_out_array(g_num_masters-1 downto 0);
      master_i      : in  t_wishbone_master_in_array (g_num_slaves -1 downto 0);
      master_o      : out t_wishbone_master_out_array(g_num_slaves -1 downto 0);
      msi_slave_i   : in  t_wishbone_slave_in_array  (g_num_slaves -1 downto 0) := (others => cc_dummy_slave_in);
      msi_slave_o   : out t_wishbone_slave_out_array (g_num_slaves -1 downto 0));
  end component;
  
  component xwb_register_link -- puts a register of delay between crossbars
    port(
      clk_sys_i : in  std_logic;
      rst_n_i   : in  std_logic;
      slave_i   : in  t_wishbone_slave_in;
      slave_o   : out t_wishbone_slave_out;
      master_i  : in  t_wishbone_master_in;
      master_o  : out t_wishbone_master_out);
  end component;

  -- skidpad. acts like a fifo in wb flow control, but costs less
  component wb_skidpad is
  generic(
     g_adrbits   : natural   := 32
  );
  Port(
     clk_i        : std_logic;
     rst_n_i      : std_logic;

     push_i       : in  std_logic;
     pop_i        : in  std_logic;
     full_o       : out std_logic;
     empty_o      : out std_logic;

     adr_i        : in  std_logic_vector(g_adrbits-1 downto 0);
     dat_i        : in  std_logic_vector(32-1 downto 0);
     sel_i        : in  std_logic_vector(4-1 downto 0);
     we_i         : in  std_logic;

     adr_o        : out std_logic_vector(g_adrbits-1 downto 0);
     dat_o        : out std_logic_vector(32-1 downto 0);
     sel_o        : out std_logic_vector(4-1 downto 0);
     we_o         : out std_logic
  );
  end component;

  component sdb_rom is
    generic(
      g_layout  : t_sdb_record_array;
      g_masters : natural;
      g_bus_end : unsigned(63 downto 0);
      g_sdb_name    : string := "WB4-Crossbar-GSI   ");
    port(
      clk_sys_i : in  std_logic;
      master_i  : in  std_logic_vector(g_masters-1 downto 0);
      slave_i   : in  t_wishbone_slave_in;
      slave_o   : out t_wishbone_slave_out);
  end component;
  
  constant c_xwb_dma_sdb : t_sdb_device := (
    abi_class     => x"0000",              -- undocumented device
    abi_ver_major => x"01",
    abi_ver_minor => x"00",
    wbd_endian    => c_sdb_endian_big,
    wbd_width     => x"7",                 -- 8/16/32-bit port granularity
    sdb_component => (
      addr_first  => x"0000000000000000",
      addr_last   => x"000000000000001f",
      product     => (
        vendor_id => x"0000000000000651",  -- GSI
        device_id => x"cababa56",
        version   => x"00000001",
        date      => x"20120518",
        name      => "WB4-Streaming-DMA_0")));
  component xwb_dma is
    generic(
      -- Value 0 cannot stream
      -- Value 1 only slaves with async ACK can stream
      -- Value 2 only slaves with combined latency <= 2 can stream
      -- Value 3 only slaves with combined latency <= 6 can stream
      -- Value 4 only slaves with combined latency <= 14 can stream
      -- ....
      logRingLen : integer := 4
      );
    port(
      -- Common wishbone signals
      clk_i       : in  std_logic;
      rst_n_i     : in  std_logic;
      slave_i     : in  t_wishbone_slave_in;
      slave_o     : out t_wishbone_slave_out;
      -- Master reader port
      r_master_i  : in  t_wishbone_master_in;
      r_master_o  : out t_wishbone_master_out;
      -- Master writer port
      w_master_i  : in  t_wishbone_master_in;
      w_master_o  : out t_wishbone_master_out;
      -- Pulsed high completion signal
      interrupt_o : out std_logic
      );
  end component;

  -- If you reset one clock domain, you must reset BOTH!
  -- Release of the reset lines may be arbitrarily out-of-phase
  component xwb_clock_crossing is
    generic(
      g_size : natural := 16);
    port(
      -- Slave control port
      slave_clk_i    : in  std_logic;
      slave_rst_n_i  : in  std_logic;
      slave_i        : in  t_wishbone_slave_in;
      slave_o        : out t_wishbone_slave_out;
      -- Master reader port
      master_clk_i   : in  std_logic;
      master_rst_n_i : in  std_logic;
      master_i       : in  t_wishbone_master_in;
      master_o       : out t_wishbone_master_out;
      -- Flow control back-channel for acks
      slave_ready_o : out std_logic;
      slave_stall_i : in  std_logic := '0');
  end component;

  -- g_size is in words
  function f_xwb_dpram(g_size : natural) return t_sdb_device;
  component xwb_dpram
    generic (
      g_size                  : natural;
      g_init_file             : string                         := "";
      g_must_have_init_file   : boolean                        := true;
      g_slave1_interface_mode : t_wishbone_interface_mode      := CLASSIC;
      g_slave2_interface_mode : t_wishbone_interface_mode      := CLASSIC;
      g_slave1_granularity    : t_wishbone_address_granularity := WORD;
      g_slave2_granularity    : t_wishbone_address_granularity := WORD);
    port (
      clk_sys_i : in  std_logic;
      rst_n_i   : in  std_logic;
      slave1_i  : in  t_wishbone_slave_in;
      slave1_o  : out t_wishbone_slave_out;
      slave2_i  : in  t_wishbone_slave_in;
      slave2_o  : out t_wishbone_slave_out);
  end component;

  component xwb_dpram_mixed
  generic(
    g_size                  : natural := 16384;
    g_init_file             : string  := "";
    g_must_have_init_file   : boolean := true;
    g_swap_word_endianness  : boolean := true;
    g_slave1_interface_mode : t_wishbone_interface_mode;
    g_slave2_interface_mode : t_wishbone_interface_mode;
    g_dpram_port_a_width    : integer := 16;
    g_dpram_port_b_width    : integer := 32;
    g_slave1_granularity    : t_wishbone_address_granularity;
    g_slave2_granularity    : t_wishbone_address_granularity);
  port(
    clk_slave1_i  : in std_logic;
    clk_slave2_i  : in std_logic;
    rst_n_i       : in std_logic;

    slave1_i      : in  t_wishbone_slave_in;
    slave1_o      : out t_wishbone_slave_out;
    slave2_i      : in  t_wishbone_slave_in;
    slave2_o      : out t_wishbone_slave_out);
  end component;
  
  -- Just like the DMA controller, but constantly at address 0
  component xwb_streamer is
    generic(
      -- Value 0 cannot stream
      -- Value 1 only slaves with async ACK can stream
      -- Value 2 only slaves with combined latency = 2 can stream
      -- Value 3 only slaves with combined latency = 6 can stream
      -- Value 4 only slaves with combined latency = 14 can stream
      -- ....
      logRingLen : integer := 4
    );
    port(
      -- Common wishbone signals
      clk_i       : in  std_logic;
      rst_n_i     : in  std_logic;
      -- Master reader port
      r_master_i  : in  t_wishbone_master_in;
      r_master_o  : out t_wishbone_master_out;
      -- Master writer port
      w_master_i  : in  t_wishbone_master_in;
      w_master_o  : out t_wishbone_master_out);
  end component;


  constant c_xwb_gpio_port_sdb : t_sdb_device := (
    abi_class     => x"0000",              -- undocumented device
    abi_ver_major => x"01",
    abi_ver_minor => x"01",
    wbd_endian    => c_sdb_endian_big,
    wbd_width     => x"7",                 -- 8/16/32-bit port granularity
    sdb_component => (
      addr_first  => x"0000000000000000",
      addr_last   => x"00000000000000ff",
      product     => (
        vendor_id => x"000000000000CE42",  -- CERN
        device_id => x"441c5143",
        version   => x"00000001",
        date      => x"20121129",
        name      => "WB-GPIO-Port       ")));


  component wb_gpio_port
    generic (
      g_interface_mode         : t_wishbone_interface_mode      := CLASSIC;
      g_address_granularity    : t_wishbone_address_granularity := WORD;
      g_num_pins               : natural range 1 to 256;
      g_with_builtin_tristates : boolean                        := false);
    port (
      clk_sys_i  : in    std_logic;
      rst_n_i    : in    std_logic;
      wb_sel_i   : in    std_logic_vector(c_wishbone_data_width/8-1 downto 0);
      wb_cyc_i   : in    std_logic;
      wb_stb_i   : in    std_logic;
      wb_we_i    : in    std_logic;
      wb_adr_i   : in    std_logic_vector(7 downto 0);
      wb_dat_i   : in    std_logic_vector(c_wishbone_data_width-1 downto 0);
      wb_dat_o   : out   std_logic_vector(c_wishbone_data_width-1 downto 0);
      wb_ack_o   : out   std_logic;
      wb_stall_o : out   std_logic;
      gpio_b     : inout std_logic_vector(g_num_pins-1 downto 0);
      gpio_out_o : out   std_logic_vector(g_num_pins-1 downto 0);
      gpio_in_i  : in    std_logic_vector(g_num_pins-1 downto 0);
      gpio_oen_o : out   std_logic_vector(g_num_pins-1 downto 0));
  end component;

  component xwb_gpio_port
    generic (
      g_interface_mode         : t_wishbone_interface_mode      := CLASSIC;
      g_address_granularity    : t_wishbone_address_granularity := WORD;
      g_num_pins               : natural range 1 to 256;
      g_with_builtin_tristates : boolean);
    port (
      clk_sys_i  : in    std_logic;
      rst_n_i    : in    std_logic;
      slave_i    : in    t_wishbone_slave_in;
      slave_o    : out   t_wishbone_slave_out;
      desc_o     : out   t_wishbone_device_descriptor;
      gpio_b     : inout std_logic_vector(g_num_pins-1 downto 0);
      gpio_out_o : out   std_logic_vector(g_num_pins-1 downto 0);
      gpio_in_i  : in    std_logic_vector(g_num_pins-1 downto 0);
      gpio_oen_o : out   std_logic_vector(g_num_pins-1 downto 0));
  end component;

  constant c_xwb_i2c_master_sdb : t_sdb_device := (
    abi_class     => x"0000",              -- undocumented device
    abi_ver_major => x"01",
    abi_ver_minor => x"01",
    wbd_endian    => c_sdb_endian_big,
    wbd_width     => x"7",                 -- 8/16/32-bit port granularity
    sdb_component => (
      addr_first  => x"0000000000000000",
      addr_last   => x"00000000000000ff",
      product     => (
        vendor_id => x"000000000000CE42",  -- CERN
        device_id => x"123c5443",
        version   => x"00000001",
        date      => x"20121129",
        name      => "WB-I2C-Master      ")));


  component wb_i2c_master
    generic (
      g_interface_mode      : t_wishbone_interface_mode      := CLASSIC;
      g_address_granularity : t_wishbone_address_granularity := WORD;
      g_num_interfaces      : integer := 1);
    port (
      clk_sys_i    : in  std_logic;
      rst_n_i      : in  std_logic;
      wb_adr_i     : in  std_logic_vector(4 downto 0);
      wb_dat_i     : in  std_logic_vector(31 downto 0);
      wb_dat_o     : out std_logic_vector(31 downto 0);
      wb_sel_i     : in  std_logic_vector(3 downto 0);
      wb_stb_i     : in  std_logic;
      wb_cyc_i     : in  std_logic;
      wb_we_i      : in  std_logic;
      wb_ack_o     : out std_logic;
      wb_int_o     : out std_logic;
      wb_stall_o   : out std_logic;
      scl_pad_i    : in  std_logic_vector(g_num_interfaces-1 downto 0);
      scl_pad_o    : out std_logic_vector(g_num_interfaces-1 downto 0);
      scl_padoen_o : out std_logic_vector(g_num_interfaces-1 downto 0);
      sda_pad_i    : in  std_logic_vector(g_num_interfaces-1 downto 0);
      sda_pad_o    : out std_logic_vector(g_num_interfaces-1 downto 0);
      sda_padoen_o : out std_logic_vector(g_num_interfaces-1 downto 0));
  end component;

  component xwb_i2c_master
    generic (
      g_interface_mode      : t_wishbone_interface_mode      := CLASSIC;
      g_address_granularity : t_wishbone_address_granularity := WORD;
      g_num_interfaces      : integer := 1);
    port (
      clk_sys_i    : in  std_logic;
      rst_n_i      : in  std_logic;
      slave_i      : in  t_wishbone_slave_in;
      slave_o      : out t_wishbone_slave_out;
      desc_o       : out t_wishbone_device_descriptor;
      scl_pad_i    : in  std_logic_vector(g_num_interfaces-1 downto 0);
      scl_pad_o    : out std_logic_vector(g_num_interfaces-1 downto 0);
      scl_padoen_o : out std_logic_vector(g_num_interfaces-1 downto 0);
      sda_pad_i    : in  std_logic_vector(g_num_interfaces-1 downto 0);
      sda_pad_o    : out std_logic_vector(g_num_interfaces-1 downto 0);
      sda_padoen_o : out std_logic_vector(g_num_interfaces-1 downto 0));
  end component;

  component xwb_lm32
    generic (
      g_profile : string;
      g_reset_vector : std_logic_vector(31 downto 0) := x"00000000";
      g_sdb_address  : std_logic_vector(31 downto 0) := x"00000000");
    port (
      clk_sys_i : in  std_logic;
      rst_n_i   : in  std_logic;
      irq_i     : in  std_logic_vector(31 downto 0);
      dwb_o     : out t_wishbone_master_out;
      dwb_i     : in  t_wishbone_master_in;
      iwb_o     : out t_wishbone_master_out;
      iwb_i     : in  t_wishbone_master_in);
  end component;

  constant c_xwb_onewire_master_sdb : t_sdb_device := (
    abi_class     => x"0000",              -- undocumented device
    abi_ver_major => x"01",
    abi_ver_minor => x"01",
    wbd_endian    => c_sdb_endian_big,
    wbd_width     => x"7",                 -- 8/16/32-bit port granularity
    sdb_component => (
      addr_first  => x"0000000000000000",
      addr_last   => x"00000000000000ff",
      product     => (
        vendor_id => x"000000000000CE42",  -- CERN
        device_id => x"779c5443",
        version   => x"00000001",
        date      => x"20121129",
        name      => "WB-OneWire-Master  ")));

  component wb_onewire_master
    generic (
      g_interface_mode      : t_wishbone_interface_mode      := CLASSIC;
      g_address_granularity : t_wishbone_address_granularity := WORD;
      g_num_ports           : integer;
      g_ow_btp_normal       : string                         := "1.0";
      g_ow_btp_overdrive    : string                         := "5.0");
    port (
      clk_sys_i   : in  std_logic;
      rst_n_i     : in  std_logic;
      wb_cyc_i    : in  std_logic;
      wb_sel_i    : in  std_logic_vector(c_wishbone_data_width/8-1 downto 0);
      wb_stb_i    : in  std_logic;
      wb_we_i     : in  std_logic;
      wb_adr_i    : in  std_logic_vector(2 downto 0);
      wb_dat_i    : in  std_logic_vector(c_wishbone_data_width-1 downto 0);
      wb_dat_o    : out std_logic_vector(c_wishbone_data_width-1 downto 0);
      wb_ack_o    : out std_logic;
      wb_int_o    : out std_logic;
      wb_stall_o  : out std_logic;
      owr_pwren_o : out std_logic_vector(g_num_ports -1 downto 0);
      owr_en_o    : out std_logic_vector(g_num_ports -1 downto 0);
      owr_i       : in  std_logic_vector(g_num_ports -1 downto 0));
  end component;

  component xwb_onewire_master
    generic (
      g_interface_mode      : t_wishbone_interface_mode      := CLASSIC;
      g_address_granularity : t_wishbone_address_granularity := WORD;
      g_num_ports           : integer;
      g_ow_btp_normal       : string                         := "5.0";
      g_ow_btp_overdrive    : string                         := "1.0");
    port (
      clk_sys_i   : in  std_logic;
      rst_n_i     : in  std_logic;
      slave_i     : in  t_wishbone_slave_in;
      slave_o     : out t_wishbone_slave_out;
      desc_o      : out t_wishbone_device_descriptor;
      owr_pwren_o : out std_logic_vector(g_num_ports -1 downto 0);
      owr_en_o    : out std_logic_vector(g_num_ports -1 downto 0);
      owr_i       : in  std_logic_vector(g_num_ports -1 downto 0));
  end component;

  constant c_xwb_spi_sdb : t_sdb_device := (
    abi_class     => x"0000",              -- undocumented device
    abi_ver_major => x"01",
    abi_ver_minor => x"01",
    wbd_endian    => c_sdb_endian_big,
    wbd_width     => x"7",                 -- 8/16/32-bit port granularity
    sdb_component => (
      addr_first  => x"0000000000000000",
      addr_last   => x"000000000000001F",
      product     => (
        vendor_id => x"000000000000CE42",  -- CERN
        device_id => x"e503947e",       -- echo "WB-SPI.Control     " | md5sum | cut -c1-8
        version   => x"00000001",
        date      => x"20121116",
        name      => "WB-SPI.Control     ")));

  component wb_spi
    generic (
      g_interface_mode      : t_wishbone_interface_mode      := CLASSIC;
      g_address_granularity : t_wishbone_address_granularity := WORD;
      g_divider_len         : integer := 16;
      g_max_char_len        : integer := 128;
      g_num_slaves          : integer := 8);
    port (
      clk_sys_i  : in  std_logic;
      rst_n_i    : in  std_logic;
      wb_adr_i   : in  std_logic_vector(4 downto 0);
      wb_dat_i   : in  std_logic_vector(31 downto 0);
      wb_dat_o   : out std_logic_vector(31 downto 0);
      wb_sel_i   : in  std_logic_vector(3 downto 0);
      wb_stb_i   : in  std_logic;
      wb_cyc_i   : in  std_logic;
      wb_we_i    : in  std_logic;
      wb_ack_o   : out std_logic;
      wb_err_o   : out std_logic;
      wb_int_o   : out std_logic;
      wb_stall_o : out std_logic;
      pad_cs_o   : out std_logic_vector(g_num_slaves-1 downto 0);
      pad_sclk_o : out std_logic;
      pad_mosi_o : out std_logic;
      pad_miso_i : in  std_logic);
  end component;

  component xwb_spi
    generic (
      g_interface_mode      : t_wishbone_interface_mode      := CLASSIC;
      g_address_granularity : t_wishbone_address_granularity := WORD;
      g_divider_len         : integer := 16;
      g_max_char_len        : integer := 128;
      g_num_slaves          : integer := 8);
    port (
      clk_sys_i  : in  std_logic;
      rst_n_i    : in  std_logic;
      slave_i    : in  t_wishbone_slave_in;
      slave_o    : out t_wishbone_slave_out;
      desc_o     : out t_wishbone_device_descriptor;
      pad_cs_o   : out std_logic_vector(g_num_slaves-1 downto 0);
      pad_sclk_o : out std_logic;
      pad_mosi_o : out std_logic;
      pad_miso_i : in  std_logic);
  end component;

  component wb_simple_uart
    generic (
      g_with_virtual_uart   : boolean                        := false;
      g_with_physical_uart  : boolean                        := true;
      g_interface_mode      : t_wishbone_interface_mode      := CLASSIC;
      g_address_granularity : t_wishbone_address_granularity := WORD;
      g_vuart_fifo_size     : integer := 1024);
    port (
      clk_sys_i  : in  std_logic;
      rst_n_i    : in  std_logic;
      wb_adr_i   : in  std_logic_vector(4 downto 0);
      wb_dat_i   : in  std_logic_vector(31 downto 0);
      wb_dat_o   : out std_logic_vector(31 downto 0);
      wb_cyc_i   : in  std_logic;
      wb_sel_i   : in  std_logic_vector(3 downto 0);
      wb_stb_i   : in  std_logic;
      wb_we_i    : in  std_logic;
      wb_ack_o   : out std_logic;
      wb_stall_o : out std_logic;
      uart_rxd_i : in  std_logic := '1';
      uart_txd_o : out std_logic);
  end component;

  component xwb_simple_uart
    generic (
      g_with_virtual_uart   : boolean                        := false;
      g_with_physical_uart  : boolean                        := true;
      g_interface_mode      : t_wishbone_interface_mode      := CLASSIC;
      g_address_granularity : t_wishbone_address_granularity := WORD;
      g_vuart_fifo_size     : integer := 1024);
    port (
      clk_sys_i  : in  std_logic;
      rst_n_i    : in  std_logic;
      slave_i    : in  t_wishbone_slave_in;
      slave_o    : out t_wishbone_slave_out;
      desc_o     : out t_wishbone_device_descriptor;
      uart_rxd_i : in  std_logic := '1';
      uart_txd_o : out std_logic);
  end component;

  component wb_simple_pwm
    generic (
      g_num_channels        : integer range 1 to 8;
      g_regs_size           : integer range 1 to 16 := 16;
      g_default_period      : integer range 0 to 255 := 0;
      g_default_presc       : integer range 0 to 255 := 0;
      g_default_val         : integer range 0 to 255 := 0;
      g_interface_mode      : t_wishbone_interface_mode      := PIPELINED;
      g_address_granularity : t_wishbone_address_granularity := BYTE);
    port (
      clk_sys_i  : in  std_logic;
      rst_n_i    : in  std_logic;
      wb_adr_i   : in  std_logic_vector(5 downto 0);
      wb_dat_i   : in  std_logic_vector(31 downto 0);
      wb_dat_o   : out std_logic_vector(31 downto 0);
      wb_cyc_i   : in  std_logic;
      wb_sel_i   : in  std_logic_vector(3 downto 0);
      wb_stb_i   : in  std_logic;
      wb_we_i    : in  std_logic;
      wb_ack_o   : out std_logic;
      wb_stall_o : out std_logic;
      pwm_o      : out std_logic_vector(g_num_channels-1 downto 0));
  end component;

  component xwb_simple_pwm
    generic (
      g_num_channels        : integer range 1 to 8;
      g_regs_size           : integer range 1 to 16 := 16;
      g_default_period      : integer range 0 to 255 := 0;
      g_default_presc       : integer range 0 to 255 := 0;
      g_default_val         : integer range 0 to 255 := 0;
      g_interface_mode      : t_wishbone_interface_mode      := PIPELINED;
      g_address_granularity : t_wishbone_address_granularity := BYTE);
    port (
      clk_sys_i : in  std_logic;
      rst_n_i   : in  std_logic;
      slave_i   : in  t_wishbone_slave_in;
      slave_o   : out t_wishbone_slave_out;
      pwm_o     : out std_logic_vector(g_num_channels-1 downto 0));
  end component;

  component wb_tics
    generic (
      g_interface_mode      : t_wishbone_interface_mode      := CLASSIC;
      g_address_granularity : t_wishbone_address_granularity := WORD;
      g_period              : integer);
    port (
      rst_n_i    : in  std_logic;
      clk_sys_i  : in  std_logic;
      wb_adr_i   : in  std_logic_vector(3 downto 0);
      wb_dat_i   : in  std_logic_vector(c_wishbone_data_width-1 downto 0);
      wb_dat_o   : out std_logic_vector(c_wishbone_data_width-1 downto 0);
      wb_cyc_i   : in  std_logic;
      wb_sel_i   : in  std_logic_vector(c_wishbone_data_width/8-1 downto 0);
      wb_stb_i   : in  std_logic;
      wb_we_i    : in  std_logic;
      wb_ack_o   : out std_logic;
      wb_stall_o : out std_logic);
  end component;

  component xwb_tics
    generic (
      g_interface_mode      : t_wishbone_interface_mode      := CLASSIC;
      g_address_granularity : t_wishbone_address_granularity := WORD;
      g_period              : integer);
    port (
      clk_sys_i : in  std_logic;
      rst_n_i   : in  std_logic;
      slave_i   : in  t_wishbone_slave_in;
      slave_o   : out t_wishbone_slave_out;
      desc_o    : out t_wishbone_device_descriptor);
  end component;

  component wb_vic
    generic (
      g_interface_mode      : t_wishbone_interface_mode;
      g_address_granularity : t_wishbone_address_granularity;
      g_num_interrupts      : natural;
      g_init_vectors        : t_wishbone_address_array := cc_dummy_address_array;
      g_retry_timeout : integer := 0
      );
    port (
      clk_sys_i    : in  std_logic;
      rst_n_i      : in  std_logic;
      wb_adr_i     : in  std_logic_vector(c_wishbone_address_width-1 downto 0);
      wb_dat_i     : in  std_logic_vector(c_wishbone_data_width-1 downto 0);
      wb_dat_o     : out std_logic_vector(c_wishbone_data_width-1 downto 0);
      wb_cyc_i     : in  std_logic;
      wb_sel_i     : in  std_logic_vector(c_wishbone_data_width/8-1 downto 0);
      wb_stb_i     : in  std_logic;
      wb_we_i      : in  std_logic;
      wb_ack_o     : out std_logic;
      wb_stall_o   : out std_logic;
      irqs_i       : in  std_logic_vector(g_num_interrupts-1 downto 0);
      irq_master_o : out std_logic);
  end component;

  constant c_xwb_vic_sdb : t_sdb_device := (
    abi_class     => x"0000",              -- undocumented device
    abi_ver_major => x"01",
    abi_ver_minor => x"01",
    wbd_endian    => c_sdb_endian_big,
    wbd_width     => x"7",                 -- 8/16/32-bit port granularity
    sdb_component => (
      addr_first  => x"0000000000000000",
      addr_last   => x"00000000000000ff",
      product     => (
        vendor_id => x"000000000000CE42",  -- CERN
        device_id => x"00000013",
        version   => x"00000002",
        date      => x"20120113",
        name      => "WB-VIC-Int.Control ")));   

  component xwb_vic
    generic (
      g_interface_mode      : t_wishbone_interface_mode;
      g_address_granularity : t_wishbone_address_granularity;
      g_num_interrupts      : natural;
      g_init_vectors        : t_wishbone_address_array := cc_dummy_address_array;
    g_retry_timeout : integer := 0);

    port (
      clk_sys_i    : in  std_logic;
      rst_n_i      : in  std_logic;
      slave_i      : in  t_wishbone_slave_in;
      slave_o      : out t_wishbone_slave_out;
      irqs_i       : in  std_logic_vector(g_num_interrupts-1 downto 0);
      irq_master_o : out std_logic);
  end component;

  constant c_wb_serial_lcd_sdb : t_sdb_device := (
    abi_class     => x"0000", -- undocumented device
    abi_ver_major => x"01",
    abi_ver_minor => x"00",
    wbd_endian    => c_sdb_endian_big,
    wbd_width     => x"7", -- 8/16/32-bit port granularity
    sdb_component => (
    addr_first    => x"0000000000000000",
    addr_last     => x"00000000000000ff",
    product => (
    vendor_id     => x"0000000000000651", -- GSI
    device_id     => x"b77a5045",
    version       => x"00000001",
    date          => x"20130222",
    name          => "SERIAL-LCD-DISPLAY ")));
  component wb_serial_lcd
    generic(
      g_cols : natural := 40;
      g_rows : natural := 24;
      g_hold : natural := 15; -- How many times to repeat a line  (for sharpness)
      g_wait : natural := 1); -- How many cycles per state change (for 20MHz timing)
    port(
      slave_clk_i  : in  std_logic;
      slave_rstn_i : in  std_logic;
      slave_i      : in  t_wishbone_slave_in;
      slave_o      : out t_wishbone_slave_out;
      di_clk_i     : in  std_logic;
      di_scp_o     : out std_logic;
      di_lp_o      : out std_logic;
      di_flm_o     : out std_logic;
      di_dat_o     : out std_logic);
  end component;

  function f_wb_spi_flash_sdb(g_bits : natural) return t_sdb_device;
  component wb_spi_flash is
    generic(
      g_port_width             : natural   := 1;  -- 1 for EPCS, 4 for EPCQ
      g_addr_width             : natural   := 24; -- log of memory (24=16MB)
      g_idle_time              : natural   := 3;
      g_dummy_time             : natural   := 8;
      -- leave these at defaults if you have:
      --   a) slow clock, b) valid constraints, or c) registered in/outputs
      g_input_latch_edge       : std_logic := '1'; -- rising
      g_output_latch_edge      : std_logic := '0'; -- falling
      g_input_to_output_cycles : natural   := 1);  -- between 1 and 8
    port(
      clk_i     : in  std_logic;
      rstn_i    : in  std_logic;
      slave_i   : in  t_wishbone_slave_in;
      slave_o   : out t_wishbone_slave_out;
      
      -- For properly constrained designs, set clk_out_i = clk_in_i.
      clk_out_i : in  std_logic;
      clk_in_i  : in  std_logic;
      ncs_o     : out std_logic;
      oe_o      : out std_logic_vector(g_port_width-1 downto 0);
      asdi_o    : out std_logic_vector(g_port_width-1 downto 0);
      data_i    : in  std_logic_vector(g_port_width-1 downto 0);
      
      external_request_i : in  std_logic := '0'; -- JTAG wants to use SPI?
      external_granted_o : out std_logic);
  end component;

  -----------------------------------------------------------------------------
  -- I2C to Wishbone bridge, following protocol defined with ELMA
  -----------------------------------------------------------------------------
  component wb_i2c_bridge is
    generic
    (
      -- FSM watchdog timeout, see Appendix A in the component documentation for
      -- an example of setting this generic
      g_fsm_wdt : positive
    );
    port
    (
      -- Clock, reset
      clk_i      : in  std_logic;
      rst_n_i    : in  std_logic;

      -- I2C lines
      scl_i      : in  std_logic;
      scl_o      : out std_logic;
      scl_en_o   : out std_logic;
      sda_i      : in  std_logic;
      sda_o      : out std_logic;
      sda_en_o   : out std_logic;

      -- I2C address
      i2c_addr_i : in  std_logic_vector(6 downto 0);

      -- Status outputs
      -- TIP  : Transfer In Progress
      --        '1' when the I2C slave detects a matching I2C address, thus a
      --            transfer is in progress
      --        '0' when idle
      -- ERR  : Error
      --       '1' when the SysMon attempts to access an invalid WB slave
      --       '0' when idle
      -- WDTO : Watchdog timeout (single clock cycle pulse)
      --        '1' -- timeout of watchdog occured
      --        '0' -- when idle
      tip_o      : out std_logic;
      err_p_o    : out std_logic;
      wdto_p_o   : out std_logic;

      -- Wishbone master signals
      wbm_stb_o  : out std_logic;
      wbm_cyc_o  : out std_logic;
      wbm_sel_o  : out std_logic_vector(3 downto 0);
      wbm_we_o   : out std_logic;
      wbm_dat_i  : in  std_logic_vector(31 downto 0);
      wbm_dat_o  : out std_logic_vector(31 downto 0);
      wbm_adr_o  : out std_logic_vector(31 downto 0);
      wbm_ack_i  : in  std_logic;
      wbm_rty_i  : in  std_logic;
      wbm_err_i  : in  std_logic
    );
  end component wb_i2c_bridge;

  ------------------------------------------------------------------------------
  -- MultiBoot component
  ------------------------------------------------------------------------------
  component xwb_xil_multiboot is
    port
    (
      -- Clock and reset input ports
      clk_i   : in  std_logic;
      rst_n_i : in  std_logic;

      -- Wishbone ports
      wbs_i   : in  t_wishbone_slave_in;
      wbs_o   : out t_wishbone_slave_out;

      -- SPI ports
      spi_cs_n_o : out std_logic;
      spi_sclk_o : out std_logic;
      spi_mosi_o : out std_logic;
      spi_miso_i : in  std_logic
    );
  end component xwb_xil_multiboot;

  constant c_xwb_xil_multiboot_sdb : t_sdb_device := (
    abi_class     => x"0000",              -- undocumented device
    abi_ver_major => x"01",
    abi_ver_minor => x"00",
    wbd_endian    => c_sdb_endian_big,
    wbd_width     => x"7",                 -- 8/16/32-bit port granularity
    sdb_component => (
      addr_first  => x"0000000000000000",
      addr_last   => x"000000000000001f",
      product     => (
        vendor_id => x"000000000000CE42",  -- CERN
        device_id => x"11da333d",          -- echo "WB-Xilinx-MultiBoot" | md5sum | cut -c1-8
        version   => x"00000001",
        date      => x"20140313",
        name      => "WB-Xilinx-MultiBoot")));

  constant cc_dummy_sdb_device : t_sdb_device := (
    abi_class     => x"0000",              -- undocumented device
    abi_ver_major => x"01",
    abi_ver_minor => x"01",
    wbd_endian    => c_sdb_endian_big,
    wbd_width     => x"7",                 -- 8/16/32-bit port granularity
    sdb_component => (
      addr_first  => x"0000000000000000",
      addr_last   => x"00000000000000ff",
      product     => (
        vendor_id => x"000000000000CE42",  -- CERN
        device_id => x"ffffffff",
        version   => x"00000001",
        date      => x"20150722",
        name      => "Unused-Device      ")));

  
end wishbone_pkg;

package body wishbone_pkg is
   -- f_wb_wr: processes a write reqest to a slave register with select lines. valid modes are "owr", "set" and "clr"
   function f_wb_wr(pval : std_logic_vector; ival : std_logic_vector; sel : std_logic_vector; mode : string := "owr") return std_logic_vector is
      variable n_sel     : std_logic_vector(pval'range);
      variable n_val     : std_logic_vector(pval'range);
      variable result 	 : std_logic_vector(pval'range);   
   begin
     for i in pval'range loop 
      n_sel(i) := sel((i-pval'low) / 8); -- subtract the low index for when register width > wishbone data width
      n_val(i) := ival(i-pval'low);
     end loop;

     if(mode = "set") then  
      result := pval or (n_val and n_sel);
     elsif (mode = "clr") then
      result := pval and not (n_val and n_sel); 
     else
      result := (pval and not n_sel) or (n_val and n_sel);    
     end if;  
     
     return result;
   end f_wb_wr;
  
  function f_ceil_log2(x : natural) return natural is
  begin
    if x <= 1
    then return 0;
    else return f_ceil_log2((x+1)/2) +1;
    end if;
  end f_ceil_log2;

  function f_sdb_embed_product(product : t_sdb_product)
    return std_logic_vector             -- (319 downto 8)
  is
    variable result : std_logic_vector(319 downto 8);
  begin
    result(319 downto 256) := product.vendor_id;
    result(255 downto 224) := product.device_id;
    result(223 downto 192) := product.version;
    result(191 downto 160) := product.date;
    for i in 0 to 18 loop               -- string to ascii
      result(159-i*8 downto 152-i*8) :=
        std_logic_vector(to_unsigned(character'pos(product.name(i+1)), 8));
    end loop;
    return result;
  end;

  function f_sdb_extract_product(sdb_record : std_logic_vector(319 downto 8))
    return t_sdb_product
  is
    variable result : t_sdb_product;
  begin
    result.vendor_id := sdb_record(319 downto 256);
    result.device_id := sdb_record(255 downto 224);
    result.version   := sdb_record(223 downto 192);
    result.date      := sdb_record(191 downto 160);
    for i in 0 to 18 loop               -- ascii to string
      result.name(i+1) := character'val(to_integer(unsigned(sdb_record(159-i*8 downto 152-i*8))));
    end loop;
    return result;
  end;

  function f_sdb_embed_component(sdb_component : t_sdb_component; address : t_wishbone_address)
    return std_logic_vector             -- (447 downto 8)
  is
    variable result : std_logic_vector(447 downto 8);

    constant first : unsigned(63 downto 0) := unsigned(sdb_component.addr_first);
    constant last  : unsigned(63 downto 0) := unsigned(sdb_component.addr_last);
    variable base  : unsigned(63 downto 0) := (others => '0');
  begin
    base(address'length-1 downto 0) := unsigned(address);

    result(447 downto 384) := std_logic_vector(base);
    result(383 downto 320) := std_logic_vector(base + last - first);
    result(319 downto 8)   := f_sdb_embed_product(sdb_component.product);
    return result;
  end;

  function f_sdb_extract_component(sdb_record : std_logic_vector(447 downto 8))
    return t_sdb_component
  is
    variable result : t_sdb_component;
  begin
    result.addr_first := sdb_record(447 downto 384);
    result.addr_last  := sdb_record(383 downto 320);
    result.product    := f_sdb_extract_product(sdb_record(319 downto 8));
    return result;
  end;

  function f_sdb_embed_device(device : t_sdb_device; address : t_wishbone_address)
    return t_sdb_record
  is
    variable result : t_sdb_record;
  begin
    result(511 downto 496) := device.abi_class;
    result(495 downto 488) := device.abi_ver_major;
    result(487 downto 480) := device.abi_ver_minor;
    result(479 downto 456) := (others => '0');
    result(455)            := device.wbd_endian;
    result(454 downto 452) := (others => '0');
    result(451 downto 448) := device.wbd_width;
    result(447 downto 8)   := f_sdb_embed_component(device.sdb_component, address);
    result(7 downto 0)     := x"01";    -- device
    return result;
  end;

  function f_sdb_extract_device(sdb_record : t_sdb_record)
    return t_sdb_device
  is
    variable result : t_sdb_device;
  begin
    result.abi_class     := sdb_record(511 downto 496);
    result.abi_ver_major := sdb_record(495 downto 488);
    result.abi_ver_minor := sdb_record(487 downto 480);
    result.wbd_endian    := sdb_record(452);
    result.wbd_width     := sdb_record(451 downto 448);
    result.sdb_component := f_sdb_extract_component(sdb_record(447 downto 8));

    assert sdb_record(7 downto 0) = x"01"
      report "Cannot extract t_sdb_device from record of type " & integer'image(to_integer(unsigned(sdb_record(7 downto 0)))) & "."
      severity failure;
    
    return result;
  end;

  function f_sdb_embed_msi(msi : t_sdb_msi; address : t_wishbone_address)
    return t_sdb_record
  is
    variable result : t_sdb_record;
  begin
    result(511 downto 456) := (others => '0');
    result(455)            := msi.wbd_endian;
    result(454 downto 452) := (others => '0');
    result(451 downto 448) := msi.wbd_width;
    result(447 downto 8)   := f_sdb_embed_component(msi.sdb_component, address);
    result(7 downto 0)     := x"03";    -- msi
    return result;
  end;

  function f_sdb_extract_msi(sdb_record : t_sdb_record)
    return t_sdb_msi
  is
    variable result : t_sdb_msi;
  begin
    result.wbd_endian    := sdb_record(452);
    result.wbd_width     := sdb_record(451 downto 448);
    result.sdb_component := f_sdb_extract_component(sdb_record(447 downto 8));

    assert sdb_record(7 downto 0) = x"03"
      report "Cannot extract t_sdb_msi from record of type " & integer'image(to_integer(unsigned(sdb_record(7 downto 0)))) & "."
      severity failure;
    
    return result;
  end;

  function f_sdb_embed_integration(integr : t_sdb_integration)
    return t_sdb_record
  is
    variable result : t_sdb_record;
  begin
    result(511 downto 320) := (others => '0');
    result(319 downto 8)   := f_sdb_embed_product(integr.product);
    result(7 downto 0)     := x"80"; -- integration record
    return result;
  end f_sdb_embed_integration;

  function f_sdb_extract_integration(sdb_record : t_sdb_record)
    return t_sdb_integration
  is
    variable result : t_sdb_integration;
  begin
    result.product := f_sdb_extract_product(sdb_record(319 downto 8));

    assert sdb_record(7 downto 0) = x"80"
    report "Cannot extract t_sdb_integration from record of type " & Integer'image(to_integer(unsigned(sdb_record(7 downto 0)))) & "."
    severity Failure;

    return result;
  end f_sdb_extract_integration;

  function f_sdb_embed_repo_url(url : t_sdb_repo_url)
    return t_sdb_record
  is
    variable result : t_sdb_record;
  begin
    result(511 downto 8) := f_string2svl(url.repo_url);
    result(  7 downto 0) := x"81"; -- repo_url record
    return result;
  end;

  function f_sdb_extract_repo_url(sdb_record : t_sdb_record)
    return t_sdb_repo_url
  is
    variable result : t_sdb_repo_url;
  begin
    result.repo_url     := f_slv2string(sdb_record(511 downto 8));

    assert sdb_record(7 downto 0) = x"81"
    report "Cannot extract t_sdb_repo_url from record of type " & Integer'image(to_integer(unsigned(sdb_record(7 downto 0)))) & "."
    severity Failure;

    return result;
  end;

  function f_sdb_embed_synthesis(syn : t_sdb_synthesis)
    return t_sdb_record
  is
    variable result : t_sdb_record;
  begin
    result(511 downto 384) := f_string2svl(syn.syn_module_name);
    result(383 downto 256) := f_string2bits(syn.syn_commit_id);
    result(255 downto 192) := f_string2svl(syn.syn_tool_name);
    result(191 downto 160) := syn.syn_tool_version;
    result(159 downto 128) := syn.syn_date;
    result(127 downto   8) := f_string2svl(syn.syn_username);
    result(  7 downto   0) := x"82"; -- synthesis record
    return result;
  end;

  function f_sdb_extract_synthesis(sdb_record : t_sdb_record)
    return t_sdb_synthesis
  is
    variable result : t_sdb_synthesis;
  begin
    result.syn_module_name  := f_slv2string(sdb_record(511 downto 384));
    result.syn_commit_id    := f_bits2string(sdb_record(383 downto 256));
    result.syn_tool_name    := f_slv2string(sdb_record(255 downto 192));
    result.syn_tool_version := sdb_record(191 downto 160);
    result.syn_date         := sdb_record(159 downto 128);
    result.syn_username     := f_slv2string(sdb_record(127 downto   8));

    assert sdb_record(7 downto 0) = x"82"
    report "Cannot extract t_sdb_repo_url from record of type " & Integer'image(to_integer(unsigned(sdb_record(7 downto 0)))) & "."
    severity Failure;

    return result;
  end;

  function f_sdb_embed_bridge(bridge : t_sdb_bridge; address : t_wishbone_address)
    return t_sdb_record
  is
    variable result : t_sdb_record;

    constant first : unsigned(63 downto 0) := unsigned(bridge.sdb_component.addr_first);
    constant child : unsigned(63 downto 0) := unsigned(bridge.sdb_child);
    variable base  : unsigned(63 downto 0) := (others => '0');
  begin
    base(address'length-1 downto 0) := unsigned(address);

    result(511 downto 448) := std_logic_vector(base + child - first);
    result(447 downto 8)   := f_sdb_embed_component(bridge.sdb_component, address);
    result(7 downto 0)     := x"02";    -- bridge
    return result;
  end;

  function f_sdb_extract_bridge(sdb_record : t_sdb_record)
    return t_sdb_bridge
  is
    variable result : t_sdb_bridge;
  begin
    result.sdb_child     := sdb_record(511 downto 448);
    result.sdb_component := f_sdb_extract_component(sdb_record(447 downto 8));

    assert sdb_record(7 downto 0) = x"02"
      report "Cannot extract t_sdb_bridge from record of type " & integer'image(to_integer(unsigned(sdb_record(7 downto 0)))) & "."
      severity failure;
    
    return result;
  end;
  
  function f_sdb_auto_device(device : t_sdb_device; enable : boolean := true; name: string := "")
    return t_sdb_record
  is
    constant c_zero  : t_wishbone_address := (others => '0');
    variable v_device: t_sdb_device := device;
    variable v_empty : t_sdb_record := (others => '0');
  begin
    v_empty(7 downto 0) := x"f1";
    if name /= "" then
      v_device.sdb_component.product.name := f_string_fix_len(name , 19, ' ', false);
    end if;
    if enable then
      v_empty := f_sdb_embed_device(v_device, c_zero);
    end if;
    return v_empty;
  end f_sdb_auto_device;
  
  function f_sdb_auto_bridge(bridge : t_sdb_bridge; enable : boolean := true; name: string := "")
    return t_sdb_record
  is
    constant c_zero  : t_wishbone_address := (others => '0');
    variable v_bridge: t_sdb_bridge := bridge;
    variable v_empty : t_sdb_record := (others => '0');
  begin
    v_empty(7 downto 0) := x"f2";
    if name /= "" then
      v_bridge.sdb_component.product.name := f_string_fix_len(name , 19, ' ', false);
    end if;
    if enable then
      v_empty := f_sdb_embed_bridge(v_bridge, c_zero);
    end if;
    return v_empty;
  end f_sdb_auto_bridge;
  
  function f_sdb_auto_msi(msi : t_sdb_msi; enable : boolean := true)
    return t_sdb_record
  is
    constant c_zero  : t_wishbone_address := (others => '0');
    variable v_empty : t_sdb_record := (others => '0');
  begin
    v_empty(7 downto 0) := x"f3";
    if enable then
      return f_sdb_embed_msi(msi, c_zero);
    else
      return v_empty;
    end if;
  end f_sdb_auto_msi;
  
  subtype t_usdb_address is unsigned(63 downto 0);
  type t_usdb_address_array is array(natural range <>) of t_usdb_address;
  
  -- We map devices by placing the smallest ones first.
  -- This is guaranteed to pack the maximum number of devices in the smallest space.
  -- If a device has an address != 0, we leave it alone and let the crossbar confirm
  -- that the address does not cause a conflict.
  function f_sdb_auto_layout_helper(records : t_sdb_record_array)
    return t_usdb_address_array
  is
    alias c_records : t_sdb_record_array(records'length-1 downto 0) is records;
    constant c_zero : t_usdb_address := (others => '0');
    
    constant c_used_entries : natural := c_records'length + 1;
    constant c_rom_entries  : natural := 2**f_ceil_log2(c_used_entries);
    constant c_rom_bytes    : natural := c_rom_entries * c_sdb_device_length / 8;
        
    variable v_component : t_sdb_component;
    variable v_sizes     : t_usdb_address_array(c_records'length downto 0);
    variable v_address   : t_usdb_address_array(c_records'length downto 0);
    variable v_bus_map   : std_logic_vector(c_records'length downto 0) := (others => '0');
    variable v_bus_cursor: unsigned(63 downto 0) := (others => '0');
    variable v_msi_map   : std_logic_vector(c_records'length downto 0) := (others => '0');
    variable v_msi_cursor: unsigned(63 downto 0) := (others => '0');
    variable v_increment : unsigned(63 downto 0) := (others => '0');
    variable v_type      : std_logic_vector(7 downto 0);
  begin
    -- First, extract the length of the devices, ignoring those not to be mapped
    for i in c_records'range loop
      v_component := f_sdb_extract_component(c_records(i)(447 downto 8));
      v_sizes(i)   := unsigned(v_component.addr_last);
      v_address(i) := unsigned(v_component.addr_first);
      
      -- Silently round up to a power of two; the crossbar will give a warning for us
      for j in 62 downto 0 loop
        v_sizes(i)(j) := v_sizes(i)(j+1) or v_sizes(i)(j);
      end loop;
      
      -- Only map devices/bridges at address zero
      if v_address(i) = c_zero then
        v_type := c_records(i)(7 downto 0);
        case v_type is
          when x"01" => v_bus_map(i) := '1';
          when x"02" => v_bus_map(i) := '1';
          when x"03" => v_msi_map(i) := '1';
          when others => null;
        end case;
      end if;
    end loop;
    
    -- Assign the SDB record a spot as well
    v_address(c_records'length) := (others => '0');
    v_sizes(c_records'length) := to_unsigned(c_rom_bytes-1, 64);
    v_bus_map(c_records'length) := '1';
    
    -- Start assigning addresses
    for j in 0 to 63 loop
      v_increment := (others => '0');
      v_increment(j) := '1';
      
      for i in 0 to c_records'length loop
        if v_bus_map(i) = '1' and v_sizes(i)(j) = '0' then
          v_bus_map(i) := '0';
          v_address(i) := v_bus_cursor;
          v_bus_cursor := v_bus_cursor + v_increment;
        end if;
        if v_msi_map(i) = '1' and v_sizes(i)(j) = '0' then
          v_msi_map(i) := '0';
          v_address(i) := v_msi_cursor;
          v_msi_cursor := v_msi_cursor + v_increment;
        end if;
      end loop;
      
      -- Round up to the next required alignment
      if v_bus_cursor(j) = '1' then
        v_bus_cursor := v_bus_cursor + v_increment;
      end if;
      if v_msi_cursor(j) = '1' then
        v_msi_cursor := v_msi_cursor + v_increment;
      end if;
    end loop;
    
    return v_address;
  end f_sdb_auto_layout_helper;
  
  function f_sdb_auto_layout(records : t_sdb_record_array)
    return t_sdb_record_array
  is
    alias    c_records : t_sdb_record_array(records'length-1 downto 0) is records;
    variable v_typ     : std_logic_vector(7 downto 0);
    variable v_result  : t_sdb_record_array(c_records'range) := c_records;
    constant c_address : t_usdb_address_array := f_sdb_auto_layout_helper(c_records);
    variable v_address : t_wishbone_address;
  begin
    -- Put the addresses into the mapping
    for i in v_result'range loop
      v_typ     := c_records(i)(7 downto 0);
      v_address := std_logic_vector(c_address(i)(t_wishbone_address'range));

      case v_typ is
        when x"01" => v_result(i) := f_sdb_embed_device(f_sdb_extract_device(v_result(i)), v_address);
        when x"02" => v_result(i) := f_sdb_embed_bridge(f_sdb_extract_bridge(v_result(i)), v_address);
        when x"03" => v_result(i) := f_sdb_embed_msi   (f_sdb_extract_msi   (v_result(i)), v_address);
        when others => null;
      end case;
    end loop;
    
    return v_result;
  end f_sdb_auto_layout;
  
  function f_sdb_auto_layout(slaves : t_sdb_record_array; masters : t_sdb_record_array)
    return t_sdb_record_array
  is begin
    return f_sdb_auto_layout(masters & slaves);
  end f_sdb_auto_layout;
  
  function f_sdb_auto_sdb(records : t_sdb_record_array)
    return t_wishbone_address
  is
    alias    c_records : t_sdb_record_array(records'length-1 downto 0) is records;
    constant c_address : t_usdb_address_array(c_records'length downto 0) := f_sdb_auto_layout_helper(c_records);
  begin
    return std_logic_vector(c_address(c_records'length)(t_wishbone_address'range));
  end f_sdb_auto_sdb;
  
  function f_sdb_auto_sdb(slaves : t_sdb_record_array; masters : t_sdb_record_array)
    return t_wishbone_address
  is begin
    return f_sdb_auto_sdb(masters & slaves);
  end f_sdb_auto_sdb;

--**************************************************************************************************************************--
-- START MAT's NEW FUNCTIONS FROM 18th Oct 2013
------------------------------------------------------------------------------------------------------------------------------


   function f_sdb_create_array(g_enum_dev_id    : boolean := false;
                            g_dev_id_offs    : natural := 0;
                            g_enum_dev_name  : boolean := false;
                            g_dev_name_offs  : natural := 0;       
                            device           : t_sdb_device; 
                            instances        : natural := 1) 
   return t_sdb_record_array is
      variable result   : t_sdb_record_array(instances-1 downto 0);  
      variable i,j, pos : natural;
      variable dev, newdev      : t_sdb_device;
      variable serial_no : string(1 to 3);
      variable text_possible : boolean := false; 
   begin
      dev := device;
             
      report "### Creating " & integer'image(instances) & " x " & dev.sdb_component.product.name
      severity note;       
      for i in 0 to instances-1 loop
         newdev := dev;        
         if(g_enum_dev_id) then         
            dev.sdb_component.product.device_id :=  
            std_logic_vector( unsigned(dev.sdb_component.product.device_id) 
                              + to_unsigned(i+g_dev_id_offs, dev.sdb_component.product.device_id'length));        
         end if;
         if(g_enum_dev_name) then         
         -- find end of name
            
            for j in dev.sdb_component.product.name'length downto 1 loop
               if(dev.sdb_component.product.name(j) /= ' ') then               
                  pos := j;                  
                  exit;
               end if;
            end loop;
         -- convert i+g_dev_name_offs to string
            serial_no := f_string_fix_len(integer'image(i+g_dev_name_offs), serial_no'length);
            report "### Now: " & serial_no & " of " & dev.sdb_component.product.name severity note;
         -- check if space is sufficient
            assert (serial_no'length+1 <= dev.sdb_component.product.name'length - pos)
            report "Not enough space in namestring of sdb_device " & dev.sdb_component.product.name
            & " to add serial number " & serial_no & ". Space available " & 
            integer'image(dev.sdb_component.product.name'length-pos-1) & ", required " 
            & integer'image(serial_no'length+1)    
            severity Failure;
         end if;
         if(g_enum_dev_name) then
            newdev.sdb_component.product.name(pos+1) := '_'; 
            for j in 1 to serial_no'length loop
               newdev.sdb_component.product.name(pos+1+j) := serial_no(j);
            end loop;            
         end if;
          
      -- insert
         report "### to: " & newdev.sdb_component.product.name severity note;
         result(i) := f_sdb_embed_device(newdev, (others=>'0'));
      end loop;
      return result;
   end f_sdb_create_array;

   function f_sdb_join_arrays(a : t_sdb_record_array; b : t_sdb_record_array) return t_sdb_record_array is
      variable result   : t_sdb_record_array(a'length+b'length-1 downto 0);  
      variable i : natural;
   begin
      for i in 0 to a'left loop
         result(i) := a(i);
      end loop;
      for i in 0 to b'left loop
         result(i+a'length) := b(i);
      end loop;
      return result;
   end f_sdb_join_arrays;


   function f_sdb_extract_base_addr(sdb_record : t_sdb_record) return std_logic_vector is
   begin
      return sdb_record(447 downto 384);
   end f_sdb_extract_base_addr;

   function f_sdb_extract_end_addr(sdb_record : t_sdb_record) return std_logic_vector is
   begin
      return sdb_record(383 downto 320);
   end f_sdb_extract_end_addr;


   function f_align_addr_offset(offs : unsigned; this_rng : unsigned; prev_rng : unsigned)
   return unsigned is
      variable this_pow, prev_pow   : natural;  
      variable start, env, result : unsigned(63 downto 0) := (others => '0');
   begin
      start(offs'left downto 0) := offs;   
      --calculate address envelopes (next power of 2) for previous and this component and choose the larger one
      this_pow := f_hot_to_bin(std_logic_vector(this_rng)); 
      prev_pow := f_hot_to_bin(std_logic_vector(prev_rng));    
      -- no max(). thank you very much, std_numeric :-/   
      if(this_pow >= prev_pow) then
         env(this_pow) := '1';
      else
         env(prev_pow) := '1';
      end if;
      --round up to the next multiple of the envelope...
      if(prev_rng /= 0) then   
         result := start + env - (start mod env);
      else
         result := start;   --...except for first element, result is start. 
      end if;
      return result;
   end f_align_addr_offset;


 -- generates aligned address map for an sdb_record_array, accepts optional start offset 
   function f_sdb_automap_array(sdb_array : t_sdb_record_array; start_offset : t_wishbone_address := (others => '0'))
   return t_sdb_record_array is
      constant len         : natural := sdb_array'length;
      variable this_rng    : unsigned(63 downto 0) := (others => '0');   
      variable prev_rng    : unsigned(63 downto 0) := (others => '0');   
      variable prev_offs   : unsigned(63 downto 0) := (others => '0');   
      variable this_offs   : unsigned(63 downto 0) := (others => '0');   
      variable device      : t_sdb_device;
      variable bridge      : t_sdb_bridge;  
      variable sdb_type    : std_logic_vector(7 downto 0);
      variable i           : natural;
      variable result      : t_sdb_record_array(sdb_array'length-1 downto 0); -- last 
   begin
   
      prev_offs(start_offset'left downto 0) := unsigned(start_offset);
      --traverse the array   
      for i in 0 to len-1 loop
         -- find the fitting extraction function by evaling the type byte. 
         -- could also use the component, but it's safer to use Wes' embed and extract functions.      
         sdb_type := sdb_array(i)(7 downto 0);
         case sdb_type is
            --device         
            when x"01"  => device      := f_sdb_extract_device(sdb_array(i));
                           this_rng    := unsigned(device.sdb_component.addr_last) - unsigned(device.sdb_component.addr_first);                      
                           this_offs   := f_align_addr_offset(prev_offs, this_rng, prev_rng);
                           result(i)   := f_sdb_embed_device(device, std_logic_vector(this_offs(31 downto 0)));
            --bridge
            when x"02"  => bridge      := f_sdb_extract_bridge(sdb_array(i));
                           this_rng    := unsigned(bridge.sdb_component.addr_last) - unsigned(bridge.sdb_component.addr_first);
                           this_offs   := f_align_addr_offset(prev_offs, this_rng, prev_rng);
                           result(i)   := f_sdb_embed_bridge(bridge, std_logic_vector(this_offs(31 downto 0)) );
            --other
            when others => result(i) := sdb_array(i);   
         end case;
         -- doesnt hurt because this_* doesnt change if its not a device or bridge
         prev_rng    := this_rng;
         prev_offs   := this_offs;
      end loop;
      report "##* " & integer'image(sdb_array'length) & " Elements, last address: " & f_bits2string(std_logic_vector(this_offs+this_rng)) severity Note;
      return result;
   end f_sdb_automap_array;


  -- find place for sdb rom on crossbar and return address. try to put it in an address gap.
   function f_sdb_create_rom_addr(sdb_array : t_sdb_record_array) return t_wishbone_address is
      constant len                  : natural := sdb_array'length;
      constant rom_bytes            : natural := (2**f_ceil_log2(sdb_array'length + 1)) * (c_sdb_device_length / 8);
      variable result               : t_wishbone_address  := (others => '0');
      variable this_base, this_end  : unsigned(63 downto 0)          := (others => '0');    
      variable prev_base, prev_end  : unsigned(63 downto 0)          := (others => '0');
      variable rom_base             : unsigned(63 downto 0)          := (others => '0');
      variable sdb_type             : std_logic_vector(7 downto 0);     
   begin
      --traverse the array   
      for i in 0 to len-1 loop     
         sdb_type := sdb_array(i)(7 downto 0);
         if(sdb_type = x"01" or sdb_type = x"02") then
            -- get         
            this_base := unsigned(f_sdb_extract_base_addr(sdb_array(i)));
            this_end  := unsigned(f_sdb_extract_end_addr(sdb_array(i)));
            if(unsigned(result) = 0) then
               rom_base := f_align_addr_offset(prev_base, to_unsigned(rom_bytes-1, 64), (prev_end-prev_base));
               if(rom_base + to_unsigned(rom_bytes, 64) <= this_base) then
                  result := std_logic_vector(rom_base(t_wishbone_address'left downto 0));
               end if;   
            end if;
            prev_base := this_base;
            prev_end  := this_end;      
         end if;
      end loop;   
      -- if there was no gap to fit the sdb rom, place it at the end   
      if(unsigned(result) = 0) then
            result := std_logic_vector(f_align_addr_offset(this_base, to_unsigned(rom_bytes-1, 64), 
                                       this_end-this_base)(t_wishbone_address'left downto 0));   
      end if;      
      return result;
   end f_sdb_create_rom_addr; 
------------------------------------------------------------------------------------------------------------------------------
-- END MAT's NEW FUNCTIONS FROM  18th Oct 2013
------------------------------------------------------------------------------------------------------------------------------

  function f_sdb_bus_end(
    g_wraparound : boolean;
    g_layout     : t_sdb_record_array;
    g_sdb_addr   : t_wishbone_address;
    msi          : boolean) return unsigned
  is
    alias c_layout : t_sdb_record_array(g_layout'length-1 downto 0) is g_layout;

    -- How much space does the ROM need?
    constant c_used_entries : natural := c_layout'length + 1;
    constant c_rom_entries  : natural := 2**f_ceil_log2(c_used_entries);  -- next power of 2
    constant c_sdb_bytes    : natural := c_sdb_device_length / 8;
    constant c_rom_bytes    : natural := c_rom_entries * c_sdb_bytes;
    
    variable result : unsigned(63 downto 0) := (others => '0');
    variable typ    : std_logic_vector(7 downto 0);
    variable last   : unsigned(63 downto 0);
  begin
    if not msi then
      -- The ROM will be an addressed slave as well
      result := (others => '0');
      result(g_sdb_addr'range) := unsigned(g_sdb_addr);
      result := result + to_unsigned(c_rom_bytes, 64) - 1;
    end if;
    
    for i in c_layout'range loop
      typ  := c_layout(i)(7 downto 0);
      last := unsigned(f_sdb_extract_component(c_layout(i)(447 downto 8)).addr_last);
      case typ is
        when x"01" => if not msi and last > result then result := last; end if;
        when x"02" => if not msi and last > result then result := last; end if;
        when x"03" => if     msi and last > result then result := last; end if;
        when others => null;
      end case;
    end loop;
    
    -- round result up to a power of two -1
    for i in 62 downto 0 loop
      result(i) := result(i) or result(i+1);
    end loop;
    
    if not g_wraparound then
      result := (others => '0');
      for i in 0 to c_wishbone_address_width-1 loop
        result(i) := '1';
      end loop;
    end if;
    
    return result;
  end f_sdb_bus_end;

  function f_xwb_bridge_manual_sdb(
    g_size     : t_wishbone_address;
    g_sdb_addr : t_wishbone_address) return t_sdb_bridge
  is
    variable result : t_sdb_bridge;
  begin
    result.sdb_child                                      := (others => '0');
    result.sdb_child(c_wishbone_address_width-1 downto 0) := g_sdb_addr;

    result.sdb_component.addr_first                                     := (others => '0');
    result.sdb_component.addr_last                                      := (others => '0');
    result.sdb_component.addr_last(c_wishbone_address_width-1 downto 0) := g_size;

    result.sdb_component.product.vendor_id := x"0000000000000651";  -- GSI
    result.sdb_component.product.device_id := x"eef0b198";
    result.sdb_component.product.version   := x"00000001";
    result.sdb_component.product.date      := x"20120511";
    result.sdb_component.product.name      := "WB4-Bridge-GSI     ";

    return result;
  end f_xwb_bridge_manual_sdb;
  
  function f_xwb_bridge_layout_sdb(     -- determine bus size from layout
    g_wraparound : boolean := true;
    g_layout     : t_sdb_record_array;
    g_sdb_addr   : t_wishbone_address) return t_sdb_bridge
  is
    variable address : t_wishbone_address;
  begin
    address := std_logic_vector(f_sdb_bus_end(g_wraparound, g_layout, g_sdb_addr, false)(address'range));
    return f_xwb_bridge_manual_sdb(address, g_sdb_addr);
  end f_xwb_bridge_layout_sdb;

  function f_xwb_msi_manual_sdb(
    g_size     : t_wishbone_address) return t_sdb_msi
  is
    variable result : t_sdb_msi;
  begin
    result.wbd_endian := '0';
    result.wbd_width  := x"7";

    result.sdb_component.addr_first                                     := (others => '0');
    result.sdb_component.addr_last                                      := (others => '0');
    result.sdb_component.addr_last(c_wishbone_address_width-1 downto 0) := g_size;

    result.sdb_component.product.vendor_id := x"0000000000000651";  -- GSI
    result.sdb_component.product.device_id := x"aa7bfb3c";
    result.sdb_component.product.version   := x"00000001";
    result.sdb_component.product.date      := x"20160422";
    result.sdb_component.product.name      := "WB4-MSI-Bridge-GSI ";

    return result;
  end f_xwb_msi_manual_sdb;
  
  function f_xwb_msi_layout_sdb(     -- determine MSI size from layout
    g_layout     : t_sdb_record_array) return t_sdb_msi
  is
    constant zero : t_wishbone_address := (others => '0');
    variable address : t_wishbone_address;
  begin
    address := std_logic_vector(f_sdb_bus_end(true, g_layout, zero, true)(address'range));
    return f_xwb_msi_manual_sdb(address);
  end f_xwb_msi_layout_sdb;
  
  function f_xwb_dpram(g_size : natural) return t_sdb_device
  is
    variable result : t_sdb_device;
  begin
    result.abi_class     := x"0001";    -- RAM device
    result.abi_ver_major := x"01";
    result.abi_ver_minor := x"00";
    result.wbd_width     := x"7";       -- 32/16/8-bit supported
    result.wbd_endian    := c_sdb_endian_big;

    result.sdb_component.addr_first := (others => '0');
    result.sdb_component.addr_last  := std_logic_vector(to_unsigned(g_size*4-1, 64));

    result.sdb_component.product.vendor_id := x"000000000000CE42";  -- CERN
    result.sdb_component.product.device_id := x"66cfeb52";
    result.sdb_component.product.version   := x"00000001";
    result.sdb_component.product.date      := x"20120305";
    result.sdb_component.product.name      := "WB4-BlockRAM       ";

    return result;
  end f_xwb_dpram;

  function f_bits2string(s : std_logic_vector) return string is
    --- extend length to full hex nibble
    variable result : string((s'length+7)/4 downto 1);
    variable s_norm : std_logic_vector(result'length*4-1 downto 0) := (others=>'0');
    variable cut : natural;
    variable nibble: std_logic_vector(3 downto 0);
    constant len : natural := result'length;
  begin
    s_norm(s'length-1 downto 0) := s;
    for i in len-1 downto 0 loop
      nibble := s_norm(i*4+3 downto i*4);
      case nibble is
        when "0000" => result(i+1) := '0';
        when "0001" => result(i+1) := '1';
        when "0010" => result(i+1) := '2';
        when "0011" => result(i+1) := '3';
        when "0100" => result(i+1) := '4';
        when "0101" => result(i+1) := '5';
        when "0110" => result(i+1) := '6';
        when "0111" => result(i+1) := '7';
        when "1000" => result(i+1) := '8';
        when "1001" => result(i+1) := '9';
        when "1010" => result(i+1) := 'a';
        when "1011" => result(i+1) := 'b';
        when "1100" => result(i+1) := 'c';
        when "1101" => result(i+1) := 'd';
        when "1110" => result(i+1) := 'e';
        when "1111" => result(i+1) := 'f';
        when others => result(i+1) := 'X';
      end case;
    end loop;

    -- trim leading 0s
    strip : for i in result'length downto 1 loop
      cut := i;
      exit strip when result(i) /= '0';
    end loop;

    return "0x" & result(cut downto 1);
  end f_bits2string;

  -- Converts string (hex number, without leading 0x) to std_logic_vector
  function f_string2bits(s : string) return std_logic_vector is
    constant len : natural := s'length;
    variable slv : std_logic_vector(s'length*4-1 downto 0);
    variable nibble : std_logic_vector(3 downto 0);
  begin
    for i in 0 to len-1 loop
      case s(i+1) is
        when '0' => nibble := X"0";
        when '1' => nibble := X"1";
        when '2' => nibble := X"2";
        when '3' => nibble := X"3";
        when '4' => nibble := X"4";
        when '5' => nibble := X"5";
        when '6' => nibble := X"6";
        when '7' => nibble := X"7";
        when '8' => nibble := X"8";
        when '9' => nibble := X"9";
        when 'a' => nibble := X"A";
        when 'A' => nibble := X"A";
        when 'b' => nibble := X"B";
        when 'B' => nibble := X"B";
        when 'c' => nibble := X"C";
        when 'C' => nibble := X"C";
        when 'd' => nibble := X"D";
        when 'D' => nibble := X"D";
        when 'e' => nibble := X"E";
        when 'E' => nibble := X"E";
        when 'f' => nibble := X"F";
        when 'F' => nibble := X"F";
        when others => nibble := "XXXX";
      end case;
      if s'ascending then
        slv(slv'length-(i*4)-1 downto slv'length-(i+1)*4) := nibble;
      else
        slv(((i+1)*4)-1 downto i*4) := nibble;
      end if;
    end loop;
    return slv;
  end f_string2bits;

  -- Converts string to ascii (std_logic_vector)
  function f_string2svl (s : string) return std_logic_vector is
    constant len : natural := s'length;
    variable slv : std_logic_vector((s'length * 8) - 1 downto 0);
  begin
    for i in 0 to len-1 loop
      slv(slv'high-i*8 downto (slv'high-7)-i*8) :=
        std_logic_vector(to_unsigned(character'pos(s(i+1)), 8));
    end loop;
    return slv;
  end f_string2svl;

  -- Converts ascii (std_logic_vector) to string
  function f_slv2string (slv : std_logic_vector) return string is
    constant len : natural := slv'length;
    variable s : string(1 to slv'length/8);
  begin
    for i in 0 to (len/8)-1 loop
      s(i+1) := character'val(to_integer(unsigned(slv(slv'high-i*8 downto (slv'high-7)-i*8))));
    end loop;
    return s;
  end f_slv2string;

    -- pads a string of unknown length to a given length (useful for integer'image)
  function f_string_fix_len ( s : string; ret_len : natural := 10; fill_char : character := '0'; justify_right : boolean := true ) return string is
      variable ret_v : string (1 to ret_len);
      constant pad_len : integer := ret_len - s'length ;
      variable pad_v : string (1 to abs(pad_len));
   begin
      if pad_len < 1 then
         ret_v := s(ret_v'range);
      else
         pad_v := (others => fill_char);
         if justify_right then
           ret_v := pad_v & s;
         else
           ret_v := s & pad_v ;
         end if;
      end if;
      return ret_v;
   end f_string_fix_len;

   -- do not synthesize
   function f_hot_to_bin(x : std_logic_vector) return natural is
      variable rv : natural;
   begin
      rv := 0;
      -- if there are few ones set in _x_ then the most significant will be
      -- translated to bin
      for i in 0 to x'left loop
         if x(i) = '1' then
           rv := i+1;
         end if;
      end loop;
      return rv;
  end function;  

  function f_wb_spi_flash_sdb(g_bits : natural) return t_sdb_device is
    variable result : t_sdb_device := (
      abi_class     => x"0000", -- undocumented device
      abi_ver_major => x"01",
      abi_ver_minor => x"02",
      wbd_endian    => c_sdb_endian_big,
      wbd_width     => x"7", -- 8/16/32-bit port granularity
      sdb_component => (
      addr_first    => x"0000000000000000",
      addr_last     => x"0000000000ffffff",
      product => (
      vendor_id     => x"0000000000000651", -- GSI
      device_id     => x"5cf12a1c",
      version       => x"00000002",
      date          => x"20140417",
      name          => "SPI-FLASH-16M-MMAP ")));
  begin
    result.sdb_component.addr_last := std_logic_vector(to_unsigned(2**g_bits-1, 64));
    return result;
  end f_wb_spi_flash_sdb;
  
end wishbone_pkg;

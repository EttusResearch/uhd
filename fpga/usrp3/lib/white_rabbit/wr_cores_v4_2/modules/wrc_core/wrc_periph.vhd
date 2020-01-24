-------------------------------------------------------------------------------
-- Title      : WhiteRabbit PTP Core peripherials
-- Project    : WhiteRabbit
-------------------------------------------------------------------------------
-- File       : wrc_periph.vhd
-- Author     : Grzegorz Daniluk <grzegorz.daniluk@cern.ch>
-- Company    : CERN (BE-CO-HT)
-- Created    : 2011-04-04
-- Last update: 2017-04-25
-- Platform   : FPGA-generics
-- Standard   : VHDL
-------------------------------------------------------------------------------
-- Description:
-- WRC_PERIPH integrates WRC_SYSCON, UART/VUART, 1-Wire Master, WRPC_DIAGS
-- 
-------------------------------------------------------------------------------
--
-- Copyright (c) 2012 - 2017 CERN
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
use ieee.numeric_std.all;

library work;
use work.wrcore_pkg.all;
use work.wishbone_pkg.all;
use work.sysc_wbgen2_pkg.all;
use work.wrc_diags_wbgen2_pkg.all;

entity wrc_periph is
  generic(
    g_board_name      : string  := "NA  ";
    g_flash_secsz_kb    : integer := 256;        -- default for SVEC (M25P128)
    g_flash_sdbfs_baddr : integer := 16#600000#; -- default for SVEC (M25P128)
    g_phys_uart       : boolean := true;
    g_virtual_uart    : boolean := false;
    g_cntr_period     : integer := 62500;
    g_mem_words       : integer := 16384;   --in 32-bit words
    g_vuart_fifo_size : integer := 1024;
    g_diag_id         : integer := 0;
    g_diag_ver        : integer := 0;
    g_diag_ro_size    : integer := 0;
    g_diag_rw_size    : integer := 0
    );
  port(
    clk_sys_i : in std_logic;
    rst_n_i   : in std_logic;

    rst_net_n_o : out std_logic;
    rst_wrc_n_o : out std_logic;

    led_red_o   : out std_logic;
    led_green_o : out std_logic;
    scl_o       : out std_logic;
    scl_i       : in  std_logic;
    sda_o       : out std_logic;
    sda_i       : in  std_logic;
    sfp_scl_o   : out std_logic;
    sfp_scl_i   : in  std_logic;
    sfp_sda_o   : out std_logic;
    sfp_sda_i   : in  std_logic;
    sfp_det_i   : in  std_logic;
    memsize_i   : in  std_logic_vector(3 downto 0);
    btn1_i      : in  std_logic;
    btn2_i      : in  std_logic;
    spi_sclk_o  : out std_logic;
    spi_ncs_o   : out std_logic;
    spi_mosi_o  : out std_logic;
    spi_miso_i  : in  std_logic;

    slave_i : in  t_wishbone_slave_in_array(0 to 3);
    slave_o : out t_wishbone_slave_out_array(0 to 3);

    uart_rxd_i : in  std_logic;
    uart_txd_o : out std_logic;

    -- 1-Wire
    owr_pwren_o: out std_logic_vector(1 downto 0);
    owr_en_o : out std_logic_vector(1 downto 0);
    owr_i    : in  std_logic_vector(1 downto 0);

    -- optional diagnostics from external HDL modules
    diag_array_in  : in  t_generic_word_array(g_diag_ro_size-1 downto 0) := (others=>(others=>'0'));
    diag_array_out : out t_generic_word_array(g_diag_rw_size-1 downto 0)
    );
end wrc_periph;

architecture struct of wrc_periph is

  function f_cnt_memsize(words : integer) return std_logic_vector is
  begin
    return std_logic_vector(to_unsigned(words * 4 / 1024 / 16 - 1, 4));
    -- *4     - to get size in bytes
    -- /1024  - to get size in kB
    -- /16 -1 - to get size in format of MEMSIZE@sysc_hwfr register
  end f_cnt_memsize;

  function f_board_name_conv(name : string) return std_logic_vector is
    variable ret : std_logic_vector(31 downto 0);
  begin
    assert (name'length= 4)
    report "Board name has to be exactly 4 letters string" severity failure;
    ret(31 downto 24) := std_logic_vector(to_unsigned(character'pos(name(1)), 8));
    ret(23 downto 16) := std_logic_vector(to_unsigned(character'pos(name(2)), 8));
    ret(15 downto  8) := std_logic_vector(to_unsigned(character'pos(name(3)), 8));
    ret( 7 downto  0) := std_logic_vector(to_unsigned(character'pos(name(4)), 8));
    return ret;
  end f_board_name_conv;

  signal sysc_regs_i : t_sysc_in_registers;
  signal sysc_regs_o : t_sysc_out_registers;

  signal cntr_div      : unsigned(23 downto 0);
  signal cntr_tics     : unsigned(31 downto 0);
  signal cntr_overflow : std_logic;
  
  signal rst_wrc_n_o_reg : std_logic := '1';
  signal diag_adr : unsigned(15 downto 0);
  signal diag_dat : std_logic_vector(31 downto 0);
  signal diag_out_regs : t_generic_word_array(g_diag_rw_size - 1 downto 0);
  signal diag_in       : t_generic_word_array(g_diag_ro_size + g_diag_rw_size-1 downto 0);
  signal wrpc_diag_regs_in     : t_wrc_diags_in_registers;
  signal wrpc_diag_regs_out    : t_wrc_diags_out_registers;

begin

  rst_wrc_n_o <= rst_n_i and rst_wrc_n_o_reg;
  process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if(rst_n_i = '0') then
        rst_net_n_o <= '0';
        rst_wrc_n_o_reg <= '1';
      else

        if(sysc_regs_o.rstr_trig_wr_o = '1' and sysc_regs_o.rstr_trig_o = x"deadbee") then
          rst_wrc_n_o_reg <= not sysc_regs_o.rstr_rst_o;
        end if; 
            
        rst_net_n_o <= not sysc_regs_o.gpsr_net_rst_o;
      end if; 
    end if; 
  end process;
  
  -------------------------------------
  -- LEDs
  -------------------------------------
  process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if(sysc_regs_o.gpsr_led_link_o = '1') then
        led_red_o <= '1';
      elsif(sysc_regs_o.gpcr_led_link_o = '1') then
        led_red_o <= '0';
      end if;

      if(sysc_regs_o.gpsr_led_stat_o = '1') then
        led_green_o <= '1';
      elsif(sysc_regs_o.gpcr_led_stat_o = '1') then
        led_green_o <= '0';
      end if;
    end if;
  end process;

  -------------------------------------
  -- buttons
  -------------------------------------
  sysc_regs_i.gpsr_btn1_i <= btn1_i;
  sysc_regs_i.gpsr_btn2_i <= btn2_i;

  -------------------------------------
  -- MEMSIZE
  -------------------------------------
  sysc_regs_i.hwfr_memsize_i(3 downto 0) <= f_cnt_memsize(g_mem_words);

  -------------------------------------
  -- BOARD NAME and Flash info
  -------------------------------------
  sysc_regs_i.hwir_name_i         <= f_board_name_conv(g_board_name);
  sysc_regs_i.hwfr_storage_sec_i  <= std_logic_vector(to_unsigned(g_flash_secsz_kb, 16));
  sysc_regs_i.hwfr_storage_type_i <= "00";  -- for now these parameters are only for Flash
  sysc_regs_i.sdbfs_baddr_i       <= std_logic_vector(to_unsigned(g_flash_sdbfs_baddr, 32));

  -------------------------------------
  -- TIMER
  -------------------------------------
  sysc_regs_i.tvr_i      <= std_logic_vector(cntr_tics);
  sysc_regs_i.tcr_tdiv_i <= (others => '0');

  process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if(rst_n_i = '0') then
        cntr_div      <= (others => '0');
        cntr_overflow <= '0';
      elsif(sysc_regs_o.tcr_enable_o = '1') then
        if(cntr_div = g_cntr_period-1) then
          cntr_div      <= (others => '0');
          cntr_overflow <= '1';
        else
          cntr_div      <= cntr_div + 1;
          cntr_overflow <= '0';
        end if;
      end if;
    end if;
  end process;

  --msec counter
  process(clk_sys_i)
  begin
    if(rising_edge(clk_sys_i)) then
      if(rst_n_i = '0') then
        cntr_tics <= (others => '0');
      elsif(cntr_overflow = '1') then
        cntr_tics <= cntr_tics + 1;
      end if;
    end if;
  end process;

  -------------------------------------
  -- I2C - FMC
  -------------------------------------
  p_drive_i2c : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        scl_o <= '1';
        sda_o <= '1';
      else
        if(sysc_regs_o.gpsr_fmc_sda_load_o = '1' and sysc_regs_o.gpsr_fmc_sda_o = '1') then
          sda_o <= '1';
        elsif(sysc_regs_o.gpcr_fmc_sda_o = '1') then
          sda_o <= '0';
        end if;

        if(sysc_regs_o.gpsr_fmc_scl_load_o = '1' and sysc_regs_o.gpsr_fmc_scl_o = '1') then
          scl_o <= '1';
        elsif(sysc_regs_o.gpcr_fmc_scl_o = '1') then
          scl_o <= '0';
        end if;
      end if;
    end if;
  end process;

  sysc_regs_i.gpsr_fmc_sda_i <= sda_i;
  sysc_regs_i.gpsr_fmc_scl_i <= scl_i;

  -------------------------------------
  -- I2C - SFP
  -------------------------------------
  p_drive_sfpi2c : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        sfp_scl_o <= '1';
        sfp_sda_o <= '1';
      else
        if(sysc_regs_o.gpsr_sfp_sda_load_o = '1' and sysc_regs_o.gpsr_sfp_sda_o = '1') then
          sfp_sda_o <= '1';
        elsif(sysc_regs_o.gpcr_sfp_sda_o = '1') then
          sfp_sda_o <= '0';
        end if;

        if(sysc_regs_o.gpsr_sfp_scl_load_o = '1' and sysc_regs_o.gpsr_sfp_scl_o = '1') then
          sfp_scl_o <= '1';
        elsif(sysc_regs_o.gpcr_sfp_scl_o = '1') then
          sfp_scl_o <= '0';
        end if;
      end if;
    end if;
  end process;

  sysc_regs_i.gpsr_sfp_sda_i <= sfp_sda_i;
  sysc_regs_i.gpsr_sfp_scl_i <= sfp_scl_i;

  sysc_regs_i.gpsr_sfp_det_i <= sfp_det_i;

  -------------------------------------
  -- SPI - Flash
  -------------------------------------
  p_drive_spi: process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        spi_sclk_o  <= '0';
        spi_mosi_o  <= '0';
        spi_ncs_o   <= '1';
      else
        if(sysc_regs_o.gpsr_spi_sclk_load_o = '1' and sysc_regs_o.gpsr_spi_sclk_o = '1') then
          spi_sclk_o <= '1';
        elsif(sysc_regs_o.gpcr_spi_sclk_o = '1') then
          spi_sclk_o <= '0';
        end if;

        if(sysc_regs_o.gpsr_spi_ncs_load_o = '1' and sysc_regs_o.gpsr_spi_ncs_o = '1') then
          spi_ncs_o <= '1';
        elsif(sysc_regs_o.gpcr_spi_cs_o = '1') then
          spi_ncs_o <= '0';
        end if;

        if(sysc_regs_o.gpsr_spi_mosi_load_o = '1' and sysc_regs_o.gpsr_spi_mosi_o = '1') then
          spi_mosi_o <= '1';
        elsif(sysc_regs_o.gpcr_spi_mosi_o = '1') then
          spi_mosi_o <= '0';
        end if;
      end if;
    end if;
  end process;
  sysc_regs_i.gpsr_spi_sclk_i <= '0';
  sysc_regs_i.gpsr_spi_ncs_i  <= '0';
  sysc_regs_i.gpsr_spi_mosi_i <= '0';
  sysc_regs_i.gpsr_spi_miso_i <= spi_miso_i;


  -------------------------------------
  -- DIAG to/from external modules
  -------------------------------------
  -- first, provide all the constants
  sysc_regs_i.diag_info_id_i  <= std_logic_vector(to_unsigned(g_diag_id, 16));
  sysc_regs_i.diag_info_ver_i <= std_logic_vector(to_unsigned(g_diag_ver, 16));
  sysc_regs_i.diag_nw_ro_i  <= std_logic_vector(to_unsigned(g_diag_ro_size, 16));
  sysc_regs_i.diag_nw_rw_i <= std_logic_vector(to_unsigned(g_diag_rw_size, 16));

  diag_array_out <= diag_out_regs;
  -- r/w registers can be also read
  diag_in(g_diag_rw_size - 1 downto 0) <= diag_out_regs;
  -- r/o array after r/w registers for reading
  diag_in(g_diag_ro_size + g_diag_rw_size-1 downto g_diag_rw_size) <= diag_array_in;

  p_diag_rw: process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        diag_adr <= (others=>'0');
        diag_dat <= (others=>'0');
      else
        if sysc_regs_o.diag_cr_adr_load_o = '1' then
          diag_adr <= unsigned(sysc_regs_o.diag_cr_adr_o);
        end if;
        if sysc_regs_o.diag_dat_load_o = '1' then
          diag_dat <= sysc_regs_o.diag_dat_o;
        end if;
      end if;
    end if;
  end process;

  sysc_regs_i.diag_cr_adr_i    <= std_logic_vector(diag_adr);
  GEN_DIAG_NODAT: if g_diag_rw_size = 0 and g_diag_ro_size = 0 generate
    sysc_regs_i.diag_dat_i <= (others=>'0');
  end generate;
  GEN_DIAG_DAT: if g_diag_rw_size /= 0 or g_diag_ro_size /= 0 generate
    sysc_regs_i.diag_dat_i <= diag_in(to_integer(diag_adr));
  end generate;

  -- Write request for each r/w register
  GEN_DIAG_W: if g_diag_rw_size > 0 generate
    GEN_LOOP: for I in 0 to g_diag_rw_size-1 generate

      process(clk_sys_i)
      begin
        if rising_edge(clk_sys_i) then
          if rst_n_i = '0' then
            diag_out_regs(I) <= (others=>'0');
          elsif sysc_regs_o.diag_cr_adr_load_o = '1' and sysc_regs_o.diag_cr_rw_o = '1' and
            to_integer(unsigned(sysc_regs_o.diag_cr_adr_o)) = I then
              diag_out_regs(I) <= diag_dat;
          end if;
        end if;
      end process;

    end generate;
  end generate;

  GEN_NODIAG_W: if g_diag_rw_size = 0 generate
    diag_array_out <= (others=>(others=>'0'));
  end generate;

  ----------------------------------------
  -- SYSCON
  ----------------------------------------
  SYSCON : entity work.wrc_syscon_wb
    port map (
      rst_n_i    => rst_n_i,
      clk_sys_i  => clk_sys_i,
      wb_adr_i   => slave_i(0).adr(6 downto 2), -- shift address for word addressing
      wb_dat_i   => slave_i(0).dat,
      wb_dat_o   => slave_o(0).dat,
      wb_cyc_i   => slave_i(0).cyc,
      wb_sel_i   => slave_i(0).sel,
      wb_stb_i   => slave_i(0).stb,
      wb_we_i    => slave_i(0).we,
      wb_ack_o   => slave_o(0).ack,
      wb_stall_o => slave_o(0).stall,
      regs_i     => sysc_regs_i,
      regs_o     => sysc_regs_o);

  slave_o(0).err <= '0';
  slave_o(0).rty <= '0';
  slave_o(0).int <= '0';

  --------------------------------------
  -- UART
  --------------------------------------
  UART : xwb_simple_uart
    generic map(
      g_with_virtual_uart   => g_virtual_uart,
      g_with_physical_uart  => g_phys_uart,
      g_interface_mode      => PIPELINED,
      g_address_granularity => BYTE,
      g_vuart_fifo_size     => g_vuart_fifo_size
      )
    port map(
      clk_sys_i => clk_sys_i,
      rst_n_i   => rst_n_i,

      -- Wishbone
      slave_i => slave_i(1),
      slave_o => slave_o(1),
      desc_o  => open,

      uart_rxd_i => uart_rxd_i,
      uart_txd_o => uart_txd_o
      );

  --------------------------------------
  -- 1-WIRE
  --------------------------------------
  ONEWIRE : xwb_onewire_master
    generic map(
      g_interface_mode      => PIPELINED,
      g_address_granularity => BYTE,
      g_num_ports           => 2,
      g_ow_btp_normal       => "5.0",
      g_ow_btp_overdrive    => "1.0"
      )
    port map(
      clk_sys_i => clk_sys_i,
      rst_n_i   => rst_n_i,

      -- Wishbone
      slave_i => slave_i(2),
      slave_o => slave_o(2),
      desc_o  => open,

      owr_pwren_o => owr_pwren_o,
      owr_en_o => owr_en_o,
      owr_i    => owr_i
      );

  --------------------------------------
  -- WRPC Diags
  --------------------------------------

  -- access through WB (PCI/VME/application) to diagnostics of WRPC
  DIAGS: xwr_diags_wb
    generic map(
      g_interface_mode      => PIPELINED,
      g_address_granularity => BYTE
    )
    port map(
      rst_n_i   => rst_n_i,
      clk_sys_i => clk_sys_i,

      slave_i   => slave_i(3),
      slave_o   => slave_o(3),

      regs_i    => wrpc_diag_regs_in,
      regs_o    => wrpc_diag_regs_out
    );

   -- the information written to syscon WB registers by LM32 are available to the
   -- user via diag WB registers
   -- It might look strange that we use two WB modules for that. Since both LM32
   -- and user application need to access these registers through the same
   -- wishbone interface we would needed these registers to be R/W. By creating
   -- another module (xwr_diags_wb) we make read-only registers to be read by
   -- the external tool. We want to minimize the possibility of user application
   -- overwriting these values, thus we want them to be read-only.
   sysc_regs_i.wdiag_ctrl_data_snapshot_i      <= wrpc_diag_regs_out.ctrl_data_snapshot_o;
   wrpc_diag_regs_in.ctrl_data_valid_i         <= sysc_regs_o.wdiag_ctrl_data_valid_o;
   wrpc_diag_regs_in.wdiag_sstat_wr_mode_i     <= sysc_regs_o.wdiag_sstat_wr_mode_o;
   wrpc_diag_regs_in.wdiag_sstat_servostate_i  <= sysc_regs_o.wdiag_sstat_servostate_o;
   wrpc_diag_regs_in.wdiag_pstat_link_i        <= sysc_regs_o.wdiag_pstat_link_o;
   wrpc_diag_regs_in.wdiag_pstat_locked_i      <= sysc_regs_o.wdiag_pstat_locked_o;
   wrpc_diag_regs_in.wdiag_ptpstat_ptpstate_i  <= sysc_regs_o.wdiag_ptpstat_ptpstate_o;
   wrpc_diag_regs_in.wdiag_astat_aux_i         <= sysc_regs_o.wdiag_astat_aux_o;
   wrpc_diag_regs_in.wdiag_txfcnt_i            <= sysc_regs_o.wdiag_txfcnt_o;
   wrpc_diag_regs_in.wdiag_rxfcnt_i            <= sysc_regs_o.wdiag_rxfcnt_o;
   wrpc_diag_regs_in.wdiag_sec_msb_i           <= sysc_regs_o.wdiag_sec_msb_o;
   wrpc_diag_regs_in.wdiag_sec_lsb_i           <= sysc_regs_o.wdiag_sec_lsb_o;
   wrpc_diag_regs_in.wdiag_ns_i                <= sysc_regs_o.wdiag_ns_o;
   wrpc_diag_regs_in.wdiag_mu_msb_i            <= sysc_regs_o.wdiag_mu_msb_o;
   wrpc_diag_regs_in.wdiag_mu_lsb_i            <= sysc_regs_o.wdiag_mu_lsb_o;
   wrpc_diag_regs_in.wdiag_dms_msb_i           <= sysc_regs_o.wdiag_dms_msb_o;
   wrpc_diag_regs_in.wdiag_dms_lsb_i           <= sysc_regs_o.wdiag_dms_lsb_o;
   wrpc_diag_regs_in.wdiag_asym_i              <= sysc_regs_o.wdiag_asym_o;
   wrpc_diag_regs_in.wdiag_cko_i               <= sysc_regs_o.wdiag_cko_o;
   wrpc_diag_regs_in.wdiag_setp_i              <= sysc_regs_o.wdiag_setp_o;
   wrpc_diag_regs_in.wdiag_ucnt_i              <= sysc_regs_o.wdiag_ucnt_o;
   wrpc_diag_regs_in.wdiag_temp_i              <= sysc_regs_o.wdiag_temp_o;

end struct;

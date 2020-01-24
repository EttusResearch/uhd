-------------------------------------------------------------------------------
-- Title      : Rx CRC checker
-- Project    : White Rabbit 
-------------------------------------------------------------------------------
-- File       : ep_rx_crc_size_check.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Platform   : FPGA-generic
-- Standard   : VHDL '93
-------------------------------------------------------------------------------
--
-- Copyright (c) 2011 - 2017 CERN / BE-CO-HT
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
use work.gencores_pkg.all;              -- for gc_crc_gen
use work.endpoint_private_pkg.all;
use work.endpoint_pkg.all;
use work.wr_fabric_pkg.all;
use work.ep_wbgen2_pkg.all;
use work.ep_crc32_pkg.all;

-- 1st deframing pipeline stage - CRC/PCS error/Size checker

entity ep_rx_crc_size_check is
  generic
    (
      g_use_new_crc : boolean := false);
  port(
    clk_sys_i : in std_logic;
    rst_n_i   : in std_logic;

    snk_fab_i  : in  t_ep_internal_fabric;
    snk_dreq_o : out std_logic;

    src_fab_o  : out t_ep_internal_fabric;
    src_dreq_i : in  std_logic;

    regs_i         : in  t_ep_out_registers;
    rmon_pcs_err_o : out std_logic;
    rmon_giant_o   : out std_logic;
    rmon_runt_o    : out std_logic;
    rmon_crc_err_o : out std_logic
    );

end ep_rx_crc_size_check;

architecture behavioral of ep_rx_crc_size_check is

  constant c_MIN_FRAME_SIZE : integer := 64;

  component ep_rx_bypass_queue
    generic (
      g_size  : integer;
      g_width : integer);
    port (
      rst_n_i : in  std_logic;
      clk_i   : in  std_logic;
      d_i     : in  std_logic_vector(g_width-1 downto 0);
      valid_i : in  std_logic;
      dreq_o  : out std_logic;
      q_o     : out std_logic_vector(g_width-1 downto 0);
      valid_o : out std_logic;
      dreq_i  : in  std_logic;
      flush_i : in  std_logic;
      purge_i : in  std_logic;
      empty_o : out std_logic);
  end component;


  type t_state is (ST_WAIT_FRAME, ST_DATA, ST_OOB);

  signal crc_gen_enable        : std_logic;
  signal crc_gen_reset         : std_logic;
  signal crc_match, crc_match2 : std_logic;

  signal crc_cur         : std_logic_vector(31 downto 0);
  signal crc_in_data     : std_logic_vector(15 downto 0);
  signal crc_last_is_odd : std_logic;

  signal byte_cntr     : unsigned(13 downto 0);
  signal is_runt       : std_logic;
  signal is_giant      : std_logic;
  signal size_check_ok : std_logic;

  signal state : t_state;

  signal q_flush, q_empty : std_logic;
  signal q_purge          : std_logic;
  signal q_in, q_out      : std_logic_vector(17 downto 0);
  signal q_bytesel        : std_logic;
  signal q_dvalid_in      : std_logic;
  signal q_dvalid_out     : std_logic;
  signal q_dreq_out       : std_logic;
  
  -- bypass_queue stuff comes here
  constant c_crc_size    : integer := 2;
  constant c_dat_width   : integer := 18;

  type t_queue_array is array(0 to c_crc_size-1) of std_logic_vector(c_dat_width-1 downto 0);

  signal q_data           : t_queue_array;
  signal q_valid          : std_logic_vector(c_crc_size-1 downto 0);

  signal qempty, qfull    : std_logic;
  signal sreg_enable      : std_logic;
  signal oob_in           : std_logic;
  signal dat_in           : std_logic;
  signal valid_mask       : std_logic;
  signal err_on_giant     : std_logic;

  function f_queue_occupation(q : std_logic_vector; check_empty : std_logic) return std_logic is
    variable i : integer;
  begin
    for i in 0 to q'length-1 loop
      if(q(i) = check_empty) then
        return '0';
      end if;
    end loop;  -- i
    return '1';
  end function;

begin  -- behavioral

  crc_gen_reset  <= snk_fab_i.sof or (not rst_n_i);
  crc_gen_enable <= '1' when (snk_fab_i.addr = c_WRF_DATA and snk_fab_i.dvalid = '1') else '0';

  gen_old_crc : if(g_use_new_crc = false) generate
    U_rx_crc_generator : gc_crc_gen
      generic map (
        g_polynomial              => x"04C11DB7",
        g_init_value              => x"ffffffff",
        g_residue                 => x"1cdf4421",
        g_data_width              => 16,
        g_half_width              => 8,
        g_sync_reset              => 1,
        g_dual_width              => 1,
        g_registered_match_output => false)
      port map (
        clk_i   => clk_sys_i,
        rst_i   => crc_gen_reset,
        en_i    => crc_gen_enable,
        half_i  => snk_fab_i.bytesel,
        data_i  => snk_fab_i.data(15 downto 0),
        match_o => crc_match,
        crc_o   => open);
  end generate gen_old_crc;

  gen_new_crc : if(g_use_new_crc = true) generate

    crc_in_data(15 downto 8) <= snk_fab_i.data(15 downto 8);
    crc_in_data(7 downto 0)  <= x"00" when snk_fab_i.bytesel = '1' else snk_fab_i.data(7 downto 0);

    p_check_crc : process(clk_sys_i)
    begin
      if rising_edge(clk_sys_i) then
        if crc_gen_reset = '1' then
          crc_cur <= c_CRC32_INIT_VALUE;
        elsif(crc_gen_enable = '1') then
          crc_cur         <= f_update_crc32_d16(crc_cur, crc_in_data);
          crc_last_is_odd <= snk_fab_i.bytesel;
        end if;
      end if;
    end process;

    crc_match <= '1' when (crc_last_is_odd = '0' and crc_cur = c_CRC32_RESIDUE_FULL)
                 or (crc_last_is_odd = '1' and crc_cur = c_CRC32_RESIDUE_HALF) else '0';

  end generate gen_new_crc;

--   U_bypass_queue : ep_rx_bypass_queue
--     generic map (
--       g_size  => 3,
--       g_width => 18)
--     port map (
--       rst_n_i => rst_n_i,
--       clk_i   => clk_sys_i,
--       d_i     => q_in,
--       valid_i => q_dvalid_in,
--       dreq_o  => q_dreq_out,
--       q_o     => q_out,
--       valid_o => q_dvalid_out,
--       dreq_i  => src_dreq_i,
--       flush_i => '0',
--       purge_i => q_purge,
--       empty_o => q_empty);

  snk_dreq_o <= q_dreq_out and not (snk_fab_i.eof or snk_fab_i.error);


  p_count_bytes : process (clk_sys_i, rst_n_i)
  begin  -- process
    if rising_edge(clk_sys_i) then
      if (rst_n_i = '0' or regs_i.ecr_rx_en_o = '0') then
        byte_cntr <= (others => '0');
        is_runt   <= '0';
        is_giant  <= '0';
      else
        if(snk_fab_i.sof = '1') then
          byte_cntr <= (others => '0');
          is_runt   <= '1';
        end if;

        if(snk_fab_i.dvalid = '1') then
          if(snk_fab_i.bytesel = '1') then
            byte_cntr <= byte_cntr + 1;
          else
            byte_cntr <= byte_cntr + 2;
          end if;
        end if;

        if(byte_cntr = to_unsigned(c_MIN_FRAME_SIZE - 2, byte_cntr'length) and snk_fab_i.dvalid = '1' and snk_fab_i.bytesel = '0') then
          is_runt <= '0';
        end if;

        if(byte_cntr > unsigned(regs_i.rfcr_mru_o) and snk_fab_i.sof = '0') then
          is_giant <= '1';
        else
          is_giant <= '0';
        end if;
        
      end if;
    end if;
  end process;

  size_check_ok <= '0' when (is_runt = '1' and regs_i.rfcr_a_runt_o = '0') or
                   (is_giant = '1' and regs_i.rfcr_a_giant_o = '0') else '1';
  err_on_giant  <= '1' when (is_giant = '1' and regs_i.rfcr_a_giant_o = '0') else '0';

  p_gen_output : process(clk_sys_i, rst_n_i)
  begin
    if rising_edge(clk_sys_i) then

      if rst_n_i = '0' or regs_i.ecr_rx_en_o = '0' then

        q_purge   <= '0';
        q_bytesel <= '0';

        state <= ST_WAIT_FRAME;

        rmon_pcs_err_o <= '0';
        rmon_giant_o   <= '0';
        rmon_runt_o    <= '0';
        rmon_crc_err_o <= '0';

        src_fab_o.sof <= '0';

      else
        case state is
          when ST_WAIT_FRAME =>
            q_purge         <= '0';
            rmon_pcs_err_o  <= '0';
            rmon_giant_o    <= '0';
            rmon_runt_o     <= '0';
            rmon_crc_err_o  <= '0';
            q_bytesel       <= '0';
            src_fab_o.error <= '0';
            src_fab_o.sof   <= '0';

            if(snk_fab_i.sof = '1') then
              state         <= ST_DATA;
              src_fab_o.sof <= '1';
            end if;

          when ST_DATA =>

            src_fab_o.sof <= '0';

            if(snk_fab_i.dvalid = '1' and snk_fab_i.addr = c_WRF_DATA) then
              q_bytesel <= snk_fab_i.bytesel;
            end if;

            if(snk_fab_i.error = '1' or    -- an error from the source?
               (q_bytesel = '1' and oob_in = '0' and snk_fab_i.dvalid = '1')) then -- we expect bytesel at the last byte of data

              src_fab_o.error <= '1';
              rmon_pcs_err_o  <= '1';
              state           <= ST_WAIT_FRAME;
              q_purge         <= '1';

--             elsif(snk_fab_i.eof = '1' or oob_in = '1') then 
            elsif(snk_fab_i.eof = '1' or oob_in = '1' or err_on_giant = '1') then
              if(size_check_ok = '0' or crc_match = '0') then  -- bad frame?
                state           <= ST_WAIT_FRAME;
                src_fab_o.error <= '1';
                q_purge         <= '1';
              elsif(snk_fab_i.eof = '1') then
                state         <= ST_WAIT_FRAME;
              else
                state       <= ST_OOB;
              end if;

              rmon_runt_o    <= is_runt and (not regs_i.rfcr_a_runt_o);
              rmon_giant_o   <= is_giant and (not regs_i.rfcr_a_giant_o);
              rmon_crc_err_o <= not crc_match;
            end if;

            
          when ST_OOB =>
            rmon_runt_o    <= '0';
            rmon_giant_o   <= '0';
            rmon_crc_err_o <= '0';
           
            if(src_dreq_i = '1' and snk_fab_i.eof='1') then
              state         <= ST_WAIT_FRAME;
            end if;
            
        end case;
      end if;
    end if;
  end process;

  --
  q_in(15 downto 0)  <= snk_fab_i.data;
  q_in(17 downto 16) <= snk_fab_i.addr;
  q_dvalid_in        <= '1' when snk_fab_i.dvalid = '1' and (state = ST_DATA or state = ST_OOB) else '0';

  --ML optimized queue_bypass so can remove masks tuff
  src_fab_o.dvalid  <= q_dvalid_out;
  src_fab_o.data    <= q_in(15 downto 0)  when (oob_in = '1') else q_out(15 downto 0);
  src_fab_o.addr    <= q_in(17 downto 16) when (oob_in = '1') else q_out(17 downto 16) ;
  src_fab_o.bytesel <= snk_fab_i.bytesel  when (dat_in = '1') else '0';
  src_fab_o.eof     <= snk_fab_i.eof;

  src_fab_o.has_rx_timestamp   <= snk_fab_i.has_rx_timestamp;
  src_fab_o.rx_timestamp_valid <= snk_fab_i.rx_timestamp_valid;

  --------------------- the whole of bypass_queue is here ------------------------------------
  -- it was put inside as the optimization made it far from "universal" and apparently this
  -- was the cause of doing the bypass_queue a separate module
  --------------------------------------------------------------------------------------------
  qempty       <= f_queue_occupation(q_valid, '1') ;
  qfull        <= f_queue_occupation(q_valid, '0');

  q_dvalid_out <= (qfull and q_dvalid_in) or (oob_in and valid_mask);
  q_dreq_out   <= (src_dreq_i or not qfull);
  oob_in       <= '1' when (snk_fab_i.addr = c_WRF_OOB  and q_dvalid_in = '1') else '0';
  dat_in       <= '1' when (snk_fab_i.addr = c_WRF_DATA and q_dvalid_in = '1') else '0';
  

  sreg_enable  <= '1' when ((q_dvalid_in = '1') or (qempty = '0' and q_dvalid_out = '1')) else '0';
  
  q_out        <= q_data(0);

  p_fifo: process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if(sreg_enable = '1') then
        q_data(c_crc_size-1)  <= q_in;
        L0: for i in 0 to c_crc_size-2 loop
            q_data(i)       <= q_data(i+1);
        end loop L0;        
      end if;
    end if;
  end process; 

  p_queue : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' or q_purge = '1' then
        valid_mask <= '0';
        q_valid    <= (others => '0');
      else

        valid_mask <= src_dreq_i;

        if sreg_enable = '1' then
          q_valid(0)                           <= q_dvalid_in;
          if(oob_in = '1' ) then -- flashing CRC
            q_valid                            <=(others => '0');
          else
            q_valid(q_valid'length-1 downto 1) <= q_valid(q_valid'length-2 downto 0);
          end if;
        end if;
      end if;
    end if;
  end process;

end behavioral;





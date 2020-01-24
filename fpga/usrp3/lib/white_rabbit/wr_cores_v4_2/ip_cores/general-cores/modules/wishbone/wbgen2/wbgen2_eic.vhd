-------------------------------------------------------------------------------
-- Title      : WBGEN components
-- Project    : General Cores
-------------------------------------------------------------------------------
-- File       : wbgen2_eic.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN
-- Platform   : FPGA-generics
-- Standard   : VHDL '93
-------------------------------------------------------------------------------
-- Copyright (c) 2011 CERN
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


use work.wbgen2_pkg.all;

entity wbgen2_eic is
  
  generic (
    g_num_interrupts : natural := 1;

    g_irq00_mode : integer := 0;
    g_irq01_mode : integer := 0;
    g_irq02_mode : integer := 0;
    g_irq03_mode : integer := 0;
    g_irq04_mode : integer := 0;
    g_irq05_mode : integer := 0;
    g_irq06_mode : integer := 0;
    g_irq07_mode : integer := 0;
    g_irq08_mode : integer := 0;
    g_irq09_mode : integer := 0;
    g_irq0a_mode : integer := 0;
    g_irq0b_mode : integer := 0;
    g_irq0c_mode : integer := 0;
    g_irq0d_mode : integer := 0;
    g_irq0e_mode : integer := 0;
    g_irq0f_mode : integer := 0;
    g_irq10_mode : integer := 0;
    g_irq11_mode : integer := 0;
    g_irq12_mode : integer := 0;
    g_irq13_mode : integer := 0;
    g_irq14_mode : integer := 0;
    g_irq15_mode : integer := 0;
    g_irq16_mode : integer := 0;
    g_irq17_mode : integer := 0;
    g_irq18_mode : integer := 0;
    g_irq19_mode : integer := 0;
    g_irq1a_mode : integer := 0;
    g_irq1b_mode : integer := 0;
    g_irq1c_mode : integer := 0;
    g_irq1d_mode : integer := 0;
    g_irq1e_mode : integer := 0;
    g_irq1f_mode : integer := 0
    );
  port(
    rst_n_i : in std_logic;             -- reset & system clock, as always :)
    clk_i   : in std_logic;

    -- raw interrupt inputs
    irq_i : in std_logic_vector(g_num_interrupts-1 downto 0);  

    -- interrupt acknowledge signal, used for level-active interrupts to
    -- indicate that the interrupt has been handled
    irq_ack_o: out std_logic_vector(g_num_interrupts-1 downto 0);  

-- interrupt mask regsiter (slv/bus read-only)
    reg_imr_o : out std_logic_vector(g_num_interrupts-1 downto 0);

-- interrupt enable/disable registers (slv/bus pass-through)
    reg_ier_i        : in std_logic_vector(g_num_interrupts-1 downto 0);
    reg_ier_wr_stb_i : in std_logic;

    reg_idr_i        : in std_logic_vector(g_num_interrupts-1 downto 0);
    reg_idr_wr_stb_i : in std_logic;

-- interrupt status register (slv/bus write with LOAD_EXT)
    reg_isr_o        : out std_logic_vector(g_num_interrupts-1 downto 0);
    reg_isr_i        : in  std_logic_vector(g_num_interrupts-1 downto 0);
    reg_isr_wr_stb_i : in  std_logic;

-- multiplexed wishbone irq output
    wb_irq_o : out std_logic

    );

end wbgen2_eic;

architecture syn of wbgen2_eic is

  subtype t_irq_mode is integer;
  type t_irq_mode_vec is array (0 to 31) of t_irq_mode;

  constant c_IRQ_MODE_RISING_EDGE  : t_irq_mode := 0;
  constant c_IRQ_MODE_FALLING_EDGE : t_irq_mode := 1;
  constant c_IRQ_MODE_LEVEL_0      : t_irq_mode := 2;
  constant c_IRQ_MODE_LEVEL_1      : t_irq_mode := 3;


  signal irq_mode : t_irq_mode_vec;

  signal irq_mask    : std_logic_vector(g_num_interrupts-1 downto 0);
  signal irq_pending : std_logic_vector(g_num_interrupts-1 downto 0);

  signal irq_i_d0 : std_logic_vector(g_num_interrupts-1 downto 0);
  signal irq_i_d1 : std_logic_vector(g_num_interrupts-1 downto 0);
  signal irq_i_d2 : std_logic_vector(g_num_interrupts-1 downto 0);
  
begin  -- syn

  irq_mode(0)  <= g_irq00_mode;
  irq_mode(1)  <= g_irq01_mode;
  irq_mode(2)  <= g_irq02_mode;
  irq_mode(3)  <= g_irq03_mode;
  irq_mode(4)  <= g_irq04_mode;
  irq_mode(5)  <= g_irq05_mode;
  irq_mode(6)  <= g_irq06_mode;
  irq_mode(7)  <= g_irq07_mode;
  irq_mode(8)  <= g_irq08_mode;
  irq_mode(9)  <= g_irq09_mode;
  irq_mode(10) <= g_irq0a_mode;
  irq_mode(11) <= g_irq0b_mode;
  irq_mode(12) <= g_irq0c_mode;
  irq_mode(13) <= g_irq0d_mode;
  irq_mode(14) <= g_irq0e_mode;
  irq_mode(15) <= g_irq0f_mode;
  irq_mode(16) <= g_irq10_mode;
  irq_mode(17) <= g_irq11_mode;
  irq_mode(18) <= g_irq12_mode;
  irq_mode(19) <= g_irq13_mode;
  irq_mode(20) <= g_irq14_mode;
  irq_mode(21) <= g_irq15_mode;
  irq_mode(22) <= g_irq16_mode;
  irq_mode(23) <= g_irq17_mode;
  irq_mode(24) <= g_irq18_mode;
  irq_mode(25) <= g_irq19_mode;
  irq_mode(26) <= g_irq1a_mode;
  irq_mode(27) <= g_irq1b_mode;
  irq_mode(28) <= g_irq1c_mode;
  irq_mode(29) <= g_irq1d_mode;
  irq_mode(30) <= g_irq1e_mode;
  irq_mode(31) <= g_irq1f_mode;


  process(clk_i, rst_n_i)
  begin
    if(rst_n_i = '0') then
      irq_i_d0    <= (others => '0');
      irq_i_d1    <= (others => '0');
      irq_i_d1    <= (others => '0');
      irq_pending <= (others => '0');
      irq_mask    <= (others => '0');
      
    elsif rising_edge(clk_i) then

      for i in 0 to g_num_interrupts-1 loop
        
        irq_i_d0(i) <= irq_i(i);
        irq_i_d1(i) <= irq_i_d0(i);
        irq_i_d2(i) <= irq_i_d1(i);


        if((reg_isr_i(i) = '1' and reg_isr_wr_stb_i = '1') or irq_mask(i) = '0') then
          irq_pending(i) <= '0';
          irq_i_d0(i) <= '0';
          irq_i_d1(i) <= '0';
          irq_i_d2(i) <= '0';
        
        else
          
          case irq_mode(i) is
            when c_IRQ_MODE_LEVEL_0      => irq_pending(i) <= not irq_i_d2(i);
            when c_IRQ_MODE_LEVEL_1      => irq_pending(i) <= irq_i_d2(i);
            when c_IRQ_MODE_RISING_EDGE  => irq_pending(i) <= irq_pending(i) or ((not irq_i_d2(i)) and irq_i_d1(i));
            when c_IRQ_MODE_FALLING_EDGE => irq_pending(i) <= irq_pending(i) or ((not irq_i_d1(i)) and irq_i_d2(i));
            when others                  => null;
          end case;
        end if;
      end loop;  -- i

      if(reg_ier_wr_stb_i = '1') then
        for i in 0 to g_num_interrupts-1 loop
          if(reg_ier_i(i) = '1') then
            irq_mask(i) <= '1';
          end if;
        end loop;
      end if;

      if(reg_idr_wr_stb_i = '1') then
        for i in 0 to g_num_interrupts-1 loop
          if(reg_idr_i(i) = '1') then
            irq_mask(i) <= '0';
          end if;
        end loop;
      end if;
    end if;
  end process;

  -- generation of wb_irq_o

  process(clk_i, rst_n_i)
  begin
    if(rst_n_i = '0') then
      wb_irq_o <= '0';
    elsif rising_edge(clk_i) then
      if(irq_pending = std_logic_vector(to_unsigned(0, g_num_interrupts))) then
        wb_irq_o <= '0';
      else
        wb_irq_o <= '1';
      end if;
      
    end if;
  end process;

  gen_irq_ack: for i in 0 to g_num_interrupts-1 generate
    irq_ack_o(i) <= '1' when (reg_isr_wr_stb_i = '1' and reg_isr_i(i) = '1') else '0';
  end generate gen_irq_ack;

  reg_imr_o <= irq_mask;
  reg_isr_o <= irq_pending;

end syn;

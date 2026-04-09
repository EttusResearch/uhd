--
-- Copyright 2025 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: cpld_interface_wrapper
--
-- Description:
--   This module serves as a wrapper for the CPLD interface, providing
--   the necessary ports and connections for communication between the FPGA
--   and the CPLD. It includes ports for reset, clock, and data signals.

library ieee;
   use ieee.std_logic_1164.all;
   use ieee.numeric_std.all;

entity cpld_interface_wrapper is
  port (
     a_reset : in std_logic 
    ; bus_clk : in std_logic 
    ; b_reg_port_in : in std_logic_vector(50 downto 0) 
    ; b_reg_port_out : out std_logic_vector(33 downto 0) 
    ; a_cpld_ext_reset_n : out std_logic 
    ; fpga_to_cpld_clk : out std_logic 
    ; f_fpga_to_cpld_clk_en : out std_logic 
    ; f_fpga_to_cpld_data : out std_logic_vector(7 downto 0) 
    ; cpld_to_fpga_clk : in std_logic 
    ; c_cpld_to_fpga_data : in std_logic_vector(7 downto 0) 
  );
end entity cpld_interface_wrapper;

architecture rtl of cpld_interface_wrapper is
begin
end architecture rtl;

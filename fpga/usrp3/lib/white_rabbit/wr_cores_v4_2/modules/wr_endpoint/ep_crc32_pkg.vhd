-------------------------------------------------------------------------------
-- Title      : CRC32 package
-- Project    : White Rabbit 
-------------------------------------------------------------------------------
-- File       : ep_crc32_pkg.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Platform   : FPGA-generic
-- Standard   : VHDL '93
-------------------------------------------------------------------------------
--
-- Copyright (c) 2012-2013 CERN
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

package ep_crc32_pkg is


  constant c_CRC32_RESIDUE_FULL : std_logic_vector(31 downto 0) := x"1cdf4421";
  constant c_CRC32_RESIDUE_HALF : std_logic_vector(31 downto 0) := x"1df722c6";
  constant c_CRC32_INIT_VALUE   : std_logic_vector(31 downto 0) := x"00000000";

  function f_update_crc32_d16(d : std_logic_vector; data_in : std_logic_vector) return std_logic_vector;
  function f_update_crc32_d8(d  : std_logic_vector(31 downto 0); data_in : std_logic_vector(7 downto 0)) return std_logic_vector;
  
end ep_crc32_pkg;

package body ep_crc32_pkg is


  function f_update_crc32_d16(d : std_logic_vector; data_in : std_logic_vector) return std_logic_vector is
    variable q : std_logic_vector(d'length-1 downto 0);
  begin
    q(7)  := not ((not d(23)) xor (not d(17)) xor (not d(30)) xor (not d(29)) xor (not d(27)) xor data_in(7) xor data_in(1) xor data_in(14) xor data_in(13) xor data_in(11));
    q(6)  := not ((not d(23)) xor (not d(22)) xor (not d(17)) xor (not d(16)) xor (not d(30)) xor (not d(28)) xor (not d(27)) xor (not d(26)) xor data_in(7) xor data_in(6) xor data_in(1) xor data_in(0) xor data_in(14) xor data_in(12) xor data_in(11) xor data_in(10));
    q(5)  := not ((not d(23)) xor (not d(22)) xor (not d(21)) xor (not d(17)) xor (not d(16)) xor (not d(31)) xor (not d(30)) xor (not d(26)) xor (not d(25)) xor data_in(7) xor data_in(6) xor data_in(5) xor data_in(1) xor data_in(0) xor data_in(15) xor data_in(14) xor data_in(10) xor data_in(9));
    q(4)  := not ((not d(22)) xor (not d(21)) xor (not d(20)) xor (not d(16)) xor (not d(31)) xor (not d(30)) xor (not d(29)) xor (not d(25)) xor (not d(24)) xor data_in(6) xor data_in(5) xor data_in(4) xor data_in(0) xor data_in(15) xor data_in(14) xor data_in(13) xor data_in(9) xor data_in(8));
    q(3)  := not ((not d(23)) xor (not d(21)) xor (not d(20)) xor (not d(19)) xor (not d(17)) xor (not d(31)) xor (not d(28)) xor (not d(27)) xor (not d(24)) xor data_in(7) xor data_in(5) xor data_in(4) xor data_in(3) xor data_in(1) xor data_in(15) xor data_in(12) xor data_in(11) xor data_in(8));
    q(2)  := not ((not d(23)) xor (not d(22)) xor (not d(20)) xor (not d(19)) xor (not d(18)) xor (not d(17)) xor (not d(16)) xor (not d(29)) xor (not d(26)) xor data_in(7) xor data_in(6) xor data_in(4) xor data_in(3) xor data_in(2) xor data_in(1) xor data_in(0) xor data_in(13) xor data_in(10));
    q(1)  := not ((not d(22)) xor (not d(21)) xor (not d(19)) xor (not d(18)) xor (not d(17)) xor (not d(16)) xor (not d(31)) xor (not d(28)) xor (not d(25)) xor data_in(6) xor data_in(5) xor data_in(3) xor data_in(2) xor data_in(1) xor data_in(0) xor data_in(15) xor data_in(12) xor data_in(9));
    q(0)  := not ((not d(23)) xor (not d(21)) xor (not d(20)) xor (not d(18)) xor (not d(16)) xor (not d(31)) xor (not d(29)) xor (not d(24)) xor data_in(7) xor data_in(5) xor data_in(4) xor data_in(2) xor data_in(0) xor data_in(15) xor data_in(13) xor data_in(8));
    q(15) := not ((not d(23)) xor (not d(22)) xor (not d(20)) xor (not d(19)) xor (not d(31)) xor (not d(29)) xor (not d(28)) xor (not d(27)) xor data_in(7) xor data_in(6) xor data_in(4) xor data_in(3) xor data_in(15) xor data_in(13) xor data_in(12) xor data_in(11));
    q(14) := not ((not d(22)) xor (not d(21)) xor (not d(19)) xor (not d(18)) xor (not d(30)) xor (not d(28)) xor (not d(27)) xor (not d(26)) xor data_in(6) xor data_in(5) xor data_in(3) xor data_in(2) xor data_in(14) xor data_in(12) xor data_in(11) xor data_in(10));
    q(13) := not ((not d(23)) xor (not d(21)) xor (not d(20)) xor (not d(18)) xor (not d(30)) xor (not d(26)) xor (not d(25)) xor data_in(7) xor data_in(5) xor data_in(4) xor data_in(2) xor data_in(14) xor data_in(10) xor data_in(9));
    q(12) := not ((not d(23)) xor (not d(22)) xor (not d(20)) xor (not d(19)) xor (not d(30)) xor (not d(27)) xor (not d(25)) xor (not d(24)) xor data_in(7) xor data_in(6) xor data_in(4) xor data_in(3) xor data_in(14) xor data_in(11) xor data_in(9) xor data_in(8));
    q(11) := not ((not d(23)) xor (not d(22)) xor (not d(21)) xor (not d(19)) xor (not d(18)) xor (not d(17)) xor (not d(30)) xor (not d(27)) xor (not d(26)) xor (not d(24)) xor data_in(7) xor data_in(6) xor data_in(5) xor data_in(3) xor data_in(2) xor data_in(1) xor data_in(14) xor data_in(11) xor data_in(10) xor data_in(8));
    q(10) := not ((not d(22)) xor (not d(21)) xor (not d(20)) xor (not d(18)) xor (not d(17)) xor (not d(16)) xor (not d(29)) xor (not d(26)) xor (not d(25)) xor data_in(6) xor data_in(5) xor data_in(4) xor data_in(2) xor data_in(1) xor data_in(0) xor data_in(13) xor data_in(10) xor data_in(9));
    q(9)  := not ((not d(21)) xor (not d(20)) xor (not d(19)) xor (not d(17)) xor (not d(16)) xor (not d(31)) xor (not d(28)) xor (not d(25)) xor (not d(24)) xor data_in(5) xor data_in(4) xor data_in(3) xor data_in(1) xor data_in(0) xor data_in(15) xor data_in(12) xor data_in(9) xor data_in(8));
    q(8)  := not ((not d(20)) xor (not d(19)) xor (not d(18)) xor (not d(16)) xor (not d(31)) xor (not d(30)) xor (not d(27)) xor (not d(24)) xor data_in(4) xor data_in(3) xor data_in(2) xor data_in(0) xor data_in(15) xor data_in(14) xor data_in(11) xor data_in(8));
    q(23) := not ((not d(7)) xor (not d(23)) xor (not d(19)) xor (not d(18)) xor (not d(31)) xor (not d(27)) xor (not d(26)) xor data_in(7) xor data_in(3) xor data_in(2) xor data_in(15) xor data_in(11) xor data_in(10));
    q(22) := not ((not d(6)) xor (not d(22)) xor (not d(18)) xor (not d(17)) xor (not d(30)) xor (not d(26)) xor (not d(25)) xor data_in(6) xor data_in(2) xor data_in(1) xor data_in(14) xor data_in(10) xor data_in(9));
    q(21) := not ((not d(5)) xor (not d(21)) xor (not d(17)) xor (not d(16)) xor (not d(29)) xor (not d(25)) xor (not d(24)) xor data_in(5) xor data_in(1) xor data_in(0) xor data_in(13) xor data_in(9) xor data_in(8));
    q(20) := not ((not d(4)) xor (not d(20)) xor (not d(16)) xor (not d(31)) xor (not d(28)) xor (not d(24)) xor data_in(4) xor data_in(0) xor data_in(15) xor data_in(12) xor data_in(8));
    q(19) := not ((not d(3)) xor (not d(19)) xor (not d(31)) xor (not d(30)) xor (not d(27)) xor data_in(3) xor data_in(15) xor data_in(14) xor data_in(11));
    q(18) := not ((not d(2)) xor (not d(18)) xor (not d(30)) xor (not d(29)) xor (not d(26)) xor data_in(2) xor data_in(14) xor data_in(13) xor data_in(10));
    q(17) := not ((not d(1)) xor (not d(23)) xor (not d(30)) xor (not d(28)) xor (not d(27)) xor (not d(25)) xor data_in(7) xor data_in(14) xor data_in(12) xor data_in(11) xor data_in(9));
    q(16) := not ((not d(0)) xor (not d(23)) xor (not d(22)) xor (not d(17)) xor (not d(30)) xor (not d(26)) xor (not d(24)) xor data_in(7) xor data_in(6) xor data_in(1) xor data_in(14) xor data_in(10) xor data_in(8));
    q(31) := not ((not d(15)) xor (not d(22)) xor (not d(21)) xor (not d(16)) xor (not d(29)) xor (not d(25)) xor data_in(6) xor data_in(5) xor data_in(0) xor data_in(13) xor data_in(9));
    q(30) := not ((not d(14)) xor (not d(21)) xor (not d(20)) xor (not d(31)) xor (not d(28)) xor (not d(24)) xor data_in(5) xor data_in(4) xor data_in(15) xor data_in(12) xor data_in(8));
    q(29) := not ((not d(13)) xor (not d(23)) xor (not d(20)) xor (not d(19)) xor (not d(17)) xor (not d(29)) xor data_in(7) xor data_in(4) xor data_in(3) xor data_in(1) xor data_in(13));
    q(28) := not ((not d(12)) xor (not d(22)) xor (not d(19)) xor (not d(18)) xor (not d(16)) xor (not d(28)) xor data_in(6) xor data_in(3) xor data_in(2) xor data_in(0) xor data_in(12));
    q(27) := not ((not d(11)) xor (not d(21)) xor (not d(18)) xor (not d(17)) xor (not d(31)) xor (not d(27)) xor data_in(5) xor data_in(2) xor data_in(1) xor data_in(15) xor data_in(11));
    q(26) := not ((not d(10)) xor (not d(20)) xor (not d(17)) xor (not d(16)) xor (not d(30)) xor (not d(26)) xor data_in(4) xor data_in(1) xor data_in(0) xor data_in(14) xor data_in(10));
    q(25) := not ((not d(9)) xor (not d(19)) xor (not d(16)) xor (not d(31)) xor (not d(29)) xor (not d(25)) xor data_in(3) xor data_in(0) xor data_in(15) xor data_in(13) xor data_in(9));
    q(24) := not ((not d(8)) xor (not d(18)) xor (not d(31)) xor (not d(30)) xor (not d(28)) xor (not d(24)) xor data_in(2) xor data_in(15) xor data_in(14) xor data_in(12) xor data_in(8));

    return q;
  end f_update_crc32_d16;


  function f_update_crc32_d8(d : std_logic_vector(31 downto 0); data_in : std_logic_vector(7 downto 0)) return std_logic_vector is
    variable q : std_logic_vector(31 downto 0);
  begin
    q(7)  := not ((not d(31)) xor (not d(25)) xor data_in(7) xor data_in(1));
    q(6)  := not ((not d(31)) xor (not d(30)) xor (not d(25)) xor (not d(24)) xor data_in(7) xor data_in(6) xor data_in(1) xor data_in(0));
    q(5)  := not ((not d(31)) xor (not d(30)) xor (not d(29)) xor (not d(25)) xor (not d(24)) xor data_in(7) xor data_in(6) xor data_in(5) xor data_in(1) xor data_in(0));
    q(4)  := not ((not d(30)) xor (not d(29)) xor (not d(28)) xor (not d(24)) xor data_in(6) xor data_in(5) xor data_in(4) xor data_in(0));
    q(3)  := not ((not d(31)) xor (not d(29)) xor (not d(28)) xor (not d(27)) xor (not d(25)) xor data_in(7) xor data_in(5) xor data_in(4) xor data_in(3) xor data_in(1));
    q(2)  := not ((not d(31)) xor (not d(30)) xor (not d(28)) xor (not d(27)) xor (not d(26)) xor (not d(25)) xor (not d(24)) xor data_in(7) xor data_in(6) xor data_in(4) xor data_in(3) xor data_in(2) xor data_in(1) xor data_in(0));
    q(1)  := not ((not d(30)) xor (not d(29)) xor (not d(27)) xor (not d(26)) xor (not d(25)) xor (not d(24)) xor data_in(6) xor data_in(5) xor data_in(3) xor data_in(2) xor data_in(1) xor data_in(0));
    q(0)  := not ((not d(31)) xor (not d(29)) xor (not d(28)) xor (not d(26)) xor (not d(24)) xor data_in(7) xor data_in(5) xor data_in(4) xor data_in(2) xor data_in(0));
    q(15) := not ((not d(7)) xor (not d(31)) xor (not d(30)) xor (not d(28)) xor (not d(27)) xor data_in(7) xor data_in(6) xor data_in(4) xor data_in(3));
    q(14) := not ((not d(6)) xor (not d(30)) xor (not d(29)) xor (not d(27)) xor (not d(26)) xor data_in(6) xor data_in(5) xor data_in(3) xor data_in(2));
    q(13) := not ((not d(5)) xor (not d(31)) xor (not d(29)) xor (not d(28)) xor (not d(26)) xor data_in(7) xor data_in(5) xor data_in(4) xor data_in(2));
    q(12) := not ((not d(4)) xor (not d(31)) xor (not d(30)) xor (not d(28)) xor (not d(27)) xor data_in(7) xor data_in(6) xor data_in(4) xor data_in(3));
    q(11) := not ((not d(3)) xor (not d(31)) xor (not d(30)) xor (not d(29)) xor (not d(27)) xor (not d(26)) xor (not d(25)) xor data_in(7) xor data_in(6) xor data_in(5) xor data_in(3) xor data_in(2) xor data_in(1));
    q(10) := not ((not d(2)) xor (not d(30)) xor (not d(29)) xor (not d(28)) xor (not d(26)) xor (not d(25)) xor (not d(24)) xor data_in(6) xor data_in(5) xor data_in(4) xor data_in(2) xor data_in(1) xor data_in(0));
    q(9)  := not ((not d(1)) xor (not d(29)) xor (not d(28)) xor (not d(27)) xor (not d(25)) xor (not d(24)) xor data_in(5) xor data_in(4) xor data_in(3) xor data_in(1) xor data_in(0));
    q(8)  := not ((not d(0)) xor (not d(28)) xor (not d(27)) xor (not d(26)) xor (not d(24)) xor data_in(4) xor data_in(3) xor data_in(2) xor data_in(0));
    q(23) := not ((not d(15)) xor (not d(31)) xor (not d(27)) xor (not d(26)) xor data_in(7) xor data_in(3) xor data_in(2));
    q(22) := not ((not d(14)) xor (not d(30)) xor (not d(26)) xor (not d(25)) xor data_in(6) xor data_in(2) xor data_in(1));
    q(21) := not ((not d(13)) xor (not d(29)) xor (not d(25)) xor (not d(24)) xor data_in(5) xor data_in(1) xor data_in(0));
    q(20) := not ((not d(12)) xor (not d(28)) xor (not d(24)) xor data_in(4) xor data_in(0));
    q(19) := not ((not d(11)) xor (not d(27)) xor data_in(3));
    q(18) := not ((not d(10)) xor (not d(26)) xor data_in(2));
    q(17) := not ((not d(9)) xor (not d(31)) xor data_in(7));
    q(16) := not ((not d(8)) xor (not d(31)) xor (not d(30)) xor (not d(25)) xor data_in(7) xor data_in(6) xor data_in(1));
    q(31) := not ((not d(23)) xor (not d(30)) xor (not d(29)) xor (not d(24)) xor data_in(6) xor data_in(5) xor data_in(0));
    q(30) := not ((not d(22)) xor (not d(29)) xor (not d(28)) xor data_in(5) xor data_in(4));
    q(29) := not ((not d(21)) xor (not d(31)) xor (not d(28)) xor (not d(27)) xor (not d(25)) xor data_in(7) xor data_in(4) xor data_in(3) xor data_in(1));
    q(28) := not ((not d(20)) xor (not d(30)) xor (not d(27)) xor (not d(26)) xor (not d(24)) xor data_in(6) xor data_in(3) xor data_in(2) xor data_in(0));
    q(27) := not ((not d(19)) xor (not d(29)) xor (not d(26)) xor (not d(25)) xor data_in(5) xor data_in(2) xor data_in(1));
    q(26) := not ((not d(18)) xor (not d(28)) xor (not d(25)) xor (not d(24)) xor data_in(4) xor data_in(1) xor data_in(0));
    q(25) := not ((not d(17)) xor (not d(27)) xor (not d(24)) xor data_in(3) xor data_in(0));
    q(24) := not ((not d(16)) xor (not d(26)) xor data_in(2));
    return q;
  end f_update_crc32_d8;
end ep_crc32_pkg;


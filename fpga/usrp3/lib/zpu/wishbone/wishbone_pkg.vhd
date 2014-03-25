-- ZPU
--
-- Copyright 2004-2008 oharboe - Øyvind Harboe - oyvind.harboe@zylin.com
-- 
-- The FreeBSD license
-- 
-- Redistribution and use in source and binary forms, with or without
-- modification, are permitted provided that the following conditions
-- are met:
-- 
-- 1. Redistributions of source code must retain the above copyright
--    notice, this list of conditions and the following disclaimer.
-- 2. Redistributions in binary form must reproduce the above
--    copyright notice, this list of conditions and the following
--    disclaimer in the documentation and/or other materials
--    provided with the distribution.
-- 
-- THIS SOFTWARE IS PROVIDED BY THE ZPU PROJECT ``AS IS'' AND ANY
-- EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
-- THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
-- PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
-- ZPU PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
-- INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
-- (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
-- OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
-- HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
-- STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
-- ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-- 
-- The views and conclusions contained in the software and documentation
-- are those of the authors and should not be interpreted as representing
-- official policies, either expressed or implied, of the ZPU Project.

library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

package wishbone_pkg is

	type wishbone_bus_in is record
		adr			: std_logic_vector(15 downto 0); 
		sel			: std_logic_vector(3 downto 0); 
		we			: std_logic;
		dat			: std_logic_vector(31 downto 0); 	-- Note! Data written with 'we'
		cyc			: std_logic; 
		stb			: std_logic;
	end record;

	type wishbone_bus_out is record
		dat		: std_logic_vector(31 downto 0);
		ack			: std_logic;
	end record;
	
	type wishbone_bus is record
		insig		: wishbone_bus_in;
		outsig  	: wishbone_bus_out;
	end record;

	component atomic32_access is
	port (	cpu_clk			: in std_logic;
			areset			: in std_logic;
	
			-- Wishbone from CPU interface
			wb_16_i			: in wishbone_bus_in;
			wb_16_o     	: out wishbone_bus_out;
			-- Wishbone to FPGA registers and ethernet core
			wb_32_i			: in wishbone_bus_out;
			wb_32_o			: out wishbone_bus_in);
	end component;
	
	component eth_access_corr is
	port (	cpu_clk			: in std_logic;
			areset			: in std_logic;
	
			-- Wishbone from Wishbone MUX
			eth_raw_o		: out wishbone_bus_out;
			eth_raw_i		: in wishbone_bus_in;
			
			-- Wishbone ethernet core
			eth_slave_i 	: in wishbone_bus_out;
			eth_slave_o		: out wishbone_bus_in);
	end component;


end wishbone_pkg;

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
use IEEE.STD_LOGIC_UNSIGNED.all; 

library work;
use work.zpu_top_pkg.all;
use work.wishbone_pkg.all;
use work.zpupkg.all;
use work.zpu_config.all;

entity zpu_system is
	generic(
			simulate		: boolean := false);
	port (	areset			: in std_logic;
			cpu_clk			: in std_logic;

			-- ZPU Control signals
			enable			: in std_logic;
			interrupt		: in std_logic;

			zpu_status		: out std_logic_vector(63 downto 0);

			-- wishbone interfaces
			zpu_wb_i		: in wishbone_bus_out;
			zpu_wb_o		: out wishbone_bus_in);
end zpu_system;

architecture behave of zpu_system is

signal	mem_req					: std_logic;
signal	mem_we 					: std_logic;
signal	mem_ack 				: std_logic; 
signal	mem_read 				: std_logic_vector(wordSize-1 downto 0);
signal	mem_write 				: std_logic_vector(wordSize-1 downto 0);
signal	out_mem_addr 			: std_logic_vector(maxAddrBitIncIO downto 0);
signal	mem_writeMask			: std_logic_vector(wordBytes-1 downto 0);


begin

	my_zpu_core:
	zpu_core port map (
    	clk 				=> cpu_clk, 
		areset 				=> areset,
	 	enable 				=> enable,
	  	mem_req 			=> mem_req,
	 	mem_we 				=> mem_we,
	 	mem_ack 			=> mem_ack, 
	 	mem_read 			=> mem_read,
	 	mem_write 			=> mem_write,
		out_mem_addr 		=> out_mem_addr,
	 	mem_writeMask		=> mem_writeMask,
	 	interrupt			=> interrupt,
	 	zpu_status			=> zpu_status,
	 	break				=> open);

	my_zpu_wb_bridge:
	zpu_wb_bridge port map (
		clk 				=> cpu_clk,
	 	areset 				=> areset,
	  	mem_req 			=> mem_req,
	 	mem_we 				=> mem_we,
	 	mem_ack 			=> mem_ack, 
	 	mem_read 			=> mem_read,
	 	mem_write 			=> mem_write,
		out_mem_addr 		=> out_mem_addr,
	 	mem_writeMask		=> mem_writeMask,
		zpu_wb_i			=> zpu_wb_i,
		zpu_wb_o			=> zpu_wb_o);

end behave;

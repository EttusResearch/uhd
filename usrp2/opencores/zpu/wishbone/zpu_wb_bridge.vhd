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
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

library work;
use work.zpu_top_pkg.all;
use work.wishbone_pkg.all;
use work.zpupkg.all;
use work.zpu_config.all;

entity zpu_wb_bridge is
	port (	-- Native ZPU interface
 			clk 				: in std_logic;
	 		areset 				: in std_logic;

	 		mem_req 			: in std_logic;
	 		mem_we				: in std_logic;
	 		mem_ack				: out std_logic; 
	 		mem_read 			: out std_logic_vector(wordSize-1 downto 0);
	 		mem_write 			: in std_logic_vector(wordSize-1 downto 0);
			out_mem_addr 		: in std_logic_vector(maxAddrBitIncIO downto 0);
	 		mem_writeMask		: in std_logic_vector(wordBytes-1 downto 0);
			
			-- Wishbone from ZPU
			zpu_wb_i			: in wishbone_bus_out;
			zpu_wb_o			: out wishbone_bus_in);

end zpu_wb_bridge;

architecture behave of zpu_wb_bridge is

begin

	mem_read <= zpu_wb_i.dat;
	mem_ack <= zpu_wb_i.ack;
	
	zpu_wb_o.adr <= out_mem_addr;
	zpu_wb_o.dat <= mem_write;
	zpu_wb_o.sel <= mem_writeMask;
	zpu_wb_o.stb <= mem_req;
	zpu_wb_o.cyc <= mem_req;
	zpu_wb_o.we <= mem_we;

end behave;

			

	


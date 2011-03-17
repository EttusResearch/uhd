library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

library work;
use work.zpupkg.all;
use work.zpu_config.all;
use work.wishbone_pkg.all;

package zpu_top_pkg is
    component zpu_wb_bridge is
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
    end component;

    component zpu_system is
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
    end component;

end zpu_top_pkg;

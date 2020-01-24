library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

library work;
use work.zpu_top_pkg.all;
use work.wishbone_pkg.all;
use work.zpupkg.all;
use work.zpu_config.all;

------------------------------------------------------------------------
-- Top level ZPU + wishbone componenent to use in a verilog design:
--   zpu_wb_top wraps around the zpu_system component.
--   All IO lines are exposed as std_logic for verilog.
------------------------------------------------------------------------
entity zpu_wb_top is
    generic (
        dat_w: integer := 32;
        adr_w: integer := 16;
        sel_w: integer := 4
    );
    port (
        clk: in std_logic;
        rst: in std_logic;
        enb: in std_logic;

        -- wishbone interface
        dat_i: in std_logic_vector(dat_w-1 downto 0);
        ack_i: in std_logic;
        adr_o: out std_logic_vector(adr_w-1 downto 0);
        sel_o: out std_logic_vector(sel_w-1 downto 0);
        we_o: out std_logic;
        dat_o: out std_logic_vector(dat_w-1 downto 0);
        cyc_o: out std_logic;
        stb_o: out std_logic;

        -- misc zpu signals
        interrupt: in std_logic;
        zpu_status: out std_logic_vector(63 downto 0)
    );

end zpu_wb_top;

architecture syn of zpu_wb_top is

--wishbone interface (records)
signal zpu_wb_i: wishbone_bus_out;
signal zpu_wb_o: wishbone_bus_in;

begin

--assign wishbone signals to records
zpu_wb_i.dat <= dat_i;
zpu_wb_i.ack <= ack_i;

adr_o <= zpu_wb_o.adr;
sel_o <= zpu_wb_o.sel;
we_o <= zpu_wb_o.we;
dat_o <= zpu_wb_o.dat;
cyc_o <= zpu_wb_o.cyc;
stb_o <= zpu_wb_o.stb;

--instantiate the zpu system
zpu_system0: zpu_system port map(
    cpu_clk => clk,
    areset => rst,
    enable => enb,
    interrupt => interrupt,
    zpu_status => zpu_status,
    zpu_wb_i => zpu_wb_i,
    zpu_wb_o => zpu_wb_o
);

end architecture syn;

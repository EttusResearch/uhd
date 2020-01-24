-------------------------------------------------------------------------------
-- Title      : AXI4Lite-to-WB bridge wrapper
-- Project    : General Cores
-------------------------------------------------------------------------------
-- File       : xwb_axi4lite_bridge.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN
-- Platform   : FPGA-generics
-- Standard   : VHDL '93
-------------------------------------------------------------------------------
-- Copyright (c) 2017 CERN
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

use work.axi4_pkg.all;
use work.wishbone_pkg.all;

entity xwb_axi4lite_bridge is
  port (
    clk_sys_i : in std_logic;
    rst_n_i   : in std_logic;

    axi4_slave_i : in  t_axi4_lite_slave_in_32;
    axi4_slave_o : out t_axi4_lite_slave_out_32;

    wb_master_o : out t_wishbone_master_out;
    wb_master_i : in  t_wishbone_master_in

    );

end xwb_axi4lite_bridge;

architecture rtl of xwb_axi4lite_bridge is

  constant c_timeout : integer := 256;

  type t_state is
    (IDLE, ISSUE_WRITE, ISSUE_READ, COMPLETE_WRITE, COMPLETE_READ, WAIT_ACK_READ, WAIT_ACK_WRITE, RESPONSE_READ, RESPONSE_WRITE);

  signal state : t_state;

  signal count : unsigned(10 downto 0);
  
begin

  process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        axi4_slave_o    <= c_axi4_lite_default_master_in_32;
        wb_master_o.cyc <= '0';
        state           <= IDLE;
      else
        case state is
          when IDLE =>
            wb_master_o.cyc      <= '0';
            axi4_slave_o.ARREADY <= '1';
            axi4_slave_o.AWREADY <= '1';
            axi4_slave_o.WREADY  <= '0';
            axi4_slave_o.BVALID  <= '0';
            axi4_slave_o.BRESP   <= (others => 'X');
            axi4_slave_o.RDATA  <= (others => 'X');
            axi4_slave_o.RRESP  <= (others => 'X');
            axi4_slave_o.RVALID <= '0';
            axi4_slave_o.RLAST <= '0';
            
            if(axi4_slave_i.AWVALID = '1') then
              state           <= ISSUE_WRITE;
              wb_master_o.adr <= axi4_slave_i.AWADDR;
            elsif (axi4_slave_i.ARVALID = '1') then
              state           <= ISSUE_READ;
              wb_master_o.adr <= axi4_slave_i.ARADDR;
            end if;

          when ISSUE_WRITE =>
            axi4_slave_o.WREADY <= '1';

            wb_master_o.cyc <= '1';
            wb_master_o.we  <= '1';

            if(axi4_slave_i.WVALID = '1') then
              wb_master_o.stb <= '1';
              wb_master_o.sel <= axi4_slave_i.WSTRB;
              wb_master_o.dat <= axi4_slave_i.WDATA;
              state           <= COMPLETE_WRITE;
            end if;

          when ISSUE_READ =>

            wb_master_o.cyc <= '1';
            wb_master_o.stb <= '1';
            wb_master_o.we  <= '0';
            axi4_slave_o.RVALID <= '0';
            axi4_slave_o.RLAST <= '0';
            state <= COMPLETE_READ;

          when COMPLETE_READ =>
            if(wb_master_i.stall = '0') then
              wb_master_o.stb <= '0';
              if(wb_master_i.ack = '1') then
                state <= IDLE;
                axi4_slave_o.RRESP <= c_AXI4_RESP_EXOKAY;
                axi4_slave_o.RDATA <= wb_master_i.dat;
                axi4_slave_o.RVALID <= '1';
                axi4_slave_o.RLAST <= '1';
                wb_master_o.cyc    <= '0';
              else
                state <= WAIT_ACK_READ;
                count <= (others => '0');
              end if;
            end if;

            
          when COMPLETE_WRITE =>
            if(wb_master_i.stall = '0') then
              wb_master_o.stb <= '0';
              if(wb_master_i.ack = '1') then
                state <= RESPONSE_WRITE;
                axi4_slave_o.BRESP <= c_AXI4_RESP_EXOKAY;
                wb_master_o.cyc    <= '0';
              else
                state <= WAIT_ACK_WRITE;
                count <= (others => '0');
              end if;
            end if;


          when WAIT_ACK_WRITE =>
            if(wb_master_i.ack = '1') then
              state              <= RESPONSE_WRITE;
              axi4_slave_o.BRESP <= c_AXI4_RESP_EXOKAY;
              wb_master_o.cyc    <= '0';
            elsif count = c_timeout then
              state              <= RESPONSE_WRITE;
              axi4_slave_o.BRESP <= c_AXI4_RESP_SLVERR;
              wb_master_o.cyc    <= '0';
            end if;
            count <= count + 1;

          when WAIT_ACK_READ =>
            if(wb_master_i.ack = '1') then
              state              <= IDLE;
              axi4_slave_o.RRESP <= c_AXI4_RESP_EXOKAY;
              axi4_slave_o.RVALID <= '1';
              axi4_slave_o.RLAST <= '1';
              axi4_slave_o.RDATA <= wb_master_i.dat;
              wb_master_o.cyc    <= '0';
            elsif count = c_timeout then
              state              <= IDLE;
              axi4_slave_o.RRESP <= c_AXI4_RESP_SLVERR;
              axi4_slave_o.RVALID <= '1';
              axi4_slave_o.RLAST <= '1';
              axi4_slave_o.RDATA <= (others => 'X');
              wb_master_o.cyc    <= '0';
            end if;
            count <= count + 1;

            
          when RESPONSE_WRITE =>
            if (axi4_slave_i.BREADY = '1') then
              axi4_slave_o.BVALID <= '1';
              state               <= IDLE;
            end if;

          when RESPONSE_READ => null;
            
            
        end case;



      end if;
    end if;
  end process;

end rtl;


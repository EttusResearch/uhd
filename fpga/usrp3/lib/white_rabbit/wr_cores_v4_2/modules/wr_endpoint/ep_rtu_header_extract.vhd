-------------------------------------------------------------------------------
-- Title      : RTU header extract
-- Project    : White Rabbit 
-------------------------------------------------------------------------------
-- File       : ep_rtu_header_extract.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Platform   : FPGA-generic
-- Standard   : VHDL '93
-------------------------------------------------------------------------------
--
-- Copyright (c) 2012 - 2017 CERN / BE-CO-HT
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
use work.endpoint_private_pkg.all;
use work.endpoint_pkg.all;


entity ep_rtu_header_extract is
  generic(
    g_with_rtu : boolean);
  port(
    clk_sys_i : in std_logic;
    rst_n_i   : in std_logic;

    snk_fab_i  : in  t_ep_internal_fabric;
    snk_dreq_o : out std_logic;

    src_fab_o  : out t_ep_internal_fabric;
    src_dreq_i : in  std_logic;

    mbuf_is_pause_i : in std_logic; 

    vlan_class_i   : in std_logic_vector(2 downto 0);
    vlan_vid_i      : in std_logic_vector(11 downto 0);
    vlan_tag_done_i : in std_logic;
    vlan_is_tagged_i: in std_logic;
     
    rmon_drp_at_rtu_full_o: out std_logic;
    
    rtu_rq_o       : out t_ep_internal_rtu_request;
    rtu_full_i     : in  std_logic;
    rtu_rq_abort_o : out std_logic;
    rtu_rq_valid_o : out std_logic;
    rxbuf_full_i   : in  std_logic
    );

end ep_rtu_header_extract;

architecture rtl of ep_rtu_header_extract is

  signal hdr_offset : std_logic_vector(11 downto 0);
  signal in_packet  : std_logic;
  signal in_header     : std_logic;
  signal rtu_rq_valid_basic   : std_logic;
  signal rtu_rq_valid_tagged   : std_logic;
  signal rtu_rq_valid_out   : std_logic;
  signal rtu_rq_abort       : std_logic;
  

  procedure f_extract_rtu(signal q         : out std_logic_vector;
                          signal fab       :     t_ep_internal_fabric;
                          signal at_offset :     std_logic) is
  begin
    if(at_offset = '1' and fab.dvalid = '1') then
      q <= fab.data;
    end if;
  end f_extract_rtu;

begin  -- rtl

  gen_with_rtu : if(g_with_rtu) generate
    
    p_hdr_offset_sreg : process(clk_sys_i)
    begin
      if rising_edge(clk_sys_i) then
        if (rst_n_i = '0' or snk_fab_i.sof = '1') then
          hdr_offset(hdr_offset'left downto 1) <= (others => '0');
          hdr_offset(0)                        <= '1';
        elsif(snk_fab_i.dvalid = '1') then
          hdr_offset <= hdr_offset(hdr_offset'left-1 downto 0) & '0';
        end if;
      end if;
    end process;

    p_gen_rtu_request : process(clk_sys_i)
    begin
      if rising_edge(clk_sys_i) then
        if rst_n_i = '0' then
          rtu_rq_o.smac     <= (others => '0');
          rtu_rq_o.dmac     <= (others => '0');
          in_packet         <= '0';
          rtu_rq_valid_basic<= '0';
          rmon_drp_at_rtu_full_o <='0';
          rtu_rq_abort     <= '0';
          in_header        <= '0';
        else
          rmon_drp_at_rtu_full_o <='0';

          if(snk_fab_i.sof = '1' and rtu_full_i = '0' and rxbuf_full_i = '0') then
            in_packet <= '1';
            in_header <= '1';
          elsif(snk_fab_i.sof = '1' and rtu_full_i = '1') then
            rmon_drp_at_rtu_full_o <='1';
          end if;

          if((snk_fab_i.eof   = '1' and snk_fab_i.sof = '0') or -- in case both (sof & eof) are HIGH
              snk_fab_i.error = '1') then
            in_packet <= '0';
          end if;
          
          if(snk_fab_i.error = '1' or rtu_rq_valid_out = '1') then
            in_header <= '0';
          end if;
          
          
          f_extract_rtu(rtu_rq_o.dmac(47 downto 32), snk_fab_i, hdr_offset(0));
          f_extract_rtu(rtu_rq_o.dmac(31 downto 16), snk_fab_i, hdr_offset(1));
          f_extract_rtu(rtu_rq_o.dmac(15 downto 0), snk_fab_i, hdr_offset(2));
          f_extract_rtu(rtu_rq_o.smac(47 downto 32), snk_fab_i, hdr_offset(3));
          f_extract_rtu(rtu_rq_o.smac(31 downto 16), snk_fab_i, hdr_offset(4));
          f_extract_rtu(rtu_rq_o.smac(15 downto 0), snk_fab_i, hdr_offset(5));
          
          if(snk_fab_i.dvalid = '1'  and hdr_offset(6) = '1' and in_packet = '1') then
            rtu_rq_valid_basic <='1';
          elsif(rtu_rq_valid_out = '1' or   -- reset after making request or
                snk_fab_i.error = '1') then -- when there is error in the header, so we don't 
                                            -- make request for invalid frame which will be dumped
                                            -- in the SWcore (which reads error status).
                                            -- the special case when error occurs when the request
                                            -- is made, we do end with the output of error (below, end of file)
            rtu_rq_valid_basic     <= '0';
          end if;
          
          if(in_packet       = '1' and in_header    = '0' and -- if we have packet and the header is already processed
             snk_fab_i.error = '1' and rtu_rq_abort = '0') then 
            rtu_rq_abort <= '1';
          else
            rtu_rq_abort <= '0';
          end if;
          
        end if;
      end if;
    end process;
 
    rtu_rq_abort_o <= rtu_rq_abort;
    src_fab_o.sof <= snk_fab_i.sof and not rtu_full_i; -- null dev
    
    rtu_rq_valid_tagged <= rtu_rq_valid_basic and vlan_tag_done_i;
    
    -- the request is not done for PAUSE frames as they never go outside of Endpoint 
    -- (they are dropped inside Endpoint)
    rtu_rq_valid_out <= (rtu_rq_valid_tagged and (not mbuf_is_pause_i))when (vlan_is_tagged_i = '1') else
                        (rtu_rq_valid_basic  and (not mbuf_is_pause_i));
                        

  end generate gen_with_rtu;

  gen_without_rtu : if (not g_with_rtu) generate
    src_fab_o.sof          <= snk_fab_i.sof;
    rtu_rq_valid_out       <= '0';
    rtu_rq_o.smac          <= (others => '0');
    rtu_rq_o.dmac          <= (others => '0');
    rtu_rq_abort_o         <= '0';
    rmon_drp_at_rtu_full_o <= '0';
  end generate gen_without_rtu;

  snk_dreq_o <= src_dreq_i;

  src_fab_o.eof                <= snk_fab_i.eof;
  src_fab_o.dvalid             <= snk_fab_i.dvalid;
  src_fab_o.error              <= snk_fab_i.error;
  src_fab_o.bytesel            <= snk_fab_i.bytesel;
  src_fab_o.data               <= snk_fab_i.data;
  src_fab_o.addr               <= snk_fab_i.addr;
  src_fab_o.has_rx_timestamp   <= snk_fab_i.has_rx_timestamp;
  src_fab_o.rx_timestamp_valid <= snk_fab_i.rx_timestamp_valid;

  rtu_rq_o.vid      <= vlan_vid_i;
  rtu_rq_o.has_vid  <= vlan_is_tagged_i;
  rtu_rq_o.prio     <= vlan_class_i;
  rtu_rq_o.has_prio <= vlan_is_tagged_i;
  rtu_rq_valid_o    <= rtu_rq_valid_out and not snk_fab_i.ERROR;
  rtu_rq_o.hash     <= (others => '0');
  
end rtl;

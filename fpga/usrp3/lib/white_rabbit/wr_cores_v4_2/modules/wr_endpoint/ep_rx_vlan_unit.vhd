-------------------------------------------------------------------------------
-- Title      : RX VLAN unit
-- Project    : White Rabbit 
-------------------------------------------------------------------------------
-- File       : ep_rx_vlan_unit.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Platform   : FPGA-generic
-- Standard   : VHDL '93
-------------------------------------------------------------------------------
--
-- Copyright (c) 2011-2015 CERN / BE-CO-HT
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
use ieee.numeric_std.all;

library work;
use work.endpoint_private_pkg.all;
use work.endpoint_pkg.all;
use work.ep_wbgen2_pkg.all;
use work.wr_fabric_pkg.all;


-- 3rd deframing pipeline stage - VLAN Unit

entity ep_rx_vlan_unit is
  port(clk_sys_i : in std_logic;
       rst_n_i   : in std_logic;

       snk_fab_i  : in  t_ep_internal_fabric;
       snk_dreq_o : out std_logic;

       src_fab_o  : out t_ep_internal_fabric;
       src_dreq_i : in  std_logic;

       tclass_o   : out std_logic_vector(2 downto 0);
       vid_o      : out std_logic_vector(11 downto 0);
       tag_done_o : out std_logic;
       is_tagged_o: out std_logic;

       regs_i : in    t_ep_out_registers;
       regs_o : out   t_ep_in_registers
       );

end ep_rx_vlan_unit;

architecture behavioral of ep_rx_vlan_unit is

  
  type t_tag_type is (NONE, PRIO, VLAN, NULL_VLAN);
  type t_state is (WAIT_FRAME, DATA, FLUSH_STALL, DISCARD_FRAME, INSERT_TAG, END_FRAME);

  signal dreq_mask  : std_logic;
  signal hdr_offset : std_logic_vector(11 downto 0);

  signal comb_tag_type : t_tag_type;

  signal tag_type : t_tag_type;
  signal state    : t_state;

  signal at_ethertype     : std_logic;
  signal at_vid           : std_logic;
  signal at_tpid          : std_logic;
  signal is_tagged        : std_logic;
  signal stored_ethertype : std_logic_vector(15 downto 0);
  signal stored_fab       : t_ep_internal_fabric;

  signal prio_int     : std_logic_vector(2 downto 0);
  signal force_dvalid : std_logic;

  signal r_tcar_pcp_map : std_logic_vector(23 downto 0);
  signal is_tag_inserted: std_logic;

  procedure f_vlan_decision
    (tag_type       :     t_tag_type;
     qmode          : in  std_logic_vector(1 downto 0);
     admit          : out std_logic;
     use_pvid       : out std_logic;
     use_fixed_prio : out std_logic) is
  begin

    use_pvid       := 'X';
    use_fixed_prio := 'X';
    admit          := '0';

    -- From Jose's table. Thanks a lot!
    case (qmode) is
      when c_QMODE_PORT_ACCESS =>
        case tag_type is
          when NONE =>
            admit := '1'; use_pvid := '1'; use_fixed_prio := '1';
          when PRIO =>
            admit := '1'; use_pvid := '1'; use_fixed_prio := '0';
          when VLAN =>
            admit := '0'; use_pvid := '0';
          when NULL_VLAN =>
            admit := '0';
        end case;

      when c_QMODE_PORT_TRUNK =>
        case tag_type is
          when NONE =>
            admit := '0';
          when PRIO =>
            admit := '0';
          when VLAN =>
            admit          := '1';
            use_pvid       := '0';
            use_fixed_prio := '0';
          when NULL_VLAN =>
            admit := '0';
        end case;

      when c_QMODE_PORT_UNQUALIFIED =>
        
        case tag_type is
          when NONE =>
            admit    := '1';
            use_pvid := '1'; use_fixed_prio := '1';
          when PRIO =>
            admit    := '1';
            use_pvid := '1'; use_fixed_prio := '0';
          when VLAN =>
            admit    := '1';
            use_pvid := '0'; use_fixed_prio := '0';
          when NULL_VLAN =>
            admit := '0';
        end case;

      when c_QMODE_PORT_VLAN_DISABLED =>
        case tag_type is
          when NONE =>
            admit          := '1';
            use_pvid       := '0';
            use_fixed_prio := '1';
          when PRIO =>
            admit          := '1';
            use_pvid       := '0';
            use_fixed_prio := '0';
          when VLAN =>
            admit          := '1';
            use_pvid       := '0';
            use_fixed_prio := '0';
          when NULL_VLAN =>
            admit          := '1';
            use_pvid       := '0';
            use_fixed_prio := '0';
        end case;
      when others => null;
    end case;
  end procedure;

  
begin  -- behavioral

  at_ethertype <= hdr_offset(6) and snk_fab_i.dvalid;
  at_tpid      <= hdr_offset(6) and snk_fab_i.dvalid and is_tagged;--unused 
  at_vid       <= hdr_offset(7) and snk_fab_i.dvalid and is_tagged;

  snk_dreq_o <= src_dreq_i and dreq_mask;-- and not at_ethertype;

  p_decode_tag_type : process(snk_fab_i, is_tagged)
  begin
    if(is_tagged = '0') then
      comb_tag_type <= NONE;
    else
      case snk_fab_i.data(11 downto 0) is
        when x"000" => comb_tag_type <= PRIO;
        when x"fff" => comb_tag_type <= NULL_VLAN;
        when others => comb_tag_type <= VLAN;
      end case;
    end if;
  end process;

  p_tag_untag : process(clk_sys_i)
    variable admit, use_pvid, use_fixed_prio : std_logic;
    variable v_dreq_mask                     : std_logic;
    variable v_stored_fab                    : t_ep_internal_fabric;
    variable v_src_fab                       : t_ep_internal_fabric;
    variable v_next_state                    : t_state;
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' or regs_i.ecr_rx_en_o = '0' then
        hdr_offset(hdr_offset'left downto 1) <= (others => '0');
        hdr_offset(0)    <= '1';
        state            <= WAIT_FRAME;
        dreq_mask        <= '0';
        vid_o            <= (others =>'0');
        is_tag_inserted  <= '0';
        is_tagged        <= '0';
        src_fab_o.eof    <= '0';
        src_fab_o.error  <= '0';
        src_fab_o.dvalid <= '0';
        
      else
        if(snk_fab_i.error = '1') then
          state <= DISCARD_FRAME;
        else

          case state is
            when WAIT_FRAME =>
              dreq_mask       <= '1';
              src_fab_o.eof   <= '0';
              src_fab_o.error <= '0';
              is_tagged       <= '0';
              is_tag_inserted <= '0';
              prio_int        <= regs_i.vcr0_prio_val_o;
              vid_o           <= (others =>'0');

              if(snk_fab_i.sof = '1') then
                hdr_offset(hdr_offset'left downto 1) <= (others => '0');
                hdr_offset(0)                        <= '1';
                state                                <= DATA;
              end if;

            when DATA =>

              v_dreq_mask     := '1';
              v_src_fab.eof   := '0';
              v_src_fab.error := '0';
              v_next_state    := DATA;

              if (snk_fab_i.dvalid = '1' and src_dreq_i = '0') then
                v_stored_fab.bytesel := snk_fab_i.bytesel;
                v_stored_fab.data    := snk_fab_i.data;
                v_stored_fab.addr    := snk_fab_i.addr;
                v_stored_fab.dvalid  := '1';
                v_next_state         := FLUSH_STALL;
              else
                v_src_fab.addr    := snk_fab_i.addr;
                v_src_fab.data    := snk_fab_i.data;
                v_src_fab.dvalid  := src_dreq_i and snk_fab_i.dvalid;
                v_src_fab.bytesel := snk_fab_i.bytesel;
              end if;

              if(at_ethertype = '1') then
                stored_ethertype <= snk_fab_i.data;

                if(snk_fab_i.data = x"8100") then  -- got a 802.1q tagged frame
                  is_tagged <= '1';
                else
                  if (regs_i.vcr0_qmode_o = c_QMODE_PORT_ACCESS) then
                    prio_int <= regs_i.vcr0_prio_val_o;
                    is_tag_inserted  <= '1';
                    v_src_fab.dvalid := '0';
                    v_next_state     := INSERT_TAG;
                    v_dreq_mask      := '0';
                  elsif (regs_i.vcr0_qmode_o = c_QMODE_PORT_TRUNK) then
                    v_src_fab.dvalid := '0';
                    v_next_state     := DISCARD_FRAME;
                  end if;
                  is_tagged <= '0';
                end if;
              end if;

              if(snk_fab_i.eof = '1') then
                if(src_dreq_i = '1') then
                  v_src_fab.eof := '1';
                  v_next_state  := WAIT_FRAME;
                else
                  v_next_state := END_FRAME;
                end if;
              end if;

              if(at_vid = '1') then

                -- decide what to do with the frame, basing on the port mode
                -- (ACCESS, TRUNK, UNQUALIFIED), and whether the frame is tagged
                -- or not.
                f_vlan_decision(comb_tag_type, regs_i.vcr0_qmode_o, admit, use_pvid, use_fixed_prio);

                -- assign the VID
                if(admit = '0') then    -- oops...
                  v_next_state := DISCARD_FRAME;
                end if;

                if(use_pvid = '1')then
                  v_stored_fab.data(11 downto 0) := regs_i.vcr0_pvid_o;
                  v_src_fab.data(11 downto 0)    := regs_i.vcr0_pvid_o;
                end if;
                vid_o   <= v_src_fab.data(11 downto 0);
                -- assign the priority 
                if(regs_i.vcr0_fix_prio_o = '1' or use_fixed_prio = '1') then
                  -- Forced priority (or a non-priority tagged frame)? Take the priority
                  -- value from VCR0 register
                  prio_int <= regs_i.vcr0_prio_val_o;
                else
                  -- Got a priority tag - use the value from the VLAN tag
                  prio_int <= snk_fab_i.data(15 downto 13);
                end if;
              end if;

              if(snk_fab_i.dvalid = '1') then
                hdr_offset <= hdr_offset(hdr_offset'left-1 downto 0) & '0';
              end if;

              if(v_next_state = INSERT_TAG) then
                src_fab_o.eof     <= '0';              
                src_fab_o.dvalid  <= '1';
                src_fab_o.error   <= '0';
                src_fab_o.addr    <= c_WRF_DATA;
                src_fab_o.data    <= x"8100";
                src_fab_o.bytesel <= '0';
              else
                src_fab_o.eof     <= v_src_fab.eof;              
                src_fab_o.dvalid  <= v_src_fab.dvalid;
                src_fab_o.error   <= v_src_fab.error;
                src_fab_o.addr    <= v_src_fab.addr;
                src_fab_o.data    <= v_src_fab.data;
                src_fab_o.bytesel <= v_src_fab.bytesel;
              end if;

              dreq_mask  <= v_dreq_mask;
              stored_fab <= v_stored_fab;
              state      <= v_next_state;
              
            when END_FRAME =>
              if(src_dreq_i = '1')then
                src_fab_o.eof    <= '1';
                src_fab_o.dvalid <= '0';
                state            <= WAIT_FRAME;
              end if;
              
            when DISCARD_FRAME =>
              if(src_dreq_i = '1') then
                src_fab_o.error  <= '1';
                src_fab_o.dvalid <= '0';
                state            <= WAIT_FRAME;
              end if;

            when FLUSH_STALL =>
              if(src_dreq_i = '1')then
                src_fab_o.addr    <= stored_fab.addr;
                src_fab_o.data    <= stored_fab.data;
                src_fab_o.dvalid  <= stored_fab.dvalid;
                src_fab_o.bytesel <= stored_fab.bytesel;
                state             <= DATA;
              else
                src_fab_o.DATA   <= (others => 'X');
                src_fab_o.dvalid <= '0';
              end if;

            when INSERT_TAG =>
              src_fab_o.dvalid <= '0';

              if(src_dreq_i = '1') then
-- we are at 7th word from the beginning of the frame, but the sink reception
-- is disabled, so we can insert the original ethertype as the TPID

                if(hdr_offset(7) = '1') then
                  src_fab_o.addr   <= c_WRF_DATA;
                  src_fab_o.data   <= regs_i.vcr0_prio_val_o & '0' & regs_i.vcr0_pvid_o;
                  src_fab_o.dvalid <= '1';
                  vid_o            <= regs_i.vcr0_pvid_o; -- use the inserted PVID
                  dreq_mask        <= '0';
                  stored_fab.bytesel <= snk_fab_i.bytesel;
                  stored_fab.data    <= snk_fab_i.data;
                  stored_fab.addr    <= snk_fab_i.addr;
                  stored_fab.dvalid  <= snk_fab_i.dvalid;

                end if;

                if(hdr_offset(8) = '1') then
                  src_fab_o.addr   <= c_WRF_DATA;
                  src_fab_o.data   <= stored_ethertype;
                  src_fab_o.dvalid <= '1';
                  dreq_mask        <= '1';
                end if;

                if(hdr_offset(9) = '1') then
                  src_fab_o.addr    <= stored_fab.addr;
                  src_fab_o.data    <= stored_fab.data;
                  src_fab_o.dvalid  <= stored_fab.dvalid;
                  src_fab_o.bytesel <= stored_fab.bytesel;
                  dreq_mask        <= '1';
                  state            <= DATA;                  
                end if;

                hdr_offset <= hdr_offset(hdr_offset'left-1 downto 0) & '0';
              else
                src_fab_o.dvalid <= '0';
              end if;
          end case;
        end if;
      end if;
    end if;
  end process;


-- Process: p_map_prio_to_tc
-- Maps the PCP value from the 802.1q header into a traffic class for further
-- processing. The mapping table is stored in TCAR register.

  regs_o.tcar_pcp_map_i <= r_tcar_pcp_map;

  p_map_prio_to_tc : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then

      if(regs_i.tcar_pcp_map_load_o = '1') then
        r_tcar_pcp_map <= regs_i.tcar_pcp_map_o;
      end if;

      if(rst_n_i = '0' or regs_i.ecr_rx_en_o = '0' or snk_fab_i.sof = '1')then
        tag_done_o <= '0';
        tclass_o   <= "000";
      elsif(hdr_offset(9) = '1') then
        -- we're already after the headers, so prio_int is
        -- certainly valid
        tag_done_o <= '1';
        case prio_int is
          when "000"  => tclass_o <= r_tcar_pcp_map(2 downto 0);
          when "001"  => tclass_o <= r_tcar_pcp_map(5 downto 3);
          when "010"  => tclass_o <= r_tcar_pcp_map(8 downto 6);
          when "011"  => tclass_o <= r_tcar_pcp_map(11 downto 9);
          when "100"  => tclass_o <= r_tcar_pcp_map(14 downto 12);
          when "101"  => tclass_o <= r_tcar_pcp_map(17 downto 15);
          when "110"  => tclass_o <= r_tcar_pcp_map(20 downto 18);
          when "111"  => tclass_o <= r_tcar_pcp_map(23 downto 21);
          when others => tclass_o <= "000";--"XXX";  -- packet probably contains porn
        end case;
      end if;
    end if;
  end process;

  src_fab_o.sof <= regs_i.ecr_rx_en_o and snk_fab_i.sof;
  is_tagged_o   <= is_tagged or is_tag_inserted;
  
end behavioral;


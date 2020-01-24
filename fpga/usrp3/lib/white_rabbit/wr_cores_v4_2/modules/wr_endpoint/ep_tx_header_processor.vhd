-------------------------------------------------------------------------------
-- Title      : 1000base-X MAC/Endpoint - TX packet header processing unit
-- Project    : White Rabbit
-------------------------------------------------------------------------------
-- File       : ep_tx_header_processor.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2009-06-22
-- Last update: 2017-02-02
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: Processes headers and OOBs of the packets to be transmitted.
-- - provides a Wishbone-B4 interface to the host
-- - embeds source MAC addresses if they aren't defined by the host
-- - decodes TX OOB data and passes it to the timestamping unit
-------------------------------------------------------------------------------
--
-- Copyright (c) 2009 - 2017 CERN
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
use work.gencores_pkg.all;
use work.genram_pkg.all;
use work.wr_fabric_pkg.all;
use work.endpoint_private_pkg.all;
use work.endpoint_pkg.all;
use work.ep_wbgen2_pkg.all;

entity ep_tx_header_processor is
  generic(
    g_with_packet_injection : boolean;
    g_with_timestamper      : boolean;
    g_force_gap_length      : integer;
    g_runt_padding          : boolean
    );

  port (
    clk_sys_i : in std_logic;
    rst_n_i   : in std_logic;

------------------------------------------------------------------------------
-- Physical Coding Sublayer (PCS) interface
------------------------------------------------------------------------------

    src_fab_o  : out t_ep_internal_fabric;
    src_dreq_i : in  std_logic;

    pcs_busy_i  : in std_logic;
    pcs_error_i : in std_logic;

-------------------------------------------------------------------------------
-- WRF Sink (see WRF specification for the details)
-------------------------------------------------------------------------------    

    wb_snk_i : in  t_wrf_sink_in;
    wb_snk_o : out t_wrf_sink_out;

-------------------------------------------------------------------------------
-- Flow Control Unit signals
-------------------------------------------------------------------------------    

-- TX send pause frame - when active, the framer will send a PAUSE frame
-- as soon as possible. The pause quanta must be provided on tx_pause_delay_i input.
    fc_pause_req_i   : in std_logic;
    fc_pause_delay_i : in std_logic_vector(15 downto 0);

-- TX send pause acknowledge - active after the current pause send request has
-- been completed
    fc_pause_ready_o : out std_logic;

-- When active, the framer will allow for packet transmission.
    fc_flow_enable_i : in std_logic;

-------------------------------------------------------------------------------
-- OOB/TSU signals
-------------------------------------------------------------------------------    

-- Port ID value
    txtsu_port_id_o      : out std_logic_vector(4 downto 0);
-- Frame ID value
    txtsu_fid_o          : out std_logic_vector(16 -1 downto 0);
-- Encoded timestamps
    txtsu_ts_value_o     : out std_logic_vector(28 + 4 - 1 downto 0);
    txtsu_ts_incorrect_o : out std_logic;

-- TX timestamp strobe: HI tells the TX timestamping unit that a timestamp is
-- available on txtsu_ts_value_o, txtsu_fid_o andd txtsu_port_id_o. The correctness
-- of the timestamping is indiacted on txtsu_ts_incorrect_o. Line remains HI
-- until assertion of txtsu_ack_i.
    txtsu_stb_o : out std_logic;

-- TX timestamp acknowledge: HI indicates that TXTSU has successfully received
-- the timestamp
    txtsu_ack_i : in std_logic;

---------------------------------------------------------------------------
-- Timestamp input from the timestamping unit
---------------------------------------------------------------------------
    txts_timestamp_i       : in std_logic_vector(31 downto 0);
    txts_timestamp_valid_i : in std_logic;

-------------------------------------------------------------------------------
-- Control registers
-------------------------------------------------------------------------------
    ep_ctrl_i           : in std_logic;
    regs_i : in t_ep_out_registers

    );


end ep_tx_header_processor;

architecture behavioral of ep_tx_header_processor is

  constant c_IFG_LENGTH : integer := g_force_gap_length ;--0;
  constant c_MIN_FRAME_THR  : integer := 30;
  constant c_MIN_QFRAME_THR : integer := 32; -- we need to pad more 802.1q
                                             -- tagged frame if it's going to be
                                             -- untagged later in the tx_path

  type t_tx_framer_state is (TXF_IDLE, TXF_DELAYED_SOF, TXF_ADDR, TXF_DATA, TXF_GAP, TXF_PAD, TXF_ABORT, TXF_STORE_TSTAMP);

-- general signals
  signal state   : t_tx_framer_state;
  signal counter : unsigned(13 downto 0);

-- Flow Control-related signals
  signal tx_pause_mode  : std_logic;

  signal snk_valid : std_logic;

  signal sof_p1, eof_p1, abort_p1, error_p1 : std_logic;
  signal snk_cyc_d0                         : std_logic;

  signal stored_status : t_wrf_status_reg;

  type t_oob_fsm_state is (OOB_IDLE, OOB_1, OOB_2);

  signal oob_state : t_oob_fsm_state;
  signal oob       : t_wrf_oob;

  signal wb_out         : t_wrf_sink_out;
  signal decoded_status : t_wrf_status_reg;

  signal abort_now : std_logic;
  signal stall_int : std_logic;
  signal tx_en        : std_logic;
  signal ep_ctrl     : std_logic;
  signal bitsel_d    : std_logic;
  signal needs_padding  : std_logic;
  signal to_be_untagged : std_logic;
  signal sof_reg     : std_logic;

  function b2s (x : boolean)
    return std_logic is
  begin
    if(x) then
      return '1';
    else
      return '0';
    end if;
  end function;

  function f_pick (cond : boolean; when_1 : std_logic_vector; when_0 : std_logic_vector)
    return std_logic_vector is
  begin
    if(cond) then
      return when_1;
    else
      return when_0;
    end if;
  end function;

  function f_pick (cond : std_logic; when_1 : std_logic_vector; when_0 : std_logic_vector)
    return std_logic_vector is
  begin
    if(cond = '1') then
      return when_1;
    else
      return when_0;
    end if;
  end function;

  function f_pick (cond : boolean; when_1 : std_logic ; when_0 : std_logic)
    return std_logic is
  begin
    if(cond) then
      return when_1;
    else
      return when_0;
    end if;
  end function;
  
  function f_fabric_2_slv (
    in_i : t_wrf_sink_in;
    in_o : t_wrf_sink_out) return std_logic_vector is
    variable tmp : std_logic_vector(31 downto 0);
  begin
    tmp(15 downto 0)  := in_i.dat;
    tmp(17 downto 16) := in_i.adr;
    tmp(19 downto 18) := in_i.sel;
    tmp(20)           := in_i.cyc;
    tmp(21)           := in_i.stb;
    tmp(22)           := in_i.we;
    tmp(23)           := in_o.ack;
    tmp(24)           := in_o.stall;
    tmp(25)           := in_o.err;
    tmp(26)           := in_o.rty;
    return tmp;
  end f_fabric_2_slv;
  
begin  -- behavioral
  
  p_detect_frame : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        snk_cyc_d0 <= '0';
      else
        snk_cyc_d0 <= wb_snk_i.cyc;
      end if;
    end if;
  end process;

  sof_p1 <= not snk_cyc_d0 and wb_snk_i.cyc;
  eof_p1 <= snk_cyc_d0 and not wb_snk_i.cyc;

  snk_valid <= (wb_snk_i.cyc and wb_snk_i.stb and wb_snk_i.we) and not wb_out.stall;

  decoded_status <= f_unmarshall_wrf_status(wb_snk_i.dat);

  error_p1 <= snk_valid and b2s(wb_snk_i.adr = c_WRF_STATUS) and decoded_status.error;

-- abort_now <= '1' when (state /= TXF_IDLE and state /= TXF_GAP) and (regs_i.ecr_tx_en_o = '0' or error_p1 = '1') else '0';
 abort_now <= '1' when (state /= TXF_IDLE and state /= TXF_GAP) and (tx_en = '0' or error_p1 = '1') else
              '1' when (state = TXF_ABORT and wb_snk_i.cyc = '1' ) else 
              '0'; -- ML

  GEN_PADDING: if(g_runt_padding) generate
    needs_padding <= '1' when( (to_be_untagged = '1' and counter < c_MIN_QFRAME_THR) or
                                counter < c_MIN_FRAME_THR) else
                     '0';
  end generate;
  GEN_NOPADDING: if( not g_runt_padding) generate
    -- even if padding is disabled, we still need to pad short 802.1q frames
    -- that will be untagged
    needs_padding <= '1' when (to_be_untagged = '1' and counter < c_MIN_QFRAME_THR) else
                     '0';
  end generate;

  process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if(rst_n_i='0') then
        sof_reg <= '0';
      elsif(sof_p1='1') then
        sof_reg <= '1';
      elsif(state = TXF_ADDR) then
        sof_reg <= '0';
      end if;
    end if;
  end process;

  p_store_status : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      
      if rst_n_i = '0' or tx_pause_mode = '1' then
        stored_status.has_smac <= '0';
        stored_status.is_hp    <= '0';
        stored_status.has_crc  <= '0';
      else
        if(snk_valid = '1' and wb_snk_i.adr = c_WRF_STATUS) then
          stored_status <= f_unmarshall_wrf_status(wb_snk_i.dat);
        end if;
      end if;
    end if;
  end process;

  -----------------------------------------------------------------------------
  -- Out-of-band handling state machine. When a packet comes with OOB info
  -- (frame ID), it is registered here. Then the main FSM waits until the TX
  -- FIFO of the endpoint is completely empty and pushes the last timestamp 
  -- from the PCS to the TX Timestamping Unit
  -----------------------------------------------------------------------------
  gen_ts_supported : if(g_with_timestamper) generate
    p_oob_fsm : process(clk_sys_i)
    begin
      if rising_edge(clk_sys_i) then
        if (rst_n_i = '0' or state = TXF_ADDR) then
          oob_state    <= OOB_1;
          oob.valid    <= '0';
          oob.oob_type <= (others => '0');
        else
          
          case oob_state is
            when OOB_1 =>
              if(snk_valid = '1' and wb_snk_i.adr = c_WRF_OOB) then
                oob.oob_type <= wb_snk_i.dat(15 downto 12);
                oob_state    <= OOB_2;
                oob.valid    <= '0';
              end if;
              
            when OOB_2 =>
              if(snk_valid = '1' and wb_snk_i.adr = c_WRF_OOB and oob.oob_type = c_WRF_OOB_TYPE_TX) then
                oob.frame_id <= wb_snk_i.dat(15 downto 0);
                oob_state    <= OOB_IDLE;
                oob.valid    <= '1';
              end if;
              
            when OOB_IDLE =>
              oob_state <= OOB_IDLE;
          end case;
        end if;
      end if;
    end process;
  end generate gen_ts_supported;

  p_tx_fsm : process (clk_sys_i)
  begin  -- process
    if rising_edge(clk_sys_i) then
      if(rst_n_i = '0') then
        state <= TXF_IDLE;

        src_fab_o.has_rx_timestamp   <= '0';
        src_fab_o.rx_timestamp_valid <= '0';
        src_fab_o.dvalid             <= '0';
        src_fab_o.ERROR              <= '0';
        src_fab_o.sof                <= '0';
        src_fab_o.eof                <= '0';
        src_fab_o.bytesel            <= '0';

        wb_out.err <= '0';
        wb_out.rty <= '0';

        tx_pause_mode <= '0';

        fc_pause_ready_o <= '1';

        txtsu_stb_o <= '0';
        bitsel_d  <= '0';
        to_be_untagged <= '0';

      else

        -- we are in the middle of the frame and the framer has got suddenly
        -- disabled or we've received an ABORT command or an error occured in the PCS:

        if(pcs_error_i = '1') then
          state      <= TXF_IDLE;
          wb_out.rty <= '1';
          ----------------------------------------------------------------------------------
          src_fab_o.error  <= '1'; -- nasty-bug-fix: it might happen that PCS throws error
                                   -- to a previous frame, but we already start sending
                                   -- the next one, in this case the frame is stopped being
                                   -- sent but PCS does not know why... in the end we see
                                   -- two SOFs in PACs
          ----------------------------------------------------------------------------------                         
        elsif(abort_now = '1') then
          -- abort the current frame
          state            <= TXF_ABORT;
          src_fab_o.sof    <= '0';
          src_fab_o.dvalid <= '0';

        else

          case state is

-------------------------------------------------------------------------------
-- TX FSM state IDLE: awaits incoming TX frames
-------------------------------------------------------------------------------

            when TXF_IDLE =>            -- idle state - wait for the next frame

              wb_out.err <= '0';
              wb_out.rty <= '0';

              txtsu_stb_o <= '0';
              bitsel_d    <= '0';

              src_fab_o.error  <= '0';
              src_fab_o.eof    <= '0';
              src_fab_o.dvalid <= '0';
              src_fab_o.bytesel <= '0';
              to_be_untagged    <= '0';

              -- Check start-of-frame and send-pause signals and eventually
              -- commence frame transmission

--             if(src_dreq_i = '1' and (sof_p1 = '1' or fc_pause_req_i = '1') and tx_en = '1') then --ML:removed
--            EXPLANATION: removed src_dreq_i = '1' as the cycle can start on stall HIGH (dreq_i LOW), 
--            it means that if we wait for dreq to be high.... we can miss SOF and thus entire frame. 
--            New state added to include a case where SOF (start of cycle) starts when dreq is LOW.
--            (we cannot just go to TXF_ADDR... it is because the PCS needs the minimal gap to add CRC)
              if((sof_p1 = '1' or sof_reg='1' or fc_pause_req_i = '1') and tx_en = '1') then --ML

                fc_pause_ready_o <= '0';
                tx_pause_mode    <= fc_pause_req_i;

                counter       <= (others => '0');
                
                if(src_dreq_i = '1') then
                  state         <= TXF_ADDR;
                  src_fab_o.sof <= '1';
                end if;
                
              else
                src_fab_o.sof <= '0';
              end if;

-------------------------------------------------------------------------------
-- TX FSM (ML-added): this state takes into accunt the rare case where SOF happens 
-- when dreq is LOW (PCS not ready). So we wait for dreq HIGH and STALL in the 
-- meanttime (see process at the end)
-------------------------------------------------------------------------------
            when TXF_DELAYED_SOF =>
             
              if(src_dreq_i = '1') then
                state         <= TXF_ADDR;
                src_fab_o.sof <= '1';
             end if;              
-------------------------------------------------------------------------------
-- TX FSM state HEADER: processes the frame header, send pause frames
-- if compiled without packet injection support.
-------------------------------------------------------------------------------

            when TXF_ADDR =>
              src_fab_o.sof <= '0';
              
              if (src_dreq_i = '1' and ((snk_valid = '1' and wb_snk_i.adr = c_WRF_DATA) or (tx_pause_mode = '1' and not g_with_packet_injection))) then

                counter          <= counter + 1;
                src_fab_o.dvalid <= '1';

                case counter(3 downto 0) is
                  when x"0" =>
                    src_fab_o.data <= f_pick(tx_pause_mode = '1' and not g_with_packet_injection, x"0180", wb_snk_i.dat);
                  when x"1" =>
                    src_fab_o.data <= f_pick(tx_pause_mode = '1' and not g_with_packet_injection, x"c200", wb_snk_i.dat);
                  when x"2" =>
                    src_fab_o.data <= f_pick(tx_pause_mode = '1' and not g_with_packet_injection, x"0001", wb_snk_i.dat);
                  when x"3" =>
                    src_fab_o.data <= f_pick(stored_status.has_smac, wb_snk_i.dat, regs_i.mach_o);
                  when x"4" =>
                    src_fab_o.data <= f_pick(stored_status.has_smac, wb_snk_i.dat, regs_i.macl_o(31 downto 16));
                  when x"5" =>
                    src_fab_o.data <= f_pick(stored_status.has_smac, wb_snk_i.dat, regs_i.macl_o(15 downto 0));
                    if(tx_pause_mode = '0' or g_with_packet_injection) then
                      state <= TXF_DATA;
                    end if;
                  when x"6" =>
                    src_fab_o.data <= f_pick(g_with_packet_injection, "XXXXXXXXXXXXXXXX", x"8808");
                    to_be_untagged <= f_pick(wb_snk_i.dat = x"8100" and
                                             regs_i.vcr0_qmode_o = c_QMODE_PORT_ACCESS, '1', '0');
                  when x"7" =>
                    src_fab_o.data <= f_pick(g_with_packet_injection, "XXXXXXXXXXXXXXXX", x"0001"); -- peterj: IEEE 802.3 Table 31A-1 MAC control codes PAUSE (Annex 31B)
                  when x"8" =>
                    src_fab_o.data <= f_pick(g_with_packet_injection, "XXXXXXXXXXXXXXXX", fc_pause_delay_i); -- ML: bug ??? (forget optcode: 0x0001)
                    state          <= TXF_PAD;
                  when others =>
                    state <= TXF_PAD;
                end case;
                
                src_fab_o.addr   <= c_WRF_DATA;
                
              else
                src_fab_o.dvalid <= '0';
                src_fab_o.data   <= (others => 'X');
                src_fab_o.addr   <= (others => 'X');
              end if;

-------------------------------------------------------------------------------
-- TX FSM state PAD: pads a pause frame to 64 bytes.
-------------------------------------------------------------------------------

            when TXF_PAD =>
              
              if(src_dreq_i = '1') then
                counter <= counter + 1;

                src_fab_o.data   <= (others => '0');
                src_fab_o.dvalid <= '1';
                src_fab_o.addr   <= (others => '0');

                if( (to_be_untagged = '1' and counter = c_MIN_QFRAME_THR-1) or
                    (to_be_untagged = '0' and counter = c_MIN_FRAME_THR-1) ) then
                  state <= TXF_GAP;
                  src_fab_o.eof    <= '1';
                end if;
                
              else
                src_fab_o.data   <= (others => '0');
                src_fab_o.dvalid <= '0';
                src_fab_o.addr   <= (others => '0');
              end if;

-------------------------------------------------------------------------------
-- TX FSM state DATA: trasmits frame payload
-------------------------------------------------------------------------------

            when TXF_DATA =>

              -- ML: added this EOF force LOW to make sure that EOF is single cycle, withouth
              -- this, it might have happened that we had eof_p1 but PCS was busy, so we set
              -- src_fab_o.eof to HIGH but actually did not exit the TXF_DATA state... this
              -- caused EOF to be longer than one cycle
              src_fab_o.eof   <= '0'; 

              if (counter = x"6") then
                to_be_untagged <= f_pick(wb_snk_i.dat = x"8100" and
                                         regs_i.vcr0_qmode_o = c_QMODE_PORT_ACCESS, '1', '0');
              end if;
              
              if((wb_snk_i.adr = c_WRF_OOB or eof_p1='1') and needs_padding='1') then
                state <= TXF_PAD;
              elsif(eof_p1 = '1' and needs_padding='0') then
                src_fab_o.eof <= '1';
                counter       <= (others => '0');
    
                if(g_force_gap_length = 0 and bitsel_d = '1') then -- only for odd

                  -- Submit the TX timestamp to the TXTSU queue
                  if(oob.valid = '1' and oob.oob_type = c_WRF_OOB_TYPE_TX) then
                    if(pcs_busy_i = '0') then
                      txtsu_stb_o          <= '1';
                      txtsu_ts_incorrect_o <= not txts_timestamp_valid_i;
                      txtsu_ts_value_o     <= txts_timestamp_i;
                      txtsu_port_id_o      <= regs_i.ecr_portid_o;
                      txtsu_fid_o          <= oob.frame_id;
                      state                <= TXF_STORE_TSTAMP;
                    else
                       -- wait in the GAP state for pcs_busy_i LOW
                      state                <= TXF_GAP;
                    end if;                                      ---if(pcs_busy_i = '0') then
                  else
                    -- dont need timestamp, don't need GAP, just go to IDLE
                    state <= TXF_IDLE;
                  end if;                                        -- if(oob.valid = '1' and oob.oob_type = c_WRF_OOB_TYPE_TX) then
                else -- need some GAP
                  state         <= TXF_GAP;
                end if;                                          -- f(g_force_gap_length = 0 and bitsel_d = '1') then      
              end if;                                            -- if(eof_p1 = '1') then

              if(snk_valid = '1' and wb_snk_i.adr = c_WRF_DATA) then
                src_fab_o.data    <= wb_snk_i.dat;
                src_fab_o.dvalid  <= '1';
                src_fab_o.bytesel <= (not wb_snk_i.sel(0)) and (not needs_padding);
                counter <= counter + 1;
              else
                src_fab_o.dvalid  <= '0';
                src_fab_o.data    <= (others => 'X');
                src_fab_o.bytesel <= '0';
              end if;
              
              if(wb_snk_i.sel(0) = '0') then
                bitsel_d <='1';
              end if;  

              if(needs_padding='1') then
                src_fab_o.addr  <= (others=>'0');
              else
                src_fab_o.addr    <= wb_snk_i.adr;
              end if;

-------------------------------------------------------------------------------
-- TX FSM states: WAIT_CRC, EMBED_CRC: dealing with frame checksum field
-------------------------------------------------------------------------------            

            when TXF_GAP =>
              src_fab_o.eof    <= '0';
              src_fab_o.error  <= '0';
              src_fab_o.dvalid <= '0';
              wb_out.err       <= '0';
              wb_out.rty       <= '0';
              src_fab_o.bytesel <= '0';

              if(counter >= c_IFG_LENGTH or g_force_gap_length = 0) then

                -- Submit the TX timestamp to the TXTSU queue
                if(oob.valid = '1' and oob.oob_type = c_WRF_OOB_TYPE_TX) then
                  if(pcs_busy_i = '0') then
                    txtsu_stb_o          <= '1';
                    txtsu_ts_incorrect_o <= not txts_timestamp_valid_i;
                    txtsu_ts_value_o     <= txts_timestamp_i;
                    txtsu_port_id_o      <= regs_i.ecr_portid_o;
                    txtsu_fid_o          <= oob.frame_id;
                    state                <= TXF_STORE_TSTAMP;
                  end if;
                else
                  state <= TXF_IDLE;
                end if;

              else
                counter <= counter + 1;
              end if;

            when TXF_STORE_TSTAMP =>  -- to slow ??? anyway, we can finish the frame
              
              src_fab_o.eof    <= '0';
              src_fab_o.error  <= '0';
              src_fab_o.dvalid <= '0';
              wb_out.err       <= '0';
              wb_out.rty       <= '0';
              src_fab_o.bytesel<= '0';

              if(txtsu_ack_i = '1') then
                txtsu_stb_o <= '0';
                state       <= TXF_IDLE;
              end if;

-------------------------------------------------------------------------------
-- TX FSM state ABORT: signalize underlying PCS block to abort the frame
-- immediately, corrupting its contents
-------------------------------------------------------------------------------            
            when TXF_ABORT =>
              src_fab_o.sof    <= '0';
              src_fab_o.dvalid <= '1';
              src_fab_o.error  <= '1';

              counter <= (others => '0');
              state   <= TXF_IDLE;

          end case;
        end if;
      end if;
    end if;
  end process;

  tx_en <= regs_i.ecr_tx_en_o and ep_ctrl and ep_ctrl_i; 

  --p_gen_stall : process(src_dreq_i, state, regs_i, wb_snk_i, snk_cyc_d0, tx_en)
  p_gen_stall : process(src_dreq_i, state, tx_en, wb_snk_i, eof_p1)
  begin
    --if(regs_i.ecr_tx_en_o = '0') then
    if(tx_en = '0') then --ML
      wb_out.stall <= '0';              -- /dev/null if TX disabled
--     elsif((wb_snk_i.cyc xor snk_cyc_d0) = '1') then
--    elsif(wb_snk_i.cyc = '1' and snk_cyc_d0 = '0') then -- ML: do it only at the SOF, not EOF
--      wb_out.stall <= '1';              -- /block for 1 cycle right upon
                                        -- detection of a packet, so the FSM
                                        -- has time to catch up
    
    -- stall at EOF - the SWcore should not send anything, but just in case, not to miss
    -- SOF... the next cycle will be TXF_GAP (also stalling) or TXF_IDLE (can accept new frames)
    elsif(eof_p1 = '1') then -- accept OOB as is      
      wb_out.stall <= '1';
      
    -- when data is flowing (TXF_DATA) or we expect data (TXF_IDLE) stall only when no dreq_i 
    -- from other modules
---------------------------------------------------------------------------------------------
-- ML: when error at the very end of the frame (e.g. due to jambo frame), stall happenend
-- at the last cycle before cyc DOWN, subsequently, cycle did not finish and switch hanged
---------------------------------------------------------------------------------------------
--     elsif(src_dreq_i = '1' and state /= TXF_GAP and state /= TXF_ABORT and state /= TXF_DELAYED_SOF and state /= TXF_STORE_TSTAMP) then
---------------------------------------------------------------------------------------------
    elsif(src_dreq_i = '1' and state /= TXF_PAD and state /= TXF_GAP  and state /= TXF_DELAYED_SOF and state /= TXF_STORE_TSTAMP) then
      wb_out.stall <= '0';              -- during data/header phase - whenever
                                        -- the sink is ready to accept data
    
    -- when we receive OOB, there we have always resources/possibilties to accept it
    -- since it is dumped in here, so we prevent dreq_i going LOW from stopping
    -- to receive OOB
    elsif(wb_snk_i.adr = c_WRF_OOB and wb_snk_i.cyc = '1') then -- accept OOB as is
      wb_out.stall <= '0';              

    -- one other option renderds stall 
    else
      wb_out.stall <= '1';
    end if;
  end process;

  p_gen_ack : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      wb_out.ack <= snk_valid;
    end if;
  end process;

  -- in theory, this should not happen: we don't send frames to ports which are DOWN, but..
  -- we make sure that we don't start sending frames on the PHY in the middle of the frame...
  -- the TX is enabled only when we don't receive any frames from SWcore
  p_ctrl: process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if(rst_n_i = '0') then
        ep_ctrl  <= '1';
      else
        if(ep_ctrl_i = '0') then
          ep_ctrl <= '0';
        elsif(ep_ctrl_i = '1' and wb_snk_i.cyc = '0') then
          ep_ctrl <= '1';
        end if; --ep_ctr
      end if;-- rst
    end if;  -- clk   
  end process;

  wb_snk_o <= wb_out;

end behavioral;


library IEEE;
--! Standard packages
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

--! Additional library
library work;
--! Additional packages
use work.eb_internals_pkg.all;
use work.eb_hdr_pkg.all;
use work.wishbone_pkg.all;

entity eb_slave_fsm is
  port(
    clk_i       : in  std_logic;
    rstn_i      : in  std_logic;
    
    rx_cyc_i    : in  std_logic;
    rx_stb_i    : in  std_logic;
    rx_dat_i    : in  t_wishbone_data;
    rx_stall_o  : out std_logic;
    
    tag_stb_o   : out std_logic;
    tag_dat_o   : out t_tag;
    tag_full_i  : in  std_logic;
    
    pass_stb_o  : out std_logic;
    pass_dat_o  : out t_wishbone_data;
    pass_full_i : in  std_logic;
    
    cfg_stb_o   : out std_logic;
    cfg_adr_o   : out t_wishbone_address;
    cfg_full_i  : in  std_logic;
    
    wbm_stb_o   : out std_logic;
    wbm_full_i  : in  std_logic;
    wbm_busy_i  : in  std_logic;
    
    master_o       : out t_wishbone_master_out;
    master_stall_i : in  std_logic);
    
end entity;

architecture behavioral of eb_slave_fsm is

  type t_state_RX is (S_EB_HDR, S_PROBE_DROP, S_PROBE_ID, S_CYC_HDR, S_WR_ADR, S_WRITE, S_RD_ADR, S_READ, S_DROP);
  
  signal r_tag_stb_o    : std_logic;
  signal r_tag_dat_o    : t_tag;
  signal r_pass_stb_o   : std_logic;
  signal r_pass_dat_o   : t_wishbone_data;
  signal r_cfg_stb_o    : std_logic;
  signal r_wbm_stb_o    : std_logic;
  signal r_master_cyc_o : std_logic;
  signal r_master_stb_o : std_logic;
  signal r_master_we_o  : std_logic;
  signal r_master_adr_o : t_wishbone_address;
  signal r_master_dat_o : t_wishbone_data;
  signal r_wr_adr       : unsigned(t_wishbone_address'range);
  signal r_rx_rec_hdr   : t_rec_hdr;
  signal r_tx_rec_hdr   : t_rec_hdr;
  signal r_rx_cyc       : std_logic;
  signal r_state        : t_state_RX;
  signal s_stall        : std_logic;
  signal s_wbm_busy     : std_logic;
  signal s_rx_eb_hdr    : t_eb_hdr;
  signal s_tx_eb_hdr    : t_eb_hdr;
  signal s_rx_rec_hdr   : t_rec_hdr;
  signal s_tx_rec_hdr   : t_rec_hdr;
  
  function f_reply_eb(rx_eb_hdr : t_eb_hdr) return t_eb_hdr is
    variable tx_eb_hdr : t_eb_hdr := c_eb_init;
  begin
    tx_eb_hdr.probe_res := rx_eb_hdr.probe;
    return tx_eb_hdr;
  end function;

  function f_reply_rec(rx_rec_hdr : t_rec_hdr) return t_rec_hdr is
    variable tx_rec_hdr : t_rec_hdr := C_rec_init;
  begin
    tx_rec_hdr.WCA_CFG  := rx_rec_hdr.BCA_CFG;
    tx_rec_hdr.RD_FIFO  := '0';
    tx_rec_hdr.RD_CNT   := (others => '0');
    tx_rec_hdr.WR_FIFO  := rx_rec_hdr.RD_FIFO;
    tx_rec_hdr.WR_CNT   := rx_rec_hdr.RD_CNT;
    tx_rec_hdr.SEL      := rx_rec_hdr.SEL;
    tx_rec_hdr.DROP_CYC := rx_rec_hdr.DROP_CYC;
    return tx_rec_hdr;
  end function;

begin
              
  rx_stall_o <= s_stall;
  tag_stb_o  <= r_tag_stb_o;
  tag_dat_o  <= r_tag_dat_o;
  pass_stb_o <= r_pass_stb_o;
  pass_dat_o <= r_pass_dat_o;
  cfg_stb_o  <= r_cfg_stb_o;
  cfg_adr_o  <= r_master_adr_o;
  wbm_stb_o  <= r_wbm_stb_o;
  
  s_wbm_busy   <= wbm_busy_i or r_wbm_stb_o; -- cope with 1 cycle latency
  master_o.cyc <= r_master_cyc_o or s_wbm_busy;
  master_o.stb <= r_master_stb_o;
  master_o.we  <= r_master_we_o;
  master_o.adr <= r_master_adr_o;
  master_o.dat <= r_master_dat_o;
  master_o.sel <= r_rx_rec_hdr.sel(master_o.sel'range);
  
  -- Stall the RX path if:
  --   Any TX FIFO is full
  --   We are pushing a strobe that is stalled
  --   We are waiting to lower the cycle line
  s_stall <= tag_full_i OR pass_full_i OR cfg_full_i OR wbm_full_i OR 
             (r_master_stb_o and master_stall_i) OR
             (not r_master_cyc_o and s_wbm_busy);
  
  s_rx_eb_hdr <= f_parse_eb(rx_dat_i);
  s_tx_eb_hdr <= f_reply_eb(s_rx_eb_hdr);
  
  s_rx_rec_hdr <= f_parse_rec(rx_dat_i);
  s_tx_rec_hdr <= f_reply_rec(s_rx_rec_hdr);
  
  fsm : process(clk_i, rstn_i) is
  begin
    if (rstn_i = '0') then
      r_tag_stb_o    <= '0';
      r_tag_dat_o    <= (others => '0');
      r_pass_stb_o   <= '0';
      r_pass_dat_o   <= (others => '0');
      r_cfg_stb_o    <= '0';
      r_wbm_stb_o    <= '0';
      r_master_cyc_o <= '0';
      r_master_stb_o <= '0';
      r_master_we_o  <= '0';
      r_master_adr_o <= (others => '0');
      r_master_dat_o <= (others => '0');
      r_wr_adr       <= (others => '0');
      r_rx_rec_hdr   <= c_rec_init;
      r_tx_rec_hdr   <= c_rec_init;
      r_rx_cyc       <= '0';
      r_state        <= S_EB_HDR;
    elsif rising_edge(clk_i) then
    
      -- By default, write nowhere in particular
      r_tag_stb_o  <= '0';
      r_pass_stb_o <= '0';
      r_cfg_stb_o  <= '0';
      r_wbm_stb_o  <= '0';
      
      -- Lower strobe line when it is queued
      r_master_stb_o <= r_master_stb_o and master_stall_i;
      
      -- Register to enable detecting falling edge
      r_rx_cyc <= rx_cyc_i;
      
      if(rx_cyc_i = '0') then
        -- expect a new negotiation header
        r_state <= S_EB_HDR; 
        -- guard against improperly terminated streams
        r_master_cyc_o <= '0'; 
        
        -- On falling edge of RX cycle line, push a tag to drop TX cycle
        if r_rx_cyc = '1' then
          r_tag_stb_o <= '1';
          r_tag_dat_o <= c_tag_drop_tx;
        end if;
      elsif(rx_stb_i = '1' and s_stall = '0') then
        -- Every non-error state must write something
        
        case r_state is
          when s_EB_HDR =>
            -- supported EB header?
            if (s_rx_eb_hdr.magic = c_eb_magic and s_rx_eb_hdr.ver = c_eb_ver) then
              -- Raise TX cycle line if this needs to be sent
              if s_rx_eb_hdr.no_response = '1' then
                r_tag_stb_o  <= '1';
                r_tag_dat_o  <= c_tag_skip_tx;
              else
                -- Write the header using pass fifo
                r_tag_stb_o  <= '1';
                r_tag_dat_o  <= c_tag_pass_tx;
                r_pass_stb_o <= '1';
                r_pass_dat_o <= f_format_eb(s_tx_eb_hdr);
              end if;
              
              if s_rx_eb_hdr.probe = '1' then
                if s_rx_eb_hdr.addr_size(2) = '1' and
                   s_rx_eb_hdr.data_size(2) = '1' then
                  -- Allow follow-up payload for stream channels
                  r_state <= S_PROBE_ID; 
                else
                  -- Does not support 32-bit? drop anything after probe id
                  r_state <= S_PROBE_DROP;
                end if;  
              else -- not a probe
                if s_rx_eb_hdr.addr_size = x"4" or
                   s_rx_eb_hdr.data_size = x"4" then
                  -- Must be the exactly right format if not a probe!
                  r_state <= S_CYC_HDR;
                else
                  -- Do the best we can ... report proper EB header and pad with 0s
                  r_state <= S_DROP;
                end if;  
              end if;
            else  --bad eb header. drop all til cycle line is lowered again
              r_tag_stb_o <= '1';
              r_tag_dat_o <= c_tag_skip_tx;
              r_state     <= S_DROP;
            end if;
          
          when S_PROBE_DROP =>
            -- Write the probe-id using pass fifo
            r_tag_stb_o  <= '1';
            r_tag_dat_o  <= c_tag_pass_on;
            r_pass_stb_o <= '1';
            r_pass_dat_o <= rx_dat_i;
            
            r_state <= s_DROP;
          
          when S_PROBE_ID =>
            -- Write the probe-id using pass fifo
            r_tag_stb_o  <= '1';
            r_tag_dat_o  <= c_tag_pass_on;
            r_pass_stb_o <= '1';
            r_pass_dat_o <= rx_dat_i;
            
            r_state <= s_CYC_HDR;
          
          when S_CYC_HDR  =>
            r_tx_rec_hdr  <= s_tx_rec_hdr;
            r_rx_rec_hdr  <= s_rx_rec_hdr;
            
            -- Write padding/header using pass fifo
            r_tag_stb_o  <= '1';
            r_tag_dat_o  <= c_tag_pass_on;
            r_pass_stb_o <= '1';
            
            if (s_rx_rec_hdr.WR_CNT /= 0) then
              --padding logic 1. insert padding instead of the header                                  
              r_pass_dat_o <= x"00000000";
              r_state <= S_WR_ADR;  
            elsif (s_rx_rec_hdr.RD_CNT /= 0) then
              --no writes, no padding. insert the header
              r_pass_dat_o <= f_format_rec(s_tx_rec_hdr);
              r_state <= S_RD_ADR;
            else
              --no writes, no padding. insert the header 
              r_pass_dat_o <= f_format_rec(s_tx_rec_hdr);
              
              r_master_cyc_o <= r_master_cyc_o and not s_rx_rec_hdr.DROP_CYC;
              r_state <= S_CYC_HDR;
            end if;
            
          when S_WR_ADR =>
            r_wr_adr <= unsigned(rx_dat_i);
            
            -- Write padding using pass fifo
            r_tag_stb_o  <= '1';
            r_tag_dat_o  <= c_tag_pass_on;
            r_pass_stb_o <= '1';
            r_pass_dat_o <= x"00000000";
            
            r_state <= S_WRITE;
          
          when S_WRITE =>
            r_master_we_o  <= '1';
            r_master_adr_o <= std_logic_vector(r_wr_adr);
            r_master_dat_o <= rx_dat_i;
            
            if(r_rx_rec_hdr.WR_FIFO = '0') then
              r_wr_adr <= r_wr_adr + 4;
            end if;
            
            -- Write padding/header using pass fifo
            r_tag_stb_o  <= '1';
            r_pass_stb_o <= '1';
            
            -- Writes need their output discarded
            if r_rx_rec_hdr.WCA_CFG = '1' then
              r_cfg_stb_o    <= '1';
              r_tag_dat_o    <= c_tag_cfg_ign;
            else
              r_wbm_stb_o    <= '1';
              r_tag_dat_o    <= c_tag_wbm_ign;
              r_master_cyc_o <= '1';
              r_master_stb_o <= '1';
            end if;
            
            if (r_rx_rec_hdr.WR_CNT /= 1) then
              r_pass_dat_o <= x"00000000";
            else
              r_pass_dat_o <= f_format_rec(r_tx_rec_hdr);
              
              if (r_rx_rec_hdr.RD_CNT /= 0) then
                r_state <= S_RD_ADR;
              else
                r_master_cyc_o <= r_master_cyc_o and not r_rx_rec_hdr.DROP_CYC;  
                r_state <= S_CYC_HDR;
              end if;
            end if;
            
            r_rx_rec_hdr.WR_CNT <= r_rx_rec_hdr.WR_CNT - 1;
              
          when S_RD_ADR =>
            -- Copy address using pass fifo
            r_tag_stb_o  <= '1';
            r_tag_dat_o  <= c_tag_pass_on;
            r_pass_stb_o <= '1';
            r_pass_dat_o <= rx_dat_i; --pass the rx readback address as base write address to tx
            
            r_state <= S_READ;
            
          when S_READ =>
            r_master_we_o  <= '0';
            r_master_adr_o <= rx_dat_i;
            
            -- Get data from either cfg or wbm fifos
            r_tag_stb_o <= '1';
            
            if r_rx_rec_hdr.RCA_CFG = '1' then
              r_cfg_stb_o <= '1';
              r_tag_dat_o <= c_tag_cfg_req;
            else
              r_wbm_stb_o <= '1';
              r_tag_dat_o <= c_tag_wbm_req;
              r_master_cyc_o <= '1';
              r_master_stb_o <= '1';
            end if;
            
            if(r_rx_rec_hdr.RD_CNT = 1) then
              r_master_cyc_o <= r_master_cyc_o and not r_rx_rec_hdr.DROP_CYC;  
              r_state <= S_CYC_HDR;
            end if;
            
            r_rx_rec_hdr.RD_CNT <= r_rx_rec_hdr.RD_CNT - 1;
            
          when S_DROP =>
            -- preserve packet length
            r_tag_stb_o  <= '1';
            r_tag_dat_o  <= c_tag_pass_on;
            r_pass_stb_o <= '1';
            r_pass_dat_o <= (others => '0');

          when others =>
            r_state <= S_DROP;
        
        end case;
      end if; --rx_stb_i
    end if; --clk edge  
  end process;

end architecture;

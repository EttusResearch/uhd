library IEEE;
--! Standard packages
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

--! Additional library
library work;
use work.wishbone_pkg.all;
use work.wr_fabric_pkg.all;
use work.eb_hdr_pkg.all;

package eb_internals_pkg is 

  subtype t_tag is std_logic_vector(2 downto 0);

  constant c_tag_drop_tx : t_tag := "000";
  constant c_tag_skip_tx : t_tag := "001";
  constant c_tag_pass_tx : t_tag := "010";
  constant c_tag_pass_on : t_tag := "011";
  constant c_tag_cfg_req : t_tag := "100";
  constant c_tag_cfg_ign : t_tag := "101";
  constant c_tag_wbm_req : t_tag := "110";
  constant c_tag_wbm_ign : t_tag := "111";
  
  constant c_queue_depth : natural := 32;

  component eb_slave_top is
    generic(
      g_sdb_address    : t_wishbone_address;
      g_timeout_cycles : natural);
    port(
      clk_i       : in std_logic;  --! System Clk
      nRst_i      : in std_logic;  --! active low sync reset

      EB_RX_i     : in  t_wishbone_slave_in;   --! Streaming wishbone(record) sink from RX transport protocol block
      EB_RX_o     : out t_wishbone_slave_out;  --! Streaming WB sink flow control to RX transport protocol block
      EB_TX_i     : in  t_wishbone_master_in;  --! Streaming WB src flow control from TX transport protocol block
      EB_TX_o     : out t_wishbone_master_out; --! Streaming WB src to TX transport protocol block
      
      skip_stb_o  : out std_logic; --! Does a packet get discarded?
      skip_stall_i: in  std_logic;

      WB_config_i : in  t_wishbone_slave_in;    --! WB V4 interface to WB interconnect/device(s)
      WB_config_o : out t_wishbone_slave_out;   --! WB V4 interface to WB interconnect/device(s)
      WB_master_i : in  t_wishbone_master_in;   --! WB V4 interface to WB interconnect/device(s)
      WB_master_o : out t_wishbone_master_out;  --! WB V4 interface to WB interconnect/device(s)
      
      my_mac_o    : out std_logic_vector(47 downto 0);
      my_ip_o     : out std_logic_vector(31 downto 0);
      my_port_o   : out std_logic_vector(15 downto 0));
  end component;

  component eb_slave_fsm is
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
  end component;

  component eb_fifo is
    generic(
      g_width : natural;
      g_size  : natural);
    port(
      clk_i     : in  std_logic;
      rstn_i    : in  std_logic;
      w_full_o  : out std_logic;
      w_push_i  : in  std_logic;
      w_dat_i   : in  std_logic_vector(g_width-1 downto 0);
      r_empty_o : out std_logic;
      r_pop_i   : in  std_logic;
      r_dat_o   : out std_logic_vector(g_width-1 downto 0));
  end component;

  component eb_commit_fifo is
    generic(
      g_width : natural;
      g_size  : natural);
    port(
      clk_i      : in  std_logic;
      rstn_i     : in  std_logic;
      w_full_o   : out std_logic;
      w_push_i   : in  std_logic;
      w_dat_i    : in  std_logic_vector(g_width-1 downto 0);
      w_commit_i : in  std_logic;
      w_abort_i  : in  std_logic;
      r_empty_o  : out std_logic;
      r_pop_i    : in  std_logic;
      r_dat_o    : out std_logic_vector(g_width-1 downto 0));
  end component;
  
  component eb_tx_mux is
    port(
      clk_i       : in  std_logic;
      rstn_i      : in  std_logic;
      
      tag_pop_o   : out std_logic;
      tag_dat_i   : in  t_tag;
      tag_empty_i : in  std_logic;
      
      pass_pop_o   : out std_logic;
      pass_dat_i   : in  t_wishbone_data;
      pass_empty_i : in  std_logic;
      
      cfg_pop_o    : out std_logic;
      cfg_dat_i    : in  t_wishbone_data;
      cfg_empty_i  : in  std_logic;
      
      wbm_pop_o    : out std_logic;
      wbm_dat_i    : in  t_wishbone_data;
      wbm_empty_i  : in  std_logic;
      
      skip_stb_o   : out std_logic;
      skip_stall_i : in  std_logic;
      
      tx_cyc_o     : out std_logic;
      tx_stb_o     : out std_logic;
      tx_dat_o     : out t_wishbone_data;
      tx_stall_i   : in  std_logic);
  end component;

  component eb_tag_fifo is
    port(
      clk_i       : in  std_logic;
      rstn_i      : in  std_logic;
      
      fsm_stb_i   : in  std_logic;
      fsm_dat_i   : in  t_tag;
      fsm_full_o  : out std_logic;

      mux_pop_i   : in  std_logic;
      mux_dat_o   : out t_tag;
      mux_empty_o : out std_logic);
  end component;

  component eb_pass_fifo is
    port(
      clk_i       : in  std_logic;
      rstn_i      : in  std_logic;
      
      fsm_stb_i   : in  std_logic;
      fsm_dat_i   : in  t_wishbone_data;
      fsm_full_o  : out std_logic;

      mux_pop_i   : in  std_logic;
      mux_dat_o   : out t_wishbone_data;
      mux_empty_o : out std_logic);
  end component;

  component eb_cfg_fifo is
    generic(
      g_sdb_address : t_wishbone_address);
    port(
      clk_i       : in  std_logic;
      rstn_i      : in  std_logic;
      
      errreg_i    : in  std_logic_vector(63 downto 0);
      
      cfg_i       : in  t_wishbone_slave_in;
      cfg_o       : out t_wishbone_slave_out;
      
      fsm_stb_i   : in  std_logic;
      fsm_adr_i   : in  t_wishbone_address;
      fsm_full_o  : out std_logic;

      mux_pop_i   : in  std_logic;
      mux_dat_o   : out t_wishbone_data;
      mux_empty_o : out std_logic;
      
      my_mac_o    : out std_logic_vector(47 downto 0);
      my_ip_o     : out std_logic_vector(31 downto 0);
      my_port_o   : out std_logic_vector(15 downto 0));
  end component;

  component eb_wbm_fifo is
    generic(
      g_timeout_cycles : natural);
    port(
      clk_i       : in  std_logic;
      rstn_i      : in  std_logic;
      
      errreg_o    : out std_logic_vector(63 downto 0);
      wb_i        : in  t_wishbone_master_in;
      
      fsm_stb_i   : in  std_logic;
      fsm_full_o  : out std_logic;
      fsm_busy_o  : out std_logic;

      mux_pop_i   : in  std_logic;
      mux_dat_o   : out t_wishbone_data;
      mux_empty_o : out std_logic);
  end component;
  
  component eb_stream_narrow is
    generic(
      g_slave_width  : natural;
      g_master_width : natural);
    port(
      clk_i    : in  std_logic;
      rst_n_i  : in  std_logic;
      slave_i  : in  t_wishbone_slave_in;
      slave_o  : out t_wishbone_slave_out;
      master_i : in  t_wishbone_master_in;
      master_o : out t_wishbone_master_out);
  end component;

  component eb_stream_widen is
    generic(
      g_slave_width  : natural;
      g_master_width : natural);
    port(
      clk_i    : in  std_logic;
      rst_n_i  : in  std_logic;
      slave_i  : in  t_wishbone_slave_in;
      slave_o  : out t_wishbone_slave_out;
      master_i : in  t_wishbone_master_in;
      master_o : out t_wishbone_master_out);
  end component;
  
  component eb_checksum is
    port(
      clk_i  : in  std_logic;
      nRst_i : in  std_logic;
      en_i   : in  std_logic; 
      data_i : in  std_logic_vector(15 downto 0);
      done_o : out std_logic;
      sum_o  : out std_logic_vector(15 downto 0));
  end component;
  
  component eb_eth_rx is
    generic(
      g_mtu : natural);
    port(
      clk_i     : in  std_logic;
      rst_n_i   : in  std_logic;
      snk_i     : in  t_wrf_sink_in;
      snk_o     : out t_wrf_sink_out;
      master_o  : out t_wishbone_master_out;
      master_i  : in  t_wishbone_master_in;
      stb_o     : out std_logic;
      stall_i   : in  std_logic;
      mac_o     : out std_logic_vector(47 downto 0);
      ip_o      : out std_logic_vector(31 downto 0);
      port_o    : out std_logic_vector(15 downto 0);
      length_o  : out unsigned(15 downto 0));
  end component;
  
  component eb_eth_tx is
    generic(
      g_mtu : natural);
    port(
      clk_i        : in  std_logic;
      rst_n_i      : in  std_logic;
      src_i        : in  t_wrf_source_in;
      src_o        : out t_wrf_source_out;
      slave_o      : out t_wishbone_slave_out;
      slave_i      : in  t_wishbone_slave_in;
      stb_i        : in  std_logic;
      stall_o      : out std_logic;
      mac_i        : in  std_logic_vector(47 downto 0);
      ip_i         : in  std_logic_vector(31 downto 0);
      port_i       : in  std_logic_vector(15 downto 0);
      length_i     : in  unsigned(15 downto 0);
      skip_stb_i   : in  std_logic;
      skip_stall_o : out std_logic;
      my_mac_i     : in  std_logic_vector(47 downto 0);
      my_ip_i      : in  std_logic_vector(31 downto 0);
      my_port_i    : in  std_logic_vector(15 downto 0));
  end component;

----------------------------------------------------------------- 
-- EB Master Stuff
----------------------------------------------------------------- 
 

 
   component eb_master_wb_if is
    generic(g_adr_bits_hi : natural;
      g_mtu         : natural);
    port(
      clk_i       : in  std_logic;
      rst_n_i     : in  std_logic;
      
      slave_i     : in  t_wishbone_slave_in;
      slave_o     : out t_wishbone_slave_out;
      
      byte_cnt_i  : in std_logic_vector(15 downto 0);
      error_i     : in std_logic_vector(0 downto 0);
      
      
      clear_o     : out std_logic;
      flush_o     : out std_logic;
      
      my_mac_o    : out std_logic_vector(47 downto 0);
      my_ip_o     : out std_logic_vector(31 downto 0);
      my_port_o   : out std_logic_vector(15 downto 0);
      
      his_mac_o   : out std_logic_vector(47 downto 0); 
      his_ip_o    : out std_logic_vector(31 downto 0);
      his_port_o  : out std_logic_vector(15 downto 0); 
      length_o    : out unsigned(15 downto 0);
      max_ops_o   : out unsigned(15 downto 0);
      adr_hi_o    : out std_logic_vector(g_adr_bits_hi-1 downto 0);
      eb_opt_o    : out t_rec_hdr;
      
      udp_raw_o   : out std_logic;
      udp_we_o    : out std_logic;
      udp_valid_i : in  std_logic;   
      udp_data_o  : out std_logic_vector(31 downto 0));
   end component;
  
  
   component eb_framer is
   port(
    clk_i           : in   std_logic;            -- WB Clock
    rst_n_i         : in   std_logic;            -- async reset

    slave_i         : in   t_wishbone_slave_in;  -- WB op. -> not WB compliant, but the record format is convenient
    slave_o         : out  t_wishbone_slave_out;  -- flow control    
    master_o        : out  t_wishbone_master_out;
    master_i        : in   t_wishbone_master_in; 
    
    byte_cnt_o      : out  std_logic_vector(15 downto 0);
    ovf_o           : out  std_logic;
       
    tx_send_now_i   : in   std_logic;
    tx_flush_o      : out  std_logic; 
    max_ops_i       : in   unsigned(15 downto 0);
    length_i        : in   unsigned(15 downto 0); 
    cfg_rec_hdr_i   : in   t_rec_hdr -- EB cfg information, eg read from cfg space etc
   );    
   end component;
  
   component eb_record_gen is
   port(
      clk_i           : in  std_logic;            
      rst_n_i         : in  std_logic;            

      slave_i         : in  t_wishbone_slave_in;  
      slave_stall_o   : out std_logic;              
      slave_ack_o     : out std_logic;
      
      rec_valid_o     : out std_logic;            
      rec_hdr_o       : out t_rec_hdr;          
      rec_adr_rd_o    : out t_wishbone_data; 
      rec_adr_wr_o    : out t_wishbone_address; 
      rec_ack_i       : in std_logic;             
      byte_cnt_o    : out unsigned(15 downto 0); 
      cfg_rec_hdr_i   : in t_rec_hdr);   
   end component ;
  
  component eb_master_eth_tx is
  generic(
    g_mtu : natural);
  port(
    clk_i        : in  std_logic;
    rst_n_i      : in  std_logic;
    src_i        : in  t_wrf_source_in;
    src_o        : out t_wrf_source_out;
    slave_o      : out t_wishbone_slave_out;
    slave_i      : in  t_wishbone_slave_in;
    stb_i        : in  std_logic;
    stall_o      : out std_logic;
    mac_i        : in  std_logic_vector(47 downto 0);
    ip_i         : in  std_logic_vector(31 downto 0);
    port_i       : in  std_logic_vector(15 downto 0);
    skip_stb_i   : in  std_logic;
    skip_stall_o : out std_logic;
    my_mac_i     : in  std_logic_vector(47 downto 0);
    my_ip_i      : in  std_logic_vector(31 downto 0);
    my_port_i    : in  std_logic_vector(15 downto 0)
    );
   end component;
   
   component eb_commit_len_fifo is
  generic(
    g_width : natural;
    g_size  : natural);
  port(
    clk_i      : in  std_logic;
    rstn_i     : in  std_logic;
    w_cnt_o    : out unsigned(f_ceil_log2(g_size) downto 0);
    r_cnt_o    : out unsigned(f_ceil_log2(g_size) downto 0);
    w_full_o   : out std_logic;
    w_push_i   : in  std_logic;
    w_dat_i    : in  std_logic_vector(g_width-1 downto 0);
    w_commit_i : in  std_logic;
    w_abort_i  : in  std_logic;
    r_empty_o  : out std_logic;
    r_pop_i    : in  std_logic;
    r_dat_o    : out std_logic_vector(g_width-1 downto 0)
    );
   end component;
  
end package;

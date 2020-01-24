library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

library work;
use work.wishbone_pkg.all;
use work.wr_fabric_pkg.all;
use work.eb_hdr_pkg.all;

entity eb_eth_rx is
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
end eb_eth_rx;

architecture rtl of eb_eth_rx is
  subtype t_index is unsigned(f_ceil_log2(g_mtu)-1 downto 0);
  
  type t_state is (S_DROP, S_WAIT, S_ETHERNET, S_IP, S_UDP, S_STB);
  
  -- Two bytes at a time
  constant c_step : natural := 2;
  
  signal r_state      : t_state;
  signal r_state_next : t_state;
  signal r_count      : t_index;
  
  signal r_ack    : std_logic;
  signal r_pass   : std_logic;
  signal r_shift  : std_logic_vector(c_ip_len*8-1 downto 0);
  signal r_stb    : std_logic;
  signal r_mac    : std_logic_vector(47 downto 0);
  signal r_ip     : std_logic_vector(31 downto 0);
  signal r_port   : std_logic_vector(15 downto 0);
  signal r_length : unsigned(15 downto 0);
  
  signal s_stall   : std_logic;
  signal s_eth_hdr : t_eth_hdr;
  signal s_ip_hdr  : t_ip_hdr;
  signal s_udp_hdr : t_udp_hdr;
  signal s_optlen  : unsigned(3 downto 0);
  signal s_shift  : std_logic_vector(c_ip_len*8-1 downto 0);
  
  function f_step(x : natural) return t_index is
  begin
    return to_unsigned(x/2 - 1, t_index'length);
  end function;
begin

  master_o.cyc <= snk_i.cyc;
  master_o.stb <= (r_pass and snk_i.stb) when snk_i.adr = c_WRF_DATA else '0';
  master_o.adr <= (others => '0');
  master_o.sel <= "0011";
  master_o.we  <= '1';
  master_o.dat(31 downto 16) <= (others => '0');
  master_o.dat(15 downto  0) <= snk_i.dat;
  
  snk_o.ack <= r_ack;
  snk_o.err <= '0';
  snk_o.rty <= '0';
  snk_o.stall <= s_stall;
  
  stb_o    <= r_stb;
  mac_o    <= r_mac;
  ip_o     <= r_ip;
  port_o   <= r_port;
  length_o <= r_length;
  
  s_stall <= r_stb when r_state = S_UDP else (r_pass and master_i.stall);
  
  -- These are valid when header is fully shifted in
  s_shift   <= r_shift(r_shift'left-16 downto 0) & snk_i.dat;
  s_eth_hdr <= f_parse_eth(s_shift);
  s_ip_hdr  <= f_parse_ip (s_shift);
  s_udp_hdr <= f_parse_udp(s_shift);
  
  s_optlen <= unsigned(s_ip_hdr.ihl)-5;

  fsm : process(clk_i, rst_n_i) is
  begin
    if rst_n_i = '0' then
      r_state      <= S_WAIT;
      r_state_next <= S_ETHERNET;
      r_count      <= f_step(c_eth_len);
      
      r_ack    <= '0';
      r_pass   <= '0';
      r_shift  <= (others => '0');
      r_stb    <= '0';
      r_mac    <= (others => '0');
      r_ip     <= (others => '0');
      r_port   <= (others => '0');
      r_length <= (others => '0');
    elsif rising_edge(clk_i) then
      r_ack   <= snk_i.cyc and snk_i.stb and not s_stall;
      r_stb   <= r_stb and stall_i;
      
      if snk_i.cyc = '0' then
        r_pass       <= '0';
        r_state      <= S_WAIT;
        r_state_next <= S_ETHERNET;
        r_count      <= f_step(c_eth_len);
      elsif snk_i.stb = '1' and s_stall = '0' and snk_i.adr = c_WRF_DATA then
      
        -- defaults
        r_state      <= S_WAIT;
        r_state_next <= S_DROP;
        r_count      <= r_count - 1;
        r_shift      <= s_shift;
        
        case r_state is
          -- special states
          when S_DROP => 
            r_state <= S_DROP;
            r_pass <= '0';
            
          when S_WAIT =>
            r_state_next <= r_state_next;
            if r_count = to_unsigned(1, r_count'length) then
              r_state <= r_state_next;
            end if;
          
          -- payload states
          when S_ETHERNET =>
            r_mac <= s_eth_hdr.src;
            
            r_count <= f_step(c_ip_len);
            if s_eth_hdr.typ = c_eth_typ_ip then
              r_state_next <= S_IP;
            else
              r_state <= S_DROP;
            end if;
          
          when S_IP =>
            r_ip <= s_ip_hdr.src;
            
            r_count <= f_step(to_integer(s_optlen)*4 + c_udp_len);
            if s_ip_hdr.ver = x"4" and unsigned(s_ip_hdr.ihl) >= 5 and s_ip_hdr.pro = c_ip_typ_udp then
              r_state_next <= S_UDP;
            else
              r_state <= S_DROP;
            end if;
          
          when S_UDP =>
            r_port   <= s_udp_hdr.src;
            r_length <= unsigned(s_udp_hdr.len);
            
            -- Raise stb four bytes after fsm starts (=> fsm will TX)
            r_count <= f_step(4);
            
            -- Enforce MTU and 4-byte alignment
            if unsigned(s_udp_hdr.len) >= 12 and 
               unsigned(s_udp_hdr.len) <= g_mtu-c_ip_len and
               s_udp_hdr.len(1 downto 0) = "00" then
              r_pass <= '1'; -- Feed future bytes to etherbone fsm
              r_state_next <= S_STB;
            else
              r_state <= S_DROP;
            end if;
          
          when S_STB =>
            r_stb   <= '1'; -- this must be 4 bytes after r_pass='1'
            r_count <= f_step(to_integer(r_length)-c_udp_len-4);
            r_state_next <= S_DROP;
          
        end case;
      end if;
    end if;
  end process;
  
end rtl;

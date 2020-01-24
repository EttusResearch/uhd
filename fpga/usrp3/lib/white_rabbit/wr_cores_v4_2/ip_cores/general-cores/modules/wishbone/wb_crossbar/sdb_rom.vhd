library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.wishbone_pkg.all;

entity sdb_rom is
  generic(
    g_layout   : t_sdb_record_array;
    g_masters  : natural;
    g_bus_end  : unsigned(63 downto 0);
    g_sdb_name : string := "WB4-Crossbar-GSI   ");
  port(
    clk_sys_i  : in  std_logic;
    master_i   : in  std_logic_vector(g_masters-1 downto 0);
    slave_i    : in  t_wishbone_slave_in;
    slave_o    : out t_wishbone_slave_out);
end sdb_rom;

architecture rtl of sdb_rom is
  alias c_layout : t_sdb_record_array(g_layout'length downto 1) is g_layout;

  -- The ROM must describe all slaves, the crossbar itself and the optional information records
  constant c_used_entries   : natural := c_layout'high + 1;
  constant c_rom_entries    : natural := 2**f_ceil_log2(c_used_entries); -- next power of 2
  constant c_sdb_words      : natural := c_sdb_device_length / c_wishbone_data_width;
  constant c_rom_words      : natural := c_rom_entries * c_sdb_words;
  constant c_rom_depth      : natural := f_ceil_log2(c_rom_words);
  constant c_rom_lowbits    : natural := f_ceil_log2(c_wishbone_data_width / 8);
  constant c_sdb_name       : string  := f_string_fix_len(g_sdb_name , 19, ' ', false);
  
  -- Index of the MSI entry in SDB
  type t_nat_array is array(g_masters-1 downto 0) of natural;
  function f_master_positions return t_nat_array is
    variable typ    : std_logic_vector(7 downto 0);
    variable result : t_nat_array;
    variable master : natural := 0;
  begin
    for rec in c_layout'low to c_layout'high loop
      typ := c_layout(rec)(7 downto 0);
      case typ is
        when x"03" | x"f3" =>
          assert master < g_masters
          report "Too many msi records found"
          severity failure;
        
          result(master) := rec;
          master := master + 1;
        
        when others => null;
      end case;
    end loop;
    
    if master = 0 then
      result := (others => 0);
    else
      assert master = g_masters
      report "Insufficient msi records found (" & Integer'image(master) & "/" & Integer'image(g_masters) & ")"
      severity failure;
    end if;
    return result;
  end f_master_positions;
  
  constant c_master_positions : t_nat_array := f_master_positions;
  constant c_msi              : boolean     := c_master_positions(0) /= 0;
  
  function f_msi_flag_index(y : std_logic_vector) return std_logic_vector is
    variable offset : unsigned(c_rom_depth-1 downto 0) := (others => '0');
    variable result : std_logic_vector(c_rom_depth-1 downto 0) := (others => '0');
  begin
    for i in c_master_positions'range loop
      if c_msi then
        offset := to_unsigned(c_master_positions(i)*16, offset'length);
      end if;
      for b in result'range loop
        result(b) := result(b) or (offset(b) and y(i));
      end loop;
    end loop;
    return result;
  end f_msi_flag_index;
  
  type t_rom is array(c_rom_words-1 downto 0) of t_wishbone_data;

  function f_build_rom
    return t_rom 
  is
    variable res : t_rom := (others => (others => '0'));
    variable sdb_record : t_sdb_record;
    variable sdb_component : t_sdb_component;
  begin
    sdb_record(511 downto 480) := x"5344422D"  ;                                     -- sdb_magic
    sdb_record(479 downto 464) := std_logic_vector(to_unsigned(c_used_entries, 16)); -- sdb_records
    sdb_record(463 downto 456) := x"01";                                             -- sdb_version
    sdb_record(455 downto 448) := x"00";                                             -- sdb_bus_type = sdb_wishbone
    sdb_record(  7 downto   0) := x"00";                                             -- record_type  = sdb_interconnect
    
    sdb_component.addr_first := (others => '0');
    sdb_component.addr_last  := std_logic_vector(g_bus_end);
    sdb_component.product.vendor_id := x"0000000000000651"; -- GSI
    sdb_component.product.device_id := x"e6a542c9";
    sdb_component.product.version   := x"00000003";
    sdb_component.product.date      := x"20120511";
    sdb_component.product.name      := c_sdb_name;
    sdb_record(447 downto   8) := f_sdb_embed_component(sdb_component, (others => '0'));
    
    for i in 0 to c_sdb_words-1 loop
      res(c_sdb_words-1-i) := 
        sdb_record((i+1)*c_wishbone_data_width-1 downto i*c_wishbone_data_width);
    end loop;
    
    for idx in c_layout'range loop
      sdb_record := c_layout(idx);
      
      -- All local/temporary types => empty record
      if sdb_record(7 downto 4) = x"f" then
        sdb_record(3 downto 0) := x"f";
      end if;
      
      for i in 0 to c_sdb_words-1 loop
        res((idx+1)*c_sdb_words-1-i) := 
          sdb_record((i+1)*c_wishbone_data_width-1 downto i*c_wishbone_data_width);
      end loop;
    end loop;
    
    return res;
  end f_build_rom;
  
  signal rom : t_rom := f_build_rom;

  signal s_adr : unsigned(c_rom_depth-1 downto 0);
  signal s_sel : unsigned(c_rom_depth-1 downto 0);
  
  signal r_rom  : t_wishbone_data;
  signal r_flag : t_wishbone_data;
  signal r_ack  : std_logic;

begin

  slave_o.dat <= r_rom or r_flag;
  slave_o.ack <= r_ack;
  slave_o.err   <= '0';
  slave_o.rty   <= '0';
  slave_o.stall <= '0';
  slave_o.int   <= '0'; -- Tom sucks! This should not be here.

  s_adr <= unsigned(slave_i.adr(c_rom_depth+c_rom_lowbits-1 downto c_rom_lowbits));
  s_sel <= unsigned(f_msi_flag_index(master_i));
  
  slave_clk : process(clk_sys_i)
  begin
    if (rising_edge(clk_sys_i)) then
      r_ack <= slave_i.cyc and slave_i.stb;
      r_rom  <= rom(to_integer(s_adr));
      r_flag <= (others => '0');
      if s_adr = s_sel and c_msi then
        r_flag(r_flag'high) <= '1';
      end if;
    end if;
  end process;
  
end rtl;

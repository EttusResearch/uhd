-------------------------------------------------------------------------------
-- Title      : An MxS Wishbone crossbar switch
-- Project    : General Cores Library (gencores)
-------------------------------------------------------------------------------
-- File       : xwb_crossbar.vhd
-- Author     : Wesley W. Terpstra
-- Company    : GSI
-- Created    : 2011-06-08
-- Last update: 2011-09-22
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description:
--
-- An MxS Wishbone crossbar switch
-- 
-- All masters, slaves, and the crossbar itself must share the same WB clock.
-- All participants must support the same data bus width. 
-- 
-- If a master raises STB_O with an address not mapped by the crossbar,
-- ERR_I will be raised. If two masters address the same slave
-- simultaneously, the lowest numbered master is granted access.
-- 
-- The implementation of this crossbar locks a master to a slave so long as
-- CYC_O is held high. 
-- 
-- Synthesis/timing relevant facts:
--   (m)asters, (s)laves, masked (a)ddress bits
--   
--   Area required       = O(ms log(ma))
--   Arbitration depth   = O(log(msa))
--   Master->Slave depth = O(log(m))
--   Slave->Master depth = O(log(s))
-- 
--   If g_registered = false, arbitration depth is added to M->S and S->M.
--
-------------------------------------------------------------------------------
-- Copyright (c) 2011 GSI / Wesley W. Terpstra
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author          Description
-- 2012-03-05  3.0      wterpstra       made address generic and check overlap
-- 2011-11-04  2.0      wterpstra       timing improvements
-- 2011-06-08  1.0      wterpstra       import from SVN
-------------------------------------------------------------------------------


library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.wishbone_pkg.all;

entity xwb_crossbar is
  generic(
    g_num_masters : integer := 2;
    g_num_slaves  : integer := 1;
    g_registered  : boolean := false;
    -- Address of the slaves connected
    g_address     : t_wishbone_address_array;
    g_mask        : t_wishbone_address_array);
  port(
    clk_sys_i     : in  std_logic;
    rst_n_i       : in  std_logic;
    -- Master connections (INTERCON is a slave)
    slave_i       : in  t_wishbone_slave_in_array(g_num_masters-1 downto 0);
    slave_o       : out t_wishbone_slave_out_array(g_num_masters-1 downto 0);
    -- Slave connections (INTERCON is a master)
    master_i      : in  t_wishbone_master_in_array(g_num_slaves-1 downto 0);
    master_o      : out t_wishbone_master_out_array(g_num_slaves-1 downto 0);
    -- Master granted access to SDB for use by MSI crossbar (please ignore it)
    sdb_sel_o     : out std_logic_vector(g_num_masters-1 downto 0));
end xwb_crossbar;

architecture rtl of xwb_crossbar is
  alias c_address : t_wishbone_address_array(g_num_slaves-1 downto 0) is g_address;
  alias c_mask    : t_wishbone_address_array(g_num_slaves-1 downto 0) is g_mask;

  -- Confirm that no address ranges overlap
  function f_ranges_ok
    return boolean
  is
    constant zero  : t_wishbone_address := (others => '0');
    constant align : std_logic_vector(f_ceil_log2(c_wishbone_data_width)-4 downto 0) := (others => '0');
  begin
    -- all (i,j) with 0 <= i < j < n
    if g_num_slaves > 1 then
      for i in 0 to g_num_slaves-2 loop
        for j in i+1 to g_num_slaves-1 loop
          assert not (((c_mask(i) and c_mask(j)) and (c_address(i) xor c_address(j))) = zero) or
                 ((c_mask(i) or not c_address(i)) = zero) or -- disconnected slave?
                 ((c_mask(j) or not c_address(j)) = zero)    -- disconnected slave?
          report "Address ranges must be distinct (slaves " & 
                 Integer'image(i) & "[" & f_bits2string(c_address(i)) & "/" &
                                          f_bits2string(c_mask(i)) & "] & " & 
                 Integer'image(j) & "[" & f_bits2string(c_address(j)) & "/" &
                                          f_bits2string(c_mask(j)) & "])"
          severity Failure;
        end loop;
      end loop;
    end if;
    for i in 0 to g_num_slaves-1 loop
      assert (c_address(i) and not c_mask(i)) = zero or -- at least 1 bit outside mask
             (not c_address(i) or c_mask(i))  = zero    -- all bits outside mask (= disconnected)
      report "Address bits not in mask; slave #" & 
             Integer'image(i) & "[" & f_bits2string(c_address(i)) & "/" &
                                      f_bits2string(c_mask(i)) & "]"
      severity Failure;
      
      assert c_mask(i)(align'range) = align
      report "Address space smaller than a wishbone register; slave #" & 
             Integer'image(i) & "[" & f_bits2string(c_address(i)) & "/" &
                                      f_bits2string(c_mask(i)) & "]"
      severity Failure;
      
      -- Working case
      report "Mapping slave #" & 
             Integer'image(i) & "[" & f_bits2string(c_address(i)) & "/" &
                                      f_bits2string(c_mask(i)) & "]"
      severity Note;
    end loop;
    return true;
  end f_ranges_ok;
  constant c_ok : boolean := f_ranges_ok;

  -- Crossbar connection matrix
  type matrix is array (g_num_masters-1 downto 0, g_num_slaves downto 0) of std_logic;
  
  -- Add an 'error' device to the list of slaves
  signal master_ie : t_wishbone_master_in_array(g_num_slaves downto 0);
  signal master_oe : t_wishbone_master_out_array(g_num_slaves downto 0);
  signal virtual_ERR : std_logic;
  
  signal matrix_old : matrix; -- Registered connection matrix
  signal matrix_new : matrix; -- The new values of the matrix

  -- Either matrix_old or matrix_new, depending on g_registered
  signal granted : matrix;
  
  -- 1 => 0    2 => 1    3..4 => 2     5..8 => 3
  function log2(i : natural) return natural is
  begin
    if i <= 1
    then return 0;
    else return log2((i+1)/2) + 1;
    end if;
  end log2;
  
  -- 0 => 1    1 => 2      2 => 4      3 => 8
  function pow2(i : natural) return natural is
  begin
    if i = 0
    then return 1;
    else return pow2(i-1)*2;
    end if;
  end pow2;
  
  -- If any of the bits are '1', the whole thing is '1'
  -- This function makes the check explicitly have logarithmic depth.
  function vector_OR(x : std_logic_vector)
    return std_logic 
  is
    constant len : integer := x'length;
    constant mid : integer := len / 2;
    alias y : std_logic_vector(len-1 downto 0) is x;
  begin
    if len = 1 
    then return y(0);
    else return vector_OR(y(len-1 downto mid)) or
                vector_OR(y(mid-1 downto 0));
    end if;
  end vector_OR;
  
  -- Kogge-Stone network of ORs.
  -- A log(n) deep, n-wide circuit where:
  --   output(i) = OR_{j<=i} input(j)
  function ks_OR(input : std_logic_vector)
    return std_logic_vector
  is
    constant width  : natural := input'length;
    constant stages : natural := log2(width);
    variable prev   : std_logic_vector(width-1 downto 0);
    variable output : std_logic_vector(width-1 downto 0);
  begin
    prev := input;
    if stages = 0 then
      output := prev;
    else
      for l in 0 to stages-1 loop
        for i in 0 to width-1 loop
          if i >= pow2(l)
          then output(i) := prev(i) or prev(i-pow2(l));
          else output(i) := prev(i);
          end if;
        end loop;
        prev := output;
      end loop;
    end if;
    return output;
  end ks_OR;
  
  -- Impure because it accesses c_{address, mask}
  function matrix_logic(
    matrix_old : matrix;
    slave_i    : t_wishbone_slave_in_array(g_num_masters-1 downto 0))
    return matrix
  is
    subtype row    is std_logic_vector(g_num_masters-1 downto 0);
    subtype column is std_logic_vector(g_num_slaves    downto 0);
    
    variable tmp        : std_logic;
    variable tmp_column : column;
    variable tmp_row    : row;
    
    variable request    : matrix;  -- Which slaves do the masters address log(S) 
    variable selected   : matrix;  -- Which master wins arbitration  log(M) request
    variable sbusy      : column;  -- Does the slave's  previous connection persist?
    variable mbusy      : row;     -- Does the master's previous connection persist?
    variable matrix_new : matrix;
  begin
    -- A slave is busy iff it services an in-progress cycle
    for slave in g_num_slaves downto 0 loop
      for master in g_num_masters-1 downto 0 loop
        tmp_row(master) := matrix_old(master, slave) and slave_i(master).CYC;
      end loop;
      sbusy(slave) := vector_OR(tmp_row);
    end loop;
    
    -- A master is busy iff it services an in-progress cycle
    for master in g_num_masters-1 downto 0 loop
      for slave in g_num_slaves downto 0 loop
        tmp_column(slave) := matrix_old(master, slave);
      end loop;
      mbusy(master) := vector_OR(tmp_column) and slave_i(master).CYC;
    end loop;

    -- Decode the request address to see if master wants access
    for master in g_num_masters-1 downto 0 loop
      for slave in g_num_slaves-1 downto 0 loop
        tmp := not vector_OR((slave_i(master).ADR and c_mask(slave)) xor c_address(slave));
        tmp_column(slave) := tmp;
        request(master, slave) := slave_i(master).CYC and slave_i(master).STB and tmp;
      end loop;
      tmp_column(g_num_slaves) := '0';
      -- If no slaves match request, bind to 'error device'
      request(master, g_num_slaves) := slave_i(master).CYC and slave_i(master).STB and not vector_OR(tmp_column);
    end loop;

    -- Arbitrate among the requesting masters
    -- Policy: lowest numbered master first
    for slave in g_num_slaves downto 0 loop
      -- OR together all the requests by higher priority masters
      for master in 0 to g_num_masters-1 loop
        tmp_row(master) := request(master, slave);
      end loop;
      tmp_row := ks_OR(tmp_row);
      
      -- Grant to highest priority master
      selected(0, slave) := request(0, slave); -- master 0 always wins
      if g_num_masters > 1 then
        for master in 1 to g_num_masters-1 loop
          selected(master, slave) := -- only if requested and no lower requests
            not tmp_row(master-1) and request(master, slave);
        end loop;
      end if;
    end loop;

    -- Determine the master granted access
    -- Policy: if cycle still in progress, preserve the previous choice
    for slave in g_num_slaves downto 0 loop
      for master in g_num_masters-1 downto 0 loop
        if sbusy(slave) = '1' or mbusy(master) = '1' then
          matrix_new(master, slave) := matrix_old(master, slave);
        else
          matrix_new(master, slave) := selected(master, slave);
        end if;
      end loop;
    end loop;
    
    return matrix_new;
  end matrix_logic;

  subtype slave_row is std_logic_vector(g_num_masters-1 downto 0);
  type slave_matrix is array (natural range <>) of slave_row;
  
  function slave_matrix_OR(x : slave_matrix)
    return std_logic_vector is
    variable result : std_logic_vector(x'LENGTH-1 downto 0);
  begin
    for i in x'LENGTH-1 downto 0 loop
      result(i) := vector_OR(x(i));
    end loop;
    return result;
  end slave_matrix_OR;
  
  -- Select the master pins the slave will receive
  function slave_logic(slave   : integer;
                       granted : matrix;
                       slave_i : t_wishbone_slave_in_array(g_num_masters-1 downto 0))
    return t_wishbone_master_out
  is
    variable CYC_row    : slave_row;
    variable STB_row    : slave_row;
    variable ADR_matrix : slave_matrix(c_wishbone_address_width-1 downto 0);
    variable SEL_matrix : slave_matrix((c_wishbone_address_width/8)-1 downto 0);
    variable WE_row     : slave_row;
    variable DAT_matrix : slave_matrix(c_wishbone_data_width-1 downto 0);
  begin
    -- Rename all the signals ready for big_or
    for master in g_num_masters-1 downto 0 loop
      CYC_row(master) := slave_i(master).CYC and granted(master, slave);
      STB_row(master) := slave_i(master).STB and granted(master, slave);
      for bit in c_wishbone_address_width-1 downto 0 loop
        ADR_matrix(bit)(master) := slave_i(master).ADR(bit) and granted(master, slave);
      end loop;
      for bit in (c_wishbone_address_width/8)-1 downto 0 loop
        SEL_matrix(bit)(master) := slave_i(master).SEL(bit) and granted(master, slave);
      end loop;
      WE_row(master) := slave_i(master).WE and granted(master, slave);
      for bit in c_wishbone_data_width-1 downto 0 loop
        DAT_matrix(bit)(master) := slave_i(master).DAT(bit) and granted(master, slave);
      end loop;
    end loop;
    
    return (
       CYC => vector_OR(CYC_row),
       STB => vector_OR(STB_row),
       ADR => slave_matrix_OR(ADR_matrix),
       SEL => slave_matrix_OR(SEL_matrix),
       WE  => vector_OR(WE_row),
       DAT => slave_matrix_OR(DAT_matrix));
  end slave_logic;

  subtype master_row is std_logic_vector(g_num_slaves downto 0);
  type master_matrix is array (natural range <>) of master_row;
  
  function master_matrix_OR(x : master_matrix)
    return std_logic_vector is
    variable result : std_logic_vector(x'LENGTH-1 downto 0);
  begin
    for i in x'LENGTH-1 downto 0 loop
      result(i) := vector_OR(x(i));
    end loop;
    return result;
  end master_matrix_OR;
  
  -- Select the slave pins the master will receive
  function master_logic(master    : integer;
                        granted   : matrix;
                        master_ie : t_wishbone_master_in_array(g_num_slaves downto 0))
    return t_wishbone_slave_out
  is
    variable ACK_row    : master_row;
    variable ERR_row    : master_row;
    variable RTY_row    : master_row;
    variable STALL_row  : master_row;
    variable DAT_matrix : master_matrix(c_wishbone_data_width-1 downto 0);
  begin
    -- We use inverted logic on STALL so that if no slave granted => stall
    for slave in g_num_slaves downto 0 loop
      ACK_row(slave) := master_ie(slave).ACK and granted(master, slave);
      ERR_row(slave) := master_ie(slave).ERR and granted(master, slave);
      RTY_row(slave) := master_ie(slave).RTY and granted(master, slave);
      STALL_row(slave) := not master_ie(slave).STALL and granted(master, slave);
      for bit in c_wishbone_data_width-1 downto 0 loop
        DAT_matrix(bit)(slave) := master_ie(slave).DAT(bit) and granted(master, slave);
      end loop;
    end loop;
    
    return (
      ACK => vector_OR(ACK_row),
      ERR => vector_OR(ERR_row),
      RTY => vector_OR(RTY_row),
      STALL => not vector_OR(STALL_row),
      DAT => master_matrix_OR(DAT_matrix),
      INT => '0');
  end master_logic;
begin
  -- The virtual error slave is pretty straight-forward:
  master_o <= master_oe(g_num_slaves-1 downto 0);
  master_ie(g_num_slaves-1 downto 0) <= master_i;
  
  master_ie(g_num_slaves) <= (
    ACK   => '0', 
    ERR   => virtual_ERR,
    RTY   => '0', 
    STALL => '0',
    DAT   => (others => '0'),
    INT   => '0');
  virtual_error_slave : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      virtual_ERR <= master_oe(g_num_slaves).CYC and master_oe(g_num_slaves).STB;
    end if;
  end process virtual_error_slave;
  
  -- Copy the matrix to a register:
  matrix_new <= matrix_logic(matrix_old, slave_i);
  main : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        matrix_old <= (others => (others => '0'));
      else
        matrix_old <= matrix_new;
      end if;
    end if;
  end process main;
  
  -- Is the crossbar combinatorial or registered
  granted <= matrix_old when g_registered else matrix_new;

  -- Make the slave connections
  slave_matrixs : for slave in g_num_slaves downto 0 generate
    master_oe(slave) <= slave_logic(slave, granted, slave_i);
  end generate;

  -- Make the master connections
  master_matrixs : for master in g_num_masters-1 downto 0 generate
    slave_o(master) <= master_logic(master, granted, master_ie);
  end generate;
  
  -- Tell SDB which master is accessing it (SDB is last slave)
  sdb_masters : for master in g_num_masters-1 downto 0 generate
    sdb_sel_o(master) <= granted(master, g_num_slaves-1);
  end generate;

end rtl;

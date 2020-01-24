library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library std;
use std.textio.all;

library work;
use work.genram_pkg.all;

package memory_loader_pkg is

  subtype t_meminit_array is t_generic_ram_init;

  impure function f_load_mem_from_file
    (file_name : in string;
     mem_size  : in integer;
     mem_width : in integer;
     fail_if_notfound : boolean)
    return t_meminit_array;

  impure function f_load_mem32_from_file
    (file_name : in string; mem_size  : in integer; fail_if_notfound : boolean)
    return t_ram32_type;

  impure function f_load_mem16_from_file
    (file_name : in string; mem_size  : in integer; fail_if_notfound : boolean)
    return t_ram16_type;

  impure function f_load_mem8_from_file
    (file_name : in string; mem_size  : in integer; fail_if_notfound : boolean)
    return t_ram8_type;

  impure function f_load_mem32_from_file_split
    (file_name        : in string; mem_size : in integer;
     fail_if_notfound : boolean; byte_idx : in integer)
    return t_ram8_type;

end memory_loader_pkg;

package body memory_loader_pkg is

  impure function f_load_mem_from_file
    (file_name        : in string;
     mem_size         : in integer;
     mem_width        : in integer;
     fail_if_notfound : boolean)
    return t_meminit_array is

    FILE f_in  : text;
    variable l : line;
    variable tmp_bv : bit_vector(mem_width-1 downto 0);
    variable tmp_sv : std_logic_vector(mem_width-1 downto 0);
    variable mem: t_meminit_array(0 to mem_size-1, mem_width-1 downto 0);
    variable status   : file_open_status;
  begin
    if(file_name = "" or file_name = "none") then
      mem:= (others => (others => '0'));
      return mem;
    end if;

    file_open(status, f_in, file_name, read_mode);

    if(status /= open_ok) then
      if(fail_if_notfound) then
        report "f_load_mem_from_file(): can't open file '"&file_name&"'" severity failure;
      else
        report "f_load_mem_from_file(): can't open file '"&file_name&"'" severity warning;
      end if;
    end if;

    for I in 0 to mem_size-1 loop
      readline (f_in, l);
      -- read function gives us bit_vector
      read (l, tmp_bv);
      tmp_sv := to_stdlogicvector(tmp_bv);
      for J in 0 to mem_width-1 loop
        mem(i, j) := tmp_sv(j);
      end loop;
    end loop;

    file_close(f_in);
    return mem;
  end f_load_mem_from_file;

  -------------------------------------------------------------------
  -- RAM initialization for most common sizes to speed-up synthesis
  -------------------------------------------------------------------

  impure function f_load_mem32_from_file
    (file_name        : in string;
     mem_size         : in integer;
     fail_if_notfound : boolean)
    return t_ram32_type is

    FILE f_in  : text;
    variable l : line;
    variable tmp_bv : bit_vector(31 downto 0);
    variable mem: t_ram32_type(0 to mem_size-1);
    variable status   : file_open_status;
  begin
    if(file_name = "" or file_name = "none") then
      mem:= (others => (others => '0'));
      return mem;
    end if;

    file_open(status, f_in, file_name, read_mode);

    if(status /= open_ok) then
      if(fail_if_notfound) then
        report "f_load_mem_from_file(): can't open file '"&file_name&"'" severity failure;
      else
        report "f_load_mem_from_file(): can't open file '"&file_name&"'" severity warning;
      end if;
    end if;

    for I in 0 to mem_size-1 loop
      readline (f_in, l);
      -- read function gives us bit_vector
      read (l, tmp_bv);
      mem(I) := to_stdlogicvector(tmp_bv);
    end loop;

    file_close(f_in);
    return mem;
  end f_load_mem32_from_file;

  -------------------------------------------------------------------

  impure function f_load_mem16_from_file
    (file_name        : in string;
     mem_size         : in integer;
     fail_if_notfound : boolean)
    return t_ram16_type is

    FILE f_in  : text;
    variable l : line;
    variable tmp_bv : bit_vector(15 downto 0);
    variable mem: t_ram16_type(0 to mem_size-1);
    variable status   : file_open_status;
  begin
    if(file_name = "" or file_name = "none") then
      mem:= (others => (others => '0'));
      return mem;
    end if;

    file_open(status, f_in, file_name, read_mode);

    if(status /= open_ok) then
      if(fail_if_notfound) then
        report "f_load_mem_from_file(): can't open file '"&file_name&"'" severity failure;
      else
        report "f_load_mem_from_file(): can't open file '"&file_name&"'" severity warning;
      end if;
    end if;

    for I in 0 to mem_size-1 loop
      readline (f_in, l);
      -- read function gives us bit_vector
      read (l, tmp_bv);
      mem(I) := to_stdlogicvector(tmp_bv);
    end loop;

    file_close(f_in);
    return mem;
  end f_load_mem16_from_file;

  -------------------------------------------------------------------

  impure function f_load_mem8_from_file
    (file_name        : in string;
     mem_size         : in integer;
     fail_if_notfound : boolean)
    return t_ram8_type is

    FILE f_in  : text;
    variable l : line;
    variable tmp_bv : bit_vector(7 downto 0);
    variable mem: t_ram8_type(0 to mem_size-1);
    variable status   : file_open_status;
  begin
    if(file_name = "" or file_name = "none") then
      mem:= (others => (others => '0'));
      return mem;
    end if;

    file_open(status, f_in, file_name, read_mode);

    if(status /= open_ok) then
      if(fail_if_notfound) then
        report "f_load_mem_from_file(): can't open file '"&file_name&"'" severity failure;
      else
        report "f_load_mem_from_file(): can't open file '"&file_name&"'" severity warning;
      end if;
    end if;

    for I in 0 to mem_size-1 loop
      readline (f_in, l);
      -- read function gives us bit_vector
      read (l, tmp_bv);
      mem(I) := to_stdlogicvector(tmp_bv);
    end loop;

    file_close(f_in);
    return mem;
  end f_load_mem8_from_file;

  -------------------------------------------------------------------
  -- initialization for 32-bit RAM split into 4x 8-bit BRAM
  -------------------------------------------------------------------

  impure function f_load_mem32_from_file_split
    (file_name        : in string;
     mem_size         : in integer;
     fail_if_notfound : boolean;
     byte_idx         : in integer)
    return t_ram8_type is

    FILE f_in  : text;
    variable l : line;
    variable tmp_bv : bit_vector(31 downto 0);
    variable mem: t_ram8_type(0 to mem_size-1);
    variable status   : file_open_status;
  begin
    if(file_name = "" or file_name = "none") then
      mem:= (others => (others => '0'));
      return mem;
    end if;

    file_open(status, f_in, file_name, read_mode);

    if(status /= open_ok) then
      if(fail_if_notfound) then
        report "f_load_mem_from_file(): can't open file '"&file_name&"'" severity failure;
      else
        report "f_load_mem_from_file(): can't open file '"&file_name&"'" severity warning;
      end if;
    end if;

    for I in 0 to mem_size-1 loop
      readline (f_in, l);
      -- read function gives us bit_vector
      read (l, tmp_bv);
      mem(I) := to_stdlogicvector( tmp_bv((byte_idx+1)*8-1 downto byte_idx*8) );
    end loop;

    file_close(f_in);
    return mem;
  end f_load_mem32_from_file_split;

end memory_loader_pkg;

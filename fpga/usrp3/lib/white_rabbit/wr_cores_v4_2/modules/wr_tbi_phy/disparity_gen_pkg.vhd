-- 8b10b disparity generator, based on 8b10b encoder core (c) Mathias Kreider

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

package disparity_gen_pkg is

  type t_8b10b_disparity is (RD_PLUS, RD_MINUS);
  
  function f_next_8b10b_disparity8 (cur_disp : t_8b10b_disparity;
                                    ctrl     : std_logic;
                                    data     : std_logic_vector)
    return t_8b10b_disparity;

  function f_next_8b10b_disparity16 (
    cur_disp : t_8b10b_disparity;
    ctrl     : std_logic_vector;
    data     : std_logic_vector)
    return t_8b10b_disparity;

  function to_std_logic(t : t_8b10b_disparity) return std_logic;

  

end disparity_gen_pkg;

package body disparity_gen_pkg is

  function f_next_8b10b_disparity8(cur_disp : t_8b10b_disparity;
                                   ctrl     : std_logic;
                                   data     : std_logic_vector) return t_8b10b_disparity is

    constant c_disPar_6b : std_logic_vector(0 to 31) := ("11101000100000011000000110010111");

    constant c_disPar_4b    : std_logic_vector(0 to 7) := ("10001001");
    variable dp4bit, dp6bit : std_logic;
    variable new_disp       : t_8b10b_disparity;
  begin
-- use 3bit at 7-5 as index for 4bit code and disparity table \n

    dp4bit := c_disPar_4b(to_integer(unsigned(data(7 downto 5))));
    dp6bit := c_disPar_6b(to_integer(unsigned(data(4 downto 0))));

    new_disp := cur_disp;

    case cur_disp is
      when RD_MINUS =>
        if (ctrl xor dp6bit xor dp4bit) /= '0' then
          new_disp := RD_PLUS;
        end if;

      when RD_PLUS =>
        if (ctrl xor dp6bit xor dP4bit) /= '0' then
          new_disp := RD_MINUS;
        end if;
    end case;

    if ( data(1 downto 0) /= "00" and ctrl = '1') then
      new_disp := cur_disp;
    end if;
    
    return new_disp;
  end f_next_8b10b_disparity8;

  function f_next_8b10b_disparity16(cur_disp : t_8b10b_disparity;
                                    ctrl     : std_logic_vector;
                                    data     : std_logic_vector) return t_8b10b_disparity is

    variable tmp : t_8b10b_disparity;
    variable msb : std_logic_vector(7 downto 0);
  begin

    msb := data(15 downto 8);
    tmp := f_next_8b10b_disparity8(cur_disp, ctrl(1), msb);
    tmp := f_next_8b10b_disparity8(tmp, ctrl(0), data(7 downto 0));
    return tmp;
  end f_next_8b10b_disparity16;

  function to_std_logic(t : t_8b10b_disparity) return std_logic is
  begin
    if(t = RD_MINUS) then
      return '0';
    else
      return '1';
    end if;
  end to_std_logic;

end package body;


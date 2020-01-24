library ieee;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

package softpll_pkg is

  constant c_softpll_max_aux_clocks : integer := 8;

  type t_softpll_phase_detector_type is (CH_DDMTD, CH_BANGBANG);

  type t_softpll_channel_config_array is array(0 to c_softpll_max_aux_clocks-1) of t_softpll_phase_detector_type;

  constant c_softpll_default_channel_config : t_softpll_channel_config_array := (others => CH_DDMTD);

  -- External 10 MHz input divider parameters. 
  constant c_softpll_ext_div_ref     : integer := 8;
  constant c_softpll_ext_div_fb      : integer := 50;
  constant c_softpll_ext_log2_gating : integer := 13;

  constant c_softpll_out_status_off      : std_logic_vector(3 downto 0) := "0000";
  constant c_softpll_out_status_locking  : std_logic_vector(3 downto 0) := "0001";
  constant c_softpll_out_status_locked   : std_logic_vector(3 downto 0) := "0010";
  constant c_softpll_out_status_aligning : std_logic_vector(3 downto 0) := "0011";
  constant c_softpll_out_status_holdover : std_logic_vector(3 downto 0) := "0100";
  
end package;

package body softpll_pkg is

end softpll_pkg;

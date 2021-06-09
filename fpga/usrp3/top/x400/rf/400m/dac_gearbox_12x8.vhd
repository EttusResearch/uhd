--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: dac_gearbox_12x8
--
-- Description:
--
--   Gearbox to expand the data width from 12 SPC to 8 SPC.
--   Input Clocks, all aligned to one another and coming from same MMCM.
--     PLL reference clock = 61.44 or 62.5 MHz.
--     RfClk: 184.32 or 187.5 MHz (3x PLL reference clock)
--     Clk1x: 122.88 or 125 MHz (2x PLL reference clock)
--     Clk2x: 245.76 or 250 MHz (4x PLL reference clock)
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

entity dac_gearbox_12x8 is
  port(
    Clk1x            : in  std_logic;
    RfClk            : in  std_logic;
    ac1Reset_n       : in  std_logic;
    arReset_n        : in  std_logic;
    -- Data packing: [Q11,I11,Q10,I10,...,Q3,I3,Q2,I2,Q1,I1,Q0,I0] (I in LSBs)
    c1DataIn         : in  std_logic_vector(383 downto 0);
    c1DataValidIn    : in  std_logic;
    -- Data packing: [Q7,I7,Q6,I6,...,Q3,I3,Q2,I2,Q1,I1,Q0,I0] (I in LSBs)
    rDataOut         : out std_logic_vector(255 downto 0) := (others => '0');
    rReadyForOutput  : in  std_logic;
    rDataValidOut    : out std_logic
  );
end dac_gearbox_12x8;

architecture RTL of dac_gearbox_12x8 is

  constant kDataWidth   : natural := 16;

  constant kDataI0Lsb : natural := 0;
  constant kDataI0Msb : natural := kDataWidth-1;
  constant kDataQ0Lsb : natural := kDataI0Msb+1;
  constant kDataQ0Msb : natural := kDataQ0Lsb+kDataWidth-1;
  constant kDataI1Lsb : natural := kDataQ0Msb+1;
  constant kDataI1Msb : natural := kDataI1Lsb+kDataWidth-1;
  constant kDataQ1Lsb : natural := kDataI1Msb+1;
  constant kDataQ1Msb : natural := kDataQ1Lsb+kDataWidth-1;
  constant kDataI2Lsb : natural := kDataQ1Msb+1;
  constant kDataI2Msb : natural := kDataI2Lsb+kDataWidth-1;
  constant kDataQ2Lsb : natural := kDataI2Msb+1;
  constant kDataQ2Msb : natural := kDataQ2Lsb+kDataWidth-1;
  constant kDataI3Lsb : natural := kDataQ2Msb+1;
  constant kDataI3Msb : natural := kDataI3Lsb+kDataWidth-1;
  constant kDataQ3Lsb : natural := kDataI3Msb+1;
  constant kDataQ3Msb : natural := kDataQ3Lsb+kDataWidth-1;
  constant kDataI4Lsb : natural := kDataQ3Msb+1;
  constant kDataI4Msb : natural := kDataI4Lsb+kDataWidth-1;
  constant kDataQ4Lsb : natural := kDataI4Msb+1;
  constant kDataQ4Msb : natural := kDataQ4Lsb+kDataWidth-1;
  constant kDataI5Lsb : natural := kDataQ4Msb+1;
  constant kDataI5Msb : natural := kDataI5Lsb+kDataWidth-1;
  constant kDataQ5Lsb : natural := kDataI5Msb+1;
  constant kDataQ5Msb : natural := kDataQ5Lsb+kDataWidth-1;
  constant kDataI6Lsb : natural := kDataQ5Msb+1;
  constant kDataI6Msb : natural := kDataI6Lsb+kDataWidth-1;
  constant kDataQ6Lsb : natural := kDataI6Msb+1;
  constant kDataQ6Msb : natural := kDataQ6Lsb+kDataWidth-1;
  constant kDataI7Lsb : natural := kDataQ6Msb+1;
  constant kDataI7Msb : natural := kDataI7Lsb+kDataWidth-1;
  constant kDataQ7Lsb : natural := kDataI7Msb+1;
  constant kDataQ7Msb : natural := kDataQ7Lsb+kDataWidth-1;

  subtype Word_t is std_logic_vector(383 downto 0);
  type Words_t is array(natural range<>) of Word_t;

  signal rDataInDly : Words_t(3 downto 0);

  signal rDataValidDly : std_logic_vector(3 downto 0) := (others => '0');

  signal c1PhaseCount, c1DataValidInDly : std_logic := '0';
  signal rPhaseShiftReg : std_logic_vector(2 downto 0);

begin

  -----------------------------------------------------------------------------
  -- Data Packing 12 SPC to 8 SPC
  -----------------------------------------------------------------------------

  Clk1xDataCount: process(ac1Reset_n, Clk1x)
  begin
    if ac1Reset_n = '0' then
      c1PhaseCount     <= '0';
      c1DataValidInDly <= '0';
    elsif rising_edge(Clk1x) then
      c1DataValidInDly <= c1DataValidIn;
      c1PhaseCount <= (not c1PhaseCount) and (c1DataValidIn or c1DataValidInDly);
    end if;
  end process;

  DataClkCrossing: process(RfClk)
  begin
    if rising_edge(RfClk) then
      rDataInDly <= rDataInDly(rDataInDly'high-1 downto 0) & c1DataIn;
    end if;
  end process;

  -- Store clock phase information in a shift register. The shift register
  -- is a 3 bit register and it used in output data packer.
  PhaseClkCrossing: process(arReset_n,RfClk)
  begin
    if arReset_n = '0' then
      rPhaseShiftReg <= (others => '0');
    elsif rising_edge(RfClk) then
      rPhaseShiftReg(2 downto 1) <= rPhaseShiftReg(1 downto 0);
      rPhaseShiftReg(0)  <= c1PhaseCount;
    end if;
  end process;

  -----------------------------------------------------------------------------
  --
  -- Timing diagram: Data valid is asserted when both clock are edge aligned.
  --
  --                    |                       |                       |
  --                    v <-Clocks edge aligned v                       v
  -- Clk1x       ¯¯\____/¯¯¯¯¯\_____/¯¯¯¯¯\_____/¯¯¯¯¯\_____/¯¯¯¯¯\_____/¯¯¯¯¯\___
  --                                                    |
  --                                                    v <- O/p data valid assertion
  -- RfClk       ¯¯¯\___/¯¯¯\___/¯¯¯\___/¯¯¯\___/¯¯¯\___/¯¯¯\___/¯¯¯\___/¯¯¯\___/¯
  --                                    |       |       |
  -- c1DataValid _/¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|¯¯¯¯¯¯¯|¯¯¯¯¯¯¯|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
  --                                    |       |       |
  -- c1DValidDly _________/¯¯¯¯¯¯¯¯¯¯¯¯¯|¯¯¯¯¯¯¯|¯¯¯¯¯¯¯|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
  --                                    |       |       |
  -- c1PhaseCount  _______/¯¯¯¯¯¯¯¯¯¯¯¯\|_______|__/¯¯¯¯|¯¯¯¯¯¯¯\__________/¯¯
  --                                    |       |       |
  --                                    v <- rPhaseSR= "001"
  -- rPhaseSR(0) ________________/¯¯¯¯¯¯¯¯\_____|_______|_/¯¯¯¯¯¯¯\_________________
  --                                            |       |
  --                                            v <- rPhaseSR= "010"
  -- rPhaseSR(1) _________________________/¯¯¯¯¯¯¯¯\____|__________/¯¯¯¯¯¯¯\____________
  --                                                    |
  --                                                    v <- rPhaseSR= "100"
  -- rPhaseSR(2) __________________________________/¯¯¯¯¯¯¯¯\_______________/¯¯¯¯¯¯¯\___
  --
  -- rDValidDly0 _________________/¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
  --
  -- rDValidDly1 _________________________/¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
  --
  -- rDValidDly2 __________________________________/¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
  --
  -- In this design use a single bit counter on the input clock (Clk1x) domain
  -- and pass it to the RfClk domain. When data valid is asserted when both
  -- clocks are rising edge aligned, only one bit in rPhaseSR high, the
  -- remaining bits are zero. We use the position of the bit counter in the
  -- shift register to do data packing.
  --
  --
  -- Timing diagram: When data valid is asserted when both clock are NOT edge
  -- aligned.
  --
  --                    |                       |                       |
  --                    v <-Clocks edge aligned v                       v
  -- Clk1x       ¯¯\____/¯¯¯¯¯\_____/¯¯¯¯¯\_____/¯¯¯¯¯\_____/¯¯¯¯¯\_____/¯¯¯¯¯\___
  --                                                    |
  --                                                    v <- O/p data valid assertion
  -- RfClk       ¯¯¯\___/¯¯¯\___/¯¯¯\___/¯¯¯\___/¯¯¯\___/¯¯¯\___/¯¯¯\___/¯¯¯\___/¯
  --                                            |       |       |       |
  -- c1DataValid ________/¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|¯¯¯¯¯¯¯|¯¯¯¯¯¯¯|¯¯¯¯¯¯¯|¯¯¯¯¯¯¯¯¯¯¯¯¯¯
  --                                            |       |       |       |
  -- c1DValidDly  ___________________/¯¯¯¯¯¯¯¯¯¯|¯¯¯¯¯¯¯|¯¯¯¯¯¯¯|¯¯¯¯¯¯¯|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
  --                                            |       |       |       |
  -- c1PhaseCount ___________________/¯¯¯¯¯¯¯¯¯¯|¯\_____|_____/¯|¯¯¯¯¯¯¯|¯¯\__________/¯¯
  --                                            |       |       |       |
  --                                            v <- rPhaseSR= "001"    |
  -- rPhaseSR(0)         ________________/¯¯¯¯¯¯¯¯¯¯¯¯¯¯|¯\_____|_/¯¯¯¯¯|¯¯¯¯¯¯¯¯¯¯
  --                                                    |       |       |
  --                                                    v <- rPhaseSR= "011"
  -- rPhaseSR(1)                 ________________/¯¯¯¯¯¯¯¯¯¯¯¯¯¯|¯\_____|_/¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
  --                                                            |       |
  --                                                            v <- rPhaseSR= "110"
  -- rPhaseSR(2)                          ________________/¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯\_______/¯¯¯¯¯¯¯¯¯¯
  --                                                                    ^
  --                                                                    | <- rPhaseSR= "101"
  --
  -- rDValidDly0         _________________/¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
  --
  -- rDValidDly1          _________________________/¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
  --
  -- The above timing diagram is when input data valid is asserted when both
  -- clocks rising edges are not aligned. In this case the more than one bit in
  -- rPhaseSR is asserted which is unique to this case. As mentioned in the
  -- above case, we use rPhaseSR value to determine data packing.

  -- Output Data Packer
  DataOut: process(RfClk)
  begin
    if rising_edge(RfClk) then
      --  rPhaseShiftReg = "011"
      rDataOut <= rDataInDly(2)(kDataQ7Msb downto kDataI0Lsb);
      if rPhaseShiftReg = "110" or rPhaseShiftReg = "100" then
        rDataOut <= rDataInDly(2)(kDataQ3Msb downto kDataI0Lsb) &
                    rDataInDly(3)(c1DataIn'length-1 downto kDataQ7Msb+1);
      elsif rPhaseShiftReg = "101" or rPhaseShiftReg = "001" then
        rDataOut <= rDataInDly(3)(c1DataIn'length-1 downto kDataI4Lsb);
      elsif rPhaseShiftReg = "010" then
        rDataOut <= rDataInDly(3)(kDataQ7Msb downto kDataI0Lsb);
      end if;
    end if;
  end process;

  DataValidOut: process(RfClk, arReset_n)
  begin
    if arReset_n = '0' then
      rDataValidDly  <= (others => '0');
      rDataValidOut  <= '0';
    elsif rising_edge(RfClk) then
      rDataValidDly  <= rDataValidDly(rDataValidDly'left-1 downto 0) &
                        c1DataValidIn;

      -- Data valid out asserting based on phase alignment RfClk and Clk1x.
      -- When RfClk and Clk1x are not phase aligned.
      rDataValidOut  <= rDataValidDly(2) and rReadyForOutput;

      -- When RfClk and Clk1x are phase aligned.
      if (rPhaseShiftReg(2) xor rPhaseShiftReg(1) xor rPhaseShiftReg(0)) = '1' then
        rDataValidOut  <= rDataValidDly(2) and rDataValidDly(3) and rReadyForOutput;
      end if;
    end if;
  end process;

end RTL;

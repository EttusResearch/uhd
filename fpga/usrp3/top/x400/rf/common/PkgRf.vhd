--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: PkgRf
--
-- Description:
--
--   This package has some type definition and functions used in the RF data
--   chain.
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

package PkgRf is

  -- DDC sample data out width.
  constant kDdcDataOutWidth  : natural := 17;
  -- Each sample is padded in MSB with 7 extra bits of zero to byte align.
  constant kDdcDataWordWidth : natural := kDdcDataOutWidth+7;
  -- DUC sample data out width.
  constant kDucDataOutWidth  : natural := 18;
  -- Each sample is padded in MSB with 6 extra bits of zero to byte align.
  constant kDucDataWordWidth : natural := kDucDataOutWidth+6;
  -- Saturated data output width.
  constant kSatDataWidth     : natural := 16;
  -- ADC sample resolution.
  constant kAdcSampleRes     : natural := 16;

  subtype Sample18_t is    signed(17 downto 0);
  subtype Sample17_t is    signed(16 downto 0);
  subtype Sample16_t is    signed(15 downto 0);
  subtype Sample16slv_t is std_logic_vector(15 downto 0);

  type Samples16_t is array(natural range<>) of Sample16_t;
  type Samples17_t is array(natural range<>) of Sample17_t;
  type Samples18_t is array(natural range<>) of Sample18_t;

  -- These constants have the largest and smallest 18-bit, 17-bit, and 16-bit
  -- signed values.
  constant kLargest18  : Sample18_t := to_signed(2**17 - 1, 18);
  constant kSmallest18 : Sample18_t := to_signed(-2**17, 18);
  constant kLargest17  : Sample17_t := to_signed(2**16 - 1, 17);
  constant kSmallest17 : Sample17_t := to_signed(-2**16, 17);
  constant kLargest16  : Sample16_t := to_signed(2**15 - 1, 16);
  constant kSmallest16 : Sample16_t := to_signed(-2**15, 16);

  function Saturate(s : signed ) return Sample16_t;
  function to_stdlogicvector(d : Samples16_t) return std_logic_vector;
  function to_Samples16(d : std_logic_vector) return Samples16_t;
  function to_Samples17(d : std_logic_vector) return Samples17_t;
  function to_Samples18(d : std_logic_vector) return Samples18_t;
  -- Shift the ADC sample to the left by 1 bit.
  function Gain2x(d : std_logic_vector) return std_logic_vector;
  function Gain2x(s : Samples16_t) return Samples16_t;
  --synopsys translate_off
  function tb_saturate(s: std_logic_vector) return Sample16slv_t;
  --synopsys translate_on

end package PkgRf;


package body PkgRf is

  -- Function to saturate any signed number greater then 16 bits.
  -- A saturated 16-bit data is returned.
  function Saturate ( s : signed) return Sample16_t is
  begin
    if s > kLargest16 then
      return kLargest16;
    elsif s < kSmallest16 then
      return kSmallest16;
    else
      return resize(s, 16);
    end if;
  end function Saturate;

  -- This function will convert 16 bit signed array into a single
  -- std_logic_vector.
  function to_stdlogicvector(d : Samples16_t) return std_logic_vector is
    -- This alias is used to normalize the input vector to [d'length-1 downto 0]
    alias normalD : Samples16_t(d'length-1 downto 0) is d;
    variable rval : std_logic_vector(d'length * 16 - 1 downto 0);
    constant dataWidth : natural := Sample16_t'length;
  begin
    for i in normalD'range loop
      rval(i*dataWidth + dataWidth-1 downto i*dataWidth)
        := std_logic_vector(normalD(i));
    end loop;
    return rval;
  end function to_stdlogicvector;

  -- This function will convert a std_logic_vector into an array of 18 bit
  -- signed array.  The input std_logic_vector has data packed in 24 bits. But
  -- only 18 bits has valid data and remaining 6 MSB bits are padded with
  -- zeros.
  function to_Samples18(d : std_logic_vector) return Samples18_t is
    -- This alias is used to normalize the input vector to [d'length-1 downto 0]
    alias normalD : std_logic_vector(d'length-1 downto 0) is d;
    variable rval : Samples18_t(d'length / kDucDataWordWidth - 1 downto 0);
  begin
    --synopsys translate_off
    assert (((d'length) mod kDucDataWordWidth) = 0)
      report "Input to the function to_Samples18 must be a multiple of kDucDataWordWidth"
      severity error;
    --synopsys translate_on
    for i in rval'range loop
      rval(i) := Sample18_t(normalD(i*kDucDataWordWidth + Sample18_t'length-1
                                    downto i*kDucDataWordWidth));
    end loop;
    return rval;
  end function to_Samples18;

  -- This function will convert a std_logic_vector into an array of 16 bit
  -- signed array. The input std_logic_vector has data packed in 16 bits. But
  -- only 15 bits has valid data and the uper two bits only have the signed
  -- bit.
  function to_Samples16(d : std_logic_vector) return Samples16_t is
    -- This alias is used to normalize the input vector to [d'length-1 downto 0]
    alias normalD : std_logic_vector(d'length-1 downto 0) is d;
    variable rval : Samples16_t(d'length / kAdcSampleRes - 1 downto 0);
  begin
    --synopsys translate_off
    assert (((d'length) mod kAdcSampleRes) = 0)
      report "Input to the function to_Samples16 must be a multiple of kAdcSampleRes"
      severity error;
    --synopsys translate_on
    for i in rval'range loop
      rval(i) := Sample16_t(normalD(i*kAdcSampleRes + Sample16_t'length-1
                                    downto i*kAdcSampleRes));
    end loop;
    return rval;
  end function to_Samples16;

  -- This function will convert a std_logic_vector into an array of 19 bit
  -- signed array. The input std_logic_vector has data packed in 24 bits. But
  -- only 17 bits has valid data and remaining 7 MSB bits are padded with
  -- zeros.
  function to_Samples17(d : std_logic_vector) return Samples17_t is
    -- This alias is used to normalize the input vector to [d'length-1 downto 0]
    alias normalD : std_logic_vector(d'length-1 downto 0) is d;
    variable rval : Samples17_t(d'length / kDdcDataWordWidth - 1 downto 0);
  begin
    --synopsys translate_off
    assert (((d'length) mod kDdcDataWordWidth) = 0)
      report "Input to the function to_Samples17 must be a multiple of kDdcDataWordWidth"
      severity error;
    --synopsys translate_on
    for i in rval'range loop
      rval(i) := Sample17_t(normalD(i*kDdcDataWordWidth + Sample17_t'length-1
                                    downto i*kDdcDataWordWidth));
    end loop;
    return rval;
  end function to_Samples17;

  -- Function to shift the sample to the left by one bit and effectively
  -- multiply by 2.
  function Gain2x(s : Samples16_t) return Samples16_t is
    variable rval : Samples16_t(s'range);
  begin
    for i in rval'range loop
      rval(i) := s(i)(kAdcSampleRes-2 downto 0) & '0';
    end loop;
    return rval;
  end function Gain2x;

  function Gain2x (d : std_logic_vector) return std_logic_vector is
  begin
    return to_stdlogicvector(Gain2x(to_Samples16(d)));
  end function;

  --synopsys translate_off
  ---------------------------------------------------------------
  -- Function below this comment is used only for testbench.
  ---------------------------------------------------------------
  -- This function does saturation of a signed number in std_logic_vector data
  -- type. The current implementation supports only 17 or 18 bit signed
  -- number.
  function tb_saturate(s: std_logic_vector) return Sample16slv_t is
    -- This alias is used to normalize the input vector to [s'length-1 downto 0]
    alias normalS : std_logic_vector(s'length-1 downto 0) is s;
    variable rval : Sample16slv_t;
    constant len  : integer := s'length;
  begin

    -- If 2 MSBs = 00, output <= input without MSB, e.g. positive number < 1
    -- If 2 MSBs = 01, output <= 0.111111111111111, e.g. positive number >= 1
    -- If 2 MSBs = 10, output <= 1.000000000000000, e.g. negative number < -1
    -- If 2 MSBs = 11, output <= input without MSB, e.g. negative number >= -1
    if len = kDdcDataOutWidth then
      if normalS(len-1 downto len-2) = "01" then
        rval := "0111111111111111";
      elsif normalS(len-1 downto len-2) = "10" then
        rval := "1000000000000000";
      else
        rval := normalS(len-2 downto 0);
      end if;

    -- If 3 MSBs = 000, output <= input without MSB, e.g. positive number < 1
    -- If 3 MSBs = 0x1/01x, output <= 0.111111111111111, e.g. positive number >= 1
    -- If 3 MSBs = 1x0/10x, output <= 1.000000000000000, e.g. negative number < -1
    -- If 3 MSBs = 111, output <= input without MSB, e.g. negative number >= -1
    else -- len = kDucDataOutWidth
      if normalS(len-1) = '0' and normalS(len-2 downto len-3) /= "00" then
        rval := "0111111111111111";
      elsif (normalS(len-1 downto len-3) = "000") or
            (normalS(len-1 downto len-3) = "111") then
        rval := normalS(len-3 downto 0);
      else
        rval := "1000000000000000";
      end if;
    end if;
    return rval;
  end function tb_saturate;
  --synopsys translate_on

end package body;

-------------------------------------------------------------------------------
--
-- File: PkgAdcDacInterfaceTypes.vhd
-- Author: National Instruments
-- Original Project: USRP N32x
-- Date: 15 Dec 2017
--
-------------------------------------------------------------------------------
-- (c) 2018 Copyright National Instruments Corporation
-- All Rights Reserved
-- National Instruments Internal Information
-------------------------------------------------------------------------------
--
-- Purpose: Contains types for ADC and DAC data so they can more easily be
--          passed through the design.
--
-- vreview_group JesdCoreN32x
-- vreview_reviewers djepson wfife
-------------------------------------------------------------------------------

library ieee;
  use ieee.std_logic_1164.all;
  use ieee.numeric_std.all;

library work;
  use work.PkgJesdConfig.all;


package PkgAdcDacInterfaceTypes is

  -- Data type for the DACs.
  type DacData_t is record
    I : std_logic_vector(kDacDataWidth - 1 downto 0);
    Q : std_logic_vector(kDacDataWidth - 1 downto 0);
  end record;

  -- Data type for the ADCs.
  type AdcData_t is record
    I : std_logic_vector(kAdcDataWidth - 1 downto 0);
    Q : std_logic_vector(kAdcDataWidth - 1 downto 0);
  end record;

  -- Type has two bits that correspond to I and Q; example usage: overrange flags.
  type IQFlags_t is record
    I : std_logic;
    Q : std_logic;
  end record;

  -- Single data type for all information from the ADCs.
  type AdcSamples_t is record
    Data  : AdcData_t;
    Over  : IQFlags_t;
    CBit1 : IQFlags_t;
  end record;

  -- Single data type for all information to the DACs.
  type DacSamples_t is record
    Data  : DacData_t;
  end record;

  -- To support multiple data values per clock cycle, these types
  -- are arrays of the ADC and DAC data types where the size of the array
  -- corresponds to the number of samples per cycle.
  type AdcDataAry_t is array (kSamplesPerCycle - 1 downto 0) of AdcSamples_t;
  type DacDataAry_t is array (kSamplesPerCycle - 1 downto 0) of DacSamples_t;

  -- Zero/default constant
  constant kAdcDataAryZero : AdcDataAry_t :=
               (others => (Data  => (others => (others => '0')),
                           Over  => (others => '0'),
                           CBit1 => (others => '0'))
               );

  -- Zero/default constant
  constant kDacDataAryZero : DacDataAry_t :=
               (others => (Data  => (others => (others => '0')))
               );

  -- Flattened type that converts the ADC data into std_logic_vector types. This type is
  -- not suitable for use in the port maps of components that are presynthesized (into EDF
  -- or NGC files) but is useful for passing data to the top level.
  type AdcDataAryFlat_t is record
    DataI  : std_logic_vector(kSamplesPerCycle*kAdcDataWidth - 1 downto 0);
    DataQ  : std_logic_vector(kSamplesPerCycle*kAdcDataWidth - 1 downto 0);
    OverI  : std_logic_vector(kSamplesPerCycle - 1 downto 0);
    OverQ  : std_logic_vector(kSamplesPerCycle - 1 downto 0);
    CBit1I : std_logic_vector(kSamplesPerCycle - 1 downto 0);
    CBit1Q : std_logic_vector(kSamplesPerCycle - 1 downto 0);
  end record;

  -- Fully flattened ADC data for passing into and out of presynthesized components.
  subtype AdcDataAryFlatter_t is std_logic_vector(2*(kSamplesPerCycle*(kAdcDataWidth + 2)) - 1 downto 0);

  -- Flattened type that converts the DAC data into std_logic_vector types. This type is
  -- not suitable for use in the port maps of components that are presynthesized (into EDF
  -- or NGC files) but is useful for passing data from the top level.
  type DacDataAryFlat_t is record
    DataI : std_logic_vector(kSamplesPerCycle*kDacDataWidth - 1 downto 0);
    DataQ : std_logic_vector(kSamplesPerCycle*kDacDataWidth - 1 downto 0);
  end record;

  -- Fully flattened DAC data for passing into and out of presynthesized components.
  subtype DacDataAryFlatter_t is std_logic_vector(2*(kSamplesPerCycle*kDacDataWidth) - 1 downto 0);

  -- Function to convert types defined above for the ADC data
  function Flatten  (AdcData : AdcDataAry_t)         return AdcDataAryFlat_t;
  function Flatten  (AdcData : AdcDataAryFlat_t)     return AdcDataAryFlatter_t;
  function Flatten  (AdcData : AdcDataAry_t)         return AdcDataAryFlatter_t;
  function Unflatten(AdcData : AdcDataAryFlatter_t)  return AdcDataAryFlat_t;
  function Unflatten(AdcData : AdcDataAryFlat_t)     return AdcDataAry_t;
  function Unflatten(AdcData : AdcDataAryFlatter_t)  return AdcDataAry_t;


  -- Function to convert types defined above for the DAC data
  function Flatten  (DacData : DacDataAry_t)         return DacDataAryFlat_t;
  function Flatten  (DacData : DacDataAryFlat_t)     return DacDataAryFlatter_t;
  function Flatten  (DacData : DacDataAry_t)         return DacDataAryFlatter_t;
  function Unflatten(DacData : DacDataAryFlatter_t)  return DacDataAryFlat_t;
  function UnFlatten(DacData : DacDataAryFlat_t)     return DacDataAry_t;
  function Unflatten(DacData : DacDataAryFlatter_t)  return DacDataAry_t;


end package PkgAdcDacInterfaceTypes;


package body PkgAdcDacInterfaceTypes is

  -- Flattens AdcDataAry_t to AdcDataAryFlat_t
  function Flatten(AdcData : AdcDataAry_t) return AdcDataAryFlat_t
  is
    variable ReturnVar : AdcDataAryFlat_t;
  begin
    ReturnVar := (DataI  => (others => '0'), -- Note (others => (others => '0')) does not work here
                  DataQ  => (others => '0'), -- since DataX and OverX/CBit1X are of different lengths (ModelSim error)
                  OverI  => (others => '0'),
                  OverQ  => (others => '0'),
                  CBit1I => (others => '0'),
                  CBit1Q => (others => '0'));

    -- The upstream logic puts the 0th element of an array in the MSBs of its data word
    for i in 0 to kSamplesPerCycle - 1 loop
      ReturnVar.DataI((kSamplesPerCycle - i)*kAdcDataWidth - 1 downto (kSamplesPerCycle - 1 - i)*kAdcDataWidth) := AdcData(i).Data.I; -- Input Data 0 to MSB
      ReturnVar.DataQ((kSamplesPerCycle - i)*kAdcDataWidth - 1 downto (kSamplesPerCycle - 1 - i)*kAdcDataWidth) := AdcData(i).Data.Q;
      ReturnVar.OverI (kSamplesPerCycle - 1 - i) := AdcData(i).Over.I; -- Input Data 0 to MSB
      ReturnVar.OverQ (kSamplesPerCycle - 1 - i) := AdcData(i).Over.Q;
      ReturnVar.CBit1I(kSamplesPerCycle - 1 - i) := AdcData(i).CBit1.I;
      ReturnVar.CBit1Q(kSamplesPerCycle - 1 - i) := AdcData(i).CBit1.Q;
    end loop;

    return ReturnVar;
  end function Flatten;


  -- UnFlattens AdcDataAryFlat_t to AdcDataAry_t
  function Unflatten(AdcData : AdcDataAryFlat_t) return AdcDataAry_t
  is
    variable ReturnVar : AdcDataAry_t;
  begin
    ReturnVar  := (others => (Data => (others => (others => '0')), Over => (others => '0'), CBit1 => (others => '0')));
    for i in 0 to kSamplesPerCycle - 1 loop
      -- MSB of flattened word = 0th element of ADC data array - this corresponds to how TheWindow
      -- expects data arrays to be transferred.
      ReturnVar(kSamplesPerCycle - 1 - i).Data.I  := AdcData.DataI((i+1)*kAdcDataWidth - 1 downto i*kAdcDataWidth);
      ReturnVar(kSamplesPerCycle - 1 - i).Data.Q  := AdcData.DataQ((i+1)*kAdcDataWidth - 1 downto i*kAdcDataWidth);
      ReturnVar(kSamplesPerCycle - 1 - i).Over.I  := AdcData.OverI(i);
      ReturnVar(kSamplesPerCycle - 1 - i).Over.Q  := AdcData.OverQ(i);
      ReturnVar(kSamplesPerCycle - 1 - i).CBit1.I := AdcData.CBit1I(i);
      ReturnVar(kSamplesPerCycle - 1 - i).CBit1.Q := AdcData.CBit1Q(i);
    end loop;
    return ReturnVar;
  end function Unflatten;




  -- Flattens AdcDataAryFlat_t to AdcDataAryFlatter_t
  function Flatten(AdcData : AdcDataAryFlat_t) return AdcDataAryFlatter_t
  is
    variable ReturnVar : AdcDataAryFlatter_t;
  begin
    ReturnVar := AdcData.OverQ & AdcData.CBit1Q & AdcData.OverI & AdcData.CBit1I & AdcData.DataQ & AdcData.DataI;
    return ReturnVar;
  end function Flatten;

  -- UnFlattens AdcDataAryFlatter_t to AdcDataAryFlat_t
  function Unflatten(AdcData : AdcDataAryFlatter_t) return AdcDataAryFlat_t
  is
    variable ReturnVar : AdcDataAryFlat_t;
  begin
    ReturnVar.DataI  := AdcData(1*kSamplesPerCycle*kAdcDataWidth + 0*kSamplesPerCycle - 1 downto 0*kSamplesPerCycle*kAdcDataWidth + 0*kSamplesPerCycle);
    ReturnVar.DataQ  := AdcData(2*kSamplesPerCycle*kAdcDataWidth + 0*kSamplesPerCycle - 1 downto 1*kSamplesPerCycle*kAdcDataWidth + 0*kSamplesPerCycle);
    ReturnVar.CBit1I := AdcData(2*kSamplesPerCycle*kAdcDataWidth + 1*kSamplesPerCycle - 1 downto 2*kSamplesPerCycle*kAdcDataWidth + 0*kSamplesPerCycle);
    ReturnVar.OverI  := AdcData(2*kSamplesPerCycle*kAdcDataWidth + 2*kSamplesPerCycle - 1 downto 2*kSamplesPerCycle*kAdcDataWidth + 1*kSamplesPerCycle);
    ReturnVar.CBit1Q := AdcData(2*kSamplesPerCycle*kAdcDataWidth + 3*kSamplesPerCycle - 1 downto 2*kSamplesPerCycle*kAdcDataWidth + 2*kSamplesPerCycle);
    ReturnVar.OverQ  := AdcData(2*kSamplesPerCycle*kAdcDataWidth + 4*kSamplesPerCycle - 1 downto 2*kSamplesPerCycle*kAdcDataWidth + 3*kSamplesPerCycle);
    return ReturnVar;
  end function Unflatten;


  -- Flattens AdcDataAry_t to AdcDataAryFlatter_t
  function Flatten(AdcData : AdcDataAry_t) return AdcDataAryFlatter_t
  is
    variable TempVar   : AdcDataAryFlat_t;
    variable ReturnVar : AdcDataAryFlatter_t;
  begin
    TempVar   := Flatten(AdcData);
    ReturnVar := Flatten(TempVar);
    return ReturnVar;
  end function Flatten;

  -- UnFlattens AdcDataAryFlatter_t to AdcDataAry_t
  function Unflatten(AdcData : AdcDataAryFlatter_t) return AdcDataAry_t
  is
    variable TempVar   : AdcDataAryFlat_t;
    variable ReturnVar : AdcDataAry_t;
  begin
    TempVar   := Unflatten(AdcData);
    ReturnVar := Unflatten(TempVar);
    return ReturnVar;
  end function Unflatten;



  -- Flattens DacDataAry_t to DacDataAryFlat_t
  function Flatten(DacData : DacDataAry_t) return DacDataAryFlat_t
  is
    variable ReturnVar : DacDataAryFlat_t;
  begin
    ReturnVar := (others => (others => '0'));
    for i in 0 to kSamplesPerCycle - 1 loop
      -- MSB of flattened word = 0th element of ADC data array - this corresponds to how TheWindow
      -- expects data arrays to be transferred.
      ReturnVar.DataI((i+1)*kDacDataWidth - 1 downto i*kDacDataWidth) := DacData(kSamplesPerCycle - 1 - i).Data.I;
      ReturnVar.DataQ((i+1)*kDacDataWidth - 1 downto i*kDacDataWidth) := DacData(kSamplesPerCycle - 1 - i).Data.Q;
    end loop;
    return ReturnVar;
  end function Flatten;


  -- UnFlattens DacDataAryFlat_t to DacDataAry_t
  function UnFlatten(DacData : DacDataAryFlat_t) return DacDataAry_t
  is
    variable ReturnVar : DacDataAry_t;
  begin
    ReturnVar := (others => (Data  => (others => (others => '0'))));

    -- The upstream logic puts the 0th element of an array in the MSBs of its data word
    for i in 0 to kSamplesPerCycle - 1 loop
      ReturnVar(kSamplesPerCycle - 1 - i).Data.I := DacData.DataI(kDacDataWidth*(i+1) - 1 downto kDacDataWidth*i);
      ReturnVar(kSamplesPerCycle - 1 - i).Data.Q := DacData.DataQ(kDacDataWidth*(i+1) - 1 downto kDacDataWidth*i);
    end loop;

    return ReturnVar;
  end function UnFlatten;



  -- Flattens DacDataAryFlat_t to DacDataAryFlatter_t
  function Flatten(DacData : DacDataAryFlat_t) return DacDataAryFlatter_t
  is
    variable ReturnVar : DacDataAryFlatter_t;
  begin
    ReturnVar := DacData.DataQ & DacData.DataI;
    return ReturnVar;
  end function Flatten;


  -- UnFlattens DacDataAryFlatter_t to DacDataAryFlat_t
  function Unflatten(DacData : DacDataAryFlatter_t) return DacDataAryFlat_t
  is
    variable ReturnVar : DacDataAryFlat_t;
  begin
    ReturnVar.DataI  := DacData(1*kSamplesPerCycle*kDacDataWidth - 1 downto 0*kSamplesPerCycle*kDacDataWidth);
    ReturnVar.DataQ  := DacData(2*kSamplesPerCycle*kDacDataWidth - 1 downto 1*kSamplesPerCycle*kDacDataWidth);
    return ReturnVar;
  end function Unflatten;


  -- Flattens DacDataAry_t to DacDataAryFlatter_t
  function Flatten(DacData : DacDataAry_t) return DacDataAryFlatter_t
  is
    variable TempVar   : DacDataAryFlat_t;
    variable ReturnVar : DacDataAryFlatter_t;
  begin
    TempVar   := Flatten(DacData);
    ReturnVar := Flatten(TempVar);
    return ReturnVar;
  end function Flatten;

  -- UnFlattens DacDataAryFlatter_t to DacDataAry_t
  function Unflatten(DacData : DacDataAryFlatter_t) return DacDataAry_t
  is
    variable TempVar   : DacDataAryFlat_t;
    variable ReturnVar : DacDataAry_t;
  begin
    TempVar   := Unflatten(DacData);
    ReturnVar := Unflatten(TempVar);
    return ReturnVar;
  end function Unflatten;


end package body;

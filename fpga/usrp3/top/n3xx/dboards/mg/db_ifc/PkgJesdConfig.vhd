-------------------------------------------------------------------------------
--
-- File: PkgJesdConfig.vhd
-- Author: National Instruments
-- Original Project: NI 5840
-- Date: 11 March 2016
--
-------------------------------------------------------------------------------
-- Copyright 2016-2018 Ettus Research, A National Instruments Company
-- SPDX-License-Identifier: LGPL-3.0
-------------------------------------------------------------------------------
--
-- Purpose: JESD204B setup constants and functions. These constants are shared
--          between RX and TX JESD cores.
--
-------------------------------------------------------------------------------

library ieee;
  use ieee.std_logic_1164.all;
  use ieee.numeric_std.all;

library work;
  use work.PkgRegs.all;


package PkgJesdConfig is

  -- "JESD" in ASCII - with the core number 0 or 1 on the LSb.
  constant kJesdSignature : std_logic_vector(31 downto 0) := x"4a455344";

  -- Register endpoints
  constant kJesdDrpRegsInEndpoint : RegOffset_t := (kOffset => 16#0800#,   -- 0x2800 to
                                                    kWidth  => 16#0800#);  -- 0x2FFF

  -- Selects the UsrClk2 for the transceivers. For 64-bit wide transceivers, the
  -- UsrClk = 2*UserClk2 frequency. For 32-bit wide transceivers, UsrClk = UserClk2
  -- frequency. This is a generalization, the clock ratio should be confirmed based on
  -- the transceiver configuration.
  -- The N310 transceivers use the single rate reference, hence = false.
  constant kDoubleRateUsrClk : boolean := false;

  -- For the N310, all lanes are in one quad and we use the QPLL.
  constant kJesdUseQpll : boolean := true;

  constant kAdcDataWidth     : integer := 16; -- ADC data width in bits
  constant kDacDataWidth     : integer := 16; -- DAC data width in bits
  constant kSamplesPerCycle  : integer := 1;  -- Number of samples per SampleClk1x

  constant kGtxDrpAddrWidth  : natural := 9;
  constant kQpllDrpAddrWidth : natural := 8;
  -- Max supported number of lanes
  constant kMaxNumLanes      : natural := 4;
  -- Max supported number of quads (normally there is 1 quad per 4 lanes but disconnect
  -- the definitions to allow quad sharing)
  constant kMaxNumQuads      : natural := 1;


  -- JESD shared setup - LMFS = 4421, HD = 0
  constant kNumLanes               : natural   := 4;           -- L
  constant kNumConvs               : positive  := 4;           -- M
  constant kOctetsPerFrame         : natural   := 2;           -- F
  constant kDacJesdSamplesPerCycle : integer   := 1;           -- S
  constant kOctetsPerLane          : natural   := 2;           -- MGT data is kOctetsPerLane*8 = 16 bits wide
  constant kNumQuads               : natural   := kNumLanes/4; -- 4 lanes per quad
  constant kHighDensity            : boolean   := false;       -- HD
  constant kConvResBits            : positive  := kDacDataWidth-2; -- Converter resolution in bits
  constant kConvSampleBits         : positive  := kDacDataWidth;   -- Sample Length in bits
  constant kInitLaneAlignCnt       : positive  := 4;
  constant kFramesPerMulti         : natural   := 20;          -- K

  -- In the N310 case we are one SPC, so this value is simply the number of frames
  -- (samples) per multiframe.
  constant kUserClksPerMulti : integer := kFramesPerMulti;


  type NaturalVector is array ( natural range <>) of natural;

  -- The PCB connections are as follows:
  --
  --   Transceiver  MGT Channel   ADC Lane    DAC Lane
  --   ***********  ***********   ********    ********
  --   GT0: X0Y8        0            0           0
  --   GT1: X0Y9        1            1           1
  --   GT2: X0Y10       2            2           2
  --   GT3: X0Y11       3            3           3
  constant kRxLaneIndices : NaturalVector(kNumLanes - 1 downto 0) :=
    (
 -- MGT => ADC (in above table)
      0 => 0,
      1 => 1,
      2 => 2,
      3 => 3
    );

  constant kTxLaneIndices : NaturalVector(kNumLanes - 1 downto 0) :=
    (
 -- MGT => DAC lane
      0 => 0,
      1 => 1,
      2 => 2,
      3 => 3
    );

  constant kLaneToQuadMap : NaturalVector(kNumLanes - 1 downto 0) :=
    (
      -- All lanes are in one quad
      0 => 0,
      1 => 0,
      2 => 0,
      3 => 0
    );


  -- The master transceiver channel for channel bonding. E(kMasterBondingChannel)
  -- must have the highest value decrementing to b"000" for that last channels to bond.
  constant kMasterBondingChannel : integer := 1;

  -- Channel bonding occurs when a master detects a K-char sequence and aligns its
  -- internal FIFO to the start of this sequence. A signal is then generated to other
  -- slave transceivers that cause them to bond to the sequence - this bonding signal is
  -- cascaded from master to slave to slave to slave, etc where each slave must know how
  -- many levels to the master there are. The last slave to bond must be at level b"000"
  -- and the master is at the highest level; the number of levels in the sequence is
  -- governed by the size of the transceiver FIFO (see the Xilinx user guides for more
  -- information).
  type BondLevels_t is array(0 to kNumLanes - 1) of std_logic_vector(2 downto 0);
  constant kBondLevel : BondLevels_t := (
      0 => b"000", -- Control from 1
      1 => b"001", -- Master
      2 => b"000", -- Control from 1
      3 => b"000"  -- Control from 1
    );

  -- Option to pipeline stages to improve timing, if needed
  constant kPipelineDetectCharsStage : boolean := false;
  constant kPipelineCharReplStage    : boolean := false;


  -- ADC & DAC Data Types
  --

  -- ADC Words from JESD204B RX Core. The array is 4 elements wide to accommodate the
  -- I & Q elements from both RX channels.
  subtype AdcWord_t is std_logic_vector(kAdcDataWidth - 1 downto 0);
  type AdcWordArray_t is array(4 - 1 downto 0) of AdcWord_t;

  -- Data types for manipulation and presentation to outside world.
  type AdcData_t is record
    I : std_logic_vector(kAdcDataWidth - 1 downto 0);
    Q : std_logic_vector(kAdcDataWidth - 1 downto 0);
  end record;

  type DacData_t is record
    I : std_logic_vector(kDacDataWidth - 1 downto 0);
    Q : std_logic_vector(kDacDataWidth - 1 downto 0);
  end record;


  -- Flattened data types for passing into and out of pre-synthesized components.
  subtype AdcDataFlat_t is std_logic_vector(2*kAdcDataWidth - 1 downto 0);
  subtype DacDataFlat_t is std_logic_vector(2*kDacDataWidth - 1 downto 0);

  -- Functions to convert to/from types defined above.
  function Flatten  (AdcData : AdcData_t)         return AdcDataFlat_t;
  function Unflatten(AdcData : AdcDataFlat_t)     return AdcData_t;

  function Flatten  (DacData : DacData_t)         return DacDataFlat_t;
  function Unflatten(DacData : DacDataFlat_t)     return DacData_t;


end package;


package body PkgJesdConfig is





  -- Flattens AdcData_t to AdcDataFlat_t
  function Flatten  (AdcData : AdcData_t)         return AdcDataFlat_t
  is
    variable ReturnVar : AdcDataFlat_t;
  begin
    ReturnVar := (others => '0');
    -- MSB is I
    ReturnVar := AdcData.I & AdcData.Q;
    return ReturnVar;
  end function Flatten;


  -- UnFlattens AdcDataFlat_t to AdcData_t
  function Unflatten(AdcData : AdcDataFlat_t) return AdcData_t
  is
    variable ReturnVar : AdcData_t;
  begin
    ReturnVar  := (others => (others => '0'));
    -- MSB is I
    ReturnVar.I  := AdcData(2*kAdcDataWidth - 1 downto kAdcDataWidth);
    ReturnVar.Q  := AdcData(  kAdcDataWidth - 1 downto 0);
    return ReturnVar;
  end function Unflatten;



  -- Flattens DacData_t to DacDataFlat_t
  function Flatten  (DacData : DacData_t)         return DacDataFlat_t
  is
    variable ReturnVar : DacDataFlat_t;
  begin
    ReturnVar := (others => '0');
    -- MSB is I
    ReturnVar := DacData.I & DacData.Q;
    return ReturnVar;
  end function Flatten;


  -- UnFlattens DacDataFlat_t to DacData_t
  function Unflatten(DacData : DacDataFlat_t) return DacData_t
  is
    variable ReturnVar : DacData_t;
  begin
    ReturnVar  := (others => (others => '0'));
    -- MSB is I
    ReturnVar.I  := DacData(2*kDacDataWidth - 1 downto kDacDataWidth);
    ReturnVar.Q  := DacData(  kDacDataWidth - 1 downto 0);
    return ReturnVar;
  end function Unflatten;





end package body;
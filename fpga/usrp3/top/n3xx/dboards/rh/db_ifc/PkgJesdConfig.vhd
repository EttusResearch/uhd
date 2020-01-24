-------------------------------------------------------------------------------
--
-- File: PkgJesdConfig.vhd
-- Author: National Instruments
-- Original Project: N32x
-- Date: 15 Dec 2017
--
-------------------------------------------------------------------------------
-- Copyright 2016-2018 Ettus Research, A National Instruments Company
-- SPDX-License-Identifier: LGPL-3.0
-------------------------------------------------------------------------------
--
-- Purpose: JESD204B setup constants and functions. These constants are shared
--          between RX and TX JESD cores.
--
-- vreview_group JesdCoreN32x
-- vreview_reviewers djepson wfife
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

  -- For the N32x, all lanes are in one quad and we use the QPLL.
  constant kJesdUseQpll : boolean := true;

  constant kAdcDataWidth     : integer := 14; -- ADC data width in bits
  constant kDacDataWidth     : integer := 16; -- DAC data width in bits
  constant kSamplesPerCycle  : integer := 2;  -- Number of samples per SampleClk1x

  constant kGtxDrpAddrWidth    : natural := 9;
  constant kGtxAddrLsbPosition : natural := 2;
  constant kQpllDrpAddrWidth   : natural := 8;
  constant kGtxDrpDataWidth    : natural := 16;
  
  -- Max supported number of lanes
  constant kMaxNumLanes      : natural := 4;
  -- Max supported number of quads (normally there is 1 quad per 4 lanes but disconnect
  -- the definitions to allow quad sharing)
  constant kMaxNumQuads      : natural := 1;

  -- Rhodium:
  -- JESD shared setup - LMFS = 4211, HD = 1 (Samples are split across multiple lanes).
  constant kNumLanes               : natural   := 4;             -- L
  constant kNumConvs               : positive  := 2;             -- M
  constant kOctetsPerFrame         : natural   := 1;             -- F
  constant kDacJesdSamplesPerCycle : integer   := 1;             -- S
  constant kOctetsPerLane          : natural   := 2;             -- MGT data is kOctetsPerLane*8 = 16 bits wide
  constant kNumQuads               : natural   := kNumLanes / 4; -- 4 lanes per quad
  constant kHighDensity            : boolean   := true;          -- HD
  constant kConvResBits            : positive  := kDacDataWidth; -- Converter resolution in bits
  constant kConvSampleBits         : positive  := 16;            -- Sample Length in bits
  constant kInitLaneAlignCnt       : positive  := 4;
  constant kFramesPerMulti         : natural   := 24;            -- K

  -- Rhodium:
  -- The converters are running at 400/491.52/500 MSPS (DeviceClk), and the sampling
  -- clock at the FPGA (UserClk) is 200/245.76/250 MHz; so UsrClk = (DeviceClk / 2).
  -- The frame rate = DeviceClk, and the Multiframe rate = (frame rate / kFramesPerMulti)
  -- Thus, kUserClksPerMulti = (UsrClk / Multiframe rate)
  --                         = (UsrClk / (DeviceClk / kFramesPerMulti))
  -- since UsrClk = DeviceClk / 2 then,
  --       kUserClksPerMulti = ((DeviceClk / 2) / (DeviceClk / kFramesPerMulti))
  -- therefore,
  --       kUserClksPerMulti = kFramesPerMulti / 2 
  constant kUserClksPerMulti : integer := kFramesPerMulti / 2;


  type NaturalVector is array ( natural range <>) of natural;

  -- The PCB connections are are passed trough, any swapping is handled somewhere else.
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


  -- User Rx Data
  -- ADC Word data width: 14 sample bits + 2 tails bits
  constant kAdcWordWidth : integer := 16;
  subtype AdcWord_t is std_logic_vector(kAdcWordWidth - 1 downto 0);
  type AdcWordArray_t is array(kSamplesPerCycle*2 - 1 downto 0) of AdcWord_t; -- The *2 is because there are two samples (I and Q) per "sample"

  -- Constants to specify the contents of the AdcWord_t vector.
  constant kAdcWordDataMsb : integer := 15;
  constant kAdcWordDataLsb : integer := 2;
  constant kAdcWordOver    : integer := 1;
  constant kAdcWordCBit1   : integer := 0;


  -- Option to pipeline stages to improve timing, if needed
  constant kPipelineDetectCharsStage : boolean := false;
  constant kPipelineCharReplStage    : boolean := false;

end package;

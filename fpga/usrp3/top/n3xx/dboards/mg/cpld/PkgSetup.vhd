-------------------------------------------------------------------------------
--
-- File: PkgSetup.vhd
-- Author: Daniel Jepson
-- Original Project: N310
-- Date: 22 September 2017
--
-------------------------------------------------------------------------------
-- Copyright 2016-2017 Ettus Research, A National Instruments Company
-- SPDX-License-Identifier: GPL-3.0
-------------------------------------------------------------------------------
--
-- Purpose:
--
-- Default values for front end config and CPLD constants.
--
-- Contains the revision constants that must be bumped when the CPLD is updated.
--
-------------------------------------------------------------------------------

library ieee;
  use ieee.std_logic_1164.all;
  use ieee.numeric_std.all;

library work;
  use work.PkgMgCpld.all;

package PkgSetup is


  constant kRdWtWidth   : integer :=  1;
  constant kAddrWidth   : integer :=  7;
  constant kDataWidth   : integer := 16;
  constant kTotalWidth  : integer := kRdWtWidth + kAddrWidth + kDataWidth;

  subtype InterfaceData_t is std_logic_vector(kDataWidth-1 downto 0);

  constant kSignature : InterfaceData_t := x"CAFE";


  -- UPDATE THESE REVISIONS when making changes to the CPLD -----------------------------
  -- ------------------------------------------------------------------------------------
  constant kMinorRev  : InterfaceData_t := std_logic_vector(to_unsigned(0,kDataWidth));
  constant kMajorRev  : InterfaceData_t := std_logic_vector(to_unsigned(5,kDataWidth));
  -- Currently just the timestamp of the build time/date:  yymmddhh
  constant kBuildCode : std_logic_vector(31 downto 0) := X"18010408";


  function kTxChDefault     return InterfaceData_t;
  function kTxChDefaultRun  return InterfaceData_t;
  function kRxChDefault0    return InterfaceData_t;
  function kRxChDefault1    return InterfaceData_t;
  function kRxChDefault0Run return InterfaceData_t;
  function kRxChDefault1Run return InterfaceData_t;

  function Tx2Switch2Mod(kCh1Val : std_logic_vector) return std_logic_vector;
  function Tx2TrxMod    (kCh1Val : std_logic_vector) return std_logic_vector;

  function Rx2Switch1Mod(kCh1Val : std_logic_vector) return std_logic_vector;
  function Rx2Switch2Mod(kCh1Val : std_logic_vector) return std_logic_vector;
  function Rx2Switch3Mod(kCh1Val : std_logic_vector) return std_logic_vector;
  function Rx2Switch4Mod(kCh1Val : std_logic_vector) return std_logic_vector;
  function Rx2Switch5Mod(kCh1Val : std_logic_vector) return std_logic_vector;
  function Rx2Switch6Mod(kCh1Val : std_logic_vector) return std_logic_vector;


end package;

package body PkgSetup is

  function kTxChDefault return InterfaceData_t is
    variable RetVal : InterfaceData_t := (others => '0');
  begin
    RetVal(kCh1SwTrxMsb downto kCh1SwTrx) := std_logic_vector(to_unsigned(kFromLowerFilterBankTxSw1, kCh1SwTrxSize));
    RetVal(kCh1TxSw1Msb downto kCh1TxSw1) := std_logic_vector(to_unsigned(kShutdownTxSw1, kCh1TxSw1Size));
    RetVal(kCh1TxSw2Msb downto kCh1TxSw2) := std_logic_vector(to_unsigned(kToTxFilterLp3400MHz, kCh1TxSw2Size));
    RetVal(kCh1TxSw3 downto kCh1TxSw3)    := std_logic_vector(to_unsigned(kToTxFilterBanks, kCh1TxSw3Size));
    RetVal(kCh1TxLowbandMixerPathSelect)  := '0';
    RetVal(kCh1TxMixerEn)                 := '0';
    RetVal(kCh1TxAmpEn)                   := '0';
    RetVal(kCh1TxPaEn)                    := '0';
    RetVal(kCh1TxLed)                     := '0';
    RetVal(kCh1MykEnTx)                   := '1';
    return RetVal;
  end kTxChDefault;

  function kTxChDefaultRun return InterfaceData_t is
    variable RetVal : InterfaceData_t := (others => '0');
  begin
    RetVal(kCh1SwTrxMsb downto kCh1SwTrx) := std_logic_vector(to_unsigned(kFromLowerFilterBankTxSw1, kCh1SwTrxSize));
    RetVal(kCh1TxSw1Msb downto kCh1TxSw1) := std_logic_vector(to_unsigned(kFromTxFilterLp3400MHz, kCh1TxSw1Size));
    RetVal(kCh1TxSw2Msb downto kCh1TxSw2) := std_logic_vector(to_unsigned(kToTxFilterLp3400MHz, kCh1TxSw2Size));
    RetVal(kCh1TxSw3 downto kCh1TxSw3)    := std_logic_vector(to_unsigned(kToTxFilterBanks, kCh1TxSw3Size));
    RetVal(kCh1TxLowbandMixerPathSelect)  := '0';
    RetVal(kCh1TxMixerEn)                 := '0';
    RetVal(kCh1TxAmpEn)                   := '1';
    RetVal(kCh1TxPaEn)                    := '1';
    RetVal(kCh1TxLed)                     := '1';
    RetVal(kCh1MykEnTx)                   := '1';
    return RetVal;
  end kTxChDefaultRun;




  function kRxChDefault0 return InterfaceData_t is
    variable RetVal : InterfaceData_t := (others => '0');
  begin
    RetVal(kCh1RxSw1Msb downto kCh1RxSw1) := std_logic_vector(to_unsigned(kRx2Input, kCh1RxSw1Size));
    RetVal(kCh1RxSw2Msb downto kCh1RxSw2) := std_logic_vector(to_unsigned(kShutdownSw2, kCh1RxSw2Size));
    RetVal(kCh1RxSw3Msb downto kCh1RxSw3) := std_logic_vector(to_unsigned(kShutdownSw3, kCh1RxSw3Size));
    RetVal(kCh1RxSw4Msb downto kCh1RxSw4) := std_logic_vector(to_unsigned(kFilter2100x2850MHzFrom, kCh1RxSw4Size));
    RetVal(kCh1RxSw5Msb downto kCh1RxSw5) := std_logic_vector(to_unsigned(kFilter0490LpMHzFrom, kCh1RxSw5Size));
    return RetVal;
  end kRxChDefault0;

  function kRxChDefault1 return InterfaceData_t is
    variable RetVal : InterfaceData_t := (others => '0');
  begin
    RetVal(kCh1RxSw6Msb downto kCh1RxSw6) := std_logic_vector(to_unsigned(kUpperFilterBankFromSwitch4, kCh1RxSw6Size));
    RetVal(kCh1RxLowbandMixerPathSelect)  := '0';
    RetVal(kCh1RxMixerEn)                 := '0';
    RetVal(kCh1RxAmpEn)                   := '0';
    RetVal(kCh1RxLna1En)                  := '0';
    RetVal(kCh1RxLna2En)                  := '0';
    RetVal(kCh1Rx2Led)                    := '0';
    RetVal(kCh1RxLed)                     := '0';
    RetVal(kCh1MykEnRx)                   := '1';
    return RetVal;
  end kRxChDefault1;

  function kRxChDefault0Run return InterfaceData_t is
    variable RetVal : InterfaceData_t := (others => '0');
  begin
    RetVal(kCh1RxSw1Msb downto kCh1RxSw1) := std_logic_vector(to_unsigned(kRx2Input, kCh1RxSw1Size));
    RetVal(kCh1RxSw2Msb downto kCh1RxSw2) := std_logic_vector(to_unsigned(kLowerFilterBankToSwitch3, kCh1RxSw2Size));
    RetVal(kCh1RxSw3Msb downto kCh1RxSw3) := std_logic_vector(to_unsigned(kFilter2100x2850MHz, kCh1RxSw3Size));
    RetVal(kCh1RxSw4Msb downto kCh1RxSw4) := std_logic_vector(to_unsigned(kFilter2100x2850MHzFrom, kCh1RxSw4Size));
    RetVal(kCh1RxSw5Msb downto kCh1RxSw5) := std_logic_vector(to_unsigned(kFilter0490LpMHzFrom, kCh1RxSw5Size));
    return RetVal;
  end kRxChDefault0Run;

  function kRxChDefault1Run return InterfaceData_t is
    variable RetVal : InterfaceData_t := (others => '0');
  begin
    RetVal(kCh1RxSw6Msb downto kCh1RxSw6) := std_logic_vector(to_unsigned(kUpperFilterBankFromSwitch4, kCh1RxSw6Size));
    RetVal(kCh1RxLowbandMixerPathSelect)  := '0';
    RetVal(kCh1RxMixerEn)                 := '0';
    RetVal(kCh1RxAmpEn)                   := '1';
    RetVal(kCh1RxLna1En)                  := '1';
    RetVal(kCh1RxLna2En)                  := '1';
    RetVal(kCh1Rx2Led)                    := '1';     -- turn on a LED for grins
    RetVal(kCh1RxLed)                     := '0';
    RetVal(kCh1MykEnRx)                   := '1';
    return RetVal;
  end kRxChDefault1Run;






  function Tx2Switch2Mod(kCh1Val : std_logic_vector) return std_logic_vector is
    variable RetVal : std_logic_vector(kCh1Val'range) := (others => '0');
  begin
    -- Encoding for this switch is one-hot, so we just flip around the bits here.
    RetVal(kCh1Val'low + 0) := kCh1Val(kCh1Val'low + 0);
    RetVal(kCh1Val'low + 3) := kCh1Val(kCh1Val'low + 1);
    RetVal(kCh1Val'low + 1) := kCh1Val(kCh1Val'low + 2);
    RetVal(kCh1Val'low + 2) := kCh1Val(kCh1Val'low + 3);
    return RetVal;
  end Tx2Switch2Mod;

  function Tx2TrxMod(kCh1Val : std_logic_vector) return std_logic_vector is
    variable RetVal : std_logic_vector(kCh1Val'range) := (others => '0');
  begin
    if    kCh1Val = "00" then RetVal := "00";
    elsif kCh1Val = "01" then RetVal := "10";
    elsif kCh1Val = "10" then RetVal := "01";
    elsif kCh1Val = "11" then RetVal := "11";
    else                      RetVal := "00"; end if;
    return RetVal;
  end Tx2TrxMod;



  function Rx2Switch1Mod(kCh1Val : std_logic_vector) return std_logic_vector is
    variable RetVal : std_logic_vector(kCh1Val'range) := (others => '0');
  begin
    -- Encoding for this switch is binary, so we need to mux.
       if kCh1Val = "00" then RetVal := "01";
    elsif kCh1Val = "01" then RetVal := "00";
    elsif kCh1Val = "10" then RetVal := "11";
    elsif kCh1Val = "11" then RetVal := "10";
    else                      RetVal := "00"; end if;
    return RetVal;
  end Rx2Switch1Mod;

  function Rx2Switch2Mod(kCh1Val : std_logic_vector) return std_logic_vector is
    variable RetVal : std_logic_vector(kCh1Val'range) := (others => '0');
  begin
    -- Encoding for this switch is binary, so we need to mux.
       if kCh1Val = "00" then RetVal := "00";
    elsif kCh1Val = "01" then RetVal := "11";
    elsif kCh1Val = "10" then RetVal := "10";
    elsif kCh1Val = "11" then RetVal := "01";
    else                      RetVal := "00"; end if;
    return RetVal;
  end Rx2Switch2Mod;

  function Rx2Switch3Mod(kCh1Val : std_logic_vector) return std_logic_vector is
    variable RetVal : std_logic_vector(kCh1Val'range) := (others => '0');
  begin
    -- Encoding for this switch is binary, so we need to mux.
       if kCh1Val = "000" then RetVal := "100";
    elsif kCh1Val = "001" then RetVal := "101";
    elsif kCh1Val = "010" then RetVal := "110";
    elsif kCh1Val = "011" then RetVal := "011";
    elsif kCh1Val = "100" then RetVal := "001";
    elsif kCh1Val = "101" then RetVal := "000";
    elsif kCh1Val = "110" then RetVal := "010";
    elsif kCh1Val = "111" then RetVal := "111";
    else                       RetVal := "000"; end if;
    return RetVal;
  end Rx2Switch3Mod;

  function Rx2Switch4Mod(kCh1Val : std_logic_vector) return std_logic_vector is
    variable RetVal : std_logic_vector(kCh1Val'range) := (others => '0');
  begin
    -- Encoding for this switch is one-hot, so we just flip around the bits here.
    RetVal(kCh1Val'low + 2) := kCh1Val(kCh1Val'low + 0);
    RetVal(kCh1Val'low + 1) := kCh1Val(kCh1Val'low + 1);
    RetVal(kCh1Val'low + 0) := kCh1Val(kCh1Val'low + 2);
    return RetVal;
  end Rx2Switch4Mod;

  function Rx2Switch5Mod(kCh1Val : std_logic_vector) return std_logic_vector is
    variable RetVal : std_logic_vector(kCh1Val'range) := (others => '0');
  begin
    -- Encoding for this switch is one-hot, so we just flip around the bits here.
    RetVal(kCh1Val'low + 1) := kCh1Val(kCh1Val'low + 0);
    RetVal(kCh1Val'low + 0) := kCh1Val(kCh1Val'low + 1);
    RetVal(kCh1Val'low + 3) := kCh1Val(kCh1Val'low + 2);
    RetVal(kCh1Val'low + 2) := kCh1Val(kCh1Val'low + 3);
    return RetVal;
  end Rx2Switch5Mod;

  function Rx2Switch6Mod(kCh1Val : std_logic_vector) return std_logic_vector is
    variable RetVal : std_logic_vector(kCh1Val'range) := (others => '0');
  begin
    -- Encoding for this switch is one-hot, so we just flip around the bits here.
    RetVal(kCh1Val'low + 2) := kCh1Val(kCh1Val'low + 0);
    RetVal(kCh1Val'low + 1) := kCh1Val(kCh1Val'low + 1);
    RetVal(kCh1Val'low + 0) := kCh1Val(kCh1Val'low + 2);
    return RetVal;
  end Rx2Switch6Mod;


end package body;
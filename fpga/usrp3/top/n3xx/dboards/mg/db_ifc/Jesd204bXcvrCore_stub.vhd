-- Copyright 1986-2015 Xilinx, Inc. All Rights Reserved.
-- --------------------------------------------------------------------------------
-- Tool Version: Vivado v.2015.4.2 (win64) Build 1494164 Fri Feb 26 04:18:56 MST 2016
-- Date        : Wed Jan 10 10:53:33 2018
-- Host        : djepson-lt running 64-bit major release  (build 9200)
-- Command     : write_vhdl -mode synth_stub -force -file ./Jesd204bXcvrCore_stub.vhd
-- Design      : Jesd204bXcvrCore
-- Purpose     : Stub declaration of top-level module interface
-- Device      : xc7z100ffg900-2
-- --------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity Jesd204bXcvrCore is
  Port ( 
    bBusReset : in STD_LOGIC;
    BusClk : in STD_LOGIC;
    ReliableClk40 : in STD_LOGIC;
    FpgaClk1x : in STD_LOGIC;
    FpgaClk2x : in STD_LOGIC;
    bFpgaClksStable : in STD_LOGIC;
    bRegPortInFlat : in STD_LOGIC_VECTOR ( 49 downto 0 );
    bRegPortOutFlat : out STD_LOGIC_VECTOR ( 33 downto 0 );
    aLmkSync : out STD_LOGIC;
    cSysRefFpgaLvds_p : in STD_LOGIC;
    cSysRefFpgaLvds_n : in STD_LOGIC;
    fSysRef : out STD_LOGIC;
    CaptureSysRefClk : in STD_LOGIC;
    JesdRefClk_p : in STD_LOGIC;
    JesdRefClk_n : in STD_LOGIC;
    bJesdRefClkPresent : out STD_LOGIC;
    aAdcRx_p : in STD_LOGIC_VECTOR ( 3 downto 0 );
    aAdcRx_n : in STD_LOGIC_VECTOR ( 3 downto 0 );
    aSyncAdcOut_n : out STD_LOGIC;
    aDacTx_p : out STD_LOGIC_VECTOR ( 3 downto 0 );
    aDacTx_n : out STD_LOGIC_VECTOR ( 3 downto 0 );
    aSyncDacIn_n : in STD_LOGIC;
    fAdc0DataFlat : out STD_LOGIC_VECTOR ( 31 downto 0 );
    fAdc1DataFlat : out STD_LOGIC_VECTOR ( 31 downto 0 );
    fDac0DataFlat : in STD_LOGIC_VECTOR ( 31 downto 0 );
    fDac1DataFlat : in STD_LOGIC_VECTOR ( 31 downto 0 );
    fAdcDataValid : out STD_LOGIC;
    fDacReadyForInput : out STD_LOGIC;
    aDacSync : out STD_LOGIC;
    aAdcSync : out STD_LOGIC
  );

end Jesd204bXcvrCore;

architecture stub of Jesd204bXcvrCore is
attribute syn_black_box : boolean;
attribute black_box_pad_pin : string;
attribute syn_black_box of stub : architecture is true;
attribute black_box_pad_pin of stub : architecture is "bBusReset,BusClk,ReliableClk40,FpgaClk1x,FpgaClk2x,bFpgaClksStable,bRegPortInFlat[49:0],bRegPortOutFlat[33:0],aLmkSync,cSysRefFpgaLvds_p,cSysRefFpgaLvds_n,fSysRef,CaptureSysRefClk,JesdRefClk_p,JesdRefClk_n,bJesdRefClkPresent,aAdcRx_p[3:0],aAdcRx_n[3:0],aSyncAdcOut_n,aDacTx_p[3:0],aDacTx_n[3:0],aSyncDacIn_n,fAdc0DataFlat[31:0],fAdc1DataFlat[31:0],fDac0DataFlat[31:0],fDac1DataFlat[31:0],fAdcDataValid,fDacReadyForInput,aDacSync,aAdcSync";
begin
end;

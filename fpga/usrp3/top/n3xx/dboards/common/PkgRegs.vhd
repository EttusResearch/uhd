--
-- Copyright 2018 Ettus Research, a National Instruments Company
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- This package contains functions for reading and writing N310 registers.

library ieee;
  use ieee.std_logic_1164.all;
  use ieee.numeric_std.all;


package PkgRegs is


  -- RegPort Type Definitions : ---------------------------------------------------------
  -- ------------------------------------------------------------------------------------

  constant kAddressWidth : integer := 16;

  subtype InterfaceData_t is std_logic_vector(31 downto 0);
  type RegDataAry_t is array (natural range <>) of InterfaceData_t;
  constant kRegPortDataZero : InterfaceData_t := (others => '0');

  -- The type of the signal used to communicate from the Interface
  -- component to the frameworks
  type RegPortIn_t is record
    Address : unsigned(kAddressWidth - 1 downto 0);
    Data    : InterfaceData_t;
    Rd      : boolean;                  -- Must be a one clock cycle pulse
    Wt      : boolean;                  -- Must be a one clock cycle pulse
  end record;

  -- The type of the signal used to communicate to the Interface
  -- component from the frameworks
  -- Ready is just the Ready signal from the Handshake component.
  -- Address in RegPortIn_t should be valid in the cycle where Data, DataValid,
  -- or Ready are being sampled by the bus communication interface.
  type RegPortOut_t is record
    Data      : InterfaceData_t;
    DataValid : boolean;                -- Must be a one clock cycle pulse
    Ready     : boolean;                -- Must be valid one clock after Wt assertion
  end record;

  -- Constants for the RegPort
  constant kRegPortInZero : RegPortIn_t := (
    Address => to_unsigned(0,kAddressWidth),
    Data => (others => '0'),
    Rd => false,
    Wt => false);

  constant kRegPortOutZero : RegPortOut_t := (
    Data => (others=>'0'),
    DataValid => false,
    Ready => true);



  -- Register Offset Types : ------------------------------------------------------------
  -- ------------------------------------------------------------------------------------

  -- Custom type for defining register spaces. Is it assumed that all defined register
  -- addresses for each space are kOffset <= Address < kOffset+kWidth. Therefore when
  -- Address equals kOffset+kWidth, we are not talking to this space but the space
  -- above it.
  type RegOffset_t is record
    kOffset : integer;
    kWidth  : integer;
  end record;

  constant kRegOffsetZero : RegOffset_t := (kOffset => 16#0#, kWidth => 16#04#);



  -- Access Functions : -----------------------------------------------------------------
  -- ------------------------------------------------------------------------------------

  -- Helper function to combine register ports on their way back upstream.
  function "+" (L, R : RegPortOut_t) return RegPortOut_t;

  function Mask(RegPortIn       : in RegPortIn_t;
                kRegisterOffset : in RegOffset_t) return RegPortIn_t;

  -- Helper functions to determine when a register is targeted by the RegPort. There
  -- are three groups: RegSelected, RegWrite, and RegRead. The latter two call
  -- RegSelected to determine if a register is targeted and being read or written.
  -- RegSelected is also overloaded to accommodate the RegOffset_t type.
  -- function RegSelected (RegPortIn      : RegPortIn_t;
                        -- RegisterOffset : RegOffset_t) return boolean;
  function RegSelected (RegOffset : integer;
                        RegPortIn : RegPortIn_t) return boolean;
  function RegWrite    (Address   : integer;
                        RegPortIn : RegPortIn_t) return boolean;
  function RegRead     (Address   : integer;
                        RegPortIn : RegPortIn_t) return boolean;

  function OrArray(ArrayIn : RegDataAry_t) return std_logic_vector;




  -- Flattening Functions : -------------------------------------------------------------
  -- ------------------------------------------------------------------------------------
  
  constant kFlatRegPortInSize  : natural := kAddressWidth +
                                            InterfaceData_t'length +
                                            2;

  subtype FlatRegPortIn_t is std_logic_vector(kFlatRegPortInSize-1 downto 0);

  constant kFlatRegPortOutSize : natural := InterfaceData_t'length +
                                            2;

  subtype FlatRegPortOut_t is std_logic_vector(kFlatRegPortOutSize-1 downto 0);

  function Flatten(Var : RegPortIn_t) return FlatRegPortIn_t;
  function Unflatten(Var  : FlatRegPortIn_t) return RegPortIn_t;

  function Flatten(Var : RegPortOut_t) return FlatRegPortOut_t;
  function Unflatten(Var  : FlatRegPortOut_t) return RegPortOut_t;




end PkgRegs;


package body PkgRegs is

  -- Combines RegPortOut_t types together
  function "+" (L, R : RegPortOut_t) return RegPortOut_t
  is
    variable ReturnVal : RegPortOut_t;
  begin
    ReturnVal           := kRegPortOutZero;
    ReturnVal.Data      := L.Data      or  R.Data;
    ReturnVal.DataValid := L.DataValid or  R.DataValid;
    ReturnVal.Ready     := L.Ready     and R.Ready;
    return ReturnVal;
  end function;


  -- This function lops off the portion of the register bus that is
  -- decoded in the InAddrSpace function in order to reduce the number of bits
  -- decoded by the register read logic. Also, the Rd and Wt strobes are gated
  -- as well.
  function Mask(RegPortIn       : in RegPortIn_t;
                kRegisterOffset : in RegOffset_t) return RegPortIn_t
  is
    variable RegPortInVar : RegPortIn_t;
    variable InSpace  : boolean := false;
  begin
    
    InSpace := (RegPortIn.Address >= kRegisterOffset.kOffset) and 
               (RegPortIn.Address <  kRegisterOffset.kOffset + kRegisterOffset.kWidth);
    
    -- Compare the most significant bits of the address bus downto the LSb
    -- that we just calculated.
    if InSpace then
      -- If in address space then allow Rd and Wt to assert
      RegPortInVar.Rd    := RegPortIn.Rd;
      RegPortInVar.Wt    := RegPortIn.Wt;
    else
      RegPortInVar.Rd    := kRegPortInZero.Rd;
      RegPortInVar.Wt    := kRegPortInZero.Wt;
    end if;

    RegPortInVar.Data    := RegPortIn.Data;
    RegPortInVar.Address := RegPortIn.Address - kRegisterOffset.kOffset;
    return RegPortInVar;
  end function Mask;


  -- Returns true when this chip is selected and the address matches the register.
  -- Note that RegOffset is divided by 4 before being compared against the register
  -- port Address value.
  function RegSelected (RegOffset : integer;
                        RegPortIn : RegPortIn_t) return boolean is
  begin
    return RegPortIn.Address = to_unsigned(RegOffset, RegPortIn.Address'length);
  end function RegSelected;

  -- Returns true when the register is being written.
  function RegWrite (Address   : integer;
                     RegPortIn : RegPortIn_t) return boolean is
  begin
    return RegSelected(Address, RegPortIn) and RegPortIn.Wt;
  end function RegWrite;

  -- Returns true when the register is being read.
  function RegRead (Address   : integer;
                    RegPortIn : RegPortIn_t) return boolean is
  begin
    return RegSelected(Address, RegPortIn) and RegPortIn.Rd;
  end function RegRead;

  -- Overloaded version of RegSelected for the RegOffset_t
  -- NOTE!!! Offset <= Address < Offset+Width
  -- Therefore, this function assumes that when Address = Offset+Width we are talking to
  -- a different register group than the one given in RegisterOffset.
  -- function RegSelected (RegPortIn      : RegPortIn_t;
                        -- RegisterOffset : RegOffset_t) return boolean is
  -- begin
    -- return (RegPortIn.Address >= to_unsigned(RegisterOffset.kOffset, RegPortIn.Address'length)) and
           -- (RegPortIn.Address <  to_unsigned(RegisterOffset.kOffset + RegisterOffset.kWidth, RegPortIn.Address'length));
  -- end function RegSelected;

  function OrArray(ArrayIn : RegDataAry_t) return std_logic_vector
  is
    variable ReturnVar : std_logic_vector(ArrayIn(ArrayIn'right)'range);
  begin
    ReturnVar := (others => '0');
    for i in ArrayIn'range loop
      ReturnVar := ReturnVar or ArrayIn(i);
    end loop;
    return ReturnVar;
  end function OrArray;


  function to_Boolean (s : std_ulogic) return boolean is
  begin
    return (To_X01(s)='1');
  end to_Boolean;

  function to_StdLogic(b : boolean) return std_ulogic is
  begin
    if b then
      return '1';
    else
      return '0';
    end if;
  end to_StdLogic;



  -----------------------------------------------------
  -- REG PORTS (FROM PkgCommunicationInterface)
  --
  -- subtype InterfaceData_t is std_logic_vector(31 downto 0);
  --
  -- constant kAddressWidth : positive := kAddressWidth - 2;
  --
  -- type RegPortIn_t is record
  --   Address : unsigned(kAddressWidth - 1 downto 0);
  --   Data    : InterfaceData_t;
  --   Rd      : boolean;                  -- Must be a one clock cycle pulse
  --   Wt      : boolean;                  -- Must be a one clock cycle pulse
  -- end record;

  function Flatten(Var : RegPortIn_t) return FlatRegPortIn_t is
    variable Index  : natural;
    variable RetVar : FlatRegPortIn_t;
  begin
    Index := 0;
    RetVar(Index) := to_StdLogic(Var.Wt); Index := Index + 1;
    RetVar(Index) := to_StdLogic(Var.Rd); Index := Index + 1;
    RetVar(Index + Var.Data'length - 1 downto Index) := std_logic_vector(Var.Data);
      Index := Index + Var.Data'length;
    RetVar(Index + Var.Address'length - 1 downto Index) := std_logic_vector(Var.Address);
      Index := Index + Var.Address'length;

    return RetVar;
  end function Flatten;

  function Unflatten(Var  : FlatRegPortIn_t) return RegPortIn_t is
    variable Index  : natural;
    variable RetVal : RegPortIn_t;
  begin
    Index := 0;
    RetVal.Wt      := to_Boolean(Var(Index)); Index := Index + 1;
    RetVal.Rd      := to_Boolean(Var(Index)); Index := Index + 1;
    RetVal.Data    := InterfaceData_t(Var(Index + RetVal.Data'length - 1 downto Index));
      Index := Index + RetVal.Data'length;
    RetVal.Address := unsigned(Var(Index + RetVal.Address'length - 1 downto Index));
      Index := Index + RetVal.Address'length;

    return RetVal;
  end function Unflatten;

  -- type RegPortOut_t is record
  --   Data      : InterfaceData_t;
  --   DataValid : boolean;                -- Must be a one clock cycle pulse
  --   Ready     : boolean;                -- Must be valid one clock after Wt assertion
  -- end record;

  function Flatten(Var : RegPortOut_t) return FlatRegPortOut_t is
    variable Index  : natural;
    variable RetVar : FlatRegPortOut_t;
  begin
    Index := 0;
    RetVar(Index) := to_StdLogic(Var.Ready);     Index := Index + 1;
    RetVar(Index) := to_StdLogic(Var.DataValid); Index := Index + 1;
    RetVar(Index + Var.Data'length - 1 downto Index) := std_logic_vector(Var.Data);
      Index := Index + Var.Data'length;

    return RetVar;
  end function Flatten;

  function Unflatten(Var  : FlatRegPortOut_t) return RegPortOut_t is
    variable Index  : natural;
    variable RetVal : RegPortOut_t;
  begin
    Index := 0;
    RetVal.Ready     := to_Boolean(Var(Index)); Index := Index + 1;
    RetVal.DataValid := to_Boolean(Var(Index)); Index := Index + 1;
    RetVal.Data      := InterfaceData_t(Var(Index + RetVal.Data'length - 1 downto Index));
      Index := Index + RetVal.Data'length;

    return RetVal;
  end function Unflatten;



end PkgRegs;

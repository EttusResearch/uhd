-------------------------------------------------------------------------------
--
-- File: TopCpld.vhd
-- Author: Daniel Jepson
-- Original Project: N310
-- Date: 24 October 2016
--
-------------------------------------------------------------------------------
-- Copyright 2016-2017 Ettus Research, A National Instruments Company
-- SPDX-License-Identifier: GPL-3.0
-------------------------------------------------------------------------------
--
-- Purpose:
--
-- Top level file for the Magnesium CPLD.
--
-- This file instantiates two SPI slave ports. One slave port comes from the PS
-- of the motherboard Zynq. It has three slave select pins, mapped as follows:
--   sPlsSpiLe     = CPLD Internal Registers
--   aPsSpiAddr(0) = LMK Endpoint
--   aPsSpiAddr(1) = Phase DAC Endpoint
--
-- The other slave port comes from the PL of the motherboard Zynq. It also has
-- three slave select pins:
--   lPlSpiLe      = CPLD Internal Registers
--   aPlSpiAddr(0) = TX Lowband LO
--   aPlSpiAddr(1) = RX Lowband LO
--
-- The final address line for the PL slave is used as a passthrough for the LMK
-- SYNC pin.
--
--
-- For either SPI interface, the CPLD has internal registers that can be addressed
-- whenever the appropriate slave select is driven asserted. These register groups
-- are completely independent from one another, meaning the PS SPI interface cannot
-- access the PL registers, and vice-versa.
--
-- See the register interface XML at the bottom of this file for details on how
-- each SPI port is expected to be driven, and for the register maps for the PS
-- and PL slaves.
--
--
-- BUMPING THE REVISION:
-- In PkgSetup the kMinorRev and kMajorRev are defined. Whenever a change
-- is made to the CPLD, no matter how small, bump the kMinorRev value. If this change
-- breaks compatibility with current HW or SW drivers, increment the kMajorRev value
-- and reset the kMinorRev to zero. Similarly, there is a constant to define the build
-- code, kBuildCode. Currently this is simply the year, month, day, and hour the CPLD is
-- built, but could be user-definable.
--
--
-------------------------------------------------------------------------------

library ieee;
  use ieee.std_logic_1164.all;
  use ieee.numeric_std.all;
  use ieee.math_real.all;

library work;
  use work.PkgMgCpld.all;
  use work.PkgSetup.all;

entity TopCpld is
  port(

    -- SPI Port Incoming from FPGA --
    PlSpiSck               : in  std_logic;
    lPlSpiSdi              : in  std_logic;
    lPlSpiSdo              : out std_logic;
    lPlSpiLe               : in  std_logic;
    aPlSpiAddr             : in  std_logic_vector(2 downto 0);

    -- SPI Port Incoming from PS --
    PsSpiSck               : in  std_logic;
    sPsSpiSdi              : in  std_logic;
    sPsSpiSdo              : out std_logic;
    sPsSpiLe               : in  std_logic;
    aPsSpiAddr             : in  std_logic_vector(1 downto 0);

    -- ATR bits from FPGA --
    aAtrRx1                : in  std_logic;
    aAtrRx2                : in  std_logic;
    aAtrTx1                : in  std_logic;
    aAtrTx2                : in  std_logic;

    -- Ch 1 TX (15 bits) --
    aCh1LedTx              : out std_logic;

    aCh1TxPaEn             : out std_logic;
    aCh1TxAmpEn            : out std_logic;
    aCh1TxMixerEn          : out std_logic;

    aCh1TxSw1              : out std_logic_vector(1 downto 0);
    aCh1TxSw2              : out std_logic_vector(3 downto 0);
    aCh1TxSw3              : out std_logic;
    aCh1TxSw4              : out std_logic_vector(1 downto 0);
    aCh1TxSw5              : out std_logic_vector(1 downto 0);

    -- Ch 1 RX (29 bits) --
    aCh1LedRx              : out std_logic;
    aCh1LedRx2             : out std_logic;

    aCh1RxAmpEn            : out std_logic;
    aCh1RxMixerEn          : out std_logic;
    aCh1RxLna1En           : out std_logic;
    aCh1RxLna2En           : out std_logic;
    aCh1SwTrx              : out std_logic_vector(1 downto 0);

    aCh1RxSw1              : out std_logic_vector(1 downto 0);
    aCh1RxSw2              : out std_logic_vector(1 downto 0);
    aCh1RxSw3              : out std_logic_vector(2 downto 0);
    aCh1RxSw4              : out std_logic_vector(2 downto 0);
    aCh1RxSw5              : out std_logic_vector(3 downto 0);
    aCh1RxSw6              : out std_logic_vector(2 downto 0);
    aCh1RxSw7              : out std_logic_vector(1 downto 0);
    aCh1RxSw8              : out std_logic_vector(1 downto 0);

    -- Ch 2 TX --
    aCh2LedTx              : out std_logic;

    aCh2TxPaEn             : out std_logic;
    aCh2TxAmpEn            : out std_logic;
    aCh2TxMixerEn          : out std_logic;

    aCh2TxSw1              : out std_logic_vector(1 downto 0);
    aCh2TxSw2              : out std_logic_vector(3 downto 0);
    aCh2TxSw3              : out std_logic;
    aCh2TxSw4              : out std_logic_vector(1 downto 0);
    aCh2TxSw5              : out std_logic_vector(1 downto 0);

    -- Ch 2 RX --
    aCh2LedRx              : out std_logic;
    aCh2LedRx2             : out std_logic;

    aCh2RxAmpEn            : out std_logic;
    aCh2RxMixerEn          : out std_logic;
    aCh2RxLna1En           : out std_logic;
    aCh2RxLna2En           : out std_logic;
    aCh2SwTrx              : out std_logic_vector(1 downto 0);

    aCh2RxSw1              : out std_logic_vector(1 downto 0);
    aCh2RxSw2              : out std_logic_vector(1 downto 0);
    aCh2RxSw3              : out std_logic_vector(2 downto 0);
    aCh2RxSw4              : out std_logic_vector(2 downto 0);
    aCh2RxSw5              : out std_logic_vector(3 downto 0);
    aCh2RxSw6              : out std_logic_vector(2 downto 0);
    aCh2RxSw7              : out std_logic_vector(1 downto 0);
    aCh2RxSw8              : out std_logic_vector(1 downto 0);

    -- LMK --
    aLmkSpiSdio            : out std_logic;
    aLmkSpiSck             : out std_logic;
    aLmkSpiCs_n            : out std_logic;
    aLmkClkinSel           : in  std_logic_vector(0 downto 0); -- SDO
    aLmkSync               : out std_logic; -- direct connect to aPlSpiAddr(2)

    -- Phase DAC --
    aDacDin                : out std_logic;
    aDacSync_n             : out std_logic;
    aDacSck                : out std_logic;
    aVcxoCtrl              : out std_logic; -- @PS-REG-WR

    -- RX and TX LOs -- (timed)
    aLoSpiSync             : in  std_logic; -- Clock! (unused atm, only for reclocking
    aRxLoSck               : out std_logic; -- the SPI bus if needed for sync)
    aRxLoDin               : out std_logic;
    aRxLoCs_n              : out std_logic;
    aRxLoMuxOut            : in  std_logic;
    aRxLoLockDetect        : in  std_logic; -- @PS-REG-RD
    aTxLoSck               : out std_logic;
    aTxLoDin               : out std_logic;
    aTxLoCs_n              : out std_logic;
    aTxLoMuxOut            : in  std_logic;
    aTxLoLockDetect        : in  std_logic; -- @PS-REG-RD

    -- Mykonos Interface --
    aMkReset_n             : out std_logic; -- @PS-REG-WR
    aMkRx1En               : out std_logic;
    aMkRx2En               : out std_logic;
    aMkTx1En               : out std_logic;
    aMkTx2En               : out std_logic

  );
end TopCpld;


architecture RTL of TopCpld is

  -- PS MOSI
  signal sCpldPsSpiActive : boolean;
  signal sPsMosiIndex  : unsigned(integer(ceil(log2(real(kTotalWidth)))) downto 0);
  signal sPsMosiBuffer : InterfaceData_t := (others => '0');
  signal sPsRd : boolean := false;
  signal sPsRegAddr : unsigned(kAddrWidth-1 downto 0) := (others => '0');

  -- PS MISO
  signal sPsCpldMiso : std_logic;
  signal sPsMisoIndex  : unsigned(integer(ceil(log2(real(kTotalWidth)))) downto 0);
  signal sPsMisoBuffer : std_logic_vector(kTotalWidth-1 downto 0);

  -- PS Register Signals
  signal aRxLoLockDetect_ms, sRxLoLockDetect,
         aTxLoLockDetect_ms, sTxLoLockDetect : std_logic := '0';
  signal sReset : boolean := false;
  signal sScratchVal : InterfaceData_t := (others => '0');
  signal sVcxoControl : std_logic := '1';
  signal sMykonosReset : std_logic := '0';

  -- PL MOSI
  signal lCpldPlSpiActive : boolean;
  signal lPlMosiIndex  : unsigned(integer(ceil(log2(real(kTotalWidth)))) downto 0);
  signal lPlMosiBuffer : InterfaceData_t := (others => '0');
  signal lPlRd : boolean := false;
  signal lPlRegAddr : unsigned(kAddrWidth-1 downto 0) := (others => '0');

  -- PL MISO
  signal lPlCpldMiso : std_logic;
  signal lPlMisoIndex  : unsigned(integer(ceil(log2(real(kTotalWidth)))) downto 0);
  signal lPlMisoBuffer : std_logic_vector(kTotalWidth-1 downto 0);

  -- PL Register Signals
  signal lScratchVal : InterfaceData_t := (others => '0');
  signal lReset : boolean := false;

  -- See PkgSetup for each Default definition.
  signal lTxCh1IdleReg   : InterfaceData_t := kTxChDefault;
  signal lTxCh2IdleReg   : InterfaceData_t := kTxChDefault;

  signal lTxCh1TxOnReg   : InterfaceData_t := kTxChDefaultRun;
  signal lTxCh2TxOnReg   : InterfaceData_t := kTxChDefaultRun;

  signal lRxCh1_0IdleReg : InterfaceData_t := kRxChDefault0;
  signal lRxCh1_1IdleReg : InterfaceData_t := kRxChDefault1;
  signal lRxCh2_0IdleReg : InterfaceData_t := kRxChDefault0;
  signal lRxCh2_1IdleReg : InterfaceData_t := kRxChDefault1;

  signal lRxCh1_0RxOnReg : InterfaceData_t := kRxChDefault0Run;
  signal lRxCh1_1RxOnReg : InterfaceData_t := kRxChDefault1Run;
  signal lRxCh2_0RxOnReg : InterfaceData_t := kRxChDefault0Run;
  signal lRxCh2_1RxOnReg : InterfaceData_t := kRxChDefault1Run;

  signal lTxCh1   : InterfaceData_t;
  signal lTxCh2   : InterfaceData_t;
  signal lRxCh1_0 : InterfaceData_t;
  signal lRxCh1_1 : InterfaceData_t;
  signal lRxCh2_0 : InterfaceData_t;
  signal lRxCh2_1 : InterfaceData_t;

begin


  -- Direct Pass-through Pins : ---------------------------------------------------------
  -- ------------------------------------------------------------------------------------

  -- LMK SYNC assignment for direct passthrough to LMK from SPI Addr line.
  aLmkSync <= aPlSpiAddr(2);



  -- PS SPI Interface : -----------------------------------------------------------------
  -- Composed of a few modules:
  --  1) PsMosiIndex - generate pointer for MOSI Buffer
  --  2) PsMosiBuffer - actually implement the buffer, only when the CPLD is targeted.
  --  3) PsMosiProcessing - process the MOSI data: sort into Rd/!Wt, Address, and Data.
  --     This process works on the falling edge of the clock to register the pieces
  --     of the MOSI packet as they are complete. The final falling edge registers
  --     the data into the individual registers, so it is critical that the clock idle
  --     LOW after the transaction is complete such that this final falling edge occurs.
  --  4) PsMisoBuffer - generate pointer for the MISO buffer. The buffer itself is
  --     completely async.
  --  5) PsMisoBufferMux - Mux all the register data back into the MISO buffer.
  --  6) StatusSynchronizer - double-synchronizers for status bits from the LMK and LOs.
  -- ------------------------------------------------------------------------------------

  -- Decode the PS SPI Address bits... which are actually going to be used as individual
  -- chip selects coming from the PS.
  sCpldPsSpiActive <= sPsSpiLe = '0';
  aLmkSpiCs_n      <= aPsSpiAddr(0);
  aDacSync_n       <= aPsSpiAddr(1);

  -- Assign the remainder of the SPI lines to the LMK and DAC.
  aLmkSpiSck  <= PsSpiSck;
  aLmkSpiSdio <= sPsSpiSdi;
  aDacSck     <= PsSpiSck;
  aDacDin     <= sPsSpiSdi;

  -- Output mux for data back to the FPGA (PS core). The LMK and CPLD are the only
  -- endpoints that have readback enabled.
  sPsSpiSdo <= aLmkClkinSel(0) when aPsSpiAddr(0) = '0' else
               sPsCpldMiso;



  -- Use the LE signal (Cs_n) as the asynchronous reset to the shift register counter.
  -- LE will hold the counter in reset until this endpoint is targeted, when it will
  -- release the reset (long before the clock toggles) and allow the shift operation
  -- to begin.
  --
  -- !!! SAFE COUNTER STARTUP!!!
  -- This counter starts safely from reset because the PsSpiSck will not start toggling
  -- until long after the asynchronous reset (sCpldPsSpiActive) de-asserts. Similarly,
  -- the reset will only assert long after the last clock edge is received.
  PsMosiIndex : process(PsSpiSck, sCpldPsSpiActive)
  begin
    if not sCpldPsSpiActive then
      sPsMosiIndex <= (others => '0');
    elsif rising_edge(PsSpiSck) then
      sPsMosiIndex <= sPsMosiIndex + 1;
    end if;
  end process PsMosiIndex;


  -- Shift in SDI (MOSI) data from the PS on the rising edge of the clock. Only use
  -- synchronous resets from here on out.
  PsMosiBuffer : process(PsSpiSck)
  begin
    if rising_edge(PsSpiSck) then
      if sReset then
        sPsMosiBuffer <= (others => '0');
      else
        if sCpldPsSpiActive then
          sPsMosiBuffer <= sPsMosiBuffer(sPsMosiBuffer'high-1 downto 0) & sPsSpiSdi; -- left shift
        end if;
      end if;
    end if;
  end process PsMosiBuffer;


  -- As portions of the command and data packets become available, register them here
  -- using the falling edge of the PS SPI clock.
  PsMosiProcessing : process(PsSpiSck)
  begin
    if falling_edge(PsSpiSck) then
      if sReset then
        -- sReset is intentionally self-clearing. It clears on the first falling edge of
        -- the next SPI transaction after it is set. Logic on the first rising edge of
        -- that next transaction is therefore held in reset. This will not matter
        -- as long as SW follows the recommended reset procedure (writing a '1' to reset
        -- then writing a '0'), since the first bit of the transaction is '0' for a
        -- write operation.
        sReset        <= false;
        sScratchVal   <= (others => '0');
        sVcxoControl  <= '1';
        sMykonosReset <= '0';
        sPsRd         <= false;
        sPsRegAddr    <= (others => '0');
      else
        -- After the first bit is captured, we can determine if it is a write or read.
        if (sPsMosiIndex = (kRdWtWidth)) then
          sPsRd <= sPsMosiBuffer(0) = '1';
        end if;

        -- After the entire command word is captured, the address is ready for capture.
        if (sPsMosiIndex = (kAddrWidth + kRdWtWidth)) then
          sPsRegAddr <= unsigned(sPsMosiBuffer(kAddrWidth - 1 downto 0));
        end if;

        -- And finally after the entire transaction is complete we can save off the data
        -- on the final falling edge of the SPI clock into it's appropriate place, based
        -- off the address value captured above.
        if (sPsMosiIndex = kTotalWidth) and (not sPsRd) then

          -- ----------------------------------------------------------------------------
          -- Assign writable register values here! --------------------------------------
          -- ----------------------------------------------------------------------------
          if (sPsRegAddr = kScratch) then
            sScratchVal <= sPsMosiBuffer;
          end if;

          if (sPsRegAddr = kCpldControl) then
            sReset <= sPsMosiBuffer(kCpldReset) = '1';
          end if;

          if (sPsRegAddr = kLmkControl) then
            sVcxoControl <= sPsMosiBuffer(kVcxoControl);
          end if;

          if (sPsRegAddr = kMykonosControl) then
            sMykonosReset <= sPsMosiBuffer(kMykonosReset);
          end if;

        end if;
      end if;
    end if;
  end process PsMosiProcessing;


  -- Send MISO back to FPGA (PS) on the falling edge as well.
  --
  -- !!! SAFE COUNTER STARTUP!!!
  -- This counter starts safely from reset because the PsSpiSck will not start toggling
  -- until long after the asynchronous reset (sCpldPsSpiActive) de-asserts. Similarly,
  -- the reset will only assert long after the last clock edge is received.
  PsMisoBuffer : process(PsSpiSck, sCpldPsSpiActive)
  begin
    if not sCpldPsSpiActive then
      sPsMisoIndex <= to_unsigned(kTotalWidth-1, sPsMisoIndex'length);
    elsif falling_edge(PsSpiSck) then
      if sPsMisoIndex > 0 then
        sPsMisoIndex <= sPsMisoIndex - 1;
      end if;
    end if;
  end process PsMisoBuffer;

  sPsCpldMiso <= sPsMisoBuffer(to_integer(sPsMisoIndex));


  -- Mux the register data from the CPLD back to the FPGA.
  PsMisoBufferMux : process(sPsRegAddr, sScratchVal, sVcxoControl,
                            sTxLoLockDetect, sRxLoLockDetect, sMykonosReset)
  begin
    sPsMisoBuffer <= (others => '0');
    case to_integer(sPsRegAddr) is
      when kSignatureReg       => sPsMisoBuffer(kDataWidth-1 downto 0) <= kSignature;
      when kMinorRevReg        => sPsMisoBuffer(kDataWidth-1 downto 0) <= kMinorRev;
      when kMajorRevReg        => sPsMisoBuffer(kDataWidth-1 downto 0) <= kMajorRev;
      when kBuildCodeLSB       => sPsMisoBuffer(kDataWidth-1 downto 0) <= kBuildCode(15 downto 0);
      when kBuildCodeMSB       => sPsMisoBuffer(kDataWidth-1 downto 0) <= kBuildCode(31 downto 16);
      when kScratch            => sPsMisoBuffer(kDataWidth-1 downto 0) <= sScratchVal;
      when kLmkControl         => sPsMisoBuffer(kVcxoControl)    <= sVcxoControl;
      when kLoStatus           => sPsMisoBuffer(kTxLoLockDetect) <= sTxLoLockDetect;
                                  sPsMisoBuffer(kRxLoLockDetect) <= sRxLoLockDetect;
      when kMykonosControl     => sPsMisoBuffer(kMykonosReset)   <= sMykonosReset;
      when others              => sPsMisoBuffer(kDataWidth-1 downto 0) <= (others => '0');
    end case;
  end process PsMisoBufferMux;


  -- Double-synchronize the async inputs to the PS clock domain. However, this clock
  -- isn't toggling all the time. Whenever it is toggling, let's capture these bits.
  StatusSynchronizer : process(PsSpiSck)
  begin
    if rising_edge(PsSpiSck) then
      aRxLoLockDetect_ms <= aRxLoLockDetect;
      sRxLoLockDetect    <= aRxLoLockDetect_ms;

      aTxLoLockDetect_ms <= aTxLoLockDetect;
      sTxLoLockDetect    <= aTxLoLockDetect_ms;
    end if;
  end process;


  -- PS SPI locals to outputs.
  aVcxoCtrl  <= sVcxoControl;
  aMkReset_n <= not sMykonosReset;



  -- PL SPI Interface : -----------------------------------------------------------------
  -- Composed of a few modules:
  --  1) PlMosiIndex - generate pointer for MOSI Buffer
  --  2) PlMosiBuffer - actually implement the buffer, only when the CPLD is targeted.
  --  3) PlMosiProcessing - process the MOSI data: sort into Rd/!Wt, Address, and Data.
  --     This process works on the falling edge of the clock to register the pieces
  --     of the MOSI packet as they are complete. The final falling edge registers
  --     the data into the individual registers, so it is critical that the clock idle
  --     LOW after the transaction is complete such that this final falling edge occurs.
  --  4) PlMisoBuffer - generate pointer for the MISO buffer. The buffer itself is
  --     completely async.
  --  5) PlMisoBufferMux - Mux all the register data back into the MISO buffer.
  --  6) StatusSynchronizer - double-synchronizers for status bits from the LMK and LOs.
  -- ------------------------------------------------------------------------------------

  -- Decode the PL SPI Address bits... which are actually going to be used as individual
  -- chip selects coming from the PL.
  lCpldPlSpiActive <= lPlSpiLe = '0';
  aTxLoCs_n        <= aPlSpiAddr(0);
  aRxLoCs_n        <= aPlSpiAddr(1);

  -- Assign the remainder of the SPI lines to the LOs.
  aRxLoSck  <= PlSpiSck;
  aRxLoDin  <= lPlSpiSdi;
  aTxLoSck  <= PlSpiSck;
  aTxLoDin  <= lPlSpiSdi;

  -- Output mux for data back to the FPGA (PL core). The LMK and CPLD are the only
  -- endpoints that have readback enabled.
  lPlSpiSdo <= aTxLoMuxOut when aPlSpiAddr(0) = '0' else
               aRxLoMuxOut when aPlSpiAddr(1) = '0' else
               lPlCpldMiso;



  -- Use the LE signal (Cs_n) as the asynchronous reset to the shift register counter.
  -- LE will hold the counter in reset until this endpoint is targeted, when it will
  -- release the reset (long before the clock toggles) and allow the shift operation
  -- to begin.
  --
  -- !!! SAFE COUNTER STARTUP!!!
  -- This counter starts safely from reset because the PlSpiSck will not start toggling
  -- until long after the asynchronous reset (lCpldPlSpiActive) de-asserts. Similarly,
  -- the reset will only assert long after the last clock edge is received.
  PlMosiIndex : process(PlSpiSck, lCpldPlSpiActive)
  begin
    if not lCpldPlSpiActive then
      lPlMosiIndex <= (others => '0');
    elsif rising_edge(PlSpiSck) then
      lPlMosiIndex <= lPlMosiIndex + 1;
    end if;
  end process PlMosiIndex;


  -- Shift in SDI (MOSI) data from the PL on the rising edge of the clock. Only use
  -- synchronous resets from here on out.
  PlMosiBuffer : process(PlSpiSck)
  begin
    if rising_edge(PlSpiSck) then
      if lReset then
        lPlMosiBuffer <= (others => '0');
      else
        if lCpldPlSpiActive then
          lPlMosiBuffer <= lPlMosiBuffer(lPlMosiBuffer'high-1 downto 0) & lPlSpiSdi; -- left shift
        end if;
      end if;
    end if;
  end process PlMosiBuffer;


  -- As portions of the command and data packets become available, register them here
  -- using the falling edge of the PL SPI clock.
  PlMosiProcessing : process(PlSpiSck)
  begin
    if falling_edge(PlSpiSck) then
      if lReset then
        -- lReset is intentionally self-clearing. It clears on the first falling edge of
        -- the next SPI transaction after it is set. Logic on the first rising edge of
        -- that next transaction is therefore held in reset. This will not matter
        -- as long as SW follows the recommended reset procedure (writing a '1' to reset
        -- then writing a '0'), since the first bit of the transaction is '0' for a
        -- write operation.
        lReset         <= false;
        lScratchVal    <= (others => '0');
        lTxCh1IdleReg   <= kTxChDefault;
        lTxCh1TxOnReg   <= kTxChDefault;
        lTxCh2IdleReg   <= kTxChDefault;
        lTxCh2TxOnReg   <= kTxChDefault;
        lRxCh1_0IdleReg <= kRxChDefault0;
        lRxCh1_1IdleReg <= kRxChDefault1;
        lRxCh1_0RxOnReg <= kRxChDefault0;
        lRxCh1_1RxOnReg <= kRxChDefault1;
        lRxCh2_0IdleReg <= kRxChDefault0;
        lRxCh2_1IdleReg <= kRxChDefault1;
        lRxCh2_0RxOnReg <= kRxChDefault0;
        lRxCh2_1RxOnReg <= kRxChDefault1;
        lPlRd         <= false;
        lPlRegAddr    <= (others => '0');
      else
        -- After the first bit is captured, we can determine if it is a write or read.
        if (lPlMosiIndex = (kRdWtWidth)) then
          lPlRd <= lPlMosiBuffer(0) = '1';
        end if;

        -- After the entire command word is captured, the address is ready for capture.
        if (lPlMosiIndex = (kAddrWidth + kRdWtWidth)) then
          lPlRegAddr <= unsigned(lPlMosiBuffer(kAddrWidth - 1 downto 0));
        end if;

        -- And finally after the entire transaction is complete we can save off the data
        -- on the final falling edge of the SPI clock into it's appropriate place, based
        -- off the address value captured above.
        if (lPlMosiIndex = kTotalWidth) and (not lPlRd) then

          -- ----------------------------------------------------------------------------
          -- Assign writable register values here! --------------------------------------
          -- ----------------------------------------------------------------------------
          if (lPlRegAddr = kPlScratch) then
            lScratchVal <= lPlMosiBuffer;
          end if;

          if (lPlRegAddr = kPlCpldControl) then
            lReset <= lPlMosiBuffer(kCpldReset) = '1';
          end if;

          if (lPlRegAddr = kTxCh1_Idle) then
            lTxCh1IdleReg <= lPlMosiBuffer;
          end if;
          if (lPlRegAddr = kTxCh1_TxOn) then
            lTxCh1TxOnReg <= lPlMosiBuffer;
          end if;
          if (lPlRegAddr = kTxCh2_Idle) then
            lTxCh2IdleReg <= lPlMosiBuffer;
          end if;
          if (lPlRegAddr = kTxCh2_TxOn) then
            lTxCh2TxOnReg <= lPlMosiBuffer;
          end if;

          if (lPlRegAddr = kRxCh1_0_Idle) then
            lRxCh1_0IdleReg <= lPlMosiBuffer;
          end if;
          if (lPlRegAddr = kRxCh1_1_Idle) then
            lRxCh1_1IdleReg <= lPlMosiBuffer;
          end if;
          if (lPlRegAddr = kRxCh1_0_RxOn) then
            lRxCh1_0RxOnReg <= lPlMosiBuffer;
          end if;
          if (lPlRegAddr = kRxCh1_1_RxOn) then
            lRxCh1_1RxOnReg <= lPlMosiBuffer;
          end if;

          if (lPlRegAddr = kRxCh2_0_Idle) then
            lRxCh2_0IdleReg <= lPlMosiBuffer;
          end if;
          if (lPlRegAddr = kRxCh2_1_Idle) then
            lRxCh2_1IdleReg <= lPlMosiBuffer;
          end if;
          if (lPlRegAddr = kRxCh2_0_RxOn) then
            lRxCh2_0RxOnReg <= lPlMosiBuffer;
          end if;
          if (lPlRegAddr = kRxCh2_1_RxOn) then
            lRxCh2_1RxOnReg <= lPlMosiBuffer;
          end if;

        end if;
      end if;
    end if;
  end process PlMosiProcessing;


  -- Send MISO back to FPGA (PL) on the falling edge as well.
  --
  -- !!! SAFE COUNTER STARTUP!!!
  -- This counter starts safely from reset because the PlSpiSck will not start toggling
  -- until long after the asynchronous reset (lCpldPlSpiActive) de-asserts. Similarly,
  -- the reset will only assert long after the last clock edge is received.
  PlMisoBuffer : process(PlSpiSck, lCpldPlSpiActive)
  begin
    if not lCpldPlSpiActive then
      lPlMisoIndex <= to_unsigned(kTotalWidth-1, lPlMisoIndex'length);
    elsif falling_edge(PlSpiSck) then
      if lPlMisoIndex > 0 then
        lPlMisoIndex <= lPlMisoIndex - 1;
      end if;
    end if;
  end process PlMisoBuffer;

  lPlCpldMiso <= lPlMisoBuffer(to_integer(lPlMisoIndex));


  -- Mux the register data from the CPLD back to the FPGA.
  PlMisoBufferMux : process(lPlRegAddr, lScratchVal, lTxCh1IdleReg, lTxCh1TxOnReg,
                            lTxCh2IdleReg, lTxCh2TxOnReg, lRxCh1_0IdleReg,
                            lRxCh1_1IdleReg, lRxCh1_0RxOnReg, lRxCh1_1RxOnReg,
                            lRxCh2_0IdleReg, lRxCh2_1IdleReg, lRxCh2_0RxOnReg,
                            lRxCh2_1RxOnReg)
  begin
    lPlMisoBuffer <= (others => '0');
    case to_integer(lPlRegAddr) is
      when kPlScratch     => lPlMisoBuffer(kDataWidth-1 downto 0) <= lScratchVal;
      when kTxCh1_Idle    => lPlMisoBuffer(kDataWidth-1 downto 0) <= lTxCh1IdleReg;
      when kTxCh1_TxOn    => lPlMisoBuffer(kDataWidth-1 downto 0) <= lTxCh1TxOnReg;
      when kTxCh2_Idle    => lPlMisoBuffer(kDataWidth-1 downto 0) <= lTxCh2IdleReg;
      when kTxCh2_TxOn    => lPlMisoBuffer(kDataWidth-1 downto 0) <= lTxCh2TxOnReg;
      when kRxCh1_0_Idle  => lPlMisoBuffer(kDataWidth-1 downto 0) <= lRxCh1_0IdleReg;
      when kRxCh1_1_Idle  => lPlMisoBuffer(kDataWidth-1 downto 0) <= lRxCh1_1IdleReg;
      when kRxCh1_0_RxOn  => lPlMisoBuffer(kDataWidth-1 downto 0) <= lRxCh1_0RxOnReg;
      when kRxCh1_1_RxOn  => lPlMisoBuffer(kDataWidth-1 downto 0) <= lRxCh1_1RxOnReg;
      when kRxCh2_0_Idle  => lPlMisoBuffer(kDataWidth-1 downto 0) <= lRxCh2_0IdleReg;
      when kRxCh2_1_Idle  => lPlMisoBuffer(kDataWidth-1 downto 0) <= lRxCh2_1IdleReg;
      when kRxCh2_0_RxOn  => lPlMisoBuffer(kDataWidth-1 downto 0) <= lRxCh2_0RxOnReg;
      when kRxCh2_1_RxOn  => lPlMisoBuffer(kDataWidth-1 downto 0) <= lRxCh2_1RxOnReg;
      when others         => lPlMisoBuffer(kDataWidth-1 downto 0) <= (others => '0');
    end case;
  end process PlMisoBufferMux;


  -- Use the ATR bits to mux the output values.
  lTxCh1 <= lTxCh1IdleReg when aAtrTx1 = '0' else lTxCh1TxOnReg;
  lTxCh2 <= lTxCh2IdleReg when aAtrTx2 = '0' else lTxCh2TxOnReg;

  lRxCh1_0 <= lRxCh1_0IdleReg when aAtrRx1 = '0' else lRxCh1_0RxOnReg;
  lRxCh1_1 <= lRxCh1_1IdleReg when aAtrRx1 = '0' else lRxCh1_1RxOnReg;
  lRxCh2_0 <= lRxCh2_0IdleReg when aAtrRx2 = '0' else lRxCh2_0RxOnReg;
  lRxCh2_1 <= lRxCh2_1IdleReg when aAtrRx2 = '0' else lRxCh2_1RxOnReg;

  -- PL SPI locals to outputs. All the register values are set for Channel 1. Channel 2
  -- values are mixed around here in order for the same register settings to work for
  -- Channel 1 and Channel 2, even though Ch2 switch configuration is different in HW.
  aCh1LedTx        <= lTxCh1(kCh1TxLed);
  aCh1TxPaEn       <= lTxCh1(kCh1TxPaEn);
  aCh1TxAmpEn      <= lTxCh1(kCh1TxAmpEn);
  aCh1TxMixerEn    <= lTxCh1(kCh1TxMixerEn);
  aCh1TxSw1        <= lTxCh1(kCh1TxSw1Msb downto kCh1TxSw1);
  aCh1TxSw2        <= lTxCh1(kCh1TxSw2Msb downto kCh1TxSw2);
  aCh1TxSw3        <= lTxCh1(kCh1TxSw3);
  aCh1TxSw4        <= "01" when lTxCh1(kCh1TxLowbandMixerPathSelect) = '1' else "10";
  aCh1TxSw5        <= "10" when lTxCh1(kCh1TxLowbandMixerPathSelect) = '1' else "01";
  aCh1SwTrx        <= lTxCh1(kCh1SwTrxMsb downto kCh1SwTrx);
  aMkTx1En         <= lTxCh1(kCh1MykEnTx);

  aCh2LedTx        <= lTxCh2(kCh1TxLed);
  aCh2TxPaEn       <= lTxCh2(kCh1TxPaEn);
  aCh2TxAmpEn      <= lTxCh2(kCh1TxAmpEn);
  aCh2TxMixerEn    <= lTxCh2(kCh1TxMixerEn);
  aCh2TxSw1        <= lTxCh2(kCh1TxSw1Msb downto kCh1TxSw1);
  aCh2TxSw2        <= Tx2Switch2Mod(lTxCh2(kCh1TxSw2Msb downto kCh1TxSw2));
  aCh2TxSw3        <= lTxCh2(kCh1TxSw3);
  aCh2TxSw4        <= "10" when lTxCh2(kCh1TxLowbandMixerPathSelect) = '1' else "01";
  aCh2TxSw5        <= "01" when lTxCh2(kCh1TxLowbandMixerPathSelect) = '1' else "10";
  aCh2SwTrx        <= Tx2TrxMod(lTxCh2(kCh1SwTrxMsb downto kCh1SwTrx));
  aMkTx2En         <= lTxCh2(kCh1MykEnTx);

  aCh1RxSw1        <= lRxCh1_0(kCh1RxSw1Msb downto kCh1RxSw1);
  aCh1RxSw2        <= lRxCh1_0(kCh1RxSw2Msb downto kCh1RxSw2);
  aCh1RxSw3        <= lRxCh1_0(kCh1RxSw3Msb downto kCh1RxSw3);
  aCh1RxSw4        <= lRxCh1_0(kCh1RxSw4Msb downto kCh1RxSw4);
  aCh1RxSw5        <= lRxCh1_0(kCh1RxSw5Msb downto kCh1RxSw5);
  aCh1RxSw6        <= lRxCh1_1(kCh1RxSw6Msb downto kCh1RxSw6);
  aCh1RxSw7        <= "01" when lRxCh1_1(kCh1RxLowbandMixerPathSelect) = '1' else "10";
  aCh1RxSw8        <= "01" when lRxCh1_1(kCh1RxLowbandMixerPathSelect) = '1' else "10";
  aCh1LedRx        <= lRxCh1_1(kCh1RxLed) and not lTxCh1(kCh1TxLed);
  aCh1LedRx2       <= lRxCh1_1(kCh1Rx2Led);
  aCh1RxAmpEn      <= lRxCh1_1(kCh1RxAmpEn);
  aCh1RxMixerEn    <= lRxCh1_1(kCh1RxMixerEn);
  aCh1RxLna1En     <= lRxCh1_1(kCh1RxLna1En);
  aCh1RxLna2En     <= lRxCh1_1(kCh1RxLna2En);
  aMkRx1En         <= lRxCh1_1(kCh1MykEnRx);

  aCh2RxSw1        <= Rx2Switch1Mod(lRxCh2_0(kCh1RxSw1Msb downto kCh1RxSw1));
  aCh2RxSw2        <= Rx2Switch2Mod(lRxCh2_0(kCh1RxSw2Msb downto kCh1RxSw2));
  aCh2RxSw3        <= Rx2Switch3Mod(lRxCh2_0(kCh1RxSw3Msb downto kCh1RxSw3));
  aCh2RxSw4        <= Rx2Switch4Mod(lRxCh2_0(kCh1RxSw4Msb downto kCh1RxSw4));
  aCh2RxSw5        <= Rx2Switch5Mod(lRxCh2_0(kCh1RxSw5Msb downto kCh1RxSw5));
  aCh2RxSw6        <= Rx2Switch6Mod(lRxCh2_1(kCh1RxSw6Msb downto kCh1RxSw6));
  aCh2RxSw7        <= "10" when lRxCh2_1(kCh1RxLowbandMixerPathSelect) = '1' else "01";
  aCh2RxSw8        <= "10" when lRxCh2_1(kCh1RxLowbandMixerPathSelect) = '1' else "01";
  aCh2LedRx        <= lRxCh2_1(kCh1RxLed) and not lTxCh2(kCh1TxLed);
  aCh2LedRx2       <= lRxCh2_1(kCh1Rx2Led);
  aCh2RxAmpEn      <= lRxCh2_1(kCh1RxAmpEn);
  aCh2RxMixerEn    <= lRxCh2_1(kCh1RxMixerEn);
  aCh2RxLna1En     <= lRxCh2_1(kCh1RxLna1En);
  aCh2RxLna2En     <= lRxCh2_1(kCh1RxLna2En);
  aMkRx2En         <= lRxCh2_1(kCh1MykEnRx);


end RTL;




--XmlParse xml_on
--<top name="MgCpld"></top>
--<regmap name="MgCpld">
--  <group name="PsSpi_CpldRegisters" order="1">
--
--    <info>
--      These registers are accessed via the PS SPI interface to the CPLD. They are all
--      internal to the CPLD. The SPI format is 24 bits total. On MOSI, shift (msb first)
--      Rd/!Wt | Addr(6:0) | Data(15:0) (lsb). The SPI clock {b}MUST{/b} idle LOW before
--      and after the transaction. CPOL=CPHA=0. To access these registers, use the chip
--      select line named "CPLD-PS-SPI-SLE-33" as an active-low select.
--    </info>
--
--    <register name="SignatureReg" size="16" offset="0x00" attributes="Readable">
--      <info>
--        This register contains the device signature.
--      </info>
--      <bitfield name="ProductSignature" range="15..0">
--        <info>
--          Represents the product family name/number. This field reads back as
--          0xCAFE.
--        </info>
--      </bitfield>
--    </register>
--
--    <register name="MinorRevReg" size="16" offset="0x01" attributes="Readable">
--      <info>
--        This register contains the device revision numeric code.
--      </info>
--      <bitfield name="CpldMinorRevision" range="15..0">
--        <info>
--          Contains minor revision code (0,1,2,...).
--        </info>
--      </bitfield>
--    </register>
--
--    <register name="MajorRevReg" size="16" offset="0x02" attributes="Readable">
--      <info>
--        This register contains the major revision value.
--      </info>
--      <bitfield name="CpldMajorRevision" range="15..0">
--        <info>
--          Contains major revision code.
--        </info>
--      </bitfield>
--    </register>
--
--    <register name="BuildCodeLSB" size="16" offset="0x03" attributes="Readable">
--      <info>
--        Build code... right now it's the date it was built. LSB in this register.
--      </info>
--      <bitfield name="BuildCodeHH" range="7..0">
--        <info>
--          Contains build code hour code.
--        </info>
--      </bitfield>
--      <bitfield name="BuildCodeDD" range="15..8">
--        <info>
--          Contains build code day code.
--        </info>
--      </bitfield>
--    </register>
--
--    <register name="BuildCodeMSB" size="16" offset="0x04" attributes="Readable">
--      <info>
--        Build code... right now it's the date it was built. MSB in this register.
--      </info>
--      <bitfield name="BuildCodeMM" range="7..0">
--        <info>
--          Contains build code month code.
--        </info>
--      </bitfield>
--      <bitfield name="BuildCodeYY" range="15..8">
--        <info>
--          Contains build code revision year code.
--        </info>
--      </bitfield>
--    </register>
--
--    <register name="Scratch" size="16" offset="0x05" attributes="Readable|Writable">
--      <info>
--      </info>
--      <bitfield name="ScratchVal" range="15..0">
--        <info>
--          Contains scratch value for testing. The state of this register has
--          no effect on any other operation in the CPLD.
--        </info>
--      </bitfield>
--    </register>
--
--
--    <register name="CpldControl" size="16" offset="0x10" attributes="Writable">
--      <info>
--      </info>
--      <bitfield name="CpldReset" range="0">
--        <info>
--          Asserting this bit resets all the CPLD logic.
--          This reset will return all registers on the PS SPI interface to their default
--          state! To use this reset correctly, first write CpldReset to '1', then write
--          it to '0'. Registers will be reset on the _falling_ edge of CpldReset.
--        </info>
--      </bitfield>
--    </register>
--
--
--    <register name="LmkControl" size="16" offset="0x11" attributes="Readable|Writable">
--      <info>
--      </info>
--      <bitfield name="VcxoControl" range="4">
--        <info>
--          Setting this bit to '0' will allow the Phase DAC to exclusively control the
--          VCXO voltage. Defaults to '1', which allows the Phase DAC to adjust the
--          voltage (but the LMK still has control as well).
--        </info>
--      </bitfield>
--    </register>
--
--    <register name="LoStatus" size="16" offset="0x12" attributes="Readable">
--      <info>
--      </info>
--      <bitfield name="RxLoLockDetect" range="0" attributes="Readable">
--        <info>
--          Live lock detect status from the RX LO.
--        </info>
--      </bitfield>
--      <bitfield name="TxLoLockDetect" range="4" attributes="Readable">
--        <info>
--          Live lock detect status from the TX LO.
--        </info>
--      </bitfield>
--    </register>
--
--    <register name="MykonosControl" size="16" offset="0x13" attributes="Readable|Writable">
--      <info>
--      </info>
--      <bitfield name="MykonosReset" range="0">
--        <info>
--          Drives the Mykonos hard reset line. Defaults to de-asserted. Write a '1' to
--          assert the reset, and a '0' to de-assert.
--        </info>
--      </bitfield>
--    </register>
--
--  </group>
--
--
--
--  <group name="PlSpi_FrontEndControl" order="2">
--
--    <info>
--      These registers are accessed via the PL SPI interface to the CPLD. They are all
--      internal to the CPLD. The SPI format is 24 bits total. On MOSI, shift (msb first)
--      Rd/!Wt | Addr(6:0) | Data(15:0) (lsb). The SPI clock {b}MUST{/b} idle LOW before
--      and after the transaction. CPOL=CPHA=0. To access these registers, use the chip
--      select line named "CPLD-PL-SPI-LE-25" as an active-low select. {br}{br}
--
--      The ATR bits ultimately control which of these registers actually control
--      the RF front end.
--    </info>
--
--    <register name="PlScratch" size="16" offset="0x40" attributes="Readable|Writable">
--      <bitfield name="PlScratchVal" range="15..0">
--        <info>
--          Contains scratch value for testing. The state of this register has no effect
--          on any other operation in the CPLD.
--        </info>
--      </bitfield>
--    </register>
--
--    <register name="PlCpldControl" size="16" offset="0x41" attributes="Writable">
--      <info>
--      </info>
--      <bitfield name="PlCpldReset" range="0">
--        <info>
--          Asserting this bit resets all the CPLD logic on the PL SPI interface.
--          This reset will return all registers to their default state! To use this
--          reset correctly, first write PlCpldReset to '1', then write it to '0'.
--          Registers will be reset on the _falling_ edge of PlCpldReset.
--        </info>
--      </bitfield>
--    </register>
--
--
--
--    <enumeratedtype name="TrxSwitch">
--      <value name="FromLowerFilterBankTxSw1"        integer="0"/>
--      <value name="FromTxUpperFilterBankLp6400MHz"  integer="1"/>
--      <value name="RxChannelPath"                   integer="2"/>
--      <value name="BypassPathToTxSw3"               integer="3"/>
--    </enumeratedtype>
--
--    <enumeratedtype name="TxSwitch1">
--      <value name="ShutdownTxSw1"          integer="0"/>
--      <value name="FromTxFilterLp1700MHz"  integer="1"/>
--      <value name="FromTxFilterLp3400MHz"  integer="2"/>
--      <value name="FromTxFilterLp0800MHz"  integer="3"/>
--    </enumeratedtype>
--
--    <enumeratedtype name="TxSwitch2">
--      <value name="ToTxFilterLp3400MHz"  integer="1"/>
--      <value name="ToTxFilterLp1700MHz"  integer="2"/>
--      <value name="ToTxFilterLp0800MHz"  integer="4"/>
--      <value name="ToTxFilterLp6400MHz"  integer="8"/>
--    </enumeratedtype>
--
--    <enumeratedtype name="TxSwitch3">
--      <value name="ToTxFilterBanks"   integer="0"/>
--      <value name="BypassPathToTrxSw" integer="1"/>
--    </enumeratedtype>
--
--
--    <register name="TxCh1_Idle" size="16" offset="0x50" attributes="Readable|Writable">
--      <info>
--        Load this register with the front-end configuration for channel TX 1 when the
--        ATR bits are configured: TX = 0, RX = don't-care.
--      </info>
--      <bitfield name="Ch1TxSw1" type="TxSwitch1" range="1..0">
--        <info>
--          Controls Switch 1. Filter bank receive switch.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1TxSw2" type="TxSwitch2" range="5..2">
--        <info>
--          Controls Switch 2. Filter bank distribution switch.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1TxSw3" type="TxSwitch3" range="6">
--        <info>
--          Controls Switch 3. Bypasses the filter bank and PA, or doesn't.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1TxLowbandMixerPathSelect" range="7">
--        <info>
--          Controls Switches 4 and 5. Write a '1' to select the Lowband Mixer path.
--          Writing '0' will select the bypass path around the mixer. Default is '0'. Note:
--          Individual control over these switches was removed as an optimization to
--          allow all TX controls to fit in one 16 bit register.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1TxMixerEn" range="8">
--        <info>
--          Write a '1' to enable the lowband mixer. Note that Ch1TxLowbandMixerPathSelect
--          must be properly configured to select the mixer path.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1TxAmpEn" range="9">
--        <info>
--          Write a '1' to enable the TX path Amp in between TX switches 3 and 4. The path
--          (from Mykonos) is: TxSw4 -> Amp -> DSA -> TxSw3.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1TxPaEn" range="10">
--        <info>
--          Write a '1' to enable the TX path PA in between TX switches 2 and 3.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1SwTrx" type="TrxSwitch" range="12..11">
--        <info>
--          TRX switch control.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1TxLed" range="13">
--        <info>
--          Red/Green combo LED for the TRX channel.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1MykEnTx" range="14">
--        <info>
--          Drives the Mykonos input port TX1_ENABLE.
--        </info>
--      </bitfield>
--    </register>
--
--    <register name="TxCh1_TxOn" size="16" offset="0x53" attributes="Readable|Writable">
--      <info>
--        Load this register with the front-end configuration for channel TX 1 when the
--        ATR bits are configured: TX = 1, RX = don't-care. The bitfields are the same
--        as for the Tx1_Off register.
--      </info>
--    </register>
--
--    <register name="TxCh2_Idle" size="16" offset="0x60" attributes="Readable|Writable">
--      <info>
--        Load this register with the front-end configuration for channel TX 2 when the
--        ATR bits are configured: TX = 0, RX = don't-care. The bitfields are the same
--        as for the Tx1_Off register.
--      </info>
--    </register>
--
--    <register name="TxCh2_TxOn" size="16" offset="0x63" attributes="Readable|Writable">
--      <info>
--        Load this register with the front-end configuration for channel TX 2 when the
--        ATR bits are configured: TX = 1, RX = don't-care. The bitfields are the same
--        as for the Tx1_Off register.
--      </info>
--    </register>
--
--
--    <enumeratedtype name="Rx1Switch1">
--      <value name="TxRxInput"       integer="0"/>
--      <value name="RxLoCalInput"    integer="1"/>
--      <value name="TrxSwitchOutput" integer="2"/>
--      <value name="Rx2Input"        integer="3"/>
--    </enumeratedtype>
--
--    <enumeratedtype name="Rx1Switch2">
--      <value name="ShutdownSw2"              integer="0"/>
--      <value name="LowerFilterBankToSwitch3" integer="1"/>
--      <value name="BypassPathToSwitch6"      integer="2"/>
--      <value name="UpperFilterBankToSwitch4" integer="3"/>
--    </enumeratedtype>
--
--    <enumeratedtype name="Rx1Switch3">
--      <value name="Filter2100x2850MHz"    integer="0"/>
--      <value name="Filter0490LpMHz"       integer="1"/>
--      <value name="Filter1600x2250MHz"    integer="2"/>
--      <value name="Filter0440x0530MHz"    integer="4"/>
--      <value name="Filter0650x1000MHz"    integer="5"/>
--      <value name="Filter1100x1575MHz"    integer="6"/>
--      <value name="ShutdownSw3"           integer="7"/>
--    </enumeratedtype>
--
--    <enumeratedtype name="Rx1Switch4">
--      <value name="Filter2100x2850MHzFrom"    integer="1"/>
--      <value name="Filter1600x2250MHzFrom"    integer="2"/>
--      <value name="Filter2700HpMHz"           integer="4"/>
--    </enumeratedtype>
--
--    <enumeratedtype name="Rx1Switch5">
--      <value name="Filter0440x0530MHzFrom"    integer="1"/>
--      <value name="Filter1100x1575MHzFrom"    integer="2"/>
--      <value name="Filter0490LpMHzFrom"       integer="4"/>
--      <value name="Filter0650x1000MHzFrom"    integer="8"/>
--    </enumeratedtype>
--
--    <enumeratedtype name="Rx1Switch6">
--      <value name="LowerFilterBankFromSwitch5" integer="1"/>
--      <value name="UpperFilterBankFromSwitch4" integer="2"/>
--      <value name="BypassPathFromSwitch2"      integer="4"/>
--    </enumeratedtype>
--
--
--
--    <register name="RxCh1_0_Idle" size="16" offset="0x51" attributes="Readable|Writable">
--      <info>
--        Load this register with the front-end configuration for channel RX 1 when the
--        ATR bits are configured: TX = don't-care, RX = 0.
--      </info>
--      <bitfield name="Ch1RxSw1" type="Rx1Switch1" range="1..0">
--        <info>
--          Controls Switch 1. Selects between the cal, bypass, RX2, and TRX paths.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1RxSw2" type="Rx1Switch2" range="3..2">
--        <info>
--          Controls Switch 2. First filter switch. Selects between bypass path and
--          the upper/lower filter banks.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1RxSw3" type="Rx1Switch3" range="6..4">
--        <info>
--          Controls Switch 3. Lower filter bank transmit switch.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1RxSw4" type="Rx1Switch4" range="9..7">
--        <info>
--          Controls Switch 4. Upper filter bank receive switch.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1RxSw5" type="Rx1Switch5" range="13..10">
--        <info>
--          Controls Switch 5. Lower filter bank receive switch.
--        </info>
--      </bitfield>
--    </register>
--
--    <register name="RxCh1_1_Idle" size="16" offset="0x52" attributes="Readable|Writable">
--      <info>
--        Load this register with the front-end configuration for channel RX 1 when the
--        ATR bits are configured: TX = don't-care, RX = 0.
--      </info>
--      <bitfield name="Ch1RxSw6" type="Rx1Switch6" range="2..0">
--        <info>
--          Controls Switch 6. Selects between the upper and lower filter banks and
--          bypass path.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1RxLowbandMixerPathSelect" range="3">
--        <info>
--          Controls Switches 7 and 8. Write a '1' to select the Lowband Mixer path.
--          Writing '0' will select the bypass path around the mixer. Default is '0'. Note:
--          Individual control over these switches was removed as an optimization to
--          allow all TX controls to fit in one 16 bit register... so the same was done
--          for the RX path for continuity.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1RxMixerEn" range="4">
--        <info>
--          Write a '1' to enable the lowband mixer. Note that Ch1RxLowbandMixerPathSelect
--          must be properly configured to select the mixer path.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1RxAmpEn" range="5">
--        <info>
--          Write a '1' to enable the RX path Amp directly before the Mykonos inputs.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1RxLna1En" range="6">
--        <info>
--          Write a '1' to enable the RX path LNA1 between RxSw4 and RxSw6.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1RxLna2En" range="7">
--        <info>
--          Write a '1' to enable the RX path LNA2 between RxSw5 and RxSw6.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1Rx2Led" range="8">
--        <info>
--          Green LED for RX2 channel.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1RxLed" range="9">
--        <info>
--          Red/Green combo LED for the TRX channel.
--        </info>
--      </bitfield>
--      <bitfield name="Ch1MykEnRx" range="10">
--        <info>
--          Drives the Mykonos input port RX1_ENABLE.
--        </info>
--      </bitfield>
--    </register>
--
--    <register name="RxCh1_0_RxOn" size="16" offset="0x54" attributes="Readable|Writable">
--      <info>
--        Load this register with the front-end configuration for channel RX 1 when the
--        ATR bits are configured: TX = don't-care, RX = 1. The bitfields are the same
--        as for the RxCh1_0_Idle register.
--      </info>
--    </register>
--
--    <register name="RxCh1_1_RxOn" size="16" offset="0x55" attributes="Readable|Writable">
--      <info>
--        Load this register with the front-end configuration for channel RX 1 when the
--        ATR bits are configured: TX = don't-care, RX = 1. The bitfields are the same
--        as for the RxCh1_1_Idle register.
--      </info>
--    </register>
--
--    <register name="RxCh2_0_Idle" size="16" offset="0x61" attributes="Readable|Writable">
--      <info>
--        Load this register with the front-end configuration for channel RX 2 when the
--        ATR bits are configured: TX = don't-care, RX = 0. The bitfields are the same
--        as for the RxCh1_0_Idle register.
--      </info>
--    </register>
--
--    <register name="RxCh2_1_Idle" size="16" offset="0x62" attributes="Readable|Writable">
--      <info>
--        Load this register with the front-end configuration for channel RX 2 when the
--        ATR bits are configured: TX = don't-care, RX = 0. The bitfields are the same
--        as for the RxCh1_1_Idle register.
--      </info>
--    </register>
--
--    <register name="RxCh2_0_RxOn" size="16" offset="0x64" attributes="Readable|Writable">
--      <info>
--        Load this register with the front-end configuration for channel RX 2 when the
--        ATR bits are configured: TX = don't-care, RX = 1. The bitfields are the same
--        as for the RxCh1_0_Idle register.
--      </info>
--    </register>
--
--    <register name="RxCh2_1_RxOn" size="16" offset="0x65" attributes="Readable|Writable">
--      <info>
--        Load this register with the front-end configuration for channel RX 2 when the
--        ATR bits are configured: TX = don't-care, RX = 1. The bitfields are the same
--        as for the RxCh1_1_Idle register.
--      </info>
--    </register>
--
--  </group>
--
--</regmap>
--XmlParse xml_off




--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: rf_nco_reset
--
-- Description:
--
--   This entity has the logic needed to synchronously reset the NCO inside the
--   RF section.
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

entity rf_nco_reset is
  port(
    -- AXI-lite clock used for RFDC configuration.
    ConfigClk               : in std_logic;

    -- Radio clock used in the converter data path.
    DataClk                 : in std_logic;

    -- PL SYSREF
    dSysref                 : in std_logic;

    --Strobe dNcoResetEn for one DataClk cycle to initiate NCO reset.
    dStartNcoReset          : in std_logic;

    ---------------------------------------------------------------------------
    -- NCO reset controls and status
    ---------------------------------------------------------------------------
    -- Port naming convention:
    -- cDac<Tile Number><Converter Number><signal name>
    -- cAdc<Tile Number><Converter Number><signal name>

    -----------------------------------
    -- DAC Tile 228
    -----------------------------------
    -- DAC common NCO update controls and status.
    cDac0xNcoUpdateBusy     : in  std_logic_vector(1 downto 0);
    cDac0xNcoUpdateReq      : out std_logic := '0';
    cDac0xSysrefIntGating   : out std_logic := '0';
    cDac0xSysrefIntReenable : out std_logic := '0';

    -----------------------------------
    -- DAC Tile 229
    -----------------------------------
    -- DAC common NCO update controls and status.
    cDac1xNcoUpdateBusy     : in  std_logic;
    cDac1xNcoUpdateReq      : out std_logic := '0';

    -----------------------------------
    --ADC Tile 224
    -----------------------------------
    -- ADC common NCO update controls and status.
    cAdc0xNcoUpdateBusy     : in  std_logic;
    cAdc0xNcoUpdateReq      : out std_logic := '0';

    -----------------------------------
    --ADC Tile 226
    -----------------------------------
    -- ADC common NCO update controls and status.
    cAdc2xNcoUpdateBusy     : in  std_logic;
    cAdc2xNcoUpdateReq      : out std_logic := '0';

    -- NCO reset can be initiated only when cNcoPhaseRst is set to '1' and
    -- cNcoUpdateEn = 0x20. The FSM in this entity will set these values when
    -- an NCO reset is initiated during synchronization. These ports are common
    -- for all the converters. So, we will fan these signals out to each
    -- converter outside this entity.
    cNcoPhaseRst            : out std_logic := '1';
    cNcoUpdateEn            : out std_logic_vector(5 downto 0) := "100000";

    -- NCO reset status back to the user.
    dNcoResetDone           : out std_logic := '0'
  );
end rf_nco_reset;

architecture RTL of rf_nco_reset is

  -- State machine to sequence NCO reset across different RFDC tiles.
  type ResetState_t is (Idle, ReqGating, CheckGating, CheckUpdateDone,
                        CheckResetDone, ResetDone);
  signal cResetState : ResetState_t := Idle;

  signal dNcoResetDone_ms, cNcoResetDone : std_logic := '0';
  signal dStartNcoResetReg, cStartNcoReset_ms, cStartNcoReset : std_logic := '0';
  signal cSysref_ms, cSysref, cSysrefDlyd : std_logic := '0';
  signal cSysrefIntGating, dSysrefIntGating_ms,
         dSysrefIntGating : std_logic := '0';
begin

  -- NCO start signal from the user is a one DataClk cycle strobe. In this
  -- process, we register the NCO start request from the user. This NCO start
  -- request register is cleared after the NCO reset sequence is initiated. We
  -- used the signal used to gate SYSREF to clear this register.
  RegNcoStart: process(DataClk)
  begin
    if rising_edge(DataClk) then
      dSysrefIntGating_ms <= cSysrefIntGating;
      dSysrefIntGating    <= dSysrefIntGating_ms;
      if dSysrefIntGating = '1' then
        dStartNcoResetReg <= '0';
      elsif dStartNcoReset = '1' then
        dStartNcoResetReg <= '1';
      end if;
    end if;
  end process RegNcoStart;

  -- Irrespective of when NCO reset strobe is issued by the user, we need to
  -- initiate NCO reset only on the rising edge of SYSREF. This is because, we
  -- have to complete the reset within a SYSREF period.
  ConfigClkCross: process(ConfigClk)
  begin
    if rising_edge(ConfigClk) then
      cSysref_ms        <= dSysref;
      cSysref           <= cSysref_ms;
      cSysrefDlyd       <= cSysref;
      cStartNcoReset_ms <= dStartNcoResetReg;
      cStartNcoReset    <= cStartNcoReset_ms;
    end if;
  end process ConfigClkCross;

  -- These signals can be set to a constant value as NCO phase reset is only
  -- initiated by *NcoUpdateReq signal.
  cNcoPhaseRst <= '1';
  cNcoUpdateEn <= "100000";

  -- ! STATE MACHINE STARTUP !
  -- The state machine starts in Idle state and does not change state until
  -- cStartNcoReset is set to '1'. cStartNcoReset signal and cSysref are based
  -- of ConfigClock so changing state from Idle cannot go metastable. State
  -- machine to initiate NCO reset on all enabled RFDC tiles. This state
  -- machine was written based of the information provided in "NCO frequency
  -- hopping" section in PG269 (v2.2). We use multi-mode for NCO reset.
  ResetFsm: process(ConfigClk)
  begin
    if rising_edge(ConfigClk) then
      cResetState             <= Idle;
      cNcoResetDone           <= '0';
      cDac0xNcoUpdateReq      <= '0';
      cSysrefIntGating        <= '0';
      cDac0xSysrefIntReenable <= '0';
      cDac1xNcoUpdateReq      <= '0';
      cAdc0xNcoUpdateReq      <= '0';
      cAdc2xNcoUpdateReq      <= '0';
      case cResetState is
        -- Stay in this state until NCO reset sequence is initiated. NCO reset
        -- is initiated only on the rising edge of SYSREF.
        when Idle =>
          if cSysref = '1' and cSysrefDlyd = '0' and cStartNcoReset = '1' then
             cResetState      <= ReqGating;
             cSysrefIntGating <= '1';
          end if;

        -- When NCO reset is initiated, gate the RFDC internal SYSREF. To gate
        -- internal SYSREF set cSysrefIntGating to '1'. To request NCO reset
        -- strobe cDac0xNcoUpdateReq for one ConfigClk period. At this point,
        -- we can only request NCO reset for RF-DAC tile 228.
        when ReqGating =>
          cResetState <= CheckGating;
          cDac0xNcoUpdateReq <= '1';
          cSysrefIntGating <= '1';

        -- Since we are gating SYSREF inside RFDC, we need to wait until SYSREF
        -- is gated internally. RFDC sets cDac0xNcoUpdateBusy[0] to '1' when
        -- SYSREF is gated. cDac0xNcoUpdateBusy[1] is also set to '1' to
        -- indicate that NCO reset is still in progress. After the SYSREF is
        -- gated request NCO reset on all other converter tiles.
        when CheckGating =>
          cSysrefIntGating <= '1';
          cResetState <= CheckGating;
          if cDac0xNcoUpdateBusy = "11" then
            cResetState <= CheckUpdateDone;
            cDac1xNcoUpdateReq <= '1';
            cAdc0xNcoUpdateReq <= '1';
            cAdc2xNcoUpdateReq <= '1';
          end if;

        -- In this state, we check if the RFDC block is ready for NCO reset.
        -- This check is done using the *Busy signal from RFDC. Once RFDC is
        -- ready for NCO reset, disable internal SYSREF gating.
        when CheckUpdateDone =>
          cSysrefIntGating <= '1';
          cResetState <= CheckUpdateDone;
          if cDac0xNcoUpdateBusy = "10" and  cAdc0xNcoUpdateBusy = '0' and
             cAdc2xNcoUpdateBusy = '0'  and  cDac1xNcoUpdateBusy = '0'  and
             cSysref = '1' and cSysrefDlyd = '0' then
            cDac0xSysrefIntReenable <= '1';
            cResetState <= CheckResetDone;
          end if;

        -- NCO reset is done when cDac0xNcoUpdateBusy[1] is set to '0'. RFDC is
        -- programmed from software to reset the NCO on a SYSREF rising edge.
        when CheckResetDone =>
          cSysrefIntGating <= '1';
          cResetState <= CheckResetDone;
          if cDac0xNcoUpdateBusy = "00" then
            cResetState <= ResetDone;
          end if;

        -- Wait in this state until another NCO reset request is issued.
        when ResetDone =>
          cNcoResetDone <= '1';
          cResetState <= ResetDone;
          if cSysref = '1' and cSysrefDlyd = '0' and cStartNcoReset = '1' then
             cResetState <= ReqGating;
             cSysrefIntGating <= '1';
          end if;
      end case;
    end if;
  end process ResetFsm;

  cDac0xSysrefIntGating <= cSysrefIntGating;

  -- Move the NCO reset done status to DataClk domain.
  DataClkCrossing: process(DataClk)
  begin
    if rising_edge(DataClk) then
      dNcoResetDone_ms <= cNcoResetDone;
      dNcoResetDone    <= dNcoResetDone_ms;
    end if;
  end process DataClkCrossing;

end RTL;

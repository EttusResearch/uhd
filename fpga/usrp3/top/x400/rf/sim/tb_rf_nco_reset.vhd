--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: tb_rf_nco_reset
--
-- Description:
--
--   Self-checking testbench for NCO reset sequencing.
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

entity tb_rf_nco_reset is
end tb_rf_nco_reset;


architecture RTL of tb_rf_nco_reset is

  signal cAdc0xNcoUpdateReq      : std_logic;
  signal cAdc2xNcoUpdateReq      : std_logic;
  signal cDac0xNcoUpdateReq      : std_logic;
  signal cDac0xSysrefIntGating   : std_logic;
  signal cDac0xSysrefIntReenable : std_logic;
  signal cDac1xNcoUpdateReq      : std_logic;
  signal cNcoPhaseRst            : std_logic;
  signal cNcoUpdateEn            : std_logic_vector(5 downto 0);
  signal dNcoResetDone           : std_logic;

  signal cDac0xNcoUpdateBusy : std_logic_vector(1 downto 0) := "00";
  signal dStartNcoReset      : std_logic := '0';
  signal cAdc0xNcoUpdateBusy : std_logic := '0';
  signal cAdc2xNcoUpdateBusy : std_logic := '0';
  signal cDac1xNcoUpdateBusy : std_logic := '0';

  signal cSysref_ms, cSysref       : std_logic := '0';
  signal cSysrefDlyd               : std_logic_vector(1 downto 0) := "00";
  signal cDac0xSysrefIntGatingDlyd : std_logic := '0';
  signal cNcoPhaseRstDlyd          : std_logic_vector(2 downto 0) := "000";

  signal cWrCount : integer := 0;
  type RfdcNcoState_t is (Idle, GateSysref, UpdateReq, CheckUpdate,
                          SysrefEn, WaitForSysref, ResetDone);
  signal cRfdcNcoState : RfdcNcoState_t := Idle;

  signal StopSim : boolean;
  constant kConfigClkPer : time := 25 ns;
  -- SYSREF period is 2.5 MHz.
  constant kSysrefPer    : time := 400 ns;
  -- DataClk period is 125 MHz and generated from the same clocking chip that
  -- generated SYSREF and are related.
  constant kDataClkPer     : time := kSysrefPer/50;

  signal ConfigClk : std_logic := '0';
  signal DataClk   : std_logic := '0';
  signal dSysref   : std_logic := '0';

  procedure DataClkWait(X : positive := 1) is
  begin
    for i in 1 to X loop
      wait until rising_edge(DataClk);
    end loop;
  end procedure DataClkWait;

  procedure ConfigClkWait(X : positive := 1) is
  begin
    for i in 1 to X loop
      wait until rising_edge(ConfigClk);
    end loop;
  end procedure ConfigClkWait;

  procedure SysrefWait(X : positive := 1) is
  begin
    for i in 1 to X loop
      wait until rising_edge(dSysref);
    end loop;
  end procedure SysrefWait;

begin

  ConfigClk <= not ConfigClk after kConfigClkPer/2 when not StopSim else '0';
  DataClk   <= not DataClk   after kDataClkPer/2   when not StopSim else '0';
  dSysref   <= not dSysref   after kSysrefPer/2    when not StopSim else '0';

  -- Both cNcoPhaseRst and cNcoUpdateEn are constants in the DUT.
  dut: entity WORK.rf_nco_reset (RTL)
    port map (
      ConfigClk               => ConfigClk,
      DataClk                 => DataClk,
      dSysref                 => dSysref,
      dStartNcoReset          => dStartNcoReset,
      cDac0xNcoUpdateBusy     => cDac0xNcoUpdateBusy,
      cDac0xNcoUpdateReq      => cDac0xNcoUpdateReq,
      cDac0xSysrefIntGating   => cDac0xSysrefIntGating,
      cDac0xSysrefIntReenable => cDac0xSysrefIntReenable,
      cDac1xNcoUpdateBusy     => cDac1xNcoUpdateBusy,
      cDac1xNcoUpdateReq      => cDac1xNcoUpdateReq,
      cAdc0xNcoUpdateBusy     => cAdc0xNcoUpdateBusy,
      cAdc0xNcoUpdateReq      => cAdc0xNcoUpdateReq,
      cAdc2xNcoUpdateBusy     => cAdc2xNcoUpdateBusy,
      cAdc2xNcoUpdateReq      => cAdc2xNcoUpdateReq,
      cNcoPhaseRst            => cNcoPhaseRst,
      cNcoUpdateEn            => cNcoUpdateEn,
      dNcoResetDone           => dNcoResetDone
    );

  main: process

    -- Procedure to sweep the entire SYSREF period.
    -- When we strobe dStartNcoReset for one DataClk cycle. NCO reset sequence
    -- is initiated. In this procedure, we sweep the dStartNcoReset strobe the
    -- entire SYSREF cycle.
    procedure SysrefSweep is
      constant kSysrefInRfCycles : integer := kSysrefPer/kDataClkPer;
    begin
      for i in 1 to kSysrefInRfCycles loop
        wait until cDac0xSysrefIntGating = '0' for 1 us;
        assert cDac0xSysrefIntGating = '0'
          report "NCO phase reset does not de-assert"
          severity error;
        SysrefWait;
        DataClkWait(i);
        dStartNcoReset <= '0';
        DataClkWait;
        dStartNcoReset <= '1';
        DataClkWait;
        dStartNcoReset <= '0';
        -- Wait for a minimum of 3 SYSREF period. 1 SYSREF edge is used to
        -- initiate NCO reset, 1 SYSREF edge is used to re-enable SYSREF and 1
        -- SYSREF edge is used by RFDC to reset all NCOs.
        SysrefWait(3);
      end loop;
    end procedure;

  begin

    -- Strobe dStartNcoReset across entire SYSREF period.
    SysrefSweep;
    -- Wait for a minimum of 3 SYSREF cycles to make sure NCO reset is complete.
    SysrefWait(3);

    StopSim <= true;
    wait;
  end process;

  -- Process to mimic RFDC NCO reset
  -- This state machine is based of "NCO frequency hopping" section in PG269
  -- (v2.2). Refer to multi-mode subsection for more details.
  MimicRfdc: process(ConfigClk)
  begin
    if falling_edge(ConfigClk) then
      cRfdcNcoState <= Idle;
      case cRfdcNcoState is

        -- Wait until SYSREF internal gating is asserted.
        when Idle =>
          cWrCount <= 0;
          if cDac0xSysrefIntGating = '1' then
            cRfdcNcoState <= GateSysref;
          end if;

        -- Change cDac0xNcoUpdateBusy to "11" to indicate SYSREF is gated
        -- internally when NCO update is requested on DAC tile 228.
        -- cDac0xNcoUpdateBusy(0) is set to '1', the SYSREF is gated and
        -- cDac0xNcoUpdateBusy(1) is set to '1', to indicate the NCO reset
        -- process has started, but not complete.
        when GateSysref =>
          cRfdcNcoState <= GateSysref;
          if cDac0xNcoUpdateReq = '1' then
            cRfdcNcoState <= UpdateReq;
            cDac0xNcoUpdateBusy <= "11";
          end if;

        -- If NCO reset is requested on other tiles, assert NCO update busy on
        -- other tiles as well.
        when UpdateReq =>
          cRfdcNcoState <= CheckUpdate;
          cDac1xNcoUpdateBusy <= cDac1xNcoUpdateReq;
          cAdc0xNcoUpdateBusy <= cAdc0xNcoUpdateReq;
          cAdc2xNcoUpdateBusy <= cAdc2xNcoUpdateReq;

        -- It takes 5 clock cycles to update each RFDC internal registers with
        -- the used request change. In rf_nco_reset entity, we only want to
        -- reset the NCO, which is a single bit. So, it should take only 5
        -- ConfigClk for the update. When the internal register is updated, set
        -- cDac0xNcoUpdateBusy(0) to '0'.
        when CheckUpdate =>
          cRfdcNcoState <= CheckUpdate;
          if cWrCount > 4 then
            cRfdcNcoState <= SysrefEn;
            cDac0xNcoUpdateBusy <= "10"; --Indicates that SYSREF is gated.
            cDac1xNcoUpdateBusy <= '0';
            cAdc0xNcoUpdateBusy <= '0';
            cAdc2xNcoUpdateBusy <= '0';
          end if;
          cWrCount <= cWrCount + 1;

        -- Wait until internal SYSREF gating is disabled.
        when SysrefEn =>
          cWrCount <= 0;
          cRfdcNcoState <= SysrefEn;
          if cDac0xSysrefIntReenable = '1' then
            if cSysrefDlyd(0) = '0' and cSysref = '1' then
              cDac0xNcoUpdateBusy <= "00"; --Indicates that NCO reset is complete.
              cRfdcNcoState <= ResetDone;
            else
              cRfdcNcoState <= WaitForSysref;
            end if;
          end if;

        -- NCO reset is done on the rising edge of SYSREF. When NCO reset is
        -- complete, set cDac0xNcoUpdateBusy(1) to '0'.
        when WaitForSysref =>
          cRfdcNcoState <= WaitForSysref;
          if cSysrefDlyd(0) = '0' and cSysref = '1' then
            cDac0xNcoUpdateBusy <= "00"; --Indicates that NCO reset is complete.
            cRfdcNcoState <= ResetDone;
          end if;

        -- Wait in this state, until the next NCO reset is requested.
        when ResetDone =>
          cRfdcNcoState <= ResetDone;
          if cDac0xSysrefIntGating = '1' then
            cRfdcNcoState <= GateSysref;
          end if;
      end case;
    end if;
  end process;

  -- SYSREF clock crossing from DataClk to ConfigClk and some pipelines.
  ConfigClkSysref: process(ConfigClk)
  begin
    if rising_edge(ConfigClk) then
      cSysref_ms  <= dSysref;
      cSysref     <= cSysref_ms;
      cSysrefDlyd <= cSysrefDlyd(cSysrefDlyd'high-1) & cSysref;
      cDac0xSysrefIntGatingDlyd <= cDac0xSysrefIntGating;
      cNcoPhaseRstDlyd <= cNcoPhaseRstDlyd(cNcoPhaseRstDlyd'high downto 1)
                          & cDac0xNcoUpdateBusy(1);
    end if;
  end process;

  -- Assertions
  process(ConfigClk)
  begin
    if falling_edge(ConfigClk) then

      --Check if cNcoPhaseRst is a constant of '1'.
      assert cNcoPhaseRst = '1'
        report "NCO phase reset signal should be constant."
        severity error;
      -- Check if cNcoUpdateEn is a constant of "100000".
      assert cNcoUpdateEn = "100000"
        report "NCO phase reset signal should be constant."
        severity error;
      -- Check if NCO reset was requested on the rising edge of SYSREF.
      if cDac0xSysrefIntGating = '1' and cDac0xSysrefIntGatingDlyd = '0' then
        assert cSysrefDlyd = "01"
          report "NCO reset did not start on SYSREF rising edge"
          severity error;
      end if;

      -- We wait for couple of clock cycles after NCO done signal is toggled in
      -- from the RFDC. RFDC uses cDac0xNcoUpdateBusy(1) to indicate NCO reset
      -- process is done. It is important to wait a minimum of three clock
      -- cycles before this check is done. This wait is needed for clock
      -- crossing.
      if cNcoPhaseRstDlyd(2) = '1' and cNcoPhaseRstDlyd(1) = '0' then
        assert dNcoResetDone = '1'
          report "NCO Reset done should have been asserted after NCO " &
                 "reset request is de-asserted"
          severity error;
      end if;

    end if;
  end process;

end RTL;

--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: clock_gates
--
-- Description:
--
--   Gate propagation of DataClk and RfdcClk instances until the PLL lock
--   status signal is stable and software has acknowledged it by asserting the
--   pertinent controls.
--
--   RfdcClks are used on other Xilinx IP components in the Board Design, and
--   Vivado fails to detect their frequency correctly their buffer is
--   explicitly instantiated in the Block Design. Therefore, we only generate
--   the buffer enable signals for these clocks within this component.
--
--   Since DataClk are only used in other Custom IP blocks within the Block
--   design, it is possible to instantiate the clock buffers within this block
--   for without running into IP generation failures.
--
-- Parameters:
--
--   kReliableClkPeriodNs: Clock period (ns) for ReliableClk.
--

library IEEE;
  use IEEE.std_logic_1164.ALL;
  use IEEE.numeric_std.ALL;

library UNISIM;
  use UNISIM.Vcomponents.ALL;

library WORK;
use WORK.PkgRFDC_REGS_REGMAP.all;


entity clock_gates is
  generic (
    kReliableClkPeriodNs : integer  := 25
  );
  port (
    -- MMCM reset
    -- This clock will be asserted via AXI access before any clocking
    -- configuration done, signals coming into this component will not change
    -- immediately after this reset is de-asserted.
    rPllReset_n            : in  std_logic;

    aPllLocked             : in  std_logic;

    -- Input Clocks (from MMCM)
    ReliableClk            : in  std_logic;
    DataClk1xPll           : in  std_logic;
    DataClk2xPll           : in  std_logic;

    -- Buffered Clock Outputs (to design)
    DataClk1x              : out std_logic;
    DataClk2x              : out std_logic;

    -- Buffers for these signals must be instantiated on Block design for clock
    -- rates to be identified. The Utility Buffers instantiated on the Block
    -- Design require signals to be of type std_logic_vector.
    aEnableRfBufg1x        : out std_logic_vector(0 downto 0);
    aEnableRfBufg2x        : out std_logic_vector(0 downto 0);

    -- PLL Status Signals
    rPllLocked             : out std_logic;

    -- Window Interface
    rSafeToEnableGatedClks : in  std_logic;
    rGatedBaseClksValid    : out std_logic;

    -- AXI GPIO interface
    rSoftwareControl       : in  std_logic_vector(31 downto 0);
    rSoftwareStatus        : out std_logic_vector(31 downto 0)
  );
end clock_gates;

architecture STRUCT of clock_gates is

  component sync_wrapper
    generic (
      WIDTH            : integer := 1;
      STAGES           : integer := 2;
      INITIAL_VAL      : integer := 0;
      FALSE_PATH_TO_IN : integer := 1);
    port (
      clk        : in  std_logic;
      rst        : in  std_logic;
      signal_in  : in  std_logic_vector((WIDTH-1) downto 0);
      signal_out : out std_logic_vector((WIDTH-1) downto 0));
  end component;

  component BUFGCE
    generic(
      CE_TYPE : string);
    port (
      O  : out std_ulogic;
      CE : in std_ulogic;
      I  : in std_ulogic);
  end component;

  -- UltraScale MMCM max lock time = 100 us / 25 ns = 4,000 clk cycles. If the
  -- division kPllLockTimeNs / kReliableClkPeriodNs does not evaluate to an
  -- integer, Vivado could either round up or down. In case they round down, we
  -- add '1' to the result to ensure we have the full lock time accounted for.
  -- In this case, it is better to count 1 more than necessary than kill the
  -- process prematurely.
  constant kPllLockTimeNs     : integer := 100000;
  constant kMaxPllLockCount   : integer := kPllLockTimeNs / kReliableClkPeriodNs + 1;
  signal   rLockedFilterCount : integer range 0 to kMaxPllLockCount-1 := kMaxPllLockCount-1;

  signal   rClearDataClkUnlockedSticky : std_logic;

  -----------------------------------------------------------------------------
  -- PLL locked signals
  -----------------------------------------------------------------------------

  -- Synchronizer signals
  signal aPllLockedLcl : std_logic_vector(0 downto 0);
  signal rPllLockedDs  : std_logic_vector(0 downto 0)  := (others => '0');

  -- Lock status indicators
  signal rPllLockedLcl      : std_logic := '0';
  signal rPllUnlockedSticky : std_logic := '0';

  -- Safe BUFG enable signals
  signal rEnableDataClk1x,
         rEnableDataClk2x,
         rEnableRfdcClk1x,
         rEnableRfdcClk2x : std_logic;

  signal rEnableDataBufg1x    : std_logic := '0';
  signal rEnableDataBufg2x    : std_logic := '0';
  signal rEnableRfdcBufg1xLcl : std_logic := '0';
  signal rEnableRfdcBufg2xLcl : std_logic := '0';

  -- Active high version of reset required for synchronizer blocks.
  signal rPllReset : std_logic;

  -- Since these signals control sensitive components (clock enables), we apply
  -- a dont_touch attribute to preserve the signals through both synthesis and
  -- P&R. Implementation of "dont_touch" has been confirmed after P&R.
  attribute dont_touch : string;
  attribute dont_touch of rEnableDataBufg1x : signal is "TRUE";
  attribute dont_touch of rEnableDataBufg2x : signal is "TRUE";
  attribute dont_touch of aEnableRfBufg1x   : signal is "TRUE";
  attribute dont_touch of aEnableRfBufg2x   : signal is "TRUE";

  attribute X_INTERFACE_INFO      : string;
  attribute X_INTERFACE_PARAMETER : string;

  attribute X_INTERFACE_INFO of DataClk1xPll    : signal is
    "xilinx.com:signal:clock:1.0 DataClk1xPll CLK";
  attribute X_INTERFACE_INFO of DataClk2xPll    : signal is
    "xilinx.com:signal:clock:1.0 DataClk2xPll CLK";

begin

  rPllReset <= not rPllReset_n;

  -- Assert rGatedBaseClksValid once the PLL has been locked for the specified
  -- time.
  rGatedBaseClksValid <= rPllLockedLcl;

  DataClkEnables : process(ReliableClk)
  begin
    if rising_edge(ReliableClk) then
      if rPllReset_n = '0' then
        rEnableDataBufg1x    <= '0';
        rEnableDataBufg2x    <= '0';
        rEnableRfdcBufg1xLcl <= '0';
        rEnableRfdcBufg2xLcl <= '0';
      else
        rEnableDataBufg1x <=
          rSafeToEnableGatedClks and
          rEnableDataClk1x and
          (not rPllUnlockedSticky);

        rEnableDataBufg2x <=
          rSafeToEnableGatedClks and
          rEnableDataClk2x and
          (not rPllUnlockedSticky);

        rEnableRfdcBufg1xLcl <=
          rSafeToEnableGatedClks and
          rEnableRfdcClk1x and
          (not rPllUnlockedSticky);

        rEnableRfdcBufg2xLcl <=
          rSafeToEnableGatedClks and
          rEnableRfdcClk2x and
          (not rPllUnlockedSticky);
      end if;
    end if;
  end process DataClkEnables;

  aEnableRfBufg1x(0) <= rEnableRfdcBufg1xLcl;
  aEnableRfBufg2x(0) <= rEnableRfdcBufg2xLcl;

  DataClk1xSafeBufg: BUFGCE
    generic map(
      CE_TYPE => "ASYNC"
    )
    port map (
      I  => DataClk1xPll,
      CE => rEnableDataBufg1x,
      O  => DataClk1x
    );

  DataClk2xSafeBufg: BUFGCE
    generic map(
      CE_TYPE => "ASYNC"
    )
    port map (
      I  => DataClk2xPll,
      CE => rEnableDataBufg2x,
      O  => DataClk2x
    );


  -----------------------------------------------------------------------------
  -- Create PLL Lock Signal
  -----------------------------------------------------------------------------
  -- Double-sync the incoming aPllLocked signal from the PLL.

  aPllLockedLcl(0) <= aPllLocked;

  DataClkPllLockedDS: sync_wrapper
    generic map (
      WIDTH            => 1,
      STAGES           => open,
      INITIAL_VAL      => open,
      FALSE_PATH_TO_IN => open)
    port map (
      clk        => ReliableClk,
      rst        => rPllReset,
      signal_in  => aPllLockedLcl,
      signal_out => rPllLockedDs
    );

  -- Filter the Lock signal. Assert a lock when the PLL lock signal has been
  -- asserted for kPllLockTimeNs
  --
  -- !!! SAFE COUNTER STARTUP !!!
  -- rLockedFilterCount cannot start incrementing until rPllReset_n is
  -- de-asserted. Once rPllReset_n is de-asserted through a AXI access, input
  -- values for the registers in this state machine will not change until the
  -- MMCM locks and the double synchronizer reflects a locked status, making
  -- this start-up safe.
  PllLockFilter: process (ReliableClk)
  begin
    if rising_edge(ReliableClk) then
      if rPllReset_n = '0' then
        rLockedFilterCount <= kMaxPllLockCount-1;
        rPllLockedLcl      <= '0';
      else
        if rPllLockedDs(0) = '1' then
          if rLockedFilterCount = 0 then
            rPllLockedLcl <= '1';
          else
            rPllLockedLcl <= '0';
            rLockedFilterCount <= rLockedFilterCount - 1;
          end if;
        else
          rLockedFilterCount <= kMaxPllLockCount-1;
          rPllLockedLcl      <= '0';
        end if;
      end if;
    end if;
  end process PllLockFilter;

  -- Sticky bit to hold '1' if PLL ever comes unlocked
  PllStickyBit: process (ReliableClk)
  begin
    if rising_edge(ReliableClk) then
      if (not rPllReset_n or rClearDataClkUnlockedSticky) = '1' then
        rPllUnlockedSticky <= '0';
      else
        if rPllLockedLcl = '1' and rPllLockedDs(0) = '0' then
          rPllUnlockedSticky <= '1';
        end if;
      end if;
    end if;
  end process;

  rPllLocked <= rPllLockedLcl;

  -- AXI transaction decoding
  rClearDataClkUnlockedSticky <= rSoftwareControl(kCLEAR_DATA_CLK_UNLOCKED);
  rEnableDataClk1x            <= rSoftwareControl(kENABLE_DATA_CLK);
  rEnableDataClk2x            <= rSoftwareControl(kENABLE_DATA_CLK_2X);
  rEnableRfdcClk1x            <= rSoftwareControl(kENABLE_RF_CLK);
  rEnableRfdcClk2x            <= rSoftwareControl(kENABLE_RF_CLK_2X);

  rSoftwareStatus(kDATA_CLK_PLL_LOCKED)          <= rPllLockedLcl;
  rSoftwareStatus(kDATA_CLK_PLL_UNLOCKED_STICKY) <= rPllUnlockedSticky;

end STRUCT;

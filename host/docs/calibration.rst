========================================================================
UHD - Calibration Application Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Self-calibration
------------------------------------------------------------------------
UHD comes with several self-calibration utilities for minimizing IQ imbalance and DC offset.
These utilities perform calibration sweeps using transmit leakage into the receive path
(special equipment is not required).
The results from a calibration are written to a CSV file in the user's home directory.
UHD will automatically apply corrections at runtime when the user re-tunes the daughterboard LO.
Calibration results are specific to an individual RF board.

**Note:**
When a calibration table is present,
and the user wishes to override the calibration settings through the API:
the user should re-apply the desired setting every time the LO is re-tuned.

UHD comes with the following calibration utilities:

 * **uhd_cal_rx_iq_balance:** - mimimizes RX IQ imbalance vs. LO frequency
 * **uhd_cal_tx_dc_offset:** - mimimizes TX DC offset vs. LO frequency
 * **uhd_cal_tx_iq_balance:** - mimimizes TX IQ imbalance vs. LO frequency

The following RF frontends are supported by the self-calibration utilities:

 * WBX transceiver board
 * SBX transceiver board
 * RFX transceiver board

********************************************
Calibration Utilities
********************************************
UHD installs the calibration utilities into **<install-path>/bin**.
**Disconnect** any external hardware from the RF antenna ports,
and run the following from the command line.
Each utility will take several minutes to complete.
::

    uhd_cal_rx_iq_balance --verbose --args=<optional device args>
    uhd_cal_tx_iq_balance --verbose --args=<optional device args>
    uhd_cal_tx_dc_offset --verbose --args=<optional device args>

See the output given by --help for more advanced options, such as:
manually choosing the frequency range and step size for the sweeps.

**Note:**
Your daughterboard needs a serial number to run a calibration utility. Some older daughterboards
may not have a serial number. If this is the case, run the following command to burn a serial number
into the daughterboard's EEPROM:
::

    <install dir>/share/uhd/utils/usrp_burn_db_eeprom --ser=<desired serial> --args=<optional device args>

********************************************
Calibration Data
********************************************
Calibration files are stored in the user's home/application directory.
They can easily be moved from machine to another by copying the "cal" directory.
Re-running a calibration utility will replace the existing calibration file.
The old calibration file will be renamed so it may be recovered by the user.

 * **Linux:** ${HOME}/.uhd/cal/
 * **Windows:** %APPDATA%\\.uhd\\cal\\


========================================================================
UHD - Daughterboard Application Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Daughterboard Properties
------------------------------------------------------------------------

The following contains interesting notes about each daughterboard.
Eventually, this page will be expanded to list out the full
properties of each board as well.

^^^^^^^^^^^^^^^^^^^^^^^^^^^
Basic RX and LFRX
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The Basic RX and LFRX boards have 4 frontends:

* **Frontend A:** real signal on antenna RXA
* **Frontend B:** real signal on antenna RXB
* **Frontend AB:** quadrature frontend using both antennas (IQ)
* **Frontend BA:** quadrature frontend using both antennas (QI)

The boards have no tunable elements or programmable gains.
Through the magic of aliasing, you can down-convert signals
greater than the Nyquist rate of the ADC.

BasicRX Bandwidth (Hz): 

* **For Real-Mode (A or B frontend)**: 250M
* **For Complex (AB or BA frontend)**: 500M

LFRX Bandwidth (Hz):

* **For Real-Mode (A or B frontend)**: 33M
* **For Complex (AB or BA frontend)**: 66M

^^^^^^^^^^^^^^^^^^^^^^^^^^^
Basic TX and LFTX
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The Basic TX and LFTX boards have 4 frontends:

* **Frontend A:** real signal on antenna TXA
* **Frontend B:** real signal on antenna TXB
* **Frontend AB:** quadrature frontend using both antennas (IQ)
* **Frontend BA:** quadrature frontend using both antennas (QI)

The boards have no tunable elements or programmable gains.
Through the magic of aliasing, you can up-convert signals
greater than the Nyquist rate of the DAC.

BasicTX Bandwidth (Hz): 250M

* **For Real-Mode (A or B frontend**): 250M
* **For Complex (AB or BA frontend)**: 500M

LFTX Bandwidth (Hz): 33M

* **For Real-Mode (A or B frontend)**: 33M
* **For Complex (AB or BA frontend)**: 66M

^^^^^^^^^^^^^^^^^^^^^^^^^^^
DBSRX
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The DBSRX board has 1 quadrature frontend.  
It defaults to direct conversion but can use a low IF through lo_offset in **uhd::tune_request_t**.

Receive Antennas: **J3**

* **Frontend 0:** Complex baseband signal from antenna J3

The board has no user selectable antenna setting.

Receive Gains:

* **GC1**, Range: 0-56dB
* **GC2**, Range: 0-24dB

Bandwidth (Hz): 8M-66M

Sensors:

* **lo_locked**: boolean for LO lock state

^^^^^^^^^^^^^^^^^^^^^^^^^^^
DBSRX2
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The DBSRX2 board has 1 quadrature frontend.
It defaults to direct conversion, but can use a low IF through lo_offset in **uhd::tune_request_t**.

Receive Antennas: **J3**

* **Frontend 0:** Complex baseband signal from antenna J3

The board has no user-selectable antenna setting.

Receive Gains:

* **GC1**, Range: 0-73dB
* **BBG**, Range: 0-15dB

Bandwidth (Hz): 8M-80M

Sensors:

* **lo_locked**: boolean for LO lock state

^^^^^^^^^^^^^^^^^^^^^^^^^^^
RFX Series
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The RFX Series boards have 2 quadrature frontends: Transmit and Receive.
Transmit defaults to low IF, and Receive defaults to direct conversion.
The IF can be adjusted through lo_offset in **uhd::tune_request_t**.

The RFX Series boards have independent receive and transmit LO's and synthesizers 
allowing full-duplex operation on different transmit and receive frequencies.

Transmit Antennas: **TX/RX**

Receive Antennas: **TX/RX** or **RX2**

* **Frontend 0:** Complex baseband signal for selected antenna

The user may set the receive antenna to be TX/RX or RX2.
However, when using an RFX board in full-duplex mode,
the receive antenna will always be set to RX2, regardless of the settings.

Receive Gains: **PGA0**, Range: 0-70dB (except RFX400 range is 0-45dB)

Bandwidths (Hz):

* **RX**: 40M
* **TX**: 40M

Sensors:

* **lo_locked**: boolean for LO lock state

^^^^^^^^^^^^^^^^^^^^^^^^^^^
XCVR 2450
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The XCVR2450 has 2 quadrature frontends, one transmit, one receive.
Transmit and Receive default to direct conversion but
can be used in low IF mode through lo_offset in uhd::tune_request_t

The XCVR2450 has a non-contiguous tuning range consisting of a 
high band (4.9-6.0GHz) and a low band (2.4-2.5GHz).

Transmit Antennas: **J1** or **J2**

Receive Antennas: **J1** or **J2**

* **Frontend 0:** Complex baseband signal for selected antenna

The XCVR2450 uses a common LO for both receive and transmit.
Even though the API allows the RX and TX LOs to be individually set,
a change of one LO setting will be reflected in the other LO setting.

The XCVR2450 does not support full-duplex mode, attempting to operate 
in full-duplex will result in transmit-only operation.

Transmit Gains:

* **VGA**, Range: 0-30dB
* **BB**, Range: 0-5dB

Receive Gains:

* **LNA**, Range: 0-30.5dB
* **VGA**, Range: 0-62dB

Bandwidths (Hz):

* **RX**: 15M, 19M, 28M, 36M; (each +-0, 5, or 10%)
* **TX**: 24M, 36M, 48M

Sensors:

* **lo_locked**: boolean for LO lock state
* **rssi**:      float for rssi in dBm

^^^^^^^^^^^^^^^^^^^^^^^^^^^
WBX Series
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The WBX Series boards have 2 quadrature frontends, one transmit, one receive.
Transmit and Receive default to direct conversion but
can be used in low IF mode through lo_offset in **uhd::tune_request_t**.

The WBX Series boards have independent receive and transmit LO's and synthesizers 
allowing full-duplex operation on different transmit and receive frequencies.

Transmit Antennas: **TX/RX**

Receive Antennas: **TX/RX** or **RX2**

* **Frontend 0:** Complex baseband signal for selected antenna

The user may set the receive antenna to be TX/RX or RX2.
However, when using an WBX board in full-duplex mode,
the receive antenna will always be set to RX2, regardless of the settings.

Transmit Gains: **PGA0**, Range: 0-25dB

Receive Gains: **PGA0**, Range: 0-31.5dB

Bandwidths (Hz):

* **RX**: 40M
* **TX**: 40M

Sensors:

* **lo_locked**: boolean for LO lock state

^^^^^^^^^^^^^^^^^^^^^^^^^^^
SBX Series
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The SBX Series boards have 2 quadrature frontends, one transmit, one receive.
Transmit and Receive default to direct conversion but
can be used in low IF mode through lo_offset in **uhd::tune_request_t**.

The SBX Series boards have independent receive and transmit LO's and synthesizers 
allowing full-duplex operation on different transmit and receive frequencies.

Transmit Antennas: **TX/RX**

Receive Antennas: **TX/RX** or **RX2**

* **Frontend 0:** Complex baseband signal for selected antenna

The user may set the receive antenna to be TX/RX or RX2.
However, when using an SBX board in full-duplex mode,
the receive antenna will always be set to RX2, regardless of the settings.

Transmit Gains: **PGA0**, Range: 0-31.5dB

Receive Gains: **PGA0**, Range: 0-31.5dB

Bandwidths (Hz):

* **RX**: 40M
* **TX**: 40M

Sensors:

* **lo_locked**: boolean for LO lock state

LEDs:

* All LEDs flash when dboard control is initialized
* **TX LD**: Transmit Synthesizer Lock Detect
* **TX/RX**: Receiver on TX/RX antenna port (No TX)
* **RX LD**: Receive Synthesizer Lock Detect
* **RX1/RX2**: Receiver on RX2 antenna port

^^^^^^^^^^^^^^^^^^^^^^^^^^^
TVRX
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The TVRX board has 1 real-mode frontend.
It is operated at a low IF.

Receive Antennas: RX

* **Frontend 0:** real-mode baseband signal from antenna RX

Receive Gains:

* **RF**, Range: -13.3-50.3dB (frequency-dependent)
* **IF**, Range: -1.5-32.5dB

Bandwidth: 6MHz

^^^^^^^^^^^^^^^^^^^^^^^^^^^
TVRX2
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The TVRX2 board has 2 real-mode frontends.
It is operated at a low IF.

Receive Frontends:

* **Frontend RX1:** real-mode baseband from antenna J100
* **Frontend RX2:** real-mode baseband from antenna J140

Note: The TVRX2 has always-on AGC; the software controllable gain is the
final gain stage which controls the AGC set-point for output to ADC.

Receive Gains:

* **IF**, Range: 0.0-30.0dB

Bandwidth: 1.7MHz, 6MHz, 7MHz, 8MHz, 10MHz

Sensors:

* **lo_locked**: boolean for LO lock state
* **rssi**: float for measured RSSI in dBm
* **temperature**: float for measured temperature in degC

------------------------------------------------------------------------
Daughterboard Modifications
------------------------------------------------------------------------

Sometimes, daughterboards will require modification
to work on certain frequencies or to work with certain hardware.
Modification usually involves moving/removing an SMT component
and burning a new daughterboard ID into the EEPROM.

^^^^^^^^^^^^^^^^^^^^^^^^^^^
DBSRX - Mod
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Due to different clocking capabilities,
the DBSRX will require modifications to operate on a non-USRP1 motherboard.
On a USRP1 motherboard, a divided clock is provided from an FPGA pin
because the standard daughterboard clock lines cannot provided a divided clock.
However, on other USRP motherboards, the divided clock is provided
over the standard daughterboard clock lines.

**Step 1: Move the clock configuration resistor**

Remove **R193** (which is 10 ohms, 0603 size), and put it on **R194**, which is empty.
This is made somewhat more complicated by the fact that the silkscreen is not clear in that area.
**R193** is on the back, immediately below the large beige connector, **J2**.
**R194** is just below, and to the left of **R193**.
The silkscreen for **R193** is ok, but for **R194**,
it is upside down, and partially cut off.
If you lose **R193**, you can use anything from 0 to 10 ohms there.

**Step 2: Burn a new daughterboard id into the EEPROM**

With the daughterboard plugged-in, run the following commands:
::

    cd <install-path>/share/uhd/utils
    ./usrp_burn_db_eeprom --id=0x000d --unit=RX --args=<args> --slot=<slot>

* **<args>** are device address arguments (optional if only one USRP is on your machine)
* **<slot>** is the name of the daughterboard slot (optional if the USRP has only one slot)

^^^^^^^^^^^^^^^^^^^^^^^^^^^
RFX - Mod
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Older RFX boards require modifications to use the motherboard oscillator.
If this is the case, UHD will print a warning about the modification.
Please follow the modification procedures below:

**Step 1: Disable the daughterboard clocks**

Move **R64** to **R84**. Move **R142** to **R153**.

**Step 2: Connect the motherboard blocks**

Move **R35** to **R36**. Move **R117** to **R115**.
These are all 0-ohm, so if you lose one, just short across the appropriate pads.

**Step 3: Burn the appropriate daughterboard ID into the EEPROM**

With the daughterboard plugged-in, run the following commands:
::

    cd <install-path>/share/uhd/utils
    ./usrp_burn_db_eeprom --id=<rx_id> --unit=RX --args=<args> --slot=<slot>
    ./usrp_burn_db_eeprom --id=<tx_id> --unit=TX --args=<args> --slot=<slot>

* **<rx_id>** choose the appropriate RX ID for your daughterboard

  * **RFX400:** 0x0024
  * **RFX900:** 0x0025
  * **RFX1800:** 0x0034
  * **RFX1200:** 0x0026
  * **RFX2400:** 0x0027
* **<tx_id>** choose the appropriate TX ID for your daughterboard

  * **RFX400:** 0x0028
  * **RFX900:** 0x0029
  * **RFX1800:** 0x0035
  * **RFX1200:** 0x002a
  * **RFX2400:** 0x002b
* **<args>** are device address arguments (optional if only one USRP is on your machine)
* **<slot>** is the name of the daughterboard slot (optional if the USRP has only one slot)

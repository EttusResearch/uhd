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
Basic RX and and LFRX
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The Basic RX and LFRX boards have 4 subdevices:

* **Subdevice A:** real signal on antenna RXA
* **Subdevice B:** real signal on antenna RXB
* **Subdevice AB:** quadrature subdevice using both antennas (IQ)
* **Subdevice BA:** quadrature subdevice using both antennas (QI)

The boards have no tunable elements or programmable gains.
Though the magic of aliasing, you can down-convert signals
greater than the Nyquist rate of the ADC.

BasicRX Bandwidth (Hz): 250M

LFRX Bandwidth (Hz): 30M

^^^^^^^^^^^^^^^^^^^^^^^^^^^
Basic TX and and LFTX
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The Basic TX and LFTX boards have 4 subdevices:

* **Subdevice A:** real signal on antenna TXA
* **Subdevice B:** real signal on antenna TXB
* **Subdevice AB:** quadrature subdevice using both antennas (IQ)
* **Subdevice BA:** quadrature subdevice using both antennas (QI)

The boards have no tunable elements or programmable gains.
Though the magic of aliasing, you can up-convert signals
greater than the Nyquist rate of the DAC.

BasicTX Bandwidth (Hz): 250M

LFTX Bandwidth (Hz): 30M

^^^^^^^^^^^^^^^^^^^^^^^^^^^
DBSRX
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The DBSRX board has 1 quadrature subdevice.

Receive Antennas: **J3**

The board has no user selectable antenna setting

Receive Gains: 
    **GC1**, Range: 0-56dB
    **GC2**, Range: 0-24dB

Bandwidth (Hz): 8M-66M

^^^^^^^^^^^^^^^^^^^^^^^^^^^
RFX Series
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Transmit Antennas: **TX/RX**

Receive Antennas: **TX/RX** or **RX2**

The user may set the receive antenna to be TX/RX or RX2.
However, when using an RFX board in full-duplex mode,
the receive antenna will always be set to RX2, regardless of the settings.

Receive Gains: **PGA0**, Range: 0-70dB (except RFX400 range is 0-45dB)

Bandwidths (Hz):
 * **RX**: 40M
 * **TX**: 40M

^^^^^^^^^^^^^^^^^^^^^^^^^^^
XCVR 2450
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The XCVR2450 has a non-contiguous tuning range consisting of a 
high band (4.9-6.0GHz) and a low band (2.4-2.5GHz).

Transmit Antennas: **J1** or **J2**

Receive Antennas: **J1** or **J2**

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

^^^^^^^^^^^^^^^^^^^^^^^^^^^
WBX Series
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Transmit Antennas: **TX/RX**

Receive Antennas: **TX/RX** or **RX2**

The user may set the receive antenna to be TX/RX or RX2.
However, when using an WBX board in full-duplex mode,
the receive antenna will always be set to RX2, regardless of the settings.

Transmit Gains: **PGA0**, Range: 0-25dB

Receive Gains: **PGA0**, Range: 0-31.5dB

Bandwidths (Hz):
 * **RX**: 40M
 * **TX**: 40M

^^^^^^^^^^^^^^^^^^^^^^^^^^^
TVRX
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Receive Antennas: RX

Receive Gains:
 * **RF**, Range: -13.3-50.3dB (frequency-dependent)
 * **IF**, Range: -1.5-32.5dB

Bandwidth: 6MHz

------------------------------------------------------------------------
Daughterboard Modifications
------------------------------------------------------------------------

Sometimes, daughterboards will require modification
to work on certain frequencies or to work with certain hardware.
Modification usually involves moving/removing a SMT component
and burning a new daughterboard id into the eeprom.

^^^^^^^^^^^^^^^^^^^^^^^^^^^
DBSRX
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Due to different clocking capabilities,
the DBSRX will require modifications to operate on a non-USRP1 motherboard.
On a USRP1 motherboard, a divided clock is provided from an FPGA pin
because the standard daughterboard clock lines cannot provided a divided clock.
However, on other USRP motherboards, the divided clock is provided
over the standard daughterboard clock lines.

**Step 1: Move the clock configuration resistor**

Remove R193 (which is 10 ohms, 0603 size) and put it on R194, which is empty.
This is made somewhat more complicated by the fact that the silkscreen is not clear in that area.
R193 is on the back, immediately below the large beige connector, J2.
R194 is just below, and to the left of R193.
The silkscreen for R193 is ok, but for R194,
it is upside down, and partially cut off.
If you lose R193, you can use anything from 0 to 10 ohms there.

**Step 2: Burn a new daughterboard id into the EEPROM**

With the daughterboard plugged-in, run the following commands:
::

    cd <prefix>/share/uhd/utils
    ./usrp_burn_db_eeprom --id=0x000d --unit=RX --args=<args> --slot=<slot>

* <args> are device address arguments (optional if only one USRP is on your machine)
* <slot> is the name of the daughterboard slot (optional if the USRP has only one slot)

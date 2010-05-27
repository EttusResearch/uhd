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
The Basic RX and LFRX boards have 3 subdevices:

* **Subdevice A:** real signal on antenna RXA
* **Subdevice B:** real signal on antenna RXB
* **Subdevice AB:** quadrature subdevice using both antennas

The boards have no tunable elements or programmable gains.
Though the magic of aliasing, you can down-convert signals
greater than the nyquist rate of the ADC.

^^^^^^^^^^^^^^^^^^^^^^^^^^^
Basic TX and and LFTX
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The Basic TX and LFTX boards have 1 quadrature subdevice using both antennas.

The boards have no tunable elements or programmable gains.
Though the magic of aliasing, you can up-convert signals
greater than the nyquist rate of the DAC.

^^^^^^^^^^^^^^^^^^^^^^^^^^^
RFX Series
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Transmit Antennas: **TX/RX**

Receive Antennas: **TX/RX** or **RX2**

The user may set the receive antenna to be TX/RX or RX2.
However, when using an RFX board in full-duplex mode,
the receive antenna will always be set to RX2, regardless of the settings.

^^^^^^^^^^^^^^^^^^^^^^^^^^^
XCVR 2450
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The XCVR2450 has a non-contiguous tuning range consiting of a high band and a low band.
The high band consists of frequencies between...TODO

Transmit Antennas: **J1** or **J2**

Receive Antennas: **J1** or **J2**

When using the XCVR2450 in full-duplex mode,
the user must set the receive antenna and the transmit antenna to be different;
not doing so will yeild undefined results.

The XCVR2450 uses a common LO for both receive and transmit.
Even though the API allows the RX and TX LOs to be individually set,
a change of one LO setting will be reflected in the other LO setting.

^^^^^^^^^^^^^^^^^^^^^^^^^^^
WBX Series
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Transmit Antennas: **TX/RX**

Receive Antennas: **TX/RX** or **RX2**

The user may set the receive antenna to be TX/RX or RX2.
However, when using an RFX board in full-duplex mode,
the receive antenna will always be set to RX2, regardless of the settings.

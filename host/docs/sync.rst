========================================================================
UHD - Synchronization Application Notes
========================================================================

.. contents:: Table of Contents

The following application notes explain how to synchronize multiple USRPs
with the goal of transmitting or receiving time-aligned samples.

**Note:** The following synchronization notes do not apply to USRP1
which does not support the advanced features available in newer products.

------------------------------------------------------------------------
Common reference signals
------------------------------------------------------------------------
USRPs take two reference signals in order to synchronize clocks and time.

* A 10MHz reference to synchronize the clocks across devices.
* A pulse-per-second to synchronize the time across devices.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Provide reference signals
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Most USRPs have front panel SMA connectors to source these reference signals.
Typically, these signals are provided by an external GPSDO.
Some USRP models can provide these signals from an optional internal GPSDO.
For user's generating their own signals,
the pulse-per-second should be clocked from the 10MHz reference.
See the application notes for your device for specific signal requirements.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Set the clock configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The user should set the "external" clock configuration when configuring the USRP device object.
::

    usrp->set_clock_config(uhd::clock_config_t::external());

------------------------------------------------------------------------
Synchronizing the device time
------------------------------------------------------------------------
The purpose of the PPS signal is to synchronously latch a time into the device.
The device time may be absolute (GPS, UTC) or relative.
The user should decide what is most useful to the application.
To synchronously set the device time,
the user should determine the PPS edge,
and then make the API call set_time_next_pps(...).
The user can determine the PPS edge in a variety of ways...

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Method 1 - poll the USRP time registers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
One way to catch the PPS edge is to poll the "last PPS" time from the USRP device.
When the last PPS time increments, the user can determine that a PPS has occurred.

::

    const uhd::time_spec_t last_pps_time = usrp->get_time_last_pps();
    while (last_pps_time != usrp->get_time_last_pps()){
        //sleep 100 milliseconds (give or take)
    }
    usrp->set_time_next_pps(uhd::time_spec_t(0.0));

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Method 2 - query the GPSDO for seconds
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Most GPSDO can be configured to output a NMEA string over the serial port once every PPS.
The user can wait for this string to determine the PPS edge,
and the user can also parse this string to determine GPS time.

::

    //call user's function to wait for NMEA message...
    usrp->set_time_next_pps(uhd::time_spec_t(0.0));

    -- OR --

    //call user's function to wait for NMEA message...
    //call user's function to parse the NMEA message...
    usrp->set_time_next_pps(uhd::time_spec_t(gps_time+1));

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Method 3 - query the gps_time sensor
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
This is a variant of method 2 for USRPs with internal GPSDOs.
The user can query the gps_time sensor to wait for the NMEA string.

::

    //wait for NMEA string from internal GPSDO
    usrp->get_mboard_sensor("gps_time");
    usrp->set_time_next_pps(uhd::time_spec_t(0.0));

    -- OR --

    //wait for the NMEA string and set GPS time
    const time_t gps_time = usrp->get_mboard_sensor("gps_time").to_int();
    usrp->set_time_next_pps(uhd::time_spec_t(gps_time+1));

------------------------------------------------------------------------
Synchronizing channel phase
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Align CORDICs in the DSP
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The frequency translation between baseband and IF can introduce a phase ambiguity between channels.
This is due to the property that the cordic in the DSP chain can have a random phase offset.
Fortunately, the CORDIC is reset on each start-of-burst.
Therefore, the method to phase-align the cordics is to coordinate streaming at a particular time.

* For receive, a burst is started when the user issues a stream command. This stream command should have a time spec set.
* For transmit, a burst is started when the user calls send. The metadata should have a time spec and start of burst set.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Align LOs in the front-end
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
After tuning the RF front-ends,
each local oscillator will have a random phase offset.
Some daughterboards have mechanisms to phase-align the oscillators,
however, these mechanisms are not currently implemented.
In any case, LO phase-alignment will not forgo the need for calibration.

**RX Calibration**

1) Tune the setup.
2) Receive known signals.
3) Capture calibration data.
4) Do not re-tune.
5) Receive live data.

**TX Calibration**

1) Tune the setup.
2) Transmit known signals.
3) Capture calibration data.
4) Do not re-tune.
5) Transmit live data.

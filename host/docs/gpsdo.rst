========================================================================
UHD - Internal GPSDO Application Notes
========================================================================

.. contents:: Table of Contents

This application note describes the use of integrated GPS-disciplined
oscillators with Ettus Research USRP devices. It pertains specifically
to the Jackson Labs Firefly-1A device unless noted otherwise.

------------------------------------------------------------------------
Specifications
------------------------------------------------------------------------
* **Receiver type**: 50 channel with WAAS, EGNOS, MSAS
* **10MHz ADEV**: 1e-11 over >24h
* **1PPS RMS jitter**: <50ns 1-sigma
* **Holdover**: <11us over 3h
* **Phase noise**:

  * **1Hz:** -80dBc/Hz
  * **10Hz:** -110dBc/Hz
  * **100Hz:** -135dBc/Hz
  * **1kHz:** -145dBc/Hz
  * **10kHz:** <-145dBc/Hz

**Antenna Types:**

The GPSDO is capable of supplying a 3V for active GPS antennas or supporting passive antennas.

------------------------------------------------------------------------
Installation Instructions
------------------------------------------------------------------------
Installation instructions can be found here:
`http://www.ettus.com/content/files/gpsdo-kit_2.pdf <http://www.ettus.com/content/files/gpsdo-kit_2.pdf>`_

********************************************
Post-installation Task (N-Series only)
********************************************

**Note:** The following instructions are only necessary for UHD 3.4.* and below.

This is necessary if you require absolute GPS time in your application
or need to communicate with the GPSDO to obtain location, satellite info, etc.
If you only require 10MHz and PPS signals for reference or MIMO use
(see the `Synchronization Application Notes <./sync.html>`_),
it is not necessary to perform this step.

To configure the USRP to communicate with the GPSDO, use the
**usrp_burn_mb_eeprom** utility:

::

    cd <install-path>/share/uhd/utils
    ./usrp_burn_mb_eeprom --args=<optional device args> --key=gpsdo --val=internal

    -- restore original setting --
    ./usrp_burn_mb_eeprom --args=<optional device args> --key=gpsdo --val=none

------------------------------------------------------------------------
Using the GPSDO in Your Application
------------------------------------------------------------------------
By default, if a GPSDO is detected at startup, the USRP will be configured
to use it as a frequency and time reference. The internal VITA timestamp
will be initialized to the GPS time, and the internal oscillator will be
phase-locked to the 10MHz GPSDO reference. If the GPSDO is not locked to
satellites, the VITA time will not be initialized.

GPS data is obtained through the **mboard_sensors** interface. To retrieve
the current GPS time, use the **gps_time** sensor:

::

    usrp->get_mboard_sensor("gps_time");

The returned value will be the current epoch time, in seconds since
January 1, 1970. This value is readily converted into human-readable
format using the **time.h** library in C, **boost::posix_time** in C++, etc.

Other information can be fetched as well. You can query the lock status
with the **gps_locked** sensor, as well as obtain raw NMEA sentences using
the **gps_gprmc**, and **gps_gpgga** sensors. Location
information can be parsed out of the **gps_gpgga** sensor by using **gpsd** or
another NMEA parser.

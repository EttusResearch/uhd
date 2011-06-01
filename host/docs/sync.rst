========================================================================
UHD - Synchronization Application Notes
========================================================================

.. contents:: Table of Contents

The following application notes explain how to synchronize multiple USRPs
with the goal of transmitting or receiving time-aligned samples for MIMO
or other applications requiring multiple USRPs operating synchronously.

**Note:** The following synchronization notes do not apply to USRP1,
which does not support the advanced features available in newer products.

------------------------------------------------------------------------
Common reference signals
------------------------------------------------------------------------
USRPs take two reference signals in order to synchronize clocks and time:

* A 10MHz reference to provide a single frequency reference for both devices, and
* A pulse-per-second (1PPS) to synchronize the sample time across devices.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Provide reference signals
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Most USRPs have front panel SMA connectors to source these reference signals.
Typically, these signals are provided by an external GPSDO.
Some USRP models can provide these signals from an optional internal GPSDO.
For users generating their own signals,
the pulse-per-second should be clocked from the 10MHz reference.
See the application notes for your device for specific signal requirements.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Set the clock configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In order to synchronize to an external clock, configure the USRP device
using the "external" clock configuration:

::

    usrp->set_clock_config(uhd::clock_config_t::external());

------------------------------------------------------------------------
Synchronizing the device time
------------------------------------------------------------------------
The purpose of the PPS signal is to synchronously latch a time into the device.
You can use the set_time_next_pps(...) function to either initialize the sample time to 0,
or to an absolute time such as GPS time or UTC time.
For the purposes of synchronizing devices,
it doesn't matter what time you initialize to when using set_time_next_pps(...).

Some GPSDOs synchronize their PPS with the rising edge of the 10MHz clock,
and some synchronize with the falling edge of the clock.
You should find out which edge your GPSDO uses,
and then pass that in as an argument to set_time_next_pps(...).

Here are two examples of how to set the PPS time to synchronize USRPs
to a clock source.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Method 1 - poll the USRP time registers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
One way to initialize the PPS edge is to poll the "last PPS" time from the USRP device.
When the last PPS time increments, the user can determine that a PPS has occurred:

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
and the user can also parse this string to determine GPS time:

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
In order to achieve phase alignment between USRPs, the CORDICS in both
devices must be aligned with respect to each other. This is easily achieved
by issuing stream commands with a time spec property, which instructs the
streaming to begin at a specified time. Since the devices are already
synchronized via the 10MHz and PPS inputs, the streaming will start at exactly
the same time on both devices. The CORDICs are reset at each start-of-burst
command, so users should ensure that every start-of-burst also has a time spec set.

For receive, a burst is started when the user issues a stream command. This stream command should have a time spec set.

::

    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps = spb;
    stream_cmd.stream_now = false;
    stream_cmd.time_spec = uhd::time_spec_t(seconds_in_future);
    usrp->issue_stream_cmd(stream_cmd);

For transmit, a burst is started when the user calls send(). The metadata should have a time spec and start of burst set.

::

    uhd::tx_metadata_t md;
    md.start_of_burst = true;
    md.end_of_burst = false;
    md.has_time_spec = true;
    md.time_spec = uhd::time_spec_t(time_to_send);

    //send a single packet
    size_t num_tx_samps = usrp->get_device()->send(
        buffs, samps_to_send, md,
        uhd::io_type_t::COMPLEX_FLOAT32,
        uhd::device::SEND_MODE_ONE_PACKET, timeout
    );

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Align LOs in the front-end
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
After tuning the RF front-ends,
each local oscillator may have a random phase offset due to the dividers
in the VCO/PLL chains. This offset will remain constant after the device
has been initialized, and will remain constant until the device is closed
or re-tuned. This phase offset is typically removed by the user in MIMO
applications, using a training sequence to estimate the offset. It will
be necessary to re-align the LOs after each tune command.

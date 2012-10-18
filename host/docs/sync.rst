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
Common Reference Signals
------------------------------------------------------------------------
USRPs take two reference signals in order to synchronize clocks and time:

* A 10MHz reference to provide a single frequency reference for both devices.
* A pulse-per-second (PPS) to synchronize the sample time across devices.
* A MIMO cable transmits an encoded time message from one device to another.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
PPS and 10 MHz reference signals
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Connect the front panel SMA connectors to the reference sources.
Typically, these signals are provided by an external GPSDO.
However, some USRP models can provide these signals from an optional internal GPSDO.

::

    usrp->set_clock_source("external");
    usrp->set_time_source("external");

**Note:**
Sometimes the delay on the PPS signal will cause it to arrive inside the timing
margin the FPGA sampling clock, causing PPS edges to be separated by less or
more than 100 million cycles of the FPGA clock. If this is the case,
you can change the edge reference of the PPS signal with this parameter:

::

    usrp->set_time_source("_external_");

**Note2:**
For users generating their own signals for the external SMA connectors,
the PPS should be clocked from the 10MHz reference.
See the application notes for your device for specific signal requirements.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
MIMO cable reference signals
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Use the MIMO expansion cable to share reference sources (USRP2 and N-Series).
The MIMO cable can be used synchronize one device to another device.
Users of the MIMO cable may use Method 1 (explained below) to synchronize multiple pairs of devices.

::

    usrp->set_clock_source("mimo");
    usrp->set_time_source("mimo");

------------------------------------------------------------------------
Synchronizing the Device Time
------------------------------------------------------------------------
The purpose of the PPS signal is to synchronously latch a time into the device.
You can use the **set_time_next_pps(...)** function to either initialize the sample time to 0
or an absolute time, such as GPS time or UTC time.
For the purposes of synchronizing devices,
it doesn't matter what time you initialize to when using **set_time_next_pps(...)**.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Method 1 - poll the USRP time registers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
One way to initialize the PPS edge is to poll the "last PPS" time from the USRP device.
When the last PPS time increments, the user can determine that a PPS has occurred:

::

    const uhd::time_spec_t last_pps_time = usrp->get_time_last_pps();
    while (last_pps_time == usrp->get_time_last_pps()){
        //sleep 100 milliseconds (give or take)
    }
    usrp->set_time_next_pps(uhd::time_spec_t(0.0));

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Method 2 - query the GPSDO for seconds
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Most GPSDOs can be configured to output a NMEA string over the serial port once every PPS.
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
Method 3 - internal GPSDO
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
USRPs with internal GPSDOs properly configured will automatically
configure themselves to set the VITA time to current UTC time.
See the `GPSDO Application Notes <./gpsdo.html>`_ for more details.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Method 4 - MIMO cable
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
A USRP can synchronize its time to another USRP via the MIMO cable.
Unlike the other methods, this does not use a real "pulse per second".
Rather, the USRP sends an encoded time message over the MIMO cable.
The slave device will automatically synchronize to the time on the master device.
See the `MIMO Cable Application Notes <./usrp2.html#using-the-mimo-cable>`_ for more detail.

------------------------------------------------------------------------
Synchronizing Channel Phase
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

For receive, a burst is started when the user issues a stream command. This stream command should have a time spec set:
::

    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps = samps_to_recv;
    stream_cmd.stream_now = false;
    stream_cmd.time_spec = time_to_recv;
    usrp->issue_stream_cmd(stream_cmd);

For transmit, a burst is started when the user calls send(). The metadata should have a time spec set:
::

    uhd::tx_metadata_t md;
    md.start_of_burst = true;
    md.end_of_burst = false;
    md.has_time_spec = true;
    md.time_spec = time_to_send;

    //send a single packet
    size_t num_tx_samps = tx_streamer->send(buffs, samps_to_send, md);

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Align LOs in the front-end (SBX/WBX + N-Series)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Using timed commands, multiple frontends can be tuned at a specific time.
This timed-tuning ensures that the phase offsets between VCO/PLL chains
will remain constant after each re-tune. See notes below:

* There is a random phase offset between any two frontends
* This phase offset is different for different LO frequencies
* This phase offset remains constant after retuning

  * Due to divider, WBX phase offset will be randomly +/- 180 deg after re-tune

* This phase offset will drift over time due to thermal and other characteristics
* Periodic calibration will be necessary for phase-coherent applications

Code snippet example, tuning with timed commands:
::

    //we will tune the frontends in 100ms from now
    uhd::time_spec_t cmd_time = usrp->get_time_now() + uhd::time_spec_t(0.1);

    //sets command time on all devices
    //the next commands are all timed
    usrp->set_command_time(cmd_time);

    //tune channel 0 and channel 1
    usrp->set_rx_freq(1.03e9, 0/*ch0*/);
    usrp->set_rx_freq(1.03e9, 1/*ch1*/);

    //end timed commands
    usrp->clear_command_time();

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Align LOs in the front-end (others)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
After tuning the RF front-ends,
each local oscillator may have a random phase offset due to the dividers
in the VCO/PLL chains. This offset will remain constant after the device
has been initialized, and will remain constant until the device is closed
or re-tuned. This phase offset is typically removed by the user in MIMO
applications, using a training sequence to estimate the offset. It will
be necessary to re-align the LOs after each tune command.

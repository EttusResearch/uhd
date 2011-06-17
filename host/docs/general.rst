========================================================================
UHD - General Application Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Tuning notes
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Two-stage tuning process
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
A USRP device has two stages of tuning:

* RF front-end: translates bewteen RF and IF
* DSP: translates between IF and baseband

In a typical use-case, the user specifies an overall center frequency for the signal chain.
The RF front-end will be tuned as close as possible to the center frequency,
and the DSP will account for the error in tuning between target frequency and actual frequency.
The user may also explicitly control both stages of tuning through the tune_request_t object.

Pseudo-code for tuning the receive chain:
::

    //tuning to a desired center frequency
    usrp->set_rx_freq(my_frequency_in_hz);

    --OR--

    //advanced tuning with tune_request_t
    uhd::tune_request_t tune_req;
    tune_req.target_freq = my_frequency_in_hz;
    //fill in tune request fields...
    usrp->set_rx_freq(tune_req);

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
RF front-end settling time
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
After tuning, the RF front-end will need time to settle into a usable state.
Typically, this means that the local oscillators must be given time to lock before streaming begins.
Lock time is not consistent; it varies depending upon the device and requested settings.
After tuning and before streaming, the user should
wait for the "lo_locked" sensor to become true,
or sleep for a conservative amount of time (perhaps a second).

Pseudo-code for dealing with settling time after tuning on receive:
::

    usrp->set_rx_freq(...);
    sleep(1);
    usrp->issue_stream_command(...);

    --OR--

    usrp->set_rx_freq(...);
    while (not usrp->get_rx_sensor("lo_locked").to_bool()){
        //sleep for a short time in milliseconds
    }
    usrp->issue_stream_command(...);

------------------------------------------------------------------------
Overflow/Underflow notes
------------------------------------------------------------------------
**Note:** The following overflow/underflow notes do not apply to USRP1,
which does not support the advanced features available in newer products.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Overflow notes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When receiving, the device produces samples at a constant rate.
Overflows occurs when the host does not consume data fast enough.
When UHD detects the overflow, it prints an "O" to stdout,
and pushes an inline message packet into the receive stream.

**Network-based devices**:
The host does not back-pressure the receive stream.
When the kernel's socket buffer becomes full, it will drop subsequent packets.
UHD detects the overflow as a discontinuity in the packet's sequence numbers,
and muxes an inline message packet into the receive stream.

**Other devices**:
The host back-pressures the receive stream.
Therefore, overflows always occur in the device itself.
When the device's internal buffers become full, streaming is shutoff,
and an inline message packet is sent to the host.
If the device was in continuous streaming mode,
the UHD will automatically restart streaming.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Underflow notes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When transmitting, the device consumes samples at a constant rate.
Underflow occurs when the host does not produce data fast enough.
When the UHD detects underflow, it prints an "U" to stdout,
and pushes a message packet into the async message stream.

------------------------------------------------------------------------
Threading notes
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Thread safety notes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
For the most part, UHD is thread-safe.
Please observe the following limitations:

**Fast-path thread requirements:**
There are three fast-path methods for a device: send(), recv(), and recv_async_msg().
All three methods are thread-safe and can be called from different thread contexts.
For performance, the user should call each method from a separate thread context.
These methods can also be used in a non-blocking fashion by using a timeout of zero.

**Slow-path thread requirements:**
It is safe to change multiple settings simultaneously. However,
this could leave the settings for a device in an uncertain state.
The is because changing one setting could have an impact on how a call affects other settings.
Example: setting the channel mapping affects how the antennas are set.
It is recommended to use at most one thread context for manipulating device settings.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Thread priority scheduling
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When the UHD spawns a new thread it may try to boost the thread's scheduling priority.
When setting the priority fails, the UHD prints out an error.
This error is harmless, it simply means that the thread will have a normal scheduling priority.

**Linux Notes:**

Non-privileged users need special permission to change the scheduling priority.
Add the following line to */etc/security/limits.conf*:
::

    @<my_group>    -    rtprio    99

Replace <my_group> with a group to which your user belongs.
Settings will not take effect until the user has logged in and out.

------------------------------------------------------------------------
Misc notes
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Support for dynamically loadable modules
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
For a module to be loaded at runtime, it must be:

* found in the UHD_MODULE_PATH environment variable,
* installed into the <install-path>/share/uhd/modules directory,
* or installed into /usr/share/uhd/modules directory (unix only).

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Disabling or redirecting prints to stdout
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The user can disable the UHD library from printing directly to stdout by registering a custom message handler.
The handler will intercept all messages, which can be dropped or redirected.
Only one handler can be registered at a time.
Make "register_handler" your first call into UHD:

::

    #include <uhd/utils/msg.hpp>

    void my_handler(uhd::msg::type_t type, const std::string &msg){
        //handle the message...
    }

    uhd::msg::register_handler(&my_handler);

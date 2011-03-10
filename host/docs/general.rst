========================================================================
UHD - General Application Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Misc notes
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Thread safety notes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
For the most part, UHD is thread-safe.
Please observe the following limitations:

**Fast-path thread requirements:**
It is safe to call send() and recv() simultaneously. However,
it is not safe to call recv() simultaneously from different thread contexts.
The same rule applies for recv(), send(), and recv_async_msg().
One thread context per fast-path device method at a time.

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

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Support for dynamically loadable modules
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
For a module to be loaded at runtime, it must be:

* found in the UHD_MODULE_PATH environment variable,
* installed into the <prefix>/share/uhd/modules directory,
* or installed into /usr/share/uhd/modules directory (unix only).

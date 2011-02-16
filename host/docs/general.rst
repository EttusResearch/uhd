========================================================================
UHD - General Application Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Misc notes
------------------------------------------------------------------------

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

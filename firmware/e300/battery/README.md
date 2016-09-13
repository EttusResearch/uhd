USRP E312 Firmware
==================

Welcome to the NI Ettus Research USRP E310/E312 Firmware distribution.

# Dependencies

In order to build you'll *avr-gcc*, *avr-libc*, and *srec_cat*.

# Building

The included Makefile specifies all the required flags. To build type:
```
$ make
```

# Flashing

In order to program the device with the firmware type:
```
$ make install
```

Note: The Makefile will have to be modified depending on which programmer you use.
Known good programmers include 'Atmel AVR Dragon, Atmel JTAGICEIII, Atmel AtmelICE'.

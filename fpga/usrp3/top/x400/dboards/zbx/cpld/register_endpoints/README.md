# CPLD Default values

The memory of the CPLD is configured to set all the RF components to safe
values. That means:
- LEDs off
- DSAs are set to maximum attenuation

The Python script `memory_init_files/gen_defaults.py` creates the .hex files
that get read into the CPLD bitfile. This is run as part of the Makefile
process, and the script does not have to be executed manually.

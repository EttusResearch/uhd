# EEPROM Utilities for embedded USRP devices

## N3x0 and E320

The USRP N3x0 and E320 series share the same EEPROM formats. They are initialized,
configured, and queried using the utilities in this folder (mainly, eeprom-dump
and eeprom-id to read the EEPROM, eeprom-init to configure it, eeprom-blank to
erase it, and eeprom-set-flags to configure the MCU flags).

The N3x0 series, which has additional EEPROMs on the daughterboard, has additional
tools for those (db-dump, db-id, db-init).

The structure of the data is fixed. A good overview of how data is stored can
be looked up either in `usrp_mpm/eeprom.py` or the structs in `eeprom.h` in this
directory.

## X410

The USRP X410 uses a different EEPROM data format (tag/length/value, TLV). It is
more flexible than the format used on the previous devices. The tools to operate
on the EEPROMs for those devices are stored under `tlv_eeprom`.

## E31x

The USRP E31x series is not supported by these tools.

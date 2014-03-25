#!/bin/bash

utils/usrp_burn_mb_eeprom --key=serial --val="x310rC12"
utils/usrp_burn_mb_eeprom --key=name --val="dillen"

utils/usrp_burn_mb_eeprom --key=product --val="30410"
utils/usrp_burn_mb_eeprom --key=revision --val="3"

mac0=$(python -c "import random; mac = [0x00, 0x16, 0x3e, random.randint(0x00, 0x7f),random.randint(0x00, 0xff),random.randint(0x00, 0xff)]; print ':'.join(map(lambda x: '%02x' % x, mac))")
mac1=$(python -c "import random; mac = [0x00, 0x16, 0x3e, random.randint(0x00, 0x7f),random.randint(0x00, 0xff),random.randint(0x00, 0xff)]; print ':'.join(map(lambda x: '%02x' % x, mac))")

utils/usrp_burn_mb_eeprom --key=mac-addr0 --val="${mac0}"
utils/usrp_burn_mb_eeprom --key=mac-addr1 --val="${mac1}"

utils/usrp_burn_mb_eeprom --key=gateway --val="192.168.10.1"

utils/usrp_burn_mb_eeprom --key=subnet0 --val="255.255.255.0"
utils/usrp_burn_mb_eeprom --key=subnet1 --val="255.255.255.0"
utils/usrp_burn_mb_eeprom --key=subnet2 --val="255.255.255.0"
utils/usrp_burn_mb_eeprom --key=subnet3 --val="255.255.255.0"

utils/usrp_burn_mb_eeprom --key=ip-addr0 --val="192.168.10.2"
utils/usrp_burn_mb_eeprom --key=ip-addr1 --val="192.168.10.3"
utils/usrp_burn_mb_eeprom --key=ip-addr2 --val="192.168.10.2"
utils/usrp_burn_mb_eeprom --key=ip-addr3 --val="192.168.10.3"

#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#
#   Timing constraints for the x440's motherboard CPLD.
#


#####################################################################
# DB specific LED constraints
#####################################################################

# LED signals
set led_outputs [get_ports {QSFP0_LED_ACTIVE[*] QSFP0_LED_LINK[*] \
  QSFP1_LED_ACTIVE[*] QSFP1_LED_LINK[*] CH*_RX2_LED[*] CH*_TX_LED[*] CH*_RX_LED[*] } ]
set_min_delay -to $led_outputs 0
set_max_delay -to $led_outputs $prc_clock_period


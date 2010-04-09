#!/usr/bin/python

import os

# Memory Map
misc_base = 0
uart_base = 1
spi_base = 2
i2c_base = 3
gpio_base = 4 * 128
settings_base = 5

# GPIO offset
gpio_pins = 0
gpio_ddr = 4
gpio_ctrl_lo = 8
gpio_ctrl_hi = 12

def set_reg(reg, val):
    os.system("./usrp1-e-ctl w %d 1 %d" % (reg,val))

def get_reg(reg):
    fin,fout = os.popen4("./usrp1-e-ctl r %d 1" % (reg,))
    print fout.read()

# Set DDRs to output
set_reg(gpio_base+gpio_ddr, 0xFFFF)
set_reg(gpio_base+gpio_ddr+2, 0xFFFF)

# Set CTRL to Debug #0 ( A is for debug 0, F is for debug 1 )
set_reg(gpio_base+gpio_ctrl_lo, 0xAAAA)
set_reg(gpio_base+gpio_ctrl_lo+2, 0xAAAA)
set_reg(gpio_base+gpio_ctrl_hi, 0xAAAA)
set_reg(gpio_base+gpio_ctrl_hi+2, 0xAAAA)


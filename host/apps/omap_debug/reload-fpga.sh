#!/bin/sh

rmmod usrp_e
fpga-downloader /home/root/u1e.bin
modprobe usrp_e
usrp-e-debug-pins 1


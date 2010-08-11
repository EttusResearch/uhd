CFLAGS=-Wall -I../../lib/usrp/usrp_e/ -march=armv7-a -mtune=cortex-a8 -mfpu=neon -O3
CXXFLAGS=-Wall -I../../lib/usrp/usrp_e/ -march=armv7-a -mtune=cortex-a8 -mfpu=neon -O3

all : usrp-e-spi usrp-e-i2c usrp-e-loopback usrp-e-mm-loopback usrp-e-uart usrp-e-led usrp-e-ctl usrp-e-button usrp-e-uart-rx fpga-downloader usrp-e-gpio usrp-e-debug-pins usrp-e-random-loopback usrp-e-timed usrp-e-lb-test usrp-e-crc-rw clkgen-config

usrp-e-spi : usrp-e-spi.c

usrp-e-i2c : usrp-e-i2c.c

usrp-e-loopback : usrp-e-loopback.c
	gcc -o $@ $< -lpthread ${CFLAGS}

usrp-e-mm-loopback : usrp-e-mm-loopback.c
	gcc -o $@ $< -lpthread ${CFLAGS}

usrp-e-timed : usrp-e-timed.c
	gcc -o $@ $< -lpthread ${CFLAGS}

usrp-e-random-loopback : usrp-e-random-loopback.c
	gcc -o $@ $< -lpthread ${CFLAGS}

usrp-e-crc-rw : usrp-e-crc-rw.c
	gcc -o $@ $< -lpthread ${CFLAGS}

usrp-e-uart : usrp-e-uart.c

usrp-e-uart-rx : usrp-e-uart-rx.c

usrp-e-led : usrp-e-led.c

usrp-e-ctl : usrp-e-ctl.c

usrp-e-button : usrp-e-button.c

fpga-downloader : fpga-downloader.cc

clkgen-config : clkgen-config.cc

usrp-e-gpio : usrp-e-gpio.c

usrp-e-lb-test : usrp-e-lb-test.c

usrp-e-debug-pins : usrp-e-debug-pins.c
clean :
	rm -f usrp-e-spi
	rm -f usrp-e-i2c
	rm -f usrp-e-loopback
	rm -f usrp-e-mm-loopback
	rm -f usrp-e-timed
	rm -f usrp-e-rw-random
	rm -f usrp-e-uart
	rm -f usrp-e-uart-rx
	rm -f usrp-e-led
	rm -f usrp-e-ctl
	rm -f usrp-e-button
	rm -f fpga-downloader
	rm -f usrp-e-gpio
	rm -f usrp-e-debug-pins
	rm -f usrp-e-lb-test
	rm -f usrp-e-crc-rw
	rm -f clkgen-config

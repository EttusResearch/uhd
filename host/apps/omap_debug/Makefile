CFLAGS=-Wall -I../../lib/usrp/usrp_e/ -march=armv7-a -mtune=cortex-a8 -mfpu=neon -O3
CXXFLAGS=-Wall -I../../lib/usrp/usrp_e/ -march=armv7-a -mtune=cortex-a8 -mfpu=neon -O3

all : usrp-e-spi usrp-e-i2c usrp-e-uart usrp-e-led usrp-e-ctl usrp-e-button usrp-e-uart-rx usrp-e-gpio usrp-e-debug-pins

usrp-e-spi : usrp-e-spi.c

usrp-e-i2c : usrp-e-i2c.c

usrp-e-uart : usrp-e-uart.c

usrp-e-uart-rx : usrp-e-uart-rx.c

usrp-e-led : usrp-e-led.c

usrp-e-ctl : usrp-e-ctl.c

usrp-e-button : usrp-e-button.c

usrp-e-gpio : usrp-e-gpio.c

usrp-e-debug-pins : usrp-e-debug-pins.c
clean :
	rm -f usrp-e-spi
	rm -f usrp-e-i2c
	rm -f usrp-e-uart
	rm -f usrp-e-uart-rx
	rm -f usrp-e-led
	rm -f usrp-e-ctl
	rm -f usrp-e-button
	rm -f usrp-e-gpio
	rm -f usrp-e-debug-pins
	rm -f usrp-e-lb-test

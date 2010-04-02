CFLAGS=-Wall -I../../lib/usrp/usrp_e/

all : usrp-e-spi usrp-e-i2c usrp-e-rw usrp-e-uart usrp-e-led usrp-e-ctl usrp-e-button

usrp-e-spi : usrp-e-spi.c

usrp-e-i2c : usrp-e-i2c.c

usrp-e-rw : usrp-e-rw.c
	gcc -o $@ $< -lpthread

usrp-e-uart : usrp-e-uart.c

usrp-e-led : usrp-e-led.c

usrp-e-ctl : usrp-e-ctl.c

usrp-e-button : usrp-e-button.c

clean :
	rm -f usrp-e-spi
	rm -f usrp-e-i2c
	rm -f usrp-e-rw
	rm -f usrp-e-uart
	rm -f usrp-e-led
	rm -f usrp-e-ctl
	rm -f usrp-e-button

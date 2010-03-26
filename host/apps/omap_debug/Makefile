CFLAGS=-Wall

all : usrp-e-spi usrp-e-i2c usrp-e-rw usrp-e-uart

usrp-e-spi : usrp-e-spi.c

usrp-e-i2c : usrp-e-i2c.c

usrp-e-rw : usrp-e-rw.c
	gcc -o $@ $< -lpthread

usrp-e-uart : usrp-e-uart.c

clean :
	rm -f usrp-e-spi
	rm -f usrp-e-i2c
	rm -f usrp-e-rw
	rm -f usrp-e-uart


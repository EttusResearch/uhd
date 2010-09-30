/* -*- c++ -*- */
/*
 * Copyright 2003,2004,2008,2009 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
*/

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstdlib>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/spi/spidev.h>

/*
 * Configuration connections
 *
 * CCK    - MCSPI1_CLK
 * DIN    - MCSPI1_MOSI
 * PROG_B - GPIO_175     - output (change mux)
 * DONE   - GPIO_173     - input  (change mux)
 * INIT_B - GPIO_114     - input  (change mux)
 *
*/

const unsigned int PROG_B = 175;
const unsigned int DONE   = 173;
const unsigned int INIT_B = 114;

static std::string bit_file = "safe_u1e.bin";

const int BUF_SIZE = 4096;

enum gpio_direction {IN, OUT};

class gpio {
	public:

	gpio(unsigned int gpio_num, gpio_direction pin_direction);

	bool get_value();
	void set_value(bool state);

	private:

	std::stringstream base_path;
	std::fstream value_file;	
};

class spidev {
	public:

	spidev(std::string dev_name);
	~spidev();

	void send(char *wbuf, char *rbuf, unsigned int nbytes);

	private:

	int fd;

};

gpio::gpio(unsigned int gpio_num, gpio_direction pin_direction)
{
	std::fstream export_file;

	export_file.open("/sys/class/gpio/export", std::ios::out);
	if (!export_file.is_open())  ///\todo Poor error handling
		std::cout << "Failed to open gpio export file." << std::endl;

	export_file << gpio_num << std::endl;

	base_path << "/sys/class/gpio/gpio" << gpio_num << std::flush;

	std::fstream direction_file;
	std::string direction_file_name;

	if (gpio_num != 114) {
		direction_file_name = base_path.str() + "/direction";

		direction_file.open(direction_file_name.c_str()); 
		if (!direction_file.is_open())
			std::cout << "Failed to open direction file." << std::endl;
		if (pin_direction == OUT)
			direction_file << "out" << std::endl;
		else
			direction_file << "in" << std::endl;
	}

	std::string value_file_name;

	value_file_name = base_path.str() + "/value";

	value_file.open(value_file_name.c_str(), std::ios_base::in | std::ios_base::out);
	if (!value_file.is_open())
		std::cout << "Failed to open value file." << std::endl;
}

bool gpio::get_value()
{

	std::string val;

	std::getline(value_file, val);
	value_file.seekg(0);

	if (val == "0")
		return false;
	else if (val == "1")
		return true;
	else
		std::cout << "Data read from value file|" << val << "|" << std::endl;

	return false;
}

void gpio::set_value(bool state)
{

	if (state)
		value_file << "1" << std::endl;
	else
		value_file << "0" << std::endl;
}

static void prepare_fpga_for_configuration(gpio &prog, gpio &init)
{

	prog.set_value(true);
	prog.set_value(false);
	prog.set_value(true);

#if 0
	bool ready_to_program(false);
	unsigned int count(0);
	do {
		ready_to_program = init.get_value();
		count++;

		sleep(1);
	} while (count < 10 && !ready_to_program);

	if (count == 10) {
		std::cout << "FPGA not ready for programming." << std::endl;
		exit(-1);
	}
#endif
}

spidev::spidev(std::string fname)
{
	int ret;
	int mode = 0;
	int speed = 12000000;
	int bits = 8;

	fd = open(fname.c_str(), O_RDWR);

	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
}
	

spidev::~spidev()
{
	close(fd);
}

void spidev::send(char *buf, char *rbuf, unsigned int nbytes)
{
	int ret;

	struct spi_ioc_transfer tr;
	tr.tx_buf = (unsigned long) buf;
	tr.rx_buf = (unsigned long) rbuf;
	tr.len = nbytes;
	tr.delay_usecs = 0;
	tr.speed_hz = 48000000;
	tr.bits_per_word = 8;

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);	

}

static void send_file_to_fpga(std::string &file_name, gpio &error, gpio &done)
{
	std::ifstream bitstream;

	std::cout << "File name - " << file_name.c_str() << std::endl;

	bitstream.open(file_name.c_str(), std::ios::binary);
	if (!bitstream.is_open())
		std::cout << "File " << file_name << " not opened succesfully." << std::endl;

	spidev spi("/dev/spidev1.0");
	char buf[BUF_SIZE];
	char rbuf[BUF_SIZE];

	do {
		bitstream.read(buf, BUF_SIZE);
		spi.send(buf, rbuf, bitstream.gcount());

		if (error.get_value())
			std::cout << "INIT_B went high, error occured." << std::endl;

		if (!done.get_value())
			std::cout << "Configuration complete." << std::endl;

	} while (bitstream.gcount() == BUF_SIZE);
}

int main(int argc, char *argv[])
{

	gpio gpio_prog_b(PROG_B, OUT);
	gpio gpio_init_b(INIT_B, IN);
	gpio gpio_done  (DONE,   IN);

	if (argc == 2)
		bit_file = argv[1];

	std::cout << "FPGA config file: " << bit_file << std::endl;

	prepare_fpga_for_configuration(gpio_prog_b, gpio_init_b);

	std::cout << "Done = " << gpio_done.get_value() << std::endl;

	send_file_to_fpga(bit_file, gpio_init_b, gpio_done);
}


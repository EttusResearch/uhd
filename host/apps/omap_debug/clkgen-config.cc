/* -*- c++ -*- */
/*
 * Copyright 2003,2004,2008,2009 Free Software Foundation, Inc.
 *
 * This file is part of UHD
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


// Programming data for clock gen chip
static const unsigned int config_data[] = {
	0x000024,
	0x023201,
	0x000081,
	0x000400,
	0x00104c,
	0x001101,
	0x001200,
	0x001300,
	0x001414,
	0x001500,
	0x001604,
	0x001704,
	0x001807,
	0x001900,
	0x001a32,
	0x001b12,
	0x001c44,
	0x001d00,
	0x001e00,
	0x00f062,
	0x00f162,
	0x00f262,
	0x00f362,
	0x00f462,
	0x00f562,
	0x00f662,
	0x00f762,
	0x00f862,
	0x00f962,
	0x00fa62,
	0x00fb62,
	0x00fc00,
	0x00fd00,
	0x019021,
	0x019100,
	0x019200,
	0x019333,
	0x019400,
	0x019500,
	0x019611,
	0x019700,
	0x019800,
	0x019900,
	0x019a00,
	0x019b00,
	0x01e003,
	0x01e102,
	0x023000,
	0x023201,
	0x0b0201,
	0x0b0300,
	0x001fff,
	0x0a0000,
	0x0a0100,
	0x0a0200,
	0x0a0302,
	0x0a0400,
	0x0a0504,
	0x0a060e,
	0x0a0700,
	0x0a0810,
	0x0a090e,
	0x0a0a00,
	0x0a0bf0,
	0x0a0c0b,
	0x0a0d01,
	0x0a0e90,
	0x0a0f01,
	0x0a1001,
	0x0a11e0,
	0x0a1201,
	0x0a1302,
	0x0a1430,
	0x0a1580,
	0x0a16ff,
	0x023201,
	0x0b0301,
	0x023201,
};


const unsigned int CLKGEN_SELECT = 145;


enum gpio_direction {IN, OUT};

class gpio {
	public:

	gpio(unsigned int gpio_num, gpio_direction pin_direction, bool close_action);
	~gpio();

	bool get_value();
	void set_value(bool state);

	private:

	unsigned int gpio_num;

	std::stringstream base_path;
	std::fstream value_file;
	std::fstream direction_file;
	bool close_action; // True set to input and release, false do nothing
};

class spidev {
	public:

	spidev(std::string dev_name);
	~spidev();

	void send(char *wbuf, char *rbuf, unsigned int nbytes);

	private:

	int fd;

};

gpio::gpio(unsigned int _gpio_num, gpio_direction pin_direction, bool close_action)
{
	std::fstream export_file;

	gpio_num = _gpio_num;

	export_file.open("/sys/class/gpio/export", std::ios::out);
	if (!export_file.is_open())  ///\todo Poor error handling
		std::cout << "Failed to open gpio export file." << std::endl;

	export_file << gpio_num << std::endl;

	base_path << "/sys/class/gpio/gpio" << gpio_num << std::flush;

	std::string direction_file_name;

	direction_file_name = base_path.str() + "/direction";

	direction_file.open(direction_file_name.c_str()); 
	if (!direction_file.is_open())
		std::cout << "Failed to open direction file." << std::endl;
	if (pin_direction == OUT)
		direction_file << "out" << std::endl;
	else
		direction_file << "in" << std::endl;

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

gpio::~gpio()
{
	if (close_action) {
		std::fstream unexport_file;

		direction_file << "in" << std::endl;

		unexport_file.open("/sys/class/gpio/unexport", std::ios::out);
		if (!unexport_file.is_open())  ///\todo Poor error handling
			std::cout << "Failed to open gpio export file." << std::endl;

		unexport_file << gpio_num << std::endl;
		
	 }

}

spidev::spidev(std::string fname)
{
	int ret;
	int mode = 0;
	int speed = 12000000;
	int bits = 32;

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
	tr.speed_hz = 12000000;
	tr.bits_per_word = 32;

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);	

}

static void send_config_to_clkgen(gpio &chip_select, const unsigned int data[], unsigned int data_size)
{
	spidev spi("/dev/spidev1.0");
	unsigned int rbuf;

	for (unsigned int i = 0; i < data_size; i++) {

		chip_select.set_value(1);
		spi.send((char *)&data[i], (char *)&rbuf, 4);
		chip_select.set_value(0);

	};
}

int main(int argc, char *argv[])
{

	gpio clkgen_select(CLKGEN_SELECT, OUT, true);

	send_config_to_clkgen(clkgen_select, config_data, sizeof(config_data)/sizeof(unsigned int));
}


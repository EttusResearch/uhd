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
#include "ad9522_regs.hpp"

#include <linux/spi/spidev.h>


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
	int speed = 12000;
	int bits = 24;

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
	tr.bits_per_word = 24;

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);	

}

static void spi_write_word(spidev spi, gpio &chip_select, const unsigned int addr, const unsigned char data) {
	unsigned char out_data[3], in_data[3];
    unsigned char rw_w1_w0 = 0; //write one byte
    out_data[0] = (rw_w1_w0 << 5) | (addr >> 8);
    out_data[1] = addr & 0xff;
    out_data[2] = data;
	
	chip_select.set_value(0);
	spi.send((char *)out_data, (char *)in_data, 4);
	chip_select.set_value(1);
}

static void send_config_to_clkgen(gpio &chip_select)
{
	spidev spi("/dev/spidev1.0");

	//do a soft reset
    spi_write_word(spi, chip_select, 0x000, 1 << 5 | 1 << 2);
    spi_write_word(spi, chip_select, 0x232, 0x1);

    // init some registers;
    ad9522_regs_t ad9522_regs;
    ad9522_regs.sdo_active = ad9522_regs_t::SDO_ACTIVE_SDO_SDIO; //use sdo and sdi
    ad9522_regs.mirror = 1; //mirror sdo active
    ad9522_regs.io_update = 1; //latch the registers
    ad9522_regs.status_pin_control = 0x1; //n divider
    ad9522_regs.ld_pin_control = 0x32; //show ref2
    ad9522_regs.refmon_pin_control = 0x12; //show ref2
    ad9522_regs.enb_stat_eeprom_at_stat_pin = 0; //use status pin as debug

    ad9522_regs.enable_ref2 = 0x1;
    ad9522_regs.enable_ref1 = 0x0;
    ad9522_regs.select_ref = ad9522_regs_t::SELECT_REF_REF2;

    ad9522_regs.set_r_counter(1);
    ad9522_regs.a_counter = 0;
    ad9522_regs.set_b_counter(20);
    ad9522_regs.prescaler_p = ad9522_regs_t::PRESCALER_P_DIV8_9;

    ad9522_regs.pll_power_down = ad9522_regs_t::PLL_POWER_DOWN_NORMAL; //normal mode
    ad9522_regs.cp_current = ad9522_regs_t::CP_CURRENT_1_2MA;

    ad9522_regs.vco_calibration_now = 1; //calibrate it!
    ad9522_regs.vco_divider = ad9522_regs_t::VCO_DIVIDER_DIV5;
    ad9522_regs.select_vco_or_clock = ad9522_regs_t::SELECT_VCO_OR_CLOCK_VCO;

    ad9522_regs.out0_format = ad9522_regs_t::OUT0_FORMAT_LVDS;
    ad9522_regs.divider0_low_cycles = 2; //3 low
    ad9522_regs.divider0_high_cycles = 1; //2 high
    ad9522_regs.divider1_low_cycles = 2; //3 low
    ad9522_regs.divider1_high_cycles = 1; //2 high

    ad9522_regs.enable_eeprom_write = 1;

    //write the registers
    int reg_list[] = 			{0, 4, 16, 17, 18, 19, 20, 21, 22, 23, 24,
                                 25, 26, 27, 28, 29, 30, 240, 241, 242, 243,
                                 244, 245, 246, 247, 248, 249, 250, 251, 252,
                                 253, 400, 401, 402, 403, 404, 405, 406, 407,
                                 408, 409, 410, 411, 480, 481, 560, 562, 2818,
                                 2819};
                                 
    for(int i=0; i<49; i++) { //blame std::vector for this (no static initialization bs)
		spi_write_word(spi, chip_select, reg_list[i], ad9522_regs.get_reg(reg_list[i]));
	}

    sleep(1);

    if (1){//write settings to eeprom

        //load the register buffs
        spi_write_word(spi, chip_select, 0xa00, 0x0);
        spi_write_word(spi, chip_select, 0xa01, 0x0);
        spi_write_word(spi, chip_select, 0xa02, 0x0);
        spi_write_word(spi, chip_select, 0xa03, 0x2);
        spi_write_word(spi, chip_select, 0xa04, 0x0);
        spi_write_word(spi, chip_select, 0xa05, 0x4);
        spi_write_word(spi, chip_select, 0xa06, 0xe);
        spi_write_word(spi, chip_select, 0xa07, 0x0);
        spi_write_word(spi, chip_select, 0xa08, 0x10);
        spi_write_word(spi, chip_select, 0xa09, 0xe);
        spi_write_word(spi, chip_select, 0xa0a, 0x0);
        spi_write_word(spi, chip_select, 0xa0b, 0xf0);
        spi_write_word(spi, chip_select, 0xa0c, 0xb);
        spi_write_word(spi, chip_select, 0xa0d, 0x1);
        spi_write_word(spi, chip_select, 0xa0e, 0x90);
        spi_write_word(spi, chip_select, 0xa0f, 0x1);
        spi_write_word(spi, chip_select, 0xa10, 0x1);
        spi_write_word(spi, chip_select, 0xa11, 0xe0);
        spi_write_word(spi, chip_select, 0xa12, 0x1);
        spi_write_word(spi, chip_select, 0xa13, 0x2);
        spi_write_word(spi, chip_select, 0xa14, 0x30);
        spi_write_word(spi, chip_select, 0xa15, 0x80);
        spi_write_word(spi, chip_select, 0xa16, 0xff);

        spi_write_word(spi, chip_select, 0x232, 0x01); //latch
        sleep(1);
        ////////////////////////////////////////////////////////////////
        
        ad9522_regs.reg2eeprom = 1;
        //write to eeprom
        spi_write_word(spi, chip_select, 0xB03, ad9522_regs.get_reg(0xB03));
        //io update
        spi_write_word(spi, chip_select, 0x232, ad9522_regs.get_reg(0x232)); //latch
        sleep(1);
    }

}

int main(int argc, char *argv[])
{
	gpio clkgen_select(CLKGEN_SELECT, OUT, true);

	send_config_to_clkgen(clkgen_select);
}


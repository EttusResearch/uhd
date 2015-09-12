//
// Copyright 2010-2011,2014-2015 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <uhd/config.hpp>
#ifdef UHD_DLL_EXPORTS
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <uhd/exception.hpp>
#include <uhd/device.hpp>
#include <uhd/image_loader.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/static.hpp>
#include "e100_impl.hpp"
#else //special case when this file is externally included
#include <stdexcept>
#include <iostream>
#define UHD_MSG(type) std::cout
namespace uhd{
    typedef std::runtime_error os_error;
    typedef std::runtime_error io_error;
}
#endif

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

namespace usrp_e_fpga_downloader_utility{

const unsigned int PROG_B = 175;
const unsigned int DONE   = 173;
const unsigned int INIT_B = 114;

//static std::string bit_file = "safe_u1e.bin";

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
	if (not export_file.is_open()) throw uhd::os_error(
		"Failed to open gpio export file."
	);

	export_file << gpio_num << std::endl;

	base_path << "/sys/class/gpio/gpio" << gpio_num << std::flush;

	std::fstream direction_file;
	std::string direction_file_name;

	if (gpio_num != 114) {
		direction_file_name = base_path.str() + "/direction";

		direction_file.open(direction_file_name.c_str());
		if (!direction_file.is_open()) throw uhd::os_error(
			"Failed to open direction file."
		);
		if (pin_direction == OUT)
			direction_file << "out" << std::endl;
		else
			direction_file << "in" << std::endl;
	}

	std::string value_file_name;

	value_file_name = base_path.str() + "/value";

	value_file.open(value_file_name.c_str(), std::ios_base::in | std::ios_base::out);
	if (!value_file.is_open()) throw uhd::os_error(
		"Failed to open value file."
	);
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
		throw uhd::os_error("Data read from value file|" + val + "|");

	return false;
}

void gpio::set_value(bool state)
{

	if (state)
		value_file << "1" << std::endl;
	else
		value_file << "0" << std::endl;
}

static void prepare_fpga_for_configuration(gpio &prog, gpio &)//init)
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
		throw uhd::os_error("FPGA not ready for programming.");
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

static void send_file_to_fpga(const std::string &file_name, gpio &error, gpio &done)
{
	std::ifstream bitstream;

	bitstream.open(file_name.c_str(), std::ios::binary);
	if (!bitstream.is_open()) throw uhd::os_error(
		"Could not open the file: " + file_name
	);

	spidev spi("/dev/spidev1.0");
	char buf[BUF_SIZE];
	char rbuf[BUF_SIZE];

	do {
		bitstream.read(buf, BUF_SIZE);
		spi.send(buf, rbuf, bitstream.gcount());

		if (error.get_value())
			throw uhd::os_error("INIT_B went high, error occured.");

		if (!done.get_value())
			UHD_MSG(status) << "Configuration complete." << std::endl;

	} while (bitstream.gcount() == BUF_SIZE);
}

}//namespace usrp_e_fpga_downloader_utility

void e100_load_fpga(const std::string &bin_file){
	using namespace usrp_e_fpga_downloader_utility;

	gpio gpio_prog_b(PROG_B, OUT);
	gpio gpio_init_b(INIT_B, IN);
	gpio gpio_done  (DONE,   IN);

	UHD_MSG(status) << "Loading FPGA image: " << bin_file << "... " << std::flush;

//	if(std::system("/sbin/rmmod usrp_e") != 0){
//		UHD_MSG(warning) << "USRP-E100 FPGA downloader: could not unload usrp_e module" << std::endl;
//	}

	prepare_fpga_for_configuration(gpio_prog_b, gpio_init_b);

	UHD_MSG(status) << "done = " << gpio_done.get_value() << std::endl;

	send_file_to_fpga(bin_file, gpio_init_b, gpio_done);

//	if(std::system("/sbin/modprobe usrp_e") != 0){
//		UHD_MSG(warning) << "USRP-E100 FPGA downloader: could not load usrp_e module" << std::endl;
//	}

}

#ifdef UHD_DLL_EXPORTS
namespace fs = boost::filesystem;

static bool e100_image_loader(const uhd::image_loader::image_loader_args_t &image_loader_args){
    // Make sure this is an E1x0
    uhd::device_addrs_t devs = e100_find(uhd::device_addr_t());
    if(devs.size() == 0 or !image_loader_args.load_fpga) return false;

    std::string fpga_filename;
    if(image_loader_args.fpga_path == ""){
        fpga_filename = uhd::find_image_path(get_default_e1x0_fpga_image(devs[0]));
    }
    else{
        if(not fs::exists(image_loader_args.fpga_path)){
            throw uhd::runtime_error(str(boost::format("The path \"%s\" does not exist.")
                                         % image_loader_args.fpga_path));
        }
        else fpga_filename = image_loader_args.fpga_path;
    }

    e100_load_fpga(fpga_filename);
    return true;
}

UHD_STATIC_BLOCK(register_e100_image_loader){
    std::string recovery_instructions = "The default FPGA image will be loaded the next time "
                                        "UHD uses this device.";

    uhd::image_loader::register_image_loader("e100", e100_image_loader, recovery_instructions);
}
#endif /* UHD_DLL_EXPORTS */

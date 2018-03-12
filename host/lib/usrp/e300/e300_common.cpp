//
// Copyright 2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include <uhd/image_loader.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/static.hpp>

#include "e300_impl.hpp"
#include "e300_fifo_config.hpp"
#include "e300_fifo_config.hpp"

#include "e300_common.hpp"

#include <boost/filesystem.hpp>
#include <fstream>
#include <string>

#ifdef E300_NATIVE
namespace uhd { namespace usrp { namespace e300 {

namespace common {

void load_fpga_image(const std::string &path)
{
    if (not boost::filesystem::exists("/dev/xdevcfg"))
        ::system("mknod /dev/xdevcfg c 259 0");

    UHD_LOGGER_INFO("E300") << "Loading FPGA image: " << path << "...";

    std::ifstream fpga_file(path.c_str(), std::ios_base::binary);
    UHD_ASSERT_THROW(fpga_file.good());

    std::FILE *wfile;
    wfile = std::fopen("/dev/xdevcfg", "wb");
    UHD_ASSERT_THROW(!(wfile == NULL));

    char buff[16384]; // devcfg driver can't handle huge writes
    do {
        fpga_file.read(buff, sizeof(buff));
        std::fwrite(buff, 1, size_t(fpga_file.gcount()), wfile);
    } while (fpga_file);

    fpga_file.close();
    std::fclose(wfile);

    UHD_LOGGER_INFO("E300") << "FPGA image loaded";
}

static bool e300_image_loader(const image_loader::image_loader_args_t &image_loader_args) {
    // Make sure this is an E3x0 and we don't want to use anything connected
    uhd::device_addrs_t devs = e300_find(image_loader_args.args);
    if(devs.size() == 0 or !image_loader_args.load_fpga) return false;

    std::string fpga_filename, idle_image; // idle_image never used, just needed for function
    if(image_loader_args.fpga_path == "") {
        get_e3x0_fpga_images(devs[0], fpga_filename, idle_image);
    }
    else {
        if(not boost::filesystem::exists(image_loader_args.fpga_path)) {
            throw uhd::runtime_error(str(boost::format("The path \"%s\" does not exist.")
                                         % image_loader_args.fpga_path));
        }
        else fpga_filename = image_loader_args.fpga_path;
    }

    load_fpga_image(fpga_filename);
    return true;
}

UHD_STATIC_BLOCK(register_e300_image_loader) {
    std::string recovery_instructions = "The default FPGA image will be loaded the next "
                                        "time UHD uses this device.";

    image_loader::register_image_loader("e3x0", e300_image_loader, recovery_instructions);
}

}

}}}

#else
namespace uhd { namespace usrp { namespace e300 {

namespace common {

void load_fpga_image(const std::string&)
{
    throw uhd::assertion_error("load_fpga_image() !E300_NATIVE");
}

}

}}}
#endif

//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "adf5355_regs.hpp"
#include "adf5356_regs.hpp"
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/common/adf535x.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <boost/test/unit_test.hpp>
#include <map>


template <typename adf535x_regs_t>
class adf535x_mem
{
public:
    adf535x_mem()
    {
        // Copy chip defaults into mem
        for (uint8_t addr = 0; addr < 13; addr++) {
            mem[addr] = regs.get_reg(addr);
        }
    }

    void poke32(const uint8_t addr, const uint32_t data)
    {
        UHD_LOG_INFO("ADF535x",
            "W[" << boost::format("%02d") % static_cast<int>(addr) << "] => "
                 << boost::format("%08X") % data);
        mem[addr] = data;
    }

    adf535x_regs_t regs;
    std::map<uint8_t, uint16_t> mem;
};


BOOST_AUTO_TEST_CASE(adf5355_init_test)
{
    auto mem = adf535x_mem<adf5355_regs_t>{};
    UHD_LOG_INFO("ADF535x", "Initializing ADF5355...");
    auto lo = adf535x_iface::make_adf5355(
        [&](const std::vector<uint32_t> regs) {
            for (auto& reg : regs) {
                mem.poke32(uhd::narrow_cast<uint8_t>(reg & 0xF), reg);
            }
        },
        [&](const uint32_t wait_time_us) {
            std::cout << "Waiting " << wait_time_us << "us..." << std::endl;
        });
    lo->commit();
    UHD_LOG_INFO("ADF535x", "De-initializing ADF5355...");
}

BOOST_AUTO_TEST_CASE(adf5356_init_test)
{
    auto mem = adf535x_mem<adf5356_regs_t>{};
    UHD_LOG_INFO("ADF535x", "Initializing ADF5356...");
    auto lo = adf535x_iface::make_adf5356(
        [&](const std::vector<uint32_t> regs) {
            for (auto& reg : regs) {
                mem.poke32(uhd::narrow_cast<uint8_t>(reg & 0xF), reg);
            }
        },
        [&](const uint32_t wait_time_us) {
            std::cout << "Waiting " << wait_time_us << "us..." << std::endl;
        });
    lo->commit();
    UHD_LOG_INFO("ADF535x", "De-initializing ADF5356...");
}

BOOST_AUTO_TEST_CASE(adf5356_test_freqs)
{
    auto mem = adf535x_mem<adf5356_regs_t>{};
    auto lo  = adf535x_iface::make_adf5356(
        [&](const std::vector<uint32_t> regs) {
            for (auto& reg : regs) {
                mem.poke32(uhd::narrow_cast<uint8_t>(reg & 0xF), reg);
            }
        },
        [&](const uint32_t wait_time_us) {
            std::cout << "Waiting " << wait_time_us << "us..." << std::endl;
        });
    lo->commit();
    // Use TwinRX RevC settings here (LO1 is ADF5355 for lower revs, and ADF5356
    // for RevC and beyond).
    lo->set_pfd_freq(12.5e6);
    lo->set_output_power(adf535x_iface::OUTPUT_POWER_5DBM);
    lo->set_reference_freq(100e6);
    lo->set_muxout_mode(adf535x_iface::MUXOUT_DLD);

    for (double f = 2.0e9; f < 6.8e9; f += 10e6) {
        lo->set_frequency(f, 2, false);
    }
}
